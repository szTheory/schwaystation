/* Each replay selects its bounded corpus at compile time and uses the same
   replay_main interface. */
#include "seeds.h"

#if defined(SCHWAY_FUZZ_CUE_SEEDS) || defined(SCHWAY_FUZZ_CNF_SEEDS) || \
    defined(SCHWAY_FUZZ_M3U_SEEDS) || defined(SCHWAY_FUZZ_CDROM_SEEDS) || \
    defined(SCHWAY_FUZZ_MDEC_SEEDS)
#include <string.h>

#if defined(SCHWAY_FUZZ_CUE_SEEDS)
static const char *const names[] = {"valid_track", "path_escape", "maximum_cue_text",
                                    "maximum_child_name", "oversized_child_name"};

uint32_t seeds_count(void)
{
    return (uint32_t)(sizeof names / sizeof names[0]);
}

const char *seeds_name(uint32_t i)
{
    return names[i];
}

size_t seeds_build(uint32_t i, uint8_t *buf)
{
    static const char valid[] = "FILE \"track.bin\" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 00:00:00\n";
    static const char escape[] = "FILE \"../outside.bin\" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 00:00:00\n";
    static const char prefix[] = "FILE \"";
    static const char suffix[] = "\" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 00:00:00\n";
    if (i == 0u) {
        memcpy(buf, valid, sizeof valid - 1u);
        return sizeof valid - 1u;
    }
    if (i == 1u) {
        memcpy(buf, escape, sizeof escape - 1u);
        return sizeof escape - 1u;
    }
    if (i == 2u) {
        memset(buf, '#', SEEDS_SIZE_MAX);
        buf[0] = '#';
        return SEEDS_SIZE_MAX;
    }
    size_t at = 0u;
    size_t name_length = i == 3u ? 255u : 256u;
    memcpy(buf + at, prefix, sizeof prefix - 1u);
    at += sizeof prefix - 1u;
    size_t base_length = name_length - 4u;
    memset(buf + at, 'a', base_length);
    at += base_length;
    memcpy(buf + at, ".bin", 4u);
    at += 4u;
    memcpy(buf + at, suffix, sizeof suffix - 1u);
    at += sizeof suffix - 1u;
    return at;
}
#elif defined(SCHWAY_FUZZ_CNF_SEEDS)
#include "disc_image.h"

static const char *const names[] = {"valid_limits", "unsafe_boot_path", "malformed_number",
                                    "parser_maximum", "harness_maximum"};

uint32_t seeds_count(void)
{
    return (uint32_t)(sizeof names / sizeof names[0]);
}

const char *seeds_name(uint32_t i)
{
    return names[i];
}

size_t seeds_build(uint32_t i, uint8_t *buf)
{
    static const char valid[] = "BOOT = cdrom:\\SLUS_123.45;1\r\nTCB = 64\r\nEVENT = 256\r\nSTACK = 801FFF00h\r\n";
    static const char escape[] = "BOOT = CDROM:\\..\\SLUS_123.45;1\n";
    static const char bad_number[] = "BOOT = cdrom:\\SLUS_123.45;1\nTCB = 4294967296\n";
    if (i == 0u || i == 1u || i == 2u) {
        const char *text = i == 0u ? valid : i == 1u ? escape : bad_number;
        size_t size = strlen(text);
        memcpy(buf, text, size);
        return size;
    }
    size_t size = i == 3u ? SCHWAY_DISC_SYSTEM_CNF_MAX_BYTES + 1u : SEEDS_SIZE_MAX;
    memset(buf, ' ', size);
    return size;
}
#elif defined(SCHWAY_FUZZ_M3U_SEEDS)
static const char *const names[] = {"single_disc", "path_escape", "cycle", "thirty_two_entries",
                                    "thirty_three_entries", "maximum_path", "oversized_path"};

uint32_t seeds_count(void)
{
    return (uint32_t)(sizeof names / sizeof names[0]);
}

const char *seeds_name(uint32_t i)
{
    return names[i];
}

