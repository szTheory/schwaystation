/* The CPU's registers: power-on values and the state walk. */
#include <string.h>

#include "state.h"

static const char name_r[] = "cpu.r";
static const char name_hi[] = "cpu.hi";
static const char name_lo[] = "cpu.lo";
static const char name_pc[] = "cpu.pc";
static const char name_cop0[] = "cop0.r";

/* General registers, HI and LO are 0 and the program counter is the BIOS
   entry: the machine starts at BFC00000h (ARCHITECTURE section 7,
   Power-on). */
void schway__cpu_reset(schway *inst)
{
    struct schway__cpu *c = &inst->cpu;
    memset(c->r, 0, sizeof c->r);
    c->hi = 0u;
    c->lo = 0u;
    c->pc = SCHWAY__CPU_RESET_PC;
    schway__cop0_reset(inst);
}

void schway__cpu_state_io(struct schway__state_io *io, schway *inst)
{
    struct schway__cpu *c = &inst->cpu;
    schway__state_u32_array(io, name_r, c->r, 32u, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_hi, &c->hi, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_lo, &c->lo, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_pc, &c->pc, 0u, UINT32_MAX);
    schway__state_u32_array(io, name_cop0, c->cop0.r, 32u, 0u, UINT32_MAX);
}
