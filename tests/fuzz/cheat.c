/* Deterministic replay and bounded fuzzing for the production GameShark parser. */
#include <schwaystation.h>

#include "cheats.h"
#include "seeds.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FUZZ_CHEAT_INPUT_MAX 2048u
#define FUZZ_CHEAT_LINE "30000010 00A5\n"
#define FUZZ_CHEAT_LINE_BYTES (sizeof FUZZ_CHEAT_LINE - 1u)
#define FUZZ_CHEAT_SEED_COUNT 10u
#define FUZZ_CHEAT_MAX_TOUCHED_BYTES (SCHWAY_HOST_CHEAT_TOTAL_LINES * 2u)

enum fuzz_seed {
    SEED_VALID_SINGLE = 0,
    SEED_VALID_BYTE_HALFWORD,
    SEED_EMPTY,
    SEED_UNSUPPORTED_TYPE,
    SEED_MISALIGNED_HALFWORD,
    SEED_OUT_OF_RANGE,
    SEED_LINE_LIMIT,
    SEED_GLOBAL_LIMIT,
    SEED_GLOBAL_LIMIT_EXCEEDED,
    SEED_TEXT_LIMIT
};

static const char *const fuzz_seed_names[FUZZ_CHEAT_SEED_COUNT] = {
    "valid_single", "valid_byte_halfword", "empty", "unsupported_type",
    "misaligned_halfword", "out_of_range", "line_limit", "global_limit",
    "global_limit_exceeded", "text_limit"
};

struct expected_byte {
    uint32_t offset;
    uint8_t value;
};

static void broken(const char *what)
{
    fprintf(stderr, "fuzz.regress.cheat: %s\n", what);
    abort();
}

static void *fuzz_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void fuzz_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static schway *make_instance(void)
{
    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = fuzz_alloc;
    config.allocator.free = fuzz_free;
    schway *inst = NULL;
    if (schway_create(&config, &inst) != SCHWAY_STATUS_OK || inst == NULL) {
        broken("could not create bounded RAM instance");
    }
    return inst;
}

static size_t build_lines(char *text, size_t capacity, uint32_t line_count)
{
    size_t size = 0u;
    if (text == NULL || line_count > SCHWAY_HOST_CHEAT_LINES_PER_ENTRY ||
        capacity < (size_t)line_count * FUZZ_CHEAT_LINE_BYTES) {
        broken("invalid generated cheat seed size");
    }
    for (uint32_t i = 0u; i < line_count; i++) {
        memcpy(text + size, FUZZ_CHEAT_LINE, FUZZ_CHEAT_LINE_BYTES);
        size += FUZZ_CHEAT_LINE_BYTES;
    }
    return size;
}

uint32_t seeds_count(void)
{
    return FUZZ_CHEAT_SEED_COUNT;
}

const char *seeds_name(uint32_t i)
{
    return i < FUZZ_CHEAT_SEED_COUNT ? fuzz_seed_names[i] : "invalid";
}

size_t seeds_build(uint32_t i, uint8_t *buf)
{
    static const char valid_single[] = "30000020 00A5";
    static const char valid_pair[] = "30000020 00A5\n80000022 1234";
    static const char unsupported[] = "90000020 00EE";
    static const char misaligned[] = "80000021 1234";
    static const char out_of_range[] = "30200000 0011";
    char lines[SCHWAY_HOST_CHEAT_ENTRY_TEXT_MAX + 2u];
    size_t text_size = 0u;
    uint8_t mode = 0u;
    if (buf == NULL || i >= FUZZ_CHEAT_SEED_COUNT) return 0u;
    switch (i) {
    case SEED_VALID_SINGLE:
        text_size = sizeof valid_single - 1u;
        memcpy(buf + 1u, valid_single, text_size);
        break;
    case SEED_VALID_BYTE_HALFWORD:
        mode = 1u;
        text_size = sizeof valid_pair - 1u;
        memcpy(buf + 1u, valid_pair, text_size);
        break;
    case SEED_EMPTY:
        break;
    case SEED_UNSUPPORTED_TYPE:
        text_size = sizeof unsupported - 1u;
        memcpy(buf + 1u, unsupported, text_size);
        break;
    case SEED_MISALIGNED_HALFWORD:
        text_size = sizeof misaligned - 1u;
        memcpy(buf + 1u, misaligned, text_size);
        break;
    case SEED_OUT_OF_RANGE:
        text_size = sizeof out_of_range - 1u;
        memcpy(buf + 1u, out_of_range, text_size);
        break;
    case SEED_LINE_LIMIT:
        text_size = build_lines(lines, sizeof lines, SCHWAY_HOST_CHEAT_LINES_PER_ENTRY) +
                    FUZZ_CHEAT_LINE_BYTES;
        memcpy(buf + 1u, lines, SCHWAY_HOST_CHEAT_LINES_PER_ENTRY * FUZZ_CHEAT_LINE_BYTES);
        memcpy(buf + 1u + SCHWAY_HOST_CHEAT_LINES_PER_ENTRY * FUZZ_CHEAT_LINE_BYTES,
               FUZZ_CHEAT_LINE, FUZZ_CHEAT_LINE_BYTES);
        break;
    case SEED_GLOBAL_LIMIT:
        text_size = sizeof valid_single - 1u;
        memcpy(buf + 1u, valid_single, text_size);
        break;
    case SEED_GLOBAL_LIMIT_EXCEEDED:
        text_size = build_lines(lines, sizeof lines, 2u);
        memcpy(buf + 1u, lines, text_size);
        break;
    case SEED_TEXT_LIMIT:
        text_size = SCHWAY_HOST_CHEAT_ENTRY_TEXT_MAX + 1u;
        memset(buf + 1u, '0', text_size);
        break;
    default:
        return 0u;
    }
    buf[0] = mode;
    return text_size + 1u;
}

