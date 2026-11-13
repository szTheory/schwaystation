/* GPU line and rectangle spans share the instance pixel path. */
#include <stdint.h>

#include "internal.h"

/* Signed GP0 coordinates follow docs/hardware/gpu.md; PSX-SPX raster section three. */
static int32_t sign11(uint32_t value)
{
    value &= 0x7FFu;
    return (value & 0x400u) != 0u ? (int32_t)value - 0x800 : (int32_t)value;
}

static uint32_t abs_i32(int32_t value)
{
    return value < 0 ? (uint32_t)(-value) : (uint32_t)value;
}

static int32_t step_sign(int32_t value)
{
    return value < 0 ? -1 : (value > 0 ? 1 : 0);
}

static void color_step(uint32_t color, uint32_t end, uint32_t shift, uint32_t steps,
                       uint32_t *value, uint32_t *error, uint32_t *step,
                       uint32_t *remainder, int32_t *direction)
{
    uint32_t first = (color >> shift) & 0xFFu;
    uint32_t last = (end >> shift) & 0xFFu;
    *value = (first << 12) + 2048u;
    uint32_t delta = (first < last ? last - first : first - last) << 12;
    *direction = first < last ? 1 : (first > last ? -1 : 0);
    *error = 0u;
    *step = steps == 0u ? 0u : delta / steps;
    *remainder = 0u;
}

static void color_advance(uint32_t steps, uint32_t step, uint32_t remainder, int32_t direction,
                          uint32_t *value, uint32_t *error)
{
    if (steps == 0u) {
        return;
    }
    *value = (uint32_t)((int32_t)*value + direction * (int32_t)step);
    *error += remainder;
    if (*error >= steps) {
        *value = (uint32_t)((int32_t)*value + direction);
        *error -= steps;
    }
}

/* GP0 lines include both vertices, use integer major-axis DDA, and reject spans
   above 1023 by 511 before clipping (PSX-SPX raster section three point seven). */
void schway__gpu_line(schway *inst, uint32_t command, uint32_t position0, uint32_t position1,
                      uint32_t color0, uint32_t color1, uint32_t flags)
{
    if (inst == NULL) {
        return;
    }
    struct schway__gpu *g = &inst->gpu;
    int32_t x0 = sign11(position0) + sign11(g->draw_offset);
    int32_t y0 = sign11(position0 >> 16) + sign11(g->draw_offset >> 11);
    int32_t x1 = sign11(position1) + sign11(g->draw_offset);
    int32_t y1 = sign11(position1 >> 16) + sign11(g->draw_offset >> 11);
    int32_t dx_signed = x1 - x0;
    int32_t dy_signed = y1 - y0;
    uint32_t dx = abs_i32(dx_signed);
    uint32_t dy = abs_i32(dy_signed);
    if (dx > 1023u || dy > 511u) {
        return;
    }

    int32_t left = (int32_t)(g->draw_area_tl & 0x3FFu);
    int32_t top = (int32_t)((g->draw_area_tl >> 10) & 0x1FFu);
    int32_t right = (int32_t)(g->draw_area_br & 0x3FFu);
    int32_t bottom = (int32_t)((g->draw_area_br >> 10) & 0x1FFu);
    if (left > right || top > bottom) {
        return;
    }

    uint32_t steps = dx > dy ? dx : dy;
    uint32_t red, green, blue;
    uint32_t red_error, green_error, blue_error;
    uint32_t red_step, green_step, blue_step;
    uint32_t red_remainder, green_remainder, blue_remainder;
    int32_t red_direction, green_direction, blue_direction;
    color_step(color0, color1, 0u, steps, &red, &red_error, &red_step, &red_remainder,
               &red_direction);
    color_step(color0, color1, 8u, steps, &green, &green_error, &green_step, &green_remainder,
               &green_direction);
    color_step(color0, color1, 16u, steps, &blue, &blue_error, &blue_step, &blue_remainder,
               &blue_direction);

    uint32_t op = command >> 24;
    uint32_t pixel_flags = flags | SCHWAY__GPU_PIXEL_DITHER_ELIGIBLE;
    if ((op & 2u) != 0u) {
        pixel_flags |= SCHWAY__GPU_PIXEL_SEMI_TRANSPARENT;
    }
    int32_t sx = step_sign(dx_signed);
    int32_t sy = step_sign(dy_signed);
    int32_t x = x0;
    int32_t y = y0;
    uint32_t minor_error = steps / 2u;
    if (steps != 0u && ((dx >= dy && sy < 0) || (dx < dy && sx > 0))) {
        minor_error = (steps - 1u) / 2u;
    }
    uint32_t minor_delta = dx > dy ? dy : dx;
    for (uint32_t i = 0u; i <= steps; i++) {
        if (x >= left && x <= right && y >= top && y <= bottom) {
            uint32_t color = (red >> 12) | ((green >> 12) << 8) | ((blue >> 12) << 16);
            schway__gpu_pixel(inst, (uint32_t)x, (uint32_t)y, 0u, color, pixel_flags);
        }
        if (i == steps) {
            break;
        }
        if (dx >= dy) {
            x += sx;
            minor_error += minor_delta;
            if (minor_error >= steps) {
                y += sy;
                minor_error -= steps;
            }
        } else {
            y += sy;
            minor_error += minor_delta;
            if (minor_error >= steps) {
                x += sx;
                minor_error -= steps;
            }
        }
        color_advance(steps, red_step, red_remainder, red_direction, &red, &red_error);
        color_advance(steps, green_step, green_remainder, green_direction, &green, &green_error);
        color_advance(steps, blue_step, blue_remainder, blue_direction, &blue, &blue_error);
    }
}

