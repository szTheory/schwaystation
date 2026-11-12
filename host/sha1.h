/* Streaming SHA-1 for host-side track identity only. */
#ifndef SCHWAYSTATION_HOST_SHA1_H
#define SCHWAYSTATION_HOST_SHA1_H

#include <stddef.h>
#include <stdint.h>

typedef struct schway__sha1 {
    uint32_t h[5];
    uint8_t block[64];
    uint32_t used;
    uint64_t total;
} schway__sha1;

void schway__sha1_init(schway__sha1 *state);
void schway__sha1_update(schway__sha1 *state, const void *data, size_t size);
void schway__sha1_final(schway__sha1 *state, uint8_t digest[20]);
void schway__sha1_hex(const uint8_t digest[20], char hex[41]);

#endif
