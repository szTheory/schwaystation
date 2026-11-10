/* The fixture mailbox: layout and rules are in tests/fixtures/src/mailbox.inc and docs/runner.md.
   The runner reads it with schway_peek, so the core needs no knowledge of it. */
#include "mailbox.h"

#include "crc32.h"

#define MAILBOX_BASE 0x10000u /* RAM offset of the load address 80010000h */
#define MAILBOX_SIZE 0x100u
#define MAILBOX_MAGIC 0x424D5753u /* "SWMB", little-endian */
#define MAILBOX_RECORDS 0x10u
#define MAILBOX_RECORDS_MAX 30u
#define STATUS_DONE 0x00u
#define STATUS_FAULT 0xFFu

static int peek32(const schway *inst, uint32_t offset, uint32_t *value)
{
    return schway_peek(inst, SCHWAY_SPACE_RAM, MAILBOX_BASE + offset, 4u, value) ==
           SCHWAY_STATUS_OK;
}

int schway__mailbox_check(const schway *inst, FILE *out)
{
    uint32_t magic = 0u;
    uint32_t head = 0u;
    if (!peek32(inst, 0u, &magic) || magic != MAILBOX_MAGIC || !peek32(inst, 8u, &head)) {
        return SCHWAY__MAILBOX_PENDING;
    }
    uint32_t status = head & 0xFFu;
    if (status != STATUS_DONE && status != STATUS_FAULT) {
        return SCHWAY__MAILBOX_PENDING;
    }
    /* The whole mailbox, as bytes, so the CRC and the records read the same copy. */
    uint8_t mb[MAILBOX_SIZE];
    for (uint32_t i = 0; i < MAILBOX_SIZE; i += 4u) {
        uint32_t w = 0u;
        if (!peek32(inst, i, &w)) {
            return SCHWAY__MAILBOX_PENDING;
        }
        for (uint32_t b = 0; b < 4u; b++) {
            mb[i + b] = (uint8_t)(w >> (8u * b));
        }
    }
    /* A fault leaves the CRC as the fixture last wrote it, so only a done status is checked. */
    if (status == STATUS_DONE) {
        uint32_t want = (uint32_t)mb[4] | ((uint32_t)mb[5] << 8) | ((uint32_t)mb[6] << 16) |
                        ((uint32_t)mb[7] << 24);
        if (schway__runner_crc32(0u, mb + MAILBOX_RECORDS, MAILBOX_SIZE - MAILBOX_RECORDS) !=
            want) {
            fputs("mailbox crc mismatch\n", out);
            return SCHWAY__MAILBOX_BAD_CRC;
        }
    }
    uint32_t count = mb[0x0A];
    if (count > MAILBOX_RECORDS_MAX) {
        count = MAILBOX_RECORDS_MAX;
    }
    fprintf(out, "mailbox SWMB status=%02lx records=%lu\n", (unsigned long)status,
            (unsigned long)count);
    for (uint32_t n = 0; n < count; n++) {
        const uint8_t *r = mb + MAILBOX_RECORDS + 8u * n;
        uint32_t id = (uint32_t)r[0] | ((uint32_t)r[1] << 8);
        uint32_t value = (uint32_t)r[4] | ((uint32_t)r[5] << 8) | ((uint32_t)r[6] << 16) |
                         ((uint32_t)r[7] << 24);
        fprintf(out, "mailbox %04lx=%08lx\n", (unsigned long)id, (unsigned long)value);
    }
    return status == STATUS_DONE ? SCHWAY__MAILBOX_DONE : SCHWAY__MAILBOX_FAULT;
}
