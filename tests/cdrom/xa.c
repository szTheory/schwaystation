/* XA-ADPCM sector decode tests through the internal CD decoder seam. */
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "cdxa.h"
#include "internal.h"
#include "../check.h"

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

static schway *make(void)
{
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) {
        exit(1);
    }
    return inst;
}

static void make_sector(uint8_t sector[SCHWAY__CDXA_RAW_SECTOR_BYTES], uint8_t coding,
                        uint8_t sample)
{
    memset(sector, 0, SCHWAY__CDXA_RAW_SECTOR_BYTES);
    memset(&sector[1], 0xFF, 10u);
    sector[11] = 0u;
    sector[15] = 2u;
    sector[16] = 1u;
    sector[17] = 2u;
    sector[18] = 0x64u;
    sector[19] = coding;
    (void)memcpy(&sector[20], &sector[16], 4u);
    for (uint32_t portion = 0u; portion < 18u; portion++) {
        uint8_t *chunk = &sector[24u + portion * 128u];
        for (uint32_t header = 0u; header < 8u; header++) {
            chunk[4u + header] = 0u;
        }
        (void)memcpy(chunk, &chunk[4], 4u);
        (void)memcpy(&chunk[12], &chunk[8], 4u);
        memset(&chunk[16], sample, 112u);
    }
}

static void clear_samples(uint8_t sector[SCHWAY__CDXA_RAW_SECTOR_BYTES])
{
    for (uint32_t portion = 0u; portion < 18u; portion++) {
        memset(&sector[24u + portion * 128u + 16u], 0, 112u);
    }
}

static void set_selectors(uint8_t sector[SCHWAY__CDXA_RAW_SECTOR_BYTES], uint8_t selector)
{
    for (uint32_t portion = 0u; portion < 18u; portion++) {
        uint8_t *chunk = &sector[24u + portion * 128u];
        memset(&chunk[4], selector, 8u);
        (void)memcpy(chunk, &chunk[4], 4u);
        (void)memcpy(&chunk[12], &chunk[8], 4u);
    }
}

static void decode_sector(schway *inst, uint8_t sector[SCHWAY__CDXA_RAW_SECTOR_BYTES],
                          int16_t *decoded, uint32_t *frames, uint32_t *low_rate)
{
    CHECK_EQ_U64(schway__cdxa_decode_adpcm(inst, sector, SCHWAY__CDXA_RAW_SECTOR_BYTES,
                                            decoded, SCHWAY__CDXA_MAX_DECODED_FRAMES,
                                            frames, low_rate), 1u);
}

static void decode_form2_audio(void)
{
    schway *inst = make();
    uint8_t sector[SCHWAY__CDXA_RAW_SECTOR_BYTES];
    int16_t output[SCHWAY__CDXA_MAX_OUTPUT_FRAMES * 2u];
    uint32_t output_frames = 0u;
    make_sector(sector, 1u, 0x11u);
    uint32_t decoded = schway__cdxa_decode(inst, sector, sizeof sector, output,
                                             SCHWAY__CDXA_MAX_OUTPUT_FRAMES, &output_frames);
    CHECK_EQ_U64(decoded, 1u);
    CHECK_EQ_U64(output_frames, 2352u);
    CHECK_EQ_U64(inst->cdrom.xa_history[0][0], 4096u);
    CHECK_EQ_U64(inst->cdrom.xa_history[1][0], 4096u);
    schway_destroy(inst);
}

