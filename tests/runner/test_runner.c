/* The runner's bench statistics, one case per argv[1]: percentile, bytes
   or format. */
#include <stdint.h>
#include <string.h>

#include "../../runner/bench.h"
#include "../check.h"

static const uint32_t all_ppm[] = {500000u, 900000u, 990000u, 999000u, 999900u, 999990u, 1000000u};

/* 300,000 samples, static rather than on the stack. */
#define BIG 300000u
static uint32_t big[BIG];

/* Sorts n samples and checks the seven reported values against want. */
static void expect_stats(const char *what, uint32_t *us, uint32_t n, const uint32_t want[7])
{
    uint32_t got[7];
    schway__bench_stats(us, n, got);
    for (int i = 0; i < 7; i++) {
        if (got[i] != want[i]) {
            fprintf(stderr, "%s: value %d is %lu, expected %lu\n", what, i, (unsigned long)got[i],
                    (unsigned long)want[i]);
            check_failures++;
        }
    }
}

/* Nearest rank is ceil(n * p), clamped to 1..n, and every reported value is
   a sample at that rank of the sorted input, never an interpolation. */
static void percentile(void)
{
    for (size_t i = 0; i < sizeof all_ppm / sizeof all_ppm[0]; i++) {
        CHECK_EQ_U64(schway__bench_rank(1u, all_ppm[i]), 1u);
    }
    CHECK_EQ_U64(schway__bench_rank(2u, 500000u), 1u);
    CHECK_EQ_U64(schway__bench_rank(2u, 900000u), 2u);
    CHECK_EQ_U64(schway__bench_rank(100u, 990000u), 99u);
    CHECK_EQ_U64(schway__bench_rank(100u, 999000u), 100u);
    CHECK_EQ_U64(schway__bench_rank(BIG, 999990u), 299997u);
    CHECK_EQ_U64(schway__bench_rank(BIG, 500000u), 150000u);
    CHECK_EQ_U64(schway__bench_rank(BIG, 1000000u), BIG);
    CHECK_EQ_U64(schway__bench_rank(7u, 500000u), 4u);
    CHECK_EQ_U64(schway__bench_rank(0xffffffffu, 999990u), 4294924346u);
    CHECK_EQ_U64(schway__bench_us(1999u), 1u);
    CHECK_EQ_U64(schway__bench_us(0xffffffffffffffffu), 0xffffffffu);

    /* One sample: every percentile is that sample. */
    uint32_t one[1] = {7u};
    const uint32_t want_one[7] = {7u, 7u, 7u, 7u, 7u, 7u, 7u};
    expect_stats("one sample", one, 1u, want_one);

    /* Two samples, given in descending order. */
    uint32_t two[2] = {5u, 3u};
    const uint32_t want_two[7] = {3u, 5u, 5u, 5u, 5u, 5u, 5u};
    expect_stats("two samples", two, 2u, want_two);

    /* 100 samples 100 down to 1: the p-th percentile is p itself. */
    uint32_t hundred[100];
    for (uint32_t i = 0; i < 100u; i++) {
        hundred[i] = 100u - i;
    }
    const uint32_t want_hundred[7] = {50u, 90u, 99u, 100u, 100u, 100u, 100u};
    expect_stats("100 descending", hundred, 100u, want_hundred);

    /* 300,000 equal samples. */
    for (uint32_t i = 0; i < BIG; i++) {
        big[i] = 42u;
    }
    const uint32_t want_equal[7] = {42u, 42u, 42u, 42u, 42u, 42u, 42u};
    expect_stats("300000 equal", big, BIG, want_equal);

    /* 300,000 samples BIG down to 1: each percentile is its rank. */
    for (uint32_t i = 0; i < BIG; i++) {
        big[i] = BIG - i;
    }
    const uint32_t want_big[7] = {150000u, 270000u, 297000u, 299700u, 299970u, 299997u, BIG};
    expect_stats("300000 descending", big, BIG, want_big);
}

/* The sample buffer's size is exact up to the cap and 0 above it, never a
   wrapped product: on 32 bits, 2^30 frames would wrap to 0 bytes. */
static void bytes(void)
{
    CHECK_EQ_U64(schway__bench_bytes(0u), 0u);
    CHECK_EQ_U64(schway__bench_bytes(1u), 4u);
    CHECK_EQ_U64(schway__bench_bytes(BIG), 1200000u);
    CHECK_EQ_U64(schway__bench_bytes(SCHWAY__BENCH_MAX_FRAMES), 0xfffffffcu);
    CHECK_EQ_U64(schway__bench_bytes(SCHWAY__BENCH_MAX_FRAMES + 1u), 0u);
    CHECK_EQ_U64(schway__bench_bytes(0xffffffffu), 0u);
}

/* The report is nine "key value" lines in a fixed order, from sorted
   samples. */
static void format(void)
{
    uint32_t us[4] = {40u, 10u, 30u, 20u};
    char buf[512];
    const char *want = "frames 4\n"
                       "p50_us 20\n"
                       "p90_us 40\n"
                       "p99_us 40\n"
                       "p99.9_us 40\n"
                       "p99.99_us 40\n"
                       "p99.999_us 40\n"
                       "max_us 40\n"
                       "max_rss_kib 123\n";
    size_t len = schway__bench_format(buf, sizeof buf, us, 4u, 123u);
    CHECK_EQ_U64(len, strlen(want));
    CHECK(len == strlen(want) && strcmp(buf, want) == 0);
    if (len != strlen(want) || strcmp(buf, want) != 0) {
        fprintf(stderr, "want:\n%sgot:\n%.*s", want, (int)len, buf);
    }
    CHECK_EQ_U64(us[0], 10u);
    CHECK_EQ_U64(schway__bench_format(buf, 20u, us, 4u, 123u), 0u);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-bench percentile|bytes|format\n");
        return 2;
    }
    if (strcmp(argv[1], "percentile") == 0) {
        percentile();
    } else if (strcmp(argv[1], "bytes") == 0) {
        bytes();
    } else if (strcmp(argv[1], "format") == 0) {
        format();
    } else {
        fprintf(stderr, "unknown case %s\n", argv[1]);
        return 2;
    }
    CHECK_DONE();
}
