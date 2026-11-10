/* The root counters (psx-spx, Timers; the public timers page in
   docs/hardware/timers.md says what each rule rests on).

   Each counter is lazy and closed-form. It keeps its view as of one tick
   (timers.base); the value, the reached bits and the interrupt effects at any
   later tick are O(1) functions of the view and the number of source ticks in
   between. Every access to the registers and every I_STAT access first brings
   the counters up to the tick of the access, so a program that polls COUNT
   inside one block reads live values. The only deadline a counter gives the
   scheduler is the next event that would raise a new I_STAT edge. */
#include <string.h>

#include "internal.h"
#include "state.h"

static const char name_value[] = "timers.value";
static const char name_target[] = "timers.target";
static const char name_mode[] = "timers.mode";
static const char name_hold[] = "timers.hold";
static const char name_reached[] = "timers.reached";
static const char name_irq_line[] = "timers.irq_line";
static const char name_oneshot_done[] = "timers.oneshot_done";
static const char name_paused[] = "timers.paused";
static const char name_blank[] = "timers.blank";

/* MODE bits (psx-spx, Timers). Bits 0-9 are what a program writes; bit 10 is
   the interrupt line (1 is idle), kept in irq_line; bits 11 and 12 are the
   reached flags, kept in reached. */
#define MODE_RESET_TARGET 0x008u
#define MODE_IRQ_TARGET 0x010u
#define MODE_IRQ_FFFF 0x020u
#define MODE_REPEAT 0x040u
#define MODE_TOGGLE 0x080u
#define MODE_WRITABLE 0x3FFu

#define REACHED_TARGET 1u
#define REACHED_FFFF 2u

#define COUNT_MAX 0xFFFFu

/* A written COUNT or MODE shows its value for two of the counter's cycles
   before counting resumes; a reset at the target shows its 0 for one cycle
   more than the cycle it lands on, which makes the period target + 2. */
#define HOLD_WRITE 2u
#define HOLD_RESET 1u

/* The longest distance, in counter cycles, to an event that raises an edge:
   two events of the slowest loop plus the position before the first. */
#define EDGE_HORIZON UINT64_C(0x30000)

void schway__timers_reset(schway *inst)
{
    memset(&inst->timers, 0, sizeof inst->timers);
}

/* The lane of the 32-bit register that an access of width bytes at address p
   covers. */
static uint32_t lane_get(uint32_t reg, uint32_t p, uint32_t width)
{
    uint32_t mask = width >= 4u ? 0xFFFFFFFFu : (UINT32_C(1) << (8u * width)) - 1u;
    return (reg >> (8u * (p & 3u))) & mask;
}

static uint32_t lane_put(uint32_t old, uint32_t p, uint32_t width, uint32_t value)
{
    uint32_t mask = width >= 4u ? 0xFFFFFFFFu : (UINT32_C(1) << (8u * width)) - 1u;
    uint32_t shift = 8u * (p & 3u);
    return (old & ~(mask << shift)) | ((value << shift) & (mask << shift));
}

/* The clock a counter counts, from its MODE clock source (psx-spx, Timers):
   counters 0 and 1 count the system clock for sources 0 and 2, and the dot
   clock or horizontal blank for 1 and 3, which move when the beam pushes them
   (timers_dots, timers_hblank) and not with the ticks. Counter 2 counts the
   system clock for sources 0 and 1 and the system clock over 8 for 2 and 3. */
enum { SRC_NONE, SRC_SYS, SRC_DIV8 };

static uint32_t source_of(uint32_t n, uint32_t mode)
{
    uint32_t s = (mode >> 8) & 3u;
    if (n == 2u) {
        return s >= 2u ? SRC_DIV8 : SRC_SYS;
    }
    return (s & 1u) != 0u ? SRC_NONE : SRC_SYS;
}

/* The counter's own cycles in the ticks (from, to]. The system clock over 8
   ticks at every absolute tick that is a multiple of 8, so its phase is the
   tick itself and keeps no state. */
static uint64_t source_steps(uint32_t n, uint32_t mode, uint64_t from, uint64_t to)
{
    if (to <= from) {
        return 0u;
    }
    switch (source_of(n, mode)) {
    case SRC_SYS:
        return to - from;
    case SRC_DIV8:
        return (to >> 3) - (from >> 3);
    default:
        return 0u;
    }
}

