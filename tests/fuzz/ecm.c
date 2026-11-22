/* Bounded replay of hostile ECM streams through the production image reader. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disc_ecm.h"
#include "disc_image.h"
#include "../test_open.h"
#ifdef SCHWAY_FUZZ_REPLAY
#include "seeds.h"
#endif

#ifndef SCHWAY_FUZZ_WORK_DIR
#error "CMake must provide a build-owned ECM fuzz directory"
#endif
#ifndef SCHWAY_FUZZ_ECM_LITERAL
#error "CMake must provide the owned literal ECM fixture"
#endif
#ifndef SCHWAY_FUZZ_ECM_MODELED
#error "CMake must provide the owned modeled ECM fixture"
#endif

#define ECM_INPUT_MAX 2101249u
#define ECM_READ_MAX 70240u
#define ECM_RECORD_MAX 1048576u
#define ECM_CHECKPOINT_MAX 16386u
#define ECM_SEED_COUNT 10u

static void broken(const char *what)
{
    fprintf(stderr, "fuzz.regress.ecm: %s\n", what);
    abort();
}

static int write_file(const char *path, const uint8_t *bytes, size_t size)
{
    FILE *file = schway__test_open_file(path, "wb");
    int ok;
    if (file == NULL) return 0;
    ok = size == 0u || fwrite(bytes, 1u, size, file) == size;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

#ifdef SCHWAY_FUZZ_REPLAY
static uint8_t *read_fixture(const char *path, size_t *size)
{
    FILE *file = schway__test_open_file(path, "rb");
    long length;
    uint8_t *bytes;
    if (file == NULL) return NULL;
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0 ||
        (size_t)length > ECM_INPUT_MAX || fseek(file, 0, SEEK_SET) != 0) {
        (void)fclose(file);
        return NULL;
    }
    bytes = (uint8_t *)malloc(length == 0 ? 1u : (size_t)length);
    if (bytes == NULL || ((size_t)length != 0u &&
        fread(bytes, 1u, (size_t)length, file) != (size_t)length)) {
        free(bytes);
        (void)fclose(file);
        return NULL;
    }
    if (fclose(file) != 0) {
        free(bytes);
        return NULL;
    }
    *size = (size_t)length;
    return bytes;
}

static int put_count(uint8_t *bytes, size_t capacity, size_t *at,
                     uint8_t type, uint64_t count)
{
    uint64_t value;
    uint8_t byte;
    if (count == 0u || count > UINT32_MAX) return 0;
    value = count - 1u;
    byte = (uint8_t)(((value & 31u) << 2) | type);
    value >>= 5;
    if (value != 0u) byte |= 0x80u;
    if (*at >= capacity) return 0;
    bytes[(*at)++] = byte;
    while (value != 0u) {
        byte = (uint8_t)(value & 0x7fu);
        value >>= 7;
        if (value != 0u) byte |= 0x80u;
        if (*at >= capacity) return 0;
        bytes[(*at)++] = byte;
    }
    return 1;
}

static const char *const ecm_seed_names[ECM_SEED_COUNT] = {
    "valid_literal", "valid_modeled", "bad_magic", "varint_overflow",
    "record_limit", "decoded_extent", "modeled_type1", "bad_terminator",
    "bad_edc", "truncated_tail",
};

uint32_t seeds_count(void) { return ECM_SEED_COUNT; }

const char *seeds_name(uint32_t index)
{
    if (index >= ECM_SEED_COUNT) return "invalid";
    if (index == 0u) {
        printf("fuzz.regress.ecm seed_count=%lu file_cap=%lu fixtures=disc_boot_legacy_literal.ecm,disc_boot_legacy_modeled.ecm\n",
               (unsigned long)seeds_count(), (unsigned long)SEEDS_SIZE_MAX);
    }
    printf("fuzz.regress.ecm seed[%lu]=%s\n", (unsigned long)index,
           ecm_seed_names[index]);
    return ecm_seed_names[index];
}

size_t seeds_build(uint32_t index, uint8_t *bytes)
{
    static uint8_t *literal;
    static size_t literal_size;
    static uint8_t *modeled;
    static size_t modeled_size;
    size_t at = 0u;
    if (bytes == NULL || index >= ECM_SEED_COUNT) return 0u;
    if (index == 0u || index == 9u) {
        if (literal == NULL) literal = read_fixture(SCHWAY_FUZZ_ECM_LITERAL, &literal_size);
        if (literal == NULL || literal_size > SEEDS_SIZE_MAX) return 0u;
        memcpy(bytes, literal, literal_size);
        if (index == 9u) return literal_size - 1u;
        return literal_size;
    }
    if (index == 1u) {
        if (modeled == NULL) modeled = read_fixture(SCHWAY_FUZZ_ECM_MODELED, &modeled_size);
        if (modeled == NULL || modeled_size > SEEDS_SIZE_MAX) return 0u;
        memcpy(bytes, modeled, modeled_size);
        return modeled_size;
    }
    memset(bytes, 0, SEEDS_SIZE_MAX);
    memcpy(bytes, "ECM\0", 4u);
    at = 4u;
    if (index == 2u) {
        bytes[0] = 'B';
        return 4u;
    }
    if (index == 3u) {
        bytes[at++] = 0xfcu;
        bytes[at++] = 0xffu;
        bytes[at++] = 0xffu;
        bytes[at++] = 0xffu;
        bytes[at++] = 0x7fu;
        return at;
    }
    if (index == 4u) {
        for (uint32_t i = 0u; i <= ECM_RECORD_MAX; i++) {
            bytes[at++] = 0u;
            bytes[at++] = (uint8_t)i;
        }
        return at;
    }
    if (index == 5u) {
        if (!put_count(bytes, SEEDS_SIZE_MAX, &at, 0u, UINT64_C(1073741825))) return 0u;
        return at;
    }
    if (index == 6u) {
        uint32_t amount = 2051u;
        if (!put_count(bytes, SEEDS_SIZE_MAX, &at, 1u, 1u)) return 0u;
        for (uint32_t i = 0u; i < amount; i++) bytes[at++] = (uint8_t)(i * 13u + 7u);
        bytes[at++] = 0xfcu; bytes[at++] = 0xffu; bytes[at++] = 0xffu;
        bytes[at++] = 0xffu; bytes[at++] = 0x3fu;
        at += 4u;
        return at;
    }
    if (index == 7u || index == 8u) {
        if (literal == NULL) literal = read_fixture(SCHWAY_FUZZ_ECM_LITERAL, &literal_size);
        if (literal == NULL || literal_size > SEEDS_SIZE_MAX) return 0u;
        memcpy(bytes, literal, literal_size);
        if (index == 7u) bytes[literal_size - 9u] ^= 1u;
        else bytes[literal_size - 1u] ^= 1u;
        return literal_size;
    }
    return 0u;
}

static int replay_seed(const uint8_t *data, size_t size)
{
    static uint8_t *candidate;
    if (candidate == NULL) {
        candidate = (uint8_t *)malloc(SEEDS_SIZE_MAX);
        if (candidate == NULL) broken("could not allocate seed comparator");
    }
    for (uint32_t i = 0u; i < seeds_count(); i++) {
        size_t expected_size = seeds_build(i, candidate);
        if (expected_size == size && memcmp(data, candidate, size) == 0) return (int)i;
    }
    return -1;
}
#endif

static int limits_are_bounded(void)
{
    return schway__disc_ecm_test_limits(UINT64_C(1073741824), UINT64_C(1073741824),
                                        ECM_RECORD_MAX, ECM_CHECKPOINT_MAX) &&
           !schway__disc_ecm_test_limits(UINT64_C(1073741824), UINT64_C(1073741824),
                                         ECM_RECORD_MAX + 1u, ECM_CHECKPOINT_MAX) &&
           !schway__disc_ecm_test_limits(UINT64_C(1073741824), UINT64_C(1073741824),
                                         ECM_RECORD_MAX, ECM_CHECKPOINT_MAX + 1u);
}

static int sample_image(schway_disc_image *image, uint8_t first[2352],
                        uint8_t last[2352], uint32_t *leadout)
{
    const schway_disc *disc = schway_disc_image_disc(image);
    schway_toc toc;
    uint8_t subcode[96];
    if (disc == NULL || disc->read_sector == NULL || disc->toc == NULL) return 0;
    memset(&toc, 0, sizeof toc);
    toc.size = (uint32_t)sizeof toc;
    if (disc->toc(disc->user, &toc) == 0u || toc.track_count != 1u ||
        toc.tracks[0].leadout < 16u) return 0;
    *leadout = toc.tracks[0].leadout;
    return disc->read_sector(disc->user, 0u, first, subcode) != 0u &&
           disc->read_sector(disc->user, *leadout - 1u, last, subcode) != 0u;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static schway_disc_image *baseline;
    static char input_path[1024];
    static int initialized;
    uint8_t before_first[2352], before_last[2352];
    uint8_t after_first[2352], after_last[2352];
    uint32_t before_leadout = 0u, after_leadout = 0u;
    schway_disc_image *image = NULL;
    schway__disc_ecm *reader = NULL;
    schway_disc_image_error error;
    schway_disc_image_status dispatched, direct;
    if (size > ECM_INPUT_MAX || (size != 0u && data == NULL) || !limits_are_bounded()) {
        if (!limits_are_bounded()) broken("ECM count/index limits changed");
        return 0;
    }
    if (initialized == 0) {
        int n = snprintf(input_path, sizeof input_path, "%s/input.ecm", SCHWAY_FUZZ_WORK_DIR);
        if (n < 0 || (size_t)n >= sizeof input_path ||
            schway_disc_image_open(SCHWAY_FUZZ_ECM_LITERAL, &baseline, &error) !=
                SCHWAY_DISC_IMAGE_OK || baseline == NULL ||
            !sample_image(baseline, before_first, before_last, &before_leadout)) {
            broken("owned literal baseline could not be opened");
        }
        initialized = 1;
    }
    if (!sample_image(baseline, before_first, before_last, &before_leadout) ||
        !write_file(input_path, data, size)) broken("could not preserve baseline or write input");
    dispatched = schway_disc_image_open(input_path, &image, &error);
    direct = schway__disc_ecm_open(input_path, &reader, &error);
    if ((dispatched == SCHWAY_DISC_IMAGE_OK) != (direct == SCHWAY_DISC_IMAGE_OK) ||
        (dispatched == SCHWAY_DISC_IMAGE_OK) != (image != NULL) ||
        (direct == SCHWAY_DISC_IMAGE_OK) != (reader != NULL)) {
        schway_disc_image_close(image);
        schway__disc_ecm_close(reader);
        broken("dispatcher/direct open or failure-atomic output state diverged");
    }
#ifdef SCHWAY_FUZZ_REPLAY
    int seed = replay_seed(data, size);
    static const schway_disc_image_status expected[ECM_SEED_COUNT] = {
        SCHWAY_DISC_IMAGE_OK, SCHWAY_DISC_IMAGE_OK, SCHWAY_DISC_IMAGE_BAD_IMAGE,
        SCHWAY_DISC_IMAGE_BAD_IMAGE, SCHWAY_DISC_IMAGE_LIMIT, SCHWAY_DISC_IMAGE_LIMIT,
        SCHWAY_DISC_IMAGE_BAD_IMAGE, SCHWAY_DISC_IMAGE_BAD_IMAGE,
        SCHWAY_DISC_IMAGE_BAD_IMAGE, SCHWAY_DISC_IMAGE_BAD_IMAGE,
    };
    if (seed >= 0 && dispatched != expected[seed]) {
        fprintf(stderr, "fuzz.regress.ecm seed %d (%s) status %u expected %u\n",
                seed, ecm_seed_names[seed], (unsigned int)dispatched,
                (unsigned int)expected[seed]);
        schway_disc_image_close(image);
        schway__disc_ecm_close(reader);
        broken("named ECM seed changed its expected parser result");
    }
#endif
    if (dispatched == SCHWAY_DISC_IMAGE_OK) {
        uint64_t decoded = 0u;
        schway__disc_ecm_read_metrics metrics;
        uint8_t byte;
        uint8_t crossing[32];
        uint8_t sector[2352], subcode[96];
        const schway_disc *disc = schway_disc_image_disc(image);
        if (!sample_image(image, after_first, after_last, &after_leadout) ||
            after_leadout != before_leadout ||
            schway__disc_ecm_decoded_size(reader, &decoded) != SCHWAY_DISC_IMAGE_OK ||
            decoded != (uint64_t)after_leadout * 2352u || decoded == 0u ||
            schway__disc_ecm_read_bytes(reader, 0u, &byte, 1u) != SCHWAY_DISC_IMAGE_OK ||
            schway__disc_ecm_read_bytes(reader, decoded - 1u, &byte, 1u) != SCHWAY_DISC_IMAGE_OK ||
            (decoded > 65560u && schway__disc_ecm_read_bytes(
                reader, 65528u, crossing, sizeof crossing) != SCHWAY_DISC_IMAGE_OK)) {
            schway_disc_image_close(image);
            schway__disc_ecm_close(reader);
            broken("accepted ECM failed bounded first/last production reads");
        }
        memset(&metrics, 0, sizeof metrics);
        metrics.size = (uint32_t)sizeof metrics;
        if (schway__disc_ecm_read_metrics_get(reader, &metrics) != SCHWAY_DISC_IMAGE_OK ||
            metrics.decoded_forward_bytes > ECM_READ_MAX ||
            metrics.record_headers > ECM_READ_MAX || metrics.checkpoint_count > ECM_CHECKPOINT_MAX) {
            schway_disc_image_close(image);
            schway__disc_ecm_close(reader);
            broken("ECM read exceeded its work, record, or index bound");
        }
        memset(sector, 0xa5, sizeof sector);
        memset(subcode, 0x5a, sizeof subcode);
        if (disc == NULL || disc->read_sector(disc->user, after_leadout,
                                              sector, subcode) != 0u ||
            sector[0] != 0xa5u || subcode[0] != 0x5au) {
            schway_disc_image_close(image);
            schway__disc_ecm_close(reader);
            broken("ECM out-of-range read changed output bytes");
        }
    }
    if (!sample_image(baseline, after_first, after_last, &after_leadout) ||
        after_leadout != before_leadout || memcmp(before_first, after_first, 2352u) != 0 ||
        memcmp(before_last, after_last, 2352u) != 0) {
        schway_disc_image_close(image);
        schway__disc_ecm_close(reader);
        broken("candidate open changed a previously retained ECM reader");
    }
    schway_disc_image_close(image);
    schway__disc_ecm_close(reader);
    return 0;
}

#define SCHWAY_REPLAY_NAME "fuzz.regress.ecm"
#include "replay_main.h"
