/* The loader's seed inputs (seeds.h), each a table row built by seed_file.
   Each row sits on one edge of the header rules in src/exe.c: a field one
   below or one above a bound, a segment that wraps or leaves RAM, or code that
   hammers the addresses the bus does not decode. The names are stable: they
   become file names in the build directory. */
#include "seeds.h"

#include <string.h>

#define HEADER 0x800u
#define RAM 0x200000u
#define TEXT 0x80010000u
#define STACK_TOP 0x80200000u

/* What the first words of the text segment hold. */
enum code {
    CODE_NONE,    /* zero bytes: no-operations */
    CODE_SPIN,    /* a jump to itself */
    CODE_DEVICES, /* one read and write of each device window, then a jump back to the start */
    CODE_GAPS,    /* the same on addresses no device decodes */
    CODE_DMA,     /* arm DMA channels with hostile addresses and counts, then spin */
    CODE_TIMERS,  /* arm the counters at target 0 with their interrupts on and no handler, then spin */
    CODE_GTE      /* enable the coprocessor and write all ones to every register, then spin */
};

struct seed {
    const char *name;
    uint32_t pc0, t_addr, t_size, s_addr, s_size;
    uint32_t n; /* the file's length */
    enum code code;
};

/* Fields: pc0, t_addr, t_size, s_addr, s_size, n, code. A file with a whole
   sector of text is HEADER + 0800h bytes; the largest the loader accepts is
   HEADER + RAM. */
static const struct seed seeds[] = {
    {"valid", TEXT, TEXT, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_SPIN},
    {"valid_stack", TEXT, TEXT, 0x800u, STACK_TOP - 0x10u, 0x10u, HEADER + 0x800u, CODE_SPIN},
    {"hammer_devices", TEXT, TEXT, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_DEVICES},
    {"hammer_gaps", TEXT, TEXT, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_GAPS},
    {"text_kuseg", 0x00010000u, 0x00010000u, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_SPIN},
    {"text_kseg1", 0xA0010000u, 0xA0010000u, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_SPIN},
    {"text_low_64k", 0x80000000u, 0x80000000u, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_SPIN},
    {"text_last_sector", 0x801FF800u, 0x801FF800u, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_SPIN},
    {"text_at_ram_end", TEXT, 0x80200000u, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_SPIN},
    {"text_ends_at_wrap", TEXT, 0xFFFFF800u, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_SPIN},
    {"text_size_wraps", TEXT, 0x80000800u, 0xFFFFF800u, 0u, 0u, HEADER + 0x800u, CODE_SPIN},
    {"text_kseg2", TEXT, 0xC0000000u, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_SPIN},
    {"text_unaligned", TEXT, TEXT + 2u, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_SPIN},
    {"text_one_below_ram", TEXT, 0x80000800u, RAM - 0x800u, 0u, 0u, HEADER + RAM - 0x800u,
     CODE_SPIN},
    {"text_one_over_ram", TEXT, 0x80000800u, RAM, 0u, 0u, HEADER + RAM, CODE_SPIN},
    {"text_largest", TEXT, 0x80000000u, RAM, 0u, 0u, HEADER + RAM, CODE_SPIN},
    {"text_size_below_sector", TEXT, TEXT, 0x7FFu, 0u, 0u, HEADER + 0x1000u, CODE_SPIN},
    {"text_size_above_sector", TEXT, TEXT, 0x801u, 0u, 0u, HEADER + 0x1000u, CODE_SPIN},
    {"text_size_past_file", TEXT, TEXT, 0x1000u, 0u, 0u, HEADER + 0x800u, CODE_SPIN},
    {"text_size_zero", TEXT, TEXT, 0u, 0u, 0u, HEADER + 0x800u, CODE_NONE},
    {"file_below_header", TEXT, TEXT, 0u, 0u, 0u, HEADER - 1u, CODE_NONE},
    {"file_header_only", TEXT, TEXT, 0u, 0u, 0u, HEADER, CODE_NONE},
    {"file_one_over_header", TEXT, TEXT, 0u, 0u, 0u, HEADER + 1u, CODE_NONE},
    {"file_one_over_largest", TEXT, 0x80000000u, RAM, 0u, 0u, HEADER + RAM + 1u, CODE_SPIN},
    {"pc0_zero", 0u, TEXT, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_SPIN},
    {"pc0_unaligned", TEXT + 2u, TEXT, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_SPIN},
    {"pc0_unmapped", 0xBFA00000u, TEXT, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_SPIN},
    {"pc0_kseg2", 0xFFFE0000u, TEXT, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_SPIN},
    {"stack_at_ram_end", TEXT, TEXT, 0x800u, STACK_TOP - 0x10u, 0x10u, HEADER + 0x800u,
     CODE_SPIN},
    {"stack_over_ram", TEXT, TEXT, 0x800u, STACK_TOP - 0x10u, 0x11u, HEADER + 0x800u, CODE_SPIN},
    {"stack_kseg2", TEXT, TEXT, 0x800u, 0xC0000000u, 0x10u, HEADER + 0x800u, CODE_SPIN},
    {"stack_size_wraps", TEXT, TEXT, 0x800u, 0x80000010u, 0xFFFFFFF0u, HEADER + 0x800u,
     CODE_SPIN},
    {"dma_self_ref_list", TEXT, TEXT, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_DMA},
    {"dma_list_scratch", TEXT, TEXT, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_DMA},
    {"dma_list_bios", TEXT, TEXT, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_DMA},
    {"dma_list_unmapped", TEXT, TEXT, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_DMA},
    {"dma_otc_bcr_zero_wrap", TEXT, TEXT, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_DMA},
    {"dma_otc_ignored_bits", TEXT, TEXT, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_DMA},
    {"dma_bcr_all_ones", TEXT, TEXT, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_DMA},
    {"timer_storm", TEXT, TEXT, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_TIMERS},
    {"gte_all_ones", TEXT, TEXT, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_GTE},
    {"gte_every_command", TEXT, TEXT, 0x800u, 0u, 0u, HEADER + 0x800u, CODE_GTE},
};

