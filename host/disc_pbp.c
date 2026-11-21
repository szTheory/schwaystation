#if !defined(_WIN32) && !defined(_XOPEN_SOURCE)
#define _XOPEN_SOURCE 700
#endif
#include "disc_pbp.h"
#include "disc_pbp_metadata.h"
#include "media_limits.h"
#include "pbp_inflate.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32)
#include <sys/types.h>
#endif

#define SCHWAY__DISC_PBP_BLOCK_BYTES UINT32_C(0x9300)
#define SCHWAY__DISC_PBP_SECTOR_BYTES UINT32_C(2352)
#define SCHWAY__DISC_PBP_SECTORS_PER_BLOCK 16u

struct schway__disc_pbp {
    FILE *file;
    schway__disc_pbp_metadata_map map;
    uint8_t *compressed;
    uint8_t decoded[SCHWAY__DISC_PBP_BLOCK_BYTES];
    uint32_t cached_block;
    uint8_t cache_valid;
};

static schway_disc_image_status schway__disc_pbp_error(
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

static int schway__disc_pbp_file_size(FILE *file, uint64_t *size)
{
#if defined(_WIN32)
    __int64 end;
    if (_fseeki64(file, 0, SEEK_END) != 0 ||
        (end = _ftelli64(file)) < 0 || _fseeki64(file, 0, SEEK_SET) != 0) {
        return 0;
    }
#else
    off_t end;
    if (fseeko(file, 0, SEEK_END) != 0 ||
        (end = ftello(file)) < (off_t)0 || fseeko(file, 0, SEEK_SET) != 0) {
        return 0;
    }
#endif
    *size = (uint64_t)end;
    return 1;
}

static schway_disc_image_status schway__disc_pbp_read_at(
    void *context, uint64_t offset, uint8_t *bytes, size_t size)
{
    FILE *file = (FILE *)context;
    if (file == NULL || bytes == NULL || offset > (uint64_t)INT64_MAX) {
        return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    }
#if defined(_WIN32)
    if (_fseeki64(file, (__int64)offset, SEEK_SET) != 0) return SCHWAY_DISC_IMAGE_IO;
#else
    if (fseeko(file, (off_t)offset, SEEK_SET) != 0) return SCHWAY_DISC_IMAGE_IO;
#endif
    if (fread(bytes, 1u, size, file) == size) return SCHWAY_DISC_IMAGE_OK;
    return ferror(file) != 0 ? SCHWAY_DISC_IMAGE_IO : SCHWAY_DISC_IMAGE_BAD_IMAGE;
}

static int schway__disc_pbp_decode_block(schway__disc_pbp *reader,
                                          uint32_t block_index)
{
    const schway__disc_pbp_block *block;
    uint64_t file_offset;
    if (reader == NULL || block_index >= reader->map.block_count) return 0;
    if (reader->cache_valid != 0u && reader->cached_block == block_index) return 1;
    block = &reader->map.blocks[block_index];
    if (block->size == 0u || block->size > SCHWAY__DISC_PBP_BLOCK_BYTES ||
        !schway__media_add_u64(reader->map.data_offset, (uint64_t)block->offset,
                               &file_offset) ||
        schway__disc_pbp_read_at(reader->file, file_offset, reader->compressed,
                                 (size_t)block->size) != SCHWAY_DISC_IMAGE_OK) {
        return 0;
    }
    if (block->size == SCHWAY__DISC_PBP_BLOCK_BYTES) {
        memcpy(reader->decoded, reader->compressed, sizeof(reader->decoded));
    } else if (!schway__pbp_inflate_raw(reader->compressed, (size_t)block->size,
                                       reader->decoded, sizeof(reader->decoded))) {
        return 0;
    }
    reader->cached_block = block_index;
    reader->cache_valid = 1u;
    return 1;
}

schway_disc_image_status schway__disc_pbp_open(const char *path,
                                                schway__disc_pbp **out,
                                                schway_disc_image_error *error)
{
    schway__disc_pbp_metadata_map map;
    schway_disc_image_status status;
    uint64_t file_bytes;
    uint64_t logical_bytes;
    uint64_t scanned_bytes = 0u;
    FILE *file;
    schway__disc_pbp *reader;

    if (out != NULL) *out = NULL;
    if (path == NULL || path[0] == '\0' || out == NULL) {
        return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_BAD_ARGUMENT,
                                      "PBP path and output are required");
    }
    file = fopen(path, "rb");
    if (file == NULL) {
        return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_IO,
                                      "could not open PBP file");
    }
    if (!schway__disc_pbp_file_size(file, &file_bytes)) {
        (void)fclose(file);
        return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_IO,
                                      "could not determine PBP file size");
    }
    if (file_bytes > SCHWAY__MEDIA_IMAGE_MAX_BYTES) {
        (void)fclose(file);
        return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                      "PBP file exceeds the 1 GiB limit");
    }
    memset(&map, 0, sizeof(map));
    status = schway__disc_pbp_metadata_parse(schway__disc_pbp_read_at, file,
                                              file_bytes, &map);
    if (status != SCHWAY_DISC_IMAGE_OK) {
        schway__disc_pbp_metadata_clear(&map);
        (void)fclose(file);
        return schway__disc_pbp_error(error, status,
            status == SCHWAY_DISC_IMAGE_UNSUPPORTED ?
            "PBP layout is unsupported" : "PBP metadata is invalid");
    }
    if (map.block_count == 0u ||
        !schway__media_mul_u64((uint64_t)map.sector_count,
                               SCHWAY__DISC_PBP_SECTOR_BYTES, &logical_bytes) ||
        logical_bytes > SCHWAY__MEDIA_IMAGE_MAX_BYTES) {
        schway__disc_pbp_metadata_clear(&map);
        (void)fclose(file);
        return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                      "PBP logical image exceeds the 1 GiB limit");
    }
    reader = (schway__disc_pbp *)calloc(1u, sizeof(*reader));
    if (reader == NULL) {
        schway__disc_pbp_metadata_clear(&map);
        (void)fclose(file);
        return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_OUT_OF_MEMORY,
                                      "could not allocate PBP reader");
    }
    reader->file = file;
    reader->map = map;
    memset(&map, 0, sizeof(map));
    reader->compressed = (uint8_t *)malloc(SCHWAY__DISC_PBP_BLOCK_BYTES);
    if (reader->compressed == NULL) {
        schway__disc_pbp_close(reader);
        return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_OUT_OF_MEMORY,
                                      "could not allocate PBP block input");
    }
    for (uint32_t i = 0u; i < reader->map.block_count; i++) {
        if (!schway__media_add_u64(scanned_bytes, SCHWAY__DISC_PBP_BLOCK_BYTES,
                                   &scanned_bytes) || scanned_bytes > logical_bytes ||
            !schway__disc_pbp_decode_block(reader, i)) {
            schway__disc_pbp_close(reader);
            return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                          "PBP data block is truncated or invalid");
        }
    }
    if (scanned_bytes != logical_bytes ||
        reader->map.block_count != reader->map.sector_count /
                                   SCHWAY__DISC_PBP_SECTORS_PER_BLOCK) {
        schway__disc_pbp_close(reader);
        return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "PBP block count does not match its TOC");
    }
    reader->cache_valid = 0u;
    *out = reader;
    return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_OK, "ok");
}

