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

#define GPU_IRQ_BIT 1u
#define GPU_IRQ_MASK (UINT32_C(1) << GPU_IRQ_BIT)

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

static void packet_clear(struct schway__gpu *g)
{
    memset(g->fifo, 0, sizeof g->fifo);
    g->fifo_count = 0u;
    g->packet_words = 0u;
    g->phase = SCHWAY__GPU_IDLE;
    g->polyline_gouraud = 0u;
    g->polyline_color = 0u;
    transfer_clear(g);
}

/* GPU power-on register defaults (psx-spx, GPU Display Control Commands;
   .planning/preparation/PSX-GPU-AND-GTE.md, sections 1.3 and 2.4). */
void schway__gpu_reset(schway *inst)
{
    struct schway__gpu *g = &inst->gpu;
    memset(g, 0, sizeof *g);
    g->display_disabled = 1u;
    g->horizontal_range = 0x00C60260u;
    g->vertical_range = 0x00040010u;
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
        uint32_t variable_size = (op & 0x03u) == 0u ? 1u : 0u;
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

static void parser_idle(struct schway__gpu *g)
{
    memset(g->fifo, 0, sizeof g->fifo);
    g->fifo_count = 0u;
    g->packet_words = 0u;
    g->phase = SCHWAY__GPU_IDLE;
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
        if ((g->mask & 2u) == 0u || (old & 0x8000u) == 0u) {
            inst->vram[y][x] = (uint16_t)(color | ((g->mask & 1u) != 0u ? 0x8000u : 0u));
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
        if (g->fifo_count < SCHWAY__GPU_FIFO_WORDS) {
            status |= UINT32_C(1) << 25;
        }
        break;
    case 2u:
        if (g->phase == SCHWAY__GPU_IDLE) {
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
    if (g->phase == SCHWAY__GPU_IDLE) {
        status |= (UINT32_C(1) << 26) | (UINT32_C(1) << 28);
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

static void process_packet(schway *inst)
{
    struct schway__gpu *g = &inst->gpu;
    uint32_t command = g->fifo[0];
    uint32_t op = command >> 24;
    uint32_t word_count = g->fifo_count;
    if (op == 0x02u && word_count == 3u) {
        fill_vram(inst, command, g->fifo[1], g->fifo[2]);
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
    } else if (op >= 0x40u && op <= 0x5Fu && (op & 0x08u) != 0u) {
        g->polyline_gouraud = (op & 0x10u) != 0u ? 1u : 0u;
        g->polyline_color = command & 0x00FFFFFFu;
        parser_idle(g);
        g->phase = g->polyline_gouraud != 0u ? SCHWAY__GPU_POLYLINE_COLOR
                                             : SCHWAY__GPU_POLYLINE_VERTEX;
        return;
    }
    parser_idle(g);
}

static int polyline_terminator(uint32_t word)
{
    return (word & 0xF000F000u) == 0x50005000u;
}

static void polyline_word(struct schway__gpu *g, uint32_t word)
{
    if (g->phase == SCHWAY__GPU_POLYLINE_COLOR) {
        if (polyline_terminator(word)) {
            parser_idle(g);
            return;
        }
        g->polyline_color = word & 0x00FFFFFFu;
        g->phase = SCHWAY__GPU_POLYLINE_VERTEX;
        return;
    }
    if (polyline_terminator(word)) {
        parser_idle(g);
    } else if (g->polyline_gouraud != 0u) {
        g->phase = SCHWAY__GPU_POLYLINE_COLOR;
    }
}

/* CPU writes and DMA2 enter here, one bounded word at a time. */
void schway__gpu_word(schway *inst, uint32_t word)
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
        polyline_word(g, word);
        return;
    }
    if (g->phase == SCHWAY__GPU_PACKET) {
        if (g->fifo_count >= SCHWAY__GPU_FIFO_WORDS || g->fifo_count >= g->packet_words) {
            packet_clear(g);
            return;
        }
        g->fifo[g->fifo_count++] = word;
        if (g->fifo_count == g->packet_words) {
            process_packet(inst);
        }
        return;
    }
    g->fifo[0] = word;
    g->fifo_count = 1u;
    g->packet_words = command_words(word);
    if (g->packet_words > SCHWAY__GPU_FIFO_WORDS || g->packet_words == 0u) {
        packet_clear(g);
    } else if (g->packet_words == 1u) {
        process_packet(inst);
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

static void gpu_gp1(schway *inst, uint32_t command)
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
        break;
    }
    case 0x01u:
        packet_clear(g);
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
    if (wr != 0u) {
        uint32_t word = (*value & lane_mask(width)) << lane_shift(addr);
        if (slot == 0u) {
            schway__gpu_word(inst, word);
        } else {
            gpu_gp1(inst, word);
        }
    } else {
        if (schway__gpu_peek(inst, addr, width, value) != 0) {
            return 1;
        }
        if (slot == 0u && inst->gpu.phase == SCHWAY__GPU_TRANSFER_FROM_VRAM &&
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
    *cycles += 1u;
    return 0;
}

static uint32_t expected_packet_words(uint32_t command)
{
    return command_words(command);
}

/* One state walk owns VRAM and every field that can affect command decoding
   or a partially completed transfer. */
void schway__gpu_state_io(struct schway__state_io *io, schway *inst)
{
    struct schway__gpu *g = &inst->gpu;
    schway__state_u16_array(io, name_vram, &inst->vram[0][0], SCHWAY__VRAM_W * SCHWAY__VRAM_H,
                            0u, UINT16_MAX);
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
    (void)schway__state_u32(io, name_polyline_color, &g->polyline_color, 0u, 0x00FFFFFFu);
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
    (void)schway__state_u32(io, name_display_mode, &g->display_mode, 0u, 0xFFu);
    (void)schway__state_u32(io, name_allow_2mb, &g->allow_2mb, 0u, 1u);
    (void)schway__state_u32(io, name_read_latch, &g->read_latch, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_irq_pending, &g->irq_pending, 0u, 1u);

    schway__state_check(io, phase != SCHWAY__GPU_PACKET ||
                                (fifo_count != 0u && packet_words > fifo_count &&
                                 packet_words == expected_packet_words(packet_command)));
    schway__state_check(io, phase == SCHWAY__GPU_PACKET || (fifo_count == 0u && packet_words == 0u));
    if (phase == SCHWAY__GPU_IDLE) {
        schway__state_check(io, owed_words == 0u && transfer_x == 0u && transfer_y == 0u &&
                                    transfer_width == 0u && transfer_height == 0u &&
                                    transfer_index == 0u && polyline_gouraud == 0u);
    } else if (phase == SCHWAY__GPU_PACKET) {
        schway__state_check(io, owed_words == 0u && transfer_x == 0u && transfer_y == 0u &&
                                    transfer_width == 0u && transfer_height == 0u &&
                                    transfer_index == 0u && polyline_gouraud == 0u);
    } else if (phase == SCHWAY__GPU_POLYLINE_VERTEX || phase == SCHWAY__GPU_POLYLINE_COLOR) {
        schway__state_check(io, owed_words == 0u && transfer_x == 0u && transfer_y == 0u &&
                                    transfer_width == 0u && transfer_height == 0u &&
                                    transfer_index == 0u &&
                                    (phase != SCHWAY__GPU_POLYLINE_COLOR || polyline_gouraud != 0u));
    } else if (phase == SCHWAY__GPU_TRANSFER_TO_VRAM ||
               phase == SCHWAY__GPU_TRANSFER_FROM_VRAM) {
        schway__state_check(io, fifo_count == 0u && packet_words == 0u &&
                                    transfer_width != 0u && transfer_width <= SCHWAY__VRAM_W &&
                                    transfer_height != 0u && transfer_height <= SCHWAY__VRAM_H &&
                                    polyline_gouraud == 0u);
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
