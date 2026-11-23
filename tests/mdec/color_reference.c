/* First-display MDEC callers checked against the pinned build-158 captures. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "sha256.h"
#include "internal.h"
#include "input_coverage.h"

int schway_mdec_color_reference_run(int argc, char **argv);

#ifndef SCHWAY_MDEC_FRAME15_EXE_SHA256
#error "CMake must bind the pinned MDEC frame guest digests"
#endif
#ifndef SCHWAY_MDEC_FRAME24_EXE_SHA256
#error "CMake must bind the pinned MDEC frame guest digests"
#endif
#ifndef SCHWAY_MDEC_MOVIE15_EXE_SHA256
#error "CMake must bind the pinned MDEC movie guest digests"
#endif
#ifndef SCHWAY_MDEC_MOVIE24_EXE_SHA256
#error "CMake must bind the pinned MDEC movie guest digests"
#endif
#ifndef SCHWAY_MDEC_FRAME15_VRAM_SHA256
#error "CMake must bind the decoded frame reference digests"
#endif
#ifndef SCHWAY_MDEC_FRAME24_VRAM_SHA256
#error "CMake must bind the decoded frame reference digests"
#endif
#ifndef SCHWAY_MDEC_MOVIE15_VRAM_SHA256
#error "CMake must bind the decoded movie reference digests"
#endif
#ifndef SCHWAY_MDEC_MOVIE24_VRAM_SHA256
#error "CMake must bind the decoded movie reference digests"
#endif
#ifndef SCHWAY_MDEC_FRAME_SCENE_SHA256
#error "CMake must bind the frame display reference digest"
#endif
#ifndef SCHWAY_MDEC_MOVIE_SCENE_SHA256
#error "CMake must bind the movie display reference digest"
#endif

enum {
    COLOR_REFERENCE_FILE_LIMIT = 2u * 1024u * 1024u,
    COLOR_REFERENCE_RAW_SIZE = 1024u * 512u * 2u,
    COLOR_REFERENCE_TTY_LIMIT = 256u * 1024u,
    COLOR_REFERENCE_RUN_LIMIT = 19200u,
    COLOR_REFERENCE_RUN_CYCLES = 256u,
    COLOR_REFERENCE_CYCLE_CAP = COLOR_REFERENCE_RUN_LIMIT * COLOR_REFERENCE_RUN_CYCLES,
    MOVIE15_COMPLETE_BLOCKS = 1440u,
    MOVIE15_COMPLETE_MACROBLOCKS = 240u,
    MOVIE15_EXHAUSTED_COMMAND_WORDS = 1696u,
    MOVIE15_DMA1_REMAIN = 9u,
    MOVIE15_DMA1_PHASE = 2u,
    COLOR_REFERENCE_RAM_SIZE = 2u * 1024u * 1024u
};

typedef struct color_case {
    const char *name;
    const char *exe_sha256;
    const char *vram_sha256;
    uint32_t width;
    uint32_t height;
    uint32_t rgb24;
    uint32_t movie;
} color_case;

static const color_case color_cases[] = {
    {"frame15", SCHWAY_MDEC_FRAME15_EXE_SHA256, SCHWAY_MDEC_FRAME15_VRAM_SHA256,
     320u, 240u, 0u, 0u},
    {"frame24", SCHWAY_MDEC_FRAME24_EXE_SHA256, SCHWAY_MDEC_FRAME24_VRAM_SHA256,
     320u, 240u, 1u, 0u},
    {"movie15", SCHWAY_MDEC_MOVIE15_EXE_SHA256, SCHWAY_MDEC_MOVIE15_VRAM_SHA256,
     256u, 240u, 0u, 1u},
    {"movie24", SCHWAY_MDEC_MOVIE24_EXE_SHA256, SCHWAY_MDEC_MOVIE24_VRAM_SHA256,
     256u, 240u, 1u, 1u}
};

static void *color_reference_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void color_reference_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static uint8_t *read_bounded_file(const char *path, size_t limit, size_t *size_out)
{
    *size_out = 0u;
    FILE *file = fopen(path, "rb");
    if (file == NULL || fseek(file, 0, SEEK_END) != 0) {
        if (file != NULL) fclose(file);
        return NULL;
    }
    long end = ftell(file);
    if (end <= 0 || (unsigned long)end > limit || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    size_t size = (size_t)end;
    uint8_t *bytes = (uint8_t *)malloc(size);
    if (bytes == NULL) {
        fclose(file);
        return NULL;
    }
    size_t count = fread(bytes, 1u, size, file);
    int close_result = fclose(file);
    if (count != size || close_result != 0) {
        free(bytes);
        return NULL;
    }
    *size_out = size;
    return bytes;
}

static int digest_matches(const uint8_t *bytes, size_t size, const char *expected)
{
    schway__sha256 hash;
    uint8_t digest[32];
    char hex[65];
    schway__sha256_init(&hash);
    schway__sha256_update(&hash, bytes, size);
    schway__sha256_final(&hash, digest);
    schway__sha256_hex(digest, hex);
    return strcmp(hex, expected) == 0;
}

static int contains_bytes(const uint8_t *bytes, size_t size,
                          const char *needle, size_t needle_size)
{
    if (needle_size > size) return 0;
    for (size_t i = 0u; i <= size - needle_size; i++) {
        if (memcmp(bytes + i, needle, needle_size) == 0) return 1;
    }
    return 0;
}

static uint32_t read_le32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static uint16_t read_le16(const uint8_t *bytes)
{
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}

static uint32_t ram_word(const schway *inst, uint32_t address)
{
    return read_le32(inst->bus.ram + address);
}

static void write_ram_word(schway *inst, uint32_t address, uint32_t value)
{
    for (uint32_t byte = 0u; byte < 4u; byte++)
        inst->bus.ram[address + byte] = (uint8_t)(value >> (byte * 8u));
}

static int apply_movie_first_frame_patch(schway *inst, const color_case *test_case)
{
    if (test_case->movie == 0u) return 1;
    /* These two instructions are pinned by the source at f727802fead1. The
       first removes the 120-VSync startup delay; the second ends after frame 1. */
    uint32_t wait_branch = test_case->rgb24 != 0u ? 0x000100B8u : 0x00010098u;
    uint32_t frame_limit = test_case->rgb24 != 0u ? 0x000100D0u : 0x000100B0u;
    if (wait_branch > COLOR_REFERENCE_RAM_SIZE - 4u ||
        frame_limit > COLOR_REFERENCE_RAM_SIZE - 4u ||
        ram_word(inst, wait_branch) != 0x1600FFFDu ||
        ram_word(inst, frame_limit) != 0x265407D0u) {
        fprintf(stderr, "MDEC_COLOR_PATCH_ERROR: %s guest code changed at first-frame patch sites\n",
                test_case->name);
        return 0;
    }
    write_ram_word(inst, wait_branch, 0u);
    write_ram_word(inst, frame_limit, 0x26540008u);
    return 1;
}

