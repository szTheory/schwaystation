/* The trace ring through the public API: schwaystation-test-trace <case>. A run
   whose budget sets SCHWAY_RUN_TRACE leaves the last 4,096 records of the
   executor in the instance, and schway_get_trace reads them from the oldest.
   The test sees src/ for the ring's size and the state's field names. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

#define TEXT 0x80010000u
#define RING 4096u

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

static schway_trace get(const schway *inst, uint32_t index)
{
    schway_trace t;
    memset(&t, 0, sizeof t);
    t.size = (uint32_t)sizeof t;
    CHECK_EQ_U64(schway_get_trace(inst, index, &t), SCHWAY_STATUS_OK);
    return t;
}

static uint32_t held(const schway *inst)
{
    schway_trace t;
    memset(&t, 0, sizeof t);
    t.size = (uint32_t)sizeof t;
    return schway_get_trace(inst, 0u, &t) == SCHWAY_STATUS_OK ? t.count : 0u;
}

/* The program of the first cases: a word stored and loaded back, then the
   loaded word kept as a marker at 80020004h, then a jump to itself. */
static const uint32_t program_marker[] = {
    0x3C088002u, /* lui   t0, 8002h */
    0x24090055u, /* addiu t1, zero, 55h */
    0xAD090000u, /* sw    t1, 0(t0) */
    0x8D0A0000u, /* lw    t2, 0(t0) */
    0x00000000u, /* nop (the load's delay slot) */
    0xAD0A0004u, /* sw    t2, 4(t0) */
    0x08004006u, /* j     80010018h */
    0x00000000u, /* nop */
};

/* A program that counts forever: addiu, then a jump to it with a nop slot. */
static const uint32_t program_count[] = {
    0x25080001u, /* addiu t0, t0, 1 */
    0x08004000u, /* j     80010000h */
    0x00000000u, /* nop */
};

/* Runs traced in short slices until the marker program has written its marker,
   so the ring holds the stub kernel's whole path to it and the program. */
static void run_to_marker(schway *inst)
{
    uint32_t marker = 0u;
    for (uint32_t slice = 0u; slice < 4000u && marker == 0u; slice++) {
        (void)run(inst, 64u, SCHWAY_RUN_TRACE);
        CHECK_EQ_U64(schway_peek(inst, SCHWAY_SPACE_RAM, 0x20004u, 4u, &marker), SCHWAY_STATUS_OK);
    }
    CHECK_EQ_HEX(marker, 0x55u);
}

/* Every record carries the number held, the event is one of the five, and the
   cycles never go backwards. */
static void check_shape(const schway *inst, uint32_t n)
{
    uint64_t last = 0u;
    for (uint32_t i = 0u; i < n; i++) {
        schway_trace t = get(inst, i);
        CHECK_EQ_U64(t.size, sizeof t);
        CHECK_EQ_U64(t.count, n);
        CHECK(t.event >= SCHWAY_TRACE_EXEC && t.event <= SCHWAY_TRACE_FAULT);
        CHECK(t.cycle >= last);
        last = t.cycle;
    }
}