size_t seeds_build(uint32_t i, uint8_t *buf)
{
    static const char *const fixed[] = {"disc.iso\n", "../outside.iso\n", "input.m3u\n"};
    if (i < 3u) {
        size_t size = strlen(fixed[i]);
        memcpy(buf, fixed[i], size);
        return size;
    }
    if (i == 3u || i == 4u) {
        uint32_t count = i == 3u ? 32u : 33u;
        size_t at = 0u;
        for (uint32_t n = 0u; n < count; n++) {
            memcpy(buf + at, "disc.iso\n", 9u);
            at += 9u;
        }
        return at;
    }
    size_t at = 0u;
    size_t name_length = i == 5u ? 255u : 256u;
    memset(buf, 'a', name_length - 4u);
    at += name_length - 4u;
    memcpy(buf + at, ".iso\n", 5u);
    at += 5u;
    return at;
}
#elif defined(SCHWAY_FUZZ_CDROM_SEEDS)
static const char *const names[] = {
    "parameter_fifo_overflow", "reserved_commands_00", "reserved_commands_01",
    "reserved_commands_02", "reserved_commands_03", "reserved_commands_04",
    "reserved_commands_05", "reserved_commands_06", "reserved_commands_07",
    "reserved_commands_08", "reserved_commands_09", "reserved_commands_10",
    "reserved_commands_11", "reserved_commands_12", "lid_reset_mix",
};

uint32_t seeds_count(void)
{
    return (uint32_t)(sizeof names / sizeof names[0]);
}

const char *seeds_name(uint32_t i)
{
    return names[i];
}

static size_t cdrom_record(uint8_t *buf, size_t at, uint8_t action, uint8_t a, uint8_t b, uint8_t c)
{
    buf[at] = action;
    buf[at + 1u] = a;
    buf[at + 2u] = b;
    buf[at + 3u] = c;
    return at + 4u;
}

size_t seeds_build(uint32_t i, uint8_t *buf)
{
    size_t at = 0u;
    if (i == 0u) {
        for (uint32_t n = 0u; n < 20u; n++) {
            at = cdrom_record(buf, at, 0u, 19u, (uint8_t)n, (uint8_t)~n);
        }
    } else if (i < 14u) {
        uint32_t first = (i - 1u) * 20u;
        uint32_t end = first + 20u;
        if (end > 256u) end = 256u;
        for (uint32_t command = first; command < end; command++) {
            at = cdrom_record(buf, at, 1u, (uint8_t)command, (uint8_t)(command * 3u),
                              (uint8_t)(command * 7u));
        }
    } else {
        static const uint8_t records[][4] = {
            {1u, 0x02u, 0x00u, 0x00u}, {1u, 0x06u, 0x00u, 0x00u}, {2u, 0xFFu, 0xFFu, 0u},
            {3u, 1u, 0u, 0u},          {1u, 0x1Fu, 0u, 0u},       {3u, 0u, 0u, 0u},
            {4u, 0u, 0u, 0u},          {5u, 0x20u, 0xAAu, 0x55u}, {6u, 0u, 0u, 0u},
        };
        memcpy(buf, records, sizeof records);
        at = sizeof records;
    }
    return at;
}
#elif defined(SCHWAY_FUZZ_MDEC_SEEDS)
static const char *const names[] = {"quant_table_truncated", "scale_table_truncated", "rle_run_overflow",
                                    "truncated_block", "fifo_refill", "eob_block"};

uint32_t seeds_count(void)
{
    return (uint32_t)(sizeof names / sizeof names[0]);
}

const char *seeds_name(uint32_t i)
{
    return names[i];
}

static size_t mdec_word(uint8_t *buf, size_t at, uint32_t word)
{
    buf[at] = (uint8_t)word;
    buf[at + 1u] = (uint8_t)(word >> 8);
    buf[at + 2u] = (uint8_t)(word >> 16);
    buf[at + 3u] = (uint8_t)(word >> 24);
    return at + 4u;
}

