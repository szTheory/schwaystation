/* PPF tests generate all input media under the build directory. */
#include "disc_image.h"
#include "sha256.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../test_open.h"

#define PATCH_SECTOR_BYTES 2352u
#define PATCH_SECTOR_COUNT 17u
#define PATCH_SOURCE_BYTES ((size_t)PATCH_SECTOR_BYTES * PATCH_SECTOR_COUNT)
#define PATCH_PPF_BLOCK_OFFSET 0x9320u
#define PATCH_PPF_BLOCK_BYTES 1024u
#define PATCH_PPF2_HEADER_BYTES (60u + PATCH_PPF_BLOCK_BYTES)
#define PATCH_BPS_TARGET_BYTES PATCH_SOURCE_BYTES
#define PATCH_BPS_LARGE_BYTES (PATCH_SECTOR_BYTES * 446u)

static const char *test_case;

#define CHECK(condition, message) do { \
    if (!(condition)) { \
        fprintf(stderr, "ASSERT %s: %s\n", test_case, (message)); \
        result = 1; \
        goto cleanup; \
    } \
} while (0)

static int make_path(char path[1024], const char *root, const char *name)
{
    int size = snprintf(path, 1024u, "%s/%s", root, name);
    return size >= 0 && (size_t)size < 1024u;
}

static uint8_t *make_raw_source(void)
{
    uint8_t *bytes = (uint8_t *)malloc(PATCH_SOURCE_BYTES);
    if (bytes == NULL) return NULL;
    for (size_t i = 0u; i < PATCH_SOURCE_BYTES; i++) {
        bytes[i] = (uint8_t)(i * 37u + 11u);
    }
    return bytes;
}

static int write_bytes(const char *path, const uint8_t *bytes, size_t size)
{
    FILE *file = schway__test_open_file(path, "wb");
    if (file == NULL) return 0;
    if (fwrite(bytes, 1u, size, file) != size) {
        (void)fclose(file);
        return 0;
    }
    return fclose(file) == 0;
}

static int write_text(const char *path, const char *text)
{
    FILE *file = schway__test_open_file(path, "wb");
    size_t size = strlen(text);
    if (file == NULL) return 0;
    if (fwrite(text, 1u, size, file) != size) {
        (void)fclose(file);
        return 0;
    }
    return fclose(file) == 0;
}

static uint32_t bps_test_crc32(const uint8_t *bytes, size_t size)
{
    uint32_t crc = UINT32_MAX;
    for (size_t i = 0u; i < size; i++) {
        crc ^= bytes[i];
        for (unsigned int bit = 0u; bit < 8u; bit++)
            crc = (crc >> 1) ^ ((crc & 1u) != 0u ? 0xedb88320u : 0u);
    }
    return ~crc;
}

static int bps_test_number(uint8_t bytes[512], size_t *size, uint64_t value)
{
    for (;;) {
        uint8_t part = (uint8_t)(value & 0x7fu);
        value >>= 7;
        if (*size >= 512u) return 0;
        bytes[(*size)++] = value == 0u ? (uint8_t)(part | 0x80u) : part;
        if (value == 0u) return 1;
        value--;
    }
}

static int bps_test_action(uint8_t bytes[512], size_t *size, uint8_t action, uint64_t length)
{
    return length != 0u && length <= (UINT64_MAX >> 2) &&
        bps_test_number(bytes, size, ((length - 1u) << 2) | action);
}

static int bps_test_relative(uint8_t bytes[512], size_t *size, int64_t offset)
{
    uint64_t magnitude = offset < 0 ? (uint64_t)(-(offset + 1)) + 1u : (uint64_t)offset;
    return magnitude <= (UINT64_MAX >> 1) &&
        bps_test_number(bytes, size, (magnitude << 1) | (offset < 0 ? 1u : 0u));
}

static void bps_test_u32(uint8_t bytes[512], size_t *size, uint32_t value)
{
    for (unsigned int shift = 0u; shift < 32u; shift += 8u)
        bytes[(*size)++] = (uint8_t)(value >> shift);
}

static void bps_test_finish(uint8_t patch[512], size_t *size, uint32_t source_crc, uint32_t target_crc);

static size_t build_bps_sample(uint8_t patch[512], const uint8_t *source, uint8_t expected[PATCH_BPS_TARGET_BYTES])
{
    static const uint8_t first[] = {0xa1u, 0xb2u};
    static const uint8_t last[] = {0xe1u, 0xe2u, 0xe3u, 0xe4u};
    static const uint8_t unused_literal[] = {0u};
    size_t size = 4u;
    size_t output = 0u;
#define ADD_BPS_ACTION(action_, length_, relative_, literal_) do { \
    if (!bps_test_action(patch, &size, (action_), (length_)) || \
        ((action_) >= 2u && !bps_test_relative(patch, &size, (relative_)))) return 0u; \
    if ((action_) == 1u) { memcpy(patch + size, (literal_), (length_)); size += (length_); } \
} while (0)
    memcpy(patch, "BPS1", 4u);
    if (!bps_test_number(patch, &size, PATCH_SOURCE_BYTES) ||
        !bps_test_number(patch, &size, PATCH_BPS_TARGET_BYTES) ||
        !bps_test_number(patch, &size, 0u)) return 0u;
    ADD_BPS_ACTION(0u, 2351u, 0, unused_literal);
    ADD_BPS_ACTION(1u, 2u, 0, first);
    ADD_BPS_ACTION(2u, 3u, 50, unused_literal);
    ADD_BPS_ACTION(2u, 2u, -21, unused_literal);
    ADD_BPS_ACTION(3u, 16u, 2348, unused_literal);
    output = 2351u + 23u;
    ADD_BPS_ACTION(0u, PATCH_SOURCE_BYTES - 40u - output, 0, unused_literal);
    ADD_BPS_ACTION(1u, 4u, 0, last);
    ADD_BPS_ACTION(2u, 4u, 266, unused_literal);
    ADD_BPS_ACTION(3u, 32u, 37580, unused_literal);
    memcpy(expected, source, PATCH_BPS_TARGET_BYTES);
    expected[2351u] = first[0];
    expected[2352u] = first[1];
    memcpy(expected + 2353u, source + 50u, 3u);
    memcpy(expected + 2356u, source + 32u, 2u);
    for (size_t i = 0u; i < 16u; i++) expected[2358u + i] = expected[2348u + i];
    memcpy(expected + PATCH_SOURCE_BYTES - 40u, last, sizeof last);
    memcpy(expected + PATCH_SOURCE_BYTES - 36u, source + 300u, 4u);
    for (size_t i = 0u; i < 32u; i++) expected[PATCH_SOURCE_BYTES - 32u + i] =
        expected[PATCH_SOURCE_BYTES - 40u + i];
    bps_test_finish(patch, &size, bps_test_crc32(source, PATCH_SOURCE_BYTES),
                    bps_test_crc32(expected, PATCH_BPS_TARGET_BYTES));
#undef ADD_BPS_ACTION
    return size;
}

static int bps_test_header(uint8_t patch[512], size_t *size,
                           uint64_t source_size, uint64_t target_size,
                           uint64_t metadata_size)
{
    *size = 4u;
    memcpy(patch, "BPS1", 4u);
    return bps_test_number(patch, size, source_size) &&
           bps_test_number(patch, size, target_size) &&
           bps_test_number(patch, size, metadata_size);
}

static void bps_test_finish(uint8_t patch[512], size_t *size, uint32_t source_crc, uint32_t target_crc)
{
    bps_test_u32(patch, size, source_crc);
    bps_test_u32(patch, size, target_crc);
    bps_test_u32(patch, size, bps_test_crc32(patch, *size));
}

static size_t build_bps_copy_error(uint8_t patch[512], const uint8_t *source,
                                   uint8_t action, int prefix, int64_t relative)
{
    size_t size;
    uint32_t crc = bps_test_crc32(source, PATCH_SOURCE_BYTES);
    if (!bps_test_header(patch, &size, PATCH_SOURCE_BYTES, PATCH_SOURCE_BYTES, 0u)) return 0u;
    if (prefix && !bps_test_action(patch, &size, 0u, 1u)) return 0u;
    if (!bps_test_action(patch, &size, action, 1u) ||
        !bps_test_relative(patch, &size, relative)) return 0u;
    bps_test_finish(patch, &size, crc, crc);
    return size;
}

