/* The three root counters: state structs and the module's declarations. Not
   installed. The instance embeds the structs, so timers.c is edited alone. */
#ifndef SCHWAYSTATION_TIMERS_H
#define SCHWAYSTATION_TIMERS_H

#include <stdint.h>

#include "schwaystation.h"

struct schway__state_io;

/* Counters 0 to 2 (psx-spx, Timers). */
#define SCHWAY__TIMERS 3u

/* One counter as of a tick. value and target are 16 bits; mode holds the 10
   bits a program writes; hold counts the cycles a written value stays on show
   (0 to 2); reached has the target flag in bit 0 and the FFFFh flag in bit 1;
   irq_line is MODE bit 10 (1 is idle); the rest are flags. The saved state
   holds the view as of the instance's tick, so it does not depend on when a
   catch-up last ran. */
struct schway__timer {
    uint32_t value, target, mode, hold;
    uint32_t reached, irq_line, oneshot_done, paused;
};

/* base is the tick the stored views are as of. It is not state: a load sets it
   to the loaded tick. blank holds the levels of the two blanking signals that
   the beam pushes: bit 0 is the vertical blank and bit 1 the horizontal blank
   (1 is inside the blank); a MODE write derives a counter's pause from them. */
struct schway__timers {
    struct schway__timer t[SCHWAY__TIMERS];
    uint64_t base;
    uint32_t blank;
};

/* Power-on: every counter is 0. */
void schway__timers_reset(schway *inst);

/* The timers fields of the state walk (state.h). */
void schway__timers_state_io(struct schway__state_io *io, schway *inst);

/* One access to the counter registers at physical address p, of width 1, 2 or
   4 bytes: wr is 1 for a write, with the data in *value; a read leaves its
   result in *value. *cycles is the access's cost so far. */
void schway__timers_io(schway *inst, uint32_t wr, uint32_t p, uint32_t width, uint32_t *value,
                       uint32_t *cycles);

/* The tick of the next event that would raise a new I_STAT edge, or UINT64_MAX
   for never. */
uint64_t schway__timers_next(const schway *inst);

/* Timer 0 dot-source steps until its next interrupt edge, or UINT32_MAX for none. */
uint32_t schway__timers_dot_next(const schway *inst);

/* Runs what is due at tick now. */
void schway__timers_event(schway *inst, uint64_t now);

/* Brings the counters up to date as of tick now. */
void schway__timers_sync(schway *inst, uint64_t now);

/* The GPU's blanking signals, at the tick they change: vertical and
   horizontal blank go to level, and dots dot clocks have passed. */
void schway__timers_vblank(schway *inst, uint64_t tick, uint32_t level);
void schway__timers_hblank(schway *inst, uint64_t tick, uint32_t level);
void schway__timers_dots(schway *inst, uint64_t tick, uint32_t dots);

#endif /* SCHWAYSTATION_TIMERS_H */
