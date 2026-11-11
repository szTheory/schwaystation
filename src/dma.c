/* DMA: power-on values, the state walk, the register window, arbitration, the
   unit engine with the CPU hold, and the device ends. OTC, the burst and
   request-block modes and the linked list mode have units. */
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
   (docs/hardware/dma.md). */
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
    schway__state_u32_array(io, name_remain, d->remain, SCHWAY__DMA_CHANNELS, 0u,
                            SCHWAY__DMA_UNIT_WORDS_MAX);
    /* A count of the nodes a list has passed; it wraps, and any value is a
       state the engine can be in. */
    (void)schway__state_u32(io, name_nodes_done, &d->nodes_done, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_dreq, &d->dreq, 0u, (1u << SCHWAY__DMA_CHANNELS) - 1u);
    schway__state_u32_array(io, name_words, d->words, SCHWAY__DMA_CHANNELS, 0u, UINT32_MAX);
    schway__state_u32_array(io, name_crc, d->crc, SCHWAY__DMA_CHANNELS, 0u, UINT32_MAX);
}

/* The engine. One unit is one run of a channel that moves its words at once
   and keeps the channel busy for what the transfer costs; the event at the end
   of the busy time finishes it, and the next channel then starts. A unit is
   data-atomic and time-evented: a run to completion on a register write could
   hang on a 65,535-word burst and a per-word interleave would cost millions of
   events a second. A channel whose mode has no unit yet keeps its registers
   and waits (psx-spx, DMA Channels). */
#define DMA_OTC 6u
#define DMA_UNIT_CHANNELS 0x7Fu
/* phase of a channel: nothing running, its unit's words moved and the channel
   busy until busy_until, a sync mode 1 transfer between blocks (busy, no
   unit, remain blocks still to go, waiting for the next request), the CPU's
   window between two units of a list (busy, cur still the channel and
   busy_until the end of the window, no hold on the CPU), or that window over
   and the next node waiting for the arbitration. */
#define DMA_PH_IDLE 0u
#define DMA_PH_UNIT 1u
#define DMA_PH_BLOCKS 2u
#define DMA_PH_GAP 3u
#define DMA_PH_NEXT 4u
/* a unit in flight whose channel was told to stop: it ends with no flag and no
   interrupt */
#define DMA_PH_STOP 5u

/* A window of a chopped burst costs its words as any unit does plus this many
   cycles of setup, and is followed by a CPU window of CHCR's length, capped at
   32 cycles per word of the DMA window. Fitted to the 64 chopping rows of the
   public dma/chopping log, so derived: the model sits within 11 % of every row
   and within about 1 % of them on average, and that is the evidence it gives. */
#define DMA_WINDOW_OVERHEAD 6u

/* The cycles the header fetch of a list node and the setup of the next one
   cost on top of its words, and the cycles the CPU runs between two nodes.
   Fitted to the public dma/chain-looping log, in which a loop that takes
   16,040 cycles alone takes 25,632 next to a self-referencing list: derived,
   not documented, and a band rather than a measurement. */
#define DMA_NODE_OVERHEAD 20u
#define DMA_NODE_GAP 6u

/* Extra cycles a sync mode 1 block costs on top of its words: the request
   handshake and the setup of the next block. Fitted to the hardware log of
   the public dma/chopping test, so derived, not documented (the same fit as
   the window overhead of a chopped burst). */
#define DMA_BLOCK_OVERHEAD 11u

/* CHCR bit 0 is the direction (1 is RAM to the device), bit 1 the step (1 is
   backward), bit 28 starts a transfer without waiting for a request, bits 9-10
   are the sync mode (psx-spx, DMA Channels, CHCR). OTC keeps only bits 24, 28
   and 30, and bit 1 always reads as 1. */
#define DMA_CHCR_FROM_RAM 0x00000001u
#define DMA_CHCR_STEP_BACK 0x00000002u
#define DMA_CHCR_TRIGGER 0x10000000u
#define DMA_CHCR_CHOP 0x00000100u
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

/* A channel with a unit ready to run: bit 24 set, and bit 28 where the mode
   needs the start trigger. OTC always does; sync mode 0 does; sync mode 1
   waits for the device's request line; a linked list needs bit 24 alone, the
   device being taken to be ready (no device end holds a request line yet). A
   transfer whose CPU window is over is waiting for its next unit. The reserved
   mode has no unit, so it waits (psx-spx, DMA Channels, CHCR). */
