/* Polygon interpolation tests use independently tabulated packet pixels. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus.h"
#include "internal.h"

#define GP0 0x1F801810u

static int failures;

#define CHECK(expr)                                                                               \
    do {                                                                                          \
        if (!(expr)) {                                                                            \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr);                          \
            failures++;                                                                           \
        }                                                                                         \
    } while (0)

static void *plain_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void plain_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static schway *make_instance(void)
{
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = plain_alloc;
    cfg.allocator.free = plain_free;
    schway *inst = NULL;
    CHECK(schway_create(&cfg, &inst) == SCHWAY_STATUS_OK);
    return inst;
}

static void write_gp0(schway *inst, uint32_t word)
{
    uint32_t cycles = 0u;
    CHECK(schway__bus_write(inst, GP0, 4u, word, &cycles) == 0);
}

static uint32_t vertex(uint32_t x, uint32_t y)
{
    return (x & 0x7FFu) | ((y & 0x7FFu) << 16);
}

static uint32_t uv(uint32_t u, uint32_t v, uint32_t high)
{
    return (u & 0xFFu) | ((v & 0xFFu) << 8) | ((high & 0xFFFFu) << 16);
}

static void draw_area(schway *inst, uint32_t left, uint32_t top, uint32_t right,
                      uint32_t bottom)
{
    write_gp0(inst, 0xE3000000u | (left & 0x3FFu) | ((top & 0x1FFu) << 10));
    write_gp0(inst, 0xE4000000u | (right & 0x3FFu) | ((bottom & 0x1FFu) << 10));
}

static void draw_gouraud_untextured(schway *inst)
{
    write_gp0(inst, 0x30000080u);
    write_gp0(inst, vertex(1u, 1u));
    write_gp0(inst, 0x00008000u);
    write_gp0(inst, vertex(9u, 1u));
    write_gp0(inst, 0x00800000u);
    write_gp0(inst, vertex(1u, 9u));
}

static void case_gouraud_textured(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return;
    }
    draw_area(inst, 0u, 0u, 1023u, 511u);
    inst->gpu.draw_mode = 0u;
    inst->vram[2][64] = 0x3210u;
    inst->vram[4][33] = 0x7FFFu;
    inst->vram[4][34] = 0x7C00u;

    write_gp0(inst, 0x34000080u);
    write_gp0(inst, vertex(1u, 1u));
    write_gp0(inst, uv(0u, 2u, 0x0102u));
    write_gp0(inst, 0x00008000u);
    write_gp0(inst, vertex(9u, 1u));
    write_gp0(inst, uv(8u, 2u, 0x0001u));
    write_gp0(inst, 0x00800000u);
    write_gp0(inst, vertex(1u, 9u));
    write_gp0(inst, uv(0u, 2u, 0u));

    /* At (2,2), barycentric weights are 5/8, 3/16, 3/16 and U=1.5. */
    CHECK(inst->vram[2][64] == 0x3210u);
    CHECK((inst->gpu.draw_mode & 0x1FFu) == 1u);
    CHECK(inst->vram[2][2] == 0x14B3u);
    /* At (3,2), weights are 1/2, 5/16, 3/16 and U=2.5. */
    CHECK(inst->vram[2][3] == 0x1400u);
    schway_destroy(inst);
}

static void case_texture_page_clut(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return;
    }
    draw_area(inst, 0u, 0u, 1023u, 511u);
    inst->gpu.draw_mode = 0x0205u;
    inst->vram[5][65] = 3u;
    inst->vram[4][35] = 0x001Fu;
    inst->vram[0][16] = 0x03E0u;

    write_gp0(inst, 0x250000FFu);
    write_gp0(inst, vertex(1u, 1u));
    write_gp0(inst, uv(4u, 5u, 0x0102u));
    write_gp0(inst, vertex(9u, 1u));
    write_gp0(inst, uv(4u, 5u, 0x0001u));
    write_gp0(inst, vertex(1u, 9u));
    write_gp0(inst, uv(4u, 5u, 0u));

    CHECK((inst->gpu.draw_mode & 0x1FFu) == 1u);
    CHECK((inst->gpu.draw_mode & ~0x1FFu) == 0x0200u);
    CHECK(inst->vram[2][2] == 0x001Fu);
    schway_destroy(inst);
}

static void case_clip_interpolation(void)
{
    schway *clipped = make_instance();
    schway *full = make_instance();
    if (clipped == NULL || full == NULL) {
        schway_destroy(clipped);
        schway_destroy(full);
        return;
    }
    draw_area(clipped, 3u, 2u, 6u, 6u);
    draw_area(full, 0u, 0u, 1023u, 511u);
    draw_gouraud_untextured(clipped);
    draw_gouraud_untextured(full);

    /* The clipped span starts at original geometry sample (3.5,2.5). */
    CHECK(clipped->vram[2][3] == 0x0CA8u);
    for (uint32_t y = 2u; y <= 6u; y++) {
        for (uint32_t x = 3u; x <= 6u; x++) {
            CHECK(clipped->vram[y][x] == full->vram[y][x]);
        }
    }
    schway_destroy(clipped);
    schway_destroy(full);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-gpu-interpolation <case>\n");
        return 2;
    }
    if (strcmp(argv[1], "gouraud_textured") == 0) {
        case_gouraud_textured();
    } else if (strcmp(argv[1], "texture_page_clut") == 0) {
        case_texture_page_clut();
    } else if (strcmp(argv[1], "clip_interpolation") == 0) {
        case_clip_interpolation();
    } else {
        fprintf(stderr, "unknown GPU interpolation case: %s\n", argv[1]);
        return 2;
    }
    return failures == 0 ? 0 : 1;
}
