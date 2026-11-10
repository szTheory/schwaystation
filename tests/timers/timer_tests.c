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
#define TEXT 0x80010000u

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

/* A read at tick: runs to it first. */
static uint32_t at(schway *inst, uint64_t tick, uint32_t addr)
{
    advance_to(inst, tick);
    return rd32(inst, addr);
}

/* A read of the given width, for the lanes of a register. */
static uint32_t rd_width(schway *inst, uint32_t addr, uint32_t width)
{
    uint32_t cycles = 0u;
    uint32_t v = 0u;
    schway__sched_sync(inst);
    CHECK_EQ_U64(schway__bus_read(inst, addr, width, &v, &cycles), 0u);
    return v;
}

static void wr_width(schway *inst, uint32_t addr, uint32_t width, uint32_t v)
{
    uint32_t cycles = 0u;
    schway__sched_sync(inst);
    CHECK_EQ_U64(schway__bus_write(inst, addr, width, v, &cycles), 0u);
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

/* Fault prevented: a reset at the target whose loop is not target + 2 cycles
   long (0, 0, 1, 0, 0, 1 at target 1), or one that dwells too long or too
   short at 0 for a larger target, or a reached flag at another tick than the
   one the counter shows the target on. The first two values of a sequence are
   the hold after MODE. */
static void case_target_period(void)
{
    static const uint32_t one[] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1};
    static const uint32_t five[] = {0, 0, 1, 2, 3, 4, 5, 0, 0, 1, 2, 3, 4, 5, 0, 0};
    schway *inst = make();
    wr32(inst, TARGET(0u), 1u);
    wr32(inst, MODE(0u), M_RESET_TARGET);
    for (uint32_t i = 0; i < sizeof one / sizeof one[0]; i++) {
        CHECK_EQ_U64(at(inst, i + 1u, COUNT(0u)), one[i]);
    }
    schway_destroy(inst);

    inst = make();
    wr32(inst, TARGET(1u), 5u);
    wr32(inst, MODE(1u), M_RESET_TARGET);
    for (uint32_t i = 0; i < sizeof five / sizeof five[0]; i++) {
        CHECK_EQ_U64(at(inst, i + 1u, COUNT(1u)), five[i]);
    }
    schway_destroy(inst);

    /* The counter shows the target on ticks 7, 14, 21: seven cycles apart. */
    inst = make();
    wr32(inst, TARGET(1u), 5u);
    wr32(inst, MODE(1u), M_RESET_TARGET);
    CHECK_EQ_HEX(at(inst, 6u, MODE(1u)) & (M_REACHED_TARGET | M_REACHED_FFFF), 0u);
    CHECK_EQ_HEX(at(inst, 7u, MODE(1u)) & (M_REACHED_TARGET | M_REACHED_FFFF), M_REACHED_TARGET);
    CHECK_EQ_HEX(at(inst, 13u, MODE(1u)) & M_REACHED_TARGET, 0u);
    CHECK_EQ_HEX(at(inst, 14u, MODE(1u)) & M_REACHED_TARGET, M_REACHED_TARGET);
    CHECK_EQ_HEX(at(inst, 21u, MODE(1u)) & M_REACHED_TARGET, M_REACHED_TARGET);
    schway_destroy(inst);
}

/* Fault prevented: a wrap from FFFFh to 0 that dwells at 0 as the target
   reset does, so that the loop is not 10000h cycles long, or a reached flag
   for FFFFh that is not set on the cycle the counter shows it. With the reset
   at FFFFh and the target 0 the counter reaches the target on the wrap. */
