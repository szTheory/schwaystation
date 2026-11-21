#include "disc_chd_metadata.h"
#include "media_limits.h"

#include <string.h>

#define SCHWAY__DISC_CHD_FRAME_BYTES UINT32_C(2448)
#define SCHWAY__DISC_CHD_FRAME_PAD UINT32_C(4)

typedef struct schway__disc_chd_record_fields {
    uint32_t number;
    uint32_t frames;
    uint32_t pregap;
    uint32_t postgap;
    uint32_t toc_type;
    uint32_t pregap_type;
    uint32_t stored_pregap;
} schway__disc_chd_record_fields;

static int schway__disc_chd_next_token(const char **cursor, const char *key,
                                       char *value, size_t capacity)
{
    const char *start;
    const char *end;
    size_t key_length = strlen(key);
    size_t value_length;

    if (strncmp(*cursor, key, key_length) != 0) return 0;
    start = *cursor + key_length;
    end = start;
    while (*end != '\0' && *end != ' ') end++;
    value_length = (size_t)(end - start);
    if (value_length == 0u || value_length >= capacity) return 0;
    memcpy(value, start, value_length);
    value[value_length] = '\0';
    if (*end == ' ') end++;
    *cursor = end;
    return 1;
}

static int schway__disc_chd_parse_u32(const char *text, uint32_t *value)
{
    uint32_t parsed = 0u;
    size_t i;

    if (text == NULL || text[0] == '\0') return 0;
    for (i = 0u; text[i] != '\0'; i++) {
        uint32_t digit;
        if (text[i] < '0' || text[i] > '9') return 0;
        digit = (uint32_t)(text[i] - '0');
        if (parsed > (UINT32_MAX - digit) / 10u) return 0;
        parsed = parsed * 10u + digit;
    }
    *value = parsed;
    return 1;
}

static int schway__disc_chd_type(const char *text, uint32_t *type)
{
    if (strcmp(text, "AUDIO") == 0) {
        *type = SCHWAY_DISC_TRACK_AUDIO;
        return 1;
    }
    if (strcmp(text, "MODE1_RAW") == 0) {
        *type = SCHWAY_DISC_TRACK_MODE1;
        return 1;
    }
    if (strcmp(text, "MODE2_RAW") == 0) {
        *type = SCHWAY_DISC_TRACK_MODE2;
        return 1;
    }
    return 0;
}

static int schway__disc_chd_parse_record(
    const schway__disc_chd_metadata_record *record,
    schway__disc_chd_record_fields *fields)
{
    char number[16];
    char type[32];
    char subtype[16];
    char frames[16];
    char pregap[16];
    char pregap_type[32];
    char pregap_subtype[16];
    char postgap[16];
    const char *cursor;
    int modern;

    if (record == NULL || fields == NULL || record->text == NULL ||
        record->length < 2u || record->length > SCHWAY__DISC_CHD_METADATA_TEXT_MAX ||
        record->text[record->length - 1u] != '\0' ||
        memchr(record->text, '\0', record->length - 1u) != NULL) return 0;
    modern = record->tag == SCHWAY__DISC_CHD_TAG_CHT2;
    if (!modern && record->tag != SCHWAY__DISC_CHD_TAG_CHTR) return -1;

    cursor = record->text;
    if (!schway__disc_chd_next_token(&cursor, "TRACK:", number, sizeof number) ||
        !schway__disc_chd_next_token(&cursor, "TYPE:", type, sizeof type) ||
        !schway__disc_chd_next_token(&cursor, "SUBTYPE:", subtype, sizeof subtype) ||
        !schway__disc_chd_next_token(&cursor, "FRAMES:", frames, sizeof frames) ||
        !schway__disc_chd_parse_u32(number, &fields->number) ||
        !schway__disc_chd_parse_u32(frames, &fields->frames)) return 0;
    if (strcmp(subtype, "NONE") != 0) return -1;
    if (!schway__disc_chd_type(type, &fields->toc_type)) return -1;
    fields->pregap = 0u;
    fields->postgap = 0u;
    fields->pregap_type = fields->toc_type;
    fields->stored_pregap = 0u;

    if (modern) {
        if (!schway__disc_chd_next_token(&cursor, "PREGAP:", pregap, sizeof pregap) ||
            !schway__disc_chd_next_token(&cursor, "PGTYPE:", pregap_type, sizeof pregap_type) ||
            !schway__disc_chd_next_token(&cursor, "PGSUB:", pregap_subtype, sizeof pregap_subtype) ||
            !schway__disc_chd_next_token(&cursor, "POSTGAP:", postgap, sizeof postgap) ||
            !schway__disc_chd_parse_u32(pregap, &fields->pregap) ||
            !schway__disc_chd_parse_u32(postgap, &fields->postgap) || *cursor != '\0') return 0;
        if (strcmp(pregap_subtype, "NONE") != 0) return -1;
        if (fields->postgap != 0u) return -1;
        if (fields->pregap != 0u) {
            if (pregap_type[0] == 'V') {
                fields->stored_pregap = 1u;
                memmove(pregap_type, pregap_type + 1u, strlen(pregap_type));
            }
            if (!schway__disc_chd_type(pregap_type, &fields->pregap_type) ||
                fields->pregap_type != fields->toc_type) return -1;
        }
    } else if (*cursor != '\0') {
        return 0;
    }
    if (fields->frames == 0u || fields->number == 0u) return 0;
    if (fields->stored_pregap != 0u && fields->pregap >= fields->frames) return 0;
    return 1;
}

