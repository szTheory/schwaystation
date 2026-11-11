/* Host-only file operations for raw memory cards. */
#include "cards.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define SCHWAY_HOST_CARD_FRAME_SIZE 128u
#define SCHWAY_HOST_CARD_FRAME_COUNT (SCHWAY_MEMCARD_SIZE / SCHWAY_HOST_CARD_FRAME_SIZE)
#define SCHWAY_HOST_CARD_HEADER_FRAME 63u
#define SCHWAY_HOST_CARD_TEMP_CANDIDATES 64u
#define SCHWAY_HOST_CARD_TEMP_SUFFIX_CAP 24u
#define SCHWAY__HOST_CARD_FAILURE_NONE 0
#define SCHWAY__HOST_CARD_FAILURE_FLUSH 1
#define SCHWAY__HOST_CARD_FAILURE_RENAME 2

static int schway__host_cards_open_temp(const char *path)
{
#ifdef _WIN32
    return _open(path, _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY, _S_IREAD | _S_IWRITE);
#else
    return open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
#endif
}

static FILE *schway__host_cards_fdopen(int fd)
{
#ifdef _WIN32
    return _fdopen(fd, "wb");
#else
    return fdopen(fd, "wb");
#endif
}

static int schway__host_cards_close_fd(int fd)
{
#ifdef _WIN32
    return _close(fd);
#else
    return close(fd);
#endif
}

static int schway__host_cards_unlink(const char *path)
{
#ifdef _WIN32
    return _unlink(path);
#else
    return unlink(path);
#endif
}

static int schway__host_cards_sync(FILE *file)
{
#ifdef _WIN32
    return _commit(_fileno(file));
#else
    return fsync(fileno(file));
#endif
}

static int schway__host_cards_replace(const char *temporary, const char *destination)
{
#ifdef _WIN32
    return MoveFileExA(temporary, destination,
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)
               ? 0
               : -1;
#else
    return rename(temporary, destination);
#endif
}

static uint8_t schway__host_cards_frame_checksum(const uint8_t *frame)
{
    uint8_t checksum = 0u;
    for (size_t i = 0u; i < SCHWAY_HOST_CARD_FRAME_SIZE - 1u; i++) {
        checksum ^= frame[i];
    }
    return checksum;
}

static void schway__host_cards_lint(const uint8_t *bytes, schway_host_card_lint *lint)
{
    schway_host_card_lint result = {0u, 0u};
    const size_t header_offset = (size_t)SCHWAY_HOST_CARD_HEADER_FRAME * SCHWAY_HOST_CARD_FRAME_SIZE;
    if (bytes[header_offset] != (uint8_t)'M' || bytes[header_offset + 1u] != (uint8_t)'C') {
        result.flags |= SCHWAY_HOST_CARD_LINT_BAD_HEADER;
    }
    for (size_t frame = 0u; frame < SCHWAY_HOST_CARD_FRAME_COUNT; frame++) {
        const uint8_t *contents = bytes + frame * SCHWAY_HOST_CARD_FRAME_SIZE;
        if (schway__host_cards_frame_checksum(contents) != contents[SCHWAY_HOST_CARD_FRAME_SIZE - 1u]) {
            result.bad_frame_checksums++;
        }
    }
    if (result.bad_frame_checksums != 0u) {
        result.flags |= SCHWAY_HOST_CARD_LINT_BAD_CHECKSUM;
    }
    *lint = result;
}

schway_host_card_status schway_host_card_import_raw(const char *path, void *out,
                                                     size_t out_size,
                                                     schway_host_card_lint *lint)
{
    if (path == NULL || path[0] == '\0' || out == NULL || out_size != SCHWAY_MEMCARD_SIZE) {
        return SCHWAY_HOST_CARD_ERR_ARGUMENT;
    }

    uint8_t *image = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    if (image == NULL) {
        return SCHWAY_HOST_CARD_ERR_IO;
    }
    memset(image, 0xFF, SCHWAY_MEMCARD_SIZE);

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        free(image);
        return SCHWAY_HOST_CARD_ERR_IO;
    }

    size_t count = fread(image, 1u, SCHWAY_MEMCARD_SIZE, file);
    int read_error = ferror(file);
    int extra = read_error ? EOF : fgetc(file);
    int close_error = fclose(file);
    if (read_error || close_error != 0) {
        free(image);
        return SCHWAY_HOST_CARD_ERR_IO;
    }
    if (extra != EOF || count == 0u) {
        free(image);
        return SCHWAY_HOST_CARD_ERR_SIZE;
    }

    schway_host_card_lint result;
    schway__host_cards_lint(image, &result);
    memcpy(out, image, SCHWAY_MEMCARD_SIZE);
    if (lint != NULL) {
        *lint = result;
    }
    free(image);
    return SCHWAY_HOST_CARD_OK;
}

