#include "frame_hash.h"

#include <string.h>

#include "../host/xrgb8888.h"
#include "sha256.h"

static void put_u32(schway__sha256 *s, uint32_t v)
{
    uint8_t b[4];
    for (uint32_t i = 0; i < 4u; i++) {
        b[i] = (uint8_t)(v >> (8u * i));
    }
    schway__sha256_update(s, b, sizeof b);
}

int schway__runner_frame_hash(const schway_video *video, char hex[65])
{
    if (!schway_host_video_valid(video) || hex == NULL) {
        return 0;
    }
    const uint32_t tuple[13] = {video->field,  video->interlaced, video->vres,   video->hres,
                                video->dot_div, video->region,     video->x1,     video->y1,
                                video->start_x, video->start_y,    video->enabled, video->rgb24,
                                video->bpp};
    schway__sha256 s;
    uint8_t digest[32];
    uint8_t row[2u * SCHWAY_HOST_VIDEO_MAX_WIDTH];
    schway__sha256_init(&s);
    for (size_t i = 0; i < sizeof tuple / sizeof tuple[0]; i++) {
        put_u32(&s, tuple[i]);
    }
    const uint8_t *base = (const uint8_t *)video->pixels;
    for (uint32_t y = 0; y < video->height; y++) {
        const uint8_t *src = base + (size_t)y * video->pitch_bytes;
        if (video->bpp == 24u) {
            schway__sha256_update(&s, src, (size_t)video->width * 3u);
        } else {
            for (uint32_t x = 0; x < video->width; x++) {
                uint16_t word;
                memcpy(&word, src + 2u * x, sizeof word);
                row[2u * x] = (uint8_t)word;
                row[2u * x + 1u] = (uint8_t)(word >> 8);
            }
            schway__sha256_update(&s, row, (size_t)video->width * 2u);
        }
    }
    schway__sha256_final(&s, digest);
    schway__sha256_hex(digest, hex);
    return 1;
}
