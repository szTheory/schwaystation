/* MDEC bus, block and status behavior through the instance-owned device. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

#define MDEC_STATUS 0x1F801824u
#define MDEC_DATA 0x1F801820u
#define DMA_BASE 0x1F801080u
#define DMA_DPCR 0x1F8010F0u

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

static uint32_t read32(schway *inst, uint32_t address)
{
    uint32_t cycles = 0u;
    uint32_t value = 0u;
    schway__sched_sync(inst);
    CHECK_EQ_U64(schway__bus_read(inst, address, 4u, &value, &cycles), 0u);
    return value;
}

static void write32(schway *inst, uint32_t address, uint32_t value, uint32_t cycles)
{
    schway__sched_sync(inst);
    CHECK_EQ_U64(schway__bus_write(inst, address, 4u, value, &cycles), 0u);
}

static void write_ram(schway *inst, uint32_t address, uint32_t value)
{
    CHECK_EQ_U64(schway__bus_ram_word(inst, address, &value, 1u), 0u);
}

static void run_cycles(schway *inst, uint64_t cycles)
{
    schway_budget budget;
    memset(&budget, 0, sizeof budget);
    budget.size = (uint32_t)sizeof budget;
    budget.max_cycles = cycles;
    schway_run_result result;
    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    CHECK_EQ_U64(schway_run(inst, &budget, &result), SCHWAY_STATUS_OK);
}

static void enable_dma(schway *inst);
static void start_dma0(schway *inst, uint32_t base, uint32_t words);
static void load_scale_table(schway *inst, const int16_t scale[SCHWAY__MDEC_BLOCK_SAMPLES]);

static void load_guest_quant(schway *inst, const uint8_t quant[SCHWAY__MDEC_BLOCK_SAMPLES])
{
    write32(inst, MDEC_DATA, 0x40000000u, 0u);
    for (uint32_t i = 0u; i < SCHWAY__MDEC_BLOCK_SAMPLES / 4u; i++) {
        uint32_t word = quant[i * 4u] | ((uint32_t)quant[i * 4u + 1u] << 8) |
                        ((uint32_t)quant[i * 4u + 2u] << 16) |
                        ((uint32_t)quant[i * 4u + 3u] << 24);
        write32(inst, MDEC_DATA, word, 0u);
    }
}

static void case_guest_input(void)
{
    /* Captured from host RAM at 0x12000 while running the digest-pinned
       ps1-tests mdec/4bit/4bit.exe guest (build-158, commit
       f727802fead11f1daa7549285548392ef87749cb, SHA-256
       c9fc7caab7c8bcd19ec96cd703309fd176ca0ac266ab927d0c3fecc32f811944).
       It contains a DC plus 63 AC values and ends at coefficient 63, without
       an explicit FE00 EOB halfword. */
    static const uint32_t guest_input[32] = {
        0x0074041Eu, 0x039300BDu, 0x032B0024u, 0x03F90015u, 0x03DA03E9u, 0x03FE03EEu,
        0x003F002Bu, 0x03FE03C0u, 0x03FE03BEu, 0x03FD03CAu, 0x03F303F3u, 0x000703FDu,
        0x000003D7u, 0x03F503E0u, 0x00020005u, 0x000703ECu, 0x0008000Cu, 0x03F703FEu,
        0x03F903FFu, 0x03DD0001u, 0x000903D8u, 0x00010016u, 0x03CA0013u, 0x03FA0004u,
        0x03FE03FCu, 0x00090000u, 0x0006000Fu, 0x03FC03FEu, 0x03FA03F2u, 0x000803F6u,
        0x001503FAu, 0x03F50001u,
    };
    static const uint8_t guest_quant[64] = {
        0x02u, 0x10u, 0x10u, 0x13u, 0x10u, 0x13u, 0x16u, 0x16u,
        0x16u, 0x16u, 0x16u, 0x16u, 0x1Au, 0x18u, 0x1Au, 0x1Bu,
        0x1Bu, 0x1Bu, 0x1Au, 0x1Au, 0x1Au, 0x1Au, 0x1Bu, 0x1Bu,
        0x1Bu, 0x1Du, 0x1Du, 0x1Du, 0x22u, 0x22u, 0x22u, 0x1Du,
        0x1Du, 0x1Du, 0x1Bu, 0x1Bu, 0x1Du, 0x1Du, 0x20u, 0x20u,
        0x22u, 0x22u, 0x25u, 0x26u, 0x25u, 0x23u, 0x23u, 0x22u,
        0x23u, 0x26u, 0x26u, 0x28u, 0x28u, 0x28u, 0x30u, 0x30u,
        0x2Eu, 0x2Eu, 0x38u, 0x38u, 0x3Au, 0x45u, 0x45u, 0x53u,
    };
    static const int16_t guest_scale[SCHWAY__MDEC_BLOCK_SAMPLES] = {
        0x5A82, 0x5A82, 0x5A82, 0x5A82, 0x5A82, 0x5A82, 0x5A82, 0x5A82,
        0x7D8A, 0x6A6D, 0x471C, 0x18F8, (int16_t)0xE707, (int16_t)0xB8E3,
        (int16_t)0x9592, (int16_t)0x8275, 0x7641, 0x30FB, (int16_t)0xCF04,
        (int16_t)0x89BE, (int16_t)0x89BE, (int16_t)0xCF04, 0x30FB, 0x7641,
        0x6A6D, (int16_t)0xE707, (int16_t)0x8275, (int16_t)0xB8E3, 0x471C,
        0x7D8A, 0x18F8, (int16_t)0x9592, 0x5A82, (int16_t)0xA57D,
        (int16_t)0xA57D, 0x5A82, 0x5A82, (int16_t)0xA57D, (int16_t)0xA57D,
        0x5A82, 0x471C, (int16_t)0x8275, 0x18F8, 0x6A6D, (int16_t)0x9592,
        (int16_t)0xE707, 0x7D8A, (int16_t)0xB8E3, 0x30FB, (int16_t)0x89BE,
        0x7641, (int16_t)0xCF04, (int16_t)0xCF04, 0x7641, (int16_t)0x89BE,
        0x30FB, 0x18F8, (int16_t)0xB8E3, 0x6A6D, (int16_t)0x8275, 0x7D8A,
        (int16_t)0x9592, 0x471C, (int16_t)0xE707,
    };
    /* Pinned RAM expectations from psx.log members: 4-bit SHA-256
       58c470e75f321ad361ee6b4600450970fa1f1f063d055b8501863438f56c7e75,
       8-bit SHA-256 d8b19a8e8cb23d65c0f267e148097d1d78a19400d180173791a1ce0e23cdb7d9
       (pins.txt). These bytes are independent of the decoder under test. */
    static const uint32_t expected[2][16] = {
        {0x00FF0FF0u, 0x0FFFFFFDu, 0x0FFFFFEDu, 0x0EFFFFFBu, 0x00FFFFF0u,
         0x021FFF00u, 0x0230F231u, 0x11333441u},
        {0x00FFFF00u, 0x0004FFFFu, 0xEDEFECC9u, 0x00F2FCEF,
         0xE8FADBD5u, 0x00FFE8FEu, 0xEFECF3B7u, 0x00E3FFEBu,
         0xF5FFFB00u, 0x0003FFF2u, 0xFCFF0500u, 0x001A08FFu,
         0xFF1E280Fu, 0x00232A05u, 0x29403810u, 0x0F163232u},
    };
    for (uint32_t pixel = 0u; pixel < 64u; pixel++) {
        uint32_t byte = (expected[1][pixel / 4u] >> ((pixel % 4u) * 8u)) & 0xFFu;
        uint32_t nearest = (byte + 8u) >> 4u;
        if (nearest > 15u) {
            nearest = 15u;
        }
        uint32_t nibble = (expected[0][pixel / 8u] >> ((pixel % 8u) * 4u)) & 0x0Fu;
        CHECK_EQ_U64(nibble, nearest);
    }
    for (uint32_t depth = 0u; depth < 2u; depth++) {
        schway *inst = make();
        enable_dma(inst);
        load_guest_quant(inst, guest_quant);
        load_scale_table(inst, guest_scale);
        write32(inst, MDEC_DATA, 0x20000020u | (depth << 27), 0u);
        for (uint32_t i = 0u; i < 32u; i++) {
            write_ram(inst, 0x12000u + i * 4u, guest_input[i]);
        }
        start_dma0(inst, 0x12000u, 32u);
        run_cycles(inst, 4096u);
        uint32_t output_words = depth == 0u ? 8u : 16u;
        CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
        CHECK_EQ_HEX(inst->dma.madr[0], 0x12080u);
        CHECK_EQ_U64(inst->mdec.output_count, output_words);
        for (uint32_t i = 0u; i < output_words; i++) {
            CHECK_EQ_HEX(inst->mdec.output[i], expected[depth][i]);
        }
        CHECK_EQ_U64(inst->dma.dreq & (1u << 1), 1u << 1);
        CHECK_EQ_HEX(read32(inst, MDEC_STATUS) & (1u << 27), 1u << 27);
        schway_destroy(inst);
    }
}