static uint32_t rectangle_flags(uint32_t op)
{
    uint32_t flags = (op & 4u) != 0u ? SCHWAY__GPU_PIXEL_TEXTURED : 0u;
    if ((op & 1u) != 0u) {
        flags |= SCHWAY__GPU_PIXEL_RAW_TEXTURE;
    }
    if ((op & 2u) != 0u) {
        flags |= SCHWAY__GPU_PIXEL_SEMI_TRANSPARENT;
    }
    return flags;
}

/* GP0 rectangles use 10/9-bit dimensions, clip after E5, and never dither.
   Texture flips exist only on v2; see PSX-SPX raster section three point eight. */
void schway__gpu_rectangle(schway *inst, uint32_t command, uint32_t position,
                           uint32_t uv_clut, uint32_t size)
{
    if (inst == NULL) {
        return;
    }
    struct schway__gpu *g = &inst->gpu;
    uint32_t op = command >> 24;
    uint32_t textured = (op & 4u) != 0u;
    uint32_t size_mode = (op >> 3) & 3u;
    uint32_t width = size_mode == 0u ? size & 0x3FFu
                                     : (size_mode == 1u ? 1u : (size_mode == 2u ? 8u : 16u));
    uint32_t height = size_mode == 0u ? (size >> 16) & 0x1FFu : width;
    if (width == 0u || height == 0u) {
        return;
    }

    int32_t x0 = sign11(position) + sign11(g->draw_offset);
    int32_t y0 = sign11(position >> 16) + sign11(g->draw_offset >> 11);
    int32_t left = (int32_t)(g->draw_area_tl & 0x3FFu);
    int32_t top = (int32_t)((g->draw_area_tl >> 10) & 0x1FFu);
    int32_t right = (int32_t)(g->draw_area_br & 0x3FFu);
    int32_t bottom = (int32_t)((g->draw_area_br >> 10) & 0x1FFu);
    uint32_t flags = rectangle_flags(op);
    uint32_t base_u = uv_clut & 0xFFu;
    uint32_t base_v = (uv_clut >> 8) & 0xFFu;
    uint32_t gpu_v0 = (inst->profile_flags & SCHWAY_PROFILE_GPU_V0) != 0u;
    uint32_t flip_x = gpu_v0 == 0u && (g->draw_mode & (1u << 12)) != 0u;
    uint32_t flip_y = gpu_v0 == 0u && (g->draw_mode & (1u << 13)) != 0u;
    for (uint32_t row = 0u; row < height; row++) {
        int32_t y = y0 + (int32_t)row;
        if (y < top || y > bottom) {
            continue;
        }
        for (uint32_t col = 0u; col < width; col++) {
            int32_t x = x0 + (int32_t)col;
            if (x < left || x > right) {
                continue;
            }
            uint16_t texel = 0u;
            if (textured != 0u) {
                uint32_t u = (base_u + (flip_x != 0u ? 256u - col : col)) & 0xFFu;
                uint32_t v = (base_v + (flip_y != 0u ? 256u - row : row)) & 0xFFu;
                texel = schway__gpu_texel(inst, (uv_clut & 0xFFFF0000u) | (v << 8) | u);
            }
            schway__gpu_pixel(inst, (uint32_t)x, (uint32_t)y, texel,
                              command & 0x00FFFFFFu, flags);
        }
    }
}
