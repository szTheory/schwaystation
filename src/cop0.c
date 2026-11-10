/* COP0: power-on register values and the exception entry. */
#include <string.h>

#include "internal.h"

/* SR has BEV (bit 22) set and the interrupt, mode and cache bits clear; PRID
   reads 2 on the CXD8606CQ CPU; BPCM and BDAM, the breakpoint masks, read
   FFFFFFFFh; every other register, CAUSE and DCIC included, is 0
   (ARCHITECTURE section 7, Power-on; docs/hardware/cop0.md, Registers
   and Entry). */
void schway__cop0_reset(schway *inst)
{
    uint32_t *r = inst->cpu.cop0.r;
    memset(r, 0, sizeof inst->cpu.cop0.r);
    r[SCHWAY__COP0_SR] = 0x00400000u;
    r[SCHWAY__COP0_PRID] = 2u;
    r[SCHWAY__COP0_BPCM] = 0xFFFFFFFFu;
    r[SCHWAY__COP0_BDAM] = 0xFFFFFFFFu;
}

/* CAUSE bits the entry owns: ExcCode (2-6), CE (28-29), BT (30), BD (31). IP
   bits 8-15 are left as they are (docs/hardware/cop0.md, CAUSE). */
#define CAUSE_ENTRY_BITS 0xF000007Cu

/* Pushes the KU/IE stack: SR bits 0-5 are three pairs of (IE, KU), current,
   previous and old, and the entry shifts them left by 2, so the current pair
   becomes 0 (kernel mode, interrupts off) and the old pair is lost
   (docs/hardware/cop0.md, SR). */
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
    /* The entry clears IEc, so nothing is pending: the stamp is dropped. */
    c->irq_stamp = 0u;
    c->block_end |= SCHWAY__BLOCK_EVENT;
    c->pc = (sr & SCHWAY__SR_BEV) != 0u ? SCHWAY__VECTOR_BEV : SCHWAY__VECTOR_RAM;

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

/* The interrupt line the CPU takes: a CAUSE bit in 8-15 (the two software
   bits and the hardware line at bit 10) whose SR mask bit is set, with IEc set
   (docs/hardware/cop0.md, SR and Interrupts). */
uint32_t schway__cop0_irq_pending(const schway *inst)
{
    const uint32_t *r = inst->cpu.cop0.r;
    return (r[SCHWAY__COP0_SR] & 1u) != 0u &&
                   (r[SCHWAY__COP0_CAUSE] & r[SCHWAY__COP0_SR] & 0xFF00u) != 0u
               ? 1u
               : 0u;
}

/* The stamp is set on the false-to-true change of the line and nowhere else,
   so a read of CAUSE or SR never arms an interrupt. A change to false takes the
   interrupt back (docs/hardware/cop0.md, Interrupts). */
void schway__cop0_irq_edge(schway *inst, uint32_t was, uint32_t latency)
{
    if (schway__cop0_irq_pending(inst) == 0u) {
        inst->cpu.irq_stamp = 0u;
    } else if (was == 0u) {
        inst->cpu.irq_stamp = latency;
    }
}

/* The interrupt entry's hook for the GTE. On the console an interrupt taken
   while the next instruction is a GTE command lets that command execute, and it
   executes again after the return, because EPC names it (docs/hardware/cop0.md,
   Interrupts). word is the interrupted instruction.
   The command executes here, before the handler's first instruction, when
   CAUSE's ExcCode is 0, SR.CU2 is set and the word is a command; with CU2 clear
   the instruction would raise CpU instead and never reach the GTE. It keeps the
   engine busy from the entry's cycle. Returns 1 when it executed a command. */
uint32_t schway__cop0_irq_entry_cop2(schway *inst, uint32_t word)
{
    const uint32_t *r = inst->cpu.cop0.r;
    if ((word >> 25) != 0x25u || ((r[SCHWAY__COP0_CAUSE] >> 2) & 31u) != 0u ||
        (r[SCHWAY__COP0_SR] & SCHWAY__SR_CU2) == 0u) {
        return 0u;
    }
    inst->gte.ready = inst->cpu.cycle + schway__gte_exec(&inst->gte, word & 0x01FFFFFFu);
    return 1u;
}

/* SR writes keep the bits the CPU has: the coprocessor-usable bits, BEV, the
   cache and parity bits, the interrupt mask and the three mode and interrupt
   pairs; bits 6-7, 20-21 and 23-27 read 0 (docs/hardware/cop0.md, SR). */
#define SR_WRITABLE 0xF04FFF3Fu

/* CAUSE bits 8-9 are the two software interrupt flags, the only writable
   ones; bit 10 follows the interrupt registers (docs/hardware/cop0.md,
   Registers). */
#define CAUSE_SOFTWARE 0x00000300u

/* Registers 0-2, 4 and 10 are not there and raise a reserved instruction
   exception; 16-31 answer garbage without one, here 0, and ignore writes
   (docs/hardware/cop0.md, Registers). */
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
    uint32_t was = schway__cop0_irq_pending(inst);
    if (cop0_missing(rd) != 0u) {
        return 0u;
    }
    /* A COP0 write ends the block: it may change what the next instruction
       sees, SR and CAUSE above all. */
    inst->cpu.block_end |= SCHWAY__BLOCK_EVENT;
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
        schway__cop0_irq_edge(inst, was, SCHWAY__IRQ_LATENCY_EDGE);
        break;
    case SCHWAY__COP0_CAUSE:
        r[rd] = (r[rd] & ~CAUSE_SOFTWARE) | (value & CAUSE_SOFTWARE);
        schway__cop0_irq_edge(inst, was, SCHWAY__IRQ_LATENCY_NEXT);
        break;
    default:
        break;
    }
    return 1u;
}

/* RFE shifts the previous and old pairs down, SR bits 2-5 into 0-3, and
   leaves the old pair as it was; it does not jump (docs/hardware/cop0.md,
   RFE). */
void schway__cop0_rfe(schway *inst)
{
    uint32_t *r = inst->cpu.cop0.r;
    uint32_t was = schway__cop0_irq_pending(inst);
    inst->cpu.block_end |= SCHWAY__BLOCK_EVENT;
    r[SCHWAY__COP0_SR] = (r[SCHWAY__COP0_SR] & ~0xFu) | ((r[SCHWAY__COP0_SR] >> 2) & 0xFu);
    /* The restored IEc is looked at at once: a line that was held off by the
       handler's mode is taken after this instruction (docs/hardware/cop0.md, Interrupts). */
    schway__cop0_irq_edge(inst, was, SCHWAY__IRQ_LATENCY_NEXT);
}
