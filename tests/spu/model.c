/* Public PCM delivery from a bounded run through schway_get_audio. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/internal.h"
#include "schwaystation.h"
#include "../check.h"

#define AUDIO_RING_PAIRS (4u * 887u)
#define NTSC_FRAME_NUM UINT64_C(405350378496)
#define NTSC_FRAME_DEN UINT64_C(715909)
#define PAL_FRAME_NUM UINT64_C(482961862656)
#define PAL_FRAME_DEN UINT64_C(709379)

static int16_t samples[2u * AUDIO_RING_PAIRS];

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

static schway *make(uint32_t region)
{
    schway_config config;
    schway_profile profile;
    schway *inst = NULL;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = plain_alloc;
    config.allocator.free = plain_free;
    CHECK_EQ_U64(schway_create(&config, &inst), SCHWAY_STATUS_OK);
    CHECK(inst != NULL);
    if (inst == NULL) {
        return NULL;
    }
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = region;
    CHECK_EQ_U64(schway_set_profile(inst, &profile), SCHWAY_STATUS_OK);
    return inst;
}

static schway_audio drain(schway *inst, uint32_t capacity)
{
    schway_audio audio;
    memset(samples, 0x5A, sizeof samples);
    memset(&audio, 0, sizeof audio);
    audio.size = (uint32_t)sizeof audio;
    audio.samples = samples;
    audio.capacity = capacity;
    CHECK_EQ_U64(schway_get_audio(inst, &audio), SCHWAY_STATUS_OK);
    return audio;
}

static schway_run_result run_frame(schway *inst)
{
    schway_run_result result;
    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    CHECK_EQ_U64(schway_run_frame(inst, &result), SCHWAY_STATUS_OK);
    return result;
}

static schway_run_result run_cycles(schway *inst, uint64_t cycles)
{
    schway_budget budget;
    schway_run_result result;
    memset(&budget, 0, sizeof budget);
    budget.size = (uint32_t)sizeof budget;
    budget.max_cycles = cycles;
    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    CHECK_EQ_U64(schway_run(inst, &budget, &result), SCHWAY_STATUS_OK);
    return result;
}

static uint8_t *read_fixture(const char *directory, const char *name, size_t *size)
{
    char path[1024];
    int written = snprintf(path, sizeof path, "%s/%s", directory, name);
    if (written < 0 || (size_t)written >= sizeof path) {
        return NULL;
    }
    FILE *file = fopen(path, "rb");
    if (file == NULL || fseek(file, 0, SEEK_END) != 0) {
        if (file != NULL) {
            fclose(file);
        }
        return NULL;
    }
    long length = ftell(file);
    if (length <= 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    uint8_t *bytes = (uint8_t *)malloc((size_t)length);
    if (bytes == NULL || fread(bytes, 1u, (size_t)length, file) != (size_t)length) {
        free(bytes);
        fclose(file);
        return NULL;
    }
    fclose(file);
    *size = (size_t)length;
    return bytes;
}

static uint32_t tone_word_le(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) | ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static void check_tone_mailbox(const schway *inst, uint32_t count)
{
    const uint8_t *mb = inst->bus.ram + 0x10000u;
    CHECK(memcmp(mb, "SWMB", 4u) == 0);
    CHECK_EQ_U64(mb[8], 0u);
    CHECK_EQ_U64(mb[9], 1u);
    CHECK_EQ_U64(mb[10], count);
    CHECK_EQ_HEX(tone_word_le(mb + 4u), schway__crc32_update(0u, mb + 0x10u, 0xF0u));
}

static uint32_t tone_record(const schway *inst, uint32_t n)
{
    const uint8_t *record = inst->bus.ram + 0x10000u + 0x10u + 8u * (n - 1u);
    CHECK_EQ_U64((uint32_t)record[0] | ((uint32_t)record[1] << 8), n);
    CHECK_EQ_U64((uint32_t)record[2] | ((uint32_t)record[3] << 8), 1u);
    return tone_word_le(record + 4u);
}

/* The guest must boot through the stub, upload a BRR block by IO, key a voice,
   and produce nonzero asymmetric output through the public audio drain. */
