/* Bounded replay of hostile SPU registers, FIFO traffic and DMA words. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus.h"
#include "dma.h"
#include "internal.h"
#include "schwaystation.h"
#include "spu.h"

#define SPU_BASE 0x1F801C00u
#define SPU_TSA (SPU_BASE + 0x1A6u)
#define SPU_FIFO (SPU_BASE + 0x1A8u)
#define SPU_ATTR (SPU_BASE + 0x1AAu)
#define DMA4_MADR 0x1F8010C0u
#define DMA4_BCR 0x1F8010C4u
#define DMA4_CHCR 0x1F8010C8u
#define DMA_DPCR 0x1F8010F0u
#define INPUT_MAX 1024u
#define RECORD_SIZE 5u
#define RUN_MAX 1024u
#define CONTINUE_CYCLES (3u * 768u)

static uint32_t allocator_calls;

static void broken(const char *what)
{
    fprintf(stderr, "fuzz.regress.spu: %s\n", what);
    abort();
}

static void *alloc_bytes(void *user, size_t size)
{
    (void)user;
    allocator_calls++;
    return malloc(size);
}

static void free_bytes(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static schway *make(void)
{
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = alloc_bytes;
    cfg.allocator.free = free_bytes;
    schway *inst = NULL;
    if (schway_create(&cfg, &inst) != SCHWAY_STATUS_OK || inst == NULL) {
        broken("instance creation failed");
    }
    return inst;
}

static void write16(schway *inst, uint32_t address, uint16_t value)
{
    uint32_t cycles = 0u;
    if (schway__bus_write(inst, address, 2u, value, &cycles) != 0) {
        broken("mapped halfword write was refused");
    }
}

static uint16_t read16(schway *inst, uint32_t address)
{
    uint32_t cycles = 0u;
    uint32_t value = 0u;
    if (schway__bus_read(inst, address, 2u, &value, &cycles) != 0) {
        broken("mapped halfword read was refused");
    }
    return (uint16_t)value;
}

static void write32(schway *inst, uint32_t address, uint32_t value)
{
    uint32_t cycles = 0u;
    if (schway__bus_write(inst, address, 4u, value, &cycles) != 0) {
        broken("mapped word write was refused");
    }
}

static void run_bounded(schway *inst, uint64_t cycles)
{
    uint64_t before = inst->ticks;
    schway_budget budget;
    schway_run_result result;
    memset(&budget, 0, sizeof budget);
    memset(&result, 0, sizeof result);
    budget.size = (uint32_t)sizeof budget;
    budget.max_cycles = cycles;
    result.size = (uint32_t)sizeof result;
    if (schway_run(inst, &budget, &result) != SCHWAY_STATUS_OK ||
        (result.stop != SCHWAY_STOP_BUDGET && result.stop != SCHWAY_STOP_FRAME &&
         result.stop != SCHWAY_STOP_FAULT) ||
        result.ticks < before || result.ticks - before > cycles) {
        broken("public run exceeded its cycle budget or returned an invalid stop reason");
    }
}

static void dma_transfer(schway *inst, uint8_t selector, uint16_t value, uint8_t extra)
{
    uint32_t address = 0x80001000u + ((uint32_t)(selector & 7u) * 4u);
    uint32_t data = (uint32_t)value | ((uint32_t)(uint16_t)~value << 16);
    write32(inst, address, data);
    write16(inst, SPU_ATTR, (uint16_t)(0x8000u | ((extra & 1u) != 0u ? 0x0030u : 0x0020u)));
    write16(inst, SPU_TSA, (uint16_t)((uint32_t)selector * 0x100u));
    write32(inst, DMA_DPCR, 0x07654321u | (8u << 16));
    write32(inst, DMA4_MADR, address & 0x1FFFFCu);
    write32(inst, DMA4_BCR, 1u + (value & 7u));
    uint32_t direction = (extra & 1u) != 0u ? 0u : 1u;
    uint32_t sync = (extra & 2u) != 0u ? 1u : 0u;
    write32(inst, DMA4_CHCR, SCHWAY__DMA_CHCR_BUSY | direction | (sync << 9));
    run_bounded(inst, 2048u);
}

static void command(schway *inst, const uint8_t *p)
{
    uint8_t op = (uint8_t)(p[0] % 5u);
    uint8_t selector = p[1];
    uint16_t value = (uint16_t)((uint16_t)p[2] | ((uint16_t)p[3] << 8));
    if (op == 0u) {
        write16(inst, SPU_BASE + (uint32_t)(selector & 0xFFu) * 2u, value);
    } else if (op == 1u) {
        uint16_t address = (p[4] & 1u) != 0u ? value : (uint16_t)((uint32_t)selector * 0x100u);
        write16(inst, SPU_TSA, address);
        write16(inst, SPU_FIFO, value);
    } else if (op == 2u) {
        (void)read16(inst, SPU_FIFO);
        (void)read16(inst, SPU_BASE + (uint32_t)(selector & 0xFFu) * 2u);
    } else if (op == 3u) {
        dma_transfer(inst, selector, value, p[4]);
    } else {
        run_bounded(inst, (uint64_t)(p[4] & 3u) * 256u);
    }
}

static void check_bounds(const schway *inst)
{
    const struct schway__spu *spu = &inst->spu;
    if (spu->transfer_head >= SCHWAY__SPU_FIFO_HALFWORDS ||
        spu->transfer_count > SCHWAY__SPU_FIFO_HALFWORDS ||
        spu->transfer_address >= SCHWAY__SPU_RAM_SIZE || (spu->transfer_address & 1u) != 0u ||
        spu->pending_count > SCHWAY__SPU_PENDING_WRITES || spu->noise_phase > 0x1FFFFu ||
        spu->capture_cursor >= 0x200u || spu->reverb_fir_cursor >= SCHWAY__SPU_REVERB_FIR_TAPS ||
        spu->reverb_work_cursor >= SCHWAY__SPU_REVERB_WORK_WORDS ||
        spu->reverb_address > SCHWAY__SPU_RAM_SIZE - 8u ||
        (inst->dma.cur != SCHWAY__DMA_NONE && inst->dma.cur >= SCHWAY__DMA_CHANNELS)) {
        broken("SPU or DMA state escaped its documented array bounds");
    }
    for (uint32_t ch = 0u; ch < SCHWAY__DMA_CHANNELS; ch++) {
        if (inst->dma.phase[ch] > 5u || inst->dma.remain[ch] > SCHWAY__DMA_UNIT_WORDS_MAX) {
            broken("DMA phase or remaining count escaped its bound");
        }
    }
}

static void drain(schway *inst)
{
    int16_t pcm[2u * 64u];
    for (;;) {
        schway_audio audio;
        memset(&audio, 0, sizeof audio);
        audio.size = (uint32_t)sizeof audio;
        audio.samples = pcm;
        audio.capacity = 64u;
        if (schway_get_audio(inst, &audio) != SCHWAY_STATUS_OK) {
            broken("audio drain failed");
        }
        if (audio.written == 0u) {
            return;
        }
    }
}

static void same_future_pcm(schway *a, schway *b)
{
    run_bounded(a, CONTINUE_CYCLES);
    run_bounded(b, CONTINUE_CYCLES);
    int16_t pcm_a[2u * 8u];
    int16_t pcm_b[2u * 8u];
    schway_audio audio_a;
    schway_audio audio_b;
    memset(&audio_a, 0, sizeof audio_a);
    memset(&audio_b, 0, sizeof audio_b);
    audio_a.size = (uint32_t)sizeof audio_a;
    audio_a.samples = pcm_a;
    audio_a.capacity = 8u;
    audio_b.size = (uint32_t)sizeof audio_b;
    audio_b.samples = pcm_b;
    audio_b.capacity = 8u;
    if (schway_get_audio(a, &audio_a) != SCHWAY_STATUS_OK ||
        schway_get_audio(b, &audio_b) != SCHWAY_STATUS_OK ||
        audio_a.written != audio_b.written || audio_a.written > 8u ||
        memcmp(pcm_a, pcm_b, (size_t)2u * audio_a.written * sizeof pcm_a[0]) != 0) {
        broken("state-loaded instances produced different future PCM");
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size > INPUT_MAX) {
        size = INPUT_MAX;
    }
    size -= size % RECORD_SIZE;
    schway *inst = make();
    schway *loaded = make();
    uint64_t start = inst->ticks;
    for (size_t at = 0u; at < size; at += RECORD_SIZE) {
        command(inst, data + at);
    }
    if (loaded->ticks != 0u || loaded->spu.transfer_count != 0u ||
        loaded->spu.next_tick != SCHWAY__CYCLES_PER_PAIR) {
        broken("an input applied to one instance changed the independent instance");
    }
    run_bounded(inst, CONTINUE_CYCLES);
    check_bounds(inst);
    if (inst->ticks - start > (uint64_t)(INPUT_MAX / RECORD_SIZE) * 2048u + CONTINUE_CYCLES) {
        broken("emulated work exceeded the input-derived fixed bound");
    }

    drain(inst);
    size_t state_size = schway_state_size(inst);
    uint8_t *state = (uint8_t *)malloc(state_size);
    uint8_t *loaded_state = (uint8_t *)malloc(state_size);
    if (state == NULL || loaded_state == NULL ||
        schway_save_state(inst, state, state_size) != SCHWAY_STATUS_OK) {
        free(loaded_state);
        free(state);
        broken("valid SPU state could not be saved");
    }
    uint32_t calls_before_load = allocator_calls;
    if (schway_load_state(loaded, state, state_size) != SCHWAY_STATUS_OK ||
        allocator_calls != calls_before_load ||
        schway_save_state(loaded, loaded_state, state_size) != SCHWAY_STATUS_OK ||
        memcmp(state, loaded_state, state_size) != 0) {
        free(loaded_state);
        free(state);
        broken("state load allocated or changed the valid machine state");
    }
    check_bounds(loaded);
    same_future_pcm(inst, loaded);
    check_bounds(inst);
    check_bounds(loaded);
    free(loaded_state);
    free(state);
    schway_destroy(loaded);
    schway_destroy(inst);
    return 0;
}

#define SCHWAY_REPLAY_NAME "fuzz.regress.spu"
#include "replay_main.h"
