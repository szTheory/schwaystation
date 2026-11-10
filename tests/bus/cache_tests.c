/* The instruction cache, cache isolation and the write queue through the CPU:
   schwaystation-test-cache <case>. White-box like the other bus tests: the test
   places words in RAM, runs them with schway__cpu_step_one and no replay log,
   and reads the cache arrays, the queue count, the registers and the cycle
   counter. The expected behaviour is the documented one (PSX-SYSTEM section 2,
   I-cache and Cache isolation, write queue). */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

#define NOP 0u
#define ADDIU(rt, rs, imm) (0x24000000u | ((rs) << 21) | ((rt) << 16) | ((imm) & 0xFFFFu))
#define LW(rt, base, off) (0x8C000000u | ((base) << 21) | ((rt) << 16) | ((off) & 0xFFFFu))
#define LBU(rt, base, off) (0x90000000u | ((base) << 21) | ((rt) << 16) | ((off) & 0xFFFFu))
#define SB(rt, base, off) (0xA0000000u | ((base) << 21) | ((rt) << 16) | ((off) & 0xFFFFu))
#define SW(rt, base, off) (0xAC000000u | ((base) << 21) | ((rt) << 16) | ((off) & 0xFFFFu))
#define SYSCALL 0x0000000Cu

#define SR_ISC 0x00010000u
#define CTRL_TAG_IS1 0x804u
#define KSEG0 0x80000000u
#define KSEG1 0xA0000000u
#define CACHE_CTRL 0xFFFE0130u
#define VECTOR_RAM 0x80000080u

/* Code that runs from cached memory, one instruction per case word, and the data
   the stores and loads use. Their lines are different indexes of the cache. */
#define CODE (KSEG0 | 0x4000u)
#define DATA (KSEG0 | 0x10000u)

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

/* A kernel-mode CPU with interrupts off and the RAM exception vectors. */
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
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = 0u;
    return inst;
}

/* One word of RAM, written and read straight on the bus, so the cache and the
   queue never see it. */
static void put(schway *inst, uint32_t addr, uint32_t word)
{
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, KSEG1 | (addr & 0x1FFFFFFFu), 4u, word, &cycles), 0u);
}

static uint32_t get(schway *inst, uint32_t addr)
{
    uint32_t word = 0u;
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_read(inst, KSEG1 | (addr & 0x1FFFFFFFu), 4u, &word, &cycles), 0u);
    return word;
}

/* Runs the instruction at pc once and returns the cycles it took. */
static uint32_t step_at(schway *inst, uint32_t pc)
{
    uint64_t before = inst->cpu.cycle;
    inst->cpu.pc = pc;
    schway__cpu_step_one(inst, NULL);
    return (uint32_t)(inst->cpu.cycle - before);
}

/* A cycle cost read from the bus itself, for a word read at addr. */
static uint32_t bus_read_cost(schway *inst, uint32_t addr)
{
    uint32_t word = 0u;
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_read(inst, addr, 4u, &word, &cycles), 0u);
    return cycles;
}

/* A fetch that misses reads the missed word and the rest of its line, then hits.
   Line 0 of tag 1 is the RAM words 1000h-100Fh, entered at word 1: three words
   fill, the first valid bit stays clear, and the next fetches cost only the
   instruction's own cycle. A fetch in KSEG1 neither reads nor changes the line,
   and a store to RAM does not change what a hit returns. */
