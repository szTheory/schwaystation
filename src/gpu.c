/* GPU register, command-packet and VRAM-transfer state. */
#include <string.h>

#include "internal.h"
#include "state.h"

static const char name_vram[] = "gpu.vram";
static const char name_fifo[] = "gpu.fifo";
static const char name_fifo_count[] = "gpu.fifo_count";
static const char name_packet_words[] = "gpu.packet_words";
static const char name_phase[] = "gpu.phase";
static const char name_owed_words[] = "gpu.owed_words";
static const char name_transfer_x[] = "gpu.transfer_x";
static const char name_transfer_y[] = "gpu.transfer_y";
static const char name_transfer_width[] = "gpu.transfer_width";
static const char name_transfer_height[] = "gpu.transfer_height";
static const char name_transfer_index[] = "gpu.transfer_index";
static const char name_polyline_gouraud[] = "gpu.polyline_gouraud";
static const char name_polyline_color[] = "gpu.polyline_color";
static const char name_polyline_next_color[] = "gpu.polyline_next_color";
static const char name_polyline_x[] = "gpu.polyline_x";
static const char name_polyline_y[] = "gpu.polyline_y";
static const char name_polyline_has_vertex[] = "gpu.polyline_has_vertex";
static const char name_polyline_semi[] = "gpu.polyline_semi";
static const char name_draw_mode[] = "gpu.draw_mode";
static const char name_texture_window[] = "gpu.texture_window";
static const char name_draw_area_tl[] = "gpu.draw_area_tl";
static const char name_draw_area_br[] = "gpu.draw_area_br";
static const char name_draw_offset[] = "gpu.draw_offset";
static const char name_mask[] = "gpu.mask";
static const char name_display_disabled[] = "gpu.display_disabled";
static const char name_dma_direction[] = "gpu.dma_direction";
static const char name_display_start[] = "gpu.display_start";
static const char name_horizontal_range[] = "gpu.horizontal_range";
static const char name_vertical_range[] = "gpu.vertical_range";
static const char name_display_mode[] = "gpu.display_mode";
static const char name_allow_2mb[] = "gpu.allow_2mb";
static const char name_read_latch[] = "gpu.read_latch";
static const char name_irq_pending[] = "gpu.irq_pending";
static const char name_beam_tick[] = "gpu.beam_tick";
static const char name_beam_ratio_remainder[] = "gpu.beam_ratio_remainder";
static const char name_beam_line_clock[] = "gpu.beam_line_clock";
static const char name_beam_line[] = "gpu.beam_line";
static const char name_beam_dot_phase[] = "gpu.beam_dot_phase";
static const char name_texture_cache[] = "gpu.texture_cache";
static const char name_texture_tags[] = "gpu.texture_tags";
static const char name_texture_valid[] = "gpu.texture_valid";
static const char name_clut_cache[] = "gpu.clut_cache";
static const char name_clut_x[] = "gpu.clut_x";
static const char name_clut_y[] = "gpu.clut_y";
static const char name_clut_count[] = "gpu.clut_count";
static const char name_clut_valid[] = "gpu.clut_valid";
static const char name_busy_until[] = "gpu.busy_until";

#define GPU_IRQ_BIT 1u
#define GPU_IRQ_MASK (UINT32_C(1) << GPU_IRQ_BIT)
#define GPU_RATIO_DEN UINT64_C(451584)
#define GPU_HBLANK_BIT 2u
#define GPU_VBLANK_BIT 1u
#define GPU_BUSY_MAX (UINT64_C(1024) * 512u * 4u + 65536u)

static void gpu_update_dreq(schway *inst);
static void gpu_dma_resume(schway *inst, uint64_t now);

/* The beam and timer dots use the documented ratio and line lengths (PSX-GPU-AND-GTE section four
   point one; PSX-SYSTEM section one). All phases are carried as integers. */
static uint32_t gpu_clock_num(uint32_t region)
{
    return region == SCHWAY_REGION_PAL ? 709379u : 715909u;
}

static uint32_t gpu_line_clocks(uint32_t region)
{
    return region == SCHWAY_REGION_PAL ? 3406u : 3413u;
}

static uint32_t gpu_lines(uint32_t region)
{
    return region == SCHWAY_REGION_PAL ? 314u : 263u;
}

static uint32_t gpu_dot_divisor(uint32_t mode)
{
    static const uint32_t divisors[4] = {10u, 8u, 5u, 4u};
    return (mode & 0x40u) != 0u ? 7u : divisors[mode & 3u];
}

/* The display registers describe native scan-out dimensions and its source
   window (psx-spx, GPU Display Control Commands). */
void schway__gpu_display(const schway *inst, struct schway__display *display)
{
    const struct schway__gpu *g = &inst->gpu;
    uint32_t mode = g->display_mode;
    static const uint32_t horizontal_pixels[4] = {256u, 320u, 512u, 640u};

    display->x1 = g->horizontal_range & 0x0FFFu;
    display->x2 = (g->horizontal_range >> 12) & 0x0FFFu;
    display->y1 = g->vertical_range & 0x03FFu;
    display->y2 = (g->vertical_range >> 10) & 0x03FFu;
    display->dot_div = gpu_dot_divisor(mode);
    display->field = (mode & 0x20u) != 0u ? (uint32_t)(inst->frame & 1u) : 0u;
    display->interlaced = (mode >> 5) & 1u;
    display->vres = (mode & 0x04u) != 0u ? 480u : 240u;
    display->hres = (mode & 0x40u) != 0u ? 368u : horizontal_pixels[mode & 3u];
    display->start_x = g->display_start & 0x03FFu;
    display->start_y = (g->display_start >> 10) & 0x01FFu;
    display->enabled = g->display_disabled == 0u;
    display->rgb24 = (mode >> 4) & 1u;
}

/* Convert from a number of beam clocks to the first CPU tick that owns them, carrying
   the same fractional ratio through every access and scheduled event. */
static uint64_t gpu_ticks_for_clocks(const struct schway__gpu *g, uint32_t region,
                                     uint64_t clocks)
{
    if (clocks == 0u) {
        return 0u;
    }
    uint64_t numerator = clocks * GPU_RATIO_DEN - g->beam_ratio_remainder;
    uint64_t rate = gpu_clock_num(region);
    return numerator / rate + (numerator % rate != 0u ? 1u : 0u);
}

/* Advance the ratio by a bounded CPU span without multiplying a large absolute tick. */
static uint64_t gpu_clocks_for_ticks(const struct schway__gpu *g, uint32_t region,
                                     uint64_t ticks, uint32_t *remainder)
{
    uint64_t rate = gpu_clock_num(region);
    uint64_t whole = (ticks / GPU_RATIO_DEN) * rate;
    uint64_t part = g->beam_ratio_remainder + (ticks % GPU_RATIO_DEN) * rate;
    *remainder = (uint32_t)(part % GPU_RATIO_DEN);
    return whole + part / GPU_RATIO_DEN;
}

static uint32_t min_u32(uint32_t a, uint32_t b)
{
    return a < b ? a : b;
}

static void transfer_clear(struct schway__gpu *g)
{
    g->owed_words = 0u;
    g->transfer_x = 0u;
    g->transfer_y = 0u;
    g->transfer_width = 0u;
    g->transfer_height = 0u;
    g->transfer_index = 0u;
}

static void polyline_clear(struct schway__gpu *g)
{
    g->polyline_gouraud = 0u;
    g->polyline_color = 0u;
    g->polyline_next_color = 0u;
    g->polyline_x = 0u;
    g->polyline_y = 0u;
    g->polyline_has_vertex = 0u;
    g->polyline_semi = 0u;
}

static void packet_clear(struct schway__gpu *g)
{
    memset(g->fifo, 0, sizeof g->fifo);
    g->fifo_count = 0u;
    g->packet_words = 0u;
    g->phase = SCHWAY__GPU_IDLE;
    polyline_clear(g);
    transfer_clear(g);
}

