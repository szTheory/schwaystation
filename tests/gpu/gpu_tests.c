/* GPU command tests enter the ports through the real bus. */
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
    CHECK(schway_create(&cfg, &inst) == SCHWAY_STATUS_OK);
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

static void put_le32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
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
    put_le32(candidate + 16u,
             schway__crc32_update(0u, candidate + SCHWAY__STATE_HEADER_SIZE,
                                  size - SCHWAY__STATE_HEADER_SIZE));
    CHECK(schway_save_state(target, before, size) == SCHWAY_STATUS_OK);
    CHECK(schway_load_state(target, candidate, size) == SCHWAY_STATUS_ERR_STATE_FIELD);
    CHECK(schway_save_state(target, after, size) == SCHWAY_STATUS_OK);
    CHECK(memcmp(before, after, size) == 0);
    free(candidate);
    free(before);
    free(after);
    return 1;
}

static void test_state_partial_fill(void)
{
    schway *source = make_instance();
    schway *target = make_instance();
    if (source == NULL || target == NULL) {
        schway_destroy(source);
        schway_destroy(target);
        return;
    }
    write_port(source, GP0, 0x020000F8u);
    write_port(source, GP0, (7u << 16) | 19u);
    size_t size = schway_state_size(source);
    uint8_t *state = (uint8_t *)malloc(size);
    CHECK(state != NULL);
    if (state != NULL) {
        CHECK(schway_save_state(source, state, size) == SCHWAY_STATUS_OK);
        CHECK(schway_load_state(target, state, size) == SCHWAY_STATUS_OK);
        write_port(target, GP0, (2u << 16) | 17u);
        CHECK(target->vram[7][16] == 0x001Fu);
        CHECK(target->vram[8][47] == 0x001Fu);
        free(state);
    }

    schway *packet = make_instance();
    if (packet != NULL) {
        write_port(packet, GP0, 0x020000F8u);
        write_port(packet, GP0, (7u << 16) | 19u);
        CHECK(corrupt_and_refuse(target, packet, "gpu.fifo_count", 17u));
        schway_destroy(packet);
    }

    schway *transfer = make_instance();
    if (transfer != NULL) {
        write_port(transfer, GP0, 0xA0000000u);
        write_port(transfer, GP0, (6u << 16) | 100u);
        write_port(transfer, GP0, (1u << 16) | 16u);
        CHECK(corrupt_and_refuse(target, transfer, "gpu.phase", 99u));
        CHECK(corrupt_and_refuse(target, transfer, "gpu.transfer_x", SCHWAY__VRAM_W));
        schway_destroy(transfer);
    }
    schway_destroy(target);
    schway_destroy(source);
}

static void test_fill_mmio(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return;
    }
    write_port(inst, GP0, 0x020000F8u); /* fill, RGB888 red */
    write_port(inst, GP0, (7u << 16) | 19u); /* x aligns down to 16 */
    write_port(inst, GP0, (2u << 16) | 17u); /* width rounds up to 32 */
    CHECK(inst->vram[7][15] == 0u);
    CHECK(inst->vram[7][16] == 0x001Fu);
    CHECK(inst->vram[7][47] == 0x001Fu);
    CHECK(inst->vram[7][48] == 0u);
    CHECK(inst->vram[8][16] == 0x001Fu);
    CHECK(inst->vram[9][16] == 0u);

    write_port(inst, GP0, 0x0200F800u); /* wrap at the lower-right VRAM edge */
    write_port(inst, GP0, (511u << 16) | 1009u);
    write_port(inst, GP0, (2u << 16) | 17u);
    CHECK(inst->vram[511][1008] == 0x03E0u);
    CHECK(inst->vram[511][0] == 0x03E0u);
    CHECK(inst->vram[0][1008] == 0x03E0u);
    CHECK(inst->vram[0][0] == 0x03E0u);

    write_port(inst, GP0, 0x020000F8u);
    write_port(inst, GP0, 0u);
    write_port(inst, GP0, (512u << 16) | 16u); /* height masks to zero */
    CHECK(inst->vram[0][0] == 0x03E0u);
    schway_destroy(inst);
}

