/* Bounded test-only structural scan for compressed MDEC input. */
#ifndef SCHWAYSTATION_TEST_MDEC_INPUT_COVERAGE_H
#define SCHWAYSTATION_TEST_MDEC_INPUT_COVERAGE_H

#include <stddef.h>
#include <stdint.h>

enum {
    SCHWAY__MDEC_COVERAGE_COMPLETE = 0u,
    SCHWAY__MDEC_COVERAGE_INCOMPLETE = 1u,
    SCHWAY__MDEC_COVERAGE_MALFORMED = 2u,
    SCHWAY__MDEC_INPUT_MAX_BYTES = 262144u
};

typedef struct schway__mdec_coverage {
    uint32_t complete_blocks;
    uint32_t trailing_coefficients;
} schway__mdec_coverage;

static inline uint32_t schway__mdec_input_coverage_scan(const uint8_t *bytes, size_t size,
                                                        schway__mdec_coverage *coverage)
{
    if (coverage != NULL) {
        coverage->complete_blocks = 0u;
        coverage->trailing_coefficients = 0u;
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
    if (block_active != 0u) {
        coverage->trailing_coefficients = coefficients;
        return SCHWAY__MDEC_COVERAGE_INCOMPLETE;
    }
    return SCHWAY__MDEC_COVERAGE_COMPLETE;
}

#endif /* SCHWAYSTATION_TEST_MDEC_INPUT_COVERAGE_H */
