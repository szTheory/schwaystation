/* Cheap callback and run-local CHD fixture checks before guest execution. */
#include "disc_image.h"
#include "disc_chd.h"
#include "../test_open.h"
#if SCHWAYSTATION_CHD
#include "sha256.h"
#include "save_game_identity.h"
#include <stdbool.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition, message) do { \
    if (!(condition)) { \
        fprintf(stderr, "ASSERT disc.media.preflight: %s\n", (message)); \
        result = 1; \
        goto cleanup; \
    } \
} while (0)

#if SCHWAYSTATION_CHD
#define SCHWAY__CHD_FIXTURE_FIELDS 23u

struct schway__manifest {
    size_t count;
    char keys[SCHWAY__CHD_FIXTURE_FIELDS][64];
    char values[SCHWAY__CHD_FIXTURE_FIELDS][128];
};

static const char *const schway__manifest_keys[SCHWAY__CHD_FIXTURE_FIELDS] = {
    "format", "source_cue_sha256", "track1_file", "track1_type", "track1_bytes",
    "track1_sectors", "track1_sha256", "track2_file", "track2_type", "track2_bytes",
    "track2_sectors", "track2_sha256", "expected_track_count", "logical_sector_count",
    "tool_commit", "tool_archive_sha256", "tool_sha256", "codecs", "hunk_bytes",
    "unit_bytes", "workers", "chd_bytes", "chd_sha256"
};
#endif

static int read_boundary(const schway_disc *disc, uint32_t lba)
{
    uint8_t sector[2354];
    uint8_t subcode[98];

    memset(sector, 0xa5, sizeof sector);
    memset(subcode, 0xa5, sizeof subcode);
    if (disc->read_sector == NULL ||
        disc->read_sector(disc->user, lba, sector + 1u, subcode + 1u) != 1u ||
        sector[0] != 0xa5u || sector[2353] != 0xa5u ||
        subcode[0] != 0xa5u || subcode[97] != 0xa5u) {
        fprintf(stderr, "ASSERT disc.media.preflight: unsafe or unreadable LBA %u\n", lba);
        return 0;
    }
    for (size_t i = 1u; i <= 96u; i++) {
        if (subcode[i] != 0u) {
            fprintf(stderr, "ASSERT disc.media.preflight: unexpected subcode byte at LBA %u\n",
                    lba);
            return 0;
        }
    }
    return 1;
}

static int join_path(char path[1024], const char *directory, const char *leaf)
{
    int size = snprintf(path, 1024u, "%s/%s", directory, leaf);
    return size >= 0 && (size_t)size < 1024u;
}

#if SCHWAYSTATION_CHD
static int file_size(const char *path, uint64_t *size)
{
    FILE *file = schway__test_open_file(path, "rb");
    long end;
    int close_result;

    if (file == NULL || fseek(file, 0L, SEEK_END) != 0 || (end = ftell(file)) < 0L) {
        if (file != NULL) (void)fclose(file);
        return 0;
    }
    close_result = fclose(file);
    if (close_result != 0) return 0;
    *size = (uint64_t)end;
    return 1;
}

static int file_sha256(const char *path, char hex[65])
{
    uint8_t buffer[4096];
    uint8_t digest[32];
    schway__sha256 hash;
    FILE *file = schway__test_open_file(path, "rb");
    int read_error;
    int close_result;

    if (file == NULL) return 0;
    schway__sha256_init(&hash);
    while (true) {
        size_t count = fread(buffer, 1u, sizeof buffer, file);
        if (count != 0u) schway__sha256_update(&hash, buffer, count);
        if (count < sizeof buffer) break;
    }
    read_error = ferror(file);
    close_result = fclose(file);
    if (read_error != 0 || close_result != 0) return 0;
    schway__sha256_final(&hash, digest);
    schway__sha256_hex(digest, hex);
    return 1;
}

static const char *manifest_get(const struct schway__manifest *manifest,
                                const char *key)
{
    for (size_t i = 0u; i < manifest->count; i++) {
        if (strcmp(manifest->keys[i], key) == 0) return manifest->values[i];
    }
    return NULL;
}

