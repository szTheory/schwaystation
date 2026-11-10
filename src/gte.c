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

/* The numerics. Every value a command works on is an int64_t, so each product
   of two 16-bit values, or of a 17-bit quotient and a 16-bit value, is a
   widening multiply that cannot overflow; an accumulator never leaves 44 bits
   plus the term added to it. There is no signed shift: a left shift of a
   signed value is a multiply by 4096 or 16, and a right shift is sar64. The
   formulas are those of psx-spx, Geometry Transformation Engine (core
   formulas, FLAG, division inaccuracy); docs/hardware/gte.md. */
struct run {
    struct schway__gte *g;
    uint32_t flag; /* FLAG as the command builds it */
    unsigned sf;   /* command bit 19: shift the MAC results right by 12 */
    unsigned lm;   /* command bit 10: clamp IR1 to IR3 at 0 */
};

/* FLAG bits by accumulator: MAC0, MAC1, MAC2, MAC3 and IR0, IR1, IR2, IR3. */
static const uint32_t mac_over_bit[4] = {1u << 16, 1u << 30, 1u << 29, 1u << 28};
static const uint32_t mac_under_bit[4] = {1u << 15, 1u << 27, 1u << 26, 1u << 25};
static const uint32_t ir_sat_bit[4] = {1u << 12, 1u << 24, 1u << 23, 1u << 22};
#define FLAG_SZ3_OTZ (1u << 18)
#define FLAG_DIVIDE (1u << 17)
#define FLAG_SX2 (1u << 14)
#define FLAG_SY2 (1u << 13)

static int64_t s16v(uint32_t v)
{
    return (int64_t)(v & 0xFFFFu) - ((v & 0x8000u) != 0u ? 0x10000 : 0);
}

static int64_t s32v(uint32_t v)
{
    return (int64_t)v - ((v & 0x80000000u) != 0u ? 0x100000000ll : 0ll);
}

static uint32_t low32(int64_t v)
{
    return (uint32_t)((uint64_t)v & 0xFFFFFFFFu);
}

static uint32_t low16(int64_t v)
{
    return (uint32_t)((uint64_t)v & 0xFFFFu);
}

/* The low 32 bits of v, read as a signed value (a MAC register's width). */
static int64_t wrap32(int64_t v)
{
    return s32v(low32(v));
}

/* Arithmetic right shift: the sign bits come in from the top, built on
   unsigned arithmetic. n is 0 to 63. */
static int64_t sar64(int64_t v, unsigned n)
{
    uint64_t s = (uint64_t)v >> n;
    if (v < 0) {
        s |= ~(UINT64_MAX >> n);
    }
    return (int64_t)(s & (uint64_t)INT64_MAX) + ((s >> 63) != 0u ? INT64_MIN : 0);
}

/* The low 44 bits of v, read as a signed value (the MAC1 to MAC3 width). */
static int64_t sclip44(int64_t v)
{
    uint64_t u = (uint64_t)v & 0xFFFFFFFFFFFull;
    return (int64_t)u - ((u & 0x80000000000ull) != 0u ? 0x100000000000ll : 0ll);
}

/* Sets the FLAG bit of an accumulator that left its range: 32 bits for MAC0,
   44 bits for MAC1 to MAC3. The value is returned unchanged. */
static int64_t check_mac(struct run *r, unsigned id, int64_t v)
{
    int64_t lim = (int64_t)((uint64_t)1u << (id == 0u ? 31u : 43u));
    if (v < -lim) {
        r->flag |= mac_under_bit[id];
    }
    if (v > lim - 1) {
        r->flag |= mac_over_bit[id];
    }
    return v;
}

static int64_t extend(struct run *r, unsigned id, int64_t v)
{
    return sclip44(check_mac(r, id, v));
}

/* Stores an accumulator: MAC0 whole, MAC1 to MAC3 shifted by sf. Returns the
   value that went in, shifted for MAC1 to MAC3. */
