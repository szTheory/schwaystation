/* SIO0 protocol tests through JOY MMIO and the instance scheduler. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

#define JOY_DATA 0x1F801040u
#define JOY_STAT 0x1F801044u
#define JOY_MODE 0x1F801048u
#define JOY_CTRL 0x1F80104Au
#define JOY_BAUD 0x1F80104Eu
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

static void write_mmio(schway *inst, uint32_t addr, uint32_t width, uint32_t value)
{
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, addr, width, value, &cycles), 0u);
}

static uint32_t read_mmio(schway *inst, uint32_t addr, uint32_t width)
{
    uint32_t cycles = 0u;
    uint32_t value = 0u;
    CHECK_EQ_U64(schway__bus_read(inst, addr, width, &value, &cycles), 0u);
    return value;
}

static void advance_to(schway *inst, uint64_t tick)
{
    inst->ticks = tick;
    schway__sched_sync(inst);
    schway__sched_run(inst);
}

static uint64_t transfer(schway *inst, uint8_t sent, uint8_t expected, int last)
{
    uint64_t start = inst->ticks;
    write_mmio(inst, JOY_DATA, 1u, sent);

    advance_to(inst, start + 100u);
    CHECK((inst->irq.stat & 0x80u) == 0u);
    advance_to(inst, start + 1087u);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & 2u) == 0u);
    advance_to(inst, start + 1088u);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & 2u) != 0u);
    CHECK_EQ_HEX(read_mmio(inst, JOY_DATA, 1u), expected);

    if (last) {
        advance_to(inst, start + 1494u);
        CHECK((inst->irq.stat & 0x80u) == 0u);
        CHECK((read_mmio(inst, JOY_STAT, 4u) & 0x280u) == 0u);
        return inst->ticks;
    }

    advance_to(inst, start + 1425u);
    CHECK((inst->irq.stat & 0x80u) == 0u);
    advance_to(inst, start + 1426u);
    CHECK((inst->irq.stat & 0x80u) != 0u);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & 0x280u) == 0x280u);
    write_mmio(inst, I_STAT, 4u, 0x80u);
    write_mmio(inst, JOY_CTRL, 2u, 0x1013u);
    advance_to(inst, start + 1494u);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & 0x80u) == 0u);
    write_mmio(inst, JOY_CTRL, 2u, 0x1003u);
    return inst->ticks;
}

static void digital_poll_irq(void)
{
    schway *inst = make();
    write_mmio(inst, JOY_MODE, 2u, 0x000Cu);
    write_mmio(inst, JOY_BAUD, 2u, 0x0088u);
    write_mmio(inst, JOY_CTRL, 2u, 0x1003u);

    (void)transfer(inst, 0x01u, 0xFFu, 0);
    (void)transfer(inst, 0x42u, 0x41u, 0);
    (void)transfer(inst, 0x00u, 0x5Au, 0);
    (void)transfer(inst, 0x00u, 0xFFu, 0);
    (void)transfer(inst, 0x00u, 0xFFu, 1);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & 2u) == 0u);
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    if (argc != 2 || strcmp(argv[1], "digital_poll_irq") != 0) {
        return 2;
    }
    digital_poll_irq();
    CHECK_DONE();
}