static void filter_response_and_shift_controls(void)
{
    static const int16_t filter_tail[4] = {0, 7440, 17257, 12356};
    uint8_t sector[SCHWAY__CDXA_RAW_SECTOR_BYTES];
    int16_t decoded[SCHWAY__CDXA_MAX_DECODED_FRAMES * 2u];
    for (uint32_t filter = 0u; filter < 4u; filter++) {
        schway *inst = make();
        uint32_t frames = 0u, low_rate = 1u;
        make_sector(sector, 1u, 0u);
        clear_samples(sector);
        set_selectors(sector, (uint8_t)(filter << 4));
        uint8_t *last = &sector[24u + 17u * 128u + 16u];
        last[3u + 25u * 4u] = 0x01u;
        last[3u + 26u * 4u] = 0x01u;
        decode_sector(inst, sector, decoded, &frames, &low_rate);
        CHECK_EQ_U64(frames, 2016u);
        CHECK_EQ_U64(low_rate, 0u);
        CHECK_EQ_U64((uint16_t)inst->cdrom.xa_adpcm_history[0][0],
                     (uint16_t)filter_tail[filter]);
        schway_destroy(inst);
    }

    for (uint32_t shift = 0u; shift < 16u; shift++) {
        schway *inst = make();
        uint32_t frames = 0u, low_rate = 0u;
        make_sector(sector, 1u, 0u);
        clear_samples(sector);
        set_selectors(sector, (uint8_t)shift);
        sector[24u + 17u * 128u + 16u + 3u + 27u * 4u] = 1u;
        decode_sector(inst, sector, decoded, &frames, &low_rate);
        uint32_t effective = shift >= 13u ? 9u : shift;
        CHECK_EQ_U64((uint16_t)inst->cdrom.xa_adpcm_history[0][0],
                     (uint16_t)(4096u / (1u << effective)));
        schway_destroy(inst);
    }

    schway *negative = make();
    uint32_t negative_frames = 0u, negative_rate = 0u;
    make_sector(sector, 1u, 0u);
    clear_samples(sector);
    set_selectors(sector, 1u);
    sector[24u + 17u * 128u + 16u + 3u + 27u * 4u] = 0x0Fu;
    decode_sector(negative, sector, decoded, &negative_frames, &negative_rate);
    CHECK(decoded[2u * (negative_frames - 1u)] == -2048);
    schway_destroy(negative);

    for (uint32_t shift = 0u; shift < 16u; shift++) {
        schway *inst = make();
        uint32_t frames = 0u, low_rate = 0u;
        make_sector(sector, 17u, 0u);
        clear_samples(sector);
        set_selectors(sector, (uint8_t)shift);
        sector[24u + 17u * 128u + 16u + 3u + 27u * 4u] = 0x7Fu;
        decode_sector(inst, sector, decoded, &frames, &low_rate);
        uint32_t effective = shift >= 13u ? 9u : shift;
        CHECK_EQ_U64((uint16_t)inst->cdrom.xa_adpcm_history[1][0],
                     (uint16_t)(32512u / (1u << effective)));
        schway_destroy(inst);
    }
}

static void coding_matrix_and_channel_order(void)
{
    static const struct {
        uint8_t coding;
        uint32_t decoded_frames;
        uint32_t output_frames;
    } cases[] = {
        {0u, 4032u, 4704u}, {1u, 2016u, 2352u}, {4u, 4032u, 9408u},
        {5u, 2016u, 4704u}, {16u, 2016u, 2352u}, {17u, 1008u, 1176u},
        {20u, 2016u, 4704u}, {21u, 1008u, 2352u},
    };
    uint8_t sector[SCHWAY__CDXA_RAW_SECTOR_BYTES];
    int16_t decoded[SCHWAY__CDXA_MAX_DECODED_FRAMES * 2u];
    int16_t output[SCHWAY__CDXA_MAX_OUTPUT_FRAMES * 2u];
    for (size_t i = 0u; i < sizeof cases / sizeof cases[0]; i++) {
        schway *inst = make();
        uint32_t frames = 0u, low_rate = 0u, output_frames = 0u;
        uint8_t sample = (cases[i].coding & 0x10u) != 0u ? 0x01u : 0x11u;
        make_sector(sector, cases[i].coding, sample);
        decode_sector(inst, sector, decoded, &frames, &low_rate);
        CHECK_EQ_U64(frames, cases[i].decoded_frames);
        CHECK_EQ_U64(low_rate, (cases[i].coding & 4u) != 0u ? 1u : 0u);
        CHECK_EQ_U64(schway__cdxa_decode(inst, sector, sizeof sector, output,
                                         SCHWAY__CDXA_MAX_OUTPUT_FRAMES,
                                         &output_frames), 1u);
        CHECK_EQ_U64(output_frames, cases[i].output_frames);
        schway_destroy(inst);
    }

    schway *inst = make();
    uint32_t frames = 0u, low_rate = 0u;
    make_sector(sector, 17u, 0u);
    clear_samples(sector);
    sector[24u + 17u * 128u + 16u + 2u + 27u * 4u] = 0x7Fu;
    sector[24u + 17u * 128u + 16u + 3u + 27u * 4u] = 0x80u;
    decode_sector(inst, sector, decoded, &frames, &low_rate);
    CHECK_EQ_U64(frames, 1008u);
    CHECK_EQ_U64((uint16_t)inst->cdrom.xa_adpcm_history[0][0], 32512u);
    CHECK(inst->cdrom.xa_adpcm_history[1][0] == INT16_MIN);
    schway_destroy(inst);

    inst = make();
    make_sector(sector, 1u, 0u);
    clear_samples(sector);
    sector[24u + 17u * 128u + 16u + 3u + 27u * 4u] = 0x21u;
    decode_sector(inst, sector, decoded, &frames, &low_rate);
    CHECK_EQ_U64(decoded[2u * (frames - 1u)], 4096u);
    CHECK_EQ_U64(decoded[2u * (frames - 1u) + 1u], 8192u);
    schway_destroy(inst);

    inst = make();
    make_sector(sector, 0u, 0u);
    clear_samples(sector);
    sector[24u + 17u * 128u + 16u + 3u + 27u * 4u] = 0x20u;
    decode_sector(inst, sector, decoded, &frames, &low_rate);
    CHECK_EQ_U64(frames, 4032u);
    CHECK_EQ_U64(decoded[2u * (frames - 1u)], 8192u);
    CHECK_EQ_U64(decoded[2u * (frames - 1u) + 1u], 8192u);
    schway_destroy(inst);
}