static void case_wrap_no_dwell(void)
{
    static const uint32_t seq[] = {0xFFFE, 0xFFFE, 0xFFFF, 0, 1, 2};
    schway *inst = make();
    wr32(inst, MODE(0u), 0u);
    wr32(inst, COUNT(0u), 0xFFFEu);
    for (uint32_t i = 0; i < sizeof seq / sizeof seq[0]; i++) {
        CHECK_EQ_U64(at(inst, i + 1u, COUNT(0u)), seq[i]);
    }
    schway_destroy(inst);

    inst = make();
    wr32(inst, MODE(0u), 0u);
    wr32(inst, COUNT(0u), 0xFFFEu);
    CHECK_EQ_HEX(at(inst, 2u, MODE(0u)) & (M_REACHED_TARGET | M_REACHED_FFFF), 0u);
    CHECK_EQ_HEX(at(inst, 3u, MODE(0u)) & (M_REACHED_TARGET | M_REACHED_FFFF), M_REACHED_FFFF);
    CHECK_EQ_HEX(at(inst, 4u, MODE(0u)) & (M_REACHED_TARGET | M_REACHED_FFFF), M_REACHED_TARGET);
    CHECK_EQ_HEX(at(inst, 3u + 0x10000u - 1u, MODE(0u)) & M_REACHED_FFFF, 0u);
    CHECK_EQ_HEX(at(inst, 3u + 0x10000u, MODE(0u)) & M_REACHED_FFFF, M_REACHED_FFFF);
    schway_destroy(inst);
}

/* Fault prevented: a counter written above the target that compares anyway
   and resets early, or one that does not compare again after the wrap. The
   counter written 10h reaches FFFFh on tick 0FFF1h, wraps on the next and
   reaches the target 5 on tick 0FFF7h. */
static void case_count_above(void)
{
    schway *inst = make();
    wr32(inst, MODE(0u), M_RESET_TARGET);
    wr32(inst, TARGET(0u), 5u);
    wr32(inst, COUNT(0u), 0x10u);
    CHECK_EQ_U64(at(inst, 1u, COUNT(0u)), 0x10u);
    CHECK_EQ_U64(at(inst, 2u, COUNT(0u)), 0x10u);
    CHECK_EQ_U64(at(inst, 3u, COUNT(0u)), 0x11u);
    CHECK_EQ_HEX(at(inst, 0x8000u, MODE(0u)) & (M_REACHED_TARGET | M_REACHED_FFFF), 0u);
    CHECK_EQ_HEX(at(inst, 0xFFF6u, MODE(0u)) & (M_REACHED_TARGET | M_REACHED_FFFF), M_REACHED_FFFF);
    CHECK_EQ_HEX(at(inst, 0xFFF7u, MODE(0u)) & (M_REACHED_TARGET | M_REACHED_FFFF), M_REACHED_TARGET);
    CHECK_EQ_U64(at(inst, 0xFFF8u, COUNT(0u)), 0u);
    CHECK_EQ_U64(at(inst, 0xFFF9u, COUNT(0u)), 0u);
    CHECK_EQ_U64(at(inst, 0xFFFAu, COUNT(0u)), 1u);
    schway_destroy(inst);
}

/* Fault prevented: a COUNT or MODE write that is not shown for two cycles
   before the counter moves, a TARGET write that holds the counter, or a lane
   of the register that is not read or written as its bytes. */
