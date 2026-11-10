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
   Channels 2 and 5 wait with sync mode 0 and the start trigger; nothing
   starts them yet, so the pick is the arbitration alone. */
static void case_dpcr_priority(void)
{
    schway *inst = make();
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

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
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
