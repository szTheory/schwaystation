#include "disc_pbp.h"
#include "disc_pbp_metadata.h"
#include "media_limits.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../test_open.h"
#include "pbp_inflate.h"
#include "sha256.h"

#define SCHWAY__DISC_PBP_TEST_HEADER_BYTES 40u
#define SCHWAY__DISC_PBP_TEST_MARKER_BYTES 12u
#define SCHWAY__DISC_PBP_TEST_IMAGE_BYTES \
    (SCHWAY__DISC_PBP_TEST_HEADER_BYTES + SCHWAY__DISC_PBP_TEST_MARKER_BYTES)
#define SCHWAY__DISC_PBP_FIXTURE_CAP_BYTES 1572864u
#define SCHWAY__DISC_PBP_SECTOR_BYTES 2352u
#define SCHWAY__DISC_PBP_SECTORS_PER_BLOCK 16u
#define SCHWAY__DISC_PBP_BLOCK_BYTES 0x9300u
#define SCHWAY__DISC_PBP_PSAR_OFFSET 0x10000u
#define SCHWAY__DISC_PBP_DATA_BASE 0x110000u
#define SCHWAY__DISC_PBP_TOC_OFFSET 0x10800u
#define SCHWAY__DISC_PBP_INDEX_OFFSET 0x14000u
#define SCHWAY__DISC_PBP_INDEX_ENTRIES 80u
#define SCHWAY__DISC_PBP_SOURCE_CAP_BYTES 524288u

static int make_path(char path[1024], const char *directory, const char *name);
static int write_bytes(const char *path, const uint8_t *bytes, size_t size);
static void put_le32(uint8_t bytes[4], uint32_t value);

static uint16_t get_le16(const uint8_t bytes[2])
{
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}

static uint32_t get_le32(const uint8_t bytes[4])
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static int read_file(const char *path, size_t limit, uint8_t **bytes, size_t *size,
                     const char *failure, char *error, size_t error_size)
{
    FILE *file = fopen(path, "rb");
    long end;
    uint8_t *data;

    if (file == NULL) {
        (void)snprintf(error, error_size, "%s", failure);
        return 0;
    }
    if (fseek(file, 0L, SEEK_END) != 0 || (end = ftell(file)) < 0L ||
        (unsigned long)end > (unsigned long)limit || fseek(file, 0L, SEEK_SET) != 0) {
        (void)fclose(file);
        (void)snprintf(error, error_size, "%s", failure);
        return 0;
    }
    data = (uint8_t *)malloc(end == 0L ? 1u : (size_t)end);
    if (data == NULL || (end != 0L && fread(data, 1u, (size_t)end, file) != (size_t)end)) {
        free(data);
        (void)fclose(file);
        (void)snprintf(error, error_size, "%s", failure);
        return 0;
    }
    if (fclose(file) != 0) {
        free(data);
        (void)snprintf(error, error_size, "%s", failure);
        return 0;
    }
    *bytes = data;
    *size = (size_t)end;
    return 1;
}

static void hash_hex(const uint8_t *bytes, size_t size, char hex[65])
{
    schway__sha256 state;
    uint8_t digest[32];
    schway__sha256_init(&state);
    schway__sha256_update(&state, bytes, size);
    schway__sha256_final(&state, digest);
    schway__sha256_hex(digest, hex);
}

static uint8_t bcd(uint32_t value)
{
    return (uint8_t)(((value / 10u) << 4) | (value % 10u));
}

static int write_manifest(const char *path, const uint8_t *source, size_t source_size,
                          const uint8_t *generator, size_t generator_size,
                          const uint8_t *pbp, size_t pbp_size,
                          size_t sector_count, size_t block_count)
{
    char source_hash[65];
    char generator_hash[65];
    char pbp_hash[65];
    char manifest[1024];
    int length;
    hash_hex(source, source_size, source_hash);
    hash_hex(generator, generator_size, generator_hash);
    hash_hex(pbp, pbp_size, pbp_hash);
    length = snprintf(manifest, sizeof(manifest),
        "format=schwaystation-pbp-fixture-v1\n"
        "source=disc_boot_pbp.bin\n"
        "source_bytes=%zu\n"
        "source_sha256=%s\n"
        "generator=gen_pbp.py\n"
        "generator_sha256=%s\n"
        "output=schwaystation-disc_boot.pbp\n"
        "output_bytes=%zu\n"
        "output_sha256=%s\n"
        "sectors=%zu\n"
        "blocks=%zu\n",
        source_size, source_hash, generator_hash, pbp_size, pbp_hash,
        sector_count, block_count);
    if (length < 0 || (size_t)length >= sizeof(manifest)) return 0;
    return write_bytes(path, (const uint8_t *)manifest, (size_t)length);
}

