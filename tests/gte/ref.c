/* The independent geometry engine model (see ref.h). Every register format,
   read rule and formula here comes from psx-spx, Geometry Transformation
   Engine (https://psx-spx.consoledev.net/geometrytransformationenginegte/),
   or from the hardware-captured cases. Arithmetic is plain integer: 64-bit
   values, divisions that round toward minus infinity where the hardware
   description says "SAR", and no signed shift anywhere. */
#include <stdint.h>

#include "ref.h"

/* FLAG bits (psx-spx, GTE Saturation). The three-per-row groups are written as
   the bit of the first row and shifted right by the row index. */
#define F_MAC_POS 0x40000000  /* bits 30 to 28: MAC1 to MAC3 above 43 bits */
#define F_MAC_NEG 0x08000000  /* bits 27 to 25: MAC1 to MAC3 below -43 bits */
#define F_IR 0x01000000       /* bits 24 to 22: IR1 to IR3 saturated */
#define F_COLOR 0x00200000    /* bits 21 to 19: colour FIFO R, G, B saturated */
#define F_SZ_OTZ 0x00040000   /* bit 18: SZ3 or OTZ saturated */
#define F_DIV 0x00020000      /* bit 17: divide overflow */
#define F_MAC0_POS 0x00010000 /* bit 16: MAC0 above 31 bits */
#define F_MAC0_NEG 0x00008000 /* bit 15: MAC0 below -31 bits */
#define F_SX 0x00004000       /* bit 14: SX2 saturated */
#define F_SY 0x00002000       /* bit 13: SY2 saturated */
#define F_IR0 0x00001000      /* bit 12: IR0 saturated */

/* The bits software can write: 30 to 12. Bit 31 is not stored; it reads as the
   OR of bits 30 to 23 and 18 to 13. */
#define FLAG_WRITABLE 0x7FFFF000u
#define FLAG_ERROR_BITS 0x7F87E000u

static int32_t s16(uint32_t v)
{
    int32_t x = (int32_t)(v & 0xFFFFu);
    return x >= 0x8000 ? x - 0x10000 : x;
}

static int32_t s32(uint32_t v)
{
    int64_t x = (int64_t)v;
    if (x >= 0x80000000) {
        x -= 0x100000000;
    }
    return (int32_t)x;
}

static uint32_t u16(int32_t x)
{
    return (uint32_t)x & 0xFFFFu;
}

/* A pair of 16-bit registers in one word, the first in the low half. */
static uint32_t pair(int32_t lo, int32_t hi)
{
    return u16(lo) | u16(hi) << 16;
}

/* The low 32 bits of a 64-bit value, as a signed number. */
static int32_t low32(int64_t v)
{
    return s32((uint32_t)((uint64_t)v & 0xFFFFFFFFu));
}

/* v divided by 2^bits, rounded toward minus infinity (an arithmetic shift
   right). */
static int64_t floor_shr(int64_t v, unsigned bits)
{
    int64_t d = (int64_t)1 << bits;
    int64_t q = v / d;
    if (v % d < 0) {
        q -= 1;
    }
    return q;
}

/* ---- the register file ------------------------------------------------- */

/* A matrix is nine 16-bit elements, row by row, two to a register: register
   k holds elements 2k and 2k+1, the last register holds only element 8
   (psx-spx, Matrix Registers). */
static void matrix_write(int32_t m[3][3], uint32_t k, uint32_t v)
{
    uint32_t e = 2u * k;
    m[e / 3u][e % 3u] = s16(v);
    if (k < 4u) {
        m[(e + 1u) / 3u][(e + 1u) % 3u] = s16(v >> 16);
    }
}

static uint32_t matrix_read(const int32_t m[3][3], uint32_t k)
{
    uint32_t e = 2u * k;
    if (k == 4u) {
        return (uint32_t)m[2][2]; /* the last element reads sign-expanded */
    }
    return pair(m[e / 3u][e % 3u], m[(e + 1u) / 3u][(e + 1u) % 3u]);
}

