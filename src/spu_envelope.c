/* Per-voice integer envelope step helper.
   Hardware references: .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md §1.4. */
#include <stddef.h>

#include "internal.h"
#include "spu.h"

#define SPU_ENVELOPE_OFF 0u
#define SPU_ENVELOPE_ATTACK 1u
#define SPU_ENVELOPE_DECAY 2u
#define SPU_ENVELOPE_SUSTAIN 3u
#define SPU_ENVELOPE_RELEASE 4u
#define SPU_ENVELOPE_MAX 0x7FFF
#define SPU_ENVELOPE_THRESHOLD 0x6000

static int32_t spu_clamp_envelope(int32_t level)
{
    if (level < 0) {
        return 0;
    }
    if (level > SPU_ENVELOPE_MAX) {
        return SPU_ENVELOPE_MAX;
    }
    return level;
}

void schway__spu_envelope_rate(uint32_t rate, uint32_t decreasing, int32_t *step,
                               uint32_t *ticks)
{
    if (step == NULL || ticks == NULL || rate >= 128u || decreasing > 1u) {
        return;
    }
    uint32_t shift = rate >> 2;
    uint32_t step_value = rate & 3u;
    uint32_t magnitude = decreasing != 0u ? 8u - step_value : 7u - step_value;
    if (rate < 48u) {
        magnitude <<= 11u - shift;
        *ticks = 1u;
    } else {
        *ticks = UINT32_C(1) << (shift - 11u);
    }
    *step = decreasing != 0u ? -(int32_t)magnitude : (int32_t)magnitude;
}

static uint32_t spu_envelope_parameters(const struct schway__spu_voice *voice,
                                        uint32_t *rate, uint32_t *decreasing,
                                        uint32_t *exponential, uint32_t *all_ones)
{
    uint16_t adsr1 = voice->reg[4];
    uint16_t adsr2 = voice->reg[5];
    switch (voice->envelope_phase) {
    case SPU_ENVELOPE_ATTACK:
        *rate = (adsr1 >> 8) & 0x7Fu;
        *decreasing = 0u;
        *exponential = (adsr1 >> 15) & 1u;
        *all_ones = *rate == 0x7Fu;
        return (adsr1 >> 10) & 0x1Fu;
    case SPU_ENVELOPE_DECAY:
        *rate = ((uint32_t)(adsr1 >> 4) & 0x0Fu) << 2;
        *decreasing = 1u;
        *exponential = 1u;
        *all_ones = ((adsr1 >> 4) & 0x0Fu) == 0x0Fu;
        return (adsr1 >> 4) & 0x0Fu;
    case SPU_ENVELOPE_SUSTAIN:
        *rate = ((((uint32_t)adsr2 >> 8) & 0x0Fu) << 2) |
                (((uint32_t)adsr2 >> 12) & 3u);
        *decreasing = (adsr2 >> 14) & 1u;
        *exponential = (adsr2 >> 15) & 1u;
        *all_ones = *rate == 0x3Fu;
        return *rate >> 2;
    case SPU_ENVELOPE_RELEASE:
        *rate = ((uint32_t)adsr2 & 0x1Fu) << 2;
        *decreasing = 1u;
        *exponential = (adsr2 >> 7) & 1u;
        *all_ones = (adsr2 & 0x1Fu) == 0x1Fu;
        return adsr2 & 0x1Fu;
    default:
        *rate = 0u;
        *decreasing = 0u;
        *exponential = 0u;
        *all_ones = 0u;
        return 0u;
    }
}

static void spu_envelope_adjust_exponential(uint32_t shift, int32_t level,
                                            uint32_t *ticks, int32_t *step)
{
    if (level > SPU_ENVELOPE_THRESHOLD) {
        if (shift < 10u) {
            *step /= 4;
        } else if (shift >= 11u) {
            *ticks *= 4u;
        } else {
            *step /= 2;
            *ticks *= 2u;
        }
    }
}

static int32_t spu_envelope_exp_decrease(int32_t step, int32_t level)
{
    return (step * level) / 0x8000;
}

void schway__spu_envelope_step(schway *inst, uint32_t voice_index)
{
    if (inst == NULL || voice_index >= SCHWAY__SPU_VOICE_COUNT) {
        return;
    }
    struct schway__spu_voice *voice = &inst->spu.voice[voice_index];
    if (voice->envelope_phase == SPU_ENVELOPE_OFF ||
        voice->envelope_phase > SPU_ENVELOPE_RELEASE) {
        return;
    }

    uint32_t rate = 0u;
    uint32_t decreasing = 0u;
    uint32_t exponential = 0u;
    uint32_t all_ones = 0u;
    uint32_t shift = spu_envelope_parameters(voice, &rate, &decreasing,
                                              &exponential, &all_ones);
    if (all_ones != 0u) {
        return;
    }

    int32_t step = 0;
    uint32_t ticks = 0u;
    schway__spu_envelope_rate(rate, decreasing, &step, &ticks);
    if (exponential != 0u) {
        if (decreasing != 0u) {
            step = spu_envelope_exp_decrease(step, voice->envelope);
        } else {
            spu_envelope_adjust_exponential(shift, voice->envelope, &ticks, &step);
        }
    }
    if (ticks == 0u) {
        return;
    }

    if (voice->envelope_counter == 0u) {
        voice->envelope_counter = ticks;
    }
    voice->envelope_counter--;
    if (voice->envelope_counter != 0u) {
        return;
    }
    voice->envelope_counter = ticks;
    voice->envelope = spu_clamp_envelope(voice->envelope + step);
}
