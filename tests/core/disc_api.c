/* Consumer-level disc callback API coverage. */
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "../check.h"

struct disc_test_host {
    uint32_t read_calls;
    uint32_t toc_calls;
};

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

static uint32_t read_sector(void *user, uint32_t lba, uint8_t sector[2352], uint8_t subcode[96])
{
    struct disc_test_host *host = (struct disc_test_host *)user;
    (void)lba;
    (void)sector;
    (void)subcode;
    host->read_calls++;
    return 1u;
}

static uint32_t read_toc(void *user, schway_toc *out)
{
    struct disc_test_host *host = (struct disc_test_host *)user;
    host->toc_calls++;
    out->size = (uint32_t)sizeof *out;
    out->track_count = 1u;
    out->tracks[0].size = (uint32_t)sizeof out->tracks[0];
    out->tracks[0].type = SCHWAY_DISC_TRACK_MODE2;
    out->tracks[0].index0 = 150u;
    out->tracks[0].index1 = 150u;
    out->tracks[0].leadout = 1000u;
    return 1u;
}

static uint32_t bad_q(void *user, uint32_t lba)
{
    (void)user;
    (void)lba;
    return 0u;
}

int main(void)
{
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    CHECK(inst != NULL);
    if (inst == NULL) {
        CHECK_DONE();
    }

    struct disc_test_host host = {0u, 0u};
    schway_disc disc;
    memset(&disc, 0, sizeof disc);
    disc.size = (uint32_t)sizeof disc;
    disc.user = &host;
    disc.read_sector = read_sector;
    disc.toc = read_toc;
    disc.bad_q = bad_q;

    CHECK_EQ_U64(schway_open_disc(inst, &disc), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(host.toc_calls, 1u);
    CHECK_EQ_U64(host.read_calls, 0u);
    schway_destroy(inst);
    CHECK_DONE();
}
