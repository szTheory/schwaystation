/* Independent integer checks for BRR decoding and interpolation. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/bus.h"
#include "../../src/internal.h"

#define TEST_UNDECODED (UINT32_C(31) << 27)

/* Factual coefficient data transcribed from PSX-SPX §1.3; see PROVENANCE §7. */
static const int16_t reference_gaussian[512] = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001,
    0x0001, 0x0001, 0x0001, 0x0002, 0x0002, 0x0002, 0x0003, 0x0003,
    0x0003, 0x0004, 0x0004, 0x0005, 0x0005, 0x0006, 0x0007, 0x0007,
    0x0008, 0x0009, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E,
    0x000F, 0x0010, 0x0011, 0x0012, 0x0013, 0x0015, 0x0016, 0x0018,
    0x0019, 0x001B, 0x001C, 0x001E, 0x0020, 0x0021, 0x0023, 0x0025,
    0x0027, 0x0029, 0x002C, 0x002E, 0x0030, 0x0033, 0x0035, 0x0038,
    0x003A, 0x003D, 0x0040, 0x0043, 0x0046, 0x0049, 0x004D, 0x0050,
    0x0054, 0x0057, 0x005B, 0x005F, 0x0063, 0x0067, 0x006B, 0x006F,
    0x0074, 0x0078, 0x007D, 0x0082, 0x0087, 0x008C, 0x0091, 0x0096,
    0x009C, 0x00A1, 0x00A7, 0x00AD, 0x00B3, 0x00BA, 0x00C0, 0x00C7,
    0x00CD, 0x00D4, 0x00DB, 0x00E3, 0x00EA, 0x00F2, 0x00FA, 0x0101,
    0x010A, 0x0112, 0x011B, 0x0123, 0x012C, 0x0135, 0x013F, 0x0148,
    0x0152, 0x015C, 0x0166, 0x0171, 0x017B, 0x0186, 0x0191, 0x019C,
    0x01A8, 0x01B4, 0x01C0, 0x01CC, 0x01D9, 0x01E5, 0x01F2, 0x0200,
    0x020D, 0x021B, 0x0229, 0x0237, 0x0246, 0x0255, 0x0264, 0x0273,
    0x0283, 0x0293, 0x02A3, 0x02B4, 0x02C4, 0x02D6, 0x02E7, 0x02F9,
    0x030B, 0x031D, 0x0330, 0x0343, 0x0356, 0x036A, 0x037E, 0x0392,
    0x03A7, 0x03BC, 0x03D1, 0x03E7, 0x03FC, 0x0413, 0x042A, 0x0441,
    0x0458, 0x0470, 0x0488, 0x04A0, 0x04B9, 0x04D2, 0x04EC, 0x0506,
    0x0520, 0x053B, 0x0556, 0x0572, 0x058E, 0x05AA, 0x05C7, 0x05E4,
    0x0601, 0x061F, 0x063E, 0x065C, 0x067C, 0x069B, 0x06BB, 0x06DC,
    0x06FD, 0x071E, 0x0740, 0x0762, 0x0784, 0x07A7, 0x07CB, 0x07EF,
    0x0813, 0x0838, 0x085D, 0x0883, 0x08A9, 0x08D0, 0x08F7, 0x091E,
    0x0946, 0x096F, 0x0998, 0x09C1, 0x09EB, 0x0A16, 0x0A40, 0x0A6C,
    0x0A98, 0x0AC4, 0x0AF1, 0x0B1E, 0x0B4C, 0x0B7A, 0x0BA9, 0x0BD8,
    0x0C07, 0x0C38, 0x0C68, 0x0C99, 0x0CCB, 0x0CFD, 0x0D30, 0x0D63,
    0x0D97, 0x0DCB, 0x0E00, 0x0E35, 0x0E6B, 0x0EA1, 0x0ED7, 0x0F0F,
    0x0F46, 0x0F7F, 0x0FB7, 0x0FF1, 0x102A, 0x1065, 0x109F, 0x10DB,
    0x1116, 0x1153, 0x118F, 0x11CD, 0x120B, 0x1249, 0x1288, 0x12C7,
    0x1307, 0x1347, 0x1388, 0x13C9, 0x140B, 0x144D, 0x1490, 0x14D4,
    0x1517, 0x155C, 0x15A0, 0x15E6, 0x162C, 0x1672, 0x16B9, 0x1700,
    0x1747, 0x1790, 0x17D8, 0x1821, 0x186B, 0x18B5, 0x1900, 0x194B,
    0x1996, 0x19E2, 0x1A2E, 0x1A7B, 0x1AC8, 0x1B16, 0x1B64, 0x1BB3,
    0x1C02, 0x1C51, 0x1CA1, 0x1CF1, 0x1D42, 0x1D93, 0x1DE5, 0x1E37,
    0x1E89, 0x1EDC, 0x1F2F, 0x1F82, 0x1FD6, 0x202A, 0x207F, 0x20D4,
    0x2129, 0x217F, 0x21D5, 0x222C, 0x2282, 0x22DA, 0x2331, 0x2389,
    0x23E1, 0x2439, 0x2492, 0x24EB, 0x2545, 0x259E, 0x25F8, 0x2653,
    0x26AD, 0x2708, 0x2763, 0x27BE, 0x281A, 0x2876, 0x28D2, 0x292E,
    0x298B, 0x29E7, 0x2A44, 0x2AA1, 0x2AFF, 0x2B5C, 0x2BBA, 0x2C18,
    0x2C76, 0x2CD4, 0x2D33, 0x2D91, 0x2DF0, 0x2E4F, 0x2EAE, 0x2F0D,
    0x2F6C, 0x2FCC, 0x302B, 0x308B, 0x30EA, 0x314A, 0x31AA, 0x3209,
    0x3269, 0x32C9, 0x3329, 0x3389, 0x33E9, 0x3449, 0x34A9, 0x3509,
    0x3569, 0x35C9, 0x3629, 0x3689, 0x36E8, 0x3748, 0x37A8, 0x3807,
    0x3867, 0x38C6, 0x3926, 0x3985, 0x39E4, 0x3A43, 0x3AA2, 0x3B00,
    0x3B5F, 0x3BBD, 0x3C1B, 0x3C79, 0x3CD7, 0x3D35, 0x3D92, 0x3DEF,
    0x3E4C, 0x3EA9, 0x3F05, 0x3F62, 0x3FBD, 0x4019, 0x4074, 0x40D0,
    0x412A, 0x4185, 0x41DF, 0x4239, 0x4292, 0x42EB, 0x4344, 0x439C,
    0x43F4, 0x444C, 0x44A3, 0x44FA, 0x4550, 0x45A6, 0x45FC, 0x4651,
    0x46A6, 0x46FA, 0x474E, 0x47A1, 0x47F4, 0x4846, 0x4898, 0x48E9,
    0x493A, 0x498A, 0x49D9, 0x4A29, 0x4A77, 0x4AC5, 0x4B13, 0x4B5F,
    0x4BAC, 0x4BF7, 0x4C42, 0x4C8D, 0x4CD7, 0x4D20, 0x4D68, 0x4DB0,
    0x4DF7, 0x4E3E, 0x4E84, 0x4EC9, 0x4F0E, 0x4F52, 0x4F95, 0x4FD7,
    0x5019, 0x505A, 0x509A, 0x50DA, 0x5118, 0x5156, 0x5194, 0x51D0,
    0x520C, 0x5247, 0x5281, 0x52BA, 0x52F3, 0x532A, 0x5361, 0x5397,
    0x53CC, 0x5401, 0x5434, 0x5467, 0x5499, 0x54CA, 0x54FA, 0x5529,
    0x5558, 0x5585, 0x55B2, 0x55DE, 0x5609, 0x5632, 0x565B, 0x5684,
    0x56AB, 0x56D1, 0x56F6, 0x571B, 0x573E, 0x5761, 0x5782, 0x57A3,
    0x57C3, 0x57E2, 0x57FF, 0x581C, 0x5838, 0x5853, 0x586D, 0x5886,
    0x589E, 0x58B5, 0x58CB, 0x58E0, 0x58F4, 0x5907, 0x5919, 0x592A,
    0x593A, 0x5949, 0x5958, 0x5965, 0x5971, 0x597C, 0x5986, 0x598F,
    0x5997, 0x599E, 0x59A4, 0x59A9, 0x59AD, 0x59B0, 0x59B2, 0x59B3,
};

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
    schway_profile profile;
    schway *inst = NULL;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = plain_alloc;
    config.allocator.free = plain_free;
    if (schway_create(&config, &inst) != SCHWAY_STATUS_OK || inst == NULL) {
        return NULL;
    }
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = SCHWAY_REGION_NTSC_U;
    if (schway_set_profile(inst, &profile) != SCHWAY_STATUS_OK) {
        schway_destroy(inst);
        return NULL;
    }
    return inst;
}

