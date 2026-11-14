/* Reverb resampler checks against an independent scalar convolution. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../../src/internal.h"

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
    uint32_t work_cursor = SCHWAY__SPU_REVERB_WORK_WORDS - 1u;
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
            work_cursor = (work_cursor + 1u) % SCHWAY__SPU_REVERB_WORK_WORDS;
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
    }

    CHECK(total != 0, "unity wet path carries a measurable impulse");
    CHECK(moment == INT64_C(38) * total, "centered input and output filters align at 38 native samples");
}

int main(void)
{
    compare_input_fir();
    compare_output_polyphases();
    check_centered_delay(0u);
    check_centered_delay(1u);
    return failures == 0u ? 0 : 1;
}
