#include "disc_chd.h"
#include "disc_chd_metadata.h"
#include "media_limits.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if SCHWAYSTATION_CHD
#include <libchdr/chd.h>
#endif

#define SCHWAY__DISC_CHD_MAX_HUNK_BYTES UINT32_C(16777216)
#define SCHWAY__DISC_CHD_MAX_HUNKS UINT32_C(65536)
#define SCHWAY__DISC_CHD_MAX_MAP_BYTES UINT32_C(16777216)
#define SCHWAY__DISC_CHD_MAX_MAP_ALLOCATION_BYTES UINT32_C(17825792)
#define SCHWAY__DISC_CHD_MAP_ENTRY_BYTES UINT32_C(16)
#define SCHWAY__DISC_CHD_MEMORY_MAP_ENTRY_BYTES UINT32_C(32)
#define SCHWAY__DISC_CHD_METADATA_HEADER_BYTES UINT32_C(16)
#define SCHWAY__DISC_CHD_MAX_METADATA_BYTES \
    (SCHWAY_DISC_MAX_TRACKS * (SCHWAY__DISC_CHD_METADATA_TEXT_MAX + \
                               SCHWAY__DISC_CHD_METADATA_HEADER_BYTES))
#define SCHWAY__DISC_CHD_FRAME_BYTES UINT32_C(2448)

struct schway__disc_chd {
#if SCHWAYSTATION_CHD
    FILE *file;
    chd_file *chd;
    schway__disc_chd_metadata_map metadata;
    uint8_t *hunk_buffer;
    uint32_t hunk_bytes;
    uint32_t frames_per_hunk;
    uint32_t total_hunks;
    uint32_t cached_hunk;
    uint32_t cache_valid;
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
    uint64_t file_bytes;
    uint64_t metadata_offset;
    uint64_t map_offset;
    uint64_t map_start;
    uint64_t map_end;
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

static int schway__disc_chd_bytes_nonzero(const uint8_t *bytes, size_t size)
{
    for (size_t i = 0u; i < size; i++) {
        if (bytes[i] != 0u) return 1;
    }
    return 0;
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
    uint8_t map_header[16];
    uint64_t file_bytes;
    uint64_t rounded_hunks;
    uint64_t raw_map_bytes;
    uint64_t compressed_map_bytes = 0u;
    uint64_t map_allocation_bytes;
    uint64_t map_disk_bytes;
    uint64_t map_end;
    long end;
    uint32_t expected_length;
    uint32_t total_hunks = 0u;
    uint32_t version;
    uint32_t hunk_bytes;
    uint64_t logical_bytes;
    uint32_t map_entry_bytes;
    int compressed;

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
    if (file_bytes > SCHWAY__MEDIA_IMAGE_MAX_BYTES) {
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
    if (version < 3u || version > 5u) {
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_UNSUPPORTED,
                                      "CHD versions 3 through 5 are supported");
    }
    if (schway__disc_chd_be32(prefix + 8u) != expected_length ||
        file_bytes < expected_length ||
        !schway__disc_chd_seek_read(file, 0u, raw, expected_length)) {
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "CHD header length is invalid or truncated");
    }