/* IRGB: three 5-bit colours multiplied by 80h into IR1 to IR3 (psx-spx, cop2r28). */
static void irgb_write(struct gte_ref *r, uint32_t v)
{
    r->ir[1] = (int32_t)(v & 0x1Fu) * 0x80;
    r->ir[2] = (int32_t)((v >> 5) & 0x1Fu) * 0x80;
    r->ir[3] = (int32_t)((v >> 10) & 0x1Fu) * 0x80;
}

/* ORGB (and IRGB read back): IR1 to IR3 divided by 80h and saturated to 0 to
   1Fh, with no flag (psx-spx, cop2r29). */
static uint32_t orgb_read(const struct gte_ref *r)
{
    uint32_t out = 0u;
    for (int k = 1; k <= 3; k++) {
        int32_t c = r->ir[k] < 0 ? 0 : r->ir[k] / 0x80;
        if (c > 0x1F) {
            c = 0x1F;
        }
        out |= (uint32_t)c << (5 * (k - 1));
    }
    return out;
}

/* LZCR: the count of leading zeros of LZCS when it is not negative, of
   leading ones when it is, 1 to 32 (psx-spx, cop2r31). */
static uint32_t lzcr_read(const struct gte_ref *r)
{
    uint32_t v = (uint32_t)r->lzcs;
    uint32_t n = 32u;
    if (r->lzcs < 0) {
        v = ~v;
    }
    while (v != 0u) {
        v >>= 1;
        n--;
    }
    return n;
}

void gte_ref_write(struct gte_ref *r, uint32_t idx, uint32_t v)
{
    if (idx <= 5u) {
        uint32_t n = idx / 2u;
        if (idx % 2u == 0u) {
            r->vx[n] = s16(v);
            r->vy[n] = s16(v >> 16);
        } else {
            r->vz[n] = s16(v);
        }
    } else if (idx == 6u) {
        r->rgbc_r = (int32_t)(v & 0xFFu);
        r->rgbc_g = (int32_t)((v >> 8) & 0xFFu);
        r->rgbc_b = (int32_t)((v >> 16) & 0xFFu);
        r->rgbc_c = (int32_t)(v >> 24);
    } else if (idx == 7u) {
        r->otz = (int32_t)(v & 0xFFFFu);
    } else if (idx >= 8u && idx <= 11u) {
        r->ir[idx - 8u] = s16(v);
    } else if (idx >= 12u && idx <= 14u) {
        r->sx[idx - 12u] = s16(v);
        r->sy[idx - 12u] = s16(v >> 16);
    } else if (idx == 15u) {
        /* SXYP: writing it moves SXY2 and SXY1 down one stage. */
        r->sx[0] = r->sx[1];
        r->sy[0] = r->sy[1];
        r->sx[1] = r->sx[2];
        r->sy[1] = r->sy[2];
        r->sx[2] = s16(v);
        r->sy[2] = s16(v >> 16);
    } else if (idx >= 16u && idx <= 19u) {
        r->sz[idx - 16u] = (int32_t)(v & 0xFFFFu);
    } else if (idx >= 20u && idx <= 22u) {
        uint32_t n = idx - 20u;
        r->rgb_r[n] = (int32_t)(v & 0xFFu);
        r->rgb_g[n] = (int32_t)((v >> 8) & 0xFFu);
        r->rgb_b[n] = (int32_t)((v >> 16) & 0xFFu);
        r->rgb_c[n] = (int32_t)(v >> 24);
    } else if (idx == 23u) {
        r->res1 = s32(v);
    } else if (idx >= 24u && idx <= 27u) {
        r->mac[idx - 24u] = s32(v);
    } else if (idx == 28u) {
        irgb_write(r, v);
    } else if (idx == 30u) {
        r->lzcs = s32(v);
    } else if (idx >= 32u && idx <= 36u) {
        matrix_write(r->rt, idx - 32u, v);
    } else if (idx >= 37u && idx <= 39u) {
        r->tr[idx - 37u] = s32(v);
    } else if (idx >= 40u && idx <= 44u) {
        matrix_write(r->ll, idx - 40u, v);
    } else if (idx >= 45u && idx <= 47u) {
        r->bk[idx - 45u] = s32(v);
    } else if (idx >= 48u && idx <= 52u) {
        matrix_write(r->lc, idx - 48u, v);
    } else if (idx >= 53u && idx <= 55u) {
        r->fc[idx - 53u] = s32(v);
    } else if (idx == 56u) {
        r->ofx = s32(v);
    } else if (idx == 57u) {
        r->ofy = s32(v);
    } else if (idx == 58u) {
        r->h = (int32_t)(v & 0xFFFFu);
    } else if (idx == 59u) {
        r->dqa = s16(v);
    } else if (idx == 60u) {
        r->dqb = s32(v);
    } else if (idx == 61u) {
        r->zsf3 = s16(v);
    } else if (idx == 62u) {
        r->zsf4 = s16(v);
    } else if (idx == 63u) {
        r->flag = (int32_t)(v & FLAG_WRITABLE);
    }
    /* ORGB (29), LZCR (31) and indexes past 63 hold nothing to write. */
}