static int open_raw_cue(const char *root, const char *cue_name,
                        const char *source_name, const char *mode, const char *extra_line,
                        schway_disc_image **out)
{
    char cue_path[1024];
    char source_path[1024];
    char cue[512];
    schway_disc_image_error error;
    if (!make_path(cue_path, root, cue_name) ||
        !make_path(source_path, root, source_name)) return 0;
    (void)source_path;
    int size = snprintf(cue, sizeof cue,
                        "FILE \"%s\" BINARY\nTRACK 01 %s\n%s"
                        "INDEX 01 00:00:00\n",
                        source_name, mode, extra_line != NULL ? extra_line : "");
    if (size < 0 || (size_t)size >= sizeof cue || !write_text(cue_path, cue)) return 0;
    return schway_disc_image_open(cue_path, out, &error) == SCHWAY_DISC_IMAGE_OK;
}

static size_t add_ppf1_record(uint8_t *patch, size_t position, uint32_t offset,
                              const uint8_t *bytes, uint8_t count)
{
    patch[position++] = (uint8_t)offset;
    patch[position++] = (uint8_t)(offset >> 8);
    patch[position++] = (uint8_t)(offset >> 16);
    patch[position++] = (uint8_t)(offset >> 24);
    patch[position++] = count;
    memcpy(patch + position, bytes, count);
    return position + count;
}

static size_t begin_ppf1(uint8_t *patch, size_t capacity)
{
    if (capacity < 56u) return 0u;
    memcpy(patch, "PPF10", 5u);
    patch[5] = 0u;
    memset(patch + 6u, ' ', 50u);
    return 56u;
}

static void write_u32_le(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}

static void bps_test_reseal(uint8_t *patch, size_t patch_size)
{
    write_u32_le(patch + patch_size - 4u,
                 bps_test_crc32(patch, patch_size - 4u));
}

static void write_u64_le(uint8_t *bytes, uint64_t value)
{
    for (size_t i = 0u; i < 8u; i++) bytes[i] = (uint8_t)(value >> (8u * i));
}

static size_t begin_ppf2(uint8_t *patch, size_t capacity,
                         const uint8_t *source, size_t source_size)
{
    if (capacity < PATCH_PPF2_HEADER_BYTES ||
        source_size < PATCH_PPF_BLOCK_OFFSET + PATCH_PPF_BLOCK_BYTES ||
        source_size > UINT32_MAX) return 0u;
    memcpy(patch, "PPF20", 5u);
    patch[5] = 1u;
    memset(patch + 6u, ' ', 50u);
    write_u32_le(patch + 56u, (uint32_t)source_size);
    memcpy(patch + 60u, source + PATCH_PPF_BLOCK_OFFSET,
           PATCH_PPF_BLOCK_BYTES);
    return PATCH_PPF2_HEADER_BYTES;
}

static size_t begin_ppf3(uint8_t *patch, size_t capacity,
                         const uint8_t *source, uint8_t blockcheck,
                         uint8_t undo)
{
    size_t header_bytes = blockcheck != 0u ? 1084u : 60u;
    if (capacity < header_bytes ||
        (blockcheck != 0u &&
         PATCH_SOURCE_BYTES < PATCH_PPF_BLOCK_OFFSET + PATCH_PPF_BLOCK_BYTES)) {
        return 0u;
    }
    memcpy(patch, "PPF30", 5u);
    patch[5] = 2u;
    memset(patch + 6u, ' ', 50u);
    patch[56] = 0u;
    patch[57] = blockcheck;
    patch[58] = undo;
    patch[59] = 0u;
    if (blockcheck != 0u) {
        memcpy(patch + 60u, source + PATCH_PPF_BLOCK_OFFSET,
               PATCH_PPF_BLOCK_BYTES);
    }
    return header_bytes;
}

static size_t add_ppf3_record(uint8_t *patch, size_t position, uint64_t offset,
                              const uint8_t *bytes, uint8_t count,
                              const uint8_t *undo, uint8_t has_undo)
{
    write_u64_le(patch + position, offset);
    position += 8u;
    patch[position++] = count;
    memcpy(patch + position, bytes, count);
    position += count;
    if (has_undo != 0u) {
        memcpy(patch + position, undo, count);
        position += count;
    }
    return position;
}

static size_t append_ppf3_file_id(uint8_t *patch, size_t position,
                                  size_t capacity, uint16_t description_bytes,
                                  uint16_t declared_bytes)
{
    static const uint8_t begin[] = "@BEGIN_FILE_ID.DIZ";
    static const uint8_t end[] = "@END_FILE_ID.DIZ";
    size_t required = sizeof begin - 1u + (size_t)description_bytes +
                      sizeof end - 1u + 2u;
    if (position > capacity || required > capacity - position) return 0u;
    memcpy(patch + position, begin, sizeof begin - 1u);
    position += sizeof begin - 1u;
    memset(patch + position, 'D', description_bytes);
    position += description_bytes;
    memcpy(patch + position, end, sizeof end - 1u);
    position += sizeof end - 1u;
    patch[position++] = (uint8_t)declared_bytes;
    patch[position++] = (uint8_t)(declared_bytes >> 8);
    return position;
}

static size_t make_ppf1(uint8_t *patch, size_t capacity,
                        const uint8_t *source, size_t source_size,
                        uint8_t expected[PATCH_SOURCE_BYTES])
{
    static const uint8_t at_zero[] = {0xa1u};
    static const uint8_t across[] = {0xb1u, 0xb2u, 0xb3u, 0xb4u};
    static const uint8_t final_byte[] = {0xc1u};
    size_t position = 56u;
    if (capacity < 128u || source_size != PATCH_SOURCE_BYTES) return 0u;
    memcpy(expected, source, source_size);
    memcpy(patch, "PPF10", 5u);
    patch[5] = 0u;
    memset(patch + 6u, ' ', 50u);
    position = add_ppf1_record(patch, position, 0u, at_zero, sizeof at_zero);
    memcpy(expected, at_zero, sizeof at_zero);
    position = add_ppf1_record(patch, position, PATCH_SECTOR_BYTES - 1u,
                               across, sizeof across);
    memcpy(expected + PATCH_SECTOR_BYTES - 1u, across, sizeof across);
    position = add_ppf1_record(patch, position,
                               (uint32_t)(source_size - 1u), final_byte,
                               sizeof final_byte);
    memcpy(expected + source_size - 1u, final_byte, sizeof final_byte);
    return position;
}

static void digest(const uint8_t *bytes, size_t size, uint8_t out[32])
{
    schway__sha256 hash;
    schway__sha256_init(&hash);
    schway__sha256_update(&hash, bytes, size);
    schway__sha256_final(&hash, out);
}

static int read_all_sectors(const schway_disc_image *image,
                            uint8_t out[PATCH_SOURCE_BYTES])
{
    const schway_disc *disc = schway_disc_image_disc(image);
    uint8_t subcode[96];
    if (disc == NULL || disc->read_sector == NULL) return 0;
    for (uint32_t lba = 0u; lba < PATCH_SECTOR_COUNT; lba++) {
        if (disc->read_sector(disc->user, lba,
                              out + (size_t)lba * PATCH_SECTOR_BYTES,
                              subcode) == 0u) return 0;
    }
    return 1;
}

static int reject_bps_preserving_overlay(
    const schway_disc_patch_source *source, const uint8_t *patch,
    size_t patch_size, schway_disc_patch_status expected_status,
    const schway_disc_image *image, const uint8_t expected[PATCH_SOURCE_BYTES])
{
    schway_disc_patch_candidate *candidate = NULL;
    schway_disc_patch_report report;
    schway_disc_patch_error error;
    uint8_t actual[PATCH_SOURCE_BYTES];
    schway_disc_patch_status status = schway_disc_patch_load(source,
        SCHWAY_DISC_PATCH_FORMAT_BPS, patch, patch_size, NULL, &candidate,
        &report, &error);
    int rejected = status == expected_status && candidate == NULL;
    schway_disc_patch_candidate_destroy(candidate);
    return rejected && read_all_sectors(image, actual) &&
           memcmp(actual, expected, PATCH_SOURCE_BYTES) == 0;
}

