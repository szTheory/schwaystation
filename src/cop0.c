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
    c->pc = (sr & SCHWAY__SR_BEV) != 0u ? VECTOR_BEV : VECTOR_RAM;

    /* The same EPC and ExcCode taken SCHWAY__EXC_LOOP_LIMIT times in a row
       is a loop the guest cannot leave. An interrupt (ExcCode 0) is neither
       counted nor does it break a run, and a different EPC or code starts a
       new one. SYSCALL, BREAK and overflow count too: only a loop is a fault
       (ARCHITECTURE section 6). */
    if (code != SCHWAY__EXC_INT) {
        if (c->exc_repeat != 0u && c->exc_epc == epc && c->exc_code == (code & 31u)) {
            if (c->exc_repeat != UINT32_MAX) {
                c->exc_repeat++;
            }
        } else {
            c->exc_epc = epc;
            c->exc_code = code & 31u;
            c->exc_repeat = 1u;
        }
        if (c->exc_repeat >= SCHWAY__EXC_LOOP_LIMIT && inst->fault.kind == SCHWAY__FAULT_NONE) {
            schway__fault_latch(inst, SCHWAY__FAULT_EXCEPTION_LOOP, SCHWAY__DEV_NONE, 0u, epc, 0u,
                                0u);
            inst->fault.note = c->exc_repeat;
        }
    }
}

/* SR writes keep the bits the CPU has: the coprocessor-usable bits, BEV, the
   cache and parity bits, the interrupt mask and the three mode and interrupt
   pairs; bits 6-7, 20-21 and 23-27 read 0 (PSX-SYSTEM section 2, SR bits). */
#define SR_WRITABLE 0xF04FFF3Fu

/* CAUSE bits 8-9 are the two software interrupt flags, the only writable
   ones; bit 10 follows the interrupt registers (PSX-SYSTEM section 2, COP0
   registers). */
#define CAUSE_SOFTWARE 0x00000300u

/* Registers 0-2, 4 and 10 are not there and raise a reserved instruction
   exception; 16-31 answer garbage without one, here 0, and ignore writes
   (PSX-SYSTEM section 2, COP0 registers). */
static uint32_t cop0_missing(uint32_t rd)
{
    return rd <= 2u || rd == 4u || rd == 10u ? 1u : 0u;
}

uint32_t schway__cop0_mfc0(schway *inst, uint32_t rd, uint32_t *value)
{
    if (cop0_missing(rd) != 0u) {
        return 0u;
    }
    *value = rd < 16u ? inst->cpu.cop0.r[rd] : 0u;
    return 1u;
}

/* TAR, BadVaddr, EPC and PRID are read-only; the breakpoint registers keep
   what is written, as the breakpoints themselves are not modelled. */
uint32_t schway__cop0_mtc0(schway *inst, uint32_t rd, uint32_t value)
{
    uint32_t *r = inst->cpu.cop0.r;
    if (cop0_missing(rd) != 0u) {
        return 0u;
    }
    switch (rd) {
    case SCHWAY__COP0_BPC:
    case SCHWAY__COP0_BDA:
    case SCHWAY__COP0_DCIC:
    case SCHWAY__COP0_BDAM:
    case SCHWAY__COP0_BPCM:
        r[rd] = value;
        break;
    case SCHWAY__COP0_SR:
        r[rd] = value & SR_WRITABLE;
        break;
    case SCHWAY__COP0_CAUSE:
        r[rd] = (r[rd] & ~CAUSE_SOFTWARE) | (value & CAUSE_SOFTWARE);
        break;
    default:
        break;
    }
    return 1u;
}

/* RFE shifts the previous and old pairs down, SR bits 2-5 into 0-3, and
   leaves the old pair as it was; it does not jump (PSX-SYSTEM section 2, SR
   bits and EPC). */
void schway__cop0_rfe(schway *inst)
{
    uint32_t *r = inst->cpu.cop0.r;
    r[SCHWAY__COP0_SR] = (r[SCHWAY__COP0_SR] & ~0xFu) | ((r[SCHWAY__COP0_SR] >> 2) & 0xFu);
}
