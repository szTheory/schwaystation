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
#define SPU_VOLUME_MIN (-0x8000)
#define SPU_VOLUME_MAX 0x7FFF
#define SPU_REGISTER_BASE 0x1F801C00u
#define SPU_CURRENT_VOLUME_BASE 0x1F801E00u
#define SPU_MAIN_VOLUME_LEFT 0x1F801D80u

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

static int32_t spu_sar32(int32_t value, uint32_t shift)
{
    if (shift == 0u) {
        return value;
    }
    int64_t divisor = INT64_C(1) << shift;
    int64_t wide = value;
    if (wide >= 0) {
        return (int32_t)(wide / divisor);
    }
    return (int32_t)(-(((-wide) + divisor - 1) / divisor));
}

static int32_t spu_clamp_volume(int32_t level)
{
    if (level < SPU_VOLUME_MIN) {
        return SPU_VOLUME_MIN;
    }
    if (level > SPU_VOLUME_MAX) {
        return SPU_VOLUME_MAX;
    }
    return level;
}

static uint16_t spu_volume_to_wire(int32_t volume)
{
    if (volume >= 0) {
        return (uint16_t)volume;
    }
    return (uint16_t)(UINT16_MAX - (uint16_t)(-(volume + 1)));
}

static int32_t spu_direct_volume(uint16_t value)
{
    int32_t level = (int32_t)(value & 0x7FFFu);
    if ((value & 0x4000u) != 0u) {
        level -= 0x8000;
    }
    return level * 2;
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
    return spu_sar32(step * level, 15u);
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
    if (voice->envelope_phase == SPU_ENVELOPE_ATTACK &&
        voice->envelope == SPU_ENVELOPE_MAX) {
        voice->envelope_phase = SPU_ENVELOPE_DECAY;
        voice->envelope_counter = 0u;
    } else if (voice->envelope_phase == SPU_ENVELOPE_DECAY) {
        int32_t sustain = ((int32_t)(voice->reg[4] & 0x000Fu) + 1) * 0x0800;
        if (sustain > SPU_ENVELOPE_MAX) {
            sustain = SPU_ENVELOPE_MAX;
        }
        if (voice->envelope <= sustain) {
            voice->envelope = sustain;
            voice->envelope_phase = SPU_ENVELOPE_SUSTAIN;
            voice->envelope_counter = 0u;
        }
    } else if (voice->envelope_phase == SPU_ENVELOPE_RELEASE && voice->envelope == 0) {
        voice->envelope_phase = SPU_ENVELOPE_OFF;
        voice->envelope_counter = 0u;
    }
    uint32_t envx_index = ((SPU_REGISTER_BASE + voice_index * 0x10u + 0x0Cu) -
                           SPU_REGISTER_BASE) >> 1;
    inst->spu.reg[envx_index] = (uint16_t)voice->envelope;
}

static void spu_sweep_step(uint16_t value, int32_t *level, uint32_t *counter)
{
    if ((value & 0x8000u) == 0u) {
        *level = spu_direct_volume(value);
        *counter = 0u;
        return;
    }

    uint32_t shift = (value >> 2) & 0x1Fu;
    uint32_t rate = (shift << 2) | (value & 3u);
    uint32_t phase_negative = (value >> 12) & 1u;
    uint32_t decreasing = ((value >> 13) & 1u) ^ phase_negative;
    uint32_t exponential = (value >> 14) & 1u;
    if (rate == 0x7Fu) {
        return;
    }

    int32_t step = 0;
    uint32_t ticks = 0u;
    schway__spu_envelope_rate(rate, decreasing, &step, &ticks);
    if (exponential != 0u) {
        if (decreasing != 0u) {
            step = spu_sar32(step * *level, 15u);
        } else {
            spu_envelope_adjust_exponential(shift, *level, &ticks, &step);
        }
    }
    if (ticks == 0u) {
        return;
    }
    if (*counter == 0u) {
        *counter = ticks;
    }
    (*counter)--;
    if (*counter == 0u) {
        *counter = ticks;
        *level = spu_clamp_volume(*level + step);
    }
}

void schway__spu_volume_step(schway *inst, uint32_t voice_index)
{
    if (inst == NULL || voice_index >= SCHWAY__SPU_VOICE_COUNT) {
        return;
    }
    struct schway__spu_voice *voice = &inst->spu.voice[voice_index];
    for (uint32_t channel = 0u; channel < 2u; channel++) {
        spu_sweep_step(voice->reg[channel], &voice->sweep_level[channel],
                       &voice->sweep_counter[channel]);
        uint32_t current_index = ((SPU_CURRENT_VOLUME_BASE - SPU_REGISTER_BASE) >> 1) +
                                 voice_index * 2u + channel;
        inst->spu.reg[current_index] = spu_volume_to_wire(voice->sweep_level[channel]);
    }
}

void schway__spu_main_volume_step(schway *inst)
{
    if (inst == NULL) {
        return;
    }
    for (uint32_t channel = 0u; channel < 2u; channel++) {
        uint32_t index = ((SPU_MAIN_VOLUME_LEFT - SPU_REGISTER_BASE) >> 1) + channel;
        spu_sweep_step(inst->spu.reg[index], &inst->spu.main_sweep_level[channel],
                       &inst->spu.main_sweep_counter[channel]);
    }
}
