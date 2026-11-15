/* One scheduled BRR voice decode and interpolator.
   Hardware facts: docs/hardware/spu.md. */
#include <stdint.h>

#include "internal.h"
#include "spu.h"

#include "spu_tables.inc"

#define SPU_COUNTER_PHASE_MASK UINT32_C(0x0001FFFF)
#define SPU_COUNTER_CURSOR_SHIFT 27u
#define SPU_COUNTER_CURSOR_NONE 31u
#define SPU_SAMPLES_PER_BLOCK 28u
#define SPU_SAMPLE_PHASE_ONE 4096u
#define SPU_BLOCK_PHASE (SPU_SAMPLES_PER_BLOCK * SPU_SAMPLE_PHASE_ONE)
#define SPU_REGISTER_BASE 0x1F801C00u
#define SPU_PMON_ADDRESS 0x1F801D90u
#define SPU_ENDX_ADDRESS 0x1F801D9Cu

/* sample_counter's low 17 bits hold block phase; its high five bits hold the
   last decoded sample index, with 31 marking a block that has not been read. */

static const int16_t brr_filter_positive[5] = {0, 60, 115, 98, 122};
static const int16_t brr_filter_negative[5] = {0, 0, -52, -55, -60};

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

static int16_t spu_clamp16(int32_t value)
{
    if (value > INT16_MAX) {
        return INT16_MAX;
    }
    if (value < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t)value;
}

static uint32_t spu_voice_block_address(const struct schway__spu_voice *voice)
{
    return ((uint32_t)voice->reg[3] << 3) & (SCHWAY__SPU_RAM_SIZE - 1u);
}

static void spu_decode_sample(schway *inst, struct schway__spu_voice *voice,
                              uint32_t block_address, uint32_t sample_index)
{
    uint16_t header = schway__spu_ram_read(inst, block_address);
    uint8_t shift = (uint8_t)(header & 0x000Fu);
    uint8_t filter = (uint8_t)((header >> 4) & 7u);
    uint32_t safe_shift = shift >= 13u ? 9u : shift;
    uint32_t safe_filter = filter > 4u ? 4u : filter;
    uint32_t packed_address = block_address + 2u + ((sample_index >> 2) << 1);
    uint16_t packed = schway__spu_ram_read(inst, packed_address);
    uint32_t nibble_shift = (sample_index & 3u) * 4u;
    uint8_t nibble = (uint8_t)((packed >> nibble_shift) & 0x000Fu);
    int32_t signed_nibble = nibble < 8u ? (int32_t)nibble : (int32_t)nibble - 16;
    int32_t residual = spu_sar32(signed_nibble * 4096, safe_shift);
    int32_t prediction = spu_sar32(
        (int32_t)voice->sample * brr_filter_positive[safe_filter] +
            (int32_t)voice->history[0] * brr_filter_negative[safe_filter],
        6u);
    int16_t decoded = spu_clamp16(residual + prediction);

    voice->history[2] = voice->history[1];
    voice->history[1] = voice->history[0];
    voice->history[0] = voice->sample;
    voice->sample = decoded;
}

static int16_t spu_interpolate(const struct schway__spu_voice *voice, uint32_t phase)
{
    uint32_t index = (phase >> 4) & 0xFFu;
    int32_t output = 0;
    output += spu_sar32((int32_t)spu_gaussian[0x0FFu - index] * voice->history[2], 15u);
    output += spu_sar32((int32_t)spu_gaussian[0x1FFu - index] * voice->history[1], 15u);
    output += spu_sar32((int32_t)spu_gaussian[0x100u + index] * voice->history[0], 15u);
    output += spu_sar32((int32_t)spu_gaussian[index] * voice->sample, 15u);
    return spu_clamp16(output);
}

static void spu_advance_block(struct schway__spu_voice *voice)
{
    voice->reg[3] = (uint16_t)(voice->reg[3] + 2u);
}

