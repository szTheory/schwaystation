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

static uint32_t ecm_edc(const uint8_t *bytes, size_t size)
{
    uint32_t value = 0u;
    for (size_t i = 0u; i < size; i++) {
        value ^= bytes[i];
        for (uint32_t bit = 0u; bit < 8u; bit++) {
            value = (value >> 1) ^ ((value & 1u) != 0u ? UINT32_C(0xd8018001) : 0u);
        }
    }
    return value;
}

static int write_count(FILE *file, uint32_t count)
{
    uint32_t encoded = count - 1u;
    uint8_t byte = (uint8_t)((encoded & 0x1fu) << 2);
    encoded >>= 5;
    if (encoded != 0u) byte |= 0x80u;
    if (fwrite(&byte, 1u, 1u, file) != 1u) return 0;
    while (encoded != 0u) {
        byte = (uint8_t)(encoded & 0x7fu);
        encoded >>= 7;
        if (encoded != 0u) byte |= 0x80u;
        if (fwrite(&byte, 1u, 1u, file) != 1u) return 0;
    }
    return 1;
}

static int write_literal_ecm(const char *path, const uint8_t *bytes, size_t size,
                             int bad_checksum)
{
    static const uint8_t terminator[] = { 0xfcu, 0xffu, 0xffu, 0xffu, 0x3fu };
    const size_t split = 73u;
    uint32_t edc = ecm_edc(bytes, size);
    uint8_t checksum[4] = {(uint8_t)edc, (uint8_t)(edc >> 8),
                           (uint8_t)(edc >> 16), (uint8_t)(edc >> 24)};
    FILE *file = schway__test_open_file(path, "wb");
    int ok;
    if (file == NULL) return 0;
    if (bad_checksum) checksum[0] ^= 1u;
    ok = fwrite("ECM\0", 1u, 4u, file) == 4u &&
         write_count(file, (uint32_t)split) &&
         fwrite(bytes, 1u, split, file) == split &&
         write_count(file, (uint32_t)(size - split)) &&
         fwrite(bytes + split, 1u, size - split, file) == size - split &&
         fwrite(terminator, 1u, sizeof terminator, file) == sizeof terminator &&
         fwrite(checksum, 1u, sizeof checksum, file) == sizeof checksum;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static int public_image_matches(const char *path, const uint8_t expected[2352])
{
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    const schway_disc *disc;
    schway_toc toc;
    uint8_t sector[2352];
    uint8_t subcode[96];
    int ok = schway_disc_image_open(path, &image, &error) == SCHWAY_DISC_IMAGE_OK;
    if (!ok) {
        fprintf(stderr, "ASSERT disc.image.ecm: valid literal ECM open failed: %s\n", error.message);
        return 0;
    }
    disc = schway_disc_image_disc(image);
    memset(&toc, 0, sizeof toc);
    toc.size = (uint32_t)sizeof toc;
    ok = disc != NULL && disc->toc != NULL && disc->read_sector != NULL &&
         disc->toc(disc->user, &toc) == 1u && toc.track_count == 1u &&
         toc.tracks[0].type == SCHWAY_DISC_TRACK_MODE2 && toc.tracks[0].leadout == 1u &&
         disc->read_sector(disc->user, 0u, sector, subcode) == 1u &&
         memcmp(sector, expected, sizeof sector) == 0;
    if (!ok) fprintf(stderr, "ASSERT disc.image.ecm: public sector or fallback TOC differs\n");
    schway_disc_image_close(image);
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
    char playlist_path[1024];
    uint8_t expected[2352];
    schway_disc_image *active = NULL;
    schway_disc_image_error error;
    const schway_disc *active_disc = NULL;
    int result = 0;

    if (argc != 2 || !make_path(input_path, argv[1], "disc-ecm-input.EcM") ||
        !make_path(iso_path, argv[1], "disc-ecm-active.iso") ||
        !make_path(playlist_path, argv[1], "disc-ecm-input.m3u")) {
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
    for (size_t i = 0u; i < sizeof expected; i++) expected[i] = (uint8_t)(i * 29u + 7u);
    if (!write_literal_ecm(input_path, expected, sizeof expected, 0) ||
        !public_image_matches(input_path, expected)) result = 1;
    static const uint8_t playlist[] = "disc-ecm-input.EcM\n";
    if (!write_file(playlist_path, playlist, sizeof playlist - 1u) ||
        !public_image_matches(playlist_path, expected)) result = 1;
    schway__disc_ecm *reader = NULL;
    if (schway__disc_ecm_open(input_path, &reader, &error) != SCHWAY_DISC_IMAGE_OK) {
        fprintf(stderr, "ASSERT disc.image.ecm: valid reader open failed for byte spans\n");
        result = 1;
    } else {
        uint8_t span[41];
        uint8_t edge;
        if (schway__disc_ecm_read_bytes(reader, 60u, span, sizeof span) != SCHWAY_DISC_IMAGE_OK ||
            memcmp(span, expected + 60u, sizeof span) != 0 ||
            schway__disc_ecm_read_bytes(reader, 0u, &edge, 1u) != SCHWAY_DISC_IMAGE_OK ||
            edge != expected[0] ||
            schway__disc_ecm_read_bytes(reader, sizeof expected - 1u, &edge, 1u) != SCHWAY_DISC_IMAGE_OK ||
            edge != expected[sizeof expected - 1u] ||
            schway__disc_ecm_read_bytes(reader, sizeof expected, &edge, 1u) != SCHWAY_DISC_IMAGE_BAD_ARGUMENT) {
            fprintf(stderr, "ASSERT disc.image.ecm: split-record or boundary byte read differed\n");
            result = 1;
        }
        schway__disc_ecm_close(reader);
    }
    if (!write_literal_ecm(input_path, expected, sizeof expected, 1) ||
        !expect_open(input_path, SCHWAY_DISC_IMAGE_BAD_IMAGE, "mutated EDC") ||
        !active_iso_is_readable(active, active_disc)) {
        fprintf(stderr, "ASSERT disc.image.ecm: bad EDC published or disturbed a source\n");
        result = 1;
    }
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
    (void)remove(playlist_path);
    return result;
}
