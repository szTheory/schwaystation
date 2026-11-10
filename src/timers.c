/* The root counters: power-on values, the state walk and the register window.
   The counting itself is not built yet: the registers still count as an
   unbuilt device. */
#include <string.h>

#include "state.h"

static const char name_value[] = "timers.value";
static const char name_target[] = "timers.target";
static const char name_mode[] = "timers.mode";
static const char name_hold[] = "timers.hold";
static const char name_reached[] = "timers.reached";
static const char name_irq_line[] = "timers.irq_line";
static const char name_oneshot_done[] = "timers.oneshot_done";
static const char name_paused[] = "timers.paused";
static const char name_free_after[] = "timers.free_after";

void schway__timers_reset(schway *inst)
{
    memset(&inst->timers, 0, sizeof inst->timers);
}

/* Each counter's nine fields in a fixed order, bounded to their widths. The
   base tick is not walked; after APPLY it is the instance's tick, which the
   walk has already applied. */
void schway__timers_state_io(struct schway__state_io *io, schway *inst)
{
    for (uint32_t i = 0; i < SCHWAY__TIMERS; i++) {
        struct schway__timer *t = &inst->timers.t[i];
        (void)schway__state_u32(io, name_value, &t->value, 0u, 0xFFFFu);
        (void)schway__state_u32(io, name_target, &t->target, 0u, 0xFFFFu);
        (void)schway__state_u32(io, name_mode, &t->mode, 0u, 0x1FFFu);
        (void)schway__state_u32(io, name_hold, &t->hold, 0u, 2u);
        (void)schway__state_u32(io, name_reached, &t->reached, 0u, 1u);
        (void)schway__state_u32(io, name_irq_line, &t->irq_line, 0u, 1u);
        (void)schway__state_u32(io, name_oneshot_done, &t->oneshot_done, 0u, 1u);
        (void)schway__state_u32(io, name_paused, &t->paused, 0u, 1u);
        (void)schway__state_u32(io, name_free_after, &t->free_after, 0u, 1u);
    }
    if (io->mode == SCHWAY__STATE_APPLY) {
        inst->timers.base = inst->ticks;
    }
}

/* The registers read 0 and drop writes, counted as an unbuilt device. */
void schway__timers_io(schway *inst, uint32_t wr, uint32_t p, uint32_t width, uint32_t *value,
                       uint32_t *cycles)
{
    (void)cycles;
    schway__bus_unbuilt(inst, SCHWAY__DEV_TIMER, p, width, wr, wr != 0u ? *value : 0u);
    if (wr == 0u) {
        *value = 0u;
    }
}

uint64_t schway__timers_next(const schway *inst)
{
    (void)inst;
    return UINT64_MAX;
}

void schway__timers_event(schway *inst, uint64_t now)
{
    (void)inst;
    (void)now;
}

void schway__timers_sync(schway *inst, uint64_t now)
{
    (void)inst;
    (void)now;
}

void schway__timers_vblank(schway *inst, uint64_t tick, uint32_t level)
{
    (void)inst;
    (void)tick;
    (void)level;
}

void schway__timers_hblank(schway *inst, uint64_t tick, uint32_t level)
{
    (void)inst;
    (void)tick;
    (void)level;
}

void schway__timers_dots(schway *inst, uint64_t tick, uint32_t dots)
{
    (void)inst;
    (void)tick;
    (void)dots;
}
