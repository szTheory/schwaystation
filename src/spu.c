/* The bounded SPU sample clock, RAM byte order and PCM ring producer.
   Hardware references: .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md
   sections 1.1 and 1.5. */
#include <string.h>

#include "internal.h"

static void ram_check_irq(schway *inst, uint32_t address)
{
    struct schway__spu *spu = &inst->spu;
    uint32_t byte_address = address & (SCHWAY__SPU_RAM_SIZE - 1u);
    if (spu->irq_enabled != 0u && (byte_address & ~7u) == spu->irqa_addr) {
        schway__irq_raise(inst, 9u);
    }
}

uint16_t schway__spu_ram_read(schway *inst, uint32_t address)
{
    uint32_t byte_address = address & (SCHWAY__SPU_RAM_SIZE - 1u);
    byte_address &= ~1u;
    ram_check_irq(inst, byte_address);
    uint32_t next = (byte_address + 1u) & (SCHWAY__SPU_RAM_SIZE - 1u);
    return (uint16_t)((uint16_t)inst->spu.ram[byte_address] |
                      ((uint16_t)inst->spu.ram[next] << 8));
}

void schway__spu_ram_write(schway *inst, uint32_t address, uint16_t value)
{
    uint32_t byte_address = address & (SCHWAY__SPU_RAM_SIZE - 1u);
    byte_address &= ~1u;
    ram_check_irq(inst, byte_address);
    uint32_t next = (byte_address + 1u) & (SCHWAY__SPU_RAM_SIZE - 1u);
    inst->spu.ram[byte_address] = (uint8_t)(value & 0xFFu);
    inst->spu.ram[next] = (uint8_t)(value >> 8);
}

uint16_t schway__spu_ram_peek(const schway *inst, uint32_t address)
{
    uint32_t byte_address = address & (SCHWAY__SPU_RAM_SIZE - 1u);
    byte_address &= ~1u;
    uint32_t next = (byte_address + 1u) & (SCHWAY__SPU_RAM_SIZE - 1u);
    return (uint16_t)((uint16_t)inst->spu.ram[byte_address] |
                      ((uint16_t)inst->spu.ram[next] << 8));
}

static void audio_push_pair(schway *inst, int16_t left, int16_t right)
{
    if (inst->audio_count >= SCHWAY__AUDIO_RING_PAIRS) {
        if (inst->audio_dropped != UINT32_MAX) {
            inst->audio_dropped++;
        }
        return;
    }
    uint32_t slot = (inst->audio_head + inst->audio_count) % SCHWAY__AUDIO_RING_PAIRS;
    inst->audio[slot][0] = left;
    inst->audio[slot][1] = right;
    inst->audio_count++;
}

void schway__spu_reset(schway *inst)
{
    memset(&inst->spu, 0, sizeof inst->spu);
    inst->spu.next_tick = SCHWAY__CYCLES_PER_PAIR;
    inst->spu.noise_lfsr = 1u;
}

uint64_t schway__spu_next(const schway *inst)
{
    return inst->spu.next_tick;
}

void schway__spu_event(schway *inst, uint64_t now)
{
    struct schway__spu *spu = &inst->spu;
    while (spu->next_tick <= now) {
        /* Later plans add voice and reverb output; a reset chip emits silence. */
        audio_push_pair(inst, 0, 0);
        if (spu->next_tick > SCHWAY__STATE_TICKS_MAX - SCHWAY__CYCLES_PER_PAIR) {
            spu->next_tick = UINT64_MAX;
            break;
        }
        spu->next_tick += SCHWAY__CYCLES_PER_PAIR;
    }
}
