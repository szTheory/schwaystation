#if !defined(_WIN32) && !defined(_XOPEN_SOURCE)
#define _XOPEN_SOURCE 700
#endif
#include "disc_ccd.h"
#include "media_limits.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#define SCHWAY__DISC_CCD_DESCRIPTOR_MAX_BYTES UINT64_C(65536)
#define SCHWAY__DISC_CCD_PATH_CAP 4096u
#define SCHWAY__DISC_CCD_ENTRY_COUNT 4u
#define SCHWAY__DISC_CCD_ENTRY_REQUIRED UINT32_C(0x3fff)
#define SCHWAY__DISC_CCD_SECTOR_BYTES UINT64_C(2352)
#define SCHWAY__DISC_CCD_MIN_SECTORS UINT64_C(16)

enum schway__disc_ccd_section {
    SCHWAY__DISC_CCD_NO_SECTION = 0,
    SCHWAY__DISC_CCD_CLONECD,
    SCHWAY__DISC_CCD_DISC,
    SCHWAY__DISC_CCD_SESSION,
    SCHWAY__DISC_CCD_ENTRY,
    SCHWAY__DISC_CCD_TRACK
};

enum schway__disc_ccd_entry_field {
    SCHWAY__DISC_CCD_ENTRY_SESSION = 1u << 0,
    SCHWAY__DISC_CCD_ENTRY_POINT = 1u << 1,
    SCHWAY__DISC_CCD_ENTRY_ADR = 1u << 2,
    SCHWAY__DISC_CCD_ENTRY_CONTROL = 1u << 3,
    SCHWAY__DISC_CCD_ENTRY_TRACK_NO = 1u << 4,
    SCHWAY__DISC_CCD_ENTRY_AMIN = 1u << 5,
    SCHWAY__DISC_CCD_ENTRY_ASEC = 1u << 6,
    SCHWAY__DISC_CCD_ENTRY_AFRAME = 1u << 7,
    SCHWAY__DISC_CCD_ENTRY_ALBA = 1u << 8,
    SCHWAY__DISC_CCD_ENTRY_ZERO = 1u << 9,
    SCHWAY__DISC_CCD_ENTRY_PMIN = 1u << 10,
    SCHWAY__DISC_CCD_ENTRY_PSEC = 1u << 11,
    SCHWAY__DISC_CCD_ENTRY_PFRAME = 1u << 12,
    SCHWAY__DISC_CCD_ENTRY_PLBA = 1u << 13
};

typedef struct schway__disc_ccd_entry {
    uint64_t session;
    uint64_t point;
    uint64_t adr;
    uint64_t control;
    uint64_t track_no;
    uint64_t amin;
    uint64_t asec;
    uint64_t aframe;
    int64_t alba;
    uint64_t zero;
    uint64_t pmin;
    uint64_t psec;
    uint64_t pframe;
    uint64_t plba;
    uint32_t seen;
} schway__disc_ccd_entry;

typedef struct schway__disc_ccd_metadata {
    uint64_t version;
    uint64_t toc_entries;
    uint64_t sessions;
    uint64_t scrambled;
    uint64_t cdtext_length;
    uint64_t pregap_mode;
    uint64_t pregap_subc;
    uint32_t clone_seen;
    uint32_t disc_seen;
    uint32_t session_seen;
    uint32_t entry_count;
    uint32_t mode;
    uint64_t index0;
    uint64_t index1;
    uint32_t track_seen;
    uint8_t have_index0;
    schway__disc_ccd_entry entries[SCHWAY__DISC_CCD_ENTRY_COUNT];
} schway__disc_ccd_metadata;

struct schway__disc_ccd {
    FILE *img;
    uint64_t image_bytes;
    uint32_t sector_count;
    schway_toc toc;
};

static unsigned char schway__disc_ccd_ascii_lower(unsigned char value)
{
    if (value >= (unsigned char)'A' && value <= (unsigned char)'Z') {
        return (unsigned char)(value - (unsigned char)'A' + (unsigned char)'a');
    }
    return value;
}

static int schway__disc_ccd_name_is(const uint8_t *bytes, size_t size,
                                    const char *name)
{
    size_t i = 0u;
    while (i < size && name[i] != '\0' &&
           schway__disc_ccd_ascii_lower(bytes[i]) ==
               schway__disc_ccd_ascii_lower((unsigned char)name[i])) i++;
    return i == size && name[i] == '\0';
}

static void schway__disc_ccd_trim(const uint8_t **bytes, size_t *size)
{
    while (*size != 0u && ((*bytes)[0] == (uint8_t)' ' ||
                            (*bytes)[0] == (uint8_t)'\t')) {
        (*bytes)++;
        (*size)--;
    }
    while (*size != 0u && ((*bytes)[*size - 1u] == (uint8_t)' ' ||
                            (*bytes)[*size - 1u] == (uint8_t)'\t')) (*size)--;
}

