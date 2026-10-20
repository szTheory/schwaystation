/* Frame-time statistics for --bench. Runner-only and integer-only.

   Percentiles use the nearest-rank method with no interpolation: the rank
   is ceil(n * p), with p in parts per million so p99.999 is exact. Each
   reported value is therefore a measured sample. */
#ifndef SCHWAYSTATION_RUNNER_BENCH_H
#define SCHWAYSTATION_RUNNER_BENCH_H

#include <stddef.h>
#include <stdint.h>

/* The largest sample count the buffer arithmetic allows: one uint32_t per
   frame, so the buffer stays below 4 GiB and its size fits a 32-bit size_t.
   The command line allows far fewer. */
#define SCHWAY__BENCH_MAX_FRAMES 1073741823u

/* The one-based nearest rank of ppm parts per million in n >= 1 samples,
   clamped to 1..n. */
uint32_t schway__bench_rank(uint32_t n, uint32_t ppm);

/* The bytes of one uint32_t sample per frame for n frames, or 0 when n is 0
   or above SCHWAY__BENCH_MAX_FRAMES. */
size_t schway__bench_bytes(uint32_t n);

/* Microseconds from nanoseconds, truncated and saturated at UINT32_MAX. */
uint32_t schway__bench_us(uint64_t ns);

/* Sorts n >= 1 microsecond samples in place and writes the seven reported
   values to out: p50, p90, p99, p99.9, p99.99, p99.999 and max. */
void schway__bench_stats(uint32_t *us, uint32_t n, uint32_t out[7]);

/* Sorts n >= 1 microsecond samples in place and writes the report into buf,
   one "key value" line each:

     frames p50_us p90_us p99_us p99.9_us p99.99_us p99.999_us max_us
     max_rss_kib

   Returns the length written, or 0 when cap is too small. */
size_t schway__bench_format(char *buf, size_t cap, uint32_t *us, uint32_t n,
                            uint64_t max_rss_kib);

#endif /* SCHWAYSTATION_RUNNER_BENCH_H */
