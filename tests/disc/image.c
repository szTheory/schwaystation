/* Host disc image tests create their input images under the build directory. */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif
#include "disc_image.h"
#include "disc_probe.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../test_open.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winioctl.h>
#include <direct.h>
#include <errno.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#define CHECK(condition, message) do { \
    if (!(condition)) { \
        fprintf(stderr, "ASSERT %s: %s\n", test_case, (message)); \
        result = 1; \
        goto cleanup; \
    } \
} while (0)

static int write_iso(const char *path)
{
    FILE *file = schway__test_open_file(path, "wb");
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

static int make_path(char path[1024], const char *directory, const char *name)
{
    int size = snprintf(path, 1024u, "%s/%s", directory, name);
    return size >= 0 && (size_t)size < 1024u;
}

static void pattern_sector(uint8_t *sector, uint32_t sector_bytes, uint32_t lba,
                           uint8_t seed)
{
    for (uint32_t i = 0u; i < sector_bytes; i++) {
        sector[i] = (uint8_t)((uint32_t)seed + lba * 17u + i);
    }
}

static int write_pattern_file(const char *path, uint32_t sectors, uint32_t sector_bytes,
                              uint8_t seed, int append)
{
    FILE *file = schway__test_open_file(path, append != 0 ? "ab" : "wb");
    uint8_t sector[2352];
    if (file == NULL || sector_bytes > sizeof sector) {
        if (file != NULL) (void)fclose(file);
        return 0;
    }
    for (uint32_t lba = 0u; lba < sectors; lba++) {
        pattern_sector(sector, sector_bytes, lba, seed);
        if (fwrite(sector, 1u, sector_bytes, file) != sector_bytes) {
            (void)fclose(file);
            return 0;
        }
    }
    return fclose(file) == 0;
}

static int write_text(const char *path, const char *text, size_t size)
{
    FILE *file = schway__test_open_file(path, "wb");
    if (file == NULL) return 0;
    if (fwrite(text, 1u, size, file) != size) {
        (void)fclose(file);
        return 0;
    }
    return fclose(file) == 0;
}

static void write_iso_u16_both(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 8);
    bytes[3] = (uint8_t)value;
}

static void write_iso_u32_both(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
    bytes[4] = (uint8_t)(value >> 24);
    bytes[5] = (uint8_t)(value >> 16);
    bytes[6] = (uint8_t)(value >> 8);
    bytes[7] = (uint8_t)value;
}

static size_t write_iso_record(uint8_t *bytes, uint32_t extent, uint32_t size,
                               uint8_t flags, const uint8_t *name, size_t name_size)
{
    size_t record_size = 33u + name_size + ((name_size & 1u) == 0u ? 1u : 0u);
    memset(bytes, 0, record_size);
    bytes[0] = (uint8_t)record_size;
    write_iso_u32_both(bytes + 2u, extent);
    write_iso_u32_both(bytes + 10u, size);
    bytes[25] = flags;
    write_iso_u16_both(bytes + 28u, 1u);
    bytes[32] = (uint8_t)name_size;
    memcpy(bytes + 33u, name, name_size);
    return record_size;
}

static int write_probe_iso(const char *path, const char *cnf, size_t cnf_size,
                           const char *boot_file, int fallback)
{
    static const uint8_t dot[] = {0u};
    static const uint8_t dotdot[] = {1u};
    uint8_t *image = (uint8_t *)calloc(24u, 2048u);
    uint8_t *pvd;
    uint8_t *root;
    size_t root_size = 0u;
    FILE *file;
    if (image == NULL) return 0;
    pvd = image + 16u * 2048u;
    pvd[0] = 1u;
    memcpy(pvd + 1u, "CD001", 5u);
    pvd[6] = 1u;
    write_iso_u32_both(pvd + 80u, 24u);
    write_iso_u16_both(pvd + 128u, 2048u);
    (void)write_iso_record(pvd + 156u, 18u, 2048u, 2u, dot, sizeof dot);
    uint8_t *terminator = image + 17u * 2048u;
    terminator[0] = 255u;
    memcpy(terminator + 1u, "CD001", 5u);
    terminator[6] = 1u;

    root = image + 18u * 2048u;
    root_size += write_iso_record(root + root_size, 18u, 2048u, 2u, dot, sizeof dot);
    root_size += write_iso_record(root + root_size, 18u, 2048u, 2u, dotdot, sizeof dotdot);
    if (cnf != NULL) {
        static const uint8_t name[] = "SYSTEM.CNF;1";
        root_size += write_iso_record(root + root_size, 19u, (uint32_t)cnf_size,
                                      0u, name, sizeof name - 1u);
        if (cnf_size <= 2048u) memcpy(image + 19u * 2048u, cnf, cnf_size);
    }
    if (boot_file != NULL) {
        size_t name_size = strlen(boot_file);
        uint8_t name[64];
        if (name_size + 2u >= sizeof name) {
            free(image);
            return 0;
        }
        memcpy(name, boot_file, name_size);
        name[name_size++] = ';';
        name[name_size++] = '1';
        root_size += write_iso_record(root + root_size, 20u, 2048u, 0u,
                                      name, name_size);
        memset(image + 20u * 2048u, 0x5au, 2048u);
    } else if (fallback != 0) {
        static const uint8_t name[] = "PSX.EXE;1";
        root_size += write_iso_record(root + root_size, 20u, 2048u, 0u,
                                      name, sizeof name - 1u);
        memset(image + 20u * 2048u, 0x5au, 2048u);
    }
    if (root_size > 2048u) {
        free(image);
        return 0;
    }
    file = schway__test_open_file(path, "wb");
    if (file == NULL) {
        free(image);
        return 0;
    }
    int ok = fwrite(image, 1u, 24u * 2048u, file) == 24u * 2048u;
    if (fclose(file) != 0) ok = 0;
    free(image);
    return ok;
}

static int expect_open_status(const char *path, schway_disc_image_status expected,
                              uint32_t expected_line, const char *expected_message)
{
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    schway_disc_image_status actual;
    memset(&error, 0, sizeof error);
    actual = schway_disc_image_open(path, &image, &error);
    schway_disc_image_close(image);
    if (actual != expected || (expected_line != UINT32_MAX && error.line != expected_line) ||
        (expected_message != NULL && strcmp(error.message, expected_message) != 0)) {
        fprintf(stderr, "expected status %s line %u, got %s line %u: %s\n",
                schway_disc_image_status_string(expected), expected_line,
                schway_disc_image_status_string(actual), error.line, error.message);
        return 0;
    }
    return 1;
}