static int schway__disc_ccd_parse_unsigned(const uint8_t *bytes, size_t size,
                                            unsigned int base, int require_hex,
                                            uint64_t *out)
{
    size_t first = 0u;
    uint64_t value = 0u;
    if (require_hex) {
        if (size < 3u || bytes[0] != (uint8_t)'0' ||
            (bytes[1] != (uint8_t)'x' && bytes[1] != (uint8_t)'X')) return 0;
        first = 2u;
        base = 16u;
    }
    if (first == size) return 0;
    for (size_t i = first; i < size; i++) {
        unsigned int digit;
        unsigned char c = bytes[i];
        if (c >= (unsigned char)'0' && c <= (unsigned char)'9') {
            digit = (unsigned int)(c - (unsigned char)'0');
        } else if (base == 16u && c >= (unsigned char)'a' && c <= (unsigned char)'f') {
            digit = 10u + (unsigned int)(c - (unsigned char)'a');
        } else if (base == 16u && c >= (unsigned char)'A' && c <= (unsigned char)'F') {
            digit = 10u + (unsigned int)(c - (unsigned char)'A');
        } else {
            return 0;
        }
        if (digit >= base || value > (UINT64_MAX - digit) / base) return 0;
        value = value * base + digit;
    }
    *out = value;
    return 1;
}

static int schway__disc_ccd_parse_signed(const uint8_t *bytes, size_t size,
                                          int64_t *out)
{
    uint64_t magnitude = 0u;
    size_t first = 0u;
    int negative = 0;
    uint64_t limit = (uint64_t)INT64_MAX;
    if (size != 0u && bytes[0] == (uint8_t)'-') {
        negative = 1;
        first = 1u;
        limit++;
    }
    if (first == size) return 0;
    for (size_t i = first; i < size; i++) {
        unsigned int digit;
        if (bytes[i] < (uint8_t)'0' || bytes[i] > (uint8_t)'9') return 0;
        digit = (unsigned int)(bytes[i] - (uint8_t)'0');
        if (magnitude > (limit - digit) / 10u) return 0;
        magnitude = magnitude * 10u + digit;
    }
    if (negative) {
        if (magnitude == (uint64_t)INT64_MAX + 1u) *out = INT64_MIN;
        else *out = -(int64_t)magnitude;
    } else {
        *out = (int64_t)magnitude;
    }
    return 1;
}

static int schway__disc_ccd_set_unsigned(
    const uint8_t *key, size_t key_size, const char *name,
    const uint8_t *value, size_t value_size, int hex,
    uint32_t bit, uint32_t *seen, uint64_t *field)
{
    uint64_t parsed;
    if (!schway__disc_ccd_name_is(key, key_size, name)) return 0;
    if ((*seen & bit) != 0u ||
        !schway__disc_ccd_parse_unsigned(value, value_size, hex != 0 ? 16u : 10u,
                                          hex, &parsed)) return -1;
    *field = parsed;
    *seen |= bit;
    return 1;
}

static int schway__disc_ccd_assign_entry(
    schway__disc_ccd_entry *entry, const uint8_t *key, size_t key_size,
    const uint8_t *value, size_t value_size)
{
    int result;
#define SCHWAY__CCD_SET_ENTRY(label, member, bit, hex) \
    do { \
        result = schway__disc_ccd_set_unsigned(key, key_size, label, value, value_size, \
                    hex, bit, &entry->seen, &entry->member); \
        if (result != 0) return result; \
    } while (0)
    SCHWAY__CCD_SET_ENTRY("Session", session, SCHWAY__DISC_CCD_ENTRY_SESSION, 0);
    SCHWAY__CCD_SET_ENTRY("Point", point, SCHWAY__DISC_CCD_ENTRY_POINT, 1);
    SCHWAY__CCD_SET_ENTRY("ADR", adr, SCHWAY__DISC_CCD_ENTRY_ADR, 1);
    SCHWAY__CCD_SET_ENTRY("Control", control, SCHWAY__DISC_CCD_ENTRY_CONTROL, 1);
    SCHWAY__CCD_SET_ENTRY("TrackNo", track_no, SCHWAY__DISC_CCD_ENTRY_TRACK_NO, 0);
    SCHWAY__CCD_SET_ENTRY("AMin", amin, SCHWAY__DISC_CCD_ENTRY_AMIN, 0);
    SCHWAY__CCD_SET_ENTRY("ASec", asec, SCHWAY__DISC_CCD_ENTRY_ASEC, 0);
    SCHWAY__CCD_SET_ENTRY("AFrame", aframe, SCHWAY__DISC_CCD_ENTRY_AFRAME, 0);
    SCHWAY__CCD_SET_ENTRY("Zero", zero, SCHWAY__DISC_CCD_ENTRY_ZERO, 0);
    SCHWAY__CCD_SET_ENTRY("PMin", pmin, SCHWAY__DISC_CCD_ENTRY_PMIN, 0);
    SCHWAY__CCD_SET_ENTRY("PSec", psec, SCHWAY__DISC_CCD_ENTRY_PSEC, 0);
    SCHWAY__CCD_SET_ENTRY("PFrame", pframe, SCHWAY__DISC_CCD_ENTRY_PFRAME, 0);
    SCHWAY__CCD_SET_ENTRY("PLBA", plba, SCHWAY__DISC_CCD_ENTRY_PLBA, 0);
#undef SCHWAY__CCD_SET_ENTRY
    if (schway__disc_ccd_name_is(key, key_size, "ALBA")) {
        if ((entry->seen & SCHWAY__DISC_CCD_ENTRY_ALBA) != 0u ||
            !schway__disc_ccd_parse_signed(value, value_size, &entry->alba)) return -1;
        entry->seen |= SCHWAY__DISC_CCD_ENTRY_ALBA;
        return 1;
    }
    return 0;
}