static void icache_fill(void)
{
    schway *inst = make();
    uint32_t miss = 1u + bus_read_cost(inst, KSEG0 | 0x1004u) + 2u;
    put(inst, 0x1000u, NOP);
    put(inst, 0x1004u, NOP);
    put(inst, 0x1008u, ADDIU(2u, 0u, 1u));
    put(inst, 0x100Cu, NOP);
    CHECK_EQ_U64(inst->cpu.icache_valid[0], 0u);

    CHECK_EQ_U64(step_at(inst, KSEG0 | 0x1004u), miss);
    CHECK_EQ_U64(inst->cpu.icache_tag[0], 1u);
    CHECK_EQ_HEX(inst->cpu.icache_valid[0], 0xEu);
    CHECK_EQ_HEX(inst->cpu.icache_word[0][2], ADDIU(2u, 0u, 1u));

    /* The next two words hit. */
    put(inst, 0x1008u, ADDIU(2u, 0u, 2u));
    CHECK_EQ_U64(step_at(inst, KSEG0 | 0x1008u), 1u);
    CHECK_EQ_U64(inst->cpu.r[2], 1u);
    CHECK_EQ_U64(step_at(inst, KSEG0 | 0x100Cu), 1u);
    CHECK_EQ_HEX(inst->cpu.icache_valid[0], 0xEu);

    /* KUSEG is cached too, and is the same line. */
    CHECK_EQ_U64(step_at(inst, 0x1008u), 1u);

    /* The same word in KSEG1 is read from RAM, and the line is as it was. */
    CHECK_EQ_U64(step_at(inst, KSEG1 | 0x1008u), 1u + bus_read_cost(inst, KSEG1 | 0x1008u));
    CHECK_EQ_U64(inst->cpu.r[2], 2u);
    CHECK_EQ_U64(inst->cpu.icache_tag[0], 1u);
    CHECK_EQ_HEX(inst->cpu.icache_valid[0], 0xEu);
    CHECK_EQ_HEX(inst->cpu.icache_word[0][2], ADDIU(2u, 0u, 1u));
    CHECK_EQ_U64(get(inst, 0x1008u), ADDIU(2u, 0u, 2u));

    /* The next line is a different index and fills from its word 0. */
    put(inst, 0x1010u, NOP);
    CHECK_EQ_U64(step_at(inst, KSEG0 | 0x1010u), 1u + bus_read_cost(inst, KSEG0 | 0x1010u) + 3u);
    CHECK_EQ_HEX(inst->cpu.icache_valid[1], 0xFu);
    schway_destroy(inst);
}

/* A tag hit on a word that is not valid refills from that word to the line's end
   and keeps the valid words it has; a miss on another tag takes the line over
   with no word valid but the filled ones. IBLKSZ does not shorten a fill, and a
   store never clears a valid bit. A bus error on the missed word leaves the
   line as it was, and one on a later word ends the fill and leaves no fault. */
