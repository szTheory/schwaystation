/* The bounded run, the split property, the side-effect-free peek and the
   first-fault record through the public header: schwaystation-test-run
   <case>. The test also sees src/, for the state layout and to latch a fault
   without a CPU to cause one. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "state.h"
#include "bus.h"
#include "dma.h"
#include "spu.h"
#include "../check.h"

#define MAX_FIELDS 256u
#define H_BODY_CRC 16u

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

static schway *make(uint32_t region)
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
    schway_profile p;
    memset(&p, 0, sizeof p);
    p.size = (uint32_t)sizeof p;
    p.region = region;
    CHECK_EQ_U64(schway_set_profile(inst, &p), SCHWAY_STATUS_OK);
    return inst;
}

/* A saved state in a buffer sized from the instance, so it follows the
   state as it grows. */
static uint8_t *save(const schway *inst, size_t *n)
{
    *n = schway_state_size(inst);
    uint8_t *buf = (uint8_t *)malloc(*n);
    if (buf == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(schway_save_state(inst, buf, *n), SCHWAY_STATUS_OK);
    return buf;
}

static void put_le(uint8_t *p, uint64_t v, uint32_t width)
{
    for (uint32_t i = 0; i < width; i++) {
        p[i] = (uint8_t)(v >> (8u * i));
    }
}

/* Recomputes the body CRC-32 after an edit, so a load reaches the field
   rules. */
static void reseal(uint8_t *b, size_t size)
{
    schway__state_seal(b, size);
}

static uint32_t field_offset(const schway *inst, const char *name)
{
    struct schway__state_field fields[MAX_FIELDS];
    size_t nf = schway__state_record(inst, fields, MAX_FIELDS);
    CHECK(nf <= MAX_FIELDS);
    for (size_t i = 0; i < nf && i < MAX_FIELDS; i++) {
        if (strcmp(fields[i].name, name) == 0) {
            return fields[i].offset;
        }
    }
    CHECK(0 && "state field not found");
    exit(1);
}

static schway_run_result run(schway *inst, uint64_t max_cycles, uint32_t flags)
{
    schway_budget b;
    memset(&b, 0, sizeof b);
    b.size = (uint32_t)sizeof b;
    b.flags = flags;
    b.max_cycles = max_cycles;
    schway_run_result r;
    memset(&r, 0, sizeof r);
    r.size = (uint32_t)sizeof r;
    CHECK_EQ_U64(schway_run(inst, &b, &r), SCHWAY_STATUS_OK);
    return r;
}

/* A budget stops a run at its cycle count, a larger one at the frame
   boundary, 0 cycles changes nothing, and a stopped run resumes to the
   boundary. The frame's end is part of the state, so a state saved mid-frame
   resumes too, and one whose end is not within a frame of its ticks is
   refused. */
static void test_budget_resume(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    size_t n0, n1;

    /* 0 cycles: BUDGET, no change, the profile still open. */
    uint8_t *before = save(inst, &n0);
    schway_run_result r = run(inst, 0u, 0u);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(r.ticks | r.frame | r.exit_code | r.fault_present, 0u);
    uint8_t *after = save(inst, &n1);
    CHECK(n0 == n1 && memcmp(before, after, n0) == 0);
    free(before);
    free(after);
    schway_profile p;
    memset(&p, 0, sizeof p);
    p.size = (uint32_t)sizeof p;
    p.region = SCHWAY_REGION_NTSC_U;
    CHECK_EQ_U64(schway_set_profile(inst, &p), SCHWAY_STATUS_OK);

    /* 1000 cycles: BUDGET at exactly 1000, with the one audio pair that
       ends the first 768 of them; the profile is now locked. */
    r = run(inst, 1000u, 0u);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(r.ticks, 1000u);
    CHECK_EQ_U64(r.frame, 0u);
    CHECK_EQ_U64(schway_set_profile(inst, &p), SCHWAY_STATUS_ERR_PROFILE_LOCKED);
    int16_t pcm[2 * 8];
    schway_audio audio;
    memset(&audio, 0, sizeof audio);
    audio.size = (uint32_t)sizeof audio;
    audio.samples = pcm;
    audio.capacity = 8u;
    CHECK_EQ_U64(schway_get_audio(inst, &audio), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(audio.written, 1u);

    /* A budget far past the frame stops at the boundary: the first NTSC
       frame is 566,203 cycles. */
    r = run(inst, UINT64_MAX, 0u);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    CHECK_EQ_U64(r.ticks, 566203u);
    CHECK_EQ_U64(r.frame, 1u);

    /* A budget that ends exactly on the boundary reports the frame. */
    schway *whole = make(SCHWAY_REGION_NTSC_U);
    r = run(whole, 566203u, 0u);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    CHECK_EQ_U64(r.frame, 1u);
    r = run(whole, 566202u, 0u);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(r.ticks, 566203u + 566202u);
    CHECK_EQ_U64(r.frame, 1u);
    /* Resume with the frame call: the second frame ends where run_frame
       alone would put it. */
    schway *plain = make(SCHWAY_REGION_NTSC_U);
    CHECK_EQ_U64(schway_run_frame(plain, NULL), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_run_frame(plain, &r), SCHWAY_STATUS_OK);
    schway_run_result w;
    memset(&w, 0, sizeof w);
    w.size = (uint32_t)sizeof w;
    CHECK_EQ_U64(schway_run_frame(whole, &w), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(w.stop, SCHWAY_STOP_FRAME);
    CHECK_EQ_U64(w.frame, 2u);
    CHECK_EQ_U64(w.ticks, r.ticks);
    CHECK_EQ_U64(w.exit_code | w.fault_present, 0u);
    size_t na, nb;
    uint8_t *sa = save(whole, &na);
    uint8_t *sb = save(plain, &nb);
    CHECK(na == nb && memcmp(sa, sb, na) == 0);
    free(sa);
    free(sb);
    schway_destroy(plain);

    /* A state saved mid-frame resumes to the same state as the instance it
       came from. */
    schway *a = make(SCHWAY_REGION_PAL);
    (void)run(a, 300000u, 0u);
    uint8_t *mid = save(a, &na);
    schway *b = make(SCHWAY_REGION_PAL);
    CHECK_EQ_U64(schway_load_state(b, mid, na), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(run(a, UINT64_MAX, 0u).stop, SCHWAY_STOP_FRAME);
    CHECK_EQ_U64(run(b, UINT64_MAX, 0u).stop, SCHWAY_STOP_FRAME);
    sa = save(a, &na);
    sb = save(b, &nb);
    CHECK(na == nb && memcmp(sa, sb, na) == 0);
    free(sa);
    free(sb);

    /* The frame's end is at or after the ticks and within one frame of
       them: PAL frames are at most 680,824 cycles. */
    uint32_t end_at = field_offset(b, "instance.frame_end");
    uint32_t ticks_at = field_offset(b, "instance.ticks");
    uint32_t spu_tick_at = field_offset(b, "spu.next_tick");
    const uint64_t ticks = 300000u;
    const uint64_t ends[] = {ticks - 1u, ticks + 680825u};
    for (size_t i = 0; i < sizeof ends / sizeof ends[0]; i++) {
        uint8_t *bad = (uint8_t *)malloc(na);
        if (bad == NULL) {
            exit(1);
        }
        memcpy(bad, mid, na);
        put_le(bad + end_at, ends[i], 8u);
        reseal(bad, na);
        sb = save(b, &nb);
        CHECK_EQ_U64(schway_load_state(b, bad, na), SCHWAY_STATUS_ERR_STATE_FIELD);
        uint8_t *kept = save(b, &nb);
        CHECK(memcmp(sb, kept, nb) == 0);
        free(sb);
        free(kept);
        free(bad);
    }
    uint8_t *ok = (uint8_t *)malloc(na);
    if (ok == NULL) {
        exit(1);
    }
    memcpy(ok, mid, na);
    put_le(ok + end_at, ticks + 680824u, 8u);
    reseal(ok, na);
    CHECK_EQ_U64(schway_load_state(b, ok, na), SCHWAY_STATUS_OK);
    memcpy(ok, mid, na);
    put_le(ok + end_at, ticks, 8u);
    reseal(ok, na);
    CHECK_EQ_U64(schway_load_state(b, ok, na), SCHWAY_STATUS_OK);

    /* Near the end of the counters a budget that fits runs and the next
       cycle is refused with nothing changed. */
    memcpy(ok, mid, na);
    put_le(ok + ticks_at, SCHWAY__STATE_TICKS_MAX - 5u, 8u);
    put_le(ok + end_at, SCHWAY__STATE_TICKS_MAX + 10u, 8u);
    put_le(ok + spu_tick_at, UINT64_MAX, 8u);
    reseal(ok, na);
    CHECK_EQ_U64(schway_load_state(b, ok, na), SCHWAY_STATUS_OK);
    r = run(b, 5u, 0u);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(r.ticks, SCHWAY__STATE_TICKS_MAX);
    sb = save(b, &nb);
    schway_budget one;
    memset(&one, 0, sizeof one);
    one.size = (uint32_t)sizeof one;
    one.max_cycles = 1u;
    schway_run_result out;
    memset(&out, 0xA5, sizeof out);
    out.size = (uint32_t)sizeof out;
    schway_run_result untouched = out;
    CHECK_EQ_U64(schway_run(b, &one, &out), SCHWAY_STATUS_ERR_STATE_FIELD);
    CHECK(memcmp(&out, &untouched, sizeof out) == 0);
    sa = save(b, &na);
    CHECK(na == nb && memcmp(sa, sb, na) == 0);
    CHECK_EQ_U64(schway_run_frame(b, NULL), SCHWAY_STATUS_ERR_STATE_FIELD);
    free(sa);
    free(sb);
    free(ok);
    free(mid);

    /* Size tags and flags: a refused call changes nothing. A larger tag
       from a newer header is accepted while its extra bytes are zero. */
    one.size = 8u;
    CHECK_EQ_U64(schway_run(a, &one, NULL), SCHWAY_STATUS_ERR_STRUCT_SIZE);
    schway_budget big[2];
    memset(big, 0, sizeof big);
    big[0].size = (uint32_t)sizeof big;
    big[0].max_cycles = 1u;
    CHECK_EQ_U64(schway_run(a, big, NULL), SCHWAY_STATUS_OK);
    ((uint8_t *)big)[sizeof big - 1u] = 1u;
    CHECK_EQ_U64(schway_run(a, big, NULL), SCHWAY_STATUS_ERR_STRUCT_SIZE);
    one.size = (uint32_t)sizeof one;
    one.flags = 4u; /* a bit no release defines */
    CHECK_EQ_U64(schway_run(a, &one, NULL), SCHWAY_STATUS_ERR_ARGUMENT);
    one.flags = 0u;
    CHECK_EQ_U64(schway_run(NULL, &one, NULL), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_run(a, NULL, NULL), SCHWAY_STATUS_ERR_ARGUMENT);
    /* A result that ends at its first released size takes the first four
       fields and leaves the rest as the caller set them; a smaller one is
       refused. */
    schway_run_result old;
    memset(&old, 0x5A, sizeof old);
    old.size = 24u;
    CHECK_EQ_U64(schway_run(a, &one, &old), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(old.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(old.ticks, a->ticks);
    CHECK_EQ_HEX(old.exit_code, 0x5A5A5A5Au);
    CHECK_EQ_HEX(old.fault_present, 0x5A5A5A5Au);
    old.size = 23u;
    CHECK_EQ_U64(schway_run(a, &one, &old), SCHWAY_STATUS_ERR_STRUCT_SIZE);

    schway_destroy(whole);
    schway_destroy(b);
    schway_destroy(a);
    schway_destroy(inst);
}

/* Runs n cycles in as many calls as the frame boundaries need. */
static void run_cycles(schway *inst, uint64_t n)
{
    while (n != 0u) {
        uint64_t before = inst->ticks;
        schway_run_result r = run(inst, n, 0u);
        CHECK(r.ticks > before);
        if (r.ticks <= before) {
            exit(1);
        }
        n -= r.ticks - before;
    }
}

static int16_t pcm_a[2 * 4096];
static int16_t pcm_b[2 * 4096];

static uint32_t drain(schway *inst, int16_t *pcm, uint32_t *dropped)
{
    schway_audio audio;
    memset(&audio, 0, sizeof audio);
    audio.size = (uint32_t)sizeof audio;
    audio.samples = pcm;
    audio.capacity = 4096u;
    CHECK_EQ_U64(schway_get_audio(inst, &audio), SCHWAY_STATUS_OK);
    *dropped = audio.dropped;
    return audio.written;
}

/* With no program loaded the advance has no instruction boundaries, so
   running n cycles and then m leaves what n + m cycles leave: the state
   bytes, the frame, the ticks and the audio pairs, including a split on a
   frame boundary and on a 768-cycle audio tick (ARCHITECTURE section 7). */
static void test_split_equivalence(void)
{
    static const uint32_t regions[] = {SCHWAY_REGION_NTSC_J, SCHWAY_REGION_NTSC_U,
                                       SCHWAY_REGION_PAL};
    static const uint64_t splits[][2] = {
        {1u, 1u},         {767u, 1u},        {768u, 1u},          {769u, 767u},
        {768u * 3u, 768u}, {566202u, 1u},    {566203u, 566203u},  {566203u, 1u},
        {566204u, 700000u}, {1u, 1132406u},  {2000000u, 2000000u}, {3000000u, 100u},
        {0u, 5000u},      {5000u, 0u},
    };
    for (size_t k = 0; k < sizeof regions / sizeof regions[0]; k++) {
        for (size_t i = 0; i < sizeof splits / sizeof splits[0]; i++) {
            schway *split = make(regions[k]);
            schway *whole = make(regions[k]);
            run_cycles(split, splits[i][0]);
            run_cycles(split, splits[i][1]);
            run_cycles(whole, splits[i][0] + splits[i][1]);
            CHECK_EQ_U64(split->ticks, whole->ticks);
            CHECK_EQ_U64(split->frame, whole->frame);
            size_t na, nb;
            uint8_t *sa = save(split, &na);
            uint8_t *sb = save(whole, &nb);
            CHECK(na == nb && memcmp(sa, sb, na) == 0);
            free(sa);
            free(sb);
            uint32_t da, db;
            uint32_t wa = drain(split, pcm_a, &da);
            uint32_t wb = drain(whole, pcm_b, &db);
            CHECK_EQ_U64(wa, wb);
            CHECK_EQ_U64(da, db);
            CHECK(memcmp(pcm_a, pcm_b, (size_t)2u * wa * sizeof pcm_a[0]) == 0);
            /* One pair per 768 cycles run, less any the ring dropped. */
            CHECK_EQ_U64((uint64_t)wa + da, whole->ticks / 768u);
            schway_destroy(split);
            schway_destroy(whole);
        }
    }
}

static schway_status peek(const schway *inst, uint32_t space, uint32_t addr, uint32_t width,
                          uint32_t *value)
{
    return schway_peek(inst, space, addr, width, value);
}

/* Reads at all three widths, little-endian, at the first and last address of
   each space. */
static void test_peek_values(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    inst->bus.ram[0] = 0x11u;
    inst->bus.ram[1] = 0x22u;
    inst->bus.ram[2] = 0x33u;
    inst->bus.ram[3] = 0x44u;
    inst->bus.ram[0x1FFFFC] = 0xA1u;
    inst->bus.ram[0x1FFFFD] = 0xB2u;
    inst->bus.ram[0x1FFFFE] = 0xC3u;
    inst->bus.ram[0x1FFFFF] = 0xD4u;
    inst->bus.scratch[0] = 0x55u;
    inst->bus.scratch[1] = 0x66u;
    inst->bus.scratch[0x3FC] = 0x01u;
    inst->bus.scratch[0x3FD] = 0x02u;
    inst->bus.scratch[0x3FE] = 0x03u;
    inst->bus.scratch[0x3FF] = 0xFFu;
    uint32_t v = 0u;
    CHECK_EQ_U64(peek(inst, SCHWAY_SPACE_RAM, 0u, 1u, &v), SCHWAY_STATUS_OK);
    CHECK_EQ_HEX(v, 0x11u);
    CHECK_EQ_U64(peek(inst, SCHWAY_SPACE_RAM, 2u, 1u, &v), SCHWAY_STATUS_OK);
    CHECK_EQ_HEX(v, 0x33u);
    CHECK_EQ_U64(peek(inst, SCHWAY_SPACE_RAM, 0u, 2u, &v), SCHWAY_STATUS_OK);
    CHECK_EQ_HEX(v, 0x2211u);
    CHECK_EQ_U64(peek(inst, SCHWAY_SPACE_RAM, 2u, 2u, &v), SCHWAY_STATUS_OK);
    CHECK_EQ_HEX(v, 0x4433u);
    CHECK_EQ_U64(peek(inst, SCHWAY_SPACE_RAM, 0u, 4u, &v), SCHWAY_STATUS_OK);
    CHECK_EQ_HEX(v, 0x44332211u);
    CHECK_EQ_U64(peek(inst, SCHWAY_SPACE_RAM, 0x1FFFFFu, 1u, &v), SCHWAY_STATUS_OK);
    CHECK_EQ_HEX(v, 0xD4u);
    CHECK_EQ_U64(peek(inst, SCHWAY_SPACE_RAM, 0x1FFFFEu, 2u, &v), SCHWAY_STATUS_OK);
    CHECK_EQ_HEX(v, 0xD4C3u);
    CHECK_EQ_U64(peek(inst, SCHWAY_SPACE_RAM, 0x1FFFFCu, 4u, &v), SCHWAY_STATUS_OK);
    CHECK_EQ_HEX(v, 0xD4C3B2A1u);
    CHECK_EQ_U64(peek(inst, SCHWAY_SPACE_SCRATCHPAD, 0u, 2u, &v), SCHWAY_STATUS_OK);
    CHECK_EQ_HEX(v, 0x6655u);
    CHECK_EQ_U64(peek(inst, SCHWAY_SPACE_SCRATCHPAD, 0x3FFu, 1u, &v), SCHWAY_STATUS_OK);
    CHECK_EQ_HEX(v, 0xFFu);
    CHECK_EQ_U64(peek(inst, SCHWAY_SPACE_SCRATCHPAD, 0x3FCu, 4u, &v), SCHWAY_STATUS_OK);
    CHECK_EQ_HEX(v, 0xFF030201u);
    /* The two spaces are separate memories: RAM offset 0 is not scratchpad 0. */
    CHECK_EQ_U64(peek(inst, SCHWAY_SPACE_RAM, 0x3FCu, 4u, &v), SCHWAY_STATUS_OK);
    CHECK_EQ_HEX(v, 0u);
    schway_destroy(inst);
}

/* Anything but a known space, a width of 1, 2 or 4, an aligned address inside
   the space and a place to put the value is refused, and the value is left
   alone. */
static void test_peek_rejects(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    const struct {
        uint32_t space, addr, width;
    } bad[] = {
        {0u, 0u, 1u},
        {3u, 0u, 1u},
        {UINT32_MAX, 0u, 1u},
        {SCHWAY_SPACE_RAM, 0u, 0u},
        {SCHWAY_SPACE_RAM, 0u, 3u},
        {SCHWAY_SPACE_RAM, 0u, 8u},
        {SCHWAY_SPACE_RAM, 1u, 2u},
        {SCHWAY_SPACE_RAM, 2u, 4u},
        {SCHWAY_SPACE_RAM, 3u, 4u},
        {SCHWAY_SPACE_RAM, 0x200000u, 1u},
        {SCHWAY_SPACE_RAM, 0x200000u, 4u},
        {SCHWAY_SPACE_RAM, 0x80000000u, 4u},
        {SCHWAY_SPACE_RAM, 0x1FFFFFu, 2u},
        {SCHWAY_SPACE_RAM, UINT32_MAX, 1u},
        {SCHWAY_SPACE_SCRATCHPAD, 0x400u, 1u},
        {SCHWAY_SPACE_SCRATCHPAD, 0x400u, 4u},
        {SCHWAY_SPACE_SCRATCHPAD, 0x3FFu, 2u},
        {SCHWAY_SPACE_SCRATCHPAD, 0x1F800000u, 4u},
    };
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        uint32_t v = 0xDEADBEEFu;
        CHECK_EQ_U64(peek(inst, bad[i].space, bad[i].addr, bad[i].width, &v),
                     SCHWAY_STATUS_ERR_ARGUMENT);
        CHECK_EQ_HEX(v, 0xDEADBEEFu);
    }
    uint32_t v = 0u;
    CHECK_EQ_U64(peek(NULL, SCHWAY_SPACE_RAM, 0u, 4u, &v), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(peek(inst, SCHWAY_SPACE_RAM, 0u, 4u, NULL), SCHWAY_STATUS_ERR_ARGUMENT);
    schway_destroy(inst);
}

/* Peeking every address at every width, and every refused one, leaves the
   whole instance as it was: the state, the unbuilt-device counters, the
   fault record and the TTY ring. */
static void test_peek_no_side_effects(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    run_cycles(inst, 3000u);
    for (uint32_t i = 0; i < 4096u; i++) {
        inst->bus.ram[i * 7u] = (uint8_t)(i * 13u + 1u);
    }
    schway__bus_unbuilt(inst, SCHWAY__DEV_GPU, 0x1F801814u, 4u, 0u, 0u);
    schway__fault_latch(inst, SCHWAY__FAULT_UNMAPPED_READ, SCHWAY__DEV_GPU, 4u, 0x80010000u,
                        0x8C220000u, 0x1F801814u);
    schway__tty_push(inst, 'x');
    schway *snapshot = (schway *)malloc(sizeof *snapshot);
    if (snapshot == NULL) {
        exit(1);
    }
    size_t n0, n1;
    uint8_t *before = save(inst, &n0);
    memcpy(snapshot, inst, sizeof *snapshot);
    uint32_t sum = 0u;
    for (uint32_t w = 1u; w <= 4u; w <<= 1) {
        for (uint32_t addr = 0u; addr < SCHWAY__RAM_SIZE; addr += w) {
            uint32_t v;
            CHECK_EQ_U64(peek(inst, SCHWAY_SPACE_RAM, addr, w, &v), SCHWAY_STATUS_OK);
            sum += v;
        }
        for (uint32_t addr = 0u; addr < SCHWAY__SCRATCH_SIZE; addr += w) {
            uint32_t v;
            CHECK_EQ_U64(peek(inst, SCHWAY_SPACE_SCRATCHPAD, addr, w, &v), SCHWAY_STATUS_OK);
            sum += v;
        }
        for (uint32_t addr = 1u; addr < 64u; addr += 2u) {
            uint32_t v;
            (void)peek(inst, SCHWAY_SPACE_RAM, addr, 2u, &v);
            (void)peek(inst, SCHWAY_SPACE_RAM, SCHWAY__RAM_SIZE + addr, w, &v);
            (void)peek(inst, 0u, addr, w, &v);
        }
    }
    CHECK(sum != 0u);
    CHECK(memcmp(snapshot, inst, sizeof *snapshot) == 0);
    uint8_t *after = save(inst, &n1);
    CHECK(n0 == n1 && memcmp(before, after, n0) == 0);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_READ);
    CHECK_EQ_U64(inst->bus.unbuilt_n[SCHWAY__DEV_GPU], 1u);
    CHECK_EQ_U64(inst->tty.count, 1u);
    free(before);
    free(after);
    free(snapshot);
    schway_destroy(inst);
}

/* No fault yet: the record is zero with its size tag kept, a larger caller
   struct keeps its tail and a smaller one is refused. */
static void test_fault_none(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    schway_fault f[2];
    memset(f, 0xA5, sizeof f);
    f[0].size = (uint32_t)sizeof f[0];
    CHECK_EQ_U64(schway_get_fault(inst, &f[0]), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(f[0].size, sizeof f[0]);
    CHECK_EQ_U64(f[0].kind | f[0].device | f[0].width | f[0].frame | f[0].ticks, 0u);
    CHECK_EQ_U64(f[0].line | f[0].dot | f[0].pc | f[0].opcode | f[0].addr | f[0].note, 0u);
    CHECK_EQ_HEX(f[1].kind, 0xA5A5A5A5u);
    schway_run_result r = run(inst, 100u, SCHWAY_RUN_STOP_ON_FAULT);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(r.fault_present, 0u);
    CHECK_EQ_U64(schway_run_frame(inst, &r), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(r.fault_present, 0u);
    memset(f, 0xA5, sizeof f);
    f[0].size = 2u * (uint32_t)sizeof f[0];
    CHECK_EQ_U64(schway_get_fault(inst, &f[0]), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(f[0].size, 2u * sizeof f[0]);
    CHECK_EQ_HEX(f[1].kind, 0xA5A5A5A5u);
    f[0].size = (uint32_t)sizeof f[0] - 1u;
    CHECK_EQ_U64(schway_get_fault(inst, &f[0]), SCHWAY_STATUS_ERR_STRUCT_SIZE);
    f[0].size = (uint32_t)sizeof f[0];
    CHECK_EQ_U64(schway_get_fault(NULL, &f[0]), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_get_fault(inst, NULL), SCHWAY_STATUS_ERR_ARGUMENT);
    schway_destroy(inst);
}

/* A latched fault is the first one only, and every field comes back through
   the public record, the run result and a state round trip; a run that asks
   to stop on a fault stops before any cycle, and a reset clears the record. */
static void test_fault_latched(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    run_cycles(inst, 1000u);
    schway__fault_latch(inst, SCHWAY__FAULT_MISALIGNED_WRITE, SCHWAY__DEV_RAM, 4u, 0x80010004u,
                        0xAC620001u, 0x00100001u);
    schway__fault_latch(inst, SCHWAY__FAULT_COP_UNUSABLE, SCHWAY__DEV_NONE, 0u, 1u, 2u, 3u);
    inst->fault.note = 77u;
    inst->fault.line = 5u;
    inst->fault.dot = 9u;
    schway_fault f;
    memset(&f, 0, sizeof f);
    f.size = (uint32_t)sizeof f;
    CHECK_EQ_U64(schway_get_fault(inst, &f), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(f.kind, SCHWAY_FAULT_MISALIGNED_WRITE);
    CHECK_EQ_U64(f.device, SCHWAY_DEVICE_RAM);
    CHECK_EQ_U64(f.width, 4u);
    CHECK_EQ_U64(f.frame, 0u);
    CHECK_EQ_U64(f.ticks, 1000u);
    CHECK_EQ_U64(f.line, 5u);
    CHECK_EQ_U64(f.dot, 9u);
    CHECK_EQ_HEX(f.pc, 0x80010004u);
    CHECK_EQ_HEX(f.opcode, 0xAC620001u);
    CHECK_EQ_HEX(f.addr, 0x00100001u);
    CHECK_EQ_U64(f.note, 77u);

    schway_run_result r = run(inst, 50u, 0u);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(r.ticks, 1050u);
    CHECK_EQ_U64(r.fault_present, 1u);
    CHECK_EQ_U64(schway_run_frame(inst, &r), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(r.fault_present, 1u);
    CHECK_EQ_U64(r.frame, 1u);
    /* The record keeps the frame and tick it was latched at. */
    CHECK_EQ_U64(schway_get_fault(inst, &f), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(f.ticks, 1000u);

    /* STOP_ON_FAULT stops before the first cycle; without the flag the run
       goes on. */
    uint64_t ticks = inst->ticks;
    r = run(inst, 5000u, SCHWAY_RUN_STOP_ON_FAULT);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FAULT);
    CHECK_EQ_U64(r.ticks, ticks);
    CHECK_EQ_U64(r.fault_present, 1u);
    CHECK_EQ_U64(inst->ticks, ticks);
    r = run(inst, 5000u, 0u);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(r.ticks, ticks + 5000u);

    size_t n;
    uint8_t *state = save(inst, &n);
    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_get_fault(inst, &f), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(f.kind | f.device | f.ticks | f.addr, 0u);
    r = run(inst, 10u, SCHWAY_RUN_STOP_ON_FAULT);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(schway_load_state(inst, state, n), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_get_fault(inst, &f), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(f.kind, SCHWAY_FAULT_MISALIGNED_WRITE);
    CHECK_EQ_U64(f.ticks, 1000u);
    free(state);
    schway_destroy(inst);
}

/* ---- the split property with the devices armed ---- */

#define TEXT 0x80010000u

#define T0 8u
#define T1 9u
#define T2 10u
#define T3 11u
#define T4 12u
#define T5 13u
#define T6 14u

#define I_LUI(rt, imm) (0x3C000000u | ((rt) << 16) | ((uint32_t)(imm) & 0xFFFFu))
#define I_ORI(rt, rs, imm) (0x34000000u | ((rs) << 21) | ((rt) << 16) | ((uint32_t)(imm) & 0xFFFFu))
#define I_SW(rt, off, rs) (0xAC000000u | ((rs) << 21) | ((rt) << 16) | ((uint32_t)(off) & 0xFFFFu))
#define I_LW(rt, off, rs) (0x8C000000u | ((rs) << 21) | ((rt) << 16) | ((uint32_t)(off) & 0xFFFFu))
#define I_BEQ(rs, rt, off) (0x10000000u | ((rs) << 21) | ((rt) << 16) | ((uint32_t)(off) & 0xFFFFu))
#define I_ADDU(rd, rs, rt) (((rs) << 21) | ((rt) << 16) | ((rd) << 11) | 0x21u)
#define I_OR(rd, rs, rt) (((rs) << 21) | ((rt) << 16) | ((rd) << 11) | 0x25u)
#define I_NOP 0u

/* Registers a case writes through the bus: interrupt status, the three
   channels' counters (count at +0, mode at +4, target at +8) and the OTC
   channel with the arbitration and interrupt registers it needs (psx-spx,
   Timers and DMA Channels). */
#define ISTAT 0x1F801070u
#define TCOUNT(n) (0x1F801100u + 0x10u * (n))
#define TMODE(n) (TCOUNT(n) + 4u)
#define TTARGET(n) (TCOUNT(n) + 8u)
#define OTC_MADR 0x1F8010E0u
#define OTC_BCR 0x1F8010E4u
#define OTC_CHCR 0x1F8010E8u
#define DPCR 0x1F8010F0u
#define DICR 0x1F8010F4u

/* The devices a case arms. Timer 2 runs on the system clock over 8 and
   interrupts at FFFFh in repeat mode; Timer 0 runs on the system clock, resets
   at a target of 1000 and interrupts there in repeat mode; the DMA is one
   60,000-word OTC unit, with the channel's interrupt enabled. */
#define ARM_TIMER2 1u
#define ARM_TIMER0 2u
#define ARM_DMA 4u
#define ARM_ALL (ARM_TIMER2 | ARM_TIMER0 | ARM_DMA)

#define OTC_WORDS 60000u

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

/* A PS-EXE of one text sector at 80010000h. It reads the interrupt status,
   acknowledges every bit, reads Timer 0 and Timer 2 and keeps what it saw: t4
   collects the status bits, t5 and t6 add up the two counts. The loop makes the
   registers depend on the exact tick of every access, and the acknowledgement
   lets each counter raise its next interrupt. */
static void load_loop(schway *inst)
{
    static uint8_t exe[0x1000];
    const uint32_t words[] = {
        I_LUI(T0, 0x1F80u),
        I_ORI(T0, T0, 0x1000u),
        I_LW(T1, 0x70u, T0),
        I_SW(0u, 0x70u, T0),
        I_LW(T2, 0x100u, T0),
        I_LW(T3, 0x120u, T0),
        I_OR(T4, T4, T1),
        I_ADDU(T5, T5, T2),
        I_ADDU(T6, T6, T3),
        I_BEQ(0u, 0u, -8),
        I_NOP,
    };
    memset(exe, 0, sizeof exe);
    memcpy(exe, "PS-X EXE", 8);
    put32(exe + 0x10, TEXT);
    put32(exe + 0x18, TEXT);
    put32(exe + 0x1C, 0x800u);
    for (uint32_t i = 0; i < sizeof words / sizeof words[0]; i++) {
        put32(exe + 0x800u + 4u * i, words[i]);
    }
    CHECK_EQ_U64(schway_load_exe(inst, exe, sizeof exe), SCHWAY_STATUS_OK);
}

/* One register write through the bus at the instance's current tick. */
static void dev_wr32(schway *inst, uint32_t addr, uint32_t v)
{
    uint32_t cycles = 0u;
    schway__sched_sync(inst);
    CHECK_EQ_U64(schway__bus_write(inst, addr, 4u, v, &cycles), 0u);
}

static void arm(schway *inst, uint32_t flags)
{
    if ((flags & ARM_TIMER2) != 0u) {
        dev_wr32(inst, TTARGET(2u), 0u);
        dev_wr32(inst, TMODE(2u), 0x200u | 0x20u | 0x40u);
    }
    if ((flags & ARM_TIMER0) != 0u) {
        dev_wr32(inst, TTARGET(0u), 1000u);
        dev_wr32(inst, TMODE(0u), 0x08u | 0x10u | 0x40u);
    }
    if ((flags & ARM_DMA) != 0u) {
        dev_wr32(inst, DPCR, 0x0F654321u);
        dev_wr32(inst, DICR, 0x00C00000u);
        dev_wr32(inst, OTC_MADR, 0x001FFFFCu);
        dev_wr32(inst, OTC_BCR, OTC_WORDS);
        dev_wr32(inst, OTC_CHCR, 0x11000000u);
    }
}

/* A fresh instance, with the loop loaded or not, and the devices armed at tick
   0. */
static schway *make_devices(uint32_t region, int with_exe, uint32_t flags)
{
    schway *inst = make(region);
    if (with_exe) {
        load_loop(inst);
    }
    arm(inst, flags);
    return inst;
}

/* Runs until the instance's tick has reached target, in as many calls as the
   frame boundaries need. A run stops on the first instruction boundary at or
   after its budget, so a split is made by targets, not by lengths: the same
   boundaries are passed whatever the split. */
static void run_to(schway *inst, uint64_t target)
{
    while (inst->ticks < target) {
        uint64_t before = inst->ticks;
        (void)run(inst, target - before, 0u);
        CHECK(inst->ticks > before);
        if (inst->ticks <= before) {
            exit(1);
        }
    }
}

/* The name of the state field the byte at offset off belongs to. */
static const char *field_at(const schway *inst, size_t off)
{
    struct schway__state_field fields[MAX_FIELDS];
    size_t nf = schway__state_record(inst, fields, MAX_FIELDS);
    const char *name = "header";
    for (size_t i = 0; i < nf && i < MAX_FIELDS; i++) {
        if (fields[i].offset <= off) {
            name = fields[i].name;
        }
    }
    return name;
}

/* The two instances' saved states are the same bytes. A difference prints the
   first differing offset and the field it is in, which names the device whose
   state depends on where the run was cut. */
static void same_state(const schway *split, const schway *whole, const char *what)
{
    size_t na, nb;
    uint8_t *sa = save(split, &na);
    uint8_t *sb = save(whole, &nb);
    CHECK(na == nb);
    if (na == nb && memcmp(sa, sb, na) != 0) {
        size_t at = 0u;
        while (sa[at] == sb[at]) {
            at++;
        }
        fprintf(stderr, "%s: state differs at byte %llu (field %s)\n", what,
                (unsigned long long)at, field_at(whole, at));
        CHECK(0 && "split state differs from the unsplit run");
    }
    free(sa);
    free(sb);
}

/* Runs n cycles then m on one instance and n + m on another, from the same
   start, and checks the ticks, frame, state bytes and audio are the same. A run
   long enough for every armed counter to have fired also checks that the
   devices did something, so the comparison is not of two idle machines. */
static void check_split(uint32_t region, int with_exe, uint32_t flags, uint64_t n, uint64_t m)
{
    char what[96];
    snprintf(what, sizeof what, "region %u exe %d arm %u split %llu+%llu", (unsigned)region,
             with_exe, (unsigned)flags, (unsigned long long)n, (unsigned long long)m);
    schway *split = make_devices(region, with_exe, flags);
    schway *whole = make_devices(region, with_exe, flags);
    run_to(split, n);
    run_to(split, n + m);
    run_to(whole, n + m);
    CHECK_EQ_U64(split->ticks, whole->ticks);
    CHECK_EQ_U64(split->frame, whole->frame);
    same_state(split, whole, what);
    uint32_t da, db;
    uint32_t wa = drain(split, pcm_a, &da);
    uint32_t wb = drain(whole, pcm_b, &db);
    CHECK_EQ_U64(wa, wb);
    CHECK_EQ_U64(da, db);
    CHECK(memcmp(pcm_a, pcm_b, (size_t)2u * wa * sizeof pcm_a[0]) == 0);
    CHECK_EQ_U64((uint64_t)wa + da, whole->ticks / 768u);
    if (n + m >= 1200000u) {
        /* The program keeps the status bits it saw; without it the bits stay
           set in I_STAT, which nothing acknowledges. */
        uint32_t seen = with_exe ? whole->cpu.r[T4] : whole->irq.stat;
        if ((flags & ARM_TIMER2) != 0u) {
            CHECK_EQ_HEX(seen & 0x40u, 0x40u);
        }
        if ((flags & ARM_TIMER0) != 0u) {
            CHECK_EQ_HEX(seen & 0x10u, 0x10u);
        }
        if ((flags & ARM_DMA) != 0u) {
            CHECK_EQ_HEX(seen & 0x08u, 0x08u);
        }
    }
    schway_destroy(split);
    schway_destroy(whole);
}

/* Splits at the places a carried remainder or a derived deadline could make
   the result depend on the cut: a tick on each side of the first Timer 0
   deadline, of a later deadline found by running a probe, and of the OTC unit's
   end, and the middle of the unit, where the CPU is held and the rest of the
   hold is carried in the state. With no program loaded a run stops exactly on
   the tick asked for, so the split on a deadline is exact; with the program the
   run stops on the instruction boundary at or after it. */
static void check_edges(uint32_t region, int with_exe, uint32_t flags)
{
    uint64_t at[16];
    size_t n = 0u;
    schway *probe = make_devices(region, with_exe, flags);
    uint64_t d = schway__timers_next(probe);
    CHECK(d != UINT64_MAX);
    at[n++] = d - 1u;
    at[n++] = d;
    at[n++] = d + 1u;
    if ((flags & ARM_DMA) != 0u) {
        uint64_t u = probe->dma.busy_until;
        CHECK(d < u);
        at[n++] = u / 2u;
        at[n++] = u - 1u;
        at[n++] = u;
        at[n++] = u + 1u;
    }
    /* A deadline after the first unit and the first interrupts: step the probe
       until a counter has one armed. */
    run_to(probe, 100000u);
    for (uint32_t i = 0; i < 4000u && schway__timers_next(probe) == UINT64_MAX; i++) {
        (void)run(probe, 1u, 0u);
    }
    uint64_t later = schway__timers_next(probe);
    if (with_exe) {
        CHECK(later != UINT64_MAX);
    }
    if (later != UINT64_MAX) {
        at[n++] = later - 1u;
        at[n++] = later;
        at[n++] = later + 1u;
    }
    schway_destroy(probe);
    for (size_t i = 0; i < n; i++) {
        check_split(region, with_exe, flags, at[i], 200000u);
    }
}

#define SPU_BASE 0x1F801C00u
#define SPU_ATTR (SPU_BASE + 0x1AAu)
#define SPU_KON (SPU_BASE + 0x188u)
#define DMA4_MADR 0x1F8010C0u
#define DMA4_BCR 0x1F8010C4u
#define DMA4_CHCR 0x1F8010C8u
#define DMA_DPCR 0x1F8010F0u

static void spu_write(schway *inst, uint32_t address, uint32_t width, uint32_t value)
{
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, address, width, value, &cycles), 0u);
}

static void spu_deadline_is_future(const schway *inst)
{
    CHECK(inst->spu.next_tick == UINT64_MAX ||
          (inst->spu.next_tick > inst->ticks &&
           inst->spu.next_tick - inst->ticks <= SCHWAY__CYCLES_PER_PAIR));
    CHECK(inst->sched.next > inst->ticks);
}

static void check_muted_voice(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    const uint32_t voice = SPU_BASE;
    /* A looped, nonzero BRR block makes the voice decode and envelope advance
       while the main volumes remain zero. */
    schway__spu_ram_write(inst, 0u, 0x0377u);
    for (uint32_t i = 1u; i < 8u; i++) {
        schway__spu_ram_write(inst, 2u * i, 0x7777u);
    }
    spu_write(inst, voice, 2u, 0x3FFFu);
    spu_write(inst, voice + 2u, 2u, 0x3FFFu);
    spu_write(inst, voice + 4u, 2u, 0x1000u);
    spu_write(inst, voice + 6u, 2u, 0u);
    spu_write(inst, voice + 8u, 2u, 0x7F00u);
    spu_write(inst, voice + 10u, 2u, 0x1FC0u);
    spu_write(inst, SPU_BASE + 0x180u, 2u, 0u);
    spu_write(inst, SPU_BASE + 0x182u, 2u, 0u);
    spu_write(inst, SPU_ATTR, 2u, 0xC000u);
    spu_write(inst, SPU_KON, 2u, 1u);
    schway_run_result result = run(inst, 8u * SCHWAY__CYCLES_PER_PAIR, 0u);
    CHECK_EQ_U64(result.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(result.ticks, 8u * SCHWAY__CYCLES_PER_PAIR);
    CHECK_EQ_U64(inst->audio_count, 8u);
    CHECK(inst->spu.voice[0].sample != 0 || inst->spu.voice[0].output != 0);
    CHECK(inst->spu.voice[0].sample_counter != (UINT32_C(31) << 27));
    spu_deadline_is_future(inst);
    int16_t pcm[16];
    schway_audio audio;
    memset(&audio, 0, sizeof audio);
    audio.size = (uint32_t)sizeof audio;
    audio.samples = pcm;
    audio.capacity = 8u;
    CHECK_EQ_U64(schway_get_audio(inst, &audio), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(audio.written, 8u);
    for (uint32_t i = 0u; i < 2u * audio.written; i++) {
        CHECK_EQ_U64((uint16_t)pcm[i], 0u);
    }
    schway_destroy(inst);
}

static void check_dma_busy_bound(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    for (uint32_t i = 0u; i < 16u; i++) {
        spu_write(inst, 0x80002000u + 4u * i, 4u, 0xA5000000u | i);
    }
    spu_write(inst, SPU_BASE + 0x1A6u, 2u, 0x20u);
    spu_write(inst, SPU_ATTR, 2u, 0x0020u);
    schway_run_result result = run(inst, SCHWAY__CYCLES_PER_PAIR, 0u);
    CHECK_EQ_U64(result.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(inst->ticks, SCHWAY__CYCLES_PER_PAIR);
    spu_write(inst, DMA_DPCR, 4u, 0x0FEDCBA9u);
    spu_write(inst, DMA4_MADR, 4u, 0x2000u);
    spu_write(inst, DMA4_BCR, 4u, 0x00010010u);
    spu_write(inst, DMA4_CHCR, 4u, 0x01000201u);
    CHECK_EQ_U64(inst->dma.cur, 4u);
    CHECK(inst->dma.busy_until > inst->ticks);
    uint64_t before = inst->ticks;
    result = run(inst, 32u, 0u);
    CHECK_EQ_U64(result.stop, SCHWAY_STOP_BUDGET);
    CHECK(result.ticks >= before);
    CHECK(result.ticks - before <= 32u);
    CHECK((inst->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY) != 0u);
    CHECK(inst->dma.busy_until > inst->ticks);
    spu_deadline_is_future(inst);
    schway_destroy(inst);
}

static void check_maximum_loaded_state(void)
{
    schway *source = make(SCHWAY_REGION_NTSC_U);
    schway *loaded = make(SCHWAY_REGION_NTSC_U);
    struct schway__spu *spu = &source->spu;
    spu->transfer_head = SCHWAY__SPU_FIFO_HALFWORDS - 1u;
    spu->transfer_count = SCHWAY__SPU_FIFO_HALFWORDS;
    spu->transfer_address = SCHWAY__SPU_RAM_SIZE - 2u;
    spu->pending_count = SCHWAY__SPU_PENDING_WRITES;
    for (uint32_t i = 0u; i < SCHWAY__SPU_FIFO_HALFWORDS; i++) {
        spu->transfer_fifo[i] = (uint16_t)(UINT16_MAX - i);
    }
    for (uint32_t i = 0u; i < SCHWAY__SPU_PENDING_WRITES; i++) {
        spu->pending[i].address = 0x01A6u;
        spu->pending[i].value = UINT16_MAX;
    }
    spu->noise_phase = 0x1FFFFu;
    spu->noise_lfsr = UINT16_MAX;
    spu->capture_cursor = 0x1FFu;
    spu->reverb_fir_cursor = SCHWAY__SPU_REVERB_FIR_TAPS - 1u;
    spu->reverb_work_cursor = SCHWAY__SPU_REVERB_WORK_WORDS - 1u;
    spu->reverb_address = SCHWAY__SPU_RAM_SIZE - 8u;
    spu->irqa_addr = SCHWAY__SPU_RAM_SIZE - 8u;
    size_t size = 0u;
    uint8_t *state = save(source, &size);
    CHECK_EQ_U64(schway_load_state(loaded, state, size), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(loaded->spu.pending_count, SCHWAY__SPU_PENDING_WRITES);
    CHECK_EQ_U64(loaded->spu.transfer_head, SCHWAY__SPU_FIFO_HALFWORDS - 1u);
    CHECK_EQ_U64(loaded->spu.transfer_count, SCHWAY__SPU_FIFO_HALFWORDS);
    uint64_t before = loaded->ticks;
    schway_run_result result = run(loaded, SCHWAY__CYCLES_PER_PAIR, 0u);
    CHECK_EQ_U64(result.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(result.ticks - before, SCHWAY__CYCLES_PER_PAIR);
    CHECK_EQ_U64(loaded->spu.pending_count, 0u);
    CHECK(loaded->spu.transfer_address < SCHWAY__SPU_RAM_SIZE);
    spu_deadline_is_future(loaded);
    free(state);
    schway_destroy(loaded);
    schway_destroy(source);
}

/* Fault prevented: a no-content frame that polls per CPU cycle instead of
   advancing to scheduled sample deadlines, or an idle/muted/busy state that
   escapes the public run bound. The PS1 SPU schedules one sample event every
   768 CPU cycles (.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md section 1.1). */
static void test_no_busy_loop(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    CHECK_EQ_U64(inst->spu.next_tick, SCHWAY__CYCLES_PER_PAIR);
    CHECK_EQ_U64(inst->audio_count, 0u);
    schway_run_result result = run(inst, 0u, 0u);
    CHECK_EQ_U64(result.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(result.ticks, 0u);
    CHECK_EQ_U64(inst->spu.next_tick, SCHWAY__CYCLES_PER_PAIR);
    CHECK_EQ_U64(inst->audio_count, 0u);

    result = run(inst, SCHWAY__CYCLES_PER_PAIR - 1u, 0u);
    CHECK_EQ_U64(result.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(result.ticks, SCHWAY__CYCLES_PER_PAIR - 1u);
    CHECK_EQ_U64(inst->audio_count, 0u);
    result = run(inst, 1u, 0u);
    CHECK_EQ_U64(result.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(result.ticks, SCHWAY__CYCLES_PER_PAIR);
    CHECK_EQ_U64(inst->audio_count, 1u);
    CHECK_EQ_U64(inst->spu.next_tick, 2u * SCHWAY__CYCLES_PER_PAIR);
    result = run(inst, 2u * SCHWAY__CYCLES_PER_PAIR, 0u);
    CHECK_EQ_U64(result.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(result.ticks, 3u * SCHWAY__CYCLES_PER_PAIR);
    CHECK_EQ_U64(inst->audio_count, 3u);
    CHECK_EQ_U64(inst->spu.next_tick, 4u * SCHWAY__CYCLES_PER_PAIR);
    CHECK_EQ_U64(inst->audio_count, inst->ticks / SCHWAY__CYCLES_PER_PAIR);
    spu_deadline_is_future(inst);
    schway_destroy(inst);

    check_muted_voice();
    check_dma_busy_bound();
    check_maximum_loaded_state();
}

static uint32_t perf_frames(int argc, char **argv)
{
    char *end = NULL;
    unsigned long value;
    if (argc != 3) {
        fprintf(stderr, "schwaystation-test-run: SPU perf modes need a frame count\n");
        exit(2);
    }
    value = strtoul(argv[2], &end, 10);
    if (end == argv[2] || *end != '\0' || value == 0u || value > 1200u) {
        fprintf(stderr, "schwaystation-test-run: invalid bounded frame count\n");
        exit(2);
    }
    return (uint32_t)value;
}

/* Cachegrind control: same creation and exact rational frame-end arithmetic
   as the public no-content frame path, without running any device event. */
static void perf_spu_idle_control(uint32_t frames)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    uint64_t num = schway__instance_frame_num(inst->region);
    uint64_t den = schway__instance_frame_den(inst->region);
    for (uint32_t i = 0u; i < frames; i++) {
        uint64_t end = inst->frame_end;
        CHECK(end >= inst->ticks);
        inst->ticks = end;
        inst->frame++;
        inst->tick_acc = (inst->tick_acc + num) % den;
        inst->frame_end += (inst->tick_acc + num) / den;
    }
    CHECK_EQ_U64(inst->frame, frames);
    schway_destroy(inst);
}

/* Cachegrind workload: real public idle frames, with no BIOS or guest code. */
static void perf_spu_idle(uint32_t frames)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    for (uint32_t i = 0u; i < frames; i++) {
        schway_run_result result;
        memset(&result, 0, sizeof result);
        result.size = (uint32_t)sizeof result;
        CHECK_EQ_U64(schway_run_frame(inst, &result), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(result.stop, SCHWAY_STOP_FRAME);
        CHECK_EQ_U64(result.frame, (uint64_t)i + 1u);
    }
    CHECK_EQ_U64(inst->frame, frames);
    schway_destroy(inst);
}

/* Fault prevented: a device whose time depends on where a run was cut, so a
   host that runs in slices gets different timer interrupts, DMA or CPU
   registers than one that runs a whole span. Timer 2 on the system clock over 8
   interrupts at FFFFh while a program polls and acknowledges it; a run split
   after the first of those interrupts and the first frame's end gives the
   unsplit run's state bytes, frame, ticks and audio. Then Timer 0 counts to a
   target and a 60,000-word OTC unit is in flight over the splits of the plain
   equivalence case and over the edges above, with and without the program, in
   every region. */
static void test_split_devices(void)
{
    static const uint32_t regions[] = {SCHWAY_REGION_NTSC_J, SCHWAY_REGION_NTSC_U,
                                       SCHWAY_REGION_PAL};
    static const uint64_t splits[][2] = {
        {1u, 1u},         {767u, 1u},        {768u, 1u},          {769u, 767u},
        {768u * 3u, 768u}, {566202u, 1u},    {566203u, 566203u},  {566203u, 1u},
        {566204u, 700000u}, {1u, 1132406u},  {2000000u, 2000000u}, {3000000u, 100u},
        {0u, 5000u},      {5000u, 0u},
    };
    check_split(SCHWAY_REGION_NTSC_U, 1, ARM_TIMER2, 600000u, 600000u);
    for (size_t k = 0; k < sizeof regions / sizeof regions[0]; k++) {
        for (size_t i = 0; i < sizeof splits / sizeof splits[0]; i++) {
            check_split(regions[k], 0, ARM_ALL, splits[i][0], splits[i][1]);
        }
        /* The two NTSC regions share their frame length, so the runs with the
           program are made in NTSC-U and PAL; those of several million cycles
           in NTSC-U alone, to keep the case short. */
        if (regions[k] == SCHWAY_REGION_NTSC_J) {
            continue;
        }
        for (size_t i = 0; i < sizeof splits / sizeof splits[0]; i++) {
            if (splits[i][0] + splits[i][1] < 2000000u || regions[k] == SCHWAY_REGION_NTSC_U) {
                check_split(regions[k], 1, ARM_ALL, splits[i][0], splits[i][1]);
            }
        }
        check_edges(regions[k], 1, ARM_ALL);
        check_edges(regions[k], 0, ARM_ALL);
        if (regions[k] == SCHWAY_REGION_NTSC_U) {
            check_edges(regions[k], 1, ARM_TIMER2);
            check_edges(regions[k], 0, ARM_TIMER2);
        }
    }
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"budget_resume", test_budget_resume},
    {"fault_latched", test_fault_latched},
    {"fault_none", test_fault_none},
    {"peek_no_side_effects", test_peek_no_side_effects},
    {"peek_rejects", test_peek_rejects},
    {"peek_values", test_peek_values},
    {"no_busy_loop", test_no_busy_loop},
    {"split_devices", test_split_devices},
    {"split_equivalence", test_split_equivalence},
};

int main(int argc, char **argv)
{
    if (argc >= 2 && argc <= 3 && strcmp(argv[1], "spu_idle_control") == 0) {
        perf_spu_idle_control(perf_frames(argc, argv));
        CHECK_DONE();
    }
    if (argc >= 2 && argc <= 3 && strcmp(argv[1], "spu_idle") == 0) {
        perf_spu_idle(perf_frames(argc, argv));
        CHECK_DONE();
    }
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-run <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "schwaystation-test-run: unknown case %s\n", argv[1]);
    return 2;
}
