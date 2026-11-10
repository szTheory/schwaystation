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

/* sf is bit 19 of the command word: when set the results keep 12 fraction bits
   less, which is a right shift of 12; lm is bit 10 (psx-spx, GTE Command
   Encoding). */
static unsigned sf_shift(uint32_t op)
{
    return (op >> 19 & 1u) != 0u ? 12u : 0u;
}

static int lm_bit(uint32_t op)
{
    return (int)(op >> 10 & 1u);
}

/* The R, G or B byte of RGBC for k = 1 to 3. */
static int32_t rgbc_part(const struct gte_ref *r, int k)
{
    return k == 1 ? r->rgbc_r : (k == 2 ? r->rgbc_g : r->rgbc_b);
}

/* IR1 to IR3 from MAC1 to MAC3 with the command's lm. */
static void ir_all(struct gte_ref *r, int lm)
{
    for (int k = 1; k <= 3; k++) {
        ir_from_mac(r, k, lm);
    }
}

/* MAC1 to MAC3 from the three accumulators. */
static void mac_all(struct gte_ref *r, const int64_t acc[4], unsigned shift)
{
    for (int k = 1; k <= 3; k++) {
        mac_set(r, k, acc[k], shift);
    }
}

/* The colour FIFO takes MAC1 to MAC3 divided by 16 and saturated to 0 to
   FFh, with the CODE byte of RGBC; the older entries move down one stage; a
   saturation sets flags 21 to 19 (psx-spx, GTE Color Calculation Commands,
   Notes). */
static void color_push(struct gte_ref *r)
{
    int32_t c[3];
    for (int k = 1; k <= 3; k++) {
        int64_t q = floor_shr(r->mac[k], 4);
        if (q < 0) {
            q = 0;
            r->flag |= F_COLOR >> (k - 1);
        } else if (q > 0xFF) {
            q = 0xFF;
            r->flag |= F_COLOR >> (k - 1);
        }
        c[k - 1] = (int32_t)q;
    }
    for (int n = 0; n < 2; n++) {
        r->rgb_r[n] = r->rgb_r[n + 1];
        r->rgb_g[n] = r->rgb_g[n + 1];
        r->rgb_b[n] = r->rgb_b[n + 1];
        r->rgb_c[n] = r->rgb_c[n + 1];
    }
    r->rgb_r[2] = c[0];
    r->rgb_g[2] = c[1];
    r->rgb_b[2] = c[2];
    r->rgb_c[2] = r->rgbc_c;
}

/* The end of every colour command: the FIFO push, then IR1 to IR3 from the
   MACs with the command's lm. */
static void color_finish(struct gte_ref *r, int lm)
{
    color_push(r);
    ir_all(r, lm);
}

/* MAC + (FC - MAC) * IR0 on the unshifted accumulators (psx-spx, GTE Color
   Calculation Commands, Details on "MAC+(FC-MAC)*IR0"). The difference is
   shifted by sf and saturated into IR as if lm were 0, with its flag; the
   saturated IR times IR0 is added to the accumulator. */
static void far_blend(struct gte_ref *r, int64_t acc[4], unsigned shift)
{
    for (int k = 1; k <= 3; k++) {
        int64_t d = acc_step(r, k, (int64_t)r->fc[k - 1] * 0x1000 - acc[k]);
        r->mac[k] = low32(floor_shr(d, shift));
        ir_from_mac(r, k, 0);
        acc[k] = acc_step(r, k, (int64_t)r->ir[k] * r->ir[0] + acc[k]);
    }
}

/* MAC = (LLM * vector) shifted by sf, then IR = MAC (psx-spx, Details on
   "(LLM*V0) SAR (sf*12)"). */
static void light_step(struct gte_ref *r, int vi, unsigned shift, int lm)
{
    const int32_t v[3] = {r->vx[vi], r->vy[vi], r->vz[vi]};
    for (int k = 1; k <= 3; k++) {
        mac_set(r, k, row_sum(r, k, 0, r->ll[k - 1], v), shift);
    }
    ir_all(r, lm);
}