static void icache_valid_bits(void)
{
    schway *inst = make();
    uint32_t cycles = 0u;
    for (uint32_t w = 0; w < 4u; w++) {
        put(inst, 0x2000u + 4u * w, NOP);
        put(inst, 0x7000u + 4u * w, NOP);
    }
    /* Entered at word 3, then word 1 refills 1 to 3, then word 0 all of it. */
    step_at(inst, KSEG0 | 0x200Cu);
    CHECK_EQ_HEX(inst->cpu.icache_valid[0], 0x8u);
    CHECK_EQ_U64(step_at(inst, KSEG0 | 0x2004u), 1u + bus_read_cost(inst, KSEG0 | 0x2004u) + 2u);
    CHECK_EQ_HEX(inst->cpu.icache_valid[0], 0xEu);
    CHECK_EQ_U64(step_at(inst, KSEG0 | 0x2000u), 1u + bus_read_cost(inst, KSEG0 | 0x2000u) + 3u);
    CHECK_EQ_HEX(inst->cpu.icache_valid[0], 0xFu);
    CHECK_EQ_U64(step_at(inst, KSEG0 | 0x2000u), 1u);

    /* The same index with another tag takes the line over. */
    step_at(inst, KSEG0 | 0x7008u);
    CHECK_EQ_U64(inst->cpu.icache_tag[0], 7u);
    CHECK_EQ_HEX(inst->cpu.icache_valid[0], 0xCu);
    CHECK_EQ_HEX(inst->cpu.icache_word[0][2], NOP);

    /* IBLKSZ is ignored: whatever it says, a fill reaches the end of the line. */
    CHECK_EQ_U64(schway__bus_write(inst, CACHE_CTRL, 4u, 0x300u, &cycles), 0u);
    step_at(inst, KSEG0 | 0x7000u);
    CHECK_EQ_HEX(inst->cpu.icache_valid[0], 0xFu);
    CHECK_EQ_U64(schway__bus_write(inst, CACHE_CTRL, 4u, 0u, &cycles), 0u);
    inst->cpu.icache_valid[0] = 0u;
    step_at(inst, KSEG0 | 0x7000u);
    CHECK_EQ_HEX(inst->cpu.icache_valid[0], 0xFu);

    /* A store to cached RAM neither clears a valid bit nor changes the word. */
    put(inst, CODE + 0x10u, SW(2u, 1u, 0u));
    inst->cpu.r[1] = KSEG0 | 0x7008u;
    inst->cpu.r[2] = ADDIU(3u, 0u, 9u);
    step_at(inst, CODE + 0x10u);
    CHECK_EQ_U64(get(inst, 0x7008u), ADDIU(3u, 0u, 9u));
    CHECK_EQ_HEX(inst->cpu.icache_valid[0], 0xFu);
    CHECK_EQ_HEX(inst->cpu.icache_word[0][2], NOP);
    step_at(inst, KSEG0 | 0x7008u);
    CHECK_EQ_U64(inst->cpu.r[3], 0u);

    /* A bus error on the missed word leaves the line alone and is the first fault. */
    step_at(inst, KSEG0 | 0x00900000u);
    CHECK_EQ_U64(inst->cpu.icache_tag[0], 7u);
    CHECK_EQ_HEX(inst->cpu.icache_valid[0], 0xFu);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_FETCH);
    schway_destroy(inst);

    /* A line that runs from the interrupt registers into a gap: the word that
       was asked for fills, the gap is not read for the CPU and records no fault. */
    inst = make();
    step_at(inst, KSEG0 | 0x1F801074u);
    CHECK_EQ_U64(inst->cpu.icache_valid[0x07], 0x2u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    schway_destroy(inst);
}

/* With SR.Isc set a load returns the line's word whether or not the tag matches
   and no bus access happens; a store writes the word, or, with cache-control
   TAG and IS1 both set, the tag and the four valid bits; the cache-control word
   in KSEG2 still reaches the bus. The BIOS' FlushCache sequence leaves every line
   invalid. */
