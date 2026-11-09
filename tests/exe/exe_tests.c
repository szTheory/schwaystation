/* The PS-EXE validator, probe and loader: schwaystation-test-exe <case>.
   White-box: the test sees src/, so it can read guest RAM directly. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

#define HEADER 0x800u
#define RAM 0x200000u

/* The header fields a case sets; everything else in the file is zero. */
struct hdr {
    uint32_t pc0, gp0, t_addr, t_size, d_addr, d_size, b_addr, b_size, s_addr, s_size;
    const char *licence;
};

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

/* A file of n bytes: the signature and header h, then non-zero text bytes
   (i * 7 + 1 modulo 251, plus 1). At least a header's worth is allocated, so n
   may be given short to test a truncated file. */
static uint8_t *build(const struct hdr *h, size_t n)
{
    size_t room = n < HEADER ? HEADER : n;
    uint8_t *b = (uint8_t *)calloc(room, 1u);
    if (b == NULL) {
        exit(1);
    }
    memcpy(b, "PS-X EXE", 8);
    put32(b + 0x10, h->pc0);
    put32(b + 0x14, h->gp0);
    put32(b + 0x18, h->t_addr);
    put32(b + 0x1C, h->t_size);
    put32(b + 0x20, h->d_addr);
    put32(b + 0x24, h->d_size);
    put32(b + 0x28, h->b_addr);
    put32(b + 0x2C, h->b_size);
    put32(b + 0x30, h->s_addr);
    put32(b + 0x34, h->s_size);
    if (h->licence != NULL) {
        memcpy(b + 0x4C, h->licence, strlen(h->licence));
    }
    for (size_t i = HEADER; i < room; i++) {
        b[i] = (uint8_t)((i * 7u + 1u) % 251u + 1u);
    }
    return b;
}

static schway_status probe(const uint8_t *b, size_t n, schway_exe_info *info)
{
    memset(info, 0, sizeof *info);
    info->size = (uint32_t)sizeof *info;
    return schway_probe_exe(b, n, info);
}

/* Every field comes back from its own offset, in little-endian order. */
static void case_probe_valid(void)
{
    struct hdr h;
    memset(&h, 0, sizeof h);
    h.pc0 = 0x80012344u;
    h.gp0 = 0x8001F800u;
    h.t_addr = 0x80010000u;
    h.t_size = 0x1000u;
    h.d_addr = 0x11223344u;
    h.d_size = 0x55667788u;
    h.b_addr = 0x99AABBCCu;
    h.b_size = 0xDDEEFF00u;
    h.s_addr = 0x801FFF00u;
    h.s_size = 0x100u;
    uint8_t *b = build(&h, HEADER + 0x1000u);
    schway_exe_info info;
    CHECK_EQ_U64(probe(b, HEADER + 0x1000u, &info), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(info.size, sizeof info);
    CHECK_EQ_HEX(info.pc0, 0x80012344u);
    CHECK_EQ_HEX(info.gp0, 0x8001F800u);
    CHECK_EQ_HEX(info.t_addr, 0x80010000u);
    CHECK_EQ_HEX(info.t_size, 0x1000u);
    CHECK_EQ_HEX(info.d_addr, 0x11223344u);
    CHECK_EQ_HEX(info.d_size, 0x55667788u);
    CHECK_EQ_HEX(info.b_addr, 0x99AABBCCu);
    CHECK_EQ_HEX(info.b_size, 0xDDEEFF00u);
    CHECK_EQ_HEX(info.s_addr, 0x801FFF00u);
    CHECK_EQ_HEX(info.s_size, 0x100u);
    CHECK_EQ_U64(info.licence_area, SCHWAY_LICENCE_NONE);
    CHECK_EQ_U64(info.lint, 0u);
    CHECK_EQ_HEX(info.hash_len, 0x1800u);

    /* A caller's larger struct keeps its tail; a smaller tag is refused. */
    struct {
        schway_exe_info head;
        uint8_t tail[8];
    } big;
    memset(&big, 0, sizeof big);
    memset(big.tail, 0xA5, sizeof big.tail);
    big.head.size = (uint32_t)sizeof big;
    CHECK_EQ_U64(schway_probe_exe(b, HEADER + 0x1000u, &big.head), SCHWAY_STATUS_OK);
    CHECK_EQ_HEX(big.head.pc0, 0x80012344u);
    for (size_t i = 0; i < sizeof big.tail; i++) {
        CHECK_EQ_U64(big.tail[i], 0xA5u);
    }
    schway_exe_info small;
    memset(&small, 0, sizeof small);
    small.size = (uint32_t)sizeof small - 4u;
    CHECK_EQ_U64(schway_probe_exe(b, HEADER + 0x1000u, &small), SCHWAY_STATUS_ERR_STRUCT_SIZE);
    free(b);
}

typedef void (*case_fn)(void);

static const struct {
    const char *name;
    case_fn fn;
} cases[] = {
    {"probe_valid", case_probe_valid},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].fn();
            CHECK_DONE();
        }
    }
    return 2;
}