static void test_partial_packet_reset(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return;
    }
    write_port(inst, GP0, 0x020000F8u);
    write_port(inst, GP0, (7u << 16) | 19u);
    write_port(inst, GP1, 0x01000000u); /* reset command buffer */
    write_port(inst, GP0, (2u << 16) | 17u);
    CHECK(inst->vram[7][16] == 0u);
    schway_destroy(inst);
}

static void test_register_images(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return;
    }
    CHECK(read_port(inst, GP1) == 0x14802000u);

    write_port(inst, GP0, 0xE1123456u);
    CHECK((read_port(inst, GP1) & 0x000007FFu) == (0x123456u & 0x7FFu));
    write_port(inst, GP0, 0xE2000000u | 0xABCDEu);
    write_port(inst, GP0, 0xE3000000u | (0x12Au << 10) | 0x155u);
    write_port(inst, GP0, 0xE4000000u | (0x1ABu << 10) | 0x2AAu);
    write_port(inst, GP0, 0xE5000000u | 0x234567u);
    write_port(inst, GP0, 0xE6000003u);

    write_port(inst, GP1, 0x10000002u);
    uint32_t peeked = 0u;
    CHECK(schway__gpu_peek(inst, GP0, 4u, &peeked) == 0);
    CHECK(peeked == 0xABCDEu);
    CHECK(read_port(inst, GP0) == 0xABCDEu);
    write_port(inst, GP1, 0x10000003u);
    CHECK(read_port(inst, GP0) == ((0x12Au << 10) | 0x155u));
    write_port(inst, GP1, 0x10000004u);
    CHECK(read_port(inst, GP0) == ((0x1ABu << 10) | 0x2AAu));
    write_port(inst, GP1, 0x10000005u);
    CHECK(read_port(inst, GP0) == (0x234567u & 0x003FFFFFu));
    write_port(inst, GP1, 0x10000007u);
    CHECK(read_port(inst, GP0) == 2u);
    write_port(inst, GP1, 0x10000006u); /* unknown query preserves the latch */
    CHECK(read_port(inst, GP0) == 2u);
    CHECK((read_port(inst, GP1) & 0x00001800u) == 0x00001800u);

    write_port(inst, GP0, 0x1F000000u);
    CHECK((read_port(inst, GP1) & 0x01000000u) != 0u);
    CHECK((inst->irq.stat & (1u << 1)) != 0u);
    write_port(inst, GP1, 0x02000000u);
    CHECK((read_port(inst, GP1) & 0x01000000u) == 0u);
    CHECK((inst->irq.stat & (1u << 1)) == 0u);

    write_port(inst, GP1, 0x09000001u);
    write_port(inst, GP0, 0x1F000000u);
    write_port(inst, GP1, 0x00000000u);
    CHECK(read_port(inst, GP1) == 0x14802000u);
    CHECK((inst->irq.stat & (1u << 1)) == 0u);
    CHECK(inst->gpu.allow_2mb == 1u);
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-gpu <fill_mmio|partial_packet_reset>\n");
        return 2;
    }
    if (strcmp(argv[1], "fill_mmio") == 0) {
        test_fill_mmio();
    } else if (strcmp(argv[1], "partial_packet_reset") == 0) {
        test_partial_packet_reset();
    } else if (strcmp(argv[1], "register_images") == 0) {
        test_register_images();
    } else if (strcmp(argv[1], "state_partial_fill") == 0) {
        test_state_partial_fill();
    } else {
        fprintf(stderr, "unknown GPU case: %s\n", argv[1]);
        return 2;
    }
    if (failures != 0) {
        fprintf(stderr, "GPU test %s: %d assertion(s) failed\n", argv[1], failures);
        return 1;
    }
    return 0;
}