static int schway__disc_ccd_section_index(const uint8_t *bytes, size_t size,
                                           const char *prefix, uint64_t *index)
{
    size_t prefix_size = strlen(prefix);
    if (size <= prefix_size + 1u || bytes[size - 1u] != (uint8_t)']' ||
        !schway__disc_ccd_name_is(bytes, prefix_size, prefix)) return 0;
    return schway__disc_ccd_parse_unsigned(bytes + prefix_size,
                                            size - prefix_size - 1u, 10u, 0, index);
}

static int schway__disc_ccd_entry_time_matches(const schway__disc_ccd_entry *entry)
{
    uint64_t minutes;
    uint64_t seconds;
    uint64_t frames;
    uint64_t msf;
    if (entry->pmin > 99u || entry->psec >= 60u || entry->pframe >= 75u) return 0;
    if (!schway__media_mul_u64(entry->pmin, UINT64_C(4500), &minutes) ||
        !schway__media_mul_u64(entry->psec, UINT64_C(75), &seconds) ||
        !schway__media_add_u64(minutes, seconds, &msf) ||
        !schway__media_add_u64(msf, entry->pframe, &msf) || msf < 150u) return 0;
    frames = msf - 150u;
    return frames == entry->plba;
}

static schway_disc_image_status schway__disc_ccd_validate_metadata(
    const schway__disc_ccd_metadata *metadata, uint32_t *type,
    uint32_t *index0, uint32_t *index1, uint32_t *leadout)
{
    const uint64_t expected_points[SCHWAY__DISC_CCD_ENTRY_COUNT] = {
        UINT64_C(0xa0), UINT64_C(0xa1), UINT64_C(0xa2), UINT64_C(1)
    };
    uint64_t disc_type;
    if (metadata->clone_seen != 1u || metadata->version != 3u ||
        metadata->disc_seen != 15u || metadata->session_seen != 3u ||
        metadata->entry_count != SCHWAY__DISC_CCD_ENTRY_COUNT ||
        (metadata->track_seen & 5u) != 5u) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    if (metadata->toc_entries != SCHWAY__DISC_CCD_ENTRY_COUNT ||
        metadata->sessions != 1u || metadata->scrambled != 0u ||
        metadata->cdtext_length != 0u) return SCHWAY_DISC_IMAGE_UNSUPPORTED;
    if (metadata->pregap_mode != 0u || metadata->pregap_subc != 0u) {
        return SCHWAY_DISC_IMAGE_UNSUPPORTED;
    }
    if (metadata->mode != 1u && metadata->mode != 2u) return SCHWAY_DISC_IMAGE_UNSUPPORTED;
    disc_type = metadata->mode == 2u ? UINT64_C(32) : UINT64_C(0);
    for (size_t i = 0u; i < SCHWAY__DISC_CCD_ENTRY_COUNT; i++) {
        const schway__disc_ccd_entry *entry = &metadata->entries[i];
        if (entry->seen != SCHWAY__DISC_CCD_ENTRY_REQUIRED ||
            entry->session != 1u || entry->point != expected_points[i] ||
            entry->adr != 1u || entry->control != 4u ||
            entry->track_no != (i == 3u ? 1u : 0u) ||
            entry->amin != 0u || entry->asec != 0u || entry->aframe != 0u ||
            entry->alba != -150 || entry->zero != 0u ||
            !schway__disc_ccd_entry_time_matches(entry)) {
            return SCHWAY_DISC_IMAGE_BAD_IMAGE;
        }
    }
    if (metadata->entries[0].pmin != 1u ||
        metadata->entries[0].psec != disc_type || metadata->entries[0].pframe != 0u ||
        metadata->entries[1].pmin != 1u || metadata->entries[1].psec != 0u ||
        metadata->entries[1].pframe != 0u ||
        metadata->entries[2].plba == 0u || metadata->entries[2].plba > UINT32_MAX) {
        return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    }
    if (metadata->index1 > UINT32_MAX || metadata->entries[3].plba != metadata->index1 ||
        metadata->index1 >= metadata->entries[2].plba) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    if (metadata->have_index0 != 0u) {
        if (metadata->index0 > UINT32_MAX || metadata->index0 > metadata->index1) {
            return SCHWAY_DISC_IMAGE_BAD_IMAGE;
        }
    } else if (metadata->index1 != 0u) {
        return SCHWAY_DISC_IMAGE_UNSUPPORTED;
    }
    *type = metadata->mode == 1u ? SCHWAY_DISC_TRACK_MODE1 : SCHWAY_DISC_TRACK_MODE2;
    *index0 = metadata->have_index0 != 0u ? (uint32_t)metadata->index0 :
                                          (uint32_t)metadata->index1;
    *index1 = (uint32_t)metadata->index1;
    *leadout = (uint32_t)metadata->entries[2].plba;
    if (*index0 != 0u) return SCHWAY_DISC_IMAGE_UNSUPPORTED;
    return SCHWAY_DISC_IMAGE_OK;
}