/* GPU power-on register defaults (psx-spx, GPU Display Control Commands;
   .planning/preparation/PSX-GPU-AND-GTE.md, sections one point three and two point four). */
void schway__gpu_reset(schway *inst)
{
    struct schway__gpu *g = &inst->gpu;
    memset(g, 0, sizeof *g);
    g->display_disabled = 1u;
    g->horizontal_range = 0x00C60260u;
    g->vertical_range = 0x00040010u;
    g->beam_tick = inst->ticks;
}

static int gpu_beam_active(const schway *inst)
{
    return inst->exe_loaded != 0u || inst->gpu.display_mode != 0u;
}

/* Push a span of GPU clocks to Timer 0 without polling individual dots. */
static void gpu_beam_advance(schway *inst, uint64_t tick, uint64_t ticks)
{
    struct schway__gpu *g = &inst->gpu;
    uint32_t remainder;
    uint64_t clocks = gpu_clocks_for_ticks(g, inst->region, ticks, &remainder);
    uint32_t divisor = gpu_dot_divisor(g->display_mode);
    uint64_t dots = ((uint64_t)g->beam_dot_phase + clocks) / divisor;
    schway__timers_sync(inst, tick);
    g->beam_ratio_remainder = remainder;
    g->beam_line_clock += (uint32_t)clocks;
    g->beam_dot_phase = (uint32_t)(((uint64_t)g->beam_dot_phase + clocks) % divisor);
    g->beam_tick = tick;
    if (dots != 0u) {
        schway__timers_dots(inst, tick, (uint32_t)dots);
    }
}

/* Apply hblank at X2 and vblank at the Y2/Y1 line comparisons. Invalid coordinates do not
   prevent line wrap; this is the documented range comparison model, not a measured timing claim. */
static int gpu_beam_edges(schway *inst)
{
    struct schway__gpu *g = &inst->gpu;
    uint32_t line_clocks = gpu_line_clocks(inst->region);
    uint32_t lines = gpu_lines(inst->region);
    uint32_t hblank_at = (g->horizontal_range >> 12) & 0x0FFFu;
    int changed = 0;

    if (hblank_at <= line_clocks && g->beam_line_clock >= hblank_at &&
        (inst->timers.blank & GPU_HBLANK_BIT) == 0u) {
        schway__timers_hblank(inst, g->beam_tick, 1u);
        changed = 1;
    }
    if (g->beam_line_clock >= line_clocks) {
        if ((inst->timers.blank & GPU_HBLANK_BIT) != 0u) {
            schway__timers_hblank(inst, g->beam_tick, 0u);
        }
        g->beam_line_clock -= line_clocks;
        g->beam_line++;
        if (g->beam_line == lines) {
            g->beam_line = 0u;
            schway__frame_gpu_wrap(inst);
        }

        uint32_t y1 = g->vertical_range & 0x03FFu;
        uint32_t y2 = (g->vertical_range >> 10) & 0x03FFu;
        if (y2 < lines && g->beam_line == y2) {
            schway__timers_vblank(inst, g->beam_tick, 1u);
            schway__irq_raise(inst, 0u);
        }
        if (y1 < lines && g->beam_line == y1) {
            schway__timers_vblank(inst, g->beam_tick, 0u);
        }
        changed = 1;
    }
    return changed;
}

/* The next display edge, line end, or enabled Timer 0 dot-source IRQ. */
uint64_t schway__gpu_next(const schway *inst)
{
    const struct schway__gpu *g = &inst->gpu;
    uint64_t next = g->busy_until != 0u ? g->busy_until : UINT64_MAX;
    if (!gpu_beam_active(inst)) {
        return next;
    }
    uint32_t line_clocks = gpu_line_clocks(inst->region);
    if (g->beam_line_clock >= line_clocks) {
        return g->beam_tick < next ? g->beam_tick : next;
    }
    uint32_t hblank_at = (g->horizontal_range >> 12) & 0x0FFFu;
    uint64_t clocks = line_clocks - g->beam_line_clock;

    if (hblank_at <= line_clocks && (inst->timers.blank & GPU_HBLANK_BIT) == 0u) {
        uint64_t hblank_clocks = hblank_at > g->beam_line_clock
                                     ? hblank_at - g->beam_line_clock
                                     : 0u;
        if (hblank_clocks < clocks) {
            clocks = hblank_clocks;
        }
    }

    uint32_t dot_steps = schway__timers_dot_next(inst);
    if (dot_steps != UINT32_MAX) {
        uint32_t divisor = gpu_dot_divisor(g->display_mode);
        uint64_t dot_clocks = (uint64_t)dot_steps * divisor - g->beam_dot_phase;
        if (dot_clocks < clocks) {
            clocks = dot_clocks;
        }
    }

    uint64_t beam = g->beam_tick + gpu_ticks_for_clocks(g, inst->region, clocks);
    return beam < next ? beam : next;
}

/* Lazy catch-up splits only at a line/display edge or a derived dot-target deadline. */
void schway__gpu_sync(schway *inst, uint64_t now)
{
    struct schway__gpu *g = &inst->gpu;
    if (!gpu_beam_active(inst)) {
        if (g->busy_until != 0u && g->busy_until <= now) {
            g->busy_until = 0u;
            gpu_update_dreq(inst);
            gpu_dma_resume(inst, now);
        }
        return;
    }
    if (now < g->beam_tick) {
        return;
    }
    for (;;) {
        if (gpu_beam_edges(inst) != 0) {
            continue;
        }
        if (g->beam_tick == now) {
            break;
        }

        uint32_t line_clocks = gpu_line_clocks(inst->region);
        uint32_t hblank_at = (g->horizontal_range >> 12) & 0x0FFFu;
        uint64_t to_event = line_clocks - g->beam_line_clock;
        if (hblank_at <= line_clocks && (inst->timers.blank & GPU_HBLANK_BIT) == 0u &&
            hblank_at > g->beam_line_clock && hblank_at - g->beam_line_clock < to_event) {
            to_event = hblank_at - g->beam_line_clock;
        }
        uint32_t dot_steps = schway__timers_dot_next(inst);
        if (dot_steps != UINT32_MAX) {
            uint32_t divisor = gpu_dot_divisor(g->display_mode);
            uint64_t dot_clocks = (uint64_t)dot_steps * divisor - g->beam_dot_phase;
            if (dot_clocks < to_event) {
                to_event = dot_clocks;
            }
        }

        uint64_t elapsed = gpu_ticks_for_clocks(g, inst->region, to_event);
        uint64_t event_tick = g->beam_tick + elapsed;
        if (event_tick > now) {
            gpu_beam_advance(inst, now, now - g->beam_tick);
            break;
        }
        gpu_beam_advance(inst, event_tick, elapsed);
    }
    schway__timers_sync(inst, now);
    if (g->busy_until != 0u && g->busy_until <= now) {
        g->busy_until = 0u;
        gpu_update_dreq(inst);
        gpu_dma_resume(inst, now);
    }
}

void schway__gpu_event(schway *inst, uint64_t now)
{
    schway__gpu_sync(inst, now);
}

/* GP0 packet lengths and polyline terminators follow psx-spx, GPU Commands
   and .planning/preparation/PSX-GPU-AND-GTE.md section 2. */