static int32_t reference_sar(int32_t value, uint32_t shift)
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

static int16_t reference_clamp16(int32_t value)
{
    if (value > INT16_MAX) {
        return INT16_MAX;
    }
    if (value < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t)value;
}

static int16_t reference_decode(uint8_t nibble, uint8_t shift, uint8_t filter,
                                int16_t previous, int16_t older)
{
    static const int16_t positive[5] = {0, 60, 115, 98, 122};
    static const int16_t negative[5] = {0, 0, -52, -55, -60};
    int32_t signed_nibble = nibble < 8u ? (int32_t)nibble : (int32_t)nibble - 16;
    uint32_t safe_shift = shift >= 13u ? 9u : shift;
    uint32_t safe_filter = filter > 4u ? 4u : filter;
    int32_t residual = reference_sar(signed_nibble * 4096, safe_shift);
    int32_t prediction = reference_sar((int32_t)previous * positive[safe_filter] +
                                            (int32_t)older * negative[safe_filter],
                                        6u);
    return reference_clamp16(residual + prediction);
}

static int16_t reference_interpolate(const int16_t history[3], int16_t sample,
                                     uint32_t phase)
{
    int32_t output = 0;
    output += reference_sar((int32_t)reference_gaussian[0x0FFu - phase] * history[2], 15u);
    output += reference_sar((int32_t)reference_gaussian[0x1FFu - phase] * history[1], 15u);
    output += reference_sar((int32_t)reference_gaussian[0x100u + phase] * history[0], 15u);
    output += reference_sar((int32_t)reference_gaussian[phase] * sample, 15u);
    return reference_clamp16(output);
}