#define SEED_COUNT ((uint32_t)(sizeof seeds / sizeof seeds[0]))

/* The R3000A's encodings (MIPS I): the opcode in the top six bits, then rs,
   rt and a 16-bit immediate. Each emitted word is named by its mnemonic. */
#define R_T0 8u
#define R_T1 9u

static uint32_t op_i(uint32_t op, uint32_t rs, uint32_t rt, uint32_t imm)
{
    return (op << 26) | (rs << 21) | (rt << 16) | (imm & 0xFFFFu);
}

static uint32_t op_lui(uint32_t rt, uint32_t imm)
{
    return op_i(0x0Fu, 0u, rt, imm);
}

static uint32_t op_j(uint32_t target)
{
    return (0x02u << 26) | ((target >> 2) & 0x03FFFFFFu);
}

/* Windows the CPU can reach as KSEG1 addresses. The first group are the
   devices the map lists (psx-spx, Memory Map: the I/O ports, the BIOS and
   expansion regions 1 to 3), whether or not this core has built them, the
   expansion regions last because the first read there is a fault; the
   second, addresses no device decodes: the scratchpad's gap, the I/O page's
   tail, the space between expansion 3 and the BIOS, past the BIOS, the top of
   the physical window, the mirror past it and KSEG2. */
static const uint32_t device_windows[] = {
    0xBF801000u, 0xBF801040u, 0xBF801070u, 0xBF801080u, 0xBF801100u, 0xBF801800u,
    0xBF801810u, 0xBF801820u, 0xBF801C00u, 0xBFC00000u, 0xBF000000u, 0xBF802000u,
    0xBFA00000u,
};
static const uint32_t gap_windows[] = {
    0xBF800400u, 0xBF803000u, 0xBF900000u, 0xBFB00000u, 0xBFC80000u,
    0xBFFFF000u, 0xC0000000u, 0xFFFE0000u,
};

