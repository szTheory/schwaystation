/* The three root counters on the system clock: schwaystation-test-timers
   <case>. White-box: the test sees src/, so it drives the registers through
   the bus at known ticks, reads the state and runs the scheduler by hand where
   a case needs every event. Hardware facts are the psx-spx Timers page
   (sources in docs/hardware/timers.md): the reset at the target takes target + 2
   cycles, a counter written shows its value for two cycles before it counts,
   and the four interrupt modes. The page does not say how many cycles a written
   value holds for, so the hold cases pin the model's reading of it. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

#define COUNT(n) (0x1F801100u + 0x10u * (n))
#define MODE(n) (COUNT(n) + 4u)
#define TARGET(n) (COUNT(n) + 8u)
#define ISTAT 0x1F801070u

/* MODE bits a case sets: synchronisation off, the reset point, the interrupt
   sources, repeat and toggle, and the clock source for counter 2 (system clock
   over 8). */
#define M_RESET_TARGET 0x008u
#define M_IRQ_TARGET 0x010u
#define M_IRQ_FFFF 0x020u
#define M_REPEAT 0x040u
#define M_TOGGLE 0x080u
#define M_DIV8 0x200u

#define M_BIT10 0x400u
#define M_REACHED_TARGET 0x800u
#define M_REACHED_FFFF 0x1000u

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

/* One access through the bus at the instance's current tick. */
static void wr32(schway *inst, uint32_t addr, uint32_t v)
{
    uint32_t cycles = 0u;
    schway__sched_sync(inst);
    CHECK_EQ_U64(schway__bus_write(inst, addr, 4u, v, &cycles), 0u);
}

static uint32_t rd32(schway *inst, uint32_t addr)
{
    uint32_t cycles = 0u;
    uint32_t v = 0u;
    schway__sched_sync(inst);
    CHECK_EQ_U64(schway__bus_read(inst, addr, 4u, &v, &cycles), 0u);
    return v;
}

static schway_run_result run(schway *inst, uint64_t cycles)
{
    schway_budget b;
    memset(&b, 0, sizeof b);
    b.size = (uint32_t)sizeof b;
    b.max_cycles = cycles;
    schway_run_result r;
    memset(&r, 0, sizeof r);
    r.size = (uint32_t)sizeof r;
    CHECK_EQ_U64(schway_run(inst, &b, &r), SCHWAY_STATUS_OK);
    return r;
}

/* Runs the no-program loop, which stops at every deadline, until the instance
   is at tick. A run never crosses a frame end, so a long span takes several
   calls. */
static void advance_to(schway *inst, uint64_t tick)
{
    while (inst->ticks < tick) {
        (void)run(inst, tick - inst->ticks);
    }
    CHECK_EQ_U64(inst->ticks, tick);
}

/* Acknowledges one I_STAT bit with a write of 0 to it and 1 elsewhere. */
static void ack(schway *inst, uint32_t bit)
{
    wr32(inst, ISTAT, ~(UINT32_C(1) << bit));
}

static uint32_t stat_bit(const schway *inst, uint32_t bit)
{
    return (inst->irq.stat >> bit) & 1u;
}

/* Fault prevented: a counter that does not count the system clock over 8 at
   the absolute tick's multiples of 8, a FFFFh wrap that is not 10000h cycles
   long or sets its reached flag at the wrong tick, an interrupt that is not
   raised by the scheduler at the tick of the event, or raised twice. Counter 2
   with source 2 and the interrupt at FFFFh in repeat mode: MODE written at tick
   0 starts a two-cycle hold of 8 ticks each, the 65,535 counts after it reach
   FFFFh at tick 8 * 10001h, and the loop after that is 10000h cycles of 8. */
static void case_div8_wrap_irq(void)
{
    schway *inst = make();
    uint64_t first = 8u * 0x10001u;
    uint64_t second = first + 8u * 0x10000u;
    wr32(inst, MODE(2u), M_DIV8 | M_IRQ_FFFF | M_REPEAT);
    CHECK_EQ_U64(inst->ticks, 0u);
    CHECK_EQ_U64(inst->sched.next, first);

    advance_to(inst, first - 1u);
    CHECK_EQ_U64(stat_bit(inst, 6u), 0u);
    advance_to(inst, first);
    CHECK_EQ_U64(stat_bit(inst, 6u), 1u);
    CHECK_EQ_U64(inst->sched.next, UINT64_MAX);
    CHECK_EQ_U64(rd32(inst, COUNT(2u)), 0xFFFFu);
    CHECK_EQ_HEX(rd32(inst, MODE(2u)), M_DIV8 | M_IRQ_FFFF | M_REPEAT | M_BIT10 | M_REACHED_FFFF);
    CHECK_EQ_HEX(rd32(inst, MODE(2u)) & (M_REACHED_TARGET | M_REACHED_FFFF), 0u);

    ack(inst, 6u);
    CHECK_EQ_U64(stat_bit(inst, 6u), 0u);
    CHECK_EQ_U64(inst->sched.next, second);
    advance_to(inst, first + 7u);
    CHECK_EQ_U64(rd32(inst, COUNT(2u)), 0xFFFFu);
    advance_to(inst, second - 1u);
    CHECK_EQ_U64(stat_bit(inst, 6u), 0u);
    advance_to(inst, second);
    CHECK_EQ_U64(stat_bit(inst, 6u), 1u);
    advance_to(inst, second + 8u);
    CHECK_EQ_U64(rd32(inst, COUNT(2u)), 0u);
    advance_to(inst, second + 16u);
    CHECK_EQ_U64(rd32(inst, COUNT(2u)), 1u);
    schway_destroy(inst);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"div8_wrap_irq", case_div8_wrap_irq},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-timers <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "schwaystation-test-timers: unknown case %s\n", argv[1]);
    return 2;
}