static void case_exec_records(void)
{
    CHECK(sizeof(struct schway__trace_rec) <= 32u);
    CHECK(sizeof(struct schway__trace_ring) <= RING * 32u + 64u);
    schway *inst = make();
    load(inst, program_marker, 8u);
    run_to_marker(inst);
    uint32_t n = held(inst);
    CHECK(n > 100u && n < RING);
    check_shape(inst, n);

    /* The first record is the stub kernel's first instruction at reset, on
       cycle 0 of frame 0. */
    schway_trace first = get(inst, 0u);
    CHECK_EQ_U64(first.event, SCHWAY_TRACE_EXEC);
    CHECK_EQ_HEX(first.pc, 0xBFC00000u);
    CHECK_EQ_U64(first.cycle, 0u);
    CHECK_EQ_U64(first.frame, 0u);
    CHECK_EQ_U64(first.line | first.dot, 0u);

    /* The program's own path: each instruction an exec record, and the store
       and the load a wr and an rd of RAM between them. */
    uint32_t k = 0u;
    while (k < n && !(get(inst, k).event == SCHWAY_TRACE_EXEC && get(inst, k).pc == TEXT)) {
        k++;
    }
    CHECK(k + 8u < n);
    schway_trace t;
    t = get(inst, k);
    CHECK_EQ_HEX(t.opcode, 0x3C088002u);
    t = get(inst, k + 1u);
    CHECK_EQ_U64(t.event, SCHWAY_TRACE_EXEC);
    CHECK_EQ_HEX(t.pc, TEXT + 4u);
    CHECK_EQ_HEX(t.opcode, 0x24090055u);
    t = get(inst, k + 2u);
    CHECK_EQ_U64(t.event, SCHWAY_TRACE_EXEC);
    CHECK_EQ_HEX(t.pc, TEXT + 8u);
    t = get(inst, k + 3u);
    CHECK_EQ_U64(t.event, SCHWAY_TRACE_WR);
    CHECK_EQ_HEX(t.pc, TEXT + 8u);
    CHECK_EQ_HEX(t.addr, 0x80020000u);
    CHECK_EQ_U64(t.width, 4u);
    CHECK_EQ_U64(t.device, SCHWAY_DEVICE_RAM);
    CHECK_EQ_HEX(t.value, 0x55u);
    t = get(inst, k + 4u);
    CHECK_EQ_U64(t.event, SCHWAY_TRACE_EXEC);
    CHECK_EQ_HEX(t.pc, TEXT + 12u);
    t = get(inst, k + 5u);
    CHECK_EQ_U64(t.event, SCHWAY_TRACE_RD);
    CHECK_EQ_HEX(t.addr, 0x80020000u);
    CHECK_EQ_U64(t.device, SCHWAY_DEVICE_RAM);
    CHECK_EQ_HEX(t.value, 0x55u);
    t = get(inst, k + 6u);
    CHECK_EQ_U64(t.event, SCHWAY_TRACE_EXEC);
    CHECK_EQ_HEX(t.pc, TEXT + 16u);

    /* The stub's writes to I_STAT and I_MASK name the interrupt registers, and
       its first memory-control write the memory control. */
    uint32_t irq_writes = 0u;
    uint32_t memctl_writes = 0u;
    for (uint32_t i = 0u; i < n; i++) {
        t = get(inst, i);
        if (t.event == SCHWAY_TRACE_WR && t.device == SCHWAY_DEVICE_IRQ) {
            irq_writes++;
        }
        if (t.event == SCHWAY_TRACE_WR && t.device == SCHWAY_DEVICE_MEMCTL) {
            memctl_writes++;
        }
    }
    CHECK_EQ_U64(irq_writes, 2u);
    CHECK(memctl_writes >= 10u);

    /* Frame 0: line and dot follow from the cycles, 11/7 GPU clocks each,
       3413 clocks to a line. */
    t = get(inst, k);
    CHECK_EQ_U64(t.line, (t.cycle * 11u / 7u) / 3413u);
    CHECK_EQ_U64(t.dot, (t.cycle * 11u / 7u) % 3413u);
    CHECK(t.line > 0u || t.dot > 0u);
    schway_destroy(inst);
}

