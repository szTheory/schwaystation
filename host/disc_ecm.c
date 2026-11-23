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
#define SCHWAY__DISC_ECM_TYPE1_INPUT 2051u
#define SCHWAY__DISC_ECM_TYPE2_INPUT 2052u
#define SCHWAY__DISC_ECM_TYPE3_INPUT 2328u
#define SCHWAY__DISC_ECM_MODE2_OUTPUT 2336u

typedef struct schway__disc_ecm_checkpoint {
    uint64_t source_offset;
    uint64_t decoded_offset;
    uint32_t remaining;
    uint8_t type;
    uint16_t output_offset;
    uint8_t reserved;
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
                                            uint64_t remaining, uint8_t type,
                                            uint16_t output_offset)
{
    schway__disc_ecm_checkpoint *checkpoint;
    if (remaining > UINT32_MAX || reader->checkpoint_count >= SCHWAY__DISC_ECM_CHECKPOINT_MAX) {
        return 0;
    }
    checkpoint = &reader->checkpoints[reader->checkpoint_count++];
    checkpoint->source_offset = source_offset;
    checkpoint->decoded_offset = decoded_offset;
    checkpoint->remaining = (uint32_t)remaining;
    checkpoint->type = type;
    checkpoint->output_offset = output_offset;
    checkpoint->reserved = 0u;
    return 1;
}

static uint32_t schway__disc_ecm_input_unit(uint8_t type)
{
    return type == 1u ? SCHWAY__DISC_ECM_TYPE1_INPUT :
           type == 2u ? SCHWAY__DISC_ECM_TYPE2_INPUT :
           type == 3u ? SCHWAY__DISC_ECM_TYPE3_INPUT : 1u;
}

static uint32_t schway__disc_ecm_output_unit(uint8_t type)
{
    return type == 1u ? SCHWAY__DISC_ECM_SECTOR_BYTES :
           type == 2u || type == 3u ? SCHWAY__DISC_ECM_MODE2_OUTPUT : 1u;
}

static uint8_t schway__disc_ecm_ecc_forward(uint8_t value)
{
    uint16_t doubled = (uint16_t)((uint16_t)value << 1);
    if ((value & 0x80u) != 0u) doubled = (uint16_t)(doubled ^ 0x11du);
    return (uint8_t)doubled;
}

static void schway__disc_ecm_ecc(const uint8_t *source, uint32_t major_count,
                                 uint32_t minor_count, uint32_t major_mult,
                                 uint32_t minor_inc, uint8_t *destination)
{
    uint32_t size = major_count * minor_count;
    uint8_t backward[256];
    for (uint32_t i = 0u; i < 256u; i++) {
        uint8_t value = (uint8_t)i;
        backward[value ^ schway__disc_ecm_ecc_forward(value)] = value;
    }
    for (uint32_t major = 0u; major < major_count; major++) {
        uint32_t index = (major >> 1) * major_mult + (major & 1u);
        uint8_t a = 0u;
        uint8_t b = 0u;
        for (uint32_t minor = 0u; minor < minor_count; minor++) {
            uint8_t value = source[index];
            index += minor_inc;
            if (index >= size) index -= size;
            a ^= value;
            b ^= value;
            a = schway__disc_ecm_ecc_forward(a);
        }
        a = backward[schway__disc_ecm_ecc_forward(a) ^ b];
        destination[major] = a;
        destination[major + major_count] = (uint8_t)(a ^ b);
    }
}

static void schway__disc_ecm_mode1_trailer(uint8_t sector[SCHWAY__DISC_ECM_SECTOR_BYTES])
{
    uint32_t edc = schway__disc_ecm_edc(0u, sector, 2064u);
    sector[2064] = (uint8_t)edc;
    sector[2065] = (uint8_t)(edc >> 8);
    sector[2066] = (uint8_t)(edc >> 16);
    sector[2067] = (uint8_t)(edc >> 24);
    memset(sector + 2068u, 0, 8u);
    schway__disc_ecm_ecc(sector + 12u, 86u, 24u, 2u, 86u, sector + 2076u);
    schway__disc_ecm_ecc(sector + 12u, 52u, 43u, 86u, 88u, sector + 2248u);
}

