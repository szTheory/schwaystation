#ifndef SCHWAYSTATION_HOST_PBP_INFLATE_H
#define SCHWAYSTATION_HOST_PBP_INFLATE_H

#include <stddef.h>
#include <stdint.h>

#define SCHWAY__PBP_INFLATE_MAX_OUTPUT_BYTES 0x9300u

int schway__pbp_inflate_raw(const uint8_t *input, size_t input_size,
                            uint8_t *output, size_t output_size);

#endif
