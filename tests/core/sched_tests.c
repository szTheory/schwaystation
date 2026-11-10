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

/* A jump to itself with a no-op in its slot. */
static void load_spin(schway *inst)
{
    static const uint32_t spin[] = {
        0x08004000u, /* j 80010000h */
        0x00000000u, /* nop */
    };
    load(inst, spin, 2u);
}

/* 100 no-ops, then a jump to the first of them: blocks of the program are long. */
static void load_nops_then_jump(schway *inst)
{
    uint32_t words[102];
    memset(words, 0, sizeof words);
    words[100] = 0x08004000u; /* j 80010000h */
    load(inst, words, 102u);
}

/* Runs the stub kernel's boot one cycle at a time until the program's first
   instruction is next, so the program starts on a block of its own. */
static void run_to_entry(schway *inst)
{
    for (uint32_t i = 0; i < 4000u && inst->cpu.pc != TEXT; i++) {
        (void)run(inst, 1u, 0u);
    }
    CHECK_EQ_HEX(inst->cpu.pc, TEXT);
}

/* The instance spinning in the program, past the stub kernel's boot. */
static schway *make_spinning(void)
{
    schway *inst = make();
    load_spin(inst);
    (void)run(inst, 20000u, 0u);
    CHECK(inst->ticks >= 20000u);
    return inst;
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

/* A stall freezes the CPU: with 500 cycles of stall left and a 1,000-cycle
   budget, 500 cycles pass with no instruction, the CPU then runs the other 500
   (to its next instruction boundary), and the stall is over. */
static void case_stall_freeze(void)
{
    schway *inst = make_spinning();
    uint64_t t0 = inst->ticks;
    uint64_t c0 = inst->cpu.cycle;
    inst->dma.stall_until = t0 + 500u;
    schway_run_result r = run(inst, 1000u, 0u);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_BUDGET);
    uint64_t dt = r.ticks - t0;
    uint64_t dc = inst->cpu.cycle - c0;
    CHECK(dt >= 1000u && dt < 1000u + 16u);
    CHECK_EQ_U64(dc, dt - 500u);
    CHECK(dc >= 500u && dc < 500u + 16u);
    CHECK_EQ_U64(inst->dma.stall_until, t0 + 500u);
    schway_destroy(inst);
}

/* A stall longer than the budget uses all of it: ticks advance, no instruction
   runs, the stamp stays and the next call keeps waiting; the call that outlasts
   the stall runs the rest of its budget. */
static void case_stall_carries(void)
{
    schway *inst = make_spinning();
    uint64_t t0 = inst->ticks;
    uint64_t c0 = inst->cpu.cycle;
    uint32_t pc0 = inst->cpu.pc;
    inst->dma.stall_until = t0 + 5000u;
    schway_run_result r = run(inst, 1000u, 0u);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(r.ticks, t0 + 1000u);
    CHECK_EQ_U64(inst->cpu.cycle, c0);
    CHECK_EQ_U64(inst->cpu.pc, pc0);
    CHECK_EQ_U64(inst->dma.stall_until, t0 + 5000u);
    r = run(inst, 1000u, 0u);
    CHECK_EQ_U64(r.ticks, t0 + 2000u);
    CHECK_EQ_U64(inst->cpu.cycle, c0);
    CHECK_EQ_U64(inst->dma.stall_until, t0 + 5000u);
    r = run(inst, 4000u, 0u);
    CHECK(r.ticks >= t0 + 6000u && r.ticks < t0 + 6000u + 16u);
    CHECK(inst->cpu.cycle - c0 >= 1000u && inst->cpu.cycle - c0 < 1000u + 16u);
    schway_destroy(inst);
}

/* A GPU edge ends the block that is running, and the event runs at the first
   instruction boundary after it. The program is 100 straight-line no-ops, so a
   block that ignored the deadline would run on past the budget. A beam edge
   beyond the run is left alone. */