static int check_movie_patch_asset(const char *path, const char *digest,
                                   const color_case *test_case)
{
    size_t exe_size = 0u;
    uint8_t *exe = read_bounded_file(path, COLOR_REFERENCE_FILE_LIMIT, &exe_size);
    if (exe == NULL || !digest_matches(exe, exe_size, digest)) {
        free(exe);
        return 0;
    }
    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = color_reference_alloc;
    config.allocator.free = color_reference_free;
    schway *inst = NULL;
    schway_status status = schway_create(&config, &inst);
    if (status != SCHWAY_STATUS_OK || inst == NULL) {
        free(exe);
        schway_destroy(inst);
        return 0;
    }
    schway_profile profile;
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = SCHWAY_REGION_NTSC_U;
    profile.flags = SCHWAY_PROFILE_DEBUG_PORT;
    int loaded = schway_set_profile(inst, &profile) == SCHWAY_STATUS_OK &&
                 schway_load_exe(inst, exe, exe_size) == SCHWAY_STATUS_OK &&
                 apply_movie_first_frame_patch(inst, test_case);
    free(exe);
    schway_destroy(inst);
    return loaded;
}

static const color_case *find_color_case(const char *name)
{
    for (size_t i = 0u; i < sizeof color_cases / sizeof color_cases[0]; i++) {
        if (strcmp(color_cases[i].name, name) == 0) return &color_cases[i];
    }
    return NULL;
}

