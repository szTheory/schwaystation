/* CD-ROM controller register tests: schwaystation-test-cdrom <case>.
   Each case drives the real MMIO and scheduler, then checks device-visible
   registers and the instance's saved device state. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

#define CDROM 0x1F801800u
#define I_STAT 0x1F801070u

static void *test_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void test_free(void *user, void *ptr, size_t size)
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
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) {
        exit(1);
    }
    return inst;
}

static void wr8(schway *inst, uint32_t offset, uint8_t value)
{
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, CDROM + offset, 1u, value, &cycles), 0u);
}

static uint8_t rd8(schway *inst, uint32_t offset)
{
    uint32_t cycles = 0u;
    uint32_t value = 0u;
    CHECK_EQ_U64(schway__bus_read(inst, CDROM + offset, 1u, &value, &cycles), 0u);
    return (uint8_t)value;
}

static uint32_t rd32(schway *inst, uint32_t address)
{
    uint32_t cycles = 0u;
    uint32_t value = 0u;
    CHECK_EQ_U64(schway__bus_read(inst, address, 4u, &value, &cycles), 0u);
    return value;
}

static void run_cycles(schway *inst, uint64_t cycles)
{
    while (cycles != 0u) {
        schway_budget budget;
        memset(&budget, 0, sizeof budget);
        budget.size = (uint32_t)sizeof budget;
        budget.max_cycles = cycles;
        schway_run_result result;
        memset(&result, 0, sizeof result);
        result.size = (uint32_t)sizeof result;
        uint64_t before = inst->ticks;
        CHECK_EQ_U64(schway_run(inst, &budget, &result), SCHWAY_STATUS_OK);
        uint64_t elapsed = result.ticks - before;
        CHECK(elapsed != 0u);
        CHECK(elapsed <= cycles);
        if (elapsed == 0u || elapsed > cycles) {
            return;
        }
        cycles -= elapsed;
    }
}

static uint8_t hint_status(schway *inst)
{
    wr8(inst, 0u, 1u);
    return rd8(inst, 3u);
}

static void acknowledge(schway *inst, uint8_t interrupt)
{
    wr8(inst, 0u, 1u);
    wr8(inst, 3u, (uint8_t)(1u << (interrupt - 1u)));
}

static void command(schway *inst, uint8_t value)
{
    wr8(inst, 0u, 0u);
    wr8(inst, 1u, value);
}

static void test_command(void)
{
    schway *inst = make();
    wr8(inst, 0u, 1u);
    wr8(inst, 2u, 0u); /* Keep INT3 masked until its status is pending. */
    command(inst, 0x01u); /* Nop. */
    run_cycles(inst, 50000u);

    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    CHECK_EQ_U64(rd32(inst, I_STAT) & (1u << 2), 0u);
    wr8(inst, 2u, 0x04u); /* Enabling a pending INT3 raises IRQ2. */
    CHECK((rd32(inst, I_STAT) & (1u << 2)) != 0u);
    CHECK_EQ_U64(rd8(inst, 1u), 0u); /* Nop returns the current status. */
    acknowledge(inst, 3u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 0u);

    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, I_STAT, 4u, 0u, &cycles), 0u);
    CHECK_EQ_U64(rd32(inst, I_STAT) & (1u << 2), 0u);
    schway_destroy(inst);
}

static void test_timing(void)
{
    schway *inst = make();
    command(inst, 0x01u);
    run_cycles(inst, 49999u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 0u);
    CHECK_EQ_U64(rd32(inst, I_STAT) & (1u << 2), 0u);

    run_cycles(inst, 1u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    CHECK_EQ_U64(rd8(inst, 1u), 0u);
    schway_destroy(inst);
}

static void test_bounds(void)
{
    schway *inst = make();
    wr8(inst, 0u, 0u);
    for (uint32_t i = 0u; i < SCHWAY__CDROM_FIFO_CAP + 1u; i++) {
        wr8(inst, 2u, (uint8_t)i);
    }
    CHECK_EQ_U64(inst->cdrom.parameter_count, SCHWAY__CDROM_FIFO_CAP);
    CHECK_EQ_U64(rd8(inst, 0u) & 0x18u, 0u); /* Not empty, no more write room. */

    wr8(inst, 0u, 1u);
    wr8(inst, 3u, 0x40u); /* Clear parameter FIFO. */
    wr8(inst, 0u, 0u);
    CHECK_EQ_U64(rd8(inst, 0u) & 0x18u, 0x18u);
    CHECK_EQ_U64(rd8(inst, 1u), 0u); /* Empty result reads are zero. */

    for (uint32_t i = 0u; i < SCHWAY__CDROM_FIFO_CAP; i++) {
        command(inst, 0x01u);
        run_cycles(inst, 50000u);
        acknowledge(inst, 3u);
    }
    CHECK_EQ_U64(inst->cdrom.result_count, SCHWAY__CDROM_FIFO_CAP);
    for (uint32_t i = 0u; i < 10u; i++) {
        CHECK_EQ_U64(rd8(inst, 1u), 0u);
    }
    CHECK_EQ_U64(inst->cdrom.result_head, 10u);
    for (uint32_t i = 0u; i < 10u; i++) {
        command(inst, 0x01u);
        run_cycles(inst, 50000u);
        acknowledge(inst, 3u);
    }
    CHECK_EQ_U64(inst->cdrom.result_count, SCHWAY__CDROM_FIFO_CAP);
    for (uint32_t i = 0u; i < SCHWAY__CDROM_FIFO_CAP; i++) {
        CHECK_EQ_U64(rd8(inst, 1u), 0u);
    }
    CHECK_EQ_U64(inst->cdrom.result_head, 10u);
    CHECK_EQ_U64(inst->cdrom.result_count, 0u);
    schway_destroy(inst);
}

static void test_region(void)
{
    schway *inst = make();
    command(inst, 0x1Au); /* GetID with no installed disc. */
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    CHECK_EQ_U64(rd8(inst, 1u), 0u);
    acknowledge(inst, 3u);

    run_cycles(inst, 0x4A00u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x08u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x40u);
    for (uint32_t i = 0u; i < 6u; i++) {
        CHECK_EQ_U64(rd8(inst, 1u), 0u);
    }
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-cdrom <case>\n");
        return 2;
    }
    if (strcmp(argv[1], "command") == 0) {
        test_command();
    } else if (strcmp(argv[1], "timing") == 0) {
        test_timing();
    } else if (strcmp(argv[1], "bounds") == 0) {
        test_bounds();
    } else if (strcmp(argv[1], "region") == 0) {
        test_region();
    } else {
        fprintf(stderr, "unknown CD-ROM case: %s\n", argv[1]);
        return 2;
    }
    CHECK_DONE();
}