    if (version <= 4u) {
        total_hunks = schway__disc_chd_be32(raw + 24u);
        logical_bytes = schway__disc_chd_be64(raw + 28u);
        bounds->metadata_offset = schway__disc_chd_be64(raw + 36u);
        hunk_bytes = schway__disc_chd_be32(raw + (version == 3u ? 76u : 44u));
        if ((schway__disc_chd_be32(raw + 16u) & 1u) != 0u ||
            (version == 3u &&
             (schway__disc_chd_bytes_nonzero(raw + 60u, 16u) != 0 ||
              schway__disc_chd_bytes_nonzero(raw + 100u, 20u) != 0)) ||
            (version == 4u && schway__disc_chd_bytes_nonzero(raw + 68u, 20u) != 0)) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_UNSUPPORTED,
                                          "CHD parent images are unsupported");
        }
    } else {
        logical_bytes = schway__disc_chd_be64(raw + 32u);
        bounds->map_offset = schway__disc_chd_be64(raw + 40u);
        bounds->metadata_offset = schway__disc_chd_be64(raw + 48u);
        hunk_bytes = schway__disc_chd_be32(raw + 56u);
        if (schway__disc_chd_bytes_nonzero(raw + 104u, 20u) != 0) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_UNSUPPORTED,
                                          "CHD parent images are unsupported");
        }
    }
    if (logical_bytes == 0u || hunk_bytes == 0u) {
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "CHD logical size and hunk size must be nonzero");
    }
    if (logical_bytes > SCHWAY__MEDIA_IMAGE_MAX_BYTES ||
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
    if (hunk_bytes % SCHWAY__DISC_CHD_FRAME_BYTES != 0u ||
        logical_bytes % SCHWAY__DISC_CHD_FRAME_BYTES != 0u) {
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "CHD hunk and logical sizes must align to CD frames");
    }
    if ((bounds->metadata_offset != 0u &&
         (bounds->metadata_offset < expected_length || bounds->metadata_offset >= file_bytes)) ||
        (version == 5u && bounds->map_offset < expected_length)) {
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "CHD map or metadata offset exceeds the file");
    }
    if (version < 5u) {
        bounds->map_start = expected_length;
        if (!schway__media_mul_u64(total_hunks, SCHWAY__DISC_CHD_MAP_ENTRY_BYTES,
                                   &map_disk_bytes) ||
            !schway__media_add_u64(expected_length, map_disk_bytes, &map_end) ||
            !schway__media_mul_u64(total_hunks,
                                   SCHWAY__DISC_CHD_MEMORY_MAP_ENTRY_BYTES,
                                   &map_allocation_bytes)) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                          "CHD map size overflows checked arithmetic");
        }
        if (map_allocation_bytes > SCHWAY__DISC_CHD_MAX_MAP_ALLOCATION_BYTES) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                          "CHD map allocation exceeds its limit");
        }
        if (map_end >= file_bytes ||
            (bounds->metadata_offset != 0u && bounds->metadata_offset < map_end)) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                          "CHD map extent is truncated or overlaps metadata");
        }
        bounds->map_end = map_end;
    } else {
        bounds->map_start = bounds->map_offset;
        compressed = schway__disc_chd_be32(raw + 16u) != 0u;
        map_entry_bytes = compressed != 0 ? 12u : 4u;
        if (!schway__media_mul_u64(rounded_hunks, map_entry_bytes, &raw_map_bytes) ||
            raw_map_bytes > SCHWAY__DISC_CHD_MAX_MAP_ALLOCATION_BYTES) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                          "CHD decoded map allocation exceeds its limit");
        }
        if (compressed != 0) {
            if (!schway__media_add_u64(bounds->map_offset, sizeof(map_header), &map_end) ||
                map_end > file_bytes ||
                !schway__disc_chd_seek_read(file, bounds->map_offset, map_header,
                                             sizeof(map_header))) {
                return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                              "CHD compressed map header is truncated");
            }
            compressed_map_bytes = schway__disc_chd_be32(map_header);
            if (compressed_map_bytes == 0u) {
                return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                              "CHD compressed map is empty");
            }
            if (compressed_map_bytes > SCHWAY__DISC_CHD_MAX_MAP_BYTES) {
                return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                              "CHD compressed map exceeds its limit");
            }
            if (map_header[12] > 32u || map_header[13] > 32u || map_header[14] > 32u ||
                !schway__media_add_u64(sizeof(map_header), compressed_map_bytes,
                                       &map_disk_bytes) ||
                !schway__media_add_u64(bounds->map_offset, map_disk_bytes, &map_end)) {
                return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                              "CHD compressed map fields are invalid");
            }
        } else {
            map_disk_bytes = raw_map_bytes;
            if (!schway__media_add_u64(bounds->map_offset, map_disk_bytes, &map_end)) {
                return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                              "CHD uncompressed map extent overflows");
            }
        }
        if (!schway__media_add_u64(raw_map_bytes, compressed_map_bytes,
                                   &map_allocation_bytes) ||
            map_allocation_bytes > SCHWAY__DISC_CHD_MAX_MAP_ALLOCATION_BYTES) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                          "CHD map allocation total exceeds its limit");
        }
        if ((compressed != 0 ? map_end > file_bytes : map_end >= file_bytes) ||
            (bounds->metadata_offset != 0u && bounds->metadata_offset >= bounds->map_offset &&
             bounds->metadata_offset < map_end)) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                          "CHD map extent is truncated or overlaps metadata");
        }
        bounds->map_end = map_end;
    }
    bounds->logical_bytes = logical_bytes;
    bounds->file_bytes = file_bytes;
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

