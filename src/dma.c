/* DMA: power-on values, the state walk and the register window. The engine
   itself is not built yet: the registers still count as an unbuilt device. */
#include <string.h>

#include "state.h"

static const char name_busy_until[] = "dma.busy_until";
static const char name_stall_until[] = "dma.stall_until";
static const char name_madr[] = "dma.madr";
static const char name_bcr[] = "dma.bcr";
static const char name_chcr[] = "dma.chcr";
static const char name_dpcr[] = "dma.dpcr";
static const char name_dicr[] = "dma.dicr";
static const char name_cur[] = "dma.cur";
static const char name_phase[] = "dma.phase";
static const char name_remain[] = "dma.remain";
static const char name_nodes_done[] = "dma.nodes_done";
static const char name_dreq[] = "dma.dreq";
static const char name_words[] = "dma.words";
static const char name_crc[] = "dma.crc";

/* DPCR is 07654321h after a power reset and the other registers are 0
   (PSX-SYSTEM section 4). */
void schway__dma_reset(schway *inst)
{
    struct schway__dma *d = &inst->dma;
    memset(d, 0, sizeof *d);
    d->dpcr = 0x07654321u;
    d->cur = SCHWAY__DMA_NONE;
}

/* a + b, or UINT64_MAX when the sum does not fit. */
static uint64_t sat_add(uint64_t a, uint64_t b)
{
    return a > UINT64_MAX - b ? UINT64_MAX : a + b;
}

/* The clock fields are bounded by the loaded state's own ticks, so the bound
   is each field's recorded maximum. An engine that is serving a channel needs
   that channel's start bit; CHCR and DICR hold only the bits the hardware
   stores. */
void schway__dma_state_io(struct schway__state_io *io, schway *inst, uint64_t ticks)
{
    struct schway__dma *d = &inst->dma;
    uint64_t clock_max = sat_add(ticks, SCHWAY__DMA_BUSY_MAX);
    uint32_t chcr[SCHWAY__DMA_CHANNELS];
    (void)schway__state_u64(io, name_busy_until, &d->busy_until, 0u, clock_max);
    (void)schway__state_u64(io, name_stall_until, &d->stall_until, 0u, clock_max);
    schway__state_u32_array(io, name_madr, d->madr, SCHWAY__DMA_CHANNELS, 0u,
                            SCHWAY__DMA_MADR_MASK);
    schway__state_u32_array(io, name_bcr, d->bcr, SCHWAY__DMA_CHANNELS, 0u, UINT32_MAX);
    for (uint32_t i = 0; i < SCHWAY__DMA_CHANNELS; i++) {
        chcr[i] = schway__state_u32(io, name_chcr, &d->chcr[i], 0u, UINT32_MAX);
        schway__state_check(io, (chcr[i] & ~SCHWAY__DMA_CHCR_MASK) == 0u);
    }
    (void)schway__state_u32(io, name_dpcr, &d->dpcr, 0u, UINT32_MAX);
    uint32_t dicr = schway__state_u32(io, name_dicr, &d->dicr, 0u, UINT32_MAX);
    schway__state_check(io, (dicr & ~SCHWAY__DMA_DICR_MASK) == 0u);
    uint32_t cur = schway__state_u32(io, name_cur, &d->cur, 0u, SCHWAY__DMA_NONE);
    schway__state_check(io, cur == SCHWAY__DMA_NONE ||
                                (cur < SCHWAY__DMA_CHANNELS &&
                                 (chcr[cur] & SCHWAY__DMA_CHCR_BUSY) != 0u));
    schway__state_u32_array(io, name_phase, d->phase, SCHWAY__DMA_CHANNELS, 0u, 7u);
    schway__state_u32_array(io, name_remain, d->remain, SCHWAY__DMA_CHANNELS, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_nodes_done, &d->nodes_done, 0u, SCHWAY__DMA_NODES_MAX);
    (void)schway__state_u32(io, name_dreq, &d->dreq, 0u, (1u << SCHWAY__DMA_CHANNELS) - 1u);
    schway__state_u32_array(io, name_words, d->words, SCHWAY__DMA_CHANNELS, 0u, UINT32_MAX);
    schway__state_u32_array(io, name_crc, d->crc, SCHWAY__DMA_CHANNELS, 0u, UINT32_MAX);
}

/* The registers read 0 and drop writes, counted as an unbuilt device. */
void schway__dma_io(schway *inst, uint32_t wr, uint32_t p, uint32_t width, uint32_t *value,
                    uint32_t *cycles)
{
    (void)cycles;
    schway__bus_unbuilt(inst, SCHWAY__DEV_DMA, p, width, wr, wr != 0u ? *value : 0u);
    if (wr == 0u) {
        *value = 0u;
    }
}

uint64_t schway__dma_next(const schway *inst)
{
    (void)inst;
    return UINT64_MAX;
}

void schway__dma_event(schway *inst, uint64_t now)
{
    (void)inst;
    (void)now;
}

void schway__dma_bus_access(schway *inst, uint64_t now)
{
    (void)inst;
    (void)now;
}

/* The levels are latched, a bit per channel. */
void schway__dma_dreq(schway *inst, uint32_t ch, uint32_t level)
{
    if (ch >= SCHWAY__DMA_CHANNELS) {
        return;
    }
    if (level != 0u) {
        inst->dma.dreq |= 1u << ch;
    } else {
        inst->dma.dreq &= ~(1u << ch);
    }
}