static uint32_t dma_pending(const schway *inst, uint32_t ch)
{
    const struct schway__dma *d = &inst->dma;
    uint32_t c = d->chcr[ch];
    uint32_t sync = (c >> 9) & 3u;
    if ((c & SCHWAY__DMA_CHCR_BUSY) == 0u) {
        return 0u;
    }
    if (d->phase[ch] == DMA_PH_NEXT) {
        return ch != 2u || sync != 2u || schway__gpu_dma_ready(inst);
    }
    if (ch == DMA_OTC || sync == 0u) {
        return (c & DMA_CHCR_TRIGGER) != 0u ? 1u : 0u;
    }
    if (sync == 2u) {
        return ch != 2u || schway__gpu_dma_ready(inst);
    }
    return sync == 1u && ((d->dreq >> ch) & 1u) != 0u ? 1u : 0u;
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
        if ((mask & (1u << ch)) == 0u || (field & 8u) == 0u ||
            dma_pending(inst, ch) == 0u) {
            continue;
        }
        if ((field & 7u) <= best_pri) {
            best = ch;
            best_pri = field & 7u;
        }
    }
    return best;
}

/* Clocks per word on the device side (psx-spx, DMA Channels, DMA Transfer
   Rates). */
static uint32_t dma_word_cycles(uint32_t ch)
{
    switch (ch) {
    case 3u:
        /* The BIOS default of 24; most games write the delay setting that
           makes it 40. The CD-ROM device end chooses between them when it
           is built, so nothing here reads the delay register. */
        return 24u;
    case 4u:
        return 4u;
    case 5u:
        return 20u;
    default:
        return 1u; /* MDEC in and out, GPU and OTC */
    }
}

/* The cycles a unit of n words takes: each word on the device's rate, plus one
   cycle per 16 words for the DRAM row address (psx-spx, DMA Channels, DRAM
   Hyper Page mode). At one cycle per word this is ceil(n * 17 / 16). */
static uint64_t dma_cost(uint32_t ch, uint32_t n)
{
    return (uint64_t)n * dma_word_cycles(ch) + (n + 15u) / 16u;
}

/* The stand-in for a device that is not built: the sink counts the words and
   folds their bytes, little-endian, into a CRC-32 kept per channel in the
   state; the source supplies zeros (an unbuilt device reads 0). Both take no
   extra cycles. A device replaces the one case of its channel. */
static uint32_t dma_stub_sink(schway *inst, uint32_t ch, uint32_t word)
{
    struct schway__dma *d = &inst->dma;
    uint8_t b[4];
    for (uint32_t i = 0; i < 4u; i++) {
        b[i] = (uint8_t)(word >> (8u * i));
    }
    d->words[ch]++;
    d->crc[ch] = schway__crc32_update(d->crc[ch], b, sizeof b);
    return 0u;
}

static uint32_t dma_stub_source(uint32_t *word)
{
    *word = 0u;
    return 0u;
}

/* One word from RAM to the device of channel ch; returns the extra busy
   cycles. */
static uint32_t dma_dev_write(schway *inst, uint32_t ch, uint32_t word, uint64_t tick)
{
    switch (ch) {
    case 0u: /* MDEC in */
        return dma_stub_sink(inst, ch, word);
    case 1u: /* MDEC out */
        return dma_stub_sink(inst, ch, word);
    case 2u: /* GPU */
        schway__gpu_dma_write(inst, word, tick);
        return 0u;
    case 3u: /* CD-ROM */
        return dma_stub_sink(inst, ch, word);
    case 4u: /* SPU */
        return dma_stub_sink(inst, ch, word);
    case 5u: /* PIO */
        return dma_stub_sink(inst, ch, word);
    default:
        return 0u;
    }
}

/* One word from the device of channel ch to *word; returns the extra busy
   cycles. */
static uint32_t dma_dev_read(schway *inst, uint32_t ch, uint32_t *word, uint64_t tick)
{
    switch (ch) {
    case 0u: /* MDEC in */
        return dma_stub_source(word);
    case 1u: /* MDEC out */
        return dma_stub_source(word);
    case 2u: /* GPU */
        *word = schway__gpu_dma_read(inst, tick);
        return 0u;
    case 3u: /* CD-ROM */
        return dma_stub_source(word);
    case 4u: /* SPU */
        return dma_stub_source(word);
    case 5u: /* PIO */
        return dma_stub_source(word);
    default:
        *word = 0u;
        return 0u;
    }
}

/* Moves n words between RAM at MADR and the device of channel ch, in the
   direction and with the step CHCR gives, and leaves MADR after the last word.
   The address wraps at 24 bits (FFFFFCh to 0 and back); a word whose address
   is past the RAM window is dropped when it is written and reads 0. Returns
   the extra cycles the device asked for. */
