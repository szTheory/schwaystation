/* The bounded SPU sample clock, RAM byte order and PCM ring producer.
   Hardware references: .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md
   sections 1.1 and 1.5–1.6. */
#include <string.h>

#include "internal.h"
#include "state.h"

#define SPU_REGISTER_BASE 0x1F801C00u
#define SPU_REGISTER_END 0x1F801DFFu
#define SPU_CURRENT_VOLUME_BASE 0x1F801E00u
#define SPU_CURRENT_VOLUME_END 0x1F801E5Fu
#define SPU_REGISTER_RAM_BASE 0x1F801E60u
#define SPU_REGISTER_RAM_END 0x1F801E7Fu
#define SPU_ATTR_ADDRESS 0x1F801DAAu
#define SPU_IRQA_ADDRESS 0x1F801DA4u
#define SPU_TSA_ADDRESS 0x1F801DA6u
#define SPU_FIFO_ADDRESS 0x1F801DA8u
#define SPU_RAM_CTRL_ADDRESS 0x1F801DACu
#define SPU_STATX_ADDRESS 0x1F801DAEu
#define SPU_KON_ADDRESS 0x1F801D88u
#define SPU_KOFF_ADDRESS 0x1F801D8Cu
#define SPU_ENDX_ADDRESS 0x1F801D9Cu
#define SPU_DEV4_CONTROL_INDEX 5u
#define SPU_DEV4_DMA_READ_CONTROL 0x220931E1u
#define IRQ9_MASK (UINT32_C(1) << 9)

uint32_t schway__spu_reg_supported(uint32_t address)
{
    address &= ~1u;
    return (address >= SPU_REGISTER_BASE && address <= SPU_REGISTER_END) ||
           (address >= SPU_CURRENT_VOLUME_BASE && address <= SPU_CURRENT_VOLUME_END) ||
           (address >= SPU_REGISTER_RAM_BASE && address <= SPU_REGISTER_RAM_END);
}

static uint16_t spu_fifo_peek(const struct schway__spu *spu)
{
    if (spu->transfer_count == 0u) {
        return 0u;
    }
    return spu->transfer_fifo[spu->transfer_head];
}

static uint16_t spu_fifo_pop(struct schway__spu *spu)
{
    if (spu->transfer_count == 0u) {
        return 0u;
    }
    uint16_t value = spu_fifo_peek(spu);
    spu->transfer_head = (spu->transfer_head + 1u) % SCHWAY__SPU_FIFO_HALFWORDS;
    spu->transfer_count--;
    return value;
}

static void spu_fifo_push(struct schway__spu *spu, uint16_t value)
{
    if (spu->transfer_count >= SCHWAY__SPU_FIFO_HALFWORDS) {
        return;
    }
    uint32_t slot = (spu->transfer_head + spu->transfer_count) % SCHWAY__SPU_FIFO_HALFWORDS;
    spu->transfer_fifo[slot] = value;
    spu->transfer_count++;
}

static uint16_t spu_pending_register_value(const struct schway__spu *spu, uint32_t address)
{
    uint32_t offset = address - SPU_REGISTER_BASE;
    uint16_t value = spu->reg[offset >> 1];
    for (uint32_t i = spu->pending_count; i > 0u; i--) {
        if (spu->pending[i - 1u].address == offset) {
            return spu->pending[i - 1u].value;
        }
    }
    return value;
}

static uint16_t spu_register_value(const schway *inst, uint32_t address)
{
    uint32_t aligned = address & ~1u;
    if (!schway__spu_reg_supported(aligned)) {
        return 0u;
    }
    if (aligned == SPU_FIFO_ADDRESS) {
        return spu_fifo_peek(&inst->spu);
    }
    if (aligned == SPU_RAM_CTRL_ADDRESS) {
        return spu_pending_register_value(&inst->spu, aligned);
    }
    uint32_t index = (aligned - SPU_REGISTER_BASE) >> 1;
    if (aligned == SPU_STATX_ADDRESS) {
        uint16_t attr = inst->spu.reg[(SPU_ATTR_ADDRESS - SPU_REGISTER_BASE) >> 1];
        uint16_t value = (uint16_t)(attr & 0x003Fu);
        value |= (uint16_t)((attr & 0x0020u) << 2);
        uint16_t ram_ctrl = inst->spu.reg[(SPU_RAM_CTRL_ADDRESS - SPU_REGISTER_BASE) >> 1];
        uint32_t mode = (attr >> 4) & 3u;
        if (mode == 1u && inst->spu.transfer_count != 0u) {
            value |= 0x0400u;
        }
        if (mode == 2u && inst->spu.transfer_count == 0u) {
            value |= 0x0100u;
        } else if (mode == 3u) {
            value |= 0x0200u;
        }
        if ((ram_ctrl & 0x000Cu) != 0u && inst->spu.capture_cursor >= 0x100u) {
            value |= 0x0800u;
        }
        if ((inst->irq.stat & IRQ9_MASK) != 0u) {
            value |= 0x0040u;
        }
        return value;
    }
    return inst->spu.reg[index];
}