static schway_disc_image_status schway__disc_ccd_parse_descriptor(
    const uint8_t *bytes, size_t size, schway__disc_ccd_metadata *metadata)
{
    enum schway__disc_ccd_section section = SCHWAY__DISC_CCD_NO_SECTION;
    uint32_t clone_version_seen = 0u;
    uint32_t disc_seen = 0u;
    uint32_t session_seen = 0u;
    uint32_t track_seen = 0u;
    uint32_t entry_index = 0u;
    size_t position = 0u;
    memset(metadata, 0, sizeof *metadata);
    while (position < size) {
        size_t start = position;
        size_t line_size;
        const uint8_t *line;
        const uint8_t *equal;
        const uint8_t *key;
        const uint8_t *value;
        size_t key_size;
        size_t value_size;
        while (position < size && bytes[position] != (uint8_t)'\r' &&
               bytes[position] != (uint8_t)'\n') position++;
        line = bytes + start;
        line_size = position - start;
        while (position < size && (bytes[position] == (uint8_t)'\r' ||
                                    bytes[position] == (uint8_t)'\n')) position++;
        schway__disc_ccd_trim(&line, &line_size);
        if (line_size == 0u || line[0] == (uint8_t)';') continue;
        for (size_t i = 0u; i < line_size; i++) {
            if (line[i] == (uint8_t)';') {
                line_size = i;
                break;
            }
        }
        schway__disc_ccd_trim(&line, &line_size);
        if (line_size == 0u) continue;
        if (line[0] == (uint8_t)'[') {
            uint64_t section_number;
            if (schway__disc_ccd_name_is(line, line_size, "[CloneCD]")) {
                if (section != SCHWAY__DISC_CCD_NO_SECTION || metadata->clone_seen != 0u) {
                    return SCHWAY_DISC_IMAGE_BAD_IMAGE;
                }
                metadata->clone_seen = 1u;
                section = SCHWAY__DISC_CCD_CLONECD;
            } else if (schway__disc_ccd_name_is(line, line_size, "[Disc]")) {
                if (metadata->clone_seen == 0u || disc_seen != 0u ||
                    section == SCHWAY__DISC_CCD_TRACK) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
                disc_seen = 1u;
                section = SCHWAY__DISC_CCD_DISC;
            } else if (schway__disc_ccd_name_is(line, line_size, "[Session 1]")) {
                if (disc_seen == 0u || session_seen != 0u || entry_index != 0u) {
                    return SCHWAY_DISC_IMAGE_BAD_IMAGE;
                }
                session_seen = 1u;
                section = SCHWAY__DISC_CCD_SESSION;
            } else if (schway__disc_ccd_section_index(line, line_size, "[Entry ",
                                                       &section_number)) {
                if (session_seen == 0u || section == SCHWAY__DISC_CCD_TRACK) {
                    return SCHWAY_DISC_IMAGE_BAD_IMAGE;
                }
                if (section_number >= SCHWAY__DISC_CCD_ENTRY_COUNT) {
                    return SCHWAY_DISC_IMAGE_UNSUPPORTED;
                }
                if (section_number != entry_index) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
                entry_index++;
                section = SCHWAY__DISC_CCD_ENTRY;
            } else if (schway__disc_ccd_name_is(line, line_size, "[TRACK 1]")) {
                if (entry_index != SCHWAY__DISC_CCD_ENTRY_COUNT || track_seen != 0u) {
                    return SCHWAY_DISC_IMAGE_BAD_IMAGE;
                }
                track_seen = 1u;
                section = SCHWAY__DISC_CCD_TRACK;
            } else if (schway__disc_ccd_section_index(line, line_size, "[TRACK ",
                                                       &section_number)) {
                return SCHWAY_DISC_IMAGE_UNSUPPORTED;
            } else {
                return metadata->clone_seen == 0u ? SCHWAY_DISC_IMAGE_BAD_IMAGE :
                                                    SCHWAY_DISC_IMAGE_UNSUPPORTED;
            }
            continue;
        }
        equal = (const uint8_t *)memchr(line, '=', line_size);
        if (equal == NULL) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
        key = line;
        key_size = (size_t)(equal - line);
        value = equal + 1;
        value_size = line_size - key_size - 1u;
        schway__disc_ccd_trim(&key, &key_size);
        schway__disc_ccd_trim(&value, &value_size);
        if (key_size == 0u || value_size == 0u) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
        if (section == SCHWAY__DISC_CCD_CLONECD) {
            uint64_t parsed;
            if (!schway__disc_ccd_name_is(key, key_size, "Version") ||
                clone_version_seen != 0u ||
                !schway__disc_ccd_parse_unsigned(value, value_size, 10u, 0, &parsed) ||
                parsed == 0u) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
            clone_version_seen = 1u;
            metadata->version = parsed;
            if (parsed != 3u) return SCHWAY_DISC_IMAGE_UNSUPPORTED;
        } else if (section == SCHWAY__DISC_CCD_DISC) {
            int result;
#define SCHWAY__CCD_SET_DISC(label, member, bit) \
    do { \
        result = schway__disc_ccd_set_unsigned(key, key_size, label, value, value_size, \
                    0, bit, &metadata->disc_seen, &metadata->member); \
        if (result != 0) { if (result < 0) return SCHWAY_DISC_IMAGE_BAD_IMAGE; else goto next_line; } \
    } while (0)
            SCHWAY__CCD_SET_DISC("TocEntries", toc_entries, 1u);
            SCHWAY__CCD_SET_DISC("Sessions", sessions, 2u);
            SCHWAY__CCD_SET_DISC("DataTracksScrambled", scrambled, 4u);
            SCHWAY__CCD_SET_DISC("CDTextLength", cdtext_length, 8u);
#undef SCHWAY__CCD_SET_DISC
            return SCHWAY_DISC_IMAGE_UNSUPPORTED;
        } else if (section == SCHWAY__DISC_CCD_SESSION) {
            int result;
#define SCHWAY__CCD_SET_SESSION(label, member, bit) \
    do { \
        result = schway__disc_ccd_set_unsigned(key, key_size, label, value, value_size, \
                    0, bit, &metadata->session_seen, &metadata->member); \
        if (result != 0) { if (result < 0) return SCHWAY_DISC_IMAGE_BAD_IMAGE; else goto next_line; } \
    } while (0)
            SCHWAY__CCD_SET_SESSION("PreGapMode", pregap_mode, 1u);
            SCHWAY__CCD_SET_SESSION("PreGapSubC", pregap_subc, 2u);
#undef SCHWAY__CCD_SET_SESSION
            return SCHWAY_DISC_IMAGE_UNSUPPORTED;
        } else if (section == SCHWAY__DISC_CCD_ENTRY) {
            int assigned = schway__disc_ccd_assign_entry(&metadata->entries[entry_index - 1u],
                                                          key, key_size, value, value_size);
            if (assigned < 0) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
            if (assigned == 0) return SCHWAY_DISC_IMAGE_UNSUPPORTED;
        } else if (section == SCHWAY__DISC_CCD_TRACK) {
            uint64_t parsed;
            if (schway__disc_ccd_name_is(key, key_size, "MODE")) {
                if ((metadata->track_seen & 1u) != 0u ||
                    !schway__disc_ccd_parse_unsigned(value, value_size, 10u, 0, &parsed) ||
                    parsed > UINT32_MAX) return SCHWAY_DISC_IMAGE_BAD_IMAGE;
                metadata->mode = (uint32_t)parsed;
                metadata->track_seen |= 1u;
            } else if (schway__disc_ccd_name_is(key, key_size, "INDEX 0")) {
                if ((metadata->track_seen & 2u) != 0u ||
                    !schway__disc_ccd_parse_unsigned(value, value_size, 10u, 0, &parsed)) {
                    return SCHWAY_DISC_IMAGE_BAD_IMAGE;
                }
                metadata->index0 = parsed;
                metadata->have_index0 = 1u;
                metadata->track_seen |= 2u;
            } else if (schway__disc_ccd_name_is(key, key_size, "INDEX 1")) {
                if ((metadata->track_seen & 4u) != 0u ||
                    !schway__disc_ccd_parse_unsigned(value, value_size, 10u, 0, &parsed)) {
                    return SCHWAY_DISC_IMAGE_BAD_IMAGE;
                }
                metadata->index1 = parsed;
                metadata->track_seen |= 4u;
            } else if (key_size >= 6u &&
                       schway__disc_ccd_name_is(key, 6u, "INDEX ")) {
                return SCHWAY_DISC_IMAGE_UNSUPPORTED;
            } else {
                return SCHWAY_DISC_IMAGE_UNSUPPORTED;
            }
        } else {
            return SCHWAY_DISC_IMAGE_BAD_IMAGE;
        }
next_line:
        ;
    }
    metadata->entry_count = entry_index;
    metadata->clone_seen = metadata->clone_seen != 0u && clone_version_seen != 0u ? 1u : 0u;
    if (disc_seen == 0u || session_seen == 0u || track_seen == 0u) {
        return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    }
    return SCHWAY_DISC_IMAGE_OK;
}

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

