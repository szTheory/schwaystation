/* MDEC bus, block and status behavior through the instance-owned device. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"
#include "input_coverage.h"

#define MDEC_STATUS 0x1F801824u
#define MDEC_DATA 0x1F801820u
#define DMA_BASE 0x1F801080u
#define DMA_DPCR 0x1F8010F0u
#define GPU_GP0 0x1F801810u
#define GPU_GP1 0x1F801814u

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
static void dma1_transfer(schway *inst, uint32_t base, uint32_t words);
static void load_scale_table(schway *inst, const int16_t scale[SCHWAY__MDEC_BLOCK_SAMPLES]);

static int mdec_8bit_regression_matches(const uint8_t *actual, size_t size)
{
    return size == sizeof schway__mdec_8bit_regression_bytes &&
           memcmp(actual, schway__mdec_8bit_regression_bytes,
                  sizeof schway__mdec_8bit_regression_bytes) == 0;
}

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

static void case_guest_input_modes(uint32_t depth_mask)
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
        0x7D8A, 0x6A6D, 0x471C, 0x18F8, -0x18F9, -0x471D,
        -0x6A6E, -0x7D8B, 0x7641, 0x30FB, -0x30FC,
        -0x7642, -0x7642, -0x30FC, 0x30FB, 0x7641,
        0x6A6D, -0x18F9, -0x7D8B, -0x471D, 0x471C,
        0x7D8A, 0x18F8, -0x6A6E, 0x5A82, -0x5A83,
        -0x5A83, 0x5A82, 0x5A82, -0x5A83, -0x5A83,
        0x5A82, 0x471C, -0x7D8B, 0x18F8, 0x6A6D, -0x6A6E,
        -0x18F9, 0x7D8A, -0x471D, 0x30FB, -0x7642,
        0x7641, -0x30FC, -0x30FC, 0x7641, -0x7642,
        0x30FB, 0x18F8, -0x471D, 0x6A6D, -0x7D8B, 0x7D8A,
        -0x6A6E, 0x471C, -0x18F9,
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
    uint8_t input_bytes[sizeof guest_input];
    for (uint32_t word = 0u; word < 32u; word++) {
        for (uint32_t byte = 0u; byte < 4u; byte++) {
            input_bytes[word * 4u + byte] = (uint8_t)(guest_input[word] >> (byte * 8u));
        }
    }
    schway__mdec_coverage input_coverage;
    memset(&input_coverage, 0, sizeof input_coverage);
    uint32_t input_status = schway__mdec_input_coverage_scan(
        input_bytes, sizeof input_bytes, &input_coverage);
    if (input_status != SCHWAY__MDEC_COVERAGE_COMPLETE ||
        input_coverage.complete_blocks != 1u || input_coverage.trailing_coefficients != 0u) {
        CHECK_EQ_U64(input_status, SCHWAY__MDEC_COVERAGE_COMPLETE);
        CHECK_EQ_U64(input_coverage.complete_blocks, 1u);
        CHECK_EQ_U64(input_coverage.trailing_coefficients, 0u);
        return;
    }
    for (uint32_t pixel = 0u; pixel < 64u; pixel++) {
        uint32_t byte = (expected[1][pixel / 4u] >> ((pixel % 4u) * 8u)) & 0xFFu;
        uint32_t nearest = (byte + 8u) >> 4u;
        if (nearest > 15u) {
            nearest = 15u;
        }
        uint32_t nibble = (expected[0][pixel / 8u] >> ((pixel % 8u) * 4u)) & 0x0Fu;
        CHECK_EQ_U64(nibble, nearest);
    }
    if ((depth_mask & (1u << 1u)) != 0u) {
        uint8_t mutated_baseline[sizeof schway__mdec_8bit_regression_bytes];
        memcpy(mutated_baseline, schway__mdec_8bit_regression_bytes,
               sizeof mutated_baseline);
        mutated_baseline[0] ^= 1u;
        CHECK_EQ_U64(mdec_8bit_regression_matches(schway__mdec_8bit_regression_bytes,
                                                   sizeof schway__mdec_8bit_regression_bytes), 1u);
        CHECK_EQ_U64(mdec_8bit_regression_matches(mutated_baseline,
                                                   sizeof mutated_baseline), 0u);
    }
    for (uint32_t depth = 0u; depth < 2u; depth++) {
        if ((depth_mask & (1u << depth)) == 0u) {
            continue;
        }
        uint32_t run_count = depth == 0u ? 1u : 2u;
        for (uint32_t run = 0u; run < run_count; run++) {
            schway *inst = make();
            enable_dma(inst);
            load_guest_quant(inst, guest_quant);
            load_scale_table(inst, guest_scale);
            write32(inst, MDEC_DATA, 0x20000020u | (depth << 27), 0u);
            for (uint32_t i = 0u; i < 32u; i++) {
                write_ram(inst, 0x12000u + i * 4u, guest_input[i]);
            }
            start_dma0(inst, 0x12000u, 32u);
            if (run == 0u) {
                run_cycles(inst, 4096u);
            } else {
                run_cycles(inst, 512u);
                run_cycles(inst, 3584u);
            }
            uint32_t output_words = depth == 0u ? 8u : 16u;
            CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
            CHECK_EQ_HEX(inst->dma.madr[0], 0x12080u);
            CHECK_EQ_U64(inst->mdec.input_count, 0u);
            CHECK_EQ_U64(inst->mdec.words_remaining, 0u);
            CHECK_EQ_U64(inst->mdec.output_count, output_words);
            if (depth == 0u) {
                for (uint32_t i = 0u; i < output_words; i++) {
                    CHECK_EQ_HEX(inst->mdec.output[i], expected[0][i]);
                }
            } else {
                uint8_t raw_fifo[sizeof schway__mdec_8bit_regression_bytes];
                for (uint32_t word = 0u; word < output_words; word++) {
                    uint32_t value = inst->mdec.output[(inst->mdec.output_head + word) %
                                                        SCHWAY__MDEC_OUTPUT_WORDS];
                    for (uint32_t byte = 0u; byte < 4u; byte++) {
                        raw_fifo[word * 4u + byte] = (uint8_t)(value >> (byte * 8u));
                    }
                }
                CHECK_EQ_U64(inst->dma.dreq & (1u << 1), 1u << 1);
                CHECK_EQ_HEX(read32(inst, MDEC_STATUS) & (1u << 27), 1u << 27);
                CHECK_EQ_U64(mdec_8bit_regression_matches(raw_fifo, sizeof raw_fifo), 1u);
                dma1_transfer(inst, 0x13000u, output_words);
                CHECK_EQ_U64(inst->mdec.output_count, 0u);
                CHECK_EQ_HEX(inst->dma.madr[1], 0x13040u);
                for (uint32_t word = 0u; word < output_words; word++) {
                    uint32_t actual_word = 0u;
                    uint32_t expected_word =
                        (uint32_t)schway__mdec_8bit_regression_bytes[word * 4u] |
                        ((uint32_t)schway__mdec_8bit_regression_bytes[word * 4u + 1u] << 8u) |
                        ((uint32_t)schway__mdec_8bit_regression_bytes[word * 4u + 2u] << 16u) |
                        ((uint32_t)schway__mdec_8bit_regression_bytes[word * 4u + 3u] << 24u);
                    CHECK_EQ_U64(schway__bus_ram_word(inst, 0x13000u + word * 4u,
                                                       &actual_word, 0u), 0u);
                    CHECK_EQ_HEX(actual_word, expected_word);
                }
            }
            schway_destroy(inst);
        }
    }
}

static void case_guest_4bit(void)
{
    case_guest_input_modes(1u << 0u);
}

static void case_guest_8bit_regression(void)
{
    case_guest_input_modes(1u << 1u);
}

static void case_input_coverage(void)
{
    /* This zero-run control has one DC and 63 AC values, so one mono block is
       complete at the declared command boundary without an EOB halfword. */
    static const uint8_t complete[128] = {0};
    schway__mdec_coverage coverage;
    memset(&coverage, 0, sizeof coverage);
    CHECK_EQ_U64(schway__mdec_input_coverage_scan(complete, sizeof complete, &coverage),
                 SCHWAY__MDEC_COVERAGE_COMPLETE);
    CHECK_EQ_U64(coverage.complete_blocks, 1u);
    CHECK_EQ_U64(coverage.trailing_coefficients, 0u);
    CHECK_EQ_U64(coverage.complete_macroblocks, 0u);
    CHECK_EQ_U64(coverage.trailing_blocks, 1u);

    /* Six independently complete blocks form one color macroblock. */
    static const uint8_t complete_color_unit[24] = {
        0u, 0u, 0u, 0xFEu, 0u, 0u, 0u, 0xFEu,
        0u, 0u, 0u, 0xFEu, 0u, 0u, 0u, 0xFEu,
        0u, 0u, 0u, 0xFEu, 0u, 0u, 0u, 0xFEu
    };
    memset(&coverage, 0, sizeof coverage);
    CHECK_EQ_U64(schway__mdec_input_coverage_scan(complete_color_unit,
                                                   sizeof complete_color_unit, &coverage),
                 SCHWAY__MDEC_COVERAGE_COMPLETE);
    CHECK_EQ_U64(coverage.complete_blocks, 6u);
    CHECK_EQ_U64(coverage.complete_macroblocks, 1u);
    CHECK_EQ_U64(coverage.trailing_blocks, 0u);

    /* A structurally valid five-block prefix is not a complete color unit. */
    memset(&coverage, 0, sizeof coverage);
    CHECK_EQ_U64(schway__mdec_input_coverage_scan(complete_color_unit,
                                                   sizeof complete_color_unit - 4u, &coverage),
                 SCHWAY__MDEC_COVERAGE_COMPLETE);
    CHECK_EQ_U64(coverage.complete_blocks, 5u);
    CHECK_EQ_U64(coverage.complete_macroblocks, 0u);
    CHECK_EQ_U64(coverage.trailing_blocks, 5u);

    /* FE00 padding and the next DC halfword must survive the block boundary. */
    static const uint8_t padded_blocks[] = {
        0x00u, 0xFEu, 0x00u, 0x00u, 0x00u, 0xFEu,
        0x00u, 0x00u, 0x00u, 0xFEu
    };
    memset(&coverage, 0, sizeof coverage);
    CHECK_EQ_U64(schway__mdec_input_coverage_scan(padded_blocks, sizeof padded_blocks,
                                                   &coverage),
                 SCHWAY__MDEC_COVERAGE_COMPLETE);
    CHECK_EQ_U64(coverage.complete_blocks, 2u);
    CHECK_EQ_U64(coverage.trailing_blocks, 2u);

    static const uint8_t truncated[] = {0u, 0u};
    memset(&coverage, 0, sizeof coverage);
    CHECK_EQ_U64(schway__mdec_input_coverage_scan(truncated, sizeof truncated, &coverage),
                 SCHWAY__MDEC_COVERAGE_INCOMPLETE);
    CHECK_EQ_U64(coverage.complete_blocks, 0u);
    CHECK_EQ_U64(coverage.trailing_coefficients, 1u);

    static const uint8_t odd_size[] = {0u};
    coverage.complete_macroblocks = 17u;
    coverage.trailing_blocks = 8u;
    CHECK_EQ_U64(schway__mdec_input_coverage_scan(odd_size, sizeof odd_size, &coverage),
                 SCHWAY__MDEC_COVERAGE_MALFORMED);
    CHECK_EQ_U64(coverage.complete_blocks, 0u);
    CHECK_EQ_U64(coverage.trailing_coefficients, 0u);
    CHECK_EQ_U64(coverage.complete_macroblocks, 0u);
    CHECK_EQ_U64(coverage.trailing_blocks, 0u);

    memset(&coverage, 0, sizeof coverage);
    CHECK_EQ_U64(schway__mdec_input_coverage_scan(NULL, 0u, &coverage),
                 SCHWAY__MDEC_COVERAGE_INCOMPLETE);
    CHECK_EQ_U64(coverage.complete_blocks, 0u);
    CHECK_EQ_U64(coverage.trailing_coefficients, 0u);
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

    /* A nonempty table command reports parameter-count minus one and wraps
       to 0xFFFF after the final input word, as in the SCPH-5501 capture. */
    write32(inst, MDEC_DATA, 0x40000000u, 0u);
    CHECK_EQ_HEX(read32(inst, MDEC_STATUS) & 0xFFFFu, 0x000Fu);
    for (uint32_t i = 0u; i < 16u; i++) {
        write32(inst, MDEC_DATA, 0x01010101u, 0u);
    }
    CHECK_EQ_HEX(read32(inst, MDEC_STATUS), 0x8004FFFFu);

    /* MDEC(3) has a fixed 32-word parameter count and the same terminal
       counter value. */
    write32(inst, MDEC_DATA, 0x60000000u, 0u);
    CHECK_EQ_HEX(read32(inst, MDEC_STATUS) & 0xFFFFu, 0x001Fu);
    for (uint32_t i = 0u; i < 32u; i++) {
        write32(inst, MDEC_DATA, 0x5A827D8Au, 0u);
    }
    CHECK_EQ_HEX(read32(inst, MDEC_STATUS), 0x8004FFFFu);

    /* The decode counter reaches 0xFFFF as soon as all declared input words
       arrive, while block output is still pending. */
    write32(inst, MDEC_DATA, 0x28000001u, 0u);
    CHECK_EQ_HEX(read32(inst, MDEC_STATUS) & 0xFFFFu, 0u);
    write32(inst, MDEC_DATA, 0xFE000000u, 0u);
    status = read32(inst, MDEC_STATUS);
    CHECK_EQ_HEX(status & 0xFFFFu, 0xFFFFu);
    CHECK((status & (1u << 29)) != 0u);
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

