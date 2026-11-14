/* The SPU's instance-owned machine state and private chip interface. Not
   installed. Its samples enter the existing host-drained PCM ring. */
#ifndef SCHWAYSTATION_SPU_H
#define SCHWAYSTATION_SPU_H

#include <stdint.h>

#include "schwaystation.h"

#define SCHWAY__SPU_RAM_SIZE 0x80000u
#define SCHWAY__SPU_VOICE_COUNT 24u
#define SCHWAY__SPU_REG_HALFWORDS 320u
#define SCHWAY__SPU_FIFO_HALFWORDS 32u
#define SCHWAY__SPU_PENDING_WRITES 768u
#define SCHWAY__SPU_REVERB_FIR_TAPS 39u
#define SCHWAY__SPU_REVERB_WORK_WORDS 32u

struct schway__state_io;

/* Mutable voice decode, sample and envelope state stays fixed-size. The
   register image is the eight documented halfwords per voice. */
struct schway__spu_voice {
    uint16_t reg[8];
    int16_t history[3];
    uint32_t sample_counter;
    int32_t envelope;
    uint32_t envelope_counter;
    uint32_t envelope_phase;
    int16_t sample;
    int16_t output;
    int32_t sweep_level[2];
    uint32_t sweep_counter[2];
};

struct schway__spu_pending_write {
    uint16_t address;
    uint16_t value;
};

/* SPU RAM is little-endian guest storage. Reverb and capture buffers share
   it, and all cursors are byte offsets or fixed-width counts. */
struct schway__spu {
    uint8_t ram[SCHWAY__SPU_RAM_SIZE];
    uint16_t reg[SCHWAY__SPU_REG_HALFWORDS];
    struct schway__spu_voice voice[SCHWAY__SPU_VOICE_COUNT];
    uint16_t transfer_fifo[SCHWAY__SPU_FIFO_HALFWORDS];
    uint32_t transfer_head;
    uint32_t transfer_count;
    uint32_t transfer_address;
    struct schway__spu_pending_write pending[SCHWAY__SPU_PENDING_WRITES];
    uint32_t pending_count;
    uint32_t noise_phase;
    uint16_t noise_lfsr;
    uint32_t capture_cursor;
    int16_t reverb_fir[2][SCHWAY__SPU_REVERB_FIR_TAPS];
    int32_t reverb_work[2][SCHWAY__SPU_REVERB_WORK_WORDS];
    uint32_t reverb_fir_cursor;
    uint32_t reverb_work_cursor;
    uint32_t reverb_address;
    int32_t main_sweep_level[2];
    uint32_t main_sweep_counter[2];
    uint32_t irqa_addr;
    uint32_t irq_enabled;
    uint64_t next_tick;
};

void schway__spu_reset(schway *inst);
uint64_t schway__spu_next(const schway *inst);
void schway__spu_event(schway *inst, uint64_t now);

uint16_t schway__spu_reg_read(schway *inst, uint32_t address);
void schway__spu_reg_write(schway *inst, uint32_t address, uint16_t value);
uint16_t schway__spu_reg_peek(const schway *inst, uint32_t address);
uint32_t schway__spu_reg_supported(uint32_t address);

uint16_t schway__spu_ram_read(schway *inst, uint32_t address);
void schway__spu_ram_write(schway *inst, uint32_t address, uint16_t value);
uint16_t schway__spu_ram_peek(const schway *inst, uint32_t address);

uint16_t schway__spu_dma_read(schway *inst);
void schway__spu_dma_write(schway *inst, uint16_t value);
uint32_t schway__spu_dma_ready(const schway *inst, uint32_t direction);

void schway__spu_voice_step(schway *inst, uint32_t voice);
void schway__spu_envelope_step(schway *inst, uint32_t voice);
void schway__spu_volume_step(schway *inst, uint32_t voice);
void schway__spu_main_volume_step(schway *inst);
void schway__spu_envelope_rate(uint32_t rate, uint32_t decreasing, int32_t *step,
                               uint32_t *ticks);
void schway__spu_noise_step(schway *inst);
void schway__spu_reverb_step(schway *inst);
void schway__spu_state_io(struct schway__state_io *io, schway *inst, uint64_t ticks);

#endif /* SCHWAYSTATION_SPU_H */