/* MAC = (BK * 1000h + LCM * IR) shifted by sf, then IR = MAC. */
static void tint_step(struct gte_ref *r, unsigned shift, int lm)
{
    const int32_t v[3] = {r->ir[1], r->ir[2], r->ir[3]};
    for (int k = 1; k <= 3; k++) {
        mac_set(r, k, row_sum(r, k, (int64_t)r->bk[k - 1] * 0x1000, r->lc[k - 1], v), shift);
    }
    ir_all(r, lm);
}

static uint32_t op_rtps(struct gte_ref *r, uint32_t op)
{
    rtp_vertex(r, 0, sf_shift(op), lm_bit(op), 1);
    return 15u;
}

static uint32_t op_rtpt(struct gte_ref *r, uint32_t op)
{
    for (int vi = 0; vi < 3; vi++) {
        rtp_vertex(r, vi, sf_shift(op), lm_bit(op), vi == 2);
    }
    return 23u;
}

/* MAC0 = SX0*SY1 + SX1*SY2 + SX2*SY0 - SX0*SY2 - SX1*SY0 - SX2*SY1 (psx-spx,
   NCLIP). */
static uint32_t op_nclip(struct gte_ref *r, uint32_t op)
{
    (void)op;
    int64_t s = (int64_t)r->sx[0] * r->sy[1] + (int64_t)r->sx[1] * r->sy[2] + (int64_t)r->sx[2] * r->sy[0] -
                (int64_t)r->sx[0] * r->sy[2] - (int64_t)r->sx[1] * r->sy[0] - (int64_t)r->sx[2] * r->sy[1];
    mac0_set(r, s);
    return 8u;
}

/* Cross product of IR with the diagonal of the rotation matrix (psx-spx, OP). */
static uint32_t op_op(struct gte_ref *r, uint32_t op)
{
    unsigned shift = sf_shift(op);
    int64_t i1 = r->ir[1], i2 = r->ir[2], i3 = r->ir[3];
    int64_t d1 = r->rt[0][0], d2 = r->rt[1][1], d3 = r->rt[2][2];
    mac_set(r, 1, i3 * d2 - i2 * d3, shift);
    mac_set(r, 2, i1 * d3 - i3 * d1, shift);
    mac_set(r, 3, i2 * d1 - i1 * d2, shift);
    ir_all(r, lm_bit(op));
    return 6u;
}

/* Depth cueing of the RGB bytes shifted up 16, towards the far colour. The
   triple form reads the bottom of the colour FIFO each time and so rewrites
   the whole FIFO (psx-spx, DPCS and DPCT). */
static void dpc_once(struct gte_ref *r, uint32_t op, int from_fifo)
{
    unsigned shift = sf_shift(op);
    int64_t acc[4] = {0, 0, 0, 0};
    for (int k = 1; k <= 3; k++) {
        int32_t c = from_fifo ? (k == 1 ? r->rgb_r[0] : (k == 2 ? r->rgb_g[0] : r->rgb_b[0])) : rgbc_part(r, k);
        acc[k] = (int64_t)c * 0x10000;
    }
    far_blend(r, acc, shift);
    mac_all(r, acc, shift);
    color_finish(r, lm_bit(op));
}

static uint32_t op_dpcs(struct gte_ref *r, uint32_t op)
{
    dpc_once(r, op, 0);
    return 8u;
}

static uint32_t op_dpct(struct gte_ref *r, uint32_t op)
{
    for (int n = 0; n < 3; n++) {
        dpc_once(r, op, 1);
    }
    return 17u;
}

