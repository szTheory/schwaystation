/* The geometry engine's tests; one case per command-line argument.
     schwaystation-test-gte vectors FILE --expect N [--ops LIST] [--histogram] [--flip N]
     schwaystation-test-gte regs_sweep
     schwaystation-test-gte unr_table
     schwaystation-test-gte nclip_fake_bits
     schwaystation-test-gte quirk_fc | quirk_mx3 | quirk_ir3_lm | quirk_h_sign |
                            quirk_clip_end | quirk_rtpt_ir0
     schwaystation-test-gte unlisted_noop
   vectors replays the hardware cases of FILE (vectors.h) through the engine's
   register and command entry points. LIST holds hex command numbers and "reg"
   for the register-only cases; only those cases run. It prints the number of
   cases run and the first register that differs in each failing case. Exit 0
   when exactly N cases ran and all matched, 1 for a failure or another count,
   2 for a bad command line. --histogram also requires the file to hold 50
   register-only cases and 50 for each of the 22 documented command numbers.
   --flip N inverts one bit of the expected output of case N (counted from 1)
   in memory, so that case must fail. regs_sweep drives every register and every command
   with edge values and checks that reads repeat; run under the sanitizers, it
   also proves the engine touches nothing outside its struct. unr_table
   regenerates the divide's reciprocal table from its formula and compares all
   257 entries. nclip_fake_bits checks that the unused command bits 20 to 24
   change nothing in NCLIP. Each quirk case loads a few registers, runs one
   command and requires the listed registers to hold the values worked out by
   hand in its comment and every other register to be unchanged. unlisted_noop
   runs every command number outside the 22 documented ones and requires all 64
   registers to be unchanged. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../test_open.h"

#include "schwaystation.h"
#include "gte.h"
#include "ref.h"
#include "vectors.h"

static void rd_write(void *ctx, uint32_t idx, uint32_t v)
{
    schway__gte_write((struct schway__gte *)ctx, idx, v);
}

static void rd_exec(void *ctx, uint32_t op)
{
    (void)schway__gte_exec((struct schway__gte *)ctx, op);
}

static uint32_t rd_read(void *ctx, uint32_t idx)
{
    return schway__gte_read((const struct schway__gte *)ctx, idx);
}

/* Parses LIST into a mask of command numbers and the register-only flag. */
static int parse_ops(const char *list, uint64_t *mask, int *reg)
{
    *mask = 0u;
    *reg = 0;
    while (*list != '\0') {
        if (strncmp(list, "reg", 3) == 0) {
            *reg = 1;
            list += 3;
        } else {
            char *end;
            unsigned long n = strtoul(list, &end, 16);
            if (end == list || n > 0x3Fu) {
                return -1;
            }
            *mask |= (uint64_t)1u << n;
            list = end;
        }
        if (*list == ',') {
            list++;
        } else if (*list != '\0') {
            return -1;
        }
    }
    return 0;
}

static uint8_t *read_file(const char *path, size_t *len)
{
    FILE *f = schway__test_open_file(path, "rb");
    if (f == NULL) {
        return NULL;
    }
    uint8_t *buf = NULL;
    size_t n = 0u, cap = 0u;
    for (;;) {
        if (n == cap) {
            cap = cap == 0u ? 65536u : cap * 2u;
            uint8_t *p = realloc(buf, cap);
            if (p == NULL) {
                free(buf);
                (void)fclose(f);
                return NULL;
            }
            buf = p;
        }
        size_t got = fread(buf + n, 1u, cap - n, f);
        n += got;
        if (got == 0u) {
            break;
        }
    }
    (void)fclose(f);
    *len = n;
    return buf;
}

/* The documented command numbers: each appears in 50 cases of the file, and
   50 cases are register-only (a count of the file's cases, not of a run). */