static uint64_t dma_move(schway *inst, uint32_t ch, uint32_t n, uint64_t tick)
{
    struct schway__dma *d = &inst->dma;
    uint32_t addr = d->madr[ch] & 0xFFFFFCu;
    uint32_t step = (d->chcr[ch] & DMA_CHCR_STEP_BACK) != 0u ? 0xFFFFFFFCu : 4u;
    uint32_t from_ram = d->chcr[ch] & DMA_CHCR_FROM_RAM;
    uint64_t extra = 0u;
    for (uint32_t i = 0u; i < n; i++) {
        uint32_t word = 0u;
        if (from_ram != 0u) {
            (void)schway__bus_ram_word(inst, addr, &word, 0u);
            extra += dma_dev_write(inst, ch, word, tick);
        } else {
            extra += dma_dev_read(inst, ch, &word, tick);
            (void)schway__bus_ram_word(inst, addr, &word, 1u);
        }
        addr = (addr + step) & SCHWAY__DMA_MADR_MASK;
    }
    d->madr[ch] = addr;
    return extra;
}

/* One node of a linked list. MADR names the node's header word: its top byte
   is the count of words that follow it and its low 24 bits the address of the
   next header. The words go to the device, whatever CHCR's direction bit says
   (a list is a transfer to a device), MADR then holds the next address, and a
   next address with bit 23 set ends the list, so the end marker FFFFFFh is
   left in MADR (psx-spx, DMA Channels, Linked List). The header is not sent.
   Returns the cycles the node takes: the header fetch and setup, then its
   words. remain says whether a node follows: 1, or 0 for the end. */
static uint64_t dma_list_node(schway *inst, uint32_t ch, uint64_t tick)
{
    struct schway__dma *d = &inst->dma;
    uint32_t addr = d->madr[ch] & 0xFFFFFCu;
    uint32_t header = 0u;
    (void)schway__bus_ram_word(inst, addr, &header, 0u);
    uint32_t count = header >> 24;
    uint32_t next = header & SCHWAY__DMA_MADR_MASK;
    uint64_t extra = 0u;
    for (uint32_t i = 1u; i <= count; i++) {
        uint32_t word = 0u;
        (void)schway__bus_ram_word(inst, (addr + 4u * i) & 0xFFFFFCu, &word, 0u);
        extra += dma_dev_write(inst, ch, word, tick);
    }
    d->madr[ch] = next;
    d->remain[ch] = (next & 0x800000u) != 0u ? 0u : 1u;
    return DMA_NODE_OVERHEAD + dma_cost(ch, count) + extra;
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
    return dma_cost(DMA_OTC, n);
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
    uint64_t cost;
    uint32_t n = d->bcr[ch] & 0xFFFFu;
    n = n == 0u ? SCHWAY__DMA_UNIT_WORDS_MAX : n;
    d->chcr[ch] &= ~DMA_CHCR_TRIGGER;
    d->cur = ch;
    if (ch == DMA_OTC) {
        d->remain[ch] = 0u;
        cost = dma_otc_unit(inst);
    } else if (((d->chcr[ch] >> 9) & 3u) == 1u) {
        /* Sync mode 1: BCR's high half counts the blocks (0 meaning 10000h),
           counted down in remain; one block moves per request, and the block
           takes the request with it: the device raises the line again when
           it can take or give another. */
        if (d->phase[ch] != DMA_PH_BLOCKS) {
            uint32_t blocks = d->bcr[ch] >> 16;
            d->remain[ch] = blocks == 0u ? SCHWAY__DMA_UNIT_WORDS_MAX : blocks;
        }
        if (d->remain[ch] > 0u) {
            d->remain[ch]--;
        }
        d->dreq &= ~(UINT32_C(1) << ch);
        cost = dma_cost(ch, n) + DMA_BLOCK_OVERHEAD + dma_move(inst, ch, n, t);
    } else if (((d->chcr[ch] >> 9) & 3u) == 2u) {
        /* Linked list: one node per unit, counted from the start of the
           transfer. */
        if (d->phase[ch] != DMA_PH_NEXT) {
            d->nodes_done = 0u;
        }
        d->nodes_done++;
        cost = dma_list_node(inst, ch, t);
    } else if ((d->chcr[ch] & DMA_CHCR_CHOP) != 0u) {
        /* Sync mode 0 chopped: BCR's low half words go in windows of 2^n
           words, CHCR bits 16-18, with a CPU window after each but the last;
           remain counts the words still to move. */
        uint32_t window = UINT32_C(1) << ((d->chcr[ch] >> 16) & 7u);
        if (d->phase[ch] != DMA_PH_NEXT) {
            d->remain[ch] = n;
        }
        if (window > d->remain[ch]) {
            window = d->remain[ch];
        }
        d->remain[ch] -= window;
        cost = dma_cost(ch, window) + DMA_WINDOW_OVERHEAD + dma_move(inst, ch, window, t);
    } else {
        /* Sync mode 0: one burst of BCR's low half words. */
        d->remain[ch] = 0u;
        cost = dma_cost(ch, n) + dma_move(inst, ch, n, t);
    }
    d->phase[ch] = DMA_PH_UNIT;
    d->busy_until = t + cost;
}