static void case_status(void)
{
    schway *inst = make();
    CHECK_EQ_HEX(read32(inst, MDEC_STATUS), 0x80040000u);
    CHECK_EQ_U64(inst->bus.unbuilt_n[SCHWAY__DEV_MDEC], 0u);
    write32(inst, MDEC_STATUS, 0x60000000u, 0u);
    write32(inst, MDEC_DATA, 0x24000001u, 0u);
    uint32_t status = read32(inst, MDEC_STATUS);
    CHECK((status & (1u << 28)) != 0u);
    CHECK((status & (1u << 27)) == 0u);
    CHECK((status & (1u << 29)) != 0u);
    write32(inst, MDEC_STATUS, 0u, 0u);
    status = read32(inst, MDEC_STATUS);
    CHECK((status & (1u << 28)) == 0u);
    CHECK((status & (1u << 29)) != 0u);
    write32(inst, MDEC_STATUS, 0x60000000u, 0u);
    CHECK((read32(inst, MDEC_STATUS) & (1u << 28)) != 0u);
    write32(inst, MDEC_STATUS, 0x80000000u, 0u);
    CHECK_EQ_HEX(read32(inst, MDEC_STATUS), 0x80040000u);

    /* MDEC(0) mirrors command bits 25-28 and 0-15 directly to STAT. */
    write32(inst, MDEC_DATA, 0x2F000000u, 0u);
    status = read32(inst, MDEC_STATUS);
    CHECK((status & (0x0Fu << 23)) != 0u);
    write32(inst, MDEC_DATA, 0x002003DDu, 0u);
    status = read32(inst, MDEC_STATUS);
    CHECK_EQ_HEX(status & (0x0Fu << 23), 0u);
    CHECK_EQ_HEX(status & 0xFFFFu, 0x03DDu);
    write32(inst, MDEC_DATA, 0x2F000000u, 0u);
    write32(inst, MDEC_DATA, 0x800003DDu, 0u);
    status = read32(inst, MDEC_STATUS);
    CHECK_EQ_HEX(status & (0x0Fu << 23), 0u);
    CHECK_EQ_HEX(status & 0xFFFFu, 0x03DDu);
    schway_destroy(inst);
}

