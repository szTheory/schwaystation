/* Deterministic replay and bounded fuzzing for the production PPF/BPS parser. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disc_patch.h"
#include "sha256.h"
#include "seeds.h"

#define FUZZ_SOURCE_SECTORS 17u
#define FUZZ_SECTOR_BYTES 2352u
#define FUZZ_SOURCE_BYTES (FUZZ_SOURCE_SECTORS * FUZZ_SECTOR_BYTES)
#define FUZZ_PPF_BLOCK_OFFSET 0x9320u
#define FUZZ_PPF_BLOCK_BYTES 1024u
#define FUZZ_INPUT_MAX 65536u
#define FUZZ_SEED_BUFFER 2048u

enum fuzz_seed {
    SEED_PPF1 = 0,
    SEED_PPF2,
    SEED_PPF3,
    SEED_BPS,
    SEED_UNSUPPORTED,
    SEED_PPF1_TRUNCATED,
    SEED_PPF1_BOUNDS,
    SEED_PPF2_SIZE,
    SEED_PPF2_BLOCK,
    SEED_PPF3_BLOCK,
    SEED_PPF3_UNDO,
    SEED_BPS_CRC,
    SEED_BPS_GEOMETRY,
    SEED_IDENTITY_MISMATCH,
    FUZZ_SEED_COUNT
};

static const char *const fuzz_seed_names[FUZZ_SEED_COUNT] = {
    "valid_ppf1", "valid_ppf2", "valid_ppf3", "valid_bps", "unsupported_format",
    "ppf1_truncated", "ppf1_bounds", "ppf2_source_size", "ppf2_block_mismatch",
    "ppf3_block_mismatch", "ppf3_truncated_undo", "bps_bad_crc", "bps_geometry",
    "ppf1_identity_mismatch"
};

static uint8_t fuzz_source_bytes[FUZZ_SOURCE_BYTES];
static uint8_t fuzz_source_sha256[SCHWAY_DISC_PATCH_SHA256_BYTES];
static int fuzz_source_ready;

static void broken(const char *what)
{
    fprintf(stderr, "fuzz.regress.ppf: %s\n", what);
    abort();
}

static uint32_t source_read(void *context, uint64_t offset, uint8_t *bytes, size_t count)
{
    const uint8_t *source = (const uint8_t *)context;
    if (source == NULL || bytes == NULL || offset > FUZZ_SOURCE_BYTES ||
        (uint64_t)count > (uint64_t)FUZZ_SOURCE_BYTES - offset) {
        return 0u;
    }
    memcpy(bytes, source + (size_t)offset, count);
    return 1u;
}

static void source_init(void)
{
    if (fuzz_source_ready != 0) return;
    for (size_t i = 0u; i < FUZZ_SOURCE_BYTES; i++) {
        fuzz_source_bytes[i] = (uint8_t)(i * 37u + 11u);
    }
    schway__sha256 digest;
    schway__sha256_init(&digest);
    schway__sha256_update(&digest, fuzz_source_bytes, sizeof fuzz_source_bytes);
    schway__sha256_final(&digest, fuzz_source_sha256);
    fuzz_source_ready = 1;
}

static int source_is_unchanged(void)
{
    for (size_t i = 0u; i < FUZZ_SOURCE_BYTES; i++) {
        if (fuzz_source_bytes[i] != (uint8_t)(i * 37u + 11u)) return 0;
    }
    return 1;
}

static void put_u32(uint8_t *bytes, uint32_t value)
{
    for (size_t i = 0u; i < 4u; i++) bytes[i] = (uint8_t)(value >> (8u * i));
}

static uint32_t test_crc32(const uint8_t *bytes, size_t size)
{
    uint32_t crc = UINT32_MAX;
    for (size_t i = 0u; i < size; i++) {
        crc ^= bytes[i];
        for (unsigned int bit = 0u; bit < 8u; bit++) {
            crc = (crc >> 1u) ^ ((crc & 1u) != 0u ? UINT32_C(0xEDB88320) : 0u);
        }
    }
    return ~crc;
}

static size_t put_bps_number(uint8_t *bytes, size_t at, uint64_t value)
{
    for (;;) {
        uint8_t digit = (uint8_t)(value & 0x7Fu);
        value >>= 7u;
        if (value == 0u) {
            bytes[at++] = (uint8_t)(digit | 0x80u);
            return at;
        }
        bytes[at++] = digit;
        value--;
    }
}

static size_t make_ppf(uint8_t *bytes, unsigned int version, int blockcheck,
                       int undo, uint32_t offset, int bad_block, int truncate_undo)
{
    size_t header = version == 1u ? 56u : (version == 2u ? 1084u : 60u);
    memset(bytes, 0, FUZZ_SEED_BUFFER);
    if (version == 1u) {
        memcpy(bytes, "PPF10", 5u);
        bytes[5] = 0u;
    } else if (version == 2u) {
        memcpy(bytes, "PPF20", 5u);
        bytes[5] = 1u;
        put_u32(bytes + 56u, FUZZ_SOURCE_BYTES);
        if (bad_block) {
            put_u32(bytes + 56u, FUZZ_SOURCE_BYTES + 1u);
        }
        memcpy(bytes + 60u, fuzz_source_bytes + FUZZ_PPF_BLOCK_OFFSET,
               FUZZ_PPF_BLOCK_BYTES);
        if (bad_block) bytes[60u] ^= 1u;
    } else {
        memcpy(bytes, "PPF30", 5u);
        bytes[5] = 2u;
        bytes[56] = 0u;
        bytes[57] = (uint8_t)(blockcheck != 0);
        bytes[58] = (uint8_t)(undo != 0);
        bytes[59] = 0u;
        if (blockcheck != 0) {
            memcpy(bytes + 60u, fuzz_source_bytes + FUZZ_PPF_BLOCK_OFFSET,
                   FUZZ_PPF_BLOCK_BYTES);
            if (bad_block) bytes[60u] ^= 1u;
            header += FUZZ_PPF_BLOCK_BYTES;
        }
    }

    size_t record_header = version == 3u ? 9u : 5u;
    put_u32(bytes + header, offset);
    if (version == 3u) put_u32(bytes + header + 4u, 0u);
    bytes[header + record_header - 1u] = 2u;
    bytes[header + record_header] = 0xA5u;
    bytes[header + record_header + 1u] = 0x5Au;
    size_t size = header + record_header + 2u;
    if (version == 3u && undo != 0) {
        if (truncate_undo == 0) {
            bytes[size++] = 0x11u;
            bytes[size++] = 0x22u;
        }
    }
    return size;
}

static size_t make_bps(uint8_t *bytes, int bad_crc, int change_geometry)
{
    size_t at = 0u;
    const uint64_t target_bytes = change_geometry != 0 ? FUZZ_SOURCE_BYTES - 1u :
                                                          FUZZ_SOURCE_BYTES;
    const uint64_t action = ((target_bytes - 1u) << 2u);
    memcpy(bytes, "BPS1", 4u);
    at = 4u;
    at = put_bps_number(bytes, at, FUZZ_SOURCE_BYTES);
    at = put_bps_number(bytes, at, target_bytes);
    at = put_bps_number(bytes, at, 0u);
    at = put_bps_number(bytes, at, action);
    put_u32(bytes + at, test_crc32(fuzz_source_bytes, FUZZ_SOURCE_BYTES));
    at += 4u;
    put_u32(bytes + at, change_geometry != 0 ? 0u :
            test_crc32(fuzz_source_bytes, FUZZ_SOURCE_BYTES));
    at += 4u;
    put_u32(bytes + at, test_crc32(bytes, at));
    at += 4u;
    if (bad_crc != 0) bytes[at - 1u] ^= 1u;
    return at;
}

uint32_t seeds_count(void)
{
    return FUZZ_SEED_COUNT;
}

const char *seeds_name(uint32_t i)
{
    return i < FUZZ_SEED_COUNT ? fuzz_seed_names[i] : "invalid";
}

size_t seeds_build(uint32_t i, uint8_t *buf)
{
    size_t patch_size = 0u;
    if (buf == NULL || i >= FUZZ_SEED_COUNT) return 0u;
    source_init();
    switch (i) {
    case SEED_PPF1:
        buf[0] = SCHWAY_DISC_PATCH_FORMAT_PPF1;
        buf[1] = 0u;
        patch_size = make_ppf(buf + 2u, 1u, 0, 0, 0x1200u, 0, 0);
        break;
    case SEED_PPF2:
        buf[0] = SCHWAY_DISC_PATCH_FORMAT_PPF2;
        buf[1] = 0u;
        patch_size = make_ppf(buf + 2u, 2u, 0, 0, 0x1200u, 0, 0);
        break;
    case SEED_PPF3:
        buf[0] = SCHWAY_DISC_PATCH_FORMAT_PPF3;
        buf[1] = 0u;
        patch_size = make_ppf(buf + 2u, 3u, 1, 1, 0x1200u, 0, 0);
        break;
    case SEED_BPS:
        buf[0] = SCHWAY_DISC_PATCH_FORMAT_BPS;
        buf[1] = 0u;
        patch_size = make_bps(buf + 2u, 0, 0);
        break;
    case SEED_UNSUPPORTED:
        buf[0] = 0u;
        buf[1] = 0u;
        patch_size = 0u;
        break;
    case SEED_PPF1_TRUNCATED:
        buf[0] = SCHWAY_DISC_PATCH_FORMAT_PPF1;
        buf[1] = 0u;
        memset(buf + 2u, 0, 56u);
        memcpy(buf + 2u, "PPF10", 5u);
        patch_size = 56u;
        break;
    case SEED_PPF1_BOUNDS:
        buf[0] = SCHWAY_DISC_PATCH_FORMAT_PPF1;
        buf[1] = 0u;
        patch_size = make_ppf(buf + 2u, 1u, 0, 0, FUZZ_SOURCE_BYTES - 1u, 0, 0);
        break;
    case SEED_PPF2_SIZE:
        buf[0] = SCHWAY_DISC_PATCH_FORMAT_PPF2;
        buf[1] = 0u;
        patch_size = make_ppf(buf + 2u, 2u, 0, 0, 0x1200u, 1, 0);
        break;
    case SEED_PPF2_BLOCK:
        buf[0] = SCHWAY_DISC_PATCH_FORMAT_PPF2;
        buf[1] = 0u;
        patch_size = make_ppf(buf + 2u, 2u, 0, 0, 0x1200u, 1, 0);
        put_u32(buf + 58u, FUZZ_SOURCE_BYTES);
        break;
    case SEED_PPF3_BLOCK:
        buf[0] = SCHWAY_DISC_PATCH_FORMAT_PPF3;
        buf[1] = 0u;
        patch_size = make_ppf(buf + 2u, 3u, 1, 0, 0x1200u, 1, 0);
        break;
    case SEED_PPF3_UNDO:
        buf[0] = SCHWAY_DISC_PATCH_FORMAT_PPF3;
        buf[1] = 0u;
        patch_size = make_ppf(buf + 2u, 3u, 0, 1, 0x1200u, 0, 1);
        break;
    case SEED_BPS_CRC:
        buf[0] = SCHWAY_DISC_PATCH_FORMAT_BPS;
        buf[1] = 0u;
        patch_size = make_bps(buf + 2u, 1, 0);
        break;
    case SEED_BPS_GEOMETRY:
        buf[0] = SCHWAY_DISC_PATCH_FORMAT_BPS;
        buf[1] = 0u;
        patch_size = make_bps(buf + 2u, 0, 1);
        break;
    case SEED_IDENTITY_MISMATCH:
        buf[0] = SCHWAY_DISC_PATCH_FORMAT_PPF1;
        buf[1] = 1u;
        patch_size = make_ppf(buf + 2u, 1u, 0, 0, 0x1200u, 0, 0);
        break;
    default:
        return 0u;
    }
    return patch_size + 2u;
}

static void ppf1_make_baseline(uint8_t patch[FUZZ_SEED_BUFFER])
{
    (void)make_ppf(patch, 1u, 0, 0, 0x1200u, 0, 0);
}

#ifdef SCHWAY_FUZZ_REPLAY
static int find_known_seed(const uint8_t *data, size_t size)
{
    uint8_t seed[FUZZ_SEED_BUFFER + 2u];
    for (uint32_t i = 0u; i < seeds_count(); i++) {
        size_t seed_size = seeds_build(i, seed);
        if (seed_size == size && memcmp(seed, data, size) == 0) return (int)i;
    }
    return -1;
}
#endif

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

static int apply_equal(const schway_disc_patch_candidate *left,
                       const schway_disc_patch_candidate *right)
{
    uint8_t left_bytes[FUZZ_SOURCE_BYTES];
    uint8_t right_bytes[FUZZ_SOURCE_BYTES];
    memcpy(left_bytes, fuzz_source_bytes, sizeof left_bytes);
    memcpy(right_bytes, fuzz_source_bytes, sizeof right_bytes);
    return schway_disc_patch_candidate_apply(left, 0u, left_bytes, sizeof left_bytes) ==
               SCHWAY_DISC_PATCH_OK &&
           schway_disc_patch_candidate_apply(right, 0u, right_bytes, sizeof right_bytes) ==
               SCHWAY_DISC_PATCH_OK &&
           memcmp(left_bytes, right_bytes, sizeof left_bytes) == 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    uint8_t baseline_patch[FUZZ_SEED_BUFFER];
    uint8_t expected_sha[SCHWAY_DISC_PATCH_SHA256_BYTES];
    uint8_t output_a[FUZZ_SOURCE_BYTES];
    uint8_t output_b[FUZZ_SOURCE_BYTES];
    schway_disc_patch_source source;
    schway_disc_patch_candidate *prior = NULL;
    schway_disc_patch_candidate *first = NULL;
    schway_disc_patch_candidate *second = NULL;
    schway_disc_patch_report report_a;
    schway_disc_patch_report report_b;
    schway_disc_patch_error error_a;
    schway_disc_patch_error error_b;
    if (size > FUZZ_INPUT_MAX) return 0;
    source_init();
    source.size = (uint32_t)sizeof source;
    source.sector_bytes = FUZZ_SECTOR_BYTES;
    source.byte_count = FUZZ_SOURCE_BYTES;
    source.read = source_read;
    source.user = fuzz_source_bytes;
    memcpy(expected_sha, fuzz_source_sha256, sizeof expected_sha);

    ppf1_make_baseline(baseline_patch);
    if (schway_disc_patch_load(&source, SCHWAY_DISC_PATCH_FORMAT_PPF1, baseline_patch,
                               63u, expected_sha, &prior, NULL, NULL) != SCHWAY_DISC_PATCH_OK ||
        prior == NULL) {
        broken("could not create immutable prior candidate");
    }
    memset(&report_a, 0, sizeof report_a);
    memset(&report_b, 0, sizeof report_b);
    memset(&error_a, 0, sizeof error_a);
    memset(&error_b, 0, sizeof error_b);
    report_a.size = (uint32_t)sizeof report_a;
    report_b.size = (uint32_t)sizeof report_b;
    error_a.size = (uint32_t)sizeof error_a;
    error_b.size = (uint32_t)sizeof error_b;

    uint32_t format = size == 0u ? 0u : data[0];
    uint8_t flags = size < 2u ? 0u : data[1];
    const uint8_t *patch = size < 2u ? NULL : data + 2u;
    size_t patch_size = size < 2u ? 0u : size - 2u;
    if ((flags & 1u) != 0u) expected_sha[0] ^= 0x80u;
    schway_disc_patch_status status_a = schway_disc_patch_load(
        &source, (schway_disc_patch_format)format, patch, patch_size, expected_sha,
        &first, &report_a, &error_a);
    schway_disc_patch_status status_b = schway_disc_patch_load(
        &source, (schway_disc_patch_format)format, patch, patch_size, expected_sha,
        &second, &report_b, &error_b);
    if (status_a != status_b || memcmp(&report_a, &report_b, sizeof report_a) != 0 ||
        memcmp(&error_a, &error_b, sizeof error_a) != 0) {
        broken("repeated parse was not deterministic");
    }
    if (status_a == SCHWAY_DISC_PATCH_OK) {
        if (first == NULL || second == NULL ||
            schway_disc_patch_candidate_matches_source(first, &source) == 0u ||
            schway_disc_patch_candidate_matches_source(second, &source) == 0u ||
            !apply_equal(first, second)) {
            broken("accepted candidates did not match the source and deterministic output");
        }
        memcpy(output_a, fuzz_source_bytes, sizeof output_a);
        memcpy(output_b, fuzz_source_bytes, sizeof output_b);
        if (schway_disc_patch_candidate_apply(first, 0u, output_a, sizeof output_a) !=
                SCHWAY_DISC_PATCH_OK ||
            schway_disc_patch_candidate_apply(second, 0u, output_b, sizeof output_b) !=
                SCHWAY_DISC_PATCH_OK || memcmp(output_a, output_b, sizeof output_a) != 0) {
            broken("accepted candidate changed bytes nondeterministically");
        }
    } else if (first != NULL || second != NULL) {
        broken("rejected parse exposed a candidate");
    }
    if (!source_is_unchanged() || !apply_equal(prior, prior)) {
        broken("rejected or accepted parse changed the source or prior candidate");
    }
#ifdef SCHWAY_FUZZ_REPLAY
    const int seed = find_known_seed(data, size);
    if ((seed >= SEED_PPF1 && seed <= SEED_BPS && status_a != SCHWAY_DISC_PATCH_OK) ||
        (seed >= SEED_UNSUPPORTED && status_a == SCHWAY_DISC_PATCH_OK)) {
        fprintf(stderr, "seed %s returned status %u\n",
                seed >= 0 ? seeds_name((uint32_t)seed) : "unknown",
                (unsigned int)status_a);
        broken("generated regression seed changed its acceptance result");
    }
#endif
    schway_disc_patch_candidate_destroy(second);
    schway_disc_patch_candidate_destroy(first);
    schway_disc_patch_candidate_destroy(prior);
    return 0;
}

#ifdef SCHWAY_FUZZ_REPLAY
#define SCHWAY_REPLAY_NAME "fuzz.regress.ppf"
#include "replay_main.h"
#endif