static void prepare_voice(schway *inst, uint8_t shift, uint8_t filter, uint8_t first_byte,
                          uint32_t phase)
{
    struct schway__spu_voice *voice = &inst->spu.voice[0];
    memset(voice, 0, sizeof *voice);
    voice->reg[2] = 0x1000u;
    voice->reg[3] = 0x20u;
    voice->sample_counter = TEST_UNDECODED | (phase << 4);
    inst->spu.ram[0x100u] = (uint8_t)((filter << 4) | shift);
    inst->spu.ram[0x101u] = 0u;
    memset(&inst->spu.ram[0x102u], 0x77, 14u);
    inst->spu.ram[0x102u] = first_byte;
}

static void run_voice_tick(schway *inst)
{
    uint64_t deadline = schway__spu_next(inst);
    inst->ticks = deadline;
    schway__spu_event(inst, deadline);
}

static uint32_t test_first_block_trace(schway *inst)
{
    prepare_voice(inst, 0u, 0u, 0x77u, 0xFFu);
    run_voice_tick(inst);
    uint32_t failures = 0u;
    if (inst->spu.voice[0].sample != 28672) {
        printf("# first raw sample: %d\n", inst->spu.voice[0].sample);
    }
    if (inst->spu.voice[0].output != 4206) {
        printf("# first interpolated sample: %d\n", inst->spu.voice[0].output);
    }
    failures += inst->spu.voice[0].sample != 28672 ? 1u : 0u;
    failures += inst->spu.voice[0].output != 4206 ? 1u : 0u;
    return failures;
}

