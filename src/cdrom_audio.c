/* CD-DA decode, ATV routing and the bounded CD audio queue. */
#include <stdint.h>

#include "internal.h"

static int32_t cdrom_sar(int64_t value, uint32_t shift)
{
    int64_t divisor = INT64_C(1) << shift;
    if (value >= 0) {
        return (int32_t)(value / divisor);
    }
    return (int32_t)(-(((-value) + divisor - 1) / divisor));
}

static int16_t cdrom_clamp16(int32_t value)
{
    if (value > INT16_MAX) {
        return INT16_MAX;
    }
    if (value < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t)value;
}

void schway__cdrom_audio_push(schway *inst, int16_t left, int16_t right)
{
    struct schway__cdrom *cd = &inst->cdrom;
    if (cd->audio_count >= SCHWAY__CDROM_AUDIO_LIMIT) {
        return;
    }
    int32_t output_left = cdrom_sar((int64_t)left * cd->atv_active[0], 7u) +
                          cdrom_sar((int64_t)right * cd->atv_active[3], 7u);
    int32_t output_right = cdrom_sar((int64_t)left * cd->atv_active[1], 7u) +
                           cdrom_sar((int64_t)right * cd->atv_active[2], 7u);
    uint32_t slot = (cd->audio_head + cd->audio_count) % SCHWAY__CDROM_AUDIO_CAP;
    cd->audio[slot][0] = (uint16_t)cdrom_clamp16(output_left);
    cd->audio[slot][1] = (uint16_t)cdrom_clamp16(output_right);
    cd->audio_count++;
    schway__spu_cd_audio_active(inst);
}

void schway__cdrom_audio_pop(schway *inst, int16_t *left, int16_t *right)
{
    struct schway__cdrom *cd = &inst->cdrom;
    *left = 0;
    *right = 0;
    if (cd->audio_count == 0u) {
        return;
    }
    uint32_t head = cd->audio_head;
    uint16_t raw_left = cd->audio[head][0];
    uint16_t raw_right = cd->audio[head][1];
    *left = raw_left < 0x8000u ? (int16_t)raw_left : (int16_t)((int32_t)raw_left - 0x10000);
    *right = raw_right < 0x8000u ? (int16_t)raw_right : (int16_t)((int32_t)raw_right - 0x10000);
    cd->audio_head = (head + 1u) % SCHWAY__CDROM_AUDIO_CAP;
    cd->audio_count--;
}

void schway__cdrom_audio_cdda_sector(schway *inst, const uint8_t *sector)
{
    for (uint32_t frame = 0u; frame < 588u; frame++) {
        uint32_t offset = frame * 4u;
        uint16_t left = (uint16_t)((uint16_t)sector[offset] |
                                   ((uint16_t)sector[offset + 1u] << 8));
        uint16_t right = (uint16_t)((uint16_t)sector[offset + 2u] |
                                    ((uint16_t)sector[offset + 3u] << 8));
        int32_t signed_left = left < 0x8000u ? (int32_t)left : (int32_t)left - 0x10000;
        int32_t signed_right = right < 0x8000u ? (int32_t)right : (int32_t)right - 0x10000;
        schway__cdrom_audio_push(inst, (int16_t)signed_left, (int16_t)signed_right);
    }
}
