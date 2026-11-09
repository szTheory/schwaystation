/* COP0: power-on register values and the exception entry. */
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

/* The general exception vector: BFC00180h while SR.BEV (bit 22) is set and
   80000080h otherwise (PSX-SYSTEM section 2, Vectors). */
#define VECTOR_BEV 0xBFC00180u
#define VECTOR_RAM 0x80000080u

/* CAUSE bits the entry owns: ExcCode (2-6), CE (28-29), BT (30), BD (31). IP
   bits 8-15 are left as they are (PSX-SYSTEM section 2, CAUSE bits). */
#define CAUSE_ENTRY_BITS 0xF000007Cu

/* Pushes the KU/IE stack: SR bits 0-5 are three pairs of (IE, KU), current,
   previous and old, and the entry shifts them left by 2, so the current pair
   becomes 0 (kernel mode, interrupts off) and the old pair is lost
   (PSX-SYSTEM section 2, SR bits). */
void schway__cop0_exception(schway *inst, uint32_t code, uint32_t epc, uint32_t in_delay,
                            uint32_t taken, uint32_t target, uint32_t badvaddr,
                            uint32_t has_badvaddr, uint32_t ce)
{
    struct schway__cpu *c = &inst->cpu;
    uint32_t *r = c->cop0.r;
    uint32_t sr = r[SCHWAY__COP0_SR];

    r[SCHWAY__COP0_SR] = (sr & ~0x3Fu) | ((sr << 2) & 0x3Fu);
    r[SCHWAY__COP0_CAUSE] = (r[SCHWAY__COP0_CAUSE] & ~CAUSE_ENTRY_BITS) | ((code & 31u) << 2) |
                            ((ce & 3u) << 28) | ((taken & 1u) << 30) | ((in_delay & 1u) << 31);
    r[SCHWAY__COP0_EPC] = epc;
    if (in_delay != 0u) {
        r[SCHWAY__COP0_TAR] = target;
    }
    if (has_badvaddr != 0u) {
        r[SCHWAY__COP0_BADVADDR] = badvaddr;
    }
    c->branch_slot = 0u;
    c->branch_take = 0u;
    c->branch_target = 0u;
    c->pc = (sr & 0x00400000u) != 0u ? VECTOR_BEV : VECTOR_RAM;
}
