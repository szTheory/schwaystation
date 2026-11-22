#include "disc_ccd.h"
#include "disc_image.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if !defined(_WIN32) && !defined(__wasi__)
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "../test_open.h"

static int make_path(char path[1024], const char *directory, const char *name)
{
    int length = snprintf(path, 1024u, "%s/%s", directory, name);
    return length >= 0 && (size_t)length < 1024u;
}

static int write_file(const char *path, const char *text)
{
    FILE *file = schway__test_open_file(path, "wb");
    size_t size = strlen(text);
    int ok;
    if (file == NULL) return 0;
    ok = fwrite(text, 1u, size, file) == size;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static uint8_t fixture_byte(uint32_t lba, size_t offset)
{
    return (uint8_t)(UINT32_C(0x51) + lba * UINT32_C(37) +
                     (uint32_t)offset * UINT32_C(13));
}

static int write_raw_image(const char *path, uint64_t image_bytes)
{
    uint8_t sector[2352];
    FILE *file = schway__test_open_file(path, "wb");
    uint64_t written = 0u;
    uint32_t lba = 0u;
    if (file == NULL) return 0;
    while (written < image_bytes) {
        uint64_t remaining = image_bytes - written;
        size_t chunk = remaining < sizeof sector ? (size_t)remaining : sizeof sector;
        for (size_t i = 0u; i < sizeof sector; i++) sector[i] = fixture_byte(lba, i);
        if (fwrite(sector, 1u, chunk, file) != chunk) {
            (void)fclose(file);
            return 0;
        }
        written += chunk;
        lba++;
    }
    return fclose(file) == 0;
}

static int replace_file_text(const char *path, const char *needle, const char *replacement)
{
    char contents[4096];
    FILE *file = schway__test_open_file(path, "rb");
    size_t size;
    size_t needle_size = strlen(needle);
    size_t replacement_size = strlen(replacement);
    char *match;
    int read_error;
    int close_error;
    if (file == NULL) return 0;
    size = fread(contents, 1u, sizeof contents - 1u, file);
    read_error = ferror(file);
    close_error = fclose(file);
    if (read_error || size == sizeof contents - 1u || close_error != 0) return 0;
    contents[size] = '\0';
    match = strstr(contents, needle);
    if (match == NULL || size - needle_size + replacement_size >= sizeof contents) return 0;
    memmove(match + replacement_size, match + needle_size,
            size - (size_t)(match - contents) - needle_size + 1u);
    memcpy(match, replacement, replacement_size);
    return write_file(path, contents);
}

static int write_descriptor(const char *path, uint32_t mode,
                            uint32_t index1, int have_index0)
{
    char descriptor[4096];
    uint32_t disc_type = mode == 2u ? 32u : 0u;
    uint32_t a0_plba = (UINT32_C(60) + disc_type) * UINT32_C(75) - UINT32_C(150);
    int length = snprintf(descriptor, sizeof descriptor,
        "[CloneCD]\nVersion=3\n"
        "[Disc]\nTocEntries=4\nSessions=1\nDataTracksScrambled=0\nCDTextLength=0\n"
        "[Session 1]\nPreGapMode=0\nPreGapSubC=0\n"
        "[Entry 0]\nSession=1\nPoint=0xa0\nADR=0x01\nControl=0x04\n"
        "TrackNo=0\nAMin=0\nASec=0\nAFrame=0\nALBA=-150\nZero=0\n"
        "PMin=1\nPSec=%u\nPFrame=0\nPLBA=%u\n"
        "[Entry 1]\nSession=1\nPoint=0xa1\nADR=0x01\nControl=0x04\n"
        "TrackNo=0\nAMin=0\nASec=0\nAFrame=0\nALBA=-150\nZero=0\n"
        "PMin=1\nPSec=0\nPFrame=0\nPLBA=4350\n"
        "[Entry 2]\nSession=1\nPoint=0xa2\nADR=0x01\nControl=0x04\n"
        "TrackNo=0\nAMin=0\nASec=0\nAFrame=0\nALBA=-150\nZero=0\n"
        "PMin=0\nPSec=2\nPFrame=16\nPLBA=16\n"
        "[Entry 3]\nSession=1\nPoint=0x01\nADR=0x01\nControl=0x04\n"
        "TrackNo=1\nAMin=0\nASec=0\nAFrame=0\nALBA=-150\nZero=0\n"
        "PMin=0\nPSec=2\nPFrame=%u\nPLBA=%u\n"
        "[TRACK 1]\nMODE=%u\n%sINDEX 1=%u\n",
        (unsigned int)disc_type, (unsigned int)a0_plba,
        (unsigned int)index1, (unsigned int)index1,
        (unsigned int)mode, have_index0 != 0 ? "INDEX 0=0\n" : "",
        (unsigned int)index1);
    return length > 0 && (size_t)length < sizeof descriptor &&
           write_file(path, descriptor);
}

static int expect_supported(const char *path, uint32_t mode,
                            uint32_t expected_index0, uint32_t expected_index1,
                            const char *name)
{
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    schway_toc toc;
    const schway_disc *disc;
    uint8_t sector[2352];
    uint8_t subcode[96];
    int ok = 1;
    schway_disc_image_status status = schway_disc_image_open(path, &image, &error);
    if (status != SCHWAY_DISC_IMAGE_OK || image == NULL) {
        fprintf(stderr, "ASSERT disc.image.ccd: %s open expected OK, got %u: %s\n",
                name, (unsigned int)status, error.message);
        schway_disc_image_close(image);
        return 0;
    }
    disc = schway_disc_image_disc(image);
    memset(&toc, 0, sizeof toc);
    toc.size = (uint32_t)sizeof toc;
    if (disc == NULL || disc->toc == NULL || disc->toc(disc->user, &toc) != 1u ||
        toc.track_count != 1u || toc.tracks[0].type != mode ||
        toc.tracks[0].index0 != expected_index0 ||
        toc.tracks[0].index1 != expected_index1 ||
        toc.tracks[0].pregap != expected_index1 - expected_index0 ||
        toc.tracks[0].leadout != 16u) {
        fprintf(stderr, "ASSERT disc.image.ccd: %s TOC did not match the fixture\n", name);
        ok = 0;
    }
    if (disc == NULL || disc->read_sector == NULL) {
        fprintf(stderr, "ASSERT disc.image.ccd: %s has no sector reader\n", name);
        ok = 0;
    } else {
        const uint32_t positions[2] = {0u, 15u};
        for (size_t p = 0u; p < 2u; p++) {
            memset(sector, 0xa5, sizeof sector);
            memset(subcode, 0x5a, sizeof subcode);
            if (disc->read_sector(disc->user, positions[p], sector, subcode) != 1u) {
                fprintf(stderr, "ASSERT disc.image.ccd: %s sector %u was unreadable\n",
                        name, (unsigned int)positions[p]);
                ok = 0;
                continue;
            }
            for (size_t i = 0u; i < sizeof sector; i++) {
                if (sector[i] != fixture_byte(positions[p], i)) {
                    fprintf(stderr, "ASSERT disc.image.ccd: %s sector %u differs at byte %zu\n",
                            name, (unsigned int)positions[p], i);
                    ok = 0;
                    break;
                }
            }
            for (size_t i = 0u; i < sizeof subcode; i++) {
                if (subcode[i] != 0u) {
                    fprintf(stderr, "ASSERT disc.image.ccd: %s missing SUB was not zero-filled\n", name);
                    ok = 0;
                    break;
                }
            }
        }
    }
    schway_disc_image_close(image);
    return ok;
}

static int test_supported_routes(const char *directory)
{
    char descriptor_path[1024];
    char image_path[1024];
    char playlist_path[1024];
    int ok = 1;
    if (!make_path(descriptor_path, directory, "disc-ccd-input.CCD") ||
        !make_path(image_path, directory, "disc-ccd-input.img") ||
        !make_path(playlist_path, directory, "disc-ccd-input.m3u")) return 0;

    if (!write_raw_image(image_path, UINT64_C(16) * UINT64_C(2352)) ||
        !write_descriptor(descriptor_path, 2u, 0u, 0)) {
        fprintf(stderr, "ASSERT disc.image.ccd: could not create independent MODE2 fixture\n");
        ok = 0;
    } else if (!expect_supported(descriptor_path, SCHWAY_DISC_TRACK_MODE2, 0u, 0u,
                                 "direct MODE2 with omitted INDEX 0")) {
        ok = 0;
    }

    if (!write_descriptor(descriptor_path, 1u, 3u, 1) ||
        !write_file(playlist_path, "disc-ccd-input.CCD\n") ||
        !expect_supported(playlist_path, SCHWAY_DISC_TRACK_MODE1, 0u, 3u,
                          "M3U MODE1 with explicit pregap")) {
        fprintf(stderr, "ASSERT disc.image.ccd: M3U child route or MODE1 fixture failed\n");
        ok = 0;
    }
    (void)remove(descriptor_path);
    (void)remove(image_path);
    (void)remove(playlist_path);
    return ok;
}

static int expect_open(const char *path, schway_disc_image_status expected,
                       const char *name);

static int test_malformed_metadata(const char *directory)
{
    char descriptor_path[1024];
    char image_path[1024];
    int ok = 1;
    if (!make_path(descriptor_path, directory, "disc-ccd-invalid.ccd") ||
        !make_path(image_path, directory, "disc-ccd-invalid.img") ||
        !write_raw_image(image_path, UINT64_C(16) * UINT64_C(2352))) return 0;

    if (!write_descriptor(descriptor_path, 2u, 0u, 0) ||
        !replace_file_text(descriptor_path, "PFrame=0\nPLBA=0\n[TRACK 1]",
                           "PFrame=1\nPLBA=1\n[TRACK 1]") ||
        !expect_open(descriptor_path, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                     "track Entry conflicts with INDEX 1")) ok = 0;
    if (!write_descriptor(descriptor_path, 2u, 0u, 0) ||
        !replace_file_text(descriptor_path, "PFrame=16\nPLBA=16\n[Entry 3]",
                           "PFrame=17\nPLBA=17\n[Entry 3]") ||
        !expect_open(descriptor_path, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                     "leadout conflicts with exact IMG extent")) ok = 0;
    if (!write_descriptor(descriptor_path, 2u, 0u, 0) ||
        !replace_file_text(descriptor_path, "INDEX 1=0", "INDEX 1=18446744073709551616") ||
        !expect_open(descriptor_path, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                     "INDEX 1 integer overflow")) ok = 0;
    if (!write_descriptor(descriptor_path, 2u, 3u, 0) ||
        !expect_open(descriptor_path, SCHWAY_DISC_IMAGE_UNSUPPORTED,
                     "missing INDEX 0 cannot infer a pregap")) ok = 0;
    if (!write_raw_image(image_path, UINT64_C(16) * UINT64_C(2352) - 1u) ||
        !write_descriptor(descriptor_path, 2u, 0u, 0) ||
        !expect_open(descriptor_path, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                     "truncated IMG sector")) ok = 0;
    (void)remove(descriptor_path);
    (void)remove(image_path);
    return ok;
}

static int expect_open(const char *path, schway_disc_image_status expected,
                       const char *name)
{
    schway_disc_image_error error;
    schway__disc_ccd *reader;
    int sentinel;
    schway_disc_image_status status;
    reader = (schway__disc_ccd *)(void *)&sentinel;
    memset(&error, 0, sizeof error);
    status = schway__disc_ccd_open(path, &reader, &error);
    if (reader != NULL && reader != (schway__disc_ccd *)(void *)&sentinel) {
        schway__disc_ccd_close(reader);
    }
    if (status == expected && reader == NULL &&
        error.size == (uint32_t)sizeof error && error.status == (uint32_t)expected) return 1;
    fprintf(stderr, "ASSERT disc.image.ccd: %s expected status %u and null reader, got status %u, reader %s\n",
            name, (unsigned int)expected, (unsigned int)status,
            reader == NULL ? "null" : "non-null");
    return 0;
}

static int test_companion_outcomes(const char *directory)
{
    char descriptor_path[1024];
    char image_path[1024];
    char sub_path[1024];
    int ok = 1;
    if (!make_path(descriptor_path, directory, "disc-ccd-policy.ccd") ||
        !make_path(image_path, directory, "disc-ccd-policy.img") ||
        !make_path(sub_path, directory, "disc-ccd-policy.sub")) return 0;

    if (!write_descriptor(descriptor_path, 2u, 0u, 0) ||
        !expect_open(descriptor_path, SCHWAY_DISC_IMAGE_IO, "missing required IMG")) ok = 0;
    if (!write_raw_image(image_path, UINT64_C(16) * UINT64_C(2352)) ||
        !write_descriptor(descriptor_path, 2u, 0u, 0) ||
        !write_file(sub_path, "") ||
        !expect_supported(descriptor_path, SCHWAY_DISC_TRACK_MODE2, 0u, 0u,
                          "empty optional SUB")) ok = 0;
    if (!write_file(sub_path, "sentinel") ||
        !expect_open(descriptor_path, SCHWAY_DISC_IMAGE_UNSUPPORTED,
                     "nonempty SUB remains unsupported")) ok = 0;

#if !defined(_WIN32) && !defined(__wasi__)
    if (chmod(sub_path, 0) == 0) {
        FILE *probe = schway__test_open_file(sub_path, "rb");
        schway_disc_image_status expected = SCHWAY_DISC_IMAGE_IO;
        if (probe != NULL) {
            (void)fclose(probe);
            expected = SCHWAY_DISC_IMAGE_UNSUPPORTED;
        }
        if (!expect_open(descriptor_path, expected, "unreadable SUB is an IO error")) ok = 0;
        (void)chmod(sub_path, 0600);
    }
#endif

    (void)remove(descriptor_path);
    (void)remove(image_path);
    (void)remove(sub_path);
    return ok;
}

#if !defined(_WIN32) && !defined(__wasi__)
static int test_companion_confinement(const char *directory)
{
    char sibling_directory[1024];
    char descriptor_path[1024];
    char image_path[1024];
    char sub_path[1024];
    char external_image[1024];
    char external_sub[1024];
    char missing_sub[1024];
    int sibling_length = snprintf(sibling_directory, sizeof sibling_directory,
                                  "%s-sibling", directory);
    int ok = 1;
    if (sibling_length < 0 || (size_t)sibling_length >= sizeof sibling_directory ||
        !make_path(descriptor_path, directory, "disc-ccd-img-escape.ccd") ||
        !make_path(image_path, directory, "disc-ccd-img-escape.img") ||
        !make_path(external_image, sibling_directory, "disc-ccd-img-escape.img") ||
        !make_path(sub_path, directory, "disc-ccd-img-escape.sub") ||
        !make_path(external_sub, sibling_directory, "disc-ccd-sub-escape.sub") ||
        !make_path(missing_sub, sibling_directory, "missing-target.sub") ||
        mkdir(sibling_directory, 0700) != 0) return 0;

    if (!write_raw_image(external_image, UINT64_C(16) * UINT64_C(2352)) ||
        !write_descriptor(descriptor_path, 2u, 0u, 0) ||
        symlink(external_image, image_path) != 0 ||
        !expect_open(descriptor_path, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                     "valid IMG sentinel in sibling-prefix directory is rejected")) ok = 0;
    (void)remove(image_path);
    (void)remove(descriptor_path);

    if (!write_raw_image(image_path, UINT64_C(16) * UINT64_C(2352)) ||
        !write_descriptor(descriptor_path, 2u, 0u, 0) ||
        !write_file(external_sub, "sentinel") ||
        symlink(external_sub, sub_path) != 0 ||
        !expect_open(descriptor_path, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                     "SUB sentinel outside the descriptor directory is rejected")) ok = 0;
    (void)remove(sub_path);
    (void)remove(descriptor_path);
    (void)remove(image_path);

    if (!write_raw_image(image_path, UINT64_C(16) * UINT64_C(2352)) ||
        !write_descriptor(descriptor_path, 2u, 0u, 0) ||
        symlink(missing_sub, sub_path) != 0 ||
        !expect_open(descriptor_path, SCHWAY_DISC_IMAGE_IO,
                     "dangling optional SUB is a canonicalization IO error")) ok = 0;

    (void)remove(sub_path);
    (void)remove(descriptor_path);
    (void)remove(image_path);
    (void)remove(external_image);
    (void)remove(external_sub);
    (void)rmdir(sibling_directory);
    return ok;
}
#endif

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
    char input_path[1024];
    char iso_path[1024];
    schway_disc_image *active = NULL;
    schway_disc_image_error error;
    const schway_disc *active_disc = NULL;
    int result = 0;

    if (argc != 2 || !make_path(input_path, argv[1], "disc-ccd-input.ccd") ||
        !make_path(iso_path, argv[1], "disc-ccd-active.iso")) {
        fprintf(stderr, "ASSERT disc.image.ccd: expected a writable test directory\n");
        return 1;
    }
    if (!write_active_iso(iso_path) ||
        schway_disc_image_open(iso_path, &active, &error) != SCHWAY_DISC_IMAGE_OK) {
        fprintf(stderr, "ASSERT disc.image.ccd: could not open the existing ISO source\n");
        result = 1;
        goto done;
    }
    active_disc = schway_disc_image_disc(active);
    if (!write_file(input_path, "[CloneCL]\nVersion=3\n") ||
        !expect_open(input_path, SCHWAY_DISC_IMAGE_BAD_IMAGE, "bad signature")) result = 1;
    if (!write_file(input_path, "[CloneCD]") ||
        !expect_open(input_path, SCHWAY_DISC_IMAGE_BAD_IMAGE, "truncated descriptor")) result = 1;
    if (!write_file(input_path, "[CloneCD]\nVersion=4\n") ||
        !expect_open(input_path, SCHWAY_DISC_IMAGE_UNSUPPORTED, "unsupported version")) result = 1;
    if (!write_file(input_path, "[CloneCD]\nVersion=3\n") ||
        !expect_open(input_path, SCHWAY_DISC_IMAGE_BAD_IMAGE, "incomplete descriptor")) result = 1;
    if (!test_supported_routes(argv[1])) result = 1;
    if (!test_malformed_metadata(argv[1])) result = 1;
    if (!test_companion_outcomes(argv[1])) result = 1;
#if !defined(_WIN32) && !defined(__wasi__)
    if (!test_companion_confinement(argv[1])) result = 1;
#endif
    if (!active_iso_is_readable(active, active_disc)) {
        fprintf(stderr, "ASSERT disc.image.ccd: failed open changed the active ISO source\n");
        result = 1;
    }

done:
    schway_disc_image_close(active);
    (void)remove(input_path);
    (void)remove(iso_path);
    return result;
}
