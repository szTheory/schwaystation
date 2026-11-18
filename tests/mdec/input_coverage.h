/* Bounded test-only structural scan for compressed MDEC input. */
#ifndef SCHWAYSTATION_TEST_MDEC_INPUT_COVERAGE_H
#define SCHWAYSTATION_TEST_MDEC_INPUT_COVERAGE_H

#include <stddef.h>
#include <stdint.h>

enum {
    SCHWAY__MDEC_COVERAGE_COMPLETE = 0u,
    SCHWAY__MDEC_COVERAGE_INCOMPLETE = 1u,
    SCHWAY__MDEC_COVERAGE_MALFORMED = 2u,
    SCHWAY__MDEC_INPUT_MAX_BYTES = 262144u,
    SCHWAY__MDEC_BLOCKS_PER_COLOR_MACROBLOCK = 6u
};

typedef struct schway__mdec_coverage {
    uint32_t complete_blocks;
    uint32_t trailing_coefficients;
    uint32_t complete_macroblocks;
    uint32_t trailing_blocks;
} schway__mdec_coverage;

int schway__mdec_reference_color_input_guard(const char *name, const uint8_t *exe,
                                             size_t exe_size,
                                             schway__mdec_coverage *coverage_out);

/* Regression-only 8-bit implementation bytes reconstructed from the pinned
   mono guest, quant and scale data in model.c. Acquisition command:
   `ctest --test-dir build/ci -R '^mdec[.]guest_input$' --no-tests=error
   --output-on-failure` (exit 8 on the independent pinned hardware comparison).
   Its diagnostics reported 12 differing words; exact matches at indices 0,
   5, 7 and 11 (00FFFF00, 00FFE8FE, 00E3FFEB, 001A08FF) are retained from the
   original expected[1] hardware array. Together, those diagnostics and
   matches reconstruct all 16 output words below. This is not a hardware
   oracle. The pinned hardware arrays remain separate in model.c.
   The guest ends at coefficient 63 without EOB, matching mdec_parse_block's
   declared 32-word terminal-input rule in src/mdec.c. */
static const uint8_t schway__mdec_8bit_regression_bytes[64] = {
    0x00u, 0xFFu, 0xFFu, 0x00u, 0xFFu, 0xFFu, 0x05u, 0x00u,
    0xC8u, 0xECu, 0xEEu, 0xEDu, 0xEFu, 0xFBu, 0xF2u, 0x00u,
    0xD7u, 0xDBu, 0xFAu, 0xE8u, 0xFEu, 0xE8u, 0xFFu, 0x00u,
    0xB7u, 0xF3u, 0xECu, 0xF0u, 0xEBu, 0xFFu, 0xE3u, 0x00u,
    0x00u, 0xFAu, 0xFFu, 0xF5u, 0xF2u, 0xFFu, 0x04u, 0x00u,
    0x00u, 0x05u, 0xFFu, 0xFBu, 0xFFu, 0x08u, 0x1Au, 0x00u,
    0x0Fu, 0x28u, 0x1Fu, 0xFFu, 0x06u, 0x2Au, 0x23u, 0x00u,
    0x11u, 0x38u, 0x41u, 0x29u, 0x31u, 0x33u, 0x16u, 0x0Eu,
};

static inline uint32_t schway__mdec_input_coverage_scan(const uint8_t *bytes, size_t size,
                                                        schway__mdec_coverage *coverage)
{
    if (coverage != NULL) {
        coverage->complete_blocks = 0u;
        coverage->trailing_coefficients = 0u;
        coverage->complete_macroblocks = 0u;
        coverage->trailing_blocks = 0u;
    }
    if (coverage == NULL || (bytes == NULL && size != 0u) ||
        size > SCHWAY__MDEC_INPUT_MAX_BYTES || (size & 1u) != 0u) {
        return SCHWAY__MDEC_COVERAGE_MALFORMED;
    }
    if (size == 0u) {
        return SCHWAY__MDEC_COVERAGE_INCOMPLETE;
    }
    uint32_t block_active = 0u;
    uint32_t coefficients = 0u;
    for (size_t at = 0u; at < size; at += 2u) {
        uint16_t value = (uint16_t)((uint32_t)bytes[at] |
                                    ((uint32_t)bytes[at + 1u] << 8u));
        if (value == 0xFE00u) {
            if (block_active != 0u) {
                coverage->complete_blocks++;
                block_active = 0u;
                coefficients = 0u;
            }
            continue;
        }
        if (block_active == 0u) {
            block_active = 1u;
            coefficients = 1u;
            continue;
        }
        uint32_t run = (value >> 10u) & 0x3Fu;
        coefficients += run + 1u;
        if (coefficients >= 64u) {
            coverage->complete_blocks++;
            block_active = 0u;
            coefficients = 0u;
        }
    }
    coverage->complete_macroblocks = coverage->complete_blocks /
                                     SCHWAY__MDEC_BLOCKS_PER_COLOR_MACROBLOCK;
    coverage->trailing_blocks = coverage->complete_blocks %
                                SCHWAY__MDEC_BLOCKS_PER_COLOR_MACROBLOCK;
    if (block_active != 0u) {
        coverage->trailing_coefficients = coefficients;
        return SCHWAY__MDEC_COVERAGE_INCOMPLETE;
    }
    return SCHWAY__MDEC_COVERAGE_COMPLETE;
}

#endif /* SCHWAYSTATION_TEST_MDEC_INPUT_COVERAGE_H */
