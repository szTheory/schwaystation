/* The core through its public header: schwaystation-test-core <case>.
   Expected values are written from the documents the comments cite, never
   read back from the code under test; the test card's pixel function is the
   oracle only for whole-frame comparisons. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../alloc.h"
#include "../check.h"

/* A state buffer: the 1024 x 512 VRAM and a few words, rounded up. */
#define STATE_BUF (SCHWAY__VRAM_W * SCHWAY__VRAM_H * 2u + 4096u)

/* The display, the card and the frame cycles as the documents give them:
   docs/embedding.md "The test card", ARCHITECTURE sections 1 and 4, and
   psx-spx GPU Display Control Commands for the power-on display range. */
#define CARD_W 320u
#define CARD_H 240u
#define WHITE 0x7FFFu
#define RED 0x001Fu
#define GREEN 0x03E0u
#define MID_GREY 0x4210u
#define MASK_BIT 0x8000u
#define NTSC_NUM UINT64_C(405350378496)
#define NTSC_DEN UINT64_C(715909)
#define PAL_NUM UINT64_C(482961862656)
#define PAL_DEN UINT64_C(709379)
#define RING_PAIRS (4u * 887u) /* four of the longest (PAL) frames */

#define POISON 0xA5u   /* fill the core must leave alone */
#define TAG_BUF 8192u  /* bytes in the size-tag scratch buffer */
#define TAG_MAX 4096u  /* the largest input size tag the core accepts */

static uint8_t state_buf[STATE_BUF];
static uint8_t state_buf2[STATE_BUF];
static int16_t audio_buf[2u * RING_PAIRS + 64u];
static uint16_t tight[CARD_W * CARD_H + 64u];     /* a 640-byte pitch, then a guard */
static uint16_t padded[1024u * CARD_H];           /* a 2048-byte pitch */
static uint64_t tag_buf[TAG_BUF / 8u];            /* aligned for every public struct */
static uint8_t tag_before[TAG_BUF];
static schway *shared; /* the instance the size-tag calls use */

static void ignore_field(void *user, const char *name, const void *data, uint32_t count,
                         uint32_t width)
{
    (void)user;
    (void)name;
    (void)data;
    (void)count;
    (void)width;
}

static schway_profile make_profile(uint32_t region)
{
    schway_profile p;
    memset(&p, 0, sizeof p);
    p.size = (uint32_t)sizeof p;
    p.region = region;
    return p;
}

/* The plain malloc allocator, as a host defines it. */
static void *plain_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void plain_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static schway_config plain_config(void)
{
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = plain_alloc;
    cfg.allocator.free = plain_free;
    return cfg;
}

/* A new instance with region set, before any frame. */
static schway *make(uint32_t region)
{
    schway_config cfg = plain_config();
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) {
        exit(1);
    }
    schway_profile p = make_profile(region);
    CHECK_EQ_U64(schway_set_profile(inst, &p), SCHWAY_STATUS_OK);
    return inst;
}

static schway_run_result run(schway *inst, uint32_t frames)
{
    schway_run_result r;
    memset(&r, 0, sizeof r);
    for (uint32_t i = 0; i < frames; i++) {
        memset(&r, 0, sizeof r);
        r.size = (uint32_t)sizeof r;
        CHECK_EQ_U64(schway_run_frame(inst, &r), SCHWAY_STATUS_OK);
    }
    return r;
}

static schway_video video_of(const schway *inst)
{
    schway_video v;
    memset(&v, 0, sizeof v);
    v.size = (uint32_t)sizeof v;
    CHECK_EQ_U64(schway_get_video(inst, &v), SCHWAY_STATUS_OK);
    return v;
}

static schway_info info_of(const schway *inst)
{
    schway_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    CHECK_EQ_U64(schway_get_info(inst, &info), SCHWAY_STATUS_OK);
    return info;
}

/* Drains up to cap pairs into audio_buf, filled first with a non-zero
   pattern so a pair the core did not write shows. */
static schway_audio drain(schway *inst, uint32_t cap)
{
    schway_audio a;
    for (size_t i = 0; i < sizeof audio_buf / sizeof audio_buf[0]; i++) {
        audio_buf[i] = 0x5555;
    }
    memset(&a, 0, sizeof a);
    a.size = (uint32_t)sizeof a;
    a.samples = audio_buf;
    a.capacity = cap;
    CHECK_EQ_U64(schway_get_audio(inst, &a), SCHWAY_STATUS_OK);
    return a;
}

/* One display word, read at the frame's pitch. */
static uint16_t px(const schway_video *v, uint32_t x, uint32_t y)
{
    uint16_t w;
    memcpy(&w, (const uint8_t *)v->pixels + (size_t)y * v->pitch_bytes + 2u * (size_t)x, 2u);
    return w;
}

/* FNV-1a 64 over rows of w words at pitch bytes, each word low byte first. */
static uint64_t hash_rows(const void *base, uint32_t w, uint32_t h, uint32_t pitch)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    for (uint32_t y = 0; y < h; y++) {
        const uint8_t *row = (const uint8_t *)base + (size_t)y * pitch;
        for (uint32_t x = 0; x < w; x++) {
            uint16_t word;
            memcpy(&word, row + 2u * (size_t)x, 2u);
            hash = (hash ^ (uint8_t)(word & 0xFFu)) * UINT64_C(1099511628211);
            hash = (hash ^ (uint8_t)(word >> 8)) * UINT64_C(1099511628211);
        }
    }
    return hash;
}

static uint64_t frame_hash(const schway *inst)
{
    schway_video v = video_of(inst);
    return hash_rows(v.pixels, v.width, v.height, v.pitch_bytes);
}

