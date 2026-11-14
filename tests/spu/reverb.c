/* Reverb resampler checks against an independent scalar convolution. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/internal.h"

/* Coefficient facts: .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md section 1.7. */
static const int16_t reference_taps[SCHWAY__SPU_REVERB_FIR_TAPS] = {
    -1, 0, 2, 0, -10, 0, 35, 0, -103, 0, 266, 0, -616, 0, 1332, 0,
    -2960, 0, 10246, 16384, 10246, 0, -2960, 0, 1332, 0, -616, 0,
    266, 0, -103, 0, 35, 0, -10, 0, 2, 0, -1};

static uint32_t failures;

#define CHECK(condition, message)                                                                  \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            (void)fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, message);                   \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

static int64_t shift_right_floor(int64_t value, uint32_t shift)
{
    int64_t divisor = INT64_C(1) << shift;
    if (value >= 0) {
        return value / divisor;
    }
    return -(((-value) + divisor - 1) / divisor);
}

static int16_t clamp16(int64_t value)
{
    if (value > INT16_MAX) {
        return INT16_MAX;
    }
    if (value < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t)value;
}

static int16_t reference_sum(const int16_t history[SCHWAY__SPU_REVERB_FIR_TAPS], uint32_t cursor)
{
    int64_t sum = 0;
    for (uint32_t tap = 0u; tap < SCHWAY__SPU_REVERB_FIR_TAPS; tap++) {
        uint32_t index = (cursor + SCHWAY__SPU_REVERB_FIR_TAPS - tap) %
                         SCHWAY__SPU_REVERB_FIR_TAPS;
        sum += (int64_t)history[index] * reference_taps[tap];
    }
    return clamp16(shift_right_floor(sum, 15u));
}

static int16_t reference_interpolate(const int32_t history[SCHWAY__SPU_REVERB_WORK_WORDS],
                                     uint32_t latest, uint32_t phase, uint32_t channel)
{
    int64_t sum = 0;
    for (uint32_t tap = 0u; tap < SCHWAY__SPU_REVERB_FIR_TAPS; tap++) {
        if (((phase + channel + tap) & 1u) != 0u) {
            continue;
        }
        uint32_t age = (tap + channel - phase) / 2u;
        uint32_t index = (latest + SCHWAY__SPU_REVERB_WORK_WORDS -
                          (age % SCHWAY__SPU_REVERB_WORK_WORDS)) %
                         SCHWAY__SPU_REVERB_WORK_WORDS;
        sum += (int64_t)history[index] * reference_taps[tap];
    }
    return clamp16(shift_right_floor(sum * 2, 15u));
}

static void compare_input_fir(void)
{
    int16_t production[SCHWAY__SPU_REVERB_FIR_TAPS] = {0};
    int16_t reference[SCHWAY__SPU_REVERB_FIR_TAPS] = {0};
    uint32_t cursor = 0u;
    for (uint32_t sample = 0u; sample < 96u; sample++) {
        int16_t input = sample == 0u ? 12000 :
                        (sample < 48u ? 0 : ((sample & 1u) == 0u ? INT16_MAX : INT16_MIN));
        production[cursor] = input;
        reference[cursor] = input;
        schway__spu_reverb_fir_push(production, cursor, input);
        int16_t actual = schway__spu_reverb_fir_sum(production, cursor);
        int16_t expected = reference_sum(reference, cursor);
        CHECK(actual == expected, "input FIR matches independent impulse and alternating-sign convolution");
        cursor = (cursor + 1u) % SCHWAY__SPU_REVERB_FIR_TAPS;
    }
}

static void compare_output_polyphases(void)
{
    int32_t history[SCHWAY__SPU_REVERB_WORK_WORDS] = {0};
    for (uint32_t index = 0u; index < SCHWAY__SPU_REVERB_WORK_WORDS; index++) {
        history[index] = (int32_t)((index * 997u) % 30001u) - 15000;
    }
    for (uint32_t latest = 0u; latest < SCHWAY__SPU_REVERB_WORK_WORDS; latest++) {
        for (uint32_t phase = 0u; phase < 2u; phase++) {
            for (uint32_t channel = 0u; channel < 2u; channel++) {
                int16_t expected = reference_interpolate(history, latest, phase, channel);
                int16_t actual = schway__spu_reverb_fir_interpolate(history, latest, phase, channel);
                CHECK(actual == expected, "output FIR uses the correct channel polyphase and gain");
            }
        }
    }
}

static void check_centered_delay(uint32_t channel)
{
    int16_t input_history[SCHWAY__SPU_REVERB_FIR_TAPS] = {0};
    int32_t work_history[SCHWAY__SPU_REVERB_WORK_WORDS] = {0};
    int16_t input_reference[SCHWAY__SPU_REVERB_FIR_TAPS] = {0};
    int32_t work_reference[SCHWAY__SPU_REVERB_WORK_WORDS] = {0};
    int16_t output[96] = {0};
    uint32_t input_cursor = 0u;
    uint32_t work_cursor = 0u;
    int64_t total = 0;
    int64_t moment = 0;

    for (uint32_t tick = 0u; tick < 96u; tick++) {
        uint32_t phase = tick & 1u;
        int16_t input = tick == 0u ? 16000 : 0;
        input_history[input_cursor] = input;
        input_reference[input_cursor] = input;
        schway__spu_reverb_fir_push(input_history, input_cursor, input);
        int16_t filtered = schway__spu_reverb_fir_sum(input_history, input_cursor);
        int16_t reference_filtered = reference_sum(input_reference, input_cursor);
        if (phase == channel) {
            work_history[work_cursor] = filtered;
            work_reference[work_cursor] = reference_filtered;
        }
        output[tick] = schway__spu_reverb_fir_interpolate(work_history, work_cursor,
                                                           phase, channel);
        int16_t expected = reference_interpolate(work_reference, work_cursor, phase, channel);
        CHECK(output[tick] == expected, "cascaded FIR matches independent polyphase convolution");
        total += output[tick];
        moment += (int64_t)tick * output[tick];
        input_cursor = (input_cursor + 1u) % SCHWAY__SPU_REVERB_FIR_TAPS;
        if (phase == 1u) {
            work_cursor = (work_cursor + 1u) % SCHWAY__SPU_REVERB_WORK_WORDS;
        }
    }

    CHECK(total != 0, "unity wet path carries a measurable impulse");
    CHECK(moment == INT64_C(38) * total, "centered input and output filters align at 38 native samples");
}

