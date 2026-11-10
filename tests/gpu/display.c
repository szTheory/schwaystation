/* GP1 display windows map the requested VRAM range without rounding the
   field height; interlace stays field-sequential (PSX-GPU-AND-GTE sections
   four point five and eight point one). */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bus.h"
#include "internal.h"
#include "../check.h"

#define GP1 UINT32_C(0x1F801814)

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

static schway *make_instance(void)
{
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = plain_alloc;
    cfg.allocator.free = plain_free;
    schway *inst = NULL;
    CHECK(schway_create(&cfg, &inst) == SCHWAY_STATUS_OK);
    if (inst != NULL) {
        inst->exe_loaded = 1u;
    }
    return inst;
}

static void gp1(schway *inst, uint32_t command, uint32_t value)
{
    uint32_t cycles = 0u;
    CHECK(schway__bus_write(inst, GP1, 4u, (command << 24) | value, &cycles) == 0);
}

static schway_video video_of(const schway *inst)
{
    schway_video video;
    memset(&video, 0, sizeof video);
    video.size = (uint32_t)sizeof video;
    CHECK(schway_get_video(inst, &video) == SCHWAY_STATUS_OK);
    return video;
}

static void window(schway *inst, uint32_t start_x, uint32_t start_y, uint32_t x1,
                   uint32_t x2, uint32_t y1, uint32_t y2, uint32_t mode)
{
    gp1(inst, 3u, 0u);
    gp1(inst, 5u, start_x | (start_y << 10));
    gp1(inst, 6u, x1 | (x2 << 12));
    gp1(inst, 7u, y1 | (y2 << 10));
    gp1(inst, 8u, mode);
}

static void set_vram_byte(schway *inst, uint32_t y, uint32_t x, uint8_t value)
{
    uint16_t *word = &inst->vram[y][x / 2u];
    uint32_t shift = (x & 1u) * 8u;
    *word = (uint16_t)((*word & ~(UINT16_C(0xFF) << shift)) | ((uint16_t)value << shift));
}

static void width_modes(void)
{
    static const uint32_t modes[5] = {0u, 1u, 0x40u, 2u, 3u};
    static const uint32_t dividers[5] = {10u, 8u, 7u, 5u, 4u};
    static const uint32_t widths[5] = {4u, 8u, 8u, 12u, 12u};
    schway *inst = make_instance();
    CHECK(inst != NULL);
    if (inst != NULL) {
        for (size_t i = 0; i < 5u; i++) {
            window(inst, 0u, 0u, 100u, 150u, 4u, 9u, modes[i]);
            schway_video v = video_of(inst);
            CHECK_EQ_U64(v.width, widths[i]);
            CHECK_EQ_U64(v.height, 5u);
            CHECK_EQ_U64(v.dot_div, dividers[i]);
        }
        schway_destroy(inst);
    }
}

static void native15_wrap(void)
{
    schway *inst = make_instance();
    CHECK(inst != NULL);
    if (inst == NULL) {
        return;
    }
    window(inst, 1022u, 511u, 100u, 148u, 0u, 2u, 1u);
    static const uint16_t row0[8] = {0x001Fu, 0x03E0u, 0x7C00u, 0x7FFFu,
                                     0x8000u, 0x1234u, 0x4567u, 0x7654u};
    for (uint32_t x = 0u; x < 8u; x++) {
        inst->vram[511u][(1022u + x) & 1023u] = row0[x];
        inst->vram[0u][(1022u + x) & 1023u] = (uint16_t)(row0[x] ^ 0x1111u);
    }
    schway_video v = video_of(inst);
    CHECK_EQ_U64(v.width, 8u);
    CHECK_EQ_U64(v.height, 2u);
    CHECK_EQ_U64(v.pitch_bytes, 16u);
    CHECK_EQ_U64(v.bpp, 16u);
    CHECK_EQ_U64(v.start_x, 1022u);
    CHECK_EQ_U64(v.start_y, 511u);
    const uint16_t *p = (const uint16_t *)v.pixels;
    for (uint32_t x = 0u; x < 8u; x++) {
        CHECK_EQ_HEX(p[x], row0[x]);
        CHECK_EQ_HEX(p[8u + x], (uint16_t)(row0[x] ^ 0x1111u));
    }
    schway_destroy(inst);
}

