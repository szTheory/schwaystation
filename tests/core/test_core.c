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

static uint8_t state_buf[STATE_BUF];
static int16_t audio_buf[2u * SCHWAY__AUDIO_RING_PAIRS];

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
    {"no_alloc_after_load", test_no_alloc_after_load},
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
