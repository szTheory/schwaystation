/* DMA: state struct and the module's declarations. Not installed. The
   instance embeds the struct, so dma.c is edited alone. */
#ifndef SCHWAYSTATION_DMA_H
#define SCHWAYSTATION_DMA_H

#include <stdint.h>

#include "schwaystation.h"

struct schway__state_io;

/* Seven channels, 0 MDECin to 6 OTC (psx-spx, DMA Channels). cur holds the
   channel the engine is serving, or NONE. */
#define SCHWAY__DMA_CHANNELS 7u
#define SCHWAY__DMA_NONE 7u

/* The most words one unit moves: a word count of 0 in burst mode means 10000h
   words (psx-spx, DMA Channels). */
#define SCHWAY__DMA_UNIT_WORDS_MAX 0x10000u

/* The longest a unit keeps the channel busy, in CPU cycles. The slowest
   device is the CD at 40 cycles per word, and the RAM side adds 17 cycles per
   16 words: 65,536 * 40 + 65,536 * 17 / 16 = 2,691,072. Another 256 cycles
   cover the per-unit overhead, a chopping window's setup and the longest CPU
   window, 128 cycles. The sum is 2,691,328 (docs/hardware/dma.md). A stored
   stamp further ahead of the clock than this is refused. */
#define SCHWAY__DMA_BUSY_MAX                                                                       \
    (SCHWAY__DMA_UNIT_WORDS_MAX * 40u + SCHWAY__DMA_UNIT_WORDS_MAX / 16u * 17u + 256u)

/* A linked list that passes 1000h nodes in one step yields (psx-spx, DMA
   Channels, Linked List). */
#define SCHWAY__DMA_NODES_MAX 0x1000u

/* The bits the hardware stores: MADR is 24 bits; CHCR keeps bits 0, 1, 8-10,
   16-18, 20-22, 24 and 28-30; DICR keeps bits 0-5, 15 and 16-30 (bit 31 is
   derived). Anything else in a state is refused (docs/hardware/dma.md). */
#define SCHWAY__DMA_MADR_MASK 0xFFFFFFu
#define SCHWAY__DMA_CHCR_MASK 0x71770703u
#define SCHWAY__DMA_CHCR_BUSY 0x01000000u
#define SCHWAY__DMA_DICR_MASK 0x7FFF803Fu

/* A struct of arrays, one entry per channel. madr, bcr and chcr are the raw
   registers. busy_until is the tick the current unit ends on and stall_until
   the tick the CPU is held until; phase and remain are the engine's step and
   count for a channel (blocks in sync mode 1, list nodes in sync mode 2, or
   words in a chopped burst), nodes_done the last list channel's node count,
   dreq the explicit latched request credits (a bit per channel; a sync mode 1
   block takes its channel's credit with it; DMA4 ATTR readiness is derived and
   is not stored here),
   and words and crc what the stand-in sink for a channel has taken since
   power-on (a word count and a rolling CRC-32 over the words' little-endian
   bytes). */
struct schway__dma {
    uint64_t busy_until;
    uint64_t stall_until;
    uint32_t madr[SCHWAY__DMA_CHANNELS];
    uint32_t bcr[SCHWAY__DMA_CHANNELS];
    uint32_t chcr[SCHWAY__DMA_CHANNELS];
    uint32_t dpcr;
    uint32_t dicr;
    uint32_t cur;
    uint32_t phase[SCHWAY__DMA_CHANNELS];
    uint32_t remain[SCHWAY__DMA_CHANNELS];
    uint32_t nodes_done;
    uint32_t dreq;
    uint32_t words[SCHWAY__DMA_CHANNELS];
    uint32_t crc[SCHWAY__DMA_CHANNELS];
};

/* Power-on: DPCR is 07654321h, every other register is 0 and no channel is
   current (docs/hardware/dma.md). */
void schway__dma_reset(schway *inst);

/* The dma fields of the state walk (state.h). ticks is the value the walk has
   just read for instance.ticks, so the bounds follow the loaded state's clock. */
void schway__dma_state_io(struct schway__state_io *io, schway *inst, uint64_t ticks);

/* One access to the DMA registers at physical address p, of width 1, 2 or 4
   bytes: wr is 1 for a write, with the data in *value; a read leaves its
   result in *value. *cycles is the access's cost so far. */
void schway__dma_io(schway *inst, uint32_t wr, uint32_t p, uint32_t width, uint32_t *value,
                    uint32_t *cycles);

/* The tick the engine next needs attention at, or UINT64_MAX for never: the end
   of the running unit, or, with none running and a channel waiting, the tick that
   channel starts at. */
uint64_t schway__dma_next(const schway *inst);

/* Runs what is due at tick now. One call finishes each unit whose busy_until
   is not past now and starts the next channel at that tick, until none is
   due; it always leaves schway__dma_next strictly greater than now. */
void schway__dma_event(schway *inst, uint64_t now);

/* A bus access of the CPU at tick now that waits while a unit runs: the CPU
   is held until the unit's end. */
void schway__dma_bus_access(schway *inst, uint64_t now);

/* The channel of mask (a bit per channel) that goes next, or SCHWAY__DMA_NONE:
   among the channels with a transfer waiting and their DPCR enable set, the
   lowest priority value, a tie to the higher channel number. */
uint32_t schway__dma_arbitrate(const schway *inst, uint32_t mask);

/* A device's request line for channel ch changes to level. */
void schway__dma_dreq(schway *inst, uint32_t ch, uint32_t level);

/* The exact SPU ATTR halfword changed at bus tick now; wake any newly ready
   DMA4 block without moving it ahead of a future completed-unit boundary. */
void schway__dma_spu_control_changed(schway *inst, uint64_t now);

#endif /* SCHWAYSTATION_DMA_H */
