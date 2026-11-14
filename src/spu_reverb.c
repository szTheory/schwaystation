/* Reverb resampling primitives, following psx-spx's 39-tap FIR facts.
   Hardware references: .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md section 1.7. */
#include "spu.h"

static const int16_t reverb_fir_taps[SCHWAY__SPU_REVERB_FIR_TAPS] = {
    -1, 0, 2, 0, -10, 0, 35, 0, -103, 0, 266, 0, -616, 0, 1332, 0,
    -2960, 0, 10246, 16384, 10246, 0, -2960, 0, 1332, 0, -616, 0,
    266, 0, -103, 0, 35, 0, -10, 0, 2, 0, -1};

static int64_t reverb_sar64(int64_t value, uint32_t shift)
{
    int64_t divisor = INT64_C(1) << shift;
    if (value >= 0) {
        return value / divisor;
    }
    return -(((-value) + divisor - 1) / divisor);
}

static int16_t reverb_clamp16(int64_t value)
{
    if (value > INT16_MAX) {
        return INT16_MAX;
    }
    if (value < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t)value;
}

void schway__spu_reverb_fir_push(int16_t history[SCHWAY__SPU_REVERB_FIR_TAPS],
                                 uint32_t cursor, int16_t sample)
{
    history[cursor % SCHWAY__SPU_REVERB_FIR_TAPS] = sample;
}

int16_t schway__spu_reverb_fir_sum(
    const int16_t history[SCHWAY__SPU_REVERB_FIR_TAPS], uint32_t cursor)
{
    int64_t sum = 0;
    cursor %= SCHWAY__SPU_REVERB_FIR_TAPS;
    for (uint32_t tap = 0u; tap < SCHWAY__SPU_REVERB_FIR_TAPS; tap++) {
        uint32_t index = (cursor + SCHWAY__SPU_REVERB_FIR_TAPS - tap) %
                         SCHWAY__SPU_REVERB_FIR_TAPS;
        sum += (int64_t)history[index] * reverb_fir_taps[tap];
    }
    return reverb_clamp16(reverb_sar64(sum, 15u));
}

int16_t schway__spu_reverb_fir_interpolate(
    const int32_t history[SCHWAY__SPU_REVERB_WORK_WORDS], uint32_t latest,
    uint32_t phase, uint32_t channel)
{
    int64_t sum = 0;
    latest %= SCHWAY__SPU_REVERB_WORK_WORDS;
    phase &= 1u;
    channel &= 1u;
    for (uint32_t tap = 0u; tap < SCHWAY__SPU_REVERB_FIR_TAPS; tap++) {
        if (((phase + channel + tap) & 1u) != 0u) {
            continue;
        }
        uint32_t age = (tap + channel - phase) / 2u;
        uint32_t index = (latest + SCHWAY__SPU_REVERB_WORK_WORDS -
                          (age % SCHWAY__SPU_REVERB_WORK_WORDS)) %
                         SCHWAY__SPU_REVERB_WORK_WORDS;
        sum += (int64_t)history[index] * reverb_fir_taps[tap];
    }
    return reverb_clamp16(reverb_sar64(sum * 2, 15u));
}

void schway__spu_reverb_step(schway *inst)
{
    (void)inst;
}
