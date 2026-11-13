/* Bus-level tests for GPU transfer commands and saved cursors. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"
#include "bus.h"
#include "state.h"

#define GP0 0x1F801810u
#define GP1 0x1F801814u

static int failures;

#define CHECK(expr)                                                                               \
    do {                                                                                          \
        if (!(expr)) {                                                                            \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr);                          \
            failures++;                                                                           \
        }                                                                                         \
    } while (0)

static void *plain_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void plain_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static schway *make_instance(void)
{
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = plain_alloc;
    cfg.allocator.free = plain_free;
    schway *inst = NULL;
    if (schway_create(&cfg, &inst) != SCHWAY_STATUS_OK) {
        return NULL;
    }
    return inst;
}

static void write_port(schway *inst, uint32_t addr, uint32_t word)
{
    uint32_t cycles = 0u;
    CHECK(schway__bus_write(inst, addr, 4u, word, &cycles) == 0);
}

static uint32_t read_port(schway *inst, uint32_t addr)
{
    uint32_t cycles = 0u;
    uint32_t value = 0u;
    CHECK(schway__bus_read(inst, addr, 4u, &value, &cycles) == 0);
    return value;
}

static int state_copy(schway *source, schway *target)
{
    size_t size = schway_state_size(source);
    uint8_t *state = (uint8_t *)malloc(size);
    CHECK(state != NULL);
    if (state == NULL) {
        return 0;
    }
    CHECK(schway_save_state(source, state, size) == SCHWAY_STATUS_OK);
    CHECK(schway_load_state(target, state, size) == SCHWAY_STATUS_OK);
    free(state);
    return 1;
}

struct body_offset {
    const char *wanted;
    size_t cursor;
    size_t offset;
    int found;
};

static void locate_field(void *user, const char *name, const void *data, uint32_t count,
                         uint32_t width)
{
    struct body_offset *field = (struct body_offset *)user;
    (void)data;
    if (!field->found && strcmp(name, field->wanted) == 0) {
        field->found = 1;
        field->offset = field->cursor;
    }
    field->cursor += (size_t)count * width;
}

static void put_le32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static int corrupt_and_refuse(schway *target, schway *source, const char *name, uint32_t value)
{
    size_t size = schway_state_size(source);
    uint8_t *candidate = (uint8_t *)malloc(size);
    uint8_t *before = (uint8_t *)malloc(size);
    uint8_t *after = (uint8_t *)malloc(size);
    CHECK(candidate != NULL && before != NULL && after != NULL);
    if (candidate == NULL || before == NULL || after == NULL) {
        free(candidate);
        free(before);
        free(after);
        return 0;
    }
    CHECK(schway_save_state(source, candidate, size) == SCHWAY_STATUS_OK);
    struct body_offset field = {name, 0u, 0u, 0};
    CHECK(schway_state_visit(source, locate_field, &field) == SCHWAY_STATUS_OK);
    CHECK(field.found);
    if (!field.found) {
        free(candidate);
        free(before);
        free(after);
        return 0;
    }
    put_le32(candidate + SCHWAY__STATE_HEADER_SIZE + field.offset, value);
    schway__state_seal(candidate, size);
    CHECK(schway_save_state(target, before, size) == SCHWAY_STATUS_OK);
    CHECK(schway_load_state(target, candidate, size) == SCHWAY_STATUS_ERR_STATE_FIELD);
    CHECK(schway_save_state(target, after, size) == SCHWAY_STATUS_OK);
    CHECK(memcmp(before, after, size) == 0);
    free(candidate);
    free(before);
    free(after);
    return 1;
}

static void start_transfer(schway *inst, uint32_t command, uint32_t x, uint32_t y,
                           uint32_t width, uint32_t height)
{
    write_port(inst, GP0, command);
    write_port(inst, GP0, ((y & 0x1FFu) << 16) | (x & 0x3FFu));
    write_port(inst, GP0, ((height & 0x1FFu) << 16) | (width & 0x3FFu));
}

static void copy_rect(schway *inst, uint32_t source_x, uint32_t source_y, uint32_t dest_x,
                      uint32_t dest_y, uint32_t width, uint32_t height)
{
    write_port(inst, GP0, 0x80000000u);
    write_port(inst, GP0, ((source_y & 0x1FFu) << 16) | (source_x & 0x3FFu));
    write_port(inst, GP0, ((dest_y & 0x1FFu) << 16) | (dest_x & 0x3FFu));
    write_port(inst, GP0, ((height & 0x1FFu) << 16) | (width & 0x3FFu));
}

static void test_roundtrip(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        CHECK(0);
        return;
    }

    start_transfer(inst, 0xA0000000u, 1023u, 511u, 2u, 2u);
    write_port(inst, GP0, 0x22221111u);
    write_port(inst, GP0, 0x44443333u);
    CHECK(inst->vram[511][1023] == 0x1111u);
    CHECK(inst->vram[511][0] == 0x2222u);
    CHECK(inst->vram[0][1023] == 0x3333u);
    CHECK(inst->vram[0][0] == 0x4444u);

    start_transfer(inst, 0xC0000000u, 1023u, 511u, 2u, 2u);
    CHECK(read_port(inst, GP0) == 0x22221111u);
    CHECK(read_port(inst, GP0) == 0x44443333u);
    CHECK(inst->gpu.phase == SCHWAY__GPU_IDLE);

    start_transfer(inst, 0xA0000000u, 1023u, 511u, 3u, 1u);
    write_port(inst, GP0, 0x22221111u);
    write_port(inst, GP0, 0x00003333u);
    CHECK(inst->vram[511][1023] == 0x1111u);
    CHECK(inst->vram[511][0] == 0x2222u);
    CHECK(inst->vram[511][1] == 0x3333u);
    CHECK(inst->vram[511][2] == 0u);

    write_port(inst, GP1, 0x04000003u);
    start_transfer(inst, 0xC0000000u, 1023u, 511u, 3u, 1u);
    uint32_t peeked = 0u;
    CHECK(schway__gpu_peek(inst, GP0, 4u, &peeked) == 0);
    CHECK(peeked == 0x22221111u);
    CHECK(inst->gpu.transfer_index == 0u);
    CHECK((read_port(inst, GP1) & (1u << 25)) != 0u);
    CHECK(read_port(inst, GP0) == 0x22221111u);
    CHECK(inst->gpu.transfer_index == 2u);
    CHECK(read_port(inst, GP0) == 0x00003333u);
    CHECK(inst->gpu.phase == SCHWAY__GPU_IDLE);
    CHECK((read_port(inst, GP1) & (1u << 27)) == 0u);
    CHECK(read_port(inst, GP0) == 0x00003333u);
    schway_destroy(inst);
}

static void test_dimensions(void)
{
    static const struct {
        uint32_t width, height, expected_width, expected_height, owed;
    } cases[] = {
        {0u, 0u, 1024u, 512u, 262144u},
        {0u, 1u, 1024u, 1u, 512u},
        {1u, 0u, 1u, 512u, 256u},
        {1u, 1u, 1u, 1u, 1u}
    };
    schway *inst = make_instance();
    if (inst == NULL) {
        CHECK(0);
        return;
    }
    for (size_t i = 0u; i < sizeof cases / sizeof cases[0]; i++) {
        start_transfer(inst, 0xA0000000u, 0u, 0u, cases[i].width, cases[i].height);
        CHECK(inst->gpu.transfer_width == cases[i].expected_width);
        CHECK(inst->gpu.transfer_height == cases[i].expected_height);
        CHECK(inst->gpu.owed_words == cases[i].owed);
        write_port(inst, GP1, 0x01000000u);
        CHECK(inst->gpu.phase == SCHWAY__GPU_IDLE);
    }

    start_transfer(inst, 0xA0000000u, 5u, 6u, 1u, 1u);
    write_port(inst, GP0, 0xBEEFCAFEu);
    CHECK(inst->vram[6][5] == 0xCAFEu);
    start_transfer(inst, 0xC0000000u, 5u, 6u, 1u, 1u);
    CHECK(read_port(inst, GP0) == 0x0000CAFEu);
    schway_destroy(inst);
}

static void test_partial_state(void)
{
    schway *source = make_instance();
    schway *target = make_instance();
    schway *third = make_instance();
    if (source == NULL || target == NULL || third == NULL) {
        CHECK(0);
        schway_destroy(source);
        schway_destroy(target);
        schway_destroy(third);
        return;
    }

    start_transfer(source, 0xA0000000u, 1023u, 511u, 3u, 1u);
    write_port(source, GP0, 0x22221111u);
    CHECK(state_copy(source, target));
    write_port(target, GP0, 0xDEAD3333u);
    CHECK(target->vram[511][1023] == 0x1111u);
    CHECK(target->vram[511][0] == 0x2222u);
    CHECK(target->vram[511][1] == 0x3333u);

    start_transfer(target, 0xC0000000u, 1023u, 511u, 3u, 1u);
    CHECK(read_port(target, GP0) == 0x22221111u);
    CHECK(state_copy(target, third));
    CHECK(read_port(third, GP0) == 0x00003333u);
    CHECK(third->gpu.phase == SCHWAY__GPU_IDLE);

    schway_destroy(third);
    schway_destroy(target);
    schway_destroy(source);
}

static void test_reset_mid_packet(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        CHECK(0);
        return;
    }
    write_port(inst, GP0, 0xA0000000u);
    write_port(inst, GP0, (2u << 16) | 40u);
    CHECK(inst->gpu.phase == SCHWAY__GPU_PACKET);
    write_port(inst, GP1, 0x01000000u);
    write_port(inst, GP0, (1u << 16) | 3u);
    CHECK(inst->gpu.phase == SCHWAY__GPU_IDLE);

    start_transfer(inst, 0xA0000000u, 40u, 2u, 3u, 1u);
    write_port(inst, GP0, 0x22221111u);
    write_port(inst, GP1, 0x01000000u);
    write_port(inst, GP0, 0x00003333u);
    CHECK(inst->gpu.phase == SCHWAY__GPU_IDLE);
    CHECK(inst->vram[2][40] == 0x1111u);
    CHECK(inst->vram[2][41] == 0x2222u);
    CHECK(inst->vram[2][42] == 0u);
    schway_destroy(inst);
}

static void test_invalid_cursor(void)
{
    schway *source = make_instance();
    schway *target = make_instance();
    if (source == NULL || target == NULL) {
        CHECK(0);
        schway_destroy(source);
        schway_destroy(target);
        return;
    }
    start_transfer(source, 0xA0000000u, 8u, 9u, 3u, 1u);
    write_port(source, GP0, 0x22221111u);
    write_port(target, GP0, 0x020000F8u);
    write_port(target, GP0, 0u);
    write_port(target, GP0, (1u << 16) | 16u);
    CHECK(corrupt_and_refuse(target, source, "gpu.transfer_index", 3u));
    CHECK(corrupt_and_refuse(target, source, "gpu.owed_words", 0u));
    schway_destroy(target);
    schway_destroy(source);
}

static void test_copy_order(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        CHECK(0);
        return;
    }
    inst->vram[10][10] = 0x0011u;
    inst->vram[10][11] = 0x0022u;
    inst->vram[10][12] = 0x0033u;
    copy_rect(inst, 10u, 10u, 11u, 10u, 3u, 1u);
    CHECK(inst->vram[10][10] == 0x0011u);
    CHECK(inst->vram[10][11] == 0x0011u);
    CHECK(inst->vram[10][12] == 0x0022u);
    CHECK(inst->vram[10][13] == 0x0033u);

    for (uint32_t col = 0u; col < 16u; col++) {
        inst->vram[14][100u + col] = (uint16_t)(0x0100u + col);
    }
    copy_rect(inst, 100u, 14u, 101u, 14u, 16u, 1u);
    CHECK(inst->vram[14][100] == 0x0100u);
    for (uint32_t col = 0u; col < 16u; col++) {
        CHECK(inst->vram[14][101u + col] == (uint16_t)(0x0100u + col));
    }

    for (uint32_t col = 0u; col < 17u; col++) {
        inst->vram[15][200u + col] = (uint16_t)(col + 1u);
    }
    copy_rect(inst, 200u, 15u, 201u, 15u, 17u, 1u);
    CHECK(inst->vram[15][200] == 1u);
    for (uint32_t col = 0u; col < 16u; col++) {
        CHECK(inst->vram[15][201u + col] == (uint16_t)(col + 1u));
    }
    CHECK(inst->vram[15][217] == 16u);

    inst->vram[12][10] = 0u;
    inst->vram[12][11] = 0x0044u;
    inst->vram[12][12] = 0x0055u;
    inst->vram[12][13] = 0x0066u;
    copy_rect(inst, 11u, 12u, 10u, 12u, 3u, 1u);
    CHECK(inst->vram[12][10] == 0x0044u);
    CHECK(inst->vram[12][11] == 0x0055u);
    CHECK(inst->vram[12][12] == 0x0066u);

    inst->vram[20][30] = 0x0101u;
    inst->vram[21][30] = 0x0202u;
    inst->vram[22][30] = 0x0303u;
    copy_rect(inst, 30u, 20u, 30u, 21u, 1u, 3u);
    CHECK(inst->vram[21][30] == 0x0101u);
    CHECK(inst->vram[22][30] == 0x0101u);
    CHECK(inst->vram[23][30] == 0x0101u);

    inst->vram[31][40] = 0x1111u;
    inst->vram[32][40] = 0x2222u;
    inst->vram[33][40] = 0x3333u;
    copy_rect(inst, 40u, 31u, 40u, 30u, 1u, 3u);
    CHECK(inst->vram[30][40] == 0x1111u);
    CHECK(inst->vram[31][40] == 0x2222u);
    CHECK(inst->vram[32][40] == 0x3333u);

    inst->vram[40][100] = 1u;
    inst->vram[40][101] = 2u;
    inst->vram[41][100] = 3u;
    inst->vram[41][101] = 4u;
    copy_rect(inst, 100u, 40u, 101u, 41u, 2u, 2u);
    CHECK(inst->vram[41][101] == 1u);
    CHECK(inst->vram[41][102] == 2u);
    CHECK(inst->vram[42][101] == 3u);
    CHECK(inst->vram[42][102] == 1u);
    schway_destroy(inst);
}

static void test_copy_masks(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        CHECK(0);
        return;
    }
    write_port(inst, GP0, 0xE3000000u | (10u << 10) | 10u);
    write_port(inst, GP0, 0xE4000000u | (10u << 10) | 10u);
    inst->vram[4][4] = 0x0123u;
    inst->vram[4][5] = 0x5678u;
    inst->vram[5][5] = 0x8001u;
    inst->vram[5][6] = 0x0002u;
    write_port(inst, GP0, 0xE6000003u);
    copy_rect(inst, 4u, 4u, 5u, 5u, 2u, 1u);
    CHECK(inst->vram[5][5] == 0x8001u);
    CHECK(inst->vram[5][6] == 0xD678u);

    write_port(inst, GP0, 0xE6000001u);
    inst->vram[7][7] = 0x0123u;
    inst->vram[8][8] = 0x8000u;
    copy_rect(inst, 7u, 7u, 8u, 8u, 1u, 1u);
    CHECK(inst->vram[8][8] == 0x8123u);

    write_port(inst, GP0, 0xE6000002u);
    inst->vram[9][9] = 0x9234u;
    inst->vram[10][10] = 0x8002u;
    inst->vram[9][10] = 0x8456u;
    inst->vram[10][11] = 0x0003u;
    copy_rect(inst, 9u, 9u, 10u, 10u, 2u, 1u);
    CHECK(inst->vram[10][10] == 0x8002u);
    CHECK(inst->vram[10][11] == 0x8456u);

    write_port(inst, GP0, 0xE6000003u);
    inst->vram[12][30] = 0x8001u;
    inst->vram[12][31] = 0x0002u;
    start_transfer(inst, 0xA0000000u, 30u, 12u, 2u, 1u);
    write_port(inst, GP0, 0x1234ABCDu);
    CHECK(inst->vram[12][30] == 0x8001u);
    CHECK(inst->vram[12][31] == 0x9234u);

    inst->vram[14][16] = 0x8001u;
    inst->vram[14][17] = 0x0002u;
    write_port(inst, GP0, 0x020000F8u);
    write_port(inst, GP0, (14u << 16) | 16u);
    write_port(inst, GP0, (1u << 16) | 2u);
    CHECK(inst->vram[14][16] == 0x001Fu);
    CHECK(inst->vram[14][17] == 0x001Fu);
    schway_destroy(inst);
}

static void test_copy_wrap(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        CHECK(0);
        return;
    }
    inst->vram[510][1023] = 0x0011u;
    inst->vram[510][0] = 0x0022u;
    inst->vram[511][1023] = 0x0033u;
    inst->vram[511][0] = 0x0044u;
    copy_rect(inst, 1023u, 510u, 20u, 20u, 2u, 2u);
    CHECK(inst->vram[20][20] == 0x0011u);
    CHECK(inst->vram[20][21] == 0x0022u);
    CHECK(inst->vram[21][20] == 0x0033u);
    CHECK(inst->vram[21][21] == 0x0044u);

    inst->vram[4][4] = 0x0055u;
    inst->vram[4][5] = 0x0066u;
    inst->vram[5][4] = 0x0077u;
    inst->vram[5][5] = 0x0088u;
    copy_rect(inst, 4u, 4u, 1023u, 511u, 2u, 2u);
    CHECK(inst->vram[511][1023] == 0x0055u);
    CHECK(inst->vram[511][0] == 0x0066u);
    CHECK(inst->vram[0][1023] == 0x0077u);
    CHECK(inst->vram[0][0] == 0x0088u);
    schway_destroy(inst);
}

static void test_cache_contract(void)
{
    schway *inst = make_instance();
    schway *other = make_instance();
    if (inst == NULL || other == NULL) {
        CHECK(0);
        schway_destroy(inst);
        schway_destroy(other);
        return;
    }

    inst->vram[7][8] = 0x1122u;
    CHECK(schway__gpu_texture_read(inst, 8u, 7u) == 0x1122u);
    inst->vram[7][8] = 0x3344u;
    CHECK(schway__gpu_texture_read(inst, 1032u, 7u) == 0x1122u);
    write_port(inst, GP0, 0x01000000u);
    CHECK(schway__gpu_texture_read(inst, 8u, 7u) == 0x3344u);

    other->vram[7][8] = 0x5566u;
    CHECK(schway__gpu_texture_read(other, 8u, 7u) == 0x5566u);

    inst->vram[5][1022] = 0x1001u;
    inst->vram[5][1023] = 0x1002u;
    inst->vram[5][0] = 0x1003u;
    CHECK(schway__gpu_clut_read(inst, 1022u, 5u, 0u, 16u) == 0x1001u);
    inst->vram[5][1022] = 0x2001u;
    CHECK(schway__gpu_clut_read(inst, 1022u, 5u, 0u, 16u) == 0x1001u);
    write_port(inst, GP0, 0x01000000u);
    CHECK(inst->gpu.clut_valid == 0u);
    CHECK(schway__gpu_clut_read(inst, 1022u, 5u, 0u, 16u) == 0x2001u);

    inst->vram[5][32] = 0x3001u;
    CHECK(schway__gpu_clut_read(inst, 32u, 5u, 0u, 16u) == 0x3001u);
    inst->vram[5][32] = 0x4001u;
    CHECK(schway__gpu_clut_read(inst, 32u, 5u, 0u, 16u) == 0x3001u);
    write_port(inst, GP1, 0x01000000u);
    CHECK(inst->gpu.clut_valid == 0u);
    CHECK(schway__gpu_clut_read(inst, 32u, 5u, 0u, 16u) == 0x4001u);

    other->vram[5][1022] = 0x9001u;
    CHECK(schway__gpu_clut_read(other, 1022u, 5u, 0u, 16u) == 0x9001u);
    schway_destroy(other);
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-gpu-transfer <case>\n");
        return 2;
    }
    if (strcmp(argv[1], "roundtrip") == 0) {
        test_roundtrip();
    } else if (strcmp(argv[1], "dimensions") == 0) {
        test_dimensions();
    } else if (strcmp(argv[1], "partial_state") == 0) {
        test_partial_state();
    } else if (strcmp(argv[1], "reset_mid_packet") == 0) {
        test_reset_mid_packet();
    } else if (strcmp(argv[1], "invalid_cursor") == 0) {
        test_invalid_cursor();
    } else if (strcmp(argv[1], "copy_order") == 0) {
        test_copy_order();
    } else if (strcmp(argv[1], "copy_masks") == 0) {
        test_copy_masks();
    } else if (strcmp(argv[1], "copy_wrap") == 0) {
        test_copy_wrap();
    } else if (strcmp(argv[1], "cache_contract") == 0) {
        test_cache_contract();
    } else {
        fprintf(stderr, "unknown GPU transfer case: %s\n", argv[1]);
        return 2;
    }
    if (failures != 0) {
        fprintf(stderr, "GPU transfer test %s: %d assertion(s) failed\n", argv[1], failures);
        return 1;
    }
    return 0;
}