static int color_asset_check(int argc, char **argv)
{
    static const char *const exe_hashes[] = {
        SCHWAY_MDEC_FRAME15_EXE_SHA256, SCHWAY_MDEC_FRAME24_EXE_SHA256,
        SCHWAY_MDEC_MOVIE15_EXE_SHA256, SCHWAY_MDEC_MOVIE24_EXE_SHA256
    };
    static const char *const vram_hashes[] = {
        SCHWAY_MDEC_FRAME15_VRAM_SHA256, SCHWAY_MDEC_FRAME24_VRAM_SHA256,
        SCHWAY_MDEC_MOVIE15_VRAM_SHA256, SCHWAY_MDEC_MOVIE24_VRAM_SHA256
    };
    if (argc != 10) return 2;
    for (size_t i = 0u; i < 4u; i++) {
        size_t size = 0u;
        uint8_t *exe = read_bounded_file(argv[2u + i], COLOR_REFERENCE_FILE_LIMIT, &size);
        int valid = exe != NULL && digest_matches(exe, size, exe_hashes[i]);
        free(exe);
        if (!valid) {
            fprintf(stderr, "MDEC_COLOR_ASSET_ERROR: pinned guest %s is missing or changed\n",
                    argv[2u + i]);
            return 1;
        }
    }
    for (size_t i = 0u; i < 4u; i++) {
        size_t size = 0u;
        uint8_t *raw = read_bounded_file(argv[6u + i], COLOR_REFERENCE_RAW_SIZE, &size);
        int valid = raw != NULL && size == COLOR_REFERENCE_RAW_SIZE &&
                    digest_matches(raw, size, vram_hashes[i]);
        free(raw);
        if (!valid) {
            fprintf(stderr, "MDEC_COLOR_ASSET_ERROR: decoded VRAM reference %s is missing or changed\n",
                    argv[6u + i]);
            return 1;
        }
    }
    for (size_t i = 2u; i < 4u; i++) {
        if (!check_movie_patch_asset(argv[2u + i], exe_hashes[i], &color_cases[i])) {
            fprintf(stderr, "MDEC_COLOR_ASSET_ERROR: movie first-frame patch does not match pinned code\n");
            return 1;
        }
    }
    puts("MDEC_COLOR_ASSETS_OK: four guests, four decoded PNGs, and movie patch sites verified");
    return 0;
}

static int channel_within_tolerance(uint32_t actual, uint32_t expected)
{
    return actual > expected ? actual - expected <= 1u : expected - actual <= 1u;
}

static uint32_t channel_distance(uint32_t actual, uint32_t expected)
{
    return actual > expected ? actual - expected : expected - actual;
}

static int rgb555_within_tolerance(uint16_t actual, uint16_t expected)
{
    for (uint32_t channel = 0u; channel < 3u; channel++) {
        uint32_t shift = channel * 5u;
        if (!channel_within_tolerance((actual >> shift) & 31u,
                                      (expected >> shift) & 31u)) return 0;
    }
    return 1;
}

static int color_controls(void)
{
    uint16_t expected = (uint16_t)(10u | (20u << 5u) | (30u << 10u));
    uint16_t within = (uint16_t)(11u | (19u << 5u) | (31u << 10u));
    uint16_t outside = (uint16_t)(12u | (20u << 5u) | (30u << 10u));
    if (!rgb555_within_tolerance(within, expected) ||
        rgb555_within_tolerance(outside, expected) ||
        !channel_within_tolerance(248u >> 3u, 247u >> 3u) ||
        channel_within_tolerance(255u >> 3u, 232u >> 3u)) {
        fprintf(stderr, "MDEC_COLOR_CONTROL_ERROR: per-channel ±1 tolerance controls failed\n");
        return 1;
    }
    puts("MDEC_COLOR_CONTROLS_OK: one-step channel error accepted, two-step error rejected");
    return 0;
}

