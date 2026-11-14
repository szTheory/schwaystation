/* ADSR baseline and first attack progression. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/internal.h"
#include "../../src/spu.h"

static int32_t attack_level_after_tick;
static int32_t threshold_level_after_tick;
static const uint32_t spu_test_reg_base = 0x1F801C00u;

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

static schway *make_instance(void)
{
    schway_config config;
    schway *inst = NULL;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = plain_alloc;
    config.allocator.free = plain_free;
    if (schway_create(&config, &inst) != SCHWAY_STATUS_OK || inst == NULL) {
        return NULL;
    }
    return inst;
}

/* Independently re-derived from ares ares/ps1/spu/adsr.cpp at 9408cb43d494,
   ISC. No upstream source expression or table data is copied. */
static void reference_rate(uint32_t rate, uint32_t decreasing, int32_t *step,
                           uint32_t *ticks)
{
    uint32_t shift = rate >> 2;
    uint32_t step_value = rate & 3u;
    uint32_t magnitude;
    if (rate < 48u) {
        magnitude = decreasing != 0u ? 8u - step_value : 7u - step_value;
        magnitude <<= 11u - shift;
        *ticks = 1u;
    } else {
        magnitude = decreasing != 0u ? 8u - step_value : 7u - step_value;
        *ticks = UINT32_C(1) << (shift - 11u);
    }
    *step = decreasing != 0u ? -(int32_t)magnitude : (int32_t)magnitude;
}

static uint32_t check_rate_table(void)
{
    uint32_t mismatches = 0u;
    for (uint32_t decreasing = 0u; decreasing < 2u; decreasing++) {
        for (uint32_t rate = 0u; rate < 128u; rate++) {
            int32_t expected_step = 0;
            uint32_t expected_ticks = 0u;
            int32_t actual_step = 0;
            uint32_t actual_ticks = 0u;
            reference_rate(rate, decreasing, &expected_step, &expected_ticks);
            schway__spu_envelope_rate(rate, decreasing, &actual_step, &actual_ticks);
            mismatches += actual_step != expected_step ? 1u : 0u;
            mismatches += actual_ticks != expected_ticks ? 1u : 0u;
        }
    }
    return mismatches;
}

static uint32_t check_attack_progression(void)
{
    schway *inst = calloc(1u, sizeof *inst);
    if (inst == NULL) {
        return 1u;
    }
    struct schway__spu_voice *voice = &inst->spu.voice[0];
    voice->reg[4] = 0u;
    voice->envelope_phase = 1u;
    voice->envelope_counter = 1u;
    voice->envelope = 1000;
    schway__spu_envelope_step(inst, 0u);
    attack_level_after_tick = voice->envelope;
    uint32_t failed = voice->envelope != 15336 ? 1u : 0u;
    free(inst);
    return failed;
}

static uint32_t check_all_ones_do_not_step(void)
{
    schway *inst = calloc(1u, sizeof *inst);
    if (inst == NULL) {
        return 1u;
    }
    struct schway__spu_voice *voice = &inst->spu.voice[0];
    uint32_t failed = 0u;
    voice->envelope = 0x3456;
    voice->envelope_counter = 0x123u;
    voice->envelope_phase = 1u;
    voice->reg[4] = 0x7F00u;
    schway__spu_envelope_step(inst, 0u);
    failed += voice->envelope != 0x3456 || voice->envelope_counter != 0x123u;

    voice->envelope_phase = 2u;
    voice->envelope_counter = 0x123u;
    voice->reg[4] = 0x00F0u;
    schway__spu_envelope_step(inst, 0u);
    failed += voice->envelope != 0x3456 || voice->envelope_counter != 0x123u;

    voice->envelope_phase = 3u;
    voice->envelope_counter = 0x123u;
    voice->reg[5] = 0x3F00u;
    schway__spu_envelope_step(inst, 0u);
    failed += voice->envelope != 0x3456 || voice->envelope_counter != 0x123u;

    voice->envelope_phase = 4u;
    voice->envelope_counter = 0x123u;
    voice->reg[5] = 0x001Fu;
    schway__spu_envelope_step(inst, 0u);
    failed += voice->envelope != 0x3456 || voice->envelope_counter != 0x123u;
    free(inst);
    return failed;
}

