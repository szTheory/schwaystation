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
#include "../check.h"

#define MAX_FIELDS 64u
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
    one.flags = 2u;
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

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"budget_resume", test_budget_resume},
};

int main(int argc, char **argv)
{
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
