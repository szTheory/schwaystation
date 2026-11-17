/* API version 7 appends disc profile overrides without changing older flags. */
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
    CHECK(SCHWAY_API_VERSION >= 7);
    CHECK_EQ_U64(offsetof(schway_profile, disc_flags), 12u);

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
    profile.size = (uint32_t)offsetof(schway_profile, disc_flags);
    profile.region = SCHWAY_REGION_NTSC_U;
    profile.flags = SCHWAY_PROFILE_DEBUG_PORT;
    CHECK_EQ_U64(schway_set_profile(inst, &profile), SCHWAY_STATUS_OK);

    profile.flags = SCHWAY_PROFILE_CD_LICENSED | SCHWAY_PROFILE_CD_UNLOCK;
    CHECK_EQ_U64(schway_set_profile(inst, &profile), SCHWAY_STATUS_OK);

    profile.size = (uint32_t)sizeof profile;
    profile.flags = SCHWAY_PROFILE_DEBUG_PORT;
    profile.disc_flags = SCHWAY_PROFILE_CD_LICENSED | SCHWAY_PROFILE_CD_UNLOCK;
    CHECK_EQ_U64(schway_set_profile(inst, &profile), SCHWAY_STATUS_OK);

    profile.flags = SCHWAY_PROFILE_CD_LICENSED;
    profile.disc_flags = 0u;
    CHECK_EQ_U64(schway_set_profile(inst, &profile), SCHWAY_STATUS_ERR_PROFILE_REJECTED);

    schway_destroy(inst);
    CHECK_DONE();
}