/* Display words that differ from the card's pixel function for frame. */
static uint32_t card_mismatches(const schway *inst, uint32_t frame)
{
    schway_video v = video_of(inst);
    uint32_t bad = 0;
    for (uint32_t y = 0; y < CARD_H; y++) {
        for (uint32_t x = 0; x < CARD_W; x++) {
            if (px(&v, x, y) != schway__testcard_pixel(x, y, frame)) {
                bad++;
            }
        }
    }
    return bad;
}

static size_t save(const schway *inst, uint8_t *buf)
{
    size_t n = schway_state_size(inst);
    CHECK(n > 0u && n <= STATE_BUF);
    if (n == 0u || n > STATE_BUF) {
        exit(1);
    }
    CHECK_EQ_U64(schway_save_state(inst, buf, n), SCHWAY_STATUS_OK);
    return n;
}

/* The card's level for column x: ten dots per level. */
static uint16_t grey(uint32_t level)
{
    return (uint16_t)(level | (level << 5) | (level << 10));
}

/* The card's rules in first-match order, spot by spot, for frames 1, 5 and
   60, and the whole display against the pixel function (docs/embedding.md,
   "The test card"). */
static void test_card_pixels(void)
{
    static const uint32_t frames[] = {1u, 5u, 60u};
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    uint32_t at = 0;
    for (size_t k = 0; k < sizeof frames / sizeof frames[0]; k++) {
        run(inst, frames[k] - at);
        at = frames[k];
        schway_video v = video_of(inst);
        CHECK_EQ_U64(v.frame, frames[k]);
        /* Corner block, ahead of the strip and the border. */
        CHECK_EQ_HEX(px(&v, 0, 0), WHITE);
        CHECK_EQ_HEX(px(&v, 7, 7), WHITE);
        CHECK_EQ_HEX(px(&v, 5, 5), WHITE);
        /* Border, ahead of the strip and the bands. */
        CHECK_EQ_HEX(px(&v, 319, 120), WHITE);
        CHECK_EQ_HEX(px(&v, 100, 0), WHITE);
        CHECK_EQ_HEX(px(&v, 100, 239), WHITE);
        CHECK_EQ_HEX(px(&v, 0, 100), WHITE);
        CHECK_EQ_HEX(px(&v, 319, 239), WHITE);
        /* Row-ID strip: x 1 to 8 carry the row number. */
        CHECK_EQ_HEX(px(&v, 4, 37), 37u);
        CHECK_EQ_HEX(px(&v, 1, 200), 200u);
        CHECK_EQ_HEX(px(&v, 8, 100), 100u);
        CHECK_EQ_HEX(px(&v, 9, 100), 0u);
        /* Bands, ten dots per level. */
        CHECK_EQ_HEX(px(&v, 100, 10), RED);
        CHECK_EQ_HEX(px(&v, 100, 16), RED);
        CHECK_EQ_HEX(px(&v, 100, 17), 10u);
        CHECK_EQ_HEX(px(&v, 100, 30), 10u);
        CHECK_EQ_HEX(px(&v, 310, 30), 31u);
        CHECK_EQ_HEX(px(&v, 100, 48), 10u);
        CHECK_EQ_HEX(px(&v, 100, 49), 10u << 5);
        CHECK_EQ_HEX(px(&v, 100, 60), 10u << 5);
        CHECK_EQ_HEX(px(&v, 100, 81), 10u << 10);
        CHECK_EQ_HEX(px(&v, 100, 90), 10u << 10);
        CHECK_EQ_HEX(px(&v, 100, 113), grey(10));
        CHECK_EQ_HEX(px(&v, 100, 120), grey(10));
        CHECK_EQ_HEX(px(&v, 100, 145), grey(10) | MASK_BIT);
        CHECK_EQ_HEX(px(&v, 100, 150), grey(10) | MASK_BIT);
        CHECK_EQ_HEX(px(&v, 315, 176), grey(31) | MASK_BIT);
        /* Checker: white where x ^ y is odd. */
        CHECK_EQ_HEX(px(&v, 101, 180), WHITE);
        CHECK_EQ_HEX(px(&v, 100, 180), 0u);
        CHECK_EQ_HEX(px(&v, 100, 177), WHITE);
        CHECK_EQ_HEX(px(&v, 100, 192), 0u);
        /* Green rows below the counter. */
        CHECK_EQ_HEX(px(&v, 100, 209), GREEN);
        CHECK_EQ_HEX(px(&v, 100, 220), GREEN);
        CHECK_EQ_HEX(px(&v, 100, 238), GREEN);
        CHECK_EQ_U64(card_mismatches(inst, frames[k]), 0);
    }
    schway_destroy(inst);
}

/* The counter strip in rows 193 to 208: 32 cells of 8 dots from x 32, the
   most significant bit on the left, white for 1; grey elsewhere in those
   rows (docs/embedding.md, "The test card"). */
static uint32_t cell_x(uint32_t bit)
{
    return 32u + 8u * (31u - bit);
}

