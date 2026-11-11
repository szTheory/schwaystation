/* Synthetic BIOS behavior tests. These bytes are constructed at runtime and
   contain no firmware or third-party data. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "check.h"

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

static void put_le32(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)value;
    dst[1] = (uint8_t)(value >> 8);
    dst[2] = (uint8_t)(value >> 16);
    dst[3] = (uint8_t)(value >> 24);
}

static uint8_t *make_rom(uint32_t marker)
{
    uint8_t *bytes = (uint8_t *)calloc(1u, SCHWAY_BIOS_SIZE);
    if (bytes == NULL) {
        return NULL;
    }
    put_le32(bytes, 0x3C090001u); /* LUI t1, 1 */
    put_le32(bytes + 4u, 0x24080000u | (marker & 0xFFFFu)); /* ADDIU t0, zero, marker */
    put_le32(bytes + 8u, 0xAD280000u); /* SW t0, 0(t1) */
    return bytes;
}

static schway *new_instance(void)
{
    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = test_alloc;
    config.allocator.free = test_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&config, &inst), SCHWAY_STATUS_OK);
    return inst;
}

static void synthetic_rom(void)
{
    static const uint8_t expected_sha256[32] = {
        0x63u, 0xE9u, 0xE1u, 0x49u, 0xD6u, 0x98u, 0xFAu, 0xFAu,
        0xB0u, 0x37u, 0x24u, 0x27u, 0xDEu, 0x5Bu, 0x55u, 0xEDu,
        0xA5u, 0x0Cu, 0x29u, 0x9Fu, 0xBAu, 0xADu, 0x60u, 0x09u,
        0x33u, 0x93u, 0xE4u, 0x89u, 0xB9u, 0x45u, 0xBFu, 0x91u,
    };
    schway *inst = new_instance();
    if (inst == NULL) {
        return;
    }
    uint8_t *source = make_rom(42u);
    CHECK(source != NULL);
    if (source == NULL) {
        schway_destroy(inst);
        return;
    }
    CHECK_EQ_U64(schway_load_bios(inst, source, SCHWAY_BIOS_SIZE), SCHWAY_STATUS_OK);

    schway_bios_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    CHECK_EQ_U64(schway_get_bios_info(inst, &info), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(info.flags, SCHWAY_BIOS_INFO_LOADED);
    CHECK(memcmp(info.sha256, expected_sha256, sizeof expected_sha256) == 0);

    source[0] = 0u;
    free(source);
    schway_span bios;
    CHECK_EQ_U64(schway_get_memory(inst, SCHWAY_MEMORY_BIOS, &bios), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(bios.size, SCHWAY_BIOS_SIZE);
    CHECK(bios.data != NULL);
    if (bios.data != NULL && bios.size == SCHWAY_BIOS_SIZE) {
        CHECK_EQ_HEX(((const uint8_t *)bios.data)[0], 0x01u);
    }

    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    schway_budget budget = {(uint32_t)sizeof budget, SCHWAY_RUN_TRACE, 256u};
    schway_run_result result;
    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    CHECK_EQ_U64(schway_run(inst, &budget, &result), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(result.stop, SCHWAY_STOP_BUDGET);
    schway_trace trace;
    memset(&trace, 0, sizeof trace);
    trace.size = (uint32_t)sizeof trace;
    CHECK_EQ_U64(schway_get_trace(inst, 0u, &trace), SCHWAY_STATUS_OK);
    CHECK_EQ_HEX(trace.pc, 0xBFC00000u);
    CHECK_EQ_HEX(trace.opcode, 0x3C090001u);
    uint32_t marker = 0u;
    CHECK_EQ_U64(schway_peek(inst, SCHWAY_SPACE_RAM, 0x10000u, 4u, &marker),
                 SCHWAY_STATUS_OK);
    CHECK_EQ_U64(marker, 42u);
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    CHECK(argc == 2);
    if (argc == 2 && strcmp(argv[1], "synthetic_rom") == 0) {
        synthetic_rom();
    } else {
        CHECK(0);
    }
    CHECK_DONE();
}