static void schway__disc_ecm_expand(uint8_t type, const uint8_t *stored,
                                    uint8_t sector[SCHWAY__DISC_ECM_SECTOR_BYTES],
                                    uint32_t *output_size)
{
    if (type == 1u) {
        memset(sector, 0, SCHWAY__DISC_ECM_SECTOR_BYTES);
        memset(sector + 1u, 0xff, 10u);
        memcpy(sector + 12u, stored, 3u);
        sector[15] = 1u;
        memcpy(sector + 16u, stored + 3u, 2048u);
        schway__disc_ecm_mode1_trailer(sector);
        *output_size = SCHWAY__DISC_ECM_SECTOR_BYTES;
        return;
    }
    if (type == 2u) {
        uint32_t edc;
        memset(sector, 0, SCHWAY__DISC_ECM_SECTOR_BYTES);
        memcpy(sector + 16u, stored, 4u);
        memcpy(sector + 20u, stored, 4u);
        memcpy(sector + 24u, stored + 4u, 2048u);
        edc = schway__disc_ecm_edc(0u, sector + 16u, 2056u);
        sector[2072] = (uint8_t)edc;
        sector[2073] = (uint8_t)(edc >> 8);
        sector[2074] = (uint8_t)(edc >> 16);
        sector[2075] = (uint8_t)(edc >> 24);
        schway__disc_ecm_ecc(sector + 12u, 86u, 24u, 2u, 86u, sector + 2076u);
        schway__disc_ecm_ecc(sector + 12u, 52u, 43u, 86u, 88u, sector + 2248u);
        *output_size = SCHWAY__DISC_ECM_MODE2_OUTPUT;
        memmove(sector, sector + 16u, *output_size);
        return;
    }
    if (type == 3u) {
        uint32_t edc;
        memcpy(sector, stored, 4u);
        memcpy(sector + 4u, stored, 4u);
        memcpy(sector + 8u, stored + 4u, 2324u);
        edc = schway__disc_ecm_edc(0u, sector, 2332u);
        sector[2332] = (uint8_t)edc;
        sector[2333] = (uint8_t)(edc >> 8);
        sector[2334] = (uint8_t)(edc >> 16);
        sector[2335] = (uint8_t)(edc >> 24);
        *output_size = SCHWAY__DISC_ECM_MODE2_OUTPUT;
    }
}

static int schway__disc_ecm_add_record_checkpoints(
    schway__disc_ecm *reader, uint64_t source_offset, uint64_t decoded_offset,
    uint64_t count, uint8_t type, uint32_t input_unit, uint32_t output_unit,
    uint64_t decoded_end, uint64_t *next_checkpoint)
{
    while (*next_checkpoint < decoded_end) {
        uint64_t within = *next_checkpoint - decoded_offset;
        uint64_t source_delta = within;
        uint64_t remaining = count - within;
        uint16_t output_offset = 0u;
        uint64_t indexed_source;
        if (type != 0u) {
            uint64_t unit = within / output_unit;
            if (!schway__media_mul_u64(unit, input_unit, &source_delta)) return 0;
            remaining = count - unit;
            output_offset = (uint16_t)(within % output_unit);
        }
        if (!schway__media_add_u64(source_offset, source_delta, &indexed_source) ||
            !schway__disc_ecm_add_checkpoint(reader, indexed_source, *next_checkpoint,
                                             remaining, type, output_offset)) return 0;
        *next_checkpoint += SCHWAY__DISC_ECM_CHECKPOINT_STEP;
    }
    return 1;
}

static schway_disc_image_status schway__disc_ecm_validate(
    schway__disc_ecm *reader)
{
    uint8_t buffer[8192];
    uint8_t stored[SCHWAY__DISC_ECM_TYPE3_INPUT];
    uint8_t sector[SCHWAY__DISC_ECM_SECTOR_BYTES];
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
        uint64_t input_bytes;
        uint64_t output_bytes;
        uint64_t next_decoded;
        uint32_t input_unit;
        uint32_t output_unit;
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
        input_unit = schway__disc_ecm_input_unit(type);
        output_unit = schway__disc_ecm_output_unit(type);
        if (type > 3u || !schway__media_mul_u64(count, input_unit, &input_bytes) ||
            !schway__media_mul_u64(count, output_unit, &output_bytes) ||
            !schway__media_add_u64(decoded, output_bytes, &next_decoded)) {
            return SCHWAY_DISC_IMAGE_LIMIT;
        }
        if (next_decoded > SCHWAY__MEDIA_IMAGE_MAX_BYTES) return SCHWAY_DISC_IMAGE_LIMIT;
        if (!schway__disc_ecm_tell(reader->file, &position) || position > reader->file_bytes ||
            input_bytes > reader->file_bytes - position) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
        if (!schway__disc_ecm_add_record_checkpoints(reader, position, decoded, count,
                                                     type, input_unit, output_unit,
                                                     next_decoded, &next_checkpoint)) {
            return SCHWAY_DISC_IMAGE_LIMIT;
        }
        if (type == 0u) {
            uint64_t remaining = input_bytes;
            while (remaining != 0u) {
                size_t chunk = remaining > sizeof buffer ? sizeof buffer : (size_t)remaining;
                if (fread(buffer, 1u, chunk, reader->file) != chunk) {
                    return ferror(reader->file) != 0 ? SCHWAY_DISC_IMAGE_IO : SCHWAY_DISC_IMAGE_BAD_IMAGE;
                }
                edc = schway__disc_ecm_edc(edc, buffer, chunk);
                remaining -= chunk;
            }
        } else {
            for (uint64_t unit = 0u; unit < count; unit++) {
                uint32_t expanded = 0u;
                if (fread(stored, 1u, input_unit, reader->file) != input_unit) {
                    return ferror(reader->file) != 0 ? SCHWAY_DISC_IMAGE_IO : SCHWAY_DISC_IMAGE_BAD_IMAGE;
                }
                schway__disc_ecm_expand(type, stored, sector, &expanded);
                if (expanded != output_unit) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
                edc = schway__disc_ecm_edc(edc, sector, expanded);
            }
        }
        decoded = next_decoded;
    }
}