static uint32_t command_words(uint32_t command)
{
    uint32_t op = command >> 24;
    if (op >= 0x20u && op <= 0x3Fu) {
        uint32_t vertices = (op & 0x08u) != 0u ? 4u : 3u;
        uint32_t gouraud = (op & 0x10u) != 0u ? 1u : 0u;
        uint32_t textured = (op & 0x04u) != 0u ? 1u : 0u;
        return 1u + vertices * (1u + textured) + (vertices - 1u) * gouraud;
    }
    if (op >= 0x40u && op <= 0x5Fu) {
        if ((op & 0x08u) != 0u) {
            return 2u; /* command/color and the first vertex; then a terminator stream */
        }
        return 3u + ((op & 0x10u) != 0u ? 1u : 0u);
    }
    if (op >= 0x60u && op <= 0x7Fu) {
        uint32_t textured = (op & 0x04u) != 0u ? 1u : 0u;
        uint32_t variable_size = (op & 0x18u) == 0u ? 1u : 0u;
        return 2u + textured + variable_size;
    }
    if (op >= 0x80u && op <= 0x9Fu) {
        return 4u;
    }
    if (op >= 0xA0u && op <= 0xDFu) {
        return 3u;
    }
    if (op == 0x02u) {
        return 3u;
    }
    if (op == 0x03u) {
        return 1u; /* unknown command: consume its FIFO word, without effects */
    }
    return 1u;
}

enum gpu_busy_class {
    GPU_BUSY_FILL = 0u,
    GPU_BUSY_RECTANGLE,
    GPU_BUSY_RECTANGLE_SEMI,
    GPU_BUSY_TEXTURED_RECTANGLE,
    GPU_BUSY_TEXTURED_RECTANGLE_SEMI,
    GPU_BUSY_POLYGON,
    GPU_BUSY_POLYGON_SEMI,
    GPU_BUSY_TEXTURED_POLYGON,
    GPU_BUSY_TEXTURED_POLYGON_SEMI,
    GPU_BUSY_LINE,
    GPU_BUSY_COPY,
    GPU_BUSY_CLASS_COUNT
};

struct gpu_busy_cost {
    uint32_t setup;
    uint32_t milli_per_pixel;
};

/* Integer setup-plus-pixel costs seeded from PSX-GPU-AND-GTE section four point four and
   choice four point A. They are a deterministic model, not a hardware timing claim. */
static const struct gpu_busy_cost gpu_busy_costs[GPU_BUSY_CLASS_COUNT] = {
    {8u, 81u},   {8u, 530u},  {8u, 797u},  {8u, 1188u}, {8u, 1497u}, {8u, 538u},
    {8u, 812u},  {8u, 2980u}, {8u, 3463u}, {8u, 1000u}, {8u, 1000u},
};

static uint32_t gpu_extent(uint32_t encoded, uint32_t mask)
{
    return ((encoded - 1u) & mask) + 1u;
}

static int32_t gpu_signed_vertex(uint32_t word, uint32_t shift)
{
    int32_t value = (int32_t)((word >> shift) & 0x7FFu);
    return (value & 0x400) != 0 ? value - 0x800 : value;
}

static uint32_t gpu_abs_i32(int32_t value)
{
    return value < 0 ? (uint32_t)(-value) : (uint32_t)value;
}

static uint64_t gpu_busy_cost(uint32_t cost_class, uint64_t pixels)
{
    const struct gpu_busy_cost *cost = &gpu_busy_costs[cost_class];
    return (uint64_t)cost->setup + (pixels * cost->milli_per_pixel + 999u) / 1000u;
}

static uint32_t gpu_fill_pixels(uint32_t size)
{
    uint32_t width = size & 0x3FFu;
    uint32_t height = gpu_extent((size >> 16) & 0x1FFu, 0x1FFu);
    width = (width + 15u) & ~15u;
    if (width == 0u) {
        width = 16u;
    }
    if (width > SCHWAY__VRAM_W) {
        width = SCHWAY__VRAM_W;
    }
    return width * height;
}

static uint64_t gpu_packet_busy_clocks(const struct schway__gpu *g, uint32_t command)
{
    uint32_t op = command >> 24;
    if (op == 0x02u && g->fifo_count == 3u) {
        return gpu_busy_cost(GPU_BUSY_FILL, gpu_fill_pixels(g->fifo[2]));
    }
    if (op >= 0x80u && op <= 0x9Fu && g->fifo_count == 4u) {
        uint32_t width = gpu_extent(g->fifo[3] & 0x3FFu, 0x3FFu);
        uint32_t height = gpu_extent((g->fifo[3] >> 16) & 0x1FFu, 0x1FFu);
        return gpu_busy_cost(GPU_BUSY_COPY, (uint64_t)width * height);
    }
    if (op >= 0x60u && op <= 0x7Fu) {
        uint32_t textured = (op & 4u) != 0u;
        uint32_t size = (op & 0x18u) == 0u ? g->fifo[2u + textured] : 0u;
        uint32_t width;
        uint32_t height;
        if ((op & 0x18u) == 0u) {
            width = gpu_extent(size & 0x3FFu, 0x3FFu);
            height = gpu_extent((size >> 16) & 0x1FFu, 0x1FFu);
        } else {
            static const uint32_t fixed[4] = {1u, 1u, 8u, 16u};
            width = fixed[(op >> 3) & 3u];
            height = width;
        }
        uint32_t cost_class = textured != 0u
                                  ? ((op & 2u) != 0u ? GPU_BUSY_TEXTURED_RECTANGLE_SEMI
                                                     : GPU_BUSY_TEXTURED_RECTANGLE)
                                  : ((op & 2u) != 0u ? GPU_BUSY_RECTANGLE_SEMI
                                                     : GPU_BUSY_RECTANGLE);
        return gpu_busy_cost(cost_class, (uint64_t)width * height);
    }
    if (op >= 0x20u && op <= 0x3Fu) {
        uint32_t vertices = (op & 0x08u) != 0u ? 4u : 3u;
        uint32_t textured = (op & 0x04u) != 0u;
        uint32_t gouraud = (op & 0x10u) != 0u;
        uint32_t cursor = 1u;
        int32_t min_x = INT32_MAX;
        int32_t max_x = INT32_MIN;
        int32_t min_y = INT32_MAX;
        int32_t max_y = INT32_MIN;
        for (uint32_t i = 0u; i < vertices; i++) {
            int32_t x = gpu_signed_vertex(g->fifo[cursor], 0u);
            int32_t y = gpu_signed_vertex(g->fifo[cursor], 16u);
            cursor++;
            if (x < min_x) {
                min_x = x;
            }
            if (x > max_x) {
                max_x = x;
            }
            if (y < min_y) {
                min_y = y;
            }
            if (y > max_y) {
                max_y = y;
            }
            if (textured != 0u) {
                cursor++;
            }
            if (gouraud != 0u && i + 1u < vertices) {
                cursor++;
            }
        }
        uint32_t width = (uint32_t)(max_x - min_x + 1);
        uint32_t height = (uint32_t)(max_y - min_y + 1);
        if (width > SCHWAY__VRAM_W) {
            width = SCHWAY__VRAM_W;
        }
        if (height > SCHWAY__VRAM_H) {
            height = SCHWAY__VRAM_H;
        }
        uint32_t cost_class = textured != 0u
                                  ? ((op & 2u) != 0u ? GPU_BUSY_TEXTURED_POLYGON_SEMI
                                                     : GPU_BUSY_TEXTURED_POLYGON)
                                  : ((op & 2u) != 0u ? GPU_BUSY_POLYGON_SEMI : GPU_BUSY_POLYGON);
        return gpu_busy_cost(cost_class, (uint64_t)width * height);
    }
    if (op >= 0x40u && op <= 0x5Fu && (op & 8u) == 0u) {
        uint32_t gouraud = (op & 0x10u) != 0u;
        uint32_t word0 = g->fifo[1];
        uint32_t word1 = g->fifo[gouraud != 0u ? 3u : 2u];
        int32_t x0 = gpu_signed_vertex(word0, 0u);
        int32_t y0 = gpu_signed_vertex(word0, 16u);
        int32_t x1 = gpu_signed_vertex(word1, 0u);
        int32_t y1 = gpu_signed_vertex(word1, 16u);
        uint32_t dx = gpu_abs_i32(x1 - x0);
        uint32_t dy = gpu_abs_i32(y1 - y0);
        return gpu_busy_cost(GPU_BUSY_LINE, (uint64_t)(dx > dy ? dx : dy) + 1u);
    }
    return 0u;
}