/* Emits, for each window, a lui and then a read and a write of every width at
   its first word (aligned, so no access faults for being misaligned), and
   ends with a jump back to start and its delay slot. */
static size_t emit_hammer(uint32_t *w, uint32_t start, const uint32_t *windows, size_t count)
{
    static const uint32_t loads[] = {0x23u, 0x21u, 0x20u};  /* lw, lh, lb */
    static const uint32_t stores[] = {0x2Bu, 0x29u, 0x28u}; /* sw, sh, sb */
    size_t n = 0;
    for (size_t i = 0; i < count; i++) {
        w[n++] = op_lui(R_T0, windows[i] >> 16);
        for (size_t k = 0; k < 3u; k++) {
            w[n++] = op_i(loads[k], R_T0, R_T1, windows[i] & 0xFFFFu);
            w[n++] = op_i(stores[k], R_T0, R_T1, windows[i] & 0xFFFFu);
        }
    }
    w[n++] = op_j(start);
    w[n++] = 0u; /* sll zero, zero, 0 (nop) in the delay slot */
    return n;
}

/* The coprocessor move encodings (MIPS I): mtc0 is op 10h, rs 4; mtc2 is op 12h,
   rs 4 and ctc2 rs 6, with rt the CPU register and rd the register moved. */
static uint32_t op_cop(uint32_t op, uint32_t rs, uint32_t rt, uint32_t rd)
{
    return (op << 26) | (rs << 21) | (rt << 16) | (rd << 11);
}

/* Stores a word at an address whose low half is below 8000h: lui, lui, ori, sw. */
static size_t emit_sw(uint32_t *w, size_t n, uint32_t addr, uint32_t value)
{
    w[n++] = op_lui(R_T0, addr >> 16);
    w[n++] = op_lui(R_T1, value >> 16);
    w[n++] = op_i(0x0Du, R_T1, R_T1, value);
    w[n++] = op_i(0x2Bu, R_T0, R_T1, addr);
    return n;
}

/* A jump to itself and its delay slot, so the program ends in a spin. */
static size_t emit_spin(uint32_t *w, size_t n, uint32_t start)
{
    w[n] = op_j(start + 4u * (uint32_t)n);
    w[n + 1u] = 0u;
    return n + 2u;
}

/* DPCR with every channel enabled, then one row of first channel, last
   channel, MADR, BCR and CHCR (psx-spx, DMA Channels): a linked list into a
   node that points at itself, into the scratchpad, the BIOS and unmapped
   space; OTC with BCR 0 from the top of RAM (10000h words wrap past 0); OTC
   with every bit it ignores set; and every channel with BCR all ones. */
static size_t emit_dma(uint32_t *w, uint32_t start, uint32_t variant)
{
    static const uint32_t rows[][5] = {
        {2u, 2u, 0x00100000u, 0u, 0x01000401u}, {2u, 2u, 0x1F800000u, 0u, 0x01000401u},
        {2u, 2u, 0x1FC00000u, 0u, 0x01000401u}, {2u, 2u, 0x1FA00000u, 0u, 0x01000401u},
        {6u, 6u, 0x00FFFFFCu, 0u, 0x11000000u}, {6u, 6u, 0x00100000u, 0x10u, 0x11770703u},
        {0u, 6u, 0x00100000u, 0xFFFFFFFFu, 0x11000001u},
    };
    const uint32_t *r = rows[variant];
    size_t n = emit_sw(w, 0u, 0xBF8010F0u, 0x08888888u);
    if (variant == 0u) {
        n = emit_sw(w, n, 0xA0100000u, 0x00100000u); /* the node: no words, next is itself */
    }
    for (uint32_t ch = r[0]; ch <= r[1]; ch++) {
        uint32_t base = 0xBF801080u + 0x10u * ch;
        n = emit_sw(w, n, base, r[2]);
        n = emit_sw(w, n, base + 4u, r[3]);
        n = emit_sw(w, n, base + 8u, r[4]);
    }
    return emit_spin(w, n, start);
}

