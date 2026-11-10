/* GPU command tests enter the ports through the real bus. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"
#include "bus.h"

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

static void write_port(schway *inst, uint32_t addr, uint32_t word)
{
    uint32_t cycles = 0u;
    CHECK(schway__bus_write(inst, addr, 4u, word, &cycles) == 0);
}

static void test_fill_mmio(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return;
    }
    write_port(inst, GP0, 0x020000F8u); /* fill, RGB888 red */
    write_port(inst, GP0, (7u << 16) | 19u); /* x aligns down to 16 */
    write_port(inst, GP0, (2u << 16) | 17u); /* width rounds up to 32 */
    CHECK(inst->vram[7][15] == 0u);
    CHECK(inst->vram[7][16] == 0x001Fu);
    CHECK(inst->vram[7][47] == 0x001Fu);
    CHECK(inst->vram[7][48] == 0u);
    CHECK(inst->vram[8][16] == 0x001Fu);
    CHECK(inst->vram[9][16] == 0u);
    schway_destroy(inst);
}

static void test_partial_packet_reset(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return;
    }
    write_port(inst, GP0, 0x020000F8u);
    write_port(inst, GP0, (7u << 16) | 19u);
    write_port(inst, GP1, 0x01000000u); /* reset command buffer */
    write_port(inst, GP0, (2u << 16) | 17u);
    CHECK(inst->vram[7][16] == 0u);
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-gpu <fill_mmio|partial_packet_reset>\n");
        return 2;
    }
    if (strcmp(argv[1], "fill_mmio") == 0) {
        test_fill_mmio();
    } else if (strcmp(argv[1], "partial_packet_reset") == 0) {
        test_partial_packet_reset();
    } else {
        fprintf(stderr, "unknown GPU case: %s\n", argv[1]);
        return 2;
    }
    if (failures != 0) {
        fprintf(stderr, "GPU test %s: %d assertion(s) failed\n", argv[1], failures);
        return 1;
    }
    return 0;
}