static void test_card_counter(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    run(inst, 5);
    schway_video v = video_of(inst);
    for (uint32_t y = 193; y <= 208; y++) {
        CHECK_EQ_HEX(px(&v, cell_x(2), y), WHITE);
        CHECK_EQ_HEX(px(&v, cell_x(2) + 7u, y), WHITE);
        CHECK_EQ_HEX(px(&v, cell_x(1), y), 0u);
        CHECK_EQ_HEX(px(&v, cell_x(0), y), WHITE);
        CHECK_EQ_HEX(px(&v, cell_x(31), y), 0u);
        CHECK_EQ_HEX(px(&v, 20, y), MID_GREY);
        CHECK_EQ_HEX(px(&v, 31, y), MID_GREY);
        CHECK_EQ_HEX(px(&v, 288, y), MID_GREY);
        CHECK_EQ_HEX(px(&v, 318, y), MID_GREY);
    }
    CHECK_EQ_HEX(px(&v, 192, 200), 0u); /* bit 11 of 5 */
    /* 60 is 111100 in binary. */
    run(inst, 55);
    v = video_of(inst);
    CHECK_EQ_HEX(px(&v, cell_x(5), 200), WHITE);
    CHECK_EQ_HEX(px(&v, cell_x(4), 200), WHITE);
    CHECK_EQ_HEX(px(&v, cell_x(3), 200), WHITE);
    CHECK_EQ_HEX(px(&v, cell_x(2), 200), WHITE);
    CHECK_EQ_HEX(px(&v, cell_x(1), 200), 0u);
    CHECK_EQ_HEX(px(&v, cell_x(0), 200), 0u);
    CHECK_EQ_HEX(px(&v, cell_x(6), 200), 0u);
    /* Frame 0x80000000 lights only the leftmost cell, x 32 to 39. */
    inst->frame = UINT64_C(0x7FFFFFFF);
    run(inst, 1);
    v = video_of(inst);
    CHECK_EQ_U64(v.frame, UINT64_C(0x80000000));
    for (uint32_t x = 32; x < 288; x++) {
        CHECK_EQ_HEX(px(&v, x, 200), x < 40u ? WHITE : 0u);
    }
    CHECK_EQ_U64(card_mismatches(inst, 0x80000000u), 0);
    schway_destroy(inst);
}

/* The card moves: a stuck or stale frame shows. */
static void test_card_frames_differ(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    run(inst, 5);
    uint64_t h5 = frame_hash(inst);
    run(inst, 1);
    uint64_t h6 = frame_hash(inst);
    run(inst, 54);
    uint64_t h60 = frame_hash(inst);
    CHECK(h5 != h60);
    CHECK(h5 != h6);
    schway_destroy(inst);
}

/* Every frame redraws all 76,800 display words: none of a sentinel written
   into the display rectangle survives a frame. */
static void test_card_full_redraw(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    run(inst, 1);
    for (uint32_t y = 0; y < CARD_H; y++) {
        for (uint32_t x = 0; x < CARD_W; x++) {
            inst->vram[y][x] = (uint16_t)~(uint32_t)schway__testcard_pixel(x, y, 2u);
        }
    }
    run(inst, 1);
    schway_video v = video_of(inst);
    uint32_t survived = 0;
    uint32_t seen = 0;
    for (uint32_t y = 0; y < v.height; y++) {
        for (uint32_t x = 0; x < v.width; x++) {
            seen++;
            if (px(&v, x, y) == (uint16_t)~(uint32_t)schway__testcard_pixel(x, y, 2u)) {
                survived++;
            }
        }
    }
    CHECK_EQ_U64(seen, 76800);
    CHECK_EQ_U64(survived, 0);
    CHECK_EQ_U64(card_mismatches(inst, 2u), 0);
    schway_destroy(inst);
}

/* The display tuple after power-on: GP1(06h) X1 260h, X2 C60h, GP1(07h)
   Y1 10h, Y2 100h, divider 8 (psx-spx, GPU Display Control Commands), so
   width (((X2 - X1) / 8) + 2) & ~3 is 320 and height Y2 - Y1 is 240
   (ARCHITECTURE section 4). */
static void check_tuple(const schway *inst, uint32_t region, uint64_t frame)
{
    schway_video v = video_of(inst);
    CHECK_EQ_U64(v.size, sizeof v);
    CHECK_EQ_U64(v.frame, frame);
    CHECK(v.pixels == (const void *)&inst->vram[0][0]);
    CHECK_EQ_U64(v.width, 320);
    CHECK_EQ_U64(v.height, 240);
    CHECK_EQ_U64(v.pitch_bytes, 2048);
    CHECK_EQ_U64(v.bpp, 16);
    CHECK_EQ_U64(v.field, 0);
    CHECK_EQ_U64(v.interlaced, 0);
    CHECK_EQ_U64(v.vres, 240);
    CHECK_EQ_U64(v.hres, 320);
    CHECK_EQ_U64(v.dot_div, 8);
    CHECK_EQ_U64(v.region, region);
    CHECK_EQ_HEX(v.x1, 0x260);
    CHECK_EQ_HEX(v.y1, 0x10);
    CHECK_EQ_U64(v.start_x, 0);
    CHECK_EQ_U64(v.start_y, 0);
    CHECK_EQ_U64(v.enabled, 1);
    CHECK_EQ_U64(v.rgb24, 0);
    CHECK_EQ_U64(v.reserved0, 0);
    CHECK_EQ_U64(v.reserved1, 0);
    CHECK_EQ_HEX(inst->display.x2, 0xC60);
    CHECK_EQ_HEX(inst->display.y2, 0x100);
}

static void test_video_tuple(void)
{
    schway_config cfg = plain_config();
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) {
        exit(1);
    }
    check_tuple(inst, SCHWAY_REGION_NTSC_U, 0);
    run(inst, 3);
    check_tuple(inst, SCHWAY_REGION_NTSC_U, 3);
    schway_destroy(inst);
    inst = make(SCHWAY_REGION_PAL);
    run(inst, 1);
    check_tuple(inst, SCHWAY_REGION_PAL, 1);
    /* The size follows the range words: 256 dots and 224 lines. */
    inst->display.x2 = 0x260u + 256u * 8u;
    inst->display.y2 = 0x10u + 224u;
    schway_video v = video_of(inst);
    CHECK_EQ_U64(v.width, 256);
    CHECK_EQ_U64(v.height, 224);
    schway_destroy(inst);
}

/* A host copying the display at a tight 640-byte pitch and a padded
   2048-byte pitch gets the same words, and the padding keeps its bytes. */
