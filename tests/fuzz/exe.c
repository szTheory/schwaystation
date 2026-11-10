/* The PS-EXE loader fuzz target. Each input is a whole file. It is probed and
   loaded into a power-reset instance, and the two must agree; a refused file
   must change nothing; an accepted one may change only the text segment and
   the launch block; and the loaded program must hand control back: two
   frames' cycles run under STOP_ON_FAULT, each schway_run returning OK with a
   stop reason (AGENTS rule 4, a hostile executable). A broken invariant
   prints which one and aborts, so libFuzzer records the input as a crasher.
   White-box: it reads guest RAM through src/. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"

#define RAM 0x200000u
#define LAUNCH 0x200u
#define LAUNCH_SIZE 16u

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

static void *fuzz_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void fuzz_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static void broken(const char *invariant)
{
    fprintf(stderr, "fuzz.exe: invariant broken: %s\n", invariant);
    abort();
}

/* The instance and the RAM it held before the load live across inputs, so an
   input costs a reset and a copy, not an allocation. */
static schway *inst;
static uint8_t before[RAM];

static schway *instance(void)
{
    if (inst == NULL) {
        schway_config cfg;
        memset(&cfg, 0, sizeof cfg);
        cfg.size = (uint32_t)sizeof cfg;
        cfg.allocator.alloc = fuzz_alloc;
        cfg.allocator.free = fuzz_free;
        if (schway_create(&cfg, &inst) != SCHWAY_STATUS_OK || inst == NULL) {
            broken("the instance could not be created");
        }
    }
    return inst;
}

/* The refused file changed nothing, and an accepted one only the text
   segment (its physical span, from the probe) and the launch block: patch
   those spans of the saved copy from the instance, and nothing may differ. */
static void check_ram(schway *s, int loaded, const schway_exe_info *info)
{
    if (loaded) {
        uint32_t at = info->t_addr & 0x1FFFFFFFu;
        memcpy(&before[at], &s->bus.ram[at], info->t_size);
        memcpy(&before[LAUNCH], &s->bus.ram[LAUNCH], LAUNCH_SIZE);
    }
    if (memcmp(before, s->bus.ram, RAM) != 0) {
        broken(loaded ? "a load changed RAM outside the text segment and the launch block"
                      : "a refused load changed RAM");
    }
}

static void run_two_frames(schway *s)
{
    schway_budget budget;
    memset(&budget, 0, sizeof budget);
    budget.size = (uint32_t)sizeof budget;
    budget.flags = SCHWAY_RUN_STOP_ON_FAULT;
    budget.max_cycles = schway__instance_frame_max(s->region);
    for (int i = 0; i < 2; i++) {
        schway_run_result result;
        memset(&result, 0, sizeof result);
        result.size = (uint32_t)sizeof result;
        if (schway_run(s, &budget, &result) != SCHWAY_STATUS_OK) {
            broken("schway_run did not return OK");
        }
        if (result.stop != SCHWAY_STOP_FRAME && result.stop != SCHWAY_STOP_BUDGET &&
            result.stop != SCHWAY_STOP_HOOK && result.stop != SCHWAY_STOP_FAULT) {
            broken("schway_run returned no stop reason");
        }
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* The public entry points refuse an empty file as an argument error. */
    if (size == 0u) {
        return 0;
    }
    schway *s = instance();
    if (schway_reset(s, SCHWAY_RESET_POWER) != SCHWAY_STATUS_OK) {
        broken("a power reset failed");
    }
    schway_exe_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    schway_status probed = schway_probe_exe(data, size, &info);
    memcpy(before, s->bus.ram, RAM);
    schway_status loaded = schway_load_exe(s, data, size);
    if (probed != loaded) {
        broken("probe and load disagree");
    }
    if (loaded != SCHWAY_STATUS_OK && loaded != SCHWAY_STATUS_ERR_EXE_INVALID &&
        loaded != SCHWAY_STATUS_ERR_EXE_RANGE) {
        broken("the status is not OK, EXE_INVALID or EXE_RANGE");
    }
    if ((loaded == SCHWAY_STATUS_OK) != (s->exe_loaded != 0u)) {
        broken("the instance's loaded flag does not follow the status");
    }
    check_ram(s, loaded == SCHWAY_STATUS_OK, &info);
    if (loaded == SCHWAY_STATUS_OK) {
        run_two_frames(s);
    }
    return 0;
}

#define SCHWAY_REPLAY_NAME "fuzz.regress.exe"
#include "replay_main.h"
