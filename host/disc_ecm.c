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

#define SCHWAY__DISC_ECM_SECTOR_BYTES 2352u
#define SCHWAY__DISC_ECM_RECORD_MAX 1048576u

struct schway__disc_ecm {
    FILE *file;
    uint64_t file_bytes;
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

static int schway__disc_ecm_seek(FILE *file, uint64_t offset)
{
#if defined(_WIN32)
    return offset <= (uint64_t)INT64_MAX && _fseeki64(file, (__int64)offset, SEEK_SET) == 0;
#else
    return offset <= (uint64_t)INT64_MAX && fseeko(file, (off_t)offset, SEEK_SET) == 0;
#endif
}

static int schway__disc_ecm_tell(FILE *file, uint64_t *offset)
{
#if defined(_WIN32)
    __int64 value = _ftelli64(file);
    if (value < 0) return 0;
#else
    off_t value = ftello(file);
    if (value < (off_t)0) return 0;
#endif
    *offset = (uint64_t)value;
    return 1;
}

static schway_disc_image_status schway__disc_ecm_read_count(
    FILE *file, uint8_t *type, uint64_t *encoded)
{
    uint8_t byte;
    uint64_t value;
    unsigned int shift = 5u;
    if (fread(&byte, 1u, 1u, file) != 1u) {
        return ferror(file) != 0 ? SCHWAY_DISC_IMAGE_IO : SCHWAY_DISC_IMAGE_BAD_IMAGE;
    }
    *type = (uint8_t)(byte & 3u);
    value = (uint64_t)((byte >> 2) & 0x1fu);
    for (unsigned int i = 0u; (byte & 0x80u) != 0u; i++) {
        if (i >= 4u || fread(&byte, 1u, 1u, file) != 1u) {
            return ferror(file) != 0 ? SCHWAY_DISC_IMAGE_IO : SCHWAY_DISC_IMAGE_BAD_IMAGE;
        }
        value |= (uint64_t)(byte & 0x7fu) << shift;
        shift += 7u;
    }
    if (value > UINT32_MAX) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    *encoded = value;
    return SCHWAY_DISC_IMAGE_OK;
}

static uint32_t schway__disc_ecm_edc(uint32_t edc, const uint8_t *bytes, size_t size)
{
    for (size_t i = 0u; i < size; i++) {
        edc ^= bytes[i];
        for (uint32_t bit = 0u; bit < 8u; bit++) {
            edc = (edc >> 1) ^ ((edc & 1u) != 0u ? UINT32_C(0xd8018001) : 0u);
        }
    }
    return edc;
}

static schway_disc_image_status schway__disc_ecm_validate(
    FILE *file, uint64_t file_bytes, uint64_t *decoded_bytes)
{
    uint8_t buffer[8192];
    uint32_t edc = 0u;
    uint32_t records = 0u;
    uint64_t decoded = 0u;
    if (!schway__disc_ecm_seek(file, 4u)) return SCHWAY_DISC_IMAGE_IO;
    for (;;) {
        uint8_t type;
        uint64_t encoded;
        uint64_t position = 0u;
        uint64_t count;
        uint64_t next_decoded;
        schway_disc_image_status status;
        if (records >= SCHWAY__DISC_ECM_RECORD_MAX) return SCHWAY_DISC_IMAGE_LIMIT;
        status = schway__disc_ecm_read_count(file, &type, &encoded);
        if (status != SCHWAY_DISC_IMAGE_OK) return status;
        records++;
        if (type == 0u && encoded == UINT32_MAX) {
            uint8_t checksum[4];
            uint32_t expected;
            if (decoded == 0u || decoded % SCHWAY__DISC_ECM_SECTOR_BYTES != 0u ||
                !schway__disc_ecm_tell(file, &position) || position > file_bytes ||
                file_bytes - position != sizeof checksum) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
            if (fread(checksum, 1u, sizeof checksum, file) != sizeof checksum) {
                return ferror(file) != 0 ? SCHWAY_DISC_IMAGE_IO : SCHWAY_DISC_IMAGE_BAD_IMAGE;
            }
            expected = (uint32_t)checksum[0] | ((uint32_t)checksum[1] << 8) |
                       ((uint32_t)checksum[2] << 16) | ((uint32_t)checksum[3] << 24);
            if (expected != edc) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
            if (!schway__disc_ecm_tell(file, &position) || position != file_bytes) {
                return SCHWAY_DISC_IMAGE_BAD_IMAGE;
            }
            *decoded_bytes = decoded;
            return SCHWAY_DISC_IMAGE_OK;
        }
        count = encoded + 1u;
        if (count > UINT32_C(0x7fffffff)) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
        if (type != 0u) return SCHWAY_DISC_IMAGE_UNSUPPORTED;
        if (!schway__media_add_u64(decoded, count, &next_decoded)) {
            return SCHWAY_DISC_IMAGE_LIMIT;
        }
        if (next_decoded > SCHWAY__MEDIA_IMAGE_MAX_BYTES) return SCHWAY_DISC_IMAGE_LIMIT;
        if (!schway__disc_ecm_tell(file, &position) || position > file_bytes ||
            count > file_bytes - position) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
        uint64_t remaining = count;
        while (remaining != 0u) {
            size_t chunk = remaining > sizeof buffer ? sizeof buffer : (size_t)remaining;
            if (fread(buffer, 1u, chunk, file) != chunk) {
                return ferror(file) != 0 ? SCHWAY_DISC_IMAGE_IO : SCHWAY_DISC_IMAGE_BAD_IMAGE;
            }
            edc = schway__disc_ecm_edc(edc, buffer, chunk);
            remaining -= chunk;
        }
        decoded = next_decoded;
    }
}

schway_disc_image_status schway__disc_ecm_open(const char *path,
                                                schway__disc_ecm **out,
                                                schway_disc_image_error *error)
{
    static const char *messages[] = {
        "ok", "invalid ECM path or output", "cannot open ECM file",
        "ECM header is truncated", "ECM stream is malformed",
        "ECM exceeds the 1 GiB limit", "out of memory opening ECM",
        "unsupported ECM record type"
    };
    FILE *file;
    uint8_t signature[4];
    uint64_t file_bytes;
    uint64_t decoded_bytes = 0u;
    schway__disc_ecm *reader;
    schway_disc_image_status status;
    const char *message;
    if (out != NULL) *out = NULL;
    if (path == NULL || path[0] == '\0' || out == NULL) {
        return schway__disc_ecm_error(error, SCHWAY_DISC_IMAGE_BAD_ARGUMENT, messages[1]);
    }
    if (error != NULL) {
        memset(error, 0, sizeof(*error));
        error->size = (uint32_t)sizeof(*error);
    }
    file = schway__disc_ecm_open_file(path);
    if (file == NULL) return schway__disc_ecm_error(error, SCHWAY_DISC_IMAGE_IO, messages[2]);
    if (!schway__disc_ecm_file_size(file, &file_bytes)) {
        (void)fclose(file);
        return schway__disc_ecm_error(error, SCHWAY_DISC_IMAGE_IO,
                                      "could not determine ECM file size");
    }
    if (file_bytes > SCHWAY__MEDIA_IMAGE_MAX_BYTES) {
        (void)fclose(file);
        return schway__disc_ecm_error(error, SCHWAY_DISC_IMAGE_LIMIT, messages[5]);
    }
    if (file_bytes < sizeof signature ||
        fread(signature, 1u, sizeof signature, file) != sizeof signature) {
        status = ferror(file) != 0 ? SCHWAY_DISC_IMAGE_IO : SCHWAY_DISC_IMAGE_BAD_IMAGE;
        (void)fclose(file);
        return schway__disc_ecm_error(error, status,
                                      status == SCHWAY_DISC_IMAGE_IO ? messages[2] : messages[3]);
    }
    if (memcmp(signature, "ECM\0", sizeof signature) != 0) {
        (void)fclose(file);
        return schway__disc_ecm_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "ECM signature is invalid");
    }
    status = schway__disc_ecm_validate(file, file_bytes, &decoded_bytes);
    if (status != SCHWAY_DISC_IMAGE_OK) {
        (void)fclose(file);
        message = status == SCHWAY_DISC_IMAGE_LIMIT ? messages[5] :
                  status == SCHWAY_DISC_IMAGE_UNSUPPORTED ? messages[7] :
                  status == SCHWAY_DISC_IMAGE_IO ? messages[2] : messages[4];
        return schway__disc_ecm_error(error, status, message);
    }
    reader = (schway__disc_ecm *)calloc(1u, sizeof(*reader));
    if (reader == NULL) {
        (void)fclose(file);
        return schway__disc_ecm_error(error, SCHWAY_DISC_IMAGE_OUT_OF_MEMORY, messages[6]);
    }
    reader->file = file;
    reader->file_bytes = file_bytes;
    reader->decoded_bytes = decoded_bytes;
    *out = reader;
    return schway__disc_ecm_error(error, SCHWAY_DISC_IMAGE_OK, messages[0]);
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
    memset(out, 0, sizeof(*out));
    out->size = (uint32_t)sizeof(*out);
    out->track_count = 1u;
    out->tracks[0].size = (uint32_t)sizeof(out->tracks[0]);
    out->tracks[0].type = SCHWAY_DISC_TRACK_MODE2;
    out->tracks[0].leadout = (uint32_t)(reader->decoded_bytes / SCHWAY__DISC_ECM_SECTOR_BYTES);
    return SCHWAY_DISC_IMAGE_OK;
}