/* The cycles the CPU runs after a unit of ch that has more to follow: the
   fixed window between list nodes, or a chopped burst's CPU window, 2^m cycles
   from CHCR bits 20-22 and at most 32 per word of the DMA window (the cap is a
   fit to the public dma/chopping log). */
static uint64_t dma_gap(const struct schway__dma *d, uint32_t ch)
{
    uint32_t c = d->chcr[ch];
    if (((c >> 9) & 3u) == 2u) {
        return DMA_NODE_GAP;
    }
    uint64_t cpu = UINT64_C(1) << ((c >> 20) & 7u);
    uint64_t cap = (UINT64_C(1) << ((c >> 16) & 7u)) * 32u;
    return cpu < cap ? cpu : cap;
}

/* The end of the running unit, which ended at tick t; the engine is being
   caught up to tick now. A unit of a channel that was told to stop ends it with
   no flag and no interrupt. A sync mode 1 block with blocks to go leaves the
   channel busy and waiting for the next request, and the CPU runs until it
   comes: no flag, no interrupt. A chopped burst or a list with more to go
   leaves the channel busy for the CPU's window, a gap in which the engine holds
   nothing; every 1000h nodes of a list that gap reaches past now, so the call
   that is catching up returns with the channel still busy and the next node's
   start after now: a yield, not an end (psx-spx, DMA Channels, Linked List).
   Otherwise the channel stops being busy, its DICR flag is set when both its
   enable and the master enable allow it, and the interrupt edge follows
   (psx-spx, DMA Channels, DICR). */
static void dma_finish(schway *inst, uint32_t ch, uint64_t t, uint64_t now)
{
    struct schway__dma *d = &inst->dma;
    uint32_t dicr = d->dicr;
    if (d->phase[ch] == DMA_PH_STOP) {
        d->chcr[ch] &= ~SCHWAY__DMA_CHCR_BUSY;
        d->phase[ch] = DMA_PH_IDLE;
        d->remain[ch] = 0u;
        d->cur = SCHWAY__DMA_NONE;
        return;
    }
    if (d->remain[ch] > 0u) {
        if (((d->chcr[ch] >> 9) & 3u) == 1u) {
            d->phase[ch] = DMA_PH_BLOCKS;
            d->cur = SCHWAY__DMA_NONE;
        } else {
            uint64_t until = t + dma_gap(d, ch);
            if (((d->chcr[ch] >> 9) & 3u) == 2u &&
                d->nodes_done % SCHWAY__DMA_NODES_MAX == 0u && until <= now) {
                until = now + 1u;
            }
            d->phase[ch] = DMA_PH_GAP;
            d->busy_until = until;
        }
        return;
    }
    d->chcr[ch] &= ~SCHWAY__DMA_CHCR_BUSY;
    d->phase[ch] = DMA_PH_IDLE;
    d->cur = SCHWAY__DMA_NONE;
    if (((dicr >> (16u + ch)) & 1u) != 0u && (dicr & DMA_DICR_MASTER_EN) != 0u) {
        dicr |= UINT32_C(1) << (24u + ch);
    }
    dicr_set(inst, dicr);
}

/* With a unit running, its end. With none running and a channel waiting, the
   tick that channel starts at, which is the end of the unit before it (only a
   register read leaves this state: it finishes a running unit ahead of the
   clock, and the next one must not start before the clock gets there). */
uint64_t schway__dma_next(const schway *inst)
{
    if (inst->dma.cur == SCHWAY__DMA_NONE &&
        schway__dma_arbitrate(inst, DMA_UNIT_CHANNELS) == SCHWAY__DMA_NONE) {
        return UINT64_MAX;
    }
    return inst->dma.busy_until;
}

/* Runs what is due at tick now, in order: a unit whose busy_until is not past
   now finishes at that tick, and the channel that goes next starts at that
   same tick and may finish too, until none is due. With start_next 0 the loop
   stops after the finish and the channel that goes next stays waiting; it
   starts at busy_until, the finished unit's end, in a later call that is due
   by then. Each finish clears a channel's bit 24, so the loop ends. */
