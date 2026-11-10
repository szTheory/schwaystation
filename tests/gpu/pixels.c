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

    /* E1 selects page X=1 and direct 15-bit texels; E3/E4 expose the target. */
    inst->vram[5u][67u] = 0x001Fu;
    write_gp0(inst, 0xE1000101u);
    write_gp0(inst, 0xE3000000u);
    write_gp0(inst, 0xE407FFFFu);
    write_gp0(inst, 0xE6000000u);

    /* 6Ch is a fixed 1x1 modulated texture rectangle at (10,10), UV=(3,5). */
    rectangle(inst, 0x6Cu, 0x00808080u, 10u, 10u, (5u << 8) | 3u);
    CHECK(inst->vram[10u][10u] == 0x001Fu);

    /* Four-bit packed texels select a cached palette entry after E2 windowing. */
    clear_lookup_caches(inst);
    write_gp0(inst, 0xE1000002u); /* page X=2, 4-bit depth */
    write_gp0(inst, 0xE2000401u); /* mask X=1 and offset X=1 */
    inst->vram[7u][128u + (11u >> 2)] = 0xA000u;
    inst->vram[4u][16u + 10u] = 0x1234u;
    rectangle(inst, 0x6Cu, 0x00808080u, 11u, 10u,
              (257u << 16) | (7u << 8) | 3u);
    CHECK(inst->vram[10u][11u] == 0x1234u);

    /* Eight-bit packed texels use the high byte and a 256-entry CLUT. */
    clear_lookup_caches(inst);
    write_gp0(inst, 0xE1000083u); /* page X=3, 8-bit depth */
    write_gp0(inst, 0xE2000000u);
    inst->vram[9u][192u + (5u >> 1)] = 0xABCDu;
    inst->vram[5u][32u + 0xABu] = 0x2345u;
    rectangle(inst, 0x6Cu, 0x00808080u, 12u, 10u,
              (322u << 16) | (9u << 8) | 5u);
    CHECK(inst->vram[10u][12u] == 0x2345u);

    /* A zero decoded texel leaves the old destination untouched. */
    clear_lookup_caches(inst);
    write_gp0(inst, 0xE1000101u);
    inst->vram[5u][68u] = 0u;
    inst->vram[10u][13u] = 0x4567u;
    rectangle(inst, 0x6Cu, 0x00808080u, 13u, 10u, (5u << 8) | 4u);
    CHECK(inst->vram[10u][13u] == 0x4567u);

    /* ABE blends only when STP is set in the fetched 15-bit texel. */
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

    /* E6 skips a marked destination and forces bit 15 on an accepted write. */
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

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-gpu-pixels <case>\n");
        return 2;
    }
    if (strcmp(argv[1], "textured_rectangle_mmio") == 0) {
        case_textured_rectangle_mmio();
    } else {
        fprintf(stderr, "unknown GPU pixel case: %s\n", argv[1]);
        return 2;
    }
    return failures == 0 ? 0 : 1;
}
