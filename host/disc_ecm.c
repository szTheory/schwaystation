#if !defined(_WIN32) && !defined(_XOPEN_SOURCE)
#define _XOPEN_SOURCE 700
#endif
#include "disc_ecm.h"
#include "media_limits.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32)
#include <sys/types.h>
#endif

struct schway__disc_ecm {
    FILE *file;
    uint64_t decoded_bytes;
};

static FILE *schway__disc_ecm_open_file(const char *path)
{
#if defined(_MSC_VER)
    FILE *file = NULL;
    if (fopen_s(&file, path, "rb") != 0) return NULL;
    return file;
#else
    return fopen(path, "rb");
#endif
}

static schway_disc_image_status schway__disc_ecm_error(
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

static int schway__disc_ecm_file_size(FILE *file, uint64_t *size)
{
#if defined(_WIN32)
    __int64 end;
    if (_fseeki64(file, 0, SEEK_END) != 0 ||
        (end = _ftelli64(file)) < 0 || _fseeki64(file, 0, SEEK_SET) != 0) return 0;
#else
    off_t end;
    if (fseeko(file, 0, SEEK_END) != 0 ||
        (end = ftello(file)) < (off_t)0 || fseeko(file, 0, SEEK_SET) != 0) return 0;
#endif
    *size = (uint64_t)end;
    return 1;
}

schway_disc_image_status schway__disc_ecm_open(const char *path,
                                                schway__disc_ecm **out,
                                                schway_disc_image_error *error)
{
    FILE *file;
    uint8_t signature[4];
    uint8_t record_count;
    unsigned int count_bytes = 1u;
    uint64_t file_bytes;
    if (out != NULL) *out = NULL;
    if (path == NULL || path[0] == '\0' || out == NULL) {
        return schway__disc_ecm_error(error, SCHWAY_DISC_IMAGE_BAD_ARGUMENT,
                                      "ECM path and output are required");
    }
    if (error != NULL) {
        memset(error, 0, sizeof(*error));
        error->size = (uint32_t)sizeof(*error);
    }
    file = schway__disc_ecm_open_file(path);
    if (file == NULL) {
        return schway__disc_ecm_error(error, SCHWAY_DISC_IMAGE_IO,
                                      "could not open ECM file");
    }
    if (!schway__disc_ecm_file_size(file, &file_bytes)) {
        (void)fclose(file);
        return schway__disc_ecm_error(error, SCHWAY_DISC_IMAGE_IO,
                                      "could not determine ECM file size");
    }
    if (file_bytes > SCHWAY__MEDIA_IMAGE_MAX_BYTES) {
        (void)fclose(file);
        return schway__disc_ecm_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                      "ECM file exceeds the 1 GiB limit");
    }
    if (file_bytes < sizeof(signature) ||
        fread(signature, 1u, sizeof(signature), file) != sizeof(signature)) {
        (void)fclose(file);
        return schway__disc_ecm_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "ECM header is truncated");
    }
    if (memcmp(signature, "ECM\0", sizeof(signature)) != 0) {
        (void)fclose(file);
        return schway__disc_ecm_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "ECM signature is invalid");
    }
    if (file_bytes == sizeof(signature)) {
        (void)fclose(file);
        return schway__disc_ecm_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "ECM record stream is truncated");
    }
    if (fread(&record_count, 1u, 1u, file) != 1u) {
        schway_disc_image_status status = ferror(file) != 0 ?
            SCHWAY_DISC_IMAGE_IO : SCHWAY_DISC_IMAGE_BAD_IMAGE;
        (void)fclose(file);
        return schway__disc_ecm_error(error, status,
                                      "ECM record count is truncated");
    }
    while ((record_count & 0x80u) != 0u) {
        if (count_bytes >= 10u) {
            (void)fclose(file);
            return schway__disc_ecm_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                          "ECM record count is too long");
        }
        if (fread(&record_count, 1u, 1u, file) != 1u) {
            schway_disc_image_status status = ferror(file) != 0 ?
                SCHWAY_DISC_IMAGE_IO : SCHWAY_DISC_IMAGE_BAD_IMAGE;
            (void)fclose(file);
            return schway__disc_ecm_error(error, status,
                                          "ECM record count is truncated");
        }
        count_bytes++;
    }
    (void)fclose(file);
    return schway__disc_ecm_error(error, SCHWAY_DISC_IMAGE_UNSUPPORTED,
                                  "ECM records are not yet validated");
}

void schway__disc_ecm_close(schway__disc_ecm *reader)
{
    if (reader == NULL) return;
    if (reader->file != NULL) (void)fclose(reader->file);
    free(reader);
}

schway_disc_image_status schway__disc_ecm_toc(const schway__disc_ecm *reader,
                                               schway_toc *out)
{
    if (reader == NULL || out == NULL || out->size != sizeof(*out)) {
        return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    }
    return SCHWAY_DISC_IMAGE_UNSUPPORTED;
}

schway_disc_image_status schway__disc_ecm_decoded_size(
    const schway__disc_ecm *reader, uint64_t *out)
{
    if (out != NULL) *out = 0u;
    if (reader == NULL || out == NULL) return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    return SCHWAY_DISC_IMAGE_UNSUPPORTED;
}

schway_disc_image_status schway__disc_ecm_read_bytes(
    schway__disc_ecm *reader, uint64_t offset, uint8_t *bytes, size_t size)
{
    if (reader == NULL || (bytes == NULL && size != 0u) ||
        offset > UINT64_MAX - (uint64_t)size) return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    return SCHWAY_DISC_IMAGE_UNSUPPORTED;
}

uint32_t schway__disc_ecm_read_sector(void *user, uint32_t lba,
                                      uint8_t sector[2352], uint8_t subcode[96])
{
    (void)user;
    (void)lba;
    (void)sector;
    (void)subcode;
    return 0u;
}
