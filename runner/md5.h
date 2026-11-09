/* MD5 (RFC 1321), owned code for the runner's image hashes. */
#ifndef SCHWAYSTATION_RUNNER_MD5_H
#define SCHWAYSTATION_RUNNER_MD5_H

#include <stddef.h>
#include <stdint.h>

typedef struct schway__md5 {
    uint32_t h[4];     /* the buffer A, B, C, D */
    uint8_t block[64]; /* bytes of the current, unfinished block */
    uint32_t used;     /* bytes in block */
    uint64_t total;    /* message length in bytes */
} schway__md5;

void schway__md5_init(schway__md5 *s);
void schway__md5_update(schway__md5 *s, const void *data, size_t len);
void schway__md5_final(schway__md5 *s, uint8_t digest[16]);

/* Writes the digest as 32 lowercase hex digits and a terminating NUL. */
void schway__md5_hex(const uint8_t digest[16], char hex[33]);

#endif /* SCHWAYSTATION_RUNNER_MD5_H */
