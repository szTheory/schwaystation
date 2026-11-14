/* Reverb resampling primitives, following psx-spx's 39-tap FIR facts.
   Hardware references: .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md section 1.7. */
#include "internal.h"

#define SCHWAY__SPU_REVERB_TABLES
#include "spu_tables.inc"

#define SPU_REGISTER_BASE 0x1F801C00u
#define SPU_ATTR_ADDRESS 0x1F801DAAu
#define SPU_ESA_ADDRESS 0x1F801DA2u
#define SPU_VLIN_ADDRESS 0x1F801DFCu
#define SPU_VRIN_ADDRESS 0x1F801DFEu
#define SPU_EVOL_L_ADDRESS 0x1F801D84u
#define SPU_EVOL_R_ADDRESS 0x1F801D86u
#define SPU_DAPF1_ADDRESS 0x1F801DC0u
#define SPU_DAPF2_ADDRESS 0x1F801DC2u
#define SPU_VIIR_ADDRESS 0x1F801DC4u
#define SPU_VCOMB1_ADDRESS 0x1F801DC6u
#define SPU_VCOMB2_ADDRESS 0x1F801DC8u
#define SPU_VCOMB3_ADDRESS 0x1F801DCAu
#define SPU_VCOMB4_ADDRESS 0x1F801DCCu
#define SPU_VWALL_ADDRESS 0x1F801DCEu
#define SPU_VAPF1_ADDRESS 0x1F801DD0u
#define SPU_VAPF2_ADDRESS 0x1F801DD2u
#define SPU_MLSAME_ADDRESS 0x1F801DD4u
#define SPU_MRSAME_ADDRESS 0x1F801DD6u
#define SPU_MLCOMB1_ADDRESS 0x1F801DD8u
#define SPU_MRCOMB1_ADDRESS 0x1F801DDAu
#define SPU_MLCOMB2_ADDRESS 0x1F801DDCu
#define SPU_MRCOMB2_ADDRESS 0x1F801DDEu
#define SPU_DLSAME_ADDRESS 0x1F801DE0u
#define SPU_DRSAME_ADDRESS 0x1F801DE2u
#define SPU_MLDIFF_ADDRESS 0x1F801DE4u
#define SPU_MRDIFF_ADDRESS 0x1F801DE6u
#define SPU_MLCOMB3_ADDRESS 0x1F801DE8u
#define SPU_MRCOMB3_ADDRESS 0x1F801DEAu
#define SPU_MLCOMB4_ADDRESS 0x1F801DECu
#define SPU_MRCOMB4_ADDRESS 0x1F801DEEu
#define SPU_DLDIFF_ADDRESS 0x1F801DF0u
#define SPU_DRDIFF_ADDRESS 0x1F801DF2u
#define SPU_MLAPF1_ADDRESS 0x1F801DF4u
#define SPU_MRAPF1_ADDRESS 0x1F801DF6u
#define SPU_MLAPF2_ADDRESS 0x1F801DF8u
#define SPU_MRAPF2_ADDRESS 0x1F801DFAu

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

static int32_t reverb_signed16(uint16_t value)
{
    return (value & 0x8000u) != 0u ? (int32_t)value - 0x10000 : (int32_t)value;
}

static int16_t reverb_scale(int16_t sample, uint16_t volume)
{
    int32_t signed_volume = reverb_signed16(volume);
    /* Signed Q15 -1.0 is ordinary negation here, including the measured
       vLIN/vRIN case. The separate vIIR quirk is handled after its sum. */
    return reverb_clamp16(reverb_sar64((int64_t)sample * signed_volume, 15u));
}

static uint32_t reverb_work_address(const struct schway__spu *spu, int32_t offset)
{
    uint32_t start = ((uint32_t)spu->reg[reverb_reg_index(SPU_ESA_ADDRESS)] << 3) &
                     (SCHWAY__SPU_RAM_SIZE - 8u);
    uint32_t size = SCHWAY__SPU_RAM_SIZE - start;
    int64_t relative = (int64_t)spu->reverb_address + (int64_t)offset * 8 - start;
    int64_t wrapped = relative % size;
    if (wrapped < 0) {
        wrapped += size;
    }
    return start + (uint32_t)wrapped;
}

static int16_t reverb_read(schway *inst, int32_t offset)
{
    uint32_t address = reverb_work_address(&inst->spu, offset);
    return (int16_t)reverb_signed16(schway__spu_ram_read(inst, address));
}

static void reverb_write(schway *inst, int32_t offset, int16_t value)
{
    uint32_t address = reverb_work_address(&inst->spu, offset);
    uint16_t attr = inst->spu.reg[reverb_reg_index(SPU_ATTR_ADDRESS)];
    if ((attr & 0x0080u) != 0u) {
        schway__spu_ram_write(inst, address, (uint16_t)value);
    } else {
        /* ATTR bit 7 substitutes a RAM read for each inhibited reverb write. */
        (void)schway__spu_ram_read(inst, address);
    }
}

static int16_t reverb_saturating_add(int32_t left, int32_t right)
{
    return reverb_clamp16((int64_t)left + right);
}