static uint32_t test_filter_nibble_and_shift_matrix(schway *inst)
{
    uint32_t failures = 0u;
    for (uint32_t shift = 0u; shift < 16u; shift++) {
        for (uint32_t filter = 0u; filter < 8u; filter++) {
            for (uint32_t nibble = 0u; nibble < 16u; nibble++) {
                int16_t previous = -300;
                int16_t older = 500;
                uint8_t first_byte = (uint8_t)(nibble | 0x70u);
                prepare_voice(inst, (uint8_t)shift, (uint8_t)filter, first_byte, 0xFFu);
                struct schway__spu_voice *voice = &inst->spu.voice[0];
                voice->sample = previous;
                voice->history[0] = older;
                voice->history[1] = -700;
                voice->history[2] = 900;
                int16_t expected_sample =
                    reference_decode((uint8_t)nibble, (uint8_t)shift, (uint8_t)filter,
                                     previous, older);
                int16_t expected_history[3] = {previous, older, -700};
                run_voice_tick(inst);
                int16_t expected_output =
                    reference_interpolate(expected_history, expected_sample, 0xFFu);
                failures += voice->sample != expected_sample ? 1u : 0u;
                failures += voice->output != expected_output ? 1u : 0u;
            }
        }
    }
    return failures;
}

static uint32_t test_gaussian_phases(schway *inst)
{
    uint32_t failures = 0u;
    for (uint32_t phase = 0u; phase < 256u; phase++) {
        int32_t sum = (int32_t)reference_gaussian[phase] +
                      reference_gaussian[0x0FFu - phase] +
                      reference_gaussian[0x100u + phase] +
                      reference_gaussian[0x1FFu - phase];
        failures += sum < 0x7F7F || sum > 0x7F81 ? 1u : 0u;
        prepare_voice(inst, 0u, 0u, 0x77u, phase);
        struct schway__spu_voice *voice = &inst->spu.voice[0];
        voice->sample = 12345;
        voice->history[0] = -4321;
        voice->history[1] = 2345;
        voice->history[2] = -6789;
        int16_t expected_sample = reference_decode(7u, 0u, 0u, 12345, -4321);
        int16_t expected_history[3] = {12345, -4321, 2345};
        run_voice_tick(inst);
        int16_t expected_output =
            reference_interpolate(expected_history, expected_sample, phase);
        failures += voice->sample != expected_sample ? 1u : 0u;
        failures += voice->output != expected_output ? 1u : 0u;
    }
    return failures;
}

static uint32_t test_block_history_and_irq(schway *inst)
{
    struct schway__spu_voice *voice = &inst->spu.voice[0];
    memset(voice, 0, sizeof *voice);
    voice->reg[2] = 0x1000u;
    voice->reg[3] = 0x20u;
    voice->sample_counter = TEST_UNDECODED;
    inst->spu.ram[0x100u] = 0u;
    inst->spu.ram[0x101u] = 0u;
    memset(&inst->spu.ram[0x102u], 0x77, 14u);
    inst->spu.ram[0x110u] = 0x40u; /* filter 4 */
    inst->spu.ram[0x111u] = 0u;
    memset(&inst->spu.ram[0x112u], 0xEE, 14u);
    inst->spu.irqa_addr = 0x100u;
    inst->spu.irq_enabled = 1u;

    for (uint32_t i = 0u; i < 29u; i++) {
        run_voice_tick(inst);
    }
    int16_t expected = reference_decode(0x0Eu, 0u, 4u, 28672, 28672);
    uint32_t failures = 0u;
    failures += voice->sample != expected ? 1u : 0u;
    failures += voice->reg[3] != 0x22u ? 1u : 0u;
    failures += (inst->irq.stat & (UINT32_C(1) << 9)) == 0u ? 1u : 0u;
    return failures;
}

