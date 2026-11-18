/* CD-XA ADPCM decode and deterministic 7-phase resampling. */
#include <limits.h>
#include <stddef.h>
#include <string.h>

#include "cdxa.h"
#include "internal.h"

#define SCHWAY__CDXA_PORTIONS 18u
#define SCHWAY__CDXA_PORTION_BYTES 128u
#define SCHWAY__CDXA_HEADER_BYTES 16u

/* psx-spx, CDROM Format, XA-ADPCM Pos/neg Tables. */
static const int16_t xa_positive[4] = {0, 60, 115, 98};
static const int16_t xa_negative[4] = {0, 0, -52, -55};

/* psx-spx prints these as 29 rows by seven output phases. The signed
   coefficients use Q15 scaling; dividing the integer sum by 32768 is the
   selected deterministic rounding baseline. */
static const int16_t xa_taps[SCHWAY__CDROM_XA_TAPS][7] = {
    {0, 0, 0, 0, -1, 2, -5},
    {0, 0, 0, -1, 3, -8, 17},
    {0, 0, -1, 3, -8, 16, -35},
    {0, -2, 3, -8, 17, -35, 70},
    {0, 0, -2, 6, -16, 43, -23},
    {-2, 3, -5, 5, 10, 26, -68},
    {10, -19, 31, -27, 107, -235, 347},
    {-34, 60, -74, 166, -365, 635, -839},
    {65, -75, 179, -424, 848, -1352, 2062},
    {-84, 162, -402, 882, -1571, 2810, -4681},
    {52, -227, 689, -1471, 3021, -5882, 15367},
    {9, 306, -926, 2488, -6016, 21472, 21472},
    {-266, -67, 1272, -4532, 26516, 15367, -5882},
    {1024, -615, -1446, 29883, 9036, -4681, 2810},
    {-2680, 3229, 31033, 3229, -2680, 2062, -1352},
    {9036, 29883, -1446, -615, 1024, -839, 635},
    {26516, -4532, 1272, -67, -266, 347, -235},
    {-6016, 2488, -926, 306, 9, -68, 26},
    {3021, -1471, 689, -227, 52, -23, 43},
    {-1571, 882, -402, 162, -84, 70, -35},
    {848, -424, 179, -75, 65, -35, 16},
    {-365, 166, -74, 60, -34, 17, -8},
    {107, -27, 31, -19, 10, -5, 2},
    {10, 5, -5, 3, -1, 0, 0},
    {-16, 6, -2, 0, 0, 0, 0},
    {17, -8, 3, -2, 1, 0, 0},
    {-8, 3, -1, 0, 0, 0, 0},
    {3, -1, 0, 0, 0, 0, 0},
    {-1, 0, 0, 0, 0, 0, 0},
};

struct schway__cdxa_format {
    uint32_t stereo;
    uint32_t low_rate;
    uint32_t eight_bit;
};

static uint32_t cdxa_format(const uint8_t *sector, uint32_t length,
                            struct schway__cdxa_format *format)
{
    if (sector == NULL || format == NULL || length != SCHWAY__CDXA_RAW_SECTOR_BYTES ||
        sector[0] != 0u || sector[11] != 0u || sector[15] != 2u ||
        (sector[18] & 0x24u) != 0x24u || memcmp(&sector[16], &sector[20], 4u) != 0) {
        return 0u;
    }
    for (uint32_t i = 1u; i < 11u; i++) {
        if (sector[i] != 0xFFu) {
            return 0u;
        }
    }

    uint8_t coding = sector[19];
    uint32_t channels = coding & 3u;
    uint32_t bits = (coding >> 4) & 3u;
    if (channels > 1u || bits > 1u || (coding & 0xC8u) != 0u) {
        return 0u;
    }
    format->stereo = channels;
    format->low_rate = (coding >> 2) & 1u;
    format->eight_bit = bits;

    for (uint32_t portion = 0u; portion < SCHWAY__CDXA_PORTIONS; portion++) {
        const uint8_t *chunk = &sector[24u + portion * SCHWAY__CDXA_PORTION_BYTES];
        if (memcmp(chunk, &chunk[4], 4u) != 0 || memcmp(&chunk[12], &chunk[8], 4u) != 0) {
            return 0u;
        }
        for (uint32_t header = 4u; header < 12u; header++) {
            if ((chunk[header] & 0xC0u) != 0u) {
                return 0u;
            }
        }
    }
    return 1u;
}

static uint32_t decoded_frames(const struct schway__cdxa_format *format)
{
    uint32_t per_portion;
    if (format->eight_bit != 0u) {
        per_portion = format->stereo != 0u ? 56u : 112u;
    } else {
        per_portion = format->stereo != 0u ? 112u : 224u;
    }
    return SCHWAY__CDXA_PORTIONS * per_portion;
}

static int16_t clamp_sample(int32_t value)
{
    if (value > INT16_MAX) {
        return INT16_MAX;
    }
    if (value < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t)value;
}

