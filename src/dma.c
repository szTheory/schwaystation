/* DMA: power-on values, the state walk, the register window, arbitration and
   the unit engine with the CPU hold. Only OTC has a unit so far. */
#include <string.h>

#include "internal.h"
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

/* The engine. One unit is one run of a channel that moves its words at once
   and keeps the channel busy for what the transfer costs; the event at the end
   of the busy time finishes it, and the next channel then starts. A unit is
   data-atomic and time-evented: a run to completion on a register write could
   hang on a 65,535-word burst and a per-word interleave would cost millions of
   events a second. Only OTC has a unit so far, so only it can start; the
   other channels keep their registers and wait (psx-spx, DMA Channels). */
#define DMA_OTC 6u
#define DMA_UNIT_CHANNELS (1u << DMA_OTC)

/* phase of a channel: nothing running, or its unit's words moved and the
   channel busy until busy_until. */
#define DMA_PH_IDLE 0u
#define DMA_PH_UNIT 1u

/* CHCR bit 28 starts a transfer without waiting for a request; bits 9-10 are
   the sync mode (psx-spx, DMA Channels, CHCR). OTC keeps only bits 24, 28 and
   30, and bit 1 always reads as 1. */
#define DMA_CHCR_TRIGGER 0x10000000u
#define DMA_CHCR_OTC_MASK 0x51000000u
#define DMA_CHCR_OTC_FIXED 0x00000002u

/* DICR: bits 0-5, 15 and 16-23 are stored; 24-30 are the channel flags, which
   a write of 1 clears; bit 31 is derived from the others and never stored
   (psx-spx, DMA Channels, DICR). */
#define DMA_DICR_WRITE_MASK 0x00FF803Fu
#define DMA_DICR_FLAGS 0x7F000000u
#define DMA_DICR_FORCE 0x00008000u
#define DMA_DICR_MASTER_EN 0x00800000u

/* The word at an access's width and byte offset within its register, and the
   register with that lane replaced. Every address answers at every width. */
static uint32_t lane_mask(uint32_t width)
{
    return width >= 4u ? UINT32_MAX : (UINT32_C(1) << (8u * width)) - 1u;
}

static uint32_t reg_get(uint32_t r, uint32_t p, uint32_t width)
{
    return (r >> ((p & 3u) * 8u)) & lane_mask(width);
}

static uint32_t reg_put(uint32_t old, uint32_t p, uint32_t width, uint32_t v)
{
    uint32_t shift = (p & 3u) * 8u;
    uint32_t m = lane_mask(width) << shift;
    return (old & ~m) | ((v << shift) & m);
}

/* Bit 31 of DICR: the bus-error force bit, or the master enable with any
   flag set. The per-channel enables do not enter it: they only decide whether
   a completion sets its flag (psx-spx, DMA Channels, DICR). */
static uint32_t dicr_master(uint32_t dicr)
{
    uint32_t any = (dicr & DMA_DICR_MASTER_EN) != 0u && (dicr & DMA_DICR_FLAGS) != 0u ? 1u : 0u;
    return (dicr & DMA_DICR_FORCE) != 0u || any != 0u ? 1u : 0u;
}

/* Stores a new DICR and raises I_STAT bit 3 on a 0 to 1 change of bit 31 only
   (psx-spx, DMA Channels, DICR). */
static void dicr_set(schway *inst, uint32_t dicr)
{
    uint32_t was = dicr_master(inst->dma.dicr);
    inst->dma.dicr = dicr;
    if (was == 0u && dicr_master(dicr) != 0u) {
        schway__irq_raise(inst, 3u); /* I_STAT bit 3 is the DMA interrupt */
    }
}

/* A channel with a transfer waiting: bit 24 set, and bit 28 where the mode
   needs the start trigger. OTC always does; sync mode 0 does; the request
   modes start on their own request line (psx-spx, DMA Channels, CHCR). */
static uint32_t dma_pending(const struct schway__dma *d, uint32_t ch)
{
    uint32_t c = d->chcr[ch];
    if ((c & SCHWAY__DMA_CHCR_BUSY) == 0u) {
        return 0u;
    }
    if (ch == DMA_OTC || ((c >> 9) & 3u) == 0u) {
        return (c & DMA_CHCR_TRIGGER) != 0u ? 1u : 0u;
    }
    return 1u;
}

/* The channel of mask that goes next, or SCHWAY__DMA_NONE: among the channels
   with a transfer waiting and their DPCR master enable set, the lowest
   priority value wins and a tie goes to the higher channel number; the CPU
   priority field in bits 28-31 has no effect (psx-spx, DMA Channels, DPCR;
   measured on hardware by the public dma-priority test, SCPH-5501 and
   SCPH-1001). */
