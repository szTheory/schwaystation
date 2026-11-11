/* 15-bit display words to XRGB8888, shared by the runner and the libretro
   core. Outside the core's determinism boundary: nothing here is hashed. */
#ifndef SCHWAYSTATION_HOST_XRGB8888_H
#define SCHWAYSTATION_HOST_XRGB8888_H

#include <stdint.h>

#include "schwaystation.h"

#define SCHWAY_HOST_VIDEO_MAX_WIDTH 1024u
#define SCHWAY_HOST_VIDEO_MAX_HEIGHT 512u
#define SCHWAY_HOST_VIDEO_MAX_BYTES (SCHWAY_HOST_VIDEO_MAX_WIDTH * SCHWAY_HOST_VIDEO_MAX_HEIGHT * 3u)

/* One word 0BBBBBGGGGGRRRRR (bit 15 ignored) as 0x00RRGGBB, each channel
   widened from 5 to 8 bits by bit replication. */
uint32_t schway_host_xrgb8888_pixel(uint16_t w);

/* Whether a native 15/24-bit video view has bounded, readable row extents. */
int schway_host_video_valid(const schway_video *video);

/* Converts video's width x height rectangle into out, out_pitch_pixels
   apart per row. Returns 1 when the input and output extents are valid. */
int schway_host_xrgb8888_frame(const schway_video *video, uint32_t *out,
                               uint32_t out_pitch_pixels);

#endif /* SCHWAYSTATION_HOST_XRGB8888_H */
