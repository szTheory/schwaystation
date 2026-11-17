/* Consumer-level disc callback API coverage. */
#include <stdlib.h>
#include <string.h>

#include "internal.h"
#include "../check.h"

struct disc_test_host {
    uint32_t read_calls;
    uint32_t toc_calls;
    uint32_t toc_succeeds;
    schway_toc toc;
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
    if (host->toc_succeeds == 0u) {
        return 0u;
    }
    memcpy(out, &host->toc, sizeof *out);
    return 1u;
}

static uint32_t bad_q(void *user, uint32_t lba)
{
    (void)user;
    (void)lba;
    return 0u;
}

static void make_toc(struct disc_test_host *host, uint32_t tracks)
{
    memset(&host->toc, 0, sizeof host->toc);
    host->toc.size = (uint32_t)sizeof host->toc;
    host->toc.track_count = tracks;
    if (tracks > 0u) {
        host->toc.tracks[0].size = (uint32_t)sizeof host->toc.tracks[0];
        host->toc.tracks[0].type = SCHWAY_DISC_TRACK_MODE2;
        host->toc.tracks[0].index0 = 150u;
        host->toc.tracks[0].index1 = 150u;
        host->toc.tracks[0].leadout = 1000u;
    }
    if (tracks > 1u) {
        host->toc.tracks[1].size = (uint32_t)sizeof host->toc.tracks[1];
        host->toc.tracks[1].type = SCHWAY_DISC_TRACK_AUDIO;
        host->toc.tracks[1].index0 = 1000u;
        host->toc.tracks[1].index1 = 1010u;
        host->toc.tracks[1].pregap = 10u;
        host->toc.tracks[1].leadout = 2000u;
    }
    host->toc_succeeds = 1u;
}

static schway_disc make_disc(struct disc_test_host *host)
{
    schway_disc disc;
    memset(&disc, 0, sizeof disc);
    disc.size = (uint32_t)sizeof disc;
    disc.user = host;
    disc.read_sector = read_sector;
    disc.toc = read_toc;
    disc.bad_q = bad_q;
    disc.identity_size = (uint32_t)sizeof disc.identity_sha256;
    memset(disc.identity_sha256, 0xA5, sizeof disc.identity_sha256);
    return disc;
}

