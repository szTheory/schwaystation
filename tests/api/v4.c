/* API version 4 adds a read-only VRAM view and the early-GPU profile flag. */
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "../check.h"

#if SCHWAY_API_VERSION >= 4
static void *v4_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void v4_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}
#endif

int main(void)
{
    CHECK(SCHWAY_API_VERSION >= 4);
#if SCHWAY_API_VERSION >= 4
    CHECK_EQ_U64(SCHWAY_INPUT_DEVICE_NONE, 0u);
    CHECK_EQ_U64(SCHWAY_INPUT_DEVICE_DIGITAL, 1u);
    CHECK_EQ_U64(SCHWAY_INPUT_DEVICE_DUALSHOCK, 2u);
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = v4_alloc;
    cfg.allocator.free = v4_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    if (inst != NULL) {
        CHECK_EQ_U64(SCHWAY_PROFILE_GPU_V0, 4u);
        schway_profile profile = {sizeof profile, SCHWAY_REGION_NTSC_U, SCHWAY_PROFILE_GPU_V0};
        CHECK_EQ_U64(schway_set_profile(inst, &profile), SCHWAY_STATUS_OK);
        schway_span first;
        schway_span second;
        CHECK_EQ_U64(schway_get_vram(inst, &first), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(first.size, 1024u * 512u * sizeof(uint16_t));
        CHECK(first.data != NULL);
        CHECK(((const uint16_t *)first.data)[0] == 0u);
        CHECK_EQ_U64(schway_run_frame(inst, NULL), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(schway_get_vram(inst, &second), SCHWAY_STATUS_OK);
        CHECK(second.data == first.data);
        CHECK_EQ_U64(second.size, first.size);

        schway_input input;
        memset(&input, 0, sizeof input);
        input.size = (uint32_t)sizeof input;
        input.device[0] = SCHWAY_INPUT_DEVICE_DUALSHOCK;
        input.device[1] = SCHWAY_INPUT_DEVICE_NONE;
        input.axes[0][0] = 128u;
        input.axes[0][1] = 128u;
        input.axes[0][2] = 128u;
        input.axes[0][3] = 128u;
        CHECK_EQ_U64(schway_set_input(inst, &input), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(schway_set_input(inst, NULL), SCHWAY_STATUS_ERR_ARGUMENT);

        schway_run_result result;
        memset(&result, 0, sizeof result);
        result.size = (uint32_t)sizeof result;
        CHECK_EQ_U64(schway_run_frame(inst, &result), SCHWAY_STATUS_OK);
        (void)result.lag;
        (void)result.rumble[0];
        (void)result.rumble[1];

        struct old_run_result {
            uint32_t size;
            uint32_t stop;
            uint64_t frame;
            uint64_t ticks;
            uint32_t canary;
        } old_result;
        memset(&old_result, 0, sizeof old_result);
        old_result.size = 24u;
        old_result.canary = 0xC35AA53Cu;
        CHECK_EQ_U64(schway_run_frame(inst, (schway_run_result *)(void *)&old_result),
                     SCHWAY_STATUS_OK);
        CHECK_EQ_HEX(old_result.canary, 0xC35AA53Cu);
        schway_destroy(inst);
    }
#endif
    CHECK_DONE();
}