static int histogram_ok(const uint8_t *data, long total)
{
    static const uint8_t ops[22] = {0x01, 0x06, 0x0C, 0x10, 0x11, 0x12, 0x13, 0x14, 0x16, 0x1B, 0x1C,
                                    0x1E, 0x20, 0x28, 0x29, 0x2A, 0x2D, 0x2E, 0x30, 0x3D, 0x3E, 0x3F};
    long count[64] = {0};
    long reg_only = 0;
    for (long n = 0; n < total; n++) {
        struct gtev_case c;
        gtev_load(data, (uint32_t)n, &c);
        if (c.opcode == GTEV_REG_ONLY) {
            reg_only++;
        } else {
            count[c.opcode & 0x3Fu]++;
        }
    }
    int ok = reg_only == 50 && total == 50 * 23;
    if (reg_only != 50) {
        fprintf(stderr, "gte vectors: %ld register-only cases, expected 50\n", reg_only);
    }
    for (unsigned k = 0u; k < 22u; k++) {
        if (count[ops[k]] != 50) {
            fprintf(stderr, "gte vectors: command %02X in %ld cases, expected 50\n", (unsigned)ops[k],
                    count[ops[k]]);
            ok = 0;
        }
    }
    if (total != 50 * 23) {
        fprintf(stderr, "gte vectors: %ld cases in the file, expected %d\n", total, 50 * 23);
    }
    return ok;
}

static int run_vectors(int argc, char **argv)
{
    const char *file = NULL;
    long expect = -1;
    uint64_t mask = 0u;
    int reg = 0, have_ops = 0, histogram = 0;
    long flip = 0;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--expect") == 0 && i + 1 < argc) {
            expect = strtol(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--ops") == 0 && i + 1 < argc) {
            have_ops = 1;
            if (parse_ops(argv[++i], &mask, &reg) != 0) {
                fprintf(stderr, "gte vectors: bad --ops list\n");
                return 2;
            }
        } else if (strcmp(argv[i], "--histogram") == 0) {
            histogram = 1;
        } else if (strcmp(argv[i], "--flip") == 0 && i + 1 < argc) {
            flip = strtol(argv[++i], NULL, 10);
        } else if (file == NULL && argv[i][0] != '-') {
            file = argv[i];
        } else {
            fprintf(stderr, "gte vectors: bad argument %s\n", argv[i]);
            return 2;
        }
    }
    if (file == NULL || expect < 0) {
        fprintf(stderr, "usage: schwaystation-test-gte vectors FILE --expect N [--ops LIST] "
                        "[--histogram] [--flip N]\n");
        return 2;
    }
    size_t len = 0u;
    uint8_t *data = read_file(file, &len);
    long total = data == NULL ? -1 : gtev_count(data, len);
    if (total < 0) {
        fprintf(stderr, "gte vectors: %s is unreadable or truncated\n", file);
        free(data);
        return 1;
    }
    if (histogram && !histogram_ok(data, total)) {
        free(data);
        return 1;
    }
    struct schway__gte *g = malloc(sizeof *g);
    if (g == NULL) {
        free(data);
        return 1;
    }
    long ran = 0, failed = 0;
    for (long n = 0; n < total; n++) {
        struct gtev_case c;
        gtev_load(data, (uint32_t)n, &c);
        if (n + 1 == flip) {
            c.out[0] ^= 1u;
        }
        if (have_ops) {
            int wanted = c.opcode == GTEV_REG_ONLY ? reg : (int)((mask >> (c.opcode & 0x3Fu)) & 1u);
            if (!wanted) {
                continue;
            }
        }
        ran++;
        uint32_t got = 0u, want = 0u;
        schway__gte_reset(g);
        int bad = gtev_replay(&c, rd_write, rd_exec, rd_read, g, &got, &want);
        if (bad >= 0) {
            failed++;
            fprintf(stderr, "gte vectors: case %ld command %06lX register %d: got %08lX want %08lX\n",
                    n + 1, c.opcode == GTEV_REG_ONLY ? 0xFFFFFFL : (long)c.opcode, bad,
                    (unsigned long)got, (unsigned long)want);
        }
    }
    free(g);
    free(data);
    printf("gte vectors: %ld cases, %ld failures\n", ran, failed);
    if (ran != expect) {
        fprintf(stderr, "gte vectors: ran %ld cases, expected %ld\n", ran, expect);
        return 1;
    }
    return failed != 0 ? 1 : 0;
}

/* Every register at edge values, then every command number with sf and lm both
   ways on registers filled with each edge value. Reads must repeat, and a
   command must not claim more cycles than the longest one. */
