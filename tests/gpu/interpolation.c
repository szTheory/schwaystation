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

static void draw_flat_triangle(schway *inst, uint32_t opcode, uint32_t color)
{
    write_gp0(inst, (opcode << 24) | (color & 0x00FFFFFFu));
    write_gp0(inst, vertex(1u, 1u));
    write_gp0(inst, vertex(9u, 1u));
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

    /* At integer sample (2,2), the vertex-anchored 12-bit planes give U=1. */
    CHECK(inst->vram[2][64] == 0x3210u);
    CHECK((inst->gpu.draw_mode & 0x1FFu) == 1u);
    CHECK(inst->vram[2][2] == 0x0C77u);
    /* At integer sample (3,2), the vertex-anchored planes give U=2. */
    CHECK(inst->vram[2][3] == 0x0C00u);
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

    /* Clipping retains the original vertex anchor at integer sample (3,2). */
    CHECK(clipped->vram[2][3] == 0x088Au);
    for (uint32_t y = 2u; y <= 6u; y++) {
        for (uint32_t x = 3u; x <= 6u; x++) {
            CHECK(clipped->vram[y][x] == full->vram[y][x]);
        }
    }
    schway_destroy(clipped);
    schway_destroy(full);
}

static void case_flag_matrix(void)
{
    static const uint16_t expected[2][2][2] = {
        {{0x2108u, 0x2108u}, {0x1093u, 0x1093u}},
        {{0xA108u, 0xC210u}, {0x9093u, 0xA117u}}};
    for (uint32_t depth = 0u; depth < 4u; depth++) {
        for (uint32_t textured = 0u; textured < 2u; textured++) {
            for (uint32_t semi = 0u; semi < 2u; semi++) {
                for (uint32_t raw = 0u; raw < (textured != 0u ? 2u : 1u); raw++) {
                    schway *inst = make_instance();
                    if (inst == NULL) {
                        return;
                    }
                    draw_area(inst, 0u, 0u, 1023u, 511u);
                    inst->vram[2][64] = 0xC210u;
                    inst->vram[1][160] = 0xC210u;
                    inst->vram[1][176] = 0xC210u;
                    inst->vram[2][2] = 0x001Fu;
                    uint32_t page = 1u | (depth << 7);
                    uint32_t opcode = 0x20u | (textured << 2) | (semi << 1) | raw;
                    if (textured != 0u) {
                        write_gp0(inst, (opcode << 24) | 0x00404040u);
                        write_gp0(inst, vertex(1u, 1u));
                        write_gp0(inst, uv(0u, 2u, 0x004Au));
                        write_gp0(inst, vertex(9u, 1u));
                        write_gp0(inst, uv(0u, 2u, page));
                        write_gp0(inst, vertex(1u, 9u));
                        write_gp0(inst, uv(0u, 2u, 0u));
                    } else {
                        write_gp0(inst, (opcode << 24) | 0x00404040u);
                        write_gp0(inst, vertex(1u, 1u));
                        write_gp0(inst, vertex(9u, 1u));
                        write_gp0(inst, vertex(1u, 9u));
                    }
                    CHECK(inst->vram[2][2] == expected[textured][semi][raw]);
                    if (textured != 0u) {
                        CHECK((inst->gpu.draw_mode & 0x1FFu) == page);
                    }
                    schway_destroy(inst);
                }
            }
        }
    }
}

static void case_equal_gouraud(void)
{
    schway *flat = make_instance();
    schway *gouraud = make_instance();
    if (flat == NULL || gouraud == NULL) {
        schway_destroy(flat);
        schway_destroy(gouraud);
        return;
    }
    draw_area(flat, 0u, 0u, 1023u, 511u);
    draw_area(gouraud, 0u, 0u, 1023u, 511u);
    draw_flat_triangle(flat, 0x20u, 0x00808080u);
    write_gp0(gouraud, 0x30808080u);
    write_gp0(gouraud, vertex(1u, 1u));
    write_gp0(gouraud, 0x00808080u);
    write_gp0(gouraud, vertex(9u, 1u));
    write_gp0(gouraud, 0x00808080u);
    write_gp0(gouraud, vertex(1u, 9u));
    CHECK(memcmp(flat->vram, gouraud->vram, sizeof flat->vram) == 0);

    memset(flat->vram, 0, sizeof flat->vram);
    memset(gouraud->vram, 0, sizeof gouraud->vram);
    write_gp0(flat, 0xE1000200u);
    write_gp0(gouraud, 0xE1000200u);
    draw_flat_triangle(flat, 0x20u, 0x00808080u);
    write_gp0(gouraud, 0x30808080u);
    write_gp0(gouraud, vertex(1u, 1u));
    write_gp0(gouraud, 0x00808080u);
    write_gp0(gouraud, vertex(9u, 1u));
    write_gp0(gouraud, 0x00808080u);
    write_gp0(gouraud, vertex(1u, 9u));
    CHECK(flat->vram[1][1] == 0x4210u);
    CHECK(gouraud->vram[1][1] == 0x3DEFu);
    schway_destroy(flat);
    schway_destroy(gouraud);
}