static int manifest_read(const char *path, struct schway__manifest *manifest)
{
    char line[256];
    FILE *file = schway__test_open_file(path, "rb");
    int valid = 1;

    if (file == NULL) return 0;
    memset(manifest, 0, sizeof *manifest);
    while (fgets(line, (int)sizeof line, file) != NULL) {
        char *equals = strchr(line, '=');
        char *newline = strchr(line, '\n');
        size_t key_size;
        size_t value_size;
        if (equals == NULL || newline == NULL || newline[1] != '\0' ||
            (newline > line && newline[-1] == '\r') || manifest->count >= SCHWAY__CHD_FIXTURE_FIELDS) {
            valid = 0;
            break;
        }
        *equals = '\0';
        *newline = '\0';
        key_size = strlen(line);
        value_size = strlen(equals + 1);
        if (key_size == 0u || key_size >= sizeof manifest->keys[0] ||
            value_size >= sizeof manifest->values[0]) {
            valid = 0;
            break;
        }
        int known_key = 0;
        for (size_t i = 0u; i < SCHWAY__CHD_FIXTURE_FIELDS; i++) {
            if (strcmp(schway__manifest_keys[i], line) == 0) known_key = 1;
        }
        if (known_key == 0) {
            valid = 0;
            break;
        }
        for (size_t i = 0u; i < manifest->count; i++) {
            if (strcmp(manifest->keys[i], line) == 0) valid = 0;
        }
        if (!valid) break;
        memcpy(manifest->keys[manifest->count], line, key_size + 1u);
        memcpy(manifest->values[manifest->count], equals + 1, value_size + 1u);
        manifest->count++;
    }
    if (ferror(file) != 0) valid = 0;
    if (fclose(file) != 0) valid = 0;
    return valid && manifest->count == SCHWAY__CHD_FIXTURE_FIELDS;
}

static int parse_u64(const char *text, uint64_t *value)
{
    uint64_t parsed = 0u;
    if (text == NULL || text[0] == '\0') return 0;
    for (size_t i = 0u; text[i] != '\0'; i++) {
        uint64_t digit;
        if (text[i] < '0' || text[i] > '9') return 0;
        digit = (uint64_t)(text[i] - '0');
        if (parsed > (UINT64_MAX - digit) / 10u) return 0;
        parsed = parsed * 10u + digit;
    }
    *value = parsed;
    return 1;
}

static int cue_directory(char directory[1024], const char *cue)
{
    size_t size = strlen(cue);
    const char *separator = NULL;
    if (size == 0u || size >= 1024u) return 0;
    for (size_t i = 0u; i < size; i++) {
        if (cue[i] == '/' || cue[i] == '\\') separator = cue + i;
    }
    if (separator == NULL) {
        memcpy(directory, ".", 2u);
        return 1;
    }
    size = (size_t)(separator - cue);
    if (size == 0u) size = 1u;
    memcpy(directory, cue, size);
    directory[size] = '\0';
    return 1;
}