static int run_bps_rejections(const schway_disc_patch_source *source,
                              schway_disc_image *image, const uint8_t *raw,
                              const uint8_t *valid_patch, size_t valid_size,
                              const uint8_t expected[PATCH_SOURCE_BYTES])
{
    static const struct { size_t offset; schway_disc_patch_status status; }
        crc_cases[] = {{12u, SCHWAY_DISC_PATCH_BAD_IDENTITY},
                       {8u, SCHWAY_DISC_PATCH_BAD_PATCH},
                       {4u, SCHWAY_DISC_PATCH_BAD_PATCH}};
    static const struct {
        uint8_t action;
        int prefix;
        int64_t relative;
    } cursor_cases[] = {{2u, 0, PATCH_SOURCE_BYTES}, {2u, 0, -1}, {3u, 0, 0}, {2u, 1, -1}};
    uint8_t patch[512];
    uint8_t source_sha[32];
    uint8_t wrong_sha[32];
    uint8_t actual[PATCH_SOURCE_BYTES];
    schway_disc_patch_candidate *candidate = NULL;
    schway_disc_patch_report report;
    schway_disc_patch_error error;
    size_t size;
    uint32_t crc = bps_test_crc32(raw, PATCH_SOURCE_BYTES);
#define REJECT_BPS(status_) do { \
    if (!reject_bps_preserving_overlay(source, patch, size, (status_), image, expected)) return 0; \
} while (0)
#define MUTATE_VALID_BPS(offset_, status_) do { \
    memcpy(patch, valid_patch, valid_size); patch[(offset_)] ^= 1u; \
    bps_test_reseal(patch, valid_size); size = valid_size; REJECT_BPS(status_); \
} while (0)
    for (size_t i = 0u; i < sizeof crc_cases / sizeof crc_cases[0]; i++) {
        memcpy(patch, valid_patch, valid_size);
        patch[valid_size - crc_cases[i].offset] ^= 1u;
        if (i != 2u) bps_test_reseal(patch, valid_size);
        size = valid_size;
        REJECT_BPS(crc_cases[i].status);
    }
    MUTATE_VALID_BPS(7u, SCHWAY_DISC_PATCH_BAD_PATCH);
    MUTATE_VALID_BPS(4u, SCHWAY_DISC_PATCH_BAD_IDENTITY);
    if (!bps_test_header(patch, &size, PATCH_SOURCE_BYTES, PATCH_SOURCE_BYTES, 0u)) return 0;
    memset(patch + size, 0, 11u); size += 11u;
    REJECT_BPS(SCHWAY_DISC_PATCH_BAD_PATCH);
    memcpy(patch, "BPS1", 4u); size = 14u; memset(patch + 4u, 0, 10u);
    bps_test_finish(patch, &size, crc, crc);
    REJECT_BPS(SCHWAY_DISC_PATCH_BAD_PATCH);
    if (!bps_test_header(patch, &size, PATCH_SOURCE_BYTES, PATCH_SOURCE_BYTES, 1024u * 1024u + 1u)) return 0;
    bps_test_finish(patch, &size, crc, crc);
    REJECT_BPS(SCHWAY_DISC_PATCH_LIMIT);
    for (size_t i = 0u; i < sizeof cursor_cases / sizeof cursor_cases[0]; i++) {
        size = build_bps_copy_error(patch, raw, cursor_cases[i].action,
                                    cursor_cases[i].prefix, cursor_cases[i].relative);
        if (size == 0u) return 0;
        REJECT_BPS(SCHWAY_DISC_PATCH_BOUNDS);
    }
    digest(raw, PATCH_SOURCE_BYTES, source_sha);
    if (schway_disc_patch_load(source, SCHWAY_DISC_PATCH_FORMAT_BPS, valid_patch,
            valid_size, source_sha, &candidate, &report, &error) != SCHWAY_DISC_PATCH_OK ||
        report.checks != (SCHWAY_DISC_PATCH_CHECK_SOURCE_SHA256 |
            SCHWAY_DISC_PATCH_CHECK_BPS_SOURCE_CRC | SCHWAY_DISC_PATCH_CHECK_BPS_TARGET_CRC |
            SCHWAY_DISC_PATCH_CHECK_BPS_PATCH_CRC) ||
        memcmp(report.source_sha256, source_sha, sizeof source_sha) != 0) {
        schway_disc_patch_candidate_destroy(candidate);
        return 0;
    }
    schway_disc_patch_candidate_destroy(candidate);
    candidate = NULL;
    memcpy(wrong_sha, source_sha, sizeof wrong_sha);
    wrong_sha[0] ^= 1u;
    if (schway_disc_patch_load(source, SCHWAY_DISC_PATCH_FORMAT_BPS, valid_patch,
            valid_size, wrong_sha, &candidate, &report, &error) != SCHWAY_DISC_PATCH_BAD_IDENTITY ||
        candidate != NULL) return 0;
    return source->read(source->user, 0u, actual, sizeof actual) != 0u &&
           memcmp(actual, raw, PATCH_SOURCE_BYTES) == 0 &&
           read_all_sectors(image, actual) &&
           memcmp(actual, expected, PATCH_SOURCE_BYTES) == 0;
#undef REJECT_BPS
#undef MUTATE_VALID_BPS
}

static int run_mapping(const char *root)
{
    schway_disc_image *image = NULL;
    schway_disc_image *mode1_image = NULL;
    schway_disc_image *pregap_image = NULL;
    schway_disc_patch_source source;
    schway_disc_patch_error error;
    uint8_t *raw = make_raw_source();
    uint8_t sectors[PATCH_SOURCE_BYTES];
    char source_path[1024];
    int result = 0;
    CHECK(raw != NULL, "allocate generated raw source");
    CHECK(make_path(source_path, root, "mapping.bin"), "build source path");
    CHECK(write_bytes(source_path, raw, PATCH_SOURCE_BYTES), "write generated raw source");
    CHECK(open_raw_cue(root, "mapping.cue", "mapping.bin", "MODE2/2352", NULL, &image),
          "open one-track MODE2/2352 CUE");
    CHECK(schway_disc_image_patch_source(image, &source, &error) == SCHWAY_DISC_PATCH_OK,
          "accept exact one-file raw-track mapping");
    CHECK(source.size == sizeof source && source.sector_bytes == PATCH_SECTOR_BYTES &&
          source.byte_count == PATCH_SOURCE_BYTES && source.read != NULL,
          "report exact raw source length and sector geometry");
    CHECK(read_all_sectors(image, sectors), "read mapped raw sectors");
    CHECK(memcmp(sectors, raw, PATCH_SOURCE_BYTES) == 0,
          "MODE2/2352 sector reads preserve the exact raw source bytes");
    CHECK(open_raw_cue(root, "mode1.cue", "mapping.bin", "MODE1/2352", NULL,
                       &mode1_image), "accept one-file MODE1/2352 CUE");
    CHECK(schway_disc_image_patch_source(mode1_image, &source, &error) ==
              SCHWAY_DISC_PATCH_OK,
          "accept MODE1/2352 as a raw patch source");
    CHECK(read_all_sectors(mode1_image, sectors) &&
          memcmp(sectors, raw, PATCH_SOURCE_BYTES) == 0,
          "MODE1/2352 reads preserve all raw source bytes");
    schway_disc_image_close(mode1_image);
    mode1_image = NULL;
    CHECK(open_raw_cue(root, "pregap.cue", "mapping.bin", "MODE2/2352",
                       "PREGAP 00:00:01\n", &pregap_image), "open CUE with virtual pregap");
    CHECK(schway_disc_image_patch_source(pregap_image, &source, &error) ==
              SCHWAY_DISC_PATCH_UNSUPPORTED,
          "reject virtual-pregap mapping");
cleanup:
    schway_disc_image_close(pregap_image);
    schway_disc_image_close(mode1_image);
    schway_disc_image_close(image);
    free(raw);
    return result;
}

static int run_bps_large_stage(void);