#define SPU_MMIO_BASE UINT32_C(0x1F801C00)
#define SPU_ATTR UINT32_C(0x1F801DAA)
#define SPU_IRQA UINT32_C(0x1F801DA4)
#define SPU_TSA UINT32_C(0x1F801DA6)
#define SPU_FIFO UINT32_C(0x1F801DA8)
#define SPU_KON UINT32_C(0x1F801D88)
#define SPU_KOFF UINT32_C(0x1F801D8C)
#define SPU_PMON UINT32_C(0x1F801D90)
#define SPU_NON UINT32_C(0x1F801D94)
#define SPU_ENDX UINT32_C(0x1F801D9C)
#define SPU_MAIN_VOL_L UINT32_C(0x1F801D80)
#define SPU_MAIN_VOL_R UINT32_C(0x1F801D82)

static uint32_t mmio_write16(schway *inst, uint32_t address, uint16_t value)
{
    uint32_t cycles = 0u;
    return schway__bus_write(inst, address, 2u, value, &cycles) == 0 ? 0u : 1u;
}

static uint32_t mmio_read16(schway *inst, uint32_t address, uint16_t *value)
{
    uint32_t cycles = 0u;
    uint32_t result = 0u;
    if (schway__bus_read(inst, address, 2u, &result, &cycles) != 0) {
        return 1u;
    }
    *value = (uint16_t)result;
    return 0u;
}

static uint32_t test_guest_upload_kon_to_public_pcm(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return 1u;
    }
    uint32_t failures = 0u;
    failures += mmio_write16(inst, SPU_ATTR, 0xC050u);
    failures += mmio_write16(inst, SPU_TSA, 0x20u);
    failures += mmio_write16(inst, SPU_IRQA, 0x20u);
    failures += mmio_write16(inst, SPU_MMIO_BASE + 0x00u, 0x3FFFu);
    failures += mmio_write16(inst, SPU_MMIO_BASE + 0x02u, 0x1FFFu);
    failures += mmio_write16(inst, SPU_MMIO_BASE + 0x04u, 0x1000u);
    failures += mmio_write16(inst, SPU_MMIO_BASE + 0x06u, 0x20u);
    failures += mmio_write16(inst, SPU_MMIO_BASE + 0x08u, 0x7F00u);
    failures += mmio_write16(inst, SPU_MAIN_VOL_L, 0x3FFFu);
    failures += mmio_write16(inst, SPU_MAIN_VOL_R, 0x3FFFu);
    const uint16_t block[8] = {0x0000u, 0x7777u, 0x7777u, 0x7777u,
                               0x7777u, 0x7777u, 0x7777u, 0x7777u};
    for (uint32_t i = 0; i < 8u; i++) {
        failures += mmio_write16(inst, SPU_FIFO, block[i]);
    }
    failures += mmio_write16(inst, SPU_KON, 1u);
    run_voice_tick(inst);
    failures += mmio_write16(inst, SPU_MMIO_BASE + 0x0Cu, 0x7FFFu);
    run_voice_tick(inst);

    int16_t samples[4] = {0, 0, 0, 0};
    schway_audio audio;
    memset(&audio, 0, sizeof audio);
    audio.size = (uint32_t)sizeof audio;
    audio.samples = samples;
    audio.capacity = 2u;
    failures += schway_get_audio(inst, &audio) != SCHWAY_STATUS_OK ? 1u : 0u;
    failures += audio.written != 2u ? 1u : 0u;
    failures += samples[2] == 0 || samples[3] == 0 || samples[2] == samples[3] ? 1u : 0u;

    const struct schway__spu_voice *voice = &inst->spu.voice[0];
    int16_t raw = reference_interpolate(voice->history, voice->sample, 0u);
    int32_t env_sample = reference_sar((int32_t)raw * 0x7FFF, 15u);
    int32_t left_voice = reference_sar(env_sample * (int32_t)0x7FFEu, 15u);
    int32_t right_voice = reference_sar(env_sample * (int32_t)0x3FFEu, 15u);
    int16_t expected_left = reference_clamp16(
        reference_sar(left_voice * (int32_t)0x7FFEu, 15u));
    int16_t expected_right = reference_clamp16(
        reference_sar(right_voice * (int32_t)0x7FFEu, 15u));
    failures += samples[2] != expected_left || samples[3] != expected_right ? 1u : 0u;
    failures += (inst->irq.stat & (UINT32_C(1) << 9)) == 0u ? 1u : 0u;

    run_voice_tick(inst);
    failures += mmio_write16(inst, SPU_KOFF, 1u);
    run_voice_tick(inst);
    failures += voice->envelope_phase != 4u ? 1u : 0u;
    uint16_t endx = 0u;
    failures += mmio_read16(inst, SPU_ENDX, &endx);
    failures += (endx & 1u) != 0u ? 1u : 0u;
    if (failures != 0u) {
        printf("# MMIO path: failures=%u written=%u PCM=(%d,%d) expected=(%d,%d) "
               "OUTX=%d ENVX=%d ENDX=%04X IRQ=%u\n",
               failures, audio.written, samples[2], samples[3], expected_left, expected_right,
               voice->output, voice->envelope,
               (unsigned)endx, (unsigned)((inst->irq.stat >> 9) & 1u));
    }
    schway_destroy(inst);
    return failures;
}