static void dma_run(schway *inst, uint64_t now, uint32_t start_next)
{
    struct schway__dma *d = &inst->dma;
    for (;;) {
        if (d->cur == SCHWAY__DMA_NONE) {
            if (start_next == 0u || d->busy_until > now ||
                schway__dma_arbitrate(inst, DMA_UNIT_CHANNELS) == SCHWAY__DMA_NONE) {
                return;
            }
            dma_start(inst, d->busy_until);
            continue;
        }
        if (d->busy_until > now) {
            return;
        }
        uint64_t t = d->busy_until;
        uint32_t ch = d->cur;
        if (d->phase[ch] == DMA_PH_GAP) {
            /* The CPU's window is over: the channel's next unit waits to be
               picked, with the others, by the arbitration. */
            d->phase[ch] = DMA_PH_NEXT;
            d->cur = SCHWAY__DMA_NONE;
        } else {
            dma_finish(inst, ch, t, now);
        }
    }
}

/* Every return leaves schway__dma_next strictly greater than now (a unit
   still running ends after now; with none running and none waiting it is
   UINT64_MAX), so the scheduler always makes progress. A unit that passes 1000h
   list nodes yields in the same way: it returns with the channel still busy and
   no flag or interrupt. */
void schway__dma_event(schway *inst, uint64_t now)
{
    dma_run(inst, now, 1u);
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
    if (d->cur == SCHWAY__DMA_NONE || now >= d->busy_until || d->phase[d->cur] == DMA_PH_GAP) {
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
        if (ch != DMA_OTC && d->phase[ch] == DMA_PH_IDLE && (c & SCHWAY__DMA_CHCR_BUSY) != 0u &&
            ((c >> 9) & 3u) == 3u) {
            /* Synchronisation mode 3 is reserved: no device end answers it, so a
               channel written with it and the start bit would wait for ever. It
               does not start (the start bits are dropped) and the first fault
               names the register, the value and the writing instruction. A
               later write with a usable mode starts the channel as usual. */
            c &= ~(SCHWAY__DMA_CHCR_BUSY | DMA_CHCR_TRIGGER);
            schway__fault_latch(inst, SCHWAY__FAULT_DMA, SCHWAY__DEV_DMA, width, inst->cpu.pc, v,
                                p & ~3u);
        }
        if (d->phase[ch] != DMA_PH_IDLE) {
            if (width == 4u && v == 0u && d->phase[ch] != DMA_PH_UNIT &&
                d->phase[ch] != DMA_PH_STOP) {
                /* The word 0 stops a channel with no unit in flight at once:
                   a CPU window, a wait for the next node or block. This is
                   the write the public chain-looping test makes between its
                   cases; no other write to a busy channel is modelled, and
                   each leaves it busy. */
                d->phase[ch] = DMA_PH_IDLE;
                d->remain[ch] = 0u;
                if (d->cur == ch) {
                    d->cur = SCHWAY__DMA_NONE;
                }
            } else {
                /* The unit's words have moved and it ends on its own: a write
                   cannot end it early, so the channel stays busy. The word 0
                   marks it to end with no flag and no interrupt. */
                if (width == 4u && v == 0u) {
                    d->phase[ch] = DMA_PH_STOP;
                }
                c |= SCHWAY__DMA_CHCR_BUSY;
            }
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
   the program reads the completed state. The next channel does not start
   yet: its unit would end a whole unit past a clock that has not moved, beyond
   what a state may hold, so it starts at the finished unit's end tick, when
   the clock gets there. */
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
    if (d->cur != SCHWAY__DMA_NONE && d->phase[d->cur] != DMA_PH_GAP) {
        dma_run(inst, d->busy_until, 0u);
        schway__sched_recalc(inst);
    }
    *value = reg_get(dma_read(inst, p), p, width);
}

/* The levels are latched, a bit per channel. A raised line starts the channel
   when it is waiting for it and nothing is running; with a unit running, the
   next unit boundary picks it up. */
void schway__dma_dreq(schway *inst, uint32_t ch, uint32_t level)
{
    if (ch >= SCHWAY__DMA_CHANNELS) {
        return;
    }
    if (level != 0u) {
        inst->dma.dreq |= 1u << ch;
        if (inst->dma.cur == SCHWAY__DMA_NONE) {
            dma_start(inst, schway__sched_now(inst, 0u));
            schway__sched_recalc(inst);
        }
    } else {
        inst->dma.dreq &= ~(1u << ch);
    }
}
