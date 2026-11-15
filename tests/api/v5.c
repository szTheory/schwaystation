/* API version 5 adds the host-provided disc callback surface. */
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "../check.h"

struct v5_disc_host {
    schway_toc toc;
    uint32_t toc_calls;
    uint32_t sector_calls;
    uint32_t bad_q_calls;
};

static void *v5_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void v5_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static uint32_t v5_read_sector(void *user, uint32_t lba, uint8_t sector[2352],
                               uint8_t subcode[96])
{
    struct v5_disc_host *host = (struct v5_disc_host *)user;
    host->sector_calls++;
    memset(sector, (int)(lba & 0xFFu), 2352u);
    memset(subcode, 0u, 96u);
    return 1u;
}

static uint32_t v5_read_toc(void *user, schway_toc *out)
{
    struct v5_disc_host *host = (struct v5_disc_host *)user;
    host->toc_calls++;
    memcpy(out, &host->toc, sizeof *out);
    return 1u;
}

static uint32_t v5_bad_q(void *user, uint32_t lba)
{
    struct v5_disc_host *host = (struct v5_disc_host *)user;
    host->bad_q_calls++;
    return lba == 42u ? 1u : 0u;
}

static void v5_make_toc(struct v5_disc_host *host)
{
    memset(&host->toc, 0, sizeof host->toc);
    host->toc.size = (uint32_t)sizeof host->toc;
    host->toc.track_count = 3u;

    host->toc.tracks[0].size = (uint32_t)sizeof host->toc.tracks[0];
    host->toc.tracks[0].type = SCHWAY_DISC_TRACK_MODE2;
    host->toc.tracks[0].index0 = 150u;
    host->toc.tracks[0].index1 = 150u;
    host->toc.tracks[0].leadout = 1000u;

    host->toc.tracks[1].size = (uint32_t)sizeof host->toc.tracks[1];
    host->toc.tracks[1].type = SCHWAY_DISC_TRACK_AUDIO;
    host->toc.tracks[1].index0 = 1000u;
    host->toc.tracks[1].index1 = 1010u;
    host->toc.tracks[1].pregap = 10u;
    host->toc.tracks[1].leadout = 2000u;

    host->toc.tracks[2].size = (uint32_t)sizeof host->toc.tracks[2];
    host->toc.tracks[2].type = SCHWAY_DISC_TRACK_MODE1;
    host->toc.tracks[2].index0 = 2000u;
    host->toc.tracks[2].index1 = 2000u;
    host->toc.tracks[2].leadout = 2500u;
}

static schway_disc v5_make_disc(struct v5_disc_host *host)
{
    schway_disc disc;
    memset(&disc, 0, sizeof disc);
    disc.size = (uint32_t)sizeof disc;
    disc.user = host;
    schway_disc_read_sector_fn read_sector = v5_read_sector;
    schway_disc_toc_fn toc = v5_read_toc;
    schway_disc_bad_q_fn bad_q = v5_bad_q;
    disc.read_sector = read_sector;
    disc.toc = toc;
    disc.bad_q = bad_q;
    return disc;
}

int main(void)
{
    CHECK(SCHWAY_API_VERSION >= 5);
    CHECK_EQ_U64(SCHWAY_STATUS_ERR_EXE_RANGE, 12u);
    CHECK_EQ_U64(SCHWAY_STATUS_ERR_LID_CLOSED, 13u);
    CHECK_EQ_U64(SCHWAY_DISC_TRACK_AUDIO, 1u);
    CHECK_EQ_U64(SCHWAY_DISC_TRACK_MODE1, 2u);
    CHECK_EQ_U64(SCHWAY_DISC_TRACK_MODE2, 3u);
    CHECK_EQ_U64(SCHWAY_DISC_MAX_TRACKS, 99u);

    struct v5_disc_host first;
    struct v5_disc_host second;
    memset(&first, 0, sizeof first);
    memset(&second, 0, sizeof second);
    v5_make_toc(&first);
    v5_make_toc(&second);
    schway_disc first_disc = v5_make_disc(&first);
    schway_disc second_disc = v5_make_disc(&second);

    uint8_t sector[2352];
    uint8_t subcode[96];
    schway_disc_read_sector_fn read_sector = first_disc.read_sector;
    schway_disc_bad_q_fn bad_q = first_disc.bad_q;
    CHECK_EQ_U64(read_sector(first_disc.user, 42u, sector, subcode), 1u);
    CHECK_EQ_U64(first.sector_calls, 1u);
    CHECK_EQ_U64(sector[0], 42u);
    CHECK_EQ_U64(subcode[0], 0u);
    CHECK_EQ_U64(bad_q(first_disc.user, 42u), 1u);
    CHECK_EQ_U64(bad_q(first_disc.user, 43u), 0u);
    CHECK_EQ_U64(first.bad_q_calls, 2u);

    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = v5_alloc;
    cfg.allocator.free = v5_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    CHECK(inst != NULL);
    if (inst == NULL) {
        CHECK_DONE();
    }

    CHECK_EQ_U64(schway_open_disc(inst, &first_disc), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(first.toc_calls, 1u);
    CHECK_EQ_U64(schway_swap_disc(inst, &second_disc), SCHWAY_STATUS_ERR_LID_CLOSED);
    CHECK_EQ_U64(second.toc_calls, 0u);
    CHECK_EQ_U64(schway_set_lid(inst, 2u), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_swap_disc(inst, &second_disc), SCHWAY_STATUS_ERR_LID_CLOSED);
    CHECK_EQ_U64(second.toc_calls, 0u);
    CHECK_EQ_U64(schway_set_lid(inst, 1u), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_swap_disc(inst, &second_disc), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(second.toc_calls, 1u);
    CHECK_EQ_U64(schway_set_lid(inst, 0u), SCHWAY_STATUS_OK);

    schway_destroy(inst);
    CHECK_DONE();
}