static void native24_wrap(void)
{
    schway *inst = make_instance();
    CHECK(inst != NULL);
    if (inst == NULL) {
        return;
    }
    window(inst, 1022u, 511u, 100u, 148u, 0u, 2u, 0x11u);
    uint8_t expected[2][24];
    for (uint32_t y = 0u; y < 2u; y++) {
        uint32_t source_y = (511u + y) & 511u;
        for (uint32_t i = 0u; i < 24u; i++) {
            uint8_t value = (uint8_t)(y * 64u + i + 1u);
            expected[y][i] = value;
            set_vram_byte(inst, source_y, (2044u + i) & 2047u, value);
        }
    }
    schway_video v = video_of(inst);
    CHECK_EQ_U64(v.width, 8u);
    CHECK_EQ_U64(v.height, 2u);
    CHECK_EQ_U64(v.pitch_bytes, 24u);
    CHECK_EQ_U64(v.bpp, 24u);
    CHECK_EQ_U64(v.rgb24, 1u);
    CHECK(memcmp(v.pixels, expected, sizeof expected) == 0);
    schway_destroy(inst);
}

static void interlace_and_off(void)
{
    schway *inst = make_instance();
    CHECK(inst != NULL);
    if (inst == NULL) {
        return;
    }
    inst->frame = 7u;
    window(inst, 0u, 0u, 100u, 148u, 20u, 25u, 0x25u);
    inst->vram[0][0] = 0x7FFFu;
    schway_video v = video_of(inst);
    CHECK_EQ_U64(v.height, 5u);
    CHECK_EQ_U64(v.interlaced, 1u);
    CHECK_EQ_U64(v.vres, 480u);
    CHECK_EQ_U64(v.field, 1u);
    gp1(inst, 3u, 1u);
    v = video_of(inst);
    CHECK_EQ_U64(v.enabled, 0u);
    const uint8_t *bytes = (const uint8_t *)v.pixels;
    uint32_t nonzero = 0u;
    for (size_t i = 0; i < (size_t)v.pitch_bytes * v.height; i++) {
        nonzero += (uint32_t)(bytes[i] != 0u);
    }
    CHECK_EQ_U64(nonzero, 0u);
    schway_destroy(inst);
}

static void malformed_windows(void)
{
    schway *inst = make_instance();
    CHECK(inst != NULL);
    if (inst != NULL) {
        window(inst, 0u, 0u, 100u, 99u, 4u, 8u, 1u);
        schway_video v = video_of(inst);
        CHECK_EQ_U64(v.width, 0u);
        CHECK_EQ_U64(v.height, 0u);
        window(inst, 0u, 0u, 100u, 148u, 4u, 3u, 1u);
        v = video_of(inst);
        CHECK_EQ_U64(v.width, 0u);
        CHECK_EQ_U64(v.height, 0u);
        window(inst, 0u, 0u, 100u, 148u, 0u, 513u, 1u);
        v = video_of(inst);
        CHECK_EQ_U64(v.width, 0u);
        CHECK_EQ_U64(v.height, 0u);
        schway_destroy(inst);
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        return 2;
    }
    if (strcmp(argv[1], "width_modes") == 0) {
        width_modes();
    } else if (strcmp(argv[1], "native15_wrap") == 0) {
        native15_wrap();
    } else if (strcmp(argv[1], "native24_wrap") == 0) {
        native24_wrap();
    } else if (strcmp(argv[1], "interlace_and_off") == 0) {
        interlace_and_off();
    } else if (strcmp(argv[1], "malformed_windows") == 0) {
        malformed_windows();
    } else {
        return 2;
    }
    CHECK_DONE();
}
