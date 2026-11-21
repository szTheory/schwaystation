#include "disc_pbp_metadata.h"
#include "media_limits.h"

#include <stdlib.h>
#include <string.h>

#define SCHWAY__DISC_PBP_HEADER_BYTES UINT64_C(0x28)
#define SCHWAY__DISC_PBP_PSAR_HEADER_BYTES 20u
#define SCHWAY__DISC_PBP_PSAR_TOC_OFFSET UINT64_C(0x800)
#define SCHWAY__DISC_PBP_TOC_BYTES 0xa00u
#define SCHWAY__DISC_PBP_INDEX_OFFSET UINT64_C(0x4000)
#define SCHWAY__DISC_PBP_INDEX_ENTRY_BYTES 32u
#define SCHWAY__DISC_PBP_DATA_OFFSET UINT64_C(0x100000)
#define SCHWAY__DISC_PBP_INDEX_MAX_BLOCKS \
    ((SCHWAY__DISC_PBP_DATA_OFFSET - SCHWAY__DISC_PBP_INDEX_OFFSET) / \
     SCHWAY__DISC_PBP_INDEX_ENTRY_BYTES)
#define SCHWAY__DISC_PBP_SECTOR_BYTES UINT64_C(2352)
#define SCHWAY__DISC_PBP_SECTORS_PER_BLOCK UINT64_C(16)
#define SCHWAY__DISC_PBP_BLOCK_BYTES UINT16_C(0x9300)
#define SCHWAY__DISC_PBP_LEAD_IN_FRAMES UINT64_C(150)

static uint16_t schway__disc_pbp_metadata_le16(const uint8_t bytes[2])
{
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}

static uint32_t schway__disc_pbp_metadata_le32(const uint8_t bytes[4])
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static int schway__disc_pbp_metadata_bcd(uint8_t packed, uint32_t *value)
{
    uint32_t high = (uint32_t)(packed >> 4);
    uint32_t low = (uint32_t)(packed & 0x0fu);
    if (high > 9u || low > 9u) return 0;
    *value = high * 10u + low;
    return 1;
}

static int schway__disc_pbp_metadata_msf(const uint8_t bytes[3], uint64_t *frames)
{
    uint32_t minute;
    uint32_t second;
    uint32_t frame;
    if (!schway__disc_pbp_metadata_bcd(bytes[0], &minute) ||
        !schway__disc_pbp_metadata_bcd(bytes[1], &second) ||
        !schway__disc_pbp_metadata_bcd(bytes[2], &frame) ||
        second >= 60u || frame >= 75u) return 0;
    *frames = (uint64_t)minute * UINT64_C(4500) +
              (uint64_t)second * UINT64_C(75) + (uint64_t)frame;
    return 1;
}

static int schway__disc_pbp_metadata_zero(const uint8_t *bytes, size_t size)
{
    for (size_t i = 0u; i < size; i++) {
        if (bytes[i] != 0u) return 0;
    }
    return 1;
}

static schway_disc_image_status schway__disc_pbp_metadata_read(
    schway__disc_pbp_read_at_fn read_at, void *context, uint64_t file_bytes,
    uint64_t offset, uint8_t *bytes, size_t size)
{
    uint64_t end;
    schway_disc_image_status status;
    if (!schway__media_add_u64(offset, (uint64_t)size, &end) || end > file_bytes) {
        return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    }
    status = read_at(context, offset, bytes, size);
    if (status == SCHWAY_DISC_IMAGE_OK) return status;
    return status == SCHWAY_DISC_IMAGE_IO ? status : SCHWAY_DISC_IMAGE_BAD_IMAGE;
}

void schway__disc_pbp_metadata_clear(schway__disc_pbp_metadata_map *map)
{
    if (map == NULL) return;
    free(map->blocks);
    memset(map, 0, sizeof(*map));
}