uint32_t gte_ref_read(const struct gte_ref *r, uint32_t idx)
{
    if (idx <= 5u) {
        uint32_t n = idx / 2u;
        return idx % 2u == 0u ? pair(r->vx[n], r->vy[n]) : (uint32_t)r->vz[n];
    }
    if (idx == 6u) {
        return (uint32_t)r->rgbc_r | (uint32_t)r->rgbc_g << 8 | (uint32_t)r->rgbc_b << 16 |
               (uint32_t)r->rgbc_c << 24;
    }
    if (idx == 7u) {
        return (uint32_t)r->otz;
    }
    if (idx >= 8u && idx <= 11u) {
        return (uint32_t)r->ir[idx - 8u]; /* sign-expanded to 32 bits */
    }
    if (idx >= 12u && idx <= 15u) {
        uint32_t n = idx == 15u ? 2u : idx - 12u; /* SXYP reads as SXY2 */
        return pair(r->sx[n], r->sy[n]);
    }
    if (idx >= 16u && idx <= 19u) {
        return (uint32_t)r->sz[idx - 16u];
    }
    if (idx >= 20u && idx <= 22u) {
        uint32_t n = idx - 20u;
        return (uint32_t)r->rgb_r[n] | (uint32_t)r->rgb_g[n] << 8 | (uint32_t)r->rgb_b[n] << 16 |
               (uint32_t)r->rgb_c[n] << 24;
    }
    if (idx == 23u) {
        return (uint32_t)r->res1;
    }
    if (idx >= 24u && idx <= 27u) {
        return (uint32_t)r->mac[idx - 24u];
    }
    if (idx == 28u || idx == 29u) {
        return orgb_read(r);
    }
    if (idx == 30u) {
        return (uint32_t)r->lzcs;
    }
    if (idx == 31u) {
        return lzcr_read(r);
    }
    if (idx >= 32u && idx <= 36u) {
        return matrix_read(r->rt, idx - 32u);
    }
    if (idx >= 37u && idx <= 39u) {
        return (uint32_t)r->tr[idx - 37u];
    }
    if (idx >= 40u && idx <= 44u) {
        return matrix_read(r->ll, idx - 40u);
    }
    if (idx >= 45u && idx <= 47u) {
        return (uint32_t)r->bk[idx - 45u];
    }
    if (idx >= 48u && idx <= 52u) {
        return matrix_read(r->lc, idx - 48u);
    }
    if (idx >= 53u && idx <= 55u) {
        return (uint32_t)r->fc[idx - 53u];
    }
    if (idx == 56u) {
        return (uint32_t)r->ofx;
    }
    if (idx == 57u) {
        return (uint32_t)r->ofy;
    }
    if (idx == 58u) {
        /* H is unsigned, but reading it sign-expands it (psx-spx, Screen
           Offset and Distance). */
        return (uint32_t)(r->h >= 0x8000 ? r->h - 0x10000 : r->h);
    }
    if (idx == 59u) {
        return (uint32_t)r->dqa;
    }
    if (idx == 60u) {
        return (uint32_t)r->dqb;
    }
    if (idx == 61u) {
        return (uint32_t)r->zsf3;
    }
    if (idx == 62u) {
        return (uint32_t)r->zsf4;
    }
    if (idx == 63u) {
        uint32_t f = (uint32_t)r->flag;
        return (f & FLAG_ERROR_BITS) != 0u ? f | 0x80000000u : f;
    }
    return 0u;
}