uint16_t schway__spu_reg_read(schway *inst, uint32_t address)
{
    if ((address & ~1u) == SPU_FIFO_ADDRESS) {
        return spu_fifo_pop(&inst->spu);
    }
    return spu_register_value(inst, address);
}

uint16_t schway__spu_reg_peek(const schway *inst, uint32_t address)
{
    return spu_register_value(inst, address);
}

static void spu_apply_register_write(schway *inst, uint32_t address, uint16_t value)
{
    struct schway__spu *spu = &inst->spu;
    uint32_t aligned = address & ~1u;
    uint32_t index = (aligned - SPU_REGISTER_BASE) >> 1;
    spu->reg[index] = value;

    if (aligned < SPU_REGISTER_BASE + SCHWAY__SPU_VOICE_COUNT * 0x10u) {
        uint32_t voice = (aligned - SPU_REGISTER_BASE) >> 4;
        uint32_t reg = ((aligned - SPU_REGISTER_BASE) >> 1) & 7u;
        spu->voice[voice].reg[reg] = value;
        if (reg == 6u) {
            spu->voice[voice].envelope = value & 0x7FFFu;
        }
    } else if (aligned == SPU_ATTR_ADDRESS) {
        spu->irq_enabled = (value & 0x8040u) == 0x8040u;
        if ((value & 0x0040u) == 0u) {
            schway__irq_write_stat(inst, 0u, IRQ9_MASK);
        }
    } else if (aligned == SPU_KON_ADDRESS || aligned == SPU_KON_ADDRESS + 2u) {
        uint32_t first_voice = aligned == SPU_KON_ADDRESS ? 0u : 16u;
        for (uint32_t bit = 0u; bit < 16u; bit++) {
            if ((value & (uint16_t)(1u << bit)) == 0u) {
                continue;
            }
            uint32_t voice_index = first_voice + bit;
            if (voice_index >= SCHWAY__SPU_VOICE_COUNT) {
                continue;
            }
            struct schway__spu_voice *voice = &spu->voice[voice_index];
            voice->envelope = 0;
            voice->envelope_counter = 0u;
            voice->envelope_phase = 1u;
            uint32_t ssa_index = (voice_index * 0x10u + 0x06u) >> 1;
            voice->reg[3] = spu->reg[ssa_index];
            voice->sample_counter = UINT32_C(31) << 27;
            memset(voice->history, 0, sizeof voice->history);
            voice->sample = 0;
            voice->output = 0;
            uint32_t envx_index = (voice_index * 0x10u + 0x0Cu) >> 1;
            spu->reg[envx_index] = 0u;
            voice->reg[6] = 0u;
            uint32_t endx_index = ((SPU_ENDX_ADDRESS - SPU_REGISTER_BASE) >> 1) +
                                  (voice_index >> 4);
            spu->reg[endx_index] = (uint16_t)(spu->reg[endx_index] &
                                             (uint16_t)~(uint16_t)(1u << (voice_index & 15u)));
        }
    } else if (aligned == SPU_KOFF_ADDRESS || aligned == SPU_KOFF_ADDRESS + 2u) {
        uint32_t first_voice = aligned == SPU_KOFF_ADDRESS ? 0u : 16u;
        for (uint32_t bit = 0u; bit < 16u; bit++) {
            if ((value & (uint16_t)(1u << bit)) == 0u) {
                continue;
            }
            uint32_t voice_index = first_voice + bit;
            if (voice_index >= SCHWAY__SPU_VOICE_COUNT) {
                continue;
            }
            spu->voice[voice_index].envelope_phase = 4u;
            spu->voice[voice_index].envelope_counter = 0u;
        }
    } else if (aligned == SPU_IRQA_ADDRESS) {
        spu->irqa_addr = ((uint32_t)value << 3) & (SCHWAY__SPU_RAM_SIZE - 8u);
    } else if (aligned == SPU_TSA_ADDRESS) {
        spu->transfer_address = ((uint32_t)value << 3) & (SCHWAY__SPU_RAM_SIZE - 8u);
    }
}