uint32_t schway__dma_arbitrate(const schway *inst, uint32_t mask)
{
    const struct schway__dma *d = &inst->dma;
    uint32_t best = SCHWAY__DMA_NONE;
    uint32_t best_pri = 8u;
    for (uint32_t ch = 0u; ch < SCHWAY__DMA_CHANNELS; ch++) {
        uint32_t field = d->dpcr >> (4u * ch);
        if ((mask & (1u << ch)) == 0u || (field & 8u) == 0u || dma_pending(d, ch) == 0u) {
            continue;
        }
        if ((field & 7u) <= best_pri) {
            best = ch;
            best_pri = field & 7u;
        }
    }
    return best;
}

/* OTC writes BCR words (0 means 10000h) from MADR downward: each word is the
   address of the entry before it and the last is FFFFFFh, so the table ends
   where it was meant to be walked from. The address wraps from 0 to FFFFFCh,
   and a word past the RAM window is dropped without a fault. The direction,
   step, sync and chopping bits do not matter. MADR is left as written. A word
   costs one cycle, plus one more per 16 for the DRAM row (psx-spx, DMA
   Channels, DMA Transfer Rates and DRAM Hyper Page mode), so 4,096 words take
   4,352 cycles. Returns the cycles. */
static uint64_t dma_otc_unit(schway *inst)
{
    const struct schway__dma *d = &inst->dma;
    uint32_t n = d->bcr[DMA_OTC] & 0xFFFFu;
    uint32_t addr = d->madr[DMA_OTC] & 0xFFFFFCu;
    if (n == 0u) {
        n = SCHWAY__DMA_UNIT_WORDS_MAX;
    }
    for (uint32_t i = 0u; i < n; i++) {
        uint32_t prev = (addr - 4u) & SCHWAY__DMA_MADR_MASK;
        uint32_t word = i + 1u == n ? 0xFFFFFFu : prev;
        (void)schway__bus_ram_word(inst, addr, &word, 1u);
        addr = prev;
    }
    return ((uint64_t)n * 17u + 15u) / 16u;
}

/* Starts the unit of the channel that goes next, at tick t. Bit 28 is cleared
   when the transfer begins and bit 24 when it ends (psx-spx, DMA Channels,
   CHCR). Nothing starts when no channel is waiting. */
static void dma_start(schway *inst, uint64_t t)
{
    struct schway__dma *d = &inst->dma;
    uint32_t ch = schway__dma_arbitrate(inst, DMA_UNIT_CHANNELS);
    if (ch == SCHWAY__DMA_NONE) {
        return;
    }
    d->chcr[ch] &= ~DMA_CHCR_TRIGGER;
    d->phase[ch] = DMA_PH_UNIT;
    d->cur = ch;
    d->busy_until = t + dma_otc_unit(inst);
}

/* The end of the running unit: the channel stops being busy, its DICR flag is
   set when both its enable and the master enable allow it, and the interrupt
   edge follows (psx-spx, DMA Channels, DICR). */
static void dma_finish(schway *inst, uint32_t ch)
{
    struct schway__dma *d = &inst->dma;
    uint32_t dicr = d->dicr;
    d->chcr[ch] &= ~SCHWAY__DMA_CHCR_BUSY;
    d->phase[ch] = DMA_PH_IDLE;
    d->cur = SCHWAY__DMA_NONE;
    if (((dicr >> (16u + ch)) & 1u) != 0u && (dicr & DMA_DICR_MASTER_EN) != 0u) {
        dicr |= UINT32_C(1) << (24u + ch);
    }
    dicr_set(inst, dicr);
}

uint64_t schway__dma_next(const schway *inst)
{
    return inst->dma.cur != SCHWAY__DMA_NONE ? inst->dma.busy_until : UINT64_MAX;
}

/* Runs what is due at tick now, in order: a unit whose busy_until is not past
   now finishes at that tick, and the channel that goes next starts at that
   same tick and may finish too, until none is due. Each finish clears a
   channel's bit 24, so the loop ends, and every return leaves
   schway__dma_next strictly greater than now (a unit still running ends after
   now; with none running it is UINT64_MAX), so the scheduler always makes
   progress. A unit that passes 1000h list nodes yields in the same way: it
   returns with the channel still busy and no flag or interrupt. */
void schway__dma_event(schway *inst, uint64_t now)
{
    struct schway__dma *d = &inst->dma;
    while (d->cur != SCHWAY__DMA_NONE && d->busy_until <= now) {
        uint64_t t = d->busy_until;
        dma_finish(inst, d->cur);
        dma_start(inst, t);
    }
}

