/* The scheduler spine: the earliest deadline over the devices that keep time. */
#include "internal.h"

uint64_t schway__sched_now(const schway *inst, uint32_t cycles)
{
    (void)cycles;
    return inst->ticks;
}

void schway__sched_sync(schway *inst)
{
    (void)inst;
}

void schway__sched_recalc(schway *inst)
{
    uint64_t dma = schway__dma_next(inst);
    uint64_t timers = schway__timers_next(inst);
    inst->sched.next = dma < timers ? dma : timers;
}

void schway__sched_run(schway *inst)
{
    schway__sched_recalc(inst);
}