void schway__spu_reg_write(schway *inst, uint32_t address, uint16_t value)
{
    struct schway__spu *spu = &inst->spu;
    uint32_t aligned = address & ~1u;
    if (!schway__spu_reg_supported(aligned) || aligned == SPU_STATX_ADDRESS ||
        (aligned >= SPU_CURRENT_VOLUME_BASE && aligned <= SPU_CURRENT_VOLUME_END)) {
        return;
    }
    if (aligned == SPU_FIFO_ADDRESS) {
        spu_fifo_push(spu, value);
        return;
    }
    if (spu->pending_count < SCHWAY__SPU_PENDING_WRITES) {
        uint32_t slot = spu->pending_count++;
        spu->pending[slot].address = (uint16_t)(aligned - SPU_REGISTER_BASE);
        spu->pending[slot].value = value;
    }
}

static void spu_apply_pending_writes(schway *inst)
{
    struct schway__spu *spu = &inst->spu;
    uint32_t count = spu->pending_count;
    spu->pending_count = 0u;
    for (uint32_t i = 0; i < count; i++) {
        uint32_t address = SPU_REGISTER_BASE + spu->pending[i].address;
        spu_apply_register_write(inst, address, spu->pending[i].value);
    }
}

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

static int32_t spu_sar32(int32_t value, uint32_t shift)
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

static int32_t spu_sar64(int64_t value, uint32_t shift)
{
    if (shift == 0u) {
        return (int32_t)value;
    }
    int64_t divisor = INT64_C(1) << shift;
    if (value >= 0) {
        return (int32_t)(value / divisor);
    }
    return (int32_t)(-(((-value) + divisor - 1) / divisor));
}

static int16_t spu_clamp16(int32_t value)
{
    if (value > INT16_MAX) {
        return INT16_MAX;
    }
    if (value < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t)value;
}

static int16_t spu_mix_voice(const struct schway__spu_voice *voice, uint32_t channel)
{
    int32_t envelope_sample = spu_sar32((int32_t)voice->output * voice->envelope, 15u);
    int32_t volume_sample = spu_sar32(envelope_sample * voice->sweep_level[channel], 15u);
    return spu_clamp16(volume_sample);
}

static void spu_mix_sample(schway *inst)
{
    int32_t left = 0;
    int32_t right = 0;
    for (uint32_t voice = 0u; voice < SCHWAY__SPU_VOICE_COUNT; voice++) {
        left += spu_mix_voice(&inst->spu.voice[voice], 0u);
        right += spu_mix_voice(&inst->spu.voice[voice], 1u);
    }
    uint16_t attr = inst->spu.reg[(SPU_ATTR_ADDRESS - SPU_REGISTER_BASE) >> 1];
    int16_t output_left = 0;
    int16_t output_right = 0;
    if ((attr & 0xC000u) == 0xC000u) {
        output_left = spu_clamp16(spu_sar64((int64_t)left * inst->spu.main_sweep_level[0], 15u));
        output_right = spu_clamp16(spu_sar64((int64_t)right * inst->spu.main_sweep_level[1], 15u));
    }
    audio_push_pair(inst, output_left, output_right);
}

void schway__spu_reset(schway *inst)
{
    memset(&inst->spu, 0, sizeof inst->spu);
    inst->spu.next_tick = SCHWAY__CYCLES_PER_PAIR;
    inst->spu.noise_lfsr = 1u;
    for (uint32_t i = 0u; i < SCHWAY__SPU_VOICE_COUNT; i++) {
        inst->spu.voice[i].sample_counter = UINT32_C(31) << 27;
    }
}

uint64_t schway__spu_next(const schway *inst)
{
    return inst->spu.next_tick;
}

static uint32_t spu_ram_replication(const struct schway__spu *spu)
{
    uint16_t ram_ctrl = spu->reg[(SPU_RAM_CTRL_ADDRESS - SPU_REGISTER_BASE) >> 1];
    switch (ram_ctrl & 0x000Eu) {
    case 0x0004u:
        return 1u;
    case 0x0006u:
        return 2u;
    case 0x0008u:
        return 4u;
    case 0x000Au:
        return 8u;
    default:
        return 1u;
    }
}

static uint16_t spu_dma_attr(const struct schway__spu *spu)
{
    return spu_pending_register_value(spu, SPU_ATTR_ADDRESS);
}

/* ATTR selects DMA4 direction. Request readiness comes from the SPU's DMA
   mode and FIFO state; the channel engine inserts the timed request interval
   between blocks (PSX-SPX SPU transfers, preparation §1.5). */