static int run_bps(const char *root)
{
    schway_disc_image *image = NULL;
    schway_disc_patch_source source;
    schway_disc_patch_candidate *candidate = NULL;
    schway_disc_patch_report report;
    schway_disc_patch_error error;
    uint8_t *raw = make_raw_source();
    uint8_t expected[PATCH_BPS_TARGET_BYTES];
    uint8_t actual[PATCH_SOURCE_BYTES];
    uint8_t patch[512];
    char source_path[1024];
    size_t patch_size;
    int result = 0;
    CHECK(raw != NULL, "allocate BPS source");
    CHECK(make_path(source_path, root, "bps.bin"), "build BPS source path");
    CHECK(write_bytes(source_path, raw, PATCH_SOURCE_BYTES), "write BPS source");
    CHECK(open_raw_cue(root, "bps.cue", "bps.bin", "MODE2/2352", NULL, &image), "open supported raw CUE");
    CHECK(schway_disc_image_patch_source(image, &source, &error) == SCHWAY_DISC_PATCH_OK, "bind raw source");
    patch_size = build_bps_sample(patch, raw, expected);
    CHECK(patch_size != 0u, "build independent BPS actions and checksums");
    CHECK(schway_disc_patch_load(&source, SCHWAY_DISC_PATCH_FORMAT_BPS,
          patch, patch_size, NULL, &candidate, &report, &error) == SCHWAY_DISC_PATCH_OK &&
          report.checks == (SCHWAY_DISC_PATCH_CHECK_BPS_SOURCE_CRC |
                            SCHWAY_DISC_PATCH_CHECK_BPS_TARGET_CRC |
                            SCHWAY_DISC_PATCH_CHECK_BPS_PATCH_CRC),
          "validate complete BPS candidate and all native checks");
    CHECK(read_all_sectors(image, actual) && memcmp(actual, raw, PATCH_SOURCE_BYTES) == 0, "not yet visible");
    CHECK(schway_disc_image_patch_replace(image, &candidate, &error) == SCHWAY_DISC_PATCH_OK && !candidate, "replace");
    CHECK(read_all_sectors(image, actual) &&
          memcmp(actual, expected, PATCH_BPS_TARGET_BYTES) == 0,
          "all BPS actions produce the complete expected target");
    CHECK(read_all_sectors(image, actual) &&
          memcmp(actual, expected, PATCH_BPS_TARGET_BYTES) == 0,
          "BPS target remains deterministic across complete sector reads");
    CHECK(run_bps_rejections(&source, image, raw, patch, patch_size, expected), "rejections preserve overlay");
    CHECK(run_bps_large_stage(), "stage large BPS target and clean up after I/O failure");
cleanup:
    schway_disc_patch_candidate_destroy(candidate);
    schway_disc_image_close(image);
    free(raw);
    return result;
}

typedef struct bps_memory_source {
    const uint8_t *bytes;
    uint64_t size;
    uint32_t scanned;
    uint32_t copy_reads;
    uint32_t fail_staging;
} bps_memory_source;

static uint32_t bps_memory_read(void *user, uint64_t offset,
                               uint8_t *bytes, size_t count)
{
    bps_memory_source *source = (bps_memory_source *)user;
    if (offset > source->size || count > source->size - offset ||
        (source->scanned != 0u && source->fail_staging != 0u &&
         source->copy_reads++ == 1u)) return 0u;
    memcpy(bytes, source->bytes + (size_t)offset, count);
    if (offset + count == source->size) source->scanned = 1u;
    return 1u;
}

static int run_bps_large_stage(void)
{
    uint8_t patch[512];
    uint8_t *raw = (uint8_t *)malloc(PATCH_BPS_LARGE_BYTES);
    uint8_t *actual = (uint8_t *)malloc(PATCH_BPS_LARGE_BYTES);
    bps_memory_source memory = {raw, PATCH_BPS_LARGE_BYTES, 0u, 0u, 0u};
    schway_disc_patch_source source = {sizeof source, PATCH_SECTOR_BYTES,
        PATCH_BPS_LARGE_BYTES, bps_memory_read, &memory};
    schway_disc_patch_candidate *candidate = NULL;
    schway_disc_patch_report report;
    schway_disc_patch_error error;
    size_t size;
    int passed = 0;
    if (raw == NULL || actual == NULL) goto cleanup;
    for (size_t i = 0u; i < PATCH_BPS_LARGE_BYTES; i++) raw[i] = (uint8_t)(i * 13u);
    if (!bps_test_header(patch, &size, PATCH_BPS_LARGE_BYTES, PATCH_BPS_LARGE_BYTES, 0u) ||
        !bps_test_action(patch, &size, 0u, PATCH_BPS_LARGE_BYTES)) goto cleanup;
    uint32_t crc = bps_test_crc32(raw, PATCH_BPS_LARGE_BYTES);
    bps_test_finish(patch, &size, crc, crc);
    if (schway_disc_patch_load(&source, SCHWAY_DISC_PATCH_FORMAT_BPS,
            patch, size, NULL, &candidate, &report, &error) != SCHWAY_DISC_PATCH_OK ||
        schway_disc_patch_candidate_apply(candidate, 0u, actual, PATCH_BPS_LARGE_BYTES) != SCHWAY_DISC_PATCH_OK ||
        memcmp(actual, raw, PATCH_BPS_LARGE_BYTES) != 0) goto cleanup;
    schway_disc_patch_candidate_destroy(candidate); candidate = NULL;
    memory.scanned = memory.copy_reads = 0u; memory.fail_staging = 1u;
    passed = schway_disc_patch_load(&source, SCHWAY_DISC_PATCH_FORMAT_BPS,
        patch, size, NULL, &candidate, &report, &error) == SCHWAY_DISC_PATCH_IO &&
        candidate == NULL && memcmp(actual, raw, PATCH_BPS_LARGE_BYTES) == 0;
cleanup:
    schway_disc_patch_candidate_destroy(candidate);
    free(actual);
    free(raw);
    return passed;
}

static int run_ppf1(const char *root)
{
    schway_disc_image *image = NULL;
    schway_disc_patch_source source;
    schway_disc_patch_candidate *candidate = NULL;
    schway_disc_patch_report report;
    schway_disc_patch_error error;
    uint8_t *raw = make_raw_source();
    uint8_t expected[PATCH_SOURCE_BYTES];
    uint8_t actual[PATCH_SOURCE_BYTES];
    uint8_t file_copy[PATCH_SOURCE_BYTES];
    uint8_t patch[128];
    uint8_t wrong_sha[32];
    uint8_t source_sha[32];
    char source_path[1024];
    size_t patch_size = 0u;
    int result = 0;
    CHECK(raw != NULL, "allocate generated raw source");
    CHECK(make_path(source_path, root, "ppf1.bin"), "build source path");
    CHECK(write_bytes(source_path, raw, PATCH_SOURCE_BYTES), "write generated raw source");
    CHECK(open_raw_cue(root, "ppf1.cue", "ppf1.bin", "MODE2/2352", NULL, &image),
          "open supported raw CUE");
    CHECK(schway_disc_image_patch_source(image, &source, &error) == SCHWAY_DISC_PATCH_OK,
          "obtain immutable raw patch source");
    patch_size = make_ppf1(patch, sizeof patch, raw, PATCH_SOURCE_BYTES, expected);
    CHECK(patch_size > 56u, "build PPF1 fixture with boundary records");
    digest(raw, PATCH_SOURCE_BYTES, source_sha);
    CHECK(schway_disc_patch_load(&source, SCHWAY_DISC_PATCH_FORMAT_PPF1,
                                 patch, patch_size, source_sha, &candidate,
                                 &report, &error) == SCHWAY_DISC_PATCH_OK,
          "load and validate PPF1 candidate");
    CHECK(candidate != NULL && report.format == SCHWAY_DISC_PATCH_FORMAT_PPF1 &&
          report.records == 3u &&
          (report.checks & SCHWAY_DISC_PATCH_CHECK_SOURCE_SHA256) != 0u,
          "report PPF1 format, record count and whole-source SHA-256 check");
    CHECK(schway_disc_image_patch_replace(image, &candidate, &error) ==
              SCHWAY_DISC_PATCH_OK && candidate == NULL,
          "attach validated PPF1 candidate");
    CHECK(read_all_sectors(image, actual), "read patched sectors");
    CHECK(memcmp(actual, expected, PATCH_SOURCE_BYTES) == 0,
          "apply byte zero, final byte and cross-sector patch record");
    CHECK(read_all_sectors(image, file_copy) &&
          memcmp(file_copy, expected, PATCH_SOURCE_BYTES) == 0,
          "repeated reads return deterministic patched sectors");
    digest(raw, PATCH_SOURCE_BYTES, wrong_sha);
    wrong_sha[0] ^= 1u;
    CHECK(schway_disc_patch_load(&source, SCHWAY_DISC_PATCH_FORMAT_PPF1,
                                 patch, patch_size, wrong_sha, &candidate,
                                 &report, &error) == SCHWAY_DISC_PATCH_BAD_IDENTITY &&
          candidate == NULL,
          "reject wrong full-source SHA-256 without a candidate");
    CHECK(read_all_sectors(image, actual) &&
          memcmp(actual, expected, PATCH_SOURCE_BYTES) == 0,
          "wrong identity exposes no replacement sectors");
    patch_size = 56u;
    memcpy(patch, "PPF10", 5u);
    patch[5] = 0u;
    memset(patch + 6u, ' ', 50u);
    static const uint8_t valid[] = {0x55u};
    static const uint8_t invalid[] = {0x66u};
    patch_size = add_ppf1_record(patch, patch_size, 0u, valid, sizeof valid);
    patch_size = add_ppf1_record(patch, patch_size,
                                 (uint32_t)PATCH_SOURCE_BYTES, invalid,
                                 sizeof invalid);
    CHECK(schway_disc_patch_load(&source, SCHWAY_DISC_PATCH_FORMAT_PPF1,
                                 patch, patch_size, source_sha, &candidate,
                                 &report, &error) == SCHWAY_DISC_PATCH_BOUNDS &&
          candidate == NULL,
          "reject a late out-of-range record after a valid prefix");
    CHECK(read_all_sectors(image, actual) &&
          memcmp(actual, expected, PATCH_SOURCE_BYTES) == 0,
          "late invalid record exposes no partial overlay");
    FILE *source_file = schway__test_open_file(source_path, "rb");
    CHECK(source_file != NULL, "reopen source after patch reads");
    CHECK(fread(file_copy, 1u, sizeof file_copy, source_file) == sizeof file_copy,
          "read unchanged raw source");
    CHECK(fclose(source_file) == 0, "close source copy");
    CHECK(memcmp(file_copy, raw, PATCH_SOURCE_BYTES) == 0,
          "PPF application never rewrites source image bytes");
cleanup:
    schway_disc_patch_candidate_destroy(candidate);
    schway_disc_image_close(image);
    free(raw);
    return result;
}

