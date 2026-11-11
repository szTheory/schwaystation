/* Raw card import and atomic persistence through the host helper. */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "cards.h"
#include "../check.h"

#define TEMP_PATH_CAP 256u
#define TEMP_CANDIDATES 64u

static int make_directory(const char *path)
{
#ifdef _WIN32
    return _mkdir(path);
#else
    return mkdir(path, 0700);
#endif
}

static int remove_directory(const char *path)
{
#ifdef _WIN32
    return _rmdir(path);
#else
    return rmdir(path);
#endif
}

static int create_work_directory(char *path, size_t capacity)
{
    for (unsigned attempt = 0u; attempt < 100u; attempt++) {
        int written = snprintf(path, capacity, "card-files-%u", attempt);
        if (written < 0 || (size_t)written >= capacity) {
            return 0;
        }
        if (make_directory(path) == 0) {
            return 1;
        }
        if (errno != EEXIST) {
            return 0;
        }
    }
    return 0;
}

static int set_path(char *out, size_t capacity, const char *directory, const char *name)
{
    int written = snprintf(out, capacity, "%s/%s", directory, name);
    return written >= 0 && (size_t)written < capacity;
}

static int write_file(const char *path, const void *bytes, size_t size)
{
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return 0;
    }
    int ok = fwrite(bytes, 1u, size, file) == size;
    if (fclose(file) != 0) {
        ok = 0;
    }
    return ok;
}

static int read_file(const char *path, void *out, size_t capacity, size_t *size)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return 0;
    }
    size_t read = fread(out, 1u, capacity, file);
    int extra = fgetc(file);
    int read_error = ferror(file);
    int close_error = fclose(file);
    if (read_error || extra != EOF || close_error != 0) {
        return 0;
    }
    *size = read;
    return 1;
}

static int set_temp_path(char *out, size_t capacity, const char *destination, unsigned index)
{
    int written = snprintf(out, capacity, "%s.schwaystation.tmp.%u", destination, index);
    return written >= 0 && (size_t)written < capacity;
}

static void atomic_roundtrip(void)
{
    char directory[TEMP_PATH_CAP] = {0};
    char source[TEMP_PATH_CAP] = {0};
    char destination[TEMP_PATH_CAP] = {0};
    char temporary[TEMP_PATH_CAP] = {0};
    uint8_t raw[37];
    uint8_t source_readback[sizeof raw];
    uint8_t marker = 0xA7u;
    uint8_t *initial = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    uint8_t *imported = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    uint8_t *dirty = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    uint8_t *readback = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    int made_directory = 0;
    int have_source = 0;
    int have_destination = 0;
    int have_temp[TEMP_CANDIDATES] = {0};

    CHECK(initial != NULL && imported != NULL && dirty != NULL && readback != NULL);
    if (initial == NULL || imported == NULL || dirty == NULL || readback == NULL) {
        goto cleanup;
    }
    made_directory = create_work_directory(directory, sizeof directory);
    CHECK(made_directory != 0);
    if (!made_directory || !set_path(source, sizeof source, directory, "source.raw") ||
        !set_path(destination, sizeof destination, directory, "save.mcr")) {
        CHECK(0);
        goto cleanup;
    }

    for (size_t i = 0u; i < sizeof raw; i++) {
        raw[i] = (uint8_t)(i * 17u + 3u);
    }
    memset(initial, 0x5Au, SCHWAY_MEMCARD_SIZE);
    memset(imported, 0x11, SCHWAY_MEMCARD_SIZE);
    CHECK(write_file(source, raw, sizeof raw));
    have_source = 1;
    CHECK(write_file(destination, initial, SCHWAY_MEMCARD_SIZE));
    have_destination = 1;

    schway_host_card_lint lint = {0u, 0u};
    schway_host_card_status status =
        schway_host_card_import_raw(source, imported, SCHWAY_MEMCARD_SIZE, &lint);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    if (status != SCHWAY_HOST_CARD_OK) {
        goto cleanup;
    }
    CHECK(memcmp(imported, raw, sizeof raw) == 0);
    size_t source_size = 0u;
    CHECK(read_file(source, source_readback, sizeof source_readback, &source_size));
    CHECK_EQ_U64(source_size, sizeof raw);
    CHECK(memcmp(source_readback, raw, sizeof raw) == 0);
    for (size_t i = sizeof raw; i < SCHWAY_MEMCARD_SIZE; i++) {
        if (imported[i] != 0xFFu) {
            CHECK(imported[i] == 0xFFu);
            break;
        }
    }
    for (size_t i = 0u; i < SCHWAY_MEMCARD_SIZE; i++) {
        dirty[i] = (uint8_t)(imported[i] ^ (uint8_t)(i * 29u));
    }

    CHECK(set_temp_path(temporary, sizeof temporary, destination, 0u));
    if (temporary[0] == '\0' || !write_file(temporary, &marker, sizeof marker)) {
        CHECK(0);
        goto cleanup;
    }
    have_temp[0] = 1;
    status = schway_host_card_save_atomic(destination, dirty, SCHWAY_MEMCARD_SIZE);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    if (status != SCHWAY_HOST_CARD_OK) {
        goto cleanup;
    }
    size_t readback_size = 0u;
    CHECK(read_file(destination, readback, SCHWAY_MEMCARD_SIZE, &readback_size));
    CHECK_EQ_U64(readback_size, SCHWAY_MEMCARD_SIZE);
    CHECK(memcmp(readback, dirty, SCHWAY_MEMCARD_SIZE) == 0);
    uint8_t marker_read = 0u;
    size_t marker_size = 0u;
    CHECK(read_file(temporary, &marker_read, sizeof marker_read, &marker_size));
    CHECK_EQ_U64(marker_size, sizeof marker);
    CHECK_EQ_HEX(marker_read, marker);

    for (unsigned i = 0u; i < TEMP_CANDIDATES; i++) {
        if (i == 0u) {
            continue;
        }
        CHECK(set_temp_path(temporary, sizeof temporary, destination, i));
        uint8_t candidate_marker = (uint8_t)(0x40u + i);
        CHECK(write_file(temporary, &candidate_marker, sizeof candidate_marker));
        have_temp[i] = 1;
    }
    memset(dirty, 0xC3, SCHWAY_MEMCARD_SIZE);
    status = schway_host_card_save_atomic(destination, dirty, SCHWAY_MEMCARD_SIZE);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_ERR_IO);
    CHECK(read_file(destination, readback, SCHWAY_MEMCARD_SIZE, &readback_size));
    CHECK_EQ_U64(readback_size, SCHWAY_MEMCARD_SIZE);
    for (size_t i = 0u; i < SCHWAY_MEMCARD_SIZE; i++) {
        if (readback[i] != (uint8_t)(imported[i] ^ (uint8_t)(i * 29u))) {
            CHECK(0);
            break;
        }
    }

cleanup:
    for (unsigned i = 0u; i < TEMP_CANDIDATES; i++) {
        if (have_temp[i]) {
            if (set_temp_path(temporary, sizeof temporary, destination, i)) {
                (void)remove(temporary);
            }
        }
    }
    if (have_source) {
        (void)remove(source);
    }
    if (have_destination) {
        (void)remove(destination);
    }
    if (made_directory) {
        (void)remove_directory(directory);
    }
    free(readback);
    free(dirty);
    free(imported);
    free(initial);
}

int main(int argc, char **argv)
{
    if (argc != 2 || strcmp(argv[1], "atomic_roundtrip") != 0) {
        fprintf(stderr, "expected one known card file test name\n");
        return 2;
    }
    atomic_roundtrip();
    CHECK_DONE();
}