static void gpu_busy_add(schway *inst, uint64_t tick, uint64_t clocks)
{
    struct schway__gpu *g = &inst->gpu;
    if (clocks == 0u) {
        return;
    }
    uint64_t base = g->busy_until > tick ? g->busy_until : tick;
    uint64_t delta = gpu_ticks_for_clocks(g, inst->region, clocks);
    uint64_t proposed = base > UINT64_MAX - delta ? UINT64_MAX : base + delta;
    uint64_t limit = tick > UINT64_MAX - GPU_BUSY_MAX ? UINT64_MAX : tick + GPU_BUSY_MAX;
    g->busy_until = proposed < limit ? proposed : limit;
}

static void parser_idle(struct schway__gpu *g)
{
    memset(g->fifo, 0, sizeof g->fifo);
    g->fifo_count = 0u;
    g->packet_words = 0u;
    g->phase = SCHWAY__GPU_IDLE;
    polyline_clear(g);
}

static uint16_t rgb555(uint32_t rgb)
{
    return (uint16_t)(((rgb >> 3) & 0x001Fu) | ((rgb >> 6) & 0x03E0u) |
                      ((rgb >> 9) & 0x7C00u));
}

/* GP0(02h) uses halfword coordinates, aligned X, 16-pixel width rounding,
   9-bit height, and wraps at both VRAM edges (psx-spx, GP0 Commands). */
static void fill_vram(schway *inst, uint32_t command, uint32_t position, uint32_t size)
{
    uint32_t x = (position & 0x3FFu) & 0x3F0u;
    uint32_t y = (position >> 16) & 0x1FFu;
    uint32_t width = ((size & 0x3FFu) + 0x0Fu) & ~0x0Fu;
    uint32_t height = (size >> 16) & 0x1FFu;
    uint16_t color = rgb555(command & 0x00FFFFFFu);
    if (width == 0u || height == 0u) {
        return;
    }
    for (uint32_t row = 0u; row < height; row++) {
        uint16_t *dst = inst->vram[(y + row) & 0x1FFu];
        for (uint32_t col = 0u; col < width; col++) {
            dst[(x + col) & 0x3FFu] = color;
        }
    }
}

static uint32_t transfer_pixels(const struct schway__gpu *g)
{
    return g->transfer_width * g->transfer_height;
}

static int mask_allows_write(const struct schway__gpu *g, uint16_t destination)
{
    return (g->mask & 2u) == 0u || (destination & 0x8000u) == 0u;
}

static uint16_t mask_write_value(const struct schway__gpu *g, uint16_t value)
{
    return (uint16_t)(value | ((g->mask & 1u) != 0u ? 0x8000u : 0u));
}

static void texture_cache_clear(struct schway__gpu *g)
{
    memset(g->texture_cache, 0, sizeof g->texture_cache);
    memset(g->texture_tags, 0, sizeof g->texture_tags);
    memset(g->texture_valid, 0, sizeof g->texture_valid);
}

static void clut_cache_clear(struct schway__gpu *g)
{
    memset(g->clut_cache, 0, sizeof g->clut_cache);
    g->clut_x = 0u;
    g->clut_y = 0u;
    g->clut_count = 0u;
    g->clut_valid = 0u;
}

/* The 2 KiB texture cache is 256 direct-mapped eight-byte lines. */
uint16_t schway__gpu_texture_read(schway *inst, uint32_t x, uint32_t y)
{
    if (inst == NULL) {
        return 0u;
    }
    x &= 0x3FFu;
    y &= 0x1FFu;
    struct schway__gpu *g = &inst->gpu;
    uint32_t block = (y * SCHWAY__VRAM_W + x) / SCHWAY__GPU_TEXTURE_CACHE_PIXELS;
    uint32_t line = block % SCHWAY__GPU_TEXTURE_CACHE_LINES;
    uint32_t tag = block / SCHWAY__GPU_TEXTURE_CACHE_LINES;
    if (g->texture_valid[line] == 0u || g->texture_tags[line] != tag) {
        uint32_t base_x = x & ~(SCHWAY__GPU_TEXTURE_CACHE_PIXELS - 1u);
        for (uint32_t i = 0u; i < SCHWAY__GPU_TEXTURE_CACHE_PIXELS; i++) {
            g->texture_cache[line][i] = inst->vram[y][base_x + i];
        }
        g->texture_tags[line] = tag;
        g->texture_valid[line] = 1u;
    }
    return g->texture_cache[line][x & (SCHWAY__GPU_TEXTURE_CACHE_PIXELS - 1u)];
}

/* CLUT rows persist until a new palette address or GP1(01h) replaces them. */
uint16_t schway__gpu_clut_read(schway *inst, uint32_t x, uint32_t y, uint32_t index,
                               uint32_t count)
{
    if (inst == NULL || (count != 16u && count != SCHWAY__GPU_CLUT_CACHE_ENTRIES) ||
        index >= count) {
        return 0u;
    }
    x &= 0x3FFu;
    y &= 0x1FFu;
    struct schway__gpu *g = &inst->gpu;
    if (g->clut_valid == 0u || g->clut_x != x || g->clut_y != y || count > g->clut_count) {
        for (uint32_t i = 0u; i < count; i++) {
            g->clut_cache[i] = inst->vram[y][(x + i) & 0x3FFu];
        }
        g->clut_x = x;
        g->clut_y = y;
        g->clut_count = count;
        g->clut_valid = 1u;
    }
    return g->clut_cache[index];
}

/* GP0(80h) scans source and destination rows in order; each coordinate wraps
   independently and E6 checks the old destination before forcing bit 15
   (psx-spx, GPU Commands; .planning/preparation/PSX-GPU-AND-GTE.md, GP0 Commands). */
static void copy_vram(schway *inst, uint32_t source, uint32_t destination, uint32_t size)
{
    struct schway__gpu *g = &inst->gpu;
    uint32_t source_x = source & 0x3FFu;
    uint32_t source_y = (source >> 16) & 0x1FFu;
    uint32_t dest_x = destination & 0x3FFu;
    uint32_t dest_y = (destination >> 16) & 0x1FFu;
    uint32_t encoded_width = size & 0x3FFu;
    uint32_t encoded_height = (size >> 16) & 0x1FFu;
    uint32_t width = ((encoded_width - 1u) & 0x3FFu) + 1u;
    uint32_t height = ((encoded_height - 1u) & 0x1FFu) + 1u;

    for (uint32_t row = 0u; row < height; row++) {
        uint32_t sy = (source_y + row) & 0x1FFu;
        uint32_t dy = (dest_y + row) & 0x1FFu;
        for (uint32_t col = 0u; col < width; col++) {
            uint32_t sx = (source_x + col) & 0x3FFu;
            uint32_t dx = (dest_x + col) & 0x3FFu;
            uint16_t color = inst->vram[sy][sx];
            uint16_t old = inst->vram[dy][dx];
            if (mask_allows_write(g, old)) {
                inst->vram[dy][dx] = mask_write_value(g, color);
            }
        }
    }
}

static void transfer_begin(struct schway__gpu *g, uint32_t phase, uint32_t position,
                           uint32_t size)
{
    uint32_t width = size & 0x3FFu;
    uint32_t height = (size >> 16) & 0x1FFu;
    g->transfer_x = position & 0x3FFu;
    g->transfer_y = (position >> 16) & 0x1FFu;
    g->transfer_width = ((width - 1u) & 0x3FFu) + 1u;
    g->transfer_height = ((height - 1u) & 0x1FFu) + 1u;
    g->transfer_index = 0u;
    g->owed_words = (transfer_pixels(g) + 1u) / 2u;
    g->phase = phase;
}

static void transfer_finish(struct schway__gpu *g)
{
    transfer_clear(g);
    g->phase = SCHWAY__GPU_IDLE;
}