static void enable_dma(schway *inst)
{
    write32(inst, MDEC_STATUS, 0x60000000u, 0u);
    write32(inst, DMA_DPCR, 0x0FEDCBA9u, 0u);
}

static void start_dma0(schway *inst, uint32_t base, uint32_t words)
{
    write32(inst, DMA_BASE, base, 0u);
    write32(inst, DMA_BASE + 4u, (1u << 16) | words, 0u);
    write32(inst, DMA_BASE + 8u, 0x01000201u, 0u);
    CHECK_EQ_U64(inst->dma.cur, 0u);
    schway__dma_event(inst, inst->dma.busy_until);
    if (inst->ticks < inst->dma.busy_until) {
        run_cycles(inst, inst->dma.busy_until - inst->ticks);
    }
}

static void dma1_transfer(schway *inst, uint32_t base, uint32_t words)
{
    write32(inst, DMA_BASE + 0x10u, base, 0u);
    write32(inst, DMA_BASE + 0x14u, (1u << 16) | words, 0u);
    write32(inst, DMA_BASE + 0x18u, 0x01000200u, 0u);
    CHECK_EQ_U64(inst->dma.cur, 1u);
    uint32_t limit = words * 4u + 32u;
    for (uint32_t i = 0u; i < limit && inst->dma.cur != SCHWAY__DMA_NONE; i++) {
        schway__dma_event(inst, inst->dma.busy_until);
    }
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    if (inst->ticks < inst->dma.busy_until) {
        run_cycles(inst, inst->dma.busy_until - inst->ticks);
    }
}

