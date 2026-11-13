/* The independent model's two cases: the hardware-captured cases replayed
   through it (the anchor to hardware data), and the differential test that
   drives the engine and the model with the same seeded inputs and compares
   every register and the cycle count afterwards. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../test_open.h"

#include "gte.h"
#include "../prop.h"
#include "ref.h"
#include "vectors.h"

static void model_write(void *ctx, uint32_t idx, uint32_t v)
{
    gte_ref_write((struct gte_ref *)ctx, idx, v);
}

static void model_exec(void *ctx, uint32_t op)
{
    (void)gte_ref_exec((struct gte_ref *)ctx, op);
}

static uint32_t model_read(void *ctx, uint32_t idx)
{
    return gte_ref_read((const struct gte_ref *)ctx, idx);
}

/* LIST is comma-separated hexadecimal command numbers and the word "reg" for
   the register-only cases; the result is a mask of command numbers and a flag. */
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

int ref_run_vectors(int argc, char **argv)
{
    const char *file = NULL;
    long expect = -1;
    uint64_t mask = 0u;
    int reg = 0, have_ops = 0;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--expect") == 0 && i + 1 < argc) {
            expect = strtol(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--ops") == 0 && i + 1 < argc) {
            have_ops = 1;
            if (parse_ops(argv[++i], &mask, &reg) != 0) {
                fprintf(stderr, "gte ref vectors: bad --ops list\n");
                return 2;
            }
        } else if (file == NULL && argv[i][0] != '-') {
            file = argv[i];
        } else {
            fprintf(stderr, "gte ref vectors: bad argument %s\n", argv[i]);
            return 2;
        }
    }
    if (file == NULL || expect < 0) {
        fprintf(stderr, "usage: schwaystation-test-gte ref_vectors FILE --expect N [--ops LIST]\n");
        return 2;
    }
    size_t len = 0u;
    uint8_t *data = read_file(file, &len);
    long total = data == NULL ? -1 : gtev_count(data, len);
    if (total < 0) {
        fprintf(stderr, "gte ref vectors: %s is unreadable or truncated\n", file);
        free(data);
        return 1;
    }
    long run = 0, bad = 0;
    for (long n = 0; n < total; n++) {
        struct gtev_case c;
        gtev_load(data, (uint32_t)n, &c);
        if (have_ops) {
            int wanted = c.opcode == GTEV_REG_ONLY ? reg : (int)((mask >> (c.opcode & 0x3Fu)) & 1u);
            if (!wanted) {
                continue;
            }
        }
        run++;
        struct gte_ref r;
        memset(&r, 0, sizeof r);
        uint32_t got = 0u, want = 0u;
        int at = gtev_replay(&c, model_write, model_exec, model_read, &r, &got, &want);
        if (at >= 0) {
            bad++;
            if (bad <= 10) {
                fprintf(stderr, "gte ref vectors: case %ld command %02X register %d got %08X want %08X\n", n,
                        c.opcode == GTEV_REG_ONLY ? 0xFFu : (unsigned)(c.opcode & 0x3Fu), at, (unsigned)got,
                        (unsigned)want);
            }
        }
    }
    free(data);
    if (run != expect) {
        fprintf(stderr, "gte ref vectors: replayed %ld cases, expected %ld\n", run, expect);
        return 1;
    }
    if (bad != 0) {
        fprintf(stderr, "gte ref vectors: %ld of %ld cases differ\n", bad, run);
        return 1;
    }
    printf("gte ref vectors: %ld cases match\n", run);
    return 0;
}

/* ---- the differential test --------------------------------------------- */

/* The stream's seed, printed with every run so a failure replays. */
#define PROP_SEED UINT64_C(0x6774652D70726F70)
#define PROP_COMMANDS 22u
#define PROP_PER_COMMAND 1000u
#define PROP_TOTAL 22000u /* PROP_COMMANDS * PROP_PER_COMMAND, asserted at the end of the run */

/* The case the --mutate run damages: one bit of IR0 on the engine's side. */
#define PROP_MUTATE_CASE 12345u
#define PROP_MUTATE_REG 8u

/* The documented command numbers (psx-spx, GTE Opcode Summary). */
static const uint8_t prop_commands[PROP_COMMANDS] = {0x01, 0x06, 0x0C, 0x10, 0x11, 0x12, 0x13, 0x14,
                                                     0x16, 0x1B, 0x1C, 0x1E, 0x20, 0x28, 0x29, 0x2A,
                                                     0x2D, 0x2E, 0x30, 0x3D, 0x3E, 0x3F};

/* Words where a rule changes: zero, the 16-bit and 32-bit limits, pairs of
   16-bit limits, 1.0 in 4.12 and 16.16 fixed point, and words that put the
   translation times 1000h, or a product plus an offset, on the 43-bit and
   31-bit boundaries from both sides. */
