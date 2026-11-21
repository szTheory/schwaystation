#if !defined(_WIN32) && !defined(_XOPEN_SOURCE)
#define _XOPEN_SOURCE 700
#endif
#include "disc_pbp.h"
#include "disc_pbp_metadata.h"
#include "media_limits.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32)
#include <sys/types.h>
#endif

struct schway__disc_pbp {
    uint8_t reserved;
};

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

static schway_disc_image_status schway__disc_pbp_read_at(
    void *context, uint64_t offset, uint8_t *bytes, size_t size)
{
    FILE *file = (FILE *)context;
    if (file == NULL || bytes == NULL || offset > (uint64_t)INT64_MAX) {
        return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    }
#if defined(_WIN32)
    if (_fseeki64(file, (__int64)offset, SEEK_SET) != 0) return SCHWAY_DISC_IMAGE_IO;
#else
    if (fseeko(file, (off_t)offset, SEEK_SET) != 0) return SCHWAY_DISC_IMAGE_IO;
#endif
    if (fread(bytes, 1u, size, file) == size) return SCHWAY_DISC_IMAGE_OK;
    return ferror(file) != 0 ? SCHWAY_DISC_IMAGE_IO : SCHWAY_DISC_IMAGE_BAD_IMAGE;
}

schway_disc_image_status schway__disc_pbp_open(const char *path,
                                                schway__disc_pbp **out,
                                                schway_disc_image_error *error)
{
    schway__disc_pbp_metadata_map map;
    schway_disc_image_status status;
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
    memset(&map, 0, sizeof(map));
    status = schway__disc_pbp_metadata_parse(schway__disc_pbp_read_at, file,
                                              file_bytes, &map);
    schway__disc_pbp_metadata_clear(&map);
    if (fclose(file) != 0 && status == SCHWAY_DISC_IMAGE_OK) {
        status = SCHWAY_DISC_IMAGE_IO;
    }
    if (status != SCHWAY_DISC_IMAGE_OK) {
        return schway__disc_pbp_error(error, status,
            status == SCHWAY_DISC_IMAGE_UNSUPPORTED ?
            "PBP layout is unsupported" : "PBP metadata is invalid");
    }
    return schway__disc_pbp_error(error, SCHWAY_DISC_IMAGE_UNSUPPORTED,
                                  "PBP metadata is valid; sector reading is unavailable");
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