static uint64_t raw_output_hash(const schway *inst, uint32_t words)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    for (uint32_t i = 0u; i < words; i++) {
        uint32_t word = inst->mdec.output[(inst->mdec.output_head + i) %
                                           SCHWAY__MDEC_OUTPUT_WORDS];
        for (uint32_t byte = 0u; byte < 4u; byte++) {
            hash = (hash ^ (uint8_t)(word >> (byte * 8u))) * UINT64_C(1099511628211);
        }
    }
    return hash;
}

static void load_flat_tables(schway *inst)
{
    write32(inst, MDEC_DATA, 0x40000001u, 0u);
    for (uint32_t i = 0u; i < 32u; i++) {
        write32(inst, MDEC_DATA, 0x01010101u, 0u);
    }
    for (uint32_t i = 0u; i < SCHWAY__MDEC_TABLE_BYTES; i++) {
        CHECK_EQ_U64(inst->mdec.quant[i], 1u);
    }
    write32(inst, MDEC_DATA, 0x60000000u, 0u);
    for (uint32_t i = 0u; i < 32u; i++) {
        write32(inst, MDEC_DATA, 0x40004000u, 0u);
    }
    for (uint32_t i = 0u; i < SCHWAY__MDEC_BLOCK_SAMPLES; i++) {
        CHECK_EQ_U64(inst->mdec.scale[i], 0x4000u);
    }
}

static void load_scale_table(schway *inst, const int16_t scale[SCHWAY__MDEC_BLOCK_SAMPLES])
{
    write32(inst, MDEC_DATA, 0x60000000u, 0u);
    for (uint32_t i = 0u; i < SCHWAY__MDEC_BLOCK_SAMPLES / 2u; i++) {
        uint32_t word = (uint16_t)scale[i * 2u] |
                        ((uint32_t)(uint16_t)scale[i * 2u + 1u] << 16);
        write32(inst, MDEC_DATA, word, 0u);
    }
}

static void decode_words(schway *inst, uint32_t command, const uint32_t *input,
                         uint32_t input_words, uint32_t output_words)
{
    write32(inst, MDEC_DATA, command, 0u);
    for (uint32_t i = 0u; i < input_words; i++) {
        write32(inst, MDEC_DATA, input[i], 0u);
    }
    run_cycles(inst, 1000u);
    CHECK_EQ_U64(inst->mdec.output_count, output_words);
}

