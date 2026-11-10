/* The scheduler spine: the block clock, the deadline that ends a block, the
   event runner and the CPU stall that the devices share: schwaystation-test-sched
   <case>. The test sees src/ to set the device deadlines and stalls by hand,
   because the devices behind them are not the subject. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

#define TEXT 0x80010000u

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

static schway_run_result run(schway *inst, uint64_t cycles, uint32_t flags)
{
    schway_budget b;
    memset(&b, 0, sizeof b);
    b.size = (uint32_t)sizeof b;
    b.flags = flags;
    b.max_cycles = cycles;
    schway_run_result r;
    memset(&r, 0, sizeof r);
    r.size = (uint32_t)sizeof r;
    CHECK_EQ_U64(schway_run(inst, &b, &r), SCHWAY_STATUS_OK);
    return r;
}

static schway_fault fault_of(const schway *inst)
{
    schway_fault f;
    memset(&f, 0, sizeof f);
    f.size = (uint32_t)sizeof f;
    CHECK_EQ_U64(schway_get_fault(inst, &f), SCHWAY_STATUS_OK);
    return f;
}

/* 40 no-ops, then a load from 1F000000h, the first expansion region, which
   answers nothing, then a no-op. */
static void load_nops_then_fault(schway *inst)
{
    uint32_t words[44];
    memset(words, 0, sizeof words);
    words[40] = 0x3C081F00u; /* lui t0, 1F00h */
    words[41] = 0x8D090000u; /* lw  t1, 0(t0) */
    load(inst, words, 44u);
}

/* The first fault of a long block carries the tick its own instruction began
   on, not the tick the block started on. The reference runs the same program
   one cycle at a time, so each block is one instruction and the tick before the
   last call is where the faulting load began; the stub kernel runs first, so
   the tick the program's first instruction began on is noted too, and the 42
   instructions of the program are one block in the whole run. */
static void case_fault_ticks(void)
{
    schway *step = make();
    load_nops_then_fault(step);
    uint64_t began = 0u;
    uint64_t entry = 0u;
    uint64_t last = 0u;
    for (uint32_t i = 0; i < 2000u && fault_of(step).kind == 0u; i++) {
        if (step->cpu.pc == TEXT && entry == 0u) {
            entry = last;
        }
        began = last;
        last = run(step, 1u, 0u).ticks;
    }
    CHECK(fault_of(step).kind != 0u);
    CHECK(entry != 0u);
    CHECK(began >= entry + 40u);

    schway *whole = make();
    load_nops_then_fault(whole);
    (void)run(whole, 20000u, 0u);
    schway_fault f = fault_of(whole);
    CHECK(f.kind != 0u);
    CHECK_EQ_U64(f.ticks, began);
    CHECK_EQ_U64(f.ticks, fault_of(step).ticks);
    CHECK(f.ticks >= entry + 40u);
    schway_destroy(step);
    schway_destroy(whole);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"fault_ticks", case_fault_ticks},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-sched <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "schwaystation-test-sched: unknown case %s\n", argv[1]);
    return 2;
}
