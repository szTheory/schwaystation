/* Host-only file operations for raw memory cards. */
#ifndef _WIN32
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif
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
#include <share.h>
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
#define SCHWAY_HOST_CARD_GME_HEADER_SIZE 0x0F40u
#define SCHWAY_HOST_CARD_VGS_HEADER_SIZE 64u
#define SCHWAY_HOST_CARD_PSX_HEADER_SIZE 256u
#define SCHWAY_HOST_CARD_SAVE_BLOCK_SIZE 8192u
#define SCHWAY_HOST_CARD_SAVE_BLOCK_COUNT 15u
#define SCHWAY_HOST_CARD_BASENAME_CAP 64u
#define SCHWAY__HOST_CARD_FAILURE_NONE 0
#define SCHWAY__HOST_CARD_FAILURE_FLUSH 1
#define SCHWAY__HOST_CARD_FAILURE_RENAME 2

static int schway__host_cards_open_temp(const char *path)
{
#ifdef _WIN32
    int fd = -1;
    if (_sopen_s(&fd, path, _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY, _SH_DENYNO,
                 _S_IREAD | _S_IWRITE) != 0) {
        return -1;
    }
    return fd;
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

static schway_host_card_status schway__host_cards_open_read(const char *path, FILE **file)
{
#if defined(_MSC_VER)
    int open_error = fopen_s(file, path, "rb");
    if (open_error != 0) {
        errno = open_error;
    }
#else
    *file = fopen(path, "rb");
#endif
    if (*file != NULL) {
        return SCHWAY_HOST_CARD_OK;
    }
    return errno == ENOENT ? SCHWAY_HOST_CARD_ERR_NOT_FOUND : SCHWAY_HOST_CARD_ERR_IO;
}

static int schway__host_cards_finish_read(FILE *file, int *extra)
{
    int read_error = ferror(file);
    *extra = EOF;
    if (!read_error) {
        *extra = fgetc(file);
    }
    int trailing_error = ferror(file);
    int close_error = fclose(file);
    return !read_error && !trailing_error && close_error == 0;
}

static int schway__host_cards_extension_is(const char *path, const char *extension)
{
    const char *name = path;
    for (const char *cursor = path; *cursor != '\0'; cursor++) {
        if (*cursor == '/' || *cursor == '\\') {
            name = cursor + 1;
        }
    }
    const char *dot = strrchr(name, '.');
    if (dot == NULL) {
        return 0;
    }
    dot++;
    size_t extension_size = strlen(extension);
    if (strlen(dot) != extension_size) {
        return 0;
    }
    for (size_t i = 0u; i < extension_size; i++) {
        char value = dot[i];
        if (value >= 'A' && value <= 'Z') {
            value = (char)(value + ('a' - 'A'));
        }
        if (value != extension[i]) {
            return 0;
        }
    }
    return 1;
}

static int schway__host_cards_partial_signature(const uint8_t *bytes, size_t size,
                                                const char *signature,
                                                size_t signature_size)
{
    return size != 0u && size < signature_size && memcmp(bytes, signature, size) == 0;
}

static int schway__host_cards_ascii_field_terminated(const uint8_t *bytes, size_t size,
                                                      int require_nonempty)
{
    for (size_t i = 0u; i < size; i++) {
        if (bytes[i] == 0u) {
            return !require_nonempty || i != 0u;
        }
        if (bytes[i] < 0x20u || bytes[i] > 0x7Eu) {
            return 0;
        }
    }
    return 0;
}

static int schway__host_cards_save_blocks_valid(size_t image_size, size_t header_size)
{
    if (image_size < header_size) {
        return 0;
    }
    size_t payload_size = image_size - header_size;
    return payload_size >= SCHWAY_HOST_CARD_SAVE_BLOCK_SIZE &&
           payload_size <= SCHWAY_HOST_CARD_SAVE_BLOCK_SIZE * SCHWAY_HOST_CARD_SAVE_BLOCK_COUNT &&
           payload_size % SCHWAY_HOST_CARD_SAVE_BLOCK_SIZE == 0u;
}

static int schway__host_cards_xploder_save(const char *path, const uint8_t *bytes, size_t size)
{
    const size_t header_size = 54u;
    return schway__host_cards_extension_is(path, "psx") &&
           schway__host_cards_save_blocks_valid(size, header_size) &&
           schway__host_cards_ascii_field_terminated(bytes, 21u, 1) &&
           schway__host_cards_ascii_field_terminated(bytes + 21u, 33u, 0) &&
           bytes[header_size] == (uint8_t)'S' && bytes[header_size + 1u] == (uint8_t)'C';
}

static int schway__host_cards_mcs_save(const char *path, const uint8_t *bytes, size_t size)
{
    const size_t header_size = 128u;
    return schway__host_cards_extension_is(path, "mcs") &&
           schway__host_cards_save_blocks_valid(size, header_size) &&
           bytes[header_size] == (uint8_t)'S' && bytes[header_size + 1u] == (uint8_t)'C';
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

    FILE *file = NULL;
    schway_host_card_status open_status = schway__host_cards_open_read(path, &file);
    if (open_status != SCHWAY_HOST_CARD_OK) {
        free(image);
        return open_status;
    }

    size_t count = fread(image, 1u, SCHWAY_MEMCARD_SIZE, file);
    int extra = EOF;
    if (!schway__host_cards_finish_read(file, &extra)) {
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

schway_host_card_status schway_host_card_import_file(const char *path, void *out,
                                                      size_t out_size,
                                                      schway_host_card_lint *lint)
{
    if (path == NULL || path[0] == '\0' || out == NULL || out_size != SCHWAY_MEMCARD_SIZE) {
        return SCHWAY_HOST_CARD_ERR_ARGUMENT;
    }

    const size_t input_capacity =
        (size_t)SCHWAY_HOST_CARD_GME_HEADER_SIZE + (size_t)SCHWAY_MEMCARD_SIZE;
    uint8_t *input = (uint8_t *)malloc(input_capacity);
    uint8_t *image = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    if (input == NULL || image == NULL) {
        free(image);
        free(input);
        return SCHWAY_HOST_CARD_ERR_IO;
    }

    FILE *file = NULL;
    schway_host_card_status open_status = schway__host_cards_open_read(path, &file);
    if (open_status != SCHWAY_HOST_CARD_OK) {
        free(image);
        free(input);
        return open_status;
    }
    size_t count = fread(input, 1u, input_capacity, file);
    int extra = EOF;
    if (!schway__host_cards_finish_read(file, &extra)) {
        free(image);
        free(input);
        return SCHWAY_HOST_CARD_ERR_IO;
    }
    if (extra != EOF || count == 0u) {
        free(image);
        free(input);
        return SCHWAY_HOST_CARD_ERR_SIZE;
    }

    const uint8_t *card = input;
    size_t card_size = count;
    int gme_extension = schway__host_cards_extension_is(path, "gme");
    int vgs_extension = schway__host_cards_extension_is(path, "mem") ||
                        schway__host_cards_extension_is(path, "vgs");
    int psx_extension = schway__host_cards_extension_is(path, "psx");
    int psv_extension = schway__host_cards_extension_is(path, "psv");
    int gme_signature = count >= 11u && memcmp(input, "123-456-STD", 11u) == 0;
    int vgs_signature = count >= 4u && memcmp(input, "VgsM", 4u) == 0;
    int psv_signature = count >= 3u && memcmp(input, "PSV", 3u) == 0;
    static const uint8_t gme_marker_fields[5] = {0u, 0u, 1u, 0u, 1u};

    if ((gme_extension && schway__host_cards_partial_signature(input, count,
                                                                "123-456-STD", 11u)) ||
        (vgs_extension && schway__host_cards_partial_signature(input, count, "VgsM", 4u)) ||
        ((psx_extension || psv_extension) &&
         schway__host_cards_partial_signature(input, count, "PSV", 3u))) {
        free(image);
        free(input);
        return SCHWAY_HOST_CARD_ERR_SIZE;
    }

    if (gme_signature) {
        if (count != (size_t)SCHWAY_HOST_CARD_GME_HEADER_SIZE + SCHWAY_MEMCARD_SIZE ||
            input[11] != 0u ||
            memcmp(input + 0x10u, gme_marker_fields, sizeof gme_marker_fields) != 0) {
            free(image);
            free(input);
            return SCHWAY_HOST_CARD_ERR_SIZE;
        }
        card = input + SCHWAY_HOST_CARD_GME_HEADER_SIZE;
        card_size = SCHWAY_MEMCARD_SIZE;
    } else if (vgs_signature) {
        if (count != (size_t)SCHWAY_HOST_CARD_VGS_HEADER_SIZE + SCHWAY_MEMCARD_SIZE) {
            free(image);
            free(input);
            return SCHWAY_HOST_CARD_ERR_SIZE;
        }
        card = input + SCHWAY_HOST_CARD_VGS_HEADER_SIZE;
        card_size = SCHWAY_MEMCARD_SIZE;
    } else if (psv_signature && psv_extension) {
        free(image);
        free(input);
        return SCHWAY_HOST_CARD_ERR_SIZE;
    } else if (psv_signature) {
        if (count != (size_t)SCHWAY_HOST_CARD_PSX_HEADER_SIZE + SCHWAY_MEMCARD_SIZE) {
            free(image);
            free(input);
            return SCHWAY_HOST_CARD_ERR_SIZE;
        }
        card = input + SCHWAY_HOST_CARD_PSX_HEADER_SIZE;
        card_size = SCHWAY_MEMCARD_SIZE;
    } else if (schway__host_cards_xploder_save(path, input, count) ||
               schway__host_cards_mcs_save(path, input, count)) {
        free(image);
        free(input);
        return SCHWAY_HOST_CARD_ERR_SIZE;
    } else if (count > SCHWAY_MEMCARD_SIZE) {
        free(image);
        free(input);
        return SCHWAY_HOST_CARD_ERR_SIZE;
    }

    memset(image, 0xFF, SCHWAY_MEMCARD_SIZE);
    memcpy(image, card, card_size);
    schway_host_card_lint result;
    schway__host_cards_lint(image, &result);
    memcpy(out, image, SCHWAY_MEMCARD_SIZE);
    if (lint != NULL) {
        *lint = result;
    }
    free(image);
    free(input);
    return SCHWAY_HOST_CARD_OK;
}

static int schway__host_cards_basename_length(const char *basename, size_t *length)
{
    if (basename == NULL || length == NULL) {
        return 0;
    }
    for (size_t i = 0u; i < SCHWAY_HOST_CARD_BASENAME_CAP; i++) {
        char value = basename[i];
        if (value == '\0') {
            if (i == 0u) {
                return 0;
            }
            *length = i;
            return 1;
        }
        if (!((value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z') ||
              (value >= '0' && value <= '9') || value == '_' || value == '-' || value == '.')) {
            return 0;
        }
        if (i == 0u && !((value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z') ||
                         (value >= '0' && value <= '9'))) {
            return 0;
        }
    }
    return 0;
}

static int schway__host_cards_name_size(size_t directory_size, size_t separator_size,
                                         size_t serial_size, size_t suffix_size,
                                         size_t *required)
{
    size_t size = directory_size;
    if (size > SIZE_MAX - separator_size) {
        return 0;
    }
    size += separator_size;
    if (size > SIZE_MAX - serial_size) {
        return 0;
    }
    size += serial_size;
    if (size > SIZE_MAX - suffix_size) {
        return 0;
    }
    *required = size + suffix_size;
    return 1;
}

static void schway__host_cards_write_name(char *out, const char *directory,
                                          size_t directory_size, size_t separator_size,
                                          const char *serial, size_t serial_size,
                                          const char *suffix, size_t suffix_size)
{
    size_t offset = 0u;
    memcpy(out + offset, directory, directory_size);
    offset += directory_size;
    if (separator_size != 0u) {
        out[offset++] = '/';
    }
    memcpy(out + offset, serial, serial_size);
    offset += serial_size;
    memcpy(out + offset, suffix, suffix_size);
}

static schway_host_card_status schway__host_cards_paths_for_component(
    const char *directory, const char *component, size_t component_size,
    char *slot1, size_t slot1_size, char *slot2, size_t slot2_size)
{
    if (directory == NULL || component == NULL || slot1 == NULL || slot2 == NULL ||
        slot1 == slot2) {
        return SCHWAY_HOST_CARD_ERR_ARGUMENT;
    }

    size_t directory_size = strlen(directory);
    size_t separator_size =
        directory_size != 0u && directory[directory_size - 1u] != '/' &&
                directory[directory_size - 1u] != '\\'
            ? 1u
            : 0u;
    size_t slot1_required = 0u;
    size_t slot2_required = 0u;
    if (!schway__host_cards_name_size(directory_size, separator_size, component_size,
                                       sizeof(".srm"), &slot1_required) ||
        !schway__host_cards_name_size(directory_size, separator_size, component_size,
                                       sizeof(".1.mcr"), &slot2_required)) {
        return SCHWAY_HOST_CARD_ERR_SIZE;
    }
    if (slot1_size < slot1_required || slot2_size < slot2_required) {
        return SCHWAY_HOST_CARD_ERR_SIZE;
    }

    schway__host_cards_write_name(slot1, directory, directory_size, separator_size,
                                  component, component_size, ".srm", sizeof(".srm"));
    schway__host_cards_write_name(slot2, directory, directory_size, separator_size,
                                  component, component_size, ".1.mcr", sizeof(".1.mcr"));
    return SCHWAY_HOST_CARD_OK;
}

schway_host_card_status schway_host_card_paths_for_basename(const char *directory,
                                                             const char *basename,
                                                             char *slot1, size_t slot1_size,
                                                             char *slot2, size_t slot2_size)
{
    size_t basename_size = 0u;
    if (!schway__host_cards_basename_length(basename, &basename_size)) {
        return SCHWAY_HOST_CARD_ERR_ARGUMENT;
    }
    return schway__host_cards_paths_for_component(directory, basename, basename_size,
                                                 slot1, slot1_size, slot2, slot2_size);
}

schway_host_card_status schway__host_card_paths_for_identity(const char *directory,
                                                              const char *identity,
                                                              char *slot1, size_t slot1_size,
                                                              char *slot2, size_t slot2_size)
{
    static const char digest_v1_prefix[] = "sgi1-";
    static const char digest_v2_prefix[] = "sgi2-";
    if (identity == NULL) {
        return SCHWAY_HOST_CARD_ERR_ARGUMENT;
    }
    size_t prefix_size;
    if (strncmp(identity, digest_v1_prefix, sizeof(digest_v1_prefix) - 1u) == 0) {
        prefix_size = sizeof(digest_v1_prefix) - 1u;
    } else if (strncmp(identity, digest_v2_prefix, sizeof(digest_v2_prefix) - 1u) == 0) {
        prefix_size = sizeof(digest_v2_prefix) - 1u;
    } else {
        return schway_host_card_paths_for_basename(directory, identity,
                                                  slot1, slot1_size, slot2, slot2_size);
    }

    size_t identity_size = strlen(identity);
    if (identity_size != prefix_size + 64u) {
        return SCHWAY_HOST_CARD_ERR_ARGUMENT;
    }
    for (size_t i = prefix_size; i < identity_size; i++) {
        char value = identity[i];
        if (!((value >= '0' && value <= '9') || (value >= 'a' && value <= 'f'))) {
            return SCHWAY_HOST_CARD_ERR_ARGUMENT;
        }
    }
    return schway__host_cards_paths_for_component(directory, identity, identity_size,
                                                 slot1, slot1_size, slot2, slot2_size);
}

static schway_host_card_status schway__host_cards_publish_atomic(
    const char *path, const void *bytes, size_t size, int failure)
{
    if (path == NULL || path[0] == '\0' || bytes == NULL) {
        return SCHWAY_HOST_CARD_ERR_ARGUMENT;
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

static size_t schway__host_cards_export_header_size(schway_host_card_format format)
{
    switch (format) {
    case SCHWAY_HOST_CARD_FORMAT_RAW:
        return 0u;
    case SCHWAY_HOST_CARD_FORMAT_GME:
        return SCHWAY_HOST_CARD_GME_HEADER_SIZE;
    case SCHWAY_HOST_CARD_FORMAT_VGS:
        return SCHWAY_HOST_CARD_VGS_HEADER_SIZE;
    case SCHWAY_HOST_CARD_FORMAT_PSV:
        return SCHWAY_HOST_CARD_PSX_HEADER_SIZE;
    }
    return SIZE_MAX;
}

static schway_host_card_status schway__host_cards_export_atomic(
    const char *path, const void *bytes, size_t size, schway_host_card_format format,
    int failure)
{
    if (path == NULL || path[0] == '\0' || bytes == NULL) {
        return SCHWAY_HOST_CARD_ERR_ARGUMENT;
    }
    if (size != SCHWAY_MEMCARD_SIZE) {
        return SCHWAY_HOST_CARD_ERR_SIZE;
    }
    size_t header_size = schway__host_cards_export_header_size(format);
    if (header_size == SIZE_MAX || header_size > SIZE_MAX - size) {
        return SCHWAY_HOST_CARD_ERR_ARGUMENT;
    }
    size_t output_size = header_size + size;
    uint8_t *output = (uint8_t *)malloc(output_size);
    if (output == NULL) {
        return SCHWAY_HOST_CARD_ERR_IO;
    }
    if (header_size != 0u) {
        memset(output, 0, header_size);
    }

    const uint8_t *card = (const uint8_t *)bytes;
    if (format == SCHWAY_HOST_CARD_FORMAT_GME) {
        static const uint8_t marker_fields[5] = {0u, 0u, 1u, 0u, 1u};
        memcpy(output, "123-456-STD", 11u);
        output[11] = 0u;
        memcpy(output + 0x10u, marker_fields, sizeof marker_fields);
        for (size_t sector = 0u; sector < 16u; sector++) {
            output[0x15u + sector] = card[sector * SCHWAY_HOST_CARD_FRAME_SIZE];
            output[0x25u + sector] = card[sector * SCHWAY_HOST_CARD_FRAME_SIZE + 8u];
        }
    } else if (format == SCHWAY_HOST_CARD_FORMAT_VGS) {
        memcpy(output, "VgsM", 4u);
    } else if (format == SCHWAY_HOST_CARD_FORMAT_PSV) {
        memcpy(output, "PSV", 3u);
    }
    memcpy(output + header_size, card, size);

    schway_host_card_status status =
        schway__host_cards_publish_atomic(path, output, output_size, failure);
    free(output);
    return status;
}

schway_host_card_status schway_host_card_save_atomic(const char *path, const void *bytes,
                                                      size_t size)
{
    return schway__host_cards_export_atomic(path, bytes, size, SCHWAY_HOST_CARD_FORMAT_RAW,
                                            SCHWAY__HOST_CARD_FAILURE_NONE);
}

schway_host_card_status schway_host_card_export_atomic(const char *path, const void *bytes,
                                                        size_t size,
                                                        schway_host_card_format format)
{
    return schway__host_cards_export_atomic(path, bytes, size, format,
                                            SCHWAY__HOST_CARD_FAILURE_NONE);
}

#if defined(SCHWAY_HOST_CARD_TESTING)
schway_host_card_status schway_host_card_test_save_atomic(const char *path, const void *bytes,
                                                           size_t size,
                                                           schway_host_card_test_failure failure)
{
    return schway__host_cards_export_atomic(path, bytes, size, SCHWAY_HOST_CARD_FORMAT_RAW,
                                            (int)failure);
}

schway_host_card_status schway_host_card_test_export_atomic(
    const char *path, const void *bytes, size_t size, schway_host_card_format format,
    schway_host_card_test_failure failure)
{
    return schway__host_cards_export_atomic(path, bytes, size, format, (int)failure);
}
#endif
