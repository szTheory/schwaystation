#if !defined(_WIN32) && !defined(_XOPEN_SOURCE)
#define _XOPEN_SOURCE 700
#endif
#include "disc_pbp.h"
#include "media_limits.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32)
#include <sys/types.h>
#endif

#define SCHWAY__DISC_PBP_HEADER_BYTES 40u
#define SCHWAY__DISC_PBP_OFFSET_COUNT 8u
#define SCHWAY__DISC_PBP_PSAR_MARKER_BYTES 12u

static schway_disc_image_status schway__disc_pbp_error(
    schway_disc_image_error *error, schway_disc_image_status status,
    const char *message)
{
    if (error != NULL) {
        memset(error, 0, sizeof(*error));
        error->size = (uint32_t)sizeof(*error);
        error->status = (uint32_t)status;
        (void)snprintf(error->message, sizeof(error->message), "%s", message);
    }
    return status;
}

static uint32_t schway__disc_pbp_le32(const uint8_t bytes[4])
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static int schway__disc_pbp_file_size(FILE *file, uint64_t *size)
{
#if defined(_WIN32)
    __int64 end;
    if (_fseeki64(file, 0, SEEK_END) != 0 ||
        (end = _ftelli64(file)) < 0 || _fseeki64(file, 0, SEEK_SET) != 0) {
        return 0;
    }
#else
    off_t end;
    if (fseeko(file, 0, SEEK_END) != 0 ||
        (end = ftello(file)) < (off_t)0 || fseeko(file, 0, SEEK_SET) != 0) {
        return 0;
    }
#endif
    *size = (uint64_t)end;
    return 1;
}

schway_disc_image_status schway__disc_pbp_open(const char *path,
                                                schway__disc_pbp **out,
                                                schway_disc_image_error *error)
{
    uint8_t header[SCHWAY__DISC_PBP_HEADER_BYTES];
    uint8_t marker[SCHWAY__DISC_PBP_PSAR_MARKER_BYTES];
    uint32_t offsets[SCHWAY__DISC_PBP_OFFSET_COUNT];
    uint64_t file_bytes;
    FILE *file;

    if (out != NULL) *out = NULL;
    if (path == NULL || path[0] == '\0' || out == NULL) {
        return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_BAD_ARGUMENT,
                                      "PBP path and output are required");
    }
    file = fopen(path, "rb");
    if (file == NULL) {
        return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_IO,
                                      "could not open PBP file");
    }
    if (!schway__disc_pbp_file_size(file, &file_bytes)) {
        (void)fclose(file);
        return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_IO,
                                      "could not determine PBP file size");
    }
    if (file_bytes > SCHWAY__MEDIA_IMAGE_MAX_BYTES) {
        (void)fclose(file);
        return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                      "PBP file exceeds the 1 GiB limit");
    }
    if (file_bytes < sizeof(header)) {
        (void)fclose(file);
        return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "PBP header is truncated");
    }
    if (fread(header, 1u, sizeof(header), file) != sizeof(header)) {
        schway_disc_image_status status = ferror(file) != 0 ?
            SCHWAY_DISC_IMAGE_IO : SCHWAY_DISC_IMAGE_BAD_IMAGE;
        (void)fclose(file);
        return schway__disc_pbp_error(error, status, "could not read PBP header");
    }
    if (memcmp(header, "\0PBP", 4u) != 0) {
        (void)fclose(file);
        return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "PBP header magic is invalid");
    }
    for (uint32_t i = 0u; i < SCHWAY__DISC_PBP_OFFSET_COUNT; i++) {
        offsets[i] = schway__disc_pbp_le32(header + 8u + (size_t)i * 4u);
        if (offsets[i] < SCHWAY__DISC_PBP_HEADER_BYTES ||
            (uint64_t)offsets[i] > file_bytes ||
            (i != 0u && offsets[i] < offsets[i - 1u])) {
            (void)fclose(file);
            return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                          "PBP section offsets are invalid");
        }
    }
    if (file_bytes - offsets[SCHWAY__DISC_PBP_OFFSET_COUNT - 1u] < sizeof(marker)) {
        (void)fclose(file);
        return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "PBP data section is truncated");
    }
    if ((uint64_t)offsets[SCHWAY__DISC_PBP_OFFSET_COUNT - 1u] > (uint64_t)LONG_MAX ||
        fseek(file, (long)offsets[SCHWAY__DISC_PBP_OFFSET_COUNT - 1u], SEEK_SET) != 0) {
        (void)fclose(file);
        return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_IO,
                                      "could not seek to PBP data section");
    }
    if (fread(marker, 1u, sizeof(marker), file) != sizeof(marker)) {
        schway_disc_image_status status = ferror(file) != 0 ?
            SCHWAY_DISC_IMAGE_IO : SCHWAY_DISC_IMAGE_BAD_IMAGE;
        (void)fclose(file);
        return schway__disc_pbp_error(error, status, "could not read PBP data marker");
    }
    (void)fclose(file);
    if (memcmp(marker, "PSISOIMG0000", sizeof(marker)) != 0) {
        return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_UNSUPPORTED,
                                      "PBP encrypted or unknown data marker is unsupported");
    }
    return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_UNSUPPORTED,
                                  "PBP PSISO parsing is not available in this provider slice");
}

void schway__disc_pbp_close(schway__disc_pbp *reader)
{
    free(reader);
}

schway_disc_image_status schway__disc_pbp_toc(const schway__disc_pbp *reader,
                                               schway_toc *out)
{
    if (reader == NULL || out == NULL) return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    return SCHWAY_DISC_IMAGE_UNSUPPORTED;
}

uint32_t schway__disc_pbp_read_sector(void *user, uint32_t lba,
                                      uint8_t sector[2352], uint8_t subcode[96])
{
    (void)user;
    (void)lba;
    (void)sector;
    (void)subcode;
    return 0u;
}
