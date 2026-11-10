/* The root counters one cycle at a time, from the psx-spx Timers page and
   nothing else: a counter holds its value for the cycles after a write, counts
   one a cycle, resets at the target or wraps, and every landing on the target
   or on FFFFh is an event for the interrupt mode. It shares no code with
   src/timers.c, which computes the same thing in closed form. */
#include <string.h>

#include "ref.h"

struct counter {
    uint32_t value, target, mode, hold, reached, line, done, paused;
};

static struct counter c[3];
static uint32_t now, istat, vblank, hblank;

void ref_reset(void)
{
    memset(c, 0, sizeof c);
    now = istat = vblank = hblank = 0u;
}

/* One event: pulse mode raises, toggle mode flips the line and raises on the
   flip to 0; a one-shot counter does so once until MODE is written. */
static void event(uint32_t n)
{
    struct counter *r = &c[n];
    if ((r->mode & 0x40u) == 0u && r->done != 0u) {
        return;
    }
    if ((r->mode & 0x80u) == 0u) {
        istat |= 0x10u << n;
        r->done = 1u;
    } else {
        r->line ^= 1u;
        if (r->line == 0u) {
            istat |= 0x10u << n;
            r->done = 1u;
        }
    }
}

/* One cycle of the counter's own clock. */
static void step(uint32_t n)
{
    struct counter *r = &c[n];
    uint32_t hit = 0u;
    if (r->hold != 0u) {
        r->hold--;
        return;
    }
    if (r->paused != 0u) {
        return;
    }
    if ((r->mode & 0x08u) != 0u && r->value == r->target) {
        r->value = 0u;
        r->hold = 1u;
    } else {
        r->value = (r->value + 1u) & 0xFFFFu;
    }
    if (r->value == r->target) {
        r->reached |= 1u;
        hit |= r->mode & 0x10u;
    }
    if (r->value == 0xFFFFu) {
        r->reached |= 2u;
        hit |= r->mode & 0x20u;
    }
    if (hit != 0u) {
        event(n);
    }
}

void ref_run(uint32_t cycles)
{
    for (; cycles != 0u; cycles--) {
        now++;
        for (uint32_t n = 0; n < 2u; n++) {
            if ((c[n].mode & 0x100u) == 0u) {
                step(n);
            }
        }
        if ((c[2].mode & 0x200u) == 0u || (now & 7u) == 0u) {
            step(2u);
        }
    }
}

/* Paused right after a MODE write, from the sync mode and the blank level. */
static uint32_t paused_at_write(uint32_t n)
{
    uint32_t sync = (c[n].mode >> 1) & 3u;
    uint32_t level = n == 0u ? hblank : vblank;
    if ((c[n].mode & 1u) == 0u) {
        return 0u;
    }
    if (n == 2u) {
        return sync == 0u || sync == 3u;
    }
    return sync == 0u ? level : sync == 1u ? 0u : sync == 2u ? level ^ 1u : 1u;
}

void ref_write(uint32_t n, uint32_t reg, uint32_t value)
{
    struct counter *r = &c[n];
    if (reg == 0u) {
        r->value = value & 0xFFFFu;
        r->hold = 2u;
    } else if (reg == 4u) {
        r->mode = value & 0x3FFu;
        r->value = 0u;
        r->hold = 2u;
        r->line = 1u;
        r->done = 0u;
        r->paused = paused_at_write(n);
    } else {
        r->target = value & 0xFFFFu;
    }
}

uint32_t ref_read(uint32_t n, uint32_t reg)
{
    struct counter *r = &c[n];
    uint32_t v;
    if (reg == 0u) {
        return r->value;
    }
    if (reg == 8u) {
        return r->target;
    }
    v = r->mode | (r->line << 10) | ((r->reached & 1u) << 11) | ((r->reached & 2u) << 11);
    r->reached = 0u;
    return v;
}

uint32_t ref_istat(void)
{
    return istat;
}

void ref_ack(uint32_t mask)
{
    istat &= ~mask;
}

/* A blank goes to lv. A counter that synchronises to it pauses, resets or is
   released as its sync mode says. Returns 1 when the blank began. */
static uint32_t blank(uint32_t n, uint32_t lv)
{
    uint32_t *level = n == 0u ? &hblank : &vblank;
    struct counter *r = &c[n];
    uint32_t sync = (r->mode >> 1) & 3u;
    if (*level == lv) {
        return 0u;
    }
    *level = lv;
    if ((r->mode & 1u) != 0u && lv != 0u) {
        if (sync == 0u) {
            r->paused = 1u;
        } else if (sync == 1u || sync == 2u) {
            r->value = 0u;
            r->hold = 0u;
            r->paused = 0u;
        } else {
            r->paused = 0u;
        }
    } else if ((r->mode & 1u) != 0u) {
        r->paused = sync == 0u ? 0u : sync == 2u ? 1u : r->paused;
    }
    return lv;
}

void ref_vblank(uint32_t level)
{
    (void)blank(1u, level);
}

void ref_hblank(uint32_t level)
{
    if (blank(0u, level) != 0u && (c[1].mode & 0x100u) != 0u) {
        step(1u);
    }
}

void ref_dots(uint32_t dots)
{
    for (; dots != 0u && (c[0].mode & 0x100u) != 0u; dots--) {
        step(0u);
    }
}