static uint32_t check_sustain_change(uint16_t adsr2, int32_t level, int32_t expected)
{
    schway *inst = calloc(1u, sizeof *inst);
    if (inst == NULL) {
        return 1u;
    }
    struct schway__spu_voice *voice = &inst->spu.voice[0];
    voice->reg[5] = adsr2;
    voice->envelope_phase = 3u;
    voice->envelope_counter = 1u;
    voice->envelope = level;
    schway__spu_envelope_step(inst, 0u);
    uint32_t failed = voice->envelope != expected ? 1u : 0u;
    free(inst);
    return failed;
}

static uint32_t check_threshold_step(uint32_t shift, uint32_t expected_ticks,
                                     uint32_t expected_calls)
{
    schway *inst = calloc(1u, sizeof *inst);
    if (inst == NULL) {
        return 1u;
    }
    struct schway__spu_voice *voice = &inst->spu.voice[0];
    voice->reg[4] = (uint16_t)(0x8000u | (shift << 10));
    voice->envelope_phase = 1u;
    voice->envelope = 0x7000;
    voice->envelope_counter = 0u;
    for (uint32_t i = 0u; i < expected_calls; i++) {
        schway__spu_envelope_step(inst, 0u);
    }
    uint32_t expected_level = 0x7000u;
    if (expected_calls >= expected_ticks) {
        uint32_t step = 7u << (11u - shift);
        if (shift < 10u) {
            step /= 4u;
        } else if (shift == 10u) {
            step /= 2u;
        }
        expected_level += step;
    }
    uint32_t expected_counter = expected_ticks - (expected_calls % expected_ticks);
    if (expected_counter == 0u) {
        expected_counter = expected_ticks;
    }
    threshold_level_after_tick = voice->envelope;
    uint32_t failed = (uint32_t)voice->envelope != expected_level ? 1u : 0u;
    failed += voice->envelope_counter != expected_counter;
    free(inst);
    return failed;
}

static uint32_t check_phase_transitions(void)
{
    schway *inst = calloc(1u, sizeof *inst);
    if (inst == NULL) {
        return 1u;
    }
    struct schway__spu_voice *voice = &inst->spu.voice[0];
    voice->reg[4] = 0u;
    voice->envelope_phase = 1u;
    voice->envelope_counter = 1u;
    voice->envelope = 0x7FFEu;
    schway__spu_envelope_step(inst, 0u);
    uint32_t failed = voice->envelope_phase != 2u || voice->envelope != 0x7FFF;

    voice->envelope_phase = 2u;
    voice->envelope_counter = 1u;
    voice->envelope = 0x1000;
    schway__spu_envelope_step(inst, 0u);
    failed += voice->envelope_phase != 3u || voice->envelope != 0x0800;

    voice->reg[5] = 0u;
    voice->envelope_phase = 4u;
    voice->envelope_counter = 1u;
    voice->envelope = 0x1000;
    schway__spu_envelope_step(inst, 0u);
    failed += voice->envelope_phase != 0u || voice->envelope != 0;
    free(inst);
    return failed;
}

static uint32_t check_kon_koff_and_envx(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return 1u;
    }
    schway__spu_reg_write(inst, spu_test_reg_base + 0x08u, 0x7F00u);
    schway__spu_reg_write(inst, spu_test_reg_base + 0x0Au, 0x001Fu);
    schway__spu_reg_write(inst, spu_test_reg_base + 0x188u, 1u);
    schway__spu_event(inst, 768u);
    struct schway__spu_voice *voice = &inst->spu.voice[0];
    uint32_t failed = voice->envelope_phase != 1u || voice->envelope != 0;

    schway__spu_reg_write(inst, spu_test_reg_base + 0x0Cu, 0x1234u);
    schway__spu_event(inst, 1536u);
    failed += voice->envelope != 0x1234;
    failed += schway__spu_reg_read(inst, spu_test_reg_base + 0x0Cu) != 0x1234u;

    schway__spu_reg_write(inst, spu_test_reg_base + 0x18Cu, 1u);
    schway__spu_event(inst, 2304u);
    failed += voice->envelope_phase != 4u || voice->envelope != 0x1234;
    schway__spu_reg_write(inst, spu_test_reg_base + 0x18Cu, 1u);
    schway__spu_event(inst, 3072u);
    failed += voice->envelope_phase != 4u || voice->envelope != 0x1234;

    schway__spu_reg_write(inst, spu_test_reg_base + 0x188u, 1u);
    schway__spu_event(inst, 3840u);
    failed += voice->envelope_phase != 1u || voice->envelope != 0;
    schway_destroy(inst);
    return failed;
}

