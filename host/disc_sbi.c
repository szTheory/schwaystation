#include "disc_sbi.h"

#include <stdlib.h>
#include <string.h>

static uint32_t schway__disc_sbi_bcd(uint8_t value, uint32_t maximum,
                                    uint32_t *out)
{
    uint32_t high = value >> 4;
    uint32_t low = value & 0x0Fu;
    if (high > 9u || low > 9u) return 0u;
    *out = high * 10u + low;
    return *out <= maximum;
}

static schway_disc_sbi_status schway__disc_sbi_lba(
    const uint8_t msf[3], uint32_t lba_count, uint32_t *out)
{
    uint32_t minute;
    uint32_t second;
    uint32_t frame;
    if (schway__disc_sbi_bcd(msf[0], 99u, &minute) == 0u ||
        schway__disc_sbi_bcd(msf[1], 59u, &second) == 0u ||
        schway__disc_sbi_bcd(msf[2], 74u, &frame) == 0u) {
        return SCHWAY_DISC_SBI_BAD_RECORD;
    }
    uint64_t absolute = (uint64_t)minute * UINT64_C(4500) +
                        (uint64_t)second * UINT64_C(75) + frame;
    if (absolute < UINT64_C(150)) return SCHWAY_DISC_SBI_BAD_RECORD;
    uint64_t lba = absolute - UINT64_C(150);
    if (lba >= lba_count || lba > UINT32_MAX) return SCHWAY_DISC_SBI_OUT_OF_RANGE;
    *out = (uint32_t)lba;
    return SCHWAY_DISC_SBI_OK;
}

static int schway__disc_sbi_compare_lba(const void *left, const void *right)
{
    uint32_t a = *(const uint32_t *)left;
    uint32_t b = *(const uint32_t *)right;
    return a < b ? -1 : a > b ? 1 : 0;
}

schway_disc_sbi_status schway_disc_sbi_parse(const uint8_t *bytes, size_t size,
                                              uint32_t lba_count,
                                              schway_disc_sbi *out)
{
    schway_disc_sbi candidate = {0};
    size_t capacity;
    size_t offset = 4u;
    if (bytes == NULL || out == NULL || out->lbas != NULL || out->count != 0u) {
        return SCHWAY_DISC_SBI_BAD_ARGUMENT;
    }
    if (size < 4u || memcmp(bytes, "SBI\0", 4u) != 0) {
        return SCHWAY_DISC_SBI_BAD_HEADER;
    }
    if (size > SCHWAY_DISC_SBI_MAX_BYTES) return SCHWAY_DISC_SBI_LIMIT;
    capacity = (size - 4u) / 7u;
    if (capacity != 0u) {
        candidate.lbas = (uint32_t *)malloc(capacity * sizeof *candidate.lbas);
        if (candidate.lbas == NULL) return SCHWAY_DISC_SBI_OUT_OF_MEMORY;
    }
    while (offset < size) {
        uint32_t lba;
        size_t record_size;
        if (size - offset < 4u) {
            free(candidate.lbas);
            return SCHWAY_DISC_SBI_TRUNCATED;
        }
        if (bytes[offset + 3u] == 1u) {
            record_size = 14u;
        } else if (bytes[offset + 3u] == 2u || bytes[offset + 3u] == 3u) {
            record_size = 7u;
        } else {
            free(candidate.lbas);
            return SCHWAY_DISC_SBI_BAD_RECORD;
        }
        if (size - offset < record_size) {
            free(candidate.lbas);
            return SCHWAY_DISC_SBI_TRUNCATED;
        }
        schway_disc_sbi_status status = schway__disc_sbi_lba(bytes + offset,
                                                             lba_count, &lba);
        if (status != SCHWAY_DISC_SBI_OK) {
            free(candidate.lbas);
            return status;
        }
        if (candidate.count == SCHWAY_DISC_SBI_MAX_RECORDS) {
            free(candidate.lbas);
            return SCHWAY_DISC_SBI_LIMIT;
        }
        candidate.lbas[candidate.count++] = lba;
        offset += record_size;
    }
    if (candidate.count > 1u) {
        qsort(candidate.lbas, candidate.count, sizeof *candidate.lbas,
              schway__disc_sbi_compare_lba);
        size_t unique = 1u;
        for (size_t i = 1u; i < candidate.count; i++) {
            if (candidate.lbas[i] != candidate.lbas[unique - 1u]) {
                candidate.lbas[unique++] = candidate.lbas[i];
            }
        }
        candidate.count = unique;
    }
    *out = candidate;
    return SCHWAY_DISC_SBI_OK;
}

void schway_disc_sbi_clear(schway_disc_sbi *overlay)
{
    if (overlay == NULL) return;
    free(overlay->lbas);
    overlay->lbas = NULL;
    overlay->count = 0u;
}
