/* Pinned SPU noise clock and capture checks. The noise schedule follows
   psx-spx commit 3717718f642ce2ca9ff590b65918fb3a46279cc4. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/internal.h"

#define SPU_REGISTER_BASE UINT32_C(0x1F801C00)
#define SPU_ATTR UINT32_C(0x1F801DAA)
#define SPU_NON UINT32_C(0x1F801D94)
#define SPU_RAM_CTRL UINT32_C(0x1F801DAC)
#define SPU_IRQA UINT32_C(0x1F801DA4)
#define SPU_MAIN_VOL_L UINT32_C(0x1F801D80)
#define SPU_MAIN_VOL_R UINT32_C(0x1F801D82)
#define IRQ9_MASK (UINT32_C(1) << 9)
#define TEST_UNDECODED (UINT32_C(31) << 27)

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
    profile.region = SCHWAY_REGION_NTSC_U;
    if (schway_set_profile(inst, &profile) != SCHWAY_STATUS_OK) {
        schway_destroy(inst);
        return NULL;
    }
    return inst;
}

static void run_sample(schway *inst)
{
    uint64_t deadline = schway__spu_next(inst);
    inst->ticks = deadline;
    schway__spu_event(inst, deadline);
}

static uint16_t lfsr_clock(uint16_t value)
{
    uint16_t parity = (uint16_t)(((value >> 15) ^ (value >> 12) ^ (value >> 11) ^
                                  (value >> 10) ^ 1u) & 1u);
    return (uint16_t)((uint32_t)value * 2u + parity);
}

static uint32_t reg_index(uint32_t address)
{
    return (address - SPU_REGISTER_BASE) >> 1;
}

static void reset_noise(schway *inst, uint32_t shift, uint32_t step)
{
    inst->spu.noise_phase = 0u;
    inst->spu.noise_lfsr = 1u;
    inst->spu.reg[reg_index(SPU_ATTR)] =
        (uint16_t)(0x8000u | (shift << 10) | (step << 8));
}

static uint32_t test_first_slot_matrix(schway *inst)
{
    uint32_t failures = 0u;
    for (uint32_t shift = 0u; shift < 16u; shift++) {
        for (uint32_t step = 0u; step < 4u; step++) {
            reset_noise(inst, shift, step);
            run_sample(inst);
            failures += inst->spu.noise_lfsr != 3u ? 1u : 0u;
        }
    }
    return failures;
}

static uint32_t test_slot_masks(schway *inst)
{
    static const uint8_t masks[4] = {0x55u, 0x57u, 0x77u, 0x7Fu};
    const uint32_t period = 0x20000u;
    uint32_t failures = 0u;
    for (uint32_t step = 0u; step < 4u; step++) {
        for (uint32_t slot = 0u; slot < 8u; slot++) {
            reset_noise(inst, 0u, step);
            inst->spu.noise_phase = (slot * period) / 8u;
            run_sample(inst);
            uint16_t expected = (masks[step] & (uint8_t)(1u << slot)) != 0u ? 3u : 1u;
            failures += inst->spu.noise_lfsr != expected ? 1u : 0u;
        }
    }
    return failures;
}

static uint32_t test_long_gap_and_continuity(schway *inst)
{
    uint32_t failures = 0u;
    reset_noise(inst, 0u, 0u);
    run_sample(inst);
    inst->spu.noise_phase = 0x20000u / 4u - 1u;
    run_sample(inst);
    failures += inst->spu.noise_lfsr != 3u ? 1u : 0u;
    run_sample(inst);
    failures += inst->spu.noise_lfsr != 7u ? 1u : 0u;

    reset_noise(inst, 15u, 0u);
    run_sample(inst);
    inst->spu.reg[reg_index(SPU_ATTR)] &= 0x7FFFu;
    run_sample(inst);
    failures += inst->spu.noise_lfsr != 7u ? 1u : 0u;
    inst->spu.reg[reg_index(SPU_ATTR)] |= 0x8000u;
    run_sample(inst);
    failures += inst->spu.noise_lfsr != 15u ? 1u : 0u;
    return failures;
}

static uint32_t test_lfsr_cycle(schway *inst)
{
    uint32_t failures = 0u;
    reset_noise(inst, 15u, 0u);
    uint16_t expected = inst->spu.noise_lfsr;
    for (uint32_t i = 0u; i < 65535u; i++) {
        expected = lfsr_clock(expected);
        run_sample(inst);
        failures += inst->spu.noise_lfsr == UINT16_MAX ? 1u : 0u;
        failures += inst->spu.noise_lfsr != expected ? 1u : 0u;
    }
    failures += expected != 1u ? 1u : 0u;
    failures += inst->spu.noise_lfsr != expected ? 1u : 0u;
    return failures;
}

static uint32_t test_noise_voice_selection(schway *inst)
{
    uint32_t failures = 0u;
    inst->audio_head = 0u;
    inst->audio_count = 0u;
    inst->audio_dropped = 0u;
    reset_noise(inst, 15u, 0u);
    inst->spu.noise_lfsr = 0x2000u;
    inst->spu.reg[reg_index(SPU_ATTR)] |= 0x4000u;
    inst->spu.reg[reg_index(SPU_NON)] = 0x0005u;
    inst->spu.reg[reg_index(SPU_MAIN_VOL_L)] = 0x3FFFu;
    inst->spu.reg[reg_index(SPU_MAIN_VOL_R)] = 0x3FFFu;
    for (uint32_t i = 0u; i < 3u; i += 2u) {
        inst->spu.voice[i].envelope = 0x4000;
        inst->spu.voice[i].reg[0] = 0x3FFFu;
        inst->spu.voice[i].reg[1] = 0x3FFFu;
    }
    run_sample(inst);
    failures += inst->spu.voice[0].output != 0x4001 ? 1u : 0u;
    failures += inst->spu.voice[2].output != 0x4001 ? 1u : 0u;
    int16_t samples[2] = {0, 0};
    schway_audio audio;
    memset(&audio, 0, sizeof audio);
    audio.size = (uint32_t)sizeof audio;
    audio.samples = samples;
    audio.capacity = 1u;
    failures += schway_get_audio(inst, &audio) != SCHWAY_STATUS_OK ? 1u : 0u;
    failures += audio.written != 1u || samples[0] <= 0 || samples[1] != samples[0] ? 1u : 0u;
    return failures;
}

static uint32_t test_noise_capture_and_wrap(schway *inst)
{
    uint32_t failures = 0u;
    reset_noise(inst, 15u, 0u);
    inst->spu.noise_lfsr = 0x2000u;
    inst->spu.reg[reg_index(SPU_RAM_CTRL)] = 4u;
    inst->spu.reg[reg_index(SPU_NON)] = 0x0005u;
    inst->spu.voice[0].envelope = 0x4000;
    inst->spu.voice[2].envelope = 0x6000;
    run_sample(inst);
    failures += schway__spu_ram_peek(inst, 0u) != 0u ? 1u : 0u;
    failures += schway__spu_ram_peek(inst, 0x400u) != 0u ? 1u : 0u;
    failures += schway__spu_ram_peek(inst, 0x800u) != 8192u ? 1u : 0u;
    failures += schway__spu_ram_peek(inst, 0xC00u) != 12288u ? 1u : 0u;

    for (uint32_t i = 1u; i < 256u; i++) {
        run_sample(inst);
    }
    failures += inst->spu.capture_cursor != 0x100u ? 1u : 0u;
    failures += (schway__spu_reg_peek(inst, 0x1F801DAEu) & 0x0800u) == 0u ? 1u : 0u;
    for (uint32_t i = 256u; i < 512u; i++) {
        run_sample(inst);
    }
    failures += inst->spu.capture_cursor != 0u ? 1u : 0u;
    failures += (schway__spu_reg_peek(inst, 0x1F801DAEu) & 0x0800u) != 0u ? 1u : 0u;
    return failures;
}

static void reset_voice_fetches(schway *inst, uint16_t address_units)
{
    for (uint32_t i = 0u; i < SCHWAY__SPU_VOICE_COUNT; i++) {
        memset(&inst->spu.voice[i], 0, sizeof inst->spu.voice[i]);
        inst->spu.voice[i].reg[3] = address_units;
        inst->spu.voice[i].sample_counter = TEST_UNDECODED;
    }
}

static uint32_t test_silent_capture_and_irqa(schway *inst)
{
    uint32_t failures = 0u;
    reset_noise(inst, 15u, 0u);
    inst->spu.noise_lfsr = 0x2000u;
    inst->spu.reg[reg_index(SPU_ATTR)] &= 0x7FFFu;
    inst->spu.reg[reg_index(SPU_NON)] = 0x0005u;
    inst->spu.reg[reg_index(SPU_RAM_CTRL)] = 4u;
    inst->spu.voice[0].envelope = 0x4000;
    inst->spu.voice[2].envelope = 0x6000;
    inst->spu.capture_cursor = 0u;
    inst->spu.irq_enabled = 0u;
    inst->irq.stat &= ~IRQ9_MASK;
    run_sample(inst);
    failures += inst->spu.noise_lfsr != 0x4001u ? 1u : 0u;
    failures += schway__spu_ram_peek(inst, 0x800u) != 8192u ? 1u : 0u;
    failures += schway__spu_ram_peek(inst, 0xC00u) != 12288u ? 1u : 0u;
    failures += (inst->irq.stat & IRQ9_MASK) != 0u ? 1u : 0u;

    /* Muted, enabled voices still fetch guest RAM; unrelated IRQA misses. */
    inst->spu.reg[reg_index(SPU_ATTR)] = 0x8040u;
    inst->spu.irq_enabled = 1u;
    inst->spu.reg[reg_index(SPU_RAM_CTRL)] = 0u;
    inst->spu.irqa_addr = 0x4000u;
    reset_voice_fetches(inst, 0x0800u);
    run_sample(inst);
    failures += (inst->irq.stat & IRQ9_MASK) == 0u ? 1u : 0u;
    inst->irq.stat &= ~IRQ9_MASK;
    inst->spu.irqa_addr = 0x5000u;
    reset_voice_fetches(inst, 0x0800u);
    run_sample(inst);
    failures += (inst->irq.stat & IRQ9_MASK) != 0u ? 1u : 0u;

    /* Capture writes use the same little-endian helper and can raise IRQA. */
    inst->spu.irqa_addr = 0u;
    inst->spu.reg[reg_index(SPU_RAM_CTRL)] = 4u;
    inst->spu.capture_cursor = 0u;
    reset_voice_fetches(inst, 0x0800u);
    run_sample(inst);
    failures += (inst->irq.stat & IRQ9_MASK) == 0u ? 1u : 0u;
    inst->irq.stat &= ~IRQ9_MASK;
    inst->spu.reg[reg_index(SPU_RAM_CTRL)] = 0u;
    run_sample(inst);
    failures += (inst->irq.stat & IRQ9_MASK) != 0u ? 1u : 0u;
    failures += inst->spu.capture_cursor != 1u ? 1u : 0u;

    /* ATTR bit 6 acknowledges, and the next capture access can refire. */
    schway__spu_reg_write(inst, SPU_ATTR, 0x8000u);
    run_sample(inst);
    failures += (inst->irq.stat & IRQ9_MASK) != 0u ? 1u : 0u;
    schway__spu_reg_write(inst, SPU_ATTR, 0x8040u);
    inst->spu.reg[reg_index(SPU_RAM_CTRL)] = 4u;
    inst->spu.irqa_addr = 0u;
    inst->spu.capture_cursor = 0u;
    run_sample(inst);
    failures += (inst->irq.stat & IRQ9_MASK) == 0u ? 1u : 0u;
    return failures;
}

