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
#define SCHWAY__DISC_ECM_CHECKPOINT_STEP UINT64_C(65536)
#define SCHWAY__DISC_ECM_CHECKPOINT_MAX 16386u
#define SCHWAY__DISC_ECM_READ_MAX 70240u
#define SCHWAY__DISC_ECM_INDEX_BYTES UINT64_C(1048576)

typedef struct schway__disc_ecm_checkpoint {
    uint64_t source_offset;
    uint64_t decoded_offset;
    uint32_t remaining;
    uint8_t type;
    uint8_t reserved[3];
} schway__disc_ecm_checkpoint;

_Static_assert(sizeof(schway__disc_ecm_checkpoint) <= 48u,
               "ECM checkpoint entries must stay within the index budget");
_Static_assert(sizeof(schway__disc_ecm_checkpoint) * SCHWAY__DISC_ECM_CHECKPOINT_MAX <=
                   SCHWAY__DISC_ECM_INDEX_BYTES,
               "ECM checkpoint storage must stay below one MiB");

struct schway__disc_ecm {
    FILE *file;
    uint64_t file_bytes;
    uint64_t decoded_bytes;
    schway__disc_ecm_checkpoint *checkpoints;
    uint32_t checkpoint_count;
    uint32_t last_forward_bytes;
    uint32_t last_record_headers;
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

static int schway__disc_ecm_limits_ok(uint64_t file_bytes, uint64_t decoded_bytes,
                                      uint32_t records, uint32_t checkpoints)
{
    uint64_t index_bytes;
    return file_bytes <= SCHWAY__MEDIA_IMAGE_MAX_BYTES &&
           decoded_bytes <= SCHWAY__MEDIA_IMAGE_MAX_BYTES &&
           records <= SCHWAY__DISC_ECM_RECORD_MAX &&
           checkpoints <= SCHWAY__DISC_ECM_CHECKPOINT_MAX &&
           schway__media_mul_u64(checkpoints, sizeof(schway__disc_ecm_checkpoint),
                                 &index_bytes) && index_bytes <= SCHWAY__DISC_ECM_INDEX_BYTES;
}

int schway__disc_ecm_test_limits(uint64_t file_bytes, uint64_t decoded_bytes,
                                 uint32_t records, uint32_t checkpoints)
{
    return schway__disc_ecm_limits_ok(file_bytes, decoded_bytes, records, checkpoints);
}

static int schway__disc_ecm_add_checkpoint(schway__disc_ecm *reader,
                                            uint64_t source_offset,
                                            uint64_t decoded_offset,
                                            uint64_t remaining)
{
    schway__disc_ecm_checkpoint *checkpoint;
    if (remaining > UINT32_MAX || reader->checkpoint_count >= SCHWAY__DISC_ECM_CHECKPOINT_MAX) {
        return 0;
    }
    checkpoint = &reader->checkpoints[reader->checkpoint_count++];
    checkpoint->source_offset = source_offset;
    checkpoint->decoded_offset = decoded_offset;
    checkpoint->remaining = (uint32_t)remaining;
    checkpoint->type = 0u;
    memset(checkpoint->reserved, 0, sizeof checkpoint->reserved);
    return 1;
}

static schway_disc_image_status schway__disc_ecm_validate(
    schway__disc_ecm *reader)
{
    uint8_t buffer[8192];
    uint32_t edc = 0u;
    uint32_t records = 0u;
    uint64_t decoded = 0u;
    uint64_t next_checkpoint = 0u;
    if (!schway__disc_ecm_seek(reader->file, 4u)) return SCHWAY_DISC_IMAGE_IO;
    for (;;) {
        uint8_t type;
        uint64_t encoded;
        uint64_t position = 0u;
        uint64_t count;
        uint64_t next_decoded;
        schway_disc_image_status status;
        if (records >= SCHWAY__DISC_ECM_RECORD_MAX) return SCHWAY_DISC_IMAGE_LIMIT;
        status = schway__disc_ecm_read_count(reader->file, &type, &encoded);
        if (status != SCHWAY_DISC_IMAGE_OK) return status;
        records++;
        if (type == 0u && encoded == UINT32_MAX) {
            uint8_t checksum[4];
            uint32_t expected;
            if (decoded == 0u || decoded % SCHWAY__DISC_ECM_SECTOR_BYTES != 0u ||
                !schway__disc_ecm_tell(reader->file, &position) || position > reader->file_bytes ||
                reader->file_bytes - position != sizeof checksum) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
            if (fread(checksum, 1u, sizeof checksum, reader->file) != sizeof checksum) {
                return ferror(reader->file) != 0 ? SCHWAY_DISC_IMAGE_IO : SCHWAY_DISC_IMAGE_BAD_IMAGE;
            }
            expected = (uint32_t)checksum[0] | ((uint32_t)checksum[1] << 8) |
                       ((uint32_t)checksum[2] << 16) | ((uint32_t)checksum[3] << 24);
            if (expected != edc) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
            if (!schway__disc_ecm_tell(reader->file, &position) || position != reader->file_bytes) {
                return SCHWAY_DISC_IMAGE_BAD_IMAGE;
            }
            if (!schway__disc_ecm_limits_ok(reader->file_bytes, decoded, records,
                                            reader->checkpoint_count)) {
                return SCHWAY_DISC_IMAGE_LIMIT;
            }
            reader->decoded_bytes = decoded;
            return SCHWAY_DISC_IMAGE_OK;
        }
        count = encoded + 1u;
        if (count > UINT32_C(0x7fffffff)) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
        if (type != 0u) return SCHWAY_DISC_IMAGE_UNSUPPORTED;
        if (!schway__media_add_u64(decoded, count, &next_decoded)) {
            return SCHWAY_DISC_IMAGE_LIMIT;
        }
        if (next_decoded > SCHWAY__MEDIA_IMAGE_MAX_BYTES) return SCHWAY_DISC_IMAGE_LIMIT;
        if (!schway__disc_ecm_tell(reader->file, &position) || position > reader->file_bytes ||
            count > reader->file_bytes - position) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
        if (decoded == next_checkpoint) {
            if (!schway__disc_ecm_add_checkpoint(reader, position, decoded, count)) {
                return SCHWAY_DISC_IMAGE_LIMIT;
            }
            next_checkpoint += SCHWAY__DISC_ECM_CHECKPOINT_STEP;
        }
        while (next_checkpoint < next_decoded) {
            uint64_t offset = position + (next_checkpoint - decoded);
            if (!schway__disc_ecm_add_checkpoint(reader, offset, next_checkpoint,
                                                 next_decoded - next_checkpoint)) {
                return SCHWAY_DISC_IMAGE_LIMIT;
            }
            next_checkpoint += SCHWAY__DISC_ECM_CHECKPOINT_STEP;
        }
        uint64_t remaining = count;
        while (remaining != 0u) {
            size_t chunk = remaining > sizeof buffer ? sizeof buffer : (size_t)remaining;
            if (fread(buffer, 1u, chunk, reader->file) != chunk) {
                return ferror(reader->file) != 0 ? SCHWAY_DISC_IMAGE_IO : SCHWAY_DISC_IMAGE_BAD_IMAGE;
            }
            edc = schway__disc_ecm_edc(edc, buffer, chunk);
            remaining -= chunk;
        }
        decoded = next_decoded;
    }
}

static schway_disc_image_status schway__disc_ecm_next_record(
    schway__disc_ecm *reader, uint64_t *source_offset, uint32_t *remaining)
{
    uint8_t type;
    uint64_t encoded;
    uint64_t count;
    schway_disc_image_status status;
    if (reader->last_record_headers >= SCHWAY__DISC_ECM_READ_MAX) {
        return SCHWAY_DISC_IMAGE_LIMIT;
    }
    status = schway__disc_ecm_read_count(reader->file, &type, &encoded);
    if (status != SCHWAY_DISC_IMAGE_OK) return status;
    reader->last_record_headers++;
    if (type != 0u || encoded == UINT32_MAX) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    count = encoded + 1u;
    if (count > UINT32_C(0x7fffffff)) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    if (!schway__disc_ecm_tell(reader->file, source_offset)) return SCHWAY_DISC_IMAGE_IO;
    *remaining = (uint32_t)count;
    return SCHWAY_DISC_IMAGE_OK;
}

static schway_disc_image_status schway__disc_ecm_open_impl(
    const char *path, schway__disc_ecm **out, schway_disc_image_error *error,
    int fail_index_allocation)
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
    reader = (schway__disc_ecm *)calloc(1u, sizeof(*reader));
    if (reader == NULL) {
        (void)fclose(file);
        return schway__disc_ecm_error(error, SCHWAY_DISC_IMAGE_OUT_OF_MEMORY, messages[6]);
    }
    reader->file = file;
    reader->file_bytes = file_bytes;
    if (!fail_index_allocation) {
        reader->checkpoints = (schway__disc_ecm_checkpoint *)calloc(
            SCHWAY__DISC_ECM_CHECKPOINT_MAX, sizeof(*reader->checkpoints));
    }
    if (reader->checkpoints == NULL) {
        schway__disc_ecm_close(reader);
        return schway__disc_ecm_error(error, SCHWAY_DISC_IMAGE_OUT_OF_MEMORY, messages[6]);
    }
    status = schway__disc_ecm_validate(reader);
    if (status != SCHWAY_DISC_IMAGE_OK) {
        schway__disc_ecm_close(reader);
        message = status == SCHWAY_DISC_IMAGE_LIMIT ? messages[5] :
                  status == SCHWAY_DISC_IMAGE_UNSUPPORTED ? messages[7] :
                  status == SCHWAY_DISC_IMAGE_IO ? messages[2] : messages[4];
        return schway__disc_ecm_error(error, status, message);
    }
    *out = reader;
    return schway__disc_ecm_error(error, SCHWAY_DISC_IMAGE_OK, messages[0]);
}

