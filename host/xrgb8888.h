/* 15-bit display words to XRGB8888, shared by the runner and the libretro
   core. Outside the core's determinism boundary: nothing here is hashed. */
#ifndef SCHWAYSTATION_HOST_XRGB8888_H
#define SCHWAYSTATION_HOST_XRGB8888_H

#include <stdint.h>

#include "schwaystation.h"

/* One word 0BBBBBGGGGGRRRRR (bit 15 ignored) as 0x00RRGGBB, each channel
   widened from 5 to 8 bits by bit replication. */
uint32_t schway_host_xrgb8888_pixel(uint16_t w);

/* Converts video's width x height rectangle into out, out_pitch_pixels
   apart per row. */
void schway_host_xrgb8888_frame(const schway_video *video, uint32_t *out,
                                uint32_t out_pitch_pixels);

#endif /* SCHWAYSTATION_HOST_XRGB8888_H */