static schway_disc_image_status schway__disc_chd_preflight_metadata(
    FILE *file, const schway__disc_chd_bounds *bounds,
    schway_disc_image_error *error)
{
    uint64_t seen[SCHWAY_DISC_MAX_TRACKS];
    uint64_t offset = bounds->metadata_offset;
    uint64_t total_bytes = 0u;
    uint32_t expected_tag = 0u;
    uint32_t count = 0u;

    while (offset != 0u) {
        uint8_t header[SCHWAY__DISC_CHD_METADATA_HEADER_BYTES];
        uint8_t terminator;
        uint64_t data_offset;
        uint64_t record_end;
        uint64_t record_bytes;
        uint64_t next;
        uint32_t tag;
        uint32_t text_length;

        if (count >= SCHWAY_DISC_MAX_TRACKS) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                          "CHD metadata has more than 99 records");
        }
        for (uint32_t i = 0u; i < count; i++) {
            if (seen[i] == offset) {
                return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                              "CHD metadata chain contains a cycle");
            }
        }
        if (offset < bounds->header_length || offset > bounds->file_bytes ||
            bounds->file_bytes - offset < sizeof(header) ||
            !schway__disc_chd_seek_read(file, offset, header, sizeof(header))) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                          "CHD metadata header is outside the file");
        }
        tag = schway__disc_chd_be32(header);
        text_length = schway__disc_chd_be32(header + 4u) & UINT32_C(0x00ffffff);
        next = schway__disc_chd_be64(header + 8u);
        if (tag != SCHWAY__DISC_CHD_TAG_CHT2 && tag != SCHWAY__DISC_CHD_TAG_CHTR) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_UNSUPPORTED,
                                          "CHD contains unsupported non-track metadata");
        }
        if (expected_tag == 0u) expected_tag = tag;
        if (tag != expected_tag) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_UNSUPPORTED,
                                          "CHD mixes track metadata formats");
        }
        if (text_length < 2u) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                          "CHD metadata text is truncated");
        }
        if (text_length > SCHWAY__DISC_CHD_METADATA_TEXT_MAX) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                          "CHD metadata text exceeds its limit");
        }
        if (!schway__media_add_u64(offset, sizeof(header), &data_offset) ||
            !schway__media_add_u64(data_offset, text_length, &record_end) ||
            record_end > bounds->file_bytes ||
            (offset < bounds->map_end && record_end > bounds->map_start)) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                          "CHD metadata extent is truncated or overlaps its map");
        }
        if (!schway__disc_chd_seek_read(file, record_end - 1u, &terminator, 1u) ||
            terminator != 0u) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                          "CHD metadata text is not terminated");
        }
        if (!schway__media_add_u64(sizeof(header), text_length, &record_bytes) ||
            !schway__media_add_u64(total_bytes, record_bytes, &total_bytes) ||
            total_bytes > SCHWAY__DISC_CHD_MAX_METADATA_BYTES) {
            return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                          "CHD metadata total exceeds its limit");
        }
        seen[count++] = offset;
        offset = next;
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
    status = schway__disc_chd_preflight_metadata(file, &bounds, error);
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
        header.totalhunks != bounds.total_hunks || header.metaoffset != bounds.metadata_offset ||
        (bounds.version == 5u && header.mapoffset != bounds.map_offset)) {
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
    reader->hunk_buffer = (uint8_t *)malloc(bounds.hunk_bytes);
    if (reader->hunk_buffer == NULL) {
        chd_close(chd);
        (void)fclose(file);
        free(reader);
        return schway__disc_chd_error(error, SCHWAY_DISC_IMAGE_OUT_OF_MEMORY,
                                      "could not allocate the bounded CHD hunk cache");
    }
    reader->file = file;
    reader->chd = chd;
    reader->metadata = metadata;
    reader->hunk_bytes = bounds.hunk_bytes;
    reader->frames_per_hunk = bounds.hunk_bytes / SCHWAY__DISC_CHD_FRAME_BYTES;
    reader->total_hunks = bounds.total_hunks;
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
    free(reader->hunk_buffer);
#endif
    free(reader);
}