static int validate_chd_fixture(const char *cue, const char *chd,
                                const char *manifest_path, const char *tool,
                                const char *track_directory)
{
    static const char tool_commit[] = "7f71bdf3d4099dc38bded49c6745e453005d6e41";
    static const char tool_archive_sha256[] =
        "7346f16486f114af808c6b361228ce9c816156bfd910c8ca8f7727a21d2b6a89";
    struct schway__manifest manifest;
    char cue_sha256[65];
    char track1_sha256[65];
    char track2_sha256[65];
    char tool_sha256[65];
    char chd_sha256[65];
    char directory[1024];
    char track1[1024];
    char track2[1024];
    uint64_t track1_bytes;
    uint64_t track2_bytes;
    uint64_t track1_sectors;
    uint64_t track2_sectors;
    uint64_t logical_sectors;
    uint64_t chd_bytes;
    uint64_t manifest_value;
    schway__disc_chd *reader = NULL;
    schway_disc_image_error error;
    int valid = 0;

    if (!manifest_read(manifest_path, &manifest) || !cue_directory(directory, cue)) return 0;
    if (track_directory == NULL) track_directory = directory;
    if (!join_path(track1, track_directory, "disc_boot.bin") ||
        !join_path(track2, track_directory, "disc_boot_track02.bin")) return 0;

    if (strcmp(manifest_get(&manifest, "format"), "schwaystation-chd-fixture-v1") != 0 ||
        strcmp(manifest_get(&manifest, "track1_file"), "disc_boot.bin") != 0 ||
        strcmp(manifest_get(&manifest, "track1_type"), "MODE2/2352") != 0 ||
        strcmp(manifest_get(&manifest, "track2_file"), "disc_boot_track02.bin") != 0 ||
        strcmp(manifest_get(&manifest, "track2_type"), "AUDIO") != 0 ||
        strcmp(manifest_get(&manifest, "tool_commit"), tool_commit) != 0 ||
        strcmp(manifest_get(&manifest, "tool_archive_sha256"), tool_archive_sha256) != 0 ||
        strcmp(manifest_get(&manifest, "codecs"), "cdlz,cdzl,cdfl,0") != 0 ||
        strcmp(manifest_get(&manifest, "hunk_bytes"), "19584") != 0 ||
        strcmp(manifest_get(&manifest, "unit_bytes"), "2448") != 0 ||
        strcmp(manifest_get(&manifest, "workers"), "1") != 0 ||
        strcmp(manifest_get(&manifest, "expected_track_count"), "2") != 0) return 0;

    if (!file_sha256(cue, cue_sha256) ||
        strcmp(manifest_get(&manifest, "source_cue_sha256"), cue_sha256) != 0 ||
        !file_sha256(track1, track1_sha256) ||
        strcmp(manifest_get(&manifest, "track1_sha256"), track1_sha256) != 0 ||
        !file_sha256(track2, track2_sha256) ||
        strcmp(manifest_get(&manifest, "track2_sha256"), track2_sha256) != 0 ||
        !file_sha256(tool, tool_sha256) ||
        strcmp(manifest_get(&manifest, "tool_sha256"), tool_sha256) != 0 ||
        !file_sha256(chd, chd_sha256) ||
        strcmp(manifest_get(&manifest, "chd_sha256"), chd_sha256) != 0 ||
        !file_size(track1, &track1_bytes) || !file_size(track2, &track2_bytes) ||
        !file_size(chd, &chd_bytes)) return 0;

    if (track1_bytes % 2352u != 0u || track2_bytes % 2352u != 0u) return 0;
    track1_sectors = track1_bytes / 2352u;
    track2_sectors = track2_bytes / 2352u;
    logical_sectors = track1_sectors + track2_sectors;
    if (track1_sectors != 64u || track2_sectors != 158u || logical_sectors != 222u ||
        !parse_u64(manifest_get(&manifest, "track1_bytes"), &manifest_value) || manifest_value != track1_bytes ||
        !parse_u64(manifest_get(&manifest, "track1_sectors"), &manifest_value) || manifest_value != track1_sectors ||
        !parse_u64(manifest_get(&manifest, "track2_bytes"), &manifest_value) || manifest_value != track2_bytes ||
        !parse_u64(manifest_get(&manifest, "track2_sectors"), &manifest_value) || manifest_value != track2_sectors ||
        !parse_u64(manifest_get(&manifest, "logical_sector_count"), &manifest_value) || manifest_value != logical_sectors ||
        !parse_u64(manifest_get(&manifest, "chd_bytes"), &manifest_value) || manifest_value != chd_bytes ||
        chd_bytes == 0u || chd_bytes >= UINT64_C(524288)) return 0;

    memset(&error, 0, sizeof error);
    if (schway__disc_chd_open(chd, &reader, &error) != SCHWAY_DISC_IMAGE_OK || reader == NULL) goto cleanup;
    valid = 1;

cleanup:
    schway__disc_chd_close(reader);
    return valid;
}

static int write_manifest_variant(const char *source, const char *output,
                                  const char *key, const char *replacement)
{
    char line[256];
    char prefix[80];
    FILE *input = schway__test_open_file(source, "rb");
    FILE *result = schway__test_open_file(output, "wb");
    int found = 0;
    int valid = 1;
    int prefix_size = snprintf(prefix, sizeof prefix, "%s=", key);

    if (input == NULL || result == NULL || prefix_size < 0 || (size_t)prefix_size >= sizeof prefix) {
        if (input != NULL) (void)fclose(input);
        if (result != NULL) (void)fclose(result);
        return 0;
    }
    while (fgets(line, (int)sizeof line, input) != NULL) {
        if (strncmp(line, prefix, (size_t)prefix_size) == 0) {
            if (found != 0 || fprintf(result, "%s=%s\n", key, replacement) < 0) valid = 0;
            found = 1;
        } else if (fputs(line, result) == EOF) {
            valid = 0;
        }
    }
    if (ferror(input) != 0) valid = 0;
    if (fclose(input) != 0) valid = 0;
    if (fclose(result) != 0) valid = 0;
    return valid && found != 0;
}