static int64_t set_mac(struct run *r, unsigned id, int64_t v)
{
    (void)check_mac(r, id, v);
    if (id != 0u) {
        v = sar64(v, r->sf != 0u ? 12u : 0u);
    }
    r->g->d[24u + id] = low32(v);
    return v;
}

/* IR0 clamps to 0 to 1000h, IR1 to IR3 to -8000h or 0 up to 7FFFh. */
static int64_t sat_ir(struct run *r, unsigned id, int64_t v, unsigned lm)
{
    int64_t lo = id == 0u ? 0 : -0x8000;
    int64_t hi = id == 0u ? 0x1000 : 0x7FFF;
    int64_t lmm = lm != 0u ? 0 : lo;
    if (v < lmm) {
        v = lmm;
        r->flag |= ir_sat_bit[id];
    }
    if (v > hi) {
        v = hi;
        r->flag |= ir_sat_bit[id];
    }
    return v;
}

static void set_ir(struct run *r, unsigned id, int64_t v, unsigned lm)
{
    r->g->d[8u + id] = low16(sat_ir(r, id, v, lm));
}

static void set_mac_ir(struct run *r, unsigned id, int64_t v, unsigned lm)
{
    set_ir(r, id, wrap32(set_mac(r, id, v)), lm);
}

static int64_t ir_of(const struct run *r, unsigned id)
{
    return s16v(r->g->d[8u + id]);
}

/* Element e of a 3 by 3 matrix of 16-bit values packed two to a word in row
   order; the ninth sits alone in the last word. */
static int64_t mat_elem(const uint32_t *w, unsigned e)
{
    return s16v((e & 1u) != 0u ? w[e >> 1] >> 16 : w[e >> 1]);
}

/* Pushes the screen FIFO's Z, clamped to 0 to FFFFh. */
static void push_sz(struct run *r, int64_t sz)
{
    struct schway__gte *g = r->g;
    if (sz < 0) {
        sz = 0;
        r->flag |= FLAG_SZ3_OTZ;
    }
    if (sz > 0xFFFF) {
        sz = 0xFFFF;
        r->flag |= FLAG_SZ3_OTZ;
    }
    g->d[16] = g->d[17];
    g->d[17] = g->d[18];
    g->d[18] = g->d[19];
    g->d[19] = (uint32_t)sz;
}

/* Pushes X (half 0) or Y (half 1) of the screen FIFO, clamped to -400h to
   3FFh; the two halves of each entry move on their own. */
static void push_sxy(struct run *r, unsigned half, int64_t v)
{
    struct schway__gte *g = r->g;
    uint32_t sh = 16u * half;
    uint32_t keep = ~(0xFFFFu << sh);
    if (v < -1024) {
        v = -1024;
        r->flag |= half != 0u ? FLAG_SY2 : FLAG_SX2;
    }
    if (v > 1023) {
        v = 1023;
        r->flag |= half != 0u ? FLAG_SY2 : FLAG_SX2;
    }
    g->d[12] = (g->d[12] & keep) | (((g->d[13] >> sh) & 0xFFFFu) << sh);
    g->d[13] = (g->d[13] & keep) | (((g->d[14] >> sh) & 0xFFFFu) << sh);
    g->d[14] = (g->d[14] & keep) | (low16(v) << sh);
}

/* The reciprocal table of the divide: entry i is the closest 8-bit
   correction of 1 / (1 + i / 256), as max(0, (40000h / (i + 100h) + 1) / 2 -
   101h) for i below 256, and 0 for the last entry (psx-spx, Geometry
   Transformation Engine, GTE Division Inaccuracy). The unr_table test
   regenerates it from that formula and compares all 257 bytes. */