schway_disc_image_status schway__disc_ecm_decoded_size(
    const schway__disc_ecm *reader, uint64_t *out)
{
    if (out != NULL) *out = 0u;
    if (reader == NULL || out == NULL) return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    *out = reader->decoded_bytes;
    return SCHWAY_DISC_IMAGE_OK;
}

schway_disc_image_status schway__disc_ecm_read_bytes(
    schway__disc_ecm *reader, uint64_t offset, uint8_t *bytes, size_t size)
{
    uint64_t decoded = 0u;
    size_t copied = 0u;
    if (reader == NULL || (bytes == NULL && size != 0u)) return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    if (offset > reader->decoded_bytes || (uint64_t)size > reader->decoded_bytes - offset) {
        return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    }
    if (size == 0u) return SCHWAY_DISC_IMAGE_OK;
    if (!schway__disc_ecm_seek(reader->file, 4u)) return SCHWAY_DISC_IMAGE_IO;
    while (copied < size) {
        uint8_t type;
        uint64_t encoded;
        uint64_t count;
        uint64_t data_start;
        uint64_t record_end;
        schway_disc_image_status status = schway__disc_ecm_read_count(
            reader->file, &type, &encoded);
        if (status != SCHWAY_DISC_IMAGE_OK) return status;
        if (type != 0u || encoded == UINT32_MAX) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
        count = encoded + 1u;
        if (!schway__disc_ecm_tell(reader->file, &data_start) ||
            !schway__media_add_u64(decoded, count, &record_end)) return SCHWAY_DISC_IMAGE_IO;
        if (offset >= record_end) {
            if (!schway__disc_ecm_seek(reader->file, data_start + count)) {
                return SCHWAY_DISC_IMAGE_IO;
            }
            decoded = record_end;
            continue;
        }
        uint64_t skipped = offset > decoded ? offset - decoded : 0u;
        size_t available = (size_t)(count - skipped);
        size_t amount = size - copied < available ? size - copied : available;
        if (!schway__disc_ecm_seek(reader->file, data_start + skipped) ||
            fread(bytes + copied, 1u, amount, reader->file) != amount) {
            return ferror(reader->file) != 0 ? SCHWAY_DISC_IMAGE_IO : SCHWAY_DISC_IMAGE_BAD_IMAGE;
        }
        copied += amount;
        offset += amount;
        decoded = record_end;
    }
    return SCHWAY_DISC_IMAGE_OK;
}

uint32_t schway__disc_ecm_read_sector(void *user, uint32_t lba,
                                      uint8_t sector[2352], uint8_t subcode[96])
{
    schway__disc_ecm *reader = (schway__disc_ecm *)user;
    uint64_t offset;
    if (reader == NULL || sector == NULL ||
        !schway__media_mul_u64(lba, SCHWAY__DISC_ECM_SECTOR_BYTES, &offset) ||
        schway__disc_ecm_read_bytes(reader, offset, sector,
                                    SCHWAY__DISC_ECM_SECTOR_BYTES) != SCHWAY_DISC_IMAGE_OK) {
        return 0u;
    }
    if (subcode != NULL) memset(subcode, 0, 96u);
    return 1u;
}
