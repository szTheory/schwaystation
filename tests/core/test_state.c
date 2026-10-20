/* Save states and the field visitor through the public header:
   schwaystation-test-state <case>. The layout is ARCHITECTURE section 5: a
   header (magic, state format, total size, body CRC-32), then every field
   little-endian in visit order. The test also sees src/, for the header
   size, the field record and the CRC. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "state.h"
#include "../check.h"

#define STATE_BUF (SCHWAY__VRAM_W * SCHWAY__VRAM_H * 2u + 4096u)
#define MAX_FIELDS 64u
#define H_FORMAT 8u
#define H_TOTAL 12u
#define H_BODY_CRC 16u

static uint8_t good[STATE_BUF];
static uint8_t bad[STATE_BUF];
static uint8_t before[STATE_BUF];
static uint8_t after[STATE_BUF];
static uint8_t stream[STATE_BUF]; /* the visitor's own serialisation */
static struct schway__state_field fields[MAX_FIELDS];

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

static void run_to(schway *inst, uint64_t frame)
{
    while (inst->frame < frame) {
        CHECK_EQ_U64(schway_run_frame(inst, NULL), SCHWAY_STATUS_OK);
    }
}

static size_t save(const schway *inst, uint8_t *buf)
{
    size_t n = schway_state_size(inst);
    CHECK(n > SCHWAY__STATE_HEADER_SIZE && n <= STATE_BUF);
    if (n <= SCHWAY__STATE_HEADER_SIZE || n > STATE_BUF) {
        exit(1);
    }
    CHECK_EQ_U64(schway_save_state(inst, buf, n), SCHWAY_STATUS_OK);
    return n;
}

/* FNV-1a 64 over the display words, low byte first. */
static uint64_t frame_hash(const schway *inst)
{
    schway_video v;
    memset(&v, 0, sizeof v);
    v.size = (uint32_t)sizeof v;
    CHECK_EQ_U64(schway_get_video(inst, &v), SCHWAY_STATUS_OK);
    uint64_t hash = UINT64_C(14695981039346656037);
    for (uint32_t y = 0; y < v.height; y++) {
        const uint8_t *row = (const uint8_t *)v.pixels + (size_t)y * v.pitch_bytes;
        for (uint32_t i = 0; i < 2u * v.width; i++) {
            hash = (hash ^ row[i]) * UINT64_C(1099511628211);
        }
    }
    return hash;
}

static void put_le(uint8_t *p, uint64_t v, uint32_t width)
{
    for (uint32_t i = 0; i < width; i++) {
        p[i] = (uint8_t)(v >> (8u * i));
    }
}

/* Recomputes the body CRC-32 after an edit, so a load gets past the CRC
   check to the step a case names. */
static void reseal(uint8_t *b, size_t size)
{
    uint32_t crc = schway__crc32_update(0u, b + SCHWAY__STATE_HEADER_SIZE,
                                        size - SCHWAY__STATE_HEADER_SIZE);
    put_le(b + H_BODY_CRC, crc, 4u);
}

/* Loads b into inst expecting want, and checks the instance saves the same
   bytes as before the call. */
static void expect_refused(const char *what, schway *inst, const uint8_t *b, size_t size,
                           schway_status want)
{
    size_t n = save(inst, before);
    schway_status got = schway_load_state(inst, b, size);
    if (got != want) {
        fprintf(stderr, "%s: load status %u, want %u\n", what, (unsigned)got, (unsigned)want);
        check_failures++;
    }
    if (save(inst, after) != n || memcmp(before, after, n) != 0) {
        fprintf(stderr, "%s: a refused load changed the instance\n", what);
        check_failures++;
    }
}

/* Run to 30 and save, run to 60; load the save and run to 60 again: the
   same frame and the same state. */
