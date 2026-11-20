#include "disc_chd.h"
#include "disc_chd_metadata.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if SCHWAYSTATION_CHD
#include <libchdr/chd.h>
#endif

#define SCHWAY__DISC_CHD_MAX_BYTES UINT64_C(1073741824)
#define SCHWAY__DISC_CHD_MAX_HUNK_BYTES UINT32_C(16777216)
#define SCHWAY__DISC_CHD_MAX_HUNKS UINT32_C(65536)

struct schway__disc_chd {
#if SCHWAYSTATION_CHD
    FILE *file;
    chd_file *chd;
    schway__disc_chd_metadata_map metadata;
#else
    uint32_t unavailable;
#endif
};

static schway_disc_image_status schway__disc_chd_error(
    schway_disc_image_error *error, schway_disc_image_status status,
    const char *message)
{
    if (error != NULL) {
        memset(error, 0, sizeof(*error));
        error->size = (uint32_t)sizeof(*error);
        error->status = (uint32_t)status;
        (void)snprintf(error->message, sizeof(error->message), "%s", message);
    }
    return status;
}

#if SCHWAYSTATION_CHD
typedef struct schway__disc_chd_bounds {
    uint64_t logical_bytes;
    uint64_t metadata_offset;
    uint64_t map_offset;
    uint32_t hunk_bytes;
    uint32_t total_hunks;
    uint32_t version;
    uint32_t header_length;
} schway__disc_chd_bounds;

static uint32_t schway__disc_chd_be32(const uint8_t *bytes)
{
    return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) |
           ((uint32_t)bytes[2] << 8) | (uint32_t)bytes[3];
}

static uint64_t schway__disc_chd_be64(const uint8_t *bytes)
{
    return ((uint64_t)schway__disc_chd_be32(bytes) << 32) |
           (uint64_t)schway__disc_chd_be32(bytes + 4);
}

static int schway__disc_chd_mul_u64(uint64_t left, uint64_t right,
                                     uint64_t *out)
{
    if (out == NULL || (right != 0u && left > UINT64_MAX / right)) return 0;
    *out = left * right;
    return 1;
}

static uint32_t schway__disc_chd_header_size(uint32_t version)
{
    switch (version) {
    case 1u: return CHD_V1_HEADER_SIZE;
    case 2u: return CHD_V2_HEADER_SIZE;
    case 3u: return CHD_V3_HEADER_SIZE;
    case 4u: return CHD_V4_HEADER_SIZE;
    case 5u: return CHD_V5_HEADER_SIZE;
    default: return 0u;
    }
}

static int schway__disc_chd_seek_read(FILE *file, uint64_t offset,
                                       uint8_t *bytes, size_t size)
{
    if (offset > (uint64_t)LONG_MAX || fseek(file, (long)offset, SEEK_SET) != 0) return 0;
    return fread(bytes, 1u, size, file) == size;
}