static void malformed_input_and_capacity(void)
{
    schway *inst = make();
    uint8_t sector[SCHWAY__CDXA_RAW_SECTOR_BYTES];
    int16_t output[SCHWAY__CDXA_MAX_OUTPUT_FRAMES * 2u];
    uint32_t frames = 99u;
    make_sector(sector, 1u, 0x11u);
    CHECK_EQ_U64(schway__cdxa_decode(inst, sector, sizeof sector - 1u, output,
                                     SCHWAY__CDXA_MAX_OUTPUT_FRAMES, &frames), 0u);
    CHECK_EQ_U64(frames, 0u);
    CHECK_EQ_U64(inst->cdrom.xa_adpcm_history[0][0], 0u);
    sector[20] ^= 1u;
    CHECK_EQ_U64(schway__cdxa_decode(inst, sector, sizeof sector, output,
                                     SCHWAY__CDXA_MAX_OUTPUT_FRAMES, &frames), 0u);
    CHECK_EQ_U64(frames, 0u);
    sector[20] ^= 1u;
    CHECK_EQ_U64(schway__cdxa_decode(inst, sector, sizeof sector, output, 1u, &frames), 0u);
    CHECK_EQ_U64(frames, 0u);
    CHECK_EQ_U64(inst->cdrom.xa_adpcm_history[0][0], 0u);
    inst->cdrom.xa_phase = 6u;
    CHECK_EQ_U64(schway__cdxa_decode(inst, sector, sizeof sector, output,
                                     SCHWAY__CDXA_MAX_OUTPUT_FRAMES, &frames), 0u);
    CHECK_EQ_U64(frames, 0u);
    CHECK_EQ_U64(inst->cdrom.xa_adpcm_history[0][0], 0u);
    inst->cdrom.xa_phase = 0u;
    make_sector(sector, 1u, 0x11u);
    sector[19] = 2u;
    memcpy(&sector[20], &sector[16], 4u);
    CHECK_EQ_U64(schway__cdxa_decode(inst, sector, sizeof sector, output,
                                     SCHWAY__CDXA_MAX_OUTPUT_FRAMES, &frames), 0u);
    CHECK_EQ_U64(frames, 0u);
    make_sector(sector, 1u, 0x11u);
    sector[19] = 9u;
    memcpy(&sector[20], &sector[16], 4u);
    CHECK_EQ_U64(schway__cdxa_decode(inst, sector, sizeof sector, output,
                                     SCHWAY__CDXA_MAX_OUTPUT_FRAMES, &frames), 0u);
    CHECK_EQ_U64(frames, 0u);
    CHECK_EQ_U64(inst->cdrom.xa_adpcm_history[0][0], 0u);
    make_sector(sector, 0x41u, 0x11u);
    CHECK_EQ_U64(schway__cdxa_decode(inst, sector, sizeof sector, output,
                                     SCHWAY__CDXA_MAX_OUTPUT_FRAMES, &frames), 0u);
    CHECK_EQ_U64(frames, 0u);
    CHECK_EQ_U64(inst->cdrom.xa_adpcm_history[0][0], 0u);
    make_sector(sector, 1u, 0x11u);
    sector[24u] ^= 1u;
    CHECK_EQ_U64(schway__cdxa_decode(inst, sector, sizeof sector, output,
                                     SCHWAY__CDXA_MAX_OUTPUT_FRAMES, &frames), 0u);
    CHECK_EQ_U64(frames, 0u);
    CHECK_EQ_U64(inst->cdrom.xa_adpcm_history[0][0], 0u);
    make_sector(sector, 1u, 0x11u);
    sector[24u] |= 0x40u;
    sector[28u] |= 0x40u;
    CHECK_EQ_U64(schway__cdxa_decode(inst, sector, sizeof sector, output,
                                     SCHWAY__CDXA_MAX_OUTPUT_FRAMES, &frames), 0u);
    CHECK_EQ_U64(frames, 0u);
    CHECK_EQ_U64(inst->cdrom.xa_adpcm_history[0][0], 0u);
    schway_destroy(inst);
}

