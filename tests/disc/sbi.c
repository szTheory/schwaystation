#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "disc_sbi.h"
#include "../check.h"

static void test_format_one(void)
{
    static const uint8_t bytes[] = {
        'S', 'B', 'I', 0,
        0x00u, 0x03u, 0x00u, 0x01u,
        0x41u, 0x01u, 0x01u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x03u, 0x00u
    };
    schway_disc_sbi overlay = {0};
    CHECK_EQ_U64(schway_disc_sbi_parse(bytes, sizeof bytes, 100u, &overlay),
                 SCHWAY_DISC_SBI_OK);
    CHECK_EQ_U64(overlay.count, 1u);
    if (overlay.count == 1u) CHECK_EQ_U64(overlay.lbas[0], 75u);
    schway_disc_sbi_clear(&overlay);
}

static void test_bad_header_is_atomic(void)
{
    static const uint8_t bytes[] = {
        'X', 'B', 'I', 0,
        0x00u, 0x03u, 0x00u, 0x01u,
        0x41u, 0x01u, 0x01u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x03u, 0x00u
    };
    schway_disc_sbi overlay = {0};
    CHECK_EQ_U64(schway_disc_sbi_parse(bytes, sizeof bytes, 100u, &overlay),
                 SCHWAY_DISC_SBI_BAD_HEADER);
    CHECK(overlay.lbas == NULL);
    CHECK_EQ_U64(overlay.count, 0u);
}

static void test_truncated_record_is_atomic(void)
{
    static const uint8_t bytes[] = {
        'S', 'B', 'I', 0,
        0x00u, 0x03u, 0x00u, 0x01u,
        0x41u, 0x01u, 0x01u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x03u
    };
    schway_disc_sbi overlay = {0};
    CHECK_EQ_U64(schway_disc_sbi_parse(bytes, sizeof bytes, 100u, &overlay),
                 SCHWAY_DISC_SBI_TRUNCATED);
    CHECK(overlay.lbas == NULL);
    CHECK_EQ_U64(overlay.count, 0u);
    for (size_t size = 5u; size < sizeof bytes; size++) {
        CHECK_EQ_U64(schway_disc_sbi_parse(bytes, size, 100u, &overlay),
                     SCHWAY_DISC_SBI_TRUNCATED);
    }
}

static void test_formats_two_and_three(void)
{
    static const uint8_t bytes[] = {
        'S', 'B', 'I', 0,
        0x00u, 0x03u, 0x00u, 0x02u, 0xFAu, 0xFFu, 0x7Fu,
        0x00u, 0x04u, 0x00u, 0x03u, 0xFFu, 0xFAu, 0x7Fu
    };
    schway_disc_sbi overlay = {0};
    CHECK_EQ_U64(schway_disc_sbi_parse(bytes, sizeof bytes, 200u, &overlay),
                 SCHWAY_DISC_SBI_OK);
    CHECK_EQ_U64(overlay.count, 2u);
    if (overlay.count == 2u) {
        CHECK_EQ_U64(overlay.lbas[0], 75u);
        CHECK_EQ_U64(overlay.lbas[1], 150u);
    }
    schway_disc_sbi_clear(&overlay);
}

static void test_duplicate_records_are_sorted_and_idempotent(void)
{
    static const uint8_t bytes[] = {
        'S', 'B', 'I', 0,
        0x00u, 0x04u, 0x00u, 0x01u,
        0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu,
        0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu,
        0x00u, 0x03u, 0x00u, 0x02u, 0xFAu, 0xFAu, 0xFAu,
        0x00u, 0x04u, 0x00u, 0x03u, 0xFFu, 0xFFu, 0xFFu
    };
    schway_disc_sbi overlay = {0};
    CHECK_EQ_U64(schway_disc_sbi_parse(bytes, sizeof bytes, 200u, &overlay),
                 SCHWAY_DISC_SBI_OK);
    CHECK_EQ_U64(overlay.count, 2u);
    if (overlay.count == 2u) {
        CHECK_EQ_U64(overlay.lbas[0], 75u);
        CHECK_EQ_U64(overlay.lbas[1], 150u);
    }
    schway_disc_sbi_clear(&overlay);
}

