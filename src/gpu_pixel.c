/* Pixel order follows PSX-GPU-AND-GTE.md, sections three point one to three point five. */
#include "gpu.h"

#include "internal.h"

static uint16_t min_u16(uint16_t a, uint16_t b)
{
    return a < b ? a : b;
}

/* Apply E2's five-bit masks and offsets before adding the E1 texture page. */
static uint32_t window_coordinate(const struct schway__gpu *g, uint32_t uv, uint32_t axis)
{
    uint32_t shift = axis == 0u ? 0u : 5u;
    uint32_t offset_shift = axis == 0u ? 10u : 15u;
    uint32_t mask = (g->texture_window >> shift) & 0x1Fu;
    uint32_t offset = (g->texture_window >> offset_shift) & 0x1Fu;
    return (uv & 0xFFu & ~(mask * 8u)) | ((offset & mask) * 8u);
}

/* Decode one packed 4/8-bit texel or direct 15-bit word through the GPU caches. */
uint16_t schway__gpu_texel(schway *inst, uint32_t uv_clut)
{
    if (inst == NULL) {
        return 0u;
    }
    const struct schway__gpu *g = &inst->gpu;
    uint32_t u = window_coordinate(g, uv_clut, 0u);
    uint32_t v = window_coordinate(g, uv_clut >> 8, 1u);
    uint32_t page_x = (g->draw_mode & 0xFu) << 6;
    uint32_t page_y = ((g->draw_mode >> 4) & 1u) << 8;
    uint32_t depth = (g->draw_mode >> 7) & 3u;
    uint32_t clut = uv_clut >> 16;
    uint32_t clut_x = (clut & 0x3Fu) << 4;
    uint32_t clut_y = (clut >> 6) & 0x1FFu;

    if (depth == 0u) {
        uint16_t packed = schway__gpu_texture_read(inst, page_x + (u >> 2), page_y + v);
        uint32_t index = (packed >> ((u & 3u) * 4u)) & 0xFu;
        return schway__gpu_clut_read(inst, clut_x, clut_y, index, 16u);
    }
    if (depth == 1u) {
        uint16_t packed = schway__gpu_texture_read(inst, page_x + (u >> 1), page_y + v);
        uint32_t index = (packed >> ((u & 1u) * 8u)) & 0xFFu;
        return schway__gpu_clut_read(inst, clut_x, clut_y, index,
                                     SCHWAY__GPU_CLUT_CACHE_ENTRIES);
    }
    /* Reserved depth 3 follows the documented direct-15-bit model. */
    return schway__gpu_texture_read(inst, page_x + u, page_y + v);
}

static uint32_t channel8(uint32_t color, uint32_t shift)
{
    return (color >> shift) & 0xFFu;
}

static uint32_t modulate_channel(uint32_t texel, uint32_t color8)
{
    uint32_t value = (texel * color8) >> 4;
    return value > 0xFFu ? 0xFFu : value;
}

static uint32_t source_channel(uint16_t texel, uint32_t color, uint32_t flags,
                              uint32_t component)
{
    uint32_t color8 = channel8(color, component * 8u);
    if ((flags & SCHWAY__GPU_PIXEL_TEXTURED) == 0u) {
        return color8;
    }
    uint32_t texel5 = ((uint32_t)texel >> (component * 5u)) & 0x1Fu;
    if ((flags & SCHWAY__GPU_PIXEL_RAW_TEXTURE) != 0u) {
        return texel5 << 3;
    }
    return modulate_channel(texel5, color8);
}

static int32_t dither_value(uint32_t x, uint32_t y)
{
    static const int8_t matrix[4][4] = {
        {-4, 0, -3, 1}, {2, -2, 3, -1}, {-3, 1, -4, 0}, {3, -1, 2, -2}};
    return matrix[y & 3u][x & 3u];
}