/* Interpolation of IR (shifted up 12) towards the far colour. */
static uint32_t op_intpl(struct gte_ref *r, uint32_t op)
{
    unsigned shift = sf_shift(op);
    int64_t acc[4] = {0, 0, 0, 0};
    for (int k = 1; k <= 3; k++) {
        acc[k] = (int64_t)r->ir[k] * 0x1000;
    }
    far_blend(r, acc, shift);
    mac_all(r, acc, shift);
    color_finish(r, lm_bit(op));
    return 8u;
}

/* Multiply a vector by a matrix and add a vector (psx-spx, MVMVA). The matrix
   field 3 is a made-up matrix, and the far-colour translation (field 2) keeps
   only the last two products of each row in MAC and IR, while its first
   product still sets the flags as the whole sum would. */
static uint32_t op_mvmva(struct gte_ref *r, uint32_t op)
{
    unsigned shift = sf_shift(op);
    int lm = lm_bit(op);
    uint32_t mx = op >> 17 & 3u, vv = op >> 15 & 3u, cv = op >> 13 & 3u;
    int32_t m[3][3];
    int32_t v[3];
    int32_t t[3] = {0, 0, 0};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (mx == 0u) {
                m[i][j] = r->rt[i][j];
            } else if (mx == 1u) {
                m[i][j] = r->ll[i][j];
            } else if (mx == 2u) {
                m[i][j] = r->lc[i][j];
            } else if (i == 0) {
                m[i][j] = j == 0 ? -(r->rgbc_r * 16) : (j == 1 ? r->rgbc_r * 16 : r->ir[0]);
            } else {
                m[i][j] = i == 1 ? r->rt[0][2] : r->rt[1][1];
            }
        }
    }
    if (vv < 3u) {
        v[0] = r->vx[vv];
        v[1] = r->vy[vv];
        v[2] = r->vz[vv];
    } else {
        v[0] = r->ir[1];
        v[1] = r->ir[2];
        v[2] = r->ir[3];
    }
    for (int i = 0; i < 3; i++) {
        if (cv == 0u) {
            t[i] = r->tr[i];
        } else if (cv == 1u) {
            t[i] = r->bk[i];
        } else if (cv == 2u) {
            t[i] = r->fc[i];
        }
    }
    for (int k = 1; k <= 3; k++) {
        int64_t sum;
        if (cv != 2u) {
            sum = row_sum(r, k, (int64_t)t[k - 1] * 0x1000, m[k - 1], v);
        } else {
            int64_t first = acc_step(r, k, (int64_t)t[k - 1] * 0x1000 + (int64_t)m[k - 1][0] * v[0]);
            int32_t q = low32(floor_shr(first, shift));
            if (q < -0x8000 || q > 0x7FFF) {
                r->flag |= F_IR >> (k - 1);
            }
            sum = acc_step(r, k, (int64_t)m[k - 1][1] * v[1] + (int64_t)m[k - 1][2] * v[2]);
        }
        mac_set(r, k, sum, shift);
    }
    ir_all(r, lm);
    return 8u;
}

/* The three accumulators for a colour: the RGBC byte times IR, shifted up 4
   (psx-spx, GTE Color Calculation Commands). */
static void tint_product(const struct gte_ref *r, int64_t acc[4])
{
    for (int k = 1; k <= 3; k++) {
        acc[k] = (int64_t)rgbc_part(r, k) * r->ir[k] * 16;
    }
}

/* Light, tint, colour: one vertex of NCCS (and each of NCCT's three). */
static void nccs_vertex(struct gte_ref *r, uint32_t op, int vi)
{
    unsigned shift = sf_shift(op);
    int64_t acc[4] = {0, 0, 0, 0};
    light_step(r, vi, shift, lm_bit(op));
    tint_step(r, shift, lm_bit(op));
    tint_product(r, acc);
    mac_all(r, acc, shift);
    color_finish(r, lm_bit(op));
}