size_t seeds_build(uint32_t i, uint8_t *buf)
{
    size_t at = 0u;
    if (i == 0u) {
        at = mdec_word(buf, at, 0x40000001u);
        at = mdec_word(buf, at, UINT32_MAX);
    } else if (i == 1u) {
        at = mdec_word(buf, at, 0x60000000u);
        at = mdec_word(buf, at, UINT32_MAX);
    } else if (i == 2u) {
        at = mdec_word(buf, at, 0x20000001u);
        at = mdec_word(buf, at, 0xFC010001u);
    } else if (i == 3u) {
        at = mdec_word(buf, at, 0x20000001u);
        at = mdec_word(buf, at, 0x000000A0u);
    } else if (i == 4u) {
        at = mdec_word(buf, at, 0x2000FFFFu);
        for (uint32_t n = 0u; n < 64u; n++) {
            at = mdec_word(buf, at, n % 2u == 0u ? 0xFE0000A0u : UINT32_MAX);
        }
    } else {
        at = mdec_word(buf, at, 0x20000002u);
        at = mdec_word(buf, at, 0x000000A0u);
        at = mdec_word(buf, at, 0x0000FE00u);
    }
    return at;
}
#endif
#elif defined(SCHWAY_FUZZ_GPU_SEEDS)
static const uint8_t corpus[][4] = {{0u}, {1u}, {2u}, {3u}, {4u}};
static const char *const names[] = {"partial_packet", "wrap_transfer", "dma_packet", "span_extrema",
                                    "reset_instance"};
#elif defined(SCHWAY_FUZZ_SPU_SEEDS)
static const char *const names[] = {"fifo_wrap", "irqa_edges", "voice_noise", "reverb_edges",
                                    "dma_transfer"};
#elif defined(SCHWAY_FUZZ_SIO_SEEDS)
static const uint8_t corpus[][4] = {{0u}, {1u}, {2u}};
static const char *const names[] = {"scheduled_ack", "deselect_pending", "empty_fifo"};
#elif defined(SCHWAY_FUZZ_MEMCARD_SEEDS)
static const uint8_t corpus[][4] = {{0u}, {1u}, {2u}};
static const char *const names[] = {"staged_write", "bad_checksum", "abort_write"};
#endif

#if defined(SCHWAY_FUZZ_CUE_SEEDS) || defined(SCHWAY_FUZZ_CNF_SEEDS) || \
    defined(SCHWAY_FUZZ_M3U_SEEDS) || defined(SCHWAY_FUZZ_CDROM_SEEDS) || \
    defined(SCHWAY_FUZZ_MDEC_SEEDS)
/* The new campaign-specific seed interfaces are defined above. */
#elif defined(SCHWAY_FUZZ_SPU_SEEDS)
static size_t spu_record(uint8_t *buf, size_t at, uint8_t op, uint8_t selector, uint16_t value,
                         uint8_t extra)
{
    buf[at] = op;
    buf[at + 1u] = selector;
    buf[at + 2u] = (uint8_t)value;
    buf[at + 3u] = (uint8_t)(value >> 8);
    buf[at + 4u] = extra;
    return at + 5u;
}

uint32_t seeds_count(void)
{
    return (uint32_t)(sizeof names / sizeof names[0]);
}

const char *seeds_name(uint32_t i)
{
    return names[i];
}

