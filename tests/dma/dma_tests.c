/* The DMA registers, arbitration, the unit engine, OTC and the CPU hold:
   schwaystation-test-dma <case>. White-box: the test sees src/, so it drives
   the registers through the bus, reads the engine's state and builds the
   few programs that need a CPU by hand. Hardware facts are the psx-spx DMA
   Channels page and the public ps1-tests dma/otc-test and nugget dma tests
   (pins in docs/hardware/dma.md); the cycle counts follow the model that page
   describes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

#define TEXT 0x80010000u

#define MADR(ch) (0x1F801080u + 0x10u * (ch))
#define BCR(ch) (MADR(ch) + 4u)
#define CHCR(ch) (MADR(ch) + 8u)
#define DPCR 0x1F8010F0u
#define DICR 0x1F8010F4u
#define ISTAT 0x1F801070u

/* DPCR with every channel's master enable set and the default priorities
   1 to 7 for channels 0 to 6 (reset value 07654321h plus the enables), and
   with only OTC enabled. */
#define DPCR_ALL 0x0FEDCBA9u
#define DPCR_OTC 0x0F654321u

#define OTC_TOP 0x001FFFFCu

static void *test_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void test_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static schway *make(void)
{
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) {
        exit(1);
    }
    return inst;
}

/* One access through the bus at the instance's current tick. */
static void wr32(schway *inst, uint32_t addr, uint32_t v)
{
    uint32_t cycles = 0u;
    schway__sched_sync(inst);
    CHECK_EQ_U64(schway__bus_write(inst, addr, 4u, v, &cycles), 0u);
}

static void wr16(schway *inst, uint32_t addr, uint16_t v)
{
    uint32_t cycles = 0u;
    schway__sched_sync(inst);
    CHECK_EQ_U64(schway__bus_write(inst, addr, 2u, v, &cycles), 0u);
}

static void wr16_at(schway *inst, uint32_t addr, uint16_t v, uint64_t tick)
{
    uint64_t base = schway__sched_now(inst, 0u);
    CHECK(tick >= base && tick - base < UINT32_MAX);
    uint64_t delta = tick - base;
    uint32_t cycles = delta == 0u ? 0u : (uint32_t)(delta + 1u);
    CHECK_EQ_U64(schway__bus_write(inst, addr, 2u, v, &cycles), 0u);
}

static uint32_t rd32(schway *inst, uint32_t addr)
{
    uint32_t cycles = 0u;
    uint32_t v = 0u;
    schway__sched_sync(inst);
    CHECK_EQ_U64(schway__bus_read(inst, addr, 4u, &v, &cycles), 0u);
    return v;
}

/* The tick an access made now would carry. */
static uint64_t now_of(const schway *inst)
{
    return schway__sched_now(inst, 0u);
}

static uint32_t ram_word(const schway *inst, uint32_t addr)
{
    uint32_t v = 0u;
    CHECK_EQ_U64(schway_peek(inst, SCHWAY_SPACE_RAM, addr, 4u, &v), SCHWAY_STATUS_OK);
    return v;
}

static schway_run_result run(schway *inst, uint64_t cycles)
{
    schway_budget b;
    memset(&b, 0, sizeof b);
    b.size = (uint32_t)sizeof b;
    b.max_cycles = cycles;
    schway_run_result r;
    memset(&r, 0, sizeof r);
    r.size = (uint32_t)sizeof r;
    CHECK_EQ_U64(schway_run(inst, &b, &r), SCHWAY_STATUS_OK);
    return r;
}

/* Drive the SPU sample event before the DMA event at a shared request
   deadline, matching scheduler order for one DMA4 request block. */
static void spu_dma_event(schway *inst, uint64_t tick)
{
    if (tick > inst->ticks) {
        inst->ticks = tick;
        inst->cpu.blk_tick = tick;
        inst->cpu.blk_cycle = inst->cpu.cycle;
    }
    schway__spu_event(inst, tick);
    schway__dma_event(inst, tick);
}

/* Advance a bounded number of DMA deadlines until the expected word count is
   reached. Each block uses its measured completion cost and waits for the
   next SPU sample deadline before the following request. */
static void spu_dma_until_words(schway *inst, uint64_t words)
{
    for (uint32_t i = 0u; i < 10000u && inst->dma.words[4] < words; i++) {
        uint64_t tick = inst->dma.busy_until;
        if (tick == UINT64_MAX) {
            break;
        }
        spu_dma_event(inst, tick);
    }
    CHECK_EQ_U64(inst->dma.words[4], words);
}

/* Starts OTC on an instance with DPCR and DICR enables set: nwords words from
   OTC_TOP downward, CHCR as given. Returns the tick the CHCR write carried. */
static uint64_t otc_start(schway *inst, uint32_t madr, uint32_t nwords, uint32_t chcr)
{
    wr32(inst, DPCR, DPCR_OTC);
    wr32(inst, DICR, 0x00C00000u);
    wr32(inst, MADR(6u), madr);
    wr32(inst, BCR(6u), nwords);
    uint64_t start = now_of(inst);
    wr32(inst, CHCR(6u), chcr);
    return start;
}

/* Checks the ordering table of n words from top: each word holds the address
   of the one below it and the last holds FFFFFFh. */
static void check_table(const schway *inst, uint32_t top, uint32_t n)
{
    uint32_t bad = 0u;
    for (uint32_t i = 0u; i < n; i++) {
        uint32_t a = top - 4u * i;
        uint32_t want = i + 1u == n ? 0xFFFFFFu : a - 4u;
        if (ram_word(inst, a) != want) {
            bad++;
        }
    }
    CHECK_EQ_U64(bad, 0u);
}

/* A unit in flight on channel ch, ending at tick until, as the engine would
   have started it. The words are not moved: this is the engine's state, for
   the cases that need a completion on a channel with no unit of its own yet. */
static void fake_unit(schway *inst, uint32_t ch, uint64_t until)
{
    inst->dma.chcr[ch] = SCHWAY__DMA_CHCR_BUSY;
    inst->dma.cur = ch;
    inst->dma.phase[ch] = 1u;
    inst->dma.busy_until = until;
}

static void finish_fake(schway *inst, uint32_t ch)
{
    uint64_t until = now_of(inst) + 10u;
    fake_unit(inst, ch, until);
    schway__dma_event(inst, until);
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
}

/* Fault prevented: an OTC that writes the wrong table, reads back the wrong
   register, forgets the flag or the interrupt, or finishes at another tick
   than 4,096 words on the 17 per 16 row give. */
static void case_otc_clear(void)
{
    schway *inst = make();
    uint64_t start = otc_start(inst, 0x801FFFFCu, 4096u, 0x11000000u);
    CHECK_EQ_U64(inst->dma.cur, 6u);
    CHECK_EQ_U64(inst->dma.busy_until, start + 4352u);
    CHECK_EQ_U64(inst->sched.next, start + 4352u);
    check_table(inst, OTC_TOP, 4096u);
    CHECK_EQ_HEX(ram_word(inst, OTC_TOP - 4u * 4095u), 0xFFFFFFu);
    (void)run(inst, 4351u);
    CHECK((inst->dma.chcr[6] & SCHWAY__DMA_CHCR_BUSY) != 0u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 0u);
    (void)run(inst, 2u);
    CHECK_EQ_U64(inst->dma.chcr[6] & SCHWAY__DMA_CHCR_BUSY, 0u);
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    CHECK_EQ_U64(inst->sched.next, UINT64_MAX);
    CHECK_EQ_U64(inst->irq.stat & 8u, 8u);
    CHECK_EQ_HEX(rd32(inst, CHCR(6u)), 0x00000002u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0xC0C00000u);
    schway_destroy(inst);
}

/* Fault prevented: DPCR not at 07654321h after a power reset, or a reset that
   keeps what the program wrote. */
static void case_dpcr_reset(void)
{
    schway *inst = make();
    CHECK_EQ_HEX(rd32(inst, DPCR), 0x07654321u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0u);
    wr32(inst, DPCR, 0u);
    wr32(inst, DICR, 0x00FF803Fu);
    wr32(inst, MADR(2u), 0x00123456u);
    wr32(inst, CHCR(2u), 0x01000401u);
    CHECK_EQ_HEX(rd32(inst, DPCR), 0u);
    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    CHECK_EQ_HEX(rd32(inst, DPCR), 0x07654321u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0u);
    CHECK_EQ_HEX(rd32(inst, MADR(2u)), 0u);
    CHECK_EQ_HEX(rd32(inst, CHCR(2u)), 0u);
    schway_destroy(inst);
}

/* Fault prevented: arbitration that picks by channel number alone, lets the
   CPU field decide, ignores the master enables or breaks a tie the wrong way.
   Channels 2 and 5 wait with sync mode 0 and the start trigger; a unit that
   is already running keeps the engine from starting either, so the pick is
   the arbitration alone. */
static void case_dpcr_priority(void)
{
    schway *inst = make();
    fake_unit(inst, 6u, UINT64_MAX);
    wr32(inst, CHCR(2u), 0x11000000u);
    wr32(inst, CHCR(5u), 0x11000000u);
    const uint32_t all = 0x7Fu;
    /* Nothing is enabled at reset. */
    CHECK_EQ_U64(schway__dma_arbitrate(inst, all), SCHWAY__DMA_NONE);
    /* Priorities 3 and 6: the lower value wins. */
    wr32(inst, DPCR, DPCR_ALL);
    CHECK_EQ_U64(schway__dma_arbitrate(inst, all), 2u);
    /* Channel 5 at priority 0 beats channel 2 at 3. */
    wr32(inst, DPCR, (DPCR_ALL & ~0x00F00000u) | 0x00800000u);
    CHECK_EQ_U64(schway__dma_arbitrate(inst, all), 5u);
    /* Equal priorities: the higher channel number wins. */
    wr32(inst, DPCR, (DPCR_ALL & ~0x00F00F00u) | 0x00B00B00u);
    CHECK_EQ_U64(schway__dma_arbitrate(inst, all), 5u);
    /* The CPU field, bits 28-31, changes nothing. */
    wr32(inst, DPCR, (DPCR_ALL & 0x0FFFFFFFu) | 0xF0000000u);
    CHECK_EQ_U64(schway__dma_arbitrate(inst, all), 2u);
    wr32(inst, DPCR, DPCR_ALL & 0x0FFFFFFFu);
    CHECK_EQ_U64(schway__dma_arbitrate(inst, all), 2u);
    /* A channel with its enable clear does not compete. */
    wr32(inst, DPCR, (DPCR_ALL & ~0x00800000u) | 0x00000000u);
    CHECK_EQ_U64(schway__dma_arbitrate(inst, all), 2u);
    wr32(inst, DPCR, DPCR_ALL & ~0x00000800u);
    CHECK_EQ_U64(schway__dma_arbitrate(inst, all), 5u);
    /* The mask limits the candidates. */
    wr32(inst, DPCR, DPCR_ALL);
    CHECK_EQ_U64(schway__dma_arbitrate(inst, 1u << 5), 5u);
    CHECK_EQ_U64(schway__dma_arbitrate(inst, 1u << 3), SCHWAY__DMA_NONE);
    /* Without bit 24 a channel is not waiting. */
    wr32(inst, CHCR(2u), 0x10000000u);
    CHECK_EQ_U64(schway__dma_arbitrate(inst, 1u << 2), SCHWAY__DMA_NONE);
    schway_destroy(inst);
}

/* Fault prevented: a channel that starts with its master enable clear, or
   one that waits forever after the enable is set. A DPCR write starts a
   waiting channel, the way the public tests start one. */
static void case_dpcr_enable(void)
{
    schway *inst = make();
    wr32(inst, MADR(6u), OTC_TOP);
    wr32(inst, BCR(6u), 16u);
    wr32(inst, CHCR(6u), 0x11000000u);
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    CHECK_EQ_U64(ram_word(inst, OTC_TOP), 0u);
    CHECK_EQ_HEX(rd32(inst, CHCR(6u)), 0x11000002u);
    (void)run(inst, 100u);
    CHECK_EQ_U64(ram_word(inst, OTC_TOP), 0u);
    uint64_t at = now_of(inst);
    wr32(inst, DPCR, DPCR_OTC);
    CHECK_EQ_U64(inst->dma.cur, 6u);
    CHECK_EQ_U64(inst->dma.busy_until, at + 17u);
    check_table(inst, OTC_TOP, 16u);
    (void)run(inst, 100u);
    CHECK_EQ_HEX(rd32(inst, CHCR(6u)), 0x00000002u);
    schway_destroy(inst);
}

/* Fault prevented: DICR values that differ from the console's, with the
   nugget sequences pinned (84840000h, 84800000h, 04000000h, 84008000h) on
   channel 2's flag, completed through the engine's own finish. */
static void case_dicr_reads(void)
{
    schway *inst = make();
    wr32(inst, DICR, 0x00840000u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x00840000u);
    finish_fake(inst, 2u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x84840000u);
    /* Writing 1 to a flag clears it, and bit 31 follows. */
    uint32_t d = rd32(inst, DICR);
    wr32(inst, DICR, (d & ~0x7F000000u) | 0x04000000u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x00840000u);
    finish_fake(inst, 2u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x84840000u);
    /* Writing 0 to the flags keeps them; the enables are overwritten. */
    wr32(inst, DICR, 0x00800000u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x84800000u);
    /* Bit 23 clear: the flag stays and bit 31 goes. */
    wr32(inst, DICR, 0u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x04000000u);
    wr32(inst, DICR, 0x00800000u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x84800000u);
    /* Bit 15 forces bit 31 whatever the master enable says. */
    wr32(inst, DICR, 0x00008000u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x84008000u);
    wr32(inst, DICR, 0x00800000u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x84800000u);
    /* A byte write to the flag byte clears the flag alone. */
    {
        uint32_t cycles = 0u;
        CHECK_EQ_U64(schway__bus_write(inst, DICR + 3u, 1u, 0x04u, &cycles), 0u);
    }
    CHECK_EQ_HEX(rd32(inst, DICR), 0x00800000u);
    /* Only bits 0-5, 15 and 16-23 stick. */
    wr32(inst, DICR, 0x00FFFFFFu);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x80FF803Fu);
    /* A flag is set only when its enable and the master enable allow it. */
    wr32(inst, DICR, 0x00000000u);
    wr32(inst, DICR, 0x7F000000u);
    finish_fake(inst, 4u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0u);
    wr32(inst, DICR, 0x00100000u);
    finish_fake(inst, 4u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x00100000u);
    /* The master enable alone, with the channel's enable clear, sets none. */
    wr32(inst, DICR, 0x00800000u);
    finish_fake(inst, 4u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x00800000u);
    wr32(inst, DICR, 0x00900000u);
    finish_fake(inst, 4u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x90900000u);
    /* The words after DICR read 0. */
    CHECK_EQ_HEX(rd32(inst, 0x1F8010F8u), 0u);
    CHECK_EQ_HEX(rd32(inst, 0x1F8010FCu), 0u);
    schway_destroy(inst);
}