/* The same with the far colour blended in: one vertex of NCDS and NCDT. */
static void ncds_vertex(struct gte_ref *r, uint32_t op, int vi)
{
    unsigned shift = sf_shift(op);
    int64_t acc[4] = {0, 0, 0, 0};
    light_step(r, vi, shift, lm_bit(op));
    tint_step(r, shift, lm_bit(op));
    tint_product(r, acc);
    far_blend(r, acc, shift);
    mac_all(r, acc, shift);
    color_finish(r, lm_bit(op));
}

/* Light and tint only: one vertex of NCS and NCT. */
static void ncs_vertex(struct gte_ref *r, uint32_t op, int vi)
{
    light_step(r, vi, sf_shift(op), lm_bit(op));
    tint_step(r, sf_shift(op), lm_bit(op));
    color_finish(r, lm_bit(op));
}

static uint32_t op_ncds(struct gte_ref *r, uint32_t op)
{
    ncds_vertex(r, op, 0);
    return 19u;
}

static uint32_t op_ncdt(struct gte_ref *r, uint32_t op)
{
    for (int vi = 0; vi < 3; vi++) {
        ncds_vertex(r, op, vi);
    }
    return 44u;
}

static uint32_t op_nccs(struct gte_ref *r, uint32_t op)
{
    nccs_vertex(r, op, 0);
    return 17u;
}

static uint32_t op_ncct(struct gte_ref *r, uint32_t op)
{
    for (int vi = 0; vi < 3; vi++) {
        nccs_vertex(r, op, vi);
    }
    return 39u;
}

static uint32_t op_ncs(struct gte_ref *r, uint32_t op)
{
    ncs_vertex(r, op, 0);
    return 14u;
}

static uint32_t op_nct(struct gte_ref *r, uint32_t op)
{
    for (int vi = 0; vi < 3; vi++) {
        ncs_vertex(r, op, vi);
    }
    return 30u;
}

/* Colour * IR from the IR vector already in place: tint, then the product. */
static uint32_t op_cc(struct gte_ref *r, uint32_t op)
{
    unsigned shift = sf_shift(op);
    int64_t acc[4] = {0, 0, 0, 0};
    tint_step(r, shift, lm_bit(op));
    tint_product(r, acc);
    mac_all(r, acc, shift);
    color_finish(r, lm_bit(op));
    return 11u;
}

/* The same with the far colour blended in. */
static uint32_t op_cdp(struct gte_ref *r, uint32_t op)
{
    unsigned shift = sf_shift(op);
    int64_t acc[4] = {0, 0, 0, 0};
    tint_step(r, shift, lm_bit(op));
    tint_product(r, acc);
    far_blend(r, acc, shift);
    mac_all(r, acc, shift);
    color_finish(r, lm_bit(op));
    return 13u;
}

/* The depth-cue colour light: the RGBC byte times IR, shifted up 4, towards
   the far colour. */
static uint32_t op_dcpl(struct gte_ref *r, uint32_t op)
{
    unsigned shift = sf_shift(op);
    int64_t acc[4] = {0, 0, 0, 0};
    tint_product(r, acc);
    far_blend(r, acc, shift);
    mac_all(r, acc, shift);
    color_finish(r, lm_bit(op));
    return 8u;
}

/* The square of each IR, shifted by sf. */
static uint32_t op_sqr(struct gte_ref *r, uint32_t op)
{
    for (int k = 1; k <= 3; k++) {
        mac_set(r, k, (int64_t)r->ir[k] * r->ir[k], sf_shift(op));
    }
    ir_all(r, lm_bit(op));
    return 5u;
}

/* The average of three or four Z values times a scale factor; OTZ is the sum
   divided by 1000h, saturated to 0 to FFFFh (psx-spx, AVSZ3 and AVSZ4). */
static void avsz_finish(struct gte_ref *r, int64_t s)
{
    mac0_set(r, s);
    int64_t q = floor_shr(s, 12);
    if (q < 0 || q > 0xFFFF) {
        r->flag |= F_SZ_OTZ;
        q = q < 0 ? 0 : 0xFFFF;
    }
    r->otz = (int32_t)q;
}

