/* Host disc image tests create their input images under the build directory. */
#include "disc_image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition, message) do { \
    if (!(condition)) { \
        fprintf(stderr, "ASSERT %s: %s\n", test_case, (message)); \
        result = 1; \
        goto cleanup; \
    } \
} while (0)

static int write_iso(const char *path)
{
    FILE *file = fopen(path, "wb");
    uint8_t sector[2048];
    if (file == NULL) return 0;
    for (uint32_t lba = 0u; lba < 16u; lba++) {
        for (uint32_t i = 0u; i < (uint32_t)sizeof sector; i++) {
            sector[i] = (uint8_t)(lba * 7u + i);
        }
        if (fwrite(sector, 1u, sizeof sector, file) != sizeof sector) {
            (void)fclose(file);
            return 0;
        }
    }
    return fclose(file) == 0;
}

static int test_iso_sector(const char *directory)
{
    char path[1024];
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    const schway_disc *disc;
    schway_toc toc;
    uint8_t sector[2352];
    uint8_t subcode[96];
    uint8_t expected[2048];
    int result = 0;
    const char *test_case = "disc.image.iso_sector";

    int path_size = snprintf(path, sizeof path, "%s/schway-disc-image-test.iso", directory);
    if (path_size < 0 || (size_t)path_size >= sizeof path || !write_iso(path)) {
        fprintf(stderr, "ASSERT %s: could not create temporary ISO\n", test_case);
        return 1;
    }
    memset(&error, 0, sizeof error);
    CHECK(schway_disc_image_open(path, &image, &error) == SCHWAY_DISC_IMAGE_OK,
          "expected a 16-sector cooked ISO to open");
    disc = schway_disc_image_disc(image);
    CHECK(disc != NULL && disc->read_sector != NULL && disc->toc != NULL,
          "expected a complete installed disc descriptor");
    memset(&toc, 0, sizeof toc);
    toc.size = (uint32_t)sizeof toc;
    CHECK(disc->toc(disc->user, &toc) == 1u && toc.track_count == 1u,
          "expected a one-track TOC");
    CHECK(toc.tracks[0].type == SCHWAY_DISC_TRACK_MODE1 &&
          toc.tracks[0].index0 == 0u && toc.tracks[0].index1 == 0u &&
          toc.tracks[0].leadout == 16u,
          "expected cooked ISO TOC offsets in sectors");
    memset(sector, 0xa5, sizeof sector);
    memset(subcode, 0x5a, sizeof subcode);
    CHECK(disc->read_sector(disc->user, 3u, sector, subcode) == 1u,
          "expected sector callback success");
    for (uint32_t i = 0u; i < (uint32_t)sizeof expected; i++) {
        expected[i] = (uint8_t)(3u * 7u + i);
    }
    CHECK(sector[0] == 0u && sector[1] == 0xffu && sector[10] == 0xffu &&
          sector[11] == 0u && sector[12] == 0u && sector[13] == 2u &&
          sector[14] == 3u && sector[15] == 1u,
          "expected normalized MODE1 raw sync and header");
    CHECK(memcmp(sector + 16u, expected, sizeof expected) == 0,
          "expected the exact cooked sector payload at the raw user-data offset");
    CHECK(sector[2064] == 0xe7u && sector[2065] == 0x22u &&
          sector[2066] == 0x16u && sector[2067] == 0xfeu &&
          sector[2076] == 0xebu && sector[2077] == 0x0fu &&
          sector[2351] == 0xeau,
          "expected the MODE1 EDC and parity in the raw-sector trailer");
    CHECK(subcode[0] == 0u && subcode[95] == 0u,
          "expected absent subcode to be zero filled");
    memset(sector, 0xa5, sizeof sector);
    memset(subcode, 0x5a, sizeof subcode);
    CHECK(disc->read_sector(disc->user, 16u, sector, subcode) == 0u,
          "expected EOF to fail without reading past the image");
    CHECK(sector[0] == 0xa5u && sector[2351] == 0xa5u &&
          subcode[0] == 0x5au && subcode[95] == 0x5au,
          "expected EOF not to modify callback buffers");

cleanup:
    schway_disc_image_close(image);
    (void)remove(path);
    return result;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fputs("usage: schwaystation-test-disc-image CASE BUILD_DIRECTORY\n", stderr);
        return 2;
    }
    if (strcmp(argv[1], "iso_sector") == 0) return test_iso_sector(argv[2]);
    fprintf(stderr, "unknown disc image test case: %s\n", argv[1]);
    return 2;
}