static void check_tone_fixture(const char *directory)
{
    size_t exe_size = 0u;
    uint8_t *exe = read_fixture(directory, "spu_tone.exe", &exe_size);
    CHECK(exe != NULL);
    if (exe == NULL) {
        return;
    }

    schway *inst = make(SCHWAY_REGION_NTSC_U);
    if (inst == NULL) {
        free(exe);
        return;
    }
    CHECK_EQ_U64(schway_load_exe(inst, exe, exe_size), SCHWAY_STATUS_OK);
    free(exe);

    uint64_t previous_ticks = 0u;
    uint64_t total_pairs = 0u;
    uint64_t left_energy = 0u;
    uint64_t right_energy = 0u;
    for (uint32_t frame = 0u; frame < 621u; frame++) {
        schway_run_result result = run_frame(inst);
        CHECK_EQ_U64(result.stop, SCHWAY_STOP_FRAME);
        CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
        uint64_t expected_pairs = result.ticks / SCHWAY__CYCLES_PER_PAIR -
                                  previous_ticks / SCHWAY__CYCLES_PER_PAIR;
        schway_audio audio = drain(inst, AUDIO_RING_PAIRS);
        CHECK_EQ_U64(audio.written, expected_pairs);
        CHECK_EQ_U64(audio.dropped, 0u);
        total_pairs += audio.written;
        for (uint32_t pair = 0u; pair < audio.written; pair++) {
            int32_t left = samples[pair * 2u];
            int32_t right = samples[pair * 2u + 1u];
            left_energy += (uint64_t)(left < 0 ? -left : left);
            right_energy += (uint64_t)(right < 0 ? -right : right);
        }
        previous_ticks = result.ticks;
    }
    CHECK_EQ_U64(total_pairs, previous_ticks / SCHWAY__CYCLES_PER_PAIR);
    CHECK(left_energy > 0u);
    CHECK(right_energy > 0u);
    CHECK(left_energy != right_energy);
    CHECK_EQ_HEX(schway__spu_ram_peek(inst, 0x1000u), 0x0704u);
    CHECK_EQ_HEX(schway__spu_ram_peek(inst, 0x2000u), 0x0704u);
    CHECK_EQ_HEX(inst->spu.voice[0].reg[2], 0x1000u);
    CHECK_EQ_HEX(inst->spu.voice[1].reg[2], 0x1800u);
    check_tone_mailbox(inst, 3u);
    CHECK_EQ_U64(tone_record(inst, 1u), 0x7FFFu);
    CHECK_EQ_U64(tone_record(inst, 2u), 0x7FFFu);
    CHECK_EQ_U64(tone_record(inst, 3u), 600u);
    schway_destroy(inst);
}