uint32_t schway__disc_chd_map_lba(const schway__disc_chd_metadata_map *map,
                                  uint32_t lba, uint32_t *frame,
                                  uint32_t *track_type,
                                  uint32_t *virtual_pregap)
{
    if (frame == NULL || track_type == NULL || virtual_pregap == NULL) return 0u;
    *frame = 0u;
    *track_type = 0u;
    *virtual_pregap = 0u;
    if (map == NULL || map->track_count == 0u ||
        map->track_count > SCHWAY_DISC_MAX_TRACKS) return 0u;

    for (uint32_t i = 0u; i < map->track_count; i++) {
        const schway__disc_chd_track_map *track = &map->tracks[i];
        uint64_t relative;
        uint64_t absolute;
        if (lba < track->toc.index0 || lba >= track->toc.leadout) continue;
        if (track->toc.index0 > track->toc.index1 ||
            track->toc.index1 > track->toc.leadout ||
            track->toc.pregap != track->toc.index1 - track->toc.index0 ||
            track->stored_pregap > 1u) return 0u;
        *track_type = track->toc.type;
        if (lba < track->toc.index1 && track->stored_pregap == 0u) {
            *virtual_pregap = 1u;
            return 1u;
        }
        if (track->stored_pregap != 0u) {
            relative = (uint64_t)lba - track->toc.index0;
        } else {
            relative = (uint64_t)lba - track->toc.index1;
        }
        if (relative >= track->chd_frame_count) return 0u;
        absolute = (uint64_t)track->chd_frame_start + relative;
        if (absolute >= map->chd_frame_count || absolute > UINT32_MAX) return 0u;
        *frame = (uint32_t)absolute;
        return 1u;
    }
    return 0u;
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
#if SCHWAYSTATION_CHD
    schway__disc_chd *reader = (schway__disc_chd *)user;
    uint32_t frame;
    uint32_t track_type;
    uint32_t virtual_pregap;
    uint32_t hunk_index;
    uint32_t frame_in_hunk;
    uint32_t byte_offset;
    if (reader == NULL || sector == NULL || reader->chd == NULL ||
        reader->hunk_buffer == NULL || reader->frames_per_hunk == 0u ||
        !schway__disc_chd_map_lba(&reader->metadata, lba, &frame,
                                  &track_type, &virtual_pregap)) return 0u;
    if (virtual_pregap != 0u) {
        schway__disc_image_make_virtual_sector(track_type, lba, sector);
        if (subcode != NULL) memset(subcode, 0, 96u);
        return 1u;
    }
    hunk_index = frame / reader->frames_per_hunk;
    frame_in_hunk = frame % reader->frames_per_hunk;
    if (hunk_index >= reader->total_hunks ||
        frame_in_hunk > (reader->hunk_bytes - SCHWAY__DISC_CHD_FRAME_BYTES) /
                        SCHWAY__DISC_CHD_FRAME_BYTES) return 0u;
    byte_offset = frame_in_hunk * SCHWAY__DISC_CHD_FRAME_BYTES;
    if (reader->cache_valid == 0u || reader->cached_hunk != hunk_index) {
        reader->cache_valid = 0u;
        if (chd_read(reader->chd, hunk_index, reader->hunk_buffer) != CHDERR_NONE) return 0u;
        reader->cached_hunk = hunk_index;
        reader->cache_valid = 1u;
    }
    memcpy(sector, reader->hunk_buffer + byte_offset, 2352u);
    if (track_type == SCHWAY_DISC_TRACK_AUDIO) {
        for (uint32_t i = 0u; i < 2352u; i += 2u) {
            uint8_t first = sector[i];
            sector[i] = sector[i + 1u];
            sector[i + 1u] = first;
        }
    }
    if (subcode != NULL) memcpy(subcode, reader->hunk_buffer + byte_offset + 2352u, 96u);
    return 1u;
#else
    (void)user;
    (void)lba;
    (void)sector;
    (void)subcode;
    return 0u;
#endif
}