void schway__disc_pbp_close(schway__disc_pbp *reader)
{
    if (reader == NULL) return;
    if (reader->file != NULL) (void)fclose(reader->file);
    schway__disc_pbp_metadata_clear(&reader->map);
    free(reader->compressed);
    free(reader);
}

schway_disc_image_status schway__disc_pbp_toc(const schway__disc_pbp *reader,
                                               schway_toc *out)
{
    schway_toc candidate;
    if (reader == NULL || out == NULL || out->size != sizeof(*out)) {
        return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    }
    memset(&candidate, 0, sizeof(candidate));
    candidate.size = (uint32_t)sizeof(candidate);
    candidate.track_count = 1u;
    candidate.tracks[0] = reader->map.track;
    *out = candidate;
    return SCHWAY_DISC_IMAGE_OK;
}

uint32_t schway__disc_pbp_read_sector(void *user, uint32_t lba,
                                      uint8_t sector[2352], uint8_t subcode[96])
{
    schway__disc_pbp *reader = (schway__disc_pbp *)user;
    uint32_t block_index;
    uint32_t sector_in_block;
    if (reader == NULL || sector == NULL || lba >= reader->map.sector_count) return 0u;
    block_index = lba / SCHWAY__DISC_PBP_SECTORS_PER_BLOCK;
    sector_in_block = lba % SCHWAY__DISC_PBP_SECTORS_PER_BLOCK;
    if (!schway__disc_pbp_decode_block(reader, block_index)) return 0u;
    memcpy(sector, reader->decoded + (size_t)sector_in_block * SCHWAY__DISC_PBP_SECTOR_BYTES,
           SCHWAY__DISC_PBP_SECTOR_BYTES);
    if (subcode != NULL) memset(subcode, 0, 96u);
    return 1u;
}