static int16_t reverb_iir(schway *inst, int16_t input, uint32_t delay_address,
                          uint32_t output_address)
{
    uint32_t wall_address = reverb_reg_index(SPU_VWALL_ADDRESS);
    uint32_t iir_address = reverb_reg_index(SPU_VIIR_ADDRESS);
    int32_t output_offset = (int32_t)inst->spu.reg[reverb_reg_index(output_address)];
    int16_t previous = reverb_read(inst, output_offset - 2);
    int16_t reflection = reverb_read(
        inst, (int32_t)inst->spu.reg[reverb_reg_index(delay_address)]);
    int16_t wall = reverb_scale(reflection, inst->spu.reg[wall_address]);
    int16_t excitation = reverb_saturating_add((int32_t)input + wall, -previous);
    uint16_t coefficient = inst->spu.reg[iir_address];

    /* Deterministic baseline: floor each Q15 product, saturate the excitation,
       then add the old sample. These stages follow the algebraic psx-spx
       equation; their exact hardware rounding/saturation placement is unknown. */
    int16_t filtered = reverb_saturating_add(reverb_scale(excitation, coefficient), previous);
    if (coefficient == 0x8000u) {
        /* The documented -8000h IIR quirk negates the completed stored value,
           including the prior-sample addition. */
        filtered = reverb_clamp16(-(int32_t)filtered);
    }
    reverb_write(inst, output_offset, filtered);
    return filtered;
}

static int16_t reverb_comb(schway *inst, uint32_t *offsets, uint32_t *volumes)
{
    int16_t output = 0;
    for (uint32_t comb = 0u; comb < 4u; comb++) {
        int16_t delayed = reverb_read(
            inst, (int32_t)inst->spu.reg[reverb_reg_index(offsets[comb])]);
        int16_t term = reverb_scale(
            delayed, inst->spu.reg[reverb_reg_index(volumes[comb])]);
        output = reverb_saturating_add(output, term);
    }
    return output;
}

static int16_t reverb_allpass(schway *inst, int16_t input, uint32_t delay_address,
                              uint32_t volume_address, uint32_t memory_address)
{
    uint32_t memory_index = reverb_reg_index(memory_address);
    int32_t memory_offset = (int32_t)inst->spu.reg[memory_index];
    int32_t delay = (int32_t)inst->spu.reg[reverb_reg_index(delay_address)];
    int16_t delayed = reverb_read(inst, memory_offset - delay);
    uint16_t volume = inst->spu.reg[reverb_reg_index(volume_address)];
    int16_t feedback = reverb_scale(delayed, volume);
    int16_t stored = reverb_saturating_add(input, -feedback);
    reverb_write(inst, memory_offset, stored);
    int16_t feedforward = reverb_scale(stored, volume);
    return reverb_saturating_add(feedforward, delayed);
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
    uint32_t same_delay = phase == 0u ? SPU_DLSAME_ADDRESS : SPU_DRSAME_ADDRESS;
    uint32_t same_output = phase == 0u ? SPU_MLSAME_ADDRESS : SPU_MRSAME_ADDRESS;
    uint32_t diff_delay = phase == 0u ? SPU_DRDIFF_ADDRESS : SPU_DLDIFF_ADDRESS;
    uint32_t diff_output = phase == 0u ? SPU_MLDIFF_ADDRESS : SPU_MRDIFF_ADDRESS;
    uint32_t comb_offsets[4] = {
        phase == 0u ? SPU_MLCOMB1_ADDRESS : SPU_MRCOMB1_ADDRESS,
        phase == 0u ? SPU_MLCOMB2_ADDRESS : SPU_MRCOMB2_ADDRESS,
        phase == 0u ? SPU_MLCOMB3_ADDRESS : SPU_MRCOMB3_ADDRESS,
        phase == 0u ? SPU_MLCOMB4_ADDRESS : SPU_MRCOMB4_ADDRESS};
    uint32_t comb_volumes[4] = {SPU_VCOMB1_ADDRESS, SPU_VCOMB2_ADDRESS,
                                SPU_VCOMB3_ADDRESS, SPU_VCOMB4_ADDRESS};
    (void)reverb_iir(inst, filtered, same_delay, same_output);
    (void)reverb_iir(inst, filtered, diff_delay, diff_output);
    int16_t feedback = reverb_comb(inst, comb_offsets, comb_volumes);
    feedback = reverb_allpass(inst, feedback,
                              SPU_DAPF1_ADDRESS, SPU_VAPF1_ADDRESS,
                              phase == 0u ? SPU_MLAPF1_ADDRESS : SPU_MRAPF1_ADDRESS);
    feedback = reverb_allpass(inst, feedback,
                              SPU_DAPF2_ADDRESS, SPU_VAPF2_ADDRESS,
                              phase == 0u ? SPU_MLAPF2_ADDRESS : SPU_MRAPF2_ADDRESS);
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
        uint32_t start = ((uint32_t)spu->reg[reverb_reg_index(SPU_ESA_ADDRESS)] << 3) &
                         (SCHWAY__SPU_RAM_SIZE - 8u);
        uint32_t next = (spu->reverb_address + 16u) & (SCHWAY__SPU_RAM_SIZE - 8u);
        spu->reverb_address = next < start ? start : next;
    }
}