struct filter_path {
    int16_t input_history[2][SCHWAY__SPU_REVERB_FIR_TAPS];
    int32_t work_history[2][SCHWAY__SPU_REVERB_WORK_WORDS];
    uint32_t input_cursor;
    uint32_t work_cursor;
    uint32_t phase;
};

static void run_filter_path(struct filter_path *path, const int16_t left[96],
                            const int16_t right[96], int16_t output[96][2],
                            uint32_t start, uint32_t end)
{
    for (uint32_t tick = start; tick < end; tick++) {
        schway__spu_reverb_fir_push(path->input_history[0], path->input_cursor, left[tick]);
        schway__spu_reverb_fir_push(path->input_history[1], path->input_cursor, right[tick]);
        int16_t filtered[2] = {schway__spu_reverb_fir_sum(path->input_history[0], path->input_cursor),
                               schway__spu_reverb_fir_sum(path->input_history[1], path->input_cursor)};
        path->work_history[path->phase][path->work_cursor] = filtered[path->phase];
        output[tick][0] = schway__spu_reverb_fir_interpolate(
            path->work_history[0], path->work_cursor, path->phase, 0u);
        output[tick][1] = schway__spu_reverb_fir_interpolate(
            path->work_history[1], path->work_cursor, path->phase, 1u);
        path->input_cursor = (path->input_cursor + 1u) % SCHWAY__SPU_REVERB_FIR_TAPS;
        if (path->phase == 1u) {
            path->work_cursor = (path->work_cursor + 1u) % SCHWAY__SPU_REVERB_WORK_WORDS;
        }
        path->phase ^= 1u;
    }
}

static void check_split_resume(void)
{
    struct filter_path whole;
    struct filter_path split;
    struct filter_path resumed;
    int16_t left[96] = {0};
    int16_t right[96] = {0};
    int16_t whole_output[96][2] = {{0}};
    int16_t split_output[96][2] = {{0}};
    int16_t resumed_output[96][2] = {{0}};
    memset(&whole, 0, sizeof whole);
    memset(&split, 0, sizeof split);
    left[0] = 12000;
    for (uint32_t tick = 36u; tick < 80u; tick++) {
        right[tick] = (tick & 1u) == 0u ? INT16_MAX : INT16_MIN;
    }

    run_filter_path(&whole, left, right, whole_output, 0u, 96u);
    run_filter_path(&split, left, right, split_output, 0u, 47u);
    memcpy(&resumed, &split, sizeof resumed);
    memcpy(resumed_output, split_output, 47u * sizeof split_output[0]);
    run_filter_path(&split, left, right, split_output, 47u, 96u);
    run_filter_path(&resumed, left, right, resumed_output, 47u, 96u);
    CHECK(memcmp(whole_output, split_output, sizeof whole_output) == 0,
          "split FIR calls preserve history and alternating phase");
    CHECK(memcmp(split_output, resumed_output, sizeof split_output) == 0,
          "copied FIR history and phase resume identically");
}

#define TEST_SPU_REGISTER_BASE UINT32_C(0x1F801C00)
#define TEST_SPU_ATTR UINT32_C(0x1F801DAA)
#define TEST_SPU_NON UINT32_C(0x1F801D94)
#define TEST_SPU_EON0 UINT32_C(0x1F801D98)
#define TEST_SPU_EVOL_L UINT32_C(0x1F801D84)
#define TEST_SPU_EVOL_R UINT32_C(0x1F801D86)
#define TEST_SPU_VLIN UINT32_C(0x1F801DFC)
#define TEST_SPU_VRIN UINT32_C(0x1F801DFE)
#define TEST_SPU_ESA UINT32_C(0x1F801DA2)
#define TEST_SPU_IIR UINT32_C(0x1F801DC4)
#define TEST_SPU_COMB1 UINT32_C(0x1F801DC6)
#define TEST_SPU_COMB2 UINT32_C(0x1F801DC8)
#define TEST_SPU_COMB3 UINT32_C(0x1F801DCA)
#define TEST_SPU_COMB4 UINT32_C(0x1F801DCC)
#define TEST_SPU_WALL UINT32_C(0x1F801DCE)
#define TEST_SPU_LSAME UINT32_C(0x1F801DD4)
#define TEST_SPU_RSAME UINT32_C(0x1F801DD6)
#define TEST_SPU_LCOMB1 UINT32_C(0x1F801DD8)
#define TEST_SPU_RCOMB1 UINT32_C(0x1F801DDA)
#define TEST_SPU_LCOMB2 UINT32_C(0x1F801DDC)
#define TEST_SPU_RCOMB2 UINT32_C(0x1F801DDE)
#define TEST_SPU_LCOMB4 UINT32_C(0x1F801DEC)
#define TEST_SPU_LSAME_DELAY UINT32_C(0x1F801DE0)
#define TEST_SPU_RSAME_DELAY UINT32_C(0x1F801DE2)
#define TEST_SPU_LDIFF UINT32_C(0x1F801DE4)
#define TEST_SPU_RDIFF UINT32_C(0x1F801DE6)
#define TEST_SPU_LDIFF_DELAY UINT32_C(0x1F801DF0)
#define TEST_SPU_RDIFF_DELAY UINT32_C(0x1F801DF2)
#define TEST_SPU_APF1_VOLUME UINT32_C(0x1F801DD0)
#define TEST_SPU_APF2_VOLUME UINT32_C(0x1F801DD2)
#define TEST_SPU_LAPF1 UINT32_C(0x1F801DF4)
#define TEST_SPU_RAPF1 UINT32_C(0x1F801DF6)
#define TEST_SPU_LAPF2 UINT32_C(0x1F801DF8)
#define TEST_SPU_RAPF2 UINT32_C(0x1F801DFA)
#define TEST_SPU_REVERB_BASE UINT32_C(0x1F801DC0)
#define TEST_SPU_VAPF1 UINT32_C(0x1F801DD0)
#define TEST_SPU_VAPF2 UINT32_C(0x1F801DD2)
#define TEST_SPU_VCOMB1 UINT32_C(0x1F801DC6)
#define TEST_SPU_VCOMB2 UINT32_C(0x1F801DC8)
#define TEST_SPU_VCOMB3 UINT32_C(0x1F801DCA)
#define TEST_SPU_VCOMB4 UINT32_C(0x1F801DCC)
#define TEST_SPU_MLCOMB3 UINT32_C(0x1F801DE8)
#define TEST_SPU_MLAPF1 UINT32_C(0x1F801DF4)
#define TEST_SPU_MLAPF2 UINT32_C(0x1F801DF8)
#define TEST_SPU_DAPF1 UINT32_C(0x1F801DC0)
#define TEST_SPU_DAPF2 UINT32_C(0x1F801DC2)
#define TEST_SPU_IRQA UINT32_C(0x1F801DA4)
#define TEST_SPU_STATX UINT32_C(0x1F801DAE)
#define TEST_SPU_MAIN_VOL_L UINT32_C(0x1F801D80)
#define TEST_SPU_MAIN_VOL_R UINT32_C(0x1F801D82)

