/* Memory card protocol tests through JOY MMIO and the instance scheduler. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
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
        CHECK_EQ_HEX(transfer(b, 0x57u, 0u), 0xFFu);
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
    } else {
        return 2;
    }
    CHECK_DONE();
}