static void case_vectors(void)
{
    static const int16_t scale_columns[8] = {0x7FFF, 0x6000, 0x4000, 0x2000,
                                             0x1000, 0, 0, 0};
    static const uint32_t packed4[8] = {0x00000122u, 0x00000111u, 0x00000011u,
                                        0u, 0u, 0u, 0u, 0u};
    static const uint32_t packed8[16] = {
        0x0B15202Au, 0x00000005u, 0x0810181Fu, 0x00000004u,
        0x050B1015u, 0x00000003u, 0x0305080Au, 0x00000001u,
        0x01030405u, 0x00000001u, 0u, 0u, 0u, 0u, 0u, 0u,
    };
    int16_t patterned[SCHWAY__MDEC_BLOCK_SAMPLES];
    for (uint32_t z = 0u; z < 8u; z++) {
        for (uint32_t x = 0u; x < 8u; x++) {
            patterned[x + z * 8u] = scale_columns[x];
        }
    }
    const uint32_t ac_input[2] = {0x004004A0u, 0x0000FE00u};
    for (uint32_t depth = 0u; depth < 2u; depth++) {
        schway *inst = make();
        enable_dma(inst);
        load_flat_tables(inst);
        load_scale_table(inst, patterned);
        decode_words(inst, 0x24000002u | (depth << 27), ac_input, 2u,
                     depth == 0u ? 8u : 16u);
        const uint32_t *expected = depth == 0u ? packed4 : packed8;
        uint32_t words = depth == 0u ? 8u : 16u;
        for (uint32_t i = 0u; i < words; i++) {
            CHECK_EQ_HEX(inst->mdec.output[i], expected[i]);
        }
        schway_destroy(inst);
    }

    int16_t saturating[SCHWAY__MDEC_BLOCK_SAMPLES];
    for (uint32_t i = 0u; i < SCHWAY__MDEC_BLOCK_SAMPLES; i++) {
        saturating[i] = 0x7FFF;
    }
    const uint32_t endpoints[2] = {0xFE0001FFu, 0xFE000200u};
    const uint32_t endpoint_words[2] = {0x7F7F7F7Fu, 0x80808080u};
    for (uint32_t i = 0u; i < 2u; i++) {
        schway *inst = make();
        enable_dma(inst);
        load_flat_tables(inst);
        load_scale_table(inst, saturating);
        decode_words(inst, 0x2C000001u, &endpoints[i], 1u, 16u);
        for (uint32_t word = 0u; word < 16u; word++) {
            CHECK_EQ_HEX(inst->mdec.output[word], endpoint_words[i]);
        }
        schway_destroy(inst);
    }

    schway *plain = make();
    schway *padded = make();
    enable_dma(plain);
    enable_dma(padded);
    load_flat_tables(plain);
    load_flat_tables(padded);
    const uint32_t plain_input[1] = {0xFE0000A0u};
    const uint32_t padded_input[2] = {0x00A0FE00u, 0xFE00FE00u};
    decode_words(plain, 0x24000001u, plain_input, 1u, 8u);
    decode_words(padded, 0x24000002u, padded_input, 2u, 8u);
    for (uint32_t i = 0u; i < 8u; i++) {
        CHECK_EQ_HEX(plain->mdec.output[i], 0x11111111u);
        CHECK_EQ_HEX(padded->mdec.output[i], plain->mdec.output[i]);
    }
    CHECK_EQ_HEX(raw_output_hash(padded, 8u), raw_output_hash(plain, 8u));
    schway_destroy(plain);
    schway_destroy(padded);
}