static int run_ppf2(const char *root)
{
    static const uint8_t first[] = {0x91u, 0x92u, 0x93u};
    static const uint8_t last[] = {0xe1u};
    schway_disc_image *image = NULL;
    schway_disc_patch_source source;
    schway_disc_patch_candidate *candidate = NULL;
    schway_disc_patch_report report;
    schway_disc_patch_error error;
    uint8_t *raw = make_raw_source();
    uint8_t expected[PATCH_SOURCE_BYTES];
    uint8_t actual[PATCH_SOURCE_BYTES];
    uint8_t patch[1200];
    uint8_t source_sha[32];
    uint8_t wrong_sha[32];
    char source_path[1024];
    size_t patch_size = 0u;
    int result = 0;
    CHECK(raw != NULL, "allocate generated raw source");
    CHECK(make_path(source_path, root, "ppf2.bin"), "build source path");
    CHECK(write_bytes(source_path, raw, PATCH_SOURCE_BYTES), "write generated raw source");
    CHECK(open_raw_cue(root, "ppf2.cue", "ppf2.bin", "MODE2/2352", NULL, &image),
          "open supported raw CUE");
    CHECK(schway_disc_image_patch_source(image, &source, &error) == SCHWAY_DISC_PATCH_OK,
          "obtain immutable raw patch source");
    patch_size = begin_ppf2(patch, sizeof patch, raw, PATCH_SOURCE_BYTES);
    CHECK(patch_size == PATCH_PPF2_HEADER_BYTES, "build PPF2 source checks");
    patch_size = add_ppf1_record(patch, patch_size, PATCH_PPF_BLOCK_OFFSET,
                                 first, (uint8_t)sizeof first);
    patch_size = add_ppf1_record(patch, patch_size,
                                 (uint32_t)(PATCH_SOURCE_BYTES - 1u),
                                 last, (uint8_t)sizeof last);
    memcpy(expected, raw, PATCH_SOURCE_BYTES);
    memcpy(expected + PATCH_PPF_BLOCK_OFFSET, first, sizeof first);
    expected[PATCH_SOURCE_BYTES - 1u] = last[0];
    digest(raw, PATCH_SOURCE_BYTES, source_sha);
    CHECK(schway_disc_patch_load(&source, SCHWAY_DISC_PATCH_FORMAT_PPF2,
                                 patch, patch_size, source_sha, &candidate,
                                 &report, &error) == SCHWAY_DISC_PATCH_OK,
          "load and validate PPF2 candidate");
    CHECK(candidate != NULL && report.format == SCHWAY_DISC_PATCH_FORMAT_PPF2 &&
          report.records == 2u &&
          report.checks == (SCHWAY_DISC_PATCH_CHECK_SOURCE_SHA256 |
                            SCHWAY_DISC_PATCH_CHECK_PPF2_SOURCE_SIZE |
                            SCHWAY_DISC_PATCH_CHECK_PPF2_BLOCK),
          "report PPF2 SHA-256, exact size and block checks");
    CHECK(schway_disc_image_patch_replace(image, &candidate, &error) ==
              SCHWAY_DISC_PATCH_OK && candidate == NULL,
          "attach validated PPF2 candidate");
    CHECK(read_all_sectors(image, actual) &&
          memcmp(actual, expected, PATCH_SOURCE_BYTES) == 0,
          "apply PPF2 32-bit records at source and final-byte boundaries");
    memcpy(wrong_sha, source_sha, sizeof wrong_sha);
    wrong_sha[0] ^= 1u;
    CHECK(schway_disc_patch_load(&source, SCHWAY_DISC_PATCH_FORMAT_PPF2,
                                 patch, patch_size, wrong_sha, &candidate,
                                 &report, &error) == SCHWAY_DISC_PATCH_BAD_IDENTITY &&
          candidate == NULL,
          "reject PPF2 with a wrong whole-source SHA-256");
    CHECK(read_all_sectors(image, actual) &&
          memcmp(actual, expected, PATCH_SOURCE_BYTES) == 0,
          "wrong PPF2 SHA-256 leaves the active overlay unchanged");
cleanup:
    schway_disc_patch_candidate_destroy(candidate);
    schway_disc_image_close(image);
    free(raw);
    return result;
}

static int run_ppf3(const char *root)
{
    static const uint8_t across[] = {0x71u, 0x72u, 0x73u, 0x74u};
    static const uint8_t last[] = {0xe5u};
    uint8_t undo[sizeof across];
    schway_disc_image *image = NULL;
    schway_disc_patch_source source;
    schway_disc_patch_candidate *candidate = NULL;
    schway_disc_patch_report report;
    schway_disc_patch_error error;
    uint8_t *raw = make_raw_source();
    uint8_t expected[PATCH_SOURCE_BYTES];
    uint8_t actual[PATCH_SOURCE_BYTES];
    uint8_t patch[1200];
    uint8_t source_sha[32];
    char source_path[1024];
    size_t patch_size = 0u;
    int result = 0;
    CHECK(raw != NULL, "allocate generated raw source");
    CHECK(make_path(source_path, root, "ppf3.bin"), "build source path");
    CHECK(write_bytes(source_path, raw, PATCH_SOURCE_BYTES), "write generated raw source");
    CHECK(open_raw_cue(root, "ppf3.cue", "ppf3.bin", "MODE2/2352", NULL, &image),
          "open supported raw CUE");
    CHECK(schway_disc_image_patch_source(image, &source, &error) == SCHWAY_DISC_PATCH_OK,
          "obtain immutable raw patch source");
    memcpy(undo, raw + PATCH_SECTOR_BYTES - 1u, sizeof undo);
    patch_size = begin_ppf3(patch, sizeof patch, raw, 1u, 1u);
    CHECK(patch_size == 1084u, "build PPF3 block-check and undo header");
    patch_size = add_ppf3_record(patch, patch_size, PATCH_SECTOR_BYTES - 1u,
                                 across, (uint8_t)sizeof across, undo, 1u);
    memcpy(expected, raw, PATCH_SOURCE_BYTES);
    memcpy(expected + PATCH_SECTOR_BYTES - 1u, across, sizeof across);
    digest(raw, PATCH_SOURCE_BYTES, source_sha);
    CHECK(schway_disc_patch_load(&source, SCHWAY_DISC_PATCH_FORMAT_PPF3,
                                 patch, patch_size, source_sha, &candidate,
                                 &report, &error) == SCHWAY_DISC_PATCH_OK,
          "load and validate PPF3 candidate with undo bytes");
    CHECK(candidate != NULL && report.format == SCHWAY_DISC_PATCH_FORMAT_PPF3 &&
          report.records == 1u &&
          report.checks == (SCHWAY_DISC_PATCH_CHECK_SOURCE_SHA256 |
                            SCHWAY_DISC_PATCH_CHECK_PPF3_BLOCK),
          "report PPF3 SHA-256 and enabled block check");
    CHECK(schway_disc_image_patch_replace(image, &candidate, &error) ==
              SCHWAY_DISC_PATCH_OK && candidate == NULL,
          "attach validated PPF3 candidate");
    CHECK(read_all_sectors(image, actual) &&
          memcmp(actual, expected, PATCH_SOURCE_BYTES) == 0,
          "apply PPF3 64-bit record while consuming but not applying undo bytes");

    patch_size = begin_ppf3(patch, sizeof patch, raw, 0u, 0u);
    CHECK(patch_size == 60u, "build PPF3 header with block check disabled");
    patch_size = add_ppf3_record(patch, patch_size,
                                 (uint64_t)(PATCH_SOURCE_BYTES - 1u),
                                 last, (uint8_t)sizeof last, NULL, 0u);
    memcpy(expected, raw, PATCH_SOURCE_BYTES);
    expected[PATCH_SOURCE_BYTES - 1u] = last[0];
    CHECK(schway_disc_patch_load(&source, SCHWAY_DISC_PATCH_FORMAT_PPF3,
                                 patch, patch_size, source_sha, &candidate,
                                 &report, &error) == SCHWAY_DISC_PATCH_OK,
          "load PPF3 candidate with optional block check disabled");
    CHECK(report.checks == SCHWAY_DISC_PATCH_CHECK_SOURCE_SHA256 &&
          schway_disc_image_patch_replace(image, &candidate, &error) ==
              SCHWAY_DISC_PATCH_OK && candidate == NULL,
          "report only the checks performed for a disabled block check");
    CHECK(read_all_sectors(image, actual) &&
          memcmp(actual, expected, PATCH_SOURCE_BYTES) == 0,
          "apply PPF3 record at the final source byte");
cleanup:
    schway_disc_patch_candidate_destroy(candidate);
    schway_disc_image_close(image);
    free(raw);
    return result;
}

