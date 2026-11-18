/* Consumer-level content identity and state compatibility API coverage. */
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "../check.h"

struct disc_host {
    uint32_t toc_calls;
    uint32_t read_calls;
    uint32_t bad_q_calls;
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
    struct disc_host *host = (struct disc_host *)user;
    (void)lba;
    (void)sector;
    (void)subcode;
    host->read_calls++;
    return 1u;
}

static uint32_t read_toc(void *user, schway_toc *out)
{
    struct disc_host *host = (struct disc_host *)user;
    host->toc_calls++;
    memcpy(out, &host->toc, sizeof *out);
    return 1u;
}

static uint32_t bad_q(void *user, uint32_t lba)
{
    struct disc_host *host = (struct disc_host *)user;
    (void)lba;
    host->bad_q_calls++;
    return 0u;
}

static schway_disc make_disc(struct disc_host *host, uint8_t identity_byte)
{
    schway_disc disc;
    memset(&disc, 0, sizeof disc);
    disc.size = (uint32_t)sizeof disc;
    disc.user = host;
    disc.read_sector = read_sector;
    disc.toc = read_toc;
    disc.bad_q = bad_q;
    disc.identity_size = 32u;
    memset(disc.identity_sha256, identity_byte, sizeof disc.identity_sha256);
    return disc;
}

static schway *make_instance(void)
{
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    schway *inst = NULL;
    if (schway_create(&cfg, &inst) != SCHWAY_STATUS_OK) return NULL;
    return inst;
}

static void make_toc(struct disc_host *host)
{
    memset(&host->toc, 0, sizeof host->toc);
    host->toc.size = (uint32_t)sizeof host->toc;
    host->toc.track_count = 1u;
    host->toc.tracks[0].size = (uint32_t)sizeof host->toc.tracks[0];
    host->toc.tracks[0].type = SCHWAY_DISC_TRACK_MODE2;
    host->toc.tracks[0].index0 = 150u;
    host->toc.tracks[0].index1 = 150u;
    host->toc.tracks[0].leadout = 1000u;
}

