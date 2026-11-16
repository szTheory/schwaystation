/* Bounded MDEC command, block and FIFO state. Not installed. */
#ifndef SCHWAYSTATION_MDEC_H
#define SCHWAYSTATION_MDEC_H

#include <stdint.h>

#include "schwaystation.h"

struct schway__state_io;

/* DMA0 supplies up to one 20h-word input block, and a decoded 16x16 24-bit
   macroblock is at most 192 words (PSX-SPU-CDROM-AND-MDEC, 4). */
#define SCHWAY__MDEC_INPUT_WORDS 32u
#define SCHWAY__MDEC_OUTPUT_WORDS 192u
#define SCHWAY__MDEC_TABLE_BYTES 128u
#define SCHWAY__MDEC_BLOCK_SAMPLES 64u

struct schway__mdec {
    uint32_t input[SCHWAY__MDEC_INPUT_WORDS];
    uint32_t output[SCHWAY__MDEC_OUTPUT_WORDS];
    uint8_t quant[SCHWAY__MDEC_TABLE_BYTES];
    int16_t scale[SCHWAY__MDEC_BLOCK_SAMPLES];
    int16_t block[SCHWAY__MDEC_BLOCK_SAMPLES];
    int32_t work[SCHWAY__MDEC_BLOCK_SAMPLES];
    uint32_t input_head, input_count, input_half;
    uint32_t output_head, output_count;
    uint32_t command, command_words, words_remaining;
    uint32_t block_index, depth, signed_output, bit15;
    uint32_t control, command_active, block_pending;
    uint64_t block_deadline;
};

void schway__mdec_reset(schway *inst);
void schway__mdec_state_io(struct schway__state_io *io, schway *inst, uint64_t ticks);

/* The MDEC has word-wide data/status ports; the I/O wrapper preserves lane
   selection for byte and halfword accesses. */
uint32_t schway__mdec_read(schway *inst, uint32_t address, uint64_t now);
void schway__mdec_write(schway *inst, uint32_t address, uint32_t value, uint64_t now);
void schway__mdec_io(schway *inst, uint32_t wr, uint32_t address, uint32_t width,
                     uint32_t *value, uint32_t *cycles);
void schway__mdec_dma_write(schway *inst, uint32_t word, uint64_t now);
uint32_t schway__mdec_dma_read(schway *inst, uint64_t now);

uint64_t schway__mdec_next(const schway *inst);
void schway__mdec_event(schway *inst, uint64_t now);

#endif /* SCHWAYSTATION_MDEC_H */