static int copy_and_corrupt(const char *source, const char *output)
{
    uint8_t buffer[4096];
    FILE *input = schway__test_open_file(source, "rb");
    FILE *result = schway__test_open_file(output, "wb");
    int valid = 1;

    if (input == NULL || result == NULL) {
        if (input != NULL) (void)fclose(input);
        if (result != NULL) (void)fclose(result);
        return 0;
    }
    while (true) {
        size_t count = fread(buffer, 1u, sizeof buffer, input);
        if (count != 0u && fwrite(buffer, 1u, count, result) != count) valid = 0;
        if (count < sizeof buffer) break;
    }
    if (ferror(input) != 0 || fputc(0, result) == EOF) valid = 0;
    if (fclose(input) != 0) valid = 0;
    if (fclose(result) != 0) valid = 0;
    return valid;
}

static int write_corrupt_manifest(const char *path)
{
    FILE *file = schway__test_open_file(path, "wb");
    if (file == NULL) return 0;
    if (fputs("format=invalid\n", file) == EOF) {
        (void)fclose(file);
        return 0;
    }
    return fclose(file) == 0;
}

static int run_fixture_controls(const char *cue, const char *test_directory,
                                const char *chd, const char *manifest,
                                const char *tool)
{
    char path[1024];
    char missing_chd[1024];
    int valid = 1;

    if (!join_path(path, test_directory, "chd-control-wrong-digest.manifest") ||
        !write_manifest_variant(manifest, path, "chd_sha256", "0000000000000000000000000000000000000000000000000000000000000000") ||
        validate_chd_fixture(cue, chd, path, tool, NULL)) valid = 0;
    (void)remove(path);

    if (!join_path(path, test_directory, "chd-control-missing-track.manifest") ||
        !write_manifest_variant(manifest, path, "format", "schwaystation-chd-fixture-v1") ||
        validate_chd_fixture(cue, chd, manifest, tool, test_directory)) valid = 0;
    (void)remove(path);

    if (!join_path(path, test_directory, "chd-control-wrong-count.manifest") ||
        !write_manifest_variant(manifest, path, "logical_sector_count", "221") ||
        validate_chd_fixture(cue, chd, path, tool, NULL)) valid = 0;
    (void)remove(path);

    if (!join_path(path, test_directory, "chd-control-oversized.manifest") ||
        !write_manifest_variant(manifest, path, "chd_bytes", "524288") ||
        validate_chd_fixture(cue, chd, path, tool, NULL)) valid = 0;
    (void)remove(path);

    if (!join_path(path, test_directory, "chd-control-corrupt.manifest") ||
        !write_corrupt_manifest(path) || validate_chd_fixture(cue, chd, path, tool, NULL)) valid = 0;
    (void)remove(path);

    if (!join_path(path, test_directory, "chd-control-corrupt.chd") ||
        !copy_and_corrupt(chd, path) || validate_chd_fixture(cue, path, manifest, tool, NULL)) valid = 0;
    (void)remove(path);

    if (!join_path(missing_chd, test_directory, "chd-control-missing.chd") ||
        !join_path(path, test_directory, "chd-control-missing.manifest") ||
        !write_manifest_variant(manifest, path, "format", "schwaystation-chd-fixture-v1") ||
        validate_chd_fixture(cue, missing_chd, path, tool, NULL)) valid = 0;
    (void)remove(path);

    if (!valid) {
        fprintf(stderr, "ASSERT disc.media.fixture_controls: a malformed CHD fixture passed preflight\n");
        return 1;
    }
    puts("PASS disc.media.fixture_controls");
    return 0;
}
#endif

#if !SCHWAYSTATION_CHD
static int run_disabled_fixture_controls(const char *fixture_directory)
{
    char chd[1024];
    char manifest[1024];
    FILE *file;
    schway__disc_chd *reader = NULL;
    schway_disc_image_error error;
    int result = 0;

    if (!join_path(chd, fixture_directory, "disc_boot.chd") ||
        !join_path(manifest, fixture_directory, "disc_boot.chd.manifest")) return 2;
    file = schway__test_open_file(chd, "rb");
    if (file != NULL) {
        (void)fclose(file);
        result = 1;
    }
    file = schway__test_open_file(manifest, "rb");
    if (file != NULL) {
        (void)fclose(file);
        result = 1;
    }
    memset(&error, 0, sizeof error);
    if (schway__disc_chd_open(chd, &reader, &error) != SCHWAY_DISC_IMAGE_UNSUPPORTED || reader != NULL) {
        result = 1;
    }
    schway__disc_chd_close(reader);
    if (result != 0) {
        fprintf(stderr, "ASSERT disc.media.fixture_controls: CHD-off build retained a fixture or accepted CHD\n");
        return result;
    }
    puts("PASS disc.media.fixture_controls");
    return 0;
}
#endif