int main(void)
{
    CHECK(SCHWAY_API_VERSION >= 6);
    CHECK_EQ_U64(SCHWAY_STATUS_ERR_STATE_DISC, 14u);
    CHECK(strcmp(schway_status_string(SCHWAY_STATUS_ERR_STATE_DISC), "state disc identity mismatch") == 0);
    CHECK_EQ_U64(offsetof(schway_disc, identity_size), 8u + 4u * sizeof(void *));

    schway *source = make_instance();
    schway *target = make_instance();
    schway *same = make_instance();
    CHECK(source != NULL && target != NULL && same != NULL);
    if (source == NULL || target == NULL || same == NULL) {
        schway_destroy(source);
        schway_destroy(target);
        schway_destroy(same);
        CHECK_DONE();
    }

    struct disc_host host_a = {0};
    struct disc_host host_b = {0};
    struct disc_host host_same = {0};
    make_toc(&host_a);
    make_toc(&host_b);
    make_toc(&host_same);
    schway_disc disc_a = make_disc(&host_a, 0xA5u);
    schway_disc disc_b = make_disc(&host_b, 0x5Au);
    schway_disc disc_same = make_disc(&host_same, 0xA5u);
    CHECK_EQ_U64(schway_open_disc(source, &disc_a), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_open_disc(target, &disc_b), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_open_disc(same, &disc_same), SCHWAY_STATUS_OK);

    size_t state_size = schway_state_size(source);
    uint8_t *source_state = (uint8_t *)malloc(state_size);
    uint8_t *target_before = (uint8_t *)malloc(state_size);
    uint8_t *target_after = (uint8_t *)malloc(state_size);
    CHECK(state_size != 0u && source_state != NULL && target_before != NULL && target_after != NULL);
    if (source_state == NULL || target_before == NULL || target_after == NULL) {
        free(source_state);
        free(target_before);
        free(target_after);
        schway_destroy(source);
        schway_destroy(target);
        schway_destroy(same);
        CHECK_DONE();
    }
    CHECK_EQ_U64(schway_save_state(source, source_state, state_size), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_save_state(target, target_before, state_size), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_load_state(target, source_state, state_size), SCHWAY_STATUS_ERR_STATE_DISC);
    CHECK_EQ_U64(schway_save_state(target, target_after, state_size), SCHWAY_STATUS_OK);
    CHECK(memcmp(target_before, target_after, state_size) == 0);
    CHECK_EQ_U64(schway_load_state(same, source_state, state_size), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(host_a.toc_calls, 1u);
    CHECK_EQ_U64(host_b.toc_calls, 1u);
    CHECK_EQ_U64(host_same.toc_calls, 1u);
    CHECK_EQ_U64(host_a.read_calls, 0u);
    CHECK_EQ_U64(host_b.read_calls, 0u);
    CHECK_EQ_U64(host_same.read_calls, 0u);
    CHECK_EQ_U64(host_a.bad_q_calls, 0u);
    CHECK_EQ_U64(host_b.bad_q_calls, 0u);
    CHECK_EQ_U64(host_same.bad_q_calls, 0u);

    schway *missing = make_instance();
    CHECK(missing != NULL);
    if (missing == NULL) {
        schway_destroy(source);
        schway_destroy(target);
        schway_destroy(same);
        free(source_state);
        free(target_before);
        free(target_after);
        CHECK_DONE();
    }
    CHECK_EQ_U64(schway_load_state(missing, source_state, state_size), SCHWAY_STATUS_ERR_STATE_DISC);
    schway_destroy(missing);

    schway_destroy(source);
    schway_destroy(target);
    schway_destroy(same);
    free(source_state);
    free(target_before);
    free(target_after);

    schway *unknown = make_instance();
    CHECK(unknown != NULL);
    if (unknown == NULL) CHECK_DONE();
    struct disc_host unknown_host = {0};
    make_toc(&unknown_host);
    schway_disc unknown_disc = make_disc(&unknown_host, 0u);
    unknown_disc.identity_size = 0u;
    memset(unknown_disc.identity_sha256, 0, sizeof unknown_disc.identity_sha256);
    CHECK_EQ_U64(schway_open_disc(unknown, &unknown_disc), SCHWAY_STATUS_OK);
    size_t unknown_size = schway_state_size(unknown);
    uint8_t *unknown_state = (uint8_t *)malloc(unknown_size);
    CHECK(unknown_state != NULL);
    if (unknown_state == NULL) {
        schway_destroy(unknown);
        CHECK_DONE();
    }
    memset(unknown_state, 0xA5, unknown_size);
    CHECK_EQ_U64(schway_save_state(unknown, unknown_state, unknown_size),
                 SCHWAY_STATUS_ERR_STATE_DISC);
    CHECK_EQ_U64(unknown_host.toc_calls, 1u);
    CHECK_EQ_U64(unknown_host.read_calls, 0u);
    CHECK_EQ_U64(unknown_host.bad_q_calls, 0u);
    int unknown_untouched = 1;
    for (size_t i = 0u; i < unknown_size; i++) {
        if (unknown_state[i] != 0xA5u) {
            unknown_untouched = 0;
            break;
        }
    }
    CHECK(unknown_untouched);
    schway_destroy(unknown);
    free(unknown_state);

    struct old_disc {
        uint32_t size;
        uint32_t reserved0;
        void *user;
        schway_disc_read_sector_fn read_sector;
        schway_disc_toc_fn toc;
        schway_disc_bad_q_fn bad_q;
    };
    union {
        struct old_disc v5;
        schway_disc current;
    } legacy;
    struct disc_host legacy_host = {0};
    make_toc(&legacy_host);
    memset(&legacy, 0, sizeof legacy);
    legacy.v5.size = (uint32_t)offsetof(schway_disc, identity_size);
    legacy.v5.user = &legacy_host;
    legacy.v5.read_sector = read_sector;
    legacy.v5.toc = read_toc;
    legacy.v5.bad_q = bad_q;
    schway *legacy_inst = make_instance();
    CHECK(legacy_inst != NULL);
    if (legacy_inst != NULL) {
        CHECK_EQ_U64(schway_open_disc(legacy_inst, &legacy.current), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(legacy_host.toc_calls, 1u);
        schway_destroy(legacy_inst);
    }

    struct disc_host invalid_host = {0};
    make_toc(&invalid_host);
    schway *invalid_inst = make_instance();
    CHECK(invalid_inst != NULL);
    if (invalid_inst != NULL) {
        schway_disc invalid_disc = make_disc(&invalid_host, 0xC3u);
        invalid_disc.identity_size = 31u;
        CHECK_EQ_U64(schway_open_disc(invalid_inst, &invalid_disc), SCHWAY_STATUS_ERR_ARGUMENT);
        invalid_disc.identity_size = 32u;
        invalid_disc.reserved1 = 1u;
        CHECK_EQ_U64(schway_open_disc(invalid_inst, &invalid_disc), SCHWAY_STATUS_ERR_ARGUMENT);
        invalid_disc.reserved1 = 0u;
        invalid_disc.identity_size = 0u;
        CHECK_EQ_U64(schway_open_disc(invalid_inst, &invalid_disc), SCHWAY_STATUS_ERR_ARGUMENT);
        CHECK_EQ_U64(invalid_host.toc_calls, 0u);
        schway_destroy(invalid_inst);
    }

    CHECK_DONE();
}