static int reject_preserving_overlay(
    const schway_disc_patch_source *source, const uint8_t *patch,
    size_t patch_size, const uint8_t source_sha[32],
    schway_disc_image *image, const uint8_t expected[PATCH_SOURCE_BYTES],
    schway_disc_patch_status expected_status)
{
    schway_disc_patch_candidate *candidate = NULL;
    schway_disc_patch_report report;
    schway_disc_patch_error error;
    uint8_t actual[PATCH_SOURCE_BYTES];
    schway_disc_patch_status status = schway_disc_patch_load(
        source, (schway_disc_patch_format)(patch[5] == 2u ?
            SCHWAY_DISC_PATCH_FORMAT_PPF3 :
            (memcmp(patch, "PPF20", 5u) == 0 ? SCHWAY_DISC_PATCH_FORMAT_PPF2 :
             SCHWAY_DISC_PATCH_FORMAT_PPF1)),
        patch, patch_size, source_sha, &candidate, &report, &error);
    int rejected = status == expected_status && candidate == NULL;
    schway_disc_patch_candidate_destroy(candidate);
    return rejected && read_all_sectors(image, actual) &&
           memcmp(actual, expected, PATCH_SOURCE_BYTES) == 0;
}

static int run_bounds(const char *root)
{
    static const uint8_t prior_byte[] = {0xa7u};
    static const uint8_t patch_byte[] = {0xb8u};
    schway_disc_image *image = NULL;
    schway_disc_patch_source source;
    schway_disc_patch_candidate *candidate = NULL;
    schway_disc_patch_report report;
    schway_disc_patch_error error;
    uint8_t *raw = make_raw_source();
    uint8_t *large_patch = NULL;
    uint8_t expected[PATCH_SOURCE_BYTES];
    uint8_t file_copy[PATCH_SOURCE_BYTES];
    uint8_t source_sha[32];
    uint8_t wrong_sha[32];
    uint8_t patch[8192];
    char source_path[1024];
    size_t patch_size;
    int result = 0;
#define REJECT_PPF(data_, size_, sha_, status_, message_) \
    CHECK(reject_preserving_overlay(&source, (data_), (size_), (sha_), image, \
          expected, (status_)), (message_))
#define REJECT_PPF3_BYTE(offset_, value_, status_, message_) do { \
    patch_size = begin_ppf3(patch, sizeof patch, raw, 0u, 0u); \
    patch[(offset_)] = (value_); \
    patch_size = add_ppf3_record(patch, patch_size, 200u, patch_byte, 1u, NULL, 0u); \
    REJECT_PPF(patch, patch_size, source_sha, (status_), (message_)); \
} while (0)
#define REJECT_PPF3_RECORD(offset_, data_, count_, sha_, status_, message_) do { \
    patch_size = begin_ppf3(patch, sizeof patch, raw, 0u, 0u); \
    patch_size = add_ppf3_record(patch, patch_size, (offset_), (data_), (count_), NULL, 0u); \
    REJECT_PPF(patch, patch_size, (sha_), (status_), (message_)); \
} while (0)
    CHECK(raw != NULL, "allocate generated raw source");
    CHECK(make_path(source_path, root, "bounds.bin"), "build source path");
    CHECK(write_bytes(source_path, raw, PATCH_SOURCE_BYTES), "write generated raw source");
    CHECK(open_raw_cue(root, "bounds.cue", "bounds.bin", "MODE2/2352", NULL, &image),
          "open supported raw CUE");
    CHECK(schway_disc_image_patch_source(image, &source, &error) == SCHWAY_DISC_PATCH_OK,
          "obtain immutable raw patch source");
    digest(raw, PATCH_SOURCE_BYTES, source_sha);

    patch_size = begin_ppf1(patch, sizeof patch);
    patch_size = add_ppf1_record(patch, patch_size, 100u,
                                 prior_byte, (uint8_t)sizeof prior_byte);
    memcpy(expected, raw, PATCH_SOURCE_BYTES);
    expected[100u] = prior_byte[0];
    CHECK(schway_disc_patch_load(&source, SCHWAY_DISC_PATCH_FORMAT_PPF1,
                                 patch, patch_size, source_sha, &candidate,
                                 &report, &error) == SCHWAY_DISC_PATCH_OK,
          "load initial active overlay");
    CHECK(schway_disc_image_patch_replace(image, &candidate, &error) ==
              SCHWAY_DISC_PATCH_OK && candidate == NULL,
          "attach initial active overlay");

    patch_size = begin_ppf2(patch, sizeof patch, raw, PATCH_SOURCE_BYTES);
    patch_size = add_ppf1_record(patch, patch_size, 200u,
                                 patch_byte, (uint8_t)sizeof patch_byte);
    write_u32_le(patch + 56u, (uint32_t)(PATCH_SOURCE_BYTES - 1u));
    REJECT_PPF(patch, patch_size, source_sha, SCHWAY_DISC_PATCH_BAD_IDENTITY, "reject PPF2 wrong size, keep overlay");
    patch_size = begin_ppf2(patch, sizeof patch, raw, PATCH_SOURCE_BYTES);
    patch_size = add_ppf1_record(patch, patch_size, 200u,
                                 patch_byte, (uint8_t)sizeof patch_byte);
    patch[60u] ^= 1u;
    REJECT_PPF(patch, patch_size, source_sha, SCHWAY_DISC_PATCH_BAD_IDENTITY, "reject PPF2 wrong block");
    patch_size = begin_ppf3(patch, sizeof patch, raw, 1u, 0u);
    patch_size = add_ppf3_record(patch, patch_size, 200u,
                                 patch_byte, 1u, NULL, 0u);
    patch[60u + 17u] ^= 1u;
    REJECT_PPF(patch, patch_size, source_sha, SCHWAY_DISC_PATCH_BAD_IDENTITY, "reject PPF3 wrong block");
    patch_size = begin_ppf3(patch, sizeof patch, raw, 0u, 0u);
    patch[57u] = 1u;
    REJECT_PPF(patch, 1083u, source_sha, SCHWAY_DISC_PATCH_BAD_PATCH, "reject truncated enabled PPF3 block");
    REJECT_PPF3_RECORD(UINT64_MAX, patch_byte, 1u, source_sha, SCHWAY_DISC_PATCH_BOUNDS, "reject PPF3 offset overflow");
    REJECT_PPF3_RECORD(PATCH_SOURCE_BYTES - 1u, patch_byte, 2u, source_sha, SCHWAY_DISC_PATCH_BOUNDS, "source overrun");

    patch_size = begin_ppf3(patch, sizeof patch, raw, 0u, 1u);
    write_u64_le(patch + patch_size, 200u);
    patch[patch_size + 8u] = 1u;
    patch[patch_size + 9u] = patch_byte[0];
    patch_size += 10u;
    REJECT_PPF(patch, patch_size, source_sha, SCHWAY_DISC_PATCH_BAD_PATCH, "reject truncated PPF3 undo bytes");

    patch_size = begin_ppf3(patch, sizeof patch, raw, 0u, 0u);
    patch_size = add_ppf3_record(patch, patch_size, 200u,
                                 patch_byte, 1u, NULL, 0u);
    patch_size = append_ppf3_file_id(patch, patch_size, sizeof patch,
                                     4u, 4u);
    CHECK(patch_size != 0u, "build truncated PPF3 FileID fixture");
    patch_size -= 3u;
    REJECT_PPF(patch, patch_size, source_sha, SCHWAY_DISC_PATCH_BAD_PATCH, "reject truncated PPF3 FileID trailer");
    patch_size = begin_ppf3(patch, sizeof patch, raw, 0u, 0u);
    patch_size = add_ppf3_record(patch, patch_size, 200u,
                                 patch_byte, 1u, NULL, 0u);
    patch_size = append_ppf3_file_id(patch, patch_size, sizeof patch,
                                     4u, 5u);
    REJECT_PPF(patch, patch_size, source_sha, SCHWAY_DISC_PATCH_BAD_PATCH, "reject mismatched PPF3 FileID length");
    patch_size = begin_ppf3(patch, sizeof patch, raw, 0u, 0u);
    patch_size = add_ppf3_record(patch, patch_size, 200u,
                                 patch_byte, 1u, NULL, 0u);
    patch_size = append_ppf3_file_id(patch, patch_size, sizeof patch,
                                     3073u, 3073u);
    REJECT_PPF(patch, patch_size, source_sha, SCHWAY_DISC_PATCH_BAD_PATCH, "reject oversized PPF3 FileID");
    REJECT_PPF3_BYTE(56u, 1u, SCHWAY_DISC_PATCH_UNSUPPORTED, "reject unsupported GI image type");
    REJECT_PPF3_BYTE(56u, 2u, SCHWAY_DISC_PATCH_UNSUPPORTED, "reject unknown PPF3 image type");
    REJECT_PPF3_BYTE(57u, 2u, SCHWAY_DISC_PATCH_BAD_PATCH, "reject unknown PPF3 block-check flag");
    REJECT_PPF3_BYTE(58u, 2u, SCHWAY_DISC_PATCH_BAD_PATCH, "reject unknown PPF3 undo flag");
    REJECT_PPF3_BYTE(59u, 1u, SCHWAY_DISC_PATCH_BAD_PATCH, "reject nonzero PPF3 reserved byte");

    patch_size = begin_ppf2(patch, sizeof patch, raw, PATCH_SOURCE_BYTES);
    write_u32_le(patch + patch_size, 200u);
    patch[patch_size + 4u] = 0u;
    patch_size += 5u;
    REJECT_PPF(patch, patch_size, source_sha, SCHWAY_DISC_PATCH_BAD_PATCH, "reject zero-length PPF2 record");
    patch_size = begin_ppf3(patch, sizeof patch, raw, 0u, 0u);
    write_u64_le(patch + patch_size, 200u);
    patch[patch_size + 8u] = 0u;
    patch_size += 9u;
    REJECT_PPF(patch, patch_size, source_sha, SCHWAY_DISC_PATCH_BAD_PATCH, "reject zero-length PPF3 record");
    patch_size = begin_ppf3(patch, sizeof patch, raw, 0u, 0u);
    patch_size = add_ppf3_record(patch, patch_size, 200u,
                                 patch_byte, 1u, NULL, 0u);
    write_u64_le(patch + patch_size, PATCH_SOURCE_BYTES - 1u);
    patch[patch_size + 8u] = 2u;
    patch[patch_size + 9u] = 0x11u;
    patch[patch_size + 10u] = 0x22u;
    patch_size += 11u;
    REJECT_PPF(patch, patch_size, source_sha, SCHWAY_DISC_PATCH_BOUNDS, "reject invalid late PPF3 record");

    memcpy(wrong_sha, source_sha, sizeof wrong_sha);
    wrong_sha[0] ^= 1u;
    REJECT_PPF3_RECORD(200u, patch_byte, 1u, wrong_sha, SCHWAY_DISC_PATCH_BAD_IDENTITY, "reject wrong source SHA-256");

    patch_size = begin_ppf3(patch, sizeof patch, raw, 0u, 0u);
    patch_size = add_ppf3_record(patch, patch_size, 200u,
                                 patch_byte, 1u, NULL, 0u);
    patch_size = append_ppf3_file_id(patch, patch_size, sizeof patch,
                                     3072u, 3072u);
    CHECK(patch_size != 0u &&
          schway_disc_patch_load(&source, SCHWAY_DISC_PATCH_FORMAT_PPF3,
                                 patch, patch_size, source_sha, &candidate,
                                 &report, &error) == SCHWAY_DISC_PATCH_OK &&
          report.checks == SCHWAY_DISC_PATCH_CHECK_SOURCE_SHA256,
          "accept the maximum documented PPF3 FileID description");
    schway_disc_patch_candidate_destroy(candidate);
    candidate = NULL;

    large_patch = (uint8_t *)malloc(60u +
        ((size_t)SCHWAY_DISC_PATCH_MAX_RECORDS + 1u) * 10u);
    CHECK(large_patch != NULL, "allocate bounded over-limit record fixture");
    patch_size = begin_ppf3(large_patch,
        60u + ((size_t)SCHWAY_DISC_PATCH_MAX_RECORDS + 1u) * 10u,
        raw, 0u, 0u);
    CHECK(patch_size == 60u, "start over-limit record fixture");
    for (uint32_t i = 0u; i <= SCHWAY_DISC_PATCH_MAX_RECORDS; i++) {
        patch_size = add_ppf3_record(large_patch, patch_size, 0u,
                                     patch_byte, 1u, NULL, 0u);
    }
    REJECT_PPF(large_patch, patch_size, source_sha, SCHWAY_DISC_PATCH_LIMIT, "reject excess PPF3 records");
    patch_size = begin_ppf2(large_patch,
        60u + ((size_t)SCHWAY_DISC_PATCH_MAX_RECORDS + 1u) * 10u,
        raw, PATCH_SOURCE_BYTES);
    CHECK(patch_size == PATCH_PPF2_HEADER_BYTES,
          "start over-limit PPF2 record fixture");
    for (uint32_t i = 0u; i <= SCHWAY_DISC_PATCH_MAX_RECORDS; i++) {
        patch_size = add_ppf1_record(large_patch, patch_size, 0u,
                                     patch_byte, 1u);
    }
    REJECT_PPF(large_patch, patch_size, source_sha, SCHWAY_DISC_PATCH_LIMIT, "reject excess PPF2 records");

    FILE *source_file = schway__test_open_file(source_path, "rb");
    CHECK(source_file != NULL, "reopen original source after rejected patches");
    CHECK(fread(file_copy, 1u, sizeof file_copy, source_file) == sizeof file_copy,
          "read unchanged source after rejected patches");
    CHECK(fclose(source_file) == 0, "close unchanged source file");
    CHECK(memcmp(file_copy, raw, PATCH_SOURCE_BYTES) == 0,
          "all rejected PPF candidates preserve source bytes");
    CHECK(read_all_sectors(image, file_copy) &&
          memcmp(file_copy, expected, PATCH_SOURCE_BYTES) == 0,
          "all rejected PPF candidates preserve the prior overlay");
