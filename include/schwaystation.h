/* schwaystation: a Sony PlayStation emulator core. Behaviour is described in docs/. */
#ifndef SCHWAYSTATION_H
#define SCHWAYSTATION_H

#include <stddef.h>
#include <stdint.h>

#include "schwaystation_version.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCHWAY_API_VERSION 1

typedef uint32_t schway_status;
#define SCHWAY_STATUS_OK 0u
#define SCHWAY_STATUS_ERR_ARGUMENT 1u
#define SCHWAY_STATUS_ERR_STRUCT_SIZE 2u
#define SCHWAY_STATUS_ERR_NO_MEMORY 3u
#define SCHWAY_STATUS_ERR_BUFFER_TOO_SMALL 4u
#define SCHWAY_STATUS_ERR_PROFILE_REJECTED 5u
#define SCHWAY_STATUS_ERR_PROFILE_LOCKED 6u
#define SCHWAY_STATUS_ERR_STATE_MAGIC 7u
#define SCHWAY_STATUS_ERR_STATE_VERSION 8u
#define SCHWAY_STATUS_ERR_STATE_CRC 9u
#define SCHWAY_STATUS_ERR_STATE_FIELD 10u

#define SCHWAY_REGION_NTSC_J 1u
#define SCHWAY_REGION_NTSC_U 2u
#define SCHWAY_REGION_PAL 3u

#define SCHWAY_STOP_FRAME 1u

#define SCHWAY_RESET_POWER 1u

typedef struct schway schway;

typedef struct schway_allocator {
    void *(*alloc)(void *user, size_t size); /* max_align_t-aligned */
    void (*free)(void *user, void *ptr, size_t size);
    void *user;
} schway_allocator;

typedef struct schway_config {
    uint32_t size;
    uint32_t reserved0;
    schway_allocator allocator; /* alloc and free required */
} schway_config;

typedef struct schway_versions {
    uint32_t size;
    uint32_t library_major, library_minor, library_patch;
    uint32_t api;
    uint32_t abi;
    uint32_t behaviour_revision;
    uint32_t state_format;
} schway_versions;

typedef struct schway_profile {
    uint32_t size;
    uint32_t region; /* SCHWAY_REGION_* */
} schway_profile;

typedef struct schway_info {
    uint32_t size;
    uint32_t width, height;
    uint32_t fps_num, fps_den;
    uint32_t sample_rate_num, sample_rate_den;
    uint32_t max_audio_frames; /* stereo pairs in the longest frame */
    uint32_t region;
} schway_info;

typedef struct schway_run_result {
    uint32_t size;
    uint32_t stop; /* SCHWAY_STOP_* */
    uint64_t frame;
    uint64_t ticks;
} schway_run_result;

typedef struct schway_video {
    uint32_t size;
    uint32_t reserved0;
    uint64_t frame;
    const void *pixels; /* into the instance; valid until the next run, load or reset */
    uint32_t width, height, pitch_bytes, bpp;
    uint32_t field, interlaced, vres, hres, dot_div, region;
    uint32_t x1, y1, start_x, start_y, enabled, rgb24;
    uint32_t reserved1;
} schway_video;

typedef struct schway_audio {
    uint32_t size;
    uint32_t reserved0;
    int16_t *samples;  /* in: room for capacity stereo pairs */
    uint32_t capacity; /* in: stereo pairs */
    uint32_t written;  /* out: stereo pairs */
    uint32_t dropped;  /* out: pairs lost since the last drain */
} schway_audio;

/* Called once per state field: count values of width bytes (1, 2, 4 or 8) in host order. */
typedef void (*schway_state_visitor)(void *user, const char *name, const void *data,
                                     uint32_t count, uint32_t width);

schway_status schway_get_versions(schway_versions *out);
schway_status schway_create(const schway_config *cfg, schway **out);
void schway_destroy(schway *inst); /* NULL-safe */
schway_status schway_set_profile(schway *inst, const schway_profile *profile);
schway_status schway_reset(schway *inst, uint32_t kind); /* SCHWAY_RESET_* */
schway_status schway_run_frame(schway *inst, schway_run_result *out); /* out may be NULL */
schway_status schway_get_info(const schway *inst, schway_info *out);
schway_status schway_get_video(const schway *inst, schway_video *out);
schway_status schway_get_audio(schway *inst, schway_audio *io);
size_t schway_state_size(const schway *inst);
schway_status schway_save_state(const schway *inst, void *buf, size_t size);
schway_status schway_load_state(schway *inst, const void *buf, size_t size);
schway_status schway_state_visit(const schway *inst, schway_state_visitor visitor, void *user);
const char *schway_status_string(schway_status status); /* never NULL */

#ifdef __cplusplus
}
#endif

#endif /* SCHWAYSTATION_H */