static int validate_fixture(const char *pbp_path, const char *cue_path,
                            const char *source_path, const char *manifest_path,
                            const char *generator_path, char *error, size_t error_size)
{
    static const char expected_cue[] =
        "FILE \"disc_boot_pbp.bin\" BINARY\n"
        "  TRACK 01 MODE2/2352\n"
        "    INDEX 01 00:00:00\n";
    uint8_t *source = NULL;
    uint8_t *generator = NULL;
    uint8_t *pbp = NULL;
    uint8_t *cue = NULL;
    uint8_t *manifest = NULL;
    size_t source_size = 0u;
    size_t generator_size = 0u;
    size_t pbp_size = 0u;
    size_t cue_size = 0u;
    size_t manifest_size = 0u;
    size_t sector_count;
    size_t block_count;
    char expected_manifest[1024];
    char source_hash[65];
    char generator_hash[65];
    char pbp_hash[65];
    size_t expected_manifest_size;
    int valid = 0;

    if (!read_file(source_path, SCHWAY__DISC_PBP_SOURCE_CAP_BYTES,
                   &source, &source_size, "could not read PBP source", error, error_size)) goto done;
    if (source_size == 0u || source_size % SCHWAY__DISC_PBP_SECTOR_BYTES != 0u ||
        source_size > SCHWAY__DISC_PBP_SOURCE_CAP_BYTES) {
        (void)snprintf(error, error_size, "source sector count or size is invalid");
        goto done;
    }
    sector_count = source_size / SCHWAY__DISC_PBP_SECTOR_BYTES;
    if (sector_count % SCHWAY__DISC_PBP_SECTORS_PER_BLOCK != 0u) {
        (void)snprintf(error, error_size, "source sector count is not block-aligned");
        goto done;
    }
    block_count = sector_count / SCHWAY__DISC_PBP_SECTORS_PER_BLOCK;
    if (block_count > SCHWAY__DISC_PBP_INDEX_ENTRIES) {
        (void)snprintf(error, error_size, "source has too many PBP blocks");
        goto done;
    }
    if (!read_file(cue_path, 1024u, &cue, &cue_size,
                   "could not read PBP source CUE", error, error_size)) goto done;
    if (cue_size != sizeof(expected_cue) - 1u ||
        memcmp(cue, expected_cue, sizeof(expected_cue) - 1u) != 0) {
        (void)snprintf(error, error_size, "source CUE track count or geometry is invalid");
        goto done;
    }
    if (!read_file(generator_path, 65536u, &generator, &generator_size,
                   "could not read PBP generator", error, error_size)) goto done;
    if (!read_file(pbp_path, SCHWAY__DISC_PBP_FIXTURE_CAP_BYTES + 1u,
                   &pbp, &pbp_size, "could not read PBP output", error, error_size)) goto done;
    if (pbp_size > SCHWAY__DISC_PBP_FIXTURE_CAP_BYTES) {
        (void)snprintf(error, error_size, "PBP exceeds the 1.5 MiB cap");
        goto done;
    }
    if (pbp_size < SCHWAY__DISC_PBP_DATA_BASE ||
        memcmp(pbp, "\0PBP", 4u) != 0 || get_le32(pbp + 4u) != UINT32_C(0x00010000)) {
        (void)snprintf(error, error_size, "PBP header or data base is invalid");
        goto done;
    }
    for (size_t i = 0u; i < 8u; i++) {
        uint32_t expected = i == 0u ? SCHWAY__DISC_PBP_TEST_HEADER_BYTES :
            (i == 7u ? SCHWAY__DISC_PBP_PSAR_OFFSET : UINT32_C(0x3d8));
        if (get_le32(pbp + 8u + i * 4u) != expected) {
            (void)snprintf(error, error_size, "PBP section offsets are invalid");
            goto done;
        }
    }
    if (memcmp(pbp + SCHWAY__DISC_PBP_PSAR_OFFSET, "PSISOIMG0000", 12u) != 0 ||
        get_le32(pbp + SCHWAY__DISC_PBP_PSAR_OFFSET + 12u) !=
            (uint32_t)(pbp_size - SCHWAY__DISC_PBP_PSAR_OFFSET) ||
        get_le32(pbp + SCHWAY__DISC_PBP_PSAR_OFFSET + 16u) !=
            (uint32_t)(pbp_size - 0x6420u)) {
        (void)snprintf(error, error_size, "PBP PSISO header is invalid");
        goto done;
    }
    if (memcmp(pbp + SCHWAY__DISC_PBP_PSAR_OFFSET + 0x400u, "_TEST_00000", 11u) != 0) {
        (void)snprintf(error, error_size, "PBP game identity is invalid");
        goto done;
    }
    {
        uint8_t expected_toc[0xA00];
        uint32_t leadout_frames = (uint32_t)(150u + sector_count);
        uint32_t minutes = leadout_frames / 4500u;
        uint32_t seconds = (leadout_frames % 4500u) / 75u;
        uint32_t frames = leadout_frames % 75u;
        static const uint8_t a0[10] = { 0x41u, 0u, 0xa0u, 0u, 0u, 0u, 0u, 0x01u, 0x20u, 0u };
        static const uint8_t a1[10] = { 0x41u, 0u, 0xa1u, 0u, 0u, 0u, 0u, 0x01u, 0u, 0u };
        static const uint8_t track[10] = { 0x41u, 0u, 0x01u, 0u, 0x02u, 0x01u, 0u, 0u, 0x02u, 0u };
        memset(expected_toc, 0, sizeof(expected_toc));
        memcpy(expected_toc, a0, sizeof(a0));
        memcpy(expected_toc + 10u, a1, sizeof(a1));
        expected_toc[20u] = 0x41u;
        expected_toc[22u] = 0xa2u;
        expected_toc[27u] = bcd(minutes);
        expected_toc[28u] = bcd(seconds);
        expected_toc[29u] = bcd(frames);
        memcpy(expected_toc + 30u, track, sizeof(track));
        if (memcmp(pbp + SCHWAY__DISC_PBP_TOC_OFFSET, expected_toc, sizeof(expected_toc)) != 0) {
            (void)snprintf(error, error_size, "PBP TOC track count or geometry is invalid");
            goto done;
        }
    }
    if (get_le32(pbp + SCHWAY__DISC_PBP_PSAR_OFFSET + 0x1220u) !=
            (uint32_t)(pbp_size - 0xD2CFu) ||
        get_le32(pbp + SCHWAY__DISC_PBP_PSAR_OFFSET + 0x1228u) != UINT32_C(0x7ff)) {
        (void)snprintf(error, error_size, "PBP fixture metadata is invalid");
        goto done;
    }
    if (get_le32(pbp + SCHWAY__DISC_PBP_INDEX_OFFSET) != 0u) {
        (void)snprintf(error, error_size, "PBP first block offset is invalid");
        goto done;
    }
    {
        uint8_t decoded[SCHWAY__DISC_PBP_BLOCK_BYTES];
        size_t next_offset = 0u;
        for (size_t block = 0u; block < block_count; block++) {
            const uint8_t *entry = pbp + SCHWAY__DISC_PBP_INDEX_OFFSET + block * 32u;
            uint32_t offset = get_le32(entry);
            uint16_t stored_size = get_le16(entry + 4u);
            size_t file_offset;
            const uint8_t *contents;
            if (offset != next_offset || stored_size == 0u || stored_size > SCHWAY__DISC_PBP_BLOCK_BYTES ||
                get_le16(entry + 6u) != 0u) {
                (void)snprintf(error, error_size, "PBP block extents are not contiguous");
                goto done;
            }
            file_offset = (size_t)SCHWAY__DISC_PBP_DATA_BASE + (size_t)offset;
            if (file_offset > pbp_size || (size_t)stored_size > pbp_size - file_offset) {
                (void)snprintf(error, error_size, "PBP block extent exceeds file");
                goto done;
            }
            contents = pbp + file_offset;
            if ((size_t)stored_size == SCHWAY__DISC_PBP_BLOCK_BYTES) {
                memcpy(decoded, contents, sizeof(decoded));
            } else if (!schway__pbp_inflate_raw(contents, stored_size, decoded, sizeof(decoded))) {
                (void)snprintf(error, error_size, "PBP compressed block is invalid");
                goto done;
            }
            if (memcmp(decoded, source + block * SCHWAY__DISC_PBP_BLOCK_BYTES,
                       sizeof(decoded)) != 0) {
                (void)snprintf(error, error_size, "PBP decoded sectors differ from source");
                goto done;
            }
            for (size_t i = 8u; i < 32u; i++) {
                if (entry[i] != 0u) {
                    (void)snprintf(error, error_size, "PBP block padding is not zero");
                    goto done;
                }
            }
            next_offset += (size_t)stored_size;
        }
        if (block_count * SCHWAY__DISC_PBP_SECTORS_PER_BLOCK != sector_count ||
            (size_t)SCHWAY__DISC_PBP_DATA_BASE + next_offset != pbp_size) {
            (void)snprintf(error, error_size, "PBP decoded sector total or output end is invalid");
            goto done;
        }
        for (size_t i = SCHWAY__DISC_PBP_INDEX_OFFSET + block_count * 32u;
             i < SCHWAY__DISC_PBP_DATA_BASE; i++) {
            if (pbp[i] != 0u) {
                (void)snprintf(error, error_size, "PBP unused block index or padding is not zero");
                goto done;
            }
        }
    }
    hash_hex(source, source_size, source_hash);
    hash_hex(generator, generator_size, generator_hash);
    hash_hex(pbp, pbp_size, pbp_hash);
    {
        int expected_size = snprintf(expected_manifest, sizeof(expected_manifest),
            "format=schwaystation-pbp-fixture-v1\n"
            "source=disc_boot_pbp.bin\n"
            "source_bytes=%zu\nsource_sha256=%s\n"
            "generator=gen_pbp.py\ngenerator_sha256=%s\n"
            "output=schwaystation-disc_boot.pbp\n"
            "output_bytes=%zu\noutput_sha256=%s\n"
            "sectors=%zu\nblocks=%zu\n",
            source_size, source_hash, generator_hash, pbp_size, pbp_hash,
            sector_count, block_count);
        if (expected_size < 0 || (size_t)expected_size >= sizeof(expected_manifest)) {
            (void)snprintf(error, error_size, "PBP expected manifest overflowed");
            goto done;
        }
        expected_manifest_size = (size_t)expected_size;
    }
    if (!read_file(manifest_path, sizeof(expected_manifest), &manifest, &manifest_size,
                   "could not read PBP manifest", error, error_size)) goto done;
    if (manifest_size != expected_manifest_size ||
        memcmp(manifest, expected_manifest, expected_manifest_size) != 0) {
        (void)snprintf(error, error_size, "PBP source, generator, or output manifest identity mismatch");
        goto done;
    }
    valid = 1;
done:
    free(source);
    free(generator);
    free(pbp);
    free(cue);
    free(manifest);
    return valid;
}