schway_disc_image_status schway__disc_ecm_open(const char *path,
                                                schway__disc_ecm **out,
                                                schway_disc_image_error *error)
{
    return schway__disc_ecm_open_impl(path, out, error, 0);
}

schway_disc_image_status schway__disc_ecm_test_open_fail_index_alloc(
    const char *path, schway__disc_ecm **out, schway_disc_image_error *error)
{
    return schway__disc_ecm_open_impl(path, out, error, 1);
}

void schway__disc_ecm_close(schway__disc_ecm *reader)
{
    if (reader == NULL) return;
    if (reader->file != NULL) (void)fclose(reader->file);
    free(reader->checkpoints);
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
    const schway__disc_ecm_checkpoint *checkpoint;
    uint32_t checkpoint_index;
    uint64_t forward;
    uint64_t source_offset;
    uint64_t skipped;
    uint32_t remaining;
    size_t copied = 0u;
    if (reader == NULL || (bytes == NULL && size != 0u)) return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    reader->last_forward_bytes = 0u;
    reader->last_record_headers = 0u;
    if (offset > reader->decoded_bytes || (uint64_t)size > reader->decoded_bytes - offset) {
        return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    }
    if (size == 0u) return SCHWAY_DISC_IMAGE_OK;
    checkpoint_index = (uint32_t)(offset / SCHWAY__DISC_ECM_CHECKPOINT_STEP);
    if (checkpoint_index >= reader->checkpoint_count) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    checkpoint = &reader->checkpoints[checkpoint_index];
    forward = offset - checkpoint->decoded_offset;
    if (forward > SCHWAY__DISC_ECM_READ_MAX ||
        (uint64_t)size > SCHWAY__DISC_ECM_READ_MAX - forward) {
        return SCHWAY_DISC_IMAGE_LIMIT;
    }
    reader->last_forward_bytes = (uint32_t)(forward + (uint64_t)size);
    source_offset = checkpoint->source_offset;
    remaining = checkpoint->remaining;
    skipped = forward;
    while (skipped != 0u) {
        if (remaining == 0u) {
            schway_disc_image_status status = schway__disc_ecm_next_record(
                reader, &source_offset, &remaining);
            if (status != SCHWAY_DISC_IMAGE_OK) return status;
        }
        uint32_t amount = remaining < skipped ? remaining : (uint32_t)skipped;
        source_offset += amount;
        remaining -= amount;
        skipped -= amount;
        if (remaining == 0u && !schway__disc_ecm_seek(reader->file, source_offset)) {
            return SCHWAY_DISC_IMAGE_IO;
        }
    }
    while (copied < size) {
        if (remaining == 0u) {
            schway_disc_image_status status = schway__disc_ecm_next_record(
                reader, &source_offset, &remaining);
            if (status != SCHWAY_DISC_IMAGE_OK) return status;
        }
        size_t amount = size - copied < remaining ? size - copied : remaining;
        if (!schway__disc_ecm_seek(reader->file, source_offset) ||
            fread(bytes + copied, 1u, amount, reader->file) != amount) {
            return ferror(reader->file) != 0 ? SCHWAY_DISC_IMAGE_IO : SCHWAY_DISC_IMAGE_BAD_IMAGE;
        }
        copied += amount;
        source_offset += amount;
        remaining -= (uint32_t)amount;
    }
    return SCHWAY_DISC_IMAGE_OK;
}

schway_disc_image_status schway__disc_ecm_read_metrics_get(
    const schway__disc_ecm *reader, schway__disc_ecm_read_metrics *out)
{
    if (reader == NULL || out == NULL || out->size != sizeof(*out)) {
        return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    }
    out->checkpoint_count = reader->checkpoint_count;
    out->decoded_forward_bytes = reader->last_forward_bytes;
    out->record_headers = reader->last_record_headers;
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
