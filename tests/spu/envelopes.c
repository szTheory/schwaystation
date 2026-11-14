/* ADSR baseline and first attack progression. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/internal.h"
#include "../../src/spu.h"

static int32_t attack_level_after_tick;
static int32_t threshold_level_after_tick;

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
    voice->envelope = 0x3456;
    voice->envelope_counter = 0u;
    voice->envelope_phase = 1u;
    voice->reg[4] = 0xFF00u;
    schway__spu_envelope_step(inst, 0u);
    uint32_t failed = voice->envelope != 0x3456 ? 1u : 0u;
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
    threshold_level_after_tick = voice->envelope;
    uint32_t failed = (uint32_t)voice->envelope != expected_level ? 1u : 0u;
    free(inst);
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
    puts("TAP version 13");
    puts("1..8");
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
    printf("%s 8 - all-ones attack encoding does not step\n",
           sentinel_failures == 0u ? "ok" : "not ok");
    return table_failures + attack_failures + sentinel_failures + linear_decrease_failures +
                   exponential_decrease_failures + fast_threshold_failures +
                   middle_threshold_failures + slow_threshold_failures == 0u
               ? 0
               : 1;
}