static int run_regs_sweep(void)
{
    static const uint32_t values[7] = {0u,          0x7FFFu,     0x8000u,    0xFFFFu,
                                       0x7FFFFFFFu, 0x80000000u, 0xFFFFFFFFu};
    struct schway__gte *g = malloc(sizeof *g);
    if (g == NULL) {
        return 1;
    }
    int bad = 0;
    for (uint32_t idx = 0u; idx < 64u; idx++) {
        for (uint32_t k = 0u; k < 7u; k++) {
            schway__gte_reset(g);
            schway__gte_write(g, idx, values[k]);
            uint32_t a = schway__gte_read(g, idx);
            uint32_t b = schway__gte_read(g, idx);
            if (a != b) {
                fprintf(stderr, "regs_sweep: register %u reads %08lX then %08lX\n", (unsigned)idx,
                        (unsigned long)a, (unsigned long)b);
                bad = 1;
            }
        }
    }
    for (uint32_t cmd = 0u; cmd < 64u; cmd++) {
        for (uint32_t flags = 0u; flags < 4u; flags++) {
            uint32_t op = cmd | (flags & 1u) << 19 | (flags >> 1) << 10;
            for (uint32_t k = 0u; k < 7u; k++) {
                schway__gte_reset(g);
                for (uint32_t idx = 0u; idx < 64u; idx++) {
                    schway__gte_write(g, idx, values[k]);
                }
                uint32_t cycles = schway__gte_exec(g, op);
                if (cycles > SCHWAY__GTE_STALL_MAX) {
                    fprintf(stderr, "regs_sweep: command %02lX takes %lu cycles\n",
                            (unsigned long)cmd, (unsigned long)cycles);
                    bad = 1;
                }
                for (uint32_t idx = 0u; idx < 64u; idx++) {
                    if (schway__gte_read(g, idx) != schway__gte_read(g, idx)) {
                        fprintf(stderr, "regs_sweep: register %u after command %02lX differs\n",
                                (unsigned)idx, (unsigned long)cmd);
                        bad = 1;
                    }
                }
            }
        }
    }
    free(g);
    return bad;
}

/* The divide's reciprocal table from its formula, in integers (psx-spx, GTE
   Division Inaccuracy): entry i is max(0, (40000h / (i + 100h) + 1) / 2 -
   101h), which is 0 for the last entry. */
static int run_unr_table(void)
{
    int bad = 0;
    for (uint32_t i = 0u; i < 257u; i++) {
        int32_t want = (int32_t)((0x40000u / (i + 0x100u) + 1u) / 2u) - 0x101;
        if (want < 0) {
            want = 0;
        }
        if (schway__gte_unr_entry(i) != (uint32_t)want) {
            fprintf(stderr, "unr_table: entry %lu is %02lX, the formula gives %02lX\n",
                    (unsigned long)i, (unsigned long)schway__gte_unr_entry(i), (unsigned long)want);
            bad = 1;
        }
    }
    if (schway__gte_unr_entry(257u) != 0u) {
        fprintf(stderr, "unr_table: an index past the end is not 0\n");
        bad = 1;
    }
    return bad;
}

/* NCLIP issued as 0140006h (the fake number and bit 18 set, as a game does)
   gives the same 64 outputs and cycles as 6, on triangles of both windings and
   on registers that hold stale flags and results. */