static void predictor_history_and_reset(void)
{
    schway *inst = make();
    schway *fresh = make();
    uint8_t sector[SCHWAY__CDXA_RAW_SECTOR_BYTES];
    int16_t decoded[SCHWAY__CDXA_MAX_DECODED_FRAMES * 2u];
    uint32_t frames = 0u, low_rate = 0u;
    make_sector(sector, 1u, 0x11u);
    set_selectors(sector, 0x10u);
    decode_sector(inst, sector, decoded, &frames, &low_rate);
    CHECK_EQ_U64(inst->cdrom.xa_adpcm_history[0][0], INT16_MAX);
    make_sector(sector, 1u, 0u);
    clear_samples(sector);
    set_selectors(sector, 0x10u);
    decode_sector(inst, sector, decoded, &frames, &low_rate);
    CHECK_EQ_U64(decoded[0], 30719u);
    decode_sector(fresh, sector, decoded, &frames, &low_rate);
    CHECK_EQ_U64(decoded[0], 0u);
    schway__cdrom_reset(inst);
    CHECK_EQ_U64(inst->cdrom.xa_phase, 0u);
    for (uint32_t channel = 0u; channel < 2u; channel++) {
        for (uint32_t history = 0u; history < 2u; history++) {
            CHECK_EQ_U64(inst->cdrom.xa_adpcm_history[channel][history], 0u);
        }
        for (uint32_t tap = 0u; tap < SCHWAY__CDROM_XA_TAPS; tap++) {
            CHECK_EQ_U64(inst->cdrom.xa_history[channel][tap], 0u);
        }
    }
    schway_destroy(fresh);
    schway_destroy(inst);
}

static const int16_t reference_taps[SCHWAY__CDROM_XA_TAPS][7] = {
    {0, 0, 0, 0, -1, 2, -5}, {0, 0, 0, -1, 3, -8, 17},
    {0, 0, -1, 3, -8, 16, -35}, {0, -2, 3, -8, 17, -35, 70},
    {0, 0, -2, 6, -16, 43, -23}, {-2, 3, -5, 5, 10, 26, -68},
    {10, -19, 31, -27, 107, -235, 347}, {-34, 60, -74, 166, -365, 635, -839},
    {65, -75, 179, -424, 848, -1352, 2062}, {-84, 162, -402, 882, -1571, 2810, -4681},
    {52, -227, 689, -1471, 3021, -5882, 15367}, {9, 306, -926, 2488, -6016, 21472, 21472},
    {-266, -67, 1272, -4532, 26516, 15367, -5882},
    {1024, -615, -1446, 29883, 9036, -4681, 2810},
    {-2680, 3229, 31033, 3229, -2680, 2062, -1352},
    {9036, 29883, -1446, -615, 1024, -839, 635},
    {26516, -4532, 1272, -67, -266, 347, -235},
    {-6016, 2488, -926, 306, 9, -68, 26}, {3021, -1471, 689, -227, 52, -23, 43},
    {-1571, 882, -402, 162, -84, 70, -35}, {848, -424, 179, -75, 65, -35, 16},
    {-365, 166, -74, 60, -34, 17, -8}, {107, -27, 31, -19, 10, -5, 2},
    {10, 5, -5, 3, -1, 0, 0}, {-16, 6, -2, 0, 0, 0, 0},
    {17, -8, 3, -2, 1, 0, 0}, {-8, 3, -1, 0, 0, 0, 0},
    {3, -1, 0, 0, 0, 0, 0}, {-1, 0, 0, 0, 0, 0, 0},
};