static int32_t signed_right_shift(int32_t value, uint32_t shift)
{
    /* Spell out arithmetic shift so negative samples are platform independent. */
    if (shift == 0u || value >= 0) {
        return value / (int32_t)(UINT32_C(1) << shift);
    }
    int32_t divisor = (int32_t)(UINT32_C(1) << shift);
    return -(((-value) + divisor - 1) / divisor);
}

static int16_t decode_sample(schway *inst, uint32_t channel, uint8_t encoded,
                             uint8_t selector, uint32_t eight_bit)
{
    uint32_t shift = selector & 0x0Fu;
    if (shift >= 13u) {
        shift = 9u;
    }
    uint32_t filter = (selector >> 4) & 3u;
    int32_t sample;
    if (eight_bit != 0u) {
        int32_t signed_byte = encoded >= 0x80u ? (int32_t)encoded - 256 : encoded;
        sample = signed_byte * 256;
        sample = signed_right_shift(sample, shift);
    } else {
        int32_t nibble = (int32_t)(encoded & 0x0Fu);
        if ((nibble & 8) != 0) {
            nibble -= 16;
        }
        sample = signed_right_shift(nibble * 4096, shift);
    }

    int32_t old = inst->cdrom.xa_adpcm_history[channel][0];
    int32_t older = inst->cdrom.xa_adpcm_history[channel][1];
    int32_t prediction = (old * xa_positive[filter] + older * xa_negative[filter] + 32) / 64;
    int16_t result = clamp_sample(sample + prediction);
    inst->cdrom.xa_adpcm_history[channel][1] = inst->cdrom.xa_adpcm_history[channel][0];
    inst->cdrom.xa_adpcm_history[channel][0] = result;
    return result;
}

static void put_sample(int16_t *output, uint32_t frame, uint32_t channel, int16_t sample)
{
    output[2u * frame + channel] = sample;
}

uint32_t schway__cdxa_decode_adpcm(schway *inst, const uint8_t *sector, uint32_t length,
                                   int16_t *decoded, uint32_t decoded_capacity,
                                   uint32_t *decoded_count, uint32_t *low_rate)
{
    struct schway__cdxa_format format;
    if (decoded_count != NULL) {
        *decoded_count = 0u;
    }
    if (low_rate != NULL) {
        *low_rate = 0u;
    }
    if (inst == NULL || decoded == NULL || decoded_count == NULL || low_rate == NULL ||
        cdxa_format(sector, length, &format) == 0u) {
        return 0u;
    }
    uint32_t count = decoded_frames(&format);
    if (decoded_capacity < count) {
        return 0u;
    }

    uint32_t frame_base = 0u;
    for (uint32_t portion = 0u; portion < SCHWAY__CDXA_PORTIONS; portion++) {
        const uint8_t *chunk = &sector[24u + portion * SCHWAY__CDXA_PORTION_BYTES];
        if (format.eight_bit != 0u) {
            for (uint32_t block = 0u; block < 4u; block++) {
                uint32_t channel = format.stereo != 0u ? block & 1u : 0u;
                uint32_t base = format.stereo != 0u ? (block / 2u) * 28u : block * 28u;
                uint8_t selector = chunk[4u + block];
                for (uint32_t sample_index = 0u; sample_index < 28u; sample_index++) {
                    uint8_t encoded = chunk[SCHWAY__CDXA_HEADER_BYTES + block + sample_index * 4u];
                    int16_t sample = decode_sample(inst, channel, encoded, selector, 1u);
                    put_sample(decoded, frame_base + base + sample_index, channel, sample);
                    if (format.stereo == 0u) {
                        put_sample(decoded, frame_base + base + sample_index, 1u, sample);
                    }
                }
            }
        } else if (format.stereo != 0u) {
            for (uint32_t block = 0u; block < 4u; block++) {
                uint32_t base = block * 28u;
                uint8_t left_selector = chunk[4u + block * 2u];
                uint8_t right_selector = chunk[5u + block * 2u];
                for (uint32_t sample_index = 0u; sample_index < 28u; sample_index++) {
                    uint8_t packed = chunk[SCHWAY__CDXA_HEADER_BYTES + block + sample_index * 4u];
                    int16_t left = decode_sample(inst, 0u, packed, left_selector, 0u);
                    int16_t right = decode_sample(inst, 1u, packed >> 4, right_selector, 0u);
                    put_sample(decoded, frame_base + base + sample_index, 0u, left);
                    put_sample(decoded, frame_base + base + sample_index, 1u, right);
                }
            }
        } else {
            uint32_t base = 0u;
            for (uint32_t block = 0u; block < 4u; block++) {
                uint8_t low_selector = chunk[4u + block * 2u];
                uint8_t high_selector = chunk[5u + block * 2u];
                for (uint32_t nibble = 0u; nibble < 2u; nibble++) {
                    uint8_t selector = nibble == 0u ? low_selector : high_selector;
                    for (uint32_t sample_index = 0u; sample_index < 28u; sample_index++) {
                        uint8_t packed = chunk[SCHWAY__CDXA_HEADER_BYTES + block + sample_index * 4u];
                        uint8_t encoded = nibble == 0u ? packed : (uint8_t)(packed >> 4);
                        int16_t sample = decode_sample(inst, 0u, encoded, selector, 0u);
                        put_sample(decoded, frame_base + base + sample_index, 0u, sample);
                        put_sample(decoded, frame_base + base + sample_index, 1u, sample);
                    }
                    base += 28u;
                }
            }
        }
        frame_base += format.eight_bit != 0u
                          ? (format.stereo != 0u ? 56u : 112u)
                          : (format.stereo != 0u ? 112u : 224u);
    }
    *decoded_count = count;
    *low_rate = format.low_rate;
    return 1u;
}

