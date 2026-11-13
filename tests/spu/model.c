/* Public PCM delivery from a bounded run through schway_get_audio. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "../check.h"

#define AUDIO_RING_PAIRS (4u * 887u)

static int16_t samples[2u * AUDIO_RING_PAIRS];

static void *plain_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void plain_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static schway *make(uint32_t region)
{
    schway_config config;
    schway_profile profile;
    schway *inst = NULL;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = plain_alloc;
    config.allocator.free = plain_free;
    CHECK_EQ_U64(schway_create(&config, &inst), SCHWAY_STATUS_OK);
    CHECK(inst != NULL);
    if (inst == NULL) {
        return NULL;
    }
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = region;
    CHECK_EQ_U64(schway_set_profile(inst, &profile), SCHWAY_STATUS_OK);
    return inst;
}

static schway_audio drain(schway *inst, uint32_t capacity)
{
    schway_audio audio;
    memset(samples, 0x5A, sizeof samples);
    memset(&audio, 0, sizeof audio);
    audio.size = (uint32_t)sizeof audio;
    audio.samples = samples;
    audio.capacity = capacity;
    CHECK_EQ_U64(schway_get_audio(inst, &audio), SCHWAY_STATUS_OK);
    return audio;
}

static void check_frame_drain(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    if (inst == NULL) {
        return;
    }
    schway_info info;
    schway_run_result result;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    CHECK_EQ_U64(schway_get_info(inst, &info), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(info.sample_rate_num, 44100u);
    CHECK_EQ_U64(info.sample_rate_den, 1u);
    CHECK_EQ_U64(info.max_audio_frames, 738u);

    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    CHECK_EQ_U64(schway_run_frame(inst, &result), SCHWAY_STATUS_OK);
    schway_audio audio = drain(inst, AUDIO_RING_PAIRS);
    uint64_t expected_pairs = result.ticks / 768u;
    CHECK(expected_pairs == 737u || expected_pairs == 738u);
    CHECK_EQ_U64(audio.written, expected_pairs);
    CHECK_EQ_U64(audio.dropped, 0u);
    CHECK(audio.written <= info.max_audio_frames);
    for (uint32_t i = 0u; i < audio.written; i++) {
        CHECK_EQ_U64(samples[2u * i], 0u);
        CHECK_EQ_U64(samples[2u * i + 1u], 0u);
    }
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    /* These paths are wired now for later fixture-backed cases; no fixture is
       required by this public frame-to-drain tracer. */
    if (argc != 3 || argv[1][0] == '\0' || argv[2][0] == '\0') {
        return 2;
    }
    check_frame_drain();
    CHECK_DONE();
}
