#ifndef SCHWAYSTATION_HOST_MEDIA_LIMITS_H
#define SCHWAYSTATION_HOST_MEDIA_LIMITS_H

#include <stddef.h>
#include <stdint.h>

#define SCHWAY__MEDIA_IMAGE_MAX_BYTES UINT64_C(1073741824)

static inline int schway__media_add_u64(uint64_t left, uint64_t right,
                                        uint64_t *out)
{
    if (out == NULL || right > UINT64_MAX - left) return 0;
    *out = left + right;
    return 1;
}

static inline int schway__media_mul_u64(uint64_t left, uint64_t right,
                                        uint64_t *out)
{
    if (out == NULL || (left != 0u && right > UINT64_MAX / left)) return 0;
    *out = left * right;
    return 1;
}

#endif