static void upload_word(schway *inst, uint32_t word)
{
    struct schway__gpu *g = &inst->gpu;
    uint32_t pixels = transfer_pixels(g);
    for (uint32_t half = 0u; half < 2u && g->transfer_index < pixels; half++) {
        uint32_t index = g->transfer_index++;
        uint32_t x = (g->transfer_x + index % g->transfer_width) & 0x3FFu;
        uint32_t y = (g->transfer_y + index / g->transfer_width) & 0x1FFu;
        uint16_t color = (uint16_t)(word >> (16u * half));
        uint16_t old = inst->vram[y][x];
        if (mask_allows_write(g, old)) {
            inst->vram[y][x] = mask_write_value(g, color);
        }
    }
    g->owed_words--;
    if (g->owed_words == 0u) {
        transfer_finish(g);
    }
}

static uint16_t transfer_pixel(const schway *inst, const struct schway__gpu *g, uint32_t index)
{
    uint32_t x = (g->transfer_x + index % g->transfer_width) & 0x3FFu;
    uint32_t y = (g->transfer_y + index / g->transfer_width) & 0x1FFu;
    return inst->vram[y][x];
}

static uint32_t download_peek(const schway *inst, const struct schway__gpu *g)
{
    uint32_t pixels = transfer_pixels(g);
    uint32_t index = g->transfer_index;
    uint32_t low = transfer_pixel(inst, g, index);
    uint32_t high = index + 1u < pixels ? transfer_pixel(inst, g, index + 1u) : 0u;
    return low | (high << 16);
}

static uint32_t gpu_status(const struct schway *inst)
{
    const struct schway__gpu *g = &inst->gpu;
    uint32_t idle = g->busy_until == 0u ? 1u : 0u;
    uint32_t command_ready = idle != 0u && g->phase == SCHWAY__GPU_IDLE ? 1u : 0u;
    uint32_t block_ready = command_ready;
    if (idle != 0u && (g->phase == SCHWAY__GPU_TRANSFER_TO_VRAM ||
                       g->phase == SCHWAY__GPU_POLYLINE_VERTEX ||
                       g->phase == SCHWAY__GPU_POLYLINE_COLOR)) {
        block_ready = 1u;
    }
    if (idle != 0u && g->phase == SCHWAY__GPU_PACKET) {
        uint32_t op = g->fifo[0] >> 24;
        block_ready = op < 0x20u || op > 0x5Fu;
    }
    uint32_t status = g->draw_mode & 0x7FFu;
    status |= (g->mask & 3u) << 11;
    status |= ((g->draw_mode >> 11) & 1u) << 15;
    status |= ((g->display_mode >> 7) & 1u) << 14;
    status |= ((g->display_mode >> 6) & 1u) << 16;
    status |= (g->display_mode & 3u) << 17;
    status |= ((g->display_mode >> 2) & 1u) << 19;
    status |= ((g->display_mode >> 3) & 1u) << 20;
    status |= ((g->display_mode >> 4) & 1u) << 21;
    status |= ((g->display_mode >> 5) & 1u) << 22;
    if (g->display_disabled != 0u) {
        status |= UINT32_C(1) << 23;
    }
    if (g->irq_pending != 0u) {
        status |= UINT32_C(1) << 24;
    }
    switch (g->dma_direction) {
    case 1u:
        if (idle != 0u && g->fifo_count < SCHWAY__GPU_FIFO_WORDS) {
            status |= UINT32_C(1) << 25;
        }
        break;
    case 2u:
        if (block_ready != 0u) {
            status |= UINT32_C(1) << 25;
        }
        break;
    case 3u:
        if (g->phase == SCHWAY__GPU_TRANSFER_FROM_VRAM && g->owed_words != 0u) {
            status |= UINT32_C(1) << 25;
        }
        break;
    default:
        break;
    }
    if (command_ready != 0u) {
        status |= UINT32_C(1) << 26;
    }
    if (block_ready != 0u) {
        status |= UINT32_C(1) << 28;
    }
    if (g->phase == SCHWAY__GPU_TRANSFER_FROM_VRAM && g->owed_words != 0u) {
        status |= UINT32_C(1) << 27;
    }
    if ((g->display_mode & 0x20u) == 0u) {
        status |= UINT32_C(1) << 13;
    }
    status |= (g->dma_direction & 3u) << 29;
    return status;
}

uint32_t schway__gpu_dma_request(const schway *inst)
{
    return (gpu_status(inst) >> 25) & 1u;
}

uint32_t schway__gpu_dma_ready(const schway *inst)
{
    return inst->gpu.busy_until == 0u ? 1u : 0u;
}

static void gpu_update_dreq(schway *inst)
{
    schway__dma_dreq(inst, 2u, schway__gpu_dma_request(inst));
}

static void gpu_dma_resume(schway *inst, uint64_t now)
{
    schway__dma_event(inst, now);
}

static void apply_environment(schway *inst, uint32_t command)
{
    struct schway__gpu *g = &inst->gpu;
    uint32_t op = command >> 24;
    uint32_t value = command & 0x00FFFFFFu;
    switch (op) {
    case 0xE1u:
        g->draw_mode = value & 0x7FFFu;
        break;
    case 0xE2u:
        g->texture_window = value & 0x000FFFFFu;
        break;
    case 0xE3u:
        g->draw_area_tl = value & 0x000FFFFFu;
        break;
    case 0xE4u:
        g->draw_area_br = value & 0x000FFFFFu;
        break;
    case 0xE5u:
        g->draw_offset = value & 0x003FFFFFu;
        break;
    case 0xE6u:
        g->mask = value & 3u;
        break;
    default:
        break;
    }
}

static void gpu_ack_irq(schway *inst)
{
    inst->gpu.irq_pending = 0u;
    schway__irq_write_stat(inst, 0u, GPU_IRQ_MASK);
}

