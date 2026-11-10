/* GPU pixel tests use the bus-facing GP0 packet path and native RGB555 VRAM. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus.h"
#include "internal.h"

#define GP0 0x1F801810u
#define GP1 0x1F801814u

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

static void write_gp1(schway *inst, uint32_t word)
{
    uint32_t cycles = 0u;
    CHECK(schway__bus_write(inst, GP1, 4u, word, &cycles) == 0);
}

static void rectangle(schway *inst, uint32_t opcode, uint32_t color, uint32_t x, uint32_t y,
                      uint32_t uv_clut)
{
    write_gp0((inst), (opcode << 24) | color);
    write_gp0(inst, x | (y << 16));
    write_gp0(inst, uv_clut);
}

static void clear_lookup_caches(schway *inst)
{
    write_gp0(inst, 0x01000000u);
    write_gp1(inst, 0x01000000u);
}

static void case_textured_rectangle_mmio(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return;
    }

    /* E1 page/depth and E3/E4 area are described in docs/hardware/gpu.md#textured-rectangles. */
    inst->vram[5u][67u] = 0x001Fu;
    write_gp0(inst, 0xE1000101u);
    write_gp0(inst, 0xE3000000u);
    write_gp0(inst, 0xE407FFFFu);
    write_gp0(inst, 0xE6000000u);

    /* The fixed 1x1 packet and modulation follow docs/hardware/gpu.md#textured-rectangles. */
    rectangle(inst, 0x6Cu, 0x00808080u, 10u, 10u, (5u << 8) | 3u);
    CHECK(inst->vram[10u][10u] == 0x001Fu);

    /* Reserved E1 depth 3 follows docs/hardware/gpu.md#textured-rectangles. */
    clear_lookup_caches(inst);
    write_gp0(inst, 0xE1000180u);
    inst->vram[4u][8u] = 0x1357u;
    rectangle(inst, 0x6Cu, 0x00808080u, 23u, 10u, (4u << 8) | 8u);
    CHECK(inst->vram[10u][23u] == 0x1357u);

    /* Rectangles stay undithered with E1 enabled (docs/hardware/gpu.md#pixel-pipeline). */
    clear_lookup_caches(inst);
    write_gp0(inst, 0xE1000300u);
    inst->vram[0u][0u] = 13u;
    rectangle(inst, 0x6Cu, 0x00808080u, 0u, 0u, 0u);
    CHECK(inst->vram[0u][0u] == 13u);

    /* Four-bit texels use CLUT lookup after E2 windowing (docs/hardware/gpu.md#textured-rectangles). */
    clear_lookup_caches(inst);
    write_gp0(inst, 0xE1000002u); /* E1 page X=2, 4-bit depth; docs/hardware/gpu.md#textured-rectangles. */
    write_gp0(inst, 0xE2000401u); /* E2 mask X=1 and offset X=1; docs/hardware/gpu.md#textured-rectangles. */
    inst->vram[7u][128u + (11u >> 2)] = 0xA000u;
    inst->vram[4u][16u + 10u] = 0x1234u;
    rectangle(inst, 0x6Cu, 0x00808080u, 11u, 10u,
              (257u << 16) | (7u << 8) | 3u);
    CHECK(inst->vram[10u][11u] == 0x1234u);

    /* Eight-bit texels use byte lanes and a 256-entry CLUT (docs/hardware/gpu.md#textured-rectangles). */
    clear_lookup_caches(inst);
    write_gp0(inst, 0xE1000083u); /* E1 page X=3, 8-bit depth; docs/hardware/gpu.md#textured-rectangles. */
    write_gp0(inst, 0xE2000000u);
    inst->vram[9u][192u + (5u >> 1)] = 0xABCDu;
    inst->vram[5u][32u + 0xABu] = 0x2345u;
    rectangle(inst, 0x6Cu, 0x00808080u, 12u, 10u,
              (322u << 16) | (9u << 8) | 5u);
    CHECK(inst->vram[10u][12u] == 0x2345u);

    /* A zero texel leaves the destination untouched (docs/hardware/gpu.md#pixel-pipeline). */
    clear_lookup_caches(inst);
    write_gp0(inst, 0xE1000101u);
    inst->vram[5u][68u] = 0u;
    inst->vram[10u][13u] = 0x4567u;
    rectangle(inst, 0x6Cu, 0x00808080u, 13u, 10u, (5u << 8) | 4u);
    CHECK(inst->vram[10u][13u] == 0x4567u);

    /* Textured ABE requires STP (docs/hardware/gpu.md#pixel-pipeline). */
    clear_lookup_caches(inst);
    write_gp0(inst, 0xE1000101u);
    inst->vram[5u][67u] = 0x801Fu;
    inst->vram[5u][68u] = 0x001Fu;
    inst->vram[10u][14u] = 1u;
    inst->vram[10u][15u] = 1u;
    rectangle(inst, 0x6Eu, 0x00808080u, 14u, 10u, (5u << 8) | 3u);
    rectangle(inst, 0x6Eu, 0x00808080u, 15u, 10u, (5u << 8) | 4u);
    CHECK(inst->vram[10u][14u] == 0x8010u);
    CHECK(inst->vram[10u][15u] == 0x001Fu);

    /* E6 skip/force behavior follows docs/hardware/gpu.md#pixel-pipeline. */
    clear_lookup_caches(inst);
    write_gp0(inst, 0xE1000101u);
    inst->vram[5u][67u] = 0x001Fu;
    inst->vram[10u][16u] = 0x8001u;
    write_gp0(inst, 0xE6000002u);
    rectangle(inst, 0x6Cu, 0x00808080u, 16u, 10u, (5u << 8) | 3u);
    CHECK(inst->vram[10u][16u] == 0x8001u);
    inst->vram[10u][17u] = 0u;
    write_gp0(inst, 0xE6000001u);
    rectangle(inst, 0x6Cu, 0x00808080u, 17u, 10u, (5u << 8) | 3u);
    CHECK(inst->vram[10u][17u] == 0x801Fu);
    schway_destroy(inst);
}