static void baseline_build(schway__host_cheats *cheats, uint32_t total_lines)
{
    char text[SCHWAY_HOST_CHEAT_ENTRY_TEXT_MAX];
    uint32_t remaining = total_lines;
    schway__host_cheats_reset(cheats);
    for (uint32_t index = 0u; index < 4u; index++) {
        uint32_t lines = remaining > SCHWAY_HOST_CHEAT_LINES_PER_ENTRY ?
                         SCHWAY_HOST_CHEAT_LINES_PER_ENTRY : remaining;
        size_t text_size = build_lines(text, sizeof text, lines);
        if (lines != 0u && !schway__host_cheats_replace(cheats, index, true, text, text_size)) {
            broken("could not build prior full-table cheat state");
        }
        remaining -= lines;
    }
    if (remaining != 0u || cheats->total_lines != total_lines ||
        !schway__host_cheats_validate(cheats)) {
        broken("prior full-table cheat state exceeded its bound");
    }
}

static void expected_set(struct expected_byte *bytes, size_t *count,
                         uint32_t offset, uint8_t value)
{
    for (size_t i = 0u; i < *count; i++) {
        if (bytes[i].offset == offset) {
            bytes[i].value = value;
            return;
        }
    }
    if (*count >= FUZZ_CHEAT_MAX_TOUCHED_BYTES) broken("expected-byte model exceeded its bound");
    bytes[*count].offset = offset;
    bytes[*count].value = value;
    (*count)++;
}

static size_t expected_ram(const schway__host_cheats *cheats,
                           struct expected_byte bytes[FUZZ_CHEAT_MAX_TOUCHED_BYTES])
{
    size_t count = 0u;
    for (uint32_t i = 0u; i < SCHWAY_HOST_CHEAT_ENTRY_COUNT; i++) {
        const schway__host_cheat_entry *entry = &cheats->entries[i];
        if (entry->enabled == 0u) continue;
        for (uint32_t j = 0u; j < entry->line_count; j++) {
            const schway__host_cheat_write *write = &entry->lines[j];
            expected_set(bytes, &count, write->offset, (uint8_t)write->value);
            if (write->width == 2u) {
                expected_set(bytes, &count, write->offset + 1u,
                             (uint8_t)(write->value >> 8u));
            }
        }
    }
    return count;
}

static void check_ram(const schway *inst, const schway__host_cheats *cheats)
{
    struct expected_byte expected[FUZZ_CHEAT_MAX_TOUCHED_BYTES];
    size_t count = expected_ram(cheats, expected);
    for (size_t i = 0u; i < count; i++) {
        uint32_t actual = UINT32_MAX;
        if (schway_peek(inst, SCHWAY_SPACE_RAM, expected[i].offset, 1u, &actual) !=
                SCHWAY_STATUS_OK || actual != expected[i].value) {
            broken("bounded ordered cheat writes did not match the instance RAM");
        }
    }
}