static uint32_t check_direct_and_sweep_volumes(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return 1u;
    }
    schway__spu_reg_write(inst, spu_test_reg_base, 0x4001u);
    schway__spu_reg_write(inst, spu_test_reg_base + 2u, 0x3FFFu);
    schway__spu_reg_write(inst, 0x1F801D80u, 0x4001u);
    schway__spu_reg_write(inst, 0x1F801D82u, 0x3FFFu);
    schway__spu_event(inst, 768u);
    uint32_t failed = schway__spu_reg_read(inst, 0x1F801E00u) != 0x8002u;
    failed += schway__spu_reg_read(inst, 0x1F801E02u) != 0x7FFEu;
    failed += inst->spu.main_sweep_level[0] != -32766;
    failed += inst->spu.main_sweep_level[1] != 32766;

    schway__spu_reg_write(inst, spu_test_reg_base, 0u);
    schway__spu_reg_write(inst, spu_test_reg_base + 2u, 0u);
    schway__spu_reg_write(inst, 0x1F801D80u, 0u);
    schway__spu_reg_write(inst, 0x1F801D82u, 0u);
    schway__spu_event(inst, 1536u);
    schway__spu_reg_write(inst, spu_test_reg_base, 0x8000u);
    schway__spu_reg_write(inst, spu_test_reg_base + 2u, 0x9000u);
    schway__spu_reg_write(inst, 0x1F801D80u, 0x8000u);
    schway__spu_reg_write(inst, 0x1F801D82u, 0x9000u);
    schway__spu_event(inst, 2304u);
    failed += schway__spu_reg_read(inst, 0x1F801E00u) != 0x3800u;
    failed += schway__spu_reg_read(inst, 0x1F801E02u) != 0xC000u;
    failed += inst->spu.main_sweep_level[0] != 0x3800;
    failed += inst->spu.main_sweep_level[1] != -0x4000;
    schway_destroy(inst);
    return failed;
}

static void prepare_tone(schway *inst, uint16_t attr)
{
    schway__spu_ram_write(inst, 0x100u, 0x0004u);
    for (uint32_t i = 0u; i < 7u; i++) {
        schway__spu_ram_write(inst, 0x102u + i * 2u, 0x7777u);
    }
    schway__spu_reg_write(inst, spu_test_reg_base, 0x3FFFu);
    schway__spu_reg_write(inst, spu_test_reg_base + 2u, 0x4001u);
    schway__spu_reg_write(inst, spu_test_reg_base + 4u, 0x1000u);
    schway__spu_reg_write(inst, spu_test_reg_base + 6u, 0x0020u);
    schway__spu_reg_write(inst, spu_test_reg_base + 8u, 0u);
    schway__spu_reg_write(inst, 0x1F801D80u, 0x3FFFu);
    schway__spu_reg_write(inst, 0x1F801D82u, 0x3FFFu);
    schway__spu_reg_write(inst, spu_test_reg_base + 0x1AAu, attr);
    schway__spu_reg_write(inst, spu_test_reg_base + 0x188u, 1u);
}

static uint32_t get_one_audio_pair(schway *inst, int16_t *left, int16_t *right)
{
    int16_t samples[2] = {0, 0};
    schway_audio audio;
    memset(&audio, 0, sizeof audio);
    audio.size = (uint32_t)sizeof audio;
    audio.samples = samples;
    audio.capacity = 1u;
    if (schway_get_audio(inst, &audio) != SCHWAY_STATUS_OK || audio.written != 1u) {
        return 1u;
    }
    *left = samples[0];
    *right = samples[1];
    return 0u;
}

static uint32_t get_audio_pairs(schway *inst, int16_t *samples, uint32_t capacity,
                                uint32_t *written)
{
    schway_audio audio;
    memset(&audio, 0, sizeof audio);
    audio.size = (uint32_t)sizeof audio;
    audio.samples = samples;
    audio.capacity = capacity;
    if (schway_get_audio(inst, &audio) != SCHWAY_STATUS_OK) {
        return 1u;
    }
    *written = audio.written;
    return 0u;
}

