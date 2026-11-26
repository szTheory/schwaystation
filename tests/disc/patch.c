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
           strcmp(name, "atomic") == 0;
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
    if (strcmp(test_case, "mapping") == 0) return run_mapping(argv[2]);
    if (strcmp(test_case, "atomic") == 0) return run_atomic(argv[2]);
    return run_ppf1(argv[2]);
}
