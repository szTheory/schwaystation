/* The PS-EXE validator, probe and loader: schwaystation-test-exe <case>, or
   schwaystation-test-exe write <dir> for the runner's probe tests.
   White-box: the test sees src/, so it can read guest RAM directly. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"
#include "../../runner/md5.h"

#define HEADER 0x800u
#define RAM 0x200000u
#define LAUNCH 0x200u

static void *test_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void test_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static schway *make(void)
{
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) {
        exit(1);
    }
    return inst;
}

/* The header fields a case sets; everything else in the file is zero. */
struct hdr {
    uint32_t pc0, gp0, t_addr, t_size, d_addr, d_size, b_addr, b_size, s_addr, s_size;
    const char *licence;
};

/* A valid header: entry and text at 80010000h, one text sector, default stack. */
static struct hdr valid(void)
{
    struct hdr h;
    memset(&h, 0, sizeof h);
    h.pc0 = 0x80010000u;
    h.t_addr = 0x80010000u;
    h.t_size = 0x800u;
    return h;
}

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
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

/* The instance's whole state as bytes, for before-and-after comparisons. */
static uint8_t *snapshot(const schway *inst, size_t *n)
{
    *n = schway_state_size(inst);
    uint8_t *buf = (uint8_t *)malloc(*n);
    if (buf == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(schway_save_state(inst, buf, *n), SCHWAY_STATUS_OK);
    return buf;
}

/* One instance's guest RAM is entirely zero. */
static int ram_is_zero(const schway *inst)
{
    for (uint32_t i = 0; i < RAM; i++) {
        if (inst->bus.ram[i] != 0u) {
            return 0;
        }
    }
    return 1;
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

/* A file the probe and the loader both refuse with status want, the loader
   leaving RAM untouched and nothing marked loaded. */
static void refused(const uint8_t *b, size_t n, schway_status want, int line)
{
    schway_exe_info info;
    schway *inst = make();
    schway_status pst = probe(b, n, &info);
    schway_status lst = schway_load_exe(inst, b, n);
    if (pst != want || lst != want) {
        fprintf(stderr, "%s:%d: probe %u, load %u, want %u\n", __FILE__, line, (unsigned)pst,
                (unsigned)lst, (unsigned)want);
        check_failures++;
    }
    CHECK(ram_is_zero(inst));
    CHECK_EQ_U64(inst->exe_loaded, 0u);
    schway_destroy(inst);
}

#define REFUSED(b, n, want) refused((b), (n), (want), __LINE__)

/* A header that fails to a given status over a file of n bytes. */
static void refused_hdr(struct hdr h, size_t n, schway_status want, int line)
{
    uint8_t *b = build(&h, n);
    refused(b, n, want, line);
    free(b);
}

#define REFUSED_HDR(h, n, want) refused_hdr((h), (n), (want), __LINE__)

/* Each rule: no byte lands. */
static void case_rejects(void)
{
    const size_t one = HEADER + 0x800u;
    struct hdr h = valid();
    uint8_t *b = build(&h, one);

    /* Signature and size of the file. */
    b[7] = 'F';
    REFUSED(b, one, SCHWAY_STATUS_ERR_EXE_INVALID);
    b[7] = 'E';
    b[0] = 0u;
    REFUSED(b, one, SCHWAY_STATUS_ERR_EXE_INVALID);
    b[0] = 'P';
    REFUSED(b, HEADER - 1u, SCHWAY_STATUS_ERR_EXE_INVALID);
    REFUSED(b, 8u, SCHWAY_STATUS_ERR_EXE_INVALID);
    free(b);

    /* Text size: a whole sector, inside the file. */
    h.t_size = 0x801u;
    REFUSED_HDR(h, one + 0x800u, SCHWAY_STATUS_ERR_EXE_INVALID);
    h.t_size = 0x400u;
    REFUSED_HDR(h, one, SCHWAY_STATUS_ERR_EXE_INVALID);
    h.t_size = 0x1000u;
    REFUSED_HDR(h, one, SCHWAY_STATUS_ERR_EXE_INVALID);
    h.t_size = 0xFFFFF800u;
    REFUSED_HDR(h, one, SCHWAY_STATUS_ERR_EXE_INVALID);

    /* Text address: aligned, mapped to RAM, ending inside it. */
    h = valid();
    h.t_addr = 0x80010002u;
    REFUSED_HDR(h, one, SCHWAY_STATUS_ERR_EXE_INVALID);
    h.t_addr = 0x80010001u;
    REFUSED_HDR(h, one, SCHWAY_STATUS_ERR_EXE_INVALID);
    h.t_addr = 0xC0010000u;
    REFUSED_HDR(h, one, SCHWAY_STATUS_ERR_EXE_RANGE);
    h.t_addr = 0xFFFFF800u;
    REFUSED_HDR(h, one, SCHWAY_STATUS_ERR_EXE_RANGE);
    h.t_addr = 0x80200000u;
    REFUSED_HDR(h, one, SCHWAY_STATUS_ERR_EXE_RANGE);
    h.t_addr = 0x801FF800u;
    h.t_size = 0x1000u;
    REFUSED_HDR(h, HEADER + 0x1000u, SCHWAY_STATUS_ERR_EXE_RANGE);
    h.t_addr = 0x80200004u; /* even nothing to copy has to start inside RAM */
    h.t_size = 0u;
    REFUSED_HDR(h, HEADER, SCHWAY_STATUS_ERR_EXE_RANGE);

    /* Entry point: non-zero and word-aligned. */
    h = valid();
    h.pc0 = 0u;
    REFUSED_HDR(h, one, SCHWAY_STATUS_ERR_EXE_INVALID);
    h.pc0 = 0x80010002u;
    REFUSED_HDR(h, one, SCHWAY_STATUS_ERR_EXE_INVALID);
    h.pc0 = 0x80010001u;
    REFUSED_HDR(h, one, SCHWAY_STATUS_ERR_EXE_INVALID);

    /* Stack: zero, or a range inside RAM. */
    h = valid();
    h.s_addr = 0x80200000u;
    h.s_size = 4u;
    REFUSED_HDR(h, one, SCHWAY_STATUS_ERR_EXE_RANGE);
    h.s_addr = 0x801FFF00u;
    h.s_size = 0x101u;
    REFUSED_HDR(h, one, SCHWAY_STATUS_ERR_EXE_RANGE);
    h.s_addr = 0xC0000000u;
    h.s_size = 0u;
    REFUSED_HDR(h, one, SCHWAY_STATUS_ERR_EXE_RANGE);
    h.s_addr = 0x80100000u;
    h.s_size = 0xFFFFFFFFu;
    REFUSED_HDR(h, one, SCHWAY_STATUS_ERR_EXE_RANGE);

    /* A file past 0800h + 2 MiB. */
    h = valid();
    REFUSED_HDR(h, (size_t)HEADER + RAM + 1u, SCHWAY_STATUS_ERR_EXE_RANGE);

    /* Arguments. */
    schway_exe_info info;
    schway *inst = make();
    b = build(&h, one);
    CHECK_EQ_U64(schway_load_exe(NULL, b, one), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_load_exe(inst, NULL, one), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_load_exe(inst, b, 0u), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_probe_exe(NULL, one, &info), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_probe_exe(b, 0u, &info), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_probe_exe(b, one, NULL), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK(ram_is_zero(inst));
    schway_destroy(inst);
    free(b);
}

/* A file of n bytes under header h loads and probes OK; the loaded instance
   is returned for the caller to inspect. */
static schway *accepted(const struct hdr *h, size_t n, uint8_t **file, schway_exe_info *info,
                        int line)
{
    uint8_t *b = build(h, n);
    schway *inst = make();
    schway_status pst = probe(b, n, info);
    schway_status lst = schway_load_exe(inst, b, n);
    if (pst != SCHWAY_STATUS_OK || lst != SCHWAY_STATUS_OK) {
        fprintf(stderr, "%s:%d: probe %u, load %u, want OK\n", __FILE__, line, (unsigned)pst,
                (unsigned)lst);
        check_failures++;
    }
    *file = b;
    return inst;
}

#define ACCEPTED(h, n, file, info) accepted((h), (n), (file), (info), __LINE__)

/* The edges of every limit: the last byte of RAM, one sector past it, an empty
   text, the largest and next-larger file, and the three segment aliases. */
static void case_boundaries(void)
{
    schway_exe_info info;
    uint8_t *b;
    schway *inst;
    struct hdr h = valid();

    /* Text ending exactly at the end of RAM, and 0800h more. */
    h.t_addr = 0x801FF800u;
    inst = ACCEPTED(&h, HEADER + 0x800u, &b, &info);
    CHECK(memcmp(&inst->bus.ram[RAM - 0x800u], b + HEADER, 0x800u) == 0);
    schway_destroy(inst);
    free(b);
    h.t_size = 0x1000u;
    REFUSED_HDR(h, HEADER + 0x1000u, SCHWAY_STATUS_ERR_EXE_RANGE);

    /* A header and no text. */
    h = valid();
    h.t_size = 0u;
    inst = ACCEPTED(&h, HEADER, &b, &info);
    CHECK_EQ_HEX(info.hash_len, HEADER);
    CHECK_EQ_U64(inst->exe_loaded, 1u);
    CHECK_EQ_HEX(get32(&inst->bus.ram[LAUNCH]), 0x80010000u);
    schway_destroy(inst);
    free(b);

    /* All of RAM as text: the largest file, and one byte more or fewer. */
    h = valid();
    h.t_addr = 0x80000000u;
    h.t_size = RAM;
    inst = ACCEPTED(&h, (size_t)HEADER + RAM, &b, &info);
    CHECK(memcmp(&inst->bus.ram[LAUNCH + 16u], b + HEADER + LAUNCH + 16u, RAM - LAUNCH - 16u) ==
          0);
    CHECK(memcmp(inst->bus.ram, b + HEADER, LAUNCH) == 0); /* below the launch block */
    schway_destroy(inst);
    free(b);
    REFUSED_HDR(h, (size_t)HEADER + RAM + 1u, SCHWAY_STATUS_ERR_EXE_RANGE);
    REFUSED_HDR(h, (size_t)HEADER + RAM - 1u, SCHWAY_STATUS_ERR_EXE_INVALID);

    /* Stack ranges ending exactly at the end of RAM. */
    h = valid();
    h.s_addr = 0x801FFF00u;
    h.s_size = 0x100u;
    inst = ACCEPTED(&h, HEADER + 0x800u, &b, &info);
    schway_destroy(inst);
    free(b);
    h.s_addr = 0xA01FFF00u;
    inst = ACCEPTED(&h, HEADER + 0x800u, &b, &info);
    schway_destroy(inst);
    free(b);

    /* KUSEG, KSEG0 and KSEG1 reach the same RAM. */
    uint8_t *file = NULL;
    schway *ref = NULL;
    static const uint32_t aliases[] = {0x80010000u, 0x00010000u, 0xA0010000u};
    for (size_t i = 0; i < sizeof aliases / sizeof aliases[0]; i++) {
        h = valid();
        h.t_addr = aliases[i];
        inst = ACCEPTED(&h, HEADER + 0x800u, &b, &info);
        CHECK(memcmp(&inst->bus.ram[0x10000], b + HEADER, 0x800u) == 0);
        if (ref == NULL) {
            ref = inst;
            file = b;
            continue;
        }
        CHECK(memcmp(ref->bus.ram, inst->bus.ram, RAM) == 0);
        schway_destroy(inst);
        free(b);
    }
    schway_destroy(ref);
    free(file);
}

/* The low 64 KiB and trailing bytes are noted in lint, never refused. */
static void case_lint(void)
{
    static const struct {
        uint32_t t_addr, t_size;
        size_t extra;
        uint32_t lint;
    } rows[] = {
        {0x80010000u, 0x800u, 0u, 0u},
        {0x80000800u, 0x800u, 0u, SCHWAY_EXE_LINT_LOW_64K},
        {0x0000FFFCu, 0x800u, 0u, SCHWAY_EXE_LINT_LOW_64K},
        {0x00000000u, 0x800u, 0u, SCHWAY_EXE_LINT_LOW_64K},
        {0xA000FFFCu, 0x1000u, 1u, SCHWAY_EXE_LINT_LOW_64K | SCHWAY_EXE_LINT_TRAILING},
        {0x80000000u, 0u, 0u, 0u},
        {0x80010000u, 0x800u, 1u, SCHWAY_EXE_LINT_TRAILING},
        {0x80010000u, 0x800u, 0x800u, SCHWAY_EXE_LINT_TRAILING},
        {0x80010000u, 0u, 4u, SCHWAY_EXE_LINT_TRAILING},
    };
    for (size_t i = 0; i < sizeof rows / sizeof rows[0]; i++) {
        struct hdr h = valid();
        h.t_addr = rows[i].t_addr;
        h.t_size = rows[i].t_size;
        size_t n = HEADER + rows[i].t_size + rows[i].extra;
        schway_exe_info info;
        uint8_t *b;
        schway *inst = ACCEPTED(&h, n, &b, &info);
        if (info.lint != rows[i].lint) {
            fprintf(stderr, "%s:%d: row %zu lint %u, want %u\n", __FILE__, __LINE__, i,
                    (unsigned)info.lint, (unsigned)rows[i].lint);
            check_failures++;
        }
        schway_destroy(inst);
        free(b);
    }
}

/* The text at 4Ch names the region by three exact strings. */
static void case_licence(void)
{
    static const char japan[] = "Sony Computer Entertainment Inc. for Japan area";
    static const char europe[] = "Sony Computer Entertainment Inc. for Europe area";
    static const char america[] = "Sony Computer Entertainment Inc. for North America area";
    static const struct {
        const char *text;
        uint32_t want;
    } rows[] = {
        {japan, SCHWAY_LICENCE_JAPAN},
        {europe, SCHWAY_LICENCE_EUROPE},
        {america, SCHWAY_LICENCE_NORTH_AMERICA},
        {NULL, SCHWAY_LICENCE_NONE},
    };
    for (size_t i = 0; i < sizeof rows / sizeof rows[0]; i++) {
        struct hdr h = valid();
        h.licence = rows[i].text;
        uint8_t *b = build(&h, HEADER + 0x800u);
        schway_exe_info info;
        CHECK_EQ_U64(probe(b, HEADER + 0x800u, &info), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(info.licence_area, rows[i].want);
        if (rows[i].text != NULL) {
            /* One byte off at the start, the middle and the end is no region,
               and neither is the same text one byte later. */
            size_t len = strlen(rows[i].text);
            static const size_t at[] = {0u, 10u, 33u};
            for (size_t k = 0; k < sizeof at / sizeof at[0]; k++) {
                b[0x4C + at[k]] ^= 1u;
                CHECK_EQ_U64(probe(b, HEADER + 0x800u, &info), SCHWAY_STATUS_OK);
                CHECK_EQ_U64(info.licence_area, SCHWAY_LICENCE_NONE);
                b[0x4C + at[k]] ^= 1u;
            }
            b[0x4C + len - 1u] ^= 1u;
            CHECK_EQ_U64(probe(b, HEADER + 0x800u, &info), SCHWAY_STATUS_OK);
            CHECK_EQ_U64(info.licence_area, SCHWAY_LICENCE_NONE);
            b[0x4C + len - 1u] ^= 1u;
            CHECK_EQ_U64(probe(b, HEADER + 0x800u, &info), SCHWAY_STATUS_OK);
            CHECK_EQ_U64(info.licence_area, rows[i].want);
            memmove(b + 0x4D, b + 0x4C, len);
            b[0x4C] = 0u;
            CHECK_EQ_U64(probe(b, HEADER + 0x800u, &info), SCHWAY_STATUS_OK);
            CHECK_EQ_U64(info.licence_area, SCHWAY_LICENCE_NONE);
        }
        free(b);
    }
}

/* The text lands at its physical address, the launch block at 0200h, and
   every other byte of RAM stays zero. */
static void case_load_writes(void)
{
    struct hdr h = valid();
    h.pc0 = 0x80010010u;
    h.gp0 = 0x80018000u;
    h.t_size = 0x1000u;
    h.s_addr = 0x801FFF00u;
    h.s_size = 0x40u;
    uint8_t *b = build(&h, HEADER + 0x1000u);
    schway *inst = make();
    struct schway__cpu cpu;
    struct schway__irq irq;
    memcpy(&cpu, &inst->cpu, sizeof cpu);
    memcpy(&irq, &inst->irq, sizeof irq);
    CHECK_EQ_U64(schway_load_exe(inst, b, HEADER + 0x1000u), SCHWAY_STATUS_OK);
    CHECK(memcmp(&inst->bus.ram[0x10000], b + HEADER, 0x1000u) == 0);
    CHECK_EQ_HEX(get32(&inst->bus.ram[LAUNCH]), 0x80010010u);
    CHECK_EQ_HEX(get32(&inst->bus.ram[LAUNCH + 4u]), 0x80018000u);
    CHECK_EQ_HEX(get32(&inst->bus.ram[LAUNCH + 8u]), 0x801FFF00u);
    CHECK_EQ_HEX(get32(&inst->bus.ram[LAUNCH + 12u]), 0x40u);
    for (uint32_t i = 0; i < RAM; i++) {
        int text = i >= 0x10000u && i < 0x11000u;
        int block = i >= LAUNCH && i < LAUNCH + 16u;
        if (!text && !block && inst->bus.ram[i] != 0u) {
            fprintf(stderr, "%s:%d: RAM byte %06x is not zero\n", __FILE__, __LINE__,
                    (unsigned)i);
            check_failures++;
            break;
        }
    }
    CHECK_EQ_U64(inst->exe_loaded, 1u);
    CHECK_EQ_U64(inst->frame, 0u);
    CHECK_EQ_U64(inst->ticks, 0u);
    CHECK(memcmp(&inst->cpu, &cpu, sizeof cpu) == 0);
    CHECK(memcmp(&inst->irq, &irq, sizeof irq) == 0);
    for (uint32_t i = 0; i < SCHWAY__SCRATCH_SIZE; i++) {
        CHECK_EQ_U64(inst->bus.scratch[i], 0u);
    }

    /* A power reset erases the load. */
    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    CHECK(ram_is_zero(inst));
    CHECK_EQ_U64(inst->exe_loaded, 0u);
    schway_destroy(inst);
    free(b);
}

/* Refused loads leave the instance's saved state byte for byte as it was. */
static void case_error_changes_nothing(void)
{
    struct hdr h = valid();
    uint8_t *good = build(&h, HEADER + 0x800u);
    schway *inst = make();
    CHECK_EQ_U64(schway_load_exe(inst, good, HEADER + 0x800u), SCHWAY_STATUS_OK);
    size_t n;
    uint8_t *before = snapshot(inst, &n);

    struct hdr bad[6];
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        bad[i] = valid();
        bad[i].t_addr = 0x80030000u;
        bad[i].gp0 = 0x1234u;
    }
    bad[0].t_size = 0x801u;
    bad[1].t_addr = 0xC0000000u;
    bad[2].t_addr = 0x801FF800u;
    bad[2].t_size = 0x1000u;
    bad[3].pc0 = 0u;
    bad[4].s_addr = 0x801FFFF0u;
    bad[4].s_size = 0x20u;
    bad[5].t_addr = 0x80030002u;
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        size_t len = HEADER + (i == 2u ? 0x1000u : 0x800u);
        uint8_t *b = build(&bad[i], len);
        CHECK(schway_load_exe(inst, b, len) != SCHWAY_STATUS_OK);
        free(b);
        size_t after_n;
        uint8_t *after = snapshot(inst, &after_n);
        CHECK_EQ_U64(after_n, n);
        CHECK(after_n == n && memcmp(before, after, n) == 0);
        free(after);
    }
    CHECK(schway_load_exe(inst, NULL, 10u) != SCHWAY_STATUS_OK);
    CHECK(schway_load_exe(inst, good, 0u) != SCHWAY_STATUS_OK);
    CHECK(schway_load_exe(inst, good, HEADER - 1u) != SCHWAY_STATUS_OK);
    uint8_t *after = snapshot(inst, &n);
    CHECK(memcmp(before, after, n) == 0);
    free(after);
    free(before);
    schway_destroy(inst);
    free(good);
}

/* A load after the first frame is refused whole; a power reset allows it. */
static void case_after_first_frame(void)
{
    struct hdr h = valid();
    uint8_t *b = build(&h, HEADER + 0x800u);
    schway *inst = make();
    CHECK_EQ_U64(schway_run_frame(inst, NULL), SCHWAY_STATUS_OK);
    size_t n;
    uint8_t *before = snapshot(inst, &n);
    CHECK_EQ_U64(schway_load_exe(inst, b, HEADER + 0x800u), SCHWAY_STATUS_ERR_ARGUMENT);
    uint8_t *after = snapshot(inst, &n);
    CHECK(memcmp(before, after, n) == 0);
    free(after);
    free(before);
    CHECK_EQ_U64(inst->exe_loaded, 0u);
    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_load_exe(inst, b, HEADER + 0x800u), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(inst->exe_loaded, 1u);
    schway_destroy(inst);
    free(b);
}

/* The debug-port bit is stored, unknown bits are refused, and an older
   caller's profile clears it. */
static void case_profile_flags(void)
{
    schway *inst = make();
    uint32_t cycles = 0u;
    schway_profile p;
    memset(&p, 0, sizeof p);
    p.size = (uint32_t)sizeof p;
    p.region = SCHWAY_REGION_NTSC_U;
    p.flags = SCHWAY_PROFILE_DEBUG_PORT;
    CHECK_EQ_U64(schway_set_profile(inst, &p), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(inst->profile_flags, SCHWAY_PROFILE_DEBUG_PORT);
    CHECK_EQ_U64(schway__bus_write(inst, 0x1F802080u, 1u, 'A', &cycles), 0);
    CHECK_EQ_U64(inst->tty.count, 1u);

    /* Unknown bits, alone or with the known one, change nothing. */
    static const uint32_t unknown[] = {2u, 3u, 0x80000000u, 0xFFFFFFFEu};
    for (size_t i = 0; i < sizeof unknown / sizeof unknown[0]; i++) {
        p.flags = unknown[i];
        p.region = SCHWAY_REGION_PAL;
        CHECK_EQ_U64(schway_set_profile(inst, &p), SCHWAY_STATUS_ERR_PROFILE_REJECTED);
        CHECK_EQ_U64(inst->profile_flags, SCHWAY_PROFILE_DEBUG_PORT);
        CHECK_EQ_U64(inst->region, SCHWAY_REGION_NTSC_U);
    }
    p.flags = SCHWAY_PROFILE_DEBUG_PORT;
    p.region = 9u;
    CHECK_EQ_U64(schway_set_profile(inst, &p), SCHWAY_STATUS_ERR_PROFILE_REJECTED);
    CHECK_EQ_U64(inst->profile_flags, SCHWAY_PROFILE_DEBUG_PORT);

    /* The flag survives a power reset. */
    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(inst->profile_flags, SCHWAY_PROFILE_DEBUG_PORT);

    /* Clear: the port is ignored. */
    p.flags = 0u;
    p.region = SCHWAY_REGION_NTSC_U;
    CHECK_EQ_U64(schway_set_profile(inst, &p), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(inst->profile_flags, 0u);
    CHECK_EQ_U64(schway__bus_write(inst, 0x1F802080u, 1u, 'B', &cycles), 0);
    CHECK_EQ_U64(inst->tty.count, 0u);

    /* A size tag that ends before flags says there are none, whatever the
       bytes past it hold. */
    p.flags = SCHWAY_PROFILE_DEBUG_PORT;
    CHECK_EQ_U64(schway_set_profile(inst, &p), SCHWAY_STATUS_OK);
    p.size = (uint32_t)offsetof(schway_profile, flags);
    p.flags = 0xFFu;
    p.region = SCHWAY_REGION_PAL;
    CHECK_EQ_U64(schway_set_profile(inst, &p), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(inst->profile_flags, 0u);
    CHECK_EQ_U64(inst->region, SCHWAY_REGION_PAL);

    /* A caller's newer struct with a non-zero tail is refused as before. */
    struct {
        schway_profile head;
        uint32_t later;
    } newer;
    memset(&newer, 0, sizeof newer);
    newer.head.size = (uint32_t)sizeof newer;
    newer.head.region = SCHWAY_REGION_NTSC_J;
    newer.later = 1u;
    CHECK_EQ_U64(schway_set_profile(inst, &newer.head), SCHWAY_STATUS_ERR_STRUCT_SIZE);

    /* After the first frame the profile is locked, flags included. */
    p.size = (uint32_t)sizeof p;
    p.flags = SCHWAY_PROFILE_DEBUG_PORT;
    p.region = SCHWAY_REGION_PAL;
    CHECK_EQ_U64(schway_run_frame(inst, NULL), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_set_profile(inst, &p), SCHWAY_STATUS_ERR_PROFILE_LOCKED);
    CHECK_EQ_U64(inst->profile_flags, 0u);
    schway_destroy(inst);
}

/* RFC 1321 appendix A.5: the test suite's seven messages and their digests,
   then the same eight-line message fed a byte at a time, which crosses the
   64-byte block boundary at every offset. */
static void case_md5_rfc1321(void)
{
    static const struct {
        const char *message;
        const char *digest;
    } suite[] = {
        {"", "d41d8cd98f00b204e9800998ecf8427e"},
        {"a", "0cc175b9c0f1b6a831c399e269772661"},
        {"abc", "900150983cd24fb0d6963f7d28e17f72"},
        {"message digest", "f96b697d7cb7938d525a2f31aaf161d0"},
        {"abcdefghijklmnopqrstuvwxyz", "c3fcd3d76192e4007dfb496cca67e13b"},
        {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
         "d174ab98d277d9f5a5611c2c9f419d9f"},
        {"1234567890123456789012345678901234567890123456789012345678901234567890"
         "1234567890",
         "57edf4a22be3c955ac49da2e2107b67a"},
    };
    for (size_t i = 0; i < sizeof suite / sizeof suite[0]; i++) {
        const char *m = suite[i].message;
        schway__md5 whole;
        schway__md5 bytes;
        uint8_t d[16];
        char hex[33];
        schway__md5_init(&whole);
        schway__md5_update(&whole, m, strlen(m));
        schway__md5_final(&whole, d);
        schway__md5_hex(d, hex);
        if (strcmp(hex, suite[i].digest) != 0) {
            fprintf(stderr, "%s:%d: md5(\"%s\") = %s, want %s\n", __FILE__, __LINE__, m, hex,
                    suite[i].digest);
            check_failures++;
        }
        schway__md5_init(&bytes);
        for (size_t k = 0; m[k] != '\0'; k++) {
            schway__md5_update(&bytes, m + k, 1u);
        }
        schway__md5_final(&bytes, d);
        schway__md5_hex(d, hex);
        CHECK(strcmp(hex, suite[i].digest) == 0);
    }
}

/* Writes n bytes to dir/name. */
static void write_file(const char *dir, const char *name, const uint8_t *b, size_t n)
{
    char path[1024];
    if (snprintf(path, sizeof path, "%s/%s", dir, name) >= (int)sizeof path) {
        exit(1);
    }
    FILE *f = fopen(path, "wb");
    if (f == NULL || (n != 0u && fwrite(b, 1u, n, f) != n) || fclose(f) != 0) {
        fprintf(stderr, "cannot write %s\n", path);
        exit(1);
    }
}

/* The files the runner's probe tests read, written into dir: one the
   validator accepts and five it refuses (wrong signature, a header cut short,
   a rejected entry point, no bytes, one byte more than the largest file). */
static void write_probe_files(const char *dir)
{
    struct hdr h = valid();
    h.licence = "Sony Computer Entertainment Inc. for Europe area";
    uint8_t *ok = build(&h, HEADER + 0x800u);
    write_file(dir, "valid.exe", ok, HEADER + 0x800u);

    ok[0] = 'X';
    write_file(dir, "bad_signature.exe", ok, HEADER + 0x800u);
    ok[0] = 'P';
    write_file(dir, "truncated.exe", ok, 100u);
    free(ok);

    h.pc0 = 0u;
    uint8_t *no_entry = build(&h, HEADER + 0x800u);
    write_file(dir, "no_entry.exe", no_entry, HEADER + 0x800u);
    free(no_entry);

    write_file(dir, "empty.exe", NULL, 0u);

    h = valid();
    size_t big = (size_t)HEADER + RAM + 1u;
    uint8_t *over = build(&h, big);
    write_file(dir, "oversized.exe", over, big);
    free(over);
}

typedef void (*case_fn)(void);

static const struct {
    const char *name;
    case_fn fn;
} cases[] = {
    {"probe_valid", case_probe_valid},
    {"rejects", case_rejects},
    {"boundaries", case_boundaries},
    {"lint", case_lint},
    {"licence", case_licence},
    {"load_writes", case_load_writes},
    {"error_changes_nothing", case_error_changes_nothing},
    {"after_first_frame", case_after_first_frame},
    {"profile_flags", case_profile_flags},
    {"md5_rfc1321", case_md5_rfc1321},
};

int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "write") == 0) {
        write_probe_files(argv[2]);
        return 0;
    }
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