/* A CPU bus access that waits on the bus at tick now, while a unit is
   transferring, is held until the unit is done: the CPU can keep running only
   on the cache, the scratchpad, COP0 and the GTE, and a read from RAM or the
   I/O registers or a fifth queued store waits for the DMA to finish (psx-spx,
   DMA Channels, CPU Operation during DMA). The hold is a freeze until the
   unit's end, not a charge, so the instruction that waits costs what it
   always does and the overrun past a budget stays one instruction. A channel
   that is only waiting for its request line is not the running unit and never
   holds the CPU. */
void schway__dma_bus_access(schway *inst, uint64_t now)
{
    struct schway__dma *d = &inst->dma;
    if (d->cur == SCHWAY__DMA_NONE || now >= d->busy_until) {
        return;
    }
    if (d->stall_until < d->busy_until) {
        d->stall_until = d->busy_until;
    }
    inst->cpu.block_end |= SCHWAY__BLOCK_EVENT;
}

/* A write to the register at p. The access has already caught the engine up
   to its own tick. */
static void dma_write(schway *inst, uint64_t now, uint32_t p, uint32_t width, uint32_t v)
{
    struct schway__dma *d = &inst->dma;
    uint32_t ch = ((p >> 4) & 0xFu) - 8u;
    uint32_t reg = p & 0xCu;
    if ((p & 0xF0u) == 0xF0u) {
        if (reg == 0u) {
            d->dpcr = reg_put(d->dpcr, p, width, v);
        } else if (reg == 4u) {
            uint32_t m = lane_mask(width) << ((p & 3u) * 8u);
            uint32_t clear = (v << ((p & 3u) * 8u)) & m & DMA_DICR_FLAGS;
            uint32_t low = reg_put(d->dicr & DMA_DICR_WRITE_MASK, p, width, v) & DMA_DICR_WRITE_MASK;
            dicr_set(inst, low | ((d->dicr & DMA_DICR_FLAGS) & ~clear));
        }
    } else if (reg == 0u) {
        d->madr[ch] = reg_put(d->madr[ch], p, width, v) & SCHWAY__DMA_MADR_MASK;
    } else if (reg == 4u) {
        d->bcr[ch] = reg_put(d->bcr[ch], p, width, v);
    } else if (reg == 8u) {
        uint32_t c = reg_put(d->chcr[ch], p, width, v);
        c &= ch == DMA_OTC ? DMA_CHCR_OTC_MASK : SCHWAY__DMA_CHCR_MASK;
        if (d->cur == ch) {
            /* The unit's words have moved and it ends on its own: a write
               cannot stop it, so the channel stays busy. */
            c |= SCHWAY__DMA_CHCR_BUSY;
        }
        d->chcr[ch] = c;
    }
    if (d->cur == SCHWAY__DMA_NONE) {
        dma_start(inst, now);
    }
}

/* The register at p. The words at 1F8010F8h and 1F8010FCh and the fourth word
   of a channel read 0. OTC's CHCR always shows bit 1 (psx-spx, DMA Channels,
   CHCR); DICR shows the derived bit 31. */
static uint32_t dma_read(const schway *inst, uint32_t p)
{
    const struct schway__dma *d = &inst->dma;
    uint32_t ch = ((p >> 4) & 0xFu) - 8u;
    uint32_t reg = p & 0xCu;
    if ((p & 0xF0u) == 0xF0u) {
        if (reg == 0u) {
            return d->dpcr;
        }
        return reg == 4u ? d->dicr | (dicr_master(d->dicr) << 31) : 0u;
    }
    if (reg == 0u) {
        return d->madr[ch];
    }
    if (reg == 4u) {
        return d->bcr[ch];
    }
    if (reg == 8u) {
        return ch == DMA_OTC ? d->chcr[ch] | DMA_CHCR_OTC_FIXED : d->chcr[ch];
    }
    return 0u;
}

/* The registers 1F801080h-10FFh, at every width. The engine is first caught
   up to the access's own tick. A write then changes the registers and may
   start a unit; every write recomputes the scheduler's deadline. A read that
   finds a unit running has already held the CPU until its end (the bus told
   the engine), so the unit is finished as of its end tick before the read, and
   the program reads the completed state. */
void schway__dma_io(schway *inst, uint32_t wr, uint32_t p, uint32_t width, uint32_t *value,
                    uint32_t *cycles)
{
    struct schway__dma *d = &inst->dma;
    uint64_t now = schway__sched_now(inst, *cycles);
    schway__dma_event(inst, now);
    if (wr != 0u) {
        dma_write(inst, now, p, width, *value);
        schway__sched_recalc(inst);
        return;
    }
    if (d->cur != SCHWAY__DMA_NONE) {
        schway__dma_event(inst, d->busy_until);
        schway__sched_recalc(inst);
    }
    *value = reg_get(dma_read(inst, p), p, width);
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
