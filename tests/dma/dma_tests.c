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

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"otc_clear", case_otc_clear},
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