static void test_invalid_addresses_and_bounds(void)
{
    static const uint8_t header[] = {'S', 'B', 'I', 0};
    static const uint8_t bad_bcd[] = {
        'S', 'B', 'I', 0, 0x0Au, 0x03u, 0x00u, 0x02u, 0x01u, 0x02u, 0x03u
    };
    static const uint8_t bad_seconds[] = {
        'S', 'B', 'I', 0, 0x00u, 0x60u, 0x00u, 0x02u, 0x01u, 0x02u, 0x03u
    };
    static const uint8_t bad_frames[] = {
        'S', 'B', 'I', 0, 0x00u, 0x03u, 0x75u, 0x02u, 0x01u, 0x02u, 0x03u
    };
    static const uint8_t before_leadin[] = {
        'S', 'B', 'I', 0, 0x00u, 0x01u, 0x74u, 0x02u, 0x01u, 0x02u, 0x03u
    };
    static const uint8_t outside_map[] = {
        'S', 'B', 'I', 0, 0x00u, 0x04u, 0x00u, 0x02u, 0x01u, 0x02u, 0x03u
    };
    static const uint8_t unknown_format[] = {
        'S', 'B', 'I', 0, 0x00u, 0x03u, 0x00u, 0x04u, 0x01u, 0x02u, 0x03u
    };
    static const uint8_t short_format_two[] = {
        'S', 'B', 'I', 0, 0x00u, 0x03u, 0x00u, 0x02u, 0x01u, 0x02u
    };
    static const uint8_t empty = 0u;
    schway_disc_sbi overlay = {0};
    CHECK_EQ_U64(schway_disc_sbi_parse(&empty, 0u, 100u, &overlay),
                 SCHWAY_DISC_SBI_BAD_HEADER);
    CHECK_EQ_U64(schway_disc_sbi_parse(header, sizeof header, 100u, &overlay),
                 SCHWAY_DISC_SBI_OK);
    CHECK(overlay.lbas == NULL);
    CHECK_EQ_U64(overlay.count, 0u);
    CHECK_EQ_U64(schway_disc_sbi_parse(bad_bcd, sizeof bad_bcd, 200u, &overlay),
                 SCHWAY_DISC_SBI_BAD_RECORD);
    CHECK_EQ_U64(schway_disc_sbi_parse(bad_seconds, sizeof bad_seconds, 200u, &overlay),
                 SCHWAY_DISC_SBI_BAD_RECORD);
    CHECK_EQ_U64(schway_disc_sbi_parse(bad_frames, sizeof bad_frames, 200u, &overlay),
                 SCHWAY_DISC_SBI_BAD_RECORD);
    CHECK_EQ_U64(schway_disc_sbi_parse(before_leadin, sizeof before_leadin, 200u, &overlay),
                 SCHWAY_DISC_SBI_BAD_RECORD);
    CHECK_EQ_U64(schway_disc_sbi_parse(outside_map, sizeof outside_map, 100u, &overlay),
                 SCHWAY_DISC_SBI_OUT_OF_RANGE);
    CHECK_EQ_U64(schway_disc_sbi_parse(unknown_format, sizeof unknown_format, 200u, &overlay),
                 SCHWAY_DISC_SBI_BAD_RECORD);
    CHECK_EQ_U64(schway_disc_sbi_parse(short_format_two, sizeof short_format_two, 200u,
                                       &overlay), SCHWAY_DISC_SBI_TRUNCATED);
    for (size_t size = 5u; size < sizeof short_format_two; size++) {
        CHECK_EQ_U64(schway_disc_sbi_parse(short_format_two, size, 200u, &overlay),
                     SCHWAY_DISC_SBI_TRUNCATED);
    }
    CHECK(overlay.lbas == NULL);
    CHECK_EQ_U64(overlay.count, 0u);
}

static void test_file_and_record_limits(void)
{
    size_t size = (size_t)SCHWAY_DISC_SBI_MAX_BYTES + 1u;
    uint8_t *bytes = (uint8_t *)calloc(size, 1u);
    schway_disc_sbi overlay = {0};
    CHECK(bytes != NULL);
    if (bytes == NULL) return;
    memcpy(bytes, "SBI\0", 4u);
    CHECK_EQ_U64(schway_disc_sbi_parse(bytes, size, 200u, &overlay),
                 SCHWAY_DISC_SBI_LIMIT);
    free(bytes);

    size = 4u + (size_t)(SCHWAY_DISC_SBI_MAX_RECORDS + 1u) * 7u;
    bytes = (uint8_t *)malloc(size);
    CHECK(bytes != NULL);
    if (bytes == NULL) return;
    memcpy(bytes, "SBI\0", 4u);
    for (size_t i = 0u; i < SCHWAY_DISC_SBI_MAX_RECORDS + 1u; i++) {
        size_t at = 4u + i * 7u;
        bytes[at] = 0x00u;
        bytes[at + 1u] = 0x03u;
        bytes[at + 2u] = 0x00u;
        bytes[at + 3u] = 0x02u;
        bytes[at + 4u] = 0xFFu;
        bytes[at + 5u] = 0xFFu;
        bytes[at + 6u] = 0xFFu;
    }
    CHECK_EQ_U64(schway_disc_sbi_parse(bytes, size, 200u, &overlay),
                 SCHWAY_DISC_SBI_LIMIT);
    free(bytes);
    CHECK(overlay.lbas == NULL);
    CHECK_EQ_U64(overlay.count, 0u);
}

int main(void)
{
    test_format_one();
    test_bad_header_is_atomic();
    test_truncated_record_is_atomic();
    test_formats_two_and_three();
    test_duplicate_records_are_sorted_and_idempotent();
    test_invalid_addresses_and_bounds();
    test_file_and_record_limits();
    CHECK_DONE();
}
