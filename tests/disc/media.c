/* Cheap callback-level checks for the owned CUE before any guest boot. */
#include "disc_image.h"
#include "../test_open.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition, message) do { \
    if (!(condition)) { \
        fprintf(stderr, "ASSERT disc.media.preflight: %s\n", (message)); \
        result = 1; \
        goto cleanup; \
    } \
} while (0)

static int read_boundary(const schway_disc *disc, uint32_t lba)
{
    uint8_t sector[2354];
    uint8_t subcode[98];

    memset(sector, 0xa5, sizeof sector);
    memset(subcode, 0xa5, sizeof subcode);
    if (disc->read_sector == NULL ||
        disc->read_sector(disc->user, lba, sector + 1u, subcode + 1u) != 1u ||
        sector[0] != 0xa5u || sector[2353] != 0xa5u ||
        subcode[0] != 0xa5u || subcode[97] != 0xa5u) {
        fprintf(stderr, "ASSERT disc.media.preflight: unsafe or unreadable LBA %u\n", lba);
        return 0;
    }
    for (size_t i = 1u; i <= 96u; i++) {
        if (subcode[i] != 0u) {
            fprintf(stderr, "ASSERT disc.media.preflight: unexpected subcode byte at LBA %u\n",
                    lba);
            return 0;
        }
    }
    return 1;
}

int main(int argc, char **argv)
{
    schway_disc_image *image = NULL;
    schway_disc_image *empty_image = NULL;
    schway_disc_image_error error;
    const schway_disc *disc;
    schway_toc toc;
    char empty_path[1024] = "";
    FILE *empty_file = NULL;
    int result = 0;

    CHECK(argc == 3, "expected the owned CUE path and a writable test directory");
    int path_size = snprintf(empty_path, sizeof empty_path,
                             "%s/schway-disc-empty.cue", argv[2]);
    CHECK(path_size > 0 && (size_t)path_size < sizeof empty_path,
          "could not form empty-input path");

    memset(&error, 0, sizeof error);
    CHECK(schway_disc_image_open(argv[1], &image, &error) == SCHWAY_DISC_IMAGE_OK,
          "owned CUE did not open through the production provider");
    disc = schway_disc_image_disc(image);
    memset(&toc, 0, sizeof toc);
    toc.size = (uint32_t)sizeof toc;
    CHECK(disc != NULL && disc->toc != NULL && disc->toc(disc->user, &toc) == 1u,
          "provider did not return a TOC");
    CHECK(toc.track_count == 2u,
          "owned CUE geometry changed: expected exactly two tracks");

    CHECK(toc.tracks[0].type == SCHWAY_DISC_TRACK_MODE2 &&
          toc.tracks[0].index0 == 0u && toc.tracks[0].index1 == 0u &&
          toc.tracks[0].pregap == 0u && toc.tracks[0].leadout == 64u,
          "track 1 geometry differs from the independently authored 64-sector data track");
    CHECK(toc.tracks[1].type == SCHWAY_DISC_TRACK_AUDIO &&
          toc.tracks[1].index0 == 64u && toc.tracks[1].index1 == 214u &&
          toc.tracks[1].pregap == 150u && toc.tracks[1].leadout == 222u,
          "track 2 geometry differs from the authored 150-sector pregap and 8-sector program");
    CHECK(toc.tracks[0].leadout == toc.tracks[1].index0 &&
          toc.tracks[0].leadout < toc.tracks[1].index1 &&
          toc.tracks[1].index1 < toc.tracks[1].leadout,
          "adjacent track boundaries or exclusive leadout are inconsistent");

    CHECK(read_boundary(disc, 0u) && read_boundary(disc, 63u) &&
          read_boundary(disc, 64u) && read_boundary(disc, 214u) &&
          read_boundary(disc, 221u),
          "a first, touching, index, or final valid sector failed its bounded read");
    uint8_t sector[2352];
    uint8_t subcode[96];
    CHECK(disc->read_sector(disc->user, 222u, sector, subcode) == 0u,
          "exclusive leadout must not be readable as a sector");

    CHECK(schway_disc_image_open("", &empty_image, &error) == SCHWAY_DISC_IMAGE_BAD_ARGUMENT &&
          empty_image == NULL,
          "empty path must be rejected without returning an image");
    empty_file = schway__test_open_file(empty_path, "wb");
    CHECK(empty_file != NULL, "could not create an empty CUE input");
    int close_result = fclose(empty_file);
    empty_file = NULL;
    CHECK(close_result == 0, "could not finish creating an empty CUE input");
    CHECK(schway_disc_image_open(empty_path, &empty_image, &error) != SCHWAY_DISC_IMAGE_OK &&
          empty_image == NULL,
          "empty CUE input must be rejected without returning an image");

cleanup:
    if (empty_file != NULL) (void)fclose(empty_file);
    schway_disc_image_close(empty_image);
    schway_disc_image_close(image);
    if (empty_path[0] != '\0') (void)remove(empty_path);
    return result;
}
