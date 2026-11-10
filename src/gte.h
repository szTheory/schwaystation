/* The geometry engine (COP2): state struct and the module's declarations.
   Not installed. The instance embeds the struct, so gte.c is edited alone. */
#ifndef SCHWAYSTATION_GTE_H
#define SCHWAYSTATION_GTE_H

#include <stdint.h>

#include "schwaystation.h"

struct schway__state_io;

/* The longest command, NCDT, takes 44 cycles (psx-spx, Geometry Transformation
   Engine, GTE Opcode Summary), so no wait on the engine is ever longer. */
#define SCHWAY__GTE_STALL_MAX 44u

/* ready is the value of cpu.cycle at which the engine finishes its last
   command; a read of a result or a new command issued before it waits to
   that stamp. d and c are the data registers (cop2r0-31) and the control
   registers (cop2r32-63) in the forms a program stores them
   (PSX-GPU-AND-GTE section 5.1). ready comes first so the layout does not
   depend on alignment. */
struct schway__gte {
    uint64_t ready;
    uint32_t d[32];
    uint32_t c[32];
};

/* Power-on: every register and the stamp are 0. */
void schway__gte_reset(struct schway__gte *g);

/* The gte fields of the state walk (state.h). cycle is the cpu.cycle value the
   walk has just read, the origin of the bound on ready. */
void schway__gte_state_io(struct schway__state_io *io, schway *inst, uint64_t cycle);

/* A register as MFC2/CFC2 and MTC2/CTC2 see it: idx 0-31 selects a data
   register, 32-63 a control register. */
uint32_t schway__gte_read(const struct schway__gte *g, uint32_t idx);
void schway__gte_write(struct schway__gte *g, uint32_t idx, uint32_t v);

/* Executes the command in the low 25 bits of op, number op & 3Fh, and returns
   the cycles the engine is busy. It takes only the engine, so a test calls it
   directly. */
uint32_t schway__gte_exec(struct schway__gte *g, uint32_t op);

#endif /* SCHWAYSTATION_GTE_H */
