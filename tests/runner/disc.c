/* Shared disc-provider integration with the public disc API. */
#include <stdlib.h>
#include <string.h>

#include "disc_image.h"
#include "inputs.h"
#include "internal.h"
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

static void check_script_controls(const schway_disc_image *image, const schway_disc *first,
                                  const schway_disc *second, const char *path)
{
    schway__input_movie movie;
    schway__inputs_error error;
    memset(&movie, 0, sizeof movie);
    memset(&error, 0, sizeof error);
    schway__inputs_status input_status = schway__inputs_load(path, &movie, &error);
    CHECK_EQ_U64(input_status, SCHWAY__INPUTS_OK);
    if (input_status != SCHWAY__INPUTS_OK) return;
    CHECK_EQ_U64(movie.count, 4u);
    CHECK_EQ_U64(schway__inputs_validate_disc_controls(&movie,
                    (uint32_t)schway_disc_image_count(image), &error), SCHWAY__INPUTS_OK);

    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = test_alloc;
    config.allocator.free = test_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&config, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) {
        schway__inputs_release(&movie);
        return;
    }
    CHECK_EQ_U64(schway_open_disc(inst, first), SCHWAY_STATUS_OK);
    uint32_t first_index = inst->disc_host.toc.tracks[1].index1;
    schway_input snapshot;
    memset(&snapshot, 0, sizeof snapshot);
    snapshot.size = (uint32_t)sizeof snapshot;
    size_t cursor = 0u;
    CHECK_EQ_U64(schway__inputs_apply_frame(inst, &movie, &cursor, &snapshot, 299u, image),
                 SCHWAY_STATUS_OK);
    CHECK_EQ_U64(cursor, 0u);
    CHECK(inst->disc_host.callbacks.user == first->user);
    CHECK_EQ_U64(inst->disc_host.lid_open, 0u);
    CHECK_EQ_U64(inst->pad.pending_valid, 0u);

    CHECK_EQ_U64(schway__inputs_apply_frame(inst, &movie, &cursor, &snapshot, 300u, image),
                 SCHWAY_STATUS_OK);
    CHECK_EQ_U64(cursor, 4u);
    CHECK(inst->disc_host.callbacks.user == second->user);
    CHECK(inst->disc_host.toc.tracks[1].index1 != first_index);
    CHECK_EQ_U64(inst->disc_host.lid_open, 0u);
    CHECK_EQ_U64(inst->pad.pending_valid, 1u);
    CHECK_EQ_U64(inst->pad.pending_input.device[0], SCHWAY_INPUT_DEVICE_DIGITAL);
    CHECK_EQ_U64(inst->pad.pending_input.buttons[0], UINT16_C(1) << 14);
    schway_destroy(inst);
    schway__inputs_release(&movie);
}

static int equal_toc(const schway_toc *left, const schway_toc *right)
{
    if (left->track_count != right->track_count) return 0;
    for (uint32_t i = 0u; i < left->track_count; i++) {
        const schway_disc_track *a = &left->tracks[i];
        const schway_disc_track *b = &right->tracks[i];
        if (a->type != b->type || a->index0 != b->index0 || a->index1 != b->index1 ||
            a->pregap != b->pregap || a->leadout != b->leadout) return 0;
    }
    return 1;
}

