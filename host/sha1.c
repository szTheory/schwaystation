/* Streaming SHA-1, used only for public track identity in the host tools. */
#include "sha1.h"

#include <string.h>

static uint32_t schway__sha1_rotl(uint32_t value, unsigned count)
{
    return (value << count) | (value >> (32u - count));
}

static uint32_t schway__sha1_be32(const uint8_t *bytes)
{
    return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) |
           ((uint32_t)bytes[2] << 8) | (uint32_t)bytes[3];
}

static void schway__sha1_block(schway__sha1 *state, const uint8_t block[64])
{
    uint32_t words[80];
    uint32_t a = state->h[0];
    uint32_t b = state->h[1];
    uint32_t c = state->h[2];
    uint32_t d = state->h[3];
    uint32_t e = state->h[4];
    for (unsigned i = 0u; i < 16u; i++) words[i] = schway__sha1_be32(block + i * 4u);
    for (unsigned i = 16u; i < 80u; i++) {
        words[i] = schway__sha1_rotl(words[i - 3u] ^ words[i - 8u] ^
                                     words[i - 14u] ^ words[i - 16u], 1u);
    }
    for (unsigned i = 0u; i < 80u; i++) {
        uint32_t f;
        uint32_t k;
        uint32_t next;
        if (i < 20u) {
            f = (b & c) | (~b & d);
            k = 0x5A827999u;
        } else if (i < 40u) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1u;
        } else if (i < 60u) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDCu;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6u;
        }
        next = schway__sha1_rotl(a, 5u) + f + e + k + words[i];
        e = d;
        d = c;
        c = schway__sha1_rotl(b, 30u);
        b = a;
        a = next;
    }
    state->h[0] += a;
    state->h[1] += b;
    state->h[2] += c;
    state->h[3] += d;
    state->h[4] += e;
}

void schway__sha1_init(schway__sha1 *state)
{
    state->h[0] = 0x67452301u;
    state->h[1] = 0xEFCDAB89u;
    state->h[2] = 0x98BADCFEu;
    state->h[3] = 0x10325476u;
    state->h[4] = 0xC3D2E1F0u;
    state->used = 0u;
    state->total = 0u;
}

void schway__sha1_update(schway__sha1 *state, const void *data, size_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    state->total += (uint64_t)size;
    while (size != 0u) {
        size_t count = sizeof state->block - state->used;
        if (count > size) count = size;
        memcpy(state->block + state->used, bytes, count);
        state->used += (uint32_t)count;
        bytes += count;
        size -= count;
        if (state->used == sizeof state->block) {
            schway__sha1_block(state, state->block);
            state->used = 0u;
        }
    }
}

void schway__sha1_final(schway__sha1 *state, uint8_t digest[20])
{
    uint64_t bits = state->total * 8u;
    state->block[state->used++] = 0x80u;
    if (state->used > 56u) {
        memset(state->block + state->used, 0, sizeof state->block - state->used);
        schway__sha1_block(state, state->block);
        state->used = 0u;
    }
    memset(state->block + state->used, 0, 56u - state->used);
    for (unsigned i = 0u; i < 8u; i++) {
        state->block[63u - i] = (uint8_t)(bits >> (i * 8u));
    }
    schway__sha1_block(state, state->block);
    for (unsigned i = 0u; i < 5u; i++) {
        digest[i * 4u] = (uint8_t)(state->h[i] >> 24);
        digest[i * 4u + 1u] = (uint8_t)(state->h[i] >> 16);
        digest[i * 4u + 2u] = (uint8_t)(state->h[i] >> 8);
        digest[i * 4u + 3u] = (uint8_t)state->h[i];
    }
    memset(state, 0, sizeof *state);
}

void schway__sha1_hex(const uint8_t digest[20], char hex[41])
{
    static const char digits[] = "0123456789abcdef";
    for (unsigned i = 0u; i < 20u; i++) {
        hex[i * 2u] = digits[digest[i] >> 4];
        hex[i * 2u + 1u] = digits[digest[i] & 0x0Fu];
    }
    hex[40] = '\0';
}