static int run_nclip_fake_bits(void)
{
    static const int32_t tri[3][6] = {
        {0, 0, 10, 0, 0, 10},
        {0, 0, 0, 10, 10, 0},
        {-1024, 1023, 1023, -1024, 32767, -32768},
    };
    static const uint32_t ops[4] = {0x0140006u, 0x1F00006u, 0x0000B46u, 0x0F80006u};
    struct schway__gte *a = malloc(sizeof *a);
    struct schway__gte *b = malloc(sizeof *b);
    if (a == NULL || b == NULL) {
        free(a);
        free(b);
        return 1;
    }
    int bad = 0;
    for (uint32_t t = 0u; t < 3u; t++) {
        for (uint32_t k = 0u; k < 4u; k++) {
            struct schway__gte *s[2] = {a, b};
            for (uint32_t w = 0u; w < 2u; w++) {
                schway__gte_reset(s[w]);
                for (uint32_t i = 0u; i < 3u; i++) {
                    schway__gte_write(s[w], 12u + i,
                                      ((uint32_t)tri[t][2u * i + 1u] << 16) |
                                          ((uint32_t)tri[t][2u * i] & 0xFFFFu));
                }
                schway__gte_write(s[w], 24u, 0x12345678u);
                schway__gte_write(s[w], 63u, 0x7FFFF000u);
            }
            uint32_t ca = schway__gte_exec(a, 6u);
            uint32_t cb = schway__gte_exec(b, ops[k]);
            if (ca != cb) {
                fprintf(stderr, "nclip_fake_bits: %06lX takes %lu cycles, 6 takes %lu\n",
                        (unsigned long)ops[k], (unsigned long)cb, (unsigned long)ca);
                bad = 1;
            }
            for (uint32_t i = 0u; i < 64u; i++) {
                if (schway__gte_read(a, i) != schway__gte_read(b, i)) {
                    fprintf(stderr, "nclip_fake_bits: %06lX register %lu is %08lX, 6 gives %08lX\n",
                            (unsigned long)ops[k], (unsigned long)i,
                            (unsigned long)schway__gte_read(b, i),
                            (unsigned long)schway__gte_read(a, i));
                    bad = 1;
                }
            }
            if (t == 0u && schway__gte_read(b, 24u) != 100u) {
                fprintf(stderr, "nclip_fake_bits: the area of the first triangle is not 100\n");
                bad = 1;
            }
            if (t == 1u && schway__gte_read(b, 24u) != 0xFFFFFF9Cu) {
                fprintf(stderr, "nclip_fake_bits: the area of the second triangle is not -100\n");
                bad = 1;
            }
        }
    }
    free(a);
    free(b);
    return bad;
}

/* A register index (0 to 31 data, 32 to 63 control) and a value. */
struct rv {
    uint32_t idx;
    uint32_t v;
};

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

/* Loads in into a cleared engine, runs op, and requires each register of want
   to read its value and every other register to read what it did before the
   command. Returns 0 when all match. */
static int quirk_run(const char *name, const struct rv *in, size_t nin, uint32_t op,
                     const struct rv *want, size_t nwant)
{
    struct schway__gte *g = malloc(sizeof *g);
    if (g == NULL) {
        return 1;
    }
    uint32_t expect[64];
    schway__gte_reset(g);
    for (size_t i = 0u; i < nin; i++) {
        schway__gte_write(g, in[i].idx, in[i].v);
    }
    for (uint32_t i = 0u; i < 64u; i++) {
        expect[i] = schway__gte_read(g, i);
    }
    for (size_t i = 0u; i < nwant; i++) {
        expect[want[i].idx] = want[i].v;
    }
    (void)schway__gte_exec(g, op);
    int bad = 0;
    for (uint32_t i = 0u; i < 64u; i++) {
        uint32_t got = schway__gte_read(g, i);
        if (got != expect[i]) {
            fprintf(stderr, "%s: command %06lX register %lu is %08lX, expected %08lX\n", name,
                    (unsigned long)op, (unsigned long)i, (unsigned long)got,
                    (unsigned long)expect[i]);
            bad = 1;
        }
    }
    free(g);
    return bad;
}

/* MVMVA with the far colour as translation (rotation matrix, V0, sf = 1,
   command 084012h). The matrix row 1 is RT11 = RT12 = 1000h (1.0), the others
   are 0, V0 = (100, 200, 0), FC = (50000, 5, -7). Done in full, MAC1 would be
   50000 + 100 + 200 = 50300 and IR1 would saturate. The hardware drops the
   translation and the first product from the result (psx-spx, Geometry
   Transformation Engine, MVMVA with the far colour): MAC1 = (1000h * 200) / 1000h
   = 200, MAC2 = MAC3 = 0, IR follows MAC. Its flags are those of the full
   first term: FC.x + 100 = 50100 saturates IR1, flag bit 24, and bit 31 with
   it; FC.y + 0 = 5 and FC.z + 0 = -7 saturate nothing. IRGB and ORGB read IR1
   = 200 as 200 / 80h = 1. */
static int run_quirk_fc(void)
{
    static const struct rv in[] = {{0u, 0x00C80064u},  {32u, 0x10001000u}, {53u, 50000u},
                                   {54u, 5u},          {55u, 0xFFFFFFF9u}};
    static const struct rv want[] = {{9u, 200u},  {25u, 200u}, {28u, 1u},
                                     {29u, 1u},   {63u, 0x81000000u}};
    return quirk_run("quirk_fc", in, COUNT(in), 0x84012u, want, COUNT(want));
}

