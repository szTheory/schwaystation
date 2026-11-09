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

/* The CPU: 32 general registers (r[0] is the zero register), HI and LO, the
   program counter, COP0 and the two delay latches. The next program counter
   is not stored: it is branch_target while a taken branch is pending and
   pc + 4 otherwise. load_reg and load_value hold the load issued by the
   previous instruction, which lands when the current one ends; branch_slot
   says the instruction at pc is a branch delay slot, branch_take that its
   branch is taken and branch_target where it goes (PSX-SYSTEM section 2,
   Load delay and Branch delay). The log pointer is not state. */
struct schway__cpu {
    uint32_t r[32];
    uint32_t hi, lo;
    uint32_t pc;
    struct schway__cop0 cop0;
    uint32_t load_reg, load_value;
    uint32_t branch_slot, branch_take, branch_target;
    struct schway__cpu_log *log;
};

/* Power-on values (ARCHITECTURE section 7). */
void schway__cpu_reset(schway *inst);
void schway__cop0_reset(schway *inst);

/* The cpu and cop0 fields of the state walk (state.h). */
void schway__cpu_state_io(struct schway__state_io *io, schway *inst);

/* Runs one instruction through the executor the run loop uses. With a log,
   fetches and reads take their values from log->expect and every access is
   recorded in log->got; the registers and latches are the caller's to load
   and to compare. */
void schway__cpu_step_one(schway *inst, struct schway__cpu_log *log);

#endif /* SCHWAYSTATION_CPU_H */