static void case_write_hold(void)
{
    schway *inst = make();
    wr32(inst, MODE(0u), 0u);
    wr32(inst, COUNT(0u), 0x100u);
    CHECK_EQ_U64(at(inst, 1u, COUNT(0u)), 0x100u);
    CHECK_EQ_U64(at(inst, 2u, COUNT(0u)), 0x100u);
    CHECK_EQ_U64(at(inst, 3u, COUNT(0u)), 0x101u);
    advance_to(inst, 10u);
    wr32(inst, MODE(0u), 0u);
    CHECK_EQ_U64(at(inst, 11u, COUNT(0u)), 0u);
    CHECK_EQ_U64(at(inst, 12u, COUNT(0u)), 0u);
    CHECK_EQ_U64(at(inst, 13u, COUNT(0u)), 1u);
    advance_to(inst, 20u);
    wr32(inst, TARGET(0u), 0x1234u);
    CHECK_EQ_U64(at(inst, 21u, COUNT(0u)), 9u);
    CHECK_EQ_U64(rd32(inst, TARGET(0u)), 0x1234u);

    /* A halfword write to the upper lane of the word does not reach a 16-bit
       register, and the lanes of TARGET read as bytes. */
    wr_width(inst, TARGET(0u) + 2u, 2u, 0xFFFFu);
    CHECK_EQ_U64(rd32(inst, TARGET(0u)), 0x1234u);
    CHECK_EQ_HEX(rd_width(inst, TARGET(0u), 1u), 0x34u);
    CHECK_EQ_HEX(rd_width(inst, TARGET(0u) + 1u, 1u), 0x12u);
    wr_width(inst, TARGET(0u) + 1u, 1u, 0xABu);
    CHECK_EQ_HEX(rd32(inst, TARGET(0u)), 0xAB34u);
    CHECK_EQ_U64(rd32(inst, COUNT(0u) + 0xCu), 0u);
    schway_destroy(inst);
}

/* Fault prevented: a MODE write that leaves the counter, the interrupt line or
   the one-shot as it was, a pause derived from the wrong synchronisation modes
   of counter 2, or bits of the register that do not read back. */
static void case_mode_write(void)
{
    schway *inst = make();
    /* The line goes to 0 on the first event of a toggling counter. */
    wr32(inst, TARGET(1u), 3u);
    wr32(inst, MODE(1u), M_RESET_TARGET | M_IRQ_TARGET | M_REPEAT | M_TOGGLE);
    CHECK_EQ_HEX(at(inst, 4u, MODE(1u)) & M_BIT10, M_BIT10);
    CHECK_EQ_HEX(at(inst, 5u, MODE(1u)) & M_BIT10, 0u);
    advance_to(inst, 6u);
    wr32(inst, MODE(1u), M_RESET_TARGET | M_IRQ_TARGET | M_REPEAT | M_TOGGLE);
    CHECK_EQ_U64(at(inst, 7u, COUNT(1u)), 0u);
    CHECK_EQ_HEX(rd32(inst, MODE(1u)), M_RESET_TARGET | M_IRQ_TARGET | M_REPEAT | M_TOGGLE | M_BIT10);
    CHECK_EQ_U64(at(inst, 8u, COUNT(1u)), 0u);
    CHECK_EQ_U64(at(inst, 9u, COUNT(1u)), 1u);
    schway_destroy(inst);

    /* Every written bit reads back, bit 10 is 1 after the write and the
       reached flags are not part of what is written. Source 3 of counter 0 is
       the dot clock, which has no source yet, so the counter stays at 0. */
    inst = make();
    wr32(inst, MODE(0u), 0xFFFFu);
    CHECK_EQ_HEX(rd32(inst, MODE(0u)), 0x7FFu);
    wr32(inst, MODE(0u), 0u);
    CHECK_EQ_HEX(rd32(inst, MODE(0u)), M_BIT10);
    schway_destroy(inst);

    /* Counter 2 stops for synchronisation modes 0 and 3 and runs for 1 and 2. */
    inst = make();
    static const struct {
        uint32_t mode;
        uint32_t value;
    } sync[] = {{0x1u, 0u}, {0x3u, 98u}, {0x5u, 98u}, {0x7u, 0u}, {0x0u, 98u}};
    uint64_t t0 = 0u;
    for (uint32_t i = 0; i < sizeof sync / sizeof sync[0]; i++) {
        wr32(inst, MODE(2u), sync[i].mode);
        CHECK_EQ_U64(at(inst, t0 + 100u, COUNT(2u)), sync[i].value);
        t0 += 100u;
    }
    schway_destroy(inst);
}

/* Fault prevented: reached flags that survive a MODE read, a read that clears
   only one of them, or a flag set on another cycle than the one the counter
   shows the value on. With the target at FFFFh the counter reaches both on
   tick 10001h. */