static void case_uv_negative_wrap(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return;
    }
    draw_area(inst, 0u, 0u, 1023u, 511u);
    inst->vram[2][159] = 0x7C00u;
    inst->vram[2][127] = 0x03E0u;
    uint32_t page = 0x010Fu;
    write_gp0(inst, 0x250000FFu);
    write_gp0(inst, vertex(1u, 1u));
    write_gp0(inst, uv(255u, 2u, 0x004Au));
    write_gp0(inst, vertex(9u, 1u));
    write_gp0(inst, uv(0u, 2u, page));
    write_gp0(inst, vertex(1u, 9u));
    write_gp0(inst, uv(255u, 2u, 0u));
    CHECK((inst->gpu.draw_mode & 0x1FFu) == page);
    CHECK(inst->vram[2][2] == 0x7C00u);
    CHECK(inst->vram[2][3] == 0x03E0u);
    schway_destroy(inst);
}

static void draw_gouraud_quad(schway *inst, uint32_t fourth_position)
{
    write_gp0(inst, 0x3A000080u);
    write_gp0(inst, vertex(1u, 1u));
    write_gp0(inst, 0x00008000u);
    write_gp0(inst, vertex(8u, 1u));
    write_gp0(inst, 0x00800000u);
    write_gp0(inst, vertex(1u, 8u));
    write_gp0(inst, 0x00808080u);
    write_gp0(inst, fourth_position);
}

static void case_quad_attributes(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return;
    }
    draw_area(inst, 0u, 0u, 1023u, 511u);
    inst->vram[4][4] = 0x001Fu;
    draw_gouraud_quad(inst, vertex(8u, 8u));
    CHECK(inst->vram[4][4] == 0x0C70u);

    memset(inst->vram, 0, sizeof inst->vram);
    write_gp0(inst, 0xE1000000u);
    write_gp0(inst, 0x38000080u);
    write_gp0(inst, vertex(1u, 1u));
    write_gp0(inst, 0x00008000u);
    write_gp0(inst, vertex(8u, 1u));
    write_gp0(inst, 0x00800000u);
    write_gp0(inst, vertex(1u, 8u));
    write_gp0(inst, 0x00808080u);
    write_gp0(inst, vertex(1u, 8u));
    CHECK(inst->vram[2][2] == 0x084Bu);
    schway_destroy(inst);
}

static void case_split_state(void)
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
        draw_area(instances[i], 0u, 0u, 1023u, 511u);
        instances[i]->vram[2][64] = 0x3210u;
        instances[i]->vram[4][33] = 0x7FFFu;
        instances[i]->vram[4][34] = 0x7C00u;
        write_gp0(instances[i], 0x34000080u);
        write_gp0(instances[i], vertex(1u, 1u));
        write_gp0(instances[i], uv(0u, 2u, 0x0102u));
        write_gp0(instances[i], 0x00008000u);
        write_gp0(instances[i], vertex(9u, 1u));
        write_gp0(instances[i], uv(8u, 2u, 0x0001u));
    }
    size_t state_size = schway_state_size(source);
    uint8_t *state = (uint8_t *)malloc(state_size);
    CHECK(state != NULL);
    if (state == NULL) {
        schway_destroy(complete);
        schway_destroy(source);
        schway_destroy(restored);
        return;
    }
    CHECK(schway_save_state(source, state, state_size) == SCHWAY_STATUS_OK);
    CHECK(schway_load_state(restored, state, state_size) == SCHWAY_STATUS_OK);
    free(state);

    uint32_t color = 0x00800000u;
    uint32_t position = vertex(1u, 9u);
    uint32_t texture = uv(0u, 2u, 0u);
    write_gp0(complete, color);
    write_gp0(complete, position);
    write_gp0(complete, texture);
    write_gp0(source, color);
    write_gp0(source, position);
    write_gp0(source, texture);
    write_gp0(restored, color);
    write_gp0(restored, position);
    write_gp0(restored, texture);
    CHECK(memcmp(complete->vram, source->vram, sizeof complete->vram) == 0);
    CHECK(memcmp(complete->vram, restored->vram, sizeof complete->vram) == 0);
    schway_destroy(complete);
    schway_destroy(source);
    schway_destroy(restored);
}

