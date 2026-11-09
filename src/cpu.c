/* The CPU: power-on values, the state walk and the one-instruction entry. */
#include <string.h>

#include "state.h"

static const char name_r[] = "cpu.r";
static const char name_hi[] = "cpu.hi";
static const char name_lo[] = "cpu.lo";
static const char name_pc[] = "cpu.pc";
static const char name_cop0[] = "cop0.r";
static const char name_load_reg[] = "cpu.load_reg";
static const char name_load_value[] = "cpu.load_value";
static const char name_branch_slot[] = "cpu.branch_slot";
static const char name_branch_take[] = "cpu.branch_take";
static const char name_branch_target[] = "cpu.branch_target";

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
    c->load_reg = SCHWAY__CPU_NO_LOAD;
    c->load_value = 0u;
    c->branch_slot = 0u;
    c->branch_take = 0u;
    c->branch_target = 0u;
    c->log = NULL;
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
    (void)schway__state_u32(io, name_load_reg, &c->load_reg, 0u, SCHWAY__CPU_NO_LOAD);
    (void)schway__state_u32(io, name_load_value, &c->load_value, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_branch_slot, &c->branch_slot, 0u, 1u);
    (void)schway__state_u32(io, name_branch_take, &c->branch_take, 0u, 1u);
    (void)schway__state_u32(io, name_branch_target, &c->branch_target, 0u, UINT32_MAX);
}

#include "cpu_exec.inc"

void schway__cpu_step_one(schway *inst, struct schway__cpu_log *log)
{
    inst->cpu.log = log;
    (void)cpu_step(inst);
    inst->cpu.log = NULL;
}
