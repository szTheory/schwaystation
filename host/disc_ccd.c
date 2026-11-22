#if !defined(_WIN32) && !defined(_XOPEN_SOURCE)
#define _XOPEN_SOURCE 700
#endif
#include "disc_ccd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32)
#include <sys/types.h>
#endif

#define SCHWAY__DISC_CCD_DESCRIPTOR_MAX_BYTES UINT64_C(65536)

struct schway__disc_ccd {
    FILE *img;
    FILE *sub;
    schway_toc toc;
};

static FILE *schway__disc_ccd_open_file(const char *path)
{
#if defined(_MSC_VER)
    FILE *file = NULL;
    if (fopen_s(&file, path, "rb") != 0) return NULL;
    return file;
#else
    return fopen(path, "rb");
#endif
}

static schway_disc_image_status schway__disc_ccd_error(
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

static int schway__disc_ccd_file_size(FILE *file, uint64_t *size)
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

static schway_disc_image_status schway__disc_ccd_validate_header(
    const uint8_t *bytes, size_t size)
{
    size_t line_end = 0u;
    size_t position;
    uint32_t version = 0u;
    uint32_t digits = 0u;
    while (line_end < size && bytes[line_end] != (uint8_t)'\r' &&
           bytes[line_end] != (uint8_t)'\n') line_end++;
    if (line_end != 9u || memcmp(bytes, "[CloneCD]", 9u) != 0) {
        return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    }
    position = line_end;
    while (position < size) {
        size_t start;
        size_t end;
        while (position < size &&
               (bytes[position] == (uint8_t)'\r' || bytes[position] == (uint8_t)'\n')) position++;
        start = position;
        while (position < size && bytes[position] != (uint8_t)'\r' &&
               bytes[position] != (uint8_t)'\n') position++;
        end = position;
        if (end - start >= 8u && memcmp(bytes + start, "Version=", 8u) == 0) {
            start += 8u;
            if (start == end) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
            for (; start < end; start++) {
                if (bytes[start] < (uint8_t)'0' || bytes[start] > (uint8_t)'9' ||
                    version > (UINT32_MAX - (uint32_t)(bytes[start] - (uint8_t)'0')) / 10u) {
                    return SCHWAY_DISC_IMAGE_BAD_IMAGE;
                }
                version = version * 10u + (uint32_t)(bytes[start] - (uint8_t)'0');
                digits++;
            }
            if (digits == 0u) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
            return version == 3u ? SCHWAY_DISC_IMAGE_OK :
                                   SCHWAY_DISC_IMAGE_UNSUPPORTED;
        }
    }
    return SCHWAY_DISC_IMAGE_BAD_IMAGE;
}

schway_disc_image_status schway__disc_ccd_open(const char *path,
                                                schway__disc_ccd **out,
                                                schway_disc_image_error *error)
{
    FILE *file;
    uint8_t *bytes;
    uint64_t file_bytes;
    size_t read_bytes;
    schway_disc_image_status status;
    if (out != NULL) *out = NULL;
    if (path == NULL || path[0] == '\0' || out == NULL) {
        return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_BAD_ARGUMENT,
                                      "CCD path and output are required");
    }
    if (error != NULL) {
        memset(error, 0, sizeof(*error));
        error->size = (uint32_t)sizeof(*error);
    }
    file = schway__disc_ccd_open_file(path);
    if (file == NULL) {
        return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_IO,
                                      "could not open CCD file");
    }
    if (!schway__disc_ccd_file_size(file, &file_bytes)) {
        (void)fclose(file);
        return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_IO,
                                      "could not determine CCD file size");
    }
    if (file_bytes > SCHWAY__DISC_CCD_DESCRIPTOR_MAX_BYTES) {
        (void)fclose(file);
        return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                      "CCD descriptor exceeds the 65536-byte limit");
    }
    if (file_bytes == 0u) {
        (void)fclose(file);
        return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "CCD descriptor is empty");
    }
    bytes = (uint8_t *)malloc((size_t)file_bytes);
    if (bytes == NULL) {
        (void)fclose(file);
        return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_OUT_OF_MEMORY,
                                      "could not allocate CCD descriptor buffer");
    }
    read_bytes = fread(bytes, 1u, (size_t)file_bytes, file);
    (void)fclose(file);
    if (read_bytes != (size_t)file_bytes) {
        free(bytes);
        return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_IO,
                                      "could not read complete CCD descriptor");
    }
    for (size_t i = 0u; i < (size_t)file_bytes; i++) {
        if (bytes[i] == 0u || (bytes[i] < 0x20u && bytes[i] != (uint8_t)'\r' &&
            bytes[i] != (uint8_t)'\n' && bytes[i] != (uint8_t)'\t') || bytes[i] > 0x7eu) {
            free(bytes);
            return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                          "CCD descriptor contains invalid text bytes");
        }
    }
    status = schway__disc_ccd_validate_header(bytes, (size_t)file_bytes);
    free(bytes);
    if (status == SCHWAY_DISC_IMAGE_BAD_IMAGE) {
        return schway__disc_ccd_error(error, status,
                                      "CCD header is malformed or truncated");
    }
    if (status == SCHWAY_DISC_IMAGE_UNSUPPORTED) {
        return schway__disc_ccd_error(error, status, "CCD version is unsupported");
    }
    return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_UNSUPPORTED,
                                  "CCD track metadata is not yet validated");
}

void schway__disc_ccd_close(schway__disc_ccd *reader)
{
    if (reader == NULL) return;
    if (reader->img != NULL) (void)fclose(reader->img);
    if (reader->sub != NULL) (void)fclose(reader->sub);
    free(reader);
}

schway_disc_image_status schway__disc_ccd_toc(const schway__disc_ccd *reader,
                                               schway_toc *out)
{
    if (reader == NULL || out == NULL || out->size != sizeof(*out)) {
        return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    }
    return SCHWAY_DISC_IMAGE_UNSUPPORTED;
}

uint32_t schway__disc_ccd_read_sector(void *user, uint32_t lba,
                                      uint8_t sector[2352], uint8_t subcode[96])
{
    (void)user;
    (void)lba;
    (void)sector;
    (void)subcode;
    return 0u;
}