static uint32_t quantize_channel(uint32_t value, int32_t dither)
{
    int32_t adjusted = (int32_t)value + dither;
    if (adjusted < 0) {
        adjusted = 0;
    } else if (adjusted > 255) {
        adjusted = 255;
    }
    return (uint32_t)adjusted >> 3;
}

static uint32_t blend_channel(uint32_t background, uint32_t foreground, uint32_t mode)
{
    uint32_t value;
    switch (mode & 3u) {
    case 0u:
        value = (background + foreground) / 2u;
        break;
    case 1u:
        value = background + foreground;
        break;
    case 2u:
        return background > foreground ? background - foreground : 0u;
    default:
        value = background + foreground / 4u;
        break;
    }
    return min_u16((uint16_t)value, 31u);
}

static uint16_t blend_rgb555(uint16_t background, uint16_t foreground, uint32_t mode)
{
    uint32_t red = blend_channel(background & 0x1Fu, foreground & 0x1Fu, mode);
    uint32_t green = blend_channel((background >> 5) & 0x1Fu,
                                   (foreground >> 5) & 0x1Fu, mode);
    uint32_t blue = blend_channel((background >> 10) & 0x1Fu,
                                  (foreground >> 10) & 0x1Fu, mode);
    return (uint16_t)(red | (green << 5) | (blue << 10));
}

/* Write one clipped pixel, applying ABE, PBC and PBW to native RGB555. */
void schway__gpu_pixel(schway *inst, uint32_t x, uint32_t y, uint16_t texel, uint32_t color,
                       uint32_t flags)
{
    if (inst == NULL) {
        return;
    }
    const struct schway__gpu *g = &inst->gpu;
    uint32_t textured = flags & SCHWAY__GPU_PIXEL_TEXTURED;
    if (textured != 0u && texel == 0u) {
        return;
    }

    uint32_t red8 = source_channel(texel, color, flags, 0u);
    uint32_t green8 = source_channel(texel, color, flags, 1u);
    uint32_t blue8 = source_channel(texel, color, flags, 2u);
    uint32_t dither_on = (flags & SCHWAY__GPU_PIXEL_DITHER_ELIGIBLE) != 0u &&
                         (g->draw_mode & (1u << 9)) != 0u &&
                         !(textured != 0u &&
                           (flags & (SCHWAY__GPU_PIXEL_RAW_TEXTURE |
                                     SCHWAY__GPU_PIXEL_SEMI_TRANSPARENT)) ==
                               (SCHWAY__GPU_PIXEL_RAW_TEXTURE |
                                SCHWAY__GPU_PIXEL_SEMI_TRANSPARENT));
    int32_t dither = dither_on != 0u ? dither_value(x, y) : 0;
    uint32_t foreground_red = quantize_channel(red8, dither);
    uint32_t foreground_green = quantize_channel(green8, dither);
    uint32_t foreground_blue = quantize_channel(blue8, dither);
    uint16_t foreground = (uint16_t)(foreground_red | (foreground_green << 5) |
                                     (foreground_blue << 10));
    uint16_t destination = inst->vram[y & 0x1FFu][x & 0x3FFu];
    if ((g->mask & 2u) != 0u && (destination & 0x8000u) != 0u) {
        return;
    }
    uint32_t semi = (flags & SCHWAY__GPU_PIXEL_SEMI_TRANSPARENT) != 0u;
    uint32_t blend = semi != 0u && (textured == 0u || (texel & 0x8000u) != 0u);
    if (blend != 0u) {
        foreground = blend_rgb555(destination, foreground, (g->draw_mode >> 5) & 3u);
    }
    uint16_t mask_bit = (g->mask & 1u) != 0u ? 0x8000u : 0u;
    uint16_t stp_bit = textured != 0u ? (uint16_t)(texel & 0x8000u) : 0u;
    inst->vram[y & 0x1FFu][x & 0x3FFu] = (uint16_t)(foreground | mask_bit | stp_bit);
}
