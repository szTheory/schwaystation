/* Bounded replay and fuzzing through the production PBP image reader. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disc_image.h"
#include "disc_pbp_metadata.h"
#include "media_limits.h"
#include "pbp_inflate.h"
#include "../test_open.h"
#ifdef SCHWAY_FUZZ_REPLAY
#include "seeds.h"
#endif

#ifndef SCHWAY_FUZZ_WORK_DIR
#error "CMake must provide a build-owned PBP fuzz directory"
#endif

#ifndef SCHWAY_FUZZ_PBP_FIXTURE
#error "CMake must provide the generated owned PBP fixture path"
#endif

#define INPUT_MAX 256u
#define PATCH_BYTES 5u
#define SCHWAY__FUZZ_PBP_FILE_CAP 1572864u
#define SCHWAY__FUZZ_PBP_BLOCK_BYTES UINT32_C(0x9300)
#define SCHWAY__FUZZ_PBP_PSAR_OFFSET UINT32_C(0x10000)
#define SCHWAY__FUZZ_PBP_TOC_OFFSET UINT32_C(0x10800)
#define SCHWAY__FUZZ_PBP_INDEX_OFFSET UINT32_C(0x14000)
#define SCHWAY__FUZZ_PBP_DATA_OFFSET UINT32_C(0x110000)
#define SCHWAY__FUZZ_PBP_TRACK_BYTES UINT32_C(2352)
#define SCHWAY__FUZZ_PBP_SECTORS_PER_BLOCK 16u
#define SCHWAY__FUZZ_PBP_MAX_PATCHES ((INPUT_MAX - 1u) / PATCH_BYTES)

static uint8_t *compressed_image;
static size_t compressed_size;
static uint8_t *raw_image;
static size_t raw_size;
static char compressed_path[1024];
static char raw_path[1024];
static int initialized;

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

static void broken(const char *what)
{
    fprintf(stderr, "fuzz.regress.pbp: %s\n", what);
    abort();
}

#ifdef SCHWAY_FUZZ_REPLAY
static uint16_t get_le16(const uint8_t *bytes)
{
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}
#endif

static uint32_t get_le32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static void put_le16(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
}

static void put_le32(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}

struct fuzz_memory {
    const uint8_t *bytes;
    size_t size;
};

static schway_disc_image_status read_memory(void *context, uint64_t offset,
                                             uint8_t *bytes, size_t size)
{
    const struct fuzz_memory *memory = (const struct fuzz_memory *)context;
    if (memory == NULL || bytes == NULL || offset > (uint64_t)memory->size ||
        (uint64_t)size > (uint64_t)memory->size - offset) {
        return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    }
    memcpy(bytes, memory->bytes + (size_t)offset, size);
    return SCHWAY_DISC_IMAGE_OK;
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

static int read_file(const char *path, uint8_t **bytes, size_t *size)
{
    FILE *file = schway__test_open_file(path, "rb");
    long end;
    uint8_t *data;
    if (file == NULL) return 0;
    if (fseek(file, 0L, SEEK_END) != 0 || (end = ftell(file)) < 0L ||
        (unsigned long)end > SCHWAY__FUZZ_PBP_FILE_CAP || fseek(file, 0L, SEEK_SET) != 0) {
        (void)fclose(file);
        return 0;
    }
    data = (uint8_t *)malloc(end == 0L ? 1u : (size_t)end);
    if (data == NULL) {
        (void)fclose(file);
        return 0;
    }
    if (end != 0L && fread(data, 1u, (size_t)end, file) != (size_t)end) {
        free(data);
        (void)fclose(file);
        return 0;
    }
    if (fclose(file) != 0) {
        free(data);
        return 0;
    }
    *bytes = data;
    *size = (size_t)end;
    return 1;
}

static void initialize(void)
{
    struct fuzz_memory memory;
    schway__disc_pbp_metadata_map map;
    uint8_t decoded[SCHWAY__FUZZ_PBP_BLOCK_BYTES];
    int length;
    if (initialized != 0) return;
    length = snprintf(compressed_path, sizeof(compressed_path), "%s/input-compressed.pbp",
                      SCHWAY_FUZZ_WORK_DIR);
    if (length < 0 || (size_t)length >= sizeof(compressed_path)) {
        broken("compressed input path did not fit");
    }
    length = snprintf(raw_path, sizeof(raw_path), "%s/input-raw.pbp", SCHWAY_FUZZ_WORK_DIR);
    if (length < 0 || (size_t)length >= sizeof(raw_path)) broken("raw input path did not fit");
    if (!read_file(SCHWAY_FUZZ_PBP_FIXTURE, &compressed_image, &compressed_size) ||
        compressed_size <= SCHWAY__FUZZ_PBP_DATA_OFFSET) {
        broken("owned compressed fixture was missing or outside its cap");
    }
    memory.bytes = compressed_image;
    memory.size = compressed_size;
    memset(&map, 0, sizeof(map));
    if (schway__disc_pbp_metadata_parse(
            read_memory, &memory,
            (uint64_t)compressed_size, &map) != SCHWAY_DISC_IMAGE_OK ||
        map.data_offset != SCHWAY__FUZZ_PBP_DATA_OFFSET || map.block_count == 0u ||
        map.blocks[0].offset != 0u ||
        !schway__pbp_inflate_raw(compressed_image + map.data_offset + map.blocks[0].offset,
                                 map.blocks[0].size, decoded, sizeof(decoded))) {
        schway__disc_pbp_metadata_clear(&map);
        broken("owned compressed fixture did not produce a valid first block");
    }
    raw_size = (size_t)map.data_offset + sizeof(decoded);
    if (raw_size > SCHWAY__FUZZ_PBP_FILE_CAP) {
        schway__disc_pbp_metadata_clear(&map);
        broken("one-block raw fixture exceeded the media cap");
    }
    raw_image = (uint8_t *)calloc(raw_size, 1u);
    if (raw_image == NULL) {
        schway__disc_pbp_metadata_clear(&map);
        broken("could not allocate one-block raw fixture");
    }
    memcpy(raw_image, compressed_image, (size_t)map.data_offset);
    memcpy(raw_image + map.data_offset, decoded, sizeof(decoded));
    raw_image[SCHWAY__FUZZ_PBP_TOC_OFFSET + 27u] = 0x00u;
    raw_image[SCHWAY__FUZZ_PBP_TOC_OFFSET + 28u] = 0x02u;
    raw_image[SCHWAY__FUZZ_PBP_TOC_OFFSET + 29u] = 0x16u;
    put_le32(raw_image + SCHWAY__FUZZ_PBP_PSAR_OFFSET + 12u,
             (uint32_t)(raw_size - SCHWAY__FUZZ_PBP_PSAR_OFFSET));
    put_le32(raw_image + SCHWAY__FUZZ_PBP_PSAR_OFFSET + 16u,
             (uint32_t)(raw_size - UINT32_C(0x6420)));
    put_le32(raw_image + SCHWAY__FUZZ_PBP_PSAR_OFFSET + UINT32_C(0x1220),
             (uint32_t)(raw_size - UINT32_C(0xD2CF)));
    memset(raw_image + SCHWAY__FUZZ_PBP_INDEX_OFFSET, 0, 32u);
    put_le16(raw_image + SCHWAY__FUZZ_PBP_INDEX_OFFSET + 4u,
             (uint16_t)SCHWAY__FUZZ_PBP_BLOCK_BYTES);
    if (!write_file(compressed_path, compressed_image, compressed_size) ||
        !write_file(raw_path, raw_image, raw_size)) {
        schway__disc_pbp_metadata_clear(&map);
        broken("could not prepare build-owned raw/compressed files");
    }
    schway__disc_pbp_metadata_clear(&map);
    initialized = 1;
}

static int apply_patches(const char *path, const uint8_t *data, size_t size,
                         size_t image_size, uint32_t offsets[SCHWAY__FUZZ_PBP_MAX_PATCHES],
                         size_t *patch_count)
{
    FILE *file;
    size_t count = size > 0u ? (size - 1u) / PATCH_BYTES : 0u;
    if (count > SCHWAY__FUZZ_PBP_MAX_PATCHES) count = SCHWAY__FUZZ_PBP_MAX_PATCHES;
    *patch_count = 0u;
    if (count == 0u) return 1;
    file = schway__test_open_file(path, "r+b");
    if (file == NULL) return 0;
    for (size_t i = 0u; i < count; i++) {
        const uint8_t *patch = data + 1u + i * PATCH_BYTES;
        uint32_t offset = get_le32(patch) % (uint32_t)image_size;
        if (fseek(file, (long)offset, SEEK_SET) != 0 || fputc(patch[4], file) == EOF) {
            (void)fclose(file);
            return 0;
        }
        offsets[*patch_count] = offset;
        (*patch_count)++;
    }
    return fclose(file) == 0;
}

static int restore_patches(const char *path, const uint8_t *baseline,
                           const uint32_t offsets[SCHWAY__FUZZ_PBP_MAX_PATCHES],
                           size_t patch_count)
{
    FILE *file = schway__test_open_file(path, "r+b");
    if (file == NULL) return 0;
    for (size_t i = 0u; i < patch_count; i++) {
        if (fseek(file, (long)offsets[i], SEEK_SET) != 0 ||
            fputc(baseline[offsets[i]], file) == EOF) {
            (void)fclose(file);
            return 0;
        }
    }
    return fclose(file) == 0;
}

static int valid_toc(const schway_disc *disc, schway_toc *toc)
{
    if (disc == NULL || disc->toc == NULL || toc == NULL) return 0;
    memset(toc, 0, sizeof(*toc));
    toc->size = (uint32_t)sizeof(*toc);
    if (disc->toc(disc->user, toc) == 0u || toc->track_count != 1u) return 0;
    return toc->tracks[0].size == sizeof(toc->tracks[0]) &&
           toc->tracks[0].type == SCHWAY_DISC_TRACK_MODE2 &&
           toc->tracks[0].index0 <= toc->tracks[0].index1 &&
           toc->tracks[0].index1 < toc->tracks[0].leadout;
}

static int subcode_is_zero(const uint8_t subcode[96])
{
    for (size_t i = 0u; i < 96u; i++) {
        if (subcode[i] != 0u) return 0;
    }
    return 1;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    uint32_t offsets[SCHWAY__FUZZ_PBP_MAX_PATCHES];
    size_t patch_count = 0u;
    const uint8_t *baseline;
    size_t baseline_size;
    const char *path;
    uint8_t mode;
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    schway_disc_image_status status;
    if (size > INPUT_MAX || (size != 0u && data == NULL)) return 0;
    initialize();
    mode = size == 0u ? 0u : (uint8_t)(data[0] & 1u);
    baseline = mode == 0u ? compressed_image : raw_image;
    baseline_size = mode == 0u ? compressed_size : raw_size;
    path = mode == 0u ? compressed_path : raw_path;
    if (!apply_patches(path, data, size, baseline_size, offsets, &patch_count)) {
        broken("could not apply bounded mutation script");
    }
    memset(&error, 0, sizeof(error));
    status = schway_disc_image_open(path, &image, &error);
    if (status == SCHWAY_DISC_IMAGE_OK) {
        const schway_disc *disc = schway_disc_image_disc(image);
        schway_toc toc;
        schway_toc repeated_toc;
        uint8_t first_sector[SCHWAY__FUZZ_PBP_TRACK_BYTES];
        uint8_t second_sector[SCHWAY__FUZZ_PBP_TRACK_BYTES];
        uint8_t first_subcode[96];
        uint8_t second_subcode[96];
        uint8_t beyond_sector[SCHWAY__FUZZ_PBP_TRACK_BYTES];
        uint8_t beyond_subcode[96];
        uint32_t first_read;
        uint32_t second_read;
        if (disc == NULL || disc->read_sector == NULL || !valid_toc(disc, &toc) ||
            !valid_toc(disc, &repeated_toc) ||
            memcmp(&toc, &repeated_toc, sizeof(toc)) != 0) {
            schway_disc_image_close(image);
            broken("accepted PBP returned an invalid or unstable TOC");
        }
        memset(first_sector, 0xa5, sizeof(first_sector));
        memset(second_sector, 0xa5, sizeof(second_sector));
        memset(first_subcode, 0x5a, sizeof(first_subcode));
        memset(second_subcode, 0x5a, sizeof(second_subcode));
        first_read = disc->read_sector(disc->user, toc.tracks[0].index1,
                                       first_sector, first_subcode);
        second_read = disc->read_sector(disc->user, toc.tracks[0].index1,
                                        second_sector, second_subcode);
        if (first_read == 0u || second_read == 0u ||
            memcmp(first_sector, second_sector, sizeof(first_sector)) != 0 ||
            memcmp(first_subcode, second_subcode, sizeof(first_subcode)) != 0 ||
            !subcode_is_zero(first_subcode)) {
            schway_disc_image_close(image);
            broken("accepted PBP changed a repeated sector read or nonzero subcode");
        }
        memset(beyond_sector, 0xa5, sizeof(beyond_sector));
        memset(beyond_subcode, 0x5a, sizeof(beyond_subcode));
        if (disc->read_sector(disc->user, toc.tracks[0].leadout,
                              beyond_sector, beyond_subcode) != 0u ||
            beyond_sector[0] != 0xa5u || beyond_sector[sizeof(beyond_sector) - 1u] != 0xa5u ||
            beyond_subcode[0] != 0x5au || beyond_subcode[95] != 0x5au) {
            schway_disc_image_close(image);
            broken("accepted PBP read beyond leadout or changed failure outputs");
        }
        schway_disc_image_close(image);
        image = NULL;
    } else if (image != NULL ||
               (status != SCHWAY_DISC_IMAGE_BAD_IMAGE &&
                status != SCHWAY_DISC_IMAGE_UNSUPPORTED &&
                status != SCHWAY_DISC_IMAGE_LIMIT)) {
        schway_disc_image_close(image);
        broken("rejected PBP returned an unexpected status or retained an image");
    }
    if (!restore_patches(path, baseline, offsets, patch_count)) {
        schway_disc_image_close(image);
        broken("could not restore the build-owned baseline after mutation");
    }
    return 0;
}

#ifdef SCHWAY_FUZZ_REPLAY
uint32_t seeds_count(void)
{
    return 14u;
}

const char *seeds_name(uint32_t index)
{
    static const char *const names[] = {
        "valid_compressed", "valid_raw", "pgd", "unknown_marker", "multidisc",
        "bad_bcd", "bad_section_offset", "bad_table_offset", "short_stream",
        "long_stream", "corrupt_first", "corrupt_final", "truncated_extent",
        "overlong_final",
    };
    return index < seeds_count() ? names[index] : "invalid";
}

static size_t add_patch(uint8_t *bytes, size_t at, uint32_t offset, uint8_t value)
{
    put_le32(bytes + at, offset);
    bytes[at + 4u] = value;
    return at + PATCH_BYTES;
}

static size_t add_le16_patch(uint8_t *bytes, size_t at, uint32_t offset, uint16_t value)
{
    at = add_patch(bytes, at, offset, (uint8_t)value);
    return add_patch(bytes, at, offset + 1u, (uint8_t)(value >> 8));
}

static size_t add_le32_patch(uint8_t *bytes, size_t at, uint32_t offset, uint32_t value)
{
    for (uint32_t i = 0u; i < 4u; i++) {
        at = add_patch(bytes, at, offset + i, (uint8_t)(value >> (i * 8u)));
    }
    return at;
}

size_t seeds_build(uint32_t index, uint8_t *bytes)
{
    schway__disc_pbp_metadata_map map;
    uint32_t block_size;
    uint32_t last_block_offset;
    uint32_t last_block_size;
    size_t at = 1u;
    if (bytes == NULL || index >= seeds_count()) return 0u;
    initialize();
    memset(bytes, 0, SEEDS_SIZE_MAX);
    bytes[0] = index == 1u ? 1u : 0u;
    if (index < 2u) return 1u;
    memset(&map, 0, sizeof(map));
    {
        struct fuzz_memory memory = { compressed_image, compressed_size };
        if (schway__disc_pbp_metadata_parse(
                read_memory, &memory,
                (uint64_t)compressed_size, &map) != SCHWAY_DISC_IMAGE_OK ||
            map.block_count < 2u) {
            schway__disc_pbp_metadata_clear(&map);
            broken("could not rebuild indexed fixture metadata for seeds");
        }
    }
    block_size = get_le16(compressed_image + SCHWAY__FUZZ_PBP_INDEX_OFFSET + 4u);
    last_block_offset = map.blocks[map.block_count - 1u].offset;
    last_block_size = map.blocks[map.block_count - 1u].size;
    switch (index) {
    case 2u:
        at = add_patch(bytes, at, SCHWAY__FUZZ_PBP_PSAR_OFFSET, 'P');
        at = add_patch(bytes, at, SCHWAY__FUZZ_PBP_PSAR_OFFSET + 1u, 'G');
        at = add_patch(bytes, at, SCHWAY__FUZZ_PBP_PSAR_OFFSET + 2u, 'D');
        break;
    case 3u:
        at = add_patch(bytes, at, SCHWAY__FUZZ_PBP_PSAR_OFFSET, 'X');
        break;
    case 4u:
        at = add_patch(bytes, at, SCHWAY__FUZZ_PBP_TOC_OFFSET + 7u, 0x02u);
        break;
    case 5u:
        at = add_patch(bytes, at, SCHWAY__FUZZ_PBP_TOC_OFFSET + 27u, 0xfau);
        break;
    case 6u:
        at = add_patch(bytes, at, 36u, 0xffu);
        break;
    case 7u:
        at = add_patch(bytes, at, SCHWAY__FUZZ_PBP_INDEX_OFFSET + 32u, 0x01u);
        break;
    case 8u:
    case 9u: {
        int32_t shift = index == 8u ? -1 : 1;
        uint16_t first_size = (uint16_t)((int32_t)block_size + shift);
        at = add_le16_patch(bytes, at, SCHWAY__FUZZ_PBP_INDEX_OFFSET + 4u, first_size);
        for (uint32_t i = 1u; i < map.block_count; i++) {
            uint32_t entry_offset = SCHWAY__FUZZ_PBP_INDEX_OFFSET + i * 32u;
            uint32_t block_offset = map.blocks[i].offset;
            at = add_le32_patch(bytes, at, entry_offset,
                                (uint32_t)((int32_t)block_offset + shift));
        }
        at = add_le16_patch(bytes, at,
            SCHWAY__FUZZ_PBP_INDEX_OFFSET + (map.block_count - 1u) * 32u + 4u,
            (uint16_t)((int32_t)last_block_size - shift));
        break;
    }
    case 10u:
        at = add_patch(bytes, at, SCHWAY__FUZZ_PBP_DATA_OFFSET + map.blocks[0].offset,
                       (uint8_t)(compressed_image[SCHWAY__FUZZ_PBP_DATA_OFFSET] ^ 0xffu));
        break;
    case 11u:
        at = add_patch(bytes, at, SCHWAY__FUZZ_PBP_DATA_OFFSET + last_block_offset,
                       (uint8_t)(compressed_image[SCHWAY__FUZZ_PBP_DATA_OFFSET +
                                                   last_block_offset] ^ 0xffu));
        break;
    case 12u:
        at = add_patch(bytes, at, SCHWAY__FUZZ_PBP_PSAR_OFFSET + 12u,
                       (uint8_t)(compressed_image[SCHWAY__FUZZ_PBP_PSAR_OFFSET + 12u] - 1u));
        break;
    case 13u:
        at = add_le16_patch(bytes, at,
            SCHWAY__FUZZ_PBP_INDEX_OFFSET + (map.block_count - 1u) * 32u + 4u,
            (uint16_t)(last_block_size + 1u));
        break;
    default:
        schway__disc_pbp_metadata_clear(&map);
        return 0u;
    }
    schway__disc_pbp_metadata_clear(&map);
    return at;
}

#define SCHWAY_REPLAY_NAME "fuzz.regress.pbp"
#include "replay_main.h"
#endif