static int run_color_guest(const color_case *test_case, const char *exe_path,
                           const char *vram_path, const char *scene_path)
{
    size_t exe_size = 0u;
    size_t vram_size = 0u;
    size_t scene_size = 0u;
    uint8_t *exe = read_bounded_file(exe_path, COLOR_REFERENCE_FILE_LIMIT, &exe_size);
    uint8_t *vram_expected = read_bounded_file(vram_path, COLOR_REFERENCE_RAW_SIZE, &vram_size);
    uint8_t *scene_expected = read_bounded_file(scene_path, COLOR_REFERENCE_RAW_SIZE, &scene_size);
    const char *scene_sha256 = test_case->movie != 0u ? SCHWAY_MDEC_MOVIE_SCENE_SHA256 :
                                                       SCHWAY_MDEC_FRAME_SCENE_SHA256;
    if (exe == NULL || !digest_matches(exe, exe_size, test_case->exe_sha256) ||
        vram_expected == NULL || vram_size != COLOR_REFERENCE_RAW_SIZE ||
        !digest_matches(vram_expected, vram_size, test_case->vram_sha256) ||
        scene_expected == NULL || scene_size != COLOR_REFERENCE_RAW_SIZE ||
        !digest_matches(scene_expected, scene_size, scene_sha256)) {
        fprintf(stderr, "MDEC_COLOR_IDENTITY_ERROR: %s guest or decoded PNG is missing or changed\n",
                test_case->name);
        free(exe);
        free(vram_expected);
        free(scene_expected);
        return 2;
    }

    schway__mdec_coverage input_coverage;
    memset(&input_coverage, 0, sizeof input_coverage);
    if (!schway__mdec_reference_color_input_guard(test_case->name, exe, exe_size,
                                                   &input_coverage)) {
        fprintf(stderr,
                "MDEC_COLOR_INPUT_GUARD_ERROR: %s pinned source span is not uniquely mapped or complete\n",
                test_case->name);
        free(exe);
        free(vram_expected);
        free(scene_expected);
        return 2;
    }
    printf("MDEC_COLOR_INPUT_GUARD_OK: mode=%s blocks=%u macroblocks=%u trailing_blocks=%u\n",
           test_case->name, input_coverage.complete_blocks,
           input_coverage.complete_macroblocks, input_coverage.trailing_blocks);

    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = color_reference_alloc;
    config.allocator.free = color_reference_free;
    schway *inst = NULL;
    schway_status status = schway_create(&config, &inst);
    if (status != SCHWAY_STATUS_OK || inst == NULL) {
        free(exe);
        free(vram_expected);
        free(scene_expected);
        schway_destroy(inst);
        return 1;
    }
    schway_profile profile;
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = SCHWAY_REGION_NTSC_U;
    profile.flags = SCHWAY_PROFILE_DEBUG_PORT;
    if (schway_set_profile(inst, &profile) != SCHWAY_STATUS_OK ||
        schway_load_exe(inst, exe, exe_size) != SCHWAY_STATUS_OK ||
        !apply_movie_first_frame_patch(inst, test_case)) {
        fprintf(stderr, "MDEC_COLOR_GUEST_ERROR: %s guest failed to load or prepare\n",
                test_case->name);
        free(exe);
        free(vram_expected);
        free(scene_expected);
        schway_destroy(inst);
        return 1;
    }
    free(exe);

    uint8_t *tty_text = (uint8_t *)calloc(COLOR_REFERENCE_TTY_LIMIT, 1u);
    if (tty_text == NULL) {
        free(vram_expected);
        free(scene_expected);
        schway_destroy(inst);
        return 1;
    }
    size_t tty_size = 0u;
    uint32_t steps = 0u;
    uint32_t exhausted_input_samples = 0u;
    int complete = 0;
    int input_exhausted = 0;
    int normal_run = 1;
    schway_budget budget;
    memset(&budget, 0, sizeof budget);
    budget.size = (uint32_t)sizeof budget;
    budget.max_cycles = COLOR_REFERENCE_RUN_CYCLES;
    budget.flags = SCHWAY_RUN_STOP_ON_FAULT;
    for (steps = 0u; steps < COLOR_REFERENCE_RUN_LIMIT; steps++) {
        schway_run_result result;
        memset(&result, 0, sizeof result);
        result.size = (uint32_t)sizeof result;
        status = schway_run(inst, &budget, &result);
        if (status != SCHWAY_STATUS_OK || result.fault_present != 0u ||
            (result.stop != SCHWAY_STOP_FRAME && result.stop != SCHWAY_STOP_BUDGET)) {
            normal_run = 0;
            fprintf(stderr, "MDEC_COLOR_GUEST_ERROR: %s stopped status=%s stop=%u fault=%u\n",
                    test_case->name, schway_status_string(status), result.stop,
                    result.fault_present);
            break;
        }
        uint8_t drained[4096];
        schway_tty tty;
        memset(&tty, 0, sizeof tty);
        tty.size = (uint32_t)sizeof tty;
        tty.bytes = drained;
        tty.capacity = (uint32_t)sizeof drained;
        status = schway_get_tty(inst, &tty);
        if (status != SCHWAY_STATUS_OK || tty.dropped != 0u ||
            tty.written > sizeof drained || tty.written > COLOR_REFERENCE_TTY_LIMIT - tty_size) {
            normal_run = 0;
            fprintf(stderr, "MDEC_COLOR_GUEST_ERROR: %s TTY drain exceeded its bound\n",
                    test_case->name);
            break;
        }
        memcpy(tty_text + tty_size, drained, tty.written);
        tty_size += tty.written;
        if (contains_bytes(tty_text, tty_size, "Done", sizeof "Done" - 1u)) {
            complete = 1;
            steps++;
            break;
        }
        int dma1_waiting = (inst->dma.chcr[1] & SCHWAY__DMA_CHCR_BUSY) != 0u;
        int frame_reader_waiting = test_case->movie == 0u &&
                                   (inst->dma.chcr[2] & SCHWAY__DMA_CHCR_BUSY) == 0u;
        if ((inst->mdec.command >> 29u) == 1u &&
            inst->mdec.command_words != 0u &&
            inst->mdec.command_active == 0u && inst->mdec.block_pending == 0u &&
            inst->mdec.input_count == 0u && inst->mdec.words_remaining == 0u &&
            inst->mdec.output_count == 0u &&
            (inst->dma.chcr[0] & SCHWAY__DMA_CHCR_BUSY) == 0u &&
            (dma1_waiting != 0 || frame_reader_waiting != 0)) {
            exhausted_input_samples++;
            if (exhausted_input_samples >= 8u) {
                input_exhausted = 1;
                break;
            }
        } else {
            exhausted_input_samples = 0u;
        }
    }
    if (!complete) {
        uint64_t cycle_cap = (uint64_t)COLOR_REFERENCE_CYCLE_CAP;
        int movie15_expected_incomplete =
            strcmp(test_case->name, "movie15") == 0 && test_case->movie != 0u &&
            input_coverage.complete_blocks == MOVIE15_COMPLETE_BLOCKS &&
            input_coverage.complete_macroblocks == MOVIE15_COMPLETE_MACROBLOCKS &&
            input_coverage.trailing_blocks == 0u &&
            input_coverage.trailing_coefficients == 0u && normal_run != 0 &&
            input_exhausted != 0 && steps < COLOR_REFERENCE_RUN_LIMIT &&
            inst->ticks < cycle_cap && inst->mdec.command == UINT32_C(0x380006A0) &&
            inst->mdec.command_words == MOVIE15_EXHAUSTED_COMMAND_WORDS &&
            inst->mdec.command_active == 0u && inst->mdec.block_pending == 0u &&
            inst->mdec.input_count == 0u && inst->mdec.words_remaining == 0u &&
            inst->mdec.output_count == 0u &&
            (inst->dma.chcr[0] & SCHWAY__DMA_CHCR_BUSY) == 0u &&
            (inst->dma.chcr[1] & SCHWAY__DMA_CHCR_BUSY) != 0u &&
            inst->dma.words[1] == 0u && inst->dma.remain[1] == MOVIE15_DMA1_REMAIN &&
            inst->dma.phase[1] == MOVIE15_DMA1_PHASE && inst->dma.dreq == 0x04u &&
            inst->dma.words[2] == 0u;
        if (movie15_expected_incomplete != 0) {
            printf("MDEC_COLOR_EDGE_RESULT: mode=movie15 source=complete blocks=%u macroblocks=%u decoder=output-fifo-empty guest=expected-incomplete display=not-compared ticks=%llu cycle_cap=%llu dma0=idle dma1=waiting dma1_remain=%u dma1_phase=%u fault=none\n",
                   input_coverage.complete_blocks, input_coverage.complete_macroblocks,
                   (unsigned long long)inst->ticks, (unsigned long long)cycle_cap,
                   inst->dma.remain[1], inst->dma.phase[1]);
            free(tty_text);
            free(vram_expected);
            free(scene_expected);
            schway_destroy(inst);
            return 0;
        }
        fprintf(stderr, "MDEC_COLOR_GUEST_ERROR: %s did not finish (%s; ticks=%llu)\n",
                test_case->name,
                input_exhausted != 0 ? "input exhausted while waiting for decoded pixels" :
                                       "run cap reached",
                (unsigned long long)inst->ticks);
        fprintf(stderr,
                "MDEC_COLOR_GUEST_STATE: pc=%08x a0=%08x v0=%08x ra=%08x mdec=%08x active=%u pending=%u in=%u rem=%u out=%u dma0=%08x/%08x/%08x dma1=%08x/%08x/%08x dma2=%08x/%08x/%08x\n",
                inst->cpu.pc, inst->cpu.r[4], inst->cpu.r[2], inst->cpu.r[31],
                inst->mdec.command, inst->mdec.command_active, inst->mdec.block_pending,
                inst->mdec.input_count, inst->mdec.words_remaining, inst->mdec.output_count,
                inst->dma.chcr[0], inst->dma.madr[0], inst->dma.bcr[0],
                inst->dma.chcr[1], inst->dma.madr[1], inst->dma.bcr[1],
                inst->dma.chcr[2], inst->dma.madr[2], inst->dma.bcr[2]);
        fprintf(stderr, "MDEC_COLOR_DMA_PROGRESS: dreq=%02x dma1_words=%u dma1_remain=%u dma1_phase=%u dma2_words=%u\n",
                inst->dma.dreq, inst->dma.words[1], inst->dma.remain[1], inst->dma.phase[1],
                inst->dma.words[2]);
        free(tty_text);
        free(vram_expected);
        free(scene_expected);
        schway_destroy(inst);
        return 1;
    }
    free(tty_text);
    printf("MDEC_COLOR_GUEST_RESULT: mode=%s completion=Done ticks=%llu cycle_cap=%u\n",
           test_case->name, (unsigned long long)inst->ticks,
           (unsigned)COLOR_REFERENCE_CYCLE_CAP);

    uint32_t vram_x = 0u;
    uint32_t vram_y = 0u;
    uint32_t bad_pixels = 0u;
    uint32_t bad_channels[3] = {0u, 0u, 0u};
    uint32_t max_distance[3] = {0u, 0u, 0u};
    for (uint32_t y = 0u; y < 512u; y++) {
        for (uint32_t x = 0u; x < 1024u; x++) {
            uint16_t expected = read_le16(vram_expected + ((size_t)y * 1024u + x) * 2u);
            uint16_t actual = inst->vram[y][x];
            int pixel_bad = 0;
            for (uint32_t channel = 0u; channel < 3u; channel++) {
                uint32_t shift = channel * 5u;
                uint32_t distance = channel_distance((actual >> shift) & 31u,
                                                     (expected >> shift) & 31u);
                if (distance > max_distance[channel]) max_distance[channel] = distance;
                if (distance > 1u) {
                    bad_channels[channel]++;
                    pixel_bad = 1;
                }
            }
            if (pixel_bad != 0) {
                bad_pixels++;
            }
            if (pixel_bad != 0 && vram_y == 0u) {
                vram_x = x + 1u;
                vram_y = y + 1u;
            }
        }
    }
    if (vram_y != 0u) {
        size_t at = ((size_t)(vram_y - 1u) * 1024u + vram_x - 1u) * 2u;
        fprintf(stderr,
                "MDEC_COLOR_VRAM_MISMATCH: %s x=%u y=%u expected=%04x actual=%04x\n",
                test_case->name, vram_x - 1u, vram_y - 1u,
                read_le16(vram_expected + at), inst->vram[vram_y - 1u][vram_x - 1u]);
        fprintf(stderr,
                "MDEC_COLOR_VRAM_DIFF: pixels=%u channels=%u/%u/%u max=%u/%u/%u\n",
                bad_pixels, bad_channels[0], bad_channels[1], bad_channels[2],
                max_distance[0], max_distance[1], max_distance[2]);
        if (vram_y == 1u && vram_x <= 4u) {
            fputs("MDEC_COLOR_VRAM_ROW0:", stderr);
            for (uint32_t x = 0u; x < 8u; x++) {
                fprintf(stderr, " %04x/%04x",
                        read_le16(vram_expected + (size_t)x * 2u), inst->vram[0][x]);
            }
            fputc('\n', stderr);
        }
        free(vram_expected);
        free(scene_expected);
        schway_destroy(inst);
        return 1;
    }
    printf("MDEC_COLOR_DECODER_RESULT: mode=%s output=matched raw_vram=1024x512 pixels=%u\n",
           test_case->name, 1024u * 512u);
    free(vram_expected);

    schway_video video;
    memset(&video, 0, sizeof video);
    video.size = (uint32_t)sizeof video;
    status = schway_get_video(inst, &video);
    if (status != SCHWAY_STATUS_OK || video.pixels == NULL ||
        video.width != test_case->width || video.height != test_case->height ||
        video.rgb24 != test_case->rgb24 || video.start_x != 0u || video.start_y != 0u ||
        video.enabled == 0u || video.bpp != (test_case->rgb24 != 0u ? 24u : 16u)) {
        fprintf(stderr,
                "MDEC_COLOR_VIDEO_ERROR: %s status=%s size=%ux%u rgb24=%u start=%u,%u enabled=%u bpp=%u\n",
                test_case->name, schway_status_string(status), video.width, video.height,
                video.rgb24, video.start_x, video.start_y, video.enabled, video.bpp);
        free(scene_expected);
        schway_destroy(inst);
        return 1;
    }
    uint32_t mismatch_x = 0u;
    uint32_t mismatch_y = 0u;
    for (uint32_t y = 0u; y < video.height && mismatch_y == 0u; y++) {
        for (uint32_t x = 0u; x < video.width; x++) {
            uint16_t expected = read_le16(scene_expected + ((size_t)y * 1024u + x) * 2u);
            if (video.rgb24 == 0u) {
                const uint8_t *pixel = (const uint8_t *)video.pixels +
                                       (size_t)y * video.pitch_bytes + (size_t)x * 2u;
                if (!rgb555_within_tolerance(read_le16(pixel), expected)) {
                    mismatch_x = x + 1u;
                    mismatch_y = y + 1u;
                    break;
                }
            } else {
                const uint8_t *pixel = (const uint8_t *)video.pixels +
                                       (size_t)y * video.pitch_bytes + (size_t)x * 3u;
                uint32_t expected_channels[3] = {
                    expected & 31u, (expected >> 5u) & 31u, (expected >> 10u) & 31u
                };
                for (uint32_t channel = 0u; channel < 3u; channel++) {
                    if (!channel_within_tolerance(pixel[channel] >> 3u,
                                                  expected_channels[channel])) {
                        mismatch_x = x + 1u;
                        mismatch_y = y + 1u;
                        break;
                    }
                }
                if (mismatch_y != 0u) break;
            }
        }
    }
    if (mismatch_y != 0u) {
        fprintf(stderr, "MDEC_COLOR_DISPLAY_MISMATCH: %s x=%u y=%u\n",
                test_case->name, mismatch_x - 1u, mismatch_y - 1u);
        free(scene_expected);
        schway_destroy(inst);
        return 1;
    }
    printf("MDEC_COLOR_DISPLAY_RESULT: mode=%s comparison=matched dimensions=%ux%u\n",
           test_case->name, video.width, video.height);
    free(scene_expected);
    printf("MDEC_COLOR_REFERENCE_PASS: mode=%s dimensions=%ux%u raw_vram=1024x512 ticks=%llu steps=%u guest=%s\n",
           test_case->name, video.width, video.height, (unsigned long long)inst->ticks, steps,
           complete != 0 ? "Done" : "frame-input-exhausted");
    schway_destroy(inst);
    return 0;
}