/* ---- arithmetic shared by the commands --------------------------------- */

/* One step of an accumulator for MAC1 to MAC3 (k = 1 to 3): 44 bits, signed.
   A value above 2^43-1 sets the positive flag and one below -2^43 the
   negative flag, and the accumulator keeps its low 44 bits (psx-spx, GTE
   Saturation, bits 30 to 25). */
static int64_t acc_step(struct gte_ref *r, int k, int64_t v)
{
    if (v > 0x7FFFFFFFFFFLL) {
        r->flag |= F_MAC_POS >> (k - 1);
    } else if (v < -0x80000000000LL) {
        r->flag |= F_MAC_NEG >> (k - 1);
    }
    uint64_t u = (uint64_t)v & 0xFFFFFFFFFFFULL;
    if ((u & 0x80000000000ULL) != 0u) {
        return (int64_t)u - 0x100000000000LL;
    }
    return (int64_t)u;
}

/* MAC0 takes a 64-bit sum and keeps its low 32 bits; a sum outside 32 bits
   sets bit 16 (above) or 15 (below). */
static void mac0_set(struct gte_ref *r, int64_t v)
{
    if (v > 0x7FFFFFFFLL) {
        r->flag |= F_MAC0_POS;
    } else if (v < -0x80000000LL) {
        r->flag |= F_MAC0_NEG;
    }
    r->mac[0] = low32(v);
}

/* MACk = acc shifted right by shift bits, kept to 32 bits. */
static void mac_set(struct gte_ref *r, int k, int64_t acc, unsigned shift)
{
    r->mac[k] = low32(floor_shr(acc, shift));
}

/* IRk = MACk saturated to -8000h..7FFFh, or to 0..7FFFh when lm is set; a
   saturation sets IR flag k (psx-spx, GTE Saturation). */
static void ir_from_mac(struct gte_ref *r, int k, int lm)
{
    int32_t lo = lm ? 0 : -0x8000;
    int32_t m = r->mac[k];
    if (m < lo) {
        r->ir[k] = lo;
        r->flag |= F_IR >> (k - 1);
    } else if (m > 0x7FFF) {
        r->ir[k] = 0x7FFF;
        r->flag |= F_IR >> (k - 1);
    } else {
        r->ir[k] = m;
    }
}

/* The sum of translation and a matrix row times a vector, step by step with
   the 44-bit check after each addition. start is the translation already
   multiplied by 1000h, or 0. */
static int64_t row_sum(struct gte_ref *r, int k, int64_t start, const int32_t m[3], const int32_t v[3])
{
    int64_t acc = start;
    for (int i = 0; i < 3; i++) {
        acc = acc_step(r, k, acc + (int64_t)m[i] * v[i]);
    }
    return acc;
}

/* ---- the divide -------------------------------------------------------- */

/* The reciprocal seed the divide looks up: max(0, (40000h / (i + 100h) + 1) / 2
   - 101h), for i 0 to 256 (psx-spx, GTE Division Inaccuracy). */
static uint32_t recip_seed(uint32_t i)
{
    int32_t v = (int32_t)((0x40000u / (i + 0x100u) + 1u) / 2u) - 0x101;
    return v < 0 ? 0u : (uint32_t)v;
}

/* The perspective divide ((H * 20000h / SZ3) + 1) / 2 by the engine's
   Newton-Raphson steps, saturated to 1FFFFh. An overflow (SZ3 * 2 <= H, and
   any SZ3 of 0) returns 1FFFFh with flag 17 set (psx-spx, GTE Division
   Inaccuracy). */
static uint32_t rtp_divide(struct gte_ref *r, uint32_t h, uint32_t sz3)
{
    if (h >= sz3 * 2u) {
        r->flag |= F_DIV;
        return 0x1FFFFu;
    }
    unsigned z = 0u;
    while (((sz3 << z) & 0x8000u) == 0u) {
        z++;
    }
    uint64_t n = (uint64_t)h << z;
    uint64_t d = (uint64_t)sz3 << z;
    uint64_t u = recip_seed((uint32_t)((d - 0x7FC0u) >> 7)) + 0x101u;
    d = (0x2000080u - d * u) >> 8;
    d = (0x80u + d * u) >> 8;
    n = (n * d + 0x8000u) >> 16;
    return n > 0x1FFFFu ? 0x1FFFFu : (uint32_t)n;
}

