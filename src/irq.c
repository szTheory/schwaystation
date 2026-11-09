/* The interrupt registers I_STAT and I_MASK (1F801070h, 1F801074h). */
#include "internal.h"

void schway__irq_reset(schway *inst)
{
    inst->irq.stat = 0u;
    inst->irq.mask = 0u;
}

/* CAUSE bit 10 is the live level: the CPU sees it as the line, at the write or
   raise that changes it and with no delay (PSX-SYSTEM section 6).
   CAUSE bits 8-9 are software latches and stay as they are. */
#define CAUSE_IP2 (UINT32_C(1) << 10)

static void irq_sync(schway *inst)
{
    uint32_t *cause = &inst->cpu.cop0.r[SCHWAY__COP0_CAUSE];
    *cause = (*cause & ~CAUSE_IP2) | (schway__irq_line(inst) != 0u ? CAUSE_IP2 : 0u);
}

/* Interrupt sources 0-10 each latch one I_STAT bit, on the 0 to 1 edge of
   the source (PSX-SYSTEM section 6). */
void schway__irq_raise(schway *inst, uint32_t bit)
{
    if (bit < 11u) {
        inst->irq.stat |= UINT32_C(1) << bit;
        irq_sync(inst);
    }
}

/* A write to I_STAT or I_MASK ends the block, so a line it raises is seen
   before another instruction runs. */
void schway__irq_write_stat(schway *inst, uint32_t lane, uint32_t mask)
{
    inst->cpu.block_end |= SCHWAY__BLOCK_EVENT;
    inst->irq.stat &= (lane | ~mask) & SCHWAY__IRQ_BITS;
    irq_sync(inst);
}

void schway__irq_write_mask(schway *inst, uint32_t lane, uint32_t mask)
{
    inst->cpu.block_end |= SCHWAY__BLOCK_EVENT;
    inst->irq.mask = ((inst->irq.mask & ~mask) | (lane & mask)) & SCHWAY__IRQ_BITS;
    irq_sync(inst);
}

/* The level the CPU sees on CAUSE bit 10: any latched source the mask lets
   through (PSX-SYSTEM section 6). */
uint32_t schway__irq_line(const schway *inst)
{
    return (inst->irq.stat & inst->irq.mask & SCHWAY__IRQ_BITS) != 0u ? 1u : 0u;
}