/* Fault prevented: an interrupt raised on every completion, on a write that
   keeps bit 31 at 1, or never after an acknowledgement. */
static void case_dicr_irq_edge(void)
{
    schway *inst = make();
    wr32(inst, DICR, 0x00940000u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 0u);
    finish_fake(inst, 2u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 8u);
    wr32(inst, ISTAT, 0u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 0u);
    /* A second flag while bit 31 is 1 is no new edge. */
    finish_fake(inst, 4u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x94940000u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 0u);
    /* A write that keeps bit 31 at 1 is none either. */
    wr32(inst, DICR, 0x00940000u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 0u);
    /* Clearing every flag takes bit 31 down with no interrupt. */
    wr32(inst, DICR, 0x7F940000u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x00940000u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 0u);
    /* A new completion is a new edge. */
    finish_fake(inst, 2u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 8u);
    wr32(inst, ISTAT, 0u);
    /* Bit 15 while bit 31 is already 1 is no edge. */
    wr32(inst, DICR, 0x00948000u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 0u);
    wr32(inst, DICR, 0x7F948000u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x80948000u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 0u);
    /* Bit 31 down, then up through bit 15 alone. */
    wr32(inst, DICR, 0x00940000u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x00940000u);
    wr32(inst, DICR, 0x00948000u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x80948000u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 8u);
    schway_destroy(inst);
}

/* Fault prevented: OTC's CHCR reading back anything but its three writable
   bits and the fixed bit 1. The writes are the public ps1-tests dma/otc-test
   values, each with bit 24 clear so nothing runs. */
static void case_otc_readback(void)
{
    schway *inst = make();
    CHECK_EQ_HEX(rd32(inst, CHCR(6u)), 0x00000002u);
    wr32(inst, CHCR(6u), 0x70770703u);
    CHECK_EQ_HEX(rd32(inst, CHCR(6u)), 0x50000002u);
    wr32(inst, CHCR(6u), 0x00000000u);
    CHECK_EQ_HEX(rd32(inst, CHCR(6u)), 0x00000002u);
    wr32(inst, CHCR(6u), 0x8E88F8FCu);
    CHECK_EQ_HEX(rd32(inst, CHCR(6u)), 0x00000002u);
    /* After a transfer the register is the fixed bit alone. */
    wr32(inst, DPCR, DPCR_OTC);
    wr32(inst, MADR(6u), OTC_TOP);
    wr32(inst, BCR(6u), 4u);
    wr32(inst, CHCR(6u), 0x11000000u);
    CHECK_EQ_HEX(rd32(inst, CHCR(6u)), 0x00000002u);
    /* A write of bits 28 and 30 without bit 24 stays as written. */
    wr32(inst, CHCR(6u), 0x50000000u);
    CHECK_EQ_HEX(rd32(inst, CHCR(6u)), 0x50000002u);
    schway_destroy(inst);
}

/* Fault prevented: a table that wraps wrongly at the bottom of the address
   space, a word past the RAM window that lands in RAM, a first fault from
   it. Words from 4 down: 4 and 0 are RAM, FFFFFCh and FFFFF8h are past the
   window and are dropped. */
