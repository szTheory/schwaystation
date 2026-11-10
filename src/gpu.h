/* The graphics processor: register/packet state embedded in each instance. */
#ifndef SCHWAYSTATION_GPU_H
#define SCHWAYSTATION_GPU_H

#include <stdint.h>

#include "schwaystation.h"

struct schway__state_io;

#define SCHWAY__GPU_FIFO_WORDS 16u
#define SCHWAY__GPU_TEXTURE_CACHE_LINES 256u
#define SCHWAY__GPU_TEXTURE_CACHE_PIXELS 4u
#define SCHWAY__GPU_CLUT_CACHE_ENTRIES 256u

#define SCHWAY__GPU_PIXEL_TEXTURED (1u << 0)
#define SCHWAY__GPU_PIXEL_RAW_TEXTURE (1u << 1)
#define SCHWAY__GPU_PIXEL_SEMI_TRANSPARENT (1u << 2)
#define SCHWAY__GPU_PIXEL_DITHER_ELIGIBLE (1u << 3)

enum schway__gpu_phase {
    SCHWAY__GPU_IDLE = 0u,
    SCHWAY__GPU_PACKET,
    SCHWAY__GPU_POLYLINE_VERTEX,
    SCHWAY__GPU_POLYLINE_COLOR,
    SCHWAY__GPU_TRANSFER_TO_VRAM,
    SCHWAY__GPU_TRANSFER_FROM_VRAM
};

struct schway__gpu {
    uint32_t fifo[SCHWAY__GPU_FIFO_WORDS];
    uint32_t fifo_count;
    uint32_t packet_words;
    uint32_t phase;
    uint32_t owed_words;
    uint32_t transfer_x, transfer_y, transfer_width, transfer_height, transfer_index;
    uint32_t polyline_gouraud;
    uint32_t polyline_color;
    uint32_t draw_mode;
    uint32_t texture_window;
    uint32_t draw_area_tl, draw_area_br;
    uint32_t draw_offset;
    uint32_t mask;
    uint32_t display_disabled;
    uint32_t dma_direction;
    uint32_t display_start;
    uint32_t horizontal_range, vertical_range;
    uint32_t display_mode;
    uint32_t allow_2mb;
    uint32_t read_latch;
    uint32_t irq_pending;
    uint16_t texture_cache[SCHWAY__GPU_TEXTURE_CACHE_LINES][SCHWAY__GPU_TEXTURE_CACHE_PIXELS];
    uint32_t texture_tags[SCHWAY__GPU_TEXTURE_CACHE_LINES];
    uint8_t texture_valid[SCHWAY__GPU_TEXTURE_CACHE_LINES];
    uint16_t clut_cache[SCHWAY__GPU_CLUT_CACHE_ENTRIES];
    uint32_t clut_x, clut_y, clut_count, clut_valid;
};

void schway__gpu_reset(schway *inst);
void schway__gpu_word(schway *inst, uint32_t word);
int schway__gpu_io(schway *inst, uint32_t wr, uint32_t addr, uint32_t width,
                   uint32_t *value, uint32_t *cycles);
int schway__gpu_peek(const schway *inst, uint32_t addr, uint32_t width, uint32_t *value);
uint16_t schway__gpu_texture_read(schway *inst, uint32_t x, uint32_t y);
uint16_t schway__gpu_clut_read(schway *inst, uint32_t x, uint32_t y, uint32_t index,
                               uint32_t count);
uint16_t schway__gpu_texel(schway *inst, uint32_t uv_clut);
void schway__gpu_pixel(schway *inst, uint32_t x, uint32_t y, uint16_t texel, uint32_t color,
                       uint32_t flags);
void schway__gpu_state_io(struct schway__state_io *io, schway *inst);

#endif /* SCHWAYSTATION_GPU_H */