/* MVMVA with matrix 3, the reserved one (command 066012h: sf = 0, lm = 0, no
   translation, V0). psx-spx gives it as rows (-R*10h, R*10h, IR0), (RT13, RT13,
   RT13), (RT22, RT22, RT22), with R the red byte of RGBC. R = 20h so R*10h =
   512, IR0 = 7, RT13 = 3, RT22 = 5, V0 = (10, 20, 30); RT11, RT12 and RT21 are
   set to other values and must not matter. Row 1: -512*10 + 512*20 + 7*30 =
   5330. Row 2: 3 * (10 + 20 + 30) = 180. Row 3: 5 * 60 = 300. IR equals MAC.
   ORGB reads 5330 / 80h = 41, limited to 31, 180 / 80h = 1 and 300 / 80h = 2,
   which is 31 + 1*20h + 2*400h = 83Fh. */
static int run_quirk_mx3(void)
{
    static const struct rv in[] = {{0u, 0x0014000Au}, {1u, 30u},          {6u, 0x20u},
                                   {8u, 7u},          {32u, 0x11111111u}, {33u, 0x07770003u},
                                   {34u, 5u}};
    static const struct rv want[] = {{9u, 5330u},  {10u, 180u},   {11u, 300u},  {25u, 5330u},
                                     {26u, 180u},  {27u, 300u},   {28u, 0x83Fu}, {29u, 0x83Fu}};
    return quirk_run("quirk_mx3", in, COUNT(in), 0x66012u, want, COUNT(want));
}

/* RTPS (sf = 1) with RT11 = RT33 = 1000h, V0 = (-5, 0, -5), so MAC1 = MAC3 =
   -5, and the rest zero. With lm = 1 IR1 saturates to 0 and sets flag bit 24;
   IR3 also becomes 0 but, as psx-spx says (GTE RTPS and RTPT), its flag is
   judged against the bound lm = 0 would use, -8000h, which -5 is inside, so
   bit 22 stays clear. The depth -5 clamps SZ3 to 0 (bit 18) and a zero depth
   overflows the divide (bit 17): FLAG = 01060000h | 80000000h. The divide gives
   1FFFFh, and 1FFFFh * 0 into SXY2 is 0. With lm = 0 IR1 and IR3 hold -5, bit 24
   is clear (FLAG = 80060000h) and SX2 = (1FFFFh * -5) / 10000h = -10 rounded
   down: -655355 / 65536 is -9.99998, floor -10, which is FFF6h. */
static int run_quirk_ir3_lm(void)
{
    static const struct rv in[] = {{0u, 0x0000FFFBu}, {1u, 0xFFFFFFFBu}, {32u, 0x1000u},
                                   {36u, 0x1000u}};
    static const struct rv want1[] = {{25u, 0xFFFFFFFBu}, {27u, 0xFFFFFFFBu}, {63u, 0x81060000u}};
    static const struct rv want0[] = {{9u, 0xFFFFFFFBu},  {11u, 0xFFFFFFFBu}, {14u, 0x0000FFF6u},
                                      {15u, 0x0000FFF6u}, {25u, 0xFFFFFFFBu}, {27u, 0xFFFFFFFBu},
                                      {63u, 0x80060000u}};
    int bad = quirk_run("quirk_ir3_lm(lm=1)", in, COUNT(in), 0x80401u, want1, COUNT(want1));
    bad |= quirk_run("quirk_ir3_lm(lm=0)", in, COUNT(in), 0x80001u, want0, COUNT(want0));
    return bad;
}

/* H = 8000h: the register reads back FFFF8000h (a 16-bit value read as signed)
   but the divide takes it as 32768. RTPS (sf = 1) with V0 = (100, -50, 4000h),
   RT11 = RT22 = 1000h, RT33 = 2000h, so MAC1 = 100, MAC2 = -50 and MAC3 =
   2000h * 4000h / 1000h = 8000h, and SZ3 = 8000h. H / SZ3 = 1.0, which is 10000h
   in the 17-bit quotient (psx-spx, GTE Division Inaccuracy: a divisor of 8000h
   has the reciprocal 20000h and the quotient is 8000h * 20000h / 10000h). With
   OFX = OFY = 0, SX2 = 10000h * 100 / 10000h = 100 and SY2 = -50. IR3 would be
   8000h, over the limit: it becomes 7FFFh and sets flag bit 22 (not an error
   bit, so FLAG is 00400000h). A sign-extended H would overflow the divide and
   saturate SX2 and SY2. IRGB and ORGB read IR3 = 7FFFh as 31 in the blue field. */