static int run_color_coverage(const char *name, const char *exe_path)
{
    const color_case *test_case = find_color_case(name);
    if (test_case == NULL) return 2;
    size_t exe_size = 0u;
    uint8_t *exe = read_bounded_file(exe_path, COLOR_REFERENCE_FILE_LIMIT, &exe_size);
    schway__mdec_coverage coverage;
    memset(&coverage, 0, sizeof coverage);
    int valid = exe != NULL && digest_matches(exe, exe_size, test_case->exe_sha256) &&
                schway__mdec_reference_color_input_guard(test_case->name, exe, exe_size,
                                                          &coverage);
    uint32_t requested_macroblocks = test_case->movie != 0u ? 240u : 300u;
    if (!valid || coverage.complete_macroblocks != requested_macroblocks ||
        coverage.trailing_blocks != 0u || coverage.trailing_coefficients != 0u) {
        fprintf(stderr, "MDEC_COLOR_COVERAGE_ERROR: %s pinned source coverage changed\n",
                test_case->name);
        free(exe);
        return 1;
    }
    printf("MDEC_COLOR_SOURCE_COVERAGE_OK: mode=%s blocks=%u macroblocks=%u requested=%u trailing_blocks=%u\n",
           test_case->name, coverage.complete_blocks, coverage.complete_macroblocks,
           requested_macroblocks, coverage.trailing_blocks);
    free(exe);
    return 0;
}

int schway_mdec_color_reference_run(int argc, char **argv)
{
    if (argc == 10 && strcmp(argv[1], "color-assets") == 0)
        return color_asset_check(argc, argv);
    if (argc == 2 && strcmp(argv[1], "color-controls") == 0) return color_controls();
    if (argc == 4 && strcmp(argv[1], "color-coverage") == 0)
        return run_color_coverage(argv[2], argv[3]);
    if (argc == 6 && strcmp(argv[1], "color") == 0) {
        const color_case *test_case = find_color_case(argv[2]);
        if (test_case != NULL)
            return run_color_guest(test_case, argv[3], argv[4], argv[5]);
    }
    fprintf(stderr,
            "MDEC_COLOR_USAGE_ERROR: expected color CASE EXE VRAM_RGB555LE SCENE_RGB555LE, color-coverage CASE EXE, color-assets EXE4 EXE8 EXEM15 EXEM24 PNG4 PNG8 PNGM15 PNGM24, or color-controls\n");
    return 2;
}