static void prepare_constant_voice(struct schway__spu_voice *voice, uint16_t pitch,
                                   int16_t sample)
{
    memset(voice, 0, sizeof *voice);
    voice->reg[2] = pitch;
    voice->sample_counter = 0u;
    voice->history[0] = sample;
    voice->history[1] = sample;
    voice->history[2] = sample;
    voice->sample = sample;
    voice->envelope = 0x7FFF;
}

static uint32_t test_all_voices_fetch_in_order(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return 1u;
    }
    inst->spu.irq_enabled = 1u;
    inst->spu.irqa_addr = 0x110u;
    for (uint32_t i = 0; i < SCHWAY__SPU_VOICE_COUNT; i++) {
        uint32_t block_address = 0x100u + i * 0x10u;
        uint32_t nibble = i % 7u + 1u;
        struct schway__spu_voice *voice = &inst->spu.voice[i];
        voice->reg[2] = 0x1000u;
        voice->reg[3] = (uint16_t)(block_address >> 3);
        voice->sample_counter = UINT32_C(31) << 27;
        inst->spu.ram[block_address] = 0u;
        inst->spu.ram[block_address + 1u] = 0u;
        memset(&inst->spu.ram[block_address + 2u], (int)(nibble | (nibble << 4)), 14u);
    }
    uint32_t failures = 0u;
    failures += mmio_write16(inst, SPU_PMON, 0xFFFEu);
    failures += mmio_write16(inst, SPU_PMON + 2u, 0x00FFu);
    failures += mmio_write16(inst, SPU_NON, 2u);
    run_voice_tick(inst);
    for (uint32_t i = 0; i < SCHWAY__SPU_VOICE_COUNT; i++) {
        int16_t expected = (int16_t)((int32_t)(i % 7u + 1u) * 4096);
        failures += inst->spu.voice[i].sample != expected ? 1u : 0u;
        uint32_t expected_phase = 0x1000u;
        if (i != 0u) {
            int32_t modulated = reference_sar(
                0x1000 * ((int32_t)inst->spu.voice[i - 1u].output + 0x8000), 15u);
            expected_phase = (uint32_t)modulated & 0xFFFFu;
            if (expected_phase > 0x4000u) {
                expected_phase = 0x4000u;
            }
        }
        failures += (inst->spu.voice[i].sample_counter & UINT32_C(0x1FFFF)) != expected_phase
                        ? 1u
                        : 0u;
    }
    failures += (inst->irq.stat & (UINT32_C(1) << 9)) == 0u ? 1u : 0u;
    schway_destroy(inst);
    return failures;
}