static int run_quirk_h_sign(void)
{
    static const struct rv in[] = {{0u, 0xFFCE0064u}, {1u, 0x4000u},     {32u, 0x1000u},
                                   {34u, 0x1000u},    {36u, 0x2000u},    {58u, 0x8000u}};
    static const struct rv want[] = {{9u, 100u},         {10u, 0xFFFFFFCEu}, {11u, 0x7FFFu},
                                     {14u, 0xFFCE0064u}, {15u, 0xFFCE0064u}, {19u, 0x8000u},
                                     {25u, 100u},        {26u, 0xFFFFFFCEu}, {27u, 0x8000u},
                                     {28u, 0x7C00u},     {29u, 0x7C00u},     {58u, 0xFFFF8000u},
                                     {63u, 0x00400000u}};
    return quirk_run("quirk_h_sign", in, COUNT(in), 0x80001u, want, COUNT(want));
}

/* MVMVA (command 006012h: rotation, V0, no translation, sf = 0). Row 1 is
   7FFFh * 7FFFh + 8001h (-7FFFh) * 7FFEh, whose two products are 3FFF0001h and
   -3FFE8002h: each is far outside IR's range, but their sum is 7FFFh, which
   fits, so IR1 is 7FFFh and nothing is flagged. Row 2 adds 1 * 1 (RT23 = 1,
   VZ0 = 1) to that, total 8000h: only then does IR2 saturate to 7FFFh, with
   flag bit 23 and bit 31, while MAC2 holds 8000h. Row 3 is all zero. ORGB reads
   7FFFh as 31 in two fields: 31 + 31*20h = 3FFh. */
static int run_quirk_clip_end(void)
{
    static const struct rv in[] = {{0u, 0x7FFE7FFFu}, {1u, 1u},          {32u, 0x80017FFFu},
                                   {33u, 0x7FFF0000u}, {34u, 0x00018001u}};
    static const struct rv want[] = {{9u, 0x7FFFu},  {10u, 0x7FFFu}, {25u, 0x7FFFu}, {26u, 0x8000u},
                                     {28u, 0x3FFu},  {29u, 0x3FFu},  {63u, 0x80800000u}};
    return quirk_run("quirk_clip_end", in, COUNT(in), 0x6012u, want, COUNT(want));
}

/* RTPT (sf = 1) with RT33 = 1000h, H = 100h, DQA = 100h, DQB = 0 and three
   vertices at X = Y = 0 with depths 2000h, 1000h and 400h, so SZ3 takes those
   values in turn and SZ0 to SZ3 end as 0, 2000h, 1000h, 400h. The divide of a
   power-of-two divisor is exact (the table entry is FFh, the reciprocal 20000h):
   H * 10000h / SZ3 = 800h, 1000h and 4000h for the three vertices. The depth
   cue IR0 and MAC0 take only the last: MAC0 = 4000h * 100h = 400000h and IR0 =
   MAC0 / 1000h = 400h; the first vertex alone would give 80h and the second
   100h. OFX = 5.0 and OFY = 3.0 give SX = 5, SY = 3 for every vertex, so the
   three FIFO entries are all 00030005h. MAC3 and IR3 hold the last depth,
   400h, which ORGB reads as 8 in the blue field (2000h). */
static int run_quirk_rtpt_ir0(void)
{
    static const struct rv in[] = {{1u, 0x2000u},    {3u, 0x1000u},     {5u, 0x400u},
                                   {36u, 0x1000u},   {56u, 0x50000u},   {57u, 0x30000u},
                                   {58u, 0x100u},    {59u, 0x100u}};
    static const struct rv want[] = {{8u, 0x400u},        {11u, 0x400u},      {12u, 0x00030005u},
                                     {13u, 0x00030005u},  {14u, 0x00030005u}, {15u, 0x00030005u},
                                     {17u, 0x2000u},      {18u, 0x1000u},     {19u, 0x400u},
                                     {24u, 0x400000u},    {27u, 0x400u},      {28u, 0x2000u},
                                     {29u, 0x2000u}};
    return quirk_run("quirk_rtpt_ir0", in, COUNT(in), 0x80030u, want, COUNT(want));
}