/* The absolute tick of the counter's steps-th cycle after tick base. */
static uint64_t source_tick(uint32_t n, uint32_t mode, uint64_t base, uint64_t steps)
{
    return source_of(n, mode) == SRC_DIV8 ? ((base >> 3) + steps) << 3 : base + steps;
}

/* The blanking signal a counter synchronises to: counter 0 the horizontal
   blank, counter 1 the vertical blank (psx-spx, Timers). */
#define BLANK_V 1u
#define BLANK_H 2u

static uint32_t level_of(uint32_t n, uint32_t blank)
{
    return (blank & (n == 0u ? BLANK_H : BLANK_V)) != 0u ? 1u : 0u;
}

/* Whether a counter is paused right after a MODE write, from its
   synchronisation mode and the blank level at that moment (psx-spx, Timers).
   Counter 2 stops for modes 0 and 3 and free-runs for 1 and 2. Counters 0 and
   1 pause during the blank (mode 0), never (mode 1), outside it (mode 2) or
   until the blank has come once (mode 3). */
static uint32_t paused_for(uint32_t n, uint32_t mode, uint32_t blank)
{
    uint32_t sync_mode = (mode >> 1) & 3u;
    uint32_t level = level_of(n, blank);
    if ((mode & 1u) == 0u) {
        return 0u;
    }
    if (n == 2u) {
        return sync_mode == 0u || sync_mode == 3u ? 1u : 0u;
    }
    switch (sync_mode) {
    case 0u:
        return level;
    case 1u:
        return 0u;
    case 2u:
        return level ^ 1u;
    default:
        return 1u;
    }
}

/* How many k in [1, m] have (v + k) mod 65536 equal to x. */
static uint64_t landings(uint32_t v, uint64_t m, uint32_t x)
{
    uint64_t first = (uint64_t)((x - v - 1u) & COUNT_MAX) + 1u;
    return first > m ? 0u : 1u + (m - first) / 0x10000u;
}

struct hits {
    uint64_t target, ffff;
};

/* Moves the counter forward by n of its own cycles, adding the times it landed
   on the target and on FFFFh to h. A landing is the counter taking the value
   by counting or by a reset, never by a write. The hold runs first and the
   counter does not move while paused. With the reset at FFFFh the counter
   cycles through all 65,536 values; with the reset at the target a counter at
   the target goes to 0, shows it for the extra cycle and counts on, so one
   loop takes target + 2 cycles; a counter above the target counts up and
   wraps with no extra cycle, and compares again from 0. */
static void advance(struct schway__timer *t, uint64_t n, struct hits *h)
{
    uint64_t held = n < t->hold ? n : t->hold;
    t->hold -= (uint32_t)held;
    n -= held;
    if (t->paused != 0u || n == 0u) {
        return;
    }
    uint32_t v = t->value;
    uint32_t tgt = t->target;
    if ((t->mode & MODE_RESET_TARGET) == 0u) {
        h->target += landings(v, n, tgt);
        h->ffff += landings(v, n, COUNT_MAX);
        t->value = (uint32_t)((v + n) & COUNT_MAX);
        return;
    }
    while (n > 0u) {
        if (v < tgt) {
            uint64_t d = tgt - v;
            uint64_t m = n < d ? n : d;
            v += (uint32_t)m;
            n -= m;
            if (m == d) {
                h->target++;
                if (tgt == COUNT_MAX) {
                    h->ffff++;
                }
            }
        } else if (v == tgt) {
            uint64_t period = (uint64_t)tgt + 2u;
            uint64_t loops = n / period;
            if (loops != 0u) {
                h->target += loops;
                if (tgt == COUNT_MAX) {
                    h->ffff += loops;
                }
                n -= loops * period;
                continue;
            }
            v = 0u;
            t->hold = HOLD_RESET;
            n -= 1u;
            if (tgt == 0u) {
                h->target++;
            }
            held = n < t->hold ? n : t->hold;
            t->hold -= (uint32_t)held;
            n -= held;
        } else {
            if (v < COUNT_MAX) {
                uint64_t d = COUNT_MAX - v;
                uint64_t m = n < d ? n : d;
                v += (uint32_t)m;
                n -= m;
                if (m == d) {
                    h->ffff++;
                }
            }
            if (n > 0u && v == COUNT_MAX) {
                v = 0u;
                n -= 1u;
                if (tgt == 0u) {
                    h->target++;
                }
            }
        }
    }
    t->value = v;
}