static void test_video_pitch(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    run(inst, 9);
    schway_video v = video_of(inst);
    memset(tight, POISON, sizeof tight);
    memset(padded, POISON, sizeof padded);
    for (uint32_t y = 0; y < v.height; y++) {
        const uint8_t *src = (const uint8_t *)v.pixels + (size_t)y * v.pitch_bytes;
        memcpy((uint8_t *)tight + (size_t)y * 640u, src, (size_t)v.width * 2u);
        memcpy((uint8_t *)padded + (size_t)y * 2048u, src, (size_t)v.width * 2u);
    }
    uint64_t ht = hash_rows(tight, v.width, v.height, 640u);
    uint64_t hp = hash_rows(padded, v.width, v.height, 2048u);
    CHECK_EQ_HEX(ht, hp);
    CHECK_EQ_HEX(ht, frame_hash(inst));
    uint32_t pad_written = 0;
    for (uint32_t y = 0; y < v.height; y++) {
        const uint8_t *row = (const uint8_t *)padded + (size_t)y * 2048u;
        for (uint32_t i = 640u; i < 2048u; i++) {
            pad_written += row[i] != POISON ? 1u : 0u;
        }
    }
    const uint8_t *guard = (const uint8_t *)tight + (size_t)CARD_H * 640u;
    for (uint32_t i = 0; i < 128u; i++) {
        pad_written += guard[i] != POISON ? 1u : 0u;
    }
    CHECK_EQ_U64(pad_written, 0);
    CHECK_EQ_U64(card_mismatches(inst, 9u), 0);
    schway_destroy(inst);
}

/* 33,868,800 cycles per second over the frame's cycles, 44,100 pairs per
   second (ARCHITECTURE section 1). */
static void test_info(void)
{
    static const struct {
        uint32_t region, fps_num, fps_den, max_pairs;
    } want[] = {
        {SCHWAY_REGION_NTSC_U, 53693175u, 897619u, 738u},
        {SCHWAY_REGION_NTSC_J, 53693175u, 897619u, 738u},
        {SCHWAY_REGION_PAL, 53203425u, 1069484u, 887u},
    };
    for (size_t i = 0; i < sizeof want / sizeof want[0]; i++) {
        schway *inst = make(want[i].region);
        schway_info info = info_of(inst);
        CHECK_EQ_U64(info.size, sizeof info);
        CHECK_EQ_U64(info.width, 320);
        CHECK_EQ_U64(info.height, 240);
        CHECK_EQ_U64(info.fps_num, want[i].fps_num);
        CHECK_EQ_U64(info.fps_den, want[i].fps_den);
        CHECK_EQ_U64(info.sample_rate_num, 44100);
        CHECK_EQ_U64(info.sample_rate_den, 1);
        CHECK_EQ_U64(info.max_audio_frames, want[i].max_pairs);
        CHECK_EQ_U64(info.region, want[i].region);
        schway_destroy(inst);
    }
}

/* Each frame is num / den cycles with the remainder carried, so frame k ends
   at floor(k * num / den) cycles, and one stereo pair is due per 768-cycle
   boundary crossed (ARCHITECTURE section 1). Every pair is silent. */
static void audio_pairs_for(uint32_t region, uint64_t num, uint64_t den, uint32_t short_pairs,
                            uint64_t short_ticks)
{
    schway *inst = make(region);
    uint64_t prev = 0;
    uint64_t total = 0;
    uint32_t bad_ticks = 0, bad_pairs = 0, bad_samples = 0, bad_acc = 0, bad_end = 0;
    uint32_t dropped = 0;
    uint32_t seen_short = 0, seen_long = 0;
    for (uint64_t k = 1; k <= 4000u; k++) {
        schway_run_result r = run(inst, 1);
        uint64_t dt = r.ticks - prev;
        bad_ticks += (dt != short_ticks && dt != short_ticks + 1u) ? 1u : 0u;
        bad_end += r.ticks != (k * num) / den ? 1u : 0u;
        bad_acc += inst->tick_acc >= den ? 1u : 0u;
        schway_audio a = drain(inst, RING_PAIRS);
        bad_pairs += (a.written != short_pairs && a.written != short_pairs + 1u) ? 1u : 0u;
        bad_pairs += a.written != r.ticks / 768u - prev / 768u ? 1u : 0u;
        seen_short += a.written == short_pairs ? 1u : 0u;
        seen_long += a.written == short_pairs + 1u ? 1u : 0u;
        for (uint32_t i = 0; i < 2u * a.written; i++) {
            bad_samples += audio_buf[i] != 0 ? 1u : 0u;
        }
        dropped += a.dropped;
        total += a.written;
        prev = r.ticks;
    }
    CHECK_EQ_U64(bad_ticks, 0);
    CHECK_EQ_U64(bad_end, 0);
    CHECK_EQ_U64(bad_acc, 0);
    CHECK_EQ_U64(bad_pairs, 0);
    CHECK_EQ_U64(bad_samples, 0);
    CHECK_EQ_U64(dropped, 0);
    CHECK(seen_short > 0u && seen_long > 0u);
    CHECK_EQ_U64(total, prev / 768u);
    schway_destroy(inst);
}

static void test_audio_pairs(void)
{
    audio_pairs_for(SCHWAY_REGION_NTSC_U, NTSC_NUM, NTSC_DEN, 737u, 566203u);
    audio_pairs_for(SCHWAY_REGION_PAL, PAL_NUM, PAL_DEN, 886u, 680823u);
}

/* The ring holds four of the longest frames; pairs past it are dropped and
   counted, the count is reported once and cleared by the drain. A drain into
   no room writes nothing and keeps the pairs queued. */
