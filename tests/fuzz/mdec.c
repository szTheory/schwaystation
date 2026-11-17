/* Bounded replay of MDEC commands and compressed bitstream words through MMIO. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus.h"
#include "internal.h"
#include "mdec.h"
#include "schwaystation.h"

#define MDEC_DATA 0x1F801820u
#define MDEC_STATUS 0x1F801824u
#define INPUT_MAX (256u * 1024u)
#define CONTINUE_CYCLES (2u * 128u)

static void broken(const char *what)
{
    fprintf(stderr, "fuzz.regress.mdec: %s\n", what);
    abort();
}

static void *alloc_bytes(void *user, size_t size)
{
    (void)user;
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
    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = alloc_bytes;
    config.allocator.free = free_bytes;
    schway *inst = NULL;
    if (schway_create(&config, &inst) != SCHWAY_STATUS_OK || inst == NULL) {
        broken("instance creation failed");
    }
    return inst;
}

static void write32(schway *inst, uint32_t address, uint32_t value)
{
    uint32_t cycles = 0u;
    schway__sched_sync(inst);
    if (schway__bus_write(inst, address, 4u, value, &cycles) != 0u) {
        broken("mapped MDEC write was refused");
    }
}

static uint32_t read32(schway *inst, uint32_t address)
{
    uint32_t cycles = 0u;
    uint32_t value = 0u;
    schway__sched_sync(inst);
    if (schway__bus_read(inst, address, 4u, &value, &cycles) != 0u) {
        broken("mapped MDEC read was refused");
    }
    return value;
}

static uint64_t run_bounded(schway *inst, uint64_t cycles)
{
    uint64_t before = inst->ticks;
    schway_budget budget;
    schway_run_result result;
    memset(&budget, 0, sizeof budget);
    memset(&result, 0, sizeof result);
    budget.size = (uint32_t)sizeof budget;
    budget.max_cycles = cycles;
    budget.flags = SCHWAY_RUN_STOP_ON_FAULT;
    result.size = (uint32_t)sizeof result;
    if (schway_run(inst, &budget, &result) != SCHWAY_STATUS_OK ||
        (result.stop != SCHWAY_STOP_BUDGET && result.stop != SCHWAY_STOP_FRAME &&
         result.stop != SCHWAY_STOP_FAULT) ||
        result.ticks < before || result.ticks - before > cycles) {
        broken("public run exceeded the requested MDEC cycle budget");
    }
    return result.ticks - before;
}

static uint32_t load_le32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static void check_bounds(const schway *inst)
{
    const struct schway__mdec *mdec = &inst->mdec;
    if (mdec->input_count > SCHWAY__MDEC_INPUT_WORDS ||
        mdec->output_count > SCHWAY__MDEC_OUTPUT_WORDS ||
        mdec->input_head >= SCHWAY__MDEC_INPUT_WORDS ||
        mdec->output_head >= SCHWAY__MDEC_OUTPUT_WORDS ||
        mdec->command_words > 0xFFFFu || mdec->words_remaining > 0xFFFFu ||
        mdec->block_index > 5u || mdec->depth > 3u) {
        broken("MDEC FIFO or command state exceeded its fixed bounds");
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size > INPUT_MAX) return 0;
    size_t word_count = size / 4u;
    if (word_count == 0u) return 0;
    schway *inst = make();
    write32(inst, MDEC_DATA, load_le32(data));
    uint64_t frame_cycles = schway__instance_frame_max(inst->region);
    uint64_t total_limit = frame_cycles * 2u;
    uint64_t elapsed = 0u;

    for (size_t word = 1u; word < word_count; word++) {
        write32(inst, MDEC_DATA, load_le32(data + word * 4u));
        if (word % SCHWAY__MDEC_INPUT_WORDS == 0u && elapsed < total_limit) {
            uint64_t requested = total_limit - elapsed;
            if (requested > CONTINUE_CYCLES) requested = CONTINUE_CYCLES;
            elapsed += run_bounded(inst, requested);
        }
        check_bounds(inst);
    }
    if (elapsed < total_limit) elapsed += run_bounded(inst, total_limit - elapsed);
    if (elapsed > total_limit) broken("total emulated time exceeded two frames");

    (void)read32(inst, MDEC_STATUS);
    for (uint32_t i = 0u; i < SCHWAY__MDEC_OUTPUT_WORDS && inst->mdec.output_count != 0u; i++) {
        (void)read32(inst, MDEC_DATA);
    }
    check_bounds(inst);
    schway_destroy(inst);
    return 0;
}

#define SCHWAY_REPLAY_NAME "fuzz.regress.mdec"
#include "replay_main.h"