static void icache_isolation(void)
{
    schway *inst = make();
    uint32_t ctrl_cycles = 0u;
    uint32_t fetch_cost;
    put(inst, 0x3000u, LW(4u, 1u, 0u));
    put(inst, 0x3004u, LBU(5u, 1u, 1u));
    put(inst, 0x3008u, SB(6u, 1u, 1u));
    put(inst, 0x300Cu, SW(7u, 1u, 0u));
    put(inst, 0x3010u, NOP);
    fetch_cost = bus_read_cost(inst, KSEG1 | 0x3000u);
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = SR_ISC;
    inst->cpu.icache_tag[5] = 0x123u;
    inst->cpu.icache_word[5][2] = 0xDEADBEEFu;
    inst->cpu.icache_valid[5] = 0u;

    /* A load: line 5 word 2 is address 58h, and its tag (123h) is not the
       address's (0). Nothing reaches the bus, and the datum lands a step later. */
    inst->cpu.r[1] = 0x58u;
    CHECK_EQ_U64(step_at(inst, KSEG1 | 0x3000u), 1u + fetch_cost);
    step_at(inst, KSEG1 | 0x3010u);
    CHECK_EQ_HEX(inst->cpu.r[4], 0xDEADBEEFu);
    CHECK_EQ_U64(get(inst, 0x58u), 0u);
    inst->cpu.r[1] = 0x58u;
    step_at(inst, KSEG1 | 0x3004u);
    step_at(inst, KSEG1 | 0x3010u);
    CHECK_EQ_HEX(inst->cpu.r[5], 0xBEu);

    /* A store without TAG writes the bytes it covers into the word. */
    inst->cpu.r[6] = 0x11u;
    step_at(inst, KSEG1 | 0x3008u);
    CHECK_EQ_HEX(inst->cpu.icache_word[5][2], 0xDEAD11EFu);
    CHECK_EQ_U64(get(inst, 0x58u), 0u);

    /* TAG alone, or IS1 alone, still writes the word. */
    inst->cpu.r[1] = 0x50u;
    inst->cpu.r[7] = 0x00ABC00Au;
    CHECK_EQ_U64(schway__bus_write(inst, CACHE_CTRL, 4u, 0x4u, &ctrl_cycles), 0u);
    step_at(inst, KSEG1 | 0x300Cu);
    CHECK_EQ_HEX(inst->cpu.icache_word[5][0], 0x00ABC00Au);
    CHECK_EQ_U64(inst->cpu.icache_tag[5], 0x123u);
    CHECK_EQ_U64(schway__bus_write(inst, CACHE_CTRL, 4u, 0x800u, &ctrl_cycles), 0u);
    inst->cpu.r[7] = 0x0000000Fu;
    step_at(inst, KSEG1 | 0x300Cu);
    CHECK_EQ_HEX(inst->cpu.icache_word[5][0], 0x0000000Fu);
    CHECK_EQ_U64(inst->cpu.icache_valid[5], 0u);

    /* TAG with IS1: the tag is bits 31:12 of the datum, the valid bits its low four. */
    CHECK_EQ_U64(schway__bus_write(inst, CACHE_CTRL, 4u, CTRL_TAG_IS1, &ctrl_cycles), 0u);
    inst->cpu.r[7] = 0x00ABC00Au;
    step_at(inst, KSEG1 | 0x300Cu);
    CHECK_EQ_HEX(inst->cpu.icache_tag[5], 0xABCu);
    CHECK_EQ_HEX(inst->cpu.icache_valid[5], 0xAu);
    CHECK_EQ_HEX(inst->cpu.icache_word[5][0], 0x0000000Fu);

    /* The cache-control word is not isolated. */
    put(inst, 0x3014u, SW(8u, 9u, 0u));
    inst->cpu.r[9] = CACHE_CTRL;
    inst->cpu.r[8] = 0x1E988u;
    step_at(inst, KSEG1 | 0x3014u);
    CHECK_EQ_HEX(inst->bus.cache_ctrl, 0x1E988u);
    CHECK_EQ_HEX(inst->cpu.icache_tag[5], 0xABCu);

    /* Leaving isolation, the same address is RAM again. */
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = 0u;
    put(inst, 0x58u, 0x01020304u);
    inst->cpu.r[1] = 0x58u;
    step_at(inst, KSEG1 | 0x3000u);
    step_at(inst, KSEG1 | 0x3010u);
    CHECK_EQ_HEX(inst->cpu.r[4], 0x01020304u);
    schway_destroy(inst);

    /* FlushCache: fill some lines by running code, then isolate with TAG and IS1
       and store zero to every line, as the BIOS routine does. */
    inst = make();
    put(inst, 0x3000u, SW(0u, 1u, 0u));
    for (uint32_t i = 0; i < 4u; i++) {
        put(inst, 0x5000u + 16u * i, NOP);
        step_at(inst, KSEG0 | (0x5000u + 16u * i));
        CHECK_EQ_HEX(inst->cpu.icache_valid[0u + i], 0xFu);
    }
    CHECK_EQ_U64(schway__bus_write(inst, CACHE_CTRL, 4u, CTRL_TAG_IS1, &ctrl_cycles), 0u);
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = SR_ISC;
    for (uint32_t i = 0; i < SCHWAY__ICACHE_LINES; i++) {
        inst->cpu.r[1] = 16u * i;
        step_at(inst, KSEG1 | 0x3000u);
    }
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = 0u;
    for (uint32_t i = 0; i < SCHWAY__ICACHE_LINES; i++) {
        CHECK_EQ_U64(inst->cpu.icache_valid[i], 0u);
        CHECK_EQ_U64(inst->cpu.icache_tag[i], 0u);
    }
    CHECK_EQ_U64(step_at(inst, KSEG0 | 0x5000u), 1u + bus_read_cost(inst, KSEG0 | 0x5000u) + 3u);
    schway_destroy(inst);
}