/* Register vectors and buffer sizes from psx-spx SPU Reverb Examples, indexed
   by .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md section 1.7. Digests are
   local regression baselines, not captured hardware waveforms. */
struct reverb_preset {
    const char *name;
    uint32_t size;
    uint16_t registers[32];
    uint64_t digest;
};

static const struct reverb_preset reverb_presets[] = {
    {"Room", 0x26C0u,
     {0x007Du, 0x005Bu, 0x6D80u, 0x54B8u, 0xBED0u, 0x0000u, 0x0000u, 0xBA80u,
      0x5800u, 0x5300u, 0x04D6u, 0x0333u, 0x03F0u, 0x0227u, 0x0374u, 0x01EFu,
      0x0334u, 0x01B5u, 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u,
      0x0000u, 0x0000u, 0x01B4u, 0x0136u, 0x00B8u, 0x005Cu, 0x8000u, 0x8000u},
     UINT64_C(0x6F67AAC5E772F379)},
    {"Studio Small", 0x1F40u,
     {0x0033u, 0x0025u, 0x70F0u, 0x4FA8u, 0xBCE0u, 0x4410u, 0xC0F0u, 0x9C00u,
      0x5280u, 0x4EC0u, 0x03E4u, 0x031Bu, 0x03A4u, 0x02AFu, 0x0372u, 0x0266u,
      0x031Cu, 0x025Du, 0x025Cu, 0x018Eu, 0x022Fu, 0x0135u, 0x01D2u, 0x00B7u,
      0x018Fu, 0x00B5u, 0x00B4u, 0x0080u, 0x004Cu, 0x0026u, 0x8000u, 0x8000u},
     UINT64_C(0x5F76DE497CE9BA10)},
    {"Studio Medium", 0x4840u,
     {0x00B1u, 0x007Fu, 0x70F0u, 0x4FA8u, 0xBCE0u, 0x4510u, 0xBEF0u, 0xB4C0u,
      0x5280u, 0x4EC0u, 0x0904u, 0x076Bu, 0x0824u, 0x065Fu, 0x07A2u, 0x0616u,
      0x076Cu, 0x05EDu, 0x05ECu, 0x042Eu, 0x050Fu, 0x0305u, 0x0462u, 0x02B7u,
      0x042Fu, 0x0265u, 0x0264u, 0x01B2u, 0x0100u, 0x0080u, 0x8000u, 0x8000u},
     UINT64_C(0xD3AA48A27516CBCA)},
    {"Studio Large", 0x6FE0u,
     {0x00E3u, 0x00A9u, 0x6F60u, 0x4FA8u, 0xBCE0u, 0x4510u, 0xBEF0u, 0xA680u,
      0x5680u, 0x52C0u, 0x0DFBu, 0x0B58u, 0x0D09u, 0x0A3Cu, 0x0BD9u, 0x0973u,
      0x0B59u, 0x08DAu, 0x08D9u, 0x05E9u, 0x07ECu, 0x04B0u, 0x06EFu, 0x03D2u,
      0x05EAu, 0x031Du, 0x031Cu, 0x0238u, 0x0154u, 0x00AAu, 0x8000u, 0x8000u},
     UINT64_C(0xD560055723FAEA2B)},
    {"Hall", 0xADE0u,
     {0x01A5u, 0x0139u, 0x6000u, 0x5000u, 0x4C00u, 0xB800u, 0xBC00u, 0xC000u,
      0x6000u, 0x5C00u, 0x15BAu, 0x11BBu, 0x14C2u, 0x10BDu, 0x11BCu, 0x0DC1u,
      0x11C0u, 0x0DC3u, 0x0DC0u, 0x09C1u, 0x0BC4u, 0x07C1u, 0x0A00u, 0x06CDu,
      0x09C2u, 0x05C1u, 0x05C0u, 0x041Au, 0x0274u, 0x013Au, 0x8000u, 0x8000u},
     UINT64_C(0xE74E03196E29C8FC)},
    {"Half Echo", 0x3C00u,
     {0x0017u, 0x0013u, 0x70F0u, 0x4FA8u, 0xBCE0u, 0x4510u, 0xBEF0u, 0x8500u,
      0x5F80u, 0x54C0u, 0x0371u, 0x02AFu, 0x02E5u, 0x01DFu, 0x02B0u, 0x01D7u,
      0x0358u, 0x026Au, 0x01D6u, 0x011Eu, 0x012Du, 0x00B1u, 0x011Fu, 0x0059u,
      0x01A0u, 0x00E3u, 0x0058u, 0x0040u, 0x0028u, 0x0014u, 0x8000u, 0x8000u},
     UINT64_C(0x6396FE43FD7A7F53)},
    {"Space Echo", 0xF6C0u,
     {0x033Du, 0x0231u, 0x7E00u, 0x5000u, 0xB400u, 0xB000u, 0x4C00u, 0xB000u,
      0x6000u, 0x5400u, 0x1ED6u, 0x1A31u, 0x1D14u, 0x183Bu, 0x1BC2u, 0x16B2u,
      0x1A32u, 0x15EFu, 0x15EEu, 0x1055u, 0x1334u, 0x0F2Du, 0x11F6u, 0x0C5Du,
      0x1056u, 0x0AE1u, 0x0AE0u, 0x07A2u, 0x0464u, 0x0232u, 0x8000u, 0x8000u},
     UINT64_C(0xD46A63C555D20A07)},
    {"Chaos Echo", 0x18040u,
     {0x0001u, 0x0001u, 0x7FFFu, 0x7FFFu, 0x0000u, 0x0000u, 0x0000u, 0x8100u,
      0x0000u, 0x0000u, 0x1FFFu, 0x0FFFu, 0x1005u, 0x0005u, 0x0000u, 0x0000u,
      0x1005u, 0x0005u, 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u,
      0x0000u, 0x0000u, 0x1004u, 0x1002u, 0x0004u, 0x0002u, 0x8000u, 0x8000u},
     UINT64_C(0x5F8029B5D6F4911F)},
    {"Delay", 0x18040u,
     {0x0001u, 0x0001u, 0x7FFFu, 0x7FFFu, 0x0000u, 0x0000u, 0x0000u, 0x0000u,
      0x0000u, 0x0000u, 0x1FFFu, 0x0FFFu, 0x1005u, 0x0005u, 0x0000u, 0x0000u,
      0x1005u, 0x0005u, 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u,
      0x0000u, 0x0000u, 0x1004u, 0x1002u, 0x0004u, 0x0002u, 0x8000u, 0x8000u},
     UINT64_C(0x42CC1BD9124DA5B5)},
    {"Reverb off", 0x0010u,
     {0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u,
      0x0000u, 0x0000u, 0x0001u, 0x0001u, 0x0001u, 0x0001u, 0x0001u, 0x0001u,
      0x0000u, 0x0000u, 0x0001u, 0x0001u, 0x0001u, 0x0001u, 0x0001u, 0x0001u,
      0x0000u, 0x0000u, 0x0001u, 0x0001u, 0x0001u, 0x0001u, 0x0000u, 0x0000u},
     UINT64_C(0x5966B6D8204FC425)},
};