static int expect_fixture_failure(const char *label, const char *pbp_path,
                                  const char *cue_path, const char *source_path,
                                  const char *manifest_path, const char *generator_path,
                                  const char *expected, char *error, size_t error_size)
{
    error[0] = '\0';
    if (validate_fixture(pbp_path, cue_path, source_path, manifest_path,
                         generator_path, error, error_size) ||
        strstr(error, expected) == NULL) {
        fprintf(stderr, "ASSERT disc.image.pbp_fixture_controls: %s did not fail for %s (%s)\n",
                label, expected, error);
        return 0;
    }
    return 1;
}

static int run_fixture_controls(const char *directory, const char *pbp_path,
                                const char *cue_path, const char *source_path,
                                const char *manifest_path, const char *generator_path)
{
    const char *names[] = {
        "wrong-digest.manifest", "missing.pbp", "short-source.bin", "bad-track.cue",
        "bad-toc.pbp", "bad-overlap.pbp", "bad-extent.pbp", "over-cap.pbp",
        "wrong-generator.py", "control.manifest"
    };
    char paths[sizeof(names) / sizeof(names[0])][1024] = {{0}};
    char error[256];
    uint8_t *source = NULL;
    uint8_t *generator = NULL;
    uint8_t *pbp = NULL;
    uint8_t *manifest = NULL;
    size_t source_size = 0u;
    size_t generator_size = 0u;
    size_t pbp_size = 0u;
    size_t manifest_size = 0u;
    int result = 1;

    for (size_t i = 0u; i < sizeof(names) / sizeof(names[0]); i++) {
        if (!make_path(paths[i], directory, names[i])) return 1;
        (void)remove(paths[i]);
    }
    if (!read_file(source_path, SCHWAY__DISC_PBP_SOURCE_CAP_BYTES, &source, &source_size,
                   "could not read source for controls", error, sizeof(error)) ||
        !read_file(generator_path, 65536u, &generator, &generator_size,
                   "could not read generator for controls", error, sizeof(error)) ||
        !read_file(pbp_path, SCHWAY__DISC_PBP_FIXTURE_CAP_BYTES, &pbp, &pbp_size,
                   "could not read PBP for controls", error, sizeof(error)) ||
        !read_file(manifest_path, 1024u, &manifest, &manifest_size,
                   "could not read manifest for controls", error, sizeof(error))) {
        fprintf(stderr, "ASSERT disc.image.pbp_fixture_controls: %s\n", error);
        result = 1;
        goto done;
    }
    if (!write_bytes(paths[9], manifest, manifest_size)) {
        result = 1;
        goto done;
    }
    {
        uint8_t *bad_manifest = (uint8_t *)malloc(manifest_size);
        const char needle[] = "output_sha256=";
        int found = 0;
        if (bad_manifest == NULL) { result = 1; goto done; }
        memcpy(bad_manifest, manifest, manifest_size);
        for (size_t i = 0u; i + sizeof(needle) < manifest_size; i++) {
            if (memcmp(bad_manifest + i, needle, sizeof(needle) - 1u) == 0) {
                size_t value = i + sizeof(needle) - 1u;
                bad_manifest[value] = bad_manifest[value] == '0' ? '1' : '0';
                found = 1;
                break;
            }
        }
        if (!found || !write_bytes(paths[0], bad_manifest, manifest_size)) {
            free(bad_manifest); result = 1; goto done;
        }
        free(bad_manifest);
    }
    if (!expect_fixture_failure("wrong digest", pbp_path, cue_path, source_path,
            paths[0], generator_path, "manifest identity mismatch", error, sizeof(error)) ||
        !expect_fixture_failure("missing input", paths[1], cue_path, source_path,
            paths[9], generator_path, "could not read PBP output", error, sizeof(error))) {
        result = 1; goto done;
    }
    if (!write_bytes(paths[2], source, source_size - 1u)) { result = 1; goto done; }
    if (!expect_fixture_failure("wrong source count", pbp_path, cue_path, paths[2],
            paths[9], generator_path, "source sector count", error, sizeof(error))) {
        result = 1; goto done;
    }
    {
        static const char bad_cue[] = "FILE \"disc_boot_pbp.bin\" BINARY\nTRACK 02 AUDIO\n";
        if (!write_bytes(paths[3], (const uint8_t *)bad_cue, sizeof(bad_cue) - 1u) ||
            !expect_fixture_failure("wrong CUE track count", pbp_path, paths[3], source_path,
                paths[9], generator_path, "source CUE track count", error, sizeof(error))) {
            result = 1; goto done;
        }
    }
    {
        uint8_t *mutated = (uint8_t *)malloc(pbp_size);
        if (mutated == NULL) { result = 1; goto done; }
        memcpy(mutated, pbp, pbp_size);
        mutated[SCHWAY__DISC_PBP_TOC_OFFSET + 10u + 7u] = 0x02u;
        if (!write_bytes(paths[4], mutated, pbp_size) ||
            !write_manifest(paths[9], source, source_size, generator, generator_size,
                mutated, pbp_size, source_size / SCHWAY__DISC_PBP_SECTOR_BYTES,
                source_size / (SCHWAY__DISC_PBP_SECTOR_BYTES * SCHWAY__DISC_PBP_SECTORS_PER_BLOCK)) ||
            !expect_fixture_failure("wrong TOC count", paths[4], cue_path, source_path,
                paths[9], generator_path, "PBP TOC track count", error, sizeof(error))) {
            free(mutated); result = 1; goto done;
        }
        memcpy(mutated, pbp, pbp_size);
        put_le32(mutated + SCHWAY__DISC_PBP_INDEX_OFFSET + 32u, 0u);
        if (!write_bytes(paths[5], mutated, pbp_size) ||
            !write_manifest(paths[9], source, source_size, generator, generator_size,
                mutated, pbp_size, source_size / SCHWAY__DISC_PBP_SECTOR_BYTES,
                source_size / (SCHWAY__DISC_PBP_SECTOR_BYTES * SCHWAY__DISC_PBP_SECTORS_PER_BLOCK)) ||
            !expect_fixture_failure("overlapping extent", paths[5], cue_path, source_path,
                paths[9], generator_path, "PBP block extents", error, sizeof(error))) {
            free(mutated); result = 1; goto done;
        }
        memcpy(mutated, pbp, pbp_size);
        {
            size_t truncated_size = pbp_size - 1u;
            put_le32(mutated + SCHWAY__DISC_PBP_PSAR_OFFSET + 12u,
                (uint32_t)(truncated_size - SCHWAY__DISC_PBP_PSAR_OFFSET));
            put_le32(mutated + SCHWAY__DISC_PBP_PSAR_OFFSET + 16u,
                (uint32_t)(truncated_size - 0x6420u));
            put_le32(mutated + SCHWAY__DISC_PBP_PSAR_OFFSET + 0x1220u,
                (uint32_t)(truncated_size - 0xD2CFu));
            if (!write_bytes(paths[6], mutated, truncated_size) ||
            !write_manifest(paths[9], source, source_size, generator, generator_size,
                mutated, truncated_size, source_size / SCHWAY__DISC_PBP_SECTOR_BYTES,
                source_size / (SCHWAY__DISC_PBP_SECTOR_BYTES * SCHWAY__DISC_PBP_SECTORS_PER_BLOCK)) ||
            !expect_fixture_failure("truncated extent", paths[6], cue_path, source_path,
                paths[9], generator_path, "PBP block extent exceeds file", error, sizeof(error))) {
                free(mutated); result = 1; goto done;
            }
        }
        free(mutated);
    }
    {
        uint8_t *oversized = (uint8_t *)calloc((size_t)SCHWAY__DISC_PBP_FIXTURE_CAP_BYTES + 1u, 1u);
        if (oversized == NULL || !write_bytes(paths[7], oversized,
                (size_t)SCHWAY__DISC_PBP_FIXTURE_CAP_BYTES + 1u)) {
            free(oversized); result = 1; goto done;
        }
        free(oversized);
        if (!expect_fixture_failure("over-cap output", paths[7], cue_path, source_path,
                paths[9], generator_path, "PBP exceeds the 1.5 MiB cap", error, sizeof(error))) {
            result = 1; goto done;
        }
    }
    {
        uint8_t *bad_generator = (uint8_t *)malloc(generator_size + 1u);
        if (bad_generator == NULL) { result = 1; goto done; }
        memcpy(bad_generator, generator, generator_size);
        bad_generator[generator_size] = 0u;
        if (!write_bytes(paths[8], bad_generator, generator_size + 1u) ||
            !expect_fixture_failure("wrong generator identity", pbp_path, cue_path, source_path,
                paths[9], paths[8], "manifest identity mismatch", error, sizeof(error))) {
            free(bad_generator); result = 1; goto done;
        }
        free(bad_generator);
    }
    result = 0;
done:
    free(source);
    free(generator);
    free(pbp);
    free(manifest);
    for (size_t i = 0u; i < sizeof(names) / sizeof(names[0]); i++) {
        if (paths[i][0] != '\0') (void)remove(paths[i]);
    }
    return result;
}