static void case_arithmetic(void)
{
    static const struct {
        uint32_t mode;
        uint32_t background;
        uint32_t foreground;
        uint32_t expected;
    } blends[] = {
        {0u, 1u, 2u, 1u}, {1u, 30u, 2u, 31u}, {2u, 1u, 2u, 0u}, {3u, 1u, 7u, 2u}};
    static const uint8_t dither_expected[4][4] = {
        {12u, 13u, 12u, 13u}, {13u, 12u, 13u, 12u},
        {12u, 13u, 12u, 13u}, {13u, 12u, 13u, 12u}};
    schway *inst = make_instance();
    if (inst == NULL) {
        return;
    }

    for (uint32_t i = 0u; i < sizeof blends / sizeof blends[0]; i++) {
        uint32_t x = i;
        write_gp0(inst, 0xE1000000u | (blends[i].mode << 5));
        inst->vram[0u][x] = (uint16_t)blends[i].background;
        schway__gpu_pixel(inst, x, 0u, 0u, blends[i].foreground << 3,
                          SCHWAY__GPU_PIXEL_SEMI_TRANSPARENT);
        CHECK((inst->vram[0u][x] & 0x1Fu) == blends[i].expected);
    }

    /* Modulation truncation/clamp follows docs/hardware/gpu.md#pixel-pipeline. */
    write_gp0(inst, 0xE1000000u);
    schway__gpu_pixel(inst, 8u, 0u, 0x4210u, 0x004080FFu, SCHWAY__GPU_PIXEL_TEXTURED);
    CHECK(inst->vram[0u][8u] == 0x221Fu);
    schway__gpu_pixel(inst, 9u, 0u, 0x7FFFu, 0x00FFFFFFu, SCHWAY__GPU_PIXEL_TEXTURED);
    CHECK(inst->vram[0u][9u] == 0x7FFFu);

    /* Raw colour and the raw semi-transparent dither exception follow docs/hardware/gpu.md#pixel-pipeline. */
    write_gp0(inst, 0xE1000220u);
    schway__gpu_pixel(inst, 10u, 0u, 0x7C1Du, 0u,
                      SCHWAY__GPU_PIXEL_TEXTURED | SCHWAY__GPU_PIXEL_RAW_TEXTURE);
    CHECK(inst->vram[0u][10u] == 0x7C1Du);
    inst->vram[0u][11u] = 0u;
    schway__gpu_pixel(inst, 11u, 0u, 0x801Du, 0u,
                      SCHWAY__GPU_PIXEL_TEXTURED | SCHWAY__GPU_PIXEL_RAW_TEXTURE |
                          SCHWAY__GPU_PIXEL_SEMI_TRANSPARENT |
                          SCHWAY__GPU_PIXEL_DITHER_ELIGIBLE);
    CHECK(inst->vram[0u][11u] == 0x801Du);

    /* Every dither entry and channel limit follows docs/hardware/gpu.md#pixel-pipeline. */
    write_gp0(inst, 0xE1000200u);
    for (uint32_t y = 0u; y < 4u; y++) {
        for (uint32_t x = 0u; x < 4u; x++) {
            uint32_t at = 32u + y * 4u + x;
            uint32_t color = 104u | (104u << 8) | (104u << 16);
            schway__gpu_pixel(inst, at, y, 0u, color,
                              SCHWAY__GPU_PIXEL_DITHER_ELIGIBLE);
            CHECK((inst->vram[y][at] & 0x1Fu) == dither_expected[y][x]);
            schway__gpu_pixel(inst, at, y + 4u, 0u, 0u,
                              SCHWAY__GPU_PIXEL_DITHER_ELIGIBLE);
            CHECK(inst->vram[y + 4u][at] == 0u);
            schway__gpu_pixel(inst, at, y + 8u, 0u, 0x00FFFFFFu,
                              SCHWAY__GPU_PIXEL_DITHER_ELIGIBLE);
            CHECK(inst->vram[y + 8u][at] == 0x7FFFu);
        }
    }

    /* Caller eligibility, raw texture and ABE policy follows docs/hardware/gpu.md#pixel-pipeline. */
    for (uint32_t flags = 0u; flags < 16u; flags++) {
        uint32_t textured = (flags & SCHWAY__GPU_PIXEL_TEXTURED) != 0u;
        uint32_t raw = (flags & SCHWAY__GPU_PIXEL_RAW_TEXTURE) != 0u;
        uint32_t semi = (flags & SCHWAY__GPU_PIXEL_SEMI_TRANSPARENT) != 0u;
        uint32_t eligible = (flags & SCHWAY__GPU_PIXEL_DITHER_ELIGIBLE) != 0u;
        uint16_t texel = (uint16_t)(raw && semi && textured ? 0x801Du
                                      : (textured && raw ? 13u : 0x0210u));
        uint32_t color = 102u;
        write_gp0(inst, 0xE1000220u); /* dither on; blend mode 1 */
        inst->vram[3u][flags] = 0u;
        schway__gpu_pixel(inst, flags, 3u, texel, color, flags);
        uint32_t raw_semi = textured && raw && semi;
        uint32_t base = raw_semi ? 29u : (textured && raw ? 13u : 12u);
        uint32_t dithered = textured && raw ? dither_expected[3u][flags & 3u]
                                            : (flags & 3u) == 1u || (flags & 3u) == 3u ? 12u
                                                                                       : 13u;
        uint32_t expected = eligible && !raw_semi ? dithered : base;
        uint32_t actual = inst->vram[3u][flags] & 0x1Fu;
        if (actual != expected) {
            fprintf(stderr, "flags=%u: expected red %u, got %u\n", flags, expected, actual);
            failures++;
        }
    }
    schway_destroy(inst);
}

