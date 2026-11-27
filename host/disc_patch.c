/* Host-only, transactional PPF byte overlay over one immutable raw track. */
#include "disc_patch.h"

#include "sha256.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SCHWAY__DISC_PATCH_HEADER_BYTES 56u
#define SCHWAY__DISC_PATCH_PPF2_HEADER_BYTES 1084u
#define SCHWAY__DISC_PATCH_PPF3_HEADER_BYTES 60u
#define SCHWAY__DISC_PATCH_BLOCK_OFFSET 0x9320u
#define SCHWAY__DISC_PATCH_BLOCK_BYTES 1024u
#define SCHWAY__DISC_PATCH_FILE_ID_MAX 3072u
#define SCHWAY__DISC_PATCH_HASH_CHUNK 4096u
#define SCHWAY__DISC_PATCH_BPS_MAX_BYTES (64u * 1024u * 1024u)

struct schway__disc_bps_target;
schway_disc_patch_status schway__disc_bps_transform(
    const schway_disc_patch_source *source, const uint8_t *patch,
    size_t patch_size, struct schway__disc_bps_target **out,
    uint32_t *action_count, uint32_t *checks, schway_disc_patch_error *error);
void schway__disc_bps_target_destroy(struct schway__disc_bps_target *target);
uint32_t schway__disc_bps_target_read_bytes(
    struct schway__disc_bps_target *target, uint64_t offset,
    uint8_t *bytes, size_t count);

typedef struct schway__disc_patch_record {
    uint64_t offset;
    uint32_t length;
    uint32_t data_offset;
} schway__disc_patch_record;

typedef struct schway__disc_patch_event {
    uint64_t offset;
    uint32_t record;
    uint8_t start;
} schway__disc_patch_event;

typedef struct schway__disc_patch_segment {
    uint64_t offset;
    uint32_t length;
    uint32_t data_offset;
} schway__disc_patch_segment;

struct schway_disc_patch_candidate {
    schway_disc_patch_source source;
    struct schway__disc_bps_target *bps_target;
    uint8_t source_sha256[SCHWAY_DISC_PATCH_SHA256_BYTES];
    uint8_t *patch_bytes;
    size_t patch_size;
    uint32_t format;
    uint32_t record_count;
    uint32_t segment_count;
    schway__disc_patch_segment *segments;
};

static schway_disc_patch_status schway__disc_patch_error_set(
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

static uint32_t schway__disc_patch_read_u32_le(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static uint64_t schway__disc_patch_read_u64_le(const uint8_t *bytes)
{
    uint64_t value = 0u;
    for (size_t i = 0u; i < 8u; i++) value |= (uint64_t)bytes[i] << (8u * i);
    return value;
}

static uint16_t schway__disc_patch_read_u16_le(const uint8_t *bytes)
{
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}

static int schway__disc_patch_event_compare(const void *left, const void *right)
{
    const schway__disc_patch_event *a = (const schway__disc_patch_event *)left;
    const schway__disc_patch_event *b = (const schway__disc_patch_event *)right;
    if (a->offset < b->offset) return -1;
    if (a->offset > b->offset) return 1;
    if (a->start < b->start) return -1;
    if (a->start > b->start) return 1;
    if (a->record < b->record) return -1;
    if (a->record > b->record) return 1;
    return 0;
}

static void schway__disc_patch_heap_push(uint32_t *heap, uint32_t *heap_size,
                                         uint32_t record)
{
    uint32_t index = (*heap_size)++;
    while (index != 0u) {
        uint32_t parent = (index - 1u) / 2u;
        if (heap[parent] >= record) break;
        heap[index] = heap[parent];
        index = parent;
    }
    heap[index] = record;
}

static void schway__disc_patch_heap_pop(uint32_t *heap, uint32_t *heap_size)
{
    uint32_t value;
    uint32_t index = 0u;
    if (*heap_size == 0u) return;
    (*heap_size)--;
    if (*heap_size == 0u) return;
    value = heap[*heap_size];
    while (index <= (*heap_size - 1u) / 2u) {
        uint32_t child = index * 2u + 1u;
        if (child >= *heap_size) break;
        if (child + 1u < *heap_size && heap[child + 1u] > heap[child]) child++;
        if (heap[child] <= value) break;
        heap[index] = heap[child];
        index = child;
    }
    heap[index] = value;
}

static uint32_t schway__disc_patch_heap_top(uint32_t *heap,
                                           uint32_t *heap_size,
                                           uint8_t *active)
{
    while (*heap_size != 0u && active[heap[0]] == 0u) {
        schway__disc_patch_heap_pop(heap, heap_size);
    }
    return *heap_size != 0u ? heap[0] : UINT32_MAX;
}

static schway_disc_patch_status schway__disc_patch_source_digest(
    const schway_disc_patch_source *source, uint8_t digest[32],
    schway_disc_patch_error *error)
{
    schway__sha256 hash;
    uint8_t buffer[SCHWAY__DISC_PATCH_HASH_CHUNK];
    uint64_t offset = 0u;
    schway__sha256_init(&hash);
    while (offset < source->byte_count) {
        uint64_t remaining = source->byte_count - offset;
        size_t count = remaining > sizeof buffer ? sizeof buffer : (size_t)remaining;
        if (source->read(source->user, offset, buffer, count) == 0u) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_IO,
                                                 offset, "cannot read complete patch source");
        }
        schway__sha256_update(&hash, buffer, count);
        offset += (uint64_t)count;
    }
    schway__sha256_final(&hash, digest);
    return SCHWAY_DISC_PATCH_OK;
}

