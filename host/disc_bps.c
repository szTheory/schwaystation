/* Host-only BPS1 transformation into an unpublished, validated target. */
#include "disc_patch.h"

#include "crc32.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCHWAY__DISC_BPS_MAX_IMAGE_BYTES (UINT64_C(1) << 30)
#define SCHWAY__DISC_BPS_MAX_PATCH_BYTES (64u * 1024u * 1024u)
#define SCHWAY__DISC_BPS_MAX_METADATA_BYTES (1024u * 1024u)
#define SCHWAY__DISC_BPS_MEMORY_TARGET_BYTES (1024u * 1024u)
#define SCHWAY__DISC_BPS_IO_CHUNK_BYTES 4096u
#define SCHWAY__DISC_BPS_FOOTER_BYTES 12u

struct schway__disc_bps_target {
    uint64_t size;
    uint8_t *memory;
    FILE *file;
};

schway_disc_patch_status schway__disc_bps_transform(
    const schway_disc_patch_source *source, const uint8_t *patch,
    size_t patch_size, struct schway__disc_bps_target **out,
    uint32_t *action_count, uint32_t *checks, schway_disc_patch_error *error);
void schway__disc_bps_target_destroy(struct schway__disc_bps_target *target);
uint32_t schway__disc_bps_target_read_bytes(
    struct schway__disc_bps_target *target, uint64_t offset,
    uint8_t *bytes, size_t count);

static schway_disc_patch_status schway__disc_bps_error(
    schway_disc_patch_error *error, schway_disc_patch_status status,
    uint64_t offset, const char *message)
{
    if (error != NULL) {
        memset(error, 0, sizeof *error);
        error->size = (uint32_t)sizeof *error;
        error->status = (uint32_t)status;
        error->offset = offset;
        (void)snprintf(error->message, sizeof error->message, "%s", message);
    }
    return status;
}

