/* Reader and replay for the geometry engine's hardware cases.
   The container is a u32 case count, then 516-byte records: a u32 opcode
   (FFFFFFFFh for a register-only case, else the 25-bit command field), 64
   input words and 64 output words, all little-endian (tools/gte2bin.py). A
   case replays in one fixed order: write 0 to the registers 0 to 63, write
   the 64 inputs in order, run the command unless the case is register-only,
   read the registers 0 to 63 and match them with the outputs. The engine is
   reached through three caller-supplied functions, so this file holds no
   engine code and any implementation can be replayed through it. */
#ifndef SCHWAYSTATION_TESTS_GTE_VECTORS_H
#define SCHWAYSTATION_TESTS_GTE_VECTORS_H

#include <stddef.h>
#include <stdint.h>

#define GTEV_REGS 64u
#define GTEV_RECORD_BYTES 516u
#define GTEV_REG_ONLY 0xFFFFFFFFu

struct gtev_case {
    uint32_t opcode;
    uint32_t in[GTEV_REGS];
    uint32_t out[GTEV_REGS];
};

typedef void (*gtev_write_fn)(void *ctx, uint32_t idx, uint32_t v);
typedef void (*gtev_exec_fn)(void *ctx, uint32_t op);
typedef uint32_t (*gtev_read_fn)(void *ctx, uint32_t idx);

static uint32_t gtev_word(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

/* The case count of a container of len bytes, or -1 when the length is not
   exactly a count word plus that many records. */
static long gtev_count(const uint8_t *data, size_t len)
{
    if (len < 4u) {
        return -1;
    }
    uint32_t n = gtev_word(data);
    if (n > (UINT32_MAX - 4u) / GTEV_RECORD_BYTES || len != 4u + (size_t)n * GTEV_RECORD_BYTES) {
        return -1;
    }
    return (long)n;
}

/* Case n (from 0) of a container gtev_count accepted. */
static void gtev_load(const uint8_t *data, uint32_t n, struct gtev_case *c)
{
    const uint8_t *p = data + 4u + (size_t)n * GTEV_RECORD_BYTES;
    c->opcode = gtev_word(p);
    for (uint32_t i = 0u; i < GTEV_REGS; i++) {
        c->in[i] = gtev_word(p + 4u + 4u * i);
        c->out[i] = gtev_word(p + 4u + 4u * GTEV_REGS + 4u * i);
    }
}

/* Replays c and returns the first register that differs, or -1 when all 64
   match; *got and *want hold the two values of that register. */
static int gtev_replay(const struct gtev_case *c, gtev_write_fn write, gtev_exec_fn exec,
                       gtev_read_fn read, void *ctx, uint32_t *got, uint32_t *want)
{
    for (uint32_t i = 0u; i < GTEV_REGS; i++) {
        write(ctx, i, 0u);
    }
    for (uint32_t i = 0u; i < GTEV_REGS; i++) {
        write(ctx, i, c->in[i]);
    }
    if (c->opcode != GTEV_REG_ONLY) {
        exec(ctx, c->opcode);
    }
    for (uint32_t i = 0u; i < GTEV_REGS; i++) {
        uint32_t v = read(ctx, i);
        if (v != c->out[i]) {
            *got = v;
            *want = c->out[i];
            return (int)i;
        }
    }
    return -1;
}

#endif /* SCHWAYSTATION_TESTS_GTE_VECTORS_H */