static void case_mode_read_clears(void)
{
    schway *inst = make();
    wr32(inst, TARGET(0u), 0xFFFFu);
    wr32(inst, MODE(0u), M_RESET_TARGET);
    CHECK_EQ_HEX(at(inst, 0x10000u, MODE(0u)) & (M_REACHED_TARGET | M_REACHED_FFFF), 0u);
    CHECK_EQ_HEX(at(inst, 0x10001u, MODE(0u)) & (M_REACHED_TARGET | M_REACHED_FFFF),
                 M_REACHED_TARGET | M_REACHED_FFFF);
    CHECK_EQ_HEX(rd32(inst, MODE(0u)), M_RESET_TARGET | M_BIT10);
    schway_destroy(inst);
}

/* Fault prevented: a one-shot counter that raises more than once, one that is
   not re-armed by a MODE write, or an interrupt on another tick than the
   target's: with the target 2 the counter shows it on ticks 4, 8, 12. */
static void case_irq_oneshot(void)
{
    schway *inst = make();
    wr32(inst, TARGET(0u), 2u);
    wr32(inst, MODE(0u), M_RESET_TARGET | M_IRQ_TARGET);
    advance_to(inst, 3u);
    CHECK_EQ_U64(stat_bit(inst, 4u), 0u);
    advance_to(inst, 4u);
    CHECK_EQ_U64(stat_bit(inst, 4u), 1u);
    advance_to(inst, 5u);
    ack(inst, 4u);
    CHECK_EQ_U64(inst->sched.next, UINT64_MAX);
    advance_to(inst, 40u);
    CHECK_EQ_U64(stat_bit(inst, 4u), 0u);
    wr32(inst, MODE(0u), M_RESET_TARGET | M_IRQ_TARGET);
    advance_to(inst, 43u);
    CHECK_EQ_U64(stat_bit(inst, 4u), 0u);
    advance_to(inst, 44u);
    CHECK_EQ_U64(stat_bit(inst, 4u), 1u);
    schway_destroy(inst);
}

/* Fault prevented: a repeating counter that raises while the bit is still
   set, that raises for an event that came before the acknowledgement, or that
   does not raise after it. Events at 4, 8, 12: the one at 8 happens while the
   bit is set and an acknowledgement at 9 must not bring it back. */
static void case_irq_repeat(void)
{
    schway *inst = make();
    wr32(inst, TARGET(0u), 2u);
    wr32(inst, MODE(0u), M_RESET_TARGET | M_IRQ_TARGET | M_REPEAT);
    advance_to(inst, 4u);
    CHECK_EQ_U64(stat_bit(inst, 4u), 1u);
    CHECK_EQ_U64(inst->sched.next, UINT64_MAX);
    advance_to(inst, 9u);
    ack(inst, 4u);
    CHECK_EQ_U64(stat_bit(inst, 4u), 0u);
    CHECK_EQ_U64(inst->sched.next, 12u);
    advance_to(inst, 11u);
    CHECK_EQ_U64(stat_bit(inst, 4u), 0u);
    advance_to(inst, 12u);
    CHECK_EQ_U64(stat_bit(inst, 4u), 1u);
    schway_destroy(inst);
}

/* Fault prevented: pulse mode that moves bit 10 or skips an edge after an
   acknowledgement. Each of five events, acknowledged at its own tick, raises
   again at the next. */
static void case_irq_pulse(void)
{
    schway *inst = make();
    wr32(inst, TARGET(1u), 2u);
    wr32(inst, MODE(1u), M_RESET_TARGET | M_IRQ_TARGET | M_REPEAT);
    for (uint32_t k = 1; k <= 5u; k++) {
        advance_to(inst, 4u * k - 1u);
        CHECK_EQ_U64(stat_bit(inst, 5u), 0u);
        advance_to(inst, 4u * k);
        CHECK_EQ_U64(stat_bit(inst, 5u), 1u);
        CHECK_EQ_HEX(rd32(inst, MODE(1u)) & M_BIT10, M_BIT10);
        ack(inst, 5u);
    }
    schway_destroy(inst);
}