schway_disc_image_status schway__disc_pbp_metadata_parse(
    schway__disc_pbp_read_at_fn read_at, void *context, uint64_t file_bytes,
    schway__disc_pbp_metadata_map *out)
{
    uint8_t header[SCHWAY__DISC_PBP_HEADER_BYTES];
    uint8_t psar_header[SCHWAY__DISC_PBP_PSAR_HEADER_BYTES];
    uint8_t toc[SCHWAY__DISC_PBP_TOC_BYTES];
    uint32_t offsets[8];
    uint64_t psar_offset;
    uint64_t data_offset;
    uint64_t toc_offset;
    uint64_t index_offset;
    uint64_t leadout_frames = 0u;
    uint64_t source_start_frames = 0u;
    uint32_t first_track = 0u;
    uint32_t last_track = 0u;
    uint32_t track_records = 0u;
    uint32_t sector_count;
    uint32_t block_count;
    uint64_t expected_block_offset = 0u;
    uint32_t have_a0 = 0u;
    uint32_t have_a1 = 0u;
    uint32_t have_a2 = 0u;
    uint64_t logical_bytes;
    schway__disc_pbp_metadata_map parsed;
    schway_disc_image_status status;

    if (out != NULL) memset(out, 0, sizeof(*out));
    if (read_at == NULL || out == NULL) return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    if (file_bytes > SCHWAY__MEDIA_IMAGE_MAX_BYTES) return SCHWAY_DISC_IMAGE_LIMIT;
    if (file_bytes < sizeof(header)) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    memset(&parsed, 0, sizeof(parsed));
    status = schway__disc_pbp_metadata_read(read_at, context, file_bytes, 0u,
        header, sizeof(header));
    if (status != SCHWAY_DISC_IMAGE_OK) return status;
    if (memcmp(header, "\0PBP", 4u) != 0 ||
        schway__disc_pbp_metadata_le32(header + 4u) != UINT32_C(0x00010000)) {
        return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    }
    for (uint32_t i = 0u; i < 8u; i++) {
        offsets[i] = schway__disc_pbp_metadata_le32(header + 8u + (size_t)i * 4u);
        if (offsets[i] < SCHWAY__DISC_PBP_HEADER_BYTES ||
            (uint64_t)offsets[i] > file_bytes ||
            (i != 0u && offsets[i] < offsets[i - 1u])) {
            return SCHWAY_DISC_IMAGE_BAD_IMAGE;
        }
    }
    psar_offset = offsets[7];
    if (!schway__media_add_u64(psar_offset, SCHWAY__DISC_PBP_PSAR_TOC_OFFSET,
                               &toc_offset) ||
        !schway__media_add_u64(psar_offset, SCHWAY__DISC_PBP_INDEX_OFFSET,
                               &index_offset) ||
        !schway__media_add_u64(psar_offset, SCHWAY__DISC_PBP_DATA_OFFSET,
                               &data_offset)) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    status = schway__disc_pbp_metadata_read(read_at, context, file_bytes,
        psar_offset, psar_header, 12u);
    if (status != SCHWAY_DISC_IMAGE_OK) return status;
    if (memcmp(psar_header, "PSISOIMG0000", 12u) != 0) {
        if (memcmp(psar_header, "PGD", 3u) == 0 ||
            memcmp(psar_header, "PSISOIMG", 8u) != 0) {
            return SCHWAY_DISC_IMAGE_UNSUPPORTED;
        }
        if (memcmp(psar_header, "PSISOIMG", 8u) == 0) {
            for (size_t i = 8u; i < 12u; i++) {
                if (psar_header[i] < (uint8_t)'0' || psar_header[i] > (uint8_t)'9') {
                    return SCHWAY_DISC_IMAGE_BAD_IMAGE;
                }
            }
        }
        return SCHWAY_DISC_IMAGE_UNSUPPORTED;
    }
    status = schway__disc_pbp_metadata_read(read_at, context, file_bytes,
        psar_offset + 12u, psar_header + 12u, sizeof(psar_header) - 12u);
    if (status != SCHWAY_DISC_IMAGE_OK) return status;
    if (psar_offset > file_bytes ||
        schway__disc_pbp_metadata_le32(psar_header + 12u) != file_bytes - psar_offset ||
        file_bytes < UINT64_C(0x6420) ||
        schway__disc_pbp_metadata_le32(psar_header + 16u) != file_bytes - UINT64_C(0x6420)) {
        return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    }
    status = schway__disc_pbp_metadata_read(read_at, context, file_bytes,
        toc_offset, toc, sizeof(toc));
    if (status != SCHWAY_DISC_IMAGE_OK) return status;

    for (size_t i = 0u; i < sizeof(toc); i += 10u) {
        const uint8_t *entry = toc + i;
        uint32_t track_number;
        if (schway__disc_pbp_metadata_zero(entry, 10u)) continue;
        if (entry[2] == 0xa0u) {
            if (have_a0 != 0u) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
            have_a0 = 1u;
            if (entry[0] != 0x41u || entry[1] != 0u || entry[3] != 0u ||
                !schway__disc_pbp_metadata_zero(entry + 4u, 3u) || entry[9] != 0u ||
                !schway__disc_pbp_metadata_bcd(entry[7], &track_number)) {
                return SCHWAY_DISC_IMAGE_BAD_IMAGE;
            }
            if (track_number != 1u || entry[8] != 0x20u) return SCHWAY_DISC_IMAGE_UNSUPPORTED;
            first_track = track_number;
        } else if (entry[2] == 0xa1u) {
            if (have_a1 != 0u) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
            have_a1 = 1u;
            if (entry[0] != 0x41u || entry[1] != 0u || entry[3] != 0u ||
                !schway__disc_pbp_metadata_zero(entry + 4u, 3u) ||
                entry[8] != 0u || entry[9] != 0u ||
                !schway__disc_pbp_metadata_bcd(entry[7], &track_number)) {
                return SCHWAY_DISC_IMAGE_BAD_IMAGE;
            }
            if (track_number != 1u) return SCHWAY_DISC_IMAGE_UNSUPPORTED;
            last_track = track_number;
        } else if (entry[2] == 0xa2u) {
            if (have_a2 != 0u) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
            have_a2 = 1u;
            if (entry[0] != 0x41u || entry[1] != 0u || entry[3] != 0u ||
                !schway__disc_pbp_metadata_zero(entry + 4u, 3u) ||
                !schway__disc_pbp_metadata_msf(entry + 7u, &leadout_frames)) {
                return SCHWAY_DISC_IMAGE_BAD_IMAGE;
            }
        } else {
            if (!schway__disc_pbp_metadata_bcd(entry[2], &track_number) ||
                track_number == 0u) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
            if (track_number != 1u) return SCHWAY_DISC_IMAGE_UNSUPPORTED;
            if (track_records != 0u) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
            track_records = 1u;
            {
                uint64_t dummy_frames;
                if (entry[1] != 0u || entry[6] != 0u ||
                    !schway__disc_pbp_metadata_msf(entry + 3u, &dummy_frames) ||
                    !schway__disc_pbp_metadata_msf(entry + 7u, &source_start_frames)) {
                    return SCHWAY_DISC_IMAGE_BAD_IMAGE;
                }
                if (entry[0] != 0x41u || dummy_frames != UINT64_C(151)) {
                    return SCHWAY_DISC_IMAGE_UNSUPPORTED;
                }
            }
        }
    }
    if (have_a0 == 0u || have_a1 == 0u || have_a2 == 0u || track_records != 1u) {
        return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    }
    if (first_track != 1u || last_track != 1u ||
        source_start_frames != SCHWAY__DISC_PBP_LEAD_IN_FRAMES) {
        return SCHWAY_DISC_IMAGE_UNSUPPORTED;
    }
    if (leadout_frames <= SCHWAY__DISC_PBP_LEAD_IN_FRAMES ||
        leadout_frames - SCHWAY__DISC_PBP_LEAD_IN_FRAMES > UINT32_MAX) {
        return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    }
    sector_count = (uint32_t)(leadout_frames - SCHWAY__DISC_PBP_LEAD_IN_FRAMES);
    if (sector_count == 0u ||
        sector_count % (uint32_t)SCHWAY__DISC_PBP_SECTORS_PER_BLOCK != 0u) {
        return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    }
    if (!schway__media_mul_u64((uint64_t)sector_count, SCHWAY__DISC_PBP_SECTOR_BYTES,
                               &logical_bytes)) return SCHWAY_DISC_IMAGE_LIMIT;
    if (logical_bytes > SCHWAY__MEDIA_IMAGE_MAX_BYTES) return SCHWAY_DISC_IMAGE_LIMIT;
    block_count = (uint32_t)(((uint64_t)sector_count +
        SCHWAY__DISC_PBP_SECTORS_PER_BLOCK - 1u) /
        SCHWAY__DISC_PBP_SECTORS_PER_BLOCK);
    if (block_count == 0u || block_count > SCHWAY__DISC_PBP_INDEX_MAX_BLOCKS) {
        return SCHWAY_DISC_IMAGE_LIMIT;
    }
    if (file_bytes < data_offset || index_offset > data_offset ||
        (uint64_t)block_count * SCHWAY__DISC_PBP_INDEX_ENTRY_BYTES >
            data_offset - index_offset) return SCHWAY_DISC_IMAGE_BAD_IMAGE;

    parsed.blocks = (schway__disc_pbp_block *)calloc(block_count, sizeof(*parsed.blocks));
    if (parsed.blocks == NULL) return SCHWAY_DISC_IMAGE_OUT_OF_MEMORY;
    parsed.data_offset = data_offset;
    parsed.sector_count = sector_count;
    parsed.block_count = block_count;
    parsed.track.size = (uint32_t)sizeof(parsed.track);
    parsed.track.type = SCHWAY_DISC_TRACK_MODE2;
    parsed.track.index0 = 0u;
    parsed.track.index1 = 0u;
    parsed.track.pregap = 0u;
    parsed.track.leadout = sector_count;
    for (uint32_t i = 0u; i < block_count; i++) {
        uint8_t entry[SCHWAY__DISC_PBP_INDEX_ENTRY_BYTES];
        uint64_t entry_offset;
        uint64_t next_expected_offset;
        uint64_t file_offset;
        uint64_t file_end;
        uint32_t block_offset;
        uint16_t block_size;

        if (!schway__media_mul_u64((uint64_t)i,
                SCHWAY__DISC_PBP_INDEX_ENTRY_BYTES, &entry_offset) ||
            !schway__media_add_u64(index_offset, entry_offset, &entry_offset)) {
            status = SCHWAY_DISC_IMAGE_LIMIT;
            goto fail;
        }
        status = schway__disc_pbp_metadata_read(read_at, context, file_bytes,
            entry_offset, entry, sizeof(entry));
        if (status != SCHWAY_DISC_IMAGE_OK) goto fail;
        block_offset = schway__disc_pbp_metadata_le32(entry);
        block_size = schway__disc_pbp_metadata_le16(entry + 4u);
        if ((uint64_t)block_offset != expected_block_offset || block_size == 0u ||
            block_size > SCHWAY__DISC_PBP_BLOCK_BYTES ||
            schway__disc_pbp_metadata_le16(entry + 6u) != 0u ||
            !schway__disc_pbp_metadata_zero(entry + 8u, sizeof(entry) - 8u)) {
            status = SCHWAY_DISC_IMAGE_BAD_IMAGE;
            goto fail;
        }
        if (!schway__media_add_u64(data_offset, (uint64_t)block_offset, &file_offset) ||
            !schway__media_add_u64(file_offset, (uint64_t)block_size, &file_end) ||
            file_end > file_bytes ||
            !schway__media_add_u64(expected_block_offset, (uint64_t)block_size,
                                   &next_expected_offset)) {
            status = SCHWAY_DISC_IMAGE_BAD_IMAGE;
            goto fail;
        }
        parsed.blocks[i].offset = block_offset;
        parsed.blocks[i].size = block_size;
        expected_block_offset = next_expected_offset;
        if (i + 1u == block_count && file_end != file_bytes) {
            status = SCHWAY_DISC_IMAGE_BAD_IMAGE;
            goto fail;
        }
    }
    *out = parsed;
    return SCHWAY_DISC_IMAGE_OK;

fail:
    schway__disc_pbp_metadata_clear(&parsed);
    return status;
}
