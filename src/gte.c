/* SPDX-License-Identifier: MIT AND ISC
   Derived from ares-emulator/ares, ares/ps1/cpu/interpreter-gte.cpp, commit
   9408cb43d4948fc3ea6e152a307a34348df3fe04 (ISC licence).
   Copyright (c) 2004-2025 ares team, Near et al
   Changes: ported to integer C: switch dispatch, no signed shifts, struct-only
   interface; the two register files are 32-word arrays in the form a program
   stores them. The licence text is in THIRD-PARTY-NOTICES.md, section "ares
   GTE seed".

   The geometry engine: power-on values, the state walk, register access and
   the command dispatch. */
#include <string.h>

#include "state.h"

static const char name_ready[] = "gte.ready";
static const char name_d[] = "gte.d";
static const char name_c[] = "gte.c";

/* FLAG (control register 31): bits 12-30 are writable and bit 31 is set when
   any of the error bits 13-18 or 23-30 is set (psx-spx, Geometry Transformation
   Engine, GTE Control Registers; docs/hardware/gte.md). */
#define FLAG_WRITABLE 0x7FFFF000u
#define FLAG_ERROR_BITS 0x7F87E000u

static uint32_t flag_fix(uint32_t flag)
{
    flag &= FLAG_WRITABLE;
    return (flag & FLAG_ERROR_BITS) != 0u ? flag | 0x80000000u : flag;
}

/* A 16-bit register is stored as its 16-bit pattern and reads sign-extended
   or zero-extended as the register's type says. */
static uint32_t sext16(uint32_t v)
{
    return (v & 0x8000u) != 0u ? (v & 0xFFFFu) | 0xFFFF0000u : v & 0xFFFFu;
}

/* Leading zeros of a positive LZCS, leading ones of a negative one: 1 to 32
   (psx-spx, GTE Division Inaccuracies and registers LZCS and LZCR). */
static uint32_t lzcr_of(uint32_t lzcs)
{
    uint32_t v = (lzcs & 0x80000000u) != 0u ? lzcs : ~lzcs;
    uint32_t n = 0u;
    while ((v & 0x80000000u) != 0u) {
        v <<= 1;
        n++;
    }
    return n;
}

/* IRGB and ORGB read back IR1-IR3 as 5-bit fields: the value over 80h,
   clamped to 0 to 1Fh, without flags. */
static uint32_t irgb_read(const struct schway__gte *g)
{
    uint32_t out = 0u;
    for (uint32_t i = 0u; i < 3u; i++) {
        uint32_t ir = sext16(g->d[9u + i]);
        uint32_t f = (ir & 0x80000000u) != 0u ? 0u : ir >> 7;
        out |= (f > 31u ? 31u : f) << (5u * i);
    }
    return out;
}

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

/* Registers 0-31 are the data registers and 32-63 the control registers
   (PSX-GPU-AND-GTE section 5.1 and psx-spx, GTE Registers Summary). d[15] and
   d[28] to d[29], d[31] hold nothing of their own: SXYP is SXY2, IRGB and
   ORGB are computed from IR1-IR3 and LZCR from LZCS. */
uint32_t schway__gte_read(const struct schway__gte *g, uint32_t idx)
{
    uint32_t r = idx & 31u;
    if ((idx & 32u) != 0u) {
        switch (r) {
        case 4u: /* RT33, L33, LB3 */
        case 12u:
        case 20u:
        case 26u: /* H reads sign-extended, as the hardware does */
        case 27u: /* DQA */
        case 29u: /* ZSF3 */
        case 30u: /* ZSF4 */
            return sext16(g->c[r]);
        case 31u:
            return flag_fix(g->c[31]);
        default:
            return g->c[r];
        }
    }
    switch (r) {
    case 1u: /* VZ0-VZ2 */
    case 3u:
    case 5u:
    case 8u: /* IR0-IR3 */
    case 9u:
    case 10u:
    case 11u:
        return sext16(g->d[r]);
    case 7u: /* OTZ */
    case 16u: /* SZ0-SZ3 */
    case 17u:
    case 18u:
    case 19u:
        return g->d[r] & 0xFFFFu;
    case 15u:
        return g->d[14];
    case 28u:
    case 29u:
        return irgb_read(g);
    default:
        return g->d[r];
    }
}

void schway__gte_write(struct schway__gte *g, uint32_t idx, uint32_t v)
{
    uint32_t r = idx & 31u;
    if ((idx & 32u) != 0u) {
        switch (r) {
        case 4u:
        case 12u:
        case 20u:
        case 26u:
        case 27u:
        case 29u:
        case 30u:
            g->c[r] = v & 0xFFFFu;
            break;
        case 31u:
            g->c[31] = flag_fix(v);
            break;
        default:
            g->c[r] = v;
            break;
        }
        return;
    }
    switch (r) {
    case 1u:
    case 3u:
    case 5u:
    case 7u:
    case 8u:
    case 9u:
    case 10u:
    case 11u:
    case 16u:
    case 17u:
    case 18u:
    case 19u:
        g->d[r] = v & 0xFFFFu;
        break;
    case 15u: /* a write to SXYP pushes the screen FIFO */
        g->d[12] = g->d[13];
        g->d[13] = g->d[14];
        g->d[14] = v;
        break;
    case 28u: /* IRGB sets IR1-IR3 to five-bit fields times 80h; ORGB and LZCR ignore writes */
        g->d[9] = (v & 31u) * 0x80u;
        g->d[10] = ((v >> 5) & 31u) * 0x80u;
        g->d[11] = ((v >> 10) & 31u) * 0x80u;
        break;
    case 29u:
    case 31u:
        break;
    case 30u:
        g->d[30] = v;
        g->d[31] = lzcr_of(v);
        break;
    default:
        g->d[r] = v;
        break;
    }
}

/* The busy cycles of each command (psx-spx, Geometry Transformation Engine,
   GTE Opcode Summary). The commands compute nothing yet: the registers are
   left as they are. A number outside the table does nothing and takes the
   shortest documented time, 5 cycles; that is a placeholder, not a claim about
   the hardware. Every command ends by recomputing FLAG bit 31. */
uint32_t schway__gte_exec(struct schway__gte *g, uint32_t op)
{
    g->c[31] = flag_fix(g->c[31]);
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
