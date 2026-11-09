/* The CPU and its system-control coprocessor: register state and the module's
   declarations. Not installed. The state structs live here, and the instance
   embeds them, so each module's source file is edited alone. */
#ifndef SCHWAYSTATION_CPU_H
#define SCHWAYSTATION_CPU_H

#include <stdint.h>

#include "schwaystation.h"

struct schway__state_io;

/* The reset vector: the BIOS ROM entry in KSEG1 (PSX-SYSTEM section 2,
   Reset state). */
#define SCHWAY__CPU_RESET_PC 0xBFC00000u

/* COP0 register numbers, with the documented names (PSX-SYSTEM section 2,
   COP0 registers). Numbers not listed read as garbage or raise a reserved
   instruction exception. */
enum {
    SCHWAY__COP0_BPC = 3,
    SCHWAY__COP0_BDA = 5,
    SCHWAY__COP0_TAR = 6,
    SCHWAY__COP0_DCIC = 7,
    SCHWAY__COP0_BADVADDR = 8,
    SCHWAY__COP0_BDAM = 9,
    SCHWAY__COP0_BPCM = 11,
    SCHWAY__COP0_SR = 12,
    SCHWAY__COP0_CAUSE = 13,
    SCHWAY__COP0_EPC = 14,
    SCHWAY__COP0_PRID = 15
};

/* Exception codes of CAUSE bits 2-6 (PSX-SYSTEM section 2, ExcCodes). */
enum {
    SCHWAY__EXC_INT = 0x00,
    SCHWAY__EXC_ADEL = 0x04,
    SCHWAY__EXC_ADES = 0x05,
    SCHWAY__EXC_IBE = 0x06,
    SCHWAY__EXC_DBE = 0x07,
    SCHWAY__EXC_SYS = 0x08,
    SCHWAY__EXC_BP = 0x09,
    SCHWAY__EXC_RI = 0x0A,
    SCHWAY__EXC_CPU = 0x0B,
    SCHWAY__EXC_OVF = 0x0C
};

/* SR bits the executor reads (PSX-SYSTEM section 2, SR bits): KUc, the
   current mode (1 is user), Isc, which isolates the cache, BEV and the
   coprocessor usable bits CU0 to CU3. */
#define SCHWAY__SR_KUC 0x00000002u
#define SCHWAY__SR_ISC 0x00010000u
#define SCHWAY__SR_BEV 0x00400000u
#define SCHWAY__SR_CU0 0x10000000u
#define SCHWAY__SR_CU1 0x20000000u
#define SCHWAY__SR_CU2 0x40000000u
#define SCHWAY__SR_CU3 0x80000000u

/* The same ExcCode taken this many times in a row at one EPC is an exception
   loop, a first fault (ARCHITECTURE section 6). */
#define SCHWAY__EXC_LOOP_LIMIT 1000u

/* COP0 registers, indexed by register number. */
struct schway__cop0 {
    uint32_t r[32];
};

/* load_reg while no load is pending. A load into register 0 is pending too:
   it lands and is discarded. */
#define SCHWAY__CPU_NO_LOAD 32u

/* One bus access of the replay log: kind is a SCHWAY__CYCLE_* value, size is
   1, 2 or 4 bytes, addr is the virtual address the instruction used and value
   is the data read or written. */
enum { SCHWAY__CYCLE_READ = 1, SCHWAY__CYCLE_WRITE = 2, SCHWAY__CYCLE_FETCH = 4 };

struct schway__cpu_cycle {
    uint32_t kind, size, addr, value;
};

/* The replay bus: while a log is attached, a read or fetch returns the value
   of expect[k] and every access is recorded in got, in order, k counting
   from the start of the instruction. An access past the end of got is counted
   in overflow and not stored. */
struct schway__cpu_log {
    const struct schway__cpu_cycle *expect;
    uint32_t expect_n;
    struct schway__cpu_cycle got[8];
    uint32_t got_n;
    uint32_t overflow;
};

/* The longest a multiply or divide takes: DIV and DIVU (PSX-SYSTEM section
   2, MULTU/MULT and DIV/DIVU). A read of HI or LO never waits longer. */
#define SCHWAY__MULDIV_MAX 36u

/* The instruction cache: 256 direct-mapped lines of 4 words, 4 KiB, indexed by
   address bits 11:4. A line holds the tag, physical address bits 31:12, and a
   valid bit per word (PSX-SYSTEM section 2, I-cache). */
#define SCHWAY__ICACHE_LINES 256u