static void test_audio_ring(void)
{
    static const uint32_t regions[] = {SCHWAY_REGION_NTSC_U, SCHWAY_REGION_PAL};
    for (size_t k = 0; k < 2u; k++) {
        schway *inst = make(regions[k]);
        schway_run_result r = run(inst, 5);
        uint64_t due = r.ticks / 768u;
        CHECK(due > RING_PAIRS);
        schway_audio a = drain(inst, 2u * RING_PAIRS);
        CHECK_EQ_U64(a.written, RING_PAIRS);
        CHECK_EQ_U64(a.dropped, due - RING_PAIRS);
        a = drain(inst, 2u * RING_PAIRS);
        CHECK_EQ_U64(a.written, 0);
        CHECK_EQ_U64(a.dropped, 0);

        uint64_t before = r.ticks;
        r = run(inst, 1);
        schway_audio none;
        memset(&none, 0, sizeof none);
        none.size = (uint32_t)sizeof none;
        none.samples = NULL;
        none.capacity = 0;
        CHECK_EQ_U64(schway_get_audio(inst, &none), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(none.written, 0);
        CHECK_EQ_U64(none.dropped, 0);
        a = drain(inst, 2u * RING_PAIRS);
        CHECK_EQ_U64(a.written, r.ticks / 768u - before / 768u);
        schway_destroy(inst);
    }
}

/* A region outside 1 to 3 is refused and changes nothing. */
static void test_profile_rejected(void)
{
    schway_config cfg = plain_config();
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(info_of(inst).region, SCHWAY_REGION_NTSC_U);
    static const uint32_t bad[] = {0u, 4u, 0xFFFFFFFFu};
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        schway_profile p = make_profile(bad[i]);
        CHECK_EQ_U64(schway_set_profile(inst, &p), SCHWAY_STATUS_ERR_PROFILE_REJECTED);
        CHECK_EQ_U64(info_of(inst).region, SCHWAY_REGION_NTSC_U);
    }
    CHECK_EQ_U64(schway_set_profile(inst, NULL), SCHWAY_STATUS_ERR_ARGUMENT);
    schway_profile j = make_profile(SCHWAY_REGION_NTSC_J);
    CHECK_EQ_U64(schway_set_profile(NULL, &j), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_set_profile(inst, &j), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(info_of(inst).region, SCHWAY_REGION_NTSC_J);
    schway_destroy(inst);
}

/* The profile may change until the first frame, and not after it, not even
   after a power reset. */
static void test_profile_locked(void)
{
    schway *inst = make(SCHWAY_REGION_PAL);
    schway_profile u = make_profile(SCHWAY_REGION_NTSC_U);
    CHECK_EQ_U64(schway_set_profile(inst, &u), SCHWAY_STATUS_OK);
    schway_profile pal = make_profile(SCHWAY_REGION_PAL);
    CHECK_EQ_U64(schway_set_profile(inst, &pal), SCHWAY_STATUS_OK);
    run(inst, 1);
    schway_profile j = make_profile(SCHWAY_REGION_NTSC_J);
    CHECK_EQ_U64(schway_set_profile(inst, &j), SCHWAY_STATUS_ERR_PROFILE_LOCKED);
    CHECK_EQ_U64(info_of(inst).region, SCHWAY_REGION_PAL);
    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_set_profile(inst, &j), SCHWAY_STATUS_ERR_PROFILE_LOCKED);
    CHECK_EQ_U64(info_of(inst).region, SCHWAY_REGION_PAL);
    schway_destroy(inst);
}

/* A power reset returns to the state after create, keeping the region; any
   other kind is refused and changes nothing. */