static int schway__disc_patch_digest_equal(const uint8_t left[32],
                                           const uint8_t right[32])
{
    uint8_t difference = 0u;
    for (size_t i = 0u; i < SCHWAY_DISC_PATCH_SHA256_BYTES; i++) {
        difference |= (uint8_t)(left[i] ^ right[i]);
    }
    return difference == 0u;
}

static schway_disc_patch_status schway__disc_patch_scan_ppf1(
    const schway_disc_patch_source *source, const uint8_t *patch_bytes,
    size_t patch_size, uint32_t *record_count, schway_disc_patch_error *error)
{
    size_t position = SCHWAY__DISC_PATCH_HEADER_BYTES;
    uint32_t records = 0u;
    uint64_t payload_bytes = 0u;
    while (position < patch_size) {
        uint32_t offset;
        uint8_t count;
        if (patch_size - position < 5u) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                                 (uint64_t)position,
                                                 "truncated PPF1 record header");
        }
        offset = schway__disc_patch_read_u32_le(patch_bytes + position);
        count = patch_bytes[position + 4u];
        if (count == 0u) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                                 (uint64_t)position,
                                                 "PPF1 records must contain at least one byte");
        }
        if (records >= SCHWAY_DISC_PATCH_MAX_RECORDS) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_LIMIT,
                                                 (uint64_t)position,
                                                 "PPF1 exceeds the record limit");
        }
        if ((uint64_t)offset > source->byte_count ||
            (uint64_t)count > source->byte_count - (uint64_t)offset) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BOUNDS,
                                                 (uint64_t)position,
                                                 "PPF1 record exceeds the raw source");
        }
        position += 5u;
        if ((size_t)count > patch_size - position) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                                 (uint64_t)position,
                                                 "truncated PPF1 record data");
        }
        payload_bytes += count;
        if (payload_bytes > SCHWAY_DISC_PATCH_MAX_BYTES) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_LIMIT,
                                                 (uint64_t)position,
                                                 "PPF1 exceeds the patch data limit");
        }
        position += (size_t)count;
        records++;
    }
    if (records == 0u) {
        return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                             SCHWAY__DISC_PATCH_HEADER_BYTES,
                                             "PPF1 contains no records");
    }
    *record_count = records;
    return SCHWAY_DISC_PATCH_OK;
}

static void schway__disc_patch_fill_records(const uint8_t *patch_bytes,
                                            size_t patch_size,
                                            schway__disc_patch_record *records)
{
    size_t position = SCHWAY__DISC_PATCH_HEADER_BYTES;
    uint32_t index = 0u;
    while (position < patch_size) {
        uint8_t count = patch_bytes[position + 4u];
        position += 5u;
        records[index].offset = schway__disc_patch_read_u32_le(patch_bytes + position - 5u);
        records[index].length = (uint32_t)count;
        records[index].data_offset = (uint32_t)position;
        position += (size_t)count;
        index++;
    }
}