static schway_disc_image_status schway__disc_chd_preflight(
    FILE *file, schway__disc_chd_bounds *bounds,
    schway_disc_image_error *error)
{
    uint8_t raw[CHD_MAX_HEADER_SIZE];
    uint8_t prefix[16];
    uint64_t file_bytes;
    uint64_t rounded_hunks;
    uint64_t legacy_hunk_bytes;
    long end;
    uint32_t expected_length;
    uint32_t total_hunks = 0u;
    uint32_t version;
    uint32_t hunk_bytes;
    uint64_t logical_bytes;

    if (fseek(file, 0L, SEEK_END) != 0 || (end = ftell(file)) < 0 ||
        fseek(file, 0L, SEEK_SET) != 0) {
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_IO,
                                      "could not determine CHD file size");
    }
    file_bytes = (uint64_t)end;
    if (file_bytes < sizeof(prefix)) {
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "CHD header is truncated");
    }
    if (file_bytes > SCHWAY__DISC_CHD_MAX_BYTES) {
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                      "CHD file exceeds the 1 GiB limit");
    }
    if (fread(prefix, 1u, sizeof(prefix), file) != sizeof(prefix)) {
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_IO,
                                      "could not read CHD header prefix");
    }
    if (memcmp(prefix, "MComprHD", 8u) != 0) {
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "CHD header magic is invalid");
    }
    expected_length = schway__disc_chd_header_size(schway__disc_chd_be32(prefix + 12u));
    version = schway__disc_chd_be32(prefix + 12u);
    if (expected_length == 0u) {
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_UNSUPPORTED,
                                      "CHD version is unsupported");
    }
    if (schway__disc_chd_be32(prefix + 8u) != expected_length ||
        file_bytes < expected_length ||
        !schway__disc_chd_seek_read(file, 0u, raw, expected_length)) {
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "CHD header length is invalid or truncated");
    }

    if (version <= 2u) {
        uint64_t sector_bytes = version == 1u ? UINT64_C(512) :
                                (uint64_t)schway__disc_chd_be32(raw + 76u);
        uint64_t hunk_sectors = (uint64_t)schway__disc_chd_be32(raw + 24u);
        uint64_t geometry = (uint64_t)schway__disc_chd_be32(raw + 32u);
        if (!schway__disc_chd_mul_u64(geometry,
                (uint64_t)schway__disc_chd_be32(raw + 36u), &geometry) ||
            !schway__disc_chd_mul_u64(geometry,
                (uint64_t)schway__disc_chd_be32(raw + 40u), &geometry) ||
            !schway__disc_chd_mul_u64(geometry, sector_bytes, &logical_bytes) ||
            !schway__disc_chd_mul_u64(hunk_sectors, sector_bytes,
                &legacy_hunk_bytes)) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                          "CHD legacy geometry overflows checked arithmetic");
        }
        if (legacy_hunk_bytes > UINT32_MAX) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                          "CHD legacy hunk size exceeds its integer range");
        }
        hunk_bytes = (uint32_t)legacy_hunk_bytes;
        total_hunks = schway__disc_chd_be32(raw + 28u);
    } else if (version <= 4u) {
        total_hunks = schway__disc_chd_be32(raw + 24u);
        logical_bytes = schway__disc_chd_be64(raw + 28u);
        bounds->metadata_offset = schway__disc_chd_be64(raw + 36u);
        hunk_bytes = schway__disc_chd_be32(raw + (version == 3u ? 76u : 44u));
    } else {
        logical_bytes = schway__disc_chd_be64(raw + 32u);
        bounds->map_offset = schway__disc_chd_be64(raw + 40u);
        bounds->metadata_offset = schway__disc_chd_be64(raw + 48u);
        hunk_bytes = schway__disc_chd_be32(raw + 56u);
    }
    if (logical_bytes == 0u || hunk_bytes == 0u) {
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "CHD logical size and hunk size must be nonzero");
    }
    if (logical_bytes > SCHWAY__DISC_CHD_MAX_BYTES ||
        hunk_bytes > SCHWAY__DISC_CHD_MAX_HUNK_BYTES) {
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                      "CHD logical size or hunk exceeds its limit");
    }
    rounded_hunks = logical_bytes / hunk_bytes +
                    (logical_bytes % hunk_bytes != 0u ? 1u : 0u);
    if (rounded_hunks == 0u || rounded_hunks > SCHWAY__DISC_CHD_MAX_HUNKS) {
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                      "CHD hunk count exceeds the 65536 hunk limit");
    }
    if (version < 5u && total_hunks != rounded_hunks) {
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "CHD hunk count does not match its logical size");
    }
    if ((bounds->metadata_offset != 0u && bounds->metadata_offset >= file_bytes) ||
        (bounds->map_offset != 0u && bounds->map_offset >= file_bytes)) {
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "CHD map or metadata offset exceeds the file");
    }
    bounds->logical_bytes = logical_bytes;
    bounds->hunk_bytes = hunk_bytes;
    bounds->total_hunks = version == 5u ? (uint32_t)rounded_hunks : total_hunks;
    bounds->version = version;
    bounds->header_length = expected_length;
    return SCHWAY_DISC_IMAGE_OK;
}

static FILE *schway__disc_chd_open_file(const char *path)
{
#if defined(_MSC_VER)
    FILE *file = NULL;
    if (fopen_s(&file, path, "rb") != 0) return NULL;
    return file;
#else
    return fopen(path, "rb");
#endif
}

static schway_disc_image_status schway__disc_chd_read_metadata(
    chd_file *chd, const schway__disc_chd_bounds *bounds,
    schway__disc_chd_metadata_map *map, schway_disc_image_error *error)
{
    schway__disc_chd_metadata_record records[SCHWAY_DISC_MAX_TRACKS];
    char text[SCHWAY_DISC_MAX_TRACKS][SCHWAY__DISC_CHD_METADATA_TEXT_MAX];
    uint32_t count = 0u;

    memset(records, 0, sizeof(records));
    memset(text, 0, sizeof(text));
    for (uint32_t index = 0u; index <= SCHWAY_DISC_MAX_TRACKS; index++) {
        char candidate[SCHWAY__DISC_CHD_METADATA_TEXT_MAX] = { 0 };
        uint32_t length = 0u;
        uint32_t tag = 0u;
        chd_error result = chd_get_metadata(chd, CHDMETATAG_WILDCARD, index,
            candidate, sizeof(candidate), &length, &tag, NULL);

        if (result == CHDERR_METADATA_NOT_FOUND) break;
        if (result != CHDERR_NONE) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                          "CHD track metadata cannot be read");
        }
        if (count >= SCHWAY_DISC_MAX_TRACKS) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                          "CHD has more than 99 tracks");
        }
        if (tag != CDROM_TRACK_METADATA2_TAG && tag != CDROM_TRACK_METADATA_TAG) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_UNSUPPORTED,
                                          "CHD contains unsupported non-track metadata");
        }
        memcpy(text[count], candidate, sizeof(candidate));
        if (length < 2u || length > SCHWAY__DISC_CHD_METADATA_TEXT_MAX ||
            text[count][length - 1u] != '\0') {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                          "CHD track metadata is oversized or unterminated");
        }
        records[count].tag = tag == CDROM_TRACK_METADATA2_TAG ?
            SCHWAY__DISC_CHD_TAG_CHT2 : SCHWAY__DISC_CHD_TAG_CHTR;
        records[count].text = text[count];
        records[count].length = length;
        count++;
    }

    schway_disc_image_status status = schway__disc_chd_metadata_parse(records, count,
        bounds->version, bounds->hunk_bytes, bounds->logical_bytes, map);
    if (status == SCHWAY_DISC_IMAGE_UNSUPPORTED) {
        return schway__disc_chd_error(error, status,
                                      "CHD track metadata is outside the supported CD subset");
    }
    if (status != SCHWAY_DISC_IMAGE_OK) {
        return schway__disc_chd_error(error, status,
                                      "CHD track metadata is malformed or inconsistent");
    }
    return SCHWAY_DISC_IMAGE_OK;
}
#endif