/* Applies ne interrupt events to the counter's line and one-shot state and
   returns 1 when one of them raises the interrupt. In pulse mode every event
   raises and the line stays at 1. In toggle mode every event flips the line
   and only the flip to 0 raises, so with the line at 1 the first event raises
   and with it at 0 the second does; only the parity of the count matters for
   the line. A one-shot counter raises once and then ignores events until MODE
   is written, so only its first two events can matter. */
static uint32_t irq_events(struct schway__timer *t, uint64_t ne)
{
    uint32_t raise = 0u;
    if (ne == 0u) {
        return 0u;
    }
    if ((t->mode & MODE_REPEAT) == 0u) {
        uint64_t k = ne < 2u ? ne : 2u;
        while (k > 0u && t->oneshot_done == 0u) {
            k--;
            if ((t->mode & MODE_TOGGLE) == 0u) {
                raise = 1u;
                t->oneshot_done = 1u;
            } else {
                t->irq_line ^= 1u;
                if (t->irq_line == 0u) {
                    raise = 1u;
                    t->oneshot_done = 1u;
                }
            }
        }
        return raise;
    }
    if ((t->mode & MODE_TOGGLE) != 0u) {
        raise = t->irq_line != 0u ? 1u : (ne >= 2u ? 1u : 0u);
        t->irq_line ^= (uint32_t)(ne & 1u);
        return raise;
    }
    return 1u;
}

/* Moves the counter forward by n of its own cycles: its value, hold and
   reached bits, and its line. Returns 1 when the interrupt is raised. */
static uint32_t run_steps(struct schway__timer *t, uint64_t n)
{
    struct hits h = {0u, 0u};
    uint64_t on_target;
    uint64_t on_ffff;
    if (n == 0u) {
        return 0u;
    }
    advance(t, n, &h);
    if (h.target != 0u) {
        t->reached |= REACHED_TARGET;
    }
    if (h.ffff != 0u) {
        t->reached |= REACHED_FFFF;
    }
    on_target = (t->mode & MODE_IRQ_TARGET) != 0u ? h.target : 0u;
    on_ffff = (t->mode & MODE_IRQ_FFFF) != 0u ? h.ffff : 0u;
    /* With the target at FFFFh the two are one landing, and one event. */
    return irq_events(t, t->target == COUNT_MAX ? on_target | on_ffff : on_target + on_ffff);
}

static int raises_within(const struct schway__timer *t, uint64_t n)
{
    struct schway__timer c = *t;
    return run_steps(&c, n) != 0u;
}

/* The fewest counter cycles after the view until an event raises the
   interrupt, or 0 for never within the horizon. The raise is monotonic in the
   cycle count, so a bisection finds it. */
static uint64_t cycles_to_edge(const struct schway__timer *t)
{
    uint64_t lo = 1u;
    uint64_t hi = EDGE_HORIZON;
    if ((t->mode & (MODE_IRQ_TARGET | MODE_IRQ_FFFF)) == 0u || t->paused != 0u ||
        !raises_within(t, hi)) {
        return 0u;
    }
    while (lo < hi) {
        uint64_t mid = lo + (hi - lo) / 2u;
        if (raises_within(t, mid)) {
            hi = mid;
        } else {
            lo = mid + 1u;
        }
    }
    return lo;
}

/* Brings the counters to tick now and raises the I_STAT bit of every counter
   with an edge on the way. The raise is idempotent, so a storm of events in
   the span costs one call. */
void schway__timers_sync(schway *inst, uint64_t now)
{
    struct schway__timers *ts = &inst->timers;
    if (now <= ts->base) {
        return;
    }
    for (uint32_t i = 0; i < SCHWAY__TIMERS; i++) {
        struct schway__timer *t = &ts->t[i];
        if (run_steps(t, source_steps(i, t->mode, ts->base, now)) != 0u) {
            schway__irq_raise(inst, 4u + i);
        }
    }
    ts->base = now;
}

/* The earliest tick, after the view, at which a counter's event would raise a
   new I_STAT edge. A counter whose bit is already set has none: its events
   change nothing the program can see, and acknowledging the bit recomputes the
   deadline. */
