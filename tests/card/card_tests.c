/* Memory card protocol tests through JOY MMIO and the instance scheduler. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "state.h"
#include "../check.h"

#define JOY_DATA 0x1F801040u
#define JOY_MODE 0x1F801048u
#define JOY_CTRL 0x1F80104Au
#define JOY_BAUD 0x1F80104Eu
#define I_STAT 0x1F801070u
#define CARD_ACK 1500u
#define CARD_READ_PAUSE 31000u

static void *test_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void test_free(void *user, void *ptr, size_t size)
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
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) {
        exit(1);
    }
    return inst;
}

static void write_mmio(schway *inst, uint32_t addr, uint32_t width, uint32_t value)
{
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, addr, width, value, &cycles), 0u);
}

static uint32_t read_mmio(schway *inst, uint32_t addr, uint32_t width)
{
    uint32_t cycles = 0u;
    uint32_t value = 0u;
    CHECK_EQ_U64(schway__bus_read(inst, addr, width, &value, &cycles), 0u);
    return value;
}

static void advance_to(schway *inst, uint64_t tick)
{
    inst->ticks = tick;
    schway__sched_sync(inst);
    schway__sched_run(inst);
}

static uint8_t *make_card(uint8_t seed)
{
    uint8_t *bytes = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    CHECK(bytes != NULL);
    if (bytes == NULL) {
        exit(1);
    }
    for (uint32_t i = 0u; i < SCHWAY_MEMCARD_SIZE; i++) {
        bytes[i] = (uint8_t)(i * 13u + seed);
    }
    return bytes;
}

static void configure(schway *inst, uint32_t port)
{
    uint32_t port_bit = port == 0u ? 0u : 0x2000u;
    write_mmio(inst, JOY_MODE, 2u, 0x000Cu);
    write_mmio(inst, JOY_BAUD, 2u, 0x0088u);
    write_mmio(inst, JOY_CTRL, 2u, 0x1003u | port_bit);
}

static uint8_t transfer(schway *inst, uint8_t sent, uint32_t ack_ticks)
{
    uint64_t start = inst->ticks;
    uint32_t port_bit = (inst->sio.ctrl & 0x2000u) != 0u ? 0x2000u : 0u;
    write_mmio(inst, JOY_DATA, 1u, sent);
    advance_to(inst, start + 1088u);
    uint8_t received = (uint8_t)read_mmio(inst, JOY_DATA, 1u);
    if (ack_ticks == 0u) {
        CHECK(inst->sio.ack_pending == 0u);
        CHECK(inst->sio.ack_low == 0u);
        return received;
    }
    CHECK(inst->sio.ack_pending != 0u);
    CHECK_EQ_U64(inst->sio.ack_deadline, start + ack_ticks);
    advance_to(inst, start + ack_ticks);
    CHECK(inst->sio.ack_low != 0u);
    write_mmio(inst, I_STAT, 4u, 0u);
    write_mmio(inst, JOY_CTRL, 2u, 0x1013u | port_bit);
    advance_to(inst, start + ack_ticks + 68u);
    CHECK(inst->sio.ack_low == 0u);
    write_mmio(inst, JOY_CTRL, 2u, 0x1003u | port_bit);
    return received;
}

static void read_prefix_to_high(schway *inst, uint32_t sector)
{
    CHECK_EQ_HEX(transfer(inst, 0x81u, CARD_ACK), 0xFFu);
    CHECK_EQ_HEX(transfer(inst, 0x52u, CARD_ACK), 0x08u);
    CHECK_EQ_HEX(transfer(inst, 0u, CARD_ACK), 0x5Au);
    CHECK_EQ_HEX(transfer(inst, 0u, CARD_ACK), 0x5Du);
    CHECK_EQ_HEX(transfer(inst, 0u, CARD_ACK), 0u);
    CHECK_EQ_HEX(transfer(inst, (uint8_t)(sector >> 8), CARD_ACK), 0u);
}

static void read_sector_tail(schway *inst, uint32_t sector, const uint8_t *expected)
{
    CHECK_EQ_HEX(transfer(inst, (uint8_t)sector, CARD_ACK), 0x5Cu);
    CHECK_EQ_HEX(transfer(inst, 0u, CARD_ACK), 0x5Du);
    CHECK_EQ_HEX(transfer(inst, 0u, CARD_READ_PAUSE), (uint8_t)(sector >> 8));
    CHECK_EQ_HEX(transfer(inst, 0u, CARD_ACK), (uint8_t)sector);
    uint8_t checksum = (uint8_t)(sector >> 8) ^ (uint8_t)sector;
    for (uint32_t i = 0u; i < 128u; i++) {
        uint8_t received = transfer(inst, 0u, CARD_ACK);
        if (expected != NULL) {
            CHECK_EQ_HEX(received, expected[i]);
        }
        checksum ^= received;
    }
    CHECK_EQ_HEX(transfer(inst, 0u, CARD_ACK), checksum);
    CHECK_EQ_HEX(transfer(inst, 0u, 0u), 0x47u);
}

static void read_sector(schway *inst, uint32_t sector, const uint8_t *expected)
{
    read_prefix_to_high(inst, sector);
    read_sector_tail(inst, sector, expected);
}

static int write_preamble(schway *inst, uint8_t expected_flag)
{
    CHECK_EQ_HEX(transfer(inst, 0x81u, CARD_ACK), 0xFFu);
    uint8_t flag = transfer(inst, 0x57u, CARD_ACK);
    if (flag != expected_flag) {
        CHECK_EQ_HEX(flag, expected_flag);
        return 0;
    }
    CHECK_EQ_HEX(transfer(inst, 0u, CARD_ACK), 0x5Au);
    CHECK_EQ_HEX(transfer(inst, 0u, CARD_ACK), 0x5Du);
    CHECK_EQ_HEX(transfer(inst, 0u, CARD_ACK), 0u);
    return 1;
}

static uint8_t write_address(schway *inst, uint32_t sector, uint32_t low_ack)
{
    CHECK_EQ_HEX(transfer(inst, (uint8_t)(sector >> 8), CARD_ACK), 0u);
    return transfer(inst, (uint8_t)sector, low_ack);
}

static void write_data(schway *inst, const uint8_t *data, uint32_t begin, uint32_t end)
{
    for (uint32_t i = begin; i < end; i++) {
        CHECK_EQ_HEX(transfer(inst, data[i], CARD_ACK), 0u);
    }
}

static uint8_t write_checksum(uint32_t sector, const uint8_t *data)
{
    uint8_t checksum = (uint8_t)(sector >> 8) ^ (uint8_t)sector;
    for (uint32_t i = 0u; i < SCHWAY__CARD_SECTOR_SIZE; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

static void write_finish(schway *inst, uint8_t checksum, uint8_t result)
{
    CHECK_EQ_HEX(transfer(inst, checksum, CARD_ACK), 0u);
    CHECK_EQ_HEX(transfer(inst, 0u, CARD_ACK), 0x5Cu);
    CHECK_EQ_HEX(transfer(inst, 0u, CARD_ACK), 0x5Du);
    CHECK_EQ_HEX(transfer(inst, 0u, 0u), result);
}

static int write_success(schway *inst, uint32_t sector, uint8_t expected_flag,
                         const uint8_t *data)
{
    if (!write_preamble(inst, expected_flag)) {
        return 0;
    }
    uint8_t address = write_address(inst, sector, CARD_ACK);
    if (address != 0u) {
        CHECK_EQ_HEX(address, 0u);
        return 0;
    }
    write_data(inst, data, 0u, SCHWAY__CARD_SECTOR_SIZE);
    write_finish(inst, write_checksum(sector, data), 0x47u);
    return 1;
}

static void make_write_data(uint8_t *data, uint32_t seed)
{
    for (uint32_t i = 0u; i < SCHWAY__CARD_SECTOR_SIZE; i++) {
        data[i] = (uint8_t)(i * 29u + seed);
    }
}

static uint64_t card_generation(const schway *inst, uint32_t slot)
{
    uint64_t generation = UINT64_MAX;
    CHECK_EQ_U64(schway_memcard_generation(inst, slot, &generation), SCHWAY_STATUS_OK);
    return generation;
}

static int replace_state_field(schway *inst, uint8_t *state, size_t size,
                               const char *name, uint32_t element, uint64_t value)
{
    size_t count = schway__state_record(inst, NULL, 0u);
    struct schway__state_field *fields =
        (struct schway__state_field *)calloc(count, sizeof *fields);
    CHECK(fields != NULL);
    if (fields == NULL) {
        return 0;
    }
    CHECK_EQ_U64(schway__state_record(inst, fields, count), count);
    int found = 0;
    for (size_t i = 0u; i < count; i++) {
        if (strcmp(fields[i].name, name) == 0) {
            CHECK(element < fields[i].count);
            CHECK(fields[i].width <= sizeof value);
            if (element < fields[i].count && fields[i].width <= sizeof value) {
                size_t offset = fields[i].offset + (size_t)element * fields[i].width;
                for (uint32_t byte = 0u; byte < fields[i].width; byte++) {
                    state[offset + byte] = (uint8_t)(value >> (8u * byte));
                }
                found = 1;
            }
            break;
        }
    }
    CHECK(found);
    if (found != 0) {
        schway__state_seal(state, size);
    }
    free(fields);
    return found;
}

static void reject_mutated_state(schway *source, schway *target, const char *name,
                                 uint32_t element, uint64_t value)
{
    size_t size = schway_state_size(source);
    uint8_t *state = (uint8_t *)malloc(size);
    uint8_t *snapshot = (uint8_t *)malloc(sizeof *target);
    CHECK(state != NULL);
    CHECK(snapshot != NULL);
    if (state == NULL || snapshot == NULL) {
        free(snapshot);
        free(state);
        return;
    }
    CHECK_EQ_U64(schway_save_state(source, state, size), SCHWAY_STATUS_OK);
    if (replace_state_field(source, state, size, name, element, value) != 0) {
        memcpy(snapshot, target, sizeof *target);
        CHECK_EQ_U64(schway_load_state(target, state, size), SCHWAY_STATUS_ERR_STATE_FIELD);
        CHECK(memcmp(target, snapshot, sizeof *target) == 0);
    }
    free(snapshot);
    free(state);
}

static uint8_t read_card_flag(schway *inst)
{
    CHECK_EQ_HEX(transfer(inst, 0x81u, CARD_ACK), 0xFFu);
    return transfer(inst, 0x53u, CARD_ACK);
}

static void read_invalid_sector(schway *inst, uint32_t sector)
{
    read_prefix_to_high(inst, sector);
    CHECK_EQ_HEX(transfer(inst, (uint8_t)sector, CARD_ACK), 0x5Cu);
    CHECK_EQ_HEX(transfer(inst, 0u, CARD_ACK), 0x5Du);
    CHECK_EQ_HEX(transfer(inst, 0u, CARD_READ_PAUSE), 0xFFu);
    CHECK_EQ_HEX(transfer(inst, 0u, 0u), 0xFFu);
}

static void get_id(schway *inst)
{
    static const uint8_t expected[8] = {0x5Au, 0x5Du, 0x5Cu, 0x5Du, 0x04u, 0u, 0u, 0x80u};
    CHECK_EQ_HEX(transfer(inst, 0x81u, CARD_ACK), 0xFFu);
    CHECK_EQ_HEX(transfer(inst, 0x53u, CARD_ACK), 0x08u);
    for (uint32_t i = 0u; i < sizeof expected; i++) {
        uint32_t ack = i + 1u == sizeof expected ? 0u : CARD_ACK;
        CHECK_EQ_HEX(transfer(inst, 0u, ack), expected[i]);
    }
}

static void protocol_read(void)
{
    const uint32_t sector = 3u;
    uint8_t *card1 = make_card(17u);
    uint8_t *card2 = make_card(93u);
    schway *inst = make();
    schway_span first;
    schway_span second;
    CHECK_EQ_U64(schway_get_memory(inst, SCHWAY_MEMORY_CARD1, &first), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_get_memory(inst, SCHWAY_MEMORY_CARD2, &second), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(first.size, SCHWAY_MEMCARD_SIZE);
    CHECK_EQ_U64(second.size, SCHWAY_MEMCARD_SIZE);
    const uint8_t *blank = (const uint8_t *)first.data;
    CHECK_EQ_HEX(blank[0], 'M');
    CHECK_EQ_HEX(blank[1], 'C');
    CHECK_EQ_HEX(blank[127], 0x0Eu);
    CHECK(memcmp(blank, &blank[63u * 128u], 128u) == 0);
    CHECK_EQ_HEX(blank[128u], 0xA0u);
    CHECK_EQ_HEX(blank[128u + 8u], 0xFFu);
    CHECK_EQ_HEX(blank[128u + 9u], 0xFFu);
    CHECK_EQ_HEX(blank[128u + 127u], 0xA0u);
    CHECK_EQ_U64(schway_load_memcard(inst, SCHWAY_CARD_SLOT_1, card1, SCHWAY_MEMCARD_SIZE),
                 SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_load_memcard(inst, SCHWAY_CARD_SLOT_2, card2, SCHWAY_MEMCARD_SIZE),
                 SCHWAY_STATUS_OK);
    CHECK(memcmp(first.data, card1, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK(memcmp(second.data, card2, SCHWAY_MEMCARD_SIZE) == 0);
    uint64_t generation = UINT64_MAX;
    CHECK_EQ_U64(schway_memcard_generation(inst, SCHWAY_CARD_SLOT_1, &generation),
                 SCHWAY_STATUS_OK);
    CHECK_EQ_U64(generation, 0u);

    configure(inst, 0u);
    read_sector(inst, 0u, card1);
    read_sector(inst, SCHWAY__CARD_SECTOR_COUNT - 1u,
                &card1[(SCHWAY__CARD_SECTOR_COUNT - 1u) * SCHWAY__CARD_SECTOR_SIZE]);
    configure(inst, 1u);
    read_sector(inst, sector, &card2[sector * 128u]);
    get_id(inst);
    CHECK_EQ_U64(schway_memcard_generation(inst, SCHWAY_CARD_SLOT_1, &generation),
                 SCHWAY_STATUS_OK);
    CHECK_EQ_U64(generation, 0u);
    schway_span after;
    CHECK_EQ_U64(schway_get_memory(inst, SCHWAY_MEMORY_CARD1, &after), SCHWAY_STATUS_OK);
    CHECK(after.data == first.data);
    CHECK_EQ_U64(after.size, first.size);
    CHECK(memcmp(after.data, card1, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_get_memory(inst, SCHWAY_MEMORY_CARD1, &after), SCHWAY_STATUS_OK);
    CHECK(after.data == first.data);
    CHECK(memcmp(after.data, card1, SCHWAY_MEMCARD_SIZE) == 0);
    schway_destroy(inst);
    free(card2);
    free(card1);
}

static void protocol_read_edges(void)
{
    const uint32_t sector = 7u;
    uint8_t *card_a = make_card(29u);
    uint8_t *card_b = make_card(111u);
    schway *a = make();
    schway *b = make();
    CHECK_EQ_U64(schway_load_memcard(a, SCHWAY_CARD_SLOT_1, card_a, SCHWAY_MEMCARD_SIZE),
                 SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_load_memcard(b, SCHWAY_CARD_SLOT_1, card_b, SCHWAY_MEMCARD_SIZE),
                 SCHWAY_STATUS_OK);
    schway_span a_card;
    schway_span b_card;
    CHECK_EQ_U64(schway_get_memory(a, SCHWAY_MEMORY_CARD1, &a_card), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_get_memory(b, SCHWAY_MEMORY_CARD1, &b_card), SCHWAY_STATUS_OK);
    CHECK(a_card.data != b_card.data);
    CHECK(memcmp(a_card.data, card_a, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK(memcmp(b_card.data, card_b, SCHWAY_MEMCARD_SIZE) == 0);

    configure(a, 0u);
    read_prefix_to_high(a, sector);
    size_t state_size = schway_state_size(a);
    uint8_t *saved = (uint8_t *)malloc(state_size);
    CHECK(saved != NULL);
    if (saved != NULL) {
        CHECK_EQ_U64(schway_save_state(a, saved, state_size), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(schway_load_state(b, saved, state_size), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(schway_get_memory(b, SCHWAY_MEMORY_CARD1, &b_card), SCHWAY_STATUS_OK);
        CHECK(b_card.data != a_card.data);
        CHECK(memcmp(b_card.data, card_b, SCHWAY_MEMCARD_SIZE) == 0);
        read_sector_tail(b, sector, &card_b[sector * SCHWAY__CARD_SECTOR_SIZE]);
        CHECK_EQ_U64(schway_get_memory(b, SCHWAY_MEMORY_CARD1, &b_card), SCHWAY_STATUS_OK);
        CHECK(b_card.data != a_card.data);
        read_sector(b, sector, &card_b[sector * SCHWAY__CARD_SECTOR_SIZE]);
        read_invalid_sector(b, SCHWAY__CARD_SECTOR_COUNT);
        read_sector(b, sector, &card_b[sector * SCHWAY__CARD_SECTOR_SIZE]);

        CHECK_EQ_HEX(transfer(b, 0x81u, CARD_ACK), 0xFFu);
        CHECK_EQ_HEX(transfer(b, 0x54u, 0u), 0xFFu);
        CHECK_EQ_HEX(transfer(b, 0x81u, CARD_ACK), 0xFFu);
        CHECK_EQ_HEX(transfer(b, 0x52u, CARD_ACK), 0x08u);
        CHECK_EQ_HEX(transfer(b, 0u, CARD_ACK), 0x5Au);
        free(saved);
    }
    CHECK(memcmp(a_card.data, card_a, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK(memcmp(b_card.data, card_b, SCHWAY_MEMCARD_SIZE) == 0);
    uint64_t generation = UINT64_MAX;
    CHECK_EQ_U64(schway_memcard_generation(b, SCHWAY_CARD_SLOT_1, &generation),
                 SCHWAY_STATUS_OK);
    CHECK_EQ_U64(generation, 0u);
    CHECK_EQ_U64(schway_reset(b, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_get_memory(b, SCHWAY_MEMORY_CARD1, &b_card), SCHWAY_STATUS_OK);
    CHECK(b_card.data != a_card.data);
    CHECK(memcmp(b_card.data, card_b, SCHWAY_MEMCARD_SIZE) == 0);
    schway_destroy(b);
    schway_destroy(a);
    free(card_b);
    free(card_a);
}

static void protocol_write_commit(void)
{
    const uint32_t sector = 17u;
    const uint32_t split = 64u;
    uint8_t *original = make_card(41u);
    uint8_t *expected = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    uint8_t data[SCHWAY__CARD_SECTOR_SIZE];
    uint8_t rejected[SCHWAY__CARD_SECTOR_SIZE];
    CHECK(expected != NULL);
    if (expected == NULL) {
        free(original);
        return;
    }
    memcpy(expected, original, SCHWAY_MEMCARD_SIZE);
    make_write_data(data, 73u);
    make_write_data(rejected, 121u);

    schway *writer = make();
    schway *restored = make();
    CHECK_EQ_U64(schway_load_memcard(writer, SCHWAY_CARD_SLOT_1, original,
                                     SCHWAY_MEMCARD_SIZE), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_load_memcard(restored, SCHWAY_CARD_SLOT_1, original,
                                     SCHWAY_MEMCARD_SIZE), SCHWAY_STATUS_OK);
    schway_span writer_card;
    schway_span restored_card;
    CHECK_EQ_U64(schway_get_memory(writer, SCHWAY_MEMORY_CARD1, &writer_card),
                 SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_get_memory(restored, SCHWAY_MEMORY_CARD1, &restored_card),
                 SCHWAY_STATUS_OK);
    configure(writer, 0u);
    if (!write_preamble(writer, 0x08u)) {
        schway_destroy(restored);
        schway_destroy(writer);
        free(expected);
        free(original);
        return;
    }
    CHECK_EQ_HEX(write_address(writer, sector, CARD_ACK), 0u);
    write_data(writer, data, 0u, split);

    size_t state_size = schway_state_size(writer);
    uint8_t *saved = (uint8_t *)malloc(state_size);
    CHECK(saved != NULL);
    if (saved != NULL) {
        CHECK_EQ_U64(schway_save_state(writer, saved, state_size), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(schway_load_state(restored, saved, state_size), SCHWAY_STATUS_OK);
        write_data(restored, data, split, SCHWAY__CARD_SECTOR_SIZE);
        memcpy(&expected[sector * SCHWAY__CARD_SECTOR_SIZE], data, sizeof data);
        uint8_t checksum = write_checksum(sector, data);
        CHECK_EQ_HEX(transfer(restored, checksum, CARD_ACK), 0u);
        CHECK_EQ_HEX(transfer(restored, 0u, CARD_ACK), 0x5Cu);
        CHECK_EQ_HEX(transfer(restored, 0u, CARD_ACK), 0x5Du);
        CHECK(memcmp(restored_card.data, original, SCHWAY_MEMCARD_SIZE) == 0);
        CHECK_EQ_U64(card_generation(restored, SCHWAY_CARD_SLOT_1), 0u);
        CHECK(memcmp(writer_card.data, original, SCHWAY_MEMCARD_SIZE) == 0);
        CHECK_EQ_U64(card_generation(writer, SCHWAY_CARD_SLOT_1), 0u);
        CHECK_EQ_HEX(transfer(restored, 0u, 0u), 0x47u);
        CHECK(memcmp(restored_card.data, expected, SCHWAY_MEMCARD_SIZE) == 0);
        CHECK_EQ_U64(card_generation(restored, SCHWAY_CARD_SLOT_1), 1u);
        CHECK(memcmp(writer_card.data, original, SCHWAY_MEMCARD_SIZE) == 0);
        CHECK_EQ_U64(card_generation(writer, SCHWAY_CARD_SLOT_1), 0u);
        free(saved);
    }

    configure(restored, 0u);
    CHECK(write_preamble(restored, 0u));
    CHECK_EQ_HEX(write_address(restored, sector, CARD_ACK), 0u);
    write_data(restored, rejected, 0u, SCHWAY__CARD_SECTOR_SIZE);
    write_finish(restored, (uint8_t)(write_checksum(sector, rejected) ^ 1u), 0x4Eu);
    CHECK_EQ_HEX(restored->sio.ctrl, 0x1003u);
    CHECK_EQ_U64(restored->card.slot[0].phase, 0u);
    CHECK_EQ_U64(restored->card.slot[0].write_step, 0u);
    CHECK(memcmp(restored_card.data, expected, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK_EQ_U64(card_generation(restored, SCHWAY_CARD_SLOT_1), 1u);
    CHECK_EQ_HEX(read_card_flag(restored), 0x04u);
    write_mmio(restored, JOY_CTRL, 2u, 0x1043u);

    configure(restored, 0u);
    CHECK(write_preamble(restored, 0u));
    CHECK_EQ_HEX(write_address(restored, SCHWAY__CARD_SECTOR_COUNT, 0u), 0xFFu);
    CHECK(memcmp(restored_card.data, expected, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK_EQ_U64(card_generation(restored, SCHWAY_CARD_SLOT_1), 1u);
    CHECK_EQ_HEX(read_card_flag(restored), 0x04u);
    write_mmio(restored, JOY_CTRL, 2u, 0x1043u);

    configure(restored, 0u);
    CHECK(write_preamble(restored, 0u));
    CHECK_EQ_HEX(write_address(restored, sector + 1u, CARD_ACK), 0u);
    write_data(restored, rejected, 0u, 23u);
    CHECK(memcmp(restored_card.data, expected, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK_EQ_U64(card_generation(restored, SCHWAY_CARD_SLOT_1), 1u);
    CHECK_EQ_U64(schway_reset(restored, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    CHECK(memcmp(restored_card.data, expected, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK_EQ_U64(card_generation(restored, SCHWAY_CARD_SLOT_1), 1u);

    configure(restored, 0u);
    CHECK(write_preamble(restored, 0x08u));
    CHECK_EQ_HEX(write_address(restored, sector + 2u, CARD_ACK), 0u);
    write_data(restored, rejected, 0u, 19u);
    write_mmio(restored, JOY_CTRL, 2u, 0x1001u);
    CHECK(memcmp(restored_card.data, expected, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK_EQ_U64(card_generation(restored, SCHWAY_CARD_SLOT_1), 1u);

    schway_destroy(restored);
    schway_destroy(writer);
    free(expected);
    free(original);
}

static void protocol_write_boundaries(void)
{
    uint8_t *original = make_card(23u);
    uint8_t *expected = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    uint8_t first[SCHWAY__CARD_SECTOR_SIZE];
    uint8_t last[SCHWAY__CARD_SECTOR_SIZE];
    CHECK(expected != NULL);
    if (expected == NULL) {
        free(original);
        return;
    }
    memcpy(expected, original, SCHWAY_MEMCARD_SIZE);
    make_write_data(first, 17u);
    make_write_data(last, 91u);
    schway *inst = make();
    CHECK_EQ_U64(schway_load_memcard(inst, SCHWAY_CARD_SLOT_1, original,
                                     SCHWAY_MEMCARD_SIZE), SCHWAY_STATUS_OK);
    schway_span card;
    CHECK_EQ_U64(schway_get_memory(inst, SCHWAY_MEMORY_CARD1, &card), SCHWAY_STATUS_OK);
    configure(inst, 0u);
    CHECK(write_success(inst, 0u, 0x08u, first));
    memcpy(expected, first, sizeof first);
    CHECK(memcmp(card.data, expected, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK_EQ_U64(card_generation(inst, SCHWAY_CARD_SLOT_1), 1u);
    CHECK(write_success(inst, SCHWAY__CARD_SECTOR_COUNT - 1u, 0u, last));
    memcpy(&expected[(SCHWAY__CARD_SECTOR_COUNT - 1u) * SCHWAY__CARD_SECTOR_SIZE],
           last, sizeof last);
    CHECK(memcmp(card.data, expected, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK_EQ_U64(card_generation(inst, SCHWAY_CARD_SLOT_1), 2u);
    schway_destroy(inst);
    free(expected);
    free(original);
}

static void protocol_write_slots(void)
{
    uint8_t *original1 = make_card(31u);
    uint8_t *original2 = make_card(87u);
    uint8_t *expected1 = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    uint8_t *expected2 = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    uint8_t data1[SCHWAY__CARD_SECTOR_SIZE];
    uint8_t data2[SCHWAY__CARD_SECTOR_SIZE];
    CHECK(expected1 != NULL);
    CHECK(expected2 != NULL);
    if (expected1 == NULL || expected2 == NULL) {
        free(expected2);
        free(expected1);
        free(original2);
        free(original1);
        return;
    }
    memcpy(expected1, original1, SCHWAY_MEMCARD_SIZE);
    memcpy(expected2, original2, SCHWAY_MEMCARD_SIZE);
    make_write_data(data1, 47u);
    make_write_data(data2, 109u);
    schway *inst = make();
    CHECK_EQ_U64(schway_load_memcard(inst, SCHWAY_CARD_SLOT_1, original1,
                                     SCHWAY_MEMCARD_SIZE), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_load_memcard(inst, SCHWAY_CARD_SLOT_2, original2,
                                     SCHWAY_MEMCARD_SIZE), SCHWAY_STATUS_OK);
    schway_span card1;
    schway_span card2;
    CHECK_EQ_U64(schway_get_memory(inst, SCHWAY_MEMORY_CARD1, &card1), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_get_memory(inst, SCHWAY_MEMORY_CARD2, &card2), SCHWAY_STATUS_OK);
    configure(inst, 0u);
    CHECK(write_success(inst, 3u, 0x08u, data1));
    memcpy(&expected1[3u * SCHWAY__CARD_SECTOR_SIZE], data1, sizeof data1);
    CHECK(memcmp(card1.data, expected1, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK(memcmp(card2.data, original2, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK_EQ_U64(card_generation(inst, SCHWAY_CARD_SLOT_1), 1u);
    CHECK_EQ_U64(card_generation(inst, SCHWAY_CARD_SLOT_2), 0u);

    configure(inst, 1u);
    CHECK(write_success(inst, 12u, 0x08u, data2));
    memcpy(&expected2[12u * SCHWAY__CARD_SECTOR_SIZE], data2, sizeof data2);
    CHECK(memcmp(card1.data, expected1, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK(memcmp(card2.data, expected2, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK_EQ_U64(card_generation(inst, SCHWAY_CARD_SLOT_1), 1u);
    CHECK_EQ_U64(card_generation(inst, SCHWAY_CARD_SLOT_2), 1u);
    schway_destroy(inst);
    free(expected2);
    free(expected1);
    free(original2);
    free(original1);
}

static void protocol_write_repeated_and_truncated(void)
{
    const uint32_t sector = 5u;
    uint8_t *original = make_card(61u);
    uint8_t *expected = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    uint8_t first[SCHWAY__CARD_SECTOR_SIZE];
    uint8_t second[SCHWAY__CARD_SECTOR_SIZE];
    uint8_t rejected[SCHWAY__CARD_SECTOR_SIZE];
    CHECK(expected != NULL);
    if (expected == NULL) {
        free(original);
        return;
    }
    memcpy(expected, original, SCHWAY_MEMCARD_SIZE);
    make_write_data(first, 33u);
    make_write_data(second, 77u);
    make_write_data(rejected, 143u);
    schway *inst = make();
    CHECK_EQ_U64(schway_load_memcard(inst, SCHWAY_CARD_SLOT_1, original,
                                     SCHWAY_MEMCARD_SIZE), SCHWAY_STATUS_OK);
    schway_span card;
    CHECK_EQ_U64(schway_get_memory(inst, SCHWAY_MEMORY_CARD1, &card), SCHWAY_STATUS_OK);
    configure(inst, 0u);
    CHECK(write_success(inst, sector, 0x08u, first));
    memcpy(&expected[sector * SCHWAY__CARD_SECTOR_SIZE], first, sizeof first);
    CHECK_EQ_U64(card_generation(inst, SCHWAY_CARD_SLOT_1), 1u);
    CHECK(write_success(inst, sector, 0u, second));
    memcpy(&expected[sector * SCHWAY__CARD_SECTOR_SIZE], second, sizeof second);
    CHECK(memcmp(card.data, expected, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK_EQ_U64(card_generation(inst, SCHWAY_CARD_SLOT_1), 2u);

    CHECK(write_preamble(inst, 0u));
    CHECK_EQ_HEX(write_address(inst, sector, CARD_ACK), 0u);
    write_data(inst, rejected, 0u, SCHWAY__CARD_SECTOR_SIZE);
    write_finish(inst, (uint8_t)(write_checksum(sector, rejected) ^ 1u), 0x4Eu);
    CHECK(memcmp(card.data, expected, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK_EQ_U64(card_generation(inst, SCHWAY_CARD_SLOT_1), 2u);
    CHECK_EQ_HEX(read_card_flag(inst), 0x04u);
    write_mmio(inst, JOY_CTRL, 2u, 0x1043u);
    configure(inst, 0u);

    CHECK(write_preamble(inst, 0u));
    CHECK_EQ_HEX(write_address(inst, sector + 1u, CARD_ACK), 0u);
    write_data(inst, rejected, 0u, 17u);
    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    CHECK(memcmp(card.data, expected, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK_EQ_U64(card_generation(inst, SCHWAY_CARD_SLOT_1), 2u);

    configure(inst, 0u);
    CHECK(write_preamble(inst, 0x08u));
    CHECK_EQ_HEX(write_address(inst, sector + 2u, CARD_ACK), 0u);
    write_data(inst, rejected, 0u, 13u);
    write_mmio(inst, JOY_CTRL, 2u, 0x1001u);
    CHECK(memcmp(card.data, expected, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK_EQ_U64(card_generation(inst, SCHWAY_CARD_SLOT_1), 2u);
    schway_destroy(inst);
    free(expected);
    free(original);
}

static void protocol_write_edges(void)
{
    protocol_write_boundaries();
    protocol_write_slots();
    protocol_write_repeated_and_truncated();
}

static void state_import_atomic(void)
{
    uint8_t short_image[37];
    for (uint32_t i = 0u; i < sizeof short_image; i++) {
        short_image[i] = (uint8_t)(i * 7u + 3u);
    }
    schway *inst = make();
    schway_span card;
    CHECK_EQ_U64(schway_get_memory(inst, SCHWAY_MEMORY_CARD1, &card), SCHWAY_STATUS_OK);
    const void *card_address = card.data;
    CHECK_EQ_U64(schway_load_memcard(inst, SCHWAY_CARD_SLOT_1, short_image,
                                     sizeof short_image), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_get_memory(inst, SCHWAY_MEMORY_CARD1, &card), SCHWAY_STATUS_OK);
    CHECK(card.data == card_address);
    CHECK(memcmp(card.data, short_image, sizeof short_image) == 0);
    const uint8_t *loaded = (const uint8_t *)card.data;
    int padded = 1;
    for (uint32_t i = sizeof short_image; i < SCHWAY_MEMCARD_SIZE; i++) {
        padded &= loaded[i] == 0xFFu;
    }
    CHECK(padded);
    CHECK_EQ_U64(card_generation(inst, SCHWAY_CARD_SLOT_1), 0u);

    uint8_t *oversized = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE + 1u);
    CHECK(oversized != NULL);
    if (oversized != NULL) {
        memset(oversized, 0xA5, SCHWAY_MEMCARD_SIZE + 1u);
        CHECK_EQ_U64(schway_load_memcard(inst, SCHWAY_CARD_SLOT_1, oversized,
                                         SCHWAY_MEMCARD_SIZE + 1u),
                     SCHWAY_STATUS_ERR_ARGUMENT);
        CHECK_EQ_U64(schway_load_memcard(inst, 0u, oversized, SCHWAY_MEMCARD_SIZE),
                     SCHWAY_STATUS_ERR_ARGUMENT);
        int refused_without_mutation = memcmp(card.data, short_image, sizeof short_image) == 0;
        for (uint32_t i = sizeof short_image; i < SCHWAY_MEMCARD_SIZE; i++) {
            refused_without_mutation &= loaded[i] == 0xFFu;
        }
        CHECK(refused_without_mutation);
        CHECK_EQ_U64(card_generation(inst, SCHWAY_CARD_SLOT_1), 0u);
    }

    size_t saved_size = schway_state_size(inst);
    uint8_t *idle_state = (uint8_t *)malloc(saved_size);
    CHECK(idle_state != NULL);
    if (idle_state != NULL) {
        CHECK_EQ_U64(schway_save_state(inst, idle_state, saved_size), SCHWAY_STATUS_OK);
        uint8_t data[SCHWAY__CARD_SECTOR_SIZE];
        make_write_data(data, 109u);
        configure(inst, 0u);
        CHECK(write_success(inst, 9u, 0x08u, data));
        uint8_t expected[SCHWAY_MEMCARD_SIZE];
        memcpy(expected, card.data, sizeof expected);
        CHECK_EQ_U64(card_generation(inst, SCHWAY_CARD_SLOT_1), 1u);
        CHECK_EQ_U64(schway_load_memcard(inst, SCHWAY_CARD_SLOT_1, short_image,
                                         sizeof short_image),
                     SCHWAY_STATUS_ERR_ARGUMENT);
        CHECK(memcmp(card.data, expected, sizeof expected) == 0);
        CHECK_EQ_U64(card_generation(inst, SCHWAY_CARD_SLOT_1), 1u);
        CHECK_EQ_U64(schway_load_state(inst, idle_state, saved_size), SCHWAY_STATUS_OK);
        CHECK(memcmp(card.data, expected, sizeof expected) == 0);
        CHECK_EQ_U64(card_generation(inst, SCHWAY_CARD_SLOT_1), 1u);
        free(idle_state);
    }
    free(oversized);
    schway_destroy(inst);

    uint8_t *card_image = make_card(23u);
    schway *source = make();
    schway *target = make();
    CHECK_EQ_U64(schway_load_memcard(source, SCHWAY_CARD_SLOT_1, card_image,
                                     SCHWAY_MEMCARD_SIZE), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_load_memcard(target, SCHWAY_CARD_SLOT_1, card_image,
                                     SCHWAY_MEMCARD_SIZE), SCHWAY_STATUS_OK);
    uint8_t data[SCHWAY__CARD_SECTOR_SIZE];
    make_write_data(data, 41u);
    configure(source, 0u);
    CHECK(write_preamble(source, 0x08u));
    CHECK_EQ_HEX(write_address(source, 3u, CARD_ACK), 0u);
    write_data(source, data, 0u, 4u);
    uint32_t bad_checksum = (source->card.slot[0].checksum + 1u) & 0xFFu;
    reject_mutated_state(source, target, "card.slot1.checksum", 0u, bad_checksum);
    schway_destroy(target);
    schway_destroy(source);
    free(card_image);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        return 2;
    }
    if (strcmp(argv[1], "protocol_read") == 0) {
        protocol_read();
    } else if (strcmp(argv[1], "protocol_read_edges") == 0) {
        protocol_read_edges();
    } else if (strcmp(argv[1], "protocol_write_commit") == 0) {
        protocol_write_commit();
    } else if (strcmp(argv[1], "protocol_write_edges") == 0) {
        protocol_write_edges();
    } else if (strcmp(argv[1], "state_import_atomic") == 0) {
        state_import_atomic();
    } else {
        return 2;
    }
    CHECK_DONE();
}