#if SCHWAYSTATION_CHD
static int same_toc(const schway_disc *left, const schway_disc *right,
                    schway_toc *left_toc, schway_toc *right_toc)
{
    memset(left_toc, 0, sizeof *left_toc);
    memset(right_toc, 0, sizeof *right_toc);
    left_toc->size = (uint32_t)sizeof *left_toc;
    right_toc->size = (uint32_t)sizeof *right_toc;
    if (left == NULL || right == NULL || left->toc == NULL || right->toc == NULL ||
        left->toc(left->user, left_toc) == 0u || right->toc(right->user, right_toc) == 0u ||
        left_toc->size != sizeof *left_toc || right_toc->size != sizeof *right_toc ||
        left_toc->track_count != right_toc->track_count ||
        left_toc->track_count == 0u || left_toc->track_count > SCHWAY_DISC_MAX_TRACKS) {
        return 0;
    }
    for (uint32_t i = 0u; i < left_toc->track_count; i++) {
        const schway_disc_track *a = &left_toc->tracks[i];
        const schway_disc_track *b = &right_toc->tracks[i];
        if (a->size != sizeof *a || b->size != sizeof *b || a->type != b->type ||
            a->index0 != b->index0 || a->index1 != b->index1 ||
            a->pregap != b->pregap || a->leadout != b->leadout) return 0;
    }
    return 1;
}

static int compare_sector(const schway_disc *cue, const schway_disc *chd,
                          uint32_t lba)
{
    uint8_t cue_sector[2354];
    uint8_t chd_sector[2354];
    uint8_t cue_subcode[98];
    uint8_t chd_subcode[98];
    memset(cue_sector, 0xa5, sizeof cue_sector);
    memset(chd_sector, 0xa5, sizeof chd_sector);
    memset(cue_subcode, 0xa5, sizeof cue_subcode);
    memset(chd_subcode, 0xa5, sizeof chd_subcode);
    if (cue == NULL || chd == NULL || cue->read_sector == NULL || chd->read_sector == NULL ||
        cue->read_sector(cue->user, lba, cue_sector + 1u, cue_subcode + 1u) != 1u ||
        chd->read_sector(chd->user, lba, chd_sector + 1u, chd_subcode + 1u) != 1u ||
        cue_sector[0] != 0xa5u || cue_sector[2353] != 0xa5u ||
        chd_sector[0] != 0xa5u || chd_sector[2353] != 0xa5u ||
        cue_subcode[0] != 0xa5u || cue_subcode[97] != 0xa5u ||
        chd_subcode[0] != 0xa5u || chd_subcode[97] != 0xa5u) return 0;
    for (size_t i = 0u; i < 2352u; i++) {
        if (cue_sector[i + 1u] != chd_sector[i + 1u]) {
            fprintf(stderr, "sector mismatch LBA %u offset %zu CUE=%02x CHD=%02x\n",
                    lba, i, cue_sector[i + 1u], chd_sector[i + 1u]);
            return 0;
        }
    }
    for (size_t i = 0u; i < 96u; i++) {
        if (cue_subcode[i + 1u] != chd_subcode[i + 1u]) {
            fprintf(stderr, "subcode mismatch LBA %u offset %zu CUE=%02x CHD=%02x\n",
                    lba, i, cue_subcode[i + 1u], chd_subcode[i + 1u]);
            return 0;
        }
    }
    for (size_t i = 1u; i <= 96u; i++) {
        if (cue_subcode[i] != 0u || chd_subcode[i] != 0u) return 0;
    }
    return 1;
}