static void case_block(void)
{
    for (uint32_t depth = 0u; depth < 2u; depth++) {
        schway *inst = make();
        uint32_t output_words = depth == 0u ? 8u : 16u;
        uint32_t base = 0x100u + depth * 0x100u;
        write_ram(inst, base, 0xFE000000u);
        write32(inst, MDEC_STATUS, 0x60000000u, 0u);
        write32(inst, MDEC_DATA, 0x24000001u | (depth << 27), 0u);
        write32(inst, DMA_DPCR, 0x0FEDCBA9u, 0u);
        write32(inst, DMA_BASE, base, 0u);
        write32(inst, DMA_BASE + 4u, (1u << 16) | 1u, 0u);
        write32(inst, DMA_BASE + 8u, 0x01000201u, 0u);
        CHECK_EQ_U64(inst->dma.cur, 0u);
        schway__dma_event(inst, inst->dma.busy_until);
        CHECK_EQ_U64(inst->mdec.block_pending, 1u);
        uint64_t deadline = inst->mdec.block_deadline;
        CHECK(deadline >= 1000u);
        CHECK_EQ_U64(schway__mdec_read(inst, MDEC_STATUS, deadline - 1u) & 0x80000000u,
                     0x80000000u);
        CHECK_EQ_U64(schway__mdec_read(inst, MDEC_STATUS, deadline) & 0x80000000u, 0u);
        CHECK_EQ_U64(inst->mdec.output_count, output_words);
        for (uint32_t i = 0u; i < output_words; i++) {
            CHECK_EQ_U64(inst->mdec.output[i], 0u);
        }
        write32(inst, DMA_BASE + 0x10u, base + 0x400u, (uint32_t)deadline);
        write32(inst, DMA_BASE + 0x14u, (1u << 16) | output_words, (uint32_t)deadline);
        write32(inst, DMA_BASE + 0x18u, 0x01000200u, (uint32_t)deadline);
        CHECK_EQ_U64(inst->dma.cur, 1u);
        schway__dma_event(inst, inst->dma.busy_until);
        CHECK_EQ_U64(inst->mdec.output_count, 0u);
        CHECK_EQ_U64(inst->dma.madr[1], base + 0x400u + output_words * 4u);
        for (uint32_t i = 0u; i < output_words; i++) {
            uint32_t value = 1u;
            CHECK_EQ_U64(schway__bus_ram_word(inst, base + 0x400u + i * 4u, &value, 0u), 0u);
            CHECK_EQ_U64(value, 0u);
        }
        schway_destroy(inst);
    }
}

static void case_sign_and_tables(void)
{
    static const uint64_t hashes[2][2] = {
        {UINT64_C(0x188245F9DDF02DC5), UINT64_C(0x1D552C65238459C5)},
        {UINT64_C(0xC9ACF7938CF08925), UINT64_C(0xBD5E5BAD6A568925)},
    };
    for (uint32_t depth = 0u; depth < 2u; depth++) {
        for (uint32_t is_signed = 0u; is_signed < 2u; is_signed++) {
            schway *inst = make();
            enable_dma(inst);
            load_flat_tables(inst);
            uint32_t base = 0x800u;
            uint32_t words = depth == 0u ? 8u : 16u;
            uint32_t expected = depth == 0u
                                    ? (is_signed != 0u ? 0x11111111u : 0x99999999u)
                                    : (is_signed != 0u ? 0x14141414u : 0x94949494u);
            write_ram(inst, base, 0xFE0000A0u);
            uint32_t command = 0x20000001u | (depth << 27) | (is_signed << 26);
            write32(inst, MDEC_DATA, command, 0u);
            start_dma0(inst, base, 1u);
            uint64_t deadline = inst->mdec.block_deadline;
            CHECK_EQ_U64(deadline, 1000u);
            CHECK_EQ_U64(schway__mdec_read(inst, MDEC_STATUS, deadline - 1u) & 0x80000000u,
                         0x80000000u);
            CHECK_EQ_U64(schway__mdec_read(inst, MDEC_STATUS, deadline) & 0x80000000u, 0u);
            CHECK_EQ_U64(inst->mdec.output_count, words);
            for (uint32_t i = 0u; i < words; i++) {
                CHECK_EQ_HEX(inst->mdec.output[i], expected);
            }
            CHECK_EQ_HEX(raw_output_hash(inst, words), hashes[depth][is_signed]);
            uint32_t destination = 0x1800u + (depth * 2u + is_signed) * 0x100u;
            dma1_transfer(inst, destination, words);
            CHECK_EQ_U64(inst->mdec.output_count, 0u);
            CHECK_EQ_U64(inst->dma.madr[1], destination + words * 4u);
            for (uint32_t i = 0u; i < words; i++) {
                uint32_t value = 0u;
                CHECK_EQ_U64(schway__bus_ram_word(inst, destination + i * 4u, &value, 0u), 0u);
                CHECK_EQ_HEX(value, expected);
            }
            schway_destroy(inst);
        }
    }
}