uint32_t schway__spu_dma_ready(const schway *inst, uint32_t direction)
{
    const struct schway__spu *spu = &inst->spu;
    uint32_t mode = (spu_dma_attr(spu) >> 4) & 3u;
    if (direction != 0u) {
        return mode == 2u && spu->transfer_count == 0u;
    }
    return mode == 3u;
}

/* DMA4 writes each guest word to the FIFO low halfword first. The data enters
   SPU RAM at the active FIFO write rate; the DMA engine accounts for its
   documented four-cycle-per-word cost (PSX-SPX SPU transfers, §1.5). */
void schway__spu_dma_write(schway *inst, uint16_t value)
{
    struct schway__spu *spu = &inst->spu;
    if (((spu_dma_attr(spu) >> 4) & 3u) != 2u) {
        return;
    }
    spu_fifo_push(spu, value);
    uint32_t copies = spu_ram_replication(spu);
    while (spu->transfer_count != 0u) {
        uint16_t data = spu_fifo_pop(spu);
        for (uint32_t i = 0u; i < copies; i++) {
            schway__spu_ram_write(inst, spu->transfer_address, data);
            spu->transfer_address = (spu->transfer_address + 2u) &
                                    (SCHWAY__SPU_RAM_SIZE - 1u);
        }
    }
}

/* DMA4 reads one little-endian halfword from SPU RAM and advances the same
   transfer address used by manual writes (PSX-SPX SPU transfers, §1.5). */
uint16_t schway__spu_dma_read(schway *inst)
{
    struct schway__spu *spu = &inst->spu;
    if (((spu_dma_attr(spu) >> 4) & 3u) != 3u) {
        return 0xFFFFu;
    }
    uint32_t address = spu->transfer_address;
    uint16_t value = inst->bus.memctl[SPU_DEV4_CONTROL_INDEX] != SPU_DEV4_DMA_READ_CONTROL &&
                             (address & 0x3Fu) == 0u
                         ? UINT16_MAX
                         : schway__spu_ram_read(inst, address);
    spu->transfer_address = (address + 2u) & (SCHWAY__SPU_RAM_SIZE - 1u);
    return value;
}

static void spu_manual_transfer_step(schway *inst)
{
    struct schway__spu *spu = &inst->spu;
    uint16_t attr = spu->reg[(SPU_ATTR_ADDRESS - SPU_REGISTER_BASE) >> 1];
    if (((attr >> 4) & 3u) != 1u || spu->transfer_count == 0u) {
        return;
    }
    uint32_t copies = spu_ram_replication(spu);
    while (spu->transfer_count != 0u) {
        uint16_t value = spu_fifo_pop(spu);
        for (uint32_t i = 0u; i < copies; i++) {
            schway__spu_ram_write(inst, spu->transfer_address, value);
            spu->transfer_address = (spu->transfer_address + 2u) &
                                    (SCHWAY__SPU_RAM_SIZE - 1u);
        }
    }
}

static void spu_capture_step(schway *inst)
{
    struct schway__spu *spu = &inst->spu;
    uint16_t ram_ctrl = spu->reg[(SPU_RAM_CTRL_ADDRESS - SPU_REGISTER_BASE) >> 1];
    if ((ram_ctrl & 0x000Cu) == 0u) {
        return;
    }
    uint32_t offset = spu->capture_cursor * 2u;
    schway__spu_ram_write(inst, offset, 0u);
    schway__spu_ram_write(inst, 0x400u + offset, 0u);
    schway__spu_ram_write(inst, 0x800u + offset, 0u);
    schway__spu_ram_write(inst, 0xC00u + offset, 0u);
    spu->capture_cursor = (spu->capture_cursor + 1u) & 0x1FFu;
}

void schway__spu_event(schway *inst, uint64_t now)
{
    struct schway__spu *spu = &inst->spu;
    while (spu->next_tick <= now) {
        spu_apply_pending_writes(inst);
        spu_manual_transfer_step(inst);
        schway__spu_envelope_step(inst, 0u);
        schway__spu_voice_step(inst, 0u);
        schway__spu_volume_step(inst, 0u);
        spu_capture_step(inst);
        for (uint32_t voice = 1u; voice < SCHWAY__SPU_VOICE_COUNT; voice++) {
            schway__spu_envelope_step(inst, voice);
            schway__spu_voice_step(inst, voice);
            schway__spu_volume_step(inst, voice);
        }
        schway__spu_main_volume_step(inst);
        spu_mix_sample(inst);
        if (spu->next_tick > SCHWAY__STATE_TICKS_MAX - SCHWAY__CYCLES_PER_PAIR) {
            spu->next_tick = UINT64_MAX;
            break;
        }
        spu->next_tick += SCHWAY__CYCLES_PER_PAIR;
    }
}