static int schway__disc_patch_is_file_id(const uint8_t *patch_bytes,
                                         size_t patch_size, size_t position)
{
    static const uint8_t begin[] = "@BEGIN_FILE_ID.DIZ";
    size_t remaining = patch_size - position;
    return remaining >= sizeof begin - 1u &&
           memcmp(patch_bytes + position, begin, sizeof begin - 1u) == 0;
}

static schway_disc_patch_status schway__disc_patch_validate_file_id(
    const uint8_t *patch_bytes, size_t patch_size, size_t position,
    schway_disc_patch_error *error)
{
    static const uint8_t begin[] = "@BEGIN_FILE_ID.DIZ";
    static const uint8_t end[] = "@END_FILE_ID.DIZ";
    const size_t begin_bytes = sizeof begin - 1u;
    const size_t end_bytes = sizeof end - 1u;
    size_t end_offset;
    size_t description_offset;
    size_t description_bytes;
    uint16_t declared_bytes;
    if (patch_size - position < begin_bytes + end_bytes + 2u ||
        memcmp(patch_bytes + position, begin, begin_bytes) != 0) {
        return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                             (uint64_t)position,
                                             "truncated PPF3 FileID trailer");
    }
    declared_bytes = schway__disc_patch_read_u16_le(patch_bytes + patch_size - 2u);
    if ((size_t)declared_bytes > SCHWAY__DISC_PATCH_FILE_ID_MAX) {
        return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                             (uint64_t)(patch_size - 2u),
                                             "PPF3 FileID exceeds 3,072 bytes");
    }
    end_offset = patch_size - 2u - end_bytes;
    description_offset = position + begin_bytes;
    if (end_offset < description_offset) {
        return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                             (uint64_t)position,
                                             "PPF3 FileID trailer bounds are invalid");
    }
    description_bytes = end_offset - description_offset;
    if (description_bytes != (size_t)declared_bytes ||
        memcmp(patch_bytes + end_offset, end, end_bytes) != 0) {
        return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                             (uint64_t)end_offset,
                                             "PPF3 FileID length or end marker is invalid");
    }
    return SCHWAY_DISC_PATCH_OK;
}

static schway_disc_patch_status schway__disc_patch_check_block(
    const schway_disc_patch_source *source, const uint8_t *patch_bytes,
    schway_disc_patch_error *error)
{
    uint8_t source_block[SCHWAY__DISC_PATCH_BLOCK_BYTES];
    uint64_t source_end = SCHWAY__DISC_PATCH_BLOCK_OFFSET +
                          SCHWAY__DISC_PATCH_BLOCK_BYTES;
    if (source->byte_count < source_end) {
        return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_IDENTITY,
                                             source->byte_count,
                                             "source is too short for the PPF validation block");
    }
    if (source->read(source->user, SCHWAY__DISC_PATCH_BLOCK_OFFSET,
                     source_block, sizeof source_block) == 0u) {
        return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_IO,
                                             SCHWAY__DISC_PATCH_BLOCK_OFFSET,
                                             "cannot read the PPF validation block");
    }
    if (memcmp(source_block, patch_bytes + 60u, sizeof source_block) != 0) {
        return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_IDENTITY,
                                             SCHWAY__DISC_PATCH_BLOCK_OFFSET,
                                             "source PPF validation block does not match");
    }
    return SCHWAY_DISC_PATCH_OK;
}

