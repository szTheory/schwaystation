/* Shared SPU noise clock. The slot schedule and ATTR-disable behavior follow
   psx-spx commit 3717718f642ce2ca9ff590b65918fb3a46279cc4, §§SPU Noise Generator/ATTR. */
#include "internal.h"

#define SPU_REGISTER_BASE 0x1F801C00u
#define SPU_ATTR_ADDRESS 0x1F801DAAu

void schway__spu_noise_step(schway *inst)
{
    if (inst == NULL) {
        return;
    }
    struct schway__spu *spu = &inst->spu;
    uint16_t attr = spu->reg[(SPU_ATTR_ADDRESS - SPU_REGISTER_BASE) >> 1];
    uint32_t shift = (attr >> 10) & 0x0Fu;
    uint32_t step = (attr >> 8) & 3u;
    uint32_t period = 0x20000u >> shift;
    uint32_t period_slots = period * 8u;
    uint32_t phase = spu->noise_phase % period;
    uint32_t sample_start = phase * 8u;
    static const uint8_t slot_masks[4] = {0x55u, 0x57u, 0x77u, 0x7Fu};
    uint8_t slot_mask = slot_masks[step];

    /* One sample spans eight slot units. Check this fixed window once, even
       when the shortest period places several clock slots inside it. */
    for (uint32_t offset = 0u; offset < 8u; offset++) {
        uint32_t slot_position = sample_start + offset;
        if (slot_position >= period_slots) {
            slot_position -= period_slots;
        }
        if ((slot_position % period) != 0u) {
            continue;
        }
        uint32_t slot = slot_position / period;
        if ((slot_mask & (uint8_t)(1u << slot)) == 0u) {
            continue;
        }
        uint16_t value = spu->noise_lfsr;
        uint16_t parity = (uint16_t)(((value >> 15) ^ (value >> 12) ^ (value >> 11) ^
                                      (value >> 10) ^ 1u) & 1u);
        spu->noise_lfsr = (uint16_t)((uint32_t)value * 2u + parity);
        break;
    }

    phase++;
    spu->noise_phase = phase == period ? 0u : phase;
}

int32_t schway__spu_noise_level(const schway *inst)
{
    if (inst == NULL) {
        return 0;
    }
    uint16_t value = inst->spu.noise_lfsr;
    return value <= INT16_MAX ? (int32_t)value : (int32_t)value - 0x10000;
}