#ifdef SCHWAY_FUZZ_REPLAY
static int find_known_seed(const uint8_t *data, size_t size)
{
    uint8_t seed[2048];
    for (uint32_t i = 0u; i < seeds_count(); i++) {
        size_t seed_size = seeds_build(i, seed);
        if (seed_size == size && memcmp(seed, data, size) == 0) return (int)i;
    }
    return -1;
}
#endif

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    schway__host_cheats baseline;
    schway__host_cheats candidate_a;
    schway__host_cheats candidate_b;
    schway__host_cheats decoded;
    uint8_t encoded[SCHWAY_HOST_CHEAT_SERIALIZED_SIZE];
    uint8_t encoded_again[SCHWAY_HOST_CHEAT_SERIALIZED_SIZE];
    schway *inst;
    const uint8_t mode = size == 0u ? 0u : (uint8_t)(data[0] & 1u);
    const char *text = size <= 1u ? "" : (const char *)(data + 1u);
    size_t text_size = size <= 1u ? 0u : size - 1u;
    bool accepted_a;
    bool accepted_b;
    if (size > FUZZ_CHEAT_INPUT_MAX) return 0;
    baseline_build(&baseline, mode == 0u ? SCHWAY_HOST_CHEAT_TOTAL_LINES - 1u :
                                          SCHWAY_HOST_CHEAT_TOTAL_LINES - 2u);
    if (!schway__host_cheats_encode(&baseline, encoded, sizeof encoded)) {
        broken("could not encode the prior full-table cheat state");
    }
    inst = make_instance();
    if (!schway__host_cheats_apply(&baseline, inst)) {
        schway_destroy(inst);
        broken("prior cheat state did not apply to RAM");
    }
    check_ram(inst, &baseline);
    candidate_a = baseline;
    candidate_b = baseline;
    accepted_a = schway__host_cheats_replace(&candidate_a, 4u, true, text, text_size);
    accepted_b = schway__host_cheats_replace(&candidate_b, 4u, true, text, text_size);
    if (accepted_a != accepted_b || memcmp(&candidate_a, &candidate_b, sizeof candidate_a) != 0) {
        broken("repeated cheat replacement was not deterministic");
    }
    if (!schway__host_cheats_validate(&candidate_a)) {
        broken("cheat replacement produced an invalid normalized table");
    }
    if (accepted_a) {
        if (!schway__host_cheats_encode(&candidate_a, encoded, sizeof encoded) ||
            !schway__host_cheats_decode(&decoded, encoded, sizeof encoded) ||
            memcmp(&candidate_a, &decoded, sizeof candidate_a) != 0 ||
            !schway__host_cheats_encode(&decoded, encoded_again, sizeof encoded_again) ||
            memcmp(encoded, encoded_again, sizeof encoded) != 0) {
            broken("accepted cheat table did not round-trip as canonical bytes");
        }
    } else {
        if (memcmp(&candidate_a, &baseline, sizeof baseline) != 0 ||
            !schway__host_cheats_encode(&candidate_a, encoded_again, sizeof encoded_again) ||
            memcmp(encoded, encoded_again, sizeof encoded) != 0) {
            broken("rejected replacement changed the prior full-table configuration");
        }
    }

    if (accepted_a) {
        if (!schway__host_cheats_apply(&candidate_a, inst)) {
            schway_destroy(inst);
            broken("accepted bounded cheat configuration did not apply");
        }
        check_ram(inst, &candidate_a);
    } else {
        check_ram(inst, &baseline);
    }

#ifdef SCHWAY_FUZZ_REPLAY
    int seed = find_known_seed(data, size);
    if ((seed == SEED_VALID_SINGLE || seed == SEED_VALID_BYTE_HALFWORD ||
         seed == SEED_GLOBAL_LIMIT) && !accepted_a) {
        schway_destroy(inst);
        broken("valid generated cheat seed was rejected");
    }
    if ((seed == SEED_EMPTY || seed == SEED_UNSUPPORTED_TYPE ||
         seed == SEED_MISALIGNED_HALFWORD || seed == SEED_OUT_OF_RANGE ||
         seed == SEED_LINE_LIMIT || seed == SEED_GLOBAL_LIMIT_EXCEEDED ||
         seed == SEED_TEXT_LIMIT) && accepted_a) {
        schway_destroy(inst);
        broken("invalid or over-limit generated cheat seed was accepted");
    }
#endif
    schway_destroy(inst);
    return 0;
}

#define SCHWAY_REPLAY_NAME "fuzz.regress.cheat"
#include "replay_main.h"
