/* Pinned SPU noise clock and capture checks. The noise schedule follows
   psx-spx commit 3717718f642ce2ca9ff590b65918fb3a46279cc4. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/internal.h"

#define SPU_REGISTER_BASE UINT32_C(0x1F801C00)
#define SPU_ATTR UINT32_C(0x1F801DAA)

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

int main(void)
{
    puts("TAP version 13");
    puts("1..1");
    schway *inst = make_instance();
    if (inst == NULL) {
        puts("not ok 1 - a scheduled noise clock shifts the shared LFSR");
        return 1;
    }

    /* Shift 15 gives a four-sample period. Step 0 clocks at its first slot. */
    uint32_t attr_index = (SPU_ATTR - SPU_REGISTER_BASE) >> 1;
    inst->spu.reg[attr_index] = 0xBC00u;
    inst->spu.noise_lfsr = 1u;
    run_sample(inst);
    uint32_t passed = inst->spu.noise_lfsr == 3u;
    printf("%s 1 - a scheduled noise clock shifts the shared LFSR\n",
           passed != 0u ? "ok" : "not ok");
    if (passed == 0u) {
        printf("# expected LFSR 3 after slot 0; got %u\n", inst->spu.noise_lfsr);
    }
    schway_destroy(inst);
    return passed != 0u ? 0 : 1;
}
