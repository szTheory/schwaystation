/* TAP reporter for the private scheduled SPU voice path. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/internal.h"

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

static int trace_first_block_sample(void)
{
    schway *inst = make_instance();
    if (inst == NULL) {
        return 0;
    }

    inst->spu.ram[0x100u] = 0u; /* shift 0, filter 0 */
    inst->spu.ram[0x101u] = 0u; /* keep decoding the first block */
    memset(&inst->spu.ram[0x102u], 0x77, 14u);
    inst->spu.voice[0].sample_counter = 0x0FF0u;
    schway__spu_reg_write(inst, 0x1F801C04u, 1u);
    schway__spu_reg_write(inst, 0x1F801C06u, 0x20u); /* block at byte 100h */
    inst->ticks = SCHWAY__CYCLES_PER_PAIR;
    schway__spu_event(inst, inst->ticks);

    int passed = inst->spu.voice[0].sample == 4204;
    if (!passed) {
        printf("# expected first interpolated sample 4204, got %d\n",
               inst->spu.voice[0].sample);
    }
    schway_destroy(inst);
    return passed;
}

int main(void)
{
    puts("TAP version 13");
    puts("1..1");
    int passed = trace_first_block_sample();
    printf("%s 1 - BRR block interpolation reaches a voice sample\n",
           passed ? "ok" : "not ok");
    return passed ? 0 : 1;
}