/* Each counter at target 0 on the system clock, MODE 78h (reset at target,
   interrupt at target and at overflow, repeat), I_MASK for the three, then SR
   with IEc and IM2 so the raise reaches the CPU with no handler installed
   (psx-spx, Timers; Interrupt Control). */
static size_t emit_timers(uint32_t *w, uint32_t start)
{
    size_t n = 0;
    for (uint32_t t = 0; t < 3u; t++) {
        n = emit_sw(w, n, 0xBF801108u + 0x10u * t, 0u);
        n = emit_sw(w, n, 0xBF801104u + 0x10u * t, 0x78u);
    }
    n = emit_sw(w, n, 0xBF801074u, 0x70u);
    w[n++] = op_i(0x0Du, 0u, R_T1, 0x401u);
    w[n++] = op_cop(0x10u, 4u, R_T1, 12u);
    return emit_spin(w, n, start);
}

/* SR.CU2, then FFFFFFFFh by MTC2 and CTC2 to each of the 32 data and 32
   control registers, and for the second variant every command number with sf
   and lm both ways (the command word is 4A000000h, sf bit 19, lm bit 10). */
static size_t emit_gte(uint32_t *w, uint32_t start, uint32_t variant)
{
    size_t n = 0;
    w[n++] = op_lui(R_T1, 0x4000u);
    w[n++] = op_cop(0x10u, 4u, R_T1, 12u);
    w[n++] = op_i(0x09u, 0u, R_T0, 0xFFFFu);
    for (uint32_t r = 0; r < 32u; r++) {
        w[n++] = op_cop(0x12u, 4u, R_T0, r);
        w[n++] = op_cop(0x12u, 6u, R_T0, r);
    }
    for (uint32_t k = 0; variant != 0u && k < 256u; k++) {
        w[n++] = 0x4A000000u | ((k & 1u) << 19) | (((k >> 1) & 1u) << 10) | (k >> 2);
    }
    return emit_spin(w, n, start);
}

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

uint32_t seeds_count(void)
{
    return SEED_COUNT;
}

const char *seeds_name(uint32_t i)
{
    return seeds[i].name;
}

/* The header (psx-spx, PSX Executable Formats: the signature, then pc0, gp0,
   the text, data and bss spans and the stack at 10h to 37h), then text bytes
   cut off where the file ends. */
size_t seeds_build(uint32_t i, uint8_t *buf)
{
    const struct seed *s = &seeds[i];
    uint32_t words[512];
    size_t count = 0;
    uint32_t variant = 0; /* this row's place among the rows of its code kind */
    for (uint32_t k = 0; k < i; k++) {
        variant += seeds[k].code == s->code;
    }
    memset(buf, 0, SEEDS_SIZE_MAX);
    memcpy(buf, "PS-X EXE", 8);
    put32(buf + 0x10, s->pc0);
    put32(buf + 0x14, 0x80080000u);
    put32(buf + 0x18, s->t_addr);
    put32(buf + 0x1C, s->t_size);
    put32(buf + 0x30, s->s_addr);
    put32(buf + 0x34, s->s_size);
    if (s->code == CODE_SPIN) {
        words[count++] = op_j(s->pc0);
        words[count++] = 0u;
    } else if (s->code == CODE_DEVICES) {
        count = emit_hammer(words, s->pc0, device_windows,
                           sizeof device_windows / sizeof device_windows[0]);
    } else if (s->code == CODE_GAPS) {
        count = emit_hammer(words, s->pc0, gap_windows,
                           sizeof gap_windows / sizeof gap_windows[0]);
    } else if (s->code == CODE_DMA) {
        count = emit_dma(words, s->pc0, variant);
    } else if (s->code == CODE_TIMERS) {
        count = emit_timers(words, s->pc0);
    } else if (s->code == CODE_GTE) {
        count = emit_gte(words, s->pc0, variant);
    }
    for (size_t k = 0; k < count && HEADER + 4u * (k + 1u) <= s->n; k++) {
        put32(buf + HEADER + 4u * k, words[k]);
    }
    return s->n;
}