/* Fault prevented: toggle mode that raises on every event, that raises on the
   flip back to 1, that loses the parity of a span of events that nobody
   watched, or that gives the scheduler the next event instead of the next
   raise. Events at 4, 8, 12: bit 10 is 0 after 4, 1 after 8; the raises are at
   4 and 12. */
static void case_irq_toggle(void)
{
    schway *inst = make();
    wr32(inst, TARGET(0u), 2u);
    wr32(inst, MODE(0u), M_RESET_TARGET | M_IRQ_TARGET | M_REPEAT | M_TOGGLE);
    advance_to(inst, 4u);
    CHECK_EQ_U64(stat_bit(inst, 4u), 1u);
    CHECK_EQ_HEX(rd32(inst, MODE(0u)) & M_BIT10, 0u);
    advance_to(inst, 5u);
    ack(inst, 4u);
    CHECK_EQ_U64(inst->sched.next, 12u);
    advance_to(inst, 9u);
    CHECK_EQ_U64(stat_bit(inst, 4u), 0u);
    CHECK_EQ_HEX(rd32(inst, MODE(0u)) & M_BIT10, M_BIT10);
    advance_to(inst, 12u);
    CHECK_EQ_U64(stat_bit(inst, 4u), 1u);
    CHECK_EQ_HEX(rd32(inst, MODE(0u)) & M_BIT10, 0u);
    ack(inst, 4u);
    CHECK_EQ_U64(inst->sched.next, 20u);
    schway_destroy(inst);

    /* A span of 25 events nobody watched is one raise and an odd count: the
       line ends at 0. The counters catch up at tick 100 with no run. */
    inst = make();
    wr32(inst, TARGET(0u), 2u);
    wr32(inst, MODE(0u), M_RESET_TARGET | M_IRQ_TARGET | M_REPEAT | M_TOGGLE);
    schway__timers_sync(inst, 100u);
    CHECK_EQ_U64(stat_bit(inst, 4u), 1u);
    CHECK_EQ_U64(inst->timers.t[0].irq_line, 0u);
    schway_destroy(inst);
}

/* The frame's no-program loop with a count of the events it runs: the scheduler
   stops at every deadline, runs what is due and goes on. The deadline each
   event leaves behind is checked here too, since a deadline at or before the
   tick it was made at would run again at once and never advance. */
static uint64_t drive(schway *inst, uint64_t until, int ack_all)
{
    uint64_t events = 0u;
    while (inst->ticks < until) {
        if (inst->sched.next <= inst->ticks) {
            uint64_t at_tick = inst->ticks;
            schway__sched_run(inst);
            events++;
            CHECK(inst->sched.next > at_tick);
            if (ack_all != 0) {
                wr32(inst, ISTAT, 0u);
                CHECK(inst->sched.next > at_tick);
            }
        }
        uint64_t step = until - inst->ticks;
        if (inst->sched.next > inst->ticks && inst->sched.next - inst->ticks < step) {
            step = inst->sched.next - inst->ticks;
        }
        inst->ticks += step;
    }
    schway__sched_sync(inst);
    return events;
}

/* Fault prevented: a pulse-repeat counter at target 0, which has an event every
   second cycle, that gives the scheduler an event each. Without an
   acknowledgement a whole frame is one event (one raise: setting the bit again
   changes nothing) and the scheduler is left with no deadline; an
   acknowledgement lets exactly one more through. */