static void check_tone_state_continuation(const char *directory)
{
    size_t exe_size = 0u;
    uint8_t *exe = read_fixture(directory, "spu_tone.exe", &exe_size);
    CHECK(exe != NULL);
    if (exe == NULL) {
        return;
    }

    schway *source = make(SCHWAY_REGION_NTSC_U);
    schway *loaded = make(SCHWAY_REGION_NTSC_U);
    if (source == NULL || loaded == NULL) {
        free(exe);
        if (source != NULL) {
            schway_destroy(source);
        }
        if (loaded != NULL) {
            schway_destroy(loaded);
        }
        return;
    }
    CHECK_EQ_U64(schway_load_exe(source, exe, exe_size), SCHWAY_STATUS_OK);
    free(exe);

    for (uint32_t frame = 0u; frame < 120u; frame++) {
        CHECK_EQ_U64(run_frame(source).stop, SCHWAY_STOP_FRAME);
        CHECK_EQ_U64(drain(source, AUDIO_RING_PAIRS).dropped, 0u);
    }
    CHECK(source->spu.voice[0].envelope != 0);
    CHECK(source->spu.capture_cursor != 0u);

    /* Exercise a pending noise-source write at the save boundary as well as
       the fixture's live BRR voice and decoder history. */
    schway__spu_reg_write(source, 0x1F801D94u, 1u);
    size_t state_size = schway_state_size(source);
    uint8_t *state = state_size == 0u ? NULL : (uint8_t *)malloc(state_size);
    CHECK(state != NULL);
    if (state != NULL) {
        CHECK_EQ_U64(schway_save_state(source, state, state_size), SCHWAY_STATUS_OK);

        schway__spu_event(loaded, loaded->spu.next_tick);
        CHECK_EQ_U64(loaded->audio_count, 1u);
        CHECK_EQ_U64(schway_load_state(loaded, state, state_size), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(loaded->audio_count, 0u);

        uint64_t compared_pairs = 0u;
        uint64_t pcm_energy = 0u;
        int16_t source_pcm[2u * 887u];
        int16_t loaded_pcm[2u * 887u];
        for (uint32_t frame = 0u; frame < 90u; frame++) {
            schway_run_result source_result = run_frame(source);
            schway_run_result loaded_result = run_frame(loaded);
            CHECK_EQ_U64(source_result.stop, SCHWAY_STOP_FRAME);
            CHECK_EQ_U64(loaded_result.stop, SCHWAY_STOP_FRAME);
            CHECK_EQ_U64(source_result.ticks, loaded_result.ticks);
            schway_audio source_audio = drain(source, AUDIO_RING_PAIRS);
            memcpy(source_pcm, samples, (size_t)source_audio.written * 2u * sizeof(int16_t));
            schway_audio loaded_audio = drain(loaded, AUDIO_RING_PAIRS);
            memcpy(loaded_pcm, samples, (size_t)loaded_audio.written * 2u * sizeof(int16_t));
            CHECK_EQ_U64(source_audio.written, loaded_audio.written);
            CHECK_EQ_U64(source_audio.dropped, 0u);
            CHECK_EQ_U64(loaded_audio.dropped, 0u);
            CHECK(memcmp(source_pcm, loaded_pcm,
                         (size_t)source_audio.written * 2u * sizeof(int16_t)) == 0);
            compared_pairs += source_audio.written;
            for (uint32_t i = 0u; i < 2u * source_audio.written; i++) {
                int32_t sample = source_pcm[i];
                pcm_energy += (uint64_t)(sample < 0 ? -sample : sample);
            }
        }
        CHECK(compared_pairs > 0u);
        CHECK(pcm_energy > 0u);
        free(state);
    }
    schway_destroy(loaded);
    schway_destroy(source);
}

static void check_region_cadence(uint32_t region, uint64_t frame_num, uint64_t frame_den,
                                uint32_t fps_num, uint32_t fps_den, uint32_t max_pairs,
                                uint32_t short_pairs)
{
    schway *inst = make(region);
    if (inst == NULL) {
        return;
    }
    schway_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    CHECK_EQ_U64(schway_get_info(inst, &info), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(info.region, region);
    CHECK_EQ_U64(info.fps_num, fps_num);
    CHECK_EQ_U64(info.fps_den, fps_den);
    CHECK_EQ_U64(info.sample_rate_num, 44100u);
    CHECK_EQ_U64(info.sample_rate_den, 1u);
    CHECK_EQ_U64(info.max_audio_frames, max_pairs);

    uint64_t previous_ticks = 0u;
    uint64_t total_pairs = 0u;
    uint32_t seen_short = 0u;
    uint32_t seen_long = 0u;
    uint32_t bad_samples = 0u;
    for (uint64_t frame = 1u; frame <= 240u; frame++) {
        schway_run_result result = run_frame(inst);
        uint64_t expected_ticks = frame * frame_num / frame_den;
        uint64_t expected_pairs = expected_ticks / 768u - previous_ticks / 768u;
        CHECK_EQ_U64(result.ticks, expected_ticks);
        schway_audio audio = drain(inst, AUDIO_RING_PAIRS);
        CHECK_EQ_U64(audio.written, expected_pairs);
        CHECK_EQ_U64(audio.dropped, 0u);
        CHECK(audio.written <= info.max_audio_frames);
        CHECK(audio.written == short_pairs || audio.written == short_pairs + 1u);
        seen_short += audio.written == short_pairs ? 1u : 0u;
        seen_long += audio.written == short_pairs + 1u ? 1u : 0u;
        for (uint32_t i = 0u; i < 2u * audio.written; i++) {
            bad_samples += samples[i] != 0 ? 1u : 0u;
        }
        total_pairs += audio.written;
        previous_ticks = result.ticks;
    }
    CHECK(seen_short > 0u);
    CHECK(seen_long > 0u);
    CHECK_EQ_U64(bad_samples, 0u);
    CHECK_EQ_U64(total_pairs, previous_ticks / 768u);
    schway_destroy(inst);
}

static void check_split_run_boundaries(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    if (inst == NULL) {
        return;
    }
    schway_run_result result = run_cycles(inst, 0u);
    CHECK_EQ_U64(result.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(result.ticks, 0u);
    schway_audio audio = drain(inst, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.written, 0u);

    result = run_cycles(inst, 767u);
    CHECK_EQ_U64(result.ticks, 767u);
    audio = drain(inst, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.written, 0u);

    result = run_cycles(inst, 1u);
    CHECK_EQ_U64(result.ticks, 768u);
    audio = drain(inst, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.written, 1u);

    result = run_cycles(inst, 768u);
    CHECK_EQ_U64(result.ticks, 1536u);
    audio = drain(inst, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.written, 1u);

    result = run_cycles(inst, 769u);
    CHECK_EQ_U64(result.ticks, 2305u);
    audio = drain(inst, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.written, 1u);
    CHECK_EQ_U64(audio.dropped, 0u);
    schway_destroy(inst);
}

static void check_instance_owns_spu_ram(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    if (inst == NULL) {
        return;
    }
    CHECK_EQ_U64(sizeof inst->spu.ram, 0x80000u);
    schway_destroy(inst);
}

static void check_spu_event_entrypoint(void)
{
    schway *first = make(SCHWAY_REGION_NTSC_U);
    schway *second = make(SCHWAY_REGION_NTSC_U);
    if (first == NULL || second == NULL) {
        schway_destroy(first);
        schway_destroy(second);
        return;
    }
    CHECK_EQ_U64(schway__spu_next(first), SCHWAY__CYCLES_PER_PAIR);
    CHECK_EQ_U64(drain(first, 1u).written, 0u);
    first->ticks = SCHWAY__CYCLES_PER_PAIR;
    schway__spu_event(first, first->ticks);
    schway_audio audio = drain(first, 1u);
    CHECK_EQ_U64(audio.written, 1u);
    CHECK_EQ_U64(samples[0], 0u);
    CHECK_EQ_U64(samples[1], 0u);
    CHECK_EQ_U64(schway__spu_next(first), 2u * SCHWAY__CYCLES_PER_PAIR);

    CHECK_EQ_U64(schway__spu_next(second), SCHWAY__CYCLES_PER_PAIR);
    CHECK_EQ_U64(drain(second, 1u).written, 0u);
    second->ticks = SCHWAY__CYCLES_PER_PAIR;
    schway__spu_event(second, second->ticks);
    CHECK_EQ_U64(drain(second, 1u).written, 1u);

    for (uint32_t i = 0u; i < AUDIO_RING_PAIRS + 2u; i++) {
        uint64_t deadline = schway__spu_next(first);
        first->ticks = deadline;
        schway__spu_event(first, deadline);
    }
    audio = drain(first, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.written, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.dropped, 2u);
    schway_destroy(first);
    schway_destroy(second);
}

static void check_bus_width_and_control_tick(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    if (inst == NULL) {
        return;
    }

    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, 0xBF801C00u, 1u, 0x12u, &cycles), 0u);
    cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, 0xBF801C01u, 1u, 0x34u, &cycles), 0u);
    CHECK_EQ_U64(inst->spu.voice[0].reg[0], 0u);
    inst->ticks = SCHWAY__CYCLES_PER_PAIR;
    schway__spu_event(inst, inst->ticks);
    CHECK_EQ_U64(inst->spu.voice[0].reg[0], 0x12u);

    cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, 0xBF801C00u, 4u, 0x56781234u, &cycles), 0u);
    inst->ticks = 2u * SCHWAY__CYCLES_PER_PAIR;
    schway__spu_event(inst, inst->ticks);
    CHECK_EQ_U64(inst->spu.voice[0].reg[0], 0x1234u);
    CHECK_EQ_U64(inst->spu.voice[0].reg[1], 0x5678u);

    cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, 0xBF801DAAu, 2u, 0x0021u, &cycles), 0u);
    uint32_t status = UINT32_MAX;
    cycles = 0u;
    CHECK_EQ_U64(schway__bus_read(inst, 0xBF801DAEu, 2u, &status, &cycles), 0u);
    CHECK_EQ_U64(status & 0x3Fu, 0u);
    inst->ticks = 3u * SCHWAY__CYCLES_PER_PAIR;
    schway__spu_event(inst, inst->ticks);
    status = UINT32_MAX;
    cycles = 0u;
    CHECK_EQ_U64(schway__bus_read(inst, 0xBF801DAEu, 2u, &status, &cycles), 0u);
    CHECK_EQ_U64(status & 0x3Fu, 0x21u);

    cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, 0xBF801DAAu, 2u, 0x8040u, &cycles), 0u);
    inst->ticks = 4u * SCHWAY__CYCLES_PER_PAIR;
    schway__spu_event(inst, inst->ticks);
    CHECK_EQ_U64(inst->spu.irq_enabled, 1u);
    (void)schway__spu_ram_read(inst, 0u);
    CHECK((inst->irq.stat & (1u << 9)) != 0u);

    cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, 0xBF801DAAu, 2u, 0u, &cycles), 0u);
    inst->ticks = 5u * SCHWAY__CYCLES_PER_PAIR;
    schway__spu_event(inst, inst->ticks);
    CHECK_EQ_U64(inst->spu.irq_enabled, 0u);
    CHECK_EQ_U64(inst->irq.stat & (1u << 9), 0u);
    CHECK_EQ_U64(inst->bus.unbuilt_n[SCHWAY__DEV_SPU], 0u);
    schway_destroy(inst);
}