static const uint32_t prop_edges[] = {
    0x00000000u, 0x00000001u, 0x00007FFFu, 0x00008000u, 0x0000FFFFu, 0x00010000u, 0x7FFFFFFFu,
    0x80000000u, 0x80000001u, 0xFFFFFFFFu, 0x7FFF7FFFu, 0x80008000u, 0x7FFF8000u, 0x80007FFFu,
    0x00001000u, 0xFFFFF000u, 0x7FFFF000u, 0x00010001u, 0x0001FFFFu, 0x3FFFFFFFu, 0xC0000000u,
    0x40000000u, 0x7FFF0000u, 0x80000FFFu, 0x00FFFFFFu, 0x00FF00FFu, 0x10001000u, 0xF000F000u};

/* A word whose two halves are both small, so that a sum stays inside its
   range and the unsaturated paths run too. */
static uint32_t prop_small(struct prop *p)
{
    uint32_t lo = prop_below(p, 0x4001u) - 0x2000u;
    uint32_t hi = prop_below(p, 0x4001u) - 0x2000u;
    return (lo & 0xFFFFu) | (hi & 0xFFFFu) << 16;
}

/* Three of every four cases draw all 64 words from the edge table or at
   random; the fourth draws small halves, so a long sum does not saturate
   every time. */
static uint32_t prop_word(struct prop *p, int small)
{
    return small ? prop_small(p) : prop_edge32(p, prop_edges, sizeof prop_edges / sizeof prop_edges[0]);
}

int ref_run_property(int argc, char **argv)
{
    int mutate = 0;
    if (argc == 3 && strcmp(argv[2], "--mutate") == 0) {
        mutate = 1;
    } else if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-gte property [--mutate]\n");
        return 2;
    }
    struct prop p;
    prop_seed(&p, PROP_SEED);
    printf("gte property: seed 0x%016llX, %u commands of %u cases%s\n", (unsigned long long)PROP_SEED,
           PROP_COMMANDS, PROP_PER_COMMAND, mutate ? ", one engine-side bit flipped" : "");
    unsigned total = 0u, flagged = 0u;
    for (unsigned k = 0u; k < PROP_COMMANDS; k++) {
        for (unsigned n = 0u; n < PROP_PER_COMMAND; n++) {
            int small = prop_below(&p, 4u) == 3u;
            uint32_t in[64];
            for (uint32_t i = 0u; i < 64u; i++) {
                in[i] = prop_word(&p, small);
            }
            /* The command field: the number, then sf, lm, the matrix, vector and
               translation fields and the fake number drawn at random, bits 6 to 24. */
            uint32_t op = prop_commands[k] | prop_below(&p, 1u << 19) << 6;

            struct schway__gte g;
            struct gte_ref r;
            memset(&g, 0, sizeof g);
            memset(&r, 0, sizeof r);
            for (uint32_t i = 0u; i < 64u; i++) {
                schway__gte_write(&g, i, in[i]);
                gte_ref_write(&r, i, in[i]);
            }
            uint32_t seed_cycles = schway__gte_exec(&g, op);
            uint32_t ref_cycles = gte_ref_exec(&r, op);
            total++;
            for (uint32_t i = 0u; i < 64u; i++) {
                uint32_t a = schway__gte_read(&g, i);
                uint32_t b = gte_ref_read(&r, i);
                if (mutate && total - 1u == PROP_MUTATE_CASE && i == PROP_MUTATE_REG) {
                    a ^= 1u;
                }
                if (a != b) {
                    fprintf(stderr,
                            "gte property: engine differs from the model: command %02X case %u (op %07X, "
                            "seed 0x%016llX) register %u engine %08X model %08X\n",
                            (unsigned)prop_commands[k], total - 1u, (unsigned)op, (unsigned long long)PROP_SEED,
                            (unsigned)i, (unsigned)a, (unsigned)b);
                    return 1;
                }
            }
            if (seed_cycles != ref_cycles) {
                fprintf(stderr,
                        "gte property: engine differs from the model: command %02X case %u (op %07X, "
                        "seed 0x%016llX) cycles engine %u model %u\n",
                        (unsigned)prop_commands[k], total - 1u, (unsigned)op, (unsigned long long)PROP_SEED,
                        (unsigned)seed_cycles, (unsigned)ref_cycles);
                return 1;
            }
            if ((gte_ref_read(&r, 63u) & 0x7FFFF000u) != 0u) {
                flagged++;
            }
        }
    }
    if (total != PROP_TOTAL) {
        fprintf(stderr, "gte property: compared %u cases, expected %u\n", total, (unsigned)PROP_TOTAL);
        return 1;
    }
    printf("gte property: %u cases compared, all 64 registers and the cycles match (%u with a flag set)\n", total,
           flagged);
    return 0;
}
