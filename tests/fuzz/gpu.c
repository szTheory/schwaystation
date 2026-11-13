/* Bounded replay of hostile GPU bus words through the existing replay driver.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"
#include "schwaystation.h"

#define GP0 0x1F801810u
#define GP1 0x1F801814u

static void broken(const char *what)
{
    fprintf(stderr, "fuzz.regress.gpu: %s\n", what);
    abort();
}

static void *alloc_bytes(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void free_bytes(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static schway *make(void)
{
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = alloc_bytes;
    cfg.allocator.free = free_bytes;
    schway *inst = NULL;
    if (schway_create(&cfg, &inst) != SCHWAY_STATUS_OK || inst == NULL) {
        broken("instance creation failed");
    }
    return inst;
}

static void write_word(schway *inst, uint32_t port, uint32_t word)
{
    uint32_t cycles = 0u;
    if (schway__bus_write(inst, port, 4u, word, &cycles) != 0u) {
        broken("GPU bus write was refused");
    }
}

static uint32_t read_word(schway *inst, uint32_t port)
{
    uint32_t cycles = 0u;
    uint32_t word = 0u;
    if (schway__bus_read(inst, port, 4u, &word, &cycles) != 0u) {
        broken("GPU bus read was refused");
    }
    return word;
}

static void full_draw_area(schway *inst)
{
    write_word(inst, GP0, 0xE3000000u);
    write_word(inst, GP0, 0xE407FFFFu);
}

static void public_bound(schway *inst)
{
    uint64_t before = inst->ticks;
    schway_budget budget;
    schway_run_result result;
    memset(&budget, 0, sizeof budget);
    memset(&result, 0, sizeof result);
    budget.size = (uint32_t)sizeof budget;
    budget.max_cycles = 32u;
    result.size = (uint32_t)sizeof result;
    if (schway_run(inst, &budget, &result) != SCHWAY_STATUS_OK ||
        (result.stop != SCHWAY_STOP_BUDGET && result.stop != SCHWAY_STOP_FRAME &&
         result.stop != SCHWAY_STOP_FAULT) ||
        result.ticks < before || result.ticks - before > budget.max_cycles) {
        broken("public run did not stop within its 32-cycle budget");
    }
}

static void pixel_is(schway *inst, uint32_t x, uint32_t y, uint16_t expected)
{
    schway_span vram;
    if (schway_get_vram(inst, &vram) != SCHWAY_STATUS_OK ||
        vram.size != SCHWAY__VRAM_W * SCHWAY__VRAM_H * sizeof(uint16_t)) {
        broken("public VRAM span is invalid");
    }
    const uint16_t *pixels = (const uint16_t *)vram.data;
    if (pixels[y * SCHWAY__VRAM_W + x] != expected) {
        fprintf(stderr, "pixel (%u,%u): got %04x expected %04x\n", x, y,
                pixels[y * SCHWAY__VRAM_W + x], expected);
        broken("independent VRAM pixel outcome changed");
    }
}

static void fill(schway *inst, uint32_t color, uint32_t x, uint32_t y, uint32_t width,
                 uint32_t height)
{
    write_word(inst, GP0, 0x02000000u | color);
    write_word(inst, GP0, ((y & 0x1FFu) << 16) | (x & 0x3FFu));
    write_word(inst, GP0, ((height & 0x1FFu) << 16) | (width & 0x3FFu));
}

static uint32_t position(int32_t x, int32_t y)
{
    return (((uint32_t)y & 0x7FFu) << 16) | ((uint32_t)x & 0x7FFu);
}

static void triangle(schway *inst, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2,
                     int32_t y2)
{
    write_word(inst, GP0, 0x200000F8u);
    write_word(inst, GP0, position(x0, y0));
    write_word(inst, GP0, position(x1, y1));
    write_word(inst, GP0, position(x2, y2));
}

static schway_run_result run_budget(schway *inst, uint64_t cycles)
{
    schway_budget budget;
    schway_run_result result;
    memset(&budget, 0, sizeof budget);
    memset(&result, 0, sizeof result);
    budget.size = (uint32_t)sizeof budget;
    budget.max_cycles = cycles;
    result.size = (uint32_t)sizeof result;
    if (schway_run(inst, &budget, &result) != SCHWAY_STATUS_OK ||
        result.stop != SCHWAY_STOP_BUDGET) {
        broken("public run did not stop at its requested budget");
    }
    return result;
}

static void split_public_run_state(void)
{
    schway *split = make();
    schway *whole = make();
    full_draw_area(split);
    full_draw_area(whole);
    fill(split, 0xF8u, 10u, 10u, 2u, 2u);
    fill(whole, 0xF8u, 10u, 10u, 2u, 2u);

    uint64_t start = split->ticks;
    schway_run_result first = run_budget(split, 13u);
    if (first.ticks < start || first.ticks - start != 13u) {
        broken("first split run ignored its elapsed-cycle budget");
    }
    start = split->ticks;
    schway_run_result second = run_budget(split, 19u);
    if (second.ticks < start || second.ticks - start != 19u) {
        broken("second split run ignored its elapsed-cycle budget");
    }
    uint64_t whole_start = whole->ticks;
    schway_run_result unsplit = run_budget(whole, 32u);
    if (unsplit.ticks < whole_start || unsplit.ticks - whole_start != 32u ||
        second.ticks != unsplit.ticks || split->frame != whole->frame) {
        broken("split and unsplit public runs ended at different machine times");
    }

    size_t split_size = schway_state_size(split);
    size_t whole_size = schway_state_size(whole);
    uint8_t *split_state = (uint8_t *)malloc(split_size);
    uint8_t *whole_state = (uint8_t *)malloc(whole_size);
    schway_span split_vram;
    schway_span whole_vram;
    if (split_state == NULL || whole_state == NULL || split_size != whole_size ||
        schway_save_state(split, split_state, split_size) != SCHWAY_STATUS_OK ||
        schway_save_state(whole, whole_state, whole_size) != SCHWAY_STATUS_OK ||
        memcmp(split_state, whole_state, split_size) != 0 ||
        schway_get_vram(split, &split_vram) != SCHWAY_STATUS_OK ||
        schway_get_vram(whole, &whole_vram) != SCHWAY_STATUS_OK ||
        split_vram.size != whole_vram.size ||
        memcmp(split_vram.data, whole_vram.data, split_vram.size) != 0) {
        free(whole_state);
        free(split_state);
        broken("split and unsplit public runs produced different state or VRAM");
    }
    free(whole_state);
    free(split_state);
    pixel_is(split, 10u, 10u, 0x001Fu);
    pixel_is(whole, 10u, 10u, 0x001Fu);
    schway_destroy(whole);
    schway_destroy(split);
}

static void scenario(schway *a, schway *b, uint8_t mode)
{
    if (mode == 0u) {
        /* A one-word triangle packet is deliberately incomplete and survives a
         * state copy. */
        full_draw_area(a);
        write_word(a, GP0, 0x200000F8u);
        size_t n = schway_state_size(a);
        uint8_t *state = (uint8_t *)malloc(n);
        if (state == NULL || schway_save_state(a, state, n) != SCHWAY_STATUS_OK ||
            schway_load_state(b, state, n) != SCHWAY_STATUS_OK ||
            b->gpu.phase != SCHWAY__GPU_PACKET || b->gpu.fifo_count != 1u) {
            free(state);
            broken("partial GPU packet did not round-trip through state");
        }
        free(state);
        /* Finish the same triangle on both instances and pin its RGB555 pixels. */
        const uint32_t vertices[] = {position(1, 1), position(5, 1), position(1, 5)};
        for (size_t i = 0u; i < sizeof vertices / sizeof vertices[0]; ++i) {
            write_word(a, GP0, vertices[i]);
            write_word(b, GP0, vertices[i]);
        }
        pixel_is(a, 1u, 1u, 0x001Fu);
        pixel_is(a, 4u, 1u, 0x001Fu);
        pixel_is(a, 4u, 2u, 0u);
        pixel_is(a, 1u, 4u, 0x001Fu);
        pixel_is(a, 2u, 4u, 0u);
        pixel_is(b, 1u, 1u, 0x001Fu);
        pixel_is(b, 4u, 1u, 0x001Fu);
        pixel_is(b, 4u, 2u, 0u);
        pixel_is(b, 1u, 4u, 0x001Fu);
        pixel_is(b, 2u, 4u, 0u);
        split_public_run_state();
    } else if (mode == 1u) {
        /* VRAM upload/readback crosses both wrap edges with a known four-word
         * payload. */
        write_word(a, GP0, 0xA0000000u);
        write_word(a, GP0, (511u << 16) | 1023u);
        write_word(a, GP0, (2u << 16) | 2u);
        write_word(a, GP0, 0x22221111u);
        write_word(a, GP0, 0x44443333u);
        pixel_is(a, 1023u, 511u, 0x1111u);
        pixel_is(a, 0u, 511u, 0x2222u);
        pixel_is(a, 1023u, 0u, 0x3333u);
        pixel_is(a, 0u, 0u, 0x4444u);
        write_word(a, GP0, 0xC0000000u);
        write_word(a, GP0, (511u << 16) | 1023u);
        write_word(a, GP0, (2u << 16) | 2u);
        if (read_word(a, GP0) != 0x22221111u || read_word(a, GP0) != 0x44443333u) {
            broken("VRAM readback did not preserve uploaded words");
        }
    } else if (mode == 2u) {
        /* DMA2 linked-list command words use the same bounded parser as GP0. */
        write_word(a, GP1, 0x04000001u);
        if (schway__gpu_dma_request(a) == 0u) {
            broken("DMA request was not asserted while the parser was idle");
        }
        schway__gpu_dma_write(a, 0x020000F8u, a->ticks);
        schway__gpu_dma_write(a, (7u << 16) | 19u, a->ticks);
        schway__gpu_dma_write(a, (2u << 16) | 17u, a->ticks);
        pixel_is(a, 16u, 7u, 0x001Fu);
        pixel_is(a, 47u, 8u, 0x001Fu);
        if ((read_word(a, GP1) & ((1u << 26) | (1u << 28))) != 0u) {
            broken("GPUSTAT reported readiness before the raster deadline");
        }
    } else if (mode == 3u) {
        /* The one-unit-over span is rejected; its adjacent legal span draws. */
        full_draw_area(a);
        triangle(a, -1, 0, 1023, 0, -1, 511);
        pixel_is(a, 0u, 0u, 0u);
        triangle(a, 0, 0, 1023, 0, 0, 511);
        pixel_is(a, 0u, 0u, 0x001Fu);
        pixel_is(a, 1u, 1u, 0x001Fu);
        fill(a, 0xF8u, 1023u, 511u, 17u, 2u);
        pixel_is(a, 1023u, 511u, 0x001Fu);
        pixel_is(a, 0u, 511u, 0x001Fu);
        pixel_is(a, 1023u, 0u, 0x001Fu);
        pixel_is(a, 0u, 0u, 0x001Fu);
    } else {
        fill(a, 0xF8u, 5u, 6u, 2u, 1u);
        fill(b, 0xF80000u, 5u, 6u, 2u, 1u);
        pixel_is(a, 5u, 6u, 0x001Fu);
        pixel_is(b, 5u, 6u, 0x7C00u);
        if (schway_reset(a, SCHWAY_RESET_POWER) != SCHWAY_STATUS_OK) {
            broken("power reset failed");
        }
        pixel_is(a, 5u, 6u, 0u);
        pixel_is(b, 5u, 6u, 0x7C00u);
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    schway *a = make();
    schway *b = make();
    uint8_t mode = size == 0u ? 0u : data[0] % 5u;
    scenario(a, b, mode);
    public_bound(a);
    schway_destroy(b);
    schway_destroy(a);
    return 0;
}

#define SCHWAY_REPLAY_NAME "fuzz.regress.gpu"
#include "replay_main.h"
