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

int main(int argc, char **argv)
{
    CHECK(argc == 2 || argc == 3);
    if (argc < 2 || argc > 3) CHECK_DONE();

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