schway_disc_image_status schway__disc_chd_open(const char *path,
                                                schway__disc_chd **out,
                                                schway_disc_image_error *error)
{
    if (out != NULL) *out = NULL;
    if (path == NULL || path[0] == '\0' || out == NULL) {
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_ARGUMENT,
                                      "CHD path and output are required");
    }
#if SCHWAYSTATION_CHD
    schway__disc_chd *reader;
    schway__disc_chd_bounds bounds;
    chd_header header;
    chd_file *chd = NULL;
    schway__disc_chd_metadata_map metadata;
    FILE *file = schway__disc_chd_open_file(path);
    if (file == NULL) {
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_IO,
                                      "could not open CHD file");
    }
    memset(&bounds, 0, sizeof(bounds));
    schway_disc_image_status status = schway__disc_chd_preflight(file, &bounds, error);
    if (status != SCHWAY_DISC_IMAGE_OK) {
        (void)fclose(file);
        return status;
    }
    chd_error chd_status = chd_read_header_file(file, &header);
    if (chd_status != CHDERR_NONE) {
        (void)fclose(file);
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
            chd_error_string(chd_status));
    }
    if (header.version != bounds.version || header.length != bounds.header_length ||
        header.logicalbytes != bounds.logical_bytes || header.hunkbytes != bounds.hunk_bytes ||
        header.totalhunks != bounds.total_hunks) {
        (void)fclose(file);
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "CHD header fields disagree with bounded geometry");
    }
    reader = (schway__disc_chd *)calloc(1u, sizeof(*reader));
    if (reader == NULL) {
        (void)fclose(file);
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_OUT_OF_MEMORY,
                                      "could not allocate CHD reader");
    }
    chd_status = chd_open_file(file, CHD_OPEN_READ, NULL, &chd);
    if (chd_status != CHDERR_NONE) {
        if (chd != NULL) chd_close(chd);
        (void)fclose(file);
        free(reader);
        schway_disc_image_status open_status = chd_status == CHDERR_OUT_OF_MEMORY ?
            SCHWAY_DISC_IMAGE_OUT_OF_MEMORY : SCHWAY_DISC_IMAGE_BAD_IMAGE;
        return schway__disc_chd_error(error, open_status, chd_error_string(chd_status));
    }
    status = schway__disc_chd_read_metadata(chd, &bounds, &metadata, error);
    if (status != SCHWAY_DISC_IMAGE_OK) {
        chd_close(chd);
        (void)fclose(file);
        free(reader);
        return status;
    }
    reader->file = file;
    reader->chd = chd;
    reader->metadata = metadata;
    *out = reader;
    return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_OK, "");
#else
    (void)path;
    return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_UNSUPPORTED,
                                  "CHD support is disabled in this build");
#endif
}

void schway__disc_chd_close(schway__disc_chd *reader)
{
    if (reader == NULL) return;
#if SCHWAYSTATION_CHD
    if (reader->chd != NULL) chd_close(reader->chd);
    if (reader->file != NULL) (void)fclose(reader->file);
#endif
    free(reader);
}

schway_disc_image_status schway__disc_chd_toc(const schway__disc_chd *reader,
                                               schway_toc *out)
{
#if SCHWAYSTATION_CHD
    schway_toc toc;
    if (reader == NULL || out == NULL || out->size != sizeof(*out)) {
        return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    }
    memset(&toc, 0, sizeof(toc));
    toc.size = (uint32_t)sizeof(toc);
    toc.track_count = reader->metadata.track_count;
    for (uint32_t i = 0u; i < toc.track_count; i++) {
        toc.tracks[i] = reader->metadata.tracks[i].toc;
    }
    *out = toc;
    return SCHWAY_DISC_IMAGE_OK;
#else
    if (reader == NULL || out == NULL || out->size != sizeof(*out)) {
        return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    }
    return SCHWAY_DISC_IMAGE_UNSUPPORTED;
#endif
}

uint32_t schway__disc_chd_read_sector(void *user, uint32_t lba,
                                      uint8_t sector[2352], uint8_t subcode[96])
{
    (void)user;
    (void)lba;
    (void)sector;
    (void)subcode;
    return 0u;
}