static void check_register_peek_is_pure(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    if (inst == NULL) {
        return;
    }
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, 0xBF801DAAu, 2u, 0x8041u, &cycles), 0u);
    inst->ticks = SCHWAY__CYCLES_PER_PAIR;
    schway__spu_event(inst, inst->ticks);
    (void)schway__spu_ram_read(inst, 0u);

    uint64_t next_tick = inst->spu.next_tick;
    uint32_t pending_count = inst->spu.pending_count;
    uint32_t transfer_head = inst->spu.transfer_head;
    uint32_t transfer_count = inst->spu.transfer_count;
    uint32_t irq_stat = inst->irq.stat;
    CHECK_EQ_U64(schway__spu_reg_peek(inst, 0x1F801DAAu), 0x8041u);
    CHECK_EQ_U64(schway__spu_reg_peek(inst, 0x1F801DAEu) & 0x007Fu, 0x0041u);
    CHECK_EQ_U64(inst->spu.next_tick, next_tick);
    CHECK_EQ_U64(inst->spu.pending_count, pending_count);
    CHECK_EQ_U64(inst->spu.transfer_head, transfer_head);
    CHECK_EQ_U64(inst->spu.transfer_count, transfer_count);
    CHECK_EQ_U64(inst->irq.stat, irq_stat);
    schway_destroy(inst);
}

static void spu_write16(schway *inst, uint32_t address, uint16_t value)
{
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, 0xBF800000u | address, 2u, value, &cycles), 0u);
}

static uint16_t spu_read16(schway *inst, uint32_t address)
{
    uint32_t value = UINT32_MAX;
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_read(inst, 0xBF800000u | address, 2u, &value, &cycles), 0u);
    return (uint16_t)value;
}

static void spu_next_tick(schway *inst)
{
    uint64_t deadline = schway__spu_next(inst);
    inst->cpu.blk_tick = deadline;
    inst->cpu.blk_cycle = inst->cpu.cycle;
    inst->ticks = deadline;
    schway__spu_event(inst, deadline);
}

static void dma_reg_write(schway *inst, uint32_t address, uint32_t value)
{
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, 0xBF800000u | address, 4u, value, &cycles), 0u);
}