static void process_packet(schway *inst, uint64_t tick)
{
    struct schway__gpu *g = &inst->gpu;
    uint32_t command = g->fifo[0];
    uint32_t op = command >> 24;
    uint32_t word_count = g->fifo_count;
    if (op == 0x02u && word_count == 3u) {
        fill_vram(inst, command, g->fifo[1], g->fifo[2]);
    } else if (op == 0x01u) {
        texture_cache_clear(g);
    } else if (op >= 0x80u && op <= 0x9Fu && word_count == 4u) {
        copy_vram(inst, g->fifo[1], g->fifo[2], g->fifo[3]);
    } else if (op >= 0x60u && op <= 0x7Fu) {
        uint32_t textured = (op & 4u) != 0u;
        uint32_t size = (op & 0x18u) == 0u ? g->fifo[2u + textured] : 0u;
        schway__gpu_rectangle(inst, command, g->fifo[1], textured != 0u ? g->fifo[2] : 0u,
                              size);
    } else if (op >= 0x20u && op <= 0x3Fu) {
        uint32_t vertex_count = (op & 0x08u) != 0u ? 4u : 3u;
        uint32_t gouraud = (op & 0x10u) != 0u;
        uint32_t textured = (op & 0x04u) != 0u;
        uint32_t flags = 0u;
        if ((op & 0x02u) != 0u) {
            flags |= SCHWAY__GPU_PIXEL_SEMI_TRANSPARENT;
        }
        if (textured != 0u) {
            flags |= SCHWAY__GPU_PIXEL_TEXTURED;
            if ((op & 0x01u) != 0u) {
                flags |= SCHWAY__GPU_PIXEL_RAW_TEXTURE;
            } else {
                flags |= SCHWAY__GPU_PIXEL_DITHER_ELIGIBLE;
            }
        }
        if (gouraud != 0u) {
            flags |= SCHWAY__GPU_PIXEL_DITHER_ELIGIBLE;
        }

        uint32_t positions[4] = {0u, 0u, 0u, 0u};
        uint32_t colors[4] = {command & 0x00FFFFFFu, command & 0x00FFFFFFu,
                              command & 0x00FFFFFFu, command & 0x00FFFFFFu};
        uint32_t uvs[4] = {0u, 0u, 0u, 0u};
        uint32_t cursor = 1u;
        for (uint32_t i = 0u; i < vertex_count; i++) {
            positions[i] = g->fifo[cursor++];
            if (textured != 0u) {
                uvs[i] = g->fifo[cursor++];
            }
            if (gouraud != 0u && i + 1u < vertex_count) {
                colors[i + 1u] = g->fifo[cursor++] & 0x00FFFFFFu;
            }
        }
        schway__gpu_polygon(inst, positions, colors, textured != 0u ? uvs : NULL,
                            vertex_count, flags);
    } else if (op == 0x1Fu) {
        g->irq_pending = 1u;
        schway__irq_raise(inst, GPU_IRQ_BIT);
    } else if (op >= 0xE1u && op <= 0xE6u) {
        apply_environment(inst, command);
    } else if (op >= 0xA0u && op <= 0xBFu && word_count == 3u) {
        transfer_begin(g, SCHWAY__GPU_TRANSFER_TO_VRAM, g->fifo[1], g->fifo[2]);
        parser_idle(g);
        g->phase = SCHWAY__GPU_TRANSFER_TO_VRAM;
        return;
    } else if (op >= 0xC0u && op <= 0xDFu && word_count == 3u) {
        transfer_begin(g, SCHWAY__GPU_TRANSFER_FROM_VRAM, g->fifo[1], g->fifo[2]);
        parser_idle(g);
        g->phase = SCHWAY__GPU_TRANSFER_FROM_VRAM;
        return;
    } else if (op >= 0x40u && op <= 0x5Fu && (op & 0x08u) == 0u) {
        uint32_t gouraud = (op & 0x10u) != 0u;
        uint32_t position0 = g->fifo[1];
        uint32_t position1 = gouraud != 0u ? g->fifo[3] : g->fifo[2];
        uint32_t color0 = command & 0x00FFFFFFu;
        uint32_t color1 = gouraud != 0u ? g->fifo[2] & 0x00FFFFFFu : color0;
        schway__gpu_line(inst, command, position0, position1, color0, color1, 0u);
    } else if (op >= 0x40u && op <= 0x5Fu && (op & 0x08u) != 0u) {
        uint32_t gouraud = (op & 0x10u) != 0u;
        uint32_t first_vertex = g->fifo[1];
        parser_idle(g);
        g->polyline_gouraud = gouraud;
        g->polyline_color = command & 0x00FFFFFFu;
        g->polyline_semi = (op & 2u) != 0u ? 1u : 0u;
        g->polyline_x = first_vertex & 0x7FFu;
        g->polyline_y = (first_vertex >> 16) & 0x7FFu;
        g->polyline_has_vertex = 1u;
        g->phase = gouraud != 0u ? SCHWAY__GPU_POLYLINE_COLOR
                                 : SCHWAY__GPU_POLYLINE_VERTEX;
        return;
    }
    gpu_busy_add(inst, tick, gpu_packet_busy_clocks(g, command));
    parser_idle(g);
}

static int polyline_terminator(uint32_t word)
{
    return (word & 0xF000F000u) == 0x50005000u;
}

static void polyline_word(schway *inst, uint32_t word, uint64_t tick)
{
    struct schway__gpu *g = &inst->gpu;
    if (g->phase == SCHWAY__GPU_POLYLINE_COLOR) {
        if (polyline_terminator(word)) {
            parser_idle(g);
            return;
        }
        g->polyline_next_color = word & 0x00FFFFFFu;
        g->phase = SCHWAY__GPU_POLYLINE_VERTEX;
        return;
    }
    if (g->polyline_gouraud == 0u && polyline_terminator(word)) {
        parser_idle(g);
        return;
    }
    uint32_t x = word & 0x7FFu;
    uint32_t y = (word >> 16) & 0x7FFu;
    if (g->polyline_has_vertex != 0u) {
        uint32_t position0 = g->polyline_x | (g->polyline_y << 16);
        uint32_t position1 = x | (y << 16);
        uint32_t color1 = g->polyline_gouraud != 0u ? g->polyline_next_color
                                                     : g->polyline_color;
        uint32_t command = (0x40u | (g->polyline_gouraud != 0u ? 0x10u : 0u) |
                            (g->polyline_semi != 0u ? 2u : 0u)) << 24;
        schway__gpu_line(inst, command, position0, position1, g->polyline_color, color1, 0u);
        uint32_t dx = gpu_abs_i32((int32_t)x - (int32_t)g->polyline_x);
        uint32_t dy = gpu_abs_i32((int32_t)y - (int32_t)g->polyline_y);
        gpu_busy_add(inst, tick, gpu_busy_cost(GPU_BUSY_LINE, (uint64_t)(dx > dy ? dx : dy) + 1u));
        if (g->polyline_gouraud != 0u) {
            g->polyline_color = g->polyline_next_color;
            g->polyline_next_color = 0u;
        }
    } else {
        g->polyline_has_vertex = 1u;
    }
    g->polyline_x = x;
    g->polyline_y = y;
    if (g->polyline_gouraud != 0u) {
        g->phase = SCHWAY__GPU_POLYLINE_COLOR;
    }
}

/* CPU writes and DMA2 enter here, one bounded word at a time. */
void schway__gpu_word(schway *inst, uint32_t word, uint64_t tick)
{
    struct schway__gpu *g = &inst->gpu;
    if (g->phase == SCHWAY__GPU_TRANSFER_TO_VRAM) {
        upload_word(inst, word);
        return;
    }
    if (g->phase == SCHWAY__GPU_TRANSFER_FROM_VRAM) {
        return;
    }
    if (g->phase == SCHWAY__GPU_POLYLINE_VERTEX ||
        g->phase == SCHWAY__GPU_POLYLINE_COLOR) {
        polyline_word(inst, word, tick);
        return;
    }
    if (g->phase == SCHWAY__GPU_PACKET) {
        if (g->fifo_count >= SCHWAY__GPU_FIFO_WORDS || g->fifo_count >= g->packet_words) {
            packet_clear(g);
            return;
        }
        g->fifo[g->fifo_count++] = word;
        if (g->fifo_count == g->packet_words) {
            process_packet(inst, tick);
        }
        return;
    }
    g->fifo[0] = word;
    g->fifo_count = 1u;
    g->packet_words = command_words(word);
    if (g->packet_words > SCHWAY__GPU_FIFO_WORDS || g->packet_words == 0u) {
        packet_clear(g);
    } else if (g->packet_words == 1u) {
        process_packet(inst, tick);
    } else {
        g->phase = SCHWAY__GPU_PACKET;
    }
}

static uint32_t lane_mask(uint32_t width)
{
    return width == 4u ? UINT32_MAX : (UINT32_C(1) << (width * 8u)) - 1u;
}

static uint32_t lane_shift(uint32_t addr)
{
    return (addr & 3u) * 8u;
}

static uint32_t lane_get(uint32_t value, uint32_t addr, uint32_t width)
{
    return (value >> lane_shift(addr)) & lane_mask(width);
}

static uint32_t gpuread(const schway *inst)
{
    const struct schway__gpu *g = &inst->gpu;
    if (g->phase == SCHWAY__GPU_TRANSFER_FROM_VRAM && g->owed_words != 0u) {
        return download_peek(inst, g);
    }
    return g->read_latch;
}

static void gpu_read_consume(schway *inst, uint32_t addr, uint32_t width)
{
    if ((addr & 0xCu) == 0u && inst->gpu.phase == SCHWAY__GPU_TRANSFER_FROM_VRAM &&
        inst->gpu.owed_words != 0u) {
        struct schway__gpu *g = &inst->gpu;
        uint32_t pixels = transfer_pixels(g);
        uint32_t remain = pixels - g->transfer_index;
        g->read_latch = lane_get(download_peek(inst, g), addr, width);
        g->transfer_index += min_u32(remain, 2u);
        g->owed_words--;
        if (g->owed_words == 0u) {
            transfer_finish(g);
        }
    }
}