static int schway__disc_ccd_canonical_path(const char *path,
                                            char out[SCHWAY__DISC_CCD_PATH_CAP])
{
#if defined(_WIN32)
    HANDLE handle = CreateFileA(path, FILE_READ_ATTRIBUTES,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    DWORD length;
    if (handle == INVALID_HANDLE_VALUE) return 0;
    length = GetFinalPathNameByHandleA(handle, out, SCHWAY__DISC_CCD_PATH_CAP,
                                       FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    (void)CloseHandle(handle);
    return length != 0u && length < SCHWAY__DISC_CCD_PATH_CAP;
#elif defined(__wasi__)
    char cwd[SCHWAY__DISC_CCD_PATH_CAP];
    const char *relative = path;
    size_t cwd_size;
    char *resolved;
    size_t length;
    int contained;
    if (getcwd(cwd, sizeof cwd) == NULL) return 0;
    cwd_size = strlen(cwd);
    if (path[0] == '/') {
        if (cwd_size == 1u && cwd[0] == '/') {
            relative = path + 1;
        } else {
            if (strncmp(path, cwd, cwd_size) != 0 ||
                (path[cwd_size] != '/' && path[cwd_size] != '\0')) return 0;
            relative = path + cwd_size;
            while (*relative == '/') relative++;
        }
        if (*relative == '\0') relative = ".";
    }
    resolved = realpath(relative, NULL);
    if (resolved == NULL) return 0;
    contained = cwd_size == 1u && cwd[0] == '/' ? resolved[0] == '/' :
                strncmp(resolved, cwd, cwd_size) == 0 &&
                (resolved[cwd_size] == '/' || resolved[cwd_size] == '\0');
    length = strlen(resolved);
    if (!contained || length >= SCHWAY__DISC_CCD_PATH_CAP) {
        free(resolved);
        return 0;
    }
    memcpy(out, resolved, length + 1u);
    free(resolved);
    return 1;
#else
    char *resolved = realpath(path, NULL);
    size_t length;
    if (resolved == NULL) return 0;
    length = strlen(resolved);
    if (length >= SCHWAY__DISC_CCD_PATH_CAP) {
        free(resolved);
        return 0;
    }
    memcpy(out, resolved, length + 1u);
    free(resolved);
    return 1;
#endif
}

static int schway__disc_ccd_paths(const char *path, char root[SCHWAY__DISC_CCD_PATH_CAP],
                                   char img[SCHWAY__DISC_CCD_PATH_CAP],
                                   char sub[SCHWAY__DISC_CCD_PATH_CAP])
{
    char canonical[SCHWAY__DISC_CCD_PATH_CAP];
    const char *slash;
    const char *backslash;
    const char *dot;
    const char *base;
    size_t root_size;
    size_t stem_size;
    if (!schway__disc_ccd_canonical_path(path, canonical)) return 0;
    slash = strrchr(canonical, '/');
    backslash = strrchr(canonical, '\\');
    if (backslash != NULL && (slash == NULL || backslash > slash)) slash = backslash;
    base = slash == NULL ? canonical : slash + 1;
    dot = strrchr(canonical, '.');
    if (dot == NULL || dot < base) return 0;
    root_size = (size_t)(base - canonical);
    stem_size = (size_t)(dot - canonical);
    if (root_size == 0u || root_size >= SCHWAY__DISC_CCD_PATH_CAP ||
        stem_size + 5u >= SCHWAY__DISC_CCD_PATH_CAP) return 0;
    memcpy(root, canonical, root_size);
    root[root_size] = '\0';
    memcpy(img, canonical, stem_size);
    memcpy(img + stem_size, ".img", 5u);
    memcpy(sub, canonical, stem_size);
    memcpy(sub + stem_size, ".sub", 5u);
    return 1;
}

static int schway__disc_ccd_path_contained(const char *root, const char *candidate)
{
    size_t root_size = strlen(root);
    if (strlen(candidate) < root_size) return 0;
    for (size_t i = 0u; i < root_size; i++) {
        unsigned char left = (unsigned char)root[i];
        unsigned char right = (unsigned char)candidate[i];
#if defined(_WIN32)
        if (left == (unsigned char)'/') left = (unsigned char)'\\';
        if (right == (unsigned char)'/') right = (unsigned char)'\\';
        if (schway__disc_ccd_ascii_lower(left) != schway__disc_ccd_ascii_lower(right)) return 0;
#else
        if (left != right) return 0;
#endif
    }
    return 1;
}

static int schway__disc_ccd_regular_file(const char *path)
{
#if defined(_WIN32)
    DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0u;
#else
    struct stat info;
    return stat(path, &info) == 0 && S_ISREG(info.st_mode);
#endif
}

static int schway__disc_ccd_optional_path_state(const char *path)
{
#if defined(_WIN32)
    DWORD attributes = GetFileAttributesA(path);
    if (attributes != INVALID_FILE_ATTRIBUTES) return 1;
    DWORD error = GetLastError();
    return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ? 0 : -1;
#else
    struct stat info;
    if (lstat(path, &info) == 0) return 1;
    return errno == ENOENT ? 0 : -1;
#endif
}

static schway_disc_image_status schway__disc_ccd_error(
    schway_disc_image_error *error, schway_disc_image_status status,
    const char *message)
{
    if (error != NULL) {
        memset(error, 0, sizeof *error);
        error->size = (uint32_t)sizeof *error;
        error->status = (uint32_t)status;
        (void)snprintf(error->message, sizeof error->message, "%s", message);
    }
    return status;
}

schway_disc_image_status schway__disc_ccd_open(const char *path,
                                                schway__disc_ccd **out,
                                                schway_disc_image_error *error)
{
    FILE *descriptor = NULL;
    FILE *sub = NULL;
    uint8_t *bytes = NULL;
    uint64_t descriptor_bytes;
    size_t read_bytes;
    char root[SCHWAY__DISC_CCD_PATH_CAP];
    char img_candidate[SCHWAY__DISC_CCD_PATH_CAP];
    char sub_candidate[SCHWAY__DISC_CCD_PATH_CAP];
    char img_path[SCHWAY__DISC_CCD_PATH_CAP];
    char sub_path[SCHWAY__DISC_CCD_PATH_CAP];
    int sub_state;
    int img_state;
    int have_sub = 0;
    uint64_t img_bytes;
    uint64_t sectors;
    uint32_t track_type;
    uint32_t index0;
    uint32_t index1;
    uint32_t leadout;
    schway__disc_ccd_metadata metadata;
    schway_disc_image_status status;
    schway__disc_ccd *reader = NULL;

    if (out != NULL) *out = NULL;
    if (path == NULL || path[0] == '\0' || out == NULL) {
        return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_BAD_ARGUMENT,
                                      "CCD path and output are required");
    }
    if (error != NULL) {
        memset(error, 0, sizeof *error);
        error->size = (uint32_t)sizeof *error;
    }
    descriptor = schway__disc_ccd_open_file(path);
    if (descriptor == NULL) {
        return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_IO,
                                      "could not open CCD file");
    }
    if (!schway__disc_ccd_file_size(descriptor, &descriptor_bytes)) {
        (void)fclose(descriptor);
        return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_IO,
                                      "could not determine CCD file size");
    }
    if (descriptor_bytes > SCHWAY__DISC_CCD_DESCRIPTOR_MAX_BYTES) {
        (void)fclose(descriptor);
        return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                      "CCD descriptor exceeds the 65536-byte limit");
    }
    if (descriptor_bytes == 0u) {
        (void)fclose(descriptor);
        return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "CCD descriptor is empty");
    }
    bytes = (uint8_t *)malloc((size_t)descriptor_bytes);
    if (bytes == NULL) {
        (void)fclose(descriptor);
        return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_OUT_OF_MEMORY,
                                      "could not allocate CCD descriptor buffer");
    }
    read_bytes = fread(bytes, 1u, (size_t)descriptor_bytes, descriptor);
    if (fclose(descriptor) != 0 || read_bytes != (size_t)descriptor_bytes) {
        free(bytes);
        return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_IO,
                                      "could not read complete CCD descriptor");
    }
    for (size_t i = 0u; i < (size_t)descriptor_bytes; i++) {
        if (bytes[i] == 0u ||
            (bytes[i] < 0x20u && bytes[i] != (uint8_t)'\r' &&
             bytes[i] != (uint8_t)'\n' && bytes[i] != (uint8_t)'\t') ||
            bytes[i] > 0x7eu) {
            free(bytes);
            return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                          "CCD descriptor contains invalid text bytes");
        }
    }
    status = schway__disc_ccd_parse_descriptor(bytes, (size_t)descriptor_bytes, &metadata);
    free(bytes);
    if (status != SCHWAY_DISC_IMAGE_OK) {
        return schway__disc_ccd_error(error, status,
                                      status == SCHWAY_DISC_IMAGE_UNSUPPORTED ?
                                      "CCD metadata is outside the supported subset" :
                                      "CCD descriptor is malformed or inconsistent");
    }
    status = schway__disc_ccd_validate_metadata(&metadata, &track_type,
                                                 &index0, &index1, &leadout);
    if (status != SCHWAY_DISC_IMAGE_OK) {
        return schway__disc_ccd_error(error, status,
                                      "CCD metadata is outside the supported subset");
    }
    if (!schway__disc_ccd_paths(path, root, img_candidate, sub_candidate)) {
        return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_IO,
                                      "could not resolve CCD companion paths");
    }
    img_state = schway__disc_ccd_optional_path_state(img_candidate);
    if (img_state <= 0) {
        return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_IO,
                                      img_state == 0 ? "CCD IMG companion is missing" :
                                                       "could not inspect CCD IMG companion");
    }
    if (!schway__disc_ccd_canonical_path(img_candidate, img_path)) {
        return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_IO,
                                      "could not resolve CCD IMG companion");
    }
    if (!schway__disc_ccd_path_contained(root, img_path)) {
        return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "CCD IMG companion escapes its directory");
    }
    if (!schway__disc_ccd_regular_file(img_path)) {
        return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                      "CCD IMG companion is not a regular file");
    }
    sub_state = schway__disc_ccd_optional_path_state(sub_candidate);
    if (sub_state < 0) {
        return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_IO,
                                      "could not inspect CCD SUB companion");
    }
    if (sub_state > 0) {
        if (!schway__disc_ccd_canonical_path(sub_candidate, sub_path)) {
            return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_IO,
                                          "could not resolve CCD SUB companion");
        }
        if (!schway__disc_ccd_path_contained(root, sub_path)) {
            return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                          "CCD SUB companion escapes its directory");
        }
        if (!schway__disc_ccd_regular_file(sub_path)) {
            return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_IO,
                                          "CCD SUB companion is not a regular file");
        }
        have_sub = 1;
    }

    reader = (schway__disc_ccd *)calloc(1u, sizeof *reader);
    if (reader == NULL) {
        return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_OUT_OF_MEMORY,
                                      "could not allocate CCD reader");
    }
    reader->img = schway__disc_ccd_open_file(img_path);
    if (reader->img == NULL) {
        status = schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_IO,
                                        "could not open CCD IMG companion");
        goto fail;
    }
    if (!schway__disc_ccd_file_size(reader->img, &img_bytes)) {
        status = schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_IO,
                                        "could not determine CCD IMG size");
        goto fail;
    }
    if (img_bytes > SCHWAY__MEDIA_IMAGE_MAX_BYTES) {
        status = schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                        "CCD IMG exceeds the 1 GiB media limit");
        goto fail;
    }
    if (img_bytes == 0u || img_bytes % SCHWAY__DISC_CCD_SECTOR_BYTES != 0u) {
        status = schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                        "CCD IMG must contain complete 2352-byte sectors");
        goto fail;
    }
    sectors = img_bytes / SCHWAY__DISC_CCD_SECTOR_BYTES;
    if (sectors < SCHWAY__DISC_CCD_MIN_SECTORS || sectors > UINT32_MAX ||
        sectors != leadout) {
        status = schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                        "CCD leadout does not match the IMG sector extent");
        goto fail;
    }
    if (have_sub != 0) {
        uint64_t sub_bytes;
        sub = schway__disc_ccd_open_file(sub_path);
        if (sub == NULL) {
            status = schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_IO,
                                            "could not open CCD SUB companion");
            goto fail;
        }
        if (!schway__disc_ccd_file_size(sub, &sub_bytes)) {
            status = schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_IO,
                                            "could not determine CCD SUB size");
            goto fail;
        }
        (void)fclose(sub);
        sub = NULL;
        if (sub_bytes != 0u) {
            status = schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_UNSUPPORTED,
                                            "nonempty CCD SUB is not supported yet");
            goto fail;
        }
    }
    reader->image_bytes = img_bytes;
    reader->sector_count = (uint32_t)sectors;
    reader->toc.size = (uint32_t)sizeof reader->toc;
    reader->toc.track_count = 1u;
    reader->toc.tracks[0].size = (uint32_t)sizeof reader->toc.tracks[0];
    reader->toc.tracks[0].type = track_type;
    reader->toc.tracks[0].index0 = index0;
    reader->toc.tracks[0].index1 = index1;
    reader->toc.tracks[0].pregap = index1 - index0;
    reader->toc.tracks[0].leadout = (uint32_t)sectors;
    *out = reader;
    return schway__disc_ccd_error(error, SCHWAY_DISC_IMAGE_OK, "ok");