static int run_chd_parity(const char *cue_path, const char *chd_path)
{
    schway_disc_image *cue_image = NULL;
    schway_disc_image *chd_image = NULL;
    schway_disc_image_error error;
    schway__save_game_identity cue_identity;
    schway__save_game_identity chd_identity;
    const schway_disc *cue_disc;
    const schway_disc *chd_disc;
    schway_toc cue_toc;
    schway_toc chd_toc;
    uint64_t input_sectors = 0u;
    uint64_t produced_sectors = 0u;
    int result = 1;

#define PARITY_CHECK(condition, message) do { \
    if (!(condition)) { \
        fprintf(stderr, "ASSERT disc.image.chd_parity: %s\n", (message)); \
        goto cleanup_parity; \
    } \
} while (0)

    memset(&error, 0, sizeof error);
    PARITY_CHECK(schway_disc_image_open(cue_path, &cue_image, &error) == SCHWAY_DISC_IMAGE_OK,
                 "owned CUE did not open through the shared provider");
    PARITY_CHECK(schway_disc_image_open(chd_path, &chd_image, &error) == SCHWAY_DISC_IMAGE_OK,
                 "owned CHD did not open through the shared provider");
    cue_disc = schway_disc_image_disc(cue_image);
    chd_disc = schway_disc_image_disc(chd_image);
    PARITY_CHECK(same_toc(cue_disc, chd_disc, &cue_toc, &chd_toc),
                 "CHD TOC differs from the CUE oracle");
    for (uint32_t i = 0u; i < cue_toc.track_count; i++) {
        input_sectors += (uint64_t)cue_toc.tracks[i].leadout - cue_toc.tracks[i].index0;
        produced_sectors += (uint64_t)chd_toc.tracks[i].leadout - chd_toc.tracks[i].index0;
    }
    PARITY_CHECK(input_sectors == UINT64_C(222) && produced_sectors == input_sectors,
                 "source and produced logical sector counts differ from 222");
    for (uint32_t lba = 0u; lba < 222u; lba++) {
        PARITY_CHECK(compare_sector(cue_disc, chd_disc, lba),
                     "sector, subcode or output guard differs from CUE");
    }
    for (uint32_t lba = 222u; lba > 0u; lba--) {
        PARITY_CHECK(compare_sector(cue_disc, chd_disc, lba - 1u),
                     "reverse-order repeated read changed sector output");
    }
    PARITY_CHECK(schway__save_game_identity_resolve(NULL, NULL, 0u, cue_path,
                 cue_image, &cue_identity) == SCHWAY__SAVE_GAME_IDENTITY_OK &&
                 schway__save_game_identity_resolve(NULL, NULL, 0u, chd_path,
                 chd_image, &chd_identity) == SCHWAY__SAVE_GAME_IDENTITY_OK &&
                 cue_identity.kind == SCHWAY__SAVE_GAME_IDENTITY_DISC_DIGEST &&
                 chd_identity.kind == cue_identity.kind &&
                 strcmp(cue_identity.value, chd_identity.value) == 0,
                 "content-based disc identity differs from the CUE");
    printf("PASS disc.image.chd_parity input_sectors=%llu produced_sectors=%llu identity=%s\n",
           (unsigned long long)input_sectors, (unsigned long long)produced_sectors,
           chd_identity.value);
    result = 0;
cleanup_parity:
    schway_disc_image_close(chd_image);
    schway_disc_image_close(cue_image);
    return result;
#undef PARITY_CHECK
}
#else
static int run_chd_parity_disabled(void)
{
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    memset(&error, 0, sizeof error);
    if (schway_disc_image_open("disabled-feature.chd", &image, &error) !=
            SCHWAY_DISC_IMAGE_UNSUPPORTED || image != NULL) {
        schway_disc_image_close(image);
        fprintf(stderr, "ASSERT disc.image.chd_parity: CHD-off build accepted CHD\n");
        return 1;
    }
    puts("PASS disc.image.chd_parity");
    return 0;
}
#endif

