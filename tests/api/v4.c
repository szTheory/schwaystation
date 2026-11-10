/* API version 4 adds a read-only view of the complete native VRAM. */
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
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = v4_alloc;
    cfg.allocator.free = v4_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    if (inst != NULL) {
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
        schway_destroy(inst);
    }
#endif
    CHECK_DONE();
}