static void check_chd_provider(const char *cue_path, const char *chd_path,
                               const char *invalid_path, int chd_enabled)
{
    schway_disc_image *cue_image = NULL;
    schway_disc_image *chd_image = NULL;
    schway_disc_image *invalid_image = NULL;
    schway_disc_image_error error;
    const schway_disc *cue = NULL;
    const schway_disc *chd = NULL;
    schway_toc cue_toc, chd_toc;
    memset(&error, 0, sizeof error);
    error.size = (uint32_t)sizeof error;
    CHECK_EQ_U64(schway_disc_image_open(cue_path, &cue_image, &error), SCHWAY_DISC_IMAGE_OK);
    CHECK(cue_image != NULL);
    if (cue_image == NULL) return;
    CHECK_EQ_U64(schway_disc_image_count(cue_image), 1u);
    cue = schway_disc_image_disc(cue_image);
    CHECK(cue != NULL && cue->read_sector != NULL && cue->toc != NULL);
    if (cue == NULL || cue->read_sector == NULL || cue->toc == NULL) {
        schway_disc_image_close(cue_image);
        return;
    }

    memset(&error, 0, sizeof error);
    error.size = (uint32_t)sizeof error;
    schway_disc_image_status chd_status = schway_disc_image_open(chd_path, &chd_image, &error);
    if (!chd_enabled) {
        CHECK_EQ_U64(chd_status, SCHWAY_DISC_IMAGE_UNSUPPORTED);
        CHECK(chd_image == NULL);
        schway_disc_image_close(cue_image);
        return;
    }
    CHECK_EQ_U64(chd_status, SCHWAY_DISC_IMAGE_OK);
    CHECK(chd_image != NULL);
    if (chd_image == NULL) {
        schway_disc_image_close(cue_image);
        return;
    }
    CHECK_EQ_U64(schway_disc_image_count(chd_image), 1u);
    chd = schway_disc_image_disc(chd_image);
    CHECK(chd != NULL && chd->read_sector != NULL && chd->toc != NULL);
    if (chd == NULL || chd->read_sector == NULL || chd->toc == NULL) {
        schway_disc_image_close(chd_image);
        schway_disc_image_close(cue_image);
        return;
    }
    memset(&cue_toc, 0, sizeof cue_toc);
    memset(&chd_toc, 0, sizeof chd_toc);
    cue_toc.size = (uint32_t)sizeof cue_toc;
    chd_toc.size = (uint32_t)sizeof chd_toc;
    CHECK_EQ_U64(cue->toc(cue->user, &cue_toc), 1u);
    CHECK_EQ_U64(chd->toc(chd->user, &chd_toc), 1u);
    CHECK(equal_toc(&cue_toc, &chd_toc));
    for (uint32_t track = 0u; track < cue_toc.track_count; track++) {
        for (uint32_t lba = cue_toc.tracks[track].index1;
             lba < cue_toc.tracks[track].leadout; lba++) {
            uint8_t cue_sector[2352], chd_sector[2352];
            uint8_t cue_subcode[96], chd_subcode[96];
            uint32_t cue_ok = cue->read_sector(cue->user, lba, cue_sector, cue_subcode);
            uint32_t chd_ok = chd->read_sector(chd->user, lba, chd_sector, chd_subcode);
            CHECK_EQ_U64(cue_ok, 1u);
            CHECK_EQ_U64(chd_ok, 1u);
            if (cue_ok != 0u && chd_ok != 0u) {
                CHECK(memcmp(cue_sector, chd_sector, sizeof cue_sector) == 0);
                CHECK(memcmp(cue_subcode, chd_subcode, sizeof cue_subcode) == 0);
            }
        }
    }

    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = test_alloc;
    config.allocator.free = test_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&config, &inst), SCHWAY_STATUS_OK);
    CHECK(inst != NULL);
    if (inst == NULL) {
        schway_disc_image_close(chd_image);
        schway_disc_image_close(cue_image);
        return;
    }
    CHECK_EQ_U64(schway_open_disc(inst, cue), SCHWAY_STATUS_OK);
    struct schway__disc_host before = inst->disc_host;
    CHECK_EQ_U64(schway_swap_disc(inst, chd), SCHWAY_STATUS_ERR_LID_CLOSED);
    CHECK(memcmp(&before, &inst->disc_host, sizeof before) == 0);
    CHECK_EQ_U64(schway_set_lid(inst, 1u), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_swap_disc(inst, chd), SCHWAY_STATUS_OK);
    CHECK(inst->disc_host.callbacks.user == chd->user);
    before = inst->disc_host;
    memset(&error, 0, sizeof error);
    error.size = (uint32_t)sizeof error;
    CHECK_EQ_U64(schway_disc_image_open(invalid_path, &invalid_image, &error),
                 SCHWAY_DISC_IMAGE_BAD_IMAGE);
    CHECK(invalid_image == NULL);
    CHECK(memcmp(&before, &inst->disc_host, sizeof before) == 0);
    CHECK_EQ_U64(schway_set_lid(inst, 0u), SCHWAY_STATUS_OK);
    schway_destroy(inst);
    schway_disc_image_close(invalid_image);
    schway_disc_image_close(chd_image);
    schway_disc_image_close(cue_image);
}

int main(int argc, char **argv)
{
    CHECK(argc == 2 || argc == 3 || argc == 6);
    if (argc < 2 || argc > 6) CHECK_DONE();

    if (argc == 6 && strcmp(argv[1], "chd") == 0) {
        check_chd_provider(argv[2], argv[3], argv[4], argv[5][0] == '1');
        CHECK_DONE();
    }

    schway_disc_image *image = NULL;
    schway_disc_image_error image_error;
    memset(&image_error, 0, sizeof image_error);
    CHECK_EQ_U64(schway_disc_image_open(argv[1], &image, &image_error), SCHWAY_DISC_IMAGE_OK);
    CHECK(image != NULL);
    if (image == NULL) CHECK_DONE();
    CHECK_EQ_U64(schway_disc_image_count(image), 2u);

    const schway_disc *first = NULL;
    const schway_disc *second = NULL;
    CHECK_EQ_U64(schway_disc_image_select(image, 0u, &first), SCHWAY_DISC_IMAGE_OK);
    CHECK_EQ_U64(schway_disc_image_select(image, 1u, &second), SCHWAY_DISC_IMAGE_OK);
    CHECK(first != NULL && second != NULL);
    if (first == NULL || second == NULL) {
        schway_disc_image_close(image);
        CHECK_DONE();
    }

    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = test_alloc;
    config.allocator.free = test_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&config, &inst), SCHWAY_STATUS_OK);
    CHECK(inst != NULL);
    if (inst == NULL) {
        schway_disc_image_close(image);
        CHECK_DONE();
    }
    CHECK_EQ_U64(schway_open_disc(inst, first), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(inst->disc_host.toc.track_count, 2u);
    uint32_t first_audio_index = inst->disc_host.toc.tracks[1].index1;
    struct schway__disc_host before = inst->disc_host;
    CHECK_EQ_U64(schway_swap_disc(inst, second), SCHWAY_STATUS_ERR_LID_CLOSED);
    CHECK(memcmp(&before, &inst->disc_host, sizeof before) == 0);

    CHECK_EQ_U64(schway_set_lid(inst, 1u), SCHWAY_STATUS_OK);
    before = inst->disc_host;
    CHECK_EQ_U64(schway_swap_disc(inst, second), SCHWAY_STATUS_OK);
    CHECK(inst->disc_host.callbacks.user == second->user);
    CHECK(inst->disc_host.toc.tracks[1].index1 != first_audio_index);
    CHECK(memcmp(&before, &inst->disc_host, sizeof before) != 0);
    CHECK_EQ_U64(schway_set_lid(inst, 0u), SCHWAY_STATUS_OK);

    schway_destroy(inst);
    if (argc == 3) check_script_controls(image, first, second, argv[2]);
    schway_disc_image_close(image);
    CHECK_DONE();
}