/* Signed machine values use explicit two's-complement wire encodings rather
   than relying on host signed representation or signed shifts. */
static uint16_t signed16_to_wire(int16_t value)
{
    if (value >= 0) {
        return (uint16_t)value;
    }
    return (uint16_t)(UINT16_MAX - (uint16_t)(-(int32_t)value - 1));
}

static int16_t signed16_from_wire(uint16_t value)
{
    if (value <= INT16_MAX) {
        return (int16_t)value;
    }
    return (int16_t)(-1 - (int32_t)(UINT16_MAX - value));
}

static uint32_t signed32_to_wire(int32_t value)
{
    if (value >= 0) {
        return (uint32_t)value;
    }
    return UINT32_MAX - (uint32_t)(-(int64_t)value - INT64_C(1));
}

static int32_t signed32_from_wire(uint32_t value)
{
    if (value <= INT32_MAX) {
        return (int32_t)value;
    }
    return (int32_t)(-INT64_C(1) - (int64_t)(UINT32_MAX - value));
}

static uint16_t state_u16(struct schway__state_io *io, const char *name, uint16_t *value,
                          uint16_t min, uint16_t max)
{
    if (io->mode == SCHWAY__STATE_VISIT) {
        io->visitor(io->user, name, value, 1u, 2u);
        return *value;
    }
    uint16_t x = (uint16_t)schway__state_field_io(io, name, *value, 2u, min, max);
    if (io->mode == SCHWAY__STATE_APPLY) {
        *value = x;
    }
    return x;
}

static void state_i16_array(struct schway__state_io *io, const char *name, int16_t *values,
                            uint32_t count)
{
    switch (io->mode) {
    case SCHWAY__STATE_RECORD:
        schway__state_record_one(io, name, 2u, count, 0u, UINT16_MAX);
        io->n += (size_t)count * 2u;
        return;
    case SCHWAY__STATE_SIZE:
        io->n += (size_t)count * 2u;
        return;
    case SCHWAY__STATE_VISIT:
        io->visitor(io->user, name, values, count, 2u);
        return;
    case SCHWAY__STATE_WRITE:
        for (uint32_t i = 0; i < count; i++) {
            uint16_t encoded = signed16_to_wire(values[i]);
            io->w[2u * i] = (uint8_t)(encoded & 0xFFu);
            io->w[2u * i + 1u] = (uint8_t)(encoded >> 8);
        }
        io->w += (size_t)count * 2u;
        return;
    default:
        for (uint32_t i = 0; i < count; i++) {
            uint16_t encoded =
                (uint16_t)((uint16_t)io->r[2u * i] | ((uint16_t)io->r[2u * i + 1u] << 8));
            if (io->mode == SCHWAY__STATE_APPLY) {
                values[i] = signed16_from_wire(encoded);
            }
        }
        io->r += (size_t)count * 2u;
        return;
    }
}

static void state_i32_array(struct schway__state_io *io, const char *name, int32_t *values,
                            uint32_t count)
{
    switch (io->mode) {
    case SCHWAY__STATE_RECORD:
        schway__state_record_one(io, name, 4u, count, 0u, UINT32_MAX);
        io->n += (size_t)count * 4u;
        return;
    case SCHWAY__STATE_SIZE:
        io->n += (size_t)count * 4u;
        return;
    case SCHWAY__STATE_VISIT:
        io->visitor(io->user, name, values, count, 4u);
        return;
    case SCHWAY__STATE_WRITE:
        for (uint32_t i = 0; i < count; i++) {
            uint32_t encoded = signed32_to_wire(values[i]);
            for (uint32_t k = 0; k < 4u; k++) {
                io->w[4u * i + k] = (uint8_t)(encoded >> (8u * k));
            }
        }
        io->w += (size_t)count * 4u;
        return;
    default:
        for (uint32_t i = 0; i < count; i++) {
            uint32_t encoded = (uint32_t)io->r[4u * i] |
                               ((uint32_t)io->r[4u * i + 1u] << 8) |
                               ((uint32_t)io->r[4u * i + 2u] << 16) |
                               ((uint32_t)io->r[4u * i + 3u] << 24);
            if (io->mode == SCHWAY__STATE_APPLY) {
                values[i] = signed32_from_wire(encoded);
            }
        }
        io->r += (size_t)count * 4u;
        return;
    }
}