uint64_t schway__timers_next(const schway *inst)
{
    uint64_t best = UINT64_MAX;
    for (uint32_t i = 0; i < SCHWAY__TIMERS; i++) {
        const struct schway__timer *t = &inst->timers.t[i];
        uint64_t steps;
        /* A counter that counts the beam's dots or horizontal blanks moves
           when they are pushed, and raises there, so it has no tick to wait
           for. */
        if ((inst->irq.stat & (UINT32_C(1) << (4u + i))) != 0u ||
            source_of(i, t->mode) == SRC_NONE) {
            continue;
        }
        steps = cycles_to_edge(t);
        if (steps != 0u) {
            uint64_t tick = source_tick(i, t->mode, inst->timers.base, steps);
            best = tick < best ? tick : best;
        }
    }
    return best;
}

/* Timer 0's dot-clock deadline is measured in dots; the GPU converts it to a CPU tick. */
uint32_t schway__timers_dot_next(const schway *inst)
{
    const struct schway__timer *t = &inst->timers.t[0];
    if ((inst->irq.stat & (UINT32_C(1) << 4)) != 0u || (t->mode & 0x100u) == 0u) {
        return UINT32_MAX;
    }
    uint64_t steps = cycles_to_edge(t);
    return steps == 0u || steps > UINT32_MAX ? UINT32_MAX : (uint32_t)steps;
}

void schway__timers_event(schway *inst, uint64_t now)
{
    schway__timers_sync(inst, now);
}

static uint32_t mode_image(const struct schway__timer *t)
{
    return (t->mode & MODE_WRITABLE) | (t->irq_line << 10) | ((t->reached & REACHED_TARGET) << 11) |
           ((t->reached & REACHED_FFFF) << 11);
}

/* The registers 1F801100h-112Fh at every width: COUNT, MODE and TARGET of
   each counter, with the word at offset 0Ch reading 0. The access first
   catches the counters up to its own tick. A MODE read returns bits 11 and 12
   and clears them. A COUNT write sets the value and starts the hold. A MODE
   write zeroes the counter, starts the hold, sets bit 10, re-arms the
   one-shot and derives the pause. Every write recomputes the deadline. */
void schway__timers_io(schway *inst, uint32_t wr, uint32_t p, uint32_t width, uint32_t *value,
                       uint32_t *cycles)
{
    uint32_t n = (p >> 4) & 3u;
    uint32_t reg = p & 0xCu;
    struct schway__timer *t;
    uint64_t tick = schway__sched_now(inst, *cycles);
    schway__gpu_sync(inst, tick);
    schway__timers_sync(inst, tick);
    t = &inst->timers.t[n];
    if (wr == 0u) {
        uint32_t r = 0u;
        if (reg == 0u) {
            r = t->value;
        } else if (reg == 4u) {
            r = mode_image(t);
            t->reached = 0u;
        } else if (reg == 8u) {
            r = t->target;
        }
        *value = lane_get(r, p, width);
    } else {
        if (reg == 0u) {
            t->value = lane_put(t->value, p, width, *value) & COUNT_MAX;
            t->hold = HOLD_WRITE;
        } else if (reg == 4u) {
            t->mode = lane_put(mode_image(t), p, width, *value) & MODE_WRITABLE;
            t->value = 0u;
            t->hold = HOLD_WRITE;
            t->irq_line = 1u;
            t->oneshot_done = 0u;
            t->paused = paused_for(n, t->mode, inst->timers.blank);
        } else if (reg == 8u) {
            t->target = lane_put(t->target, p, width, *value) & COUNT_MAX;
        }
        schway__sched_recalc(inst);
    }
}

/* The three counters as seen at tick now: the stored views carried forward by
   a pure function, with no interrupt raised and nothing written back. */
static void view_as_of(const struct schway__timers *src, uint64_t now,
                       struct schway__timer out[SCHWAY__TIMERS])
{
    for (uint32_t i = 0; i < SCHWAY__TIMERS; i++) {
        out[i] = src->t[i];
        (void)run_steps(&out[i], source_steps(i, out[i].mode, src->base, now));
    }
}

/* Each counter's eight fields in a fixed order, bounded to their widths. The
   walk is of the view as of the instance's tick, so the bytes do not depend on
   when a catch-up last ran and saving changes nothing in the instance; a load
   writes the view back and makes its tick the base. */