static uint32_t check_public_pcm_and_mute(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return 1u;
    }
    prepare_tone(inst, 0xC000u);
    for (uint32_t tick = 1u; tick <= 64u; tick++) {
        schway__spu_event(inst, (uint64_t)tick * 768u);
    }
    int16_t samples[128] = {0};
    uint32_t written = 0u;
    uint32_t failed = get_audio_pairs(inst, samples, 64u, &written);
    failed += written != 64u;
    uint32_t signed_pair = 0u;
    for (uint32_t i = 0u; i < written; i++) {
        signed_pair |= samples[i * 2u] > 0 && samples[i * 2u + 1u] < 0 ? 1u : 0u;
    }
    failed += signed_pair == 0u;
    schway_destroy(inst);

    inst = make_instance();
    if (inst == NULL) {
        return failed + 1u;
    }
    prepare_tone(inst, 0x8000u);
    struct schway__spu_voice *silent_voice = &inst->spu.voice[SCHWAY__SPU_VOICE_COUNT - 1u];
    silent_voice->reg[2] = 0x1000u;
    silent_voice->reg[3] = 0x0020u;
    silent_voice->reg[4] = 0u;
    silent_voice->envelope_phase = 1u;
    silent_voice->envelope = 0;
    silent_voice->sample_counter = UINT32_C(31) << 27;
    schway__spu_event(inst, 768u);
    failed += inst->spu.voice[0].sample == 0;
    failed += inst->spu.voice[0].envelope == 0;
    failed += silent_voice->sample == 0;
    failed += silent_voice->envelope == 0;
    int16_t left = 0;
    int16_t right = 0;
    failed += get_one_audio_pair(inst, &left, &right);
    failed += left != 0 || right != 0;
    schway_destroy(inst);
    return failed;
}

int main(void)
{
    uint32_t table_failures = check_rate_table();
    uint32_t attack_failures = check_attack_progression();
    uint32_t sentinel_failures = check_all_ones_do_not_step();
    uint32_t linear_decrease_failures =
        check_sustain_change(0x4000u, 40000, 23616);
    uint32_t exponential_decrease_failures =
        check_sustain_change(0xC000u, 0x4000, 0x2000);
    uint32_t fast_threshold_failures = check_threshold_step(3u, 1u, 1u);
    uint32_t middle_threshold_failures = check_threshold_step(10u, 2u, 1u);
    uint32_t slow_threshold_failures = check_threshold_step(11u, 4u, 1u);
    uint32_t phase_failures = check_phase_transitions();
    uint32_t key_failures = check_kon_koff_and_envx();
    uint32_t volume_failures = check_direct_and_sweep_volumes();
    uint32_t pcm_failures = check_public_pcm_and_mute();
    puts("TAP version 13");
    puts("1..12");
    if (attack_failures != 0u) {
        printf("# attack level after one tick: %ld (expected 15336)\n",
               (long)attack_level_after_tick);
    }
    if (fast_threshold_failures != 0u) {
        printf("# fast exponential attack level: %ld (expected 29120)\n",
               (long)threshold_level_after_tick);
    }
    printf("%s 1 - all 2x128 ADSR baseline rate entries match the independent generator\n",
           table_failures == 0u ? "ok" : "not ok");
    printf("%s 2 - attack rate advances the voice envelope by its table step\n",
           attack_failures == 0u ? "ok" : "not ok");
    printf("%s 3 - linear sustain decrease uses the documented fixed step\n",
           linear_decrease_failures == 0u ? "ok" : "not ok");
    printf("%s 4 - exponential sustain decrease scales the step by the current level\n",
           exponential_decrease_failures == 0u ? "ok" : "not ok");
    printf("%s 5 - exponential attack below shift ten quarters the step above 6000h\n",
           fast_threshold_failures == 0u ? "ok" : "not ok");
    printf("%s 6 - shift ten halves the step and doubles the interval above 6000h\n",
           middle_threshold_failures == 0u ? "ok" : "not ok");
    printf("%s 7 - shift eleven keeps the step and quadruples the interval above 6000h\n",
           slow_threshold_failures == 0u ? "ok" : "not ok");
    printf("%s 8 - all-ones ADSR rate encodings do not step in any phase\n",
           sentinel_failures == 0u ? "ok" : "not ok");
    printf("%s 9 - attack, decay and release transition at documented limits\n",
           phase_failures == 0u ? "ok" : "not ok");
    printf("%s 10 - MMIO KON/KOFF and ENVX writes control the reported phase and level\n",
           key_failures == 0u ? "ok" : "not ok");
    printf("%s 11 - direct and swept voice/main volumes preserve sign and stereo\n",
           volume_failures == 0u ? "ok" : "not ok");
    printf("%s 12 - public PCM is volume-scaled and mute leaves voice work running\n",
           pcm_failures == 0u ? "ok" : "not ok");
    return table_failures + attack_failures + sentinel_failures + linear_decrease_failures +
                   exponential_decrease_failures + fast_threshold_failures +
                   middle_threshold_failures + slow_threshold_failures + phase_failures +
                   key_failures + volume_failures + pcm_failures == 0u
               ? 0
               : 1;
}