static void save_state(schway *inst, uint8_t *bytes, size_t size)
{
    CHECK_EQ_U64(schway_save_state(inst, bytes, size), SCHWAY_STATUS_OK);
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

    size_t state_size = schway_state_size(inst);
    uint8_t *state_before = (uint8_t *)malloc(state_size);
    uint8_t *state_after = (uint8_t *)malloc(state_size);
    CHECK(state_size != 0u);
    CHECK(state_before != NULL);
    CHECK(state_after != NULL);
    if (state_before == NULL || state_after == NULL) {
        free(state_before);
        free(state_after);
        schway_destroy(inst);
        CHECK_DONE();
    }
    save_state(inst, state_before, state_size);

    struct disc_test_host first = {0u, 0u, 0u, {0}};
    make_toc(&first, 2u);
    schway_disc first_disc = make_disc(&first);
    schway_disc undersized_disc = first_disc;
    undersized_disc.size = 4u;
    CHECK_EQ_U64(schway_open_disc(inst, &undersized_disc), SCHWAY_STATUS_ERR_STRUCT_SIZE);
    CHECK_EQ_U64(first.toc_calls, 0u);
    CHECK_EQ_U64(first.read_calls, 0u);

    first.toc.size = 4u;
    CHECK_EQ_U64(schway_open_disc(inst, &first_disc), SCHWAY_STATUS_ERR_STRUCT_SIZE);
    CHECK_EQ_U64(first.toc_calls, 1u);
    CHECK_EQ_U64(inst->disc_host.attached, 0u);
    save_state(inst, state_after, state_size);
    CHECK(memcmp(state_before, state_after, state_size) == 0);

    make_toc(&first, 2u);
    first.toc.tracks[0].size = 4u;
    CHECK_EQ_U64(schway_open_disc(inst, &first_disc), SCHWAY_STATUS_ERR_STRUCT_SIZE);
    CHECK_EQ_U64(inst->disc_host.attached, 0u);
    save_state(inst, state_after, state_size);
    CHECK(memcmp(state_before, state_after, state_size) == 0);

    make_toc(&first, 0u);
    CHECK_EQ_U64(schway_open_disc(inst, &first_disc), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(inst->disc_host.attached, 0u);
    save_state(inst, state_after, state_size);
    CHECK(memcmp(state_before, state_after, state_size) == 0);

    make_toc(&first, 2u);
    first.toc.tracks[1].index0 = 900u;
    CHECK_EQ_U64(schway_open_disc(inst, &first_disc), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(inst->disc_host.attached, 0u);
    save_state(inst, state_after, state_size);
    CHECK(memcmp(state_before, state_after, state_size) == 0);

    make_toc(&first, 2u);
    CHECK_EQ_U64(schway_open_disc(inst, &first_disc), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(first.toc_calls, 5u);
    CHECK_EQ_U64(first.read_calls, 0u);
    CHECK_EQ_U64(inst->disc_host.attached, 1u);
    CHECK(inst->disc_host.callbacks.user == &first);
    CHECK(inst->disc_host.callbacks.read_sector == read_sector);
    CHECK(inst->disc_host.callbacks.toc == read_toc);
    CHECK(inst->disc_host.callbacks.bad_q == bad_q);
    CHECK_EQ_U64(inst->disc_host.toc.track_count, 2u);
    CHECK_EQ_U64(inst->disc_host.toc.tracks[0].leadout, 1000u);
    CHECK_EQ_U64(inst->disc_host.toc.tracks[1].index1, 1010u);

    first.toc.tracks[0].leadout = 750u;
    first_disc.user = NULL;
    first_disc.read_sector = NULL;
    first_disc.toc = NULL;
    first_disc.bad_q = NULL;
    CHECK_EQ_U64(inst->disc_host.toc.tracks[0].leadout, 1000u);
    CHECK(inst->disc_host.callbacks.user == &first);
    CHECK(inst->disc_host.callbacks.read_sector == read_sector);
    CHECK(inst->disc_host.callbacks.toc == read_toc);
    CHECK(inst->disc_host.callbacks.bad_q == bad_q);

    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    uint32_t peek_value = UINT32_MAX;
    CHECK_EQ_U64(schway_peek(inst, SCHWAY_SPACE_RAM, 0u, 4u, &peek_value), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(peek_value, 0u);
    schway_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    CHECK_EQ_U64(schway_get_info(inst, &info), SCHWAY_STATUS_OK);
    save_state(inst, state_before, state_size);
    CHECK_EQ_U64(schway_load_state(inst, state_before, state_size), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(first.read_calls, 0u);

    CHECK_EQ_U64(schway_run_frame(inst, NULL), SCHWAY_STATUS_OK);
    CHECK(inst->audio_count > 0u);

    schway_disc second_disc;
    struct disc_test_host second = {0u, 0u, 0u, {0}};
    make_toc(&second, 1u);
    second_disc = make_disc(&second);
    save_state(inst, state_before, state_size);
    struct schway__disc_host host_before = inst->disc_host;
    uint32_t audio_head_before = inst->audio_head;
    uint32_t audio_count_before = inst->audio_count;
    uint32_t audio_dropped_before = inst->audio_dropped;
    int16_t audio_before[SCHWAY__AUDIO_RING_PAIRS][2];
    memcpy(audio_before, inst->audio, sizeof audio_before);
    CHECK_EQ_U64(schway_swap_disc(inst, &second_disc), SCHWAY_STATUS_ERR_LID_CLOSED);
    CHECK_EQ_U64(second.toc_calls, 0u);
    CHECK_EQ_U64(second.read_calls, 0u);
    CHECK(memcmp(&host_before, &inst->disc_host, sizeof host_before) == 0);
    CHECK_EQ_U64(inst->audio_head, audio_head_before);
    CHECK_EQ_U64(inst->audio_count, audio_count_before);
    CHECK_EQ_U64(inst->audio_dropped, audio_dropped_before);
    CHECK(memcmp(audio_before, inst->audio, sizeof audio_before) == 0);
    save_state(inst, state_after, state_size);
    CHECK(memcmp(state_before, state_after, state_size) == 0);

    CHECK_EQ_U64(schway_set_lid(inst, 2u), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(inst->disc_host.lid_open, 0u);
    CHECK_EQ_U64(schway_set_lid(inst, 1u), SCHWAY_STATUS_OK);
    save_state(inst, state_before, state_size);
    host_before = inst->disc_host;
    second.toc.tracks[0].index0 = 1100u;
    CHECK_EQ_U64(schway_swap_disc(inst, &second_disc), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(second.toc_calls, 1u);
    CHECK_EQ_U64(second.read_calls, 0u);
    CHECK(memcmp(&host_before, &inst->disc_host, sizeof host_before) == 0);
    save_state(inst, state_after, state_size);
    CHECK(memcmp(state_before, state_after, state_size) == 0);
    CHECK_EQ_U64(inst->audio_head, audio_head_before);
    CHECK_EQ_U64(inst->audio_count, audio_count_before);
    CHECK_EQ_U64(inst->audio_dropped, audio_dropped_before);
    CHECK(memcmp(audio_before, inst->audio, sizeof audio_before) == 0);

    make_toc(&second, 1u);
    CHECK_EQ_U64(schway_swap_disc(inst, &second_disc), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(second.toc_calls, 2u);
    CHECK(inst->disc_host.callbacks.user == &second);
    CHECK_EQ_U64(inst->disc_host.toc.tracks[0].leadout, 1000u);

    struct disc_test_host empty = {0u, 0u, 0u, {0}};
    make_toc(&empty, 0u);
    schway_disc empty_disc = make_disc(&empty);
    CHECK_EQ_U64(schway_swap_disc(inst, &empty_disc), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(empty.toc_calls, 1u);
    CHECK_EQ_U64(inst->disc_host.attached, 0u);
    CHECK_EQ_U64(inst->disc_host.lid_open, 1u);
    CHECK_EQ_U64(inst->disc_host.identity_valid, 0u);
    CHECK(inst->disc_host.callbacks.read_sector == NULL);
    CHECK_EQ_U64(inst->disc_host.toc.track_count, 0u);
    CHECK_EQ_U64(schway_set_lid(inst, 0u), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(inst->disc_host.lid_open, 0u);
    CHECK_EQ_U64(schway_set_lid(inst, 1u), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_swap_disc(inst, &second_disc), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(inst->disc_host.attached, 1u);
    CHECK(inst->disc_host.callbacks.user == &second);
    CHECK_EQ_U64(schway_set_lid(inst, 0u), SCHWAY_STATUS_OK);

    host_before = inst->disc_host;
    CHECK_EQ_U64(schway_swap_disc(inst, &first_disc), SCHWAY_STATUS_ERR_LID_CLOSED);
    CHECK_EQ_U64(first.toc_calls, 5u);
    CHECK(memcmp(&host_before, &inst->disc_host, sizeof host_before) == 0);

    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(inst->disc_host.callbacks.user == &second, 1u);
    CHECK_EQ_U64(inst->disc_host.toc.tracks[0].leadout, 1000u);
    uint32_t second_reads_before = second.read_calls;
    peek_value = UINT32_MAX;
    CHECK_EQ_U64(schway_peek(inst, SCHWAY_SPACE_RAM, 0u, 4u, &peek_value), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(peek_value, 0u);
    CHECK_EQ_U64(schway_save_state(inst, state_after, schway_state_size(inst)), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_load_state(inst, state_after, schway_state_size(inst)), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(second.read_calls, second_reads_before);

    free(state_before);
    free(state_after);
    schway_destroy(inst);
    CHECK_DONE();
}
