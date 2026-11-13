/* GP0 line and polyline tests inspect the native RGB555 span writes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus.h"
#include "internal.h"
#include "state.h"

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

struct body_offset {
    const char *wanted;
    size_t cursor;
    size_t offset;
    int found;
};

static void locate_field(void *user, const char *name, const void *data, uint32_t count,
                         uint32_t width)
{
    struct body_offset *field = (struct body_offset *)user;
    (void)data;
    if (!field->found && strcmp(name, field->wanted) == 0) {
        field->found = 1;
        field->offset = field->cursor;
    }
    field->cursor += (size_t)count * width;
}

static void put_le32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static int corrupt_and_refuse(schway *target, schway *source, const char *name, uint32_t value)
{
    size_t size = schway_state_size(source);
    uint8_t *candidate = (uint8_t *)malloc(size);
    uint8_t *before = (uint8_t *)malloc(size);
    uint8_t *after = (uint8_t *)malloc(size);
    CHECK(candidate != NULL && before != NULL && after != NULL);
    if (candidate == NULL || before == NULL || after == NULL) {
        free(candidate);
        free(before);
        free(after);
        return 0;
    }
    CHECK(schway_save_state(source, candidate, size) == SCHWAY_STATUS_OK);
    struct body_offset field = {name, 0u, 0u, 0};
    CHECK(schway_state_visit(source, locate_field, &field) == SCHWAY_STATUS_OK);
    CHECK(field.found);
    if (!field.found) {
        free(candidate);
        free(before);
        free(after);
        return 0;
    }
    put_le32(candidate + SCHWAY__STATE_HEADER_SIZE + field.offset, value);
    put_le32(candidate + 16u,
             schway__crc32_update(0u, candidate + SCHWAY__STATE_HEADER_SIZE,
                                  size - SCHWAY__STATE_HEADER_SIZE));
    CHECK(schway_save_state(target, before, size) == SCHWAY_STATUS_OK);
    CHECK(schway_load_state(target, candidate, size) == SCHWAY_STATUS_ERR_STATE_FIELD);
    CHECK(schway_save_state(target, after, size) == SCHWAY_STATUS_OK);
    CHECK(memcmp(before, after, size) == 0);
    free(candidate);
    free(before);
    free(after);
    return 1;
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

static void rectangle(schway *inst, uint32_t opcode, uint32_t color, uint32_t x, uint32_t y,
                      uint32_t uv_clut, uint32_t size)
{
    write_gp0(inst, (opcode << 24) | (color & 0x00FFFFFFu));
    write_gp0(inst, vertex((int32_t)x, (int32_t)y));
    if ((opcode & 4u) != 0u) {
        write_gp0(inst, uv_clut);
    }
    if ((opcode & 0x18u) == 0u) {
        write_gp0(inst, size);
    }
}

static void check_octant(schway *inst, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                         const int32_t expected[5][2])
{
    memset(inst->vram, 0, sizeof inst->vram);
    flat_line(inst, x0, y0, x1, y1, 0x000000FFu);
    for (int32_t y = 16; y <= 24; y++) {
        for (int32_t x = 16; x <= 24; x++) {
            uint16_t wanted = 0u;
            for (uint32_t i = 0u; i < 5u; i++) {
                if (expected[i][0] == x && expected[i][1] == y) {
                    wanted = 0x001Fu;
                }
            }
            CHECK(inst->vram[y][x] == wanted);
        }
    }
}

static void case_polyline_packet(void)
{
    static const int32_t octants[8][5][2] = {
        {{20, 20}, {21, 21}, {22, 21}, {23, 22}, {24, 22}},
        {{20, 20}, {21, 20}, {22, 19}, {23, 19}, {24, 18}},
        {{20, 20}, {19, 21}, {18, 21}, {17, 22}, {16, 22}},
        {{20, 20}, {19, 20}, {18, 19}, {17, 19}, {16, 18}},
        {{20, 20}, {20, 21}, {21, 22}, {21, 23}, {22, 24}},
        {{20, 20}, {19, 21}, {19, 22}, {18, 23}, {18, 24}},
        {{20, 20}, {20, 19}, {21, 18}, {21, 17}, {22, 16}},
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
    gouraud_line(inst, 60, 60, 0u, 62, 60, 0x000000FFu);
    CHECK(inst->vram[60][61] == 0x0010u);

    memset(inst->vram, 0, sizeof inst->vram);
    gouraud_line(inst, 80, 80, 0x0000002Fu, 82, 80, 0u);
    CHECK(inst->vram[80][80] == 0x0005u);
    CHECK(inst->vram[80][81] == 0x0003u);
    CHECK(inst->vram[80][82] == 0u);

    memset(inst->vram, 0, sizeof inst->vram);
    write_gp0(inst, 0xE1000200u);
    flat_line(inst, 1, 0, 3, 0, 0x00000010u);
    CHECK(inst->vram[0][1] == 0x0002u);
    CHECK(inst->vram[0][2] == 0x0001u);
    CHECK(inst->vram[0][3] == 0x0002u);
    write_gp0(inst, 0xE1000000u);

    memset(inst->vram, 0, sizeof inst->vram);
    inst->vram[52][52] = 0x03E0u;
    write_gp0(inst, 0x420000FFu);
    write_gp0(inst, vertex(52, 52));
    write_gp0(inst, vertex(52, 52));
    CHECK(inst->vram[52][52] == 0x01EFu);

    memset(inst->vram, 0, sizeof inst->vram);
    inst->vram[54][54] = 0x03E0u;
    inst->vram[54][55] = 0x03E0u;
    write_gp0(inst, 0x4A0000FFu);
    write_gp0(inst, vertex(54, 54));
    write_gp0(inst, vertex(55, 54));
    write_gp0(inst, 0x5ABC5DEFu);
    CHECK(inst->vram[54][54] == 0x01EFu);
    CHECK(inst->vram[54][55] == 0x01EFu);

    memset(inst->vram, 0, sizeof inst->vram);
    flat_line(inst, 0, 200, 1023, 200, 0x000000FFu);
    CHECK(inst->vram[200][0] == 0x001Fu);
    CHECK(inst->vram[200][1023] == 0x001Fu);
    memset(inst->vram, 0, sizeof inst->vram);
    flat_line(inst, -1, 200, 1023, 200, 0x000000FFu);
    CHECK(inst->vram[200][0] == 0u);
    CHECK(inst->vram[200][1023] == 0u);

    memset(inst->vram, 0, sizeof inst->vram);
    write_gp0(inst, 0xE5000002u);
    flat_line(inst, -2, 40, 0, 40, 0x000000FFu);
    CHECK(inst->vram[40][0] == 0x001Fu);
    CHECK(inst->vram[40][2] == 0x001Fu);
    write_gp0(inst, 0xE5000000u);

    memset(inst->vram, 0, sizeof inst->vram);
    flat_line(inst, 200, 0, 200, 511, 0x000000FFu);
    CHECK(inst->vram[0][200] == 0x001Fu);
    CHECK(inst->vram[511][200] == 0x001Fu);
    memset(inst->vram, 0, sizeof inst->vram);
    flat_line(inst, 200, -1, 200, 511, 0x000000FFu);
    CHECK(inst->vram[0][200] == 0u);
    CHECK(inst->vram[511][200] == 0u);

    memset(inst->vram, 0, sizeof inst->vram);
    write_gp0(inst, 0xE300280Au);
    write_gp0(inst, 0xE400300Cu);
    flat_line(inst, 8, 11, 14, 11, 0x000000FFu);
    CHECK(inst->vram[11][9] == 0u);
    CHECK(inst->vram[11][10] == 0x001Fu);
    CHECK(inst->vram[11][11] == 0x001Fu);
    CHECK(inst->vram[11][12] == 0x001Fu);
    CHECK(inst->vram[11][13] == 0u);
    full_draw_area(inst);

    memset(inst->vram, 0, sizeof inst->vram);
    write_gp0(inst, 0x58000000u);
    write_gp0(inst, vertex(4, 4));
    write_gp0(inst, 0x00000080u);
    write_gp0(inst, vertex(8, 4));
    write_gp0(inst, 0x00008000u);
    write_gp0(inst, vertex(12, 4));
    write_gp0(inst, 0x5ABC5DEFu);
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
    write_gp0(inst, 0x5ABC5DEFu);
    CHECK(inst->gpu.phase == SCHWAY__GPU_IDLE);
    CHECK(inst->gpu.fifo_count == 0u);
    schway_destroy(inst);
}

static void case_polyline_state(void)
{
    schway *complete = make_instance();
    schway *source = make_instance();
    schway *restored = make_instance();
    if (complete == NULL || source == NULL || restored == NULL) {
        schway_destroy(complete);
        schway_destroy(source);
        schway_destroy(restored);
        return;
    }
    schway *instances[2] = {complete, source};
    for (uint32_t i = 0u; i < 2u; i++) {
        full_draw_area(instances[i]);
        write_gp0(instances[i], 0x58000000u);
        write_gp0(instances[i], vertex(10, 10));
        write_gp0(instances[i], 0x00000080u);
    }
    size_t size = schway_state_size(source);
    uint8_t *state = (uint8_t *)malloc(size);
    CHECK(state != NULL);
    if (state == NULL) {
        schway_destroy(complete);
        schway_destroy(source);
        schway_destroy(restored);
        return;
    }
    CHECK(schway_save_state(source, state, size) == SCHWAY_STATUS_OK);
    CHECK(schway_load_state(restored, state, size) == SCHWAY_STATUS_OK);
    free(state);
    CHECK(corrupt_and_refuse(restored, source, "gpu.polyline_x", 0x800u));

    write_gp0(complete, vertex(14, 10));
    write_gp0(complete, 0x5ABC5DEFu);
    write_gp0(source, vertex(14, 10));
    write_gp0(source, 0x5ABC5DEFu);
    write_gp0(restored, vertex(14, 10));
    write_gp0(restored, 0x5ABC5DEFu);
    CHECK(memcmp(complete->vram, source->vram, sizeof complete->vram) == 0);
    CHECK(memcmp(complete->vram, restored->vram, sizeof complete->vram) == 0);
    CHECK(complete->vram[10][11] == 0x0004u);
    CHECK(complete->vram[10][14] == 0x0010u);
    schway_destroy(complete);
    schway_destroy(source);
    schway_destroy(restored);
}

static void case_rectangle_sizes(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return;
    }
    full_draw_area(inst);

    rectangle(inst, 0x68u, 0x000000FFu, 10u, 10u, 0u, 0u);
    rectangle(inst, 0x70u, 0x0000FF00u, 20u, 20u, 0u, 0u);
    rectangle(inst, 0x78u, 0x00FF0000u, 40u, 40u, 0u, 0u);
    CHECK(inst->vram[10][10] == 0x001Fu);
    CHECK(inst->vram[10][11] == 0u);
    CHECK(inst->vram[20][20] == 0x03E0u);
    CHECK(inst->vram[27][27] == 0x03E0u);
    CHECK(inst->vram[28][20] == 0u);
    CHECK(inst->vram[40][40] == 0x7C00u);
    CHECK(inst->vram[55][55] == 0x7C00u);
    CHECK(inst->vram[56][40] == 0u);

    rectangle(inst, 0x60u, 0x000000FFu, 70u, 70u, 0u, (2u << 16) | 3u);
    CHECK(inst->vram[70][70] == 0x001Fu);
    CHECK(inst->vram[71][72] == 0x001Fu);
    CHECK(inst->vram[72][70] == 0u);
    rectangle(inst, 0x60u, 0x000000FFu, 80u, 80u, 0u, 2u);
    rectangle(inst, 0x60u, 0x000000FFu, 90u, 90u, 0u, 1u << 16);
    CHECK(inst->vram[80][80] == 0u);
    CHECK(inst->vram[90][90] == 0u);

    write_gp0(inst, 0xE3001404u);
    write_gp0(inst, 0xE4001805u);
    rectangle(inst, 0x60u, 0x000000FFu, 3u, 4u, 0u, (3u << 16) | 3u);
    CHECK(inst->vram[5][4] == 0x001Fu);
    CHECK(inst->vram[6][5] == 0x001Fu);
    CHECK(inst->vram[4][3] == 0u);
    CHECK(inst->vram[7][5] == 0u);

    full_draw_area(inst);
    write_gp0(inst, 0xE50017FFu);
    rectangle(inst, 0x68u, 0x0000FF00u, 10u, 10u, 0u, 0u);
    CHECK(inst->vram[12][9] == 0x03E0u);
    CHECK(inst->vram[10][10] == 0x001Fu);

    write_gp0(inst, 0xE5000000u);
    write_gp0(inst, 0xE1000200u);
    rectangle(inst, 0x60u, 0x00000010u, 2u, 0u, 0u, (1u << 16) | 3u);
    CHECK(inst->vram[0][2] == 0x0002u);
    CHECK(inst->vram[0][3] == 0x0002u);
    CHECK(inst->vram[0][4] == 0x0002u);

    write_gp0(inst, 0xE1000000u);
    inst->vram[30][30] = 5u;
    rectangle(inst, 0x62u, 0x000000A0u, 30u, 30u, 0u, (1u << 16) | 1u);
    CHECK(inst->vram[30][30] == 12u);

    write_gp0(inst, 0xE307FBFEu);
    write_gp0(inst, 0xE407FFFFu);
    rectangle(inst, 0x60u, 0x000000FFu, 1u, 1u, 0u, (511u << 16) | 1023u);
    CHECK(inst->vram[510][1022] == 0x001Fu);
    CHECK(inst->vram[511][1023] == 0x001Fu);
    CHECK(inst->vram[509][1023] == 0u);
    schway_destroy(inst);
}

static void case_rectangle_texture(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return;
    }
    full_draw_area(inst);
    write_gp0(inst, 0xE1000100u);
    inst->vram[0][254] = 0x001Fu;
    inst->vram[0][255] = 0x03E0u;
    inst->vram[0][0] = 0x7C00u;
    inst->vram[0][1] = 0x7FFFu;
    rectangle(inst, 0x65u, 0x00FFFFFFu, 100u, 100u, 254u,
              (1u << 16) | 4u);
    CHECK(inst->vram[100][100] == 0x001Fu);
    CHECK(inst->vram[100][101] == 0x03E0u);
    CHECK(inst->vram[100][102] == 0x7C00u);
    CHECK(inst->vram[100][103] == 0x7FFFu);

    schway_destroy(inst);

    schway *edge = make_instance();
    if (edge == NULL) {
        return;
    }
    full_draw_area(edge);
    write_gp0(edge, 0xE100010Fu);
    edge->vram[0][1022] = 0x001Fu;
    edge->vram[0][1023] = 0x03E0u;
    edge->vram[0][0] = 0x7C00u;
    rectangle(edge, 0x65u, 0x00FFFFFFu, 110u, 110u, 62u,
              (1u << 16) | 3u);
    CHECK(edge->vram[110][110] == 0x001Fu);
    CHECK(edge->vram[110][111] == 0x03E0u);
    CHECK(edge->vram[110][112] == 0x7C00u);
    schway_destroy(edge);

    schway *odd = make_instance();
    if (odd == NULL) {
        return;
    }
    full_draw_area(odd);
    write_gp0(odd, 0xE1000100u);
    odd->vram[1][3] = 0x001Fu;
    odd->vram[1][4] = 0x03E0u;
    odd->vram[1][5] = 0x7C00u;
    rectangle(odd, 0x65u, 0x00FFFFFFu, 120u, 120u, (1u << 8) | 3u,
              (1u << 16) | 3u);
    CHECK(odd->vram[120][120] == 0x001Fu);
    CHECK(odd->vram[120][121] == 0x03E0u);
    CHECK(odd->vram[120][122] == 0x7C00u);

    odd->vram[1][10] = 0x7FFFu;
    rectangle(odd, 0x64u, 0x00808080u, 130u, 130u, (1u << 8) | 10u,
              (1u << 16) | 1u);
    CHECK(odd->vram[130][130] == 0x7FFFu);
    odd->vram[1][11] = 0x801Fu;
    write_gp0(odd, 0x01000000u);
    odd->vram[140][140] = 0x7C00u;
    rectangle(odd, 0x67u, 0x00FFFFFFu, 140u, 140u, (1u << 8) | 11u,
              (1u << 16) | 1u);
    CHECK(odd->vram[140][140] == 0xBC0Fu);
    schway_destroy(odd);
}

static void case_rectangle_profile(void)
{
    schway *v2 = make_instance();
    schway *v0 = make_instance();
    if (v2 == NULL || v0 == NULL) {
        schway_destroy(v2);
        schway_destroy(v0);
        return;
    }
    schway_profile profile = {sizeof profile, SCHWAY_REGION_NTSC_U, SCHWAY_PROFILE_GPU_V0};
    CHECK(schway_set_profile(v0, &profile) == SCHWAY_STATUS_OK);
    full_draw_area(v2);
    full_draw_area(v0);
    for (uint32_t y = 19u; y <= 21u; y++) {
        for (uint32_t x = 18u; x <= 22u; x++) {
            uint16_t color = (uint16_t)((x - 17u) + (y - 19u) * 8u);
            v2->vram[y][x] = color;
            v0->vram[y][x] = color;
        }
    }
    write_gp0(v2, 0xE1003100u);
    write_gp0(v0, 0xE1003100u);
    rectangle(v2, 0x65u, 0x00FFFFFFu, 30u, 30u, (20u << 8) | 20u,
              (2u << 16) | 3u);
    rectangle(v0, 0x65u, 0x00FFFFFFu, 30u, 30u, (20u << 8) | 20u,
              (2u << 16) | 3u);
    CHECK(v2->vram[30][30] == 11u);
    CHECK(v2->vram[30][31] == 10u);
    CHECK(v2->vram[30][32] == 9u);
    CHECK(v2->vram[31][30] == 3u);
    CHECK(v2->vram[31][32] == 1u);
    CHECK(v0->vram[30][30] == 11u);
    CHECK(v0->vram[30][31] == 12u);
    CHECK(v0->vram[31][32] == 21u);
    schway_destroy(v2);
    schway_destroy(v0);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-gpu-spans <case>\n");
        return 2;
    }
    if (strcmp(argv[1], "polyline_packet") == 0) {
        case_polyline_packet();
        case_polyline_state();
    } else if (strcmp(argv[1], "rectangle_sizes") == 0) {
        case_rectangle_sizes();
    } else if (strcmp(argv[1], "rectangle_texture") == 0) {
        case_rectangle_texture();
    } else if (strcmp(argv[1], "rectangle_profile") == 0) {
        case_rectangle_profile();
    } else {
        fprintf(stderr, "unknown GPU span case: %s\n", argv[1]);
        return 2;
    }
    return failures == 0 ? 0 : 1;
}