void schway__spu_state_io(struct schway__state_io *io, schway *inst, uint64_t ticks)
{
    struct schway__spu *spu = &inst->spu;
    uint16_t voice_reg[SCHWAY__SPU_VOICE_COUNT * 8u];
    int16_t voice_history[SCHWAY__SPU_VOICE_COUNT * 3u];
    uint32_t voice_sample_counter[SCHWAY__SPU_VOICE_COUNT];
    int32_t voice_envelope[SCHWAY__SPU_VOICE_COUNT];
    uint32_t voice_envelope_counter[SCHWAY__SPU_VOICE_COUNT];
    uint32_t voice_envelope_phase[SCHWAY__SPU_VOICE_COUNT];
    int16_t voice_sample[SCHWAY__SPU_VOICE_COUNT];
    int16_t voice_output[SCHWAY__SPU_VOICE_COUNT];
    int32_t voice_sweep_level[SCHWAY__SPU_VOICE_COUNT * 2u];
    uint32_t voice_sweep_counter[SCHWAY__SPU_VOICE_COUNT * 2u];
    int32_t main_sweep_level[2];
    uint32_t main_sweep_counter[2];
    uint16_t pending_address[SCHWAY__SPU_PENDING_WRITES];
    uint16_t pending_value[SCHWAY__SPU_PENDING_WRITES];
    int16_t reverb_fir[2u * SCHWAY__SPU_REVERB_FIR_TAPS];
    int32_t reverb_work[2u * SCHWAY__SPU_REVERB_WORK_WORDS];
    uint32_t transfer_head = 0u, transfer_count = 0u, pending_count = 0u;
    uint32_t transfer_address = 0u;
    uint32_t noise_phase = 0u, capture_cursor = 0u;
    uint32_t reverb_fir_cursor = 0u, reverb_work_cursor = 0u, reverb_address = 0u;
    uint32_t irq_enabled = 0u, irqa_addr = 0u;
    uint16_t noise_lfsr = 0u;
    uint64_t next_tick = 0u;

    if (io->mode != SCHWAY__STATE_APPLY) {
        for (uint32_t i = 0; i < SCHWAY__SPU_VOICE_COUNT; i++) {
            const struct schway__spu_voice *voice = &spu->voice[i];
            for (uint32_t j = 0; j < 8u; j++) {
                voice_reg[i * 8u + j] = voice->reg[j];
            }
            for (uint32_t j = 0; j < 3u; j++) {
                voice_history[i * 3u + j] = voice->history[j];
            }
            voice_sample_counter[i] = voice->sample_counter;
            voice_envelope[i] = voice->envelope;
            voice_envelope_counter[i] = voice->envelope_counter;
            voice_envelope_phase[i] = voice->envelope_phase;
            voice_sample[i] = voice->sample;
            voice_output[i] = voice->output;
            for (uint32_t j = 0; j < 2u; j++) {
                voice_sweep_level[i * 2u + j] = voice->sweep_level[j];
                voice_sweep_counter[i * 2u + j] = voice->sweep_counter[j];
            }
        }
        for (uint32_t i = 0; i < SCHWAY__SPU_PENDING_WRITES; i++) {
            pending_address[i] = spu->pending[i].address;
            pending_value[i] = spu->pending[i].value;
        }
        for (uint32_t channel = 0; channel < 2u; channel++) {
            for (uint32_t i = 0; i < SCHWAY__SPU_REVERB_FIR_TAPS; i++) {
                reverb_fir[channel * SCHWAY__SPU_REVERB_FIR_TAPS + i] =
                    spu->reverb_fir[channel][i];
            }
            for (uint32_t i = 0; i < SCHWAY__SPU_REVERB_WORK_WORDS; i++) {
                reverb_work[channel * SCHWAY__SPU_REVERB_WORK_WORDS + i] =
                    spu->reverb_work[channel][i];
            }
        }
        transfer_head = spu->transfer_head;
        transfer_count = spu->transfer_count;
        transfer_address = spu->transfer_address;
        pending_count = spu->pending_count;
        noise_phase = spu->noise_phase;
        noise_lfsr = spu->noise_lfsr;
        capture_cursor = spu->capture_cursor;
        reverb_fir_cursor = spu->reverb_fir_cursor;
        reverb_work_cursor = spu->reverb_work_cursor;
        reverb_address = spu->reverb_address;
        main_sweep_level[0] = spu->main_sweep_level[0];
        main_sweep_level[1] = spu->main_sweep_level[1];
        main_sweep_counter[0] = spu->main_sweep_counter[0];
        main_sweep_counter[1] = spu->main_sweep_counter[1];
        irqa_addr = spu->irqa_addr;
        irq_enabled = spu->irq_enabled;
        next_tick = spu->next_tick;
    }

    schway__state_u8_array(io, "spu.ram", spu->ram, SCHWAY__SPU_RAM_SIZE, 0u, UINT8_MAX);
    schway__state_u16_array(io, "spu.reg", spu->reg, SCHWAY__SPU_REG_HALFWORDS, 0u, UINT16_MAX);
    schway__state_u16_array(io, "spu.voice.reg", voice_reg, SCHWAY__SPU_VOICE_COUNT * 8u,
                            0u, UINT16_MAX);
    state_i16_array(io, "spu.voice.history", voice_history, SCHWAY__SPU_VOICE_COUNT * 3u);
    schway__state_u32_array(io, "spu.voice.sample_counter", voice_sample_counter,
                            SCHWAY__SPU_VOICE_COUNT, 0u, UINT32_MAX);
    state_i32_array(io, "spu.voice.envelope", voice_envelope, SCHWAY__SPU_VOICE_COUNT);
    schway__state_u32_array(io, "spu.voice.envelope_counter", voice_envelope_counter,
                            SCHWAY__SPU_VOICE_COUNT, 0u, UINT32_MAX);
    schway__state_u32_array(io, "spu.voice.envelope_phase", voice_envelope_phase,
                            SCHWAY__SPU_VOICE_COUNT, 0u, 4u);
    state_i16_array(io, "spu.voice.sample", voice_sample, SCHWAY__SPU_VOICE_COUNT);
    state_i16_array(io, "spu.voice.output", voice_output, SCHWAY__SPU_VOICE_COUNT);
    state_i32_array(io, "spu.voice.sweep_level", voice_sweep_level,
                    SCHWAY__SPU_VOICE_COUNT * 2u);
    schway__state_u32_array(io, "spu.voice.sweep_counter", voice_sweep_counter,
                            SCHWAY__SPU_VOICE_COUNT * 2u, 0u, UINT32_MAX);
    schway__state_u16_array(io, "spu.transfer_fifo", spu->transfer_fifo,
                            SCHWAY__SPU_FIFO_HALFWORDS, 0u, UINT16_MAX);
    transfer_head = schway__state_u32(io, "spu.transfer_head", &transfer_head, 0u,
                                      SCHWAY__SPU_FIFO_HALFWORDS - 1u);
    transfer_count = schway__state_u32(io, "spu.transfer_count", &transfer_count, 0u,
                                       SCHWAY__SPU_FIFO_HALFWORDS);
    transfer_address = schway__state_u32(io, "spu.transfer_address", &transfer_address, 0u,
                                          SCHWAY__SPU_RAM_SIZE - 2u);
    schway__state_check(io, (transfer_address & 1u) == 0u);
    schway__state_u16_array(io, "spu.pending.address", pending_address,
                            SCHWAY__SPU_PENDING_WRITES, 0u, UINT16_MAX);
    schway__state_u16_array(io, "spu.pending.value", pending_value,
                            SCHWAY__SPU_PENDING_WRITES, 0u, UINT16_MAX);
    pending_count = schway__state_u32(io, "spu.pending_count", &pending_count, 0u,
                                      SCHWAY__SPU_PENDING_WRITES);
    noise_phase = schway__state_u32(io, "spu.noise_phase", &noise_phase, 0u, 0x1FFFFu);
    noise_lfsr = state_u16(io, "spu.noise_lfsr", &noise_lfsr, 1u, 0x7FFFu);
    capture_cursor = schway__state_u32(io, "spu.capture_cursor", &capture_cursor, 0u, 0x1FFu);
    state_i16_array(io, "spu.reverb_fir", reverb_fir, 2u * SCHWAY__SPU_REVERB_FIR_TAPS);
    state_i32_array(io, "spu.reverb_work", reverb_work, 2u * SCHWAY__SPU_REVERB_WORK_WORDS);
    reverb_fir_cursor = schway__state_u32(io, "spu.reverb_fir_cursor", &reverb_fir_cursor, 0u,
                                          SCHWAY__SPU_REVERB_FIR_TAPS - 1u);
    reverb_work_cursor = schway__state_u32(io, "spu.reverb_work_cursor", &reverb_work_cursor, 0u,
                                           SCHWAY__SPU_REVERB_WORK_WORDS - 1u);
    reverb_address = schway__state_u32(io, "spu.reverb_address", &reverb_address, 0u,
                                       SCHWAY__SPU_RAM_SIZE - 8u);
    schway__state_check(io, (reverb_address & 7u) == 0u);
    state_i32_array(io, "spu.main_sweep_level", main_sweep_level, 2u);
    schway__state_u32_array(io, "spu.main_sweep_counter", main_sweep_counter, 2u, 0u, UINT32_MAX);
    irqa_addr = schway__state_u32(io, "spu.irqa_addr", &irqa_addr, 0u,
                                  SCHWAY__SPU_RAM_SIZE - 8u);
    schway__state_check(io, (irqa_addr & 7u) == 0u);
    irq_enabled = schway__state_u32(io, "spu.irq_enabled", &irq_enabled, 0u, 1u);
    next_tick = schway__state_u64(io, "spu.next_tick", &next_tick, 0u, UINT64_MAX);
    if (io->mode == SCHWAY__STATE_VALIDATE) {
        int phase_ok = next_tick == UINT64_MAX
                           ? ticks >= SCHWAY__STATE_TICKS_MAX - SCHWAY__CYCLES_PER_PAIR
                           : (next_tick > ticks && next_tick - ticks <= SCHWAY__CYCLES_PER_PAIR &&
                              next_tick % SCHWAY__CYCLES_PER_PAIR == 0u);
        schway__state_check(io, transfer_head < SCHWAY__SPU_FIFO_HALFWORDS &&
                                    transfer_count <= SCHWAY__SPU_FIFO_HALFWORDS &&
                                    transfer_address < SCHWAY__SPU_RAM_SIZE &&
                                    pending_count <= SCHWAY__SPU_PENDING_WRITES && phase_ok);
    }
    if (io->mode == SCHWAY__STATE_APPLY) {
        for (uint32_t i = 0; i < SCHWAY__SPU_VOICE_COUNT; i++) {
            struct schway__spu_voice *voice = &spu->voice[i];
            for (uint32_t j = 0; j < 8u; j++) {
                voice->reg[j] = voice_reg[i * 8u + j];
            }
            for (uint32_t j = 0; j < 3u; j++) {
                voice->history[j] = voice_history[i * 3u + j];
            }
            voice->sample_counter = voice_sample_counter[i];
            voice->envelope = voice_envelope[i];
            voice->envelope_counter = voice_envelope_counter[i];
            voice->envelope_phase = voice_envelope_phase[i];
            voice->sample = voice_sample[i];
            voice->output = voice_output[i];
            for (uint32_t j = 0; j < 2u; j++) {
                voice->sweep_level[j] = voice_sweep_level[i * 2u + j];
                voice->sweep_counter[j] = voice_sweep_counter[i * 2u + j];
            }
        }
        for (uint32_t i = 0; i < SCHWAY__SPU_PENDING_WRITES; i++) {
            spu->pending[i].address = pending_address[i];
            spu->pending[i].value = pending_value[i];
        }
        for (uint32_t channel = 0; channel < 2u; channel++) {
            for (uint32_t i = 0; i < SCHWAY__SPU_REVERB_FIR_TAPS; i++) {
                spu->reverb_fir[channel][i] =
                    reverb_fir[channel * SCHWAY__SPU_REVERB_FIR_TAPS + i];
            }
            for (uint32_t i = 0; i < SCHWAY__SPU_REVERB_WORK_WORDS; i++) {
                spu->reverb_work[channel][i] =
                    reverb_work[channel * SCHWAY__SPU_REVERB_WORK_WORDS + i];
            }
        }
        spu->transfer_head = transfer_head;
        spu->transfer_count = transfer_count;
        spu->transfer_address = transfer_address;
        spu->pending_count = pending_count;
        spu->noise_phase = noise_phase;
        spu->noise_lfsr = noise_lfsr;
        spu->capture_cursor = capture_cursor;
        spu->reverb_fir_cursor = reverb_fir_cursor;
        spu->reverb_work_cursor = reverb_work_cursor;
        spu->reverb_address = reverb_address;
        spu->main_sweep_level[0] = main_sweep_level[0];
        spu->main_sweep_level[1] = main_sweep_level[1];
        spu->main_sweep_counter[0] = main_sweep_counter[0];
        spu->main_sweep_counter[1] = main_sweep_counter[1];
        spu->irqa_addr = irqa_addr;
        spu->irq_enabled = irq_enabled;
        spu->next_tick = next_tick;
    }
}