static void check_dma_fifo_ram_round_trip(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    if (inst == NULL) {
        return;
    }

    for (uint32_t i = 0u; i < 64u; i++) {
        inst->bus.ram[0x2000u + i] = (uint8_t)(i * 3u + 1u);
    }
    spu_write16(inst, 0x1F801DA6u, 0x20u);
    spu_write16(inst, 0x1F801DAAu, 0x0020u);
    spu_next_tick(inst);
    dma_reg_write(inst, 0x1F8010F0u, 0x0FEDCBA9u);
    dma_reg_write(inst, 0x1F8010F4u, 0x00900000u);
    dma_reg_write(inst, 0x1F8010C0u, 0x2000u);
    dma_reg_write(inst, 0x1F8010C4u, 0x00010010u);
    dma_reg_write(inst, 0x1F8010C8u, 0x01000201u);
    CHECK_EQ_U64(inst->dma.cur, 4u);
    for (uint32_t i = 0u; i < 16u; i++) {
        uint32_t source = 0u;
        CHECK_EQ_U64(schway__bus_ram_word(inst, 0x2000u + 4u * i, &source, 0u), 0u);
        CHECK_EQ_HEX(schway__spu_ram_peek(inst, 0x100u + 4u * i), (uint16_t)source);
        CHECK_EQ_HEX(schway__spu_ram_peek(inst, 0x102u + 4u * i), (uint16_t)(source >> 16));
    }
    schway__dma_event(inst, inst->dma.busy_until);
    CHECK_EQ_U64(inst->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY, 0u);
    CHECK((inst->irq.stat & (1u << 3)) != 0u);

    spu_write16(inst, 0x1F801DA6u, 0x20u);
    spu_write16(inst, 0x1F801DAAu, 0x0030u);
    spu_next_tick(inst);
    CHECK_EQ_HEX(schway__spu_reg_peek(inst, 0x1F801DAAu), 0x0030u);
    CHECK_EQ_U64(schway__spu_dma_ready(inst, 0u), 1u);
    dma_reg_write(inst, 0x1F801014u, 0x220931E1u);
    dma_reg_write(inst, 0x1F8010F4u, 0x00900000u);
    dma_reg_write(inst, 0x1F8010C0u, 0x4000u);
    dma_reg_write(inst, 0x1F8010C4u, 0x00010010u);
    dma_reg_write(inst, 0x1F8010C8u, 0x01000200u);
    CHECK_EQ_HEX(inst->dma.chcr[4], 0x01000200u);
    CHECK_EQ_U64(schway__dma_arbitrate(inst, 1u << 4), 4u);
    CHECK_EQ_U64(inst->dma.cur, 4u);
    for (uint32_t i = 0u; i < 16u; i++) {
        uint32_t source = 0u;
        uint32_t copied = 0u;
        CHECK_EQ_U64(schway__bus_ram_word(inst, 0x2000u + 4u * i, &source, 0u), 0u);
        CHECK_EQ_U64(schway__bus_ram_word(inst, 0x4000u + 4u * i, &copied, 0u), 0u);
        CHECK_EQ_HEX(copied, source);
    }
    schway__dma_event(inst, inst->dma.busy_until);
    CHECK_EQ_U64(inst->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY, 0u);
    schway_destroy(inst);
}

/* Fault prevented: DEV4's documented DMA-read insertion being ignored, or
   the workaround value being read from anything other than memory-control
   state. The SPU emits FFFFh at each 32-halfword boundary unless DEV4_CTRL
   is set to 220931E1h (PSX-SPX, SPU transfers; preparation §1.5). */
static void check_dma_read_dev4_control(void)
{
    const uint32_t base = 0x400u;
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    if (inst == NULL) {
        return;
    }

    for (uint32_t i = 0u; i < 64u; i++) {
        schway__spu_ram_write(inst, base + 2u * i, (uint16_t)(0x1200u + i));
    }
    spu_write16(inst, 0x1F801DA6u, (uint16_t)(base >> 3));
    spu_write16(inst, 0x1F801DAAu, 0x0030u);
    spu_next_tick(inst);

    dma_reg_write(inst, 0x1F801014u, 0x220931E1u);
    for (uint32_t i = 0u; i < 64u; i++) {
        CHECK_EQ_HEX(schway__spu_dma_read(inst), 0x1200u + i);
    }
    CHECK_EQ_HEX(inst->spu.transfer_address, base + 128u);

    spu_write16(inst, 0x1F801DA6u, (uint16_t)(base >> 3));
    spu_next_tick(inst);
    dma_reg_write(inst, 0x1F801014u, 0u);
    for (uint32_t i = 0u; i < 64u; i++) {
        uint16_t expected = (i % 32u) == 0u ? 0xFFFFu : (uint16_t)(0x1200u + i);
        CHECK_EQ_HEX(schway__spu_dma_read(inst), expected);
    }
    CHECK_EQ_HEX(inst->spu.transfer_address, base + 128u);
    schway_destroy(inst);
}