static uint32_t color_dc_word(int32_t dc)
{
    return UINT32_C(0xFE000000) | ((uint32_t)dc & 0x3FFu);
}

static uint32_t ram_word(schway *inst, uint32_t address)
{
    uint32_t value = 0u;
    CHECK_EQ_U64(schway__bus_ram_word(inst, address, &value, 0u), 0u);
    return value;
}

static uint64_t ram_hash(const schway *inst, uint32_t address, uint32_t words)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    for (uint32_t i = 0u; i < words; i++) {
        uint32_t at = address + i * 4u;
        uint32_t word = (uint32_t)inst->bus.ram[at] |
                        ((uint32_t)inst->bus.ram[at + 1u] << 8u) |
                        ((uint32_t)inst->bus.ram[at + 2u] << 16u) |
                        ((uint32_t)inst->bus.ram[at + 3u] << 24u);
        for (uint32_t byte = 0u; byte < 4u; byte++) {
            hash = (hash ^ (uint8_t)(word >> (byte * 8u))) * UINT64_C(1099511628211);
        }
    }
    return hash;
}

static uint64_t decode_color_macroblock_with_scale(schway *inst, uint32_t depth,
                                                   uint32_t signed_output, uint32_t bit15,
                                                   int32_t cr, int32_t cb, const int32_t y[4],
                                                   const int16_t *scale,
                                                   uint32_t input_address,
                                                   uint32_t output_address)
{
    uint32_t blocks[6] = {color_dc_word(cr), color_dc_word(cb), color_dc_word(y[0]),
                          color_dc_word(y[1]), color_dc_word(y[2]), color_dc_word(y[3])};
    uint32_t command = 0x20000006u | (depth << 27u) | (signed_output << 26u) |
                       (bit15 << 25u);
    uint32_t output_words = depth == 2u ? 192u : 128u;
    enable_dma(inst);
    load_flat_tables(inst);
    if (scale != NULL) {
        load_scale_table(inst, scale);
    }
    write32(inst, MDEC_DATA, command, 0u);
    for (uint32_t i = 0u; i < 6u; i++) {
        write_ram(inst, input_address + i * 4u, blocks[i]);
    }
    start_dma0(inst, input_address, 6u);
    run_cycles(inst, 10000u);
    CHECK_EQ_U64(inst->mdec.command_active, 0u);
    CHECK_EQ_U64(inst->mdec.block_pending, 0u);
    CHECK_EQ_U64(inst->mdec.output_count, output_words);
    uint64_t raw_hash = raw_output_hash(inst, output_words);
    dma1_transfer(inst, output_address, output_words);
    CHECK_EQ_U64(inst->mdec.output_count, 0u);
    CHECK_EQ_U64(ram_hash(inst, output_address, output_words), raw_hash);
    return raw_hash;
}

