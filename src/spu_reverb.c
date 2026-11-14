/* Reverb resampling primitives, following psx-spx's 39-tap FIR facts.
   Hardware references: .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md section 1.7. */
#include "internal.h"

#define SCHWAY__SPU_REVERB_TABLES
#include "spu_tables.inc"

#define SPU_REGISTER_BASE 0x1F801C00u
#define SPU_VLIN_ADDRESS 0x1F801DFCu
#define SPU_VRIN_ADDRESS 0x1F801DFEu
#define SPU_EVOL_L_ADDRESS 0x1F801D84u
#define SPU_EVOL_R_ADDRESS 0x1F801D86u

static uint32_t reverb_reg_index(uint32_t address)
{
    return (address - SPU_REGISTER_BASE) >> 1;
}

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
        sum += (int64_t)history[index] * spu_reverb_fir_taps[tap];
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
        sum += (int64_t)history[index] * spu_reverb_fir_taps[tap];
    }
    return reverb_clamp16(reverb_sar64(sum * 2, 15u));
}

static int16_t reverb_feedback_step(const struct schway__spu *spu, uint32_t channel,
                                    int16_t input)
{
    /* The next plan replaces this unity 22.05 kHz feedback stage. */
    (void)spu;
    (void)channel;
    return input;
}

static int32_t reverb_signed16(uint16_t value)
{
    return (value & 0x8000u) != 0u ? (int32_t)value - 0x10000 : (int32_t)value;
}

static int16_t reverb_scale(int16_t sample, uint16_t volume)
{
    int32_t signed_volume = reverb_signed16(volume);
    return reverb_clamp16(reverb_sar64((int64_t)sample * signed_volume, 15u));
}

void schway__spu_reverb_step(schway *inst, int32_t wet_left, int32_t wet_right,
                             int16_t *return_left, int16_t *return_right)
{
    if (return_left != NULL) {
        *return_left = 0;
    }
    if (return_right != NULL) {
        *return_right = 0;
    }
    if (inst == NULL || return_left == NULL || return_right == NULL) {
        return;
    }

    struct schway__spu *spu = &inst->spu;
    const uint32_t fir_cursor = spu->reverb_fir_cursor;
    int16_t input[2] = {reverb_clamp16(wet_left), reverb_clamp16(wet_right)};
    input[0] = reverb_scale(input[0], spu->reg[reverb_reg_index(SPU_VLIN_ADDRESS)]);
    input[1] = reverb_scale(input[1], spu->reg[reverb_reg_index(SPU_VRIN_ADDRESS)]);
    for (uint32_t channel = 0u; channel < 2u; channel++) {
        schway__spu_reverb_fir_push(spu->reverb_fir[channel], fir_cursor, input[channel]);
    }

    uint32_t phase = (uint32_t)((spu->next_tick / SCHWAY__CYCLES_PER_PAIR - 1u) & 1u);
    int16_t filtered = schway__spu_reverb_fir_sum(spu->reverb_fir[phase], fir_cursor);
    int16_t feedback = reverb_feedback_step(spu, phase, filtered);
    uint32_t work_cursor = spu->reverb_work_cursor;
    spu->reverb_work[phase][work_cursor] = feedback;

    int16_t resampled[2];
    for (uint32_t channel = 0u; channel < 2u; channel++) {
        resampled[channel] = schway__spu_reverb_fir_interpolate(
            spu->reverb_work[channel], work_cursor, phase, channel);
    }
    *return_left = reverb_scale(resampled[0], spu->reg[reverb_reg_index(SPU_EVOL_L_ADDRESS)]);
    *return_right = reverb_scale(resampled[1], spu->reg[reverb_reg_index(SPU_EVOL_R_ADDRESS)]);

    spu->reverb_fir_cursor = (fir_cursor + 1u) % SCHWAY__SPU_REVERB_FIR_TAPS;
    if (phase == 1u) {
        spu->reverb_work_cursor = (work_cursor + 1u) % SCHWAY__SPU_REVERB_WORK_WORDS;
    }
}