static uint32_t schway__disc_bps_u32_le(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static int schway__disc_bps_number(const uint8_t *patch, size_t end,
                                   size_t *position, uint64_t *value)
{
    uint64_t result = 0u;
    uint64_t shift = 1u;
    for (;;) {
        uint8_t byte;
        uint64_t part;
        if (*position >= end) return 0;
        byte = patch[(*position)++];
        part = (uint64_t)(byte & 0x7fu);
        if (part > (UINT64_MAX - result) / shift) return 0;
        result += part * shift;
        if ((byte & 0x80u) != 0u) break;
        if (shift > (UINT64_MAX >> 7)) return 0;
        shift <<= 7;
        if (result > UINT64_MAX - shift) return 0;
        result += shift;
    }
    *value = result;
    return 1;
}

static schway_disc_patch_status schway__disc_bps_source_crc(
    const schway_disc_patch_source *source, uint32_t *crc,
    schway_disc_patch_error *error)
{
    uint8_t buffer[SCHWAY__DISC_BPS_IO_CHUNK_BYTES];
    uint64_t offset = 0u;
    uint32_t value = 0u;
    while (offset < source->byte_count) {
        uint64_t remaining = source->byte_count - offset;
        size_t count = remaining > sizeof buffer ? sizeof buffer : (size_t)remaining;
        if (source->read(source->user, offset, buffer, count) == 0u) {
            return schway__disc_bps_error(error, SCHWAY_DISC_PATCH_IO,
                                           offset, "cannot read BPS source for CRC");
        }
        value = schway__runner_crc32(value, buffer, count);
        offset += (uint64_t)count;
    }
    *crc = value;
    return SCHWAY_DISC_PATCH_OK;
}

static struct schway__disc_bps_target *schway__disc_bps_target_create(
    uint64_t size)
{
    struct schway__disc_bps_target *target =
        (struct schway__disc_bps_target *)calloc(1u, sizeof *target);
    if (target == NULL) return NULL;
    target->size = size;
    if (size <= SCHWAY__DISC_BPS_MEMORY_TARGET_BYTES) {
        target->memory = (uint8_t *)malloc((size_t)size);
        if (target->memory == NULL) {
            free(target);
            return NULL;
        }
    } else {
        target->file = tmpfile();
        if (target->file == NULL) {
            free(target);
            return NULL;
        }
    }
    return target;
}

static int schway__disc_bps_target_write(
    struct schway__disc_bps_target *target, uint64_t offset,
    const uint8_t *bytes, size_t count)
{
    if (offset > target->size || (uint64_t)count > target->size - offset) return 0;
    if (target->memory != NULL) {
        memcpy(target->memory + (size_t)offset, bytes, count);
        return 1;
    }
    if (offset > (uint64_t)LONG_MAX ||
        fseek(target->file, (long)offset, SEEK_SET) != 0 ||
        fwrite(bytes, 1u, count, target->file) != count) return 0;
    return 1;
}

static int schway__disc_bps_target_read(
    struct schway__disc_bps_target *target, uint64_t offset,
    uint8_t *bytes, size_t count)
{
    if (offset > target->size || (uint64_t)count > target->size - offset) return 0;
    if (target->memory != NULL) {
        memcpy(bytes, target->memory + (size_t)offset, count);
        return 1;
    }
    if (offset > (uint64_t)LONG_MAX ||
        fseek(target->file, (long)offset, SEEK_SET) != 0 ||
        fread(bytes, 1u, count, target->file) != count) return 0;
    return 1;
}

static schway_disc_patch_status schway__disc_bps_copy_source(
    const schway_disc_patch_source *source,
    struct schway__disc_bps_target *target, uint64_t source_offset,
    uint64_t target_offset, uint64_t length, uint32_t *target_crc,
    schway_disc_patch_error *error)
{
    uint8_t buffer[SCHWAY__DISC_BPS_IO_CHUNK_BYTES];
    while (length != 0u) {
        size_t count = length > sizeof buffer ? sizeof buffer : (size_t)length;
        if (source->read(source->user, source_offset, buffer, count) == 0u) {
            return schway__disc_bps_error(error, SCHWAY_DISC_PATCH_IO,
                                           source_offset, "cannot read BPS source bytes");
        }
        if (!schway__disc_bps_target_write(target, target_offset, buffer, count)) {
            return schway__disc_bps_error(error, SCHWAY_DISC_PATCH_IO,
                                           target_offset, "cannot stage BPS target bytes");
        }
        *target_crc = schway__runner_crc32(*target_crc, buffer, count);
        source_offset += (uint64_t)count;
        target_offset += (uint64_t)count;
        length -= (uint64_t)count;
    }
    return SCHWAY_DISC_PATCH_OK;
}

static schway_disc_patch_status schway__disc_bps_copy_target(
    struct schway__disc_bps_target *target, uint64_t *source_offset,
    uint64_t *target_offset, uint64_t length, uint32_t *target_crc,
    schway_disc_patch_error *error)
{
    uint8_t buffer[SCHWAY__DISC_BPS_IO_CHUNK_BYTES];
    while (length != 0u) {
        uint64_t distance;
        size_t count;
        if (*source_offset >= *target_offset) {
            return schway__disc_bps_error(error, SCHWAY_DISC_PATCH_BOUNDS,
                                           *source_offset,
                                           "BPS TargetCopy reads an unwritten target byte");
        }
        distance = *target_offset - *source_offset;
        count = length > sizeof buffer ? sizeof buffer : (size_t)length;
        if ((uint64_t)count > distance) count = (size_t)distance;
        if (!schway__disc_bps_target_read(target, *source_offset, buffer, count) ||
            !schway__disc_bps_target_write(target, *target_offset, buffer, count)) {
            return schway__disc_bps_error(error, SCHWAY_DISC_PATCH_IO,
                                           *target_offset, "cannot copy staged BPS target bytes");
        }
        *target_crc = schway__runner_crc32(*target_crc, buffer, count);
        *source_offset += (uint64_t)count;
        *target_offset += (uint64_t)count;
        length -= (uint64_t)count;
    }
    return SCHWAY_DISC_PATCH_OK;
}

static int schway__disc_bps_move_cursor(uint64_t *cursor, uint64_t encoded)
{
    uint64_t magnitude = encoded >> 1;
    if ((encoded & 1u) != 0u) {
        if (magnitude > *cursor) return 0;
        *cursor -= magnitude;
    } else {
        if (magnitude > UINT64_MAX - *cursor) return 0;
        *cursor += magnitude;
    }
    return 1;
}

void schway__disc_bps_target_destroy(struct schway__disc_bps_target *target)
{
    if (target == NULL) return;
    free(target->memory);
    if (target->file != NULL) (void)fclose(target->file);
    free(target);
}

uint32_t schway__disc_bps_target_read_bytes(
    struct schway__disc_bps_target *target, uint64_t offset,
    uint8_t *bytes, size_t count)
{
    return target != NULL && (bytes != NULL || count == 0u) &&
           schway__disc_bps_target_read(target, offset, bytes, count) ? 1u : 0u;
}

schway_disc_patch_status schway__disc_bps_transform(
    const schway_disc_patch_source *source, const uint8_t *patch,
    size_t patch_size, struct schway__disc_bps_target **out,
    uint32_t *action_count, uint32_t *checks, schway_disc_patch_error *error)
{
    const size_t footer = patch_size >= SCHWAY__DISC_BPS_FOOTER_BYTES ?
        patch_size - SCHWAY__DISC_BPS_FOOTER_BYTES : 0u;
    size_t position = 4u;
    uint64_t source_size;
    uint64_t target_size;
    uint64_t metadata_size;
    uint64_t output_offset = 0u;
    uint64_t source_relative = 0u;
    uint64_t target_relative = 0u;
    uint32_t source_crc;
    uint32_t target_crc = 0u;
    uint32_t actions = 0u;
    uint32_t validated = 0u;
    struct schway__disc_bps_target *target = NULL;
    schway_disc_patch_status status;
    if (out != NULL) *out = NULL;
    if (action_count != NULL) *action_count = 0u;
    if (checks != NULL) *checks = 0u;
    if (out == NULL || action_count == NULL || checks == NULL || source == NULL ||
        patch == NULL || source->size != sizeof *source || source->read == NULL ||
        source->sector_bytes == 0u || source->byte_count == 0u ||
        source->byte_count > SCHWAY__DISC_BPS_MAX_IMAGE_BYTES ||
        source->byte_count % source->sector_bytes != 0u) {
        return schway__disc_bps_error(error, SCHWAY_DISC_PATCH_BAD_ARGUMENT,
                                       0u, "invalid BPS source or output");
    }
    if (patch_size > SCHWAY__DISC_BPS_MAX_PATCH_BYTES) {
        return schway__disc_bps_error(error, SCHWAY_DISC_PATCH_LIMIT,
                                       0u, "BPS patch exceeds 64 MiB");
    }
    if (patch_size < 4u + 3u + SCHWAY__DISC_BPS_FOOTER_BYTES ||
        memcmp(patch, "BPS1", 4u) != 0) {
        return schway__disc_bps_error(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                       0u, "BPS1 header or footer is truncated");
    }
    if (schway__disc_bps_u32_le(patch + patch_size - 4u) !=
        schway__runner_crc32(0u, patch, patch_size - 4u)) {
        return schway__disc_bps_error(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                       patch_size - 4u, "BPS patch CRC does not match");
    }
    validated |= SCHWAY_DISC_PATCH_CHECK_BPS_PATCH_CRC;
    if (!schway__disc_bps_number(patch, footer, &position, &source_size) ||
        !schway__disc_bps_number(patch, footer, &position, &target_size) ||
        !schway__disc_bps_number(patch, footer, &position, &metadata_size)) {
        return schway__disc_bps_error(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                       (uint64_t)position, "invalid BPS header varint");
    }
    if (source_size > SCHWAY__DISC_BPS_MAX_IMAGE_BYTES ||
        target_size > SCHWAY__DISC_BPS_MAX_IMAGE_BYTES) {
        return schway__disc_bps_error(error, SCHWAY_DISC_PATCH_LIMIT,
                                       (uint64_t)position, "BPS image exceeds 1 GiB");
    }
    if (source_size != source->byte_count) {
        return schway__disc_bps_error(error, SCHWAY_DISC_PATCH_BAD_IDENTITY,
                                       (uint64_t)position, "BPS source size does not match");
    }
    if (target_size != source_size) {
        return schway__disc_bps_error(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                       (uint64_t)position, "BPS target must preserve source geometry");
    }
    if (metadata_size > SCHWAY__DISC_BPS_MAX_METADATA_BYTES) {
        return schway__disc_bps_error(error, SCHWAY_DISC_PATCH_LIMIT,
                                       (uint64_t)position, "BPS metadata exceeds 1 MiB");
    }
    if (metadata_size > (uint64_t)(footer - position)) {
        return schway__disc_bps_error(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                       (uint64_t)position, "BPS metadata exceeds patch bounds");
    }
    position += (size_t)metadata_size;
    status = schway__disc_bps_source_crc(source, &source_crc, error);
    if (status != SCHWAY_DISC_PATCH_OK) return status;
    if (source_crc != schway__disc_bps_u32_le(patch + patch_size - 12u)) {
        return schway__disc_bps_error(error, SCHWAY_DISC_PATCH_BAD_IDENTITY,
                                       source->byte_count, "BPS source CRC does not match");
    }
    validated |= SCHWAY_DISC_PATCH_CHECK_BPS_SOURCE_CRC;
    target = schway__disc_bps_target_create(target_size);
    if (target == NULL) {
        return schway__disc_bps_error(error, SCHWAY_DISC_PATCH_IO,
                                       0u, "cannot create bounded BPS staging target");
    }
    while (position < footer) {
        uint64_t encoded_action;
        uint64_t length;
        uint32_t action;
        if (!schway__disc_bps_number(patch, footer, &position, &encoded_action)) {
            status = schway__disc_bps_error(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                             (uint64_t)position, "invalid BPS action varint");
            goto fail;
        }
        action = (uint32_t)(encoded_action & 3u);
        length = (encoded_action >> 2) + 1u;
        if (length > target_size - output_offset) {
            status = schway__disc_bps_error(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                             (uint64_t)position, "BPS action exceeds target size");
            goto fail;
        }
        if (actions == UINT32_MAX) {
            status = schway__disc_bps_error(error, SCHWAY_DISC_PATCH_LIMIT,
                                             (uint64_t)position, "BPS action count overflows");
            goto fail;
        }
        actions++;
        if (action == 0u) {
            status = schway__disc_bps_copy_source(source, target, output_offset,
                output_offset, length, &target_crc, error);
        } else if (action == 1u) {
            if (length > (uint64_t)(footer - position)) {
                status = schway__disc_bps_error(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                                 (uint64_t)position,
                                                 "truncated BPS TargetRead data");
                goto fail;
            }
            if (!schway__disc_bps_target_write(target, output_offset,
                                                patch + position, (size_t)length)) {
                status = schway__disc_bps_error(error, SCHWAY_DISC_PATCH_IO,
                                                 output_offset,
                                                 "cannot stage BPS TargetRead data");
                goto fail;
            }
            target_crc = schway__runner_crc32(target_crc, patch + position,
                                               (size_t)length);
            position += (size_t)length;
            status = SCHWAY_DISC_PATCH_OK;
        } else {
            uint64_t encoded_offset;
            uint64_t *relative = action == 2u ? &source_relative : &target_relative;
            if (!schway__disc_bps_number(patch, footer, &position, &encoded_offset) ||
                !schway__disc_bps_move_cursor(relative, encoded_offset)) {
                status = schway__disc_bps_error(error, SCHWAY_DISC_PATCH_BOUNDS,
                                                 (uint64_t)position,
                                                 "BPS relative cursor overflows");
                goto fail;
            }
            if (action == 2u) {
                if (*relative > source->byte_count ||
                    length > source->byte_count - *relative) {
                    status = schway__disc_bps_error(error, SCHWAY_DISC_PATCH_BOUNDS,
                                                     *relative,
                                                     "BPS SourceCopy exceeds source bounds");
                    goto fail;
                }
                status = schway__disc_bps_copy_source(source, target, *relative,
                    output_offset, length, &target_crc, error);
                if (status == SCHWAY_DISC_PATCH_OK) *relative += length;
            } else {
                status = schway__disc_bps_copy_target(target, relative,
                    &output_offset, length, &target_crc, error);
            }
        }
        if (status != SCHWAY_DISC_PATCH_OK) goto fail;
        if (action != 3u) output_offset += length;
    }
    if (output_offset != target_size) {
        status = schway__disc_bps_error(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                         output_offset, "BPS actions do not fill the target");
        goto fail;
    }
    if (target->file != NULL && fflush(target->file) != 0) {
        status = schway__disc_bps_error(error, SCHWAY_DISC_PATCH_IO,
                                         target_size, "cannot flush BPS staging target");
        goto fail;
    }
    if (target_crc != schway__disc_bps_u32_le(patch + patch_size - 8u)) {
        status = schway__disc_bps_error(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                         target_size, "BPS target CRC does not match");
        goto fail;
    }
    validated |= SCHWAY_DISC_PATCH_CHECK_BPS_TARGET_CRC;
    *out = target;
    *action_count = actions;
    *checks = validated;
    (void)schway__disc_bps_error(error, SCHWAY_DISC_PATCH_OK, 0u, "ok");
    return SCHWAY_DISC_PATCH_OK;

fail:
    schway__disc_bps_target_destroy(target);
    return status;
}