/* One store of v to addr from the cached code word at CODE. */
static uint32_t store_word(schway *inst, uint32_t addr, uint32_t v)
{
    inst->cpu.r[1] = addr;
    inst->cpu.r[2] = v;
    return step_at(inst, CODE);
}

/* The queue holds four stores and a fifth waits for one to leave, so a fifth
   store costs more than a fourth and the count stays at four. A store to the
   scratchpad, a store that fails and a store under the replay log do not join it.
   Only the order of events is pinned; the cycles of a wait are not. */
static void write_queue_depth(void)
{
    schway *inst = make();
    uint32_t cost[7];
    put(inst, CODE, SW(2u, 1u, 0u));
    store_word(inst, DATA, 1u); /* a miss: the first fetch fills the line */
    CHECK_EQ_U64(inst->cpu.wq_count, 1u);
    for (uint32_t n = 2; n <= 4u; n++) {
        cost[n] = store_word(inst, DATA + 4u * n, n);
        CHECK_EQ_U64(inst->cpu.wq_count, n);
    }
    CHECK_EQ_U64(cost[3], cost[2]);
    CHECK_EQ_U64(cost[4], cost[3]);
    cost[5] = store_word(inst, DATA + 20u, 5u);
    CHECK_EQ_U64(inst->cpu.wq_count, 4u);
    CHECK(cost[5] > cost[4]);
    cost[6] = store_word(inst, DATA + 24u, 6u);
    CHECK_EQ_U64(inst->cpu.wq_count, 4u);
    CHECK(cost[6] > cost[4]);
    CHECK_EQ_U64(get(inst, DATA + 24u), 6u);

    /* The scratchpad is not behind the queue. */
    inst->cpu.wq_count = 0u;
    store_word(inst, KSEG0 | 0x1F800000u, 7u);
    CHECK_EQ_U64(inst->cpu.wq_count, 0u);
    store_word(inst, DATA, 8u);
    CHECK_EQ_U64(inst->cpu.wq_count, 1u);

    /* A store that is a bus error is not queued; it is the data bus error. */
    inst->cpu.wq_count = 0u;
    store_word(inst, KSEG0 | 0x00900000u, 9u);
    CHECK_EQ_U64(inst->cpu.wq_count, 0u);
    CHECK_EQ_U64(inst->cpu.pc, VECTOR_RAM);
    schway_destroy(inst);

    /* With a replay log attached the store is emitted at once and the queue and
       the cache are not used. */
    inst = make();
    struct schway__cpu_cycle expect[1] = {{SCHWAY__CYCLE_FETCH, 4u, CODE, SW(2u, 1u, 0u)}};
    struct schway__cpu_log log;
    memset(&log, 0, sizeof log);
    log.expect = expect;
    log.expect_n = 1u;
    inst->cpu.r[1] = DATA;
    inst->cpu.r[2] = 0x55u;
    inst->cpu.pc = CODE;
    inst->cpu.wq_count = 2u;
    schway__cpu_step_one(inst, &log);
    CHECK_EQ_U64(log.got_n, 2u);
    CHECK_EQ_U64(log.got[1].kind, SCHWAY__CYCLE_WRITE);
    CHECK_EQ_U64(inst->cpu.wq_count, 2u);
    CHECK_EQ_U64(inst->cpu.icache_valid[(CODE >> 4) & 0xFFu], 0u);
    schway_destroy(inst);
}