static void case_event_dispatch(void)
{
    schway *inst = make();
    load_nops_then_jump(inst);
    run_to_entry(inst);
    inst->gpu.display_mode = 1u;
    inst->gpu.horizontal_range = 1u << 12;
    inst->gpu.beam_tick = inst->ticks;
    inst->gpu.beam_ratio_remainder = 0u;
    inst->gpu.beam_line_clock = 0u;
    inst->gpu.beam_line = 0u;
    inst->gpu.beam_dot_phase = 0u;
    schway__sched_sync(inst);
    uint64_t deadline = inst->sched.next;
    CHECK(deadline > inst->ticks && deadline < inst->ticks + 16u);
    (void)run(inst, 200u, 0u);
    CHECK(inst->ticks >= deadline && inst->ticks < deadline + SCHWAY__INSN_CYCLES_MAX);
    CHECK((inst->timers.blank & (1u << 1)) != 0u);
    CHECK(inst->sched.next > deadline);

    schway *far = make();
    load_nops_then_jump(far);
    run_to_entry(far);
    far->gpu.display_mode = 1u;
    far->gpu.horizontal_range = 1000u << 12;
    far->gpu.beam_tick = far->ticks;
    far->gpu.beam_ratio_remainder = 0u;
    far->gpu.beam_line_clock = 0u;
    far->gpu.beam_line = 0u;
    far->gpu.beam_dot_phase = 0u;
    schway__sched_sync(far);
    uint64_t at = far->sched.next;
    CHECK(at > far->ticks + 200u);
    (void)run(far, 200u, 0u);
    CHECK_EQ_U64(far->sched.next, at);
    schway_destroy(inst);
    schway_destroy(far);
}

/* The deadline is derived: a state that was saved while it said something else
   loads with it recomputed from the devices. */
static void case_state_next_derived(void)
{
    schway *inst = make_spinning();
    size_t n = schway_state_size(inst);
    uint8_t *buf = (uint8_t *)malloc(n);
    if (buf == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(schway_save_state(inst, buf, n), SCHWAY_STATUS_OK);
    inst->sched.next = 1u;
    CHECK_EQ_U64(schway_load_state(inst, buf, n), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(inst->sched.next, schway__gpu_next(inst));
    CHECK_EQ_U64(inst->cpu.blk_tick, inst->ticks);
    CHECK_EQ_U64(inst->cpu.blk_cycle, inst->cpu.cycle);
    free(buf);
    schway_destroy(inst);
}

/* A stall that crosses the frame's end ends the frame at the frame's end tick
   with no instruction run in the stalled stretch, and the rest of the stall is
   still in the state for the next frame. */
static void case_stall_frame_end(void)
{
    schway *inst = make_spinning();
    uint64_t end = inst->frame_end;
    (void)run(inst, end - 1000u - inst->ticks, 0u);
    CHECK(inst->ticks >= end - 1000u && inst->ticks < end - 900u);
    uint64_t held = inst->ticks + 5000u;
    uint64_t c0 = inst->cpu.cycle;
    inst->dma.stall_until = held;
    schway_run_result r;
    memset(&r, 0, sizeof r);
    r.size = (uint32_t)sizeof r;
    CHECK_EQ_U64(schway_run_frame(inst, &r), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    CHECK_EQ_U64(r.frame, 1u);
    CHECK_EQ_U64(r.ticks, end);
    CHECK_EQ_U64(inst->cpu.cycle, c0);
    CHECK_EQ_U64(inst->dma.stall_until, held);
    CHECK(held > end);
    /* The next frame keeps waiting out the stall, then runs. */
    CHECK_EQ_U64(schway_run_frame(inst, &r), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    CHECK_EQ_U64(r.frame, 2u);
    CHECK(inst->cpu.cycle > c0);
    CHECK_EQ_U64(inst->cpu.cycle - c0, r.ticks - held);
    schway_destroy(inst);
}

/* With no program loaded the beam still supplies exact GPU events; the blank
   transition runs during the bounded call and the next line deadline is kept. */
static void case_no_exe_events(void)
{
    schway *inst = make();
    CHECK_EQ_U64(inst->sched.next, UINT64_MAX);
    inst->gpu.display_mode = 1u;
    inst->gpu.horizontal_range = 1u << 12;
    inst->gpu.beam_tick = inst->ticks;
    inst->gpu.beam_ratio_remainder = 0u;
    inst->gpu.beam_line_clock = 0u;
    inst->gpu.beam_line = 0u;
    inst->gpu.beam_dot_phase = 0u;
    schway__sched_sync(inst);
    schway_run_result r = run(inst, 500u, 0u);
    CHECK_EQ_U64(r.ticks, 500u);
    CHECK((inst->timers.blank & (1u << 1)) != 0u);
    CHECK_EQ_U64(inst->sched.next, schway__gpu_next(inst));
    CHECK_EQ_U64(inst->cpu.blk_tick, 500u);
    schway_destroy(inst);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"event_dispatch", case_event_dispatch},
    {"fault_ticks", case_fault_ticks},
    {"no_exe_events", case_no_exe_events},
    {"stall_carries", case_stall_carries},
    {"stall_frame_end", case_stall_frame_end},
    {"stall_freeze", case_stall_freeze},
    {"state_next_derived", case_state_next_derived},
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
