/* The geometry engine: power-on values, the state walk, register access and
   the command dispatch. */
#include <string.h>

#include "state.h"

static const char name_ready[] = "gte.ready";
static const char name_d[] = "gte.d";
static const char name_c[] = "gte.c";

void schway__gte_reset(struct schway__gte *g)
{
    memset(g, 0, sizeof *g);
}

/* A stamp further ahead of the CPU's cycle count than the longest command is
   refused: the CPU clamps its wait at use as well, as for the multiply stamp.
   The sum saturates, so the bound is the field's own maximum. */
void schway__gte_state_io(struct schway__state_io *io, schway *inst, uint64_t cycle)
{
    struct schway__gte *g = &inst->gte;
    uint64_t max = cycle > UINT64_MAX - SCHWAY__GTE_STALL_MAX ? UINT64_MAX
                                                              : cycle + SCHWAY__GTE_STALL_MAX;
    (void)schway__state_u64(io, name_ready, &g->ready, 0u, max);
    schway__state_u32_array(io, name_d, g->d, 32u, 0u, UINT32_MAX);
    schway__state_u32_array(io, name_c, g->c, 32u, 0u, UINT32_MAX);
}

uint32_t schway__gte_read(const struct schway__gte *g, uint32_t idx)
{
    return (idx & 32u) != 0u ? g->c[idx & 31u] : g->d[idx & 31u];
}

void schway__gte_write(struct schway__gte *g, uint32_t idx, uint32_t v)
{
    if ((idx & 32u) != 0u) {
        g->c[idx & 31u] = v;
    } else {
        g->d[idx & 31u] = v;
    }
}

/* The busy cycles of each command (psx-spx, Geometry Transformation Engine,
   GTE Opcode Summary). The commands compute nothing yet: the registers are
   left as they are. A number outside the table does nothing and takes the
   shortest documented time, 5 cycles; that is a placeholder, not a claim about
   the hardware. */
uint32_t schway__gte_exec(struct schway__gte *g, uint32_t op)
{
    (void)g;
    switch (op & 0x3Fu) {
    case 0x01u: /* RTPS */
        return 15u;
    case 0x06u: /* NCLIP */
        return 8u;
    case 0x0Cu: /* OP */
        return 6u;
    case 0x10u: /* DPCS */
        return 8u;
    case 0x11u: /* INTPL */
        return 8u;
    case 0x12u: /* MVMVA */
        return 8u;
    case 0x13u: /* NCDS */
        return 19u;
    case 0x14u: /* CDP */
        return 13u;
    case 0x16u: /* NCDT */
        return 44u;
    case 0x1Bu: /* NCCS */
        return 17u;
    case 0x1Cu: /* CC */
        return 11u;
    case 0x1Eu: /* NCS */
        return 14u;
    case 0x20u: /* NCT */
        return 30u;
    case 0x28u: /* SQR */
        return 5u;
    case 0x29u: /* DCPL */
        return 8u;
    case 0x2Au: /* DPCT */
        return 17u;
    case 0x2Du: /* AVSZ3 */
        return 5u;
    case 0x2Eu: /* AVSZ4 */
        return 6u;
    case 0x30u: /* RTPT */
        return 23u;
    case 0x3Du: /* GPF */
        return 5u;
    case 0x3Eu: /* GPL */
        return 5u;
    case 0x3Fu: /* NCCT */
        return 39u;
    default:
        return 5u;
    }
}