static int make_path(char path[1024], const char *directory, const char *name)
{
    int size = snprintf(path, 1024u, "%s/%s", directory, name);
    return size >= 0 && (size_t)size < 1024u;
}

static int write_bytes(const char *path, const uint8_t *bytes, size_t size)
{
    FILE *file = schway__test_open_file(path, "wb");
    if (file == NULL) return 0;
    if (size != 0u && fwrite(bytes, 1u, size, file) != size) {
        (void)fclose(file);
        return 0;
    }
    return fclose(file) == 0;
}

static void put_le32(uint8_t bytes[4], uint32_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}

typedef struct schway__disc_pbp_test_memory {
    const uint8_t *bytes;
    size_t size;
} schway__disc_pbp_test_memory;

static schway_disc_image_status read_memory(void *context, uint64_t offset,
                                             uint8_t *bytes, size_t size)
{
    const schway__disc_pbp_test_memory *memory =
        (const schway__disc_pbp_test_memory *)context;
    if (memory == NULL || bytes == NULL || offset > memory->size ||
        size > memory->size - (size_t)offset) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    memcpy(bytes, memory->bytes + (size_t)offset, size);
    return SCHWAY_DISC_IMAGE_OK;
}

static void put_le16(uint8_t bytes[2], uint16_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
}

static int expect_metadata_status(const char *name, const uint8_t *bytes, size_t size,
                                  schway_disc_image_status expected)
{
    schway__disc_pbp_test_memory memory = { bytes, size };
    schway__disc_pbp_metadata_map map;
    schway_disc_image_status status;
    memset(&map, 0xa5, sizeof(map));
    status = schway__disc_pbp_metadata_parse(read_memory, &memory, (uint64_t)size, &map);
    if (status != expected || (status != SCHWAY_DISC_IMAGE_OK &&
        (map.blocks != NULL || map.sector_count != 0u || map.block_count != 0u))) {
        fprintf(stderr, "ASSERT disc.image.pbp_bounds: %s returned %u, expected %u\n",
                name, (unsigned int)status, (unsigned int)expected);
        schway__disc_pbp_metadata_clear(&map);
        return 0;
    }
    schway__disc_pbp_metadata_clear(&map);
    return 1;
}

static int expect_metadata_mutation(const char *name, const uint8_t *source, size_t size,
                                    size_t offset, const uint8_t *replacement,
                                    size_t replacement_size,
                                    schway_disc_image_status expected)
{
    uint8_t *mutated;
    int result;
    if (offset > size || replacement_size > size - offset) return 0;
    mutated = (uint8_t *)malloc(size);
    if (mutated == NULL) return 0;
    memcpy(mutated, source, size);
    memcpy(mutated + offset, replacement, replacement_size);
    result = expect_metadata_status(name, mutated, size, expected);
    free(mutated);
    return result;
}

static int expect_largest_toc(const uint8_t *source, size_t source_size)
{
    const uint32_t sector_count = 449840u;
    const uint32_t block_count = sector_count / SCHWAY__DISC_PBP_SECTORS_PER_BLOCK;
    const size_t expanded_size = (size_t)SCHWAY__DISC_PBP_DATA_BASE + block_count;
    uint8_t *expanded = (uint8_t *)calloc(expanded_size, 1u);
    schway__disc_pbp_test_memory memory;
    schway__disc_pbp_metadata_map map;
    schway_disc_image_status status;
    if (expanded == NULL || source_size < (size_t)SCHWAY__DISC_PBP_DATA_BASE) {
        free(expanded);
        return 0;
    }
    memcpy(expanded, source, (size_t)SCHWAY__DISC_PBP_DATA_BASE);
    expanded[SCHWAY__DISC_PBP_TOC_OFFSET + 27u] = 0x99u;
    expanded[SCHWAY__DISC_PBP_TOC_OFFSET + 28u] = 0x59u;
    expanded[SCHWAY__DISC_PBP_TOC_OFFSET + 29u] = 0x65u;
    put_le32(expanded + SCHWAY__DISC_PBP_PSAR_OFFSET + 12u,
        (uint32_t)(expanded_size - SCHWAY__DISC_PBP_PSAR_OFFSET));
    put_le32(expanded + SCHWAY__DISC_PBP_PSAR_OFFSET + 16u,
        (uint32_t)(expanded_size - 0x6420u));
    for (uint32_t i = 0u; i < block_count; i++) {
        uint8_t *entry = expanded + SCHWAY__DISC_PBP_INDEX_OFFSET + (size_t)i * 32u;
        put_le32(entry, i);
        put_le16(entry + 4u, 1u);
    }
    memory.bytes = expanded;
    memory.size = expanded_size;
    memset(&map, 0, sizeof(map));
    status = schway__disc_pbp_metadata_parse(read_memory, &memory,
                                               (uint64_t)expanded_size, &map);
    if (status != SCHWAY_DISC_IMAGE_OK || map.sector_count != sector_count ||
        map.block_count != block_count) {
        fprintf(stderr, "ASSERT disc.image.pbp_bounds: maximum legal TOC returned %u (%u sectors, %u blocks)\n",
                (unsigned int)status, map.sector_count, map.block_count);
        schway__disc_pbp_metadata_clear(&map);
        free(expanded);
        return 0;
    }
    schway__disc_pbp_metadata_clear(&map);
    free(expanded);
    return 1;
}

static int expect_open_status(const char *directory, const char *name,
                              const uint8_t *bytes, size_t size,
                              schway_disc_image_status expected)
{
    char path[1024];
    schway_disc_image_error error;
    schway__disc_pbp *reader = (schway__disc_pbp *)(uintptr_t)1u;
    schway_disc_image_status status;
    if (!make_path(path, directory, name) || !write_bytes(path, bytes, size)) return 0;
    memset(&error, 0, sizeof(error));
    status = schway__disc_pbp_open(path, &reader, &error);
    (void)remove(path);
    if (status != expected || reader != NULL || error.status != (uint32_t)expected ||
        error.size != (uint32_t)sizeof(error) || error.message[0] == '\0') {
        fprintf(stderr, "ASSERT disc.image.pbp_bounds: open %s returned %u, expected %u\n",
                name, (unsigned int)status, (unsigned int)expected);
        return 0;
    }
    return 1;
}

