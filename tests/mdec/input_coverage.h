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
    (void)bytes;
    (void)size;
    if (coverage != NULL) {
        coverage->complete_blocks = 0u;
        coverage->trailing_coefficients = 0u;
    }
    return SCHWAY__MDEC_COVERAGE_MALFORMED;
}

#endif /* SCHWAYSTATION_TEST_MDEC_INPUT_COVERAGE_H */