static int make_sparse_file(const char *path, uint64_t size)
{
#ifdef _WIN32
    HANDLE file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    LARGE_INTEGER end;
    DWORD ignored;
    BOOL ok;
    if (file == INVALID_HANDLE_VALUE) return 0;
    (void)DeviceIoControl(file, FSCTL_SET_SPARSE, NULL, 0u, NULL, 0u, &ignored, NULL);
    end.QuadPart = (LONGLONG)size;
    ok = SetFilePointerEx(file, end, NULL, FILE_BEGIN) && SetEndOfFile(file);
    (void)CloseHandle(file);
    return ok != 0;
#else
    int file = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    int ok;
    if (file < 0) return 0;
    ok = ftruncate(file, (off_t)size) == 0;
    if (close(file) != 0) ok = 0;
    return ok;
#endif
}

static int make_symlink(const char *target, const char *path)
{
#ifdef _WIN32
#ifndef SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
#define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE 0x2
#endif
    return CreateSymbolicLinkA(path, target,
                               SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE) != 0;
#else
    return symlink(target, path) == 0;
#endif
}

static int make_directory(const char *path)
{
#ifdef _WIN32
    int result = _mkdir(path);
#else
    int result = mkdir(path, 0700);
#endif
    return result == 0 || errno == EEXIST;
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

static int test_cue_modes(const char *directory)
{
    char cue_path[1024] = "";
    char mode1_path[1024] = "";
    char mode2_path[1024] = "";
    char audio_path[1024] = "";
    const char *test_case = "disc.image.cue_modes";
    static const char cue[] =
        "FILE \"mode1.bin\" BINARY\n"
        "  TRACK 01 MODE1/2048\n"
        "    INDEX 01 00:00:00\n"
        "FILE \"mode2.bin\" BINARY\n"
        "  TRACK 02 MODE2/2352\n"
        "    INDEX 01 00:00:00\n"
        "FILE \"audio.bin\" BINARY\n"
        "  TRACK 03 AUDIO\n"
        "    INDEX 01 00:00:00\n";
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    const schway_disc *disc;
    schway_toc toc;
    uint8_t sector[2352];
    uint8_t expected[2352];
    uint8_t subcode[96];
    int result = 0;

    CHECK(make_path(cue_path, directory, "schway-disc-image-modes.cue") &&
          make_path(mode1_path, directory, "mode1.bin") &&
          make_path(mode2_path, directory, "mode2.bin") &&
          make_path(audio_path, directory, "audio.bin"), "could not form cue test paths");
    CHECK(write_pattern_file(mode1_path, 16u, 2048u, 0x11u, 0) &&
          write_pattern_file(mode2_path, 16u, 2352u, 0x61u, 0) &&
          write_pattern_file(audio_path, 16u, 2352u, 0x91u, 0) &&
          write_text(cue_path, cue, sizeof cue - 1u), "could not create cue mode inputs");
    memset(&error, 0, sizeof error);
    CHECK(schway_disc_image_open(cue_path, &image, &error) == SCHWAY_DISC_IMAGE_OK,
          "expected a multi-file MODE1/MODE2/AUDIO cue to open");
    disc = schway_disc_image_disc(image);
    CHECK(disc != NULL && disc->read_sector != NULL && disc->toc != NULL,
          "expected a complete cue-backed disc descriptor");
    memset(&toc, 0, sizeof toc);
    toc.size = (uint32_t)sizeof toc;
    CHECK(disc->toc(disc->user, &toc) == 1u && toc.track_count == 3u,
          "expected three cue tracks");
    CHECK(toc.tracks[0].type == SCHWAY_DISC_TRACK_MODE1 &&
          toc.tracks[0].index0 == 0u && toc.tracks[0].index1 == 0u &&
          toc.tracks[0].leadout == 16u &&
          toc.tracks[1].type == SCHWAY_DISC_TRACK_MODE2 &&
          toc.tracks[1].index0 == 16u && toc.tracks[1].index1 == 16u &&
          toc.tracks[1].leadout == 32u &&
          toc.tracks[2].type == SCHWAY_DISC_TRACK_AUDIO &&
          toc.tracks[2].index0 == 32u && toc.tracks[2].index1 == 32u &&
          toc.tracks[2].leadout == 48u,
          "expected each FILE to extend the absolute TOC in sectors");
    CHECK(disc->read_sector(disc->user, 2u, sector, subcode) == 1u,
          "expected cooked MODE1 sector read");
    pattern_sector(expected, 2048u, 2u, 0x11u);
    CHECK(memcmp(sector + 16u, expected, 2048u) == 0,
          "expected MODE1/2048 to be normalized to raw user-data offset");
    CHECK(disc->read_sector(disc->user, 17u, sector, subcode) == 1u,
          "expected raw MODE2 sector read");
    pattern_sector(expected, 2352u, 1u, 0x61u);
    CHECK(memcmp(sector, expected, sizeof sector) == 0,
          "expected MODE2/2352 to preserve all raw bytes");
    CHECK(disc->read_sector(disc->user, 33u, sector, subcode) == 1u,
          "expected raw AUDIO sector read");
    pattern_sector(expected, 2352u, 1u, 0x91u);
    CHECK(memcmp(sector, expected, sizeof sector) == 0,
          "expected AUDIO to preserve all raw bytes");
    CHECK(disc->read_sector(disc->user, 48u, sector, subcode) == 0u,
          "expected the cue leadout to reject reads");

cleanup:
    schway_disc_image_close(image);
    if (cue_path[0] != '\0') (void)remove(cue_path);
    if (mode1_path[0] != '\0') (void)remove(mode1_path);
    if (mode2_path[0] != '\0') (void)remove(mode2_path);
    if (audio_path[0] != '\0') (void)remove(audio_path);
    return result;
}

static int test_cue_root_child(const char *directory)
{
    char child_directory[1024] = "";
    char cue_path[1024] = "";
    char track_path[1024] = "";
    static const char cue[] =
        "FILE \"schwaystation-cue-root-child/track.bin\" BINARY\n"
        "TRACK 01 MODE1/2048\n"
        "INDEX 01 00:00:00\n";
    const char *test_case = "disc.image.cue_root_child";
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    int result = 0;

    CHECK(make_path(child_directory, directory, "schwaystation-cue-root-child") &&
          make_path(cue_path, directory, "schwaystation-cue-root.cue") &&
          make_path(track_path, child_directory, "track.bin"),
          "could not form root-level cue child paths");
    CHECK(make_directory(child_directory) &&
          write_pattern_file(track_path, 16u, 2048u, 0x31u, 0) &&
          write_text(cue_path, cue, sizeof cue - 1u),
          "could not create root-level cue child inputs");
    memset(&error, 0, sizeof error);
    CHECK(schway_disc_image_open(cue_path, &image, &error) == SCHWAY_DISC_IMAGE_OK,
          "expected a root-level cue to open its nested track");

cleanup:
    schway_disc_image_close(image);
    if (cue_path[0] != '\0') (void)remove(cue_path);
    if (track_path[0] != '\0') (void)remove(track_path);
    if (child_directory[0] != '\0') {
#ifdef _WIN32
        (void)_rmdir(child_directory);
#else
        (void)rmdir(child_directory);
#endif
    }
    return result;
}

static int test_cue_pregap(const char *directory)
{
    char cue_path[1024] = "";
    char mixed_path[1024] = "";
    char next_path[1024] = "";
    const char *test_case = "disc.image.cue_pregap";
    static const char cue[] =
        "FILE \"mixed.bin\" BINARY\n"
        " TRACK 01 MODE1/2048\n"
        "  INDEX 01 00:00:00\n"
        " TRACK 02 AUDIO\n"
        "  INDEX 00 00:00:16\n"
        "  INDEX 01 00:00:18\n"
        "FILE \"next.bin\" BINARY\n"
        " TRACK 03 AUDIO\n"
        "  PREGAP 00:00:03\n"
        "  INDEX 01 00:00:00\n";
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    const schway_disc *disc;
    schway_toc toc;
    uint8_t sector[2352];
    uint8_t expected[2352];
    uint8_t subcode[96];
    int result = 0;

    CHECK(make_path(cue_path, directory, "schway-disc-image-pregap.cue") &&
          make_path(mixed_path, directory, "mixed.bin") &&
          make_path(next_path, directory, "next.bin"), "could not form pregap test paths");
    CHECK(write_pattern_file(mixed_path, 16u, 2048u, 0x21u, 0) &&
          write_pattern_file(mixed_path, 16u, 2352u, 0x61u, 1) &&
          write_pattern_file(next_path, 16u, 2352u, 0xa1u, 0) &&
          write_text(cue_path, cue, sizeof cue - 1u), "could not create pregap inputs");
    memset(&error, 0, sizeof error);
    CHECK(schway_disc_image_open(cue_path, &image, &error) == SCHWAY_DISC_IMAGE_OK,
          "expected physical INDEX 00 and virtual PREGAP to open");
    disc = schway_disc_image_disc(image);
    memset(&toc, 0, sizeof toc);
    toc.size = (uint32_t)sizeof toc;
    CHECK(disc != NULL && disc->toc(disc->user, &toc) == 1u && toc.track_count == 3u,
          "expected three tracks in pregap TOC");
    CHECK(toc.tracks[0].leadout == 16u &&
          toc.tracks[1].index0 == 16u && toc.tracks[1].index1 == 18u &&
          toc.tracks[1].pregap == 2u && toc.tracks[1].leadout == 32u &&
          toc.tracks[2].index0 == 32u && toc.tracks[2].index1 == 35u &&
          toc.tracks[2].pregap == 3u && toc.tracks[2].leadout == 51u,
          "expected physical and virtual pregaps to occupy distinct TOC sectors");
    CHECK(disc->read_sector(disc->user, 16u, sector, subcode) == 1u,
          "expected physical INDEX 00 sector to be readable");
    pattern_sector(expected, 2352u, 0u, 0x61u);
    CHECK(memcmp(sector, expected, sizeof sector) == 0,
          "expected INDEX 00 sectors to come from the image file");
    CHECK(disc->read_sector(disc->user, 18u, sector, subcode) == 1u,
          "expected INDEX 01 sector to be readable");
    pattern_sector(expected, 2352u, 2u, 0x61u);
    CHECK(memcmp(sector, expected, sizeof sector) == 0,
          "expected INDEX 01 to point to its file offset");
    CHECK(disc->read_sector(disc->user, 32u, sector, subcode) == 1u &&
          disc->read_sector(disc->user, 34u, sector, subcode) == 1u,
          "expected virtual PREGAP sectors to be readable silence");
    for (uint32_t i = 0u; i < (uint32_t)sizeof sector; i++) {
        CHECK(sector[i] == 0u, "expected virtual audio pregap to be silent");
    }
    CHECK(disc->read_sector(disc->user, 35u, sector, subcode) == 1u,
          "expected the post-PREGAP sector to be readable");
    pattern_sector(expected, 2352u, 0u, 0xa1u);
    CHECK(memcmp(sector, expected, sizeof sector) == 0,
          "expected virtual PREGAP not to consume file bytes");

cleanup:
    schway_disc_image_close(image);
    if (cue_path[0] != '\0') (void)remove(cue_path);
    if (mixed_path[0] != '\0') (void)remove(mixed_path);
    if (next_path[0] != '\0') (void)remove(next_path);
    return result;
}

static int test_cue_reject(const char *directory)
{
    char cue_path[1024] = "";
    char child_path[1024] = "";
    char playlist_path[1024] = "";
    char cue[8192];
    const char *test_case = "disc.image.cue_reject";
    const char *unsafe_names[] = {
        "../escape.bin", "..\\escape.bin", "folder\\..\\escape.bin",
        "/tmp/escape.bin", "C:\\escape.bin", "C:escape.bin", "\\\\server\\share.bin"
    };
    int result = 0;

    CHECK(make_path(cue_path, directory, "schway-disc-image-invalid.cue") &&
          make_path(child_path, directory, "invalid-child.bin") &&
          make_path(playlist_path, directory, "unsupported-entry.m3u"),
          "could not form reject test paths");
    for (size_t i = 0u; i < sizeof unsafe_names / sizeof unsafe_names[0]; i++) {
        int size = snprintf(cue, sizeof cue,
                            "FILE \"%s\" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 00:00:00\n",
                            unsafe_names[i]);
        CHECK(size >= 0 && (size_t)size < sizeof cue &&
              write_text(cue_path, cue, (size_t)size), "could not write escaping cue");
        CHECK(expect_open_status(cue_path, SCHWAY_DISC_IMAGE_BAD_CUE, 1u, NULL),
              "expected absolute, parent, drive and UNC children to be rejected");
    }
    CHECK(write_text(cue_path,
          "FILE \"missing.bin\" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 00:00:00\n",
          sizeof "FILE \"missing.bin\" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 00:00:00\n" - 1u),
          "could not write missing-child cue");
    CHECK(expect_open_status(cue_path, SCHWAY_DISC_IMAGE_BAD_CUE, 1u, NULL),
          "expected a missing child to fail with its FILE line");
    CHECK(write_text(cue_path,
          "FILE \"invalid-child.bin\" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 99:59:75\n",
          sizeof "FILE \"invalid-child.bin\" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 99:59:75\n" - 1u),
          "could not write invalid-MSF cue");
    CHECK(expect_open_status(cue_path, SCHWAY_DISC_IMAGE_BAD_CUE, 3u, NULL),
          "expected an out-of-range MSF field to fail on its line");
    CHECK(write_text(cue_path,
          "FILE \"invalid-child.bin\" BINARY\nTRACK 01 MODE1/2048\nINDEX 00 00:00:10\nINDEX 01 00:00:09\n",
          sizeof "FILE \"invalid-child.bin\" BINARY\nTRACK 01 MODE1/2048\nINDEX 00 00:00:10\nINDEX 01 00:00:09\n" - 1u),
          "could not write backward-index cue");
    CHECK(expect_open_status(cue_path, SCHWAY_DISC_IMAGE_BAD_CUE, 4u, NULL),
          "expected backward INDEX arithmetic to fail on its line");
    CHECK(write_pattern_file(child_path, 16u, 2048u, 0x17u, 0) &&
          write_text(cue_path,
          "FILE \"invalid-child.bin\" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 00:00:00\n",
          sizeof "FILE \"invalid-child.bin\" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 00:00:00\n" - 1u),
          "could not create invalid-mode cue inputs");
    CHECK(write_pattern_file(child_path, 16u, 2048u, 0x17u, 0) &&
          make_sparse_file(child_path, 16u * UINT64_C(2048) - 1u),
          "could not create a partial-sector child");
    CHECK(expect_open_status(cue_path, SCHWAY_DISC_IMAGE_BAD_IMAGE, 1u, NULL),
          "expected a child with a partial sector to be rejected before reads");
    CHECK(write_text(cue_path,
          "FILE \"unsupported.chd\" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 00:00:00\n",
          sizeof "FILE \"unsupported.chd\" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 00:00:00\n" - 1u) &&
          expect_open_status(cue_path, SCHWAY_DISC_IMAGE_UNSUPPORTED, 1u,
                             "unsupported: CHD (planned v2.0)"),
          "expected an unsupported cue child extension to be rejected consistently");
    (void)remove(cue_path);
    cue_path[0] = '\0';
    for (uint32_t i = 0u; i < 4u; i++) {
        static const char *extensions[] = {"chd", "pbp", "ecm", "ccd"};
        static const char *messages[] = {
            "unsupported: CHD (planned v2.0)", "unsupported: PBP (planned v2.0)",
            "unsupported: ECM (planned v2.0)", "unsupported: CCD (planned v2.0)"
        };
        int size = snprintf(cue_path, sizeof cue_path, "%s/unsupported.%s",
                            directory, extensions[i]);
        CHECK(size >= 0 && (size_t)size < sizeof cue_path &&
              expect_open_status(cue_path, SCHWAY_DISC_IMAGE_UNSUPPORTED, 0u, messages[i]),
              "expected unsupported media to name its planned v2.0 format");
    }
    CHECK(write_text(playlist_path, "unsupported.chd\n", sizeof "unsupported.chd\n" - 1u) &&
          expect_open_status(playlist_path, SCHWAY_DISC_IMAGE_UNSUPPORTED, 1u,
                             "unsupported: CHD (planned v2.0)"),
          "expected an unsupported m3u entry extension to be rejected consistently");

cleanup:
    if (cue_path[0] != '\0') (void)remove(cue_path);
    if (child_path[0] != '\0') (void)remove(child_path);
    if (playlist_path[0] != '\0') (void)remove(playlist_path);
    return result;
}

static int test_cue_symlink(const char *directory)
{
    char root[1024] = "";
    char cue_path[1024] = "";
    char outside_path[1024] = "";
    char link_path[1024] = "";
    static const char cue[] =
        "FILE \"link.bin\" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 00:00:00\n";
    const char *test_case = "disc.image.cue_symlink";
    int result = 0;

    CHECK(make_path(root, directory, "disc-image-symlink") && make_directory(root) &&
          make_path(cue_path, root, "disc.cue") && make_path(link_path, root, "link.bin") &&
          make_path(outside_path, directory, "outside-disc-image.bin"),
          "could not form symlink-escape paths");
    CHECK(write_pattern_file(outside_path, 16u, 2048u, 0x44u, 0) &&
          make_symlink("../outside-disc-image.bin", link_path) &&
          write_text(cue_path, cue, sizeof cue - 1u),
          "could not create symlink-escape input");
    CHECK(expect_open_status(cue_path, SCHWAY_DISC_IMAGE_BAD_CUE, 1u, NULL),
          "expected a canonical child path outside the cue directory to be rejected");

cleanup:
    if (cue_path[0] != '\0') (void)remove(cue_path);
    if (link_path[0] != '\0') (void)remove(link_path);
    if (outside_path[0] != '\0') (void)remove(outside_path);
    if (root[0] != '\0') {
#ifdef _WIN32
        (void)_rmdir(root);
#else
        (void)rmdir(root);
#endif
    }
    return result;
}

static int test_cue_limits(const char *directory)
{
    char cue_path[1024] = "";
    char child_path[1024] = "";
    char max_name[256];
    char *cue = NULL;
    size_t cue_length;
    const char *test_case = "disc.image.cue_limits";
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    schway_toc toc;
    int result = 0;
    int written;

    memset(max_name, 'n', sizeof max_name - 1u);
    max_name[sizeof max_name - 1u] = '\0';
    CHECK(make_path(cue_path, directory, "schway-disc-image-limit.cue") &&
          make_path(child_path, directory, max_name), "could not form limit test paths");
    CHECK(write_pattern_file(child_path, 16u, 2048u, 0x19u, 0),
          "could not create the 255-byte child filename");
    cue_length = strlen(max_name) + 96u;
    cue = (char *)malloc(65537u);
    CHECK(cue != NULL, "could not allocate bounded cue input");
    written = snprintf(cue, cue_length,
                       "FILE \"%s\" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 00:00:00\n",
                       max_name);
    CHECK(written >= 0 && (size_t)written < cue_length,
          "could not format maximum filename cue");
    memset(cue + written, ' ', 65536u - (size_t)written);
    CHECK(write_text(cue_path, cue, 65536u), "could not write 64 KiB cue");
    memset(&error, 0, sizeof error);
    CHECK(schway_disc_image_open(cue_path, &image, &error) == SCHWAY_DISC_IMAGE_OK,
          "expected a 64 KiB cue and 255-byte child name to be accepted");
    schway_disc_image_close(image);
    image = NULL;
    cue[65536] = ' ';
    CHECK(write_text(cue_path, cue, 65537u) &&
          expect_open_status(cue_path, SCHWAY_DISC_IMAGE_BAD_CUE, 0u, NULL),
          "expected a cue above 64 KiB to be rejected");

    /* Ninety-nine one-index tracks fit the public TOC and count cap. */
    written = snprintf(cue, 65537u, "FILE \"%s\" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 00:00:00\n",
                       max_name);
    CHECK(written >= 0, "could not start maximum-track cue");
    size_t used = (size_t)written;
    for (uint32_t track = 2u; track <= 99u; track++) {
        uint32_t frame = track + 14u;
        written = snprintf(cue + used, 65537u - used,
                           "TRACK %02u AUDIO\nINDEX 01 00:%02u:%02u\n",
                           track, frame / 75u, frame % 75u);
        CHECK(written >= 0 && (size_t)written < 65537u - used,
              "could not append maximum-track cue line");
        used += (size_t)written;
    }
    CHECK(write_pattern_file(child_path, 16u, 2048u, 0x19u, 0) &&
          write_pattern_file(child_path, 98u, 2352u, 0x39u, 1) &&
          write_text(cue_path, cue, used), "could not create 99-track inputs");
    CHECK(schway_disc_image_open(cue_path, &image, &error) == SCHWAY_DISC_IMAGE_OK,
          "expected the 99-track boundary to be accepted");
    memset(&toc, 0, sizeof toc);
    toc.size = (uint32_t)sizeof toc;
    CHECK(schway_disc_image_disc(image)->toc(schway_disc_image_disc(image)->user, &toc) == 1u &&
          toc.track_count == 99u && toc.tracks[98].leadout == 114u,
          "expected all 99 tracks to have bounded absolute offsets");
    schway_disc_image_close(image);
    image = NULL;
    CHECK(make_sparse_file(child_path, UINT64_C(1073741824)),
          "could not create exact-cap sparse cue image");
    written = snprintf(cue, 65537u,
                       "FILE \"%s\" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 00:00:00\n",
                       max_name);
    CHECK(written > 0 && write_text(cue_path, cue, (size_t)written),
          "could not write exact-cap cue sheet");
    CHECK(schway_disc_image_open(cue_path, &image, &error) == SCHWAY_DISC_IMAGE_OK,
          "expected a cue image exactly at the 1 GiB cap to be accepted");
    schway_disc_image_close(image);
    image = NULL;
    CHECK(make_sparse_file(child_path, UINT64_C(1073741824) + 2048u) &&
          expect_open_status(cue_path, SCHWAY_DISC_IMAGE_LIMIT, 1u, NULL),
          "expected cue image totals above 1 GiB to be rejected");

cleanup:
    schway_disc_image_close(image);
    free(cue);
    if (cue_path[0] != '\0') (void)remove(cue_path);
    if (child_path[0] != '\0') (void)remove(child_path);
    return result;
}

static int test_cue_index_limits(const char *directory)
{
    char cue_path[1024] = "";
    char child_path[1024] = "";
    char *cue = NULL;
    const char *test_case = "disc.image.cue_index_limits";
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    schway_toc toc;
    size_t used;
    int written;
    int result = 0;

    CHECK(make_path(cue_path, directory, "schway-disc-image-index-limits.cue") &&
          make_path(child_path, directory, "index-limits.bin"),
          "could not form index-limit test paths");
    cue = (char *)malloc(65537u);
    CHECK(cue != NULL && write_pattern_file(child_path, 100u, 2048u, 0x29u, 0),
          "could not create index-limit inputs");
    written = snprintf(cue, 65537u,
                       "FILE \"index-limits.bin\" BINARY\nTRACK 01 MODE1/2048\nINDEX 01 00:00:00\n");
    CHECK(written > 0, "could not start 99-index cue");
    used = (size_t)written;
    for (uint32_t index = 2u; index <= 99u; index++) {
        uint32_t frame = index - 1u;
        written = snprintf(cue + used, 65537u - used,
                           "INDEX %02u 00:%02u:%02u\n", index, frame / 75u, frame % 75u);
        CHECK(written > 0 && (size_t)written < 65537u - used,
              "could not append the 99th index");
        used += (size_t)written;
    }
    CHECK(write_text(cue_path, cue, used), "could not write 99-index cue");
    memset(&error, 0, sizeof error);
    CHECK(schway_disc_image_open(cue_path, &image, &error) == SCHWAY_DISC_IMAGE_OK,
          "expected 99 indexes on one track to be accepted");
    memset(&toc, 0, sizeof toc);
    toc.size = (uint32_t)sizeof toc;
    CHECK(schway_disc_image_disc(image)->toc(schway_disc_image_disc(image)->user, &toc) == 1u &&
          toc.track_count == 1u && toc.tracks[0].leadout == 100u,
          "expected the accepted index boundary to preserve the track TOC");
    schway_disc_image_close(image);
    image = NULL;

    written = snprintf(cue, 65537u,
                       "FILE \"index-limits.bin\" BINARY\nTRACK 01 MODE1/2048\nINDEX 00 00:00:00\nINDEX 01 00:00:01\n");
    CHECK(written > 0, "could not start 100-index cue");
    used = (size_t)written;
    for (uint32_t index = 2u; index <= 99u; index++) {
        uint32_t frame = index;
        written = snprintf(cue + used, 65537u - used,
                           "INDEX %02u 00:%02u:%02u\n", index, frame / 75u, frame % 75u);
        CHECK(written > 0 && (size_t)written < 65537u - used,
              "could not append a cue index");
        used += (size_t)written;
    }
    CHECK(write_text(cue_path, cue, used) &&
          expect_open_status(cue_path, SCHWAY_DISC_IMAGE_BAD_CUE, 102u, NULL),
          "expected the 100th index to be rejected with its line number");

cleanup:
    schway_disc_image_close(image);
    free(cue);
    if (cue_path[0] != '\0') (void)remove(cue_path);
    if (child_path[0] != '\0') (void)remove(child_path);
    return result;
}

static int test_iso_limits(const char *directory)
{
    char path[1024] = "";
    const char *test_case = "disc.image.iso_limits";
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    schway_toc toc;
    int result = 0;

    CHECK(make_path(path, directory, "schway-disc-image-size.iso"), "could not form ISO limit path");
    CHECK(make_sparse_file(path, 15u * UINT64_C(2048)) &&
          expect_open_status(path, SCHWAY_DISC_IMAGE_BAD_IMAGE, UINT32_MAX, NULL),
          "expected fewer than 16 sectors to be rejected");
    CHECK(make_sparse_file(path, 16u * UINT64_C(2048) + 1u) &&
          expect_open_status(path, SCHWAY_DISC_IMAGE_BAD_IMAGE, UINT32_MAX, NULL),
          "expected a partial cooked sector to be rejected");
    CHECK(make_sparse_file(path, UINT64_C(1073741824)), "could not create exact-cap sparse ISO");
    CHECK(schway_disc_image_open(path, &image, &error) == SCHWAY_DISC_IMAGE_OK,
          "expected an ISO exactly at the 1 GiB cap to be accepted");
    memset(&toc, 0, sizeof toc);
    toc.size = (uint32_t)sizeof toc;
    CHECK(schway_disc_image_disc(image)->toc(schway_disc_image_disc(image)->user, &toc) == 1u &&
          toc.tracks[0].leadout == 524288u,
          "expected exact-cap TOC sector arithmetic");
    schway_disc_image_close(image);
    image = NULL;
    CHECK(make_sparse_file(path, UINT64_C(1073741824) + 2048u) &&
          expect_open_status(path, SCHWAY_DISC_IMAGE_LIMIT, UINT32_MAX, NULL),
          "expected an ISO above 1 GiB to be rejected");

cleanup:
    schway_disc_image_close(image);
    if (path[0] != '\0') (void)remove(path);
    return result;
}

static int test_playlist(const char *directory)
{
    char playlist_path[1024] = "";
    char first_path[1024] = "";
    char second_path[1024] = "";
    char outside_path[1024] = "";
    char link_path[1024] = "";
    char *bounded_text = NULL;
    const char *test_case = "disc.playlist";
    static const char playlist[] = "# Two images\r\n\r\n first.iso \r\nsecond.iso\n";
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    const schway_disc *first_disc = NULL;
    const schway_disc *second_disc = NULL;
    const schway_disc *invalid_disc = NULL;
    uint8_t sector[2352];
    uint8_t subcode[96];
    int result = 0;

    CHECK(make_path(playlist_path, directory, "schway-disc-playlist-test.m3u") &&
          make_path(first_path, directory, "first.iso") &&
          make_path(second_path, directory, "second.iso"),
          "could not form playlist test paths");
    CHECK(write_pattern_file(first_path, 16u, 2048u, 0x11u, 0) &&
          write_pattern_file(second_path, 16u, 2048u, 0x71u, 0) &&
          write_text(playlist_path, playlist, sizeof playlist - 1u),
          "could not create playlist image inputs");
    memset(&error, 0, sizeof error);
    CHECK(schway_disc_image_open(playlist_path, &image, &error) == SCHWAY_DISC_IMAGE_OK,
          "expected a bounded two-image m3u to open");
    CHECK(schway_disc_image_count(image) == 2u,
          "expected both playlist entries to be retained");
    CHECK(schway_disc_image_select(image, 0u, &first_disc) == SCHWAY_DISC_IMAGE_OK &&
          first_disc == schway_disc_image_disc(image),
          "expected index zero to expose the initial disc descriptor");
    CHECK(schway_disc_image_select(image, 1u, &second_disc) == SCHWAY_DISC_IMAGE_OK &&
          second_disc != NULL && second_disc != first_disc &&
          schway_disc_image_disc(image) == first_disc,
          "expected index one to prepare a distinct disc descriptor");
    CHECK(strcmp(schway_disc_image_label(image, 0u), "first.iso") == 0 &&
          strcmp(schway_disc_image_label(image, 1u), "second.iso") == 0 &&
          schway_disc_image_path(image, 0u) != NULL &&
          schway_disc_image_path(image, 1u) != NULL,
          "expected bounded path and basename label metadata for each image");
    CHECK(schway_disc_image_select(image, 2u, &invalid_disc) == SCHWAY_DISC_IMAGE_BAD_ARGUMENT &&
          invalid_disc == NULL && schway_disc_image_disc(image) == first_disc,
          "expected invalid selection to preserve the initial provider image");
    CHECK(first_disc->read_sector(first_disc->user, 0u, sector, subcode) == 1u,
          "expected the first selected image to read a sector");
    CHECK(sector[16] == 0x11u,
          "expected index zero to read bytes from the first playlist image");
    CHECK(second_disc->read_sector(second_disc->user, 0u, sector, subcode) == 1u &&
          sector[16] == 0x71u,
          "expected index one to read bytes from the second playlist image");

    schway_disc_image_close(image);
    image = NULL;
    char entries[4096];
    size_t used = 0u;
    for (uint32_t i = 0u; i < SCHWAY_DISC_IMAGE_PLAYLIST_LIMIT; i++) {
        int written = snprintf(entries + used, sizeof entries - used, "first.iso\n");
        CHECK(written > 0 && (size_t)written < sizeof entries - used,
              "could not append a playlist entry at the 32-image boundary");
        used += (size_t)written;
    }
    CHECK(write_text(playlist_path, entries, used), "could not write the 32-image playlist");
    CHECK(schway_disc_image_open(playlist_path, &image, &error) == SCHWAY_DISC_IMAGE_OK &&
          schway_disc_image_count(image) == SCHWAY_DISC_IMAGE_PLAYLIST_LIMIT,
          "expected exactly 32 playlist images to be accepted");
    schway_disc_image_close(image);
    image = NULL;
    int written = snprintf(entries + used, sizeof entries - used, "first.iso\n");
    CHECK(written > 0 && (size_t)written < sizeof entries - used,
          "could not append the 33rd playlist entry");
    used += (size_t)written;
    CHECK(write_text(playlist_path, entries, used), "could not write the 33-image playlist");
    for (uint32_t attempt = 0u; attempt < 40u; attempt++) {
        memset(&error, 0, sizeof error);
        CHECK(schway_disc_image_open(playlist_path, &image, &error) == SCHWAY_DISC_IMAGE_LIMIT &&
              image == NULL && error.line == 33u && strstr(error.message, "BAD_M3U") != NULL,
              "expected the 33rd entry to fail with bounded playlist diagnostics and cleanup");
    }
    char maximum_path[257];
    memset(maximum_path, 'a', 251u);
    memcpy(maximum_path + 251u, ".chd", 4u);
    maximum_path[255] = '\n';
    maximum_path[256] = '\0';
    CHECK(write_text(playlist_path, maximum_path, 256u) &&
          expect_open_status(playlist_path, SCHWAY_DISC_IMAGE_UNSUPPORTED, 1u,
                             "unsupported: CHD (planned v2.0)"),
          "expected a 255-byte playlist path to reach consistent unsupported-type handling");
    CHECK(write_text(playlist_path, "../outside.iso\n", sizeof "../outside.iso\n" - 1u) &&
          expect_open_status(playlist_path, SCHWAY_DISC_IMAGE_BAD_M3U, 1u, NULL),
          "expected parent traversal in a playlist entry to be rejected");
    char long_entry[258];
    memset(long_entry, 'a', sizeof long_entry - 1u);
    long_entry[sizeof long_entry - 2u] = '\n';
    long_entry[sizeof long_entry - 1u] = '\0';
    CHECK(write_text(playlist_path, long_entry, sizeof long_entry - 1u) &&
          expect_open_status(playlist_path, SCHWAY_DISC_IMAGE_BAD_M3U, 1u, NULL),
          "expected a path longer than 255 bytes to be rejected");
    bounded_text = (char *)malloc(65537u);
    CHECK(bounded_text != NULL, "could not allocate bounded playlist text");
    memset(bounded_text, '#', 65537u);
    CHECK(write_text(playlist_path, bounded_text, 65536u) &&
          expect_open_status(playlist_path, SCHWAY_DISC_IMAGE_BAD_M3U, 0u, NULL),
          "expected a 64 KiB playlist to parse within its byte bound");
    CHECK(write_text(playlist_path, bounded_text, 65537u) &&
          expect_open_status(playlist_path, SCHWAY_DISC_IMAGE_LIMIT, 0u,
                             "BAD_M3U LIMIT: playlist exceeds 64 KiB"),
          "expected a playlist above 64 KiB to fail at the byte bound");
    free(bounded_text);
    bounded_text = NULL;
    static const char recursive[] = "schway-disc-playlist-test.m3u\n";
    CHECK(write_text(playlist_path, recursive, sizeof recursive - 1u) &&
          expect_open_status(playlist_path, SCHWAY_DISC_IMAGE_BAD_M3U, 1u, NULL),
          "expected recursive playlists to be rejected");
    CHECK(make_path(outside_path, directory, "../schway-disc-playlist-outside.iso") &&
          make_path(link_path, directory, "outside-link.iso") &&
          write_pattern_file(outside_path, 16u, 2048u, 0x31u, 0) &&
          make_symlink("../schway-disc-playlist-outside.iso", link_path) &&
          write_text(playlist_path, "outside-link.iso\n", sizeof "outside-link.iso\n" - 1u) &&
          expect_open_status(playlist_path, SCHWAY_DISC_IMAGE_BAD_M3U, 1u, NULL),
          "expected a playlist symlink that resolves outside its directory to be rejected");

cleanup:
    free(bounded_text);
    schway_disc_image_close(image);
    if (playlist_path[0] != '\0') (void)remove(playlist_path);
    if (first_path[0] != '\0') (void)remove(first_path);
    if (second_path[0] != '\0') (void)remove(second_path);
    if (link_path[0] != '\0') (void)remove(link_path);
    if (outside_path[0] != '\0') (void)remove(outside_path);
    return result;
}

static int test_cnf(const char *directory)
{
    char path[1024] = "";
    static const char valid[] =
        "BOOT = cdrom:\\SLUS_123.45;1\r\n"
        "TCB = 64\r\n"
        "EVENT = 256\r\n"
        "STACK = 801FFF00h\r\n";
    static const char unsafe_boot[] = "BOOT = CDROM:\\..\\SLUS_123.45;1\n";
    static const char bad_tcb[] = "BOOT = CDROM:\\SLUS_123.45;1\nTCB = 65\n";
    static const char bad_event[] = "BOOT = CDROM:\\SLUS_123.45;1\nEVENT = 257\n";
    static const char bad_stack[] = "BOOT = CDROM:\\SLUS_123.45;1\nSTACK = 80300000h\n";
    const char *test_case = "disc.cnf";
    schway_disc_system_cnf metadata;
    schway_disc_system_cnf before;
    schway_host_disc_probe probe;
    int result = 0;

    CHECK(make_path(path, directory, "schway-disc-cnf-test.iso"),
          "could not form CNF probe path");
    memset(&metadata, 0, sizeof metadata);
    CHECK(schway_disc_image_parse_system_cnf((const uint8_t *)valid, sizeof valid - 1u,
                                              &metadata) == SCHWAY_DISC_IMAGE_OK,
          "expected bounded ASCII boot and kernel metadata to parse");
    CHECK(metadata.component_count == 1u && strcmp(metadata.boot_name, "SLUS_123.45") == 0 &&
          metadata.present == (SCHWAY_DISC_SYSTEM_CNF_HAVE_TCB |
                               SCHWAY_DISC_SYSTEM_CNF_HAVE_EVENT |
                               SCHWAY_DISC_SYSTEM_CNF_HAVE_STACK) &&
          metadata.tcb == 0x40u && metadata.event == 0x100u &&
          metadata.stack == UINT32_C(0x801fff00),
          "expected normalized boot name and bounded TCB/EVENT/STACK values");
    before = metadata;
    CHECK(schway_disc_image_parse_system_cnf((const uint8_t *)unsafe_boot,
                                              sizeof unsafe_boot - 1u, &metadata) ==
              SCHWAY_DISC_IMAGE_BAD_SYSCNF && memcmp(&metadata, &before, sizeof metadata) == 0,
          "expected parent traversal to fail without changing metadata output");
    CHECK(schway_disc_image_parse_system_cnf((const uint8_t *)bad_tcb, sizeof bad_tcb - 1u,
                                              &metadata) == SCHWAY_DISC_IMAGE_BAD_SYSCNF &&
          schway_disc_image_parse_system_cnf((const uint8_t *)bad_event, sizeof bad_event - 1u,
                                              &metadata) == SCHWAY_DISC_IMAGE_BAD_SYSCNF &&
          schway_disc_image_parse_system_cnf((const uint8_t *)bad_stack, sizeof bad_stack - 1u,
                                              &metadata) == SCHWAY_DISC_IMAGE_BAD_SYSCNF,
          "expected out-of-range TCB/EVENT/STACK values to fail");
    uint8_t too_long[SCHWAY_DISC_SYSTEM_CNF_MAX_BYTES + 1u];
    memset(too_long, 'A', sizeof too_long);
    CHECK(schway_disc_image_parse_system_cnf(too_long, sizeof too_long, &metadata) ==
              SCHWAY_DISC_IMAGE_BAD_SYSCNF,
          "expected SYSTEM.CNF longer than 2,047 bytes to fail at the bound");

    CHECK(write_probe_iso(path, valid, sizeof valid - 1u, "SLUS_123.45", 0),
          "could not create valid metadata ISO");
    memset(&probe, 0, sizeof probe);
    schway_host_disc_probe_status probe_status = schway__disc_probe_image(path, &probe);
    if (probe_status != SCHWAY_HOST_DISC_PROBE_OK ||
        strcmp(probe.boot_name, "SLUS_123.45") != 0 ||
        strcmp(probe.serial, "SLUS-12345") != 0 || probe.exe_size != 2048u) {
        fprintf(stderr, "probe=%s boot='%s' serial='%s' size=%zu\n",
                schway__disc_probe_status_string(probe_status), probe.boot_name,
                probe.serial, probe.exe_size);
    }
    CHECK(probe_status == SCHWAY_HOST_DISC_PROBE_OK &&
          strcmp(probe.boot_name, "SLUS_123.45") == 0 &&
          strcmp(probe.serial, "SLUS-12345") == 0 && probe.exe_size == 2048u,
          "expected the identity probe to use the shared parser and resolve its boot file");
    schway__disc_probe_dispose(&probe);

    CHECK(write_probe_iso(path, unsafe_boot, sizeof unsafe_boot - 1u, "SLUS_123.45", 0),
          "could not create traversal CNF ISO");
    memset(&probe, 0, sizeof probe);
    memcpy(probe.boot_name, "preserve", sizeof "preserve");
    CHECK(schway__disc_probe_image(path, &probe) == SCHWAY_HOST_DISC_PROBE_BAD_SYSTEM_CNF &&
          strcmp(probe.boot_name, "preserve") == 0 && probe.exe_bytes == NULL,
          "expected invalid CNF to preserve caller metadata and executable ownership");

    CHECK(write_probe_iso(path, NULL, 0u, NULL, 1),
          "could not create PSX.EXE fallback ISO");
    memset(&probe, 0, sizeof probe);
    CHECK(schway__disc_probe_image(path, &probe) == SCHWAY_HOST_DISC_PROBE_OK &&
          strcmp(probe.boot_name, "PSX.EXE") == 0 && probe.exe_size == 2048u,
          "expected missing SYSTEM.CNF to fall back to root PSX.EXE");
    schway__disc_probe_dispose(&probe);
    CHECK(write_probe_iso(path, NULL, 0u, NULL, 0),
          "could not create ISO without boot metadata or fallback");
    memset(&probe, 0, sizeof probe);
    CHECK(schway__disc_probe_image(path, &probe) == SCHWAY_HOST_DISC_PROBE_NO_BOOT_EXE &&
          probe.exe_bytes == NULL,
          "expected missing SYSTEM.CNF and PSX.EXE to report NO_BOOT_EXE");

cleanup:
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
    if (strcmp(argv[1], "cue_modes") == 0) return test_cue_modes(argv[2]);
    if (strcmp(argv[1], "cue_root_child") == 0) return test_cue_root_child(argv[2]);
    if (strcmp(argv[1], "cue_pregap") == 0) return test_cue_pregap(argv[2]);
    if (strcmp(argv[1], "cue_reject") == 0) return test_cue_reject(argv[2]);
    if (strcmp(argv[1], "cue_symlink") == 0) return test_cue_symlink(argv[2]);
    if (strcmp(argv[1], "cue_limits") == 0) return test_cue_limits(argv[2]);
    if (strcmp(argv[1], "cue_index_limits") == 0) return test_cue_index_limits(argv[2]);
    if (strcmp(argv[1], "iso_limits") == 0) return test_iso_limits(argv[2]);
    if (strcmp(argv[1], "playlist") == 0) return test_playlist(argv[2]);
    if (strcmp(argv[1], "cnf") == 0) return test_cnf(argv[2]);
    fprintf(stderr, "unknown disc image test case: %s\n", argv[1]);
    return 2;
}