static int run_media_preflight(const char *cue, const char *test_directory,
                               const char *chd, const char *manifest,
                               const char *tool)
{
    schway_disc_image *image = NULL;
    schway_disc_image *empty_image = NULL;
    schway_disc_image_error error;
    const schway_disc *disc;
    schway_toc toc;
    char empty_path[1024] = "";
    FILE *empty_file = NULL;
    int result = 0;

    int path_size = snprintf(empty_path, sizeof empty_path,
                             "%s/schway-disc-empty.cue", test_directory);
    CHECK(path_size > 0 && (size_t)path_size < sizeof empty_path,
          "could not form empty-input path");

    memset(&error, 0, sizeof error);
    CHECK(schway_disc_image_open(cue, &image, &error) == SCHWAY_DISC_IMAGE_OK,
          "owned CUE did not open through the production provider");
    disc = schway_disc_image_disc(image);
    memset(&toc, 0, sizeof toc);
    toc.size = (uint32_t)sizeof toc;
    CHECK(disc != NULL && disc->toc != NULL && disc->toc(disc->user, &toc) == 1u,
          "provider did not return a TOC");
    CHECK(toc.track_count == 2u,
          "owned CUE geometry changed: expected exactly two tracks");

    CHECK(toc.tracks[0].type == SCHWAY_DISC_TRACK_MODE2 &&
          toc.tracks[0].index0 == 0u && toc.tracks[0].index1 == 0u &&
          toc.tracks[0].pregap == 0u && toc.tracks[0].leadout == 64u,
          "track 1 geometry differs from the independently authored 64-sector data track");
    CHECK(toc.tracks[1].type == SCHWAY_DISC_TRACK_AUDIO &&
          toc.tracks[1].index0 == 64u && toc.tracks[1].index1 == 214u &&
          toc.tracks[1].pregap == 150u && toc.tracks[1].leadout == 222u,
          "track 2 geometry differs from the authored 150-sector pregap and 8-sector program");
    CHECK(toc.tracks[0].leadout == toc.tracks[1].index0 &&
          toc.tracks[0].leadout < toc.tracks[1].index1 &&
          toc.tracks[1].index1 < toc.tracks[1].leadout,
          "adjacent track boundaries or exclusive leadout are inconsistent");

    CHECK(read_boundary(disc, 0u) && read_boundary(disc, 63u) &&
          read_boundary(disc, 64u) && read_boundary(disc, 214u) &&
          read_boundary(disc, 221u),
          "a first, touching, index, or final valid sector failed its bounded read");
    uint8_t sector[2352];
    uint8_t subcode[96];
    CHECK(disc->read_sector(disc->user, 222u, sector, subcode) == 0u,
          "exclusive leadout must not be readable as a sector");

    CHECK(schway_disc_image_open("", &empty_image, &error) == SCHWAY_DISC_IMAGE_BAD_ARGUMENT &&
          empty_image == NULL,
          "empty path must be rejected without returning an image");
    empty_file = schway__test_open_file(empty_path, "wb");
    CHECK(empty_file != NULL, "could not create an empty CUE input");
    int close_result = fclose(empty_file);
    empty_file = NULL;
    CHECK(close_result == 0, "could not finish creating an empty CUE input");
    CHECK(schway_disc_image_open(empty_path, &empty_image, &error) != SCHWAY_DISC_IMAGE_OK &&
          empty_image == NULL,
          "empty CUE input must be rejected without returning an image");

#if SCHWAYSTATION_CHD
    CHECK(chd != NULL && manifest != NULL && tool != NULL &&
          validate_chd_fixture(cue, chd, manifest, tool, NULL),
          "owned CHD, manifest, source digests, or geometry failed preflight");
#else
    (void)chd;
    (void)manifest;
    (void)tool;
#endif

cleanup:
    if (empty_file != NULL) (void)fclose(empty_file);
    schway_disc_image_close(empty_image);
    schway_disc_image_close(image);
    if (empty_path[0] != '\0') (void)remove(empty_path);
    return result;
}

int main(int argc, char **argv)
{
    int result;
    if (argc >= 2 && strcmp(argv[1], "--chd-parity") == 0) {
#if SCHWAYSTATION_CHD
        if (argc != 4) return 2;
        return run_chd_parity(argv[2], argv[3]);
#else
        if (argc != 2) return 2;
        return run_chd_parity_disabled();
#endif
    }
    if (argc >= 2 && strcmp(argv[1], "--fixture-controls") == 0) {
#if SCHWAYSTATION_CHD
        if (argc != 7) return 2;
        result = run_fixture_controls(argv[2], argv[3], argv[4], argv[5], argv[6]);
#else
        if (argc != 3) return 2;
        result = run_disabled_fixture_controls(argv[2]);
#endif
    } else {
#if SCHWAYSTATION_CHD
        if (argc != 6) return 2;
        result = run_media_preflight(argv[1], argv[2], argv[3], argv[4], argv[5]);
#else
        if (argc != 3) return 2;
        result = run_media_preflight(argv[1], argv[2], NULL, NULL, NULL);
#endif
        if (result == 0) puts("PASS disc.media.preflight");
    }
    return result;
}