static void case_bounds_and_reset(void)
{
    schway *bad = make();
    enable_dma(bad);
    uint32_t bad_input = 0x1000u;
    write_ram(bad, bad_input, 0xFFFFFFFFu);
    write32(bad, MDEC_DATA, 0x24000001u, 0u);
    start_dma0(bad, bad_input, 1u);
    CHECK_EQ_U64(bad->mdec.block_pending, 0u);
    CHECK_EQ_U64(bad->mdec.command_active, 0u);
    CHECK_EQ_U64(bad->mdec.output_count, 0u);
    CHECK_EQ_U64(schway__mdec_next(bad), UINT64_MAX);
    schway_destroy(bad);

    schway *reset = make();
    enable_dma(reset);
    uint32_t input = 0x1100u;
    write_ram(reset, input, 0xFE000000u);
    write32(reset, MDEC_DATA, 0x24000001u, 0u);
    start_dma0(reset, input, 1u);
    CHECK_EQ_U64(reset->mdec.block_pending, 1u);
    uint64_t deadline = reset->mdec.block_deadline;
    write32(reset, MDEC_STATUS, 0x80000000u, 0u);
    CHECK_EQ_U64(reset->mdec.block_pending, 0u);
    CHECK_EQ_U64(reset->mdec.output_count, 0u);
    CHECK_EQ_U64(schway__mdec_next(reset), UINT64_MAX);
    schway__mdec_event(reset, deadline + 1u);
    CHECK_EQ_U64(reset->mdec.output_count, 0u);
    CHECK((reset->dma.dreq & 3u) == 0u);
    schway_destroy(reset);
}

static void case_stream(void)
{
    schway *inst = make();
    load_flat_tables(inst);
    write32(inst, MDEC_DATA, 0x2C000005u, 0u);
    write32(inst, MDEC_DATA, 0xFE000000u, 0u);
    write32(inst, MDEC_DATA, 0x000000A0u, 0u);
    write32(inst, MDEC_DATA, 0x0050FE00u, 0u);
    write32(inst, MDEC_DATA, 0x00A0FE00u, 0u);
    write32(inst, MDEC_DATA, 0xFE000000u, 0u);
    CHECK_EQ_U64(inst->mdec.block_deadline, 1000u);
    run_cycles(inst, 1000u);
    CHECK_EQ_U64(inst->mdec.output_count, 16u);
    for (uint32_t i = 0u; i < 16u; i++) {
        CHECK_EQ_HEX(inst->mdec.output[i], 0u);
    }

    size_t state_size = schway_state_size(inst);
    uint8_t *state = (uint8_t *)malloc(state_size);
    CHECK(state != NULL);
    if (state == NULL) {
        schway_destroy(inst);
        return;
    }
    CHECK_EQ_U64(schway_save_state(inst, state, state_size), SCHWAY_STATUS_OK);
    schway *restored = make();
    CHECK_EQ_U64(schway_load_state(restored, state, state_size), SCHWAY_STATUS_OK);
    free(state);
    schway_destroy(inst);
    inst = restored;

    run_cycles(inst, 1000u);
    CHECK_EQ_U64(inst->mdec.output_count, 32u);
    for (uint32_t i = 16u; i < 32u; i++) {
        CHECK_EQ_HEX(inst->mdec.output[i], 0x14141414u);
    }
    CHECK_EQ_U64(schway__mdec_next(inst), 3000u);

    run_cycles(inst, 1000u);
    CHECK_EQ_U64(inst->mdec.output_count, 48u);
    for (uint32_t i = 32u; i < 48u; i++) {
        CHECK_EQ_HEX(inst->mdec.output[i], 0x0A0A0A0Au);
    }
    CHECK_EQ_U64(inst->mdec.block_pending, 1u);
    CHECK_EQ_U64(inst->mdec.block_deadline, 4000u);
    schway_destroy(inst);
}