static void case_otc_madr_wrap(void)
{
    schway *inst = make();
    uint8_t *before = (uint8_t *)malloc(SCHWAY__RAM_SIZE);
    if (before == NULL) {
        exit(1);
    }
    memcpy(before, inst->bus.ram, SCHWAY__RAM_SIZE);
    uint64_t start = otc_start(inst, 0x00000004u, 4u, 0x11000000u);
    /* Four words cost ceil(4 * 17 / 16) = 5 cycles. */
    CHECK_EQ_U64(inst->dma.busy_until - start, 5u);
    CHECK_EQ_HEX(ram_word(inst, 4u), 0x00000000u);
    CHECK_EQ_HEX(ram_word(inst, 0u), 0x00FFFFFCu);
    uint32_t differ = 0u;
    for (uint32_t i = 8u; i < SCHWAY__RAM_SIZE; i++) {
        differ += inst->bus.ram[i] != before[i] ? 1u : 0u;
    }
    CHECK_EQ_U64(differ, 0u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    (void)run(inst, 100u);
    CHECK_EQ_HEX(rd32(inst, CHCR(6u)), 0x00000002u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    free(before);
    schway_destroy(inst);
}

/* Fault prevented: OTC starting on bit 24 alone, or on bit 28 alone. */
static void case_otc_needs_trigger(void)
{
    schway *inst = make();
    wr32(inst, DPCR, DPCR_OTC);
    wr32(inst, MADR(6u), OTC_TOP);
    wr32(inst, BCR(6u), 4u);
    wr32(inst, CHCR(6u), 0x01000000u);
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    CHECK_EQ_U64(ram_word(inst, OTC_TOP), 0u);
    CHECK_EQ_HEX(rd32(inst, CHCR(6u)), 0x01000002u);
    wr32(inst, CHCR(6u), 0x10000000u);
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    CHECK_EQ_U64(ram_word(inst, OTC_TOP), 0u);
    CHECK_EQ_HEX(rd32(inst, CHCR(6u)), 0x10000002u);
    wr32(inst, CHCR(6u), 0x11000000u);
    CHECK_EQ_U64(inst->dma.cur, 6u);
    check_table(inst, OTC_TOP, 4u);
    schway_destroy(inst);
}

/* Fault prevented: OTC honouring the direction, step, sync or chopping bits.
   The same transfer with every one of them set builds the same table, ends at
   the same tick and reads back the same. */
static void case_otc_ignores_bits(void)
{
    schway *plain = make();
    schway *noisy = make();
    uint64_t t0 = otc_start(plain, OTC_TOP, 4096u, 0x11000000u);
    uint64_t t1 = otc_start(noisy, OTC_TOP, 4096u, 0x11770503u);
    CHECK_EQ_U64(plain->dma.cur, 6u);
    CHECK_EQ_U64(noisy->dma.cur, 6u);
    CHECK_EQ_U64(plain->dma.busy_until - t0, 4352u);
    CHECK_EQ_U64(noisy->dma.busy_until - t1, 4352u);
    check_table(noisy, OTC_TOP, 4096u);
    CHECK_EQ_U64(memcmp(plain->bus.ram, noisy->bus.ram, SCHWAY__RAM_SIZE), 0);
    (void)run(plain, 5000u);
    (void)run(noisy, 5000u);
    CHECK_EQ_HEX(rd32(plain, CHCR(6u)), 0x00000002u);
    CHECK_EQ_HEX(rd32(noisy, CHCR(6u)), 0x00000002u);
    CHECK_EQ_HEX(rd32(noisy, DICR), rd32(plain, DICR));
    schway_destroy(plain);
    schway_destroy(noisy);
}

/* Fault prevented: a word count of 0 moving nothing, or the wrong cost for
   the longest transfer: 10000h words take ceil(10000h * 17 / 16) = 69,632
   cycles. */
static void case_otc_bcr_zero(void)
{
    schway *inst = make();
    uint64_t start = otc_start(inst, OTC_TOP, 0u, 0x11000000u);
    CHECK_EQ_U64(inst->dma.busy_until - start, 69632u);
    check_table(inst, OTC_TOP, 0x10000u);
    CHECK_EQ_HEX(ram_word(inst, OTC_TOP - 4u * 0xFFFFu), 0xFFFFFFu);
    (void)run(inst, 70000u);
    CHECK_EQ_HEX(rd32(inst, CHCR(6u)), 0x00000002u);
    schway_destroy(inst);
}

/* Fault prevented: an event call that finishes a unit early, one that leaves
   its deadline at or before the tick it was called with (the scheduler would
   not advance), or one that leaves a finished unit's deadline set. */
static void case_step_contract(void)
{
    schway *inst = make();
    (void)otc_start(inst, OTC_TOP, 4096u, 0x11000000u);
    uint64_t end = inst->dma.busy_until;
    /* A CHCR write cannot stop the unit that is running: the words have
       moved and the channel stays busy to its end. */
    wr32(inst, CHCR(6u), 0u);
    CHECK_EQ_U64(inst->dma.cur, 6u);
    CHECK_EQ_U64(inst->dma.busy_until, end);
    CHECK((inst->dma.chcr[6] & SCHWAY__DMA_CHCR_BUSY) != 0u);
    schway__dma_event(inst, end - 1u);
    CHECK_EQ_U64(inst->dma.cur, 6u);
    CHECK_EQ_U64(schway__dma_next(inst), end);
    CHECK(schway__dma_next(inst) > end - 1u);
    CHECK((inst->dma.chcr[6] & SCHWAY__DMA_CHCR_BUSY) != 0u);
    schway__dma_event(inst, end + 5u);
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    CHECK_EQ_U64(schway__dma_next(inst), UINT64_MAX);
    CHECK_EQ_U64(inst->dma.chcr[6] & SCHWAY__DMA_CHCR_BUSY, 0u);
    /* A call with nothing running changes nothing. */
    schway__dma_event(inst, end + 100u);
    CHECK_EQ_U64(schway__dma_next(inst), UINT64_MAX);
    /* A call exactly at the end finishes it. */
    schway *at = make();
    (void)otc_start(at, OTC_TOP, 16u, 0x11000000u);
    schway__dma_event(at, at->dma.busy_until);
    CHECK_EQ_U64(at->dma.cur, SCHWAY__DMA_NONE);
    schway_destroy(at);
    schway_destroy(inst);
}

/* ---- programs ---- */

#define T0 8u
#define T1 9u
#define T2 10u
#define T3 11u
#define T4 12u
#define T5 13u
#define T6 14u
#define T9 25u

#define I_LUI(rt, imm) (0x3C000000u | ((rt) << 16) | ((uint32_t)(imm) & 0xFFFFu))
#define I_ORI(rt, rs, imm) (0x34000000u | ((rs) << 21) | ((rt) << 16) | ((uint32_t)(imm) & 0xFFFFu))
#define I_ADDIU(rt, rs, imm)                                                                       \
    (0x24000000u | ((rs) << 21) | ((rt) << 16) | ((uint32_t)(imm) & 0xFFFFu))
#define I_SW(rt, off, rs) (0xAC000000u | ((rs) << 21) | ((rt) << 16) | ((uint32_t)(off) & 0xFFFFu))
#define I_LW(rt, off, rs) (0x8C000000u | ((rs) << 21) | ((rt) << 16) | ((uint32_t)(off) & 0xFFFFu))
#define I_BEQ(rs, rt, off) (0x10000000u | ((rs) << 21) | ((rt) << 16) | ((uint32_t)(off) & 0xFFFFu))
#define I_J(target) (0x08000000u | (((uint32_t)(target) >> 2) & 0x03FFFFFFu))
#define I_NOP 0u

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

/* A PS-EXE of one text sector at 80010000h holding the n words, entered at its
   first word. */
static void load(schway *inst, const uint32_t *words, uint32_t n)
{
    static uint8_t exe[0x1000];
    memset(exe, 0, sizeof exe);
    memcpy(exe, "PS-X EXE", 8);
    put32(exe + 0x10, TEXT);
    put32(exe + 0x18, TEXT);
    put32(exe + 0x1C, 0x800u);
    for (uint32_t i = 0; i < n; i++) {
        put32(exe + 0x800u + 4u * i, words[i]);
    }
    CHECK_EQ_U64(schway_load_exe(inst, exe, sizeof exe), SCHWAY_STATUS_OK);
}

/* Runs the stub kernel's boot until the program's first instruction is next. */
static void run_to_entry(schway *inst)
{
    for (uint32_t i = 0; i < 4000u && inst->cpu.pc != TEXT; i++) {
        (void)run(inst, 1u);
    }
    CHECK_EQ_HEX(inst->cpu.pc, TEXT);
}

/* The first 12 words of a program that points t0 at the I/O page, sets DPCR to
   dpcr, MADR and BCR of OTC to the top of RAM and 4,096 words, and leaves the
   start value in t4 and a RAM address in t6. The next word, number 12, starts
   a line of the instruction cache, so the words after the CHCR store are
   already cached when it runs. */
static uint32_t put_setup(uint32_t *w, uint32_t dpcr)
{
    uint32_t n = 0u;
    w[n++] = I_LUI(T0, 0x1F80u);
    w[n++] = I_ORI(T0, T0, 0x1000u);
    w[n++] = I_LUI(T1, dpcr >> 16);
    w[n++] = I_ORI(T1, T1, dpcr & 0xFFFFu);
    w[n++] = I_SW(T1, 0xF0u, T0);
    w[n++] = I_LUI(T2, 0x001Fu);
    w[n++] = I_ORI(T2, T2, 0xFFFCu);
    w[n++] = I_SW(T2, 0xE0u, T0);
    w[n++] = I_ORI(T3, 0u, 0x1000u);
    w[n++] = I_SW(T3, 0xE4u, T0);
    w[n++] = I_LUI(T4, 0x1100u);
    w[n++] = I_LUI(T6, 0x8000u);
    return n;
}

/* Runs one cycle at a time until the unit has started and returns its end
   tick, or 0 when it never starts. */
static uint64_t step_to_unit(schway *inst)
{
    for (uint32_t i = 0; i < 4000u; i++) {
        if (inst->dma.cur != SCHWAY__DMA_NONE) {
            return inst->dma.busy_until;
        }
        (void)run(inst, 1u);
    }
    return 0u;
}

/* Runs the program that starts OTC, then does one RAM read, then sets t9; the
   marker's tick is returned, and the unit's end in *end (0 for none). */
static uint64_t run_read_program(uint32_t dpcr, uint64_t *end, uint64_t *entry, uint32_t chcr_read)
{
    uint32_t w[17];
    uint32_t n = put_setup(w, dpcr);
    w[n++] = I_SW(T4, 0xE8u, T0);
    w[n++] = chcr_read != 0u ? I_LW(T5, 0xE8u, T0) : I_LW(T5, 0x100u, T6);
    w[n++] = I_ADDIU(T9, 0u, 1u);
    w[n++] = I_BEQ(0u, 0u, -1);
    w[n++] = I_NOP;
    schway *inst = make();
    load(inst, w, n);
    run_to_entry(inst);
    *entry = inst->ticks;
    *end = 0u;
    uint64_t marker = 0u;
    for (uint32_t i = 0; i < 20000u && marker == 0u; i++) {
        if (*end == 0u && inst->dma.cur != SCHWAY__DMA_NONE) {
            *end = inst->dma.busy_until;
        }
        if (inst->cpu.r[T9] != 0u) {
            marker = inst->ticks;
        } else {
            (void)run(inst, 1u);
        }
    }
    if (chcr_read != 0u) {
        CHECK_EQ_HEX(inst->cpu.r[T5], 0x00000002u);
    }
    schway_destroy(inst);
    return marker;
}

/* Fault prevented: a CPU read from RAM that runs on while a transfer holds the
   bus. The instruction after the read starts no earlier than the unit's end;
   without a unit the same program takes a few hundred cycles. */
static void case_stall_cpu_access(void)
{
    uint64_t end = 0u;
    uint64_t entry = 0u;
    uint64_t marker = run_read_program(DPCR_OTC, &end, &entry, 0u);
    CHECK(end != 0u);
    CHECK(end - entry >= 4352u);
    CHECK(marker >= end);
    CHECK(marker < end + 100u);
    uint64_t cend = 1u;
    uint64_t centry = 0u;
    uint64_t control = run_read_program(0x07654321u, &cend, &centry, 0u);
    CHECK_EQ_U64(cend, 0u);
    CHECK(control != 0u);
    CHECK(control - centry < 1000u);
}

/* Fault prevented: a CHCR read that returns the busy value, or one that does
   not hold the CPU until the unit is done. The read right after the start
   shows completion, and the next instruction runs after the unit's end. */
static void case_otc_read_sees_completion(void)
{
    uint64_t end = 0u;
    uint64_t entry = 0u;
    uint64_t marker = run_read_program(DPCR_OTC, &end, &entry, 1u);
    CHECK(end != 0u);
    CHECK(marker >= end);
    CHECK(marker < end + 100u);
}

/* Fault prevented: a CPU that stops for a transfer when it only touches the
   scratchpad, the cache and the registers. After the CHCR store the loop reads
   the scratchpad and counts; it keeps going the whole unit long and the engine
   never holds it. */
static void case_stall_not_scratch(void)
{
    uint32_t w[17];
    uint32_t n = put_setup(w, DPCR_OTC);
    w[n++] = I_SW(T4, 0xE8u, T0);
    w[n++] = I_LW(T5, 0xF000u, T0);
    w[n++] = I_BEQ(0u, 0u, -2);
    w[n++] = I_ADDIU(T9, T9, 1u);
    schway *inst = make();
    load(inst, w, n);
    run_to_entry(inst);
    uint64_t end = step_to_unit(inst);
    CHECK(end != 0u);
    while (inst->ticks < end) {
        (void)run(inst, 1u);
        CHECK_EQ_U64(inst->dma.stall_until, 0u);
    }
    /* Each pass is a load, a branch and its slot: a few cycles, so a unit of
       4,352 cycles holds hundreds of passes. */
    CHECK(inst->cpu.r[T9] > 500u);
    schway_destroy(inst);
}

/* Runs the five-store loop with a unit started at its top: store addresses at
   base. Returns with the fifth store's results checked against stalls. */
static void run_store_loop(uint32_t base_hi, uint32_t expect_stall)
{
    const uint32_t loop = TEXT + 16u;
    uint32_t w[12];
    w[0] = I_LUI(T0, base_hi);
    w[1] = I_ORI(T0, T0, base_hi == 0x8000u ? 0x0400u : 0u);
    w[2] = I_NOP;
    w[3] = I_NOP;
    w[4] = I_SW(T1, 0u, T0);
    w[5] = I_SW(T1, 4u, T0);
    w[6] = I_SW(T1, 8u, T0);
    w[7] = I_SW(T1, 12u, T0);
    w[8] = I_SW(T1, 16u, T0);
    w[9] = I_ADDIU(T9, 0u, 1u);
    w[10] = I_J(loop);
    w[11] = I_NOP;
    schway *inst = make();
    load(inst, w, 12u);
    run_to_entry(inst);
    /* A few hundred passes with no unit warm the cache and fill the queue. */
    (void)run(inst, 3000u);
    for (uint32_t i = 0; i < 40u && inst->cpu.pc != loop; i++) {
        (void)run(inst, 1u);
    }
    CHECK_EQ_HEX(inst->cpu.pc, loop);
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    /* Empty queue, marker cleared, then the unit starts through the registers. */
    inst->cpu.wq_count = 0u;
    inst->cpu.r[T9] = 0u;
    wr32(inst, DPCR, DPCR_OTC);
    wr32(inst, MADR(6u), OTC_TOP);
    wr32(inst, BCR(6u), 4096u);
    wr32(inst, CHCR(6u), 0x11000000u);
    uint64_t end = inst->dma.busy_until;
    CHECK_EQ_U64(inst->dma.cur, 6u);
    CHECK(end > inst->ticks);
    CHECK_EQ_U64(inst->dma.stall_until, 0u);
    uint32_t seen_fourth = 0u;
    uint32_t seen_fifth = 0u;
    uint64_t marker = 0u;
    for (uint32_t i = 0; i < 20000u && marker == 0u; i++) {
        (void)run(inst, 1u);
        if (inst->cpu.pc == loop + 16u && seen_fourth == 0u) {
            /* Four stores done: none of them held the CPU. */
            seen_fourth = 1u;
            CHECK(inst->dma.stall_until <= inst->ticks);
        }
        if (inst->cpu.pc == loop + 20u && seen_fifth == 0u) {
            /* The fifth store is the one that waits. */
            seen_fifth = 1u;
            if (expect_stall != 0u) {
                CHECK_EQ_U64(inst->dma.stall_until, end);
            } else {
                CHECK_EQ_U64(inst->dma.stall_until, 0u);
            }
        }
        if (inst->cpu.r[T9] != 0u) {
            marker = inst->ticks;
        }
    }
    CHECK_EQ_U64(seen_fourth, 1u);
    CHECK_EQ_U64(seen_fifth, 1u);
    CHECK(marker != 0u);
    if (expect_stall != 0u) {
        CHECK(marker >= end);
    } else {
        CHECK(marker < end);
        CHECK_EQ_U64(inst->dma.stall_until, 0u);
    }
    schway_destroy(inst);
}

/* Fault prevented: a store that holds the CPU while the write queue has room,
   a fifth store that does not, or a scratchpad store that does. With the code
   already cached and the queue empty when the unit starts, the first four
   stores to RAM go through, the fifth waits for the unit's end and the
   instruction after it runs no earlier; five scratchpad stores never wait. */
static void case_stall_write_queue(void)
{
    run_store_loop(0x8000u, 1u);
    run_store_loop(0x1F80u, 0u);
}

/* Fills n words of RAM from addr with a pattern of the word index. */
static void fill_ram(schway *inst, uint32_t addr, uint32_t n, uint32_t seed)
{
    for (uint32_t i = 0u; i < n; i++) {
        wr32(inst, addr + 4u * i, seed + i * 0x01010101u);
    }
}

/* The CRC-32 of n little-endian words starting from the pattern fill_ram
   writes. */
static uint32_t pattern_crc(uint32_t n, uint32_t seed)
{
    uint32_t crc = 0u;
    for (uint32_t i = 0u; i < n; i++) {
        uint32_t w = seed + i * 0x01010101u;
        uint8_t b[4] = {(uint8_t)w, (uint8_t)(w >> 8), (uint8_t)(w >> 16), (uint8_t)(w >> 24)};
        crc = schway__crc32_update(crc, b, sizeof b);
    }
    return crc;
}

/* Fault prevented: a burst that loses a GP0 upload, leaves MADR where it was,
   or forgets the completion flag and interrupt. */
static void case_sync0_gpu_sink(void)
{
    schway *inst = make();
    wr32(inst, 0x1F801814u, 0x04000001u);
    wr32(inst, 0x1000u, 0xA0000000u);
    wr32(inst, 0x1004u, (5u << 16) | 6u);
    wr32(inst, 0x1008u, 0x00010002u);
    wr32(inst, 0x100Cu, 0x7C1F03E0u);
    wr32(inst, DPCR, DPCR_ALL);
    wr32(inst, DICR, 0x00840000u);
    wr32(inst, MADR(2u), 0x1000u);
    wr32(inst, BCR(2u), 4u);
    uint64_t start = now_of(inst);
    wr32(inst, CHCR(2u), 0x11000001u);
    CHECK_EQ_U64(inst->dma.cur, 2u);
    CHECK_EQ_HEX(inst->vram[5][6], 0x03E0u);
    CHECK_EQ_HEX(inst->vram[5][7], 0x7C1Fu);
    CHECK_EQ_HEX(inst->dma.madr[2], 0x1010u);
    CHECK_EQ_U64(inst->dma.busy_until - start, 5u);
    CHECK_EQ_U64(inst->dma.words[0] + inst->dma.words[1] + inst->dma.words[3], 0u);
    (void)run(inst, 4u);
    CHECK((inst->dma.chcr[2] & SCHWAY__DMA_CHCR_BUSY) != 0u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 0u);
    (void)run(inst, 2u);
    CHECK_EQ_U64(inst->dma.chcr[2] & SCHWAY__DMA_CHCR_BUSY, 0u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 8u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x84840000u);
    CHECK_EQ_HEX(rd32(inst, MADR(2u)), 0x1010u);
    schway_destroy(inst);
}

/* Fault prevented: a read from a device that does not write zeros to RAM,
   one that steps forward when CHCR says backward, or a cost off the CD-ROM's
   24 clocks per word. Eight words from channel 3 go to RAM downward from
   2000h over a filled area: the words at 2000h down to 1FE4h become 0 and
   nothing around them changes. */
static void case_sync0_from_device(void)
{
    schway *inst = make();
    fill_ram(inst, 0x1FC0u, 32u, 0x11111111u);
    wr32(inst, DPCR, DPCR_ALL);
    wr32(inst, MADR(3u), 0x2000u);
    wr32(inst, BCR(3u), 8u);
    uint64_t start = now_of(inst);
    wr32(inst, CHCR(3u), 0x11000002u);
    CHECK_EQ_U64(inst->dma.cur, 3u);
    CHECK_EQ_U64(inst->dma.busy_until - start, 8u * 24u + 1u);
    CHECK_EQ_HEX(inst->dma.madr[3], 0x1FE0u);
    for (uint32_t i = 0u; i < 32u; i++) {
        uint32_t a = 0x1FC0u + 4u * i;
        uint32_t filled = 0x11111111u + i * 0x01010101u;
        uint32_t moved = a >= 0x1FE4u && a <= 0x2000u;
        CHECK_EQ_HEX(ram_word(inst, a), moved ? 0u : filled);
    }
    CHECK_EQ_U64(inst->dma.words[3], 0u);
    schway_destroy(inst);
}

/* Fault prevented: DMA2 discarding GP0 words, losing a packet split across
   linked-list nodes or a CPU continuation, returning zeros instead of VRAM,
   or failing to request another sync-mode-1 block while its FIFO has room. */
static void case_gpu_linked_list_readback(void)
{
    schway *inst = make();
    schway *reference = make();
    const uint32_t x = 12u;
    const uint32_t y = 3u;
    const uint32_t pixel = 0x7C1Fu;

    wr32(inst, 0x1F801814u, 0x04000001u);
    wr32(inst, 0x1000u, (2u << 24) | 0x1020u);
    wr32(inst, 0x1004u, 0xA0000000u);
    wr32(inst, 0x1008u, (y << 16) | x);
    wr32(inst, 0x1020u, (2u << 24) | 0x00FFFFFFu);
    wr32(inst, 0x1024u, 0x00010001u);
    wr32(inst, 0x1028u, pixel);
    wr32(inst, DPCR, DPCR_ALL);
    wr32(inst, MADR(2u), 0x1000u);
    wr32(inst, CHCR(2u), 0x01000401u);
    (void)run(inst, 200u);

    wr32(reference, 0x1F801810u, 0xA0000000u);
    wr32(reference, 0x1F801810u, (y << 16) | x);
    wr32(reference, 0x1F801810u, 0x00010001u);
    wr32(reference, 0x1F801810u, pixel);
    CHECK_EQ_HEX(inst->vram[y][x], pixel);
    CHECK_EQ_HEX(inst->vram[y][x], reference->vram[y][x]);

    /* CPU starts the packet; a one-block DMA continues its owed words. */
    wr32(inst, 0x1F801810u, 0xA0000000u);
    wr32(inst, 0x1F801810u, (4u << 16) | 20u);
    wr32(inst, 0x2000u, 0x00010001u);
    wr32(inst, 0x2004u, 0x03E0u);
    wr32(inst, MADR(2u), 0x2000u);
    wr32(inst, BCR(2u), 2u);
    wr32(inst, CHCR(2u), 0x11000001u);
    (void)run(inst, 100u);
    CHECK_EQ_HEX(inst->vram[4u][20u], 0x03E0u);

    /* GPU-to-RAM DMA returns the real packed GPUREAD word. */
    wr32(inst, 0x1F801814u, 0x04000003u);
    wr32(inst, 0x1F801810u, 0xC0000000u);
    wr32(inst, 0x1F801810u, (y << 16) | x);
    wr32(inst, 0x1F801810u, 0x00010001u);
    wr32(inst, MADR(2u), 0x3000u);
    wr32(inst, BCR(2u), 1u);
    wr32(inst, CHCR(2u), 0x11000000u);
    CHECK_EQ_HEX(ram_word(inst, 0x3000u), pixel);
    CHECK_EQ_U64(inst->gpu.owed_words, 0u);
    schway_destroy(reference);
    schway_destroy(inst);
}

/* Fault prevented: sync mode 1 that moves every block on one request, waits
   for none, loses the block count or its address, or finishes without the
   flag. Four blocks of 16 words go to PIO, one block per request-line raise. */
static void case_sync1_dreq_blocks(void)
{
    schway *inst = make();
    fill_ram(inst, 0x3000u, 64u, 0x5A000001u);
    wr32(inst, DPCR, DPCR_ALL);
    wr32(inst, DICR, 0x00A00000u);
    wr32(inst, MADR(5u), 0x3000u);
    wr32(inst, BCR(5u), (4u << 16) | 16u);
    wr32(inst, CHCR(5u), 0x01000201u);
    /* No request, so nothing moves and the channel is busy. */
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    CHECK_EQ_U64(inst->dma.words[5], 0u);
    CHECK((rd32(inst, CHCR(5u)) & SCHWAY__DMA_CHCR_BUSY) != 0u);
    for (uint32_t b = 0u; b < 4u; b++) {
        uint64_t at = now_of(inst);
        schway__dma_dreq(inst, 5u, 1u);
        CHECK_EQ_U64(inst->dma.cur, 5u);
        CHECK_EQ_U64(inst->dma.words[5], 16u * (b + 1u));
        CHECK_EQ_U64(inst->dma.busy_until - at, 16u * 20u + 1u + 11u);
        CHECK_EQ_U64((inst->dma.dreq >> 5) & 1u, 0u);
        CHECK_EQ_HEX(inst->dma.madr[5], 0x3000u + 64u * (b + 1u));
        (void)run(inst, 333u);
        CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
        if (b < 3u) {
            CHECK((inst->dma.chcr[5] & SCHWAY__DMA_CHCR_BUSY) != 0u);
            CHECK_EQ_U64(inst->irq.stat & 8u, 0u);
            CHECK_EQ_U64(inst->dma.remain[5], 3u - b);
            /* Nothing moves until the next request. */
            (void)run(inst, 500u);
            CHECK_EQ_U64(inst->dma.words[5], 16u * (b + 1u));
        }
    }
    CHECK_EQ_U64(inst->dma.chcr[5] & SCHWAY__DMA_CHCR_BUSY, 0u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 8u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0xA0A00000u);
    CHECK_EQ_HEX(inst->dma.crc[5], pattern_crc(64u, 0x5A000001u));
    /* A request after the end starts nothing. */
    schway__dma_dreq(inst, 5u, 1u);
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    schway_destroy(inst);
}

/* Fault prevented: ATTR DMA-write mode failing to supply persistent readiness
   for every request-mode block sent to the unbuilt SPU sink. The 48 words are
   judged by source RAM pattern, address, block cost and final IRQ state. */
static void case_spu_request_blocks(void)
{
    const uint32_t seed = 0x5A000001u;
    schway *inst = make();
    fill_ram(inst, 0x6000u, 48u, seed);
    wr16(inst, 0x1F801DAAu, 0x0020u);
    CHECK_EQ_U64(schway__spu_dma_ready(inst, 1u), 1u);
    wr32(inst, DPCR, DPCR_ALL);
    wr32(inst, DICR, 0x00900000u);
    wr32(inst, MADR(4u), 0x6000u);
    wr32(inst, BCR(4u), (3u << 16) | 16u);
    uint64_t start = now_of(inst);
    wr32(inst, CHCR(4u), 0x01000201u);
    wr16(inst, 0x1F801DAAu, 0x0020u);
    CHECK_EQ_U64(inst->dma.cur, 4u);
    CHECK_EQ_U64(inst->dma.words[4], 16u);
    for (uint32_t i = 0u; i < 16u; i++) {
        uint32_t word = ram_word(inst, 0x6000u + 4u * i);
        CHECK_EQ_HEX(schway__spu_ram_peek(inst, 4u * i), (uint16_t)word);
        CHECK_EQ_HEX(schway__spu_ram_peek(inst, 4u * i + 2u), (uint16_t)(word >> 16));
    }
    CHECK_EQ_U64(inst->dma.busy_until - start, 76u);
    CHECK((inst->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY) != 0u);
    CHECK_EQ_U64(inst->dma.dicr & 0x01000000u, 0u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 0u);
    CHECK_EQ_HEX(inst->dma.madr[4], 0x6040u);
    CHECK_EQ_HEX(inst->dma.crc[4], pattern_crc(16u, seed));
    schway__dma_event(inst, start + 75u);
    CHECK_EQ_U64(inst->dma.words[4], 16u);
    schway__dma_event(inst, start + 76u);
    uint64_t second_request = inst->spu.next_tick;
    CHECK_EQ_U64(inst->dma.cur, 4u);
    CHECK_EQ_U64(inst->dma.phase[4], 3u);
    CHECK_EQ_U64(inst->dma.busy_until, second_request);
    schway__dma_event(inst, second_request - 1u);
    CHECK_EQ_U64(inst->dma.words[4], 16u);
    spu_dma_event(inst, second_request);
    CHECK_EQ_U64(inst->dma.words[4], 32u);
    CHECK_EQ_U64(inst->dma.busy_until, second_request + 76u);
    schway__dma_event(inst, second_request + 76u);
    uint64_t third_request = inst->spu.next_tick;
    CHECK_EQ_U64(inst->dma.busy_until, third_request);
    spu_dma_event(inst, third_request);
    CHECK_EQ_U64(inst->dma.words[4], 48u);
    schway__dma_event(inst, third_request + 76u);
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    CHECK_EQ_U64(inst->dma.words[4], 48u);
    CHECK_EQ_HEX(inst->dma.madr[4], 0x60C0u);
    CHECK_EQ_HEX(inst->dma.crc[4], pattern_crc(48u, seed));
    CHECK_EQ_U64(inst->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY, 0u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x90900000u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 8u);
    wr16(inst, 0x1F801DAAu, 0x0020u);
    wr16(inst, 0x1F801DAAu, 0x0020u);
    CHECK_EQ_U64(inst->dma.words[4], 48u);
    CHECK_EQ_U64((inst->dma.dreq >> 4) & 1u, 0u);
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    schway_destroy(inst);

    /* ATTR may become ready after an active transfer has already waited. */
    inst = make();
    fill_ram(inst, 0x7000u, 16u, seed);
    wr32(inst, DPCR, DPCR_ALL);
    wr32(inst, DICR, 0x00900000u);
    wr32(inst, MADR(4u), 0x7000u);
    wr32(inst, BCR(4u), (1u << 16) | 16u);
    wr32(inst, CHCR(4u), 0x01000201u);
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    CHECK_EQ_U64(inst->dma.words[4], 0u);
    uint64_t wake = now_of(inst);
    wr16(inst, 0x1F801DAAu, 0x0020u);
    CHECK_EQ_U64(inst->dma.cur, 4u);
    CHECK_EQ_U64(inst->dma.words[4], 16u);
    CHECK_EQ_U64(inst->dma.busy_until - wake, 76u);
    schway__dma_event(inst, wake + 76u);
    CHECK_EQ_U64(inst->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY, 0u);
    CHECK_EQ_HEX(inst->dma.madr[4], 0x7040u);
    CHECK_EQ_HEX(inst->dma.crc[4], pattern_crc(16u, seed));
    CHECK_EQ_U64(inst->irq.stat & 8u, 8u);
    schway_destroy(inst);
}

/* Fault prevented: DMA4 sync-mode-0 transfers waiting forever for bit 28 even
   though the pinned spu/memory-transfer guest starts both directions with
   bit 24 alone. */
static void case_spu_sync0_starts_without_trigger(void)
{
    const uint32_t seed = 0x3C000001u;
    schway *inst = make();
    fill_ram(inst, 0xD000u, 4u, seed);
    wr16(inst, 0x1F801DA6u, 0x0200u);
    wr16(inst, 0x1F801DAAu, 0x0020u);
    spu_dma_event(inst, inst->spu.next_tick);
    wr32(inst, DPCR, DPCR_ALL);
    wr32(inst, MADR(4u), 0xD000u);
    wr32(inst, BCR(4u), 4u);
    wr32(inst, CHCR(4u), 0x01000001u);
    CHECK_EQ_U64(inst->dma.cur, 4u);
    CHECK_EQ_U64(inst->dma.words[4], 4u);
    for (uint32_t i = 0u; i < 4u; i++) {
        uint32_t word = ram_word(inst, 0xD000u + 4u * i);
        CHECK_EQ_HEX(schway__spu_ram_peek(inst, 0x1000u + 4u * i), (uint16_t)word);
        CHECK_EQ_HEX(schway__spu_ram_peek(inst, 0x1002u + 4u * i), (uint16_t)(word >> 16));
    }
    schway__dma_event(inst, inst->dma.busy_until);
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    CHECK_EQ_U64(inst->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY, 0u);
    schway_destroy(inst);

    inst = make();
    fill_ram(inst, 0xE000u, 4u, 0u);
    wr16(inst, 0x1F801DA6u, 0x0200u);
    wr16(inst, 0x1F801DAAu, 0x0030u);
    wr32(inst, 0x1F801014u, 0x220931E1u);
    spu_dma_event(inst, inst->spu.next_tick);
    wr32(inst, DPCR, DPCR_ALL);
    for (uint32_t i = 0u; i < 8u; i++) {
        schway__spu_ram_write(inst, 0x1000u + 2u * i, (uint16_t)(0xA100u + i));
    }
    wr32(inst, MADR(4u), 0xE000u);
    wr32(inst, BCR(4u), 4u);
    wr32(inst, CHCR(4u), 0x01000000u);
    CHECK_EQ_U64(inst->dma.cur, 4u);
    CHECK_EQ_HEX(inst->dma.madr[4], 0xE010u);
    schway__dma_event(inst, inst->dma.busy_until);
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    CHECK_EQ_U64(inst->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY, 0u);
    for (uint32_t i = 0u; i < 4u; i++) {
        uint32_t word = ram_word(inst, 0xE000u + 4u * i);
        CHECK_EQ_HEX(word & 0xFFFFu, 0xA100u + 2u * i);
        CHECK_EQ_HEX(word >> 16, 0xA101u + 2u * i);
    }
    schway_destroy(inst);
}

/* Words of a program that points t0 at the I/O page, sets DPCR, then MADR,
   BCR and CHCR of channel 4 (the SPU) to a transfer of four 16-word blocks
   with the CHCR value given, and loops on a read from RAM that counts in t9. */
static uint32_t put_request_loop(uint32_t *w, uint32_t chcr)
{
    uint32_t n = 0u;
    w[n++] = I_LUI(T0, 0x1F80u);
    w[n++] = I_ORI(T0, T0, 0x1000u);
    w[n++] = I_LUI(T1, DPCR_ALL >> 16);
    w[n++] = I_ORI(T1, T1, DPCR_ALL & 0xFFFFu);
    w[n++] = I_SW(T1, 0xF0u, T0);
    w[n++] = I_ORI(T2, 0u, 0x3000u);
    w[n++] = I_SW(T2, 0xC0u, T0);
    w[n++] = I_LUI(T3, 4u);
    w[n++] = I_ORI(T3, T3, 16u);
    w[n++] = I_SW(T3, 0xC4u, T0);
    w[n++] = I_LUI(T4, chcr >> 16);
    w[n++] = I_ORI(T4, T4, chcr & 0xFFFFu);
    w[n++] = I_SW(T4, 0xC8u, T0);
    w[n++] = I_LUI(T6, 0x8000u);
    w[n++] = I_LW(T5, 0x100u, T6);
    w[n++] = I_BEQ(0u, 0u, -2);
    w[n++] = I_ADDIU(T9, T9, 1u);
    return n;
}

/* Fault prevented: a channel that waits for its request line and still holds
   the CPU, or a program whose RAM reads slow down because a transfer is
   pending. The same loop runs against a request-mode transfer that was started
   and one whose CHCR write left bit 24 clear: after the same number of cycles
   both have counted the same passes, the CPU never was held and the started
   channel is still busy. */
static void case_sync1_no_dreq_no_stall(void)
{
    uint32_t w[20];
    uint32_t n = put_request_loop(w, 0x01000201u);
    schway *busy = make();
    load(busy, w, n);
    run_to_entry(busy);
    (void)run(busy, 6000u);
    uint32_t passes_busy = busy->cpu.r[T9];
    CHECK_EQ_U64(busy->dma.stall_until, 0u);
    CHECK_EQ_U64(busy->dma.cur, SCHWAY__DMA_NONE);
    CHECK((busy->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY) != 0u);
    uint32_t idle_w[20];
    n = put_request_loop(idle_w, 0x00000201u);
    schway *idle = make();
    load(idle, idle_w, n);
    run_to_entry(idle);
    (void)run(idle, 6000u);
    CHECK_EQ_U64(idle->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY, 0u);
    CHECK(passes_busy > 100u);
    CHECK_EQ_U64(passes_busy, idle->cpu.r[T9]);
    schway_destroy(busy);
    schway_destroy(idle);
}

/* Fault prevented: a channel whose burst costs another number of cycles than
   its device rate plus the DRAM row: 64 words take 64 * rate + 4 cycles, with
   the rate 1 for the MDEC channels and the GPU, 24 for the CD-ROM (the
   documented BIOS default), 4 for the SPU and 20 for PIO. */
static void case_costs_per_channel(void)
{
    static const uint32_t rate[6] = {1u, 1u, 1u, 24u, 4u, 20u};
    static const uint32_t chcr[6] = {0x11000001u, 0x11000000u, 0x11000001u,
                                     0x11000000u, 0x11000001u, 0x11000000u};
    for (uint32_t ch = 0u; ch < 6u; ch++) {
        schway *inst = make();
        wr32(inst, DPCR, DPCR_ALL);
        wr32(inst, MADR(ch), 0x4000u);
        wr32(inst, BCR(ch), 64u);
        uint64_t start = now_of(inst);
        wr32(inst, CHCR(ch), chcr[ch]);
        CHECK_EQ_U64(inst->dma.cur, ch);
        CHECK_EQ_U64(inst->dma.busy_until - start, 64u * rate[ch] + 4u);
        schway_destroy(inst);
    }
    /* A word count of 0 is 10000h words: the CD-ROM's longest burst, within
       the bound the engine keeps on a unit. */
    schway *inst = make();
    wr32(inst, DPCR, DPCR_ALL);
    wr32(inst, MADR(3u), 0u);
    wr32(inst, BCR(3u), 0u);
    uint64_t start = now_of(inst);
    wr32(inst, CHCR(3u), 0x11000000u);
    CHECK_EQ_U64(inst->dma.busy_until - start, 65536u * 24u + 4096u);
    CHECK(inst->dma.busy_until - start <= SCHWAY__DMA_BUSY_MAX);
    schway_destroy(inst);
}

/* Fault prevented: a priority change in DPCR that does not take effect at the
   next unit boundary. Channel 1 runs; channels 4 and 5 wait with priorities 5
   and 6, so 4 goes next; the same wait with channel 5 raised to priority 0
   while channel 1 runs makes 5 go next. */
static void case_dpcr_rearbitrate(void)
{
    for (uint32_t raise = 0u; raise < 2u; raise++) {
        schway *inst = make();
        wr32(inst, DPCR, DPCR_ALL);
        wr32(inst, MADR(1u), 0x4000u);
        wr32(inst, BCR(1u), 64u);
        wr32(inst, CHCR(1u), 0x11000000u);
        CHECK_EQ_U64(inst->dma.cur, 1u);
        wr32(inst, BCR(4u), 8u);
        wr32(inst, CHCR(4u), 0x11000001u);
        wr32(inst, BCR(5u), 8u);
        wr32(inst, CHCR(5u), 0x11000001u);
        CHECK_EQ_U64(inst->dma.cur, 1u);
        if (raise != 0u) {
            wr32(inst, DPCR, (DPCR_ALL & ~0x00F00000u) | 0x00800000u);
            CHECK_EQ_U64(inst->dma.cur, 1u);
        }
        schway__dma_event(inst, inst->dma.busy_until);
        CHECK_EQ_U64(inst->dma.cur, raise != 0u ? 5u : 4u);
        schway_destroy(inst);
    }
}

/* CHCR of the GPU channel in linked list mode, from RAM, with bit 24 set. */
#define CHCR_LIST 0x01000401u

/* Runs one cycle at a time until the engine has passed n nodes of the list or
   the channel stops being busy; returns the cycles run, with 100000 as the
   bound a test never reaches. */
static uint64_t run_to_node(schway *inst, uint32_t ch, uint32_t n)
{
    uint64_t at = inst->ticks;
    for (uint32_t i = 0u; i < 100000u; i++) {
        if (inst->dma.remain[ch] >= n ||
            (inst->dma.chcr[ch] & SCHWAY__DMA_CHCR_BUSY) == 0u) {
            break;
        }
        (void)run(inst, 1u);
    }
    return inst->ticks - at;
}

/* Fault prevented: a list that sends its headers to the device, loses a node,
   follows the wrong address, ends on a header word instead of the marker's bit
   23 or finishes without the flag and interrupt. Three nodes of 2, 0 and 5
   words carry one upload packet to VRAM, and MADR is left at the end marker. */
static void case_list_walk(void)
{
    schway *inst = make();
    wr32(inst, 0x1F801814u, 0x04000001u);
    wr32(inst, 0x1000u, (2u << 24) | 0x1010u);
    wr32(inst, 0x1004u, 0xA0000000u);
    wr32(inst, 0x1008u, (8u << 16) | 7u);
    wr32(inst, 0x1010u, (0u << 24) | 0x1020u);
    wr32(inst, 0x1020u, (5u << 24) | 0x00FFFFFFu);
    wr32(inst, 0x1024u, 0x00040002u);
    wr32(inst, 0x1028u, 0x7C1F03E0u);
    wr32(inst, 0x102Cu, 0x001F7FE0u);
    wr32(inst, 0x1030u, 0x4210C210u);
    wr32(inst, 0x1034u, 0x7FFF0000u);
    wr32(inst, DPCR, DPCR_ALL);
    wr32(inst, DICR, 0x00840000u);
    wr32(inst, MADR(2u), 0x1000u);
    wr32(inst, CHCR(2u), CHCR_LIST);
    /* The first node moved at the start; its header went nowhere. */
    CHECK_EQ_U64(inst->dma.cur, 2u);
    CHECK_EQ_U64(inst->dma.remain[2], 1u);
    CHECK_EQ_HEX(inst->dma.madr[2], 0x1010u);
    (void)run_to_node(inst, 2u, 2u);
    CHECK_EQ_HEX(inst->dma.madr[2], 0x1020u);
    (void)run_to_node(inst, 2u, 3u);
    CHECK_EQ_HEX(inst->dma.madr[2], 0xFFFFFFu);
    CHECK((inst->dma.chcr[2] & SCHWAY__DMA_CHCR_BUSY) != 0u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 0u);
    (void)run(inst, 200u);
    CHECK_EQ_U64(inst->dma.chcr[2] & SCHWAY__DMA_CHCR_BUSY, 0u);
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    CHECK_EQ_U64(inst->dma.remain[2], 0u);
    CHECK_EQ_HEX(inst->vram[8][7], 0x03E0u);
    CHECK_EQ_HEX(inst->vram[8][8], 0x7C1Fu);
    CHECK_EQ_HEX(inst->vram[9][7], 0x7FE0u);
    CHECK_EQ_HEX(inst->vram[9][8], 0x001Fu);
    CHECK_EQ_HEX(inst->vram[10][7], 0xC210u);
    CHECK_EQ_HEX(inst->vram[10][8], 0x4210u);
    CHECK_EQ_HEX(inst->vram[11][7], 0x0000u);
    CHECK_EQ_HEX(inst->vram[11][8], 0x7FFFu);
    CHECK_EQ_U64(inst->irq.stat & 8u, 8u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x84840000u);
    CHECK_EQ_HEX(rd32(inst, MADR(2u)), 0xFFFFFFu);
    CHECK_EQ_HEX(rd32(inst, CHCR(2u)), 0x00000401u);
    schway_destroy(inst);
}

/* Fault prevented: an interleaved walker resetting another channel's node
   count, which would let a self-reference run past its own 1000h-node yield. */
static void case_list_independent_yields(void)
{
    schway *inst = make();
    /* Channel 1 outranks channel 2 so it can interrupt the first walk at the
       boundary immediately before channel 2 reaches its yield. */
    wr32(inst, DPCR, 0x00000980u);
    wr32(inst, 0x3000u, 0x00003000u);
    wr32(inst, 0x4000u, 0x00800000u);
    wr32(inst, MADR(2u), 0x3000u);
    wr32(inst, CHCR(2u), CHCR_LIST);
    for (uint32_t i = 1u; i < SCHWAY__DMA_NODES_MAX - 1u; i++) {
        schway__dma_event(inst, inst->dma.busy_until);
        schway__dma_event(inst, inst->dma.busy_until);
    }
    CHECK_EQ_U64(inst->dma.cur, 2u);

    wr32(inst, MADR(1u), 0x4000u);
    wr32(inst, CHCR(1u), CHCR_LIST);
    uint64_t ch2_end = inst->dma.busy_until;
    schway__dma_event(inst, ch2_end);
    schway__dma_event(inst, inst->dma.busy_until);
    CHECK_EQ_U64(inst->dma.cur, 1u);

    uint64_t ch1_end = inst->dma.busy_until;
    schway__dma_event(inst, ch1_end);
    CHECK_EQ_U64(inst->dma.cur, 2u);
    ch2_end = inst->dma.busy_until;
    uint64_t caught_up_to = ch2_end + 10u;
    schway__dma_event(inst, caught_up_to);
    CHECK_EQ_U64(inst->dma.cur, 2u);
    CHECK_EQ_U64(inst->dma.busy_until, caught_up_to + 1u);
    CHECK(schway__dma_next(inst) > caught_up_to);
    CHECK((inst->dma.chcr[2] & SCHWAY__DMA_CHCR_BUSY) != 0u);
    CHECK_EQ_U64(inst->dma.chcr[1] & SCHWAY__DMA_CHCR_BUSY, 0u);
    schway_destroy(inst);
}

/* Fault prevented: a list whose first header already holds the end marker that
   walks on, or one that finishes without the flag and the interrupt. The two
   forms the public chain-looping test builds are checked: a header that is the
   marker, and a header that points to one. The end is bit 23 of the next
   address alone: 800000h ends a list and leaves itself in MADR. */
static void case_list_empty(void)
{
    for (uint32_t form = 0u; form < 3u; form++) {
        schway *inst = make();
        uint32_t marker = form == 2u ? 0x800000u : 0xFFFFFFu;
        if (form != 1u) {
            wr32(inst, 0x2000u, marker);
        } else {
            wr32(inst, 0x2000u, 0x00002004u);
            wr32(inst, 0x2004u, 0x00FFFFFFu);
        }
        wr32(inst, DPCR, DPCR_ALL);
        wr32(inst, DICR, 0x00840000u);
        wr32(inst, MADR(2u), 0x2000u);
        wr32(inst, CHCR(2u), CHCR_LIST);
        (void)run(inst, 200u);
        CHECK_EQ_U64(inst->dma.chcr[2] & SCHWAY__DMA_CHCR_BUSY, 0u);
        CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
        CHECK_EQ_U64(inst->dma.words[2], 0u);
        CHECK_EQ_U64(inst->dma.remain[2], 0u);
        CHECK_EQ_HEX(rd32(inst, MADR(2u)), form == 1u ? 0xFFFFFFu : marker);
        CHECK_EQ_U64(inst->irq.stat & 8u, 8u);
        CHECK_EQ_HEX(rd32(inst, DICR), 0x84840000u);
        schway_destroy(inst);
    }
}

/* A list of one node that names itself (count 0), or of two that name each
   other, at 3000h: the lists the public chain-looping test builds. */
static void put_loop_list(schway *inst, uint32_t nodes)
{
    if (nodes == 1u) {
        wr32(inst, 0x3000u, 0x00003000u);
    } else {
        wr32(inst, 0x3000u, 0x00003004u);
        wr32(inst, 0x3004u, 0x00003000u);
    }
}

static void start_loop_list(schway *inst, uint32_t nodes)
{
    put_loop_list(inst, nodes);
    wr32(inst, DPCR, DPCR_ALL);
    wr32(inst, DICR, 0x00840000u);
    wr32(inst, MADR(2u), 0x3000u);
    wr32(inst, CHCR(2u), CHCR_LIST);
}

/* The DICR flag bits and I_STAT bit 3: nothing may be set by a list that does
   not end. */
static void check_no_flag(const schway *inst)
{
    CHECK_EQ_HEX(inst->dma.dicr & 0x7F000000u, 0u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 0u);
}

/* Fault prevented: a list that never ends hanging a step, finishing, raising a
   flag or an interrupt, or a frame that does not end. The guard is checked
   directly: an engine step whose tick is so far ahead that more than 2000h
   nodes are due passes 1000h of them and yields, with the channel still busy,
   no flag, no interrupt and the next start after the tick it was given; the
   next step does the same and the count of nodes since the start is 2000h.
   Then through the public API a frame ends with the channel still busy. */
static void case_list_self_ref(void)
{
    for (uint32_t nodes = 1u; nodes <= 2u; nodes++) {
        schway *inst = make();
        start_loop_list(inst, nodes);
        CHECK_EQ_U64(inst->dma.remain[2], 1u);
        uint64_t far1 = now_of(inst) + 0x10000000u;
        schway__dma_event(inst, far1);
        CHECK_EQ_HEX(inst->dma.remain[2], 0x1000u);
        CHECK((inst->dma.chcr[2] & SCHWAY__DMA_CHCR_BUSY) != 0u);
        check_no_flag(inst);
        CHECK(schway__dma_next(inst) > far1);
        uint64_t far2 = far1 + 0x10000000u;
        schway__dma_event(inst, far2);
        CHECK_EQ_HEX(inst->dma.remain[2], 0x2000u);
        CHECK((inst->dma.chcr[2] & SCHWAY__DMA_CHCR_BUSY) != 0u);
        check_no_flag(inst);
        CHECK(schway__dma_next(inst) > far2);
        schway_destroy(inst);
    }
    for (uint32_t nodes = 1u; nodes <= 2u; nodes++) {
        schway *inst = make();
        start_loop_list(inst, nodes);
        uint64_t end = inst->frame_end;
        schway_run_result r;
        memset(&r, 0, sizeof r);
        r.size = (uint32_t)sizeof r;
        CHECK_EQ_U64(schway_run_frame(inst, &r), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
        CHECK(r.ticks <= end + SCHWAY__DMA_BUSY_MAX);
        CHECK((inst->dma.chcr[2] & SCHWAY__DMA_CHCR_BUSY) != 0u);
        check_no_flag(inst);
        CHECK(inst->dma.remain[2] > 0x2000u);
        schway_destroy(inst);
    }
}

/* Fault prevented: a CHCR word of 0 that leaves a looping list running, ends
   it with a flag or an interrupt, or leaves the channel unable to run the next
   list. This is the procedure of the public chain-looping test, which writes 0
   between its cases and sees the next one run normally: after a looping list
   has run, in a unit or in its CPU window, the write stops it with no flag and
   no interrupt, the register reads 0, and an empty list on the same channel
   then completes with both. */
static void case_list_stop(void)
{
    uint32_t seen_unit = 0u;
    uint32_t seen_gap = 0u;
    for (uint32_t k = 0u; k < 24u; k++) {
        schway *inst = make();
        start_loop_list(inst, 1u);
        (void)run(inst, 1000u + k);
        CHECK((inst->dma.chcr[2] & SCHWAY__DMA_CHCR_BUSY) != 0u);
        seen_unit += inst->dma.phase[2] == 1u ? 1u : 0u;
        seen_gap += inst->dma.phase[2] == 3u ? 1u : 0u;
        wr32(inst, CHCR(2u), 0u);
        CHECK_EQ_HEX(rd32(inst, CHCR(2u)), 0u);
        CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
        CHECK_EQ_U64(schway__dma_next(inst), UINT64_MAX);
        (void)run(inst, 2000u);
        CHECK_EQ_HEX(rd32(inst, CHCR(2u)), 0u);
        check_no_flag(inst);
        wr32(inst, 0x4000u, 0x00FFFFFFu);
        wr32(inst, MADR(2u), 0x4000u);
        wr32(inst, CHCR(2u), CHCR_LIST);
        (void)run(inst, 200u);
        CHECK_EQ_U64(inst->dma.chcr[2] & SCHWAY__DMA_CHCR_BUSY, 0u);
        CHECK_EQ_U64(inst->irq.stat & 8u, 8u);
        CHECK_EQ_HEX(rd32(inst, DICR), 0x84840000u);
        schway_destroy(inst);
    }
    CHECK(seen_unit != 0u);
    CHECK(seen_gap != 0u);
}

/* The 64 chopping rows of the public ps1-tests dma/chopping hardware log
   (ps1-tests at f727802fead1, MIT: dma/chopping/psx.log, lines 68-131; the
   program is dma/chopping/main.cpp): CPU cycles a chopped transfer of 8,192
   bytes (2,048 words) from RAM to the GPU channel took, per DMA window of 2^n
   words (n = 0 to 7, the row groups) and CPU window of 2^m cycles (m = 0 to 7,
   within a group). The figures are copied as facts; the log is not kept. */
static const uint32_t chop_rows[64] = {
    16693u, 20595u, 22834u, 30867u, 47672u, 81926u, 80068u, 80007u,
    10520u, 10519u, 12511u, 16463u, 24668u, 41037u, 73765u, 73705u,
    5828u,  6287u,  7314u,  9390u,  13431u, 21832u, 37956u, 70651u,
    4011u,  4275u,  4719u,  5797u,  7776u,  11909u, 20131u, 36307u,
    3103u,  3251u,  3459u,  4021u,  4998u,  7031u,  11034u, 19235u,
    2659u,  2724u,  2786u,  3092u,  3630u,  4598u,  6563u,  10721u,
    2439u,  2469u,  2517u,  2643u,  2876u,  3392u,  4365u,  6378u,
    2320u,  2338u,  2362u,  2419u,  2536u,  2765u,  3274u,  4184u,
};

/* The ticks from the CHCR write to the end of a chopped transfer of 2,048
   words with DMA windows of 2^dw words and CPU windows of 2^cw cycles. The
   engine is stepped to each deadline in turn. */
static uint64_t chopped_cycles(uint32_t dw, uint32_t cw)
{
    schway *inst = make();
    fill_ram(inst, 0x4000u, 2048u, 0u);
    wr32(inst, 0x1F801814u, 0x04000001u);
    wr32(inst, 0x4000u, 0xA0000000u);
    wr32(inst, 0x4004u, 0u);
    wr32(inst, 0x4008u, (5u << 16) | 818u);
    wr32(inst, DPCR, DPCR_ALL);
    wr32(inst, MADR(2u), 0x4000u);
    wr32(inst, BCR(2u), 2048u);
    uint64_t start = now_of(inst);
    wr32(inst, CHCR(2u), 0x11000101u | (dw << 16) | (cw << 20));
    uint64_t t = start;
    for (uint32_t i = 0u; i < 1000000u && (inst->dma.chcr[2] & SCHWAY__DMA_CHCR_BUSY) != 0u; i++) {
        t = schway__dma_next(inst);
        CHECK(t != UINT64_MAX);
        schway__dma_event(inst, t);
    }
    CHECK_EQ_HEX(inst->vram[0][0], 0x0303u);
    CHECK_EQ_HEX(inst->vram[0][1], 0x0303u);
    CHECK_EQ_U64(inst->gpu.phase, SCHWAY__GPU_IDLE);
    CHECK_EQ_U64(inst->gpu.owed_words, 0u);
    CHECK_EQ_HEX(inst->dma.madr[2], 0x6000u);
    schway_destroy(inst);
    return t - start;
}

/* Fault prevented: a chopped transfer whose windows or CPU gaps are costed so
   that its time leaves the hardware's. Each of the 64 rows is compared with
   the model in bands, as a fitted model deserves (it is evidence of a fit to
   one log, not of the hardware): every row within 12 %, and the mean of the
   signed errors within 2 %, so the model is not biased high or low. The mean
   absolute error is printed. */
static void case_chopping_band(void)
{
    int64_t sum = 0;
    int64_t abs_sum = 0;
    for (uint32_t dw = 0u; dw < 8u; dw++) {
        for (uint32_t cw = 0u; cw < 8u; cw++) {
            uint64_t row = chop_rows[dw * 8u + cw];
            uint64_t model = chopped_cycles(dw, cw);
            int64_t err = ((int64_t)model - (int64_t)row) * 1000 / (int64_t)row;
            if (err > 120 || err < -120) {
                fprintf(stderr, "chopping: dma window 2^%u, cpu window 2^%u: model %llu, hardware %llu\n",
                        (unsigned)dw, (unsigned)cw, (unsigned long long)model,
                        (unsigned long long)row);
            }
            CHECK(err <= 120 && err >= -120);
            sum += err;
            abs_sum += err < 0 ? -err : err;
        }
    }
    printf("chopping: mean error %lld per mille, mean absolute error %lld per mille\n",
           (long long)(sum / 64), (long long)(abs_sum / 64));
    CHECK(sum / 64 <= 20 && sum / 64 >= -20);
}

/* The loop the public chain-looping test times: 1,000 passes that read a
   RAM word, add, and write it back. Marker t9 is set after the last pass. */
static uint32_t put_work_loop(uint32_t *w)
{
    uint32_t n = 0u;
    w[n++] = I_LUI(T6, 0x8000u);
    w[n++] = I_ADDIU(T1, 0u, 0u);
    w[n++] = I_ADDIU(T2, 0u, 1000u);
    w[n++] = I_LW(T3, 0x200u, T6);
    w[n++] = I_ADDIU(T1, T1, 1u);
    w[n++] = I_ADDIU(T3, T3, 1u);
    w[n++] = I_SW(T3, 0x200u, T6);
    w[n++] = 0x14000000u | (T1 << 21) | (T2 << 16) | ((uint32_t)(-5) & 0xFFFFu); /* bne t1, t2, loop */
    w[n++] = I_NOP;
    w[n++] = I_ADDIU(T9, 0u, 1u);
    w[n++] = I_BEQ(0u, 0u, -1);
    w[n++] = I_NOP;
    return n;
}

/* The cycles the work loop takes to its marker, with a looping list of the
   given node count running from the start of the loop (0 for none). */
static uint64_t work_cycles(uint32_t nodes)
{
    uint32_t w[16];
    uint32_t n = put_work_loop(w);
    schway *inst = make();
    load(inst, w, n);
    run_to_entry(inst);
    uint64_t entry = inst->ticks;
    if (nodes != 0u) {
        start_loop_list(inst, nodes);
    }
    uint64_t marker = 0u;
    for (uint32_t i = 0u; i < 200000u && marker == 0u; i++) {
        if (inst->cpu.r[T9] != 0u) {
            marker = inst->ticks;
        } else {
            (void)run(inst, 1u);
        }
    }
    CHECK(marker != 0u);
    CHECK_EQ_U64(inst->cpu.r[T1], 1000u);
    schway_destroy(inst);
    return marker - entry;
}

/* Fault prevented: a list that takes the CPU's time at a rate unlike the
   hardware's. In the public chain-looping log a 1,000-pass loop takes 16,040
   cycles alone and 25,632 and 25,640 next to a self-referencing list of one and
   two nodes, a slowdown of about 1.6 times. The model is fitted to that one log,
   so the test is a band: from 1.4 to 1.8 times for each list. The ratio is
   printed. */
static void case_list_cpu_band(void)
{
    uint64_t alone = work_cycles(0u);
    CHECK(alone > 1000u);
    for (uint32_t nodes = 1u; nodes <= 2u; nodes++) {
        uint64_t with = work_cycles(nodes);
        uint64_t ratio = with * 1000u / alone;
        printf("list_cpu_band: %u node(s): %llu cycles against %llu alone, ratio %llu per mille\n",
               (unsigned)nodes, (unsigned long long)with, (unsigned long long)alone,
               (unsigned long long)ratio);
        CHECK(ratio >= 1400u && ratio <= 1800u);
    }
}

static schway_run_result run_frame(schway *inst)
{
    schway_run_result r;
    memset(&r, 0, sizeof r);
    r.size = (uint32_t)sizeof r;
    CHECK_EQ_U64(schway_run_frame(inst, &r), SCHWAY_STATUS_OK);
    return r;
}

/* The state saves, loads into a second instance and saves again to the same
   bytes: the engine's fields, whatever the program did to them, are inside the
   bounds a load enforces. */
static void check_state_round_trip(schway *inst)
{
    size_t n = schway_state_size(inst);
    uint8_t *a = (uint8_t *)malloc(n);
    uint8_t *b = (uint8_t *)malloc(n);
    if (a == NULL || b == NULL) {
        exit(1);
    }
    schway *other = make();
    CHECK_EQ_U64(schway_save_state(inst, a, n), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_load_state(other, a, n), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_save_state(other, b, n), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(memcmp(a, b, n), 0);
    schway_destroy(other);
    free(a);
    free(b);
}

static void compare_states(const schway *left, const schway *right)
{
    size_t n = schway_state_size(left);
    CHECK_EQ_U64(schway_state_size(right), n);
    uint8_t *a = (uint8_t *)malloc(n);
    uint8_t *b = (uint8_t *)malloc(n);
    if (a == NULL || b == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(schway_save_state(left, a, n), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_save_state(right, b, n), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(memcmp(a, b, n), 0);
    free(a);
    free(b);
}

static void spu_prepare(schway *inst, uint32_t madr, uint32_t blocks, uint32_t words,
                        uint16_t attr, uint32_t dpcr)
{
    wr16(inst, 0x1F801DAAu, attr);
    wr32(inst, DPCR, dpcr);
    wr32(inst, DICR, 0x00900000u);
    wr32(inst, MADR(4u), madr);
    wr32(inst, BCR(4u), (blocks << 16) | words);
}

/* Fault prevented: a DMA4 request producer that moves on other ATTR modes,
   wrong direction or a disabled channel; a stop or mode change that breaks an
   atomic unit; a wake that bypasses a future read-ahead deadline; or a saved
   waiting/in-flight state that differs from the same uninterrupted run. */
static void case_spu_request_controls(void)
{
    static const uint16_t inactive_modes[] = {0xA000u, 0xA010u, 0xA030u};
    const uint32_t seed = 0x4B000001u;
    for (size_t i = 0u; i < sizeof inactive_modes / sizeof inactive_modes[0]; i++) {
        schway *inst = make();
        fill_ram(inst, 0x8000u, 16u, seed);
        spu_prepare(inst, 0x8000u, 1u, 16u, inactive_modes[i], DPCR_ALL);
        wr32(inst, CHCR(4u), 0x01000201u);
        CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
        CHECK_EQ_U64(inst->dma.words[4], 0u);
        inst->cpu.block_end = 0u;
        schway__dma_bus_access(inst, now_of(inst));
        CHECK_EQ_U64(inst->cpu.block_end & SCHWAY__BLOCK_EVENT, 0u);
        schway_destroy(inst);
    }

    /* ATTR in DMA-write mode cannot request a direction-to-RAM transfer. */
    schway *inst = make();
    fill_ram(inst, 0x9000u, 16u, seed);
    uint32_t first_word = ram_word(inst, 0x9000u);
    spu_prepare(inst, 0x9000u, 1u, 16u, 0x0020u, DPCR_ALL);
    wr32(inst, CHCR(4u), 0x01000200u);
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    CHECK_EQ_U64(inst->dma.words[4], 0u);
    CHECK_EQ_U64((inst->dma.dreq >> 4) & 1u, 0u);
    CHECK_EQ_HEX(ram_word(inst, 0x9000u), first_word);
    schway_destroy(inst);

    /* A disabled DPCR holds the request; enabling it wakes the existing work. */
    inst = make();
    fill_ram(inst, 0xA000u, 16u, seed);
    spu_prepare(inst, 0xA000u, 1u, 16u, 0x0020u, 0x07654321u);
    wr32(inst, CHCR(4u), 0x01000201u);
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    CHECK_EQ_U64(inst->dma.words[4], 0u);
    wr32(inst, DPCR, DPCR_ALL);
    CHECK_EQ_U64(inst->dma.cur, 4u);
    CHECK_EQ_U64(inst->dma.words[4], 16u);
    schway_destroy(inst);

    /* A waiting channel stopped before its first unit never starts when ATTR
       later changes; a stop during an atomic block takes effect at its end. */
    inst = make();
    fill_ram(inst, 0xB000u, 48u, seed);
    spu_prepare(inst, 0xB000u, 3u, 16u, 0x0010u, DPCR_ALL);
    wr32(inst, CHCR(4u), 0x01000201u);
    wr32(inst, CHCR(4u), 0u);
    CHECK_EQ_U64(inst->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY, 0u);
    wr16(inst, 0x1F801DAAu, 0x0020u);
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    CHECK_EQ_U64(inst->dma.words[4], 0u);
    schway_destroy(inst);

    inst = make();
    fill_ram(inst, 0xC000u, 48u, seed);
    spu_prepare(inst, 0xC000u, 3u, 16u, 0x0020u, DPCR_ALL);
    uint64_t start = now_of(inst);
    wr32(inst, CHCR(4u), 0x01000201u);
    CHECK_EQ_U64(inst->dma.words[4], 16u);
    wr32(inst, CHCR(4u), 0u);
    CHECK((inst->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY) != 0u);
    CHECK_EQ_U64(inst->dma.phase[4], 5u);
    check_state_round_trip(inst);
    schway__dma_event(inst, start + 76u);
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    CHECK_EQ_U64(inst->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY, 0u);
    CHECK_EQ_U64(inst->dma.words[4], 16u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 0u);
    check_state_round_trip(inst);
    schway_destroy(inst);

    /* ATTR deassertion cannot split the already moved block. Reasserting it
       at the next block boundary starts exactly there, with no accumulated
       request credit. */
    schway *split = make();
    fill_ram(split, 0xD000u, 48u, seed);
    spu_prepare(split, 0xD000u, 3u, 16u, 0x0020u, DPCR_ALL);
    start = now_of(split);
    wr32(split, CHCR(4u), 0x01000201u);
    wr16_at(split, 0x1F801DAAu, 0x0010u, start + 1u);
    CHECK_EQ_U64(split->dma.words[4], 16u);
    schway__dma_event(split, start + 75u);
    CHECK_EQ_U64(split->dma.words[4], 16u);
    schway__dma_event(split, start + 76u);
    CHECK_EQ_U64(split->dma.cur, 4u);
    CHECK_EQ_U64(split->dma.words[4], 16u);
    CHECK((split->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY) != 0u);
    CHECK_EQ_U64(split->dma.dicr & 0x10000000u, 0u);
    CHECK_EQ_U64(split->dma.phase[4], 3u);
    check_state_round_trip(split);
    wr16_at(split, 0x1F801DAAu, 0x0020u, start + 76u);
    uint64_t split_second_request = split->spu.next_tick;
    CHECK_EQ_U64(split->dma.cur, 4u);
    CHECK_EQ_U64(split->dma.busy_until, split_second_request);
    spu_dma_event(split, split_second_request);
    CHECK_EQ_U64(split->dma.words[4], 32u);
    CHECK_EQ_U64(split->dma.busy_until, split_second_request + 76u);
    schway__dma_event(split, split_second_request + 76u);
    uint64_t split_third_request = split->spu.next_tick;
    CHECK_EQ_U64(split->dma.busy_until, split_third_request);
    spu_dma_event(split, split_third_request);
    CHECK_EQ_U64(split->dma.words[4], 48u);
    CHECK_EQ_U64(split->dma.busy_until, split_third_request + 76u);
    schway__dma_event(split, split_third_request + 76u);
    CHECK_EQ_U64(split->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY, 0u);
    CHECK_EQ_U64(split->dma.words[4], 48u);
    CHECK_EQ_HEX(split->dma.crc[4], pattern_crc(48u, seed));
    CHECK_EQ_U64(split->irq.stat & 8u, 8u);
    schway_destroy(split);

    /* A DMA-register read can finish one block ahead of the clock. Neither an
       ATTR callback nor another request callback may bypass the saved SPU
       sample deadline; readiness starts one unit at each later deadline. */
    inst = make();
    fill_ram(inst, 0xE000u, 48u, seed);
    spu_prepare(inst, 0xE000u, 3u, 16u, 0x0020u, DPCR_ALL);
    start = now_of(inst);
    wr32(inst, CHCR(4u), 0x01000201u);
    CHECK_EQ_U64(inst->dma.words[4], 16u);
    CHECK((rd32(inst, CHCR(4u)) & SCHWAY__DMA_CHCR_BUSY) != 0u);
    CHECK_EQ_U64(inst->dma.cur, 4u);
    CHECK_EQ_U64(inst->dma.phase[4], 3u);
    CHECK_EQ_U64(inst->dma.busy_until, inst->spu.next_tick);
    CHECK_EQ_U64(inst->dma.words[4], 16u);
    check_state_round_trip(inst);
    wr16_at(inst, 0x1F801DAAu, 0x0020u, start + 1u);
    CHECK_EQ_U64(inst->dma.cur, 4u);
    CHECK_EQ_U64(inst->dma.busy_until, inst->spu.next_tick);
    schway__dma_dreq(inst, 4u, 1u);
    CHECK_EQ_U64(inst->dma.cur, 4u);
    CHECK_EQ_U64(inst->dma.words[4], 16u);
    CHECK_EQ_U64((inst->dma.dreq >> 4) & 1u, 1u);
    uint64_t read_second_request = inst->spu.next_tick;
    schway__dma_event(inst, read_second_request - 1u);
    CHECK_EQ_U64(inst->dma.words[4], 16u);
    spu_dma_event(inst, read_second_request);
    CHECK_EQ_U64(inst->dma.cur, 4u);
    CHECK_EQ_U64(inst->dma.words[4], 32u);
    CHECK_EQ_U64(inst->dma.busy_until, read_second_request + 76u);
    schway__dma_event(inst, read_second_request + 76u);
    uint64_t read_third_request = inst->spu.next_tick;
    CHECK_EQ_U64(inst->dma.busy_until, read_third_request);
    spu_dma_event(inst, read_third_request);
    CHECK_EQ_U64(inst->dma.words[4], 48u);
    CHECK_EQ_U64(inst->dma.busy_until, read_third_request + 76u);
    schway__dma_event(inst, read_third_request + 76u);
    CHECK_EQ_U64(inst->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY, 0u);
    CHECK_EQ_HEX(inst->dma.crc[4], pattern_crc(48u, seed));
    schway_destroy(inst);

    /* 757 independently judged blocks each cost 76 cycles and wait for the
       next 768-cycle SPU sample deadline, with the final write before finish. */
    inst = make();
    const uint32_t words = 757u * 16u;
    fill_ram(inst, 0x10000u, words, seed);
    spu_prepare(inst, 0x10000u, 757u, 16u, 0x0020u, DPCR_ALL);
    start = now_of(inst);
    wr32(inst, CHCR(4u), 0x01000201u);
    CHECK_EQ_U64(inst->dma.words[4], 16u);
    CHECK_EQ_U64(inst->dma.busy_until - start, 76u);
    spu_dma_until_words(inst, words);
    CHECK_EQ_U64(inst->dma.cur, 4u);
    CHECK_EQ_U64(inst->dma.words[4], words);
    CHECK_EQ_HEX(inst->dma.madr[4], 0x10000u + words * 4u);
    uint64_t final_end = start + 756u * 768u + 76u;
    CHECK_EQ_U64(inst->dma.busy_until, final_end);
    CHECK_EQ_HEX(inst->dma.crc[4], pattern_crc(words, seed));
    CHECK((inst->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY) != 0u);
    CHECK_EQ_U64(inst->dma.dicr & 0x10000000u, 0u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 0u);
    schway__dma_event(inst, final_end);
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    CHECK_EQ_U64(inst->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY, 0u);
    CHECK_EQ_HEX(rd32(inst, DICR), 0x90900000u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 8u);
    schway_destroy(inst);

    /* Splitting event time does not change serialized output; the state helper
       also checks that the complete engine remains loadable. */
    split = make();
    schway *whole = make();
    fill_ram(split, 0x18000u, 48u, seed);
    fill_ram(whole, 0x18000u, 48u, seed);
    spu_prepare(split, 0x18000u, 3u, 16u, 0x0020u, DPCR_ALL);
    spu_prepare(whole, 0x18000u, 3u, 16u, 0x0020u, DPCR_ALL);
    start = now_of(split);
    wr32(split, CHCR(4u), 0x01000201u);
    wr32(whole, CHCR(4u), 0x01000201u);
    schway__dma_event(split, start + 75u);
    spu_dma_until_words(split, 48u);
    spu_dma_until_words(whole, 48u);
    schway__dma_event(split, split->dma.busy_until);
    schway__dma_event(whole, whole->dma.busy_until);
    compare_states(split, whole);
    check_state_round_trip(split);
    check_state_round_trip(whole);
    schway_destroy(whole);
    schway_destroy(split);

    /* Independent ATTR latches and readiness never leak across instances. */
    schway *active = make();
    schway *waiting = make();
    fill_ram(active, 0x20000u, 16u, seed);
    fill_ram(waiting, 0x20000u, 16u, seed);
    spu_prepare(active, 0x20000u, 1u, 16u, 0x0020u, DPCR_ALL);
    spu_prepare(waiting, 0x20000u, 1u, 16u, 0x0010u, DPCR_ALL);
    start = now_of(active);
    wr32(active, CHCR(4u), 0x01000201u);
    wr32(waiting, CHCR(4u), 0x01000201u);
    CHECK_EQ_U64(active->dma.words[4], 16u);
    CHECK_EQ_U64(waiting->dma.words[4], 0u);
    schway__dma_event(active, start + 76u);
    CHECK_EQ_U64(active->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY, 0u);
    CHECK_EQ_U64(waiting->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY, SCHWAY__DMA_CHCR_BUSY);
    CHECK_EQ_U64(waiting->dma.words[4], 0u);
    CHECK_EQ_U64(schway__spu_dma_ready(active, 1u), 1u);
    CHECK_EQ_U64(schway__spu_dma_ready(waiting, 1u), 0u);
    schway_destroy(waiting);
    schway_destroy(active);
}

/* All seven channels written with BCR = FFFFFFFFh in a mode they can run, in
   channel order, and started: MDEC in from RAM, MDEC out to RAM, the GPU
   walking a list whose first header names itself, the CD-ROM to RAM, the SPU
   from RAM, PIO to RAM and OTC. The addresses leave room for 10000h words each
   and keep the list's header clear of the others. */
static void hostile_start(schway *inst)
{
    static const uint32_t chcr[7] = {0x11000001u, 0x11000000u, 0x01000401u, 0x11000000u,
                                     0x11000001u, 0x11000000u, 0x11000000u};
    wr32(inst, 0x200u, 0x00000200u);
    wr32(inst, DPCR, DPCR_ALL);
    wr32(inst, DICR, 0x00FF0000u);
    for (uint32_t ch = 0u; ch < 7u; ch++) {
        uint32_t madr = ch == 2u ? 0x200u : ch == 6u ? OTC_TOP : 0x20000u + 0x40000u * ch;
        wr32(inst, MADR(ch), madr);
        wr32(inst, BCR(ch), 0xFFFFFFFFu);
    }
    for (uint32_t ch = 0u; ch < 7u; ch++) {
        wr32(inst, CHCR(ch), chcr[ch]);
    }
    CHECK_EQ_U64(inst->dma.cur, 0u);
}

/* Fault prevented: a program that sets every channel to its largest transfer,
   or a list that never ends, stretching a frame, hanging a run, or leaving
   engine fields a load would refuse. Four frames each end with the frame stop
   within the frame's own length plus the longest unit, and the state after
   each saves and loads. The same four frames run with no program, where time
   moves only to the next deadline, and with a program that reads RAM in a loop,
   which every running unit holds, where a hold that crosses the frame's end
   ends the frame at that instruction. */
static void case_hostile_bcr(void)
{
    for (uint32_t with_program = 0u; with_program < 2u; with_program++) {
        schway *inst = make();
        if (with_program != 0u) {
            uint32_t w[4];
            w[0] = I_LUI(T6, 0x8000u);
            w[1] = I_LW(T5, 0x100u, T6);
            w[2] = I_BEQ(0u, 0u, -2);
            w[3] = I_NOP;
            load(inst, w, 4u);
            run_to_entry(inst);
        }
        hostile_start(inst);
        for (uint32_t f = 0u; f < 4u; f++) {
            uint64_t end = inst->frame_end;
            uint64_t before = inst->ticks;
            schway_run_result r = run_frame(inst);
            CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
            CHECK(r.ticks >= end);
            CHECK(r.ticks - before <= end - before + SCHWAY__DMA_BUSY_MAX);
            CHECK(r.ticks <= end + SCHWAY__INSN_CYCLES_MAX);
            CHECK_EQ_U64(r.fault_present, 0u);
            CHECK(inst->dma.busy_until <= inst->ticks + SCHWAY__DMA_BUSY_MAX);
            CHECK(schway__dma_next(inst) > inst->ticks);
            check_state_round_trip(inst);
        }
        /* The list is the channel that never ends: busy, no flag. */
        CHECK((inst->dma.chcr[2] & SCHWAY__DMA_CHCR_BUSY) != 0u);
        CHECK_EQ_U64(inst->dma.dicr & 0x04000000u, 0u);
        schway_destroy(inst);
    }
}

/* Fault prevented: a unit in flight at the frame's end that is lost, repeated
   or finished early by the frame boundary. The CD-ROM channel moves 10000h
   words, which take about three frames, and the SPU channel waits behind it.
   After the first frame the unit is still in flight; the end state is the same
   bytes whether the time was run as four frames or as slices of 100,003 cycles
   (a run never crosses a frame boundary, so it stops at each). */
static void case_preempt_carry(void)
{
    schway *whole = make();
    schway *sliced = make();
    schway *both[2] = {whole, sliced};
    for (uint32_t i = 0u; i < 2u; i++) {
        schway *inst = both[i];
        wr32(inst, DPCR, DPCR_ALL);
        wr32(inst, DICR, 0x00FF0000u);
        wr32(inst, MADR(3u), 0x40000u);
        wr32(inst, BCR(3u), 0u);
        wr32(inst, MADR(4u), 0x100000u);
        wr32(inst, BCR(4u), 4096u);
        wr32(inst, CHCR(3u), 0x11000000u);
        wr32(inst, CHCR(4u), 0x11000001u);
    }
    schway_run_result r = run_frame(whole);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    CHECK_EQ_U64(whole->dma.cur, 3u);
    CHECK(whole->dma.busy_until > whole->ticks);
    CHECK((whole->dma.chcr[3] & SCHWAY__DMA_CHCR_BUSY) != 0u);
    CHECK_EQ_U64(whole->dma.words[4], 0u);
    for (uint32_t f = 0u; f < 3u; f++) {
        (void)run_frame(whole);
    }
    CHECK_EQ_U64(whole->dma.chcr[3] & SCHWAY__DMA_CHCR_BUSY, 0u);
    CHECK_EQ_U64(whole->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY, 0u);
    CHECK_EQ_U64(whole->dma.words[4], 4096u);
    CHECK_EQ_HEX(whole->dma.dicr & 0x7F000000u, 0x18000000u);
    while (sliced->ticks < whole->ticks) {
        uint64_t left = whole->ticks - sliced->ticks;
        (void)run(sliced, left < 100003u ? left : 100003u);
    }
    CHECK_EQ_U64(sliced->ticks, whole->ticks);
    size_t n = schway_state_size(whole);
    uint8_t *a = (uint8_t *)malloc(n);
    uint8_t *b = (uint8_t *)malloc(n);
    if (a == NULL || b == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(schway_save_state(whole, a, n), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_save_state(sliced, b, n), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(memcmp(a, b, n), 0);
    free(a);
    free(b);
    schway_destroy(whole);
    schway_destroy(sliced);
}

/* One pass of the register sweep: every offset of 1F801080h to 1F8010FFh at
   widths 1, 2 and 4 with four edge values, each from a power reset, written,
   read back, run for a while and read again. Returns the CRC-32 of everything
   seen. The CHCR word FFFFFFFFh of a channel below 6 selects the reserved sync
   mode with the start bit, which latches the DMA fault; every other access
   latches no fault. */
static uint32_t sweep_pass(schway *inst)
{
    static const uint32_t widths[3] = {1u, 2u, 4u};
    static const uint32_t values[4] = {0x00000000u, 0xFFFFFFFFu, 0x55555555u, 0xAAAAAAAAu};
    uint32_t crc = 0u;
    for (uint32_t off = 0x80u; off < 0x100u; off++) {
        for (uint32_t w = 0u; w < 3u; w++) {
            for (uint32_t k = 0u; k < 4u; k++) {
                uint32_t addr = 0xBF801000u + off;
                uint32_t v = values[k];
                /* The one access that may latch the DMA fault: the word
                   FFFFFFFFh into a CHCR below OTC's, at any byte offset in it. */
                uint32_t may_fault = (off & 0xCu) == 8u && off < 0xE0u && widths[w] == 4u &&
                                             v == 0xFFFFFFFFu
                                         ? 1u
                                         : 0u;
                CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
                uint32_t cycles = 0u;
                schway__sched_sync(inst);
                CHECK_EQ_U64(schway__bus_write(inst, addr, widths[w], v, &cycles), 0u);
                CHECK(schway__dma_next(inst) > now_of(inst));
                uint32_t seen[3];
                for (uint32_t pass = 0u; pass < 2u; pass++) {
                    uint32_t rd = 0u;
                    cycles = 0u;
                    CHECK_EQ_U64(schway__bus_read(inst, addr, widths[w], &rd, &cycles), 0u);
                    seen[pass] = rd;
                    (void)run(inst, 3000u);
                }
                seen[2] = inst->fault.kind;
                CHECK(inst->fault.kind == SCHWAY__FAULT_NONE ||
                      (may_fault != 0u && inst->fault.kind == SCHWAY__FAULT_DMA));
                CHECK(schway__dma_next(inst) > inst->ticks);
                crc = schway__crc32_update(crc, (const uint8_t *)seen, sizeof seen);
                crc = schway__crc32_update(crc, (const uint8_t *)&inst->dma, sizeof inst->dma);
                crc = schway__crc32_update(crc, (const uint8_t *)&inst->ticks, sizeof inst->ticks);
            }
        }
    }
    return crc;
}

/* Fault prevented: a DMA register access, at any offset, width or edge value,
   that faults, hangs, leaves the engine with an unbounded deadline or answers
   differently the second time. Run under the sanitizer lane, an access that
   reads or writes outside the state shows up here. */
static void case_regs_sweep(void)
{
    schway *inst = make();
    uint32_t first = sweep_pass(inst);
    uint32_t second = sweep_pass(inst);
    CHECK_EQ_HEX(first, second);
    schway_destroy(inst);
}

/* Fault prevented: a CHCR write that selects the reserved sync mode (3) on a
   channel other than OTC and goes unreported, starts the channel anyway or
   leaves it busy for ever. The write with the start bits latches the first fault
   of kind DMA at the CHCR address with the value written and the CPU's program
   counter, nothing moves and the channel is not busy; the same mode without the
   start bit, and OTC with the same bits, latch nothing; a later write of a
   usable mode starts the channel; and a second hostile write leaves the first
   record alone. */
static void case_fault_sync3(void)
{
    schway *inst = make();
    wr32(inst, DPCR, DPCR_ALL);
    wr32(inst, MADR(2u), 0x1000u);
    wr32(inst, BCR(2u), 64u);
    wr32(inst, CHCR(2u), 0x00000601u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    schway_destroy(inst);

    inst = make();
    (void)otc_start(inst, OTC_TOP, 4u, 0x11000602u);
    CHECK_EQ_U64(inst->dma.cur, 6u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    schway_destroy(inst);

    inst = make();
    fill_ram(inst, 0x1000u, 64u, 0xB5000001u);
    wr32(inst, DPCR, DPCR_ALL);
    wr32(inst, DICR, 0x00840000u);
    wr32(inst, MADR(2u), 0x1000u);
    wr32(inst, BCR(2u), 64u);
    wr32(inst, CHCR(2u), 0x11000601u);
    schway_fault f;
    memset(&f, 0, sizeof f);
    f.size = (uint32_t)sizeof f;
    CHECK_EQ_U64(schway_get_fault(inst, &f), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(f.kind, SCHWAY_FAULT_DMA);
    CHECK_EQ_U64(f.device, SCHWAY_DEVICE_DMA);
    CHECK_EQ_U64(f.width, 4u);
    CHECK_EQ_HEX(f.addr, CHCR(2u));
    CHECK_EQ_HEX(f.opcode, 0x11000601u);
    CHECK_EQ_HEX(f.pc, inst->cpu.pc);
    CHECK_EQ_U64(inst->dma.chcr[2] & (SCHWAY__DMA_CHCR_BUSY | 0x10000000u), 0u);
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    CHECK_EQ_U64(inst->dma.words[2], 0u);
    CHECK_EQ_HEX(inst->dma.madr[2], 0x1000u);
    CHECK(schway__dma_next(inst) > now_of(inst));
    (void)run(inst, 1000u);
    CHECK_EQ_U64(inst->dma.words[2], 0u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 0u);
    CHECK_EQ_HEX(rd32(inst, CHCR(2u)) & SCHWAY__DMA_CHCR_BUSY, 0u);

    /* A second hostile write on another channel keeps the first record. */
    wr32(inst, CHCR(4u), 0x11000601u);
    CHECK_EQ_U64(schway_get_fault(inst, &f), SCHWAY_STATUS_OK);
    CHECK_EQ_HEX(f.addr, CHCR(2u));

    /* The same channel then takes a usable mode and moves its words. */
    wr32(inst, 0x1F801814u, 0x04000001u);
    wr32(inst, 0x1000u, 0xA0000000u);
    wr32(inst, 0x1004u, (10u << 16) | 10u);
    wr32(inst, 0x1008u, 0x00010001u);
    wr32(inst, 0x100Cu, 0x00007C1Fu);
    wr32(inst, BCR(2u), 4u);
    wr32(inst, CHCR(2u), 0x11000001u);
    CHECK_EQ_U64(inst->dma.cur, 2u);
    CHECK_EQ_HEX(inst->vram[10][10], 0x7C1Fu);
    (void)run(inst, 1000u);
    CHECK_EQ_U64(inst->dma.chcr[2] & SCHWAY__DMA_CHCR_BUSY, 0u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 8u);
    schway_destroy(inst);

    /* Replacing a live transfer with reserved sync mode must fail closed.
       The accepted model latches the write, lets only the current unit end,
       then stops without completing or starting a reserved-mode transfer. */
    inst = make();
    fill_ram(inst, 0x5000u, 64u, 0x64000001u);
    wr32(inst, DPCR, DPCR_ALL);
    wr32(inst, DICR, 0x00900000u);
    wr32(inst, MADR(4u), 0x5000u);
    wr32(inst, BCR(4u), 64u);
    wr32(inst, CHCR(4u), 0x11000001u);
    uint64_t unit_end = inst->dma.busy_until;
    wr32(inst, CHCR(4u), 0x11000601u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_DMA);
    CHECK_EQ_HEX(inst->fault.addr, CHCR(4u));
    CHECK_EQ_HEX(inst->fault.opcode, 0x11000601u);
    CHECK_EQ_U64(inst->dma.cur, 4u);
    CHECK_EQ_U64(inst->dma.busy_until, unit_end);
    CHECK_EQ_HEX(inst->dma.chcr[4] & 0x600u, 0u);
    schway__dma_event(inst, unit_end);
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    CHECK_EQ_U64(inst->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY, 0u);
    CHECK_EQ_U64(inst->dma.dicr & (1u << 28), 0u);
    CHECK_EQ_U64(inst->irq.stat & 8u, 0u);
    schway_destroy(inst);
}

/* Two channels armed at their longest, then a read of a DMA register while the
   first runs: the read sees the first unit complete, and the engine's state
   still saves, loads and saves again to the same bytes (the second unit starts
   at the first's end, not at the read), then both transfers finish in order. */
static void case_read_chain_saves(void)
{
    schway *inst = make();
    wr32(inst, DPCR, DPCR_ALL);
    wr32(inst, DICR, 0x00FF0000u);
    wr32(inst, MADR(3u), 0x20000u);
    wr32(inst, BCR(3u), 0u);
    wr32(inst, CHCR(3u), 0x11000000u);
    wr32(inst, MADR(5u), 0x80000u);
    wr32(inst, BCR(5u), 0u);
    wr32(inst, CHCR(5u), 0x11000000u);
    CHECK_EQ_U64(inst->dma.cur, 3u);
    uint64_t first_end = inst->dma.busy_until;
    CHECK_EQ_U64(rd32(inst, CHCR(3u)) & SCHWAY__DMA_CHCR_BUSY, 0u);
    CHECK(inst->dma.busy_until <= inst->ticks + SCHWAY__DMA_BUSY_MAX);
    CHECK(inst->sched.next <= first_end);
    check_state_round_trip(inst);
    /* The waiting channel starts at the first unit's end and is still busy
       after it, and both finish with their flags. */
    for (uint32_t i = 0u; i < 16u && inst->ticks <= first_end; i++) {
        (void)run(inst, first_end + 1000u);
    }
    CHECK_EQ_U64(inst->dma.cur, 5u);
    CHECK(inst->dma.busy_until > inst->ticks);
    check_state_round_trip(inst);
    for (uint32_t i = 0u; i < 16u && inst->dma.cur != SCHWAY__DMA_NONE; i++) {
        (void)run(inst, 3000000u);
    }
    CHECK_EQ_U64(inst->dma.cur, SCHWAY__DMA_NONE);
    CHECK_EQ_U64((inst->dma.chcr[3] | inst->dma.chcr[5]) & SCHWAY__DMA_CHCR_BUSY, 0u);
    CHECK_EQ_HEX(inst->dma.dicr & 0x2A000000u, 0x28000000u);
    schway_destroy(inst);
}

/* The cycles a read of an engine's result costs once a DMA unit has held the CPU
   for thousands of cycles: the program enables the GTE, issues the word issue
   (a GTE command or a divide, or a nop for the control), starts OTC, loads from
   the I/O page (the load waits for the unit) and then runs the word read, whose
   cost is measured from the tick it is next to the tick it retires. */
static uint64_t engine_read_cost(uint32_t issue, uint32_t read)
{
    uint32_t w[24];
    uint32_t n = put_setup(w, DPCR_OTC);
    w[n++] = I_LUI(15u, 0x4000u);
    w[n++] = 0x40806000u | (15u << 16);
    w[n++] = issue;
    w[n++] = I_SW(T4, 0xE8u, T0);
    w[n++] = I_LW(T5, 0xE8u, T0);
    const uint32_t read_pc = TEXT + 4u * n;
    w[n++] = read;
    w[n++] = I_ADDIU(T9, 0u, 1u);
    w[n++] = I_BEQ(0u, 0u, -1);
    w[n++] = I_NOP;
    schway *inst = make();
    load(inst, w, n);
    run_to_entry(inst);
    for (uint32_t i = 0; i < 20000u; i++) {
        if (inst->cpu.pc == read_pc && inst->dma.stall_until <= inst->ticks) {
            break;
        }
        (void)run(inst, 1u);
    }
    CHECK_EQ_HEX(inst->cpu.pc, read_pc);
    uint64_t before = inst->ticks;
    (void)run(inst, 1u);
    uint64_t cost = inst->ticks - before;
    schway_destroy(inst);
    return cost;
}

/* Fault prevented: a DMA stall that does not count in the CPU's cycle count, so
   that the GTE's and the multiply unit's busy stamps do not run down while the
   bus holds the CPU and a read of their result, long after the command, still
   waits the rest of its latency. */
static void case_stall_decays_engines(void)
{
    /* NCDT, 44 cycles, then MFC2 of IR1; DIV, 36 cycles, then MFLO. */
    const uint32_t ncdt = 0x4A000016u;
    const uint32_t mfc2 = 0x48000000u | (24u << 16) | (9u << 11);
    const uint32_t div = (T4 << 21) | (T6 << 16) | 0x1Au;
    const uint32_t mflo = (24u << 11) | 0x12u;
    uint64_t control = engine_read_cost(I_NOP, mfc2);
    CHECK(control >= 1u && control < 10u);
    CHECK_EQ_U64(engine_read_cost(ncdt, mfc2), control);
    control = engine_read_cost(I_NOP, mflo);
    CHECK(control >= 1u && control < 10u);
    CHECK_EQ_U64(engine_read_cost(div, mflo), control);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"fault_sync3", case_fault_sync3},
    {"hostile_bcr", case_hostile_bcr},
    {"preempt_carry", case_preempt_carry},
    {"regs_sweep", case_regs_sweep},
    {"list_self_ref", case_list_self_ref},
    {"list_stop", case_list_stop},
    {"chopping_band", case_chopping_band},
    {"list_cpu_band", case_list_cpu_band},
    {"list_walk", case_list_walk},
    {"list_independent_yields", case_list_independent_yields},
    {"list_empty", case_list_empty},
    {"sync0_gpu_sink", case_sync0_gpu_sink},
    {"gpu_linked_list_readback", case_gpu_linked_list_readback},
    {"sync0_from_device", case_sync0_from_device},
    {"sync1_dreq_blocks", case_sync1_dreq_blocks},
    {"spu_request_blocks", case_spu_request_blocks},
    {"spu_sync0_starts_without_trigger", case_spu_sync0_starts_without_trigger},
    {"spu_request_controls", case_spu_request_controls},
    {"sync1_no_dreq_no_stall", case_sync1_no_dreq_no_stall},
    {"costs_per_channel", case_costs_per_channel},
    {"dpcr_rearbitrate", case_dpcr_rearbitrate},
    {"otc_clear", case_otc_clear},
    {"dpcr_reset", case_dpcr_reset},
    {"dpcr_priority", case_dpcr_priority},
    {"dpcr_enable", case_dpcr_enable},
    {"dicr_reads", case_dicr_reads},
    {"dicr_irq_edge", case_dicr_irq_edge},
    {"otc_readback", case_otc_readback},
    {"otc_madr_wrap", case_otc_madr_wrap},
    {"otc_needs_trigger", case_otc_needs_trigger},
    {"otc_ignores_bits", case_otc_ignores_bits},
    {"otc_bcr_zero", case_otc_bcr_zero},
    {"step_contract", case_step_contract},
    {"stall_cpu_access", case_stall_cpu_access},
    {"stall_not_scratch", case_stall_not_scratch},
    {"stall_write_queue", case_stall_write_queue},
    {"otc_read_sees_completion", case_otc_read_sees_completion},
    {"read_chain_saves", case_read_chain_saves},
    {"stall_decays_engines", case_stall_decays_engines},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-dma <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "schwaystation-test-dma: unknown case %s\n", argv[1]);
    return 2;
}