static const uint8_t unr_table[257] = {
    0xFF, 0xFD, 0xFB, 0xF9, 0xF7, 0xF5, 0xF3, 0xF1, 0xEF, 0xEE, 0xEC, 0xEA, 0xE8, 0xE6, 0xE4, 0xE3,
    0xE1, 0xDF, 0xDD, 0xDC, 0xDA, 0xD8, 0xD6, 0xD5, 0xD3, 0xD1, 0xD0, 0xCE, 0xCD, 0xCB, 0xC9, 0xC8,
    0xC6, 0xC5, 0xC3, 0xC1, 0xC0, 0xBE, 0xBD, 0xBB, 0xBA, 0xB8, 0xB7, 0xB5, 0xB4, 0xB2, 0xB1, 0xB0,
    0xAE, 0xAD, 0xAB, 0xAA, 0xA9, 0xA7, 0xA6, 0xA4, 0xA3, 0xA2, 0xA0, 0x9F, 0x9E, 0x9C, 0x9B, 0x9A,
    0x99, 0x97, 0x96, 0x95, 0x94, 0x92, 0x91, 0x90, 0x8F, 0x8D, 0x8C, 0x8B, 0x8A, 0x89, 0x87, 0x86,
    0x85, 0x84, 0x83, 0x82, 0x81, 0x7F, 0x7E, 0x7D, 0x7C, 0x7B, 0x7A, 0x79, 0x78, 0x77, 0x75, 0x74,
    0x73, 0x72, 0x71, 0x70, 0x6F, 0x6E, 0x6D, 0x6C, 0x6B, 0x6A, 0x69, 0x68, 0x67, 0x66, 0x65, 0x64,
    0x63, 0x62, 0x61, 0x60, 0x5F, 0x5E, 0x5D, 0x5D, 0x5C, 0x5B, 0x5A, 0x59, 0x58, 0x57, 0x56, 0x55,
    0x54, 0x53, 0x53, 0x52, 0x51, 0x50, 0x4F, 0x4E, 0x4D, 0x4D, 0x4C, 0x4B, 0x4A, 0x49, 0x48, 0x48,
    0x47, 0x46, 0x45, 0x44, 0x43, 0x43, 0x42, 0x41, 0x40, 0x3F, 0x3F, 0x3E, 0x3D, 0x3C, 0x3C, 0x3B,
    0x3A, 0x39, 0x39, 0x38, 0x37, 0x36, 0x36, 0x35, 0x34, 0x33, 0x33, 0x32, 0x31, 0x31, 0x30, 0x2F,
    0x2E, 0x2E, 0x2D, 0x2C, 0x2C, 0x2B, 0x2A, 0x2A, 0x29, 0x28, 0x28, 0x27, 0x26, 0x26, 0x25, 0x24,
    0x24, 0x23, 0x22, 0x22, 0x21, 0x20, 0x20, 0x1F, 0x1E, 0x1E, 0x1D, 0x1D, 0x1C, 0x1B, 0x1B, 0x1A,
    0x19, 0x19, 0x18, 0x18, 0x17, 0x16, 0x16, 0x15, 0x15, 0x14, 0x14, 0x13, 0x12, 0x12, 0x11, 0x11,
    0x10, 0x0F, 0x0F, 0x0E, 0x0E, 0x0D, 0x0D, 0x0C, 0x0C, 0x0B, 0x0A, 0x0A, 0x09, 0x09, 0x08, 0x08,
    0x07, 0x07, 0x06, 0x06, 0x05, 0x05, 0x04, 0x04, 0x03, 0x03, 0x02, 0x02, 0x01, 0x01, 0x00, 0x00,
    0x00,
};

/* The 17-bit quotient of H over SZ3 the projection uses, clamped to 1FFFFh;
   a quotient of 2 or more sets the divide overflow bit and gives 1FFFFh. The
   divisor is normalised, its reciprocal comes from the table with one
   Newton-Raphson step, and the product is rounded (psx-spx, GTE Division
   Inaccuracy). Everything here is unsigned or a non-negative product except
   the one correction term, which uses sar64. */