static schway_disc_patch_status schway__disc_patch_scan_ppf23(
    const schway_disc_patch_source *source, const uint8_t *patch_bytes,
    size_t patch_size, schway_disc_patch_format format, size_t header_bytes,
    uint8_t has_undo, uint32_t *record_count, schway_disc_patch_error *error)
{
    size_t position = header_bytes;
    uint32_t records = 0u;
    uint64_t payload_bytes = 0u;
    while (position < patch_size) {
        uint64_t offset;
        uint8_t count;
        size_t record_header_bytes = format == SCHWAY_DISC_PATCH_FORMAT_PPF2 ? 5u : 9u;
        if (format == SCHWAY_DISC_PATCH_FORMAT_PPF3 &&
            schway__disc_patch_is_file_id(patch_bytes, patch_size, position)) {
            schway_disc_patch_status status = schway__disc_patch_validate_file_id(
                patch_bytes, patch_size, position, error);
            if (status != SCHWAY_DISC_PATCH_OK) return status;
            position = patch_size;
            break;
        }
        if (patch_size - position < record_header_bytes) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                                 (uint64_t)position,
                                                 "truncated PPF2/PPF3 record header");
        }
        offset = format == SCHWAY_DISC_PATCH_FORMAT_PPF2 ?
            (uint64_t)schway__disc_patch_read_u32_le(patch_bytes + position) :
            schway__disc_patch_read_u64_le(patch_bytes + position);
        count = patch_bytes[position + record_header_bytes - 1u];
        if (count == 0u) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                                 (uint64_t)position,
                                                 "PPF2/PPF3 records must contain at least one byte");
        }
        if (records >= SCHWAY_DISC_PATCH_MAX_RECORDS) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_LIMIT,
                                                 (uint64_t)position,
                                                 "PPF2/PPF3 exceeds the record limit");
        }
        if (offset > source->byte_count ||
            (uint64_t)count > source->byte_count - offset) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BOUNDS,
                                                 (uint64_t)position,
                                                 "PPF2/PPF3 record exceeds the raw source");
        }
        position += record_header_bytes;
        if ((size_t)count > patch_size - position) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                                 (uint64_t)position,
                                                 "truncated PPF2/PPF3 record data");
        }
        position += (size_t)count;
        payload_bytes += count;
        if (has_undo != 0u) {
            if ((size_t)count > patch_size - position) {
                return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                                     (uint64_t)position,
                                                     "truncated PPF3 undo data");
            }
            position += (size_t)count;
            payload_bytes += count;
        }
        if (payload_bytes > SCHWAY_DISC_PATCH_MAX_BYTES) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_LIMIT,
                                                 (uint64_t)position,
                                                 "PPF2/PPF3 exceeds the patch data limit");
        }
        records++;
    }
    if (records == 0u) {
        return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                             (uint64_t)header_bytes,
                                             "PPF2/PPF3 contains no records");
    }
    *record_count = records;
    return SCHWAY_DISC_PATCH_OK;
}

static void schway__disc_patch_fill_records_ppf23(
    const uint8_t *patch_bytes, schway_disc_patch_format format,
    size_t header_bytes, uint32_t record_count, uint8_t has_undo,
    schway__disc_patch_record *records)
{
    size_t position = header_bytes;
    size_t offset_bytes = format == SCHWAY_DISC_PATCH_FORMAT_PPF2 ? 4u : 8u;
    for (uint32_t index = 0u; index < record_count; index++) {
        uint8_t count = patch_bytes[position + offset_bytes];
        records[index].offset = offset_bytes == 4u ?
            (uint64_t)schway__disc_patch_read_u32_le(patch_bytes + position) :
            schway__disc_patch_read_u64_le(patch_bytes + position);
        records[index].length = (uint32_t)count;
        records[index].data_offset = (uint32_t)(position + offset_bytes + 1u);
        position += offset_bytes + 1u + (size_t)count * (has_undo != 0u ? 2u : 1u);
    }
}