static int16_t reference_clamp(int64_t value)
{
    if (value > INT16_MAX) {
        return INT16_MAX;
    }
    if (value < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t)value;
}

static uint32_t reference_resample(const int16_t *input, uint32_t frames,
                                   uint32_t low_rate, int16_t *output)
{
    int16_t history[2][SCHWAY__CDROM_XA_TAPS] = {{0}};
    uint32_t phase = 0u, written = 0u;
    for (uint32_t frame = 0u; frame < frames; frame++) {
        uint32_t repeats = low_rate != 0u ? 2u : 1u;
        for (uint32_t repeat = 0u; repeat < repeats; repeat++) {
            for (uint32_t tap = SCHWAY__CDROM_XA_TAPS - 1u; tap > 0u; tap--) {
                history[0][tap] = history[0][tap - 1u];
                history[1][tap] = history[1][tap - 1u];
            }
            history[0][0] = input[2u * frame];
            history[1][0] = input[2u * frame + 1u];
            phase++;
            if (phase == 6u) {
                phase = 0u;
                for (uint32_t output_phase = 0u; output_phase < 7u; output_phase++) {
                    for (uint32_t channel = 0u; channel < 2u; channel++) {
                        int64_t sum = 0;
                        for (uint32_t tap = 0u; tap < SCHWAY__CDROM_XA_TAPS; tap++) {
                            sum += (int64_t)history[channel][tap] *
                                   reference_taps[tap][output_phase];
                        }
                        output[2u * written + channel] = reference_clamp(sum / 32768);
                    }
                    written++;
                }
            }
        }
    }
    return written;
}

static uint32_t compare_resampler(const int16_t *input, uint32_t frames, uint32_t low_rate)
{
    schway *inst = make();
    /* Keep the maximum-size outputs off the small default WASI stack. */
    static int16_t actual[SCHWAY__CDXA_MAX_OUTPUT_FRAMES * 2u];
    static int16_t expected[SCHWAY__CDXA_MAX_OUTPUT_FRAMES * 2u];
    uint32_t actual_frames = 0u;
    uint32_t expected_frames = reference_resample(input, frames, low_rate, expected);
    CHECK_EQ_U64(schway__cdxa_resample(inst, input, frames, low_rate, actual,
                                       SCHWAY__CDXA_MAX_OUTPUT_FRAMES,
                                       &actual_frames), 1u);
    CHECK_EQ_U64(actual_frames, expected_frames);
    CHECK(memcmp(actual, expected, 2u * actual_frames * sizeof actual[0]) == 0);
    CHECK_EQ_U64(inst->cdrom.xa_phase, (frames * (low_rate != 0u ? 2u : 1u)) % 6u);
    uint32_t saturated = 0u;
    for (uint32_t i = 0u; i < 2u * actual_frames; i++) {
        if (actual[i] == INT16_MIN || actual[i] == INT16_MAX) {
            saturated = 1u;
        }
    }
    schway_destroy(inst);
    return saturated;
}