/* The write queue holds 4 stores; a fifth waits for one to leave. Each queued
   store is charged this many cycles when it leaves, an estimate: no published
   figure gives the drain rate (PSX-SYSTEM section 2, Cache isolation, write
   queue). */
#define SCHWAY__WQ_DEPTH 4u
#define SCHWAY__WQ_DRAIN_CYCLES 4u

/* The CPU: 32 general registers (r[0] is the zero register), HI and LO, the
   program counter, COP0 and the two delay latches. The next program counter
   is not stored: it is branch_target while a taken branch is pending and
   pc + 4 otherwise. load_reg and load_value hold the load issued by the
   previous instruction, which lands when the current one ends; branch_slot
   says the instruction at pc is a branch delay slot, branch_take that its
   branch is taken and branch_target where it goes (PSX-SYSTEM section 2,
   Load delay and Branch delay). cycle counts the cycles the CPU has spent
   on instructions, and muldiv_ready is the value of cycle at which the
   running multiply or divide has produced HI and LO, 0 when none is running.
   The 64-bit fields come first so the layout does not depend on the
   target's alignment of them. icache_tag, icache_word and icache_valid are
   the instruction cache and wq_count the stores still in the write queue; the
   queue is a count, because every store reaches the bus at once and only the
   cost of waiting is modelled. The log pointer is not state. */
struct schway__cpu {
    uint64_t muldiv_ready;
    uint64_t cycle;
    uint32_t r[32];
    uint32_t hi, lo;
    uint32_t pc;
    struct schway__cop0 cop0;
    uint32_t load_reg, load_value;
    uint32_t branch_slot, branch_take, branch_target;
    uint32_t exc_epc, exc_code, exc_repeat;
    uint32_t wq_count;
    uint32_t icache_tag[SCHWAY__ICACHE_LINES];
    uint32_t icache_word[SCHWAY__ICACHE_LINES][4];
    uint8_t icache_valid[SCHWAY__ICACHE_LINES];
    struct schway__cpu_log *log;
};

/* Power-on values (ARCHITECTURE section 7). */
void schway__cpu_reset(schway *inst);
void schway__cop0_reset(schway *inst);

/* The one exception entry (PSX-SYSTEM section 2, EPC and the SR, CAUSE and
   Vectors rows). code is the ExcCode, epc the address to return to (the
   branch itself for a delay-slot instruction), in_delay the BD bit and taken
   the BT bit. target is written to TAR when in_delay is set; badvaddr is
   written to BadVaddr when has_badvaddr is set; ce goes to CAUSE bits 28-29.
   The KU/IE stack is pushed, pending delay-slot state is discarded and the
   program counter moves to the vector. */
void schway__cop0_exception(schway *inst, uint32_t code, uint32_t epc, uint32_t in_delay,
                            uint32_t taken, uint32_t target, uint32_t badvaddr,
                            uint32_t has_badvaddr, uint32_t ce);

/* MFC0 and MTC0 on COP0 register rd. Each returns 1, or 0 when rd is one of
   the numbers that raise a reserved instruction exception (0-2, 4 and 10).
   MFC0 stores the value read; the caller issues it through the load delay.
   RFE pops the KU/IE stack (PSX-SYSTEM section 2, COP0 registers). */
uint32_t schway__cop0_mfc0(schway *inst, uint32_t rd, uint32_t *value);
uint32_t schway__cop0_mtc0(schway *inst, uint32_t rd, uint32_t value);
void schway__cop0_rfe(schway *inst);

/* An instruction fetch of the word at virtual address vaddr, which lies in
   KUSEG or KSEG0, through the instruction cache. A hit costs nothing beyond the
   instruction's own cycle. A miss, or a tag hit on a word that is not valid,
   fills the line from that word to its end and adds the fill's cost to
   *cycles. Returns 0 with the word in *word, or non-zero for a bus error on
   the missed word, with the line untouched (PSX-SYSTEM section 2, I-cache). */
uint32_t schway__cpu_icache_fetch(schway *inst, uint32_t vaddr, uint32_t *word, uint32_t *cycles);

/* The cpu and cop0 fields of the state walk (state.h). */
void schway__cpu_state_io(struct schway__state_io *io, schway *inst);

/* Runs one instruction through the executor the run loop uses. With a log,
   fetches and reads take their values from log->expect and every access is
   recorded in log->got; the registers and latches are the caller's to load
   and to compare. */
void schway__cpu_step_one(schway *inst, struct schway__cpu_log *log);

#endif /* SCHWAYSTATION_CPU_H */
