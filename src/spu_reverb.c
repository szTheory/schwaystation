/* Reverb resampling primitives, following psx-spx's 39-tap FIR facts.
   Hardware references: .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md section 1.7. */
#include "spu.h"

void schway__spu_reverb_fir_push(int16_t history[SCHWAY__SPU_REVERB_FIR_TAPS],
                                 uint32_t cursor, int16_t sample)
{
    history[cursor] = sample;
}

int16_t schway__spu_reverb_fir_sum(
    const int16_t history[SCHWAY__SPU_REVERB_FIR_TAPS], uint32_t cursor)
{
    (void)history;
    (void)cursor;
    return 0;
}

int16_t schway__spu_reverb_fir_interpolate(
    const int32_t history[SCHWAY__SPU_REVERB_WORK_WORDS], uint32_t latest,
    uint32_t phase, uint32_t channel)
{
    (void)history;
    (void)latest;
    (void)phase;
    (void)channel;
    return 0;
}

void schway__spu_reverb_step(schway *inst)
{
    (void)inst;
}