static schway_disc_patch_status schway__disc_patch_build_overlay(
    schway_disc_patch_candidate *candidate,
    const schway__disc_patch_record *records,
    schway_disc_patch_error *error)
{
    uint32_t event_count = candidate->record_count * 2u;
    schway__disc_patch_event *events = NULL;
    uint8_t *active = NULL;
    uint32_t *heap = NULL;
    uint32_t position = 0u;
    uint32_t heap_size = 0u;
    uint64_t previous = 0u;
    schway_disc_patch_status status = SCHWAY_DISC_PATCH_OUT_OF_MEMORY;
    events = (schway__disc_patch_event *)malloc((size_t)event_count * sizeof *events);
    active = (uint8_t *)calloc(candidate->record_count, sizeof *active);
    heap = (uint32_t *)malloc((size_t)candidate->record_count * sizeof *heap);
    candidate->segments = (schway__disc_patch_segment *)malloc(
        (size_t)event_count * sizeof *candidate->segments);
    if (events == NULL || active == NULL || heap == NULL || candidate->segments == NULL) {
        goto done;
    }
    for (uint32_t i = 0u; i < candidate->record_count; i++) {
        events[2u * i].offset = records[i].offset;
        events[2u * i].record = i;
        events[2u * i].start = 1u;
        events[2u * i + 1u].offset = records[i].offset + records[i].length;
        events[2u * i + 1u].record = i;
        events[2u * i + 1u].start = 0u;
    }
    qsort(events, event_count, sizeof *events, schway__disc_patch_event_compare);
    previous = events[0].offset;
    while (position < event_count) {
        uint64_t current = events[position].offset;
        uint32_t winner = schway__disc_patch_heap_top(heap, &heap_size, active);
        if (current > previous && winner != UINT32_MAX) {
            uint64_t length64 = current - previous;
            uint64_t data_offset64 = (uint64_t)records[winner].data_offset +
                                     (previous - records[winner].offset);
            uint32_t length = (uint32_t)length64;
            uint32_t data_offset = (uint32_t)data_offset64;
            uint32_t prior = candidate->segment_count;
            if (prior != 0u &&
                candidate->segments[prior - 1u].offset +
                    candidate->segments[prior - 1u].length == previous &&
                candidate->segments[prior - 1u].data_offset +
                    candidate->segments[prior - 1u].length == data_offset) {
                candidate->segments[prior - 1u].length += length;
            } else {
                schway__disc_patch_segment *segment =
                    &candidate->segments[candidate->segment_count++];
                segment->offset = previous;
                segment->length = length;
                segment->data_offset = data_offset;
            }
        }
        uint32_t end = position;
        while (end < event_count && events[end].offset == current) end++;
        for (uint32_t i = position; i < end; i++) {
            if (events[i].start == 0u) active[events[i].record] = 0u;
        }
        for (uint32_t i = position; i < end; i++) {
            if (events[i].start != 0u) {
                active[events[i].record] = 1u;
                schway__disc_patch_heap_push(heap, &heap_size, events[i].record);
            }
        }
        (void)schway__disc_patch_heap_top(heap, &heap_size, active);
        previous = current;
        position = end;
    }
    if (candidate->segment_count == 0u) {
        status = schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                               SCHWAY__DISC_PATCH_HEADER_BYTES,
                                               "PPF1 overlay is empty");
        goto done;
    }
    status = SCHWAY_DISC_PATCH_OK;
done:
    free(events);
    free(active);
    free(heap);
    if (status != SCHWAY_DISC_PATCH_OK) {
        free(candidate->segments);
        candidate->segments = NULL;
        candidate->segment_count = 0u;
        if (status == SCHWAY_DISC_PATCH_OUT_OF_MEMORY) {
            (void)schway__disc_patch_error_set(error, status, 0u,
                                                "out of memory building PPF1 overlay");
        }
    }
    return status;
}