static void test_reset_power(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    run(inst, 10);
    size_t n = save(inst, state_buf);
    CHECK_EQ_U64(schway_reset(inst, 2u), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_reset(inst, 0u), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_reset(NULL, SCHWAY_RESET_POWER), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(video_of(inst).frame, 10);
    CHECK_EQ_U64(save(inst, state_buf2), n);
    CHECK(memcmp(state_buf, state_buf2, n) == 0);

    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(video_of(inst).frame, 0);
    schway *fresh = make(SCHWAY_REGION_NTSC_U);
    CHECK_EQ_U64(save(inst, state_buf), n);
    CHECK_EQ_U64(save(fresh, state_buf2), n);
    CHECK(memcmp(state_buf, state_buf2, n) == 0);
    schway_audio a = drain(inst, RING_PAIRS);
    CHECK_EQ_U64(a.written, 0);
    schway_run_result r = run(inst, 1);
    CHECK_EQ_U64(r.frame, 1);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    CHECK_EQ_U64(r.ticks, NTSC_NUM / NTSC_DEN);
    CHECK_EQ_U64(info_of(inst).region, SCHWAY_REGION_NTSC_U);
    run(fresh, 1);
    CHECK_EQ_HEX(frame_hash(inst), frame_hash(fresh));
    schway_destroy(fresh);
    schway_destroy(inst);
}

/* The size-tag rules (docs/embedding.md): an input struct below its first
   released size is refused, a larger one is accepted only when the bytes
   past ours are zero and its tag is at most 4096; an output struct below its
   first size is refused and otherwise written up to min(size, ours), the
   rest left alone. A refused call changes nothing. */
static schway_status call_config(void *b)
{
    schway *inst = NULL;
    schway_status st = schway_create((const schway_config *)b, &inst);
    schway_destroy(inst);
    return st;
}

static schway_status call_profile(void *b)
{
    return schway_set_profile(shared, (const schway_profile *)b);
}

static schway_status call_audio(void *b)
{
    return schway_get_audio(shared, (schway_audio *)b);
}

static schway_status call_versions(void *b)
{
    return schway_get_versions((schway_versions *)b);
}

static schway_status call_info(void *b)
{
    return schway_get_info(shared, (schway_info *)b);
}

static schway_status call_video(void *b)
{
    return schway_get_video(shared, (schway_video *)b);
}

static schway_status call_run(void *b)
{
    return schway_run_frame(shared, (schway_run_result *)b);
}

static void set_tag(void *b, uint32_t size)
{
    memcpy(b, &size, sizeof size);
}

static void expect_status(const char *what, uint32_t size, schway_status got,
                          schway_status want)
{
    if (got != want) {
        fprintf(stderr, "%s with size %u: status %u, want %u\n", what, (unsigned)size,
                (unsigned)got, (unsigned)want);
        check_failures++;
    }
}

/* The shared instance's state, saved before a refused call. */
static size_t snap(void)
{
    return save(shared, state_buf);
}

static void same_as_snap(const char *what, size_t n)
{
    if (save(shared, state_buf2) != n || memcmp(state_buf, state_buf2, n) != 0) {
        fprintf(stderr, "%s: a refused call changed the instance\n", what);
        check_failures++;
    }
}

static void test_size_tags(void)
{
    uint8_t *b = (uint8_t *)tag_buf;
    shared = make(SCHWAY_REGION_NTSC_U);

    /* Inputs: a valid struct of ours, then the tag and the bytes past it. */
    schway_config cfg = plain_config();
    schway_profile pal = make_profile(SCHWAY_REGION_PAL);
    schway_audio aud;
    memset(&aud, 0, sizeof aud);
    aud.samples = audio_buf;
    aud.capacity = RING_PAIRS;
    const struct {
        const char *what;
        const void *valid;
        uint32_t ours, first;
        schway_status (*call)(void *);
    } inputs[] = {
        {"config", &cfg, (uint32_t)sizeof cfg, SCHWAY__CONFIG_SIZE_FIRST, call_config},
        {"profile", &pal, (uint32_t)sizeof pal, SCHWAY__PROFILE_SIZE_FIRST, call_profile},
        {"audio", &aud, (uint32_t)sizeof aud, SCHWAY__AUDIO_SIZE_FIRST, call_audio},
    };
    for (size_t i = 0; i < sizeof inputs / sizeof inputs[0]; i++) {
        const uint32_t refused[] = {0u, inputs[i].first - 1u, TAG_MAX + 1u, 0xFFFFFFFFu};
        for (size_t k = 0; k < sizeof refused / sizeof refused[0]; k++) {
            size_t n = snap();
            memset(b, 0, TAG_BUF);
            memcpy(b, inputs[i].valid, inputs[i].ours);
            set_tag(b, refused[k]);
            expect_status(inputs[i].what, refused[k], inputs[i].call(b),
                          SCHWAY_STATUS_ERR_STRUCT_SIZE);
            same_as_snap(inputs[i].what, n);
        }
        /* A larger struct with a non-zero byte past ours. */
        size_t n = snap();
        memset(b, 0, TAG_BUF);
        memcpy(b, inputs[i].valid, inputs[i].ours);
        set_tag(b, inputs[i].ours + 8u);
        b[inputs[i].ours + 7u] = 1u;
        expect_status(inputs[i].what, inputs[i].ours + 8u, inputs[i].call(b),
                      SCHWAY_STATUS_ERR_STRUCT_SIZE);
        same_as_snap(inputs[i].what, n);
        /* The same with those bytes zero, and the largest tag. */
        const uint32_t accepted[] = {inputs[i].ours, inputs[i].ours + 8u, TAG_MAX};
        for (size_t k = 0; k < sizeof accepted / sizeof accepted[0]; k++) {
            memset(b, 0, TAG_BUF);
            memcpy(b, inputs[i].valid, inputs[i].ours);
            set_tag(b, accepted[k]);
            expect_status(inputs[i].what, accepted[k], inputs[i].call(b), SCHWAY_STATUS_OK);
        }
    }
    CHECK_EQ_U64(info_of(shared).region, SCHWAY_REGION_PAL);

    /* Outputs: refused below the first size with the buffer untouched;
       otherwise our bytes written and the rest kept. */
    const struct {
        const char *what;
        uint32_t ours, first;
        schway_status (*call)(void *);
    } outputs[] = {
        {"versions", (uint32_t)sizeof(schway_versions), SCHWAY__VERSIONS_SIZE_FIRST,
         call_versions},
        {"info", (uint32_t)sizeof(schway_info), SCHWAY__INFO_SIZE_FIRST, call_info},
        {"video", (uint32_t)sizeof(schway_video), SCHWAY__VIDEO_SIZE_FIRST, call_video},
        {"run_result", (uint32_t)sizeof(schway_run_result), SCHWAY__RUN_RESULT_SIZE_FIRST,
         call_run},
    };
    for (size_t i = 0; i < sizeof outputs / sizeof outputs[0]; i++) {
        const uint32_t refused[] = {0u, outputs[i].first - 1u};
        for (size_t k = 0; k < sizeof refused / sizeof refused[0]; k++) {
            size_t n = snap();
            memset(b, POISON, TAG_BUF);
            set_tag(b, refused[k]);
            memcpy(tag_before, b, TAG_BUF);
            expect_status(outputs[i].what, refused[k], outputs[i].call(b),
                          SCHWAY_STATUS_ERR_STRUCT_SIZE);
            CHECK(memcmp(tag_before, b, TAG_BUF) == 0);
            same_as_snap(outputs[i].what, n);
        }
        const uint32_t sizes[] = {outputs[i].ours, outputs[i].ours + 64u, TAG_BUF};
        for (size_t k = 0; k < sizeof sizes / sizeof sizes[0]; k++) {
            memset(b, POISON, TAG_BUF);
            set_tag(b, sizes[k]);
            expect_status(outputs[i].what, sizes[k], outputs[i].call(b), SCHWAY_STATUS_OK);
            uint32_t tag;
            memcpy(&tag, b, sizeof tag);
            CHECK_EQ_U64(tag, sizes[k]);
            uint32_t kept = 0;
            for (uint32_t j = outputs[i].ours; j < TAG_BUF; j++) {
                kept += b[j] == POISON ? 1u : 0u;
            }
            CHECK_EQ_U64(kept, TAG_BUF - outputs[i].ours);
        }
    }
    schway_destroy(shared);
    shared = NULL;
}

/* Every status has text, an unknown one included. */
static void test_statuses(void)
{
    const char *seen[11];
    for (uint32_t s = 0; s <= 10u; s++) {
        seen[s] = schway_status_string(s);
        CHECK(seen[s] != NULL && seen[s][0] != '\0');
        for (uint32_t t = 0; t < s; t++) {
            CHECK(seen[s] != NULL && seen[t] != NULL && strcmp(seen[s], seen[t]) != 0);
        }
    }
    const char *unknown = schway_status_string(0xFFFFFFFFu);
    CHECK(unknown != NULL && unknown[0] != '\0');
    unknown = schway_status_string(11u);
    CHECK(unknown != NULL && unknown[0] != '\0');
}

/* A refused call changes nothing: not the state bytes, not the frame, not
   the queued audio. */
static void test_error_changes_nothing(void)
{
    shared = make(SCHWAY_REGION_NTSC_U);
    schway_run_result r = run(shared, 3); /* fewer pairs than the ring holds */
    size_t n = snap();

    schway_profile pal = make_profile(SCHWAY_REGION_PAL);
    CHECK_EQ_U64(schway_set_profile(shared, &pal), SCHWAY_STATUS_ERR_PROFILE_LOCKED);
    same_as_snap("set_profile locked", n);
    schway_profile bad = make_profile(9u);
    CHECK_EQ_U64(schway_set_profile(shared, &bad), SCHWAY_STATUS_ERR_PROFILE_REJECTED);
    same_as_snap("set_profile rejected", n);
    CHECK_EQ_U64(schway_reset(shared, 5u), SCHWAY_STATUS_ERR_ARGUMENT);
    same_as_snap("reset", n);

    schway_run_result small;
    memset(&small, 0, sizeof small);
    small.size = 4u;
    CHECK_EQ_U64(schway_run_frame(shared, &small), SCHWAY_STATUS_ERR_STRUCT_SIZE);
    CHECK_EQ_U64(schway_run_frame(NULL, NULL), SCHWAY_STATUS_ERR_ARGUMENT);
    same_as_snap("run_frame", n);

    schway_audio a;
    memset(&a, 0, sizeof a);
    a.size = 4u;
    a.samples = audio_buf;
    a.capacity = RING_PAIRS;
    CHECK_EQ_U64(schway_get_audio(shared, &a), SCHWAY_STATUS_ERR_STRUCT_SIZE);
    a.size = (uint32_t)sizeof a;
    a.samples = NULL;
    a.written = 77u;
    CHECK_EQ_U64(schway_get_audio(shared, &a), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(a.written, 77);
    CHECK_EQ_U64(schway_get_audio(NULL, &a), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_get_audio(shared, NULL), SCHWAY_STATUS_ERR_ARGUMENT);

    CHECK_EQ_U64(schway_get_video(NULL, NULL), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_get_info(NULL, NULL), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_get_versions(NULL), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_state_size(NULL), 0);
    CHECK_EQ_U64(schway_save_state(shared, state_buf2, n - 1u),
                 SCHWAY_STATUS_ERR_BUFFER_TOO_SMALL);
    CHECK_EQ_U64(schway_save_state(shared, NULL, n), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_load_state(shared, NULL, n), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_state_visit(shared, NULL, NULL), SCHWAY_STATUS_ERR_ARGUMENT);
    memcpy(state_buf2, state_buf, n);
    state_buf2[0] ^= 1u;
    CHECK_EQ_U64(schway_load_state(shared, state_buf2, n), SCHWAY_STATUS_ERR_STATE_MAGIC);
    same_as_snap("load_state", n);

    CHECK_EQ_U64(video_of(shared).frame, 3);
    schway_audio got = drain(shared, RING_PAIRS);
    CHECK_EQ_U64(got.written, r.ticks / 768u);
    CHECK_EQ_U64(got.dropped, 0);
    schway_destroy(shared);
    shared = NULL;
}

/* Both allocator hooks are required; a refused create allocates nothing and
   leaves *out alone. destroy(NULL) does nothing, and a frame may be run
   without a result struct. */
static void test_create_allocator(void)
{
    alloc_counts c;
    schway *sentinel = (schway *)(void *)&c;
    schway_config cfg = alloc_config(&c);
    cfg.allocator.alloc = NULL;
    schway *out = sentinel;
    CHECK_EQ_U64(schway_create(&cfg, &out), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK(out == sentinel);
    cfg = alloc_config(&c);
    cfg.allocator.free = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &out), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK(out == sentinel);
    cfg = alloc_config(&c);
    cfg.size = 4u;
    CHECK_EQ_U64(schway_create(&cfg, &out), SCHWAY_STATUS_ERR_STRUCT_SIZE);
    CHECK(out == sentinel);
    CHECK_EQ_U64(schway_create(NULL, &out), SCHWAY_STATUS_ERR_ARGUMENT);
    cfg = alloc_config(&c);
    CHECK_EQ_U64(schway_create(&cfg, NULL), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK(out == sentinel);
    CHECK_EQ_U64(c.allocs, 0);
    CHECK_EQ_U64(c.frees, 0);

    schway_destroy(NULL);

    CHECK_EQ_U64(schway_create(&cfg, &out), SCHWAY_STATUS_OK);
    CHECK(out != sentinel && out != NULL);
    CHECK_EQ_U64(c.allocs, 1);
    CHECK_EQ_U64(info_of(out).region, SCHWAY_REGION_NTSC_U);
    CHECK_EQ_U64(schway_run_frame(out, NULL), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(video_of(out).frame, 1);
    schway_destroy(out);
    CHECK_EQ_U64(c.frees, 1);
    CHECK_EQ_U64(c.free_bytes, c.alloc_bytes);
}

/* Two instances in one process share nothing: stepped interleaved they give
   the frames and the state bytes each gives alone. */
static void test_instances_independent(void)
{
    static uint64_t ha[60], hb[60];
    schway *a = make(SCHWAY_REGION_NTSC_U);
    schway *b = make(SCHWAY_REGION_PAL);
    for (uint32_t i = 0; i < 60u; i++) {
        run(a, 1);
        run(b, 1);
        ha[i] = frame_hash(a);
        hb[i] = frame_hash(b);
        (void)drain(i % 2u == 0u ? a : b, RING_PAIRS);
    }
    size_t na = save(a, state_buf);
    schway_destroy(a);
    schway *solo = make(SCHWAY_REGION_NTSC_U);
    uint32_t differ = 0;
    for (uint32_t i = 0; i < 60u; i++) {
        run(solo, 1);
        differ += frame_hash(solo) != ha[i] ? 1u : 0u;
    }
    CHECK_EQ_U64(save(solo, state_buf2), na);
    CHECK(memcmp(state_buf, state_buf2, na) == 0);
    schway_destroy(solo);

    size_t nb = save(b, state_buf);
    schway_destroy(b);
    solo = make(SCHWAY_REGION_PAL);
    for (uint32_t i = 0; i < 60u; i++) {
        run(solo, 1);
        differ += frame_hash(solo) != hb[i] ? 1u : 0u;
    }
    CHECK_EQ_U64(save(solo, state_buf2), nb);
    CHECK(memcmp(state_buf, state_buf2, nb) == 0);
    schway_destroy(solo);
    CHECK_EQ_U64(differ, 0);
}

/* The allocator contract (docs/embedding.md): create makes exactly one
   allocation, destroy frees it with the same size, and nothing between them
   allocates or frees. A failed allocation leaves *out alone and leaks
   nothing. */
static void test_no_alloc_after_load(void)
{
    alloc_counts c;
    schway_config cfg = alloc_config(&c);
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    CHECK(inst != NULL);
    if (inst == NULL) {
        return;
    }
    CHECK_EQ_U64(c.allocs, 1);
    CHECK_EQ_U64(c.frees, 0);
    size_t size = c.alloc_size;

    for (int i = 0; i < 120; i++) {
        schway_run_result r;
        memset(&r, 0, sizeof r);
        r.size = (uint32_t)sizeof r;
        CHECK_EQ_U64(schway_run_frame(inst, &r), SCHWAY_STATUS_OK);
    }
    schway_versions v;
    memset(&v, 0, sizeof v);
    v.size = (uint32_t)sizeof v;
    CHECK_EQ_U64(schway_get_versions(&v), SCHWAY_STATUS_OK);
    schway_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    CHECK_EQ_U64(schway_get_info(inst, &info), SCHWAY_STATUS_OK);
    schway_video video;
    memset(&video, 0, sizeof video);
    video.size = (uint32_t)sizeof video;
    CHECK_EQ_U64(schway_get_video(inst, &video), SCHWAY_STATUS_OK);
    schway_audio audio;
    memset(&audio, 0, sizeof audio);
    audio.size = (uint32_t)sizeof audio;
    audio.samples = audio_buf;
    audio.capacity = SCHWAY__AUDIO_RING_PAIRS;
    CHECK_EQ_U64(schway_get_audio(inst, &audio), SCHWAY_STATUS_OK);
    schway_profile pal = make_profile(SCHWAY_REGION_PAL);
    CHECK_EQ_U64(schway_set_profile(inst, &pal), SCHWAY_STATUS_ERR_PROFILE_LOCKED);
    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_set_profile(inst, &pal), SCHWAY_STATUS_ERR_PROFILE_LOCKED);
    size_t n = schway_state_size(inst);
    CHECK(n > 0u && n <= sizeof state_buf);
    if (n > 0u && n <= sizeof state_buf) {
        CHECK_EQ_U64(schway_save_state(inst, state_buf, n), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(schway_load_state(inst, state_buf, n), SCHWAY_STATUS_OK);
    }
    CHECK_EQ_U64(schway_state_visit(inst, ignore_field, NULL), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(c.allocs, 1);
    CHECK_EQ_U64(c.frees, 0);

    schway_destroy(inst);
    CHECK_EQ_U64(c.allocs, 1);
    CHECK_EQ_U64(c.frees, 1);
    CHECK_EQ_U64(c.free_size, size);
    CHECK_EQ_U64(c.free_bytes, c.alloc_bytes);

    cfg = alloc_config(&c);
    c.fail_from = 1u;
    schway *sentinel = (schway *)(void *)&c;
    schway *out = sentinel;
    CHECK_EQ_U64(schway_create(&cfg, &out), SCHWAY_STATUS_ERR_NO_MEMORY);
    CHECK(out == sentinel);
    CHECK_EQ_U64(c.allocs, 1);
    CHECK_EQ_U64(c.frees, 0);
    CHECK_EQ_U64(c.alloc_bytes, c.free_bytes);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"api.error_changes_nothing", test_error_changes_nothing},
    {"api.size_tags", test_size_tags},
    {"api.statuses", test_statuses},
    {"audio.pairs", test_audio_pairs},
    {"audio.ring", test_audio_ring},
    {"create.allocator", test_create_allocator},
    {"info", test_info},
    {"instances.independent", test_instances_independent},
    {"no_alloc_after_load", test_no_alloc_after_load},
    {"profile.locked", test_profile_locked},
    {"profile.rejected", test_profile_rejected},
    {"reset.power", test_reset_power},
    {"testcard.counter", test_card_counter},
    {"testcard.frames_differ", test_card_frames_differ},
    {"testcard.full_redraw", test_card_full_redraw},
    {"testcard.pixels", test_card_pixels},
    {"video.pitch", test_video_pitch},
    {"video.tuple", test_video_tuple},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-core <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].run();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "schwaystation-test-core: unknown case %s\n", argv[1]);
    return 2;
}