size_t seeds_build(uint32_t i, uint8_t *buf)
{
    size_t n = 0u;
    if (i == 0u) {
        for (uint32_t k = 0u; k < 32u; k++) {
            n = spu_record(buf, n, 1u, 0u, (uint16_t)k, 0u);
        }
        for (uint32_t k = 0u; k < 24u; k++) {
            n = spu_record(buf, n, 2u, 0u, 0u, 0u);
        }
        for (uint32_t k = 0u; k < 24u; k++) {
            n = spu_record(buf, n, 1u, 0u, (uint16_t)(0x8000u + k), 0u);
        }
        for (uint32_t k = 0u; k < 33u; k++) {
            n = spu_record(buf, n, 2u, 0u, 0u, 0u);
        }
    } else if (i == 1u) {
        n = spu_record(buf, n, 0u, 0xD2u, 0xFFFFu, 0u);
        n = spu_record(buf, n, 0u, 0xD5u, 0x8050u, 0u);
        n = spu_record(buf, n, 1u, 0xFFu, 0xFFFFu, 1u);
        n = spu_record(buf, n, 4u, 0u, 0u, 3u);
    } else if (i == 2u) {
        n = spu_record(buf, n, 0u, 0x01u, 0x0FFFu, 0u);
        n = spu_record(buf, n, 0u, 0x02u, 0xFFFFu, 0u);
        n = spu_record(buf, n, 0u, 0x03u, 0xFFFFu, 0u);
        n = spu_record(buf, n, 0u, 0x04u, 0xFFFFu, 0u);
        n = spu_record(buf, n, 0u, 0xC4u, 0x0001u, 0u);
        n = spu_record(buf, n, 0u, 0xCAu, 0xFFFFu, 0u);
        n = spu_record(buf, n, 4u, 0u, 0u, 3u);
        n = spu_record(buf, n, 0u, 0xC6u, 0x0001u, 0u);
    } else if (i == 3u) {
        for (uint32_t k = 0u; k < 16u; k++) {
            n = spu_record(buf, n, 0u, (uint8_t)(0xE0u + k),
                           (uint16_t)((k & 1u) != 0u ? 0xFFFFu : 0u), 0u);
        }
        n = spu_record(buf, n, 0u, 0xD5u, 0xC000u, 0u);
        n = spu_record(buf, n, 4u, 0u, 0u, 3u);
    } else {
        n = spu_record(buf, n, 3u, 0xFFu, 0xFFFFu, 3u);
        n = spu_record(buf, n, 4u, 0x00u, 0u, 3u);
    }
    return n;
}
#elif defined(SCHWAY_FUZZ_GPU_SEEDS) || defined(SCHWAY_FUZZ_SIO_SEEDS) ||                          \
    defined(SCHWAY_FUZZ_MEMCARD_SEEDS)
uint32_t seeds_count(void)
{
    return (uint32_t)(sizeof corpus / sizeof corpus[0]);
}

const char *seeds_name(uint32_t i)
{
    return names[i];
}

size_t seeds_build(uint32_t i, uint8_t *buf)
{
    buf[0] = corpus[i][0];
    return 1u;
}
#else
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
    CODE_TIMERS,  /* arm the counters at target 0 with their interrupts on and no handler, then spin
                   */
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
    {"stack_at_ram_end", TEXT, TEXT, 0x800u, STACK_TOP - 0x10u, 0x10u, HEADER + 0x800u, CODE_SPIN},
    {"stack_over_ram", TEXT, TEXT, 0x800u, STACK_TOP - 0x10u, 0x11u, HEADER + 0x800u, CODE_SPIN},
    {"stack_kseg2", TEXT, TEXT, 0x800u, 0xC0000000u, 0x10u, HEADER + 0x800u, CODE_SPIN},
    {"stack_size_wraps", TEXT, TEXT, 0x800u, 0x80000010u, 0xFFFFFFF0u, HEADER + 0x800u, CODE_SPIN},
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
    0xBF801000u, 0xBF801040u, 0xBF801070u, 0xBF801080u, 0xBF801100u, 0xBF801800u, 0xBF801810u,
    0xBF801820u, 0xBF801C00u, 0xBFC00000u, 0xBF000000u, 0xBF802000u, 0xBFA00000u,
};
static const uint32_t gap_windows[] = {
    0xBF800400u, 0xBF803000u, 0xBF900000u, 0xBFB00000u,
    0xBFC80000u, 0xBFFFF000u, 0xC0000000u, 0xFFFE0000u,
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
        {2u, 2u, 0x00100000u, 0u, 0x01000401u},          {2u, 2u, 0x1F800000u, 0u, 0x01000401u},
        {2u, 2u, 0x1FC00000u, 0u, 0x01000401u},          {2u, 2u, 0x1FA00000u, 0u, 0x01000401u},
        {6u, 6u, 0x00FFFFFCu, 0u, 0x11000000u},          {6u, 6u, 0x00100000u, 0x10u, 0x11770703u},
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
        count = emit_hammer(words, s->pc0, gap_windows, sizeof gap_windows / sizeof gap_windows[0]);
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

#endif
