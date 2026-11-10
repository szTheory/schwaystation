/* GPU pixel tests use the bus-facing GP0 packet path and native RGB555 VRAM. */
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
    write_gp0(inst, 0x6C808080u);
    write_gp0(inst, 10u | (10u << 16));
    write_gp0(inst, (5u << 8) | 3u);

    CHECK(inst->vram[10u][10u] == 0x001Fu);
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