void schway__timers_state_io(struct schway__state_io *io, schway *inst)
{
    struct schway__timer v[SCHWAY__TIMERS];
    view_as_of(&inst->timers, inst->ticks, v);
    for (uint32_t i = 0; i < SCHWAY__TIMERS; i++) {
        struct schway__timer *t = &v[i];
        (void)schway__state_u32(io, name_value, &t->value, 0u, COUNT_MAX);
        (void)schway__state_u32(io, name_target, &t->target, 0u, COUNT_MAX);
        (void)schway__state_u32(io, name_mode, &t->mode, 0u, MODE_WRITABLE);
        (void)schway__state_u32(io, name_hold, &t->hold, 0u, HOLD_WRITE);
        (void)schway__state_u32(io, name_reached, &t->reached, 0u, REACHED_TARGET | REACHED_FFFF);
        (void)schway__state_u32(io, name_irq_line, &t->irq_line, 0u, 1u);
        (void)schway__state_u32(io, name_oneshot_done, &t->oneshot_done, 0u, 1u);
        (void)schway__state_u32(io, name_paused, &t->paused, 0u, 1u);
    }
    (void)schway__state_u32(io, name_blank, &inst->timers.blank, 0u, BLANK_V | BLANK_H);
    if (io->mode == SCHWAY__STATE_APPLY) {
        memcpy(inst->timers.t, v, sizeof v);
        inst->timers.base = inst->ticks;
    }
}

/* The effect of the blank going to level on a counter that synchronises to
   it (psx-spx, Timers). Mode 0 pauses the counter inside the blank. Mode 1
   resets it to 0 as the blank begins. Mode 2 resets it as the blank begins
   and pauses it outside the blank. Mode 3 holds it until the blank begins
   once and then lets it free-run. A reset counts from the next cycle, with no
   hold. */
static void blank_edge(struct schway__timer *t, uint32_t level)
{
    uint32_t sync_mode = (t->mode >> 1) & 3u;
    if ((t->mode & 1u) == 0u) {
        return;
    }
    if (level != 0u) {
        switch (sync_mode) {
        case 0u:
            t->paused = 1u;
            break;
        case 1u:
            t->value = 0u;
            t->hold = 0u;
            break;
        case 2u:
            t->value = 0u;
            t->hold = 0u;
            t->paused = 0u;
            break;
        default:
            t->paused = 0u;
            break;
        }
    } else if (sync_mode == 0u) {
        t->paused = 0u;
    } else if (sync_mode == 2u) {
        t->paused = 1u;
    }
}

/* Brings the counters to tick, then moves one blanking signal to level.
   Returns 1 when the signal went from 0 to 1. */
static uint32_t blank_push(schway *inst, uint64_t tick, uint32_t n, uint32_t level)
{
    struct schway__timers *ts = &inst->timers;
    uint32_t bit = n == 0u ? BLANK_H : BLANK_V;
    uint32_t lv = level != 0u ? 1u : 0u;
    schway__timers_sync(inst, tick);
    if ((level_of(n, ts->blank)) == lv) {
        return 0u;
    }
    ts->blank = lv != 0u ? (ts->blank | bit) : (ts->blank & ~bit);
    blank_edge(&ts->t[n], lv);
    schway__sched_recalc(inst);
    return lv;
}

/* Moves a counter that counts the beam (counter 0 on the dot clock, counter 1
   on the horizontal blank) by steps of its own clock, and raises its
   interrupt now if one of them makes an edge. */
static void beam_feed(schway *inst, uint32_t n, uint64_t steps)
{
    struct schway__timer *t = &inst->timers.t[n];
    if ((t->mode & 0x100u) == 0u || steps == 0u) {
        return;
    }
    if (run_steps(t, steps) != 0u) {
        schway__irq_raise(inst, 4u + n);
    }
    schway__sched_recalc(inst);
}

/* The vertical blank drives counter 1's synchronisation. */
void schway__timers_vblank(schway *inst, uint64_t tick, uint32_t level)
{
    (void)blank_push(inst, tick, 1u, level);
}

/* The horizontal blank drives counter 0's synchronisation and is counter 1's
   clock when it is the source: each time the blank begins is one step. */
void schway__timers_hblank(schway *inst, uint64_t tick, uint32_t level)
{
    if (blank_push(inst, tick, 0u, level) != 0u) {
        beam_feed(inst, 1u, 1u);
    }
}

/* Dots dot clocks have passed: counter 0 counts them when the dot clock is its
   source. */
void schway__timers_dots(schway *inst, uint64_t tick, uint32_t dots)
{
    schway__timers_sync(inst, tick);
    beam_feed(inst, 0u, dots);
}