static void check_spu_enable_gates_irq(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    if (inst == NULL) {
        return;
    }

    schway__spu_ram_write(inst, 0u, 0xCDEFu);
    spu_write16(inst, 0x1F801DA4u, 0u);
    spu_write16(inst, 0x1F801DAAu, 0x0040u);
    spu_next_tick(inst);
    (void)schway__spu_ram_read(inst, 0u);
    CHECK_EQ_U64(inst->spu.irq_enabled, 0u);
    CHECK_EQ_U64(inst->irq.stat & (1u << 9), 0u);

    spu_write16(inst, 0x1F801DAAu, 0x8040u);
    spu_next_tick(inst);
    (void)schway__spu_ram_read(inst, 0u);
    CHECK_EQ_U64(inst->spu.irq_enabled, 1u);
    CHECK((inst->irq.stat & (1u << 9)) != 0u);

    spu_write16(inst, 0x1F801DAAu, 0x8000u);
    spu_next_tick(inst);
    CHECK_EQ_U64(inst->irq.stat & (1u << 9), 0u);
    CHECK_EQ_U64(schway__spu_ram_peek(inst, 0u), 0xCDEFu);
    schway_destroy(inst);
}

static void check_manual_fifo_and_ram_transfer(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    if (inst == NULL) {
        return;
    }

    for (uint32_t i = 0u; i < SCHWAY__SPU_FIFO_HALFWORDS; i++) {
        spu_write16(inst, 0x1F801DA8u, (uint16_t)(0x1000u + i));
    }
    spu_write16(inst, 0x1F801DA8u, 0xEEEEu);
    CHECK_EQ_U64(inst->spu.transfer_count, SCHWAY__SPU_FIFO_HALFWORDS);
    CHECK_EQ_U64(schway__spu_reg_peek(inst, 0x1F801DA8u), 0x1000u);
    CHECK_EQ_U64(inst->spu.transfer_count, SCHWAY__SPU_FIFO_HALFWORDS);
    CHECK_EQ_U64(spu_read16(inst, 0x1F801DA8u), 0x1000u);
    for (uint32_t i = 1u; i < SCHWAY__SPU_FIFO_HALFWORDS; i++) {
        CHECK_EQ_U64(spu_read16(inst, 0x1F801DA8u), 0x1000u + i);
    }
    CHECK_EQ_U64(inst->spu.transfer_count, 0u);
    CHECK_EQ_U64(spu_read16(inst, 0x1F801DA8u), 0u);

    spu_write16(inst, 0x1F801DACu, 4u);
    spu_write16(inst, 0x1F801DA6u, 0xFFFFu);
    spu_write16(inst, 0x1F801DAAu, 0x8000u);
    spu_next_tick(inst);
    for (uint32_t i = 0u; i < SCHWAY__SPU_FIFO_HALFWORDS; i++) {
        spu_write16(inst, 0x1F801DA8u, (uint16_t)(0xA000u + i));
    }
    spu_write16(inst, 0x1F801DAAu, 0x8010u);
    spu_next_tick(inst);
    CHECK_EQ_U64(schway__spu_reg_peek(inst, 0x1F801DAEu) & 0x0400u, 0u);
    CHECK_EQ_U64(inst->spu.transfer_count, 0u);
    CHECK_EQ_U64(schway__spu_reg_peek(inst, 0x1F801DAEu) & 0x0400u, 0u);
    CHECK_EQ_U64(schway__spu_ram_peek(inst, 0x7FFF8u), 0xA000u);
    CHECK_EQ_U64(schway__spu_ram_peek(inst, 0x7FFFEu), 0xA003u);
    CHECK_EQ_U64(schway__spu_ram_peek(inst, 0u), 0xA004u);
    CHECK_EQ_U64(schway__spu_ram_peek(inst, 0x0036u), 0xA01Fu);

    const uint16_t ram_ctrl[] = {4u, 6u, 8u, 10u};
    const uint32_t copies[] = {1u, 2u, 4u, 8u};
    for (uint32_t mode = 0u; mode < 4u; mode++) {
        uint32_t base = 0x10000u + mode * 0x100u;
        spu_write16(inst, 0x1F801DAAu, 0x8000u);
        spu_write16(inst, 0x1F801DACu, ram_ctrl[mode]);
        spu_write16(inst, 0x1F801DA6u, (uint16_t)(base >> 3));
        spu_next_tick(inst);
        spu_write16(inst, 0x1F801DA8u, (uint16_t)(0xB000u + mode));
        spu_write16(inst, 0x1F801DAAu, 0x8010u);
        spu_next_tick(inst);
        for (uint32_t copy = 0u; copy < copies[mode]; copy++) {
            CHECK_EQ_U64(schway__spu_ram_peek(inst, base + copy * 2u), 0xB000u + mode);
        }
        CHECK_EQ_U64(schway__spu_ram_peek(inst, base + copies[mode] * 2u), 0u);
    }

    spu_write16(inst, 0x1F801DAAu, 0x8020u);
    spu_next_tick(inst);
    CHECK((schway__spu_reg_peek(inst, 0x1F801DAEu) & 0x0100u) != 0u);
    for (uint32_t i = 0u; i < SCHWAY__SPU_FIFO_HALFWORDS; i++) {
        spu_write16(inst, 0x1F801DA8u, (uint16_t)(0xC000u + i));
    }
    CHECK_EQ_U64(inst->spu.transfer_count, SCHWAY__SPU_FIFO_HALFWORDS);
    CHECK_EQ_U64(schway__spu_reg_peek(inst, 0x1F801DAEu) & 0x0100u, 0u);
    spu_write16(inst, 0x1F801DAAu, 0x8030u);
    spu_next_tick(inst);
    CHECK((schway__spu_reg_peek(inst, 0x1F801DAEu) & 0x0200u) != 0u);
    schway_destroy(inst);
}

