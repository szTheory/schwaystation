/* Hostile card protocol replay with independent raw-byte and generation checks.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"
#include "state.h"
#include "schwaystation.h"

#define JOY_DATA 0x1F801040u
#define JOY_MODE 0x1F801048u
#define JOY_CTRL 0x1F80104Au
#define JOY_BAUD 0x1F80104Eu
#define I_STAT 0x1F801070u
#define ACK 1500u
#define READ_PAUSE 31000u

static void broken(const char *what)
{
    fprintf(stderr, "fuzz.regress.memcard: %s\n", what);
    abort();
}

static void *alloc_bytes(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void free_bytes(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static schway *make(void)
{
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = alloc_bytes;
    cfg.allocator.free = free_bytes;
    schway *inst = NULL;
    if (schway_create(&cfg, &inst) != SCHWAY_STATUS_OK || inst == NULL) {
        broken("instance creation failed");
    }
    return inst;
}

static void write_mmio(schway *inst, uint32_t address, uint32_t width, uint32_t value)
{
    uint32_t cycles = 0u;
    if (schway__bus_write(inst, address, width, value, &cycles) != 0u) {
        broken("card bus write was refused");
    }
}

static uint32_t read_mmio(schway *inst, uint32_t address, uint32_t width)
{
    uint32_t cycles = 0u;
    uint32_t value = 0u;
    if (schway__bus_read(inst, address, width, &value, &cycles) != 0u) {
        broken("card bus read was refused");
    }
    return value;
}

static void advance(schway *inst, uint64_t tick)
{
    inst->ticks = tick;
    schway__sched_sync(inst);
    schway__sched_run(inst);
}

static uint8_t transfer(schway *inst, uint8_t sent, uint32_t ack_ticks)
{
    uint64_t start = inst->ticks;
    write_mmio(inst, JOY_DATA, 1u, sent);
    advance(inst, start + 1088u);
    uint8_t received = (uint8_t)read_mmio(inst, JOY_DATA, 1u);
    if (ack_ticks == 0u) {
        if (inst->sio.ack_pending != 0u || inst->sio.ack_low != 0u) {
            fprintf(stderr,
                    "sent %02x ack_pending %u ack_low %u at %llu phase %u command "
                    "%02x read_step %u write_step %u\n",
                    sent, inst->sio.ack_pending, inst->sio.ack_low, (unsigned long long)start,
                    inst->card.slot[0].phase, inst->card.slot[0].command,
                    inst->card.slot[0].read_step, inst->card.slot[0].write_step);
            broken("card terminator unexpectedly acknowledged");
        }
        return received;
    }
    if (inst->sio.ack_pending == 0u || inst->sio.ack_deadline != start + ack_ticks) {
        broken("card ACK deadline changed");
    }
    if (ack_ticks == READ_PAUSE) {
        advance(inst, start + READ_PAUSE - 1u);
        if (inst->sio.ack_low != 0u) {
            broken("Read pause ACK arrived early");
        }
    }
    advance(inst, start + ack_ticks);
    if (inst->sio.ack_low == 0u) {
        broken("card ACK did not assert at deadline");
    }
    write_mmio(inst, I_STAT, 4u, 0u);
    write_mmio(inst, JOY_CTRL, 2u, 0x1013u);
    advance(inst, start + ack_ticks + 68u);
    write_mmio(inst, JOY_CTRL, 2u, 0x1003u);
    return received;
}

static void configure(schway *inst)
{
    write_mmio(inst, JOY_MODE, 2u, 0x000Cu);
    write_mmio(inst, JOY_BAUD, 2u, 0x0088u);
    write_mmio(inst, JOY_CTRL, 2u, 0x1003u);
}

static uint64_t generation(schway *inst)
{
    uint64_t value = UINT64_MAX;
    if (schway_memcard_generation(inst, SCHWAY_CARD_SLOT_1, &value) != SCHWAY_STATUS_OK) {
        broken("card generation query failed");
    }
    return value;
}

struct state_field_location {
    const char *name;
    size_t cursor;
    size_t offset;
    uint32_t width;
    uint32_t count;
    int found;
};

static void locate_state_field(void *user, const char *name, const void *data, uint32_t count,
                               uint32_t width)
{
    struct state_field_location *field = (struct state_field_location *)user;
    (void)data;
    if (field->found == 0 && strcmp(name, field->name) == 0) {
        field->found = 1;
        field->offset = field->cursor;
        field->width = width;
        field->count = count;
    }
    field->cursor += (size_t)count * width;
}

static void store_le32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static void refuse_invalid_pending_state(schway *inst)
{
    size_t size = schway_state_size(inst);
    uint8_t *invalid = (uint8_t *)malloc(size);
    uint8_t *before = (uint8_t *)malloc(size);
    uint8_t *after = (uint8_t *)malloc(size);
    if (invalid == NULL || before == NULL || after == NULL ||
        schway_save_state(inst, invalid, size) != SCHWAY_STATUS_OK) {
        free(after);
        free(before);
        free(invalid);
        broken("could not capture pending card state");
    }
    struct state_field_location field = {"card.slot1.write_step", 0u, 0u, 0u, 0u, 0};
    if (schway_state_visit(inst, locate_state_field, &field) != SCHWAY_STATUS_OK ||
        field.found == 0 || field.width != 4u || field.count != 1u) {
        free(after);
        free(before);
        free(invalid);
        broken("pending card state field was not exposed");
    }
    if (schway_save_state(inst, before, size) != SCHWAY_STATUS_OK) {
        free(after);
        free(before);
        free(invalid);
        broken("could not snapshot pending card state");
    }
    store_le32(invalid + SCHWAY__STATE_HEADER_SIZE + field.offset, UINT32_MAX);
    schway__state_seal(invalid, size);
    if (schway_load_state(inst, invalid, size) != SCHWAY_STATUS_ERR_STATE_FIELD ||
        schway_save_state(inst, after, size) != SCHWAY_STATUS_OK ||
        memcmp(before, after, size) != 0) {
        free(after);
        free(before);
        free(invalid);
        broken("out-of-range pending card state was applied");
    }
    free(after);
    free(before);
    free(invalid);
}

static uint8_t checksum(uint32_t sector, const uint8_t data[128])
{
    uint8_t result = (uint8_t)(sector >> 8) ^ (uint8_t)sector;
    for (uint32_t i = 0u; i < 128u; ++i) {
        result ^= data[i];
    }
    return result;
}

static void write_prefix(schway *inst, uint32_t sector, uint8_t flag)
{
    if (transfer(inst, 0x81u, ACK) != 0xFFu || transfer(inst, 0x57u, ACK) != flag ||
        transfer(inst, 0u, ACK) != 0x5Au || transfer(inst, 0u, ACK) != 0x5Du ||
        transfer(inst, 0u, ACK) != 0u || transfer(inst, (uint8_t)(sector >> 8), ACK) != 0u ||
        transfer(inst, (uint8_t)sector, ACK) != 0u) {
        broken("card Write preamble or address reply changed");
    }
}

static void read_sector_edge(schway *inst, uint32_t sector, const uint8_t *expected)
{
    if (transfer(inst, 0x81u, ACK) != 0xFFu) {
        broken("card select reply changed");
    }
    (void)transfer(inst, 0x52u, ACK);
    (void)transfer(inst, 0u, ACK);
    (void)transfer(inst, 0u, ACK);
    (void)transfer(inst, 0u, ACK);
    (void)transfer(inst, (uint8_t)(sector >> 8), ACK);
    if (transfer(inst, (uint8_t)sector, ACK) != 0x5Cu || transfer(inst, 0u, ACK) != 0x5Du) {
        broken("card sector address prefix changed");
    }
    if (sector >= 1024u) {
        if (transfer(inst, 0u, READ_PAUSE) != 0xFFu || transfer(inst, 0u, 0u) != 0xFFu) {
            broken("invalid sector did not stop at its bounded response");
        }
        return;
    }
    uint8_t sum = (uint8_t)(sector >> 8) ^ (uint8_t)sector;
    if (transfer(inst, 0u, READ_PAUSE) != (uint8_t)(sector >> 8) ||
        transfer(inst, 0u, ACK) != (uint8_t)sector) {
        broken("last-sector address bytes changed");
    }
    for (uint32_t i = 0u; i < 128u; ++i) {
        uint8_t value = transfer(inst, 0u, ACK);
        if (value != expected[i]) {
            broken("last-sector bytes changed");
        }
        sum ^= value;
    }
    if (transfer(inst, 0u, ACK) != sum || transfer(inst, 0u, 0u) != 0x47u) {
        broken("last-sector checksum or terminator changed");
    }
}

static void exercise(schway *inst, uint8_t variant)
{
    uint8_t data[128];
    for (uint32_t i = 0u; i < sizeof data; ++i) {
        data[i] = (uint8_t)(i * 29u + variant);
    }
    schway_span card;
    if (schway_get_memory(inst, SCHWAY_MEMORY_CARD1, &card) != SCHWAY_STATUS_OK ||
        card.size != SCHWAY_MEMCARD_SIZE) {
        broken("public card span is invalid");
    }
    const uint8_t *original = (const uint8_t *)card.data;
    uint8_t before[128];
    memcpy(before, original + 7u * 128u, sizeof before);
    configure(inst);
    write_prefix(inst, 7u, 0x08u);
    if (generation(inst) != 0u || memcmp(before, original + 7u * 128u, sizeof before) != 0) {
        broken("staged write changed persistent bytes or generation early");
    }
    for (uint32_t i = 0u; i < sizeof data; ++i) {
        if (transfer(inst, data[i], ACK) != 0u) {
            broken("write payload reply changed");
        }
    }
    uint8_t sum = checksum(7u, data);
    if (transfer(inst, sum, ACK) != 0u || transfer(inst, 0u, ACK) != 0x5Cu ||
        transfer(inst, 0u, ACK) != 0x5Du || transfer(inst, 0u, 0u) != 0x47u) {
        broken("successful Write did not complete with 47h");
    }
    if (generation(inst) != 1u || memcmp(original + 7u * 128u, data, sizeof data) != 0) {
        broken("successful Write byte/generation outcome changed");
    }
    read_sector_edge(inst, 1023u, original + 1023u * 128u);
    read_sector_edge(inst, 1024u, NULL);

    /* A second transfer with a bad checksum must be rejected atomically. */
    uint8_t rejected[128];
    for (uint32_t i = 0u; i < sizeof rejected; ++i) {
        rejected[i] = (uint8_t)(data[i] ^ 0x5Au);
    }
    write_prefix(inst, 7u, 0u);
    for (uint32_t i = 0u; i < sizeof rejected; ++i) {
        (void)transfer(inst, rejected[i], ACK);
    }
    (void)transfer(inst, (uint8_t)(checksum(7u, rejected) ^ 1u), ACK);
    (void)transfer(inst, 0u, ACK);
    (void)transfer(inst, 0u, ACK);
    if (transfer(inst, 0u, 0u) != 0x4Eu || generation(inst) != 1u ||
        memcmp(original + 7u * 128u, data, sizeof data) != 0) {
        broken("bad checksum mutated a sector or generation");
    }
    uint8_t aborted_before[128];
    memcpy(aborted_before, original + 8u * 128u, sizeof aborted_before);
    write_prefix(inst, 8u, 0x04u);
    (void)transfer(inst, data[0], ACK);
    refuse_invalid_pending_state(inst);
    if (generation(inst) != 1u ||
        memcmp(original + 8u * 128u, aborted_before, sizeof aborted_before) != 0) {
        broken("invalid saved state changed persistent card bytes");
    }
    write_mmio(inst, JOY_CTRL, 2u, 0x1001u);
    if (generation(inst) != 1u ||
        memcmp(original + 8u * 128u, aborted_before, sizeof aborted_before) != 0) {
        broken("port deselection committed an incomplete staged write");
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    schway *inst = make();
    exercise(inst, size == 0u ? 0u : data[0]);
    uint64_t before_run = inst->ticks;
    schway_budget budget;
    schway_run_result result;
    memset(&budget, 0, sizeof budget);
    memset(&result, 0, sizeof result);
    budget.size = (uint32_t)sizeof budget;
    budget.max_cycles = 32u;
    result.size = (uint32_t)sizeof result;
    if (schway_run(inst, &budget, &result) != SCHWAY_STATUS_OK || result.ticks < before_run ||
        result.ticks - before_run > 32u) {
        broken("public run exceeded its card replay bound");
    }
    schway_destroy(inst);
    return 0;
}

#define SCHWAY_REPLAY_NAME "fuzz.regress.memcard"
#include "replay_main.h"