static void case_storm_collapse(void)
{
    schway *inst = make();
    uint64_t frame = inst->frame_end;
    wr32(inst, TARGET(0u), 0u);
    wr32(inst, MODE(0u), M_RESET_TARGET | M_IRQ_TARGET | M_REPEAT);
    CHECK_EQ_U64(inst->sched.next, 3u);
    CHECK_EQ_U64(drive(inst, frame, 0), 1u);
    CHECK_EQ_U64(stat_bit(inst, 4u), 1u);
    CHECK_EQ_U64(inst->sched.next, UINT64_MAX);
    ack(inst, 4u);
    CHECK(inst->sched.next > inst->ticks);
    CHECK_EQ_U64(drive(inst, frame + 1000u, 0), 1u);
    CHECK_EQ_U64(stat_bit(inst, 4u), 1u);
    schway_destroy(inst);
}

/* Fault prevented: a deadline at or before the tick it was computed at, in any
   mode of any counter, which would make the scheduler run the same event
   forever. All three counters are armed in each of the 24 combinations of reset
   point, interrupt sources, repeat and toggle, with counts near FFFFh so both
   sources fire; every event is acknowledged at once so that deadlines go on
   being made. */
static void case_deadline_progress(void)
{
    for (uint32_t m = 0; m < 24u; m++) {
        uint32_t mode = ((m & 1u) != 0u ? M_RESET_TARGET : 0u) | ((m & 2u) != 0u ? M_REPEAT : 0u) |
                        ((m & 4u) != 0u ? M_TOGGLE : 0u);
        uint32_t irq = m / 8u;
        mode |= irq == 0u ? M_IRQ_TARGET : irq == 1u ? M_IRQ_FFFF : (M_IRQ_TARGET | M_IRQ_FFFF);
        schway *inst = make();
        static const uint32_t targets[3] = {1u, 5u, 2u};
        for (uint32_t i = 0; i < 3u; i++) {
            wr32(inst, TARGET(i), targets[i]);
            wr32(inst, MODE(i), mode | (i == 2u ? M_DIV8 : 0u));
            wr32(inst, COUNT(i), 0xFFF8u);
        }
        uint64_t events = drive(inst, 3000u, 1);
        CHECK(events >= 1u);
        CHECK(events <= 3000u);
        schway_destroy(inst);
    }
}