schway_disc_image_status schway__disc_chd_metadata_parse(
    const schway__disc_chd_metadata_record *records, uint32_t record_count,
    uint32_t version, uint32_t hunk_bytes, uint64_t logical_bytes,
    schway__disc_chd_metadata_map *out)
{
    schway__disc_chd_metadata_map parsed;
    uint64_t logical_cursor = 0u;
    uint64_t storage_cursor = 0u;
    uint32_t tag = 0u;

    if (out != NULL) memset(out, 0, sizeof(*out));
    if (out == NULL) return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    if (record_count == 0u) return SCHWAY_DISC_IMAGE_UNSUPPORTED;
    if (records == NULL) return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    if (record_count > SCHWAY_DISC_MAX_TRACKS) return SCHWAY_DISC_IMAGE_LIMIT;
    if (version < 3u || version > 5u) return SCHWAY_DISC_IMAGE_UNSUPPORTED;
    if (logical_bytes > SCHWAY__MEDIA_IMAGE_MAX_BYTES) return SCHWAY_DISC_IMAGE_LIMIT;
    if (hunk_bytes == 0u || hunk_bytes % SCHWAY__DISC_CHD_FRAME_BYTES != 0u ||
        logical_bytes == 0u || logical_bytes % SCHWAY__DISC_CHD_FRAME_BYTES != 0u) {
        return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    }

    memset(&parsed, 0, sizeof(parsed));
    parsed.track_count = record_count;
    for (uint32_t i = 0u; i < record_count; i++) {
        schway__disc_chd_record_fields fields;
        schway__disc_chd_track_map *track = &parsed.tracks[i];
        uint64_t stored_frames;
        uint64_t next_logical;
        uint64_t leadout;
        int record_result;

        if (i == 0u) tag = records[i].tag;
        if (records[i].tag != tag) return SCHWAY_DISC_IMAGE_UNSUPPORTED;
        if (tag != SCHWAY__DISC_CHD_TAG_CHT2 && tag != SCHWAY__DISC_CHD_TAG_CHTR) {
            return SCHWAY_DISC_IMAGE_UNSUPPORTED;
        }
        record_result = schway__disc_chd_parse_record(&records[i], &fields);
        if (record_result < 0) return SCHWAY_DISC_IMAGE_UNSUPPORTED;
        if (record_result == 0 || fields.number != i + 1u) return SCHWAY_DISC_IMAGE_BAD_IMAGE;

        stored_frames = ((uint64_t)fields.frames + (SCHWAY__DISC_CHD_FRAME_PAD - 1u)) /
                        SCHWAY__DISC_CHD_FRAME_PAD * SCHWAY__DISC_CHD_FRAME_PAD;
        if (storage_cursor > UINT32_MAX || stored_frames > UINT32_MAX - storage_cursor) {
            return SCHWAY_DISC_IMAGE_LIMIT;
        }
        if (logical_cursor > UINT32_MAX) return SCHWAY_DISC_IMAGE_LIMIT;
        track->toc.size = (uint32_t)sizeof(track->toc);
        track->toc.type = fields.toc_type;
        track->toc.index0 = (uint32_t)logical_cursor;
        if (logical_cursor + fields.pregap > UINT32_MAX) return SCHWAY_DISC_IMAGE_LIMIT;
        track->toc.index1 = (uint32_t)(logical_cursor + fields.pregap);
        track->toc.pregap = fields.pregap;
        leadout = logical_cursor + fields.pregap + fields.frames -
                  (fields.stored_pregap != 0u ? fields.pregap : 0u);
        if (leadout > UINT32_MAX) return SCHWAY_DISC_IMAGE_LIMIT;
        track->toc.leadout = (uint32_t)leadout;
        next_logical = leadout;
        track->chd_frame_start = (uint32_t)storage_cursor;
        track->chd_frame_count = fields.frames;
        track->stored_pregap = fields.stored_pregap;
        storage_cursor += stored_frames;
        logical_cursor = next_logical;
    }
    if (storage_cursor != logical_bytes / SCHWAY__DISC_CHD_FRAME_BYTES ||
        hunk_bytes / SCHWAY__DISC_CHD_FRAME_BYTES == 0u) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    parsed.chd_frame_count = (uint32_t)storage_cursor;
    *out = parsed;
    return SCHWAY_DISC_IMAGE_OK;
}