static void case_palette_wrap(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return;
    }
    write_gp0(inst, 0xE3000000u);
    write_gp0(inst, 0xE407FFFFu);

    /* CLUT index 20 wraps from x=1008 to x=4 (docs/hardware/gpu.md#textured-rectangles). */
    write_gp0(inst, 0xE100008Fu); /* E1 page X=15, 8-bit depth; docs/hardware/gpu.md#textured-rectangles. */
    inst->vram[17u][970u] = 20u;
    inst->vram[12u][4u] = 0x2468u;
    rectangle(inst, 0x6Cu, 0x00808080u, 20u, 20u,
              ((12u << 6 | 63u) << 16) | (17u << 8) | 20u);
    CHECK(inst->vram[20u][20u] == 0x2468u);

    /* E2 window, UV and E1 page wrap together (docs/hardware/gpu.md#textured-rectangles). */
    clear_lookup_caches(inst);
    write_gp0(inst, 0xE100011Fu); /* E1 page X/Y and direct depth; docs/hardware/gpu.md#textured-rectangles. */
    write_gp0(inst, 0xE2007C1Fu); /* E2 X mask and offset; docs/hardware/gpu.md#textured-rectangles. */
    inst->vram[256u][184u] = 0x1357u;
    rectangle(inst, 0x6Cu, 0x00808080u, 21u, 20u, 0u);
    CHECK(inst->vram[20u][21u] == 0x1357u);

    write_gp0(inst, 0xE2000000u);
    inst->vram[511u][191u] = 0x3579u;
    rectangle(inst, 0x6Cu, 0x00808080u, 22u, 20u, 0x0000FFFFu);
    CHECK(inst->vram[20u][22u] == 0x3579u);
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-gpu-pixels <case>\n");
        return 2;
    }
    if (strcmp(argv[1], "textured_rectangle_mmio") == 0) {
        case_textured_rectangle_mmio();
    } else if (strcmp(argv[1], "arithmetic") == 0) {
        case_arithmetic();
    } else if (strcmp(argv[1], "palette_wrap") == 0) {
        case_palette_wrap();
    } else {
        fprintf(stderr, "unknown GPU pixel case: %s\n", argv[1]);
        return 2;
    }
    return failures == 0 ? 0 : 1;
}
