/* The interrupt registers I_STAT and I_MASK (1F801070h, 1F801074h). */
#include "internal.h"

void schway__irq_reset(schway *inst)
{
    inst->irq.stat = 0u;
    inst->irq.mask = 0u;
}

/* Interrupt sources 0-10 each latch one I_STAT bit (PSX-SYSTEM section 6). */
void schway__irq_raise(schway *inst, uint32_t bit)
{
    if (bit < 11u) {
        inst->irq.stat |= UINT32_C(1) << bit;
    }
}

/* The level the CPU sees on CAUSE bit 10: any latched source the mask lets
   through (PSX-SYSTEM section 6). */
uint32_t schway__irq_line(const schway *inst)
{
    return (inst->irq.stat & inst->irq.mask & SCHWAY__IRQ_BITS) != 0u ? 1u : 0u;
}
