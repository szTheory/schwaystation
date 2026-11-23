/* Bounded fuzzing of hostile SBI sidecars through the production parser. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disc_sbi.h"

static void broken(const char *what)
{
    fprintf(stderr, "fuzz.regress.sbi: %s\n", what);
    abort();
}

static int matches(const uint8_t *bytes, size_t size, const uint8_t *expected,
                   size_t expected_size)
{
    return size == expected_size && memcmp(bytes, expected, expected_size) == 0;
}

static int is_record_bound_seed(const uint8_t *bytes, size_t size)
{
    static const uint8_t record[] = {0u, 2u, 0u, 2u, 0x11u, 0x22u, 0x33u};
    size_t expected_size = 4u + (SCHWAY_DISC_SBI_MAX_RECORDS + 1u) * sizeof record;
    if (size != expected_size || memcmp(bytes, "SBI\0", 4u) != 0) return 0;
    for (size_t offset = 4u; offset < size; offset += sizeof record) {
        if (memcmp(bytes + offset, record, sizeof record) != 0) return 0;
    }
    return 1;
}

static int expected_seed_status(const uint8_t *data, size_t size)
{
    static const uint8_t valid[] = {'S', 'B', 'I', 0u, 0u, 2u, 0u, 2u, 0x11u, 0x22u, 0x33u};
    static const uint8_t truncated[] = {'S', 'B', 'I', 0u, 0u, 2u, 0u};
    static const uint8_t bad_header[] = {'B', 'A', 'D', 0u};
    static const uint8_t invalid_format[] = {'S', 'B', 'I', 0u, 0u, 2u, 0u, 4u, 0u, 0u, 0u};
    static const uint8_t invalid_bcd[] = {'S', 'B', 'I', 0u, 0xFAu, 2u, 0u, 2u, 0u, 0u, 0u};
    static const uint8_t out_of_range[] = {'S', 'B', 'I', 0u, 0u, 0x20u, 0u, 2u, 0u, 0u, 0u};
    if (matches(data, size, valid, sizeof valid)) return SCHWAY_DISC_SBI_OK;
    if (matches(data, size, truncated, sizeof truncated)) return SCHWAY_DISC_SBI_TRUNCATED;
    if (matches(data, size, bad_header, sizeof bad_header)) return SCHWAY_DISC_SBI_BAD_HEADER;
    if (matches(data, size, invalid_format, sizeof invalid_format)) return SCHWAY_DISC_SBI_BAD_RECORD;
    if (matches(data, size, invalid_bcd, sizeof invalid_bcd)) return SCHWAY_DISC_SBI_BAD_RECORD;
    if (matches(data, size, out_of_range, sizeof out_of_range)) return SCHWAY_DISC_SBI_OUT_OF_RANGE;
    if (is_record_bound_seed(data, size)) return SCHWAY_DISC_SBI_LIMIT;
    return -1;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    schway_disc_sbi parsed = {NULL, 0u};
    schway_disc_sbi_status status = schway_disc_sbi_parse(data, size, 1000u, &parsed);
    int expected = expected_seed_status(data, size);
    switch (status) {
    case SCHWAY_DISC_SBI_OK:
        if ((parsed.count == 0u && parsed.lbas != NULL) ||
            (parsed.count != 0u && parsed.lbas == NULL) ||
            parsed.count > SCHWAY_DISC_SBI_MAX_RECORDS) {
            broken("successful parse returned invalid bounds");
        }
        for (size_t i = 0u; i < parsed.count; i++) {
            if (parsed.lbas[i] >= 1000u || (i != 0u && parsed.lbas[i - 1u] >= parsed.lbas[i])) {
                broken("successful parse returned an unsorted or out-of-range LBA");
            }
        }
        break;
    case SCHWAY_DISC_SBI_BAD_HEADER:
    case SCHWAY_DISC_SBI_TRUNCATED:
    case SCHWAY_DISC_SBI_BAD_RECORD:
    case SCHWAY_DISC_SBI_OUT_OF_RANGE:
    case SCHWAY_DISC_SBI_LIMIT:
    case SCHWAY_DISC_SBI_OUT_OF_MEMORY:
        if (parsed.lbas != NULL || parsed.count != 0u) {
            broken("failed parse changed its output");
        }
        break;
    default:
        broken("parser returned an unexpected status");
    }
    if (expected >= 0 && status != (schway_disc_sbi_status)expected) {
        broken("known seed changed its parser status");
    }
    if (expected == SCHWAY_DISC_SBI_OK && (parsed.count != 1u || parsed.lbas[0] != 0u)) {
        broken("valid seed returned an invalid overlay");
    }
    schway_disc_sbi_clear(&parsed);
    return 0;
}

#define SCHWAY_REPLAY_NAME "fuzz.regress.sbi"
#include "replay_main.h"
