#include "bench.h"

#include <stdio.h>
#include <stdlib.h>

/* Nearest rank: the smallest sample with at least p of the samples at or
   below it, so the rank is ceil(n * p) (NIST/SEMATECH e-Handbook of
   Statistical Methods, 7.2.6.2; no interpolation). */
uint32_t schway__bench_rank(uint32_t n, uint32_t ppm)
{
    uint64_t rank = ((uint64_t)n * ppm + 999999u) / 1000000u;
    if (rank < 1u) {
        rank = 1u;
    }
    if (rank > n) {
        rank = n;
    }
    return (uint32_t)rank;
}

/* The cap keeps the product below 2^32, so it fits a 32-bit size_t too. */
size_t schway__bench_bytes(uint32_t n)
{
    if (n == 0u || n > SCHWAY__BENCH_MAX_FRAMES) {
        return 0u;
    }
    return (size_t)n * sizeof(uint32_t);
}

uint32_t schway__bench_us(uint64_t ns)
{
    uint64_t us = ns / 1000u;
    return us > 0xffffffffu ? 0xffffffffu : (uint32_t)us;
}

static int compare_u32(const void *a, const void *b)
{
    uint32_t x = *(const uint32_t *)a;
    uint32_t y = *(const uint32_t *)b;
    return (x > y) - (x < y);
}

/* The reported percentiles in parts per million, in report order; the last
   is the maximum. */
static const struct {
    const char *key;
    uint32_t ppm;
} percentiles[7] = {
    {"p50_us", 500000u},    {"p90_us", 900000u},     {"p99_us", 990000u},  {"p99.9_us", 999000u},
    {"p99.99_us", 999900u}, {"p99.999_us", 999990u}, {"max_us", 1000000u},
};

void schway__bench_stats(uint32_t *us, uint32_t n, uint32_t out[7])
{
    qsort(us, n, sizeof us[0], compare_u32);
    for (size_t i = 0; i < 7u; i++) {
        out[i] = us[schway__bench_rank(n, percentiles[i].ppm) - 1u];
    }
}

size_t schway__bench_format(char *buf, size_t cap, uint32_t *us, uint32_t n,
                            uint64_t max_rss_kib)
{
    uint32_t v[7];
    schway__bench_stats(us, n, v);
    int w = snprintf(buf, cap, "frames %lu\n", (unsigned long)n);
    size_t len = w < 0 ? cap : (size_t)w;
    for (size_t i = 0; i < 7u && len < cap; i++) {
        w = snprintf(buf + len, cap - len, "%s %lu\n", percentiles[i].key, (unsigned long)v[i]);
        len += w < 0 ? cap : (size_t)w;
    }
    if (len < cap) {
        w = snprintf(buf + len, cap - len, "max_rss_kib %llu\n", (unsigned long long)max_rss_kib);
        len += w < 0 ? cap : (size_t)w;
    }
    return len < cap ? len : 0u;
}
