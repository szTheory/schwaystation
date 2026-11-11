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
    CHECK_EQ_U64(info.status, SCHWAY_BIOS_STATUS_UNKNOWN);
    CHECK_EQ_U64(info.region, SCHWAY_BIOS_REGION_UNKNOWN);
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

struct bios_fields {
    uint32_t loaded_count;
    uint32_t digest_count;
    uint32_t raw_bytes_count;
};

static void visit_bios_fields(void *user, const char *name, const void *data, uint32_t count,
                              uint32_t width)
{
    struct bios_fields *fields = (struct bios_fields *)user;
    (void)data;
    if (strcmp(name, "bus.bios.loaded") == 0 && count == 1u && width == 4u) {
        fields->loaded_count++;
    } else if (strcmp(name, "bus.bios.sha256") == 0 && count == 32u && width == 1u) {
        fields->digest_count++;
    } else if (strstr(name, "bios.bytes") != NULL) {
        fields->raw_bytes_count++;
    }
}

static void copy_edges(void)
{
    uint8_t *rom_a = make_rom(42u);
    uint8_t *rom_b = make_rom(99u);
    CHECK(rom_a != NULL && rom_b != NULL);
    if (rom_a == NULL || rom_b == NULL) {
        free(rom_a);
        free(rom_b);
        return;
    }

    schway *source = new_instance();
    schway *different = new_instance();
    CHECK(source != NULL && different != NULL);
    if (source == NULL || different == NULL) {
        schway_destroy(source);
        schway_destroy(different);
        free(rom_a);
        free(rom_b);
        return;
    }

    schway_bios_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    CHECK_EQ_U64(schway_get_bios_info(source, &info), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(info.flags, 0u);
    CHECK_EQ_U64(info.status, SCHWAY_BIOS_STATUS_UNKNOWN);
    CHECK_EQ_U64(info.region, SCHWAY_BIOS_REGION_UNKNOWN);
    uint8_t zero_digest[sizeof info.sha256] = {0};
    CHECK(memcmp(info.sha256, zero_digest, sizeof zero_digest) == 0);

    schway_span before;
    CHECK_EQ_U64(schway_get_memory(source, SCHWAY_MEMORY_BIOS, &before), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_load_bios(NULL, rom_a, SCHWAY_BIOS_SIZE), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_load_bios(source, NULL, SCHWAY_BIOS_SIZE), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_load_bios(source, rom_a, 0u), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_load_bios(source, rom_a, SCHWAY_BIOS_SIZE - 1u),
                 SCHWAY_STATUS_ERR_ARGUMENT);
    uint8_t *overlong = (uint8_t *)calloc(1u, SCHWAY_BIOS_SIZE + 1u);
    CHECK(overlong != NULL);
    if (overlong != NULL) {
        CHECK_EQ_U64(schway_load_bios(source, overlong, SCHWAY_BIOS_SIZE + 1u),
                     SCHWAY_STATUS_ERR_ARGUMENT);
    }
    free(overlong);
    schway_span after_invalid;
    CHECK_EQ_U64(schway_get_memory(source, SCHWAY_MEMORY_BIOS, &after_invalid), SCHWAY_STATUS_OK);
    CHECK(before.data == after_invalid.data);
    CHECK_EQ_U64(before.size, after_invalid.size);
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    CHECK_EQ_U64(schway_get_bios_info(source, &info), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(info.flags, 0u);
    CHECK_EQ_U64(info.status, SCHWAY_BIOS_STATUS_UNKNOWN);
    CHECK_EQ_U64(info.region, SCHWAY_BIOS_REGION_UNKNOWN);
    CHECK(memcmp(info.sha256, zero_digest, sizeof zero_digest) == 0);
    schway_bios_info short_info;
    memset(&short_info, 0, sizeof short_info);
    short_info.size = 39u;
    CHECK_EQ_U64(schway_get_bios_info(source, &short_info), SCHWAY_STATUS_ERR_STRUCT_SIZE);

    CHECK_EQ_U64(schway_load_bios(source, rom_a, SCHWAY_BIOS_SIZE), SCHWAY_STATUS_OK);
    schway_span copied;
    CHECK_EQ_U64(schway_get_memory(source, SCHWAY_MEMORY_BIOS, &copied), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(copied.size, SCHWAY_BIOS_SIZE);
    CHECK(copied.data != rom_a);
    rom_a[0] = 0u;
    free(rom_a);
    CHECK_EQ_HEX(((const uint8_t *)copied.data)[0], 0x01u);

    schway_bios_info source_info;
    memset(&source_info, 0, sizeof source_info);
    source_info.size = (uint32_t)sizeof source_info;
    CHECK_EQ_U64(schway_get_bios_info(source, &source_info), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(source_info.flags, SCHWAY_BIOS_INFO_LOADED);
    CHECK_EQ_U64(source_info.status, SCHWAY_BIOS_STATUS_UNKNOWN);
    CHECK_EQ_U64(source_info.region, SCHWAY_BIOS_REGION_UNKNOWN);
    CHECK_EQ_U64(schway_load_bios(source, rom_b, SCHWAY_BIOS_SIZE - 1u),
                 SCHWAY_STATUS_ERR_ARGUMENT);
    schway_span after_loaded_refusal;
    CHECK_EQ_U64(schway_get_memory(source, SCHWAY_MEMORY_BIOS, &after_loaded_refusal),
                 SCHWAY_STATUS_OK);
    CHECK(after_loaded_refusal.data == copied.data);
    CHECK_EQ_HEX(((const uint8_t *)after_loaded_refusal.data)[0], 0x01u);
    CHECK_EQ_U64(schway_load_bios(different, rom_b, SCHWAY_BIOS_SIZE), SCHWAY_STATUS_OK);
    schway *matching = new_instance();
    CHECK(matching != NULL);
    int matching_loaded = 0;
    if (matching != NULL) {
        uint8_t *matching_rom = make_rom(42u);
        CHECK(matching_rom != NULL);
        if (matching_rom != NULL) {
            matching_loaded = schway_load_bios(matching, matching_rom, SCHWAY_BIOS_SIZE) ==
                              SCHWAY_STATUS_OK;
            CHECK(matching_loaded);
            free(matching_rom);
        }
    }

    size_t state_size = schway_state_size(source);
    uint8_t *state = (uint8_t *)malloc(state_size);
    uint8_t *before_state = (uint8_t *)malloc(state_size);
    uint8_t *after_state = (uint8_t *)malloc(state_size);
    CHECK(state != NULL && before_state != NULL && after_state != NULL);
    if (state != NULL && before_state != NULL && after_state != NULL && matching_loaded != 0) {
        CHECK_EQ_U64(schway_save_state(source, state, state_size), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(schway_load_state(matching, state, state_size), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(schway_save_state(different, before_state, state_size), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(schway_load_state(different, state, state_size),
                     SCHWAY_STATUS_ERR_STATE_FIELD);
        CHECK_EQ_U64(schway_save_state(different, after_state, state_size), SCHWAY_STATUS_OK);
        CHECK(memcmp(before_state, after_state, state_size) == 0);
    }
    free(state);
    free(before_state);
    free(after_state);

    struct bios_fields fields = {0u, 0u, 0u};
    CHECK_EQ_U64(schway_state_visit(source, visit_bios_fields, &fields), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(fields.loaded_count, 1u);
    CHECK_EQ_U64(fields.digest_count, 1u);
    CHECK_EQ_U64(fields.raw_bytes_count, 0u);

    CHECK_EQ_U64(schway_reset(source, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    schway_bios_info after_reset;
    memset(&after_reset, 0, sizeof after_reset);
    after_reset.size = (uint32_t)sizeof after_reset;
    CHECK_EQ_U64(schway_get_bios_info(source, &after_reset), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(after_reset.flags, SCHWAY_BIOS_INFO_LOADED);
    CHECK_EQ_U64(after_reset.status, SCHWAY_BIOS_STATUS_UNKNOWN);
    CHECK_EQ_U64(after_reset.region, SCHWAY_BIOS_REGION_UNKNOWN);
    CHECK(memcmp(after_reset.sha256, source_info.sha256, sizeof source_info.sha256) == 0);
    schway_span after_reset_span;
    CHECK_EQ_U64(schway_get_memory(source, SCHWAY_MEMORY_BIOS, &after_reset_span),
                 SCHWAY_STATUS_OK);
    CHECK(after_reset_span.data == copied.data);
    schway_budget budget = {(uint32_t)sizeof budget, SCHWAY_RUN_TRACE, 256u};
    schway_run_result result;
    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    CHECK_EQ_U64(schway_run(source, &budget, &result), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(result.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(schway_load_bios(source, rom_b, SCHWAY_BIOS_SIZE), SCHWAY_STATUS_ERR_ARGUMENT);
    schway_bios_info after_late_load;
    memset(&after_late_load, 0, sizeof after_late_load);
    after_late_load.size = (uint32_t)sizeof after_late_load;
    CHECK_EQ_U64(schway_get_bios_info(source, &after_late_load), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(after_late_load.status, SCHWAY_BIOS_STATUS_UNKNOWN);
    CHECK_EQ_U64(after_late_load.region, SCHWAY_BIOS_REGION_UNKNOWN);
    CHECK(memcmp(after_late_load.sha256, source_info.sha256, sizeof source_info.sha256) == 0);
    schway_span after_late_span;
    CHECK_EQ_U64(schway_get_memory(source, SCHWAY_MEMORY_BIOS, &after_late_span),
                 SCHWAY_STATUS_OK);
    CHECK(after_late_span.data == copied.data);
    CHECK_EQ_HEX(((const uint8_t *)after_late_span.data)[0], 0x01u);

    free(rom_b);
    schway_destroy(matching);
    schway_destroy(source);
    schway_destroy(different);
}

static void identity(void)
{
    static uint8_t zero_bios[SCHWAY_BIOS_SIZE];
    static const uint8_t expected_sha256[32] = {
        0x07u, 0x85u, 0x4Du, 0x2Fu, 0xEFu, 0x29u, 0x7Au, 0x06u,
        0xBAu, 0x81u, 0x68u, 0x5Eu, 0x66u, 0x0Cu, 0x33u, 0x2Du,
        0xE3u, 0x6Du, 0x5Du, 0x18u, 0xD5u, 0x46u, 0x92u, 0x7Du,
        0x30u, 0xDAu, 0xADu, 0x6Du, 0x7Fu, 0xDAu, 0x15u, 0x41u,
    };
    schway *inst = new_instance();
    if (inst == NULL) {
        return;
    }
    CHECK_EQ_U64(schway_load_bios(inst, zero_bios, sizeof zero_bios), SCHWAY_STATUS_OK);
    schway_bios_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    CHECK_EQ_U64(schway_get_bios_info(inst, &info), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(info.flags, SCHWAY_BIOS_INFO_LOADED | SCHWAY_BIOS_INFO_RECOGNIZED);
    CHECK_EQ_U64(info.status, SCHWAY_BIOS_STATUS_RECOGNIZED);
    CHECK_EQ_U64(info.region, SCHWAY_REGION_NTSC_U);
    CHECK(memcmp(info.sha256, expected_sha256, sizeof expected_sha256) == 0);
    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    CHECK_EQ_U64(schway_get_bios_info(inst, &info), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(info.flags, SCHWAY_BIOS_INFO_LOADED | SCHWAY_BIOS_INFO_RECOGNIZED);
    CHECK_EQ_U64(info.status, SCHWAY_BIOS_STATUS_RECOGNIZED);
    CHECK_EQ_U64(info.region, SCHWAY_REGION_NTSC_U);
    CHECK(memcmp(info.sha256, expected_sha256, sizeof expected_sha256) == 0);
    schway_destroy(inst);
}

static void identity_edges(void)
{
    static uint8_t zero_bios[SCHWAY_BIOS_SIZE];
    schway *inst = new_instance();
    if (inst == NULL) {
        return;
    }
    CHECK_EQ_U64(schway_load_bios(inst, zero_bios, sizeof zero_bios), SCHWAY_STATUS_OK);

    /* The synthetic lookup key is isolated from the production digest table. */
    schway_bios_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    CHECK_EQ_U64(schway_get_bios_info(inst, &info), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(info.flags, SCHWAY_BIOS_INFO_LOADED);
    CHECK_EQ_U64(info.status, SCHWAY_BIOS_STATUS_UNKNOWN);
    CHECK_EQ_U64(info.region, SCHWAY_BIOS_REGION_UNKNOWN);
    uint8_t sha256_before_reset[sizeof info.sha256];
    memcpy(sha256_before_reset, info.sha256, sizeof sha256_before_reset);

    /* The original 40-byte output prefix remains valid and is not over-written. */
    schway_bios_info legacy;
    memset(&legacy, 0xA5, sizeof legacy);
    legacy.size = 40u;
    CHECK_EQ_U64(schway_get_bios_info(inst, &legacy), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(legacy.flags, SCHWAY_BIOS_INFO_LOADED);
    CHECK_EQ_HEX(legacy.status, 0xA5A5A5A5u);
    CHECK_EQ_HEX(legacy.region, 0xA5A5A5A5u);

    schway_bios_info invalid;
    memset(&invalid, 0x5A, sizeof invalid);
    invalid.size = 39u;
    schway_bios_info before = invalid;
    CHECK_EQ_U64(schway_get_bios_info(inst, &invalid), SCHWAY_STATUS_ERR_STRUCT_SIZE);
    CHECK(memcmp(&invalid, &before, sizeof invalid) == 0);

    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    CHECK_EQ_U64(schway_get_bios_info(inst, &info), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(info.flags, SCHWAY_BIOS_INFO_LOADED);
    CHECK_EQ_U64(info.status, SCHWAY_BIOS_STATUS_UNKNOWN);
    CHECK_EQ_U64(info.region, SCHWAY_BIOS_REGION_UNKNOWN);
    CHECK(memcmp(info.sha256, sha256_before_reset, sizeof sha256_before_reset) == 0);
    schway_destroy(inst);
}

static void test_key_isolation(void)
{
    static uint8_t synthetic_bios[SCHWAY_BIOS_SIZE];
    schway *inst = new_instance();
    if (inst == NULL) {
        return;
    }
    CHECK_EQ_U64(schway_load_bios(inst, synthetic_bios, sizeof synthetic_bios), SCHWAY_STATUS_OK);

    schway_bios_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    CHECK_EQ_U64(schway_get_bios_info(inst, &info), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(info.flags, SCHWAY_BIOS_INFO_LOADED);
    CHECK_EQ_U64(info.status, SCHWAY_BIOS_STATUS_UNKNOWN);
    CHECK_EQ_U64(info.region, SCHWAY_BIOS_REGION_UNKNOWN);
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    CHECK(argc == 2);
    if (argc == 2 && strcmp(argv[1], "synthetic_rom") == 0) {
        synthetic_rom();
    } else if (argc == 2 && strcmp(argv[1], "copy_edges") == 0) {
        copy_edges();
    } else if (argc == 2 && strcmp(argv[1], "identity") == 0) {
        identity();
    } else if (argc == 2 && strcmp(argv[1], "identity_edges") == 0) {
        identity_edges();
    } else if (argc == 2 && strcmp(argv[1], "test_key_isolation") == 0) {
        test_key_isolation();
    } else {
        CHECK(0);
    }
    CHECK_DONE();
}