static uint64_t decode_color_macroblock(schway *inst, uint32_t depth, uint32_t signed_output,
                                        uint32_t bit15, int32_t cr, int32_t cb,
                                        const int32_t y[4], uint32_t input_address,
                                        uint32_t output_address)
{
    return decode_color_macroblock_with_scale(inst, depth, signed_output, bit15, cr, cb, y,
                                              NULL, input_address, output_address);
}

static void mdec_to_gpu_dma2(schway *inst, uint32_t source, uint32_t words,
                             uint32_t image_words_wide, uint32_t height, uint32_t mode)
{
    write32(inst, GPU_GP1, 0x03000000u, 0u);
    write32(inst, GPU_GP1, 0x05000000u, 0u);
    write32(inst, GPU_GP1, 0x060E4064u, 0u); /* 16-pixel window at dot divider 8. */
    write32(inst, GPU_GP1, 0x07004000u, 0u); /* 16 scan lines. */
    write32(inst, GPU_GP1, 0x08000000u | mode, 0u);
    write32(inst, GPU_GP1, 0x04000002u, 0u); /* DMA direction: CPU to GP0. */
    write32(inst, GPU_GP0, 0xA0000000u, 0u);
    write32(inst, GPU_GP0, 0u, 0u);
    write32(inst, GPU_GP0, (height << 16u) | image_words_wide, 0u);
    write32(inst, DMA_BASE + 0x20u, source, 0u);
    write32(inst, DMA_BASE + 0x24u, ((words / 16u) << 16u) | 16u, 0u);
    write32(inst, DMA_BASE + 0x28u, 0x01000201u, 0u);
    for (uint32_t i = 0u; i < words * 4u + 32u && inst->dma.cur != SCHWAY__DMA_NONE; i++) {
        schway__dma_event(inst, inst->dma.busy_until);
    }
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
}