int main(void)
{
    puts("TAP version 13");
    puts("1..7");
    schway *inst = make_instance();
    if (inst == NULL) {
        puts("not ok 1 - pinned noise schedule and capture behavior");
        return 1;
    }
    uint32_t failures[7];
    failures[0] = test_first_slot_matrix(inst);
    failures[1] = test_slot_masks(inst);
    failures[2] = test_long_gap_and_continuity(inst);
    failures[3] = test_lfsr_cycle(inst);
    failures[4] = test_noise_voice_selection(inst);
    failures[5] = test_noise_capture_and_wrap(inst);
    failures[6] = test_silent_capture_and_irqa(inst);
    static const char *names[7] = {
        "all shift and step combinations clock the first slot at most once per sample",
        "step values select exactly the pinned eight-slot schedule",
        "long-period gaps and ATTR disable preserve LFSR and phase continuity",
        "the LFSR avoids FFFF and completes its 65535-clock sequence",
        "noise voices share the signed level through envelope and amplitude mixing",
        "capture rings receive scaled noise, wrap, and report the correct half",
        "muted and disabled output retains capture, RAM_CTRL gating and IRQA behavior",
    };
    uint32_t total = 0u;
    for (uint32_t i = 0u; i < 7u; i++) {
        printf("%s %u - %s\n", failures[i] == 0u ? "ok" : "not ok", i + 1u, names[i]);
        total += failures[i];
    }
    schway_destroy(inst);
    return total == 0u ? 0 : 1;
}