static void check_capture_gate_status_and_irqa(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    if (inst == NULL) {
        return;
    }

    schway__spu_ram_write(inst, 0u, 0x1234u);
    schway__spu_ram_write(inst, 0x400u, 0x2345u);
    schway__spu_ram_write(inst, 0x800u, 0x3456u);
    schway__spu_ram_write(inst, 0xC00u, 0x4567u);
    spu_write16(inst, 0x1F801DA4u, 0u);
    spu_write16(inst, 0x1F801DAAu, 0x8040u);
    spu_write16(inst, 0x1F801DACu, 0u);
    spu_next_tick(inst);
    CHECK_EQ_U64(schway__spu_ram_peek(inst, 0u), 0x1234u);
    CHECK_EQ_U64(inst->spu.capture_cursor, 0u);
    CHECK_EQ_U64(schway__spu_reg_peek(inst, 0x1F801DAEu) & 0x0800u, 0u);

    spu_write16(inst, 0x1F801DACu, 4u);
    spu_next_tick(inst);
    CHECK_EQ_U64(schway__spu_ram_peek(inst, 0u), 0u);
    CHECK_EQ_U64(schway__spu_ram_peek(inst, 0x400u), 0u);
    CHECK_EQ_U64(schway__spu_ram_peek(inst, 0x800u), 0u);
    CHECK_EQ_U64(schway__spu_ram_peek(inst, 0xC00u), 0u);
    CHECK((inst->irq.stat & (1u << 9)) != 0u);
    for (uint32_t i = 1u; i < 256u; i++) {
        spu_next_tick(inst);
    }
    CHECK_EQ_U64(inst->spu.capture_cursor, 0x100u);
    CHECK((schway__spu_reg_peek(inst, 0x1F801DAEu) & 0x0800u) != 0u);

    spu_write16(inst, 0x1F801DAAu, 0x8000u);
    spu_next_tick(inst);
    CHECK_EQ_U64(inst->irq.stat & (1u << 9), 0u);
    CHECK_EQ_U64(schway__spu_ram_peek(inst, 0x400u), 0u);
    schway_destroy(inst);
}

static uint64_t next_sample_deadline(uint64_t ticks)
{
    return (ticks / SCHWAY__CYCLES_PER_PAIR + 1u) * SCHWAY__CYCLES_PER_PAIR;
}

static void check_scheduled_split_runs(void)
{
    schway *whole = make(SCHWAY_REGION_NTSC_U);
    schway *split = make(SCHWAY_REGION_NTSC_U);
    if (whole == NULL || split == NULL) {
        schway_destroy(whole);
        schway_destroy(split);
        return;
    }
    schway_run_result result = run_cycles(whole, 1536u);
    CHECK_EQ_U64(result.ticks, 1536u);
    CHECK_EQ_U64(schway__spu_next(whole), 3u * SCHWAY__CYCLES_PER_PAIR);
    schway_audio audio = drain(whole, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.written, 2u);
    int16_t whole_pcm[4] = {samples[0], samples[1], samples[2], samples[3]};

    result = run_cycles(split, SCHWAY__CYCLES_PER_PAIR);
    CHECK_EQ_U64(result.ticks, SCHWAY__CYCLES_PER_PAIR);
    CHECK_EQ_U64(schway__spu_next(split), 2u * SCHWAY__CYCLES_PER_PAIR);
    result = run_cycles(split, SCHWAY__CYCLES_PER_PAIR);
    CHECK_EQ_U64(result.ticks, 2u * SCHWAY__CYCLES_PER_PAIR);
    CHECK_EQ_U64(schway__spu_next(split), 3u * SCHWAY__CYCLES_PER_PAIR);
    audio = drain(split, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.written, 2u);
    CHECK(memcmp(whole_pcm, samples, sizeof whole_pcm) == 0);
    schway_destroy(whole);
    schway_destroy(split);
}

static void check_cpu_and_dma_deadlines(void)
{
    schway *cpu = make(SCHWAY_REGION_NTSC_U);
    if (cpu == NULL) {
        return;
    }
    cpu->exe_loaded = 1u;
    cpu->cpu.pc = 0x80000000u;
    cpu->ticks = SCHWAY__CYCLES_PER_PAIR - 1u;
    schway__sched_sync(cpu);
    schway_run_result result = run_cycles(cpu, 1u);
    CHECK(result.ticks >= SCHWAY__CYCLES_PER_PAIR);
    CHECK_EQ_U64(schway__spu_next(cpu), next_sample_deadline(result.ticks));
    schway_audio audio = drain(cpu, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.written, result.ticks / SCHWAY__CYCLES_PER_PAIR);
    schway_destroy(cpu);

    schway *dma = make(SCHWAY_REGION_NTSC_U);
    if (dma == NULL) {
        return;
    }
    dma->exe_loaded = 1u;
    dma->ticks = SCHWAY__CYCLES_PER_PAIR - 1u;
    dma->dma.stall_until = SCHWAY__CYCLES_PER_PAIR + 2u;
    schway__sched_sync(dma);
    result = run_cycles(dma, 1u);
    CHECK_EQ_U64(result.ticks, SCHWAY__CYCLES_PER_PAIR);
    CHECK_EQ_U64(schway__spu_next(dma), 2u * SCHWAY__CYCLES_PER_PAIR);
    audio = drain(dma, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.written, 1u);
    schway_destroy(dma);
}

