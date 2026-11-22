#include "disc_ecm.h"
#include "disc_image.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../test_open.h"

static int make_path(char path[1024], const char *directory, const char *name)
{
    int length = snprintf(path, 1024u, "%s/%s", directory, name);
    return length >= 0 && (size_t)length < 1024u;
}

static int write_file(const char *path, const uint8_t *bytes, size_t size)
{
    FILE *file = schway__test_open_file(path, "wb");
    int ok;
    if (file == NULL) return 0;
    ok = size == 0u || fwrite(bytes, 1u, size, file) == size;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static int expect_open(const char *path, schway_disc_image_status expected,
                       const char *name)
{
    schway_disc_image_error error;
    schway__disc_ecm *reader;
    int sentinel;
    schway_disc_image_status status;
    reader = (schway__disc_ecm *)(void *)&sentinel;
    memset(&error, 0, sizeof error);
    status = schway__disc_ecm_open(path, &reader, &error);
    if (reader != NULL && reader != (schway__disc_ecm *)(void *)&sentinel) {
        schway__disc_ecm_close(reader);
    }
    if (status == expected && reader == NULL &&
        error.size == (uint32_t)sizeof error && error.status == (uint32_t)expected) return 1;
    fprintf(stderr, "ASSERT disc.image.ecm: %s expected status %u and null reader, got status %u, reader %s\n",
            name, (unsigned int)expected, (unsigned int)status,
            reader == NULL ? "null" : "non-null");
    return 0;
}

static int write_active_iso(const char *path)
{
    uint8_t sector[2048];
    FILE *file = schway__test_open_file(path, "wb");
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

static int active_iso_is_readable(schway_disc_image *image,
                                  const schway_disc *original_disc)
{
    const schway_disc *disc = schway_disc_image_disc(image);
    uint8_t sector[2352];
    uint8_t subcode[96];
    return disc == original_disc && disc != NULL && disc->read_sector != NULL &&
           disc->read_sector(disc->user, 3u, sector, subcode) == 1u &&
           sector[16] == 21u;
}

int main(int argc, char **argv)
{
    static const uint8_t bad_magic[] = { 'B', 'A', 'D', '!' };
    static const uint8_t short_signature[] = { 'E', 'C', 'M', 0 };
    static const uint8_t short_record[] = { 'E', 'C', 'M', 0, 0x80u };
    char input_path[1024];
    char iso_path[1024];
    schway_disc_image *active = NULL;
    schway_disc_image_error error;
    const schway_disc *active_disc = NULL;
    int result = 0;

    if (argc != 2 || !make_path(input_path, argv[1], "disc-ecm-input.bin") ||
        !make_path(iso_path, argv[1], "disc-ecm-active.iso")) {
        fprintf(stderr, "ASSERT disc.image.ecm: expected a writable test directory\n");
        return 1;
    }
    if (!write_active_iso(iso_path) ||
        schway_disc_image_open(iso_path, &active, &error) != SCHWAY_DISC_IMAGE_OK) {
        fprintf(stderr, "ASSERT disc.image.ecm: could not open the existing ISO source\n");
        result = 1;
        goto done;
    }
    active_disc = schway_disc_image_disc(active);
    if (!write_file(input_path, bad_magic, sizeof bad_magic) ||
        !expect_open(input_path, SCHWAY_DISC_IMAGE_BAD_IMAGE, "bad signature")) result = 1;
    if (!write_file(input_path, short_signature, sizeof short_signature) ||
        !expect_open(input_path, SCHWAY_DISC_IMAGE_BAD_IMAGE, "truncated signature")) result = 1;
    if (!write_file(input_path, short_record, sizeof short_record) ||
        !expect_open(input_path, SCHWAY_DISC_IMAGE_BAD_IMAGE, "truncated record count")) result = 1;
    if (!active_iso_is_readable(active, active_disc)) {
        fprintf(stderr, "ASSERT disc.image.ecm: failed open changed the active ISO source\n");
        result = 1;
    }

done:
    schway_disc_image_close(active);
    (void)remove(input_path);
    (void)remove(iso_path);
    return result;
}