static void spu_set_endx(schway *inst, uint32_t voice_index)
{
    uint32_t index = ((SPU_ENDX_ADDRESS - SPU_REGISTER_BASE) >> 1) + (voice_index >> 4);
    inst->spu.reg[index] = (uint16_t)(inst->spu.reg[index] |
                                      (uint16_t)(1u << (voice_index & 15u)));
}

static uint32_t spu_modulated_pitch(const schway *inst, uint32_t voice_index,
                                    uint32_t pitch)
{
    if (voice_index == 0u) {
        return pitch;
    }
    uint32_t pmon_index = ((SPU_PMON_ADDRESS - SPU_REGISTER_BASE) >> 1) + (voice_index >> 4);
    if ((inst->spu.reg[pmon_index] & (uint16_t)(1u << (voice_index & 15u))) == 0u) {
        return pitch;
    }

    int32_t signed_pitch = pitch <= INT16_MAX ? (int32_t)pitch : (int32_t)pitch - 0x10000;
    int32_t previous = (int32_t)inst->spu.voice[voice_index - 1u].output + 0x8000;
    uint32_t step = (uint32_t)spu_sar32(signed_pitch * previous, 15u) & 0xFFFFu;
    return step;
}

void schway__spu_voice_step(schway *inst, uint32_t voice_index)
{
    if (voice_index >= SCHWAY__SPU_VOICE_COUNT) {
        return;
    }
    struct schway__spu_voice *voice = &inst->spu.voice[voice_index];
    uint32_t counter = voice->sample_counter;
    uint32_t phase = counter & SPU_COUNTER_PHASE_MASK;
    uint32_t cursor = counter >> SPU_COUNTER_CURSOR_SHIFT;
    uint32_t sample_index = phase >> 12;
    if (sample_index >= SPU_SAMPLES_PER_BLOCK) {
        phase %= SPU_BLOCK_PHASE;
        sample_index = phase >> 12;
        cursor = SPU_COUNTER_CURSOR_NONE;
    }

    uint32_t block_address = spu_voice_block_address(voice);
    uint32_t first = cursor == SPU_COUNTER_CURSOR_NONE ? 0u : cursor + 1u;
    for (uint32_t i = first; i <= sample_index; i++) {
        spu_decode_sample(inst, voice, block_address, i);
        cursor = i;
    }
    voice->output = spu_interpolate(voice, phase);

    uint32_t pitch = spu_modulated_pitch(inst, voice_index, voice->reg[2]);
    if (pitch > 0x4000u) {
        pitch = 0x4000u;
    }
    if (pitch == 0u) {
        voice->output = 0;
        voice->sample_counter = (cursor << SPU_COUNTER_CURSOR_SHIFT) | phase;
        return;
    }

    phase += pitch;
    if (phase >= SPU_BLOCK_PHASE) {
        phase -= SPU_BLOCK_PHASE;
        /* Read the block flags at the boundary so this access remains visible
           to IRQA. Loop-start is captured in LSAX; loop-end updates ENDX and
           either repeats there or mutes the voice; see the SPU hardware guide. */
        uint8_t flags = (uint8_t)(schway__spu_ram_read(inst, block_address) >> 8);
        if ((flags & 0x04u) != 0u) {
            voice->reg[7] = (uint16_t)(block_address >> 3);
        }
        if ((flags & 0x01u) != 0u) {
            spu_set_endx(inst, voice_index);
            voice->reg[3] = voice->reg[7];
            if ((flags & 0x02u) == 0u) {
                voice->envelope = 0;
                voice->envelope_phase = 4u;
                voice->envelope_counter = 0u;
                voice->reg[6] = 0u;
                uint32_t envx_index = ((voice_index * 0x10u + 0x0Cu) >> 1);
                inst->spu.reg[envx_index] = 0u;
            }
        } else {
            spu_advance_block(voice);
        }
        cursor = SPU_COUNTER_CURSOR_NONE;
    }
    voice->sample_counter = (cursor << SPU_COUNTER_CURSOR_SHIFT) | phase;
}
