#include <stdint.h>

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
}

int main(void)
{
    test_format_one();
    test_bad_header_is_atomic();
    test_truncated_record_is_atomic();
    CHECK_DONE();
}