static uint32_t test_pitch_modulation_and_pitch_limits(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return 1u;
    }
    prepare_constant_voice(&inst->spu.voice[0], 0x1000u, 16384);
    prepare_constant_voice(&inst->spu.voice[1], 0x1000u, 1000);
    prepare_constant_voice(&inst->spu.voice[2], 0u, 1000);
    prepare_constant_voice(&inst->spu.voice[3], 0x5000u, 1000);
    inst->spu.reg[(SPU_PMON - SPU_MMIO_BASE) >> 1] = 2u;
    run_voice_tick(inst);

    uint32_t failures = 0u;
    int32_t modulated = reference_sar(
        (int32_t)inst->spu.voice[1].reg[2] *
            ((int32_t)inst->spu.voice[0].output + 0x8000),
        15u);
    uint32_t expected = (uint32_t)modulated & 0xFFFFu;
    if (expected > 0x4000u) {
        expected = 0x4000u;
    }
    failures += (inst->spu.voice[1].sample_counter & 0x1FFFFu) != expected ? 1u : 0u;
    failures += inst->spu.voice[1].output == 0 ? 1u : 0u;
    failures += (inst->spu.voice[2].sample_counter & 0x1FFFFu) != 0u ? 1u : 0u;
    failures += inst->spu.voice[2].output != 0 ? 1u : 0u;
    failures += (inst->spu.voice[3].sample_counter & 0x1FFFFu) != 0x4000u ? 1u : 0u;
    schway_destroy(inst);
    return failures;
}

static uint32_t test_loop_start_repeat_and_endx(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return 1u;
    }
    struct schway__spu_voice *voice = &inst->spu.voice[0];
    voice->reg[2] = 0x1000u;
    voice->reg[3] = 0x20u;
    voice->sample_counter = UINT32_C(31) << 27;
    voice->envelope_phase = 1u;
    voice->envelope = 0x7FFF;
    inst->spu.ram[0x100u] = 0u;
    inst->spu.ram[0x101u] = 4u;
    memset(&inst->spu.ram[0x102u], 0x11, 14u);
    inst->spu.ram[0x110u] = 0u;
    inst->spu.ram[0x111u] = 3u;
    memset(&inst->spu.ram[0x112u], 0x22, 14u);
    for (uint32_t i = 0; i < 56u; i++) {
        run_voice_tick(inst);
    }
    uint32_t endx_index = (SPU_ENDX - SPU_MMIO_BASE) >> 1;
    uint32_t failures = 0u;
    failures += voice->reg[7] != 0x20u ? 1u : 0u;
    failures += voice->reg[3] != 0x20u ? 1u : 0u;
    failures += (inst->spu.reg[endx_index] & 1u) == 0u ? 1u : 0u;

    inst->spu.ram[0x101u] = 1u;
    for (uint32_t i = 0; i < 28u; i++) {
        run_voice_tick(inst);
    }
    failures += voice->reg[3] != 0x20u ? 1u : 0u;
    failures += voice->envelope_phase != 4u || voice->envelope != 0 ? 1u : 0u;
    failures += (inst->spu.reg[endx_index] & 1u) == 0u ? 1u : 0u;
    schway_destroy(inst);
    return failures;
}