cleanup:
    schway_disc_patch_candidate_destroy(candidate);
    free(large_patch);
    schway_disc_image_close(image);
    free(raw);
    return result;
}
#undef REJECT_PPF
#undef REJECT_PPF3_BYTE
#undef REJECT_PPF3_RECORD

static int run_atomic(const char *root)
{
    static const uint8_t first_value[] = {0xa5u};
    static const uint8_t other_value[] = {0x5au};
    static const uint8_t overlap_first[] = {0xb1u};
    static const uint8_t overlap_last[] = {0xb2u};
    static const uint8_t across[] = {0xc1u, 0xc2u, 0xc3u};
    static const uint8_t across_later[] = {0xd1u};
    schway_disc_image *image = NULL;
    schway_disc_image *other_image = NULL;
    schway_disc_patch_candidate *candidate = NULL;
    schway_disc_patch_candidate *other_candidate = NULL;
    schway_disc_patch_source source;
    schway_disc_patch_source other_source;
    schway_disc_patch_report report;
    schway_disc_patch_error error;
    const schway_disc *previous_reader = NULL;
    uint8_t *raw = make_raw_source();
    uint8_t first_expected[PATCH_SOURCE_BYTES];
    uint8_t overlap_expected[PATCH_SOURCE_BYTES];
    uint8_t actual[PATCH_SOURCE_BYTES];
    uint8_t patch_first[64];
    uint8_t patch_other[64];
    uint8_t patch_overlap[96];
    uint8_t source_sha[32];
    char source_path[1024];
    size_t patch_size;
    int result = 0;
    CHECK(raw != NULL, "allocate generated raw source");
    CHECK(make_path(source_path, root, "atomic.bin"), "build primary source path");
    CHECK(write_bytes(source_path, raw, PATCH_SOURCE_BYTES), "write primary raw source");
    CHECK(make_path(source_path, root, "other.bin"), "build secondary source path");
    CHECK(write_bytes(source_path, raw, PATCH_SOURCE_BYTES), "write secondary raw source");
    CHECK(open_raw_cue(root, "atomic.cue", "atomic.bin", "MODE2/2352", NULL, &image),
          "open primary source");
    CHECK(open_raw_cue(root, "other.cue", "other.bin", "MODE2/2352", NULL,
                       &other_image), "open equal-byte secondary source");
    CHECK(schway_disc_image_patch_source(image, &source, &error) == SCHWAY_DISC_PATCH_OK &&
          schway_disc_image_patch_source(other_image, &other_source, &error) ==
              SCHWAY_DISC_PATCH_OK,
          "obtain both exact raw-track mappings");
    digest(raw, PATCH_SOURCE_BYTES, source_sha);

    patch_size = begin_ppf1(patch_first, sizeof patch_first);
    CHECK(patch_size != 0u, "start primary PPF1 fixture");
    patch_size = add_ppf1_record(patch_first, patch_size, 1000u,
                                 first_value, (uint8_t)sizeof first_value);
    memcpy(first_expected, raw, PATCH_SOURCE_BYTES);
    first_expected[1000u] = first_value[0];
    CHECK(schway_disc_patch_load(&source, SCHWAY_DISC_PATCH_FORMAT_PPF1,
                                 patch_first, patch_size, source_sha, &candidate,
                                 &report, &error) == SCHWAY_DISC_PATCH_OK,
          "load initial candidate");
    CHECK(schway_disc_image_patch_replace(image, &candidate, &error) ==
              SCHWAY_DISC_PATCH_OK && candidate == NULL,
          "attach initial overlay");
    previous_reader = schway_disc_image_disc(image);
    CHECK(read_all_sectors(image, actual) &&
          memcmp(actual, first_expected, PATCH_SOURCE_BYTES) == 0,
          "observe initial overlay before replacement");

    patch_size = begin_ppf1(patch_other, sizeof patch_other);
    CHECK(patch_size != 0u, "start secondary PPF1 fixture");
    patch_size = add_ppf1_record(patch_other, patch_size, 1000u,
                                 other_value, (uint8_t)sizeof other_value);
    CHECK(schway_disc_patch_load(&other_source, SCHWAY_DISC_PATCH_FORMAT_PPF1,
                                 patch_other, patch_size, source_sha,
                                 &other_candidate, &report, &error) ==
              SCHWAY_DISC_PATCH_OK,
          "load candidate for equal bytes from another image");
    CHECK(schway_disc_image_patch_replace(image, &other_candidate, &error) ==
              SCHWAY_DISC_PATCH_BAD_SOURCE && other_candidate != NULL,
          "reject candidate bound to a different raw source");
    CHECK(schway_disc_image_disc(image) == previous_reader &&
          read_all_sectors(image, actual) &&
          memcmp(actual, first_expected, PATCH_SOURCE_BYTES) == 0,
          "failed replacement retains prior reader and overlay bytes");
    schway_disc_patch_candidate_destroy(other_candidate);
    other_candidate = NULL;

    patch_size = begin_ppf1(patch_overlap, sizeof patch_overlap);
    CHECK(patch_size != 0u, "start overlapping PPF1 fixture");
    patch_size = add_ppf1_record(patch_overlap, patch_size, 1000u,
                                 overlap_first, (uint8_t)sizeof overlap_first);
    patch_size = add_ppf1_record(patch_overlap, patch_size, 1000u,
                                 overlap_last, (uint8_t)sizeof overlap_last);
    patch_size = add_ppf1_record(patch_overlap, patch_size,
                                 PATCH_SECTOR_BYTES - 1u, across,
                                 (uint8_t)sizeof across);
    patch_size = add_ppf1_record(patch_overlap, patch_size, PATCH_SECTOR_BYTES,
                                 across_later, (uint8_t)sizeof across_later);
    memcpy(overlap_expected, raw, PATCH_SOURCE_BYTES);
    overlap_expected[1000u] = overlap_last[0];
    overlap_expected[PATCH_SECTOR_BYTES - 1u] = across[0];
    overlap_expected[PATCH_SECTOR_BYTES] = across_later[0];
    overlap_expected[PATCH_SECTOR_BYTES + 1u] = across[2];
    CHECK(schway_disc_patch_load(&source, SCHWAY_DISC_PATCH_FORMAT_PPF1,
                                 patch_overlap, patch_size, source_sha, &candidate,
                                 &report, &error) == SCHWAY_DISC_PATCH_OK &&
          report.records == 4u,
          "load all ordered overlapping records into a replacement candidate");
    CHECK(schway_disc_image_patch_replace(image, &candidate, &error) ==
              SCHWAY_DISC_PATCH_OK && candidate == NULL,
          "atomically attach valid replacement candidate");
    CHECK(read_all_sectors(image, actual) &&
          memcmp(actual, overlap_expected, PATCH_SOURCE_BYTES) == 0,
          "later overlapping PPF1 records win across sector boundaries");
    CHECK(read_all_sectors(image, actual) &&
          memcmp(actual, overlap_expected, PATCH_SOURCE_BYTES) == 0,
          "replacement sectors remain deterministic on repeated reads");
cleanup:
    schway_disc_patch_candidate_destroy(other_candidate);
    schway_disc_patch_candidate_destroy(candidate);
    schway_disc_image_close(other_image);
    schway_disc_image_close(image);
    free(raw);
    return result;
}

