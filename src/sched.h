/* The scheduler spine: the cached deadline and the module's declarations. Not
   installed. The instance embeds the struct, so sched.c is edited alone. */
#ifndef SCHWAYSTATION_SCHED_H
#define SCHWAYSTATION_SCHED_H

#include <stdint.h>

#include "schwaystation.h"

/* next is the earliest tick a device needs attention at, UINT64_MAX for none.
   It is derived from the devices' own state, never saved, and rebuilt after a
   register write or a state load. */
struct schway__sched {
    uint64_t next;
};

/* The tick of the current access: the instance's clock plus the cycles the
   access has taken so far. */
uint64_t schway__sched_now(const schway *inst, uint32_t cycles);

/* Brings the devices that count lazily up to date as of the current tick. */
void schway__sched_sync(schway *inst);

/* Recomputes next from every device. */
void schway__sched_recalc(schway *inst);

/* Runs the events that are due, then recomputes next. */
void schway__sched_run(schway *inst);

#endif /* SCHWAYSTATION_SCHED_H */