static uint32_t test_signed_stereo_mix_and_mute(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return 1u;
    }
    inst->spu.reg[(SPU_ATTR - SPU_MMIO_BASE) >> 1] = 0xC000u;
    inst->spu.reg[(SPU_MAIN_VOL_L - SPU_MMIO_BASE) >> 1] = 0x3FFFu;
    inst->spu.reg[(SPU_MAIN_VOL_R - SPU_MMIO_BASE) >> 1] = 0x3FFFu;
    for (uint32_t i = 0; i < SCHWAY__SPU_VOICE_COUNT; i++) {
        prepare_constant_voice(&inst->spu.voice[i], 0x1000u, INT16_MAX);
        inst->spu.voice[i].reg[0] = 0x3FFFu;
        inst->spu.voice[i].reg[1] = 0x4000u;
    }
    run_voice_tick(inst);
    int16_t samples[4] = {0, 0, 0, 0};
    schway_audio audio;
    memset(&audio, 0, sizeof audio);
    audio.size = (uint32_t)sizeof audio;
    audio.samples = samples;
    audio.capacity = 1u;
    uint32_t failures = schway_get_audio(inst, &audio) != SCHWAY_STATUS_OK ? 1u : 0u;
    failures += audio.written != 1u ? 1u : 0u;
    failures += samples[0] != INT16_MAX || samples[1] != INT16_MIN ? 1u : 0u;
    failures += mmio_write16(inst, SPU_ATTR, 0x8000u);
    run_voice_tick(inst);
    memset(samples, 0, sizeof samples);
    audio.capacity = 1u;
    failures += schway_get_audio(inst, &audio) != SCHWAY_STATUS_OK ? 1u : 0u;
    failures += samples[0] != 0 || samples[1] != 0 ? 1u : 0u;
    schway_destroy(inst);
    return failures;
}

int main(void)
{
    puts("TAP version 13");
    puts("1..6");
    schway *inst = make_instance();
    if (inst == NULL) {
        puts("# instance creation failed");
        puts("not ok 1 - BRR filters, histories and Gaussian interpolation match an independent integer reference");
        return 1;
    }

    uint32_t trace_failures = test_first_block_trace(inst);
    uint32_t matrix_failures = test_filter_nibble_and_shift_matrix(inst);
    uint32_t gaussian_failures = test_gaussian_phases(inst);
    uint32_t history_failures = test_block_history_and_irq(inst);
    uint32_t failures = trace_failures + matrix_failures + gaussian_failures + history_failures;
    if (failures != 0u) {
        printf("# first-block mismatches: %u\n", trace_failures);
        printf("# filter/nibble/shift mismatches: %u\n", matrix_failures);
        printf("# Gaussian/history mismatches: %u\n", gaussian_failures);
        printf("# block-boundary/IRQA mismatches: %u\n", history_failures);
    }
    printf("%s 1 - BRR filters, histories and Gaussian interpolation match an independent integer reference\n",
           failures == 0u ? "ok" : "not ok");

    uint32_t guest_failures = test_guest_upload_kon_to_public_pcm();
    uint32_t voices_failures = test_all_voices_fetch_in_order();
    uint32_t pmon_failures = test_pitch_modulation_and_pitch_limits();
    uint32_t loop_failures = test_loop_start_repeat_and_endx();
    uint32_t mix_failures = test_signed_stereo_mix_and_mute();
    printf("%s 2 - MMIO upload and KON produce checked stereo PCM through the public audio drain\n",
           guest_failures == 0u ? "ok" : "not ok");
    printf("%s 3 - all 24 voices fetch BRR data, including silent and noise-enabled voices\n",
           voices_failures == 0u ? "ok" : "not ok");
    printf("%s 4 - pitch modulation, zero pitch and clamped pitch follow the integer reference\n",
           pmon_failures == 0u ? "ok" : "not ok");
    printf("%s 5 - loop start, repeat, end mute and ENDX follow BRR flags\n",
           loop_failures == 0u ? "ok" : "not ok");
    printf("%s 6 - signed stereo volumes saturate deterministically and ATTR mute is silent\n",
           mix_failures == 0u ? "ok" : "not ok");
    schway_destroy(inst);
    return failures + guest_failures + voices_failures + pmon_failures + loop_failures +
                   mix_failures == 0u
               ? 0
               : 1;
}
