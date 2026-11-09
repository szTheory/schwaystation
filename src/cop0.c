/* COP0: power-on register values. */
#include <string.h>

#include "internal.h"

/* SR has BEV (bit 22) set and the interrupt, mode and cache bits clear; PRID
   reads 2 on the CXD8606CQ CPU; BPCM and BDAM, the breakpoint masks, read
   FFFFFFFFh; every other register, CAUSE and DCIC included, is 0
   (ARCHITECTURE section 7, Power-on; PSX-SYSTEM section 2, COP0 registers
   and Reset state). */
void schway__cop0_reset(schway *inst)
{
    uint32_t *r = inst->cpu.cop0.r;
    memset(r, 0, sizeof inst->cpu.cop0.r);
    r[SCHWAY__COP0_SR] = 0x00400000u;
    r[SCHWAY__COP0_PRID] = 2u;
    r[SCHWAY__COP0_BPCM] = 0xFFFFFFFFu;
    r[SCHWAY__COP0_BDAM] = 0xFFFFFFFFu;
}