static int run_metadata(const char *pbp_path)
{
    uint8_t *bytes = NULL;
    size_t size = 0u;
    char error[256];
    schway__disc_pbp_test_memory memory;
    schway__disc_pbp_metadata_map map;
    schway_disc_image_error image_error;
    schway__disc_pbp *reader = (schway__disc_pbp *)(uintptr_t)1u;
    schway_toc toc;
    uint8_t decoded[SCHWAY__DISC_PBP_BLOCK_BYTES];
    uint8_t sector[SCHWAY__DISC_PBP_SECTOR_BYTES];
    uint8_t subcode[96];
    schway_disc_image_status status;
    if (!read_file(pbp_path, SCHWAY__DISC_PBP_FIXTURE_CAP_BYTES, &bytes, &size,
                   "could not read accepted PBP", error, sizeof(error))) {
        fprintf(stderr, "ASSERT disc.image.pbp_metadata: %s\n", error);
        return 1;
    }
    memory.bytes = bytes;
    memory.size = size;
    memset(&map, 0, sizeof(map));
    status = schway__disc_pbp_metadata_parse(read_memory, &memory, (uint64_t)size, &map);
    if (status != SCHWAY_DISC_IMAGE_OK || map.sector_count != 64u ||
        map.block_count != 4u || map.data_offset != SCHWAY__DISC_PBP_DATA_BASE ||
        map.track.type != SCHWAY_DISC_TRACK_MODE2 || map.track.index0 != 0u ||
        map.track.index1 != 0u || map.track.pregap != 0u || map.track.leadout != 64u) {
        fprintf(stderr, "ASSERT disc.image.pbp_metadata: fixture mapping status=%u sectors=%u blocks=%u leadout=%u\n",
                (unsigned int)status, map.sector_count, map.block_count, map.track.leadout);
        schway__disc_pbp_metadata_clear(&map);
        free(bytes);
        return 1;
    }
    for (uint32_t i = 1u; i < map.block_count; i++) {
        if (map.blocks[i].offset != map.blocks[i - 1u].offset + map.blocks[i - 1u].size) {
            fprintf(stderr, "ASSERT disc.image.pbp_metadata: block %u is not contiguous\n", i);
            schway__disc_pbp_metadata_clear(&map);
            free(bytes);
            return 1;
        }
    }
    memset(&image_error, 0, sizeof(image_error));
    status = schway__disc_pbp_open(pbp_path, &reader, &image_error);
    if (status != SCHWAY_DISC_IMAGE_OK || reader == NULL ||
        image_error.status != (uint32_t)status) {
        fprintf(stderr, "ASSERT disc.image.pbp_metadata: valid PBP open returned %u handle=%p\n",
                (unsigned int)status, (void *)reader);
        schway__disc_pbp_close(reader);
        schway__disc_pbp_metadata_clear(&map);
        free(bytes);
        return 1;
    }
    memset(&toc, 0, sizeof(toc));
    toc.size = (uint32_t)sizeof(toc);
    if (schway__disc_pbp_toc(reader, &toc) != SCHWAY_DISC_IMAGE_OK ||
        toc.track_count != 1u || toc.tracks[0].type != SCHWAY_DISC_TRACK_MODE2 ||
        toc.tracks[0].index0 != 0u || toc.tracks[0].index1 != 0u ||
        toc.tracks[0].pregap != 0u || toc.tracks[0].leadout != 64u) {
        fprintf(stderr, "ASSERT disc.image.pbp_metadata: normalized TOC is wrong\n");
        schway__disc_pbp_close(reader);
        schway__disc_pbp_metadata_clear(&map);
        free(bytes);
        return 1;
    }
    if (!schway__pbp_inflate_raw(bytes + map.data_offset + map.blocks[0].offset,
                                 map.blocks[0].size, decoded, sizeof(decoded)) ||
        schway__disc_pbp_read_sector(reader, 0u, sector, subcode) != 1u ||
        memcmp(sector, decoded, sizeof(sector)) != 0) {
        fprintf(stderr, "ASSERT disc.image.pbp_metadata: first raw sector differs from block\n");
        schway__disc_pbp_close(reader);
        schway__disc_pbp_metadata_clear(&map);
        free(bytes);
        return 1;
    }
    for (size_t i = 0u; i < sizeof(subcode); i++) {
        if (subcode[i] != 0u) {
            fprintf(stderr, "ASSERT disc.image.pbp_metadata: unavailable subcode is not zero\n");
            schway__disc_pbp_close(reader);
            schway__disc_pbp_metadata_clear(&map);
            free(bytes);
            return 1;
        }
    }
    if (!schway__pbp_inflate_raw(bytes + map.data_offset + map.blocks[3].offset,
                                 map.blocks[3].size, decoded, sizeof(decoded)) ||
        schway__disc_pbp_read_sector(reader, 63u, sector, subcode) != 1u ||
        memcmp(sector, decoded + 15u * SCHWAY__DISC_PBP_SECTOR_BYTES,
               sizeof(sector)) != 0) {
        fprintf(stderr, "ASSERT disc.image.pbp_metadata: final raw sector differs from block\n");
        schway__disc_pbp_close(reader);
        schway__disc_pbp_metadata_clear(&map);
        free(bytes);
        return 1;
    }
    memset(sector, 0xa5, sizeof(sector));
    memset(subcode, 0x5a, sizeof(subcode));
    if (schway__disc_pbp_read_sector(reader, 64u, sector, subcode) != 0u ||
        sector[0] != 0xa5u || sector[sizeof(sector) - 1u] != 0xa5u ||
        subcode[0] != 0x5au || subcode[sizeof(subcode) - 1u] != 0x5au) {
        fprintf(stderr, "ASSERT disc.image.pbp_metadata: leadout read modified outputs\n");
        schway__disc_pbp_close(reader);
        schway__disc_pbp_metadata_clear(&map);
        free(bytes);
        return 1;
    }
    schway__disc_pbp_close(reader);
    schway__disc_pbp_metadata_clear(&map);
    free(bytes);
    return 0;
}

