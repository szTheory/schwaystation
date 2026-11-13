/* The bounded SPU sample clock, RAM byte order and PCM ring producer.
   Hardware references: .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md
   sections 1.1 and 1.5. */
#include <string.h>

#include "internal.h"
#include "state.h"

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
    schway__state_u16_array(io, "spu.reg", spu->reg, 256u, 0u, UINT16_MAX);
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