static void swizzle_15bit_macroblock(schway *inst, uint32_t source, uint32_t destination)
{
    for (uint32_t y = 0u; y < 16u; y++) {
        for (uint32_t x = 0u; x < 16u; x++) {
            uint32_t block = (y / 8u) * 2u + x / 8u;
            uint32_t source_pixel = block * 64u + (y % 8u) * 8u + x % 8u;
            uint32_t word = ram_word(inst, source + (source_pixel / 2u) * 4u);
            uint16_t pixel = (uint16_t)(word >> ((source_pixel & 1u) * 16u));
            uint32_t output_pixel = y * 16u + x;
            uint32_t address = destination + (output_pixel / 2u) * 4u;
            uint32_t old = ram_word(inst, address);
            uint32_t shift = (output_pixel & 1u) * 16u;
            write_ram(inst, address, (old & ~(UINT32_C(0xFFFF) << shift)) |
                                     ((uint32_t)pixel << shift));
        }
    }
}

static void build_long_color_stream(uint32_t words[38])
{
    uint16_t halves[76];
    for (uint32_t i = 0u; i < 76u; i++) {
        halves[i] = 0xFE00u;
    }
    uint32_t half = 0u;
    halves[half++] = 0x0420u; /* qscale 1, DC 32 */
    for (uint32_t i = 0u; i < 63u; i++) {
        halves[half++] = 0x0001u; /* one nonzero AC at each remaining coefficient */
    }
    halves[half++] = 0xFE00u;
    for (uint32_t block = 1u; block < 6u; block++) {
        halves[half++] = 0x0420u;
        halves[half++] = 0xFE00u;
    }
    CHECK_EQ_U64(half, 75u);
    for (uint32_t i = 0u; i < 38u; i++) {
        words[i] = halves[i * 2u] | ((uint32_t)halves[i * 2u + 1u] << 16u);
    }
}

