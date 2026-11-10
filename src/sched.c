/* The scheduler spine: the earliest deadline over the devices that keep time. */
#include "internal.h"

/* The block clock: the tick the running block started on, plus the cycles the
   CPU has spent since, plus the cycles this access has spent before its own
   (the first cycle of an instruction is the one it starts on). cycles 0 counts
   as 1. */
uint64_t schway__sched_now(const schway *inst, uint32_t cycles)
{
    const struct schway__cpu *c = &inst->cpu;
    return c->blk_tick + (c->cycle - c->blk_cycle) + (uint64_t)(cycles != 0u ? cycles - 1u : 0u);
}

/* Restarts the block clock at the instance's tick, for a change of the tick or
   the cycle count outside a block: a power reset, a state load, a run with no
   program. */
void schway__sched_sync(schway *inst)
{
    schway__gpu_sync(inst, inst->ticks);
    inst->cpu.blk_tick = inst->ticks;
    inst->cpu.blk_cycle = inst->cpu.cycle;
    schway__sched_recalc(inst);
}

/* The earliest deadline over the devices. It ends the running block at the
   next instruction boundary, so the new deadline is seen without waiting for a
   whole block to run out. */
void schway__sched_recalc(schway *inst)
{
    uint64_t dma = schway__dma_next(inst);
    uint64_t timers = schway__timers_next(inst);
    uint64_t gpu = schway__gpu_next(inst);
    inst->sched.next = dma < timers ? dma : timers;
    if (gpu < inst->sched.next) {
        inst->sched.next = gpu;
    }
    inst->cpu.block_end |= SCHWAY__BLOCK_EVENT;
}

/* Runs each device whose deadline is due, once, then recomputes the deadline. A
   device that is still due after its event is not run again in this call, so
   the runner always returns. */
void schway__sched_run(schway *inst)
{
    uint64_t now = inst->ticks;
    if (schway__timers_next(inst) <= now) {
        schway__timers_event(inst, now);
    }
    if (schway__gpu_next(inst) <= now) {
        schway__gpu_event(inst, now);
    }
    if (schway__dma_next(inst) <= now) {
        schway__dma_event(inst, now);
    }
    schway__sched_recalc(inst);
}