static int run_raw_block(const char *pbp_path, const char *directory)
{
    uint8_t *source = NULL;
    uint8_t *raw_pbp = NULL;
    size_t source_size = 0u;
    size_t raw_size = (size_t)SCHWAY__DISC_PBP_DATA_BASE + SCHWAY__DISC_PBP_BLOCK_BYTES;
    char path[1024] = "";
    char error[256];
    schway__disc_pbp_test_memory memory;
    schway__disc_pbp_metadata_map map;
    schway__disc_pbp *reader = NULL;
    schway_disc_image_error image_error;
    schway_toc toc;
    uint8_t decoded[SCHWAY__DISC_PBP_BLOCK_BYTES];
    uint8_t sector[SCHWAY__DISC_PBP_SECTOR_BYTES];
    uint8_t subcode[96];
    schway_disc_image_status status;
    int result = 1;

    memset(&map, 0, sizeof(map));
    if (!read_file(pbp_path, SCHWAY__DISC_PBP_FIXTURE_CAP_BYTES, &source, &source_size,
                   "could not read PBP for raw-block control", error, sizeof(error))) {
        fprintf(stderr, "ASSERT disc.image.pbp_raw: %s\n", error);
        goto done;
    }
    memory.bytes = source;
    memory.size = source_size;
    status = schway__disc_pbp_metadata_parse(read_memory, &memory,
                                               (uint64_t)source_size, &map);
    if (status != SCHWAY_DISC_IMAGE_OK || map.block_count == 0u ||
        map.data_offset != SCHWAY__DISC_PBP_DATA_BASE ||
        source_size < (size_t)SCHWAY__DISC_PBP_DATA_BASE ||
        !schway__pbp_inflate_raw(source + map.data_offset + map.blocks[0].offset,
                                 map.blocks[0].size, decoded, sizeof(decoded))) {
        fprintf(stderr, "ASSERT disc.image.pbp_raw: source block could not be decoded (%u)\n",
                (unsigned int)status);
        goto done;
    }
    raw_pbp = (uint8_t *)calloc(raw_size, 1u);
    if (raw_pbp == NULL) {
        fprintf(stderr, "ASSERT disc.image.pbp_raw: could not allocate one-block PBP\n");
        goto done;
    }
    memcpy(raw_pbp, source, (size_t)SCHWAY__DISC_PBP_DATA_BASE);
    memcpy(raw_pbp + SCHWAY__DISC_PBP_DATA_BASE, decoded, sizeof(decoded));
    raw_pbp[SCHWAY__DISC_PBP_TOC_OFFSET + 27u] = 0x00u;
    raw_pbp[SCHWAY__DISC_PBP_TOC_OFFSET + 28u] = 0x02u;
    raw_pbp[SCHWAY__DISC_PBP_TOC_OFFSET + 29u] = 0x16u;
    put_le32(raw_pbp + SCHWAY__DISC_PBP_PSAR_OFFSET + 12u,
             (uint32_t)(raw_size - SCHWAY__DISC_PBP_PSAR_OFFSET));
    put_le32(raw_pbp + SCHWAY__DISC_PBP_PSAR_OFFSET + 16u,
             (uint32_t)(raw_size - 0x6420u));
    put_le32(raw_pbp + SCHWAY__DISC_PBP_PSAR_OFFSET + 0x1220u,
             (uint32_t)(raw_size - 0xD2CFu));
    {
        uint8_t *entry = raw_pbp + SCHWAY__DISC_PBP_INDEX_OFFSET;
        put_le32(entry, 0u);
        put_le16(entry + 4u, (uint16_t)SCHWAY__DISC_PBP_BLOCK_BYTES);
        put_le16(entry + 6u, 0u);
        memset(entry + 8u, 0, 24u);
    }
    if (!make_path(path, directory, "pbp-one-raw-block.pbp") ||
        !write_bytes(path, raw_pbp, raw_size)) {
        fprintf(stderr, "ASSERT disc.image.pbp_raw: could not write one-block PBP\n");
        goto done;
    }
    memset(&image_error, 0, sizeof(image_error));
    status = schway__disc_pbp_open(path, &reader, &image_error);
    if (status != SCHWAY_DISC_IMAGE_OK || reader == NULL ||
        image_error.status != (uint32_t)SCHWAY_DISC_IMAGE_OK) {
        fprintf(stderr, "ASSERT disc.image.pbp_raw: raw-block open returned %u (%s)\n",
                (unsigned int)status, image_error.message);
        goto done;
    }
    memset(&toc, 0, sizeof(toc));
    toc.size = (uint32_t)sizeof(toc);
    if (schway__disc_pbp_toc(reader, &toc) != SCHWAY_DISC_IMAGE_OK ||
        toc.track_count != 1u || toc.tracks[0].leadout != 16u) {
        fprintf(stderr, "ASSERT disc.image.pbp_raw: one-block TOC is invalid\n");
        goto done;
    }
    for (size_t i = 0u; i < 3u; i++) {
        static const uint32_t lbas[] = {0u, 7u, 15u};
        memset(sector, 0xa5, sizeof(sector));
        memset(subcode, 0x5a, sizeof(subcode));
        if (schway__disc_pbp_read_sector(reader, lbas[i], sector, subcode) != 1u ||
            memcmp(sector, decoded + (size_t)lbas[i] * SCHWAY__DISC_PBP_SECTOR_BYTES,
                   sizeof(sector)) != 0 || subcode[0] != 0u || subcode[95] != 0u) {
            fprintf(stderr, "ASSERT disc.image.pbp_raw: raw byte parity failed at LBA %u\n",
                    lbas[i]);
            goto done;
        }
    }
    result = 0;
done:
    schway__disc_pbp_close(reader);
    schway__disc_pbp_metadata_clear(&map);
    if (path[0] != '\0') (void)remove(path);
    free(raw_pbp);
    free(source);
    return result;
}

