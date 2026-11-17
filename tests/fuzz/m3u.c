/* Bounded replay of hostile playlists through the production image parser. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disc_image.h"
#include "../test_open.h"

#ifndef SCHWAY_FUZZ_WORK_DIR
#error "CMake must provide a build-owned m3u fuzz directory"
#endif

#define INPUT_MAX (64u * 1024u)

static void broken(const char *what)
{
    fprintf(stderr, "fuzz.regress.m3u: %s\n", what);
    abort();
}

static int write_file(const char *path, const uint8_t *bytes, size_t size)
{
    FILE *file = schway__test_open_file(path, "wb");
    if (file == NULL) return 0;
    int ok = size == 0u || fwrite(bytes, 1u, size, file) == size;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static void make_iso(const char *path)
{
    static int ready;
    uint8_t sectors[16u * 2048u] = {0};
    if (ready == 0) {
        if (!write_file(path, sectors, sizeof sectors)) broken("could not write bounded ISO fixture");
        ready = 1;
    }
}

static int matches(const uint8_t *bytes, size_t size, const char *text)
{
    size_t expected = strlen(text);
    return size == expected && memcmp(bytes, text, expected) == 0;
}

static int matches_entries(const uint8_t *bytes, size_t size, uint32_t count)
{
    static const char entry[] = "disc.iso\n";
    if (size != (sizeof entry - 1u) * count) return 0;
    for (uint32_t i = 0u; i < count; i++) {
        if (memcmp(bytes + i * (sizeof entry - 1u), entry, sizeof entry - 1u) != 0) return 0;
    }
    return 1;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char playlist_path[1024];
    char iso_path[1024];
    int playlist_len = snprintf(playlist_path, sizeof playlist_path, "%s/input.m3u",
                                SCHWAY_FUZZ_WORK_DIR);
    int iso_len = snprintf(iso_path, sizeof iso_path, "%s/disc.iso", SCHWAY_FUZZ_WORK_DIR);
    if (playlist_len < 0 || (size_t)playlist_len >= sizeof playlist_path || iso_len < 0 ||
        (size_t)iso_len >= sizeof iso_path) {
        broken("build-owned path did not fit");
    }
    if (size > INPUT_MAX) return 0;
    make_iso(iso_path);
    if (!write_file(playlist_path, data, size)) broken("could not write bounded playlist input");

    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    schway_disc_image_status status = schway_disc_image_open(playlist_path, &image, &error);
    int single = matches(data, size, "disc.iso\n");
    int escape = matches(data, size, "../outside.iso\n");
    int cycle = matches(data, size, "input.m3u\n");
    int thirty_two = matches_entries(data, size, 32u);
    int thirty_three = matches_entries(data, size, 33u);
    if ((single && status != SCHWAY_DISC_IMAGE_OK) ||
        (escape && status != SCHWAY_DISC_IMAGE_BAD_M3U) ||
        (cycle && status != SCHWAY_DISC_IMAGE_BAD_M3U) ||
        (thirty_two && status != SCHWAY_DISC_IMAGE_OK) ||
        (thirty_three && status != SCHWAY_DISC_IMAGE_LIMIT)) {
        fprintf(stderr, "m3u seed status %u: %s\n", (unsigned int)status, error.message);
        schway_disc_image_close(image);
        broken("known playlist boundary seed changed its parser result");
    }
    if (status == SCHWAY_DISC_IMAGE_OK) {
        if (image == NULL) broken("successful playlist parse returned no image");
        uint32_t count = schway_disc_image_count(image);
        if (count == 0u || count > SCHWAY_DISC_IMAGE_PLAYLIST_LIMIT) {
            schway_disc_image_close(image);
            broken("successful playlist parse returned an invalid entry count");
        }
        for (uint32_t i = 0u; i < count; i++) {
            const schway_disc *disc = NULL;
            if (schway_disc_image_select(image, i, &disc) != SCHWAY_DISC_IMAGE_OK || disc == NULL) {
                schway_disc_image_close(image);
                broken("successful playlist entry could not be selected");
            }
            uint8_t sector[2352];
            uint8_t subcode[96];
            if (disc->read_sector(disc->user, 0u, sector, subcode) == 0u) {
                schway_disc_image_close(image);
                broken("bounded playlist sector read failed");
            }
        }
    } else if (image != NULL) {
        schway_disc_image_close(image);
        broken("failed playlist parse retained an image");
    }
    schway_disc_image_close(image);
    return 0;
}

#define SCHWAY_REPLAY_NAME "fuzz.regress.m3u"
#include "replay_main.h"
