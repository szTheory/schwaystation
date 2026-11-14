/* Full-stream comparison between the guest SPU path and a CPU-free replay. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/internal.h"
#include "schwaystation.h"

#define FIXTURE_FRAMES 601u
#define MAX_PAIRS (FIXTURE_FRAMES * 887u)
#define RING_PAIRS (4u * 887u)

static uint32_t failures;
static uint32_t checks;
static int16_t guest_pcm[MAX_PAIRS * 2u];
static int16_t cpu_free_pcm[MAX_PAIRS * 2u];
static int16_t drain_pcm[RING_PAIRS * 2u];

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

static void expect(uint32_t condition, const char *message)
{
    checks++;
    (void)printf("%s %u - %s\n", condition != 0u ? "ok" : "not ok", checks, message);
    failures += condition == 0u ? 1u : 0u;
}

static uint32_t file_exists(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return 0u;
    }
    (void)fclose(file);
    return 1u;
}

static schway *make_instance(uint32_t region)
{
    schway_config config;
    schway_profile profile;
    schway *inst = NULL;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = plain_alloc;
    config.allocator.free = plain_free;
    if (schway_create(&config, &inst) != SCHWAY_STATUS_OK || inst == NULL) {
        return NULL;
    }
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = region;
    if (schway_set_profile(inst, &profile) != SCHWAY_STATUS_OK) {
        schway_destroy(inst);
        return NULL;
    }
    return inst;
}

static uint32_t append_audio(schway *inst, int16_t *pcm, uint32_t *pairs)
{
    schway_audio audio;
    memset(drain_pcm, 0x5A, sizeof drain_pcm);
    memset(&audio, 0, sizeof audio);
    audio.size = (uint32_t)sizeof audio;
    audio.samples = drain_pcm;
    audio.capacity = RING_PAIRS;
    if (schway_get_audio(inst, &audio) != SCHWAY_STATUS_OK || audio.dropped != 0u ||
        audio.written > MAX_PAIRS - *pairs) {
        return 1u;
    }
    memcpy(pcm + (size_t)*pairs * 2u, drain_pcm, (size_t)audio.written * 2u * sizeof(int16_t));
    *pairs += audio.written;
    return 0u;
}

static uint32_t capture_guest(const char *exe_path, uint32_t region, uint32_t *pair_count)
{
    size_t exe_size = 0u;
    FILE *file = fopen(exe_path, "rb");
    if (file == NULL || fseek(file, 0, SEEK_END) != 0) {
        if (file != NULL) {
            (void)fclose(file);
        }
        return 1u;
    }
    long length = ftell(file);
    if (length <= 0 || fseek(file, 0, SEEK_SET) != 0) {
        (void)fclose(file);
        return 1u;
    }
    uint8_t *exe = (uint8_t *)malloc((size_t)length);
    if (exe == NULL || fread(exe, 1u, (size_t)length, file) != (size_t)length) {
        free(exe);
        (void)fclose(file);
        return 1u;
    }
    exe_size = (size_t)length;
    (void)fclose(file);

    schway *inst = make_instance(region);
    if (inst == NULL) {
        free(exe);
        return 1u;
    }
    uint32_t status = (uint32_t)schway_load_exe(inst, exe, exe_size);
    free(exe);
    if (status != SCHWAY_STATUS_OK) {
        schway_destroy(inst);
        return 1u;
    }

    *pair_count = 0u;
    for (uint32_t frame = 0u; frame < FIXTURE_FRAMES; frame++) {
        schway_run_result result;
        memset(&result, 0, sizeof result);
        result.size = (uint32_t)sizeof result;
        if (schway_run_frame(inst, &result) != SCHWAY_STATUS_OK ||
            result.stop != SCHWAY_STOP_FRAME || append_audio(inst, guest_pcm, pair_count) != 0u) {
            schway_destroy(inst);
            return 1u;
        }
    }
    schway_destroy(inst);
    return 0u;
}

int main(int argc, char **argv)
{
    (void)printf("TAP version 13\n");
    if (argc != 4) {
        (void)fprintf(stderr, "usage: spu-fixture spu_tone.exe spu_blocks.inc schwaystation-run\n");
        return 2;
    }

    expect(file_exists(argv[1]), "the built tone fixture is available");
    expect(file_exists(argv[2]), "the authored BRR table source is available");
    expect(file_exists(argv[3]), "the built runner is available");

    uint32_t guest_pairs = 0u;
    expect(capture_guest(argv[1], SCHWAY_REGION_NTSC_U, &guest_pairs) == 0u,
           "the guest tone fixture runs and drains its complete PCM stream");
    uint32_t nonzero = 0u;
    for (uint32_t i = 0u; i < guest_pairs * 2u; i++) {
        nonzero |= guest_pcm[i] != 0 ? 1u : 0u;
    }
    expect(nonzero, "the guest tone fixture emits nonzero PCM");

    /* RED baseline: the CPU-free replay is wired by the GREEN implementation. */
    expect(guest_pairs != 0u && memcmp(guest_pcm, cpu_free_pcm,
                                      (size_t)guest_pairs * 2u * sizeof(int16_t)) == 0,
           "the CPU-free authored table path matches every guest PCM pair");

    (void)printf("1..%u\n", checks);
    if (failures == 0u) {
        return 0;
    }
    return 1;
}
