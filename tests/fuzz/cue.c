/* Bounded replay of hostile cue sheets through the production image parser. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disc_image.h"
#include "../test_open.h"

#ifndef SCHWAY_FUZZ_WORK_DIR
#error "CMake must provide a build-owned cue fuzz directory"
#endif

#define INPUT_MAX (64u * 1024u)

static void broken(const char *what)
{
    fprintf(stderr, "fuzz.regress.cue: %s\n", what);
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

static int matches(const uint8_t *bytes, size_t size, const char *text)
{
    size_t expected = strlen(text);
    return size == expected && memcmp(bytes, text, expected) == 0;
}

static void make_track(const char *path)
{
    static int ready;
    uint8_t sectors[16u * 2048u] = {0};
    if (ready == 0) {
        if (!write_file(path, sectors, sizeof sectors)) {
            broken("could not write bounded track fixture");
        }
        ready = 1;
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char cue_path[1024];
    char track_path[1024];
    int cue_len = snprintf(cue_path, sizeof cue_path, "%s/input.cue", SCHWAY_FUZZ_WORK_DIR);
    int track_len = snprintf(track_path, sizeof track_path, "%s/track.bin", SCHWAY_FUZZ_WORK_DIR);
    if (cue_len < 0 || (size_t)cue_len >= sizeof cue_path || track_len < 0 ||
        (size_t)track_len >= sizeof track_path) {
        broken("build-owned path did not fit");
    }
    if (size > INPUT_MAX) return 0;
    make_track(track_path);
    if (!write_file(cue_path, data, size)) broken("could not write bounded cue input");

    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    schway_disc_image_status status = schway_disc_image_open(cue_path, &image, &error);
    static const char valid[] = "FILE \"track.bin\" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 00:00:00\n";
    static const char escape[] = "FILE \"../outside.bin\" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 00:00:00\n";
    if ((matches(data, size, valid) && status != SCHWAY_DISC_IMAGE_OK) ||
        (matches(data, size, escape) && status != SCHWAY_DISC_IMAGE_BAD_CUE)) {
        fprintf(stderr, "cue seed status %u: %s\n", (unsigned int)status, error.message);
        schway_disc_image_close(image);
        broken("known valid or escaping cue seed changed its parser result");
    }
    if (status == SCHWAY_DISC_IMAGE_OK) {
        if (image == NULL || schway_disc_image_count(image) != 1u ||
            schway_disc_image_disc(image) == NULL) {
            schway_disc_image_close(image);
            broken("successful cue parse returned an invalid image");
        }
        uint8_t sector[2352];
        uint8_t subcode[96];
        const schway_disc *disc = schway_disc_image_disc(image);
        if (disc->read_sector(disc->user, 0u, sector, subcode) == 0u) {
            schway_disc_image_close(image);
            broken("bounded cue sector read failed");
        }
    } else if (image != NULL) {
        schway_disc_image_close(image);
        broken("failed cue parse retained an image");
    }
    schway_disc_image_close(image);
    return 0;
}

#define SCHWAY_REPLAY_NAME "fuzz.regress.cue"
#include "replay_main.h"