/* ---- the commands ------------------------------------------------------ */

/* One vertex of the perspective transformation (psx-spx, RTPS). With
   depth_cue set it also does the depth-cueing step, which RTPT runs for its
   last vertex only. */
static void rtp_vertex(struct gte_ref *r, int vi, unsigned shift, int lm, int depth_cue)
{
    const int32_t v[3] = {r->vx[vi], r->vy[vi], r->vz[vi]};
    int64_t sum[4] = {0, 0, 0, 0};
    for (int k = 1; k <= 3; k++) {
        sum[k] = row_sum(r, k, (int64_t)r->tr[k - 1] * 0x1000, r->rt[k - 1], v);
        mac_set(r, k, sum[k], shift);
    }
    ir_from_mac(r, 1, lm);
    ir_from_mac(r, 2, lm);

    /* The Z value is the whole 44-bit sum shifted right by 12, whatever sf is:
       the cases show a sum whose low 32 bits are positive and whose Z is zero.
       The IR3 flag looks at the same number against the range -8000h..7FFFh
       whatever lm is, while the stored IR3 follows lm and the 32-bit MAC3
       (psx-spx, GTE Saturation and RTPS). */
    int64_t z = floor_shr(sum[3], 12);
    if (z < -0x8000 || z > 0x7FFF) {
        r->flag |= F_IR >> 2;
    }
    {
        int32_t lo = lm ? 0 : -0x8000;
        int32_t m = r->mac[3];
        r->ir[3] = m < lo ? lo : (m > 0x7FFF ? 0x7FFF : m);
    }
    r->sz[0] = r->sz[1];
    r->sz[1] = r->sz[2];
    r->sz[2] = r->sz[3];
    if (z < 0) {
        r->sz[3] = 0;
        r->flag |= F_SZ_OTZ;
    } else if (z > 0xFFFF) {
        r->sz[3] = 0xFFFF;
        r->flag |= F_SZ_OTZ;
    } else {
        r->sz[3] = (int32_t)z;
    }

    int64_t n = rtp_divide(r, (uint32_t)r->h, (uint32_t)r->sz[3]);

    int64_t sx = n * r->ir[1] + r->ofx;
    int64_t sy = n * r->ir[2] + r->ofy;
    r->sx[0] = r->sx[1];
    r->sy[0] = r->sy[1];
    r->sx[1] = r->sx[2];
    r->sy[1] = r->sy[2];
    mac0_set(r, sx);
    {
        int64_t q = floor_shr(sx, 16);
        if (q < -0x400 || q > 0x3FF) {
            r->flag |= F_SX;
            q = q < 0 ? -0x400 : 0x3FF;
        }
        r->sx[2] = (int32_t)q;
    }
    mac0_set(r, sy);
    {
        int64_t q = floor_shr(sy, 16);
        if (q < -0x400 || q > 0x3FF) {
            r->flag |= F_SY;
            q = q < 0 ? -0x400 : 0x3FF;
        }
        r->sy[2] = (int32_t)q;
    }
    if (depth_cue) {
        int64_t dq = n * r->dqa + r->dqb;
        mac0_set(r, dq);
        int64_t q = floor_shr(dq, 12);
        if (q < 0 || q > 0x1000) {
            r->flag |= F_IR0;
            q = q < 0 ? 0 : 0x1000;
        }
        r->ir[0] = (int32_t)q;
    }
}

static uint32_t op_rtps(struct gte_ref *r, uint32_t op)
{
    rtp_vertex(r, 0, (op >> 19 & 1u) != 0u ? 12u : 0u, (int)(op >> 10 & 1u), 1);
    return 15u;
}

/* ---- command dispatch --------------------------------------------------- */

uint32_t gte_ref_exec(struct gte_ref *r, uint32_t op)
{
    uint32_t cycles;
    switch (op & 0x3Fu) {
    case 0x01u:
        r->flag = 0;
        cycles = op_rtps(r, op);
        break;
    default:
        return 0u;
    }
    return cycles;
}