static schway_disc_image_status schway__disc_ecm_next_record(
    schway__disc_ecm *reader, uint64_t *source_offset, uint32_t *remaining,
    uint8_t *record_type)
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
    if (encoded == UINT32_MAX) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    count = encoded + 1u;
    if (count > UINT32_C(0x7fffffff)) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    if (!schway__disc_ecm_tell(reader->file, source_offset)) return SCHWAY_DISC_IMAGE_IO;
    *remaining = (uint32_t)count;
    *record_type = type;
    return SCHWAY_DISC_IMAGE_OK;
}

static schway_disc_image_status schway__disc_ecm_decode_at(
    schway__disc_ecm *reader, uint8_t type, uint64_t source_offset,
    uint8_t sector[SCHWAY__DISC_ECM_SECTOR_BYTES], uint32_t *output_size)
{
    uint8_t stored[SCHWAY__DISC_ECM_TYPE3_INPUT];
    uint32_t input_size = schway__disc_ecm_input_unit(type);
    if (type == 0u || type > 3u || !schway__disc_ecm_seek(reader->file, source_offset)) {
        return SCHWAY_DISC_IMAGE_IO;
    }
    if (fread(stored, 1u, input_size, reader->file) != input_size) {
        return ferror(reader->file) != 0 ? SCHWAY_DISC_IMAGE_IO : SCHWAY_DISC_IMAGE_BAD_IMAGE;
    }
    schway__disc_ecm_expand(type, stored, sector, output_size);
    return *output_size == schway__disc_ecm_output_unit(type) ?
           SCHWAY_DISC_IMAGE_OK : SCHWAY_DISC_IMAGE_BAD_IMAGE;
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
    uint8_t type;
    uint16_t output_offset;
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
    type = checkpoint->type;
    output_offset = checkpoint->output_offset;
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
                reader, &source_offset, &remaining, &type);
            if (status != SCHWAY_DISC_IMAGE_OK) return status;
            output_offset = 0u;
        }
        if (type == 0u) {
            uint32_t amount = remaining < skipped ? remaining : (uint32_t)skipped;
            source_offset += amount;
            remaining -= amount;
            skipped -= amount;
            if (remaining == 0u && !schway__disc_ecm_seek(reader->file, source_offset)) {
                return SCHWAY_DISC_IMAGE_IO;
            }
        } else {
            uint8_t sector[SCHWAY__DISC_ECM_SECTOR_BYTES];
            uint32_t output_size;
            uint32_t available;
            uint32_t amount;
            schway_disc_image_status status = schway__disc_ecm_decode_at(
                reader, type, source_offset, sector, &output_size);
            if (status != SCHWAY_DISC_IMAGE_OK) return status;
            available = output_size - output_offset;
            amount = available < skipped ? available : (uint32_t)skipped;
            output_offset = (uint16_t)(output_offset + amount);
            skipped -= amount;
            if (output_offset == output_size) {
                source_offset += schway__disc_ecm_input_unit(type);
                output_offset = 0u;
                remaining--;
            }
        }
    }
    while (copied < size) {
        if (remaining == 0u) {
            schway_disc_image_status status = schway__disc_ecm_next_record(
                reader, &source_offset, &remaining, &type);
            if (status != SCHWAY_DISC_IMAGE_OK) return status;
            output_offset = 0u;
        }
        if (type == 0u) {
            size_t amount = size - copied < remaining ? size - copied : remaining;
            if (!schway__disc_ecm_seek(reader->file, source_offset) ||
                fread(bytes + copied, 1u, amount, reader->file) != amount) {
                return ferror(reader->file) != 0 ? SCHWAY_DISC_IMAGE_IO : SCHWAY_DISC_IMAGE_BAD_IMAGE;
            }
            copied += amount;
            source_offset += amount;
            remaining -= (uint32_t)amount;
        } else {
            uint8_t sector[SCHWAY__DISC_ECM_SECTOR_BYTES];
            uint32_t output_size;
            size_t available;
            size_t amount;
            schway_disc_image_status status = schway__disc_ecm_decode_at(
                reader, type, source_offset, sector, &output_size);
            if (status != SCHWAY_DISC_IMAGE_OK) return status;
            available = output_size - output_offset;
            amount = size - copied < available ? size - copied : available;
            memcpy(bytes + copied, sector + output_offset, amount);
            copied += amount;
            output_offset = (uint16_t)(output_offset + amount);
            if (output_offset == output_size) {
                source_offset += schway__disc_ecm_input_unit(type);
                output_offset = 0u;
                remaining--;
            }
        }
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