schway_disc_patch_status schway_disc_patch_load(
    const schway_disc_patch_source *source, schway_disc_patch_format format,
    const uint8_t *patch_bytes, size_t patch_size,
    const uint8_t expected_source_sha256[SCHWAY_DISC_PATCH_SHA256_BYTES],
    schway_disc_patch_candidate **out, schway_disc_patch_report *report,
    schway_disc_patch_error *error)
{
    schway_disc_patch_candidate *candidate = NULL;
    schway__disc_patch_record *records = NULL;
    uint8_t source_digest[SCHWAY_DISC_PATCH_SHA256_BYTES];
    uint32_t record_count = 0u;
    uint32_t checks = SCHWAY_DISC_PATCH_CHECK_SOURCE_SHA256;
    uint32_t ppf2_source_size = 0u;
    size_t header_bytes = SCHWAY__DISC_PATCH_HEADER_BYTES;
    uint8_t has_undo = 0u;
    schway_disc_patch_status status;
    if (out != NULL) *out = NULL;
    if (report != NULL) memset(report, 0, sizeof *report);
    if (error != NULL) {
        memset(error, 0, sizeof *error);
        error->size = (uint32_t)sizeof *error;
    }
    if (out == NULL || source == NULL || source->size != sizeof *source ||
        source->sector_bytes != 2352u || source->byte_count == 0u ||
        source->byte_count % source->sector_bytes != 0u || source->read == NULL ||
        patch_bytes == NULL) {
        return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_ARGUMENT,
                                             0u, "invalid patch source, input or output");
    }
    if (format != SCHWAY_DISC_PATCH_FORMAT_PPF1 &&
        format != SCHWAY_DISC_PATCH_FORMAT_PPF2 &&
        format != SCHWAY_DISC_PATCH_FORMAT_PPF3 &&
        format != SCHWAY_DISC_PATCH_FORMAT_BPS) {
        return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_UNSUPPORTED,
                                             0u, "selected patch format is not implemented");
    }
    if (format == SCHWAY_DISC_PATCH_FORMAT_BPS) {
        uint32_t action_count = 0u;
        uint32_t bps_checks = 0u;
        if (patch_size > SCHWAY__DISC_PATCH_BPS_MAX_BYTES) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_LIMIT,
                0u, "BPS patch exceeds 64 MiB");
        }
        if (source->byte_count > (UINT64_C(1) << 30)) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_LIMIT,
                source->byte_count, "BPS source exceeds 1 GiB");
        }
        if (expected_source_sha256 != NULL) {
            status = schway__disc_patch_source_digest(source, source_digest, error);
            if (status != SCHWAY_DISC_PATCH_OK) return status;
            if (!schway__disc_patch_digest_equal(source_digest,
                                                  expected_source_sha256)) {
                return schway__disc_patch_error_set(error,
                    SCHWAY_DISC_PATCH_BAD_IDENTITY, 0u,
                    "raw source SHA-256 does not match");
            }
            bps_checks |= SCHWAY_DISC_PATCH_CHECK_SOURCE_SHA256;
        } else {
            memset(source_digest, 0, sizeof source_digest);
        }
        candidate = (schway_disc_patch_candidate *)calloc(1u, sizeof *candidate);
        if (candidate == NULL) {
            return schway__disc_patch_error_set(error,
                SCHWAY_DISC_PATCH_OUT_OF_MEMORY, 0u,
                "out of memory allocating BPS candidate");
        }
        candidate->source = *source;
        memcpy(candidate->source_sha256, source_digest, sizeof source_digest);
        candidate->format = (uint32_t)format;
        status = schway__disc_bps_transform(source, patch_bytes, patch_size,
            &candidate->bps_target, &action_count, &bps_checks, error);
        if (status != SCHWAY_DISC_PATCH_OK) {
            schway_disc_patch_candidate_destroy(candidate);
            return status;
        }
        bps_checks |= expected_source_sha256 != NULL ?
            SCHWAY_DISC_PATCH_CHECK_SOURCE_SHA256 : 0u;
        candidate->record_count = action_count;
        if (report != NULL) {
            report->size = (uint32_t)sizeof *report;
            report->format = (uint32_t)format;
            report->checks = bps_checks;
            report->records = action_count;
            report->source_bytes = source->byte_count;
            report->patch_bytes = (uint64_t)patch_size;
            if (expected_source_sha256 != NULL) {
                memcpy(report->source_sha256, source_digest, sizeof source_digest);
            }
        }
        *out = candidate;
        (void)schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_OK,
                                            0u, "ok");
        return SCHWAY_DISC_PATCH_OK;
    }
    if (expected_source_sha256 == NULL) {
        return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_ARGUMENT,
                                             0u, "PPF requires the full-source SHA-256");
    }
    if (patch_size > SCHWAY_DISC_PATCH_MAX_BYTES) {
        return schway__disc_patch_error_set(error,
            SCHWAY_DISC_PATCH_LIMIT, 0u, "PPF patch exceeds the byte limit");
    }
    if (format == SCHWAY_DISC_PATCH_FORMAT_PPF1) {
        if (patch_size < SCHWAY__DISC_PATCH_HEADER_BYTES ||
            memcmp(patch_bytes, "PPF10", 5u) != 0 || patch_bytes[5] != 0u) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                                 0u, "PPF1 header or method is invalid");
        }
    } else if (format == SCHWAY_DISC_PATCH_FORMAT_PPF2) {
        if (patch_size < SCHWAY__DISC_PATCH_PPF2_HEADER_BYTES ||
            memcmp(patch_bytes, "PPF20", 5u) != 0 || patch_bytes[5] != 1u) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                                 0u, "PPF2 header or method is invalid");
        }
        ppf2_source_size = schway__disc_patch_read_u32_le(patch_bytes + 56u);
        header_bytes = SCHWAY__DISC_PATCH_PPF2_HEADER_BYTES;
    } else {
        if (patch_size < SCHWAY__DISC_PATCH_PPF3_HEADER_BYTES ||
            memcmp(patch_bytes, "PPF30", 5u) != 0 || patch_bytes[5] != 2u) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                                 0u, "PPF3 header or method is invalid");
        }
        if (patch_bytes[56] != 0u) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_UNSUPPORTED,
                                                 56u, "PPF3 supports BIN image type only");
        }
        if (patch_bytes[57] > 1u || patch_bytes[58] > 1u || patch_bytes[59] != 0u) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                                 57u, "PPF3 flags or reserved byte are invalid");
        }
        has_undo = patch_bytes[58];
        header_bytes = patch_bytes[57] != 0u ?
            SCHWAY__DISC_PATCH_PPF2_HEADER_BYTES :
            SCHWAY__DISC_PATCH_PPF3_HEADER_BYTES;
        if (patch_size < header_bytes) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_PATCH,
                                                 60u, "PPF3 validation block is truncated");
        }
    }
    status = schway__disc_patch_source_digest(source, source_digest, error);
    if (status != SCHWAY_DISC_PATCH_OK) return status;
    if (!schway__disc_patch_digest_equal(source_digest, expected_source_sha256)) {
        return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_IDENTITY,
                                             0u, "raw source SHA-256 does not match");
    }
    if (format == SCHWAY_DISC_PATCH_FORMAT_PPF2) {
        if (source->byte_count > UINT32_MAX ||
            (uint64_t)ppf2_source_size != source->byte_count) {
            return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_BAD_IDENTITY,
                                                 56u, "PPF2 source size does not match");
        }
        checks |= SCHWAY_DISC_PATCH_CHECK_PPF2_SOURCE_SIZE;
        status = schway__disc_patch_check_block(source, patch_bytes, error);
        if (status != SCHWAY_DISC_PATCH_OK) return status;
        checks |= SCHWAY_DISC_PATCH_CHECK_PPF2_BLOCK;
    } else if (format == SCHWAY_DISC_PATCH_FORMAT_PPF3 && patch_bytes[57] != 0u) {
        status = schway__disc_patch_check_block(source, patch_bytes, error);
        if (status != SCHWAY_DISC_PATCH_OK) return status;
        checks |= SCHWAY_DISC_PATCH_CHECK_PPF3_BLOCK;
    }
    status = format == SCHWAY_DISC_PATCH_FORMAT_PPF1 ?
        schway__disc_patch_scan_ppf1(source, patch_bytes, patch_size,
                                     &record_count, error) :
        schway__disc_patch_scan_ppf23(source, patch_bytes, patch_size,
                                      format, header_bytes, has_undo,
                                      &record_count, error);
    if (status != SCHWAY_DISC_PATCH_OK) return status;
    records = (schway__disc_patch_record *)calloc(record_count, sizeof *records);
    if (records == NULL) {
        return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_OUT_OF_MEMORY,
                                             0u, "out of memory validating PPF records");
    }
    if (format == SCHWAY_DISC_PATCH_FORMAT_PPF1) {
        schway__disc_patch_fill_records(patch_bytes, patch_size, records);
    } else {
        schway__disc_patch_fill_records_ppf23(patch_bytes, format, header_bytes,
                                              record_count, has_undo, records);
    }
    candidate = (schway_disc_patch_candidate *)calloc(1u, sizeof *candidate);
    if (candidate == NULL) {
        free(records);
        return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_OUT_OF_MEMORY,
                                             0u, "out of memory allocating patch candidate");
    }
    candidate->patch_bytes = (uint8_t *)malloc(patch_size);
    if (candidate->patch_bytes == NULL) {
        free(records);
        schway_disc_patch_candidate_destroy(candidate);
        return schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_OUT_OF_MEMORY,
                                             0u, "out of memory copying patch bytes");
    }
    memcpy(candidate->patch_bytes, patch_bytes, patch_size);
    candidate->source = *source;
    memcpy(candidate->source_sha256, source_digest, sizeof source_digest);
    candidate->patch_size = patch_size;
    candidate->format = (uint32_t)format;
    candidate->record_count = record_count;
    status = schway__disc_patch_build_overlay(candidate, records, error);
    free(records);
    if (status != SCHWAY_DISC_PATCH_OK) {
        schway_disc_patch_candidate_destroy(candidate);
        return status;
    }
    if (report != NULL) {
        report->size = (uint32_t)sizeof *report;
        report->format = (uint32_t)format;
        report->checks = checks;
        report->records = record_count;
        report->source_bytes = source->byte_count;
        report->patch_bytes = (uint64_t)patch_size;
        memcpy(report->source_sha256, source_digest, sizeof source_digest);
    }
    *out = candidate;
    (void)schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_OK, 0u, "ok");
    return SCHWAY_DISC_PATCH_OK;
}