static int run_bounds(const char *pbp_path, const char *directory)
{
    uint8_t *source = NULL;
    uint8_t *mutated = NULL;
    size_t size = 0u;
    char error[256];
    const size_t toc = SCHWAY__DISC_PBP_TOC_OFFSET;
    const size_t psar = SCHWAY__DISC_PBP_PSAR_OFFSET;
    const size_t index = SCHWAY__DISC_PBP_INDEX_OFFSET;
    static const uint8_t zero[10] = { 0u };
    static const uint8_t bad_version[4] = { 0x27u, 0u, 0u, 0u };
    static const uint8_t bad_order[4] = { 0x28u, 0u, 0u, 0u };
    static const uint8_t outside[4] = { 0xffu, 0xffu, 0xffu, 0xffu };
    static const uint8_t bad_marker[1] = { 'X' };
    static const uint8_t pgd[12] = { 'P', 'G', 'D' };
    static const uint8_t unknown[12] = { 'U', 'N', 'K', 'N', 'O', 'W', 'N' };
    static const uint8_t multi_disc[12] = {
        'P', 'S', 'I', 'S', 'O', 'I', 'M', 'G', '0', '0', '0', '1'
    };
    static const uint8_t bad_bcd[1] = { 0xfau };
    static const uint8_t bad_second[1] = { 0x60u };
    static const uint8_t bad_frame[1] = { 0x75u };
    static const uint8_t unsupported_control[1] = { 0x01u };
    uint8_t track2[10] = { 0x41u, 0u, 0x02u, 0u, 0x02u, 0x01u, 0u, 0u, 0x02u, 0u };
    int result = 1;

    if (!read_file(pbp_path, SCHWAY__DISC_PBP_FIXTURE_CAP_BYTES, &source, &size,
                   "could not read PBP for bounds", error, sizeof(error))) {
        fprintf(stderr, "ASSERT disc.image.pbp_bounds: %s\n", error);
        return 1;
    }
    if (!expect_metadata_mutation("outer offset below header", source, size, 8u,
            bad_version, sizeof(bad_version), SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_metadata_mutation("outer offsets out of order", source, size, 8u + 2u * 4u,
            bad_order, sizeof(bad_order), SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_metadata_mutation("outer offset beyond file", source, size, 8u + 7u * 4u,
            outside, sizeof(outside), SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_metadata_mutation("malformed PSISO signature", source, size, psar + 11u,
            bad_marker, sizeof(bad_marker), SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_metadata_mutation("PGD marker", source, size, psar,
            pgd, sizeof(pgd), SCHWAY_DISC_IMAGE_UNSUPPORTED) ||
        !expect_metadata_mutation("unknown marker", source, size, psar,
            unknown, sizeof(unknown), SCHWAY_DISC_IMAGE_UNSUPPORTED) ||
        !expect_metadata_mutation("multi-disc marker", source, size, psar,
            multi_disc, sizeof(multi_disc), SCHWAY_DISC_IMAGE_UNSUPPORTED) ||
        !expect_metadata_mutation("bad A0 BCD", source, size, toc + 7u,
            bad_bcd, sizeof(bad_bcd), SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_metadata_mutation("bad track BCD", source, size, toc + 32u,
            bad_bcd, sizeof(bad_bcd), SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_metadata_mutation("bad A2 BCD", source, size, toc + 27u,
            bad_bcd, sizeof(bad_bcd), SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_metadata_mutation("bad A2 seconds", source, size, toc + 28u,
            bad_second, sizeof(bad_second), SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_metadata_mutation("bad A2 frames", source, size, toc + 29u,
            bad_frame, sizeof(bad_frame), SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_metadata_mutation("duplicate A0", source, size, toc + 40u,
            source + toc, 10u, SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_metadata_mutation("duplicate A1", source, size, toc + 40u,
            source + toc + 10u, 10u, SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_metadata_mutation("duplicate A2", source, size, toc + 40u,
            source + toc + 20u, 10u, SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_metadata_mutation("duplicate track", source, size, toc + 40u,
            source + toc + 30u, 10u, SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_metadata_mutation("missing A0", source, size, toc,
            zero, sizeof(zero), SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_metadata_mutation("missing A1", source, size, toc + 10u,
            zero, sizeof(zero), SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_metadata_mutation("missing A2", source, size, toc + 20u,
            zero, sizeof(zero), SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_metadata_mutation("missing track", source, size, toc + 30u,
            zero, sizeof(zero), SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_metadata_mutation("inconsistent leadout", source, size, toc + 29u,
            (const uint8_t[]){ 0x63u }, 1u, SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_metadata_mutation("unsupported control", source, size, toc + 30u,
            unsupported_control, sizeof(unsupported_control), SCHWAY_DISC_IMAGE_UNSUPPORTED) ||
        !expect_metadata_mutation("additional track", source, size, toc + 40u,
            track2, sizeof(track2), SCHWAY_DISC_IMAGE_UNSUPPORTED) ||
        !expect_largest_toc(source, size)) goto done;

    {
        schway__disc_pbp_test_memory memory = { source, size };
        schway__disc_pbp_metadata_map map;
        schway_disc_image_status status;
        memset(&map, 0xa5, sizeof(map));
        status = schway__disc_pbp_metadata_parse(read_memory, &memory,
            SCHWAY__MEDIA_IMAGE_MAX_BYTES + 1u, &map);
        if (status != SCHWAY_DISC_IMAGE_LIMIT || map.blocks != NULL ||
            map.block_count != 0u || map.sector_count != 0u) {
            fprintf(stderr, "ASSERT disc.image.pbp_bounds: advertised size over cap returned %u\n",
                    (unsigned int)status);
            schway__disc_pbp_metadata_clear(&map);
            goto done;
        }
    }

    mutated = (uint8_t *)malloc(size);
    if (mutated == NULL) goto done;
    memcpy(mutated, source, size);
    put_le32(mutated + index + 32u, get_le32(mutated + index));
    if (!expect_metadata_status("overlapping block", mutated, size,
                                SCHWAY_DISC_IMAGE_BAD_IMAGE)) goto done;
    memcpy(mutated, source, size);
    put_le32(mutated + index + 64u, get_le32(mutated + index));
    if (!expect_metadata_status("nonmonotonic block", mutated, size,
                                SCHWAY_DISC_IMAGE_BAD_IMAGE)) goto done;
    memcpy(mutated, source, size);
    put_le16(mutated + index + (size_t)(3u * 32u) + 4u, UINT16_C(0x9300));
    if (!expect_metadata_status("out-of-file extent", mutated, size,
                                SCHWAY_DISC_IMAGE_BAD_IMAGE)) goto done;
    if (!expect_open_status(directory, "pbp-invalid-metadata.bin", mutated, size,
                            SCHWAY_DISC_IMAGE_BAD_IMAGE)) goto done;
    memcpy(mutated, source, size);
    memcpy(mutated + psar, pgd, sizeof(pgd));
    if (!expect_open_status(directory, "pbp-unsupported-metadata.bin", mutated, size,
                            SCHWAY_DISC_IMAGE_UNSUPPORTED)) goto done;
    memcpy(mutated, source, size);
    memcpy(mutated + psar, unknown, sizeof(unknown));
    if (!expect_open_status(directory, "pbp-unknown-layout.bin", mutated, size,
                            SCHWAY_DISC_IMAGE_UNSUPPORTED)) goto done;
    memcpy(mutated, source, size);
    memcpy(mutated + psar, multi_disc, sizeof(multi_disc));
    if (!expect_open_status(directory, "pbp-multidisc-layout.bin", mutated, size,
                            SCHWAY_DISC_IMAGE_UNSUPPORTED)) goto done;
    memcpy(mutated, source, size);
    mutated[toc + 30u] = 0x01u;
    if (!expect_open_status(directory, "pbp-audio-track.bin", mutated, size,
                            SCHWAY_DISC_IMAGE_UNSUPPORTED)) goto done;
    memcpy(mutated, source, size);
    memcpy(mutated + toc + 40u, track2, sizeof(track2));
    if (!expect_open_status(directory, "pbp-extra-track.bin", mutated, size,
                            SCHWAY_DISC_IMAGE_UNSUPPORTED)) goto done;
    memcpy(mutated, source, size);
    mutated[SCHWAY__DISC_PBP_DATA_BASE + get_le32(source + index)] = 0x07u;
    if (!expect_open_status(directory, "pbp-invalid-deflate.bin", mutated, size,
                            SCHWAY_DISC_IMAGE_BAD_IMAGE)) goto done;
    for (int32_t shift = -1; shift <= 1; shift += 2) {
        const uint32_t block_count = 4u;
        uint16_t first_size = get_le16(source + index + 4u);
        size_t last_entry = index + (size_t)(block_count - 1u) * 32u;
        uint16_t last_size = get_le16(source + last_entry + 4u);
        memcpy(mutated, source, size);
        put_le16(mutated + index + 4u, (uint16_t)((int32_t)first_size + shift));
        for (uint32_t block = 1u; block < block_count; block++) {
            size_t entry = index + (size_t)block * 32u;
            uint32_t offset = get_le32(source + entry);
            put_le32(mutated + entry, (uint32_t)((int32_t)offset + shift));
        }
        put_le16(mutated + last_entry + 4u,
                 (uint16_t)((int32_t)last_size - shift));
        if (!expect_open_status(directory,
                shift < 0 ? "pbp-short-deflate.bin" : "pbp-long-deflate.bin",
                mutated, size, SCHWAY_DISC_IMAGE_BAD_IMAGE)) goto done;
    }
    memcpy(mutated, source, size);
    mutated[SCHWAY__DISC_PBP_DATA_BASE + get_le32(source + index + 3u * 32u)] = 0x07u;
    if (!expect_open_status(directory, "pbp-invalid-final-deflate.bin", mutated, size,
                            SCHWAY_DISC_IMAGE_BAD_IMAGE)) goto done;

    free(mutated);
    mutated = NULL;
    {
        size_t truncated_size = (size_t)SCHWAY__DISC_PBP_DATA_BASE - 1u;
        mutated = (uint8_t *)malloc(truncated_size);
        if (mutated == NULL) goto done;
        memcpy(mutated, source, truncated_size);
        put_le32(mutated + psar + 12u, (uint32_t)(truncated_size - psar));
        put_le32(mutated + psar + 16u, (uint32_t)(truncated_size - 0x6420u));
        if (!expect_metadata_status("truncated block table", mutated, truncated_size,
                                    SCHWAY_DISC_IMAGE_BAD_IMAGE)) goto done;
    }
    result = 0;
done:
    free(mutated);
    free(source);
    return result;
}

static void make_envelope(uint8_t bytes[SCHWAY__DISC_PBP_TEST_IMAGE_BYTES],
                          const uint8_t marker[SCHWAY__DISC_PBP_TEST_MARKER_BYTES])
{
    memset(bytes, 0, SCHWAY__DISC_PBP_TEST_IMAGE_BYTES);
    memcpy(bytes, "\0PBP", 4u);
    put_le32(bytes + 4u, UINT32_C(0x00010000));
    for (uint32_t i = 0u; i < 8u; i++) {
        put_le32(bytes + 8u + (size_t)i * 4u, SCHWAY__DISC_PBP_TEST_HEADER_BYTES);
    }
    memcpy(bytes + SCHWAY__DISC_PBP_TEST_HEADER_BYTES, marker,
           SCHWAY__DISC_PBP_TEST_MARKER_BYTES);
}

static int expect_rejected(const char *directory, const char *name,
                           const uint8_t *bytes, size_t size,
                           schway_disc_image_status expected)
{
    char path[1024];
    schway_disc_image_error error;
    schway__disc_pbp *reader = (schway__disc_pbp *)(uintptr_t)1u;
    schway_disc_image_status status;

    if (!make_path(path, directory, name)) {
        fprintf(stderr, "ASSERT disc.image.pbp_headers: path is too long for %s\n", name);
        return 0;
    }
    if (!write_bytes(path, bytes, size)) {
        fprintf(stderr, "ASSERT disc.image.pbp_headers: could not write %s\n", name);
        return 0;
    }
    memset(&error, 0, sizeof(error));
    status = schway__disc_pbp_open(path, &reader, &error);
    if (reader != NULL || status != expected || error.status != (uint32_t)expected ||
        error.size != (uint32_t)sizeof(error) || error.message[0] == '\0') {
        fprintf(stderr, "ASSERT disc.image.pbp_headers: %s returned %u, expected %u, output=%p\n",
                name, (unsigned int)status, (unsigned int)expected, (void *)reader);
        if (reader == NULL) schway__disc_pbp_close(reader);
        (void)remove(path);
        return 0;
    }
    schway__disc_pbp_close(reader);
    if (remove(path) != 0) {
        fprintf(stderr, "ASSERT disc.image.pbp_headers: could not remove %s\n", name);
        return 0;
    }
    return 1;
}

static int run_headers(const char *directory)
{
    static const uint8_t pgd_marker[SCHWAY__DISC_PBP_TEST_MARKER_BYTES] = {
        'P', 'G', 'D', 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u
    };
    static const uint8_t unknown_marker[SCHWAY__DISC_PBP_TEST_MARKER_BYTES] = {
        'U', 'N', 'K', 'N', 'O', 'W', 'N', '0', '0', '0', '0', '0'
    };
    static const uint8_t psiso_marker[SCHWAY__DISC_PBP_TEST_MARKER_BYTES] = {
        'P', 'S', 'I', 'S', 'O', 'I', 'M', 'G', '0', '0', '0', '0'
    };
    uint8_t short_header[SCHWAY__DISC_PBP_TEST_HEADER_BYTES - 1u] = { 0u };
    uint8_t wrong_magic[SCHWAY__DISC_PBP_TEST_IMAGE_BYTES] = { 0u };
    uint8_t envelope[SCHWAY__DISC_PBP_TEST_IMAGE_BYTES];

    memcpy(short_header, "\0PBP", 4u);
    if (!expect_rejected(directory, "pbp-empty.bin", NULL, 0u,
                         SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_rejected(directory, "pbp-short.bin", short_header, sizeof(short_header),
                         SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_rejected(directory, "pbp-wrong-magic.bin", wrong_magic, sizeof(wrong_magic),
                         SCHWAY_DISC_IMAGE_BAD_IMAGE)) return 1;

    make_envelope(envelope, pgd_marker);
    if (!expect_rejected(directory, "pbp-encrypted-marker.bin", envelope, sizeof(envelope),
                         SCHWAY_DISC_IMAGE_UNSUPPORTED)) return 1;
    make_envelope(envelope, unknown_marker);
    if (!expect_rejected(directory, "pbp-unknown-marker.bin", envelope, sizeof(envelope),
                         SCHWAY_DISC_IMAGE_UNSUPPORTED)) return 1;

    make_envelope(envelope, psiso_marker);
    put_le32(envelope + 8u, UINT32_C(0x40000001));
    if (!expect_rejected(directory, "pbp-oversized-offset.bin", envelope, sizeof(envelope),
                         SCHWAY_DISC_IMAGE_BAD_IMAGE)) return 1;
    make_envelope(envelope, psiso_marker);
    put_le32(envelope + 8u + 7u * 4u, UINT32_C(0xfffffff8));
    if (!expect_rejected(directory, "pbp-overflow-offset.bin", envelope, sizeof(envelope),
                         SCHWAY_DISC_IMAGE_BAD_IMAGE)) return 1;
    return 0;
}

static int expect_inflate(const char *name, const uint8_t *input, size_t input_size,
                          const uint8_t *expected, size_t expected_size,
                          size_t output_size, int expected_success)
{
    uint8_t output[128];
    size_t i;
    int success;

    memset(output, 0xa5, sizeof(output));
    success = schway__pbp_inflate_raw(input, input_size, output, output_size);
    if (success != expected_success) {
        fprintf(stderr, "ASSERT disc.image.pbp_inflate: %s returned %d, expected %d\n",
                name, success, expected_success);
        return 0;
    }
    if (success != 0) {
        if (expected_size != output_size || memcmp(output, expected, expected_size) != 0) {
            fprintf(stderr, "ASSERT disc.image.pbp_inflate: %s output differs\n", name);
            return 0;
        }
        for (i = expected_size; i < sizeof(output); i++) {
            if (output[i] != 0xa5u) {
                fprintf(stderr, "ASSERT disc.image.pbp_inflate: %s wrote past output\n", name);
                return 0;
            }
        }
    } else {
        for (i = 0u; i < sizeof(output); i++) {
            if (output[i] != 0xa5u) {
                fprintf(stderr, "ASSERT disc.image.pbp_inflate: %s changed output on failure\n", name);
                return 0;
            }
        }
    }
    return 1;
}

static int run_inflate(void)
{
    static const uint8_t stored_stream[] = {
        0x01u, 0x05u, 0x00u, 0xfau, 0xffu, 'h', 'e', 'l', 'l', 'o'
    };
    static const uint8_t stored_output[] = { 'h', 'e', 'l', 'l', 'o' };
    static const uint8_t huffman_stream[] = {
        0x2bu, 0x4eu, 0xceu, 0x28u, 0x4fu, 0xacu, 0x2cu, 0x2eu, 0x49u, 0x2cu,
        0xc9u, 0xccu, 0xcfu, 0x53u, 0x48u, 0xcau, 0x2fu, 0xcdu, 0x4bu, 0x49u,
        0x4du, 0x51u, 0x28u, 0x4au, 0x2cu, 0x57u, 0x70u, 0x71u, 0x75u, 0xf3u,
        0x71u, 0x0cu, 0x71u, 0x55u, 0x28u, 0xa6u, 0x82u, 0x0au, 0x00u
    };
    static const uint8_t huffman_output[] =
        "schwaystation bounded raw DEFLATE "
        "schwaystation bounded raw DEFLATE "
        "schwaystation bounded raw DEFLATE ";
    uint8_t trailing[sizeof(stored_stream) + 1u];
    size_t output_size = sizeof(huffman_output) - 1u;

    memcpy(trailing, stored_stream, sizeof(stored_stream));
    trailing[sizeof(stored_stream)] = 0u;
    if (!expect_inflate("stored block", stored_stream, sizeof(stored_stream),
                        stored_output, sizeof(stored_output), sizeof(stored_output), 1) ||
        !expect_inflate("fixed Huffman block", huffman_stream, sizeof(huffman_stream),
                        huffman_output, output_size, output_size, 1) ||
        !expect_inflate("trailing input", trailing, sizeof(trailing),
                        stored_output, sizeof(stored_output), sizeof(stored_output), 0) ||
        !expect_inflate("truncated input", huffman_stream, sizeof(huffman_stream) - 1u,
                        huffman_output, output_size, output_size, 0) ||
        !expect_inflate("short expected output", huffman_stream, sizeof(huffman_stream),
                        huffman_output, output_size, output_size - 1u, 0) ||
        !expect_inflate("long expected output", huffman_stream, sizeof(huffman_stream),
                        huffman_output, output_size, output_size + 1u, 0) ||
        !expect_inflate("output overflow", huffman_stream, sizeof(huffman_stream),
                        huffman_output, output_size, 10u, 0) ||
        !expect_inflate("empty input", stored_stream, 0u,
                        stored_output, sizeof(stored_output), sizeof(stored_output), 0) ||
        !expect_inflate("empty output", stored_stream, sizeof(stored_stream),
                        stored_output, sizeof(stored_output), 0u, 0)) return 1;
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--inflate") == 0) return run_inflate();
    if (argc == 3 && strcmp(argv[1], "--metadata") == 0) return run_metadata(argv[2]);
    if (argc == 4 && strcmp(argv[1], "--raw") == 0) {
        return run_raw_block(argv[2], argv[3]);
    }
    if (argc == 4 && strcmp(argv[1], "--bounds") == 0) {
        return run_bounds(argv[2], argv[3]);
    }
    if (argc == 7 && strcmp(argv[1], "--preflight") == 0) {
        char error[256];
        if (validate_fixture(argv[2], argv[3], argv[4], argv[5], argv[6],
                             error, sizeof(error))) return 0;
        fprintf(stderr, "ASSERT disc.image.pbp_preflight: %s\n", error);
        return 1;
    }
    if (argc == 8 && strcmp(argv[1], "--fixture-controls") == 0) {
        return run_fixture_controls(argv[2], argv[3], argv[4], argv[5], argv[6], argv[7]);
    }
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-disc-pbp <build-test-directory>|--inflate|--metadata <pbp>|--raw <pbp> <dir>|--bounds <pbp> <dir>|--preflight <pbp> <cue> <source> <manifest> <generator>|--fixture-controls <dir> <pbp> <cue> <source> <manifest> <generator>\n");
        return 2;
    }
    return run_headers(argv[1]);
}