void schway__gpu_dma_write(schway *inst, uint32_t word, uint64_t tick)
{
    schway__gpu_sync(inst, tick);
    schway__gpu_word(inst, word, tick);
    gpu_update_dreq(inst);
}

uint32_t schway__gpu_dma_read(schway *inst, uint64_t tick)
{
    schway__gpu_sync(inst, tick);
    uint32_t value = gpuread(inst);
    gpu_read_consume(inst, 0x1F801810u, 4u);
    gpu_update_dreq(inst);
    return value;
}

/* Side-effect-free port image used by the bus and internal inspection. */
int schway__gpu_peek(const schway *inst, uint32_t addr, uint32_t width, uint32_t *value)
{
    if (inst == NULL || value == NULL ||
        (width != 1u && width != 2u && width != 4u)) {
        return 1;
    }
    uint32_t slot = addr & 0xCu;
    if (slot != 0u && slot != 4u) {
        return 1;
    }
    *value = lane_get(slot == 4u ? gpu_status(inst) : gpuread(inst), addr, width);
    return 0;
}

static void gpu_gp1(schway *inst, uint32_t command, uint64_t tick)
{
    struct schway__gpu *g = &inst->gpu;
    uint32_t op = command >> 24;
    uint32_t value = command & 0x00FFFFFFu;
    switch (op) {
    case 0x00u:
    {
        uint32_t allow_2mb = g->allow_2mb;
        gpu_ack_irq(inst);
        schway__gpu_reset(inst);
        inst->gpu.allow_2mb = allow_2mb;
        inst->gpu.beam_tick = tick;
        schway__frame_gpu_reset(inst, tick);
        schway__timers_vblank(inst, tick, 0u);
        schway__timers_hblank(inst, tick, 0u);
        break;
    }
    case 0x01u:
        packet_clear(g);
        clut_cache_clear(g);
        break;
    case 0x02u:
        gpu_ack_irq(inst);
        break;
    case 0x03u:
        g->display_disabled = value & 1u;
        break;
    case 0x04u:
        g->dma_direction = value & 3u;
        break;
    case 0x05u:
        g->display_start = value & 0x0007FFFFu;
        break;
    case 0x06u:
        g->horizontal_range = value & 0x00FFFFFFu;
        break;
    case 0x07u:
        g->vertical_range = value & 0x000FFFFFu;
        break;
    case 0x08u:
        g->display_mode = value & 0xFFu;
        g->beam_dot_phase %= gpu_dot_divisor(g->display_mode);
        break;
    case 0x09u:
        g->allow_2mb = value & 1u;
        break;
    case 0x10u: {
        uint32_t index = value & 0x0Fu;
        switch (index) {
        case 2u:
            g->read_latch = g->texture_window;
            break;
        case 3u:
            g->read_latch = g->draw_area_tl;
            break;
        case 4u:
            g->read_latch = g->draw_area_br;
            break;
        case 5u:
            g->read_latch = g->draw_offset;
            break;
        case 7u:
            g->read_latch = 2u;
            break;
        case 8u:
            g->read_latch = 0u;
            break;
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

/* One GPU register access through the physical bus. */
int schway__gpu_io(schway *inst, uint32_t wr, uint32_t addr, uint32_t width,
                   uint32_t *value, uint32_t *cycles)
{
    uint32_t slot = addr & 0xCu;
    if (slot != 0u && slot != 4u) {
        return 1;
    }
    uint64_t tick = schway__sched_now(inst, *cycles);
    schway__gpu_sync(inst, tick);
    if (wr != 0u) {
        uint32_t word = (*value & lane_mask(width)) << lane_shift(addr);
        if (slot == 0u) {
            schway__gpu_word(inst, word, tick);
        } else {
            gpu_gp1(inst, word, tick);
        }
        gpu_update_dreq(inst);
    } else {
        if (schway__gpu_peek(inst, addr, width, value) != 0) {
            return 1;
        }
        gpu_read_consume(inst, addr, width);
        gpu_update_dreq(inst);
    }
    *cycles += 1u;
    schway__sched_recalc(inst);
    return 0;
}

static uint32_t expected_packet_words(uint32_t command)
{
    return command_words(command);
}

/* One state walk owns VRAM and every field that can affect command decoding
   or a partially completed transfer. */
void schway__gpu_state_io(struct schway__state_io *io, schway *inst, uint64_t state_ticks)
{
    struct schway__gpu *g = &inst->gpu;
    schway__state_u16_array(io, name_vram, &inst->vram[0][0], SCHWAY__VRAM_W * SCHWAY__VRAM_H,
                            0u, UINT16_MAX);
    schway__state_u16_array(io, name_texture_cache, &g->texture_cache[0][0],
                            SCHWAY__GPU_TEXTURE_CACHE_LINES * SCHWAY__GPU_TEXTURE_CACHE_PIXELS,
                            0u, UINT16_MAX);
    schway__state_u32_array(io, name_texture_tags, g->texture_tags,
                            SCHWAY__GPU_TEXTURE_CACHE_LINES, 0u, 511u);
    schway__state_u8_array(io, name_texture_valid, g->texture_valid,
                           SCHWAY__GPU_TEXTURE_CACHE_LINES, 0u, 1u);
    schway__state_u16_array(io, name_clut_cache, g->clut_cache,
                            SCHWAY__GPU_CLUT_CACHE_ENTRIES, 0u, UINT16_MAX);
    uint32_t clut_x = schway__state_u32(io, name_clut_x, &g->clut_x, 0u, SCHWAY__VRAM_W - 1u);
    uint32_t clut_y = schway__state_u32(io, name_clut_y, &g->clut_y, 0u, SCHWAY__VRAM_H - 1u);
    uint32_t clut_count = schway__state_u32(io, name_clut_count, &g->clut_count, 0u,
                                            SCHWAY__GPU_CLUT_CACHE_ENTRIES);
    uint32_t clut_valid = schway__state_u32(io, name_clut_valid, &g->clut_valid, 0u, 1u);
    uint32_t packet_command = g->fifo[0];
    if (io->mode == SCHWAY__STATE_VALIDATE) {
        packet_command = (uint32_t)io->r[0] | ((uint32_t)io->r[1] << 8) |
                         ((uint32_t)io->r[2] << 16) | ((uint32_t)io->r[3] << 24);
    }
    schway__state_u32_array(io, name_fifo, g->fifo, SCHWAY__GPU_FIFO_WORDS, 0u, UINT32_MAX);
    uint32_t fifo_count = schway__state_u32(io, name_fifo_count, &g->fifo_count, 0u,
                                           SCHWAY__GPU_FIFO_WORDS);
    uint32_t packet_words = schway__state_u32(io, name_packet_words, &g->packet_words, 0u,
                                              SCHWAY__GPU_FIFO_WORDS);
    uint32_t phase = schway__state_u32(io, name_phase, &g->phase, SCHWAY__GPU_IDLE,
                                       SCHWAY__GPU_TRANSFER_FROM_VRAM);
    uint32_t owed_words = schway__state_u32(io, name_owed_words, &g->owed_words, 0u,
                                             SCHWAY__VRAM_W * SCHWAY__VRAM_H / 2u);
    uint32_t transfer_x = schway__state_u32(io, name_transfer_x, &g->transfer_x, 0u,
                                             SCHWAY__VRAM_W - 1u);
    uint32_t transfer_y = schway__state_u32(io, name_transfer_y, &g->transfer_y, 0u,
                                             SCHWAY__VRAM_H - 1u);
    uint32_t transfer_width = schway__state_u32(io, name_transfer_width, &g->transfer_width,
                                                 0u, SCHWAY__VRAM_W);
    uint32_t transfer_height = schway__state_u32(io, name_transfer_height, &g->transfer_height,
                                                  0u, SCHWAY__VRAM_H);
    uint32_t transfer_index = schway__state_u32(io, name_transfer_index, &g->transfer_index, 0u,
                                                 SCHWAY__VRAM_W * SCHWAY__VRAM_H);
    uint32_t polyline_gouraud = schway__state_u32(io, name_polyline_gouraud,
                                                   &g->polyline_gouraud, 0u, 1u);
    uint32_t polyline_color = schway__state_u32(io, name_polyline_color, &g->polyline_color, 0u,
                                                 0x00FFFFFFu);
    uint32_t polyline_next_color = schway__state_u32(io, name_polyline_next_color,
                                                      &g->polyline_next_color, 0u,
                                                      0x00FFFFFFu);
    uint32_t polyline_x = schway__state_u32(io, name_polyline_x, &g->polyline_x, 0u, 0x7FFu);
    uint32_t polyline_y = schway__state_u32(io, name_polyline_y, &g->polyline_y, 0u, 0x7FFu);
    uint32_t polyline_has_vertex = schway__state_u32(io, name_polyline_has_vertex,
                                                      &g->polyline_has_vertex, 0u, 1u);
    uint32_t polyline_semi = schway__state_u32(io, name_polyline_semi, &g->polyline_semi, 0u,
                                                1u);
    (void)schway__state_u32(io, name_draw_mode, &g->draw_mode, 0u, 0x7FFFu);
    (void)schway__state_u32(io, name_texture_window, &g->texture_window, 0u, 0x000FFFFFu);
    (void)schway__state_u32(io, name_draw_area_tl, &g->draw_area_tl, 0u, 0x000FFFFFu);
    (void)schway__state_u32(io, name_draw_area_br, &g->draw_area_br, 0u, 0x000FFFFFu);
    (void)schway__state_u32(io, name_draw_offset, &g->draw_offset, 0u, 0x003FFFFFu);
    (void)schway__state_u32(io, name_mask, &g->mask, 0u, 3u);
    (void)schway__state_u32(io, name_display_disabled, &g->display_disabled, 0u, 1u);
    (void)schway__state_u32(io, name_dma_direction, &g->dma_direction, 0u, 3u);
    (void)schway__state_u32(io, name_display_start, &g->display_start, 0u, 0x0007FFFFu);
    (void)schway__state_u32(io, name_horizontal_range, &g->horizontal_range, 0u, 0x00FFFFFFu);
    (void)schway__state_u32(io, name_vertical_range, &g->vertical_range, 0u, 0x000FFFFFu);
    uint32_t display_mode = schway__state_u32(io, name_display_mode, &g->display_mode, 0u, 0xFFu);
    (void)schway__state_u32(io, name_allow_2mb, &g->allow_2mb, 0u, 1u);
    (void)schway__state_u32(io, name_read_latch, &g->read_latch, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_irq_pending, &g->irq_pending, 0u, 1u);
    uint64_t beam_tick = schway__state_u64(io, name_beam_tick, &g->beam_tick, 0u,
                                           SCHWAY__STATE_TICKS_MAX);
    uint32_t beam_ratio_remainder = schway__state_u32(io, name_beam_ratio_remainder,
                                                       &g->beam_ratio_remainder, 0u,
                                                       (uint32_t)GPU_RATIO_DEN - 1u);
    uint32_t beam_line_clock = schway__state_u32(io, name_beam_line_clock, &g->beam_line_clock,
                                                  0u, 3413u);
    uint32_t beam_line = schway__state_u32(io, name_beam_line, &g->beam_line, 0u, 313u);
    uint32_t beam_dot_phase = schway__state_u32(io, name_beam_dot_phase, &g->beam_dot_phase,
                                                 0u, 9u);
    uint64_t busy_max = state_ticks > UINT64_MAX - GPU_BUSY_MAX
                            ? UINT64_MAX
                            : state_ticks + GPU_BUSY_MAX;
    uint64_t busy_until = schway__state_u64(io, name_busy_until, &g->busy_until, 0u, busy_max);

    schway__state_check(io, beam_tick <= state_ticks);
    schway__state_check(io, beam_line_clock < gpu_line_clocks(inst->region));
    schway__state_check(io, beam_line < gpu_lines(inst->region));
    schway__state_check(io, beam_ratio_remainder < GPU_RATIO_DEN);
    schway__state_check(io, beam_dot_phase < gpu_dot_divisor(display_mode));
    schway__state_check(io, busy_until == 0u || busy_until > state_ticks);

    schway__state_check(io, phase != SCHWAY__GPU_PACKET ||
                                (fifo_count != 0u && packet_words > fifo_count &&
                                 packet_words == expected_packet_words(packet_command)));
    schway__state_check(io, phase == SCHWAY__GPU_PACKET || (fifo_count == 0u && packet_words == 0u));
    schway__state_check(io, clut_valid == 0u
                                ? (clut_x == 0u && clut_y == 0u && clut_count == 0u)
                                : (clut_count == 16u ||
                                   clut_count == SCHWAY__GPU_CLUT_CACHE_ENTRIES));
    if (phase == SCHWAY__GPU_IDLE) {
        schway__state_check(io, owed_words == 0u && transfer_x == 0u && transfer_y == 0u &&
                                    transfer_width == 0u && transfer_height == 0u &&
                                    transfer_index == 0u && polyline_gouraud == 0u &&
                                    polyline_color == 0u &&
                                    polyline_next_color == 0u && polyline_x == 0u &&
                                    polyline_y == 0u && polyline_has_vertex == 0u &&
                                    polyline_semi == 0u);
    } else if (phase == SCHWAY__GPU_PACKET) {
        schway__state_check(io, owed_words == 0u && transfer_x == 0u && transfer_y == 0u &&
                                    transfer_width == 0u && transfer_height == 0u &&
                                    transfer_index == 0u && polyline_gouraud == 0u &&
                                    polyline_color == 0u &&
                                    polyline_next_color == 0u && polyline_x == 0u &&
                                    polyline_y == 0u && polyline_has_vertex == 0u &&
                                    polyline_semi == 0u);
    } else if (phase == SCHWAY__GPU_POLYLINE_VERTEX || phase == SCHWAY__GPU_POLYLINE_COLOR) {
        schway__state_check(io, owed_words == 0u && transfer_x == 0u && transfer_y == 0u &&
                                    transfer_width == 0u && transfer_height == 0u &&
                                    transfer_index == 0u &&
                                    (phase != SCHWAY__GPU_POLYLINE_COLOR ||
                                     (polyline_gouraud != 0u && polyline_has_vertex != 0u &&
                                      polyline_next_color == 0u)) &&
                                    (polyline_has_vertex != 0u ||
                                     (polyline_x == 0u && polyline_y == 0u &&
                                      polyline_next_color == 0u)) &&
                                    polyline_semi <= 1u &&
                                    (polyline_gouraud != 0u || polyline_next_color == 0u));
    } else if (phase == SCHWAY__GPU_TRANSFER_TO_VRAM ||
               phase == SCHWAY__GPU_TRANSFER_FROM_VRAM) {
        schway__state_check(io, fifo_count == 0u && packet_words == 0u &&
                                    transfer_width != 0u && transfer_width <= SCHWAY__VRAM_W &&
                                    transfer_height != 0u && transfer_height <= SCHWAY__VRAM_H &&
                                    polyline_gouraud == 0u && polyline_color == 0u &&
                                    polyline_next_color == 0u && polyline_x == 0u &&
                                    polyline_y == 0u && polyline_has_vertex == 0u &&
                                    polyline_semi == 0u);
        if (transfer_width != 0u && transfer_width <= SCHWAY__VRAM_W &&
            transfer_height != 0u && transfer_height <= SCHWAY__VRAM_H) {
            uint32_t pixels = transfer_width * transfer_height;
            schway__state_check(io, transfer_index < pixels &&
                                        (pixels - transfer_index + 1u) / 2u == owed_words);
        } else {
            schway__state_check(io, 0);
        }
        schway__state_check(io, phase != SCHWAY__GPU_TRANSFER_FROM_VRAM || owed_words != 0u);
    } else {
        schway__state_check(io, 0);
    }
}