static size_t save_state(const schway *inst, uint8_t **buf)
{
    size_t n = schway_state_size(inst);
    *buf = (uint8_t *)malloc(n);
    if (*buf == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(schway_save_state(inst, *buf, n), SCHWAY_STATUS_OK);
    return n;
}

/* The two instances' states are byte for byte equal, and saving moved nothing
   in either. */
static void same_state(schway *a, schway *b)
{
    struct schway__timers before = a->timers;
    uint8_t *sa;
    uint8_t *sb;
    size_t na = save_state(a, &sa);
    size_t nb = save_state(b, &sb);
    CHECK(na == nb && memcmp(sa, sb, na) == 0);
    CHECK(memcmp(before.t, a->timers.t, sizeof before.t) == 0 && before.base == a->timers.base);
    free(sa);
    free(sb);
}

/* Fault prevented: a state that depends on when the counters were last brought
   up to date, or a save that changes the instance. Two instances run one
   program of register accesses and runs across a frame end; one is caught up
   before and after every step as well and the other only by the accesses
   themselves, and their saved states are equal at both points of every step. */
static void case_state_cadence(void)
{
    static const struct {
        uint64_t tick;
        uint32_t addr, value;
    } program[] = {
        {0u, TARGET(0u), 100u},
        {0u, MODE(0u), M_RESET_TARGET | M_IRQ_TARGET | M_REPEAT | M_TOGGLE},
        {0u, MODE(1u), M_IRQ_FFFF},
        {0u, COUNT(1u), 0xFFF0u},
        {0u, TARGET(2u), 5000u},
        {0u, MODE(2u), M_RESET_TARGET | M_IRQ_TARGET | M_REPEAT | M_DIV8},
        {37u, ISTAT, 0u},
        {5000u, MODE(0u), M_RESET_TARGET | M_IRQ_TARGET | M_REPEAT | M_TOGGLE},
        {12345u, ISTAT, 0u},
        {70001u, COUNT(2u), 4990u},
        {100000u, ISTAT, 0u},
        {200001u, MODE(0u), 0u},
        {600000u, COUNT(1u), 0xFFFFu},
        {700000u, ISTAT, 0u},
    };
    schway *a = make();
    schway *b = make();
    for (size_t i = 0; i < sizeof program / sizeof program[0]; i++) {
        advance_to(a, program[i].tick);
        advance_to(b, program[i].tick);
        schway__timers_sync(a, a->ticks);
        same_state(a, b);
        wr32(a, program[i].addr, program[i].value);
        wr32(b, program[i].addr, program[i].value);
        schway__timers_sync(a, a->ticks);
        same_state(a, b);
    }
    advance_to(a, 800000u);
    advance_to(b, 800000u);
    same_state(a, b);
    schway_destroy(a);
    schway_destroy(b);
}

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

/* A PS-EXE of one text sector at 80010000h holding the n words, entered at its
   first word. */
static void load(schway *inst, const uint32_t *words, uint32_t n)
{
    static uint8_t exe[0x1000];
    memset(exe, 0, sizeof exe);
    memcpy(exe, "PS-X EXE", 8);
    put32(exe + 0x10, TEXT);
    put32(exe + 0x18, TEXT);
    put32(exe + 0x1C, 0x800u);
    for (uint32_t i = 0; i < n; i++) {
        put32(exe + 0x800u + 4u * i, words[i]);
    }
    CHECK_EQ_U64(schway_load_exe(inst, exe, sizeof exe), SCHWAY_STATUS_OK);
}

/* Fault prevented: a counter synchronised to the vertical blank that does not
   see the blank the frame end produces, so that a reset at the blank never
   happens and the counter just free-runs. A program sets counter 1 to the
   system clock with synchronisation on and mode 1 (reset at the blank), then
   spins to the frame end. Just after it the counter reads the cycles since the
   frame end (0 at the end itself), not the cycles since the MODE write, which
   were hundreds of thousands. */
static void case_t1_vblank_frame(void)
{
    static const uint32_t program[] = {
        0x3C081F80u, /* lui   t0, 1F80h */
        0x35081110u, /* ori   t0, t0, 1110h */
        0x24090003u, /* addiu t1, zero, 3 (synchronise, mode 1) */
        0xAD090004u, /* sw    t1, 4(t0) */
        0x1000FFFFu, /* beq   zero, zero, here */
        0x00000000u, /* nop */
    };
    schway *inst = make();
    load(inst, program, (uint32_t)(sizeof program / sizeof program[0]));
    schway_run_result r;
    memset(&r, 0, sizeof r);
    r.size = (uint32_t)sizeof r;
    CHECK_EQ_U64(schway_run_frame(inst, &r), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    CHECK_EQ_HEX(rd32(inst, MODE(1u)) & 0x3FFu, 3u);
    uint64_t end = inst->ticks;
    CHECK_EQ_U64(rd32(inst, COUNT(1u)), 0u);
    (void)run(inst, 1000u);
    CHECK(inst->ticks > end);
    CHECK_EQ_U64(rd32(inst, COUNT(1u)), (inst->ticks - end) & 0xFFFFu);
    schway_destroy(inst);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"count_above", case_count_above},
    {"deadline_progress", case_deadline_progress},
    {"div8_wrap_irq", case_div8_wrap_irq},
    {"irq_oneshot", case_irq_oneshot},
    {"irq_pulse", case_irq_pulse},
    {"irq_repeat", case_irq_repeat},
    {"irq_toggle", case_irq_toggle},
    {"mode_read_clears", case_mode_read_clears},
    {"mode_write", case_mode_write},
    {"state_cadence", case_state_cadence},
    {"storm_collapse", case_storm_collapse},
    {"t1_vblank_frame", case_t1_vblank_frame},
    {"target_period", case_target_period},
    {"wrap_no_dwell", case_wrap_no_dwell},
    {"write_hold", case_write_hold},
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
