/* The geometry engine's tests; one case per command-line argument.
     schwaystation-test-gte vectors FILE --expect N [--ops LIST]
     schwaystation-test-gte regs_sweep
   vectors replays the hardware cases of FILE (vectors.h) through the engine's
   register and command entry points. LIST holds hex command numbers and "reg"
   for the register-only cases; only those cases run. It prints the number of
   cases run and the first register that differs in each failing case. Exit 0
   when exactly N cases ran and all matched, 1 for a failure or another count,
   2 for a bad command line. regs_sweep drives every register and every command
   with edge values and checks that reads repeat; run under the sanitizers, it
   also proves the engine touches nothing outside its struct. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "gte.h"
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
    FILE *f = fopen(path, "rb");
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

static int run_vectors(int argc, char **argv)
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
                fprintf(stderr, "gte vectors: bad --ops list\n");
                return 2;
            }
        } else if (file == NULL && argv[i][0] != '-') {
            file = argv[i];
        } else {
            fprintf(stderr, "gte vectors: bad argument %s\n", argv[i]);
            return 2;
        }
    }
    if (file == NULL || expect < 0) {
        fprintf(stderr, "usage: schwaystation-test-gte vectors FILE --expect N [--ops LIST]\n");
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
    struct schway__gte *g = malloc(sizeof *g);
    if (g == NULL) {
        free(data);
        return 1;
    }
    long ran = 0, failed = 0;
    for (long n = 0; n < total; n++) {
        struct gtev_case c;
        gtev_load(data, (uint32_t)n, &c);
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

int main(int argc, char **argv)
{
    if (argc >= 2 && strcmp(argv[1], "vectors") == 0) {
        return run_vectors(argc, argv);
    }
    if (argc == 2 && strcmp(argv[1], "regs_sweep") == 0) {
        return run_regs_sweep();
    }
    fprintf(stderr, "usage: schwaystation-test-gte vectors FILE --expect N [--ops LIST] | regs_sweep\n");
    return 2;
}
