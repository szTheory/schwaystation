/* API version 1, as first released. FROZEN: once released this file never
   changes. It calls every function, assigns every field of every struct and
   checks every constant of version 1, so a rename, a removal or a changed
   value fails to build or fails here. Each later API version adds its own
   file, v2.c and so on, beside this one. */
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "schwaystation_version.h"
#include "../check.h"

static void *v1_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void v1_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static void v1_visit(void *user, const char *name, const void *data, uint32_t count,
                     uint32_t width)
{
    uint32_t *fields = (uint32_t *)user;
    if (name != NULL && data != NULL && count != 0u &&
        (width == 1u || width == 2u || width == 4u || width == 8u)) {
        (*fields)++;
    }
}

int main(void)
{
    CHECK_EQ_U64(SCHWAY_API_VERSION, 1);
    CHECK_EQ_U64(SCHWAY_STATUS_OK, 0);
    CHECK_EQ_U64(SCHWAY_STATUS_ERR_ARGUMENT, 1);
    CHECK_EQ_U64(SCHWAY_STATUS_ERR_STRUCT_SIZE, 2);
    CHECK_EQ_U64(SCHWAY_STATUS_ERR_NO_MEMORY, 3);
    CHECK_EQ_U64(SCHWAY_STATUS_ERR_BUFFER_TOO_SMALL, 4);
    CHECK_EQ_U64(SCHWAY_STATUS_ERR_PROFILE_REJECTED, 5);
    CHECK_EQ_U64(SCHWAY_STATUS_ERR_PROFILE_LOCKED, 6);
    CHECK_EQ_U64(SCHWAY_STATUS_ERR_STATE_MAGIC, 7);
    CHECK_EQ_U64(SCHWAY_STATUS_ERR_STATE_VERSION, 8);
    CHECK_EQ_U64(SCHWAY_STATUS_ERR_STATE_CRC, 9);
    CHECK_EQ_U64(SCHWAY_STATUS_ERR_STATE_FIELD, 10);
    CHECK_EQ_U64(SCHWAY_REGION_NTSC_J, 1);
    CHECK_EQ_U64(SCHWAY_REGION_NTSC_U, 2);
    CHECK_EQ_U64(SCHWAY_REGION_PAL, 3);
    CHECK_EQ_U64(SCHWAY_STOP_FRAME, 1);
    CHECK_EQ_U64(SCHWAY_RESET_POWER, 1);
    CHECK_EQ_U64(sizeof(schway_status), 4);
    for (schway_status st = SCHWAY_STATUS_OK; st <= SCHWAY_STATUS_ERR_STATE_FIELD + 1u; st++) {
        const char *text = schway_status_string(st);
        CHECK(text != NULL && text[0] != '\0');
    }

    schway_versions v;
    memset(&v, 0, sizeof v);
    v.size = (uint32_t)sizeof v;
    v.library_major = 0u;
    v.library_minor = 0u;
    v.library_patch = 0u;
    v.api = 0u;
    v.abi = 0u;
    v.behaviour_revision = 0u;
    v.state_format = 0u;
    CHECK_EQ_U64(schway_get_versions(&v), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(v.library_major, SCHWAYSTATION_VERSION_MAJOR);
    CHECK_EQ_U64(v.library_minor, SCHWAYSTATION_VERSION_MINOR);
    CHECK_EQ_U64(v.library_patch, SCHWAYSTATION_VERSION_PATCH);
    CHECK_EQ_U64(v.api, 1);
    CHECK(v.abi >= 1u);
    CHECK(v.behaviour_revision >= 1u);
    CHECK(v.state_format >= 1u);

    schway_allocator allocator;
    memset(&allocator, 0, sizeof allocator);
    allocator.alloc = v1_alloc;
    allocator.free = v1_free;
    allocator.user = NULL;
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.reserved0 = 0u;
    cfg.allocator = allocator;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    CHECK(inst != NULL);
    if (inst == NULL) {
        CHECK_DONE();
    }

    schway_profile profile;
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = SCHWAY_REGION_NTSC_J;
    CHECK_EQ_U64(schway_set_profile(inst, &profile), SCHWAY_STATUS_OK);
    profile.region = SCHWAY_REGION_PAL;
    CHECK_EQ_U64(schway_set_profile(inst, &profile), SCHWAY_STATUS_OK);
    profile.region = SCHWAY_REGION_NTSC_U;
    CHECK_EQ_U64(schway_set_profile(inst, &profile), SCHWAY_STATUS_OK);

    schway_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    info.width = 0u;
    info.height = 0u;
    info.fps_num = 0u;
    info.fps_den = 0u;
    info.sample_rate_num = 0u;
    info.sample_rate_den = 0u;
    info.max_audio_frames = 0u;
    info.region = 0u;
    CHECK_EQ_U64(schway_get_info(inst, &info), SCHWAY_STATUS_OK);
    CHECK(info.width != 0u && info.height != 0u);
    CHECK(info.fps_num != 0u && info.fps_den != 0u);
    CHECK(info.sample_rate_num != 0u && info.sample_rate_den != 0u);
    CHECK(info.max_audio_frames != 0u);
    CHECK_EQ_U64(info.region, SCHWAY_REGION_NTSC_U);

    schway_run_result run;
    memset(&run, 0, sizeof run);
    run.size = (uint32_t)sizeof run;
    run.stop = 0u;
    run.frame = 0u;
    run.ticks = 0u;
    CHECK_EQ_U64(schway_run_frame(inst, &run), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(run.stop, SCHWAY_STOP_FRAME);
    CHECK_EQ_U64(run.frame, 1);
    CHECK(run.ticks != 0u);
    CHECK_EQ_U64(schway_run_frame(inst, NULL), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_set_profile(inst, &profile), SCHWAY_STATUS_ERR_PROFILE_LOCKED);

    schway_video video;
    memset(&video, 0, sizeof video);
    video.size = (uint32_t)sizeof video;
    video.reserved0 = 0u;
    video.frame = 0u;
    video.pixels = NULL;
    video.width = 0u;
    video.height = 0u;
    video.pitch_bytes = 0u;
    video.bpp = 0u;
    video.field = 0u;
    video.interlaced = 0u;
    video.vres = 0u;
    video.hres = 0u;
    video.dot_div = 0u;
    video.region = 0u;
    video.x1 = 0u;
    video.y1 = 0u;
    video.start_x = 0u;
    video.start_y = 0u;
    video.enabled = 0u;
    video.rgb24 = 0u;
    video.reserved1 = 0u;
    CHECK_EQ_U64(schway_get_video(inst, &video), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(video.frame, 2);
    CHECK(video.pixels != NULL);
    CHECK(video.width != 0u && video.height != 0u && video.pitch_bytes != 0u);
    CHECK(video.bpp == 16u || video.bpp == 24u);
    CHECK(video.field <= 1u && video.interlaced <= 1u && video.rgb24 <= 1u);
    CHECK(video.vres != 0u && video.hres != 0u && video.dot_div != 0u);
    CHECK(video.x1 != 0u && video.y1 != 0u && video.start_x == 0u && video.start_y == 0u);
    CHECK_EQ_U64(video.enabled, 1);
    CHECK_EQ_U64(video.region, SCHWAY_REGION_NTSC_U);

    int16_t *samples = (int16_t *)malloc((size_t)info.max_audio_frames * 2u * sizeof *samples);
    CHECK(samples != NULL);
    schway_audio audio;
    memset(&audio, 0, sizeof audio);
    audio.size = (uint32_t)sizeof audio;
    audio.reserved0 = 0u;
    audio.samples = samples;
    audio.capacity = samples != NULL ? info.max_audio_frames : 0u;
    audio.written = 0u;
    audio.dropped = 0u;
    CHECK_EQ_U64(schway_get_audio(inst, &audio), SCHWAY_STATUS_OK);
    CHECK(audio.written <= audio.capacity);
    CHECK(samples == NULL || audio.written != 0u);
    CHECK_EQ_U64(audio.dropped, 0);

    size_t state_size = schway_state_size(inst);
    CHECK(state_size != 0u);
    void *state = malloc(state_size);
    CHECK(state != NULL);
    if (state != NULL) {
        CHECK_EQ_U64(schway_save_state(inst, state, state_size), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(schway_load_state(inst, state, state_size), SCHWAY_STATUS_OK);
    }
    uint32_t fields = 0u;
    schway_state_visitor visitor = v1_visit;
    CHECK_EQ_U64(schway_state_visit(inst, visitor, &fields), SCHWAY_STATUS_OK);
    CHECK(fields != 0u);

    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    free(state);
    free(samples);
    schway_destroy(inst);
    schway_destroy(NULL);
    CHECK_DONE();
}