static int known_case(const char *name)
{
    return strcmp(name, "mapping") == 0 || strcmp(name, "ppf1") == 0 ||
           strcmp(name, "ppf2") == 0 || strcmp(name, "ppf3") == 0 ||
           strcmp(name, "bps") == 0 || strcmp(name, "atomic") == 0 ||
           strcmp(name, "bounds") == 0;
}

int main(int argc, char **argv)
{
    if (argc != 3 || argv[1][0] == '\0' || argv[2][0] == '\0' ||
        strlen(argv[2]) >= 4096u) {
        fputs("usage: schwaystation-test-disc-patch CASE BUILD_WORK_ROOT\n", stderr);
        return 2;
    }
    if (!known_case(argv[1])) {
        fprintf(stderr, "unknown patch test case: %s\n", argv[1]);
        return 2;
    }
    test_case = argv[1];
    if (strcmp(test_case, "bps") == 0) return run_bps(argv[2]);
    if (strcmp(test_case, "mapping") == 0) return run_mapping(argv[2]);
    if (strcmp(test_case, "atomic") == 0) return run_atomic(argv[2]);
    if (strcmp(test_case, "bounds") == 0) return run_bounds(argv[2]);
    if (strcmp(test_case, "ppf2") == 0) return run_ppf2(argv[2]);
    if (strcmp(test_case, "ppf3") == 0) return run_ppf3(argv[2]);
    return run_ppf1(argv[2]);
}
