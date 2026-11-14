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
#define TEST_SPU_MAIN_VOL_L UINT32_C(0x1F801D80)
#define TEST_SPU_MAIN_VOL_R UINT32_C(0x1F801D82)

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
    inst->spu.reg[test_reg_index(TEST_SPU_ATTR)] = 0xC000u;
    inst->spu.reg[test_reg_index(TEST_SPU_NON)] = 1u;
    inst->spu.reg[test_reg_index(TEST_SPU_EON0)] = wet_input != 0u ? 1u : 0u;
    inst->spu.reg[test_reg_index(TEST_SPU_EVOL_L)] = evol_left;
    inst->spu.reg[test_reg_index(TEST_SPU_EVOL_R)] = evol_right;
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

static void run_event_until(schway *inst, uint64_t cycle)
{
    inst->ticks = cycle;
    schway__spu_event(inst, cycle);
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
    run_event_until(inst, SCHWAY__CYCLES_PER_PAIR);
    inst->spu.reg[test_reg_index(TEST_SPU_EON0)] = 0u;
    if (split != 0u) {
        run_event_until(inst, UINT64_C(23) * SCHWAY__CYCLES_PER_PAIR);
        run_event_until(inst, UINT64_C(51) * SCHWAY__CYCLES_PER_PAIR);
    }
    run_event_until(inst, UINT64_C(96) * SCHWAY__CYCLES_PER_PAIR);

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
        saw_opposite_sign |= positive_left * negative_left < 0;
    }
    CHECK(saw_wet != 0u, "EON wet input reaches the public audio mix");
    CHECK(saw_asymmetric != 0u, "left and right wet impulses remain distinct");
    CHECK(saw_opposite_sign != 0u, "signed EVOL changes the wet return sign");
    CHECK(memcmp(wet_positive, wet_split, sizeof wet_positive) == 0,
          "reverb channel phase is stable across split scheduled runs");
    CHECK(memcmp(dry, off, sizeof dry) == 0,
          "zero EVOL Off configuration contributes no wet PCM");
}

int main(void)
{
    compare_input_fir();
    compare_output_polyphases();
    check_centered_delay(0u);
    check_centered_delay(1u);
    check_split_resume();
    check_public_wet_return();
    return failures == 0u ? 0 : 1;
}
