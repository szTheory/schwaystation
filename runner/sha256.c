/* SHA-256 as specified in FIPS 180-4, sections 4.1.2, 4.2.2, 5.1.1, 5.3.3
   and 6.2. Written from the standard. */
#include "sha256.h"

#include <string.h>

/* FIPS 180-4 section 4.2.2: first 32 bits of the fractional parts of the
   cube roots of the first 64 primes. */
static const uint32_t K[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u,
    0xab1c5ed5u, 0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu,
    0x9bdc06a7u, 0xc19bf174u, 0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu,
    0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau, 0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu,
    0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u, 0xa2bfe8a1u, 0xa81a664bu,
    0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u, 0x19a4c116u,
    0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u,
    0xc67178f2u,
};

static uint32_t rotr(uint32_t x, uint32_t n)
{
    return (x >> n) | (x << (32u - n));
}

/* FIPS 180-4 section 6.2.2: process one 512-bit block. */
static void compress(uint32_t h[8], const uint8_t block[64])
{
    uint32_t w[64];
    for (uint32_t t = 0; t < 16u; t++) {
        w[t] = ((uint32_t)block[4u * t] << 24) | ((uint32_t)block[4u * t + 1u] << 16) |
               ((uint32_t)block[4u * t + 2u] << 8) | (uint32_t)block[4u * t + 3u];
    }
    for (uint32_t t = 16; t < 64u; t++) {
        uint32_t s0 = rotr(w[t - 15u], 7) ^ rotr(w[t - 15u], 18) ^ (w[t - 15u] >> 3);
        uint32_t s1 = rotr(w[t - 2u], 17) ^ rotr(w[t - 2u], 19) ^ (w[t - 2u] >> 10);
        w[t] = w[t - 16u] + s0 + w[t - 7u] + s1;
    }
    uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
    uint32_t e = h[4], f = h[5], g = h[6], k = h[7];
    for (uint32_t t = 0; t < 64u; t++) {
        uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t t1 = k + S1 + ch + K[t] + w[t];
        uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t2 = S0 + maj;
        k = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
    h[5] += f;
    h[6] += g;
    h[7] += k;
}

void schway__sha256_init(schway__sha256 *s)
{
    /* FIPS 180-4 section 5.3.3. */
    static const uint32_t H0[8] = {
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u,
    };
    memcpy(s->h, H0, sizeof H0);
    s->used = 0;
    s->total = 0;
}

void schway__sha256_update(schway__sha256 *s, const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    s->total += (uint64_t)len;
    while (len > 0u) {
        size_t take = 64u - (size_t)s->used;
        if (take > len) {
            take = len;
        }
        memcpy(s->block + s->used, p, take);
        s->used += (uint32_t)take;
        p += take;
        len -= take;
        if (s->used == 64u) {
            compress(s->h, s->block);
            s->used = 0;
        }
    }
}

void schway__sha256_final(schway__sha256 *s, uint8_t digest[32])
{
    /* FIPS 180-4 section 5.1.1: a 1 bit, zeros, then the 64-bit length in
       bits, big-endian, ending on a block boundary. */
    uint64_t bits = s->total * 8u;
    s->block[s->used++] = 0x80u;
    if (s->used > 56u) {
        memset(s->block + s->used, 0, 64u - (size_t)s->used);
        compress(s->h, s->block);
        s->used = 0;
    }
    memset(s->block + s->used, 0, 56u - (size_t)s->used);
    for (uint32_t i = 0; i < 8u; i++) {
        s->block[56u + i] = (uint8_t)(bits >> (56u - 8u * i));
    }
    compress(s->h, s->block);
    for (uint32_t i = 0; i < 8u; i++) {
        digest[4u * i] = (uint8_t)(s->h[i] >> 24);
        digest[4u * i + 1u] = (uint8_t)(s->h[i] >> 16);
        digest[4u * i + 2u] = (uint8_t)(s->h[i] >> 8);
        digest[4u * i + 3u] = (uint8_t)s->h[i];
    }
}

void schway__sha256_hex(const uint8_t digest[32], char hex[65])
{
    static const char digits[] = "0123456789abcdef";
    for (uint32_t i = 0; i < 32u; i++) {
        hex[2u * i] = digits[digest[i] >> 4];
        hex[2u * i + 1u] = digits[digest[i] & 0x0fu];
    }
    hex[64] = '\0';
}