static schway_host_card_status schway__host_cards_save_atomic(
    const char *path, const void *bytes, size_t size, int failure)
{
    if (path == NULL || path[0] == '\0' || bytes == NULL) {
        return SCHWAY_HOST_CARD_ERR_ARGUMENT;
    }
    if (size != SCHWAY_MEMCARD_SIZE) {
        return SCHWAY_HOST_CARD_ERR_SIZE;
    }
#if defined(SCHWAY_HOST_CARD_TESTING)
    if (failure < SCHWAY__HOST_CARD_FAILURE_NONE || failure > SCHWAY__HOST_CARD_FAILURE_RENAME) {
        return SCHWAY_HOST_CARD_ERR_ARGUMENT;
    }
#else
    (void)failure;
#endif

    size_t path_size = strlen(path);
    if (path_size > SIZE_MAX - SCHWAY_HOST_CARD_TEMP_SUFFIX_CAP) {
        return SCHWAY_HOST_CARD_ERR_ARGUMENT;
    }
    size_t temporary_capacity = path_size + SCHWAY_HOST_CARD_TEMP_SUFFIX_CAP;
    char *temporary = (char *)malloc(temporary_capacity);
    if (temporary == NULL) {
        return SCHWAY_HOST_CARD_ERR_IO;
    }

    int fd = -1;
    for (unsigned candidate = 0u; candidate < SCHWAY_HOST_CARD_TEMP_CANDIDATES; candidate++) {
        int written = snprintf(temporary, temporary_capacity, "%s.schwaystation.tmp.%u", path,
                              candidate);
        if (written < 0 || (size_t)written >= temporary_capacity) {
            free(temporary);
            return SCHWAY_HOST_CARD_ERR_ARGUMENT;
        }
        fd = schway__host_cards_open_temp(temporary);
        if (fd >= 0) {
            break;
        }
        if (errno != EEXIST) {
            free(temporary);
            return SCHWAY_HOST_CARD_ERR_IO;
        }
    }
    if (fd < 0) {
        free(temporary);
        return SCHWAY_HOST_CARD_ERR_IO;
    }

    FILE *file = schway__host_cards_fdopen(fd);
    if (file == NULL) {
        (void)schway__host_cards_close_fd(fd);
        (void)schway__host_cards_unlink(temporary);
        free(temporary);
        return SCHWAY_HOST_CARD_ERR_IO;
    }

    int failed = fwrite(bytes, 1u, size, file) != size;
    if (!failed && fflush(file) != 0) {
        failed = 1;
    }
#if defined(SCHWAY_HOST_CARD_TESTING)
    if (!failed && failure == SCHWAY__HOST_CARD_FAILURE_FLUSH) {
        failed = 1;
    }
#endif
    if (!failed && schway__host_cards_sync(file) != 0) {
        failed = 1;
    }
    if (fclose(file) != 0) {
        failed = 1;
    }

#if defined(SCHWAY_HOST_CARD_TESTING)
    if (!failed && failure == SCHWAY__HOST_CARD_FAILURE_RENAME) {
        failed = 1;
    }
#endif
    if (!failed && schway__host_cards_replace(temporary, path) != 0) {
        failed = 1;
    }
    if (failed) {
        (void)schway__host_cards_unlink(temporary);
    }
    free(temporary);
    return failed ? SCHWAY_HOST_CARD_ERR_IO : SCHWAY_HOST_CARD_OK;
}

schway_host_card_status schway_host_card_save_atomic(const char *path, const void *bytes,
                                                      size_t size)
{
    return schway__host_cards_save_atomic(path, bytes, size, SCHWAY__HOST_CARD_FAILURE_NONE);
}

#if defined(SCHWAY_HOST_CARD_TESTING)
schway_host_card_status schway_host_card_test_save_atomic(const char *path, const void *bytes,
                                                           size_t size,
                                                           schway_host_card_test_failure failure)
{
    return schway__host_cards_save_atomic(path, bytes, size, (int)failure);
}
#endif
