/* 15-bit display words to XRGB8888. */
#include "xrgb8888.h"

#include <stddef.h>

/* 5 to 8 bits by replicating the top bits into the bottom: 0 -> 0,
   16 -> 132, 31 -> 255. */
static uint32_t widen(uint32_t c)
{
    return (c << 3) | (c >> 2);
}

uint32_t schway_host_xrgb8888_pixel(uint16_t w)
{
    /* Bit 15 is the mask bit, not colour (psx-spx, GPU Video Memory). */
    uint32_t v = (uint32_t)w & 0x7FFFu;
    uint32_t r = widen(v & 31u);
    uint32_t g = widen((v >> 5) & 31u);
    uint32_t b = widen((v >> 10) & 31u);
    return (r << 16) | (g << 8) | b;
}

void schway_host_xrgb8888_frame(const schway_video *video, uint32_t *out,
                                uint32_t out_pitch_pixels)
{
    const uint8_t *base = (const uint8_t *)video->pixels;
    for (uint32_t y = 0; y < video->height; y++) {
        const uint8_t *row = base + (size_t)y * video->pitch_bytes;
        uint32_t *dst = out + (size_t)y * out_pitch_pixels;
        for (uint32_t x = 0; x < video->width; x++) {
            /* The core's VRAM is native uint16_t. */
            const uint16_t *src = (const uint16_t *)(const void *)(row + 2u * x);
            dst[x] = schway_host_xrgb8888_pixel(*src);
        }
    }
}