static uint32_t gte_divide(struct run *r, uint32_t lhs, uint32_t rhs)
{
    if (rhs * 2u <= lhs) {
        r->flag |= FLAG_DIVIDE;
        return 0x1FFFFu;
    }
    uint32_t shift = 0u;
    while (shift < 16u && ((rhs << shift) & 0x8000u) == 0u) {
        shift++;
    }
    lhs <<= shift;
    rhs <<= shift;
    int64_t divisor = (int64_t)(rhs | 0x8000u);
    int64_t x = 0x101 + (int64_t)unr_table[((rhs & 0x7FFFu) + 0x40u) >> 7];
    int64_t d = sar64(divisor * -x + 0x80, 8u);
    int64_t reciprocal = sar64(x * (0x20000 + d) + 0x80, 8u);
    uint64_t result = ((uint64_t)lhs * (uint64_t)reciprocal + 0x8000u) >> 16;
    return result > 0x1FFFFu ? 0x1FFFFu : (uint32_t)result;
}

/* One perspective transform: the vertex through the rotation matrix and
   translation, the depth into the Z FIFO, the screen point from H over that
   depth, and, on the last vertex of the command, the depth cue into IR0 and
   MAC0. IR3 and its flag follow the quirk of the hardware: the flag comes from
   the value over 1000h as if lm were 0, the register from the shifted MAC3
   with the command's lm (psx-spx, GTE RTPS and RTPT). */
static void rtp(struct run *r, unsigned vertex, int last)
{
    struct schway__gte *g = r->g;
    const uint32_t *rot = &g->c[0];
    uint32_t vxy = g->d[2u * vertex];
    int64_t vec[3] = {s16v(vxy), s16v(vxy >> 16), s16v(g->d[2u * vertex + 1u])};
    int64_t out[3];
    for (unsigned row = 0u; row < 3u; row++) {
        unsigned id = row + 1u;
        int64_t acc = s32v(g->c[5u + row]) * 4096;
        for (unsigned col = 0u; col < 3u; col++) {
            acc = extend(r, id, acc + mat_elem(rot, row * 3u + col) * vec[col]);
        }
        out[row] = acc;
    }
    set_mac_ir(r, 1u, out[0], r->lm);
    set_mac_ir(r, 2u, out[1], r->lm);
    (void)set_mac(r, 3u, out[2]);
    (void)sat_ir(r, 3u, wrap32(sar64(out[2], 12u)), 0u);
    int64_t mac3 = s32v(g->d[27]);
    int64_t lo = r->lm != 0u ? 0 : -0x8000;
    g->d[11] = low16(mac3 < lo ? lo : (mac3 > 0x7FFF ? 0x7FFF : mac3));

    push_sz(r, wrap32(sar64(out[2], 12u)));
    int64_t dv = (int64_t)gte_divide(r, (uint32_t)(g->c[26] & 0xFFFFu), g->d[19] & 0xFFFFu);
    int64_t sx = sar64(check_mac(r, 0u, dv * ir_of(r, 1u) + s32v(g->c[24])), 16u);
    int64_t sy = sar64(check_mac(r, 0u, dv * ir_of(r, 2u) + s32v(g->c[25])), 16u);
    push_sxy(r, 0u, wrap32(sx));
    push_sxy(r, 1u, wrap32(sy));
    if (!last) {
        return;
    }
    int64_t sz = set_mac(r, 0u, dv * s16v(g->c[27]) + s32v(g->c[28]));
    g->d[8] = low16(sat_ir(r, 0u, wrap32(sar64(sz, 12u)), 0u));
}

static void finish(struct run *r)
{
    r->g->c[31] = flag_fix(r->flag);
}

/* The busy cycles of each command (psx-spx, Geometry Transformation Engine,
   GTE Opcode Summary). RTPS computes; the others leave the registers as they
   are until their numerics land. A number outside the table does nothing and
   takes the shortest documented time, 5 cycles; that is a placeholder, not a
   claim about the hardware. A command that computes clears FLAG first and
   recomputes bit 31 last; the others only recompute it. */
uint32_t schway__gte_exec(struct schway__gte *g, uint32_t op)
{
    struct run r;
    r.g = g;
    r.flag = 0u;
    r.sf = (op >> 19) & 1u;
    r.lm = (op >> 10) & 1u;
    g->c[31] = flag_fix(g->c[31]);
    switch (op & 0x3Fu) {
    case 0x01u: /* RTPS */
        rtp(&r, 0u, 1);
        finish(&r);
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