/* A KSEG1 access, a read, a write or a fetch, drains the queue first and an empty
   queue adds nothing; a cached read does not drain it. The stores land in the
   order they were made. */
static void write_queue_drain(void)
{
    schway *inst = make();
    uint32_t empty, full;
    put(inst, CODE, SW(2u, 1u, 0u));
    put(inst, CODE + 0x10u, LW(5u, 1u, 0u));
    put(inst, CODE + 0x20u, SW(2u, 1u, 0u));
    put(inst, 0x6000u, NOP);

    /* The KSEG1 read with an empty queue costs the instruction and the bus. */
    inst->cpu.r[1] = KSEG1 | (DATA & 0x1FFFFFFFu);
    step_at(inst, CODE + 0x10u); /* the first run fills the code line */
    empty = step_at(inst, CODE + 0x10u);
    CHECK_EQ_U64(empty, 1u + bus_read_cost(inst, inst->cpu.r[1]));
    CHECK_EQ_U64(inst->cpu.wq_count, 0u);

    store_word(inst, DATA, 1u);
    store_word(inst, DATA + 4u, 2u);
    store_word(inst, DATA + 8u, 3u);
    CHECK_EQ_U64(inst->cpu.wq_count, 3u);

    /* A cached read leaves the queue alone. */
    inst->cpu.r[1] = DATA;
    step_at(inst, CODE + 0x10u);
    CHECK_EQ_U64(inst->cpu.wq_count, 3u);

    /* A KSEG1 read drains all three, and reads what the first of them wrote. */
    inst->cpu.r[1] = KSEG1 | (DATA & 0x1FFFFFFFu);
    full = step_at(inst, CODE + 0x10u);
    CHECK(full > empty);
    CHECK_EQ_U64(inst->cpu.wq_count, 0u);
    step_at(inst, 0xA0006000u);
    CHECK_EQ_U64(inst->cpu.r[5], 1u);
    CHECK_EQ_U64(get(inst, DATA), 1u);
    CHECK_EQ_U64(get(inst, DATA + 4u), 2u);
    CHECK_EQ_U64(get(inst, DATA + 8u), 3u);

    /* A KSEG1 store drains the queue and then joins it. */
    store_word(inst, DATA + 12u, 4u);
    store_word(inst, DATA + 16u, 5u);
    CHECK_EQ_U64(inst->cpu.wq_count, 2u);
    inst->cpu.r[1] = KSEG1 | ((DATA + 20u) & 0x1FFFFFFFu);
    inst->cpu.r[2] = 6u;
    step_at(inst, CODE + 0x20u);
    CHECK_EQ_U64(inst->cpu.wq_count, 1u);
    CHECK_EQ_U64(get(inst, DATA + 12u), 4u);
    CHECK_EQ_U64(get(inst, DATA + 16u), 5u);
    CHECK_EQ_U64(get(inst, DATA + 20u), 6u);

    /* A KSEG1 fetch drains it too. */
    store_word(inst, DATA + 24u, 7u);
    CHECK_EQ_U64(inst->cpu.wq_count, 2u);
    step_at(inst, 0xA0006000u);
    CHECK_EQ_U64(inst->cpu.wq_count, 0u);

    /* Two stores to one address land in order, the last one wins. */
    store_word(inst, DATA + 28u, 8u);
    store_word(inst, DATA + 28u, 9u);
    inst->cpu.r[1] = KSEG1 | ((DATA + 28u) & 0x1FFFFFFFu);
    step_at(inst, CODE + 0x10u);
    step_at(inst, 0xA0006000u);
    CHECK_EQ_U64(inst->cpu.r[5], 9u);
    schway_destroy(inst);
}