static void case_hostile_slope(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return;
    }
    draw_area(inst, 0u, 0u, 1023u, 511u);
    write_gp0(inst, 0xE50FFBFFu);
    write_gp0(inst, 0x30000000u);
    write_gp0(inst, vertex(0x401u, 0x601u));
    write_gp0(inst, 0x00000000u);
    write_gp0(inst, vertex(0x404u, 0x7F5u));
    write_gp0(inst, 0x000000FFu);
    write_gp0(inst, vertex(0x401u, 0x602u));
    CHECK(inst->vram[167][1] == 0x000Au);
    CHECK(inst->vram[166][1] == 0u);
    CHECK(inst->vram[168][1] == 0u);
    schway_destroy(inst);
}

static void case_channel_midpoint(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return;
    }
    draw_area(inst, 0u, 0u, 1023u, 511u);
    write_gp0(inst, 0x3000000Fu);
    write_gp0(inst, vertex(0u, 0u));
    write_gp0(inst, 0x00000010u);
    write_gp0(inst, vertex(8u, 0u));
    write_gp0(inst, 0x00000010u);
    write_gp0(inst, vertex(0u, 8u));
    /* At (1,2), half the barycentric weight belongs to channel 16: 15.5 truncates to 15. */
    CHECK(inst->vram[2][1] == 0x0001u);
    schway_destroy(inst);
}

static void case_vertex_plane(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return;
    }
    draw_area(inst, 0u, 0u, 1023u, 511u);

    write_gp0(inst, 0x3000000Fu);
    write_gp0(inst, vertex(0u, 0u));
    write_gp0(inst, 0x00000010u);
    write_gp0(inst, vertex(8u, 0u));
    write_gp0(inst, 0x00000010u);
    write_gp0(inst, vertex(0u, 8u));
    /* Integer sample (2,2) is 15.5; the capture-backed 12-bit plane rounds it to 16. */
    CHECK(inst->vram[2][2] == 0x0002u);

    memset(inst->vram, 0, sizeof inst->vram);
    inst->vram[0][512] = 0x001Fu;
    inst->vram[0][513] = 0x03E0u;
    write_gp0(inst, 0x2C808080u);
    write_gp0(inst, vertex(0u, 10u));
    write_gp0(inst, uv(0u, 0u, 0u));
    write_gp0(inst, vertex(6u, 10u));
    write_gp0(inst, uv(1u, 0u, 0x0108u));
    write_gp0(inst, vertex(0u, 11u));
    write_gp0(inst, uv(0u, 0u, 0u));
    write_gp0(inst, vertex(6u, 11u));
    write_gp0(inst, uv(1u, 0u, 0u));
    /* Truncated 12-bit U slope 682 yields four red texels then two green. */
    for (uint32_t x = 0u; x < 4u; x++) {
        CHECK(inst->vram[10][x] == 0x001Fu);
    }
    CHECK(inst->vram[10][4] == 0x03E0u);
    CHECK(inst->vram[10][5] == 0x03E0u);
    CHECK(inst->vram[10][6] == 0u);
    schway_destroy(inst);
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
    } else if (strcmp(argv[1], "flag_matrix") == 0) {
        case_flag_matrix();
    } else if (strcmp(argv[1], "equal_gouraud") == 0) {
        case_equal_gouraud();
    } else if (strcmp(argv[1], "uv_negative_wrap") == 0) {
        case_uv_negative_wrap();
    } else if (strcmp(argv[1], "quad_attributes") == 0) {
        case_quad_attributes();
    } else if (strcmp(argv[1], "split_state") == 0) {
        case_split_state();
    } else if (strcmp(argv[1], "hostile_slope") == 0) {
        case_hostile_slope();
    } else if (strcmp(argv[1], "channel_midpoint") == 0) {
        case_channel_midpoint();
    } else if (strcmp(argv[1], "vertex_plane") == 0) {
        case_vertex_plane();
    } else {
        fprintf(stderr, "unknown GPU interpolation case: %s\n", argv[1]);
        return 2;
    }
    return failures == 0 ? 0 : 1;
}