static uint32_t op_avsz3(struct gte_ref *r, uint32_t op)
{
    (void)op;
    avsz_finish(r, (int64_t)r->zsf3 * ((int64_t)r->sz[1] + r->sz[2] + r->sz[3]));
    return 5u;
}

static uint32_t op_avsz4(struct gte_ref *r, uint32_t op)
{
    (void)op;
    avsz_finish(r, (int64_t)r->zsf4 * ((int64_t)r->sz[0] + r->sz[1] + r->sz[2] + r->sz[3]));
    return 6u;
}

/* IR times IR0, from zero (GPF) or added to the old MAC shifted up by sf
   (GPL), then shifted down by sf; the result goes to the colour FIFO. */
static uint32_t op_gpf(struct gte_ref *r, uint32_t op)
{
    unsigned shift = sf_shift(op);
    int64_t acc[4] = {0, 0, 0, 0};
    for (int k = 1; k <= 3; k++) {
        acc[k] = (int64_t)r->ir[k] * r->ir[0];
    }
    mac_all(r, acc, shift);
    color_finish(r, lm_bit(op));
    return 5u;
}

static uint32_t op_gpl(struct gte_ref *r, uint32_t op)
{
    unsigned shift = sf_shift(op);
    int64_t scale = (int64_t)1 << shift;
    int64_t acc[4] = {0, 0, 0, 0};
    for (int k = 1; k <= 3; k++) {
        acc[k] = acc_step(r, k, (int64_t)r->mac[k] * scale + (int64_t)r->ir[k] * r->ir[0]);
    }
    mac_all(r, acc, shift);
    color_finish(r, lm_bit(op));
    return 5u;
}

/* ---- command dispatch --------------------------------------------------- */

/* Every listed command starts by clearing FLAG (psx-spx, GTE Saturation); a
   number the description does not list is left alone. */
uint32_t gte_ref_exec(struct gte_ref *r, uint32_t op)
{
    uint32_t n = op & 0x3Fu;
    if (n != 0x01u && n != 0x06u && n != 0x0Cu && n != 0x10u && n != 0x11u && n != 0x12u && n != 0x13u &&
        n != 0x14u && n != 0x16u && n != 0x1Bu && n != 0x1Cu && n != 0x1Eu && n != 0x20u && n != 0x28u &&
        n != 0x29u && n != 0x2Au && n != 0x2Du && n != 0x2Eu && n != 0x30u && n != 0x3Du && n != 0x3Eu &&
        n != 0x3Fu) {
        return 0u;
    }
    r->flag = 0;
    switch (n) {
    case 0x01u:
        return op_rtps(r, op);
    case 0x06u:
        return op_nclip(r, op);
    case 0x0Cu:
        return op_op(r, op);
    case 0x10u:
        return op_dpcs(r, op);
    case 0x11u:
        return op_intpl(r, op);
    case 0x12u:
        return op_mvmva(r, op);
    case 0x13u:
        return op_ncds(r, op);
    case 0x14u:
        return op_cdp(r, op);
    case 0x16u:
        return op_ncdt(r, op);
    case 0x1Bu:
        return op_nccs(r, op);
    case 0x1Cu:
        return op_cc(r, op);
    case 0x1Eu:
        return op_ncs(r, op);
    case 0x20u:
        return op_nct(r, op);
    case 0x28u:
        return op_sqr(r, op);
    case 0x29u:
        return op_dcpl(r, op);
    case 0x2Au:
        return op_dpct(r, op);
    case 0x2Du:
        return op_avsz3(r, op);
    case 0x2Eu:
        return op_avsz4(r, op);
    case 0x30u:
        return op_rtpt(r, op);
    case 0x3Du:
        return op_gpf(r, op);
    case 0x3Eu:
        return op_gpl(r, op);
    default: /* 0x3F */
        return op_ncct(r, op);
    }
}
