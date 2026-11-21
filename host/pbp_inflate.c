#include "pbp_inflate.h"

#include <string.h>

#include "media_limits.h"
#include "miniz.h"

int schway__pbp_inflate_raw(const uint8_t *input, size_t input_size,
                            uint8_t *output, size_t output_size)
{
    tinfl_decompressor decompressor;
    uint8_t decoded[SCHWAY__PBP_INFLATE_MAX_OUTPUT_BYTES + 1u];
    size_t input_used;
    size_t output_used;
    tinfl_status status;

    if (input == NULL || output == NULL || input_size == 0u || output_size == 0u ||
        (uint64_t)input_size > SCHWAY__MEDIA_IMAGE_MAX_BYTES ||
        output_size > SCHWAY__PBP_INFLATE_MAX_OUTPUT_BYTES) return 0;

    input_used = input_size;
    output_used = output_size + 1u;
    tinfl_init(&decompressor);
    status = tinfl_decompress(&decompressor, input, &input_used, decoded, decoded,
                              &output_used, TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    if (status != TINFL_STATUS_DONE || input_used != input_size || output_used != output_size) {
        return 0;
    }
    memcpy(output, decoded, output_size);
    return 1;
}
