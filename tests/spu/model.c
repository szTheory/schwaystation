/* Public PCM delivery from a bounded run through schway_get_audio. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/internal.h"
#include "schwaystation.h"
#include "../check.h"

#define AUDIO_RING_PAIRS (4u * 887u)
#define NTSC_FRAME_NUM UINT64_C(405350378496)
#define NTSC_FRAME_DEN UINT64_C(715909)
#define PAL_FRAME_NUM UINT64_C(482961862656)
#define PAL_FRAME_DEN UINT64_C(709379)

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

static schway_run_result run_frame(schway *inst)
{
    schway_run_result result;
    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    CHECK_EQ_U64(schway_run_frame(inst, &result), SCHWAY_STATUS_OK);
    return result;
}

static schway_run_result run_cycles(schway *inst, uint64_t cycles)
{
    schway_budget budget;
    schway_run_result result;
    memset(&budget, 0, sizeof budget);
    budget.size = (uint32_t)sizeof budget;
    budget.max_cycles = cycles;
    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    CHECK_EQ_U64(schway_run(inst, &budget, &result), SCHWAY_STATUS_OK);
    return result;
}

static void check_region_cadence(uint32_t region, uint64_t frame_num, uint64_t frame_den,
                                uint32_t fps_num, uint32_t fps_den, uint32_t max_pairs,
                                uint32_t short_pairs)
{
    schway *inst = make(region);
    if (inst == NULL) {
        return;
    }
    schway_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    CHECK_EQ_U64(schway_get_info(inst, &info), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(info.region, region);
    CHECK_EQ_U64(info.fps_num, fps_num);
    CHECK_EQ_U64(info.fps_den, fps_den);
    CHECK_EQ_U64(info.sample_rate_num, 44100u);
    CHECK_EQ_U64(info.sample_rate_den, 1u);
    CHECK_EQ_U64(info.max_audio_frames, max_pairs);

    uint64_t previous_ticks = 0u;
    uint64_t total_pairs = 0u;
    uint32_t seen_short = 0u;
    uint32_t seen_long = 0u;
    uint32_t bad_samples = 0u;
    for (uint64_t frame = 1u; frame <= 240u; frame++) {
        schway_run_result result = run_frame(inst);
        uint64_t expected_ticks = frame * frame_num / frame_den;
        uint64_t expected_pairs = expected_ticks / 768u - previous_ticks / 768u;
        CHECK_EQ_U64(result.ticks, expected_ticks);
        schway_audio audio = drain(inst, AUDIO_RING_PAIRS);
        CHECK_EQ_U64(audio.written, expected_pairs);
        CHECK_EQ_U64(audio.dropped, 0u);
        CHECK(audio.written <= info.max_audio_frames);
        CHECK(audio.written == short_pairs || audio.written == short_pairs + 1u);
        seen_short += audio.written == short_pairs ? 1u : 0u;
        seen_long += audio.written == short_pairs + 1u ? 1u : 0u;
        for (uint32_t i = 0u; i < 2u * audio.written; i++) {
            bad_samples += samples[i] != 0 ? 1u : 0u;
        }
        total_pairs += audio.written;
        previous_ticks = result.ticks;
    }
    CHECK(seen_short > 0u);
    CHECK(seen_long > 0u);
    CHECK_EQ_U64(bad_samples, 0u);
    CHECK_EQ_U64(total_pairs, previous_ticks / 768u);
    schway_destroy(inst);
}

static void check_split_run_boundaries(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    if (inst == NULL) {
        return;
    }
    schway_run_result result = run_cycles(inst, 0u);
    CHECK_EQ_U64(result.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(result.ticks, 0u);
    schway_audio audio = drain(inst, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.written, 0u);

    result = run_cycles(inst, 767u);
    CHECK_EQ_U64(result.ticks, 767u);
    audio = drain(inst, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.written, 0u);

    result = run_cycles(inst, 1u);
    CHECK_EQ_U64(result.ticks, 768u);
    audio = drain(inst, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.written, 1u);

    result = run_cycles(inst, 768u);
    CHECK_EQ_U64(result.ticks, 1536u);
    audio = drain(inst, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.written, 1u);

    result = run_cycles(inst, 769u);
    CHECK_EQ_U64(result.ticks, 2305u);
    audio = drain(inst, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.written, 1u);
    CHECK_EQ_U64(audio.dropped, 0u);
    schway_destroy(inst);
}

static void check_instance_owns_spu_ram(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    if (inst == NULL) {
        return;
    }
#ifdef SCHWAY__SPU_RAM_SIZE
    CHECK_EQ_U64(sizeof inst->spu.ram, 0x80000u);
#else
    CHECK_EQ_U64(0u, 0x80000u);
#endif
    schway_destroy(inst);
}

static void check_ring_partial_wrap_and_drops(void)
{
    schway *inst = make(SCHWAY_REGION_PAL);
    if (inst == NULL) {
        return;
    }
    schway_run_result result;
    for (uint32_t i = 0u; i < 5u; i++) {
        result = run_frame(inst);
    }
    uint64_t due = result.ticks / 768u;
    CHECK(due > AUDIO_RING_PAIRS);
    schway_audio audio = drain(inst, 100u);
    CHECK_EQ_U64(audio.written, 100u);
    CHECK_EQ_U64(audio.dropped, due - AUDIO_RING_PAIRS);

    uint64_t previous_ticks = result.ticks;
    result = run_frame(inst);
    uint64_t new_pairs = result.ticks / 768u - previous_ticks / 768u;
    CHECK(new_pairs > 100u);
    audio = drain(inst, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.written, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.dropped, new_pairs - 100u);
    audio = drain(inst, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.written, 0u);
    CHECK_EQ_U64(audio.dropped, 0u);
    schway_destroy(inst);
}

static void check_reset_and_load_clear_drain(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_J);
    if (inst == NULL) {
        return;
    }
    (void)run_frame(inst);
    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    schway_audio audio = drain(inst, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.written, 0u);
    CHECK_EQ_U64(audio.dropped, 0u);

    (void)run_frame(inst);
    size_t state_size = schway_state_size(inst);
    uint8_t *state = state_size == 0u ? NULL : (uint8_t *)malloc(state_size);
    CHECK(state != NULL);
    if (state != NULL) {
        CHECK_EQ_U64(schway_save_state(inst, state, state_size), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(schway_load_state(inst, state, state_size), SCHWAY_STATUS_OK);
        audio = drain(inst, AUDIO_RING_PAIRS);
        CHECK_EQ_U64(audio.written, 0u);
        CHECK_EQ_U64(audio.dropped, 0u);
        free(state);
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
    (void)argv;
    check_region_cadence(SCHWAY_REGION_NTSC_J, NTSC_FRAME_NUM, NTSC_FRAME_DEN,
                         53693175u, 897619u, 738u, 737u);
    check_region_cadence(SCHWAY_REGION_NTSC_U, NTSC_FRAME_NUM, NTSC_FRAME_DEN,
                         53693175u, 897619u, 738u, 737u);
    check_region_cadence(SCHWAY_REGION_PAL, PAL_FRAME_NUM, PAL_FRAME_DEN,
                         53203425u, 1069484u, 887u, 886u);
    check_split_run_boundaries();
    check_instance_owns_spu_ram();
    check_ring_partial_wrap_and_drops();
    check_reset_and_load_clear_drain();
    CHECK_DONE();
}