static void check_disabled_and_muted_deadlines(void)
{
    const uint16_t attr[] = {0u, 0x8000u};
    for (uint32_t i = 0u; i < sizeof attr / sizeof attr[0]; i++) {
        schway *inst = make(SCHWAY_REGION_NTSC_U);
        if (inst == NULL) {
            return;
        }
        /* ATTR at 1F801DAA: disabled, then enabled with UNMUTE clear. */
        inst->spu.reg[0xDDu] = attr[i];
        schway_run_result result = run_cycles(inst, SCHWAY__CYCLES_PER_PAIR);
        CHECK_EQ_U64(result.ticks, SCHWAY__CYCLES_PER_PAIR);
        CHECK_EQ_U64(schway__spu_next(inst), 2u * SCHWAY__CYCLES_PER_PAIR);
        schway_audio audio = drain(inst, AUDIO_RING_PAIRS);
        CHECK_EQ_U64(audio.written, 1u);
        schway_destroy(inst);
    }
}

static void check_ring_partial_wrap_and_drops(void)
{
    schway *inst = make(SCHWAY_REGION_PAL);
    if (inst == NULL) {
        return;
    }
    schway_run_result result;
    for (uint32_t i = 0u; i < 5u; i++) {
        result = run_frame(inst);
    }
    uint64_t due = result.ticks / 768u;
    CHECK(due > AUDIO_RING_PAIRS);
    schway_audio audio = drain(inst, 100u);
    CHECK_EQ_U64(audio.written, 100u);
    CHECK_EQ_U64(audio.dropped, due - AUDIO_RING_PAIRS);

    uint64_t previous_ticks = result.ticks;
    result = run_frame(inst);
    uint64_t new_pairs = result.ticks / 768u - previous_ticks / 768u;
    CHECK(new_pairs > 100u);
    audio = drain(inst, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.written, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.dropped, new_pairs - 100u);
    audio = drain(inst, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.written, 0u);
    CHECK_EQ_U64(audio.dropped, 0u);
    schway_destroy(inst);
}

static void check_reset_and_load_clear_drain(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_J);
    if (inst == NULL) {
        return;
    }
    (void)run_frame(inst);
    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    schway_audio audio = drain(inst, AUDIO_RING_PAIRS);
    CHECK_EQ_U64(audio.written, 0u);
    CHECK_EQ_U64(audio.dropped, 0u);

    (void)run_frame(inst);
    size_t state_size = schway_state_size(inst);
    uint8_t *state = state_size == 0u ? NULL : (uint8_t *)malloc(state_size);
    CHECK(state != NULL);
    if (state != NULL) {
        CHECK_EQ_U64(schway_save_state(inst, state, state_size), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(schway_load_state(inst, state, state_size), SCHWAY_STATUS_OK);
        audio = drain(inst, AUDIO_RING_PAIRS);
        CHECK_EQ_U64(audio.written, 0u);
        CHECK_EQ_U64(audio.dropped, 0u);
        free(state);
    }
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    /* These paths are wired now for later fixture-backed cases; no fixture is
       required by this public frame-to-drain tracer. */
    if (argc != 3 || argv[1][0] == '\0' || argv[2][0] == '\0') {
        return 2;
    }
    check_tone_fixture(argv[1]);
    check_tone_state_continuation(argv[1]);
    check_region_cadence(SCHWAY_REGION_NTSC_J, NTSC_FRAME_NUM, NTSC_FRAME_DEN,
                         53693175u, 897619u, 738u, 737u);
    check_region_cadence(SCHWAY_REGION_NTSC_U, NTSC_FRAME_NUM, NTSC_FRAME_DEN,
                         53693175u, 897619u, 738u, 737u);
    check_region_cadence(SCHWAY_REGION_PAL, PAL_FRAME_NUM, PAL_FRAME_DEN,
                         53203425u, 1069484u, 887u, 886u);
    check_split_run_boundaries();
    check_instance_owns_spu_ram();
    check_spu_event_entrypoint();
    check_bus_width_and_control_tick();
    check_register_peek_is_pure();
    check_spu_enable_gates_irq();
    check_manual_fifo_and_ram_transfer();
    check_dma_fifo_ram_round_trip();
    check_dma_read_dev4_control();
    check_capture_gate_status_and_irqa();
    check_scheduled_split_runs();
    check_cpu_and_dma_deadlines();
    check_disabled_and_muted_deadlines();
    check_ring_partial_wrap_and_drops();
    check_reset_and_load_clear_drain();
    CHECK_DONE();
}
