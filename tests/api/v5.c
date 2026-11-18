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
    uint32_t output_size_seen;
    uint32_t output_size_override;
    uint32_t toc_succeeds;
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
    host->output_size_seen = out->size;
    if (host->toc_succeeds == 0u) {
        return 0u;
    }
    memcpy(out, &host->toc, sizeof *out);
    if (host->output_size_override != 0u) {
        out->size = host->output_size_override;
    }
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
    host->toc_succeeds = 1u;

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
    CHECK_EQ_U64(SCHWAY_PROFILE_DEBUG_PORT, 1u);
    CHECK_EQ_U64(SCHWAY_PROFILE_CD_LICENSED, 2u);
    CHECK_EQ_U64(SCHWAY_PROFILE_GPU_V0, 4u);
    CHECK_EQ_U64(SCHWAY_PROFILE_CD_UNLOCK, 8u);
    CHECK_EQ_U64(SCHWAY_STATUS_ERR_ARGUMENT, 1u);
    CHECK_EQ_U64(SCHWAY_STATUS_ERR_STRUCT_SIZE, 2u);
    CHECK_EQ_U64(SCHWAY_STATUS_ERR_EXE_RANGE, 12u);
    CHECK_EQ_U64(SCHWAY_STATUS_ERR_LID_CLOSED, 13u);
    CHECK_EQ_U64(SCHWAY_DISC_TRACK_AUDIO, 1u);
    CHECK_EQ_U64(SCHWAY_DISC_TRACK_MODE1, 2u);
    CHECK_EQ_U64(SCHWAY_DISC_TRACK_MODE2, 3u);
    CHECK_EQ_U64(SCHWAY_DISC_MAX_TRACKS, 99u);
    CHECK_EQ_U64(sizeof(schway_disc_track), 28u);
    CHECK_EQ_U64(offsetof(schway_disc_track, type), 4u);
    CHECK_EQ_U64(offsetof(schway_disc_track, index0), 8u);
    CHECK_EQ_U64(offsetof(schway_disc_track, index1), 12u);
    CHECK_EQ_U64(offsetof(schway_disc_track, pregap), 16u);
    CHECK_EQ_U64(offsetof(schway_disc_track, leadout), 20u);
    CHECK_EQ_U64(offsetof(schway_disc_track, reserved0), 24u);
    CHECK_EQ_U64(offsetof(schway_toc, track_count), 4u);
    CHECK_EQ_U64(offsetof(schway_toc, reserved0), 8u);
    CHECK_EQ_U64(offsetof(schway_toc, tracks), 12u);
    CHECK_EQ_U64(sizeof(schway_toc), 12u + SCHWAY_DISC_MAX_TRACKS * sizeof(schway_disc_track));
    CHECK_EQ_U64(offsetof(schway_disc, reserved0), 4u);
    CHECK_EQ_U64(offsetof(schway_disc, user), 8u);
    CHECK(offsetof(schway_disc, read_sector) >=
          offsetof(schway_disc, user) + sizeof(((schway_disc *)0)->user));
    CHECK(offsetof(schway_disc, toc) >=
          offsetof(schway_disc, read_sector) + sizeof(((schway_disc *)0)->read_sector));
    CHECK(offsetof(schway_disc, bad_q) >=
          offsetof(schway_disc, toc) + sizeof(((schway_disc *)0)->toc));
    CHECK(sizeof(schway_disc) >=
          offsetof(schway_disc, bad_q) + sizeof(((schway_disc *)0)->bad_q));

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

    schway_disc undersized_disc = first_disc;
    undersized_disc.size = (uint32_t)sizeof undersized_disc - 1u;
    schway_disc undersized_before = undersized_disc;
    CHECK_EQ_U64(schway_open_disc(inst, &undersized_disc), SCHWAY_STATUS_ERR_STRUCT_SIZE);
    CHECK_EQ_U64(first.toc_calls, 0u);
    CHECK(memcmp(&undersized_before, &undersized_disc, sizeof undersized_disc) == 0);

    struct v5_larger_disc {
        schway_disc disc;
        uint8_t future[8];
    } newer_disc;
    memset(&newer_disc, 0, sizeof newer_disc);
    newer_disc.disc = first_disc;
    newer_disc.disc.size = (uint32_t)sizeof newer_disc;
    newer_disc.future[0] = 1u;
    struct v5_larger_disc newer_before = newer_disc;
    CHECK_EQ_U64(schway_open_disc(inst, &newer_disc.disc), SCHWAY_STATUS_ERR_STRUCT_SIZE);
    CHECK_EQ_U64(first.toc_calls, 0u);
    CHECK(memcmp(&newer_before, &newer_disc, sizeof newer_disc) == 0);

    first.output_size_override = (uint32_t)sizeof(schway_toc) - 1u;
    schway_toc toc_before = first.toc;
    CHECK_EQ_U64(schway_open_disc(inst, &first_disc), SCHWAY_STATUS_ERR_STRUCT_SIZE);
    CHECK_EQ_U64(first.toc_calls, 1u);
    CHECK_EQ_U64(first.output_size_seen, sizeof(schway_toc));
    CHECK(memcmp(&toc_before, &first.toc, sizeof toc_before) == 0);
    first.output_size_override = 0u;

    first.toc.tracks[1].size = (uint32_t)sizeof first.toc.tracks[1] - 1u;
    toc_before = first.toc;
    CHECK_EQ_U64(schway_open_disc(inst, &first_disc), SCHWAY_STATUS_ERR_STRUCT_SIZE);
    CHECK_EQ_U64(first.toc_calls, 2u);
    CHECK(memcmp(&toc_before, &first.toc, sizeof toc_before) == 0);
    v5_make_toc(&first);

    first.toc_succeeds = 0u;
    CHECK_EQ_U64(schway_open_disc(inst, &first_disc), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(first.toc_calls, 3u);
    v5_make_toc(&first);

    newer_disc.future[0] = 0u;
    CHECK_EQ_U64(schway_open_disc(inst, &newer_disc.disc), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(first.toc_calls, 4u);
    CHECK_EQ_U64(first.output_size_seen, sizeof(schway_toc));
    CHECK_EQ_U64(schway_open_disc(inst, &second_disc), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(second.toc_calls, 0u);

    CHECK_EQ_U64(schway_swap_disc(inst, &second_disc), SCHWAY_STATUS_ERR_LID_CLOSED);
    CHECK_EQ_U64(second.toc_calls, 0u);
    CHECK_EQ_U64(schway_set_lid(inst, 2u), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_swap_disc(inst, &second_disc), SCHWAY_STATUS_ERR_LID_CLOSED);
    CHECK_EQ_U64(second.toc_calls, 0u);
    CHECK_EQ_U64(schway_set_lid(inst, 1u), SCHWAY_STATUS_OK);

    second.output_size_override = (uint32_t)sizeof(schway_toc) - 1u;
    toc_before = second.toc;
    CHECK_EQ_U64(schway_swap_disc(inst, &second_disc), SCHWAY_STATUS_ERR_STRUCT_SIZE);
    CHECK_EQ_U64(second.toc_calls, 1u);
    CHECK_EQ_U64(second.output_size_seen, sizeof(schway_toc));
    CHECK(memcmp(&toc_before, &second.toc, sizeof toc_before) == 0);
    second.output_size_override = 0u;
    CHECK_EQ_U64(schway_swap_disc(inst, &second_disc), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(second.toc_calls, 2u);
    CHECK_EQ_U64(schway_set_lid(inst, 0u), SCHWAY_STATUS_OK);

    schway_destroy(inst);
    CHECK_DONE();
}
