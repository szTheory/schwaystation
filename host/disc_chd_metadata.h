#ifndef SCHWAYSTATION_HOST_DISC_CHD_METADATA_H
#define SCHWAYSTATION_HOST_DISC_CHD_METADATA_H

#include "disc_image.h"

#define SCHWAY__DISC_CHD_TAG_CHTR UINT32_C(0x43485452)
#define SCHWAY__DISC_CHD_TAG_CHT2 UINT32_C(0x43485432)
#define SCHWAY__DISC_CHD_METADATA_TEXT_MAX 256u

typedef struct schway__disc_chd_metadata_record {
    uint32_t tag;
    const char *text;
    uint32_t length;
} schway__disc_chd_metadata_record;

typedef struct schway__disc_chd_track_map {
    schway_disc_track toc;
    uint32_t chd_frame_start;
    uint32_t chd_frame_count;
    uint32_t stored_pregap;
} schway__disc_chd_track_map;

typedef struct schway__disc_chd_metadata_map {
    uint32_t track_count;
    uint32_t chd_frame_count;
    schway__disc_chd_track_map tracks[SCHWAY_DISC_MAX_TRACKS];
} schway__disc_chd_metadata_map;

schway_disc_image_status schway__disc_chd_metadata_parse(
    const schway__disc_chd_metadata_record *records, uint32_t record_count,
    uint32_t version, uint32_t hunk_bytes, uint64_t logical_bytes,
    schway__disc_chd_metadata_map *out);

#endif