fail:
    schway__disc_ccd_close(reader);
    if (sub != NULL) (void)fclose(sub);
    return status;
}

void schway__disc_ccd_close(schway__disc_ccd *reader)
{
    if (reader == NULL) return;
    if (reader->img != NULL) (void)fclose(reader->img);
    free(reader);
}

schway_disc_image_status schway__disc_ccd_toc(const schway__disc_ccd *reader,
                                               schway_toc *out)
{
    if (reader == NULL || out == NULL || out->size != sizeof *out) {
        return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    }
    *out = reader->toc;
    return SCHWAY_DISC_IMAGE_OK;
}

uint32_t schway__disc_ccd_read_sector(void *user, uint32_t lba,
                                      uint8_t sector[2352], uint8_t subcode[96])
{
    schway__disc_ccd *reader = (schway__disc_ccd *)user;
    uint8_t raw[2352];
    uint64_t offset;
    if (reader == NULL || reader->img == NULL || sector == NULL ||
        lba >= reader->sector_count ||
        !schway__media_mul_u64(lba, SCHWAY__DISC_CCD_SECTOR_BYTES, &offset) ||
        offset > reader->image_bytes ||
        SCHWAY__DISC_CCD_SECTOR_BYTES > reader->image_bytes - offset) return 0u;
#if defined(_WIN32)
    if (_fseeki64(reader->img, (__int64)offset, SEEK_SET) != 0) return 0u;
#else
    if (fseeko(reader->img, (off_t)offset, SEEK_SET) != 0) return 0u;
#endif
    if (fread(raw, 1u, sizeof raw, reader->img) != sizeof raw) return 0u;
    memcpy(sector, raw, sizeof raw);
    if (subcode != NULL) memset(subcode, 0, 96u);
    return 1u;
}
