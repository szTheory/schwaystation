/* API version 8 adds board identity to the size-tagged machine profile. */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "../check.h"

static void *test_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void test_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

int main(void)
{
    CHECK(SCHWAY_API_VERSION >= 8);
    CHECK_EQ_U64(offsetof(schway_profile, board), 16u);
    CHECK_EQ_U64(SCHWAY_PROFILE_FAST_BOOT, 16u);
    CHECK_EQ_U64(SCHWAY_BOARD_PU7, 1u);
    CHECK_EQ_U64(SCHWAY_BOARD_PU8_EARLY, 2u);
    CHECK_EQ_U64(SCHWAY_BOARD_PU8_LATE, 3u);
    CHECK_EQ_U64(SCHWAY_BOARD_PU18, 4u);
    CHECK_EQ_U64(SCHWAY_BOARD_PU20, 5u);
    CHECK_EQ_U64(SCHWAY_BOARD_PU22, 6u);
    CHECK_EQ_U64(SCHWAY_BOARD_PU23, 7u);
    CHECK_EQ_U64(SCHWAY_BOARD_PM41, 8u);
    CHECK_EQ_U64(SCHWAY_BOARD_PM41_2, 9u);

    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = test_alloc;
    config.allocator.free = test_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&config, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) CHECK_DONE();

    schway_profile profile;
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = SCHWAY_REGION_NTSC_U;
    profile.flags = SCHWAY_PROFILE_GPU_V0;
    profile.board = SCHWAY_BOARD_PU7;
    CHECK_EQ_U64(schway_set_profile(inst, &profile), SCHWAY_STATUS_OK);

    profile.flags = SCHWAY_PROFILE_GPU_V0;
    profile.board = SCHWAY_BOARD_PM41_2;
    CHECK_EQ_U64(schway_set_profile(inst, &profile), SCHWAY_STATUS_ERR_PROFILE_REJECTED);
    profile.size = (uint32_t)offsetof(schway_profile, board);
    profile.board = SCHWAY_BOARD_PM41_2;
    CHECK_EQ_U64(schway_set_profile(inst, &profile), SCHWAY_STATUS_OK);

    schway_destroy(inst);
    CHECK_DONE();
}