static void resampler_vectors_and_phase(void)
{
    int16_t input[24] = {0};
    input[0] = INT16_MAX;
    input[1] = INT16_MIN;
    (void)compare_resampler(input, 12u, 0u);
    for (uint32_t frame = 0u; frame < 12u; frame++) {
        int16_t value = frame % 2u == 0u ? INT16_MAX : INT16_MIN;
        input[2u * frame] = value;
        input[2u * frame + 1u] = frame % 2u == 0u ? INT16_MIN : INT16_MAX;
    }
    (void)compare_resampler(input, 12u, 0u);
    (void)compare_resampler(input, 6u, 1u);
    int16_t saturation_input[72] = {0};
    for (uint32_t tap = 0u; tap < SCHWAY__CDROM_XA_TAPS; tap++) {
        int16_t value = reference_taps[tap][5] < 0 ? INT16_MIN : INT16_MAX;
        uint32_t frame = 35u - tap;
        saturation_input[2u * frame] = value;
        saturation_input[2u * frame + 1u] = value;
    }
    CHECK_EQ_U64(compare_resampler(saturation_input, 36u, 0u), 1u);

    schway *split = make();
    schway *whole = make();
    int16_t split_output[14];
    int16_t whole_output[14];
    uint32_t first_count = 99u, second_count = 99u, whole_count = 0u;
    CHECK_EQ_U64(schway__cdxa_resample(split, input, 2u, 0u, split_output, 0u,
                                       &first_count), 1u);
    CHECK_EQ_U64(first_count, 0u);
    CHECK_EQ_U64(split->cdrom.xa_phase, 2u);
    CHECK_EQ_U64(schway__cdxa_resample(split, &input[4], 4u, 0u, split_output, 7u,
                                       &second_count), 1u);
    CHECK_EQ_U64(second_count, 7u);
    CHECK_EQ_U64(schway__cdxa_resample(whole, input, 6u, 0u, whole_output, 7u,
                                       &whole_count), 1u);
    CHECK_EQ_U64(whole_count, 7u);
    CHECK(memcmp(split_output, whole_output, sizeof whole_output) == 0);
    CHECK_EQ_U64(split->cdrom.xa_phase, 0u);
    schway_destroy(whole);
    schway_destroy(split);
}

static void resampler_sector_split_equivalence(void)
{
    enum { HALF_FRAMES = 2016u, JOINED_FRAMES = 4032u };
    static int16_t decoded[JOINED_FRAMES * 2u];
    static int16_t split_output[SCHWAY__CDXA_MAX_OUTPUT_FRAMES * 4u];
    static int16_t joined_output[SCHWAY__CDXA_MAX_OUTPUT_FRAMES * 2u];
    static int16_t expected[SCHWAY__CDXA_MAX_OUTPUT_FRAMES * 2u];
    schway *split = make();
    schway *joined = make();
    uint32_t split_first = 0u, split_second = 0u, joined_frames = 0u;
    for (uint32_t i = 0u; i < JOINED_FRAMES; i++) {
        int32_t left = (i & 1u) != 0u ? INT16_MIN : INT16_MAX;
        int32_t right = (i % 3u) != 0u ? 12345 : -23456;
        decoded[2u * i] = (int16_t)left;
        decoded[2u * i + 1u] = (int16_t)right;
    }
    uint32_t expected_frames = reference_resample(decoded, JOINED_FRAMES, 0u, expected);
    CHECK_EQ_U64(schway__cdxa_resample(split, decoded, HALF_FRAMES, 0u, split_output,
                                       SCHWAY__CDXA_MAX_OUTPUT_FRAMES, &split_first), 1u);
    CHECK_EQ_U64(schway__cdxa_resample(split, &decoded[2u * HALF_FRAMES], HALF_FRAMES,
                                       0u, &split_output[2u * split_first],
                                       SCHWAY__CDXA_MAX_OUTPUT_FRAMES - split_first,
                                       &split_second), 1u);
    CHECK_EQ_U64(schway__cdxa_resample(joined, decoded, JOINED_FRAMES, 0u, joined_output,
                                       SCHWAY__CDXA_MAX_OUTPUT_FRAMES, &joined_frames), 1u);
    CHECK_EQ_U64(split_first + split_second, expected_frames);
    CHECK_EQ_U64(joined_frames, expected_frames);
    CHECK(memcmp(split_output, joined_output, 2u * joined_frames * sizeof joined_output[0]) == 0);
    CHECK(memcmp(joined_output, expected, 2u * joined_frames * sizeof expected[0]) == 0);
    schway_destroy(joined);
    schway_destroy(split);
}

int main(void)
{
    decode_form2_audio();
    filter_response_and_shift_controls();
    coding_matrix_and_channel_order();
    malformed_input_and_capacity();
    predictor_history_and_reset();
    resampler_vectors_and_phase();
    resampler_sector_split_equivalence();
    CHECK_DONE();
}