/* An exception taken with stores queued keeps them: the stores have landed in
   order, the count is unchanged by the exception and by the handler's first
   instruction. */
static void write_queue_exception(void)
{
    schway *inst = make();
    put(inst, CODE, SW(2u, 1u, 0u));
    put(inst, CODE + 0x20u, SYSCALL);
    put(inst, VECTOR_RAM, NOP);
    store_word(inst, DATA, 11u);
    store_word(inst, DATA + 4u, 22u);
    store_word(inst, DATA + 8u, 33u);
    CHECK_EQ_U64(inst->cpu.wq_count, 3u);
    step_at(inst, CODE + 0x20u);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
    CHECK_EQ_HEX(inst->cpu.cop0.r[SCHWAY__COP0_EPC], CODE + 0x20u);
    CHECK_EQ_U64((inst->cpu.cop0.r[SCHWAY__COP0_CAUSE] >> 2) & 31u, SCHWAY__EXC_SYS);
    CHECK_EQ_U64(inst->cpu.wq_count, 3u);
    step_at(inst, VECTOR_RAM);
    CHECK_EQ_U64(inst->cpu.wq_count, 3u);
    CHECK_EQ_U64(get(inst, DATA), 11u);
    CHECK_EQ_U64(get(inst, DATA + 4u), 22u);
    CHECK_EQ_U64(get(inst, DATA + 8u), 33u);
    schway_destroy(inst);
}

/* A state saved with three stores queued and a filled cache restores both byte
   for byte after a power reset. */
static void write_queue_state(void)
{
    schway *inst = make();
    put(inst, CODE, SW(2u, 1u, 0u));
    store_word(inst, DATA, 1u);
    store_word(inst, DATA + 4u, 2u);
    store_word(inst, DATA + 8u, 3u);
    CHECK_EQ_U64(inst->cpu.wq_count, 3u);
    size_t n = schway_state_size(inst);
    void *buf = malloc(n);
    uint32_t line = (CODE >> 4) & 0xFFu;
    uint32_t tag = inst->cpu.icache_tag[line];
    uint32_t word = inst->cpu.icache_word[line][0];
    if (buf == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(schway_save_state(inst, buf, n), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(inst->cpu.wq_count, 0u);
    CHECK_EQ_U64(inst->cpu.icache_valid[line], 0u);
    CHECK_EQ_U64(get(inst, DATA + 8u), 0u);
    CHECK_EQ_U64(schway_load_state(inst, buf, n), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(inst->cpu.wq_count, 3u);
    CHECK_EQ_U64(inst->cpu.icache_valid[line], 0xFu);
    CHECK_EQ_U64(inst->cpu.icache_tag[line], tag);
    CHECK_EQ_HEX(inst->cpu.icache_word[line][0], word);
    CHECK_EQ_U64(get(inst, DATA), 1u);
    CHECK_EQ_U64(get(inst, DATA + 4u), 2u);
    CHECK_EQ_U64(get(inst, DATA + 8u), 3u);
    free(buf);
    schway_destroy(inst);
}


int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-cache <case>\n");
        return 2;
    }
    const char *c = argv[1];
    if (strcmp(c, "icache_fill") == 0) {
        icache_fill();
    } else if (strcmp(c, "icache_valid_bits") == 0) {
        icache_valid_bits();
    } else if (strcmp(c, "icache_isolation") == 0) {
        icache_isolation();
    } else if (strcmp(c, "write_queue_depth") == 0) {
        write_queue_depth();
    } else if (strcmp(c, "write_queue_drain") == 0) {
        write_queue_drain();
    } else if (strcmp(c, "write_queue_exception") == 0) {
        write_queue_exception();
    } else if (strcmp(c, "write_queue_state") == 0) {
        write_queue_state();
    } else {
        fprintf(stderr, "schwaystation-test-cache: unknown case %s\n", c);
        return 2;
    }
    CHECK_DONE();
}