static uint64_t finish_long_color_stream(schway *inst, uint32_t input_address,
                                         uint32_t output_address)
{
    start_dma0(inst, input_address + 32u * 4u, 6u);
    run_cycles(inst, 20000u);
    CHECK_EQ_U64(inst->mdec.output_count, 192u);
    uint64_t hash = raw_output_hash(inst, 192u);
    dma1_transfer(inst, output_address, 192u);
    CHECK_EQ_U64(inst->mdec.output_count, 0u);
    CHECK_EQ_U64(inst->mdec.command_active, 0u);
    return hash;
}

static void case_color_streaming(void)
{
    const uint32_t input_address = 0x5000u;
    const uint32_t output_address = 0x6000u;
    uint32_t words[38];
    build_long_color_stream(words);

    schway *continuous = make();
    schway *resumed = make();
    CHECK(continuous != NULL && resumed != NULL);
    if (continuous == NULL || resumed == NULL) {
        schway_destroy(continuous);
        schway_destroy(resumed);
        return;
    }

    schway *instances[2] = {continuous, resumed};
    for (uint32_t instance = 0u; instance < 2u; instance++) {
        schway *inst = instances[instance];
        enable_dma(inst);
        load_flat_tables(inst);
        for (uint32_t i = 0u; i < 38u; i++) {
            write_ram(inst, input_address + i * 4u, words[i]);
        }
        write32(inst, MDEC_DATA, 0x30000026u, 0u);
        start_dma0(inst, input_address, 32u);
        CHECK_EQ_U64(inst->mdec.command_active, 1u);
        CHECK_EQ_U64(inst->mdec.words_remaining, 6u);
        CHECK_EQ_U64(inst->mdec.input_count, 0u);
        CHECK_EQ_U64(inst->mdec.block_pending, 0u);
        CHECK_EQ_U64(inst->mdec.parse_have_dc, 1u);
        CHECK_EQ_U64(inst->mdec.parse_scan, 63u);
    }

    size_t state_size = schway_state_size(continuous);
    uint8_t *state = (uint8_t *)malloc(state_size);
    CHECK(state != NULL);
    if (state != NULL) {
        CHECK_EQ_U64(schway_save_state(continuous, state, state_size), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(schway_load_state(resumed, state, state_size), SCHWAY_STATUS_OK);
        free(state);
    }

    uint64_t continuous_hash = finish_long_color_stream(continuous, input_address,
                                                       output_address);
    uint64_t resumed_hash = finish_long_color_stream(resumed, input_address,
                                                     output_address);
    CHECK_EQ_U64(continuous_hash, resumed_hash);
    for (uint32_t i = 0u; i < 192u; i++) {
        CHECK_EQ_U64(ram_word(continuous, output_address + i * 4u),
                     ram_word(resumed, output_address + i * 4u));
    }
    schway_destroy(continuous);
    schway_destroy(resumed);
}

static void case_color(void)
{
    static const int32_t y[4] = {240, 320, 400, 480};
    static const uint16_t signed_pixels[4] = {0xA080u, 0xA4C1u, 0xA8E2u, 0xAD03u};
    static const uint16_t unsigned_pixels[4] = {0xE290u, 0xE6D1u, 0xEAF2u, 0xEF13u};
    uint64_t standard_hash = 0u;
    for (uint32_t signed_output = 0u; signed_output < 2u; signed_output++) {
        schway *inst = make();
        CHECK(inst != NULL);
        if (inst == NULL) return;
        uint32_t output = 0x3000u;
        uint64_t raw_hash = decode_color_macroblock(inst, 3u, signed_output, 1u, -160, 160, y,
                                                    0x1000u, output);
        if (signed_output != 0u) {
            standard_hash = raw_hash;
        }
        for (uint32_t block = 0u; block < 4u; block++) {
            uint32_t expected = (uint32_t)(signed_output != 0u ? signed_pixels[block]
                                                                : unsigned_pixels[block]);
            expected |= expected << 16u;
            CHECK_EQ_HEX(ram_word(inst, output + block * 32u * 4u), expected);
            CHECK_EQ_HEX(ram_word(inst, output + (block * 32u + 31u) * 4u), expected);
        }
        uint32_t reordered = 0x4000u;
        swizzle_15bit_macroblock(inst, output, reordered);
        CHECK(ram_hash(inst, reordered, 128u) != raw_hash);
        uint32_t gpu_mode = 0x01u;
        mdec_to_gpu_dma2(inst, reordered, 128u, 16u, 16u, gpu_mode);
        inst->exe_loaded = 1u;
        schway_video video;
        memset(&video, 0, sizeof video);
        video.size = (uint32_t)sizeof video;
        CHECK_EQ_U64(schway_get_video(inst, &video), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(video.width, 16u);
        CHECK_EQ_U64(video.height, 16u);
        const uint16_t *pixels = (const uint16_t *)video.pixels;
        for (uint32_t yy = 0u; yy < 16u; yy++) {
            for (uint32_t xx = 0u; xx < 16u; xx++) {
                uint32_t block = (yy / 8u) * 2u + xx / 8u;
                uint16_t expected = signed_output != 0u ? signed_pixels[block]
                                                       : unsigned_pixels[block];
                expected |= 0x8000u;
                CHECK_EQ_HEX(pixels[yy * 16u + xx], expected);
            }
        }
        schway_destroy(inst);
    }

    schway *scaled = make();
    CHECK(scaled != NULL);
    if (scaled != NULL) {
        int16_t scale[SCHWAY__MDEC_BLOCK_SAMPLES];
        for (uint32_t i = 0u; i < SCHWAY__MDEC_BLOCK_SAMPLES; i++) {
            scale[i] = (i & 1u) == 0u ? 0x4000 : 0x2000;
        }
        uint64_t scaled_hash = decode_color_macroblock_with_scale(
            scaled, 3u, 1u, 1u, -160, 160, y, scale, 0x1000u, 0x3000u);
        CHECK(scaled_hash != standard_hash);
        uint32_t chroma_varies = 0u;
        for (uint32_t i = 1u; i < SCHWAY__MDEC_BLOCK_SAMPLES; i++) {
            chroma_varies |= (uint32_t)(scaled->mdec.chroma_cr[i] !=
                                        scaled->mdec.chroma_cr[0]);
        }
        CHECK(chroma_varies != 0u);
        schway_destroy(scaled);
    }

    schway *zero_scale_ac = make();
    CHECK(zero_scale_ac != NULL);
    if (zero_scale_ac != NULL) {
        int16_t scale[SCHWAY__MDEC_BLOCK_SAMPLES] = {0};
        for (uint32_t i = 0u; i < 8u; i++) {
            scale[i + i * 8u] = 0x4000;
        }
        enable_dma(zero_scale_ac);
        load_flat_tables(zero_scale_ac);
        load_scale_table(zero_scale_ac, scale);
        write32(zero_scale_ac, MDEC_DATA, 0x28000002u, 0u);
        write_ram(zero_scale_ac, 0x1000u, 0x04280000u); /* DC 0, run 1, AC +40. */
        write_ram(zero_scale_ac, 0x1004u, 0xFE00FE00u);
        start_dma0(zero_scale_ac, 0x1000u, 2u);
        run_cycles(zero_scale_ac, 2000u);
        CHECK_EQ_U64(zero_scale_ac->mdec.output_count, 16u);
        CHECK_EQ_U64(zero_scale_ac->mdec.block[2], 5u);
        CHECK_EQ_HEX(zero_scale_ac->mdec.output[0], 0x80858080u);
        for (uint32_t i = 1u; i < 16u; i++) {
            CHECK_EQ_HEX(zero_scale_ac->mdec.output[i], 0x80808080u);
        }
        schway_destroy(zero_scale_ac);
    }

    schway *extreme = make();
    CHECK(extreme != NULL);
    if (extreme != NULL) {
        const int32_t max_y[4] = {511, 511, -512, -512};
        uint32_t output = 0x3000u;
        (void)decode_color_macroblock(extreme, 2u, 1u, 0u, 511, -512, max_y,
                                      0x1000u, output);
        CHECK_EQ_U64(extreme->mdec.output_count, 0u);
        CHECK_EQ_U64(extreme->mdec.chroma_cr[0], 64u);
        CHECK(extreme->mdec.chroma_cb[0] == -64);
        CHECK(extreme->mdec.block[0] == -64);
        CHECK_EQ_HEX(ram_word(extreme, output), 0x7FCE297Fu);
        CHECK_EQ_HEX(ram_word(extreme, output + 8u * 16u * 3u), 0x1980A919u);
        schway_destroy(extreme);
    }

    schway *incomplete = make();
    CHECK(incomplete != NULL);
    if (incomplete != NULL) {
        enable_dma(incomplete);
        load_flat_tables(incomplete);
        write32(incomplete, MDEC_DATA, 0x30000001u, 0u);
        write32(incomplete, MDEC_DATA, 0x000000A0u, 0u); /* DC, missing EOB. */
        CHECK_EQ_U64(incomplete->mdec.command_active, 0u);
        CHECK_EQ_U64(incomplete->mdec.block_pending, 0u);
        CHECK_EQ_U64(incomplete->mdec.output_count, 0u);
        write32(incomplete, MDEC_STATUS, 0x80000000u, 0u);
        run_cycles(incomplete, 1000u);
        CHECK_EQ_U64(incomplete->mdec.output_count, 0u);
        CHECK_EQ_U64(incomplete->mdec.block_pending, 0u);
        schway_destroy(incomplete);
    }

    case_color_streaming();
}

static void case_display(void)
{
    schway *inst = make();
    CHECK(inst != NULL);
    if (inst == NULL) return;
    static const int32_t y[4] = {160, 160, 160, 160};
    (void)decode_color_macroblock(inst, 2u, 0u, 0u, 0, 0, y, 0x1000u, 0x3000u);
    mdec_to_gpu_dma2(inst, 0x3000u, 192u, 24u, 16u, 0x11u);
    inst->exe_loaded = 1u;
    schway_video video;
    memset(&video, 0, sizeof video);
    video.size = (uint32_t)sizeof video;
    CHECK_EQ_U64(schway_get_video(inst, &video), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(video.width, 16u);
    CHECK_EQ_U64(video.height, 16u);
    CHECK_EQ_U64(video.bpp, 24u);
    CHECK_EQ_U64(video.rgb24, 1u);
    CHECK_EQ_U64(video.pitch_bytes, 48u);
    const uint8_t *pixels = (const uint8_t *)video.pixels;
    for (uint32_t i = 0u; i < 16u * 16u * 3u; i++) {
        CHECK_EQ_U64(pixels[i], 148u);
    }
    schway_destroy(inst);
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

    schway *clip9 = make();
    CHECK(clip9 != NULL);
    if (clip9 != NULL) {
        uint16_t dense_halfwords[66];
        uint32_t dense_words[33];
        uint8_t quant[SCHWAY__MDEC_BLOCK_SAMPLES];
        int16_t scale[SCHWAY__MDEC_BLOCK_SAMPLES];
        dense_halfwords[0] = 0xFC05u;
        for (uint32_t i = 1u; i < 64u; i++) {
            dense_halfwords[i] = 0x0002u;
        }
        dense_halfwords[64] = 0xFE00u;
        dense_halfwords[65] = 0xFE00u;
        for (uint32_t i = 0u; i < 33u; i++) {
            dense_words[i] = dense_halfwords[i * 2u] |
                             ((uint32_t)dense_halfwords[i * 2u + 1u] << 16u);
            write_ram(clip9, 0x2000u + i * 4u, dense_words[i]);
        }
        memset(quant, 0xFF, sizeof quant);
        for (uint32_t i = 0u; i < SCHWAY__MDEC_BLOCK_SAMPLES; i++) {
            scale[i] = 0x7FFF;
        }
        enable_dma(clip9);
        load_guest_quant(clip9, quant);
        load_scale_table(clip9, scale);
        write32(clip9, MDEC_DATA, 0x20000021u, 0u);
        start_dma0(clip9, 0x2000u, 33u);
        run_cycles(clip9, 2000u);
        CHECK_EQ_U64(clip9->mdec.output_count, 8u);
        for (uint32_t i = 0u; i < SCHWAY__MDEC_BLOCK_SAMPLES; i++) {
            CHECK_EQ_U64((uint32_t)(int32_t)clip9->mdec.block[i], (uint32_t)-16);
        }
        schway_destroy(clip9);
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
    write_ram(bad, bad_input, 0x000000A0u); /* Truncated DC and AC data. */
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
    } else if (strcmp(argv[1], "color") == 0) {
        case_color();
    } else if (strcmp(argv[1], "display") == 0) {
        case_display();
    } else if (strcmp(argv[1], "guest_4bit") == 0) {
        case_guest_4bit();
    } else if (strcmp(argv[1], "guest_8bit_regression") == 0) {
        case_guest_8bit_regression();
    } else if (strcmp(argv[1], "input_coverage") == 0) {
        case_input_coverage();
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