/* Every command number outside the 22 documented ones, with sf and lm each way,
   on registers that all hold different values, leaves all 64 registers as they
   were. This is how this engine treats them; it is not a claim about the
   hardware. */
static int run_unlisted_noop(void)
{
    static const uint8_t listed[22] = {0x01, 0x06, 0x0C, 0x10, 0x11, 0x12, 0x13, 0x14, 0x16, 0x1B, 0x1C,
                                       0x1E, 0x20, 0x28, 0x29, 0x2A, 0x2D, 0x2E, 0x30, 0x3D, 0x3E, 0x3F};
    struct schway__gte *g = malloc(sizeof *g);
    if (g == NULL) {
        return 1;
    }
    int bad = 0;
    unsigned tried = 0u;
    for (uint32_t cmd = 0u; cmd < 64u; cmd++) {
        int is_listed = 0;
        for (unsigned k = 0u; k < 22u; k++) {
            is_listed |= listed[k] == cmd;
        }
        if (is_listed) {
            continue;
        }
        for (uint32_t flags = 0u; flags < 4u; flags++) {
            uint32_t op = cmd | (flags & 1u) << 19 | (flags >> 1) << 10;
            uint32_t before[64];
            schway__gte_reset(g);
            for (uint32_t i = 0u; i < 64u; i++) {
                schway__gte_write(g, i, 0x9E3779B9u * (i + 1u));
            }
            for (uint32_t i = 0u; i < 64u; i++) {
                before[i] = schway__gte_read(g, i);
            }
            (void)schway__gte_exec(g, op);
            tried++;
            for (uint32_t i = 0u; i < 64u; i++) {
                if (schway__gte_read(g, i) != before[i]) {
                    fprintf(stderr, "unlisted_noop: command %06lX changed register %lu\n",
                            (unsigned long)op, (unsigned long)i);
                    bad = 1;
                }
            }
        }
    }
    if (tried != 42u * 4u) {
        fprintf(stderr, "unlisted_noop: ran %u commands, expected %u\n", tried, 42u * 4u);
        bad = 1;
    }
    free(g);
    return bad;
}

int main(int argc, char **argv)
{
    if (argc >= 2 && strcmp(argv[1], "vectors") == 0) {
        return run_vectors(argc, argv);
    }
    if (argc >= 2 && strcmp(argv[1], "ref_vectors") == 0) {
        return ref_run_vectors(argc, argv);
    }
    if (argc >= 2 && strcmp(argv[1], "property") == 0) {
        return ref_run_property(argc, argv);
    }
    if (argc == 2 && strcmp(argv[1], "regs_sweep") == 0) {
        return run_regs_sweep();
    }
    if (argc == 2 && strcmp(argv[1], "unr_table") == 0) {
        return run_unr_table();
    }
    if (argc == 2 && strcmp(argv[1], "nclip_fake_bits") == 0) {
        return run_nclip_fake_bits();
    }
    if (argc == 2 && strcmp(argv[1], "quirk_fc") == 0) {
        return run_quirk_fc();
    }
    if (argc == 2 && strcmp(argv[1], "quirk_mx3") == 0) {
        return run_quirk_mx3();
    }
    if (argc == 2 && strcmp(argv[1], "quirk_ir3_lm") == 0) {
        return run_quirk_ir3_lm();
    }
    if (argc == 2 && strcmp(argv[1], "quirk_h_sign") == 0) {
        return run_quirk_h_sign();
    }
    if (argc == 2 && strcmp(argv[1], "quirk_clip_end") == 0) {
        return run_quirk_clip_end();
    }
    if (argc == 2 && strcmp(argv[1], "quirk_rtpt_ir0") == 0) {
        return run_quirk_rtpt_ir0();
    }
    if (argc == 2 && strcmp(argv[1], "unlisted_noop") == 0) {
        return run_unlisted_noop();
    }
    fprintf(stderr, "usage: schwaystation-test-gte vectors FILE --expect N [--ops LIST] [--histogram]\n"
                    "         [--flip N] | ref_vectors FILE --expect N [--ops LIST] |\n"
                    "         property [--mutate] | regs_sweep |\n"
                    "       unr_table | nclip_fake_bits | quirk_fc | quirk_mx3 | quirk_ir3_lm |\n"
                    "       quirk_h_sign | quirk_clip_end | quirk_rtpt_ir0 | unlisted_noop\n");
    return 2;
}
