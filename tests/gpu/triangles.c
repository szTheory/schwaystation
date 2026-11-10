/* Flat polygon tests send GP0 packets and check native RGB555 VRAM footprints. */
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

static uint32_t vertex(int32_t x, int32_t y)
{
    return ((uint32_t)x & 0x7FFu) | (((uint32_t)y & 0x7FFu) << 16);
}

static void full_draw_area(schway *inst)
{
    write_gp0(inst, 0xE3000000u);
    write_gp0(inst, 0xE407FFFFu);
}

static void triangle(schway *inst, uint32_t opcode, uint32_t color,
                     const int32_t points[3][2])
{
    write_gp0(inst, (opcode << 24) | (color & 0x00FFFFFFu));
    write_gp0(inst, vertex(points[0][0], points[0][1]));
    write_gp0(inst, vertex(points[1][0], points[1][1]));
    write_gp0(inst, vertex(points[2][0], points[2][1]));
}

static void quad(schway *inst, uint32_t opcode, uint32_t color,
                 const int32_t points[4][2])
{
    write_gp0(inst, (opcode << 24) | (color & 0x00FFFFFFu));
    for (uint32_t i = 0u; i < 4u; i++) {
        write_gp0(inst, vertex(points[i][0], points[i][1]));
    }
}

static uint32_t in_right_triangle_footprint(uint32_t x, uint32_t y)
{
    /* Independent hand-enumeration for (1,1),(5,1),(1,5), not a rasterizer. */
    static const uint8_t covered[4][4] = {
        {1u, 1u, 1u, 0u}, {1u, 1u, 0u, 0u}, {1u, 0u, 0u, 0u}, {0u, 0u, 0u, 0u}};
    if (x < 1u || x > 4u || y < 1u || y > 4u) {
        return 0u;
    }
    return covered[y - 1u][x - 1u];
}

static void case_flat_packet(void)
{
    static const int32_t points[3][2] = {{1, 1}, {5, 1}, {1, 5}};
    static const uint32_t permutations[6][3] = {
        {0u, 1u, 2u}, {0u, 2u, 1u}, {1u, 0u, 2u},
        {1u, 2u, 0u}, {2u, 0u, 1u}, {2u, 1u, 0u}};
    schway *inst = make_instance();
    if (inst == NULL) {
        return;
    }
    full_draw_area(inst);

    int32_t ordered[3][2];
    for (uint32_t order = 0u; order < 6u; order++) {
        memset(inst->vram, 0, sizeof inst->vram);
        for (uint32_t i = 0u; i < 3u; i++) {
            ordered[i][0] = points[permutations[order][i]][0];
            ordered[i][1] = points[permutations[order][i]][1];
        }
        triangle(inst, 0x20u, 0x000000FFu, (const int32_t (*)[2])ordered);
        for (uint32_t y = 1u; y <= 4u; y++) {
            for (uint32_t x = 1u; x <= 4u; x++) {
                uint16_t expected = in_right_triangle_footprint(x, y) != 0u ? 0x001Fu : 0u;
                CHECK(inst->vram[y][x] == expected);
            }
        }
    }

    memset(inst->vram, 0, sizeof inst->vram);
    const int32_t one_pixel[3][2] = {{0, 0}, {2, 0}, {0, 2}};
    triangle(inst, 0x20u, 0x000000FFu, one_pixel);
    CHECK(inst->vram[0][0] == 0x001Fu);
    CHECK(inst->vram[0][1] == 0u);
    CHECK(inst->vram[1][0] == 0u);
    CHECK(inst->vram[1][1] == 0u);

    memset(inst->vram, 0, sizeof inst->vram);
    const int32_t zero_area[3][2] = {{1, 8}, {4, 8}, {7, 8}};
    triangle(inst, 0x20u, 0x000000FFu, zero_area);
    for (uint32_t x = 0u; x < 10u; x++) {
        CHECK(inst->vram[8][x] == 0u);
    }

    memset(inst->vram, 0, sizeof inst->vram);
    const int32_t square[4][2] = {{1, 1}, {5, 1}, {1, 5}, {5, 5}};
    write_gp0(inst, 0xE1000020u); /* ABR add, as documented by docs/hardware/gpu.md. */
    for (uint32_t y = 1u; y <= 4u; y++) {
        for (uint32_t x = 1u; x <= 4u; x++) {
            inst->vram[y][x] = 1u;
        }
    }
    quad(inst, 0x2Au, 0x00000010u, square);
    for (uint32_t y = 1u; y <= 4u; y++) {
        for (uint32_t x = 1u; x <= 4u; x++) {
            CHECK(inst->vram[y][x] == 3u);
        }
    }
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-gpu-triangles <case>\n");
        return 2;
    }
    if (strcmp(argv[1], "flat_packet") == 0) {
        case_flat_packet();
    } else {
        fprintf(stderr, "unknown GPU triangle case: %s\n", argv[1]);
        return 2;
    }
    return failures == 0 ? 0 : 1;
}