static void case_fifo(void)
{
    schway *inst = make();
    enable_dma(inst);
    load_flat_tables(inst);
    write32(inst, MDEC_DATA, 0x2C00000Du, 0u);
    for (uint32_t i = 0u; i < 12u; i++) {
        write32(inst, MDEC_DATA, 0xFE000000u, 0u);
    }
    write32(inst, MDEC_DATA, 0xFE0000A0u, 0u);
    for (uint32_t i = 0u; i < 12u; i++) {
        run_cycles(inst, 1000u);
        CHECK_EQ_U64(inst->mdec.output_count, (i + 1u) * 16u);
    }
    CHECK_EQ_U64(inst->mdec.output_count, SCHWAY__MDEC_OUTPUT_WORDS);
    CHECK_EQ_U64(inst->mdec.block_pending, 1u);
    CHECK_EQ_U64(inst->mdec.block_deadline, 13000u);

    dma1_transfer(inst, 0x2000u, 1u);
    CHECK_EQ_U64(inst->mdec.output_count, SCHWAY__MDEC_OUTPUT_WORDS - 1u);
    run_cycles(inst, 1000u);
    CHECK_EQ_U64(inst->mdec.block_pending, 1u);
    CHECK_EQ_U64(inst->mdec.block_deadline, 0u);
    CHECK_EQ_U64(schway__mdec_next(inst), UINT64_MAX);
    if (inst->mdec.block_pending == 0u || inst->mdec.block_deadline != 0u) {
        schway_destroy(inst);
        return;
    }

    size_t state_size = schway_state_size(inst);
    uint8_t *state = (uint8_t *)malloc(state_size);
    CHECK(state != NULL);
    if (state == NULL) {
        schway_destroy(inst);
        return;
    }
    CHECK_EQ_U64(schway_save_state(inst, state, state_size), SCHWAY_STATUS_OK);
    schway *restored = make();
    CHECK_EQ_U64(schway_load_state(restored, state, state_size), SCHWAY_STATUS_OK);
    free(state);
    schway_destroy(inst);
    inst = restored;
    CHECK_EQ_U64(inst->mdec.block_pending, 1u);
    CHECK_EQ_U64(inst->mdec.block_deadline, 0u);
    dma1_transfer(inst, 0x3000u, 15u);
    CHECK_EQ_U64(inst->mdec.output_count, SCHWAY__MDEC_OUTPUT_WORDS);
    CHECK_EQ_U64(inst->mdec.block_pending, 0u);
    if (inst->mdec.output_count != SCHWAY__MDEC_OUTPUT_WORDS) {
        schway_destroy(inst);
        return;
    }
    dma1_transfer(inst, 0x4000u, SCHWAY__MDEC_OUTPUT_WORDS);
    CHECK_EQ_U64(inst->mdec.output_count, 0u);
    uint32_t last = 0u;
    CHECK_EQ_U64(schway__bus_ram_word(inst, 0x4000u + 191u * 4u, &last, 0u), 0u);
    CHECK_EQ_HEX(last, 0x14141414u);
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        return 2;
    }
    if (strcmp(argv[1], "status") == 0) {
        case_status();
    } else if (strcmp(argv[1], "guest_input") == 0) {
        case_guest_input();
    } else if (strcmp(argv[1], "block") == 0) {
        case_block();
        case_sign_and_tables();
    } else if (strcmp(argv[1], "sign_tables") == 0) {
        case_sign_and_tables();
    } else if (strcmp(argv[1], "words") == 0) {
        case_sign_and_tables();
    } else if (strcmp(argv[1], "vectors") == 0) {
        case_vectors();
    } else if (strcmp(argv[1], "bounds") == 0) {
        case_bounds_and_reset();
    } else if (strcmp(argv[1], "stream") == 0) {
        case_stream();
    } else if (strcmp(argv[1], "fifo") == 0) {
        case_fifo();
    } else {
        return 2;
    }
    CHECK_DONE();
}
