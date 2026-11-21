#ifndef SCHWAYSTATION_HOST_DISC_PBP_METADATA_H
#define SCHWAYSTATION_HOST_DISC_PBP_METADATA_H

#include "disc_image.h"

typedef schway_disc_image_status (*schway__disc_pbp_read_at_fn)(
    void *context, uint64_t offset, uint8_t *bytes, size_t size);

typedef struct schway__disc_pbp_block {
    uint32_t offset;
    uint16_t size;
} schway__disc_pbp_block;

typedef struct schway__disc_pbp_metadata_map {
    uint64_t data_offset;
    uint32_t sector_count;
    uint32_t block_count;
    schway_disc_track track;
    schway__disc_pbp_block *blocks;
} schway__disc_pbp_metadata_map;

schway_disc_image_status schway__disc_pbp_metadata_parse(
    schway__disc_pbp_read_at_fn read_at, void *context, uint64_t file_bytes,
    schway__disc_pbp_metadata_map *out);
void schway__disc_pbp_metadata_clear(schway__disc_pbp_metadata_map *map);

#endif
