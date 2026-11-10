/* The independent model's two cases: the hardware-captured cases replayed
   through it (the anchor to hardware data), and, in a later part of this file,
   the differential test against the engine. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
