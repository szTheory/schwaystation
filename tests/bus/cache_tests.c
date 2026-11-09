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

#define KSEG0 0x80000000u
#define KSEG1 0xA0000000u
#define CACHE_CTRL 0xFFFE0130u

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

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-cache <case>\n");
        return 2;
    }
    const char *c = argv[1];
    if (strcmp(c, "icache_fill") == 0) {
        icache_fill();
    } else {
        fprintf(stderr, "schwaystation-test-cache: unknown case %s\n", c);
        return 2;
    }
    CHECK_DONE();
}