static void case_wrap(void)
{
    schway *inst = make();
    load(inst, program_count, 3u);
    schway_run_result res = run(inst, 30000u, SCHWAY_RUN_TRACE);
    CHECK_EQ_U64(res.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(held(inst), RING);
    check_shape(inst, RING);

    /* The oldest record is no longer the stub's reset: the first were
       overwritten. What is left is the loop, in order, up to the end. */
    schway_trace t = get(inst, 0u);
    CHECK(t.cycle > 0u && t.pc >= TEXT);
    uint32_t prev = 0u;
    for (uint32_t i = 0u; i < RING; i++) {
        t = get(inst, i);
        CHECK_EQ_U64(t.event, SCHWAY_TRACE_EXEC);
        CHECK(t.pc == TEXT || t.pc == TEXT + 4u || t.pc == TEXT + 8u);
        if (i > 0u) {
            CHECK_EQ_HEX(t.pc, prev == TEXT + 8u ? TEXT : prev + 4u);
        }
        prev = t.pc;
    }
    CHECK(t.cycle <= res.ticks && t.cycle + 8u >= res.ticks);

    /* Another slice keeps appending and keeps 4,096. */
    uint64_t before = t.cycle;
    (void)run(inst, 100u, SCHWAY_RUN_TRACE);
    CHECK_EQ_U64(held(inst), RING);
    CHECK(get(inst, RING - 1u).cycle > before);

    /* Across a frame end the records name the next frame and count their
       cycles from its start (566,203 CPU cycles in frame 0 of NTSC). */
    schway_run_result r2 = run(inst, 1000000u, SCHWAY_RUN_TRACE);
    CHECK_EQ_U64(r2.stop, SCHWAY_STOP_FRAME);
    CHECK_EQ_U64(r2.frame, 1u);
    (void)run(inst, 3000u, SCHWAY_RUN_TRACE);
    uint64_t start1 = UINT64_C(405350378496) / UINT64_C(715909);
    t = get(inst, RING - 1u);
    CHECK_EQ_U64(t.frame, 1u);
    CHECK(t.cycle >= start1);
    CHECK_EQ_U64(t.line, ((t.cycle - start1) * 11u / 7u) / 3413u);
    CHECK_EQ_U64(t.dot, ((t.cycle - start1) * 11u / 7u) % 3413u);
    schway_destroy(inst);
}

static void case_short_run(void)
{
    schway *inst = make();
    load(inst, program_count, 3u);
    schway_run_result res = run(inst, 1000u, SCHWAY_RUN_TRACE);
    CHECK_EQ_U64(res.stop, SCHWAY_STOP_BUDGET);
    uint32_t n = held(inst);
    CHECK(n >= 4u && n < 200u);
    check_shape(inst, n);

    /* All of it is held, from the first instruction, and each exec record
       follows the one before by its instruction's cycles. */
    schway_trace t = get(inst, 0u);
    CHECK_EQ_U64(t.event, SCHWAY_TRACE_EXEC);
    CHECK_EQ_HEX(t.pc, 0xBFC00000u);
    CHECK_EQ_U64(t.cycle, 0u);
    uint32_t execs = 0u;
    for (uint32_t i = 0u; i < n; i++) {
        if (get(inst, i).event == SCHWAY_TRACE_EXEC) {
            execs++;
        }
    }
    CHECK(execs >= 2u);
    CHECK(get(inst, n - 1u).cycle < res.ticks);

    /* A run that does not ask for the trace adds nothing to it. */
    (void)run(inst, 16u, 0u);
    CHECK_EQ_U64(held(inst), n);
    schway_destroy(inst);
}

/* A load from 1F000000h, the first expansion region, which answers nothing. */
static const uint32_t program_unmapped[] = {
    0x3C081F00u, /* lui t0, 1F00h */
    0x8D090000u, /* lw  t1, 0(t0) */
    0x00000000u, /* nop */
};

/* A handler written to the exception vector that returns to the instruction
   that raised, then a SYSCALL: the same EPC and code, taken again and again. */
static const uint32_t program_loop[] = {
    0x3C0A8000u, /* lui   t2, 8000h */
    0x3C09401Au, /* lui   t1, 401Ah */
    0x35297000u, /* ori   t1, t1, 7000h: mfc0 k0, epc */
    0xAD490080u, /* sw    t1, 80h(t2) */
    0xAD400084u, /* sw    zero, 84h(t2): nop */
    0x3C090340u, /* lui   t1, 0340h */
    0x35290008u, /* ori   t1, t1, 8: jr k0 */
    0xAD490088u, /* sw    t1, 88h(t2) */
    0x3C094200u, /* lui   t1, 4200h */
    0x35290010u, /* ori   t1, t1, 10h: rfe, in the delay slot */
    0xAD49008Cu, /* sw    t1, 8Ch(t2) */
    0x0000000Cu, /* syscall */
    0x00000000u, /* nop */
};

static void case_last_is_fault(void)
{
    schway *inst = make();
    load(inst, program_unmapped, 3u);
    schway_run_result res = run(inst, 100000u, SCHWAY_RUN_STOP_ON_FAULT | SCHWAY_RUN_TRACE);
    CHECK_EQ_U64(res.stop, SCHWAY_STOP_FAULT);
    uint32_t n = held(inst);
    CHECK(n > 3u);
    check_shape(inst, n);

    schway_fault f;
    memset(&f, 0, sizeof f);
    f.size = (uint32_t)sizeof f;
    CHECK_EQ_U64(schway_get_fault(inst, &f), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(f.kind, SCHWAY_FAULT_UNMAPPED_READ);

    /* The load, the read it made and the fault, in that order, the fault last
       and taken from the fault record. */
    schway_trace t = get(inst, n - 3u);
    CHECK_EQ_U64(t.event, SCHWAY_TRACE_EXEC);
    CHECK_EQ_HEX(t.pc, TEXT + 4u);
    schway_trace rd = get(inst, n - 2u);
    CHECK_EQ_U64(rd.event, SCHWAY_TRACE_RD);
    CHECK_EQ_HEX(rd.addr, 0x1F000000u);
    CHECK_EQ_U64(rd.device, SCHWAY_DEVICE_EXP1);
    schway_trace last = get(inst, n - 1u);
    CHECK_EQ_U64(last.event, SCHWAY_TRACE_FAULT);
    CHECK_EQ_U64(last.value, f.kind);
    CHECK_EQ_HEX(last.pc, f.pc);
    CHECK_EQ_HEX(last.addr, f.addr);
    CHECK_EQ_U64(last.width, f.width);
    CHECK_EQ_U64(last.device, f.device);
    CHECK_EQ_U64(last.note, f.note);
    CHECK_EQ_U64(last.frame, f.frame);
    CHECK_EQ_U64(last.cycle, t.cycle);
    schway_destroy(inst);

    /* An exception loop is the fault, with the loop count in note. More than
       4,096 records came before it, so the tail is full and ends with it. */
    inst = make();
    load(inst, program_loop, 13u);
    res = run(inst, 1000000u, SCHWAY_RUN_STOP_ON_FAULT | SCHWAY_RUN_TRACE);
    CHECK_EQ_U64(res.stop, SCHWAY_STOP_FAULT);
    CHECK_EQ_U64(held(inst), RING);
    last = get(inst, RING - 1u);
    CHECK_EQ_U64(last.event, SCHWAY_TRACE_FAULT);
    CHECK_EQ_U64(last.value, SCHWAY_FAULT_EXCEPTION_LOOP);
    CHECK_EQ_U64(last.note, 1000u);
    schway_destroy(inst);
}

static void case_index_past_end(void)
{
    schway *inst = make();
    schway_trace t;
    memset(&t, 0, sizeof t);
    t.size = (uint32_t)sizeof t;
    /* Nothing is held before a traced run. */
    CHECK_EQ_U64(schway_get_trace(inst, 0u, &t), SCHWAY_STATUS_ERR_ARGUMENT);
    load(inst, program_count, 3u);
    (void)run(inst, 16u, SCHWAY_RUN_TRACE);
    uint32_t n = held(inst);
    CHECK(n > 0u);
    CHECK_EQ_U64(schway_get_trace(inst, n - 1u, &t), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_get_trace(inst, n, &t), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_get_trace(inst, UINT32_MAX, &t), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_get_trace(NULL, 0u, &t), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_get_trace(inst, 0u, NULL), SCHWAY_STATUS_ERR_ARGUMENT);
    t.size = 8u;
    CHECK_EQ_U64(schway_get_trace(inst, 0u, &t), SCHWAY_STATUS_ERR_STRUCT_SIZE);

    /* A power reset empties the ring. */
    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    t.size = (uint32_t)sizeof t;
    CHECK_EQ_U64(schway_get_trace(inst, 0u, &t), SCHWAY_STATUS_ERR_ARGUMENT);
    schway_destroy(inst);
}

/* Opens the vertical-blank interrupt (I_MASK bit 0, SR.IM2 and IEc) and spins. */
static const uint32_t program_irq[] = {
    0x3C08BF80u, /* lui   t0, BF80h */
    0x34090001u, /* ori   t1, zero, 1 */
    0xAD091074u, /* sw    t1, 1074h(t0): I_MASK */
    0x340B0401u, /* ori   t3, zero, 401h */
    0x408B6000u, /* mtc0  t3, sr */
    0x08004005u, /* j     80010014h */
    0x00000000u, /* nop */
};

static void case_irq(void)
{
    schway *inst = make();
    load(inst, program_irq, 7u);
    schway_run_result res = run(inst, 100000u, SCHWAY_RUN_TRACE);
    while (res.stop != SCHWAY_STOP_FRAME) {
        res = run(inst, 100000u, SCHWAY_RUN_TRACE);
    }
    CHECK_EQ_U64(res.frame, 1u);
    /* The frame's end raised the interrupt; the next run takes it two
       retirements later, so its record leads the vector's own instructions. */
    (void)run(inst, 300u, SCHWAY_RUN_TRACE);
    uint32_t n = held(inst);
    uint32_t at = n;
    for (uint32_t i = 0u; i < n && at == n; i++) {
        if (get(inst, i).event == SCHWAY_TRACE_IRQ) {
            at = i;
        }
    }
    CHECK(at < n);
    if (at < n) {
        schway_trace t = get(inst, at);
        uint64_t start1 = UINT64_C(405350378496) / UINT64_C(715909);
        CHECK(t.pc == TEXT + 20u || t.pc == TEXT + 24u);
        CHECK_EQ_U64(t.frame, 1u);
        CHECK(t.cycle >= start1);
        CHECK_EQ_U64(t.device, SCHWAY_DEVICE_NONE);
        CHECK((t.value & 0x400u) != 0u); /* CAUSE shows the hardware line */
        CHECK(at + 1u < n);
        t = get(inst, at + 1u);
        CHECK_EQ_U64(t.event, SCHWAY_TRACE_EXEC);
        CHECK_EQ_HEX(t.pc, 0x80000080u);
    }
    schway_destroy(inst);
}

static void visit_name(void *user, const char *name, const void *data, uint32_t count,
                       uint32_t width)
{
    uint32_t *found = (uint32_t *)user;
    (void)data;
    (void)count;
    (void)width;
    if (strstr(name, "trace") != NULL) {
        (*found)++;
    }
}

/* Everything the program does: stores, loads, a bus error, an exception and the
   handler's spin. */
static const uint32_t program_mixed[] = {
    0x3C088002u, /* lui   t0, 8002h */
    0x24090055u, /* addiu t1, zero, 55h */
    0xAD090000u, /* sw    t1, 0(t0) */
    0x8D0A0000u, /* lw    t2, 0(t0) */
    0x00000000u, /* nop */
    0x3C0B1F00u, /* lui   t3, 1F00h */
    0x8D6C0000u, /* lw    t4, 0(t3) */
    0x00000000u, /* nop */
    0x08004008u, /* j     80010020h */
    0x00000000u, /* nop */
};

static void case_hash_equal(void)
{
    schway *a = make();
    schway *b = make();
    load(a, program_mixed, 10u);
    load(b, program_mixed, 10u);
    uint32_t found = 0u;
    CHECK_EQ_U64(schway_state_visit(a, visit_name, &found), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(found, 0u);

    for (uint32_t step = 0u; step < 4u; step++) {
        schway_run_result ra = run(a, 1000000u, SCHWAY_RUN_TRACE);
        schway_run_result rb = run(b, 1000000u, 0u);
        CHECK_EQ_U64(ra.stop, rb.stop);
        CHECK_EQ_U64(ra.frame, rb.frame);
        CHECK_EQ_U64(ra.ticks, rb.ticks);
        CHECK_EQ_U64(ra.fault_present, rb.fault_present);

        size_t na = schway_state_size(a);
        size_t nb = schway_state_size(b);
        CHECK_EQ_U64(na, nb);
        uint8_t *sa = (uint8_t *)malloc(na);
        uint8_t *sb = (uint8_t *)malloc(nb);
        if (sa == NULL || sb == NULL) {
            exit(1);
        }
        CHECK_EQ_U64(schway_save_state(a, sa, na), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(schway_save_state(b, sb, nb), SCHWAY_STATUS_OK);
        CHECK(na == nb && memcmp(sa, sb, na) == 0);
        free(sa);
        free(sb);

        schway_video va;
        schway_video vb;
        memset(&va, 0, sizeof va);
        memset(&vb, 0, sizeof vb);
        va.size = (uint32_t)sizeof va;
        vb.size = (uint32_t)sizeof vb;
        CHECK_EQ_U64(schway_get_video(a, &va), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(schway_get_video(b, &vb), SCHWAY_STATUS_OK);
        CHECK(va.width == vb.width && va.height == vb.height && va.pitch_bytes == vb.pitch_bytes);
        for (uint32_t y = 0u; y < va.height; y++) {
            const uint8_t *ra_row = (const uint8_t *)va.pixels + (size_t)y * va.pitch_bytes;
            const uint8_t *rb_row = (const uint8_t *)vb.pixels + (size_t)y * vb.pitch_bytes;
            CHECK(memcmp(ra_row, rb_row, va.width * 2u) == 0);
        }

        static int16_t audio_a[2u * 1024u];
        static int16_t audio_b[2u * 1024u];
        schway_audio aa;
        schway_audio ab;
        memset(&aa, 0, sizeof aa);
        memset(&ab, 0, sizeof ab);
        aa.size = (uint32_t)sizeof aa;
        ab.size = (uint32_t)sizeof ab;
        aa.samples = audio_a;
        ab.samples = audio_b;
        aa.capacity = 1024u;
        ab.capacity = 1024u;
        CHECK_EQ_U64(schway_get_audio(a, &aa), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(schway_get_audio(b, &ab), SCHWAY_STATUS_OK);
        CHECK(aa.written == ab.written && aa.dropped == ab.dropped);
        CHECK(memcmp(audio_a, audio_b, aa.written * 4u) == 0);

        schway_fault fa;
        schway_fault fb;
        memset(&fa, 0, sizeof fa);
        memset(&fb, 0, sizeof fb);
        fa.size = (uint32_t)sizeof fa;
        fb.size = (uint32_t)sizeof fb;
        CHECK_EQ_U64(schway_get_fault(a, &fa), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(schway_get_fault(b, &fb), SCHWAY_STATUS_OK);
        CHECK(memcmp(&fa, &fb, sizeof fa) == 0);
        CHECK_EQ_U64(fa.kind, SCHWAY_FAULT_UNMAPPED_READ);
    }

    /* The traced instance kept records, the other none: the flag took effect. */
    CHECK(held(a) > 100u);
    CHECK_EQ_U64(held(b), 0u);
    schway_destroy(a);
    schway_destroy(b);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        return 2;
    }
    if (strcmp(argv[1], "exec_records") == 0) {
        case_exec_records();
    } else if (strcmp(argv[1], "wrap") == 0) {
        case_wrap();
    } else if (strcmp(argv[1], "short_run") == 0) {
        case_short_run();
    } else if (strcmp(argv[1], "last_is_fault") == 0) {
        case_last_is_fault();
    } else if (strcmp(argv[1], "irq") == 0) {
        case_irq();
    } else if (strcmp(argv[1], "index_past_end") == 0) {
        case_index_past_end();
    } else if (strcmp(argv[1], "hash_equal") == 0) {
        case_hash_equal();
    } else {
        return 2;
    }
    CHECK_DONE();
}