static void *test_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void test_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static schway *make_reverb_instance(void)
{
    schway_config config;
    schway_profile profile;
    schway *inst = NULL;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = test_alloc;
    config.allocator.free = test_free;
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

static uint32_t test_reg_index(uint32_t address)
{
    return (address - TEST_SPU_REGISTER_BASE) >> 1;
}

static void setup_reverb_impulse(schway *inst, uint16_t evol_left, uint16_t evol_right,
                                 uint32_t wet_input)
{
    inst->spu.reg[test_reg_index(TEST_SPU_ATTR)] = 0xC080u;
    inst->spu.reg[test_reg_index(TEST_SPU_NON)] = 1u;
    inst->spu.reg[test_reg_index(TEST_SPU_EON0)] = wet_input != 0u ? 1u : 0u;
    inst->spu.reg[test_reg_index(TEST_SPU_EVOL_L)] = evol_left;
    inst->spu.reg[test_reg_index(TEST_SPU_EVOL_R)] = evol_right;
    inst->spu.reg[test_reg_index(TEST_SPU_IIR)] = 0x7FFFu;
    inst->spu.reg[test_reg_index(TEST_SPU_COMB1)] = 0x7FFFu;
    inst->spu.reg[test_reg_index(TEST_SPU_WALL)] = 0u;
    inst->spu.reg[test_reg_index(TEST_SPU_LSAME)] = 2u;
    inst->spu.reg[test_reg_index(TEST_SPU_RSAME)] = 2u;
    inst->spu.reg[test_reg_index(TEST_SPU_LCOMB1)] = 2u;
    inst->spu.reg[test_reg_index(TEST_SPU_RCOMB1)] = 2u;
    inst->spu.reg[test_reg_index(TEST_SPU_LSAME_DELAY)] = 0u;
    inst->spu.reg[test_reg_index(TEST_SPU_RSAME_DELAY)] = 0u;
    inst->spu.reg[test_reg_index(TEST_SPU_LDIFF)] = 2u;
    inst->spu.reg[test_reg_index(TEST_SPU_RDIFF)] = 2u;
    inst->spu.reg[test_reg_index(TEST_SPU_LDIFF_DELAY)] = 0u;
    inst->spu.reg[test_reg_index(TEST_SPU_RDIFF_DELAY)] = 0u;
    inst->spu.reg[test_reg_index(TEST_SPU_APF1_VOLUME)] = 0u;
    inst->spu.reg[test_reg_index(TEST_SPU_APF2_VOLUME)] = 0u;
    inst->spu.reg[test_reg_index(TEST_SPU_LAPF1)] = 2u;
    inst->spu.reg[test_reg_index(TEST_SPU_RAPF1)] = 2u;
    inst->spu.reg[test_reg_index(TEST_SPU_LAPF2)] = 2u;
    inst->spu.reg[test_reg_index(TEST_SPU_RAPF2)] = 2u;
    inst->spu.reg[test_reg_index(TEST_SPU_VLIN)] = 0x7FFFu;
    inst->spu.reg[test_reg_index(TEST_SPU_VRIN)] = 0x7FFFu;
    inst->spu.reg[test_reg_index(TEST_SPU_MAIN_VOL_L)] = 0x3FFFu;
    inst->spu.reg[test_reg_index(TEST_SPU_MAIN_VOL_R)] = 0x3FFFu;
    inst->spu.noise_lfsr = 0x2000u;
    inst->spu.noise_phase = 0u;
    inst->spu.voice[0].envelope = 0x4000;
    inst->spu.voice[0].reg[0] = 0x3FFFu;
    inst->spu.voice[0].reg[1] = 0x1FFFu;
}

static void run_cycles(schway *inst, uint64_t cycles)
{
    schway_budget budget;
    schway_run_result result;
    memset(&budget, 0, sizeof budget);
    budget.size = (uint32_t)sizeof budget;
    budget.max_cycles = cycles;
    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    CHECK(schway_run(inst, &budget, &result) == SCHWAY_STATUS_OK,
          "public run advances the scheduled reverb path");
    CHECK(result.stop == SCHWAY_STOP_BUDGET, "public run stops at its cycle budget");
}

static uint32_t capture_impulse_pcm(int16_t samples[96][2], uint16_t evol_left,
                                    uint16_t evol_right, uint32_t wet_input, uint32_t split)
{
    schway *inst = make_reverb_instance();
    if (inst == NULL) {
        CHECK(0, "reverb PCM instance is created");
        return 0u;
    }
    setup_reverb_impulse(inst, evol_left, evol_right, wet_input);
    run_cycles(inst, SCHWAY__CYCLES_PER_PAIR);
    inst->spu.reg[test_reg_index(TEST_SPU_EON0)] = 0u;
    if (split != 0u) {
        run_cycles(inst, UINT64_C(22) * SCHWAY__CYCLES_PER_PAIR);
        run_cycles(inst, UINT64_C(28) * SCHWAY__CYCLES_PER_PAIR);
        run_cycles(inst, UINT64_C(45) * SCHWAY__CYCLES_PER_PAIR);
    } else {
        run_cycles(inst, UINT64_C(95) * SCHWAY__CYCLES_PER_PAIR);
    }

    schway_audio audio;
    memset(&audio, 0, sizeof audio);
    audio.size = (uint32_t)sizeof audio;
    audio.samples = &samples[0][0];
    audio.capacity = 96u;
    CHECK(schway_get_audio(inst, &audio) == SCHWAY_STATUS_OK,
          "scheduled reverb output drains through public PCM");
    schway_destroy(inst);
    return audio.written;
}

static void check_public_wet_return(void)
{
    int16_t dry[96][2] = {{0}};
    int16_t wet_positive[96][2] = {{0}};
    int16_t wet_negative[96][2] = {{0}};
    int16_t wet_split[96][2] = {{0}};
    int16_t off[96][2] = {{0}};
    CHECK(capture_impulse_pcm(dry, 0x7FFFu, 0x7FFFu, 0u, 0u) == 96u,
          "dry baseline produces 96 native PCM pairs");
    CHECK(capture_impulse_pcm(wet_positive, 0x7FFFu, 0x7FFFu, 1u, 0u) == 96u,
          "positive wet impulse produces 96 native PCM pairs");
    CHECK(capture_impulse_pcm(wet_negative, 0x8001u, 0x8001u, 1u, 0u) == 96u,
          "negative wet impulse produces 96 native PCM pairs");
    CHECK(capture_impulse_pcm(wet_split, 0x7FFFu, 0x7FFFu, 1u, 1u) == 96u,
          "split event dispatch produces 96 native PCM pairs");
    CHECK(capture_impulse_pcm(off, 0u, 0u, 1u, 0u) == 96u,
          "Off configuration produces 96 native PCM pairs");

    uint32_t saw_wet = 0u;
    uint32_t saw_asymmetric = 0u;
    uint32_t saw_opposite_sign = 0u;
    for (uint32_t tick = 0u; tick < 96u; tick++) {
        int32_t positive_left = (int32_t)wet_positive[tick][0] - dry[tick][0];
        int32_t positive_right = (int32_t)wet_positive[tick][1] - dry[tick][1];
        int32_t negative_left = (int32_t)wet_negative[tick][0] - dry[tick][0];
        saw_wet |= positive_left != 0 || positive_right != 0;
        saw_asymmetric |= positive_left != positive_right;
        saw_opposite_sign |= (int64_t)positive_left * negative_left < 0;
    }
    CHECK(saw_wet != 0u, "EON wet input reaches the public audio mix");
    CHECK(saw_asymmetric != 0u, "left and right wet impulses remain distinct");
    CHECK(saw_opposite_sign != 0u, "signed EVOL changes the wet return sign");
    CHECK(memcmp(wet_positive, wet_split, sizeof wet_positive) == 0,
          "reverb channel phase is stable across split public run calls");
    CHECK(memcmp(dry, off, sizeof dry) == 0,
          "zero EVOL Off configuration contributes no wet PCM");
}

static void check_feedback_ram_path(void)
{
    schway *inst = make_reverb_instance();
    if (inst == NULL) {
        CHECK(0, "reverb feedback instance is created");
        return;
    }

    const uint32_t base = 0x0007FFF8u;
    inst->spu.reg[test_reg_index(TEST_SPU_ATTR)] = 0xC080u;
    inst->spu.reg[test_reg_index(TEST_SPU_ESA)] = (uint16_t)(base >> 3);
    inst->spu.reg[test_reg_index(TEST_SPU_IIR)] = 0x7FFFu;
    inst->spu.reg[test_reg_index(TEST_SPU_COMB1)] = 0x7FFFu;
    inst->spu.reg[test_reg_index(TEST_SPU_WALL)] = 0x7FFFu;
    inst->spu.reg[test_reg_index(TEST_SPU_LSAME)] = 2u;
    inst->spu.reg[test_reg_index(TEST_SPU_LCOMB1)] = 2u;
    inst->spu.reg[test_reg_index(TEST_SPU_LSAME_DELAY)] = 0u;
    inst->spu.reg[test_reg_index(TEST_SPU_VLIN)] = 0x7FFFu;
    inst->spu.reg[test_reg_index(TEST_SPU_VRIN)] = 0x7FFFu;
    inst->spu.reverb_address = base;
    inst->spu.irqa_addr = base;
    inst->spu.irq_enabled = 1u;

    for (uint32_t tick = 0u; tick < 128u; tick++) {
        int16_t return_left = 0;
        int16_t return_right = 0;
        inst->spu.next_tick = ((uint64_t)tick + 1u) * SCHWAY__CYCLES_PER_PAIR;
        schway__spu_reverb_step(inst, tick == 0u ? 16000 : 0, 0,
                                &return_left, &return_right);
        CHECK(inst->spu.reverb_address >= base &&
                  inst->spu.reverb_address <= SCHWAY__SPU_RAM_SIZE - 2u,
              "reverb cursor wraps within the ESA work area");
        for (uint32_t channel = 0u; channel < 2u; channel++) {
            for (uint32_t index = 0u; index < SCHWAY__SPU_REVERB_WORK_WORDS; index++) {
                int32_t stored = inst->spu.reverb_work[channel][index];
                CHECK(stored >= INT16_MIN && stored <= INT16_MAX,
                      "reverb work values stay within measured 16-bit saturation bounds");
            }
        }
    }

    uint32_t wrote_feedback = 0u;
    for (uint32_t address = base; address < SCHWAY__SPU_RAM_SIZE; address += 2u) {
        wrote_feedback |= schway__spu_ram_peek(inst, address) != 0u;
    }
    CHECK(wrote_feedback != 0u,
          "wet impulse drives the register-selected feedback path into SPU RAM");
    CHECK((schway__spu_reg_peek(inst, UINT32_C(0x1F801DAE)) & 0x0040u) != 0u,
          "reverb work-buffer accesses check IRQA");
    schway_destroy(inst);
}

static void check_negative_iir_coefficient(void)
{
    schway *inst = make_reverb_instance();
    if (inst == NULL) {
        CHECK(0, "negative-IIR instance is created");
        return;
    }

    const uint32_t base = 0x00002000u;
    inst->spu.reg[test_reg_index(TEST_SPU_ATTR)] = 0xC080u;
    inst->spu.reg[test_reg_index(TEST_SPU_ESA)] = (uint16_t)(base >> 3);
    inst->spu.reg[test_reg_index(TEST_SPU_IIR)] = 0x8000u;
    inst->spu.reg[test_reg_index(TEST_SPU_LSAME)] = 2u;
    inst->spu.reg[test_reg_index(TEST_SPU_LDIFF)] = 4u;
    inst->spu.reg[test_reg_index(TEST_SPU_VLIN)] = 0x7FFFu;
    inst->spu.reg[test_reg_index(TEST_SPU_VRIN)] = 0x7FFFu;
    inst->spu.reverb_address = base;
    inst->spu.next_tick = SCHWAY__CYCLES_PER_PAIR;
    inst->spu.reverb_fir[0][20] = 2000;
    schway__spu_ram_write(inst, base, 200);

    int16_t return_left = 0;
    int16_t return_right = 0;
    schway__spu_reverb_step(inst, 0, 0, &return_left, &return_right);
    uint16_t stored = schway__spu_ram_peek(inst, base + 16u);
    CHECK(stored == 600u,
          "vIIR=-8000h negates the completed IIR result after adding the prior sample");
    schway_destroy(inst);
}

static void check_measured_negative_one_input(void)
{
    schway *inst = make_reverb_instance();
    if (inst == NULL) {
        CHECK(0, "negative-one input instance is created");
        return;
    }
    inst->spu.reg[test_reg_index(TEST_SPU_VLIN)] = 0x8000u;
    inst->spu.reg[test_reg_index(TEST_SPU_VRIN)] = 0x8000u;
    inst->spu.next_tick = SCHWAY__CYCLES_PER_PAIR;
    int16_t return_left = 0;
    int16_t return_right = 0;
    schway__spu_reverb_step(inst, 1000, -2000, &return_left, &return_right);
    CHECK(inst->spu.reverb_fir[0][0] == -1000 &&
              inst->spu.reverb_fir[1][0] == 2000,
          "vLIN and vRIN=-8000h use ordinary signed -1.0 scaling");
    schway_destroy(inst);
}

static void check_saturation_before_final_output(void)
{
    schway *inst = make_reverb_instance();
    if (inst == NULL) {
        CHECK(0, "peak-input reverb instance is created");
        return;
    }

    const uint32_t base = 0x00004000u;
    inst->spu.reg[test_reg_index(TEST_SPU_ATTR)] = 0xC080u;
    inst->spu.reg[test_reg_index(TEST_SPU_ESA)] = (uint16_t)(base >> 3);
    inst->spu.reg[test_reg_index(TEST_SPU_VCOMB1)] = 0x7FFFu;
    inst->spu.reg[test_reg_index(TEST_SPU_VCOMB2)] = 0x7FFFu;
    inst->spu.reg[test_reg_index(TEST_SPU_VCOMB3)] = 0x7FFFu;
    inst->spu.reg[test_reg_index(TEST_SPU_LCOMB1)] = 1u;
    inst->spu.reg[test_reg_index(TEST_SPU_LCOMB2)] = 2u;
    inst->spu.reg[test_reg_index(TEST_SPU_MLCOMB3)] = 3u;
    inst->spu.reg[test_reg_index(TEST_SPU_LCOMB4)] = 4u;
    inst->spu.reg[test_reg_index(TEST_SPU_VAPF1)] = 0x7FFFu;
    inst->spu.reg[test_reg_index(TEST_SPU_VAPF2)] = 0x7FFFu;
    inst->spu.reg[test_reg_index(TEST_SPU_MLAPF1)] = 0x0080u;
    inst->spu.reg[test_reg_index(TEST_SPU_MLAPF2)] = 0x0090u;
    inst->spu.reg[test_reg_index(TEST_SPU_DAPF1)] = 0x0080u;
    inst->spu.reg[test_reg_index(TEST_SPU_DAPF2)] = 0x0090u;
    inst->spu.reverb_address = base;
    schway__spu_ram_write(inst, base + 8u, 30000u);
    schway__spu_ram_write(inst, base + 16u, 30000u);
    schway__spu_ram_write(inst, base + 24u, (uint16_t)(UINT32_C(0x10000) - 30000u));

    int16_t return_left = 0;
    int16_t return_right = 0;
    inst->spu.next_tick = SCHWAY__CYCLES_PER_PAIR;
    schway__spu_reverb_step(inst, 0, 0, &return_left, &return_right);
    int32_t stored = inst->spu.reverb_work[0][0];
    CHECK(stored > 2000 && stored < 4000,
          "comb saturation occurs before a later cancelling tap, not only at final output");
    schway_destroy(inst);
}

static void check_write_disabled_output_and_irq(void)
{
    schway *inst = make_reverb_instance();
    if (inst == NULL) {
        CHECK(0, "write-disabled reverb instance is created");
        return;
    }

    const uint32_t base = 0x00005000u;
    uint8_t before[64];
    inst->spu.reg[test_reg_index(TEST_SPU_ATTR)] = 0xC000u;
    inst->spu.reg[test_reg_index(TEST_SPU_ESA)] = (uint16_t)(base >> 3);
    inst->spu.reg[test_reg_index(TEST_SPU_LAPF2)] = 2u;
    inst->spu.reg[test_reg_index(TEST_SPU_DAPF2)] = 0u;
    inst->spu.reg[test_reg_index(TEST_SPU_LSAME)] = 4u;
    inst->spu.reverb_address = base;
    schway__spu_ram_write(inst, base + 16u, 1234u);
    schway__spu_ram_write(inst, base + 32u, (uint16_t)(int16_t)-4321);
    memcpy(before, inst->spu.ram + base, sizeof before);
    inst->spu.irqa_addr = base + 32u;
    inst->spu.irq_enabled = 1u;
    inst->spu.next_tick = SCHWAY__CYCLES_PER_PAIR;

    int16_t return_left = 0;
    int16_t return_right = 0;
    schway__spu_reverb_step(inst, 0, 0, &return_left, &return_right);
    CHECK(inst->spu.reverb_work[0][0] == 1234,
          "ATTR write-disable preserves the APF output read");
    CHECK(memcmp(before, inst->spu.ram + base, sizeof before) == 0,
          "ATTR write-disable substitutes reads without changing reverb RAM");
    CHECK((schway__spu_reg_peek(inst, TEST_SPU_STATX) & 0x0040u) != 0u,
          "an inhibited IIR state write becomes an IRQA-checked read");
    schway_destroy(inst);
}

static void check_esa_write_resets_cursor(void)
{
    schway *inst = make_reverb_instance();
    if (inst == NULL) {
        CHECK(0, "ESA cursor instance is created");
        return;
    }

    const uint32_t base = 0x00006000u;
    inst->spu.reverb_address = 0x00002000u;
    inst->spu.next_tick = SCHWAY__CYCLES_PER_PAIR;
    schway__spu_reg_write(inst, TEST_SPU_ESA, (uint16_t)(base >> 3));
    run_cycles(inst, SCHWAY__CYCLES_PER_PAIR);
    CHECK(inst->spu.reverb_address == base,
          "writing ESA resets the current reverb work cursor");
    schway_destroy(inst);
}

static void check_feedback_state_continuation(void)
{
    schway *source = make_reverb_instance();
    schway *loaded = make_reverb_instance();
    if (source == NULL || loaded == NULL) {
        CHECK(0, "active-feedback state instances are created");
        if (source != NULL) {
            schway_destroy(source);
        }
        if (loaded != NULL) {
            schway_destroy(loaded);
        }
        return;
    }

    setup_reverb_impulse(source, 0x7FFFu, 0x7FFFu, 1u);
    run_cycles(source, UINT64_C(160) * SCHWAY__CYCLES_PER_PAIR);
    uint32_t active_fir = 0u;
    uint32_t active_work = 0u;
    for (uint32_t channel = 0u; channel < 2u; channel++) {
        for (uint32_t index = 0u; index < SCHWAY__SPU_REVERB_FIR_TAPS; index++) {
            active_fir |= source->spu.reverb_fir[channel][index] != 0;
        }
        for (uint32_t index = 0u; index < SCHWAY__SPU_REVERB_WORK_WORDS; index++) {
            active_work |= source->spu.reverb_work[channel][index] != 0;
        }
    }
    CHECK(active_fir != 0u && active_work != 0u,
          "save point contains populated input and output reverb histories");

    size_t size = schway_state_size(source);
    uint8_t *state = (uint8_t *)malloc(size);
    uint8_t *source_after = (uint8_t *)malloc(size);
    uint8_t *loaded_after = (uint8_t *)malloc(size);
    int16_t source_pcm[96][2] = {{0}};
    int16_t loaded_pcm[96][2] = {{0}};
    if (state == NULL || source_after == NULL || loaded_after == NULL || size == 0u) {
        CHECK(0, "active-feedback state buffers are allocated");
        free(state);
        free(source_after);
        free(loaded_after);
        schway_destroy(source);
        schway_destroy(loaded);
        return;
    }
    CHECK(schway_save_state(source, state, size) == SCHWAY_STATUS_OK,
          "active feedback state saves");
    CHECK(schway_load_state(loaded, state, size) == SCHWAY_STATUS_OK,
          "active feedback state loads with complete histories");

    schway_audio source_audio;
    schway_audio loaded_audio;
    memset(&source_audio, 0, sizeof source_audio);
    memset(&loaded_audio, 0, sizeof loaded_audio);
    source_audio.size = (uint32_t)sizeof source_audio;
    source_audio.samples = &source_pcm[0][0];
    source_audio.capacity = 96u;
    loaded_audio.size = (uint32_t)sizeof loaded_audio;
    loaded_audio.samples = &loaded_pcm[0][0];
    loaded_audio.capacity = 96u;
    CHECK(schway_get_audio(source, &source_audio) == SCHWAY_STATUS_OK,
          "saved source queue drains before continuation comparison");
    CHECK(schway_get_audio(loaded, &loaded_audio) == SCHWAY_STATUS_OK,
          "loaded source queue drains before continuation comparison");

    run_cycles(source, UINT64_C(96) * SCHWAY__CYCLES_PER_PAIR);
    run_cycles(loaded, UINT64_C(96) * SCHWAY__CYCLES_PER_PAIR);
    memset(&source_audio, 0, sizeof source_audio);
    memset(&loaded_audio, 0, sizeof loaded_audio);
    source_audio.size = (uint32_t)sizeof source_audio;
    source_audio.samples = &source_pcm[0][0];
    source_audio.capacity = 96u;
    loaded_audio.size = (uint32_t)sizeof loaded_audio;
    loaded_audio.samples = &loaded_pcm[0][0];
    loaded_audio.capacity = 96u;
    CHECK(schway_get_audio(source, &source_audio) == SCHWAY_STATUS_OK &&
              schway_get_audio(loaded, &loaded_audio) == SCHWAY_STATUS_OK,
          "continued public PCM drains from both states");
    CHECK(source_audio.written == 96u && loaded_audio.written == 96u,
          "continued state streams contain 96 PCM pairs");
    CHECK(memcmp(source_pcm, loaded_pcm, sizeof source_pcm) == 0,
          "active reverb PCM continues identically after save and load");
    CHECK(schway_save_state(source, source_after, size) == SCHWAY_STATUS_OK &&
              schway_save_state(loaded, loaded_after, size) == SCHWAY_STATUS_OK,
          "both continued feedback states save");
    CHECK(memcmp(source_after, loaded_after, size) == 0,
          "full feedback memory and FIR histories resume byte-for-byte");

    free(state);
    free(source_after);
    free(loaded_after);
    schway_destroy(source);
    schway_destroy(loaded);
}

static uint64_t capture_preset_digest(const struct reverb_preset *preset,
                                      uint32_t *saw_output)
{
    schway *inst = make_reverb_instance();
    if (inst == NULL) {
        CHECK(0, "preset impulse instance is created");
        return 0u;
    }

    uint32_t base = SCHWAY__SPU_RAM_SIZE - preset->size;
    inst->spu.reg[test_reg_index(TEST_SPU_ATTR)] = 0xC080u;
    inst->spu.reg[test_reg_index(TEST_SPU_ESA)] = (uint16_t)(base >> 3);
    inst->spu.reg[test_reg_index(TEST_SPU_EVOL_L)] = 0x7FFFu;
    inst->spu.reg[test_reg_index(TEST_SPU_EVOL_R)] = 0x7FFFu;
    for (uint32_t i = 0u; i < 32u; i++) {
        uint32_t address = TEST_SPU_REVERB_BASE + i * 2u;
        inst->spu.reg[test_reg_index(address)] = preset->registers[i];
    }
    inst->spu.reverb_address = base;

    uint64_t digest = UINT64_C(14695981039346656037);
    *saw_output = 0u;
    for (uint32_t tick = 0u; tick < 8400u; tick++) {
        int16_t left = 0;
        int16_t right = 0;
        inst->spu.next_tick = ((uint64_t)tick + 1u) * SCHWAY__CYCLES_PER_PAIR;
        schway__spu_reverb_step(inst, tick == 0u ? 30000 : 0,
                                tick == 0u ? -28000 : 0, &left, &right);
        *saw_output |= left != 0 || right != 0;
        const uint16_t pair[2] = {(uint16_t)left, (uint16_t)right};
        for (uint32_t channel = 0u; channel < 2u; channel++) {
            digest = (digest ^ (uint8_t)pair[channel]) * UINT64_C(1099511628211);
            digest = (digest ^ (uint8_t)(pair[channel] >> 8)) * UINT64_C(1099511628211);
        }
    }
    schway_destroy(inst);
    return digest;
}

static void check_preset_impulse_digests(void)
{
    const uint32_t count = (uint32_t)(sizeof reverb_presets / sizeof reverb_presets[0]);
    CHECK(count == 10u, "psx-spx preset set has nine active modes and Reverb off");
    for (uint32_t i = 0u; i < count; i++) {
        uint32_t saw_output = 0u;
        uint64_t digest = capture_preset_digest(&reverb_presets[i], &saw_output);
        CHECK(digest == reverb_presets[i].digest,
              "preset impulse remains stable as a local regression baseline");
        if (strcmp(reverb_presets[i].name, "Reverb off") == 0) {
            CHECK(saw_output == 0u, "Off preset contributes no reverb return");
        } else {
            CHECK(saw_output != 0u, "each active preset produces a deterministic impulse return");
        }
    }
}

int main(void)
{
    compare_input_fir();
    compare_output_polyphases();
    check_centered_delay(0u);
    check_centered_delay(1u);
    check_split_resume();
    check_public_wet_return();
    check_feedback_ram_path();
    check_negative_iir_coefficient();
    check_measured_negative_one_input();
    check_saturation_before_final_output();
    check_write_disabled_output_and_irq();
    check_esa_write_resets_cursor();
    check_feedback_state_continuation();
    check_preset_impulse_digests();
    return failures == 0u ? 0 : 1;
}
