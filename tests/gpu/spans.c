/* GP0 line and polyline tests inspect the native RGB555 span writes. */
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

static void flat_line(schway *inst, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                      uint32_t color)
{
    write_gp0(inst, 0x40000000u | (color & 0x00FFFFFFu));
    write_gp0(inst, vertex(x0, y0));
    write_gp0(inst, vertex(x1, y1));
}

static void gouraud_line(schway *inst, int32_t x0, int32_t y0, uint32_t color0,
                         int32_t x1, int32_t y1, uint32_t color1)
{
    write_gp0(inst, 0x50000000u | (color0 & 0x00FFFFFFu));
    write_gp0(inst, vertex(x0, y0));
    write_gp0(inst, color1 & 0x00FFFFFFu);
    write_gp0(inst, vertex(x1, y1));
}

static void check_octant(schway *inst, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                         const int32_t expected[5][2])
{
    memset(inst->vram, 0, sizeof inst->vram);
    flat_line(inst, x0, y0, x1, y1, 0x000000FFu);
    for (uint32_t i = 0u; i < 5u; i++) {
        CHECK(inst->vram[expected[i][1]][expected[i][0]] == 0x001Fu);
    }
}

static void case_polyline_packet(void)
{
    static const int32_t octants[8][5][2] = {
        {{20, 20}, {21, 21}, {22, 21}, {23, 22}, {24, 22}},
        {{20, 20}, {21, 19}, {22, 19}, {23, 18}, {24, 18}},
        {{20, 20}, {19, 21}, {18, 21}, {17, 22}, {16, 22}},
        {{20, 20}, {19, 19}, {18, 19}, {17, 18}, {16, 18}},
        {{20, 20}, {21, 21}, {21, 22}, {22, 23}, {22, 24}},
        {{20, 20}, {19, 21}, {19, 22}, {18, 23}, {18, 24}},
        {{20, 20}, {21, 19}, {21, 18}, {22, 17}, {22, 16}},
        {{20, 20}, {19, 19}, {19, 18}, {18, 17}, {18, 16}}
    };
    static const int32_t endpoints[8][4] = {
        {20, 20, 24, 22}, {20, 20, 24, 18}, {20, 20, 16, 22}, {20, 20, 16, 18},
        {20, 20, 22, 24}, {20, 20, 18, 24}, {20, 20, 22, 16}, {20, 20, 18, 16}
    };
    schway *inst = make_instance();
    if (inst == NULL) {
        return;
    }
    full_draw_area(inst);

    for (uint32_t i = 0u; i < 8u; i++) {
        check_octant(inst, endpoints[i][0], endpoints[i][1], endpoints[i][2], endpoints[i][3],
                     octants[i]);
    }

    memset(inst->vram, 0, sizeof inst->vram);
    gouraud_line(inst, 50, 50, 0x000000FFu, 50, 50, 0x0000FF00u);
    CHECK(inst->vram[50][50] == 0x001Fu);

    memset(inst->vram, 0, sizeof inst->vram);
    flat_line(inst, 0, 200, 1023, 200, 0x000000FFu);
    CHECK(inst->vram[200][0] == 0x001Fu);
    CHECK(inst->vram[200][1023] == 0x001Fu);
    memset(inst->vram, 0, sizeof inst->vram);
    flat_line(inst, -1, 200, 1023, 200, 0x000000FFu);
    CHECK(inst->vram[200][0] == 0u);
    CHECK(inst->vram[200][1023] == 0u);

    memset(inst->vram, 0, sizeof inst->vram);
    flat_line(inst, 200, 0, 200, 511, 0x000000FFu);
    CHECK(inst->vram[0][200] == 0x001Fu);
    CHECK(inst->vram[511][200] == 0x001Fu);
    memset(inst->vram, 0, sizeof inst->vram);
    flat_line(inst, 200, -1, 200, 511, 0x000000FFu);
    CHECK(inst->vram[0][200] == 0u);
    CHECK(inst->vram[511][200] == 0u);

    memset(inst->vram, 0, sizeof inst->vram);
    write_gp0(inst, 0x58000000u);
    write_gp0(inst, vertex(4, 4));
    write_gp0(inst, 0x00000080u);
    write_gp0(inst, vertex(8, 4));
    write_gp0(inst, 0x00008000u);
    write_gp0(inst, vertex(12, 4));
    write_gp0(inst, 0x50005000u);
    CHECK(inst->vram[4][4] == 0u);
    CHECK(inst->vram[4][5] == 0x0004u);
    CHECK(inst->vram[4][6] == 0x0008u);
    CHECK(inst->vram[4][7] == 0x000Cu);
    CHECK(inst->vram[4][8] == 0x0010u);
    CHECK(inst->vram[4][9] == 0x008Cu);
    CHECK(inst->vram[4][10] == 0x0108u);
    CHECK(inst->vram[4][11] == 0x0184u);
    CHECK(inst->vram[4][12] == 0x0200u);
    CHECK(inst->vram[0][0] == 0u);

    memset(inst->vram, 0, sizeof inst->vram);
    write_gp0(inst, 0x480000FFu);
    write_gp0(inst, vertex(30, 30));
    for (uint32_t i = 0u; i < 2048u; i++) {
        write_gp0(inst, vertex(30, 30));
    }
    CHECK(inst->gpu.phase == SCHWAY__GPU_POLYLINE_VERTEX);
    CHECK(inst->gpu.fifo_count == 0u);
    write_gp0(inst, 0x50005000u);
    CHECK(inst->gpu.phase == SCHWAY__GPU_IDLE);
    CHECK(inst->gpu.fifo_count == 0u);
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    if (argc != 2 || strcmp(argv[1], "polyline_packet") != 0) {
        fprintf(stderr, "usage: schwaystation-test-gpu-spans polyline_packet\n");
        return 2;
    }
    case_polyline_packet();
    return failures == 0 ? 0 : 1;
}
