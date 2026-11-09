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

/* The CPU: 32 general registers (r[0] is the zero register), HI and LO, the
   program counter and COP0. The next program counter is not stored: it
   derives from pc and the branch-delay latch. */
struct schway__cpu {
    uint32_t r[32];
    uint32_t hi, lo;
    uint32_t pc;
    struct schway__cop0 cop0;
};

/* Power-on values (ARCHITECTURE section 7). */
void schway__cpu_reset(schway *inst);
void schway__cop0_reset(schway *inst);

/* The cpu and cop0 fields of the state walk (state.h). */
void schway__cpu_state_io(struct schway__state_io *io, schway *inst);

#endif /* SCHWAYSTATION_CPU_H */
