/* Native 15/24-bit display pixels to XRGB8888. */
#include "xrgb8888.h"

#include <stddef.h>
#include <string.h>

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

int schway_host_video_valid(const schway_video *video)
{
    if (video == NULL || video->pixels == NULL || video->width == 0u || video->height == 0u ||
        video->width > SCHWAY_HOST_VIDEO_MAX_WIDTH ||
        video->height > SCHWAY_HOST_VIDEO_MAX_HEIGHT ||
        (video->bpp != 16u && video->bpp != 24u) ||
        video->rgb24 != (video->bpp == 24u)) {
        return 0;
    }

    size_t bytes_per_pixel = video->bpp / 8u;
    size_t row_bytes = (size_t)video->width * bytes_per_pixel;
    if (video->pitch_bytes < row_bytes || video->pitch_bytes > SCHWAY_HOST_VIDEO_MAX_BYTES) {
        return 0;
    }
    size_t rows_before_last = (size_t)video->height - 1u;
    if (rows_before_last > (SIZE_MAX - row_bytes) / video->pitch_bytes) {
        return 0;
    }
    size_t extent = rows_before_last * video->pitch_bytes + row_bytes;
    return extent <= SCHWAY_HOST_VIDEO_MAX_BYTES;
}

int schway_host_xrgb8888_frame(const schway_video *video, uint32_t *out,
                               uint32_t out_pitch_pixels)
{
    if (!schway_host_video_valid(video) || out == NULL || out_pitch_pixels < video->width ||
        out_pitch_pixels > SCHWAY_HOST_VIDEO_MAX_WIDTH) {
        return 0;
    }
    const uint8_t *base = (const uint8_t *)video->pixels;
    for (uint32_t y = 0; y < video->height; y++) {
        const uint8_t *row = base + (size_t)y * video->pitch_bytes;
        uint32_t *dst = out + (size_t)y * out_pitch_pixels;
        for (uint32_t x = 0; x < video->width; x++) {
            if (video->bpp == 16u) {
                /* The core's VRAM is native uint16_t. */
                uint16_t src;
                memcpy(&src, row + 2u * x, sizeof src);
                dst[x] = schway_host_xrgb8888_pixel(src);
            } else {
                const uint8_t *rgb = row + 3u * x;
                dst[x] = ((uint32_t)rgb[0] << 16) | ((uint32_t)rgb[1] << 8) | rgb[2];
            }
        }
    }
    return 1;
}