static uint32_t resampled_count(const schway *inst, uint32_t decoded_count, uint32_t low_rate)
{
    uint64_t input_count = (uint64_t)decoded_count * (low_rate != 0u ? 2u : 1u);
    input_count += inst->cdrom.xa_phase;
    return (uint32_t)((input_count / 6u) * 7u);
}

static void resample_input(schway *inst, int16_t left, int16_t right, int16_t *output,
                           uint32_t *written)
{
    for (uint32_t tap = SCHWAY__CDROM_XA_TAPS - 1u; tap > 0u; tap--) {
        inst->cdrom.xa_history[0][tap] = inst->cdrom.xa_history[0][tap - 1u];
        inst->cdrom.xa_history[1][tap] = inst->cdrom.xa_history[1][tap - 1u];
    }
    inst->cdrom.xa_history[0][0] = left;
    inst->cdrom.xa_history[1][0] = right;
    inst->cdrom.xa_phase++;
    if (inst->cdrom.xa_phase == 6u) {
        inst->cdrom.xa_phase = 0u;
        for (uint32_t phase = 0u; phase < 7u; phase++) {
            for (uint32_t channel = 0u; channel < 2u; channel++) {
                int64_t sum = 0;
                for (uint32_t tap = 0u; tap < SCHWAY__CDROM_XA_TAPS; tap++) {
                    sum += (int64_t)inst->cdrom.xa_history[channel][tap] * xa_taps[tap][phase];
                }
                int64_t sample = sum / INT64_C(32768);
                if (sample > INT16_MAX) {
                    sample = INT16_MAX;
                } else if (sample < INT16_MIN) {
                    sample = INT16_MIN;
                }
                output[2u * *written + channel] = (int16_t)sample;
            }
            (*written)++;
        }
    }
}

uint32_t schway__cdxa_resample(schway *inst, const int16_t *decoded,
                               uint32_t decoded_count, uint32_t low_rate,
                               int16_t *output, uint32_t output_capacity,
                               uint32_t *output_count)
{
    if (output_count != NULL) {
        *output_count = 0u;
    }
    if (inst == NULL || decoded == NULL || output == NULL || output_count == NULL ||
        decoded_count > SCHWAY__CDXA_MAX_DECODED_FRAMES || low_rate > 1u ||
        inst->cdrom.xa_phase >= 6u) {
        return 0u;
    }
    uint32_t count = resampled_count(inst, decoded_count, low_rate);
    if (count > output_capacity) {
        return 0u;
    }
    uint32_t written = 0u;
    for (uint32_t frame = 0u; frame < decoded_count; frame++) {
        int16_t left = decoded[2u * frame];
        int16_t right = decoded[2u * frame + 1u];
        resample_input(inst, left, right, output, &written);
        if (low_rate != 0u) {
            resample_input(inst, left, right, output, &written);
        }
    }
    *output_count = written;
    return 1u;
}

uint32_t schway__cdxa_decode(schway *inst, const uint8_t *sector, uint32_t length,
                             int16_t *output, uint32_t output_capacity,
                             uint32_t *output_count)
{
    int16_t *decoded;
    uint32_t decoded_count = 0u;
    uint32_t low_rate = 0u;
    uint32_t actual_count = 0u;
    struct schway__cdxa_format format;
    if (output_count != NULL) {
        *output_count = 0u;
    }
    if (inst == NULL || inst->cdrom.xa_phase >= 6u || output == NULL || output_count == NULL ||
        cdxa_format(sector, length, &format) == 0u) {
        return 0u;
    }
    decoded = inst->xa_adpcm_scratch;
    uint32_t expected_count = resampled_count(inst, decoded_frames(&format), format.low_rate);
    if (expected_count > output_capacity) {
        return 0u;
    }
    if (schway__cdxa_decode_adpcm(inst, sector, length, decoded,
                                  SCHWAY__CDXA_MAX_DECODED_FRAMES, &decoded_count,
                                  &low_rate) == 0u ||
        schway__cdxa_resample(inst, decoded, decoded_count, low_rate, output,
                              output_capacity, &actual_count) == 0u) {
        return 0u;
    }
    *output_count = actual_count;
    return 1u;
}