static void test_round_trip(void)
{
    static const uint32_t regions[] = {SCHWAY_REGION_NTSC_U, SCHWAY_REGION_PAL};
    for (size_t k = 0; k < 2u; k++) {
        schway *inst = make(regions[k]);
        run_to(inst, 30);
        size_t n = save(inst, good);
        run_to(inst, 60);
        uint64_t h1 = frame_hash(inst);
        size_t m = save(inst, before);
        CHECK_EQ_U64(m, n);
        CHECK_EQ_U64(schway_load_state(inst, good, n), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(inst->frame, 30);
        run_to(inst, 60);
        uint64_t h2 = frame_hash(inst);
        CHECK_EQ_HEX(h1, h2);
        save(inst, after);
        CHECK(memcmp(before, after, n) == 0);
        /* And into a second instance of the same region. */
        schway *other = make(regions[k]);
        CHECK_EQ_U64(schway_load_state(other, good, n), SCHWAY_STATUS_OK);
        run_to(other, 60);
        CHECK_EQ_HEX(frame_hash(other), h1);
        schway_destroy(other);
        schway_destroy(inst);
    }
}

/* Save, load, save: the same bytes. */
static void test_stable_bytes(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    run_to(inst, 45);
    size_t n = save(inst, good);
    CHECK_EQ_U64(schway_load_state(inst, good, n), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(save(inst, after), n);
    CHECK(memcmp(good, after, n) == 0);
    schway_destroy(inst);
}

/* Each corruption is refused with its own status before any byte applies. */
static void test_rejects(void)
{
    schway *src = make(SCHWAY_REGION_NTSC_U);
    run_to(src, 60);
    size_t n = save(src, good);
    schway *target = make(SCHWAY_REGION_NTSC_U);
    run_to(target, 5);

    memcpy(bad, good, n);
    bad[3] ^= 0x01u;
    expect_refused("magic", target, bad, n, SCHWAY_STATUS_ERR_STATE_MAGIC);
    expect_refused("short magic", target, good, 4u, SCHWAY_STATUS_ERR_STATE_MAGIC);

    memcpy(bad, good, n);
    bad[H_FORMAT] ^= 0x02u;
    expect_refused("format", target, bad, n, SCHWAY_STATUS_ERR_STATE_VERSION);

    memcpy(bad, good, n);
    bad[H_TOTAL] ^= 0x01u;
    expect_refused("size word", target, bad, n, SCHWAY_STATUS_ERR_STATE_FIELD);
    expect_refused("one byte short", target, good, n - 1u, SCHWAY_STATUS_ERR_STATE_FIELD);

    memcpy(bad, good, n);
    bad[n - 1u] ^= 0x10u;
    expect_refused("body byte", target, bad, n, SCHWAY_STATUS_ERR_STATE_CRC);
    memcpy(bad, good, n);
    bad[H_BODY_CRC] ^= 0x01u;
    expect_refused("crc word", target, bad, n, SCHWAY_STATUS_ERR_STATE_CRC);

    /* Another valid region, resealed: the CRC passes and the field check
       refuses it, since a state loads only into its own region. */
    size_t nf = schway__state_record(src, fields, MAX_FIELDS);
    CHECK(nf <= MAX_FIELDS);
    uint32_t region_at = 0;
    for (size_t i = 0; i < nf && i < MAX_FIELDS; i++) {
        if (strcmp(fields[i].name, "instance.region") == 0) {
            region_at = fields[i].offset;
        }
    }
    CHECK(region_at >= SCHWAY__STATE_HEADER_SIZE);
    if (region_at >= SCHWAY__STATE_HEADER_SIZE) {
        static const uint32_t others[] = {SCHWAY_REGION_NTSC_J, SCHWAY_REGION_PAL};
        for (size_t k = 0; k < 2u; k++) {
            memcpy(bad, good, n);
            put_le(bad + region_at, others[k], 4u);
            reseal(bad, n);
            expect_refused("region", target, bad, n, SCHWAY_STATUS_ERR_STATE_FIELD);
        }
    }

    size_t m = schway_state_size(target);
    CHECK_EQ_U64(schway_save_state(target, after, m - 1u), SCHWAY_STATUS_ERR_BUFFER_TOO_SMALL);
    CHECK_EQ_U64(schway_save_state(target, after, 0u), SCHWAY_STATUS_ERR_BUFFER_TOO_SMALL);

    /* The control: the untouched state, resealed, loads. */
    memcpy(bad, good, n);
    reseal(bad, n);
    CHECK(memcmp(bad, good, n) == 0);
    CHECK_EQ_U64(schway_load_state(target, bad, n), SCHWAY_STATUS_OK);
    schway_destroy(target);
    schway_destroy(src);
}

/* The visitor's own little-endian serialisation of what it is handed. */
struct visit_ctx {
    size_t n;            /* bytes written to stream */
    size_t calls;
    int bad_width, bad_name, overflow;
    int saw_frame, saw_vram;
    const char *names[MAX_FIELDS];
};

static void on_field(void *user, const char *name, const void *data, uint32_t count,
                     uint32_t width)
{
    struct visit_ctx *c = (struct visit_ctx *)user;
    if (c->calls < MAX_FIELDS) {
        c->names[c->calls] = name;
    }
    c->calls++;
    if (name == NULL || name[0] == '\0') {
        c->bad_name = 1;
        return;
    }
    c->saw_frame |= strcmp(name, "instance.frame") == 0;
    c->saw_vram |= strcmp(name, "gpu.vram") == 0;
    if (width != 1u && width != 2u && width != 4u && width != 8u) {
        c->bad_width = 1;
        return;
    }
    if ((size_t)count * width > STATE_BUF - c->n) {
        c->overflow = 1;
        return;
    }
    const uint8_t *p = (const uint8_t *)data;
    for (uint32_t i = 0; i < count; i++) {
        uint64_t v = 0;
        if (width == 1u) {
            v = p[i];
        } else if (width == 2u) {
            uint16_t x;
            memcpy(&x, p + 2u * (size_t)i, 2u);
            v = x;
        } else if (width == 4u) {
            uint32_t x;
            memcpy(&x, p + 4u * (size_t)i, 4u);
            v = x;
        } else {
            memcpy(&v, p + 8u * (size_t)i, 8u);
        }
        put_le(stream + c->n, v, width);
        c->n += width;
    }
}

/* The visitor hands over every field of the body: written little-endian in
   visit order, its fields are exactly the bytes a save writes after the
   header, with the card drawn at frame 60. */
static void test_visit(void)
{
    static const uint32_t regions[] = {SCHWAY_REGION_NTSC_U, SCHWAY_REGION_PAL};
    for (size_t k = 0; k < 2u; k++) {
        schway *inst = make(regions[k]);
        run_to(inst, 60);
        size_t n = save(inst, good);
        static struct visit_ctx c, again;
        memset(&c, 0, sizeof c);
        CHECK_EQ_U64(schway_state_visit(inst, on_field, &c), SCHWAY_STATUS_OK);
        CHECK(!c.bad_name && !c.bad_width && !c.overflow);
        CHECK(c.saw_frame && c.saw_vram);
        CHECK(c.calls >= 2u && c.calls <= MAX_FIELDS);
        CHECK_EQ_U64(c.n, n - SCHWAY__STATE_HEADER_SIZE);
        CHECK(c.n == n - SCHWAY__STATE_HEADER_SIZE &&
              memcmp(stream, good + SCHWAY__STATE_HEADER_SIZE, c.n) == 0);
        /* The names are the same strings on every call. */
        memset(&again, 0, sizeof again);
        CHECK_EQ_U64(schway_state_visit(inst, on_field, &again), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(again.calls, c.calls);
        for (size_t i = 0; i < c.calls && i < MAX_FIELDS; i++) {
            CHECK(again.names[i] == c.names[i]);
        }
        schway_destroy(inst);
    }
    CHECK_EQ_U64(schway_state_visit(NULL, on_field, NULL), SCHWAY_STATUS_ERR_ARGUMENT);
}

/* Every bounded body field written just outside its range, resealed, is
   refused by the field check, not the CRC, and changes nothing; a header
   field outside its value is refused before the body is read. */
static void test_record_sweep(void)
{
    schway *src = make(SCHWAY_REGION_NTSC_U);
    run_to(src, 60);
    size_t n = save(src, good);
    schway *target = make(SCHWAY_REGION_NTSC_U);
    run_to(target, 5);
    size_t nf = schway__state_record(src, fields, MAX_FIELDS);
    CHECK(nf >= 5u && nf <= MAX_FIELDS);
    size_t swept_body = 0, swept_header = 0;
    for (size_t i = 0; i < nf && i < MAX_FIELDS; i++) {
        const struct schway__state_field *fd = &fields[i];
        CHECK(fd->name != NULL);
        CHECK((size_t)fd->offset + (size_t)fd->width * fd->count <= n);
        uint64_t type_max =
            fd->width == 8u ? UINT64_MAX : (UINT64_C(1) << (8u * fd->width)) - 1u;
        const uint64_t outside[2] = {fd->max + 1u, fd->min - 1u};
        const int sweep[2] = {fd->max < type_max, fd->min > 0u};
        int header = fd->offset < SCHWAY__STATE_HEADER_SIZE;
        for (size_t j = 0; j < 2u; j++) {
            if (!sweep[j]) {
                continue;
            }
            memcpy(bad, good, n);
            put_le(bad + fd->offset, outside[j], fd->width);
            reseal(bad, n);
            if (header) {
                /* Magic, format and size each have their own status; the
                   CRC word itself takes any value. */
                size_t n0 = save(target, before);
                schway_status st = schway_load_state(target, bad, n);
                CHECK(st != SCHWAY_STATUS_OK && st != SCHWAY_STATUS_ERR_STATE_CRC);
                CHECK(save(target, after) == n0 && memcmp(before, after, n0) == 0);
                swept_header++;
            } else {
                expect_refused(fd->name, target, bad, n, SCHWAY_STATUS_ERR_STATE_FIELD);
                swept_body++;
            }
        }
    }
    printf("state fields: %lu recorded, %lu body and %lu header values refused\n",
           (unsigned long)nf, (unsigned long)swept_body, (unsigned long)swept_header);
    /* instance.frame, instance.region (both sides), instance.ticks and
       instance.tick_acc; gpu.vram takes every halfword. */
    CHECK(swept_body >= 5u);
    CHECK(swept_header >= 8u);
    /* The accumulator's remainder is below the frame denominator, 715,909
       cycles for NTSC (ARCHITECTURE section 1), so a remainder equal to it
       is refused whatever bound the record reports. */
    for (size_t i = 0; i < nf && i < MAX_FIELDS; i++) {
        if (strcmp(fields[i].name, "instance.tick_acc") == 0) {
            memcpy(bad, good, n);
            put_le(bad + fields[i].offset, 715909u, 8u);
            reseal(bad, n);
            expect_refused("tick_acc at the denominator", target, bad, n,
                           SCHWAY_STATUS_ERR_STATE_FIELD);
            memcpy(bad, good, n);
            put_le(bad + fields[i].offset, 715908u, 8u);
            reseal(bad, n);
            CHECK_EQ_U64(schway_load_state(target, bad, n), SCHWAY_STATUS_OK);
            CHECK_EQ_U64(target->tick_acc, 715908u);
        }
    }
    memcpy(bad, good, n);
    reseal(bad, n);
    CHECK_EQ_U64(schway_load_state(target, bad, n), SCHWAY_STATUS_OK);
    schway_destroy(target);
    schway_destroy(src);
}

/* Accepted counter boundaries must either advance to another loadable state
   or refuse the frame without changing even the unsaved audio and output. */
static void test_run_bounds(void)
{
    static const uint32_t regions[] = {
        SCHWAY_REGION_NTSC_J, SCHWAY_REGION_NTSC_U, SCHWAY_REGION_PAL
    };
    for (size_t k = 0; k < sizeof regions / sizeof regions[0]; k++) {
        schway *inst = make(regions[k]);
        schway *snapshot = malloc(sizeof *snapshot);
        CHECK(snapshot != NULL);
        if (snapshot == NULL) {
            exit(1);
        }
        size_t n = save(inst, good);
        size_t nf = schway__state_record(inst, fields, MAX_FIELDS);
        CHECK(nf <= MAX_FIELDS);
        uint32_t frame_at = 0u, ticks_at = 0u, acc_at = 0u;
        uint64_t frame_max = 0u, ticks_max = 0u;
        for (size_t i = 0; i < nf && i < MAX_FIELDS; i++) {
            if (strcmp(fields[i].name, "instance.frame") == 0) {
                frame_at = fields[i].offset;
                frame_max = fields[i].max;
            } else if (strcmp(fields[i].name, "instance.ticks") == 0) {
                ticks_at = fields[i].offset;
                ticks_max = fields[i].max;
            } else if (strcmp(fields[i].name, "instance.tick_acc") == 0) {
                acc_at = fields[i].offset;
            }
        }
        CHECK(frame_at != 0u && ticks_at != 0u && acc_at != 0u);
        if (frame_at == 0u || ticks_at == 0u || acc_at == 0u) {
            exit(1);
        }
        uint64_t den = schway__instance_frame_den(regions[k]);
        for (uint32_t carry = 0; carry < 2u; carry++) {
            uint64_t acc = carry ? den - 1u : 0u;
            uint64_t step = (acc + schway__instance_frame_num(regions[k])) / den;
            const uint64_t frames[] = {frame_max, frame_max - 1u, 0u, 0u, 0u, 0u, 0u};
            const uint64_t ticks[] = {
                0u, 0u, ticks_max, ticks_max - 1u,
                ticks_max - step, ticks_max - step + 1u, ticks_max - step - 1u
            };
            const int advances[] = {0, 1, 0, 0, 1, 0, 1};
            for (size_t j = 0; j < sizeof frames / sizeof frames[0]; j++) {
                memcpy(bad, good, n);
                put_le(bad + frame_at, frames[j], 8u);
                put_le(bad + ticks_at, ticks[j], 8u);
                put_le(bad + acc_at, acc, 8u);
                reseal(bad, n);
                CHECK_EQ_U64(schway_load_state(inst, bad, n), SCHWAY_STATUS_OK);
                memcpy(snapshot, inst, sizeof *snapshot);
                schway_run_result out, untouched;
                memset(&out, 0xA5, sizeof out);
                out.size = (uint32_t)sizeof out;
                memcpy(&untouched, &out, sizeof out);
                schway_status st = schway_run_frame(inst, &out);
                if (advances[j]) {
                    CHECK_EQ_U64(st, SCHWAY_STATUS_OK);
                    CHECK_EQ_U64(out.stop, SCHWAY_STOP_FRAME);
                    CHECK_EQ_U64(out.frame, frames[j] + 1u);
                    CHECK_EQ_U64(out.ticks, ticks[j] + step);
                    CHECK_EQ_U64(save(inst, before), n);
                    CHECK_EQ_U64(schway_load_state(inst, before, n), SCHWAY_STATUS_OK);
                    CHECK_EQ_U64(save(inst, after), n);
                    CHECK(memcmp(before, after, n) == 0);
                    /* Each successful boundary advance exhausts one counter. */
                    memcpy(snapshot, inst, sizeof *snapshot);
                    memcpy(&out, &untouched, sizeof out);
                    st = schway_run_frame(inst, &out);
                }
                CHECK_EQ_U64(st, SCHWAY_STATUS_ERR_STATE_FIELD);
                CHECK(memcmp(snapshot, inst, sizeof *snapshot) == 0);
                CHECK(memcmp(&untouched, &out, sizeof out) == 0);
                CHECK_EQ_U64(schway_run_frame(inst, NULL), SCHWAY_STATUS_ERR_STATE_FIELD);
                CHECK(memcmp(snapshot, inst, sizeof *snapshot) == 0);
            }
        }
        free(snapshot);
        schway_destroy(inst);
    }
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"state.record_sweep", test_record_sweep},
    {"state.rejects", test_rejects},
    {"state.round_trip", test_round_trip},
    {"state.run_bounds", test_run_bounds},
    {"state.stable_bytes", test_stable_bytes},
    {"state.visit", test_visit},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-state <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "schwaystation-test-state: unknown case %s\n", argv[1]);
    return 2;
}