void schway_disc_patch_candidate_destroy(schway_disc_patch_candidate *candidate)
{
    if (candidate == NULL) return;
    schway__disc_bps_target_destroy(candidate->bps_target);
    free(candidate->patch_bytes);
    free(candidate->segments);
    free(candidate);
}

uint32_t schway_disc_patch_candidate_matches_source(
    const schway_disc_patch_candidate *candidate,
    const schway_disc_patch_source *source)
{
    if (candidate == NULL || source == NULL || source->size != sizeof *source) return 0u;
    return candidate->source.byte_count == source->byte_count &&
           candidate->source.sector_bytes == source->sector_bytes &&
           candidate->source.read == source->read &&
           candidate->source.user == source->user ? 1u : 0u;
}

schway_disc_patch_status schway_disc_patch_candidate_apply(
    const schway_disc_patch_candidate *candidate, uint64_t offset,
    uint8_t *bytes, size_t count)
{
    uint64_t end;
    uint32_t low = 0u;
    uint32_t high;
    if (candidate == NULL || (bytes == NULL && count != 0u)) {
        return SCHWAY_DISC_PATCH_BAD_ARGUMENT;
    }
    if (offset > candidate->source.byte_count ||
        (uint64_t)count > candidate->source.byte_count - offset) {
        return SCHWAY_DISC_PATCH_BOUNDS;
    }
    if (count == 0u) return SCHWAY_DISC_PATCH_OK;
    if (candidate->bps_target != NULL) {
        return schway__disc_bps_target_read_bytes(candidate->bps_target,
                                                  offset, bytes, count) != 0u ?
            SCHWAY_DISC_PATCH_OK : SCHWAY_DISC_PATCH_IO;
    }
    end = offset + (uint64_t)count;
    high = candidate->segment_count;
    while (low < high) {
        uint32_t middle = low + (high - low) / 2u;
        const schway__disc_patch_segment *segment = &candidate->segments[middle];
        if (segment->offset + segment->length <= offset) low = middle + 1u;
        else high = middle;
    }
    while (low < candidate->segment_count &&
           candidate->segments[low].offset < end) {
        const schway__disc_patch_segment *segment = &candidate->segments[low];
        uint64_t copy_start = segment->offset > offset ? segment->offset : offset;
        uint64_t segment_end = segment->offset + segment->length;
        uint64_t copy_end = segment_end < end ? segment_end : end;
        size_t target_offset = (size_t)(copy_start - offset);
        size_t data_offset = (size_t)segment->data_offset +
                             (size_t)(copy_start - segment->offset);
        size_t copy_size = (size_t)(copy_end - copy_start);
        memcpy(bytes + target_offset, candidate->patch_bytes + data_offset, copy_size);
        low++;
    }
    return SCHWAY_DISC_PATCH_OK;
}
