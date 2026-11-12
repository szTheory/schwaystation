/* Read bounded ISO9660 metadata for runner identity reporting. This code is
   host-only; it never submits sectors to the emulated console. */
#include "disc_probe.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sha1.h"

#define SCHWAY__ISO_SECTOR_SIZE 2048u
#define SCHWAY__ISO_MAX_BYTES 1073741824u
#define SCHWAY__ISO_DESCRIPTOR_LIMIT 64u
#define SCHWAY__ISO_DIRECTORY_MAX 65536u
#define SCHWAY__ISO_RECORD_LIMIT 4096u
#define SCHWAY__ISO_PATH_COMPONENTS 16u
#define SCHWAY__ISO_EXE_MAX 0x200800u
#define SCHWAY__SYSTEM_CNF_MAX 2047u
#define SCHWAY__CUE_MAX_BYTES 65536u
#define SCHWAY__CUE_NAME_MAX 255u
#define SCHWAY__CUE_TRACK_LIMIT 99u

static FILE *disc_probe_open_readonly(const char *path)
{
#if defined(_MSC_VER)
    FILE *file = NULL;
    if (fopen_s(&file, path, "rb") != 0) return NULL;
    return file;
#else
    return fopen(path, "rb");
#endif
}

typedef struct schway__iso_source {
    FILE *file;
    FILE *track1_file;
    uint64_t file_size;
    uint64_t data_offset;
    uint64_t data_bytes;
    uint64_t track1_offset;
    uint64_t track1_bytes;
    uint32_t sector_bytes;
    uint32_t user_offset;
    uint32_t sector_count;
} schway__iso_source;

typedef struct schway__cue_file {
    char name[SCHWAY__CUE_NAME_MAX + 1u];
    uint64_t size;
} schway__cue_file;

typedef struct schway__cue_track {
    size_t file_index;
    uint32_t sector_bytes;
    uint32_t user_offset;
    uint32_t index00;
    uint32_t index01;
    uint32_t last_index_frame;
    uint64_t start_offset;
    uint8_t last_index_number;
    int audio;
    int have_index;
    int have_index00;
    int have_index01;
} schway__cue_track;

typedef struct schway__cue_manifest {
    schway__cue_file files[SCHWAY__CUE_TRACK_LIMIT];
    schway__cue_track tracks[SCHWAY__CUE_TRACK_LIMIT];
    size_t file_count;
    size_t track_count;
    size_t index_count;
} schway__cue_manifest;

typedef struct schway__iso_record {
    uint32_t extent;
    uint32_t size;
    uint8_t flags;
    const uint8_t *name;
    size_t name_size;
} schway__iso_record;

typedef enum schway__iso_find_status {
    SCHWAY__ISO_FIND_OK = 0,
    SCHWAY__ISO_FIND_MISSING = 1,
    SCHWAY__ISO_FIND_BAD = 2,
    SCHWAY__ISO_FIND_MEMORY = 3
} schway__iso_find_status;

static uint16_t schway__iso_u16_le(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint16_t schway__iso_u16_be(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

static uint32_t schway__iso_u32_le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint32_t schway__iso_u32_be(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static unsigned char schway__iso_ascii_upper(unsigned char c)
{
    if (c >= (unsigned char)'a' && c <= (unsigned char)'z') {
        return (unsigned char)(c - (unsigned char)'a' + (unsigned char)'A');
    }
    return c;
}

static int schway__iso_both_u16(const uint8_t *p, uint16_t *value)
{
    uint16_t little = schway__iso_u16_le(p);
    if (little != schway__iso_u16_be(p + 2u)) {
        return 0;
    }
    *value = little;
    return 1;
}

static int schway__iso_both_u32(const uint8_t *p, uint32_t *value)
{
    uint32_t little = schway__iso_u32_le(p);
    if (little != schway__iso_u32_be(p + 4u)) {
        return 0;
    }
    *value = little;
    return 1;
}

static int schway__iso_read_at(const schway__iso_source *source, uint64_t offset,
                               void *out, size_t size)
{
    uint64_t logical_size = (uint64_t)source->sector_count * SCHWAY__ISO_SECTOR_SIZE;
    uint8_t *destination = (uint8_t *)out;
    size_t remaining = size;
    if (offset > logical_size || (uint64_t)size > logical_size - offset) return 0;
    while (remaining != 0u) {
        uint64_t sector = offset / SCHWAY__ISO_SECTOR_SIZE;
        size_t within = (size_t)(offset % SCHWAY__ISO_SECTOR_SIZE);
        size_t chunk = SCHWAY__ISO_SECTOR_SIZE - within;
        uint64_t physical;
        if (chunk > remaining) chunk = remaining;
        if (sector > (UINT64_MAX - source->data_offset) / source->sector_bytes) return 0;
        physical = source->data_offset + sector * source->sector_bytes;
        if (physical > UINT64_MAX - source->user_offset - within) return 0;
        physical += source->user_offset + within;
        if (physical > source->file_size || (uint64_t)chunk > source->file_size - physical ||
            physical > (uint64_t)LONG_MAX ||
            fseek(source->file, (long)physical, SEEK_SET) != 0 ||
            fread(destination, 1u, chunk, source->file) != chunk) {
            return 0;
        }
        destination += chunk;
        remaining -= chunk;
        offset += chunk;
    }
    return 1;
}

static int schway__iso_extent_ok(uint32_t extent, uint32_t size,
                                 uint32_t volume_sectors)
{
    uint64_t sectors = ((uint64_t)size + SCHWAY__ISO_SECTOR_SIZE - 1u) /
                       SCHWAY__ISO_SECTOR_SIZE;
    return (uint64_t)extent <= volume_sectors &&
           sectors <= (uint64_t)volume_sectors - extent;
}

static int schway__iso_record_read(const uint8_t *bytes, size_t available,
                                   uint32_t volume_sectors,
                                   schway__iso_record *out)
{
    uint8_t record_size = bytes[0];
    uint8_t name_size;
    size_t minimum;
    uint16_t sequence;
    if (record_size < 34u || (size_t)record_size > available ||
        bytes[1] != 0u || bytes[26] != 0u || bytes[27] != 0u || (bytes[25] & 0x80u) != 0u) {
        return 0;
    }
    name_size = bytes[32];
    minimum = 33u + (size_t)name_size + ((name_size & 1u) == 0u ? 1u : 0u);
    if (name_size == 0u || minimum > record_size ||
        !schway__iso_both_u32(bytes + 2u, &out->extent) ||
        !schway__iso_both_u32(bytes + 10u, &out->size) ||
        !schway__iso_both_u16(bytes + 28u, &sequence) || sequence == 0u ||
        !schway__iso_extent_ok(out->extent, out->size, volume_sectors)) {
        return 0;
    }
    out->flags = bytes[25];
    out->name = bytes + 33u;
    out->name_size = name_size;
    return 1;
}

static size_t schway__iso_name_size(const uint8_t *name, size_t size)
{
    for (size_t i = 0u; i < size; i++) {
        if (name[i] == (uint8_t)';') {
            size_t digits = size - i - 1u;
            if (digits == 0u) {
                return size;
            }
            for (size_t j = i + 1u; j < size; j++) {
                if (name[j] < (uint8_t)'0' || name[j] > (uint8_t)'9') {
                    return size;
                }
            }
            return i;
        }
    }
    return size;
}

static int schway__iso_ascii_equal(const uint8_t *left, size_t left_size,
                                   const char *right)
{
    size_t canonical = schway__iso_name_size(left, left_size);
    size_t right_size = strlen(right);
    if (canonical != right_size) {
        return 0;
    }
    for (size_t i = 0u; i < canonical; i++) {
        if (schway__iso_ascii_upper(left[i]) != schway__iso_ascii_upper((unsigned char)right[i])) {
            return 0;
        }
    }
    return 1;
}

static schway__iso_find_status schway__iso_find_in_directory(
    const schway__iso_source *source, uint32_t volume_sectors,
    const schway__iso_record *directory,
    const char *wanted, schway__iso_record *found)
{
    uint8_t *bytes;
    size_t offset = 0u;
    size_t records = 0u;
    if ((directory->flags & 2u) == 0u || directory->size == 0u ||
        directory->size > SCHWAY__ISO_DIRECTORY_MAX) {
        return SCHWAY__ISO_FIND_BAD;
    }
    bytes = (uint8_t *)malloc(directory->size);
    if (bytes == NULL) {
        return SCHWAY__ISO_FIND_MEMORY;
    }
    if (!schway__iso_read_at(source, (uint64_t)directory->extent * SCHWAY__ISO_SECTOR_SIZE,
                             bytes, directory->size)) {
        free(bytes);
        return SCHWAY__ISO_FIND_BAD;
    }
    while (offset < directory->size) {
        size_t sector_remaining = SCHWAY__ISO_SECTOR_SIZE - (offset % SCHWAY__ISO_SECTOR_SIZE);
        uint8_t record_size = bytes[offset];
        schway__iso_record record;
        if (record_size == 0u) {
            size_t skip = sector_remaining;
            if (skip > directory->size - offset) {
                offset = directory->size;
            } else {
                offset += skip;
            }
            continue;
        }
        records++;
        if (records > SCHWAY__ISO_RECORD_LIMIT || record_size > sector_remaining ||
            !schway__iso_record_read(bytes + offset, directory->size - offset,
                                     volume_sectors, &record)) {
            free(bytes);
            return SCHWAY__ISO_FIND_BAD;
        }
        if (record.name_size > 1u && schway__iso_ascii_equal(record.name, record.name_size, wanted)) {
            *found = record;
            free(bytes);
            return SCHWAY__ISO_FIND_OK;
        }
        offset += record_size;
    }
    free(bytes);
    return SCHWAY__ISO_FIND_MISSING;
}

static int schway__iso_safe_component(const char *bytes, size_t size)
{
    if (size == 0u || size > 63u || (size == 1u && bytes[0] == '.') ||
        (size == 2u && bytes[0] == '.' && bytes[1] == '.')) {
        return 0;
    }
    for (size_t i = 0u; i < size; i++) {
        unsigned char c = (unsigned char)bytes[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.' || c == ';')) {
            return 0;
        }
    }
    if (bytes[size - 1u] == ';') {
        return 0;
    }
    return 1;
}

static int schway__iso_parse_system_cnf(const uint8_t *bytes, size_t size,
                                         char components[SCHWAY__ISO_PATH_COMPONENTS][64],
                                         size_t *component_count)
{
    size_t line = 0u;
    const char *value = NULL;
    size_t value_size = 0u;
    *component_count = 0u;
    for (size_t i = 0u; i < size; i++) {
        uint8_t c = bytes[i];
        if ((c < 0x20u && c != (uint8_t)'\r' && c != (uint8_t)'\n' && c != (uint8_t)'\t') || c > 0x7Eu) {
            return -1;
        }
    }
    while (line < size) {
        size_t end = line;
        size_t first;
        size_t last;
        while (end < size && bytes[end] != (uint8_t)'\r' && bytes[end] != (uint8_t)'\n') {
            end++;
        }
        first = line;
        last = end;
        while (first < last && (bytes[first] == (uint8_t)' ' || bytes[first] == (uint8_t)'\t')) first++;
        while (last > first && (bytes[last - 1u] == (uint8_t)' ' || bytes[last - 1u] == (uint8_t)'\t')) last--;
        if (last - first >= 4u && schway__iso_ascii_upper(bytes[first]) == 'B' &&
            schway__iso_ascii_upper(bytes[first + 1u]) == 'O' &&
            schway__iso_ascii_upper(bytes[first + 2u]) == 'O' &&
            schway__iso_ascii_upper(bytes[first + 3u]) == 'T') {
            size_t p = first + 4u;
            while (p < last && (bytes[p] == (uint8_t)' ' || bytes[p] == (uint8_t)'\t')) p++;
            if (p < last && bytes[p] == (uint8_t)'=') {
                p++;
                while (p < last && (bytes[p] == (uint8_t)' ' || bytes[p] == (uint8_t)'\t')) p++;
                if (value != NULL || p == last) return -1;
                value = (const char *)bytes + p;
                value_size = last - p;
                for (size_t arg = 0u; arg < value_size; arg++) {
                    if (value[arg] == ' ' || value[arg] == '\t') {
                        value_size = arg;
                        break;
                    }
                }
            }
        }
        line = end;
        while (line < size && (bytes[line] == (uint8_t)'\r' || bytes[line] == (uint8_t)'\n')) line++;
    }
    if (value == NULL || value_size < 7u || schway__iso_ascii_upper((unsigned char)value[0]) != 'C' ||
        schway__iso_ascii_upper((unsigned char)value[1]) != 'D' || schway__iso_ascii_upper((unsigned char)value[2]) != 'R' ||
        schway__iso_ascii_upper((unsigned char)value[3]) != 'O' || schway__iso_ascii_upper((unsigned char)value[4]) != 'M' ||
        value[5] != ':') {
        return -1;
    }
    size_t pos = 6u;
    if (value[pos] == '/' || value[pos] == '\\') pos++;
    while (pos < value_size) {
        size_t end = pos;
        size_t component_size;
        while (end < value_size && value[end] != '/' && value[end] != '\\') end++;
        component_size = schway__iso_name_size((const uint8_t *)value + pos, end - pos);
        if (!schway__iso_safe_component(value + pos, component_size) ||
            *component_count >= SCHWAY__ISO_PATH_COMPONENTS) {
            return -1;
        }
        memcpy(components[*component_count], value + pos, component_size);
        components[*component_count][component_size] = '\0';
        (*component_count)++;
        pos = end + 1u;
        if (end == value_size) break;
    }
    return *component_count == 0u ? -1 : 1;
}

static schway__iso_find_status schway__iso_resolve_boot(
    const schway__iso_source *source, uint32_t volume_sectors,
    const schway__iso_record *root,
    char components[SCHWAY__ISO_PATH_COMPONENTS][64], size_t component_count,
    schway__iso_record *boot)
{
    schway__iso_record directory = *root;
    for (size_t i = 0u; i < component_count; i++) {
        schway__iso_record child;
        schway__iso_find_status status = schway__iso_find_in_directory(
            source, volume_sectors, &directory, components[i], &child);
        if (status != SCHWAY__ISO_FIND_OK) return status;
        if (i + 1u < component_count) {
            if ((child.flags & 2u) == 0u) return SCHWAY__ISO_FIND_BAD;
            directory = child;
        } else {
            if ((child.flags & 2u) != 0u || child.size == 0u || child.size > SCHWAY__ISO_EXE_MAX) {
                return SCHWAY__ISO_FIND_BAD;
            }
            *boot = child;
        }
    }
    return SCHWAY__ISO_FIND_OK;
}

static int schway__iso_make_identity(const char *name, char serial[16])
{
    size_t size = strlen(name);
    serial[0] = '\0';
    if (size != 11u || name[4] != '_' || name[8] != '.') return 0;
    for (size_t i = 0u; i < 4u; i++) {
        if (!((name[i] >= 'A' && name[i] <= 'Z') || (name[i] >= 'a' && name[i] <= 'z'))) return 0;
    }
    for (size_t i = 5u; i < 8u; i++) {
        if (name[i] < '0' || name[i] > '9') return 0;
    }
    for (size_t i = 9u; i < 11u; i++) {
        if (name[i] < '0' || name[i] > '9') return 0;
    }
    (void)snprintf(serial, 16u, "%.4s-%.3s%.2s", name, name + 5u, name + 9u);
    for (size_t i = 0u; serial[i] != '\0'; i++) {
        serial[i] = (char)schway__iso_ascii_upper((unsigned char)serial[i]);
    }
    return 1;
}

static schway_host_disc_probe_status schway__iso_read_exe(
    const schway__iso_source *source, const schway__iso_record *record,
    schway_host_disc_probe *result)
{
    result->exe_bytes = (uint8_t *)malloc(record->size);
    if (result->exe_bytes == NULL) return SCHWAY_HOST_DISC_PROBE_OUT_OF_MEMORY;
    if (!schway__iso_read_at(source, (uint64_t)record->extent * SCHWAY__ISO_SECTOR_SIZE,
                             result->exe_bytes, record->size)) {
        free(result->exe_bytes);
        result->exe_bytes = NULL;
        return SCHWAY_HOST_DISC_PROBE_IO;
    }
    result->exe_size = record->size;
    return SCHWAY_HOST_DISC_PROBE_OK;
}

static schway_host_disc_probe_status schway__iso_process(
    const schway__iso_source *source, schway_host_disc_probe *result)
{
    uint8_t descriptor[SCHWAY__ISO_SECTOR_SIZE];
    uint8_t pvd[SCHWAY__ISO_SECTOR_SIZE];
    uint32_t volume_sectors = 0u;
    uint32_t actual_sectors = source->sector_count;
    uint32_t descriptor_sector = 16u;
    int have_pvd = 0;
    int have_terminator = 0;
    schway__iso_record root;
    if (!schway__iso_read_at(source, 16u * SCHWAY__ISO_SECTOR_SIZE, descriptor, sizeof descriptor) ||
        memcmp(descriptor + 1u, "CD001", 5u) != 0) {
        return SCHWAY_HOST_DISC_PROBE_NOT_ISO;
    }
    for (uint32_t i = 0u; i < SCHWAY__ISO_DESCRIPTOR_LIMIT; i++, descriptor_sector++) {
        if (!schway__iso_read_at(source, (uint64_t)descriptor_sector * SCHWAY__ISO_SECTOR_SIZE,
                                 descriptor, sizeof descriptor)) {
            return SCHWAY_HOST_DISC_PROBE_BAD_ISO;
        }
        if (memcmp(descriptor + 1u, "CD001", 5u) != 0 || descriptor[6] != 1u) {
            return SCHWAY_HOST_DISC_PROBE_BAD_ISO;
        }
        if (descriptor[0] == 1u && !have_pvd) {
            memcpy(pvd, descriptor, sizeof pvd);
            have_pvd = 1;
        }
        if (descriptor[0] == 255u) {
            have_terminator = 1;
            break;
        }
    }
    if (!have_pvd || !have_terminator || !schway__iso_both_u32(pvd + 80u, &volume_sectors) ||
        volume_sectors < 18u || volume_sectors > actual_sectors) {
        return SCHWAY_HOST_DISC_PROBE_BAD_ISO;
    }
    uint16_t block_size;
    if (!schway__iso_both_u16(pvd + 128u, &block_size) || block_size != SCHWAY__ISO_SECTOR_SIZE ||
        !schway__iso_record_read(pvd + 156u, sizeof pvd - 156u, volume_sectors, &root) ||
        (root.flags & 2u) == 0u || root.size > SCHWAY__ISO_DIRECTORY_MAX) {
        return SCHWAY_HOST_DISC_PROBE_BAD_ISO;
    }

    schway__iso_record system_cnf;
    schway__iso_find_status found = schway__iso_find_in_directory(
        source, volume_sectors, &root, "SYSTEM.CNF", &system_cnf);
    schway__iso_record boot;
    char components[SCHWAY__ISO_PATH_COMPONENTS][64];
    size_t component_count = 0u;
    if (found == SCHWAY__ISO_FIND_MEMORY) return SCHWAY_HOST_DISC_PROBE_OUT_OF_MEMORY;
    if (found == SCHWAY__ISO_FIND_BAD) return SCHWAY_HOST_DISC_PROBE_BAD_ISO;
    if (found == SCHWAY__ISO_FIND_MISSING) {
        found = schway__iso_find_in_directory(source, volume_sectors, &root, "PSX.EXE", &boot);
        if (found == SCHWAY__ISO_FIND_MISSING) return SCHWAY_HOST_DISC_PROBE_NO_BOOT_EXE;
        if (found == SCHWAY__ISO_FIND_MEMORY) return SCHWAY_HOST_DISC_PROBE_OUT_OF_MEMORY;
        if (found != SCHWAY__ISO_FIND_OK || (boot.flags & 2u) != 0u || boot.size == 0u ||
            boot.size > SCHWAY__ISO_EXE_MAX) return SCHWAY_HOST_DISC_PROBE_BAD_ISO;
        memcpy(result->boot_name, "PSX.EXE", sizeof "PSX.EXE");
    } else {
        if ((system_cnf.flags & 2u) != 0u || system_cnf.size > SCHWAY__SYSTEM_CNF_MAX ||
            system_cnf.size == 0u) return SCHWAY_HOST_DISC_PROBE_BAD_SYSTEM_CNF;
        uint8_t system_bytes[SCHWAY__SYSTEM_CNF_MAX];
        if (!schway__iso_read_at(source, (uint64_t)system_cnf.extent * SCHWAY__ISO_SECTOR_SIZE,
                                 system_bytes, system_cnf.size)) return SCHWAY_HOST_DISC_PROBE_BAD_ISO;
        int parse_status = schway__iso_parse_system_cnf(system_bytes, system_cnf.size,
                                                        components, &component_count);
        if (parse_status < 0) return SCHWAY_HOST_DISC_PROBE_BAD_SYSTEM_CNF;
        for (size_t i = 0u; i + 1u < component_count; i++) {
            if (strlen(components[i]) > 63u) return SCHWAY_HOST_DISC_PROBE_BAD_BOOT_PATH;
        }
        found = schway__iso_resolve_boot(source, volume_sectors, &root, components,
                                         component_count, &boot);
        if (found == SCHWAY__ISO_FIND_MEMORY) return SCHWAY_HOST_DISC_PROBE_OUT_OF_MEMORY;
        if (found == SCHWAY__ISO_FIND_MISSING) return SCHWAY_HOST_DISC_PROBE_NO_BOOT_EXE;
        if (found == SCHWAY__ISO_FIND_BAD) return SCHWAY_HOST_DISC_PROBE_BAD_ISO;
        if (found != SCHWAY__ISO_FIND_OK) return SCHWAY_HOST_DISC_PROBE_BAD_BOOT_PATH;
        const char *last = components[component_count - 1u];
        size_t last_size = strlen(last);
        size_t canonical_size = schway__iso_name_size((const uint8_t *)last, last_size);
        if (canonical_size == 0u || canonical_size > 63u) return SCHWAY_HOST_DISC_PROBE_BAD_BOOT_PATH;
        memcpy(result->boot_name, last, canonical_size);
        result->boot_name[canonical_size] = '\0';
    }
    (void)schway__iso_make_identity(result->boot_name, result->serial);
    return schway__iso_read_exe(source, &boot, result);
}

static void schway__cue_skip_space(char **cursor)
{
    while (**cursor == ' ' || **cursor == '\t') (*cursor)++;
}

static int schway__cue_token(char **cursor, char *out, size_t capacity)
{
    size_t size = 0u;
    int quoted = 0;
    schway__cue_skip_space(cursor);
    if (**cursor == '"') {
        quoted = 1;
        (*cursor)++;
    }
    while (**cursor != '\0' && (quoted ? **cursor != '"' : (**cursor != ' ' && **cursor != '\t'))) {
        if (size + 1u >= capacity) return 0;
        out[size++] = **cursor;
        (*cursor)++;
    }
    if (size == 0u || (quoted && **cursor != '"')) return 0;
    if (quoted) (*cursor)++;
    if (**cursor != '\0' && **cursor != ' ' && **cursor != '\t') return 0;
    out[size] = '\0';
    return 1;
}

static int schway__cue_word_is(const char *word, const char *expected)
{
    size_t i = 0u;
    while (word[i] != '\0' && expected[i] != '\0' &&
           schway__iso_ascii_upper((unsigned char)word[i]) ==
               schway__iso_ascii_upper((unsigned char)expected[i])) {
        i++;
    }
    return word[i] == '\0' && expected[i] == '\0';
}

static int schway__cue_end_of_line(char *cursor)
{
    schway__cue_skip_space(&cursor);
    return *cursor == '\0';
}

static int schway__cue_two_digits(const char *text, unsigned *value)
{
    if (strlen(text) != 2u || text[0] < '0' || text[0] > '9' ||
        text[1] < '0' || text[1] > '9') return 0;
    *value = (unsigned)(text[0] - '0') * 10u + (unsigned)(text[1] - '0');
    return 1;
}

static int schway__cue_msf(const char *text, uint32_t *frames)
{
    unsigned minute;
    unsigned second;
    unsigned frame;
    char minute_text[3] = {0};
    char second_text[3] = {0};
    char frame_text[3] = {0};
    if (strlen(text) != 8u || text[2] != ':' || text[5] != ':') return 0;
    memcpy(minute_text, text, 2u);
    memcpy(second_text, text + 3u, 2u);
    memcpy(frame_text, text + 6u, 2u);
    if (!schway__cue_two_digits(minute_text, &minute) ||
        !schway__cue_two_digits(second_text, &second) ||
        !schway__cue_two_digits(frame_text, &frame) || second > 59u || frame > 74u) {
        return 0;
    }
    *frames = (uint32_t)((minute * 60u + second) * 75u + frame);
    return 1;
}

static int schway__cue_safe_name(const char *name)
{
    size_t size = strlen(name);
    size_t component_start = 0u;
    if (size == 0u || size > SCHWAY__CUE_NAME_MAX || name[0] == '/' || name[0] == '\\' ||
        (size >= 2u && name[1] == ':')) return 0;
    for (size_t i = 0u; i <= size; i++) {
        unsigned char c = (unsigned char)name[i];
        if (i < size && (c < 0x20u || c > 0x7Eu || c == ':' || c == '"' ||
                         c == '<' || c == '>' || c == '|' || c == '?' || c == '*')) return 0;
        if (i == size || name[i] == '/' || name[i] == '\\') {
            size_t component_size = i - component_start;
            if (component_size == 0u ||
                (component_size == 2u && name[component_start] == '.' &&
                 name[component_start + 1u] == '.')) return 0;
            component_start = i + 1u;
        }
    }
    return 1;
}

static char *schway__cue_join_path(const char *cue_path, const char *name)
{
    size_t name_size = strlen(name);
    const char *slash = strrchr(cue_path, '/');
    const char *backslash = strrchr(cue_path, '\\');
    const char *last = slash;
    size_t directory_size;
    char *joined;
    if (backslash != NULL && (last == NULL || backslash > last)) last = backslash;
    directory_size = last == NULL ? 0u : (size_t)(last - cue_path + 1);
    if (directory_size > SIZE_MAX - name_size - 1u) return NULL;
    joined = (char *)malloc(directory_size + name_size + 1u);
    if (joined == NULL) return NULL;
    memcpy(joined, cue_path, directory_size);
    for (size_t i = 0u; i < name_size; i++) {
        joined[directory_size + i] = name[i] == '\\' ? '/' : name[i];
    }
    joined[directory_size + name_size] = '\0';
    return joined;
}

static schway_host_disc_probe_status schway__cue_parse(const char *path,
                                                        schway__cue_manifest *manifest)
{
    FILE *file = disc_probe_open_readonly(path);
    long end = -1L;
    char *bytes;
    size_t size;
    size_t offset = 0u;
    size_t active_file = SIZE_MAX;
    if (file == NULL) return SCHWAY_HOST_DISC_PROBE_CUE_IO;
    if (fseek(file, 0L, SEEK_END) != 0 || (end = ftell(file)) < 0L) {
        (void)fclose(file);
        return SCHWAY_HOST_DISC_PROBE_CUE_IO;
    }
    if ((uint64_t)(unsigned long)end > SCHWAY__CUE_MAX_BYTES) {
        (void)fclose(file);
        return SCHWAY_HOST_DISC_PROBE_LIMIT;
    }
    size = (size_t)end;
    if (size == 0u || fseek(file, 0L, SEEK_SET) != 0) {
        (void)fclose(file);
        return SCHWAY_HOST_DISC_PROBE_BAD_CUE;
    }
    bytes = (char *)malloc(size + 1u);
    if (bytes == NULL) {
        (void)fclose(file);
        return SCHWAY_HOST_DISC_PROBE_OUT_OF_MEMORY;
    }
    int read_failed = fread(bytes, 1u, size, file) != size;
    if (fclose(file) != 0) read_failed = 1;
    if (read_failed) {
        free(bytes);
        return SCHWAY_HOST_DISC_PROBE_CUE_IO;
    }
    bytes[size] = '\0';
    for (size_t i = 0u; i < size; i++) {
        unsigned char c = (unsigned char)bytes[i];
        if (c == 0u || (c < 0x20u && c != '\r' && c != '\n' && c != '\t')) {
            free(bytes);
            return SCHWAY_HOST_DISC_PROBE_BAD_CUE;
        }
    }
    while (offset < size) {
        size_t end_line = offset;
        char *line;
        char *cursor;
        char keyword[32];
        while (end_line < size && bytes[end_line] != '\n') end_line++;
        bytes[end_line] = '\0';
        if (end_line > offset && bytes[end_line - 1u] == '\r') bytes[end_line - 1u] = '\0';
        line = bytes + offset;
        cursor = line;
        schway__cue_skip_space(&cursor);
        offset = end_line + 1u;
        if (*cursor == '\0') continue;
        if (!schway__cue_token(&cursor, keyword, sizeof keyword)) {
            free(bytes);
            return SCHWAY_HOST_DISC_PROBE_BAD_CUE;
        }
        if (schway__cue_word_is(keyword, "FILE")) {
            char name[SCHWAY__CUE_NAME_MAX + 1u];
            char type[16];
            if (manifest->file_count >= SCHWAY__CUE_TRACK_LIMIT ||
                !schway__cue_token(&cursor, name, sizeof name) ||
                !schway__cue_token(&cursor, type, sizeof type) ||
                !schway__cue_word_is(type, "BINARY") || !schway__cue_end_of_line(cursor)) {
                free(bytes);
                return SCHWAY_HOST_DISC_PROBE_BAD_CUE;
            }
            if (!schway__cue_safe_name(name)) {
                free(bytes);
                return SCHWAY_HOST_DISC_PROBE_CUE_PATH;
            }
            schway__cue_file *entry = &manifest->files[manifest->file_count];
            memcpy(entry->name, name, strlen(name) + 1u);
            active_file = manifest->file_count++;
        } else if (schway__cue_word_is(keyword, "TRACK")) {
            char number_text[8];
            char mode[24];
            unsigned number;
            if (active_file == SIZE_MAX || manifest->track_count >= SCHWAY__CUE_TRACK_LIMIT ||
                (manifest->track_count != 0u &&
                 !manifest->tracks[manifest->track_count - 1u].have_index01) ||
                !schway__cue_token(&cursor, number_text, sizeof number_text) ||
                !schway__cue_two_digits(number_text, &number) ||
                number != manifest->track_count + 1u ||
                !schway__cue_token(&cursor, mode, sizeof mode) || !schway__cue_end_of_line(cursor)) {
                free(bytes);
                return SCHWAY_HOST_DISC_PROBE_BAD_CUE;
            }
            schway__cue_track *track = &manifest->tracks[manifest->track_count++];
            memset(track, 0, sizeof *track);
            track->file_index = active_file;
            if (schway__cue_word_is(mode, "AUDIO")) {
                track->audio = 1;
                track->sector_bytes = 2352u;
            } else if (schway__cue_word_is(mode, "MODE1/2048")) {
                track->sector_bytes = 2048u;
            } else if (schway__cue_word_is(mode, "MODE1/2352")) {
                track->sector_bytes = 2352u;
                track->user_offset = 16u;
            } else if (schway__cue_word_is(mode, "MODE2/2336")) {
                track->sector_bytes = 2336u;
                track->user_offset = 8u;
            } else if (schway__cue_word_is(mode, "MODE2/2352")) {
                track->sector_bytes = 2352u;
                track->user_offset = 24u;
            } else {
                free(bytes);
                return SCHWAY_HOST_DISC_PROBE_CUE_MODE;
            }
        } else if (schway__cue_word_is(keyword, "INDEX")) {
            char number_text[8];
            char time_text[16];
            unsigned number;
            uint32_t frame;
            schway__cue_track *track;
            if (manifest->track_count == 0u || manifest->index_count >= SCHWAY__CUE_TRACK_LIMIT ||
                !schway__cue_token(&cursor, number_text, sizeof number_text) ||
                !schway__cue_two_digits(number_text, &number) || number > 99u ||
                !schway__cue_token(&cursor, time_text, sizeof time_text) ||
                !schway__cue_msf(time_text, &frame) || !schway__cue_end_of_line(cursor)) {
                free(bytes);
                return SCHWAY_HOST_DISC_PROBE_BAD_CUE;
            }
            track = &manifest->tracks[manifest->track_count - 1u];
            if ((track->have_index &&
                 (number <= track->last_index_number || frame < track->last_index_frame)) ||
                (!track->have_index && number > 1u) ||
                (number == 0u && track->have_index01)) {
                free(bytes);
                return SCHWAY_HOST_DISC_PROBE_BAD_CUE;
            }
            if (number == 0u) {
                if (track->have_index00) {
                    free(bytes);
                    return SCHWAY_HOST_DISC_PROBE_BAD_CUE;
                }
                track->index00 = frame;
                track->have_index00 = 1;
            } else if (number == 1u) {
                track->index01 = frame;
                track->have_index01 = 1;
            }
            track->have_index = 1;
            track->last_index_number = (uint8_t)number;
            track->last_index_frame = frame;
            manifest->index_count++;
        } else if (schway__cue_word_is(keyword, "REM") ||
                   schway__cue_word_is(keyword, "TITLE") ||
                   schway__cue_word_is(keyword, "PERFORMER") ||
                   schway__cue_word_is(keyword, "CATALOG") ||
                   schway__cue_word_is(keyword, "ISRC") ||
                   schway__cue_word_is(keyword, "FLAGS")) {
            continue;
        } else if (schway__cue_word_is(keyword, "PREGAP") ||
                   schway__cue_word_is(keyword, "POSTGAP")) {
            char time_text[16];
            uint32_t frame;
            if (manifest->track_count == 0u || !schway__cue_token(&cursor, time_text, sizeof time_text) ||
                !schway__cue_msf(time_text, &frame) || !schway__cue_end_of_line(cursor)) {
                free(bytes);
                return SCHWAY_HOST_DISC_PROBE_BAD_CUE;
            }
            (void)frame;
        } else {
            free(bytes);
            return SCHWAY_HOST_DISC_PROBE_BAD_CUE;
        }
    }
    free(bytes);
    if (manifest->file_count == 0u || manifest->track_count == 0u ||
        !manifest->tracks[manifest->track_count - 1u].have_index01) {
        return SCHWAY_HOST_DISC_PROBE_BAD_CUE;
    }
    for (size_t i = 0u; i < manifest->track_count; i++) {
        if (manifest->tracks[i].have_index00 &&
            manifest->tracks[i].index00 > manifest->tracks[i].index01) {
            return SCHWAY_HOST_DISC_PROBE_BAD_CUE;
        }
    }
    return SCHWAY_HOST_DISC_PROBE_OK;
}

static schway_host_disc_probe_status schway__cue_file_sizes(
    const char *cue_path, schway__cue_manifest *manifest)
{
    uint64_t total = 0u;
    for (size_t i = 0u; i < manifest->file_count; i++) {
        char *path = schway__cue_join_path(cue_path, manifest->files[i].name);
        FILE *file;
        long end;
        int failed;
        if (path == NULL) return SCHWAY_HOST_DISC_PROBE_OUT_OF_MEMORY;
        file = disc_probe_open_readonly(path);
        free(path);
        if (file == NULL) return SCHWAY_HOST_DISC_PROBE_CUE_IO;
        failed = fseek(file, 0L, SEEK_END) != 0;
        end = failed ? -1L : ftell(file);
        if (end < 0L) failed = 1;
        if (fclose(file) != 0) failed = 1;
        if (failed) return SCHWAY_HOST_DISC_PROBE_CUE_IO;
        manifest->files[i].size = (uint64_t)(unsigned long)end;
        int duplicate = 0;
        for (size_t previous = 0u; previous < i; previous++) {
            if (strcmp(manifest->files[previous].name, manifest->files[i].name) == 0) {
                duplicate = 1;
                break;
            }
        }
        if (!duplicate) {
            if (manifest->files[i].size > SCHWAY__ISO_MAX_BYTES - total) {
                return SCHWAY_HOST_DISC_PROBE_LIMIT;
            }
            total += manifest->files[i].size;
        }
    }
    return SCHWAY_HOST_DISC_PROBE_OK;
}

static schway_host_disc_probe_status schway__cue_source(
    const char *cue_path, schway__iso_source *source)
{
    schway__cue_manifest manifest;
    schway_host_disc_probe_status status;
    size_t selected = SIZE_MAX;
    uint64_t selected_bytes = 0u;
    uint64_t track1_bytes = 0u;
    FILE *file;
    long end;
    memset(&manifest, 0, sizeof manifest);
    status = schway__cue_parse(cue_path, &manifest);
    if (status != SCHWAY_HOST_DISC_PROBE_OK) return status;
    status = schway__cue_file_sizes(cue_path, &manifest);
    if (status != SCHWAY_HOST_DISC_PROBE_OK) return status;
    for (size_t i = 0u; i < manifest.track_count; i++) {
        schway__cue_track *track = &manifest.tracks[i];
        uint64_t span;
        if (i != 0u && track->file_index == manifest.tracks[i - 1u].file_index) {
            schway__cue_track *previous = &manifest.tracks[i - 1u];
            uint32_t start_frame = track->have_index00 ? track->index00 : track->index01;
            if (start_frame < previous->index01 ||
                (uint64_t)(start_frame - previous->index01) >
                    (UINT64_MAX - previous->start_offset) / previous->sector_bytes) {
                return SCHWAY_HOST_DISC_PROBE_BAD_CUE;
            }
            track->start_offset = previous->start_offset +
                (uint64_t)(start_frame - previous->index01) * previous->sector_bytes;
            if (track->have_index00) {
                uint32_t pregap = track->index01 - track->index00;
                if ((uint64_t)pregap > (UINT64_MAX - track->start_offset) / track->sector_bytes) {
                    return SCHWAY_HOST_DISC_PROBE_BAD_CUE;
                }
                track->start_offset += (uint64_t)pregap * track->sector_bytes;
            }
        } else {
            track->start_offset = (uint64_t)track->index01 * track->sector_bytes;
        }
        if (track->start_offset > manifest.files[track->file_index].size) {
            return SCHWAY_HOST_DISC_PROBE_CUE_IO;
        }
        if (i + 1u < manifest.track_count &&
            track->file_index == manifest.tracks[i + 1u].file_index) {
            schway__cue_track *next = &manifest.tracks[i + 1u];
            uint32_t end_frame = next->have_index00 ? next->index00 : next->index01;
            if (end_frame < track->index01 ||
                (uint64_t)(end_frame - track->index01) >
                    (UINT64_MAX - track->start_offset) / track->sector_bytes) {
                return SCHWAY_HOST_DISC_PROBE_BAD_CUE;
            }
            span = (uint64_t)(end_frame - track->index01) * track->sector_bytes;
        } else {
            span = manifest.files[track->file_index].size - track->start_offset;
        }
        if (span == 0u || span > manifest.files[track->file_index].size - track->start_offset ||
            span % track->sector_bytes != 0u) {
            return SCHWAY_HOST_DISC_PROBE_BAD_CUE;
        }
        if (i == 0u) track1_bytes = span;
        if (!track->audio && selected == SIZE_MAX) {
            selected = i;
            selected_bytes = span;
        }
    }
    if (selected == SIZE_MAX) return SCHWAY_HOST_DISC_PROBE_CUE_NO_DATA;
    schway__cue_track *track = &manifest.tracks[selected];
    char *path = schway__cue_join_path(cue_path, manifest.files[track->file_index].name);
    if (path == NULL) return SCHWAY_HOST_DISC_PROBE_OUT_OF_MEMORY;
    file = disc_probe_open_readonly(path);
    free(path);
    if (file == NULL) return SCHWAY_HOST_DISC_PROBE_CUE_IO;
    if (fseek(file, 0L, SEEK_END) != 0 || (end = ftell(file)) < 0L ||
        fseek(file, 0L, SEEK_SET) != 0) {
        (void)fclose(file);
        return SCHWAY_HOST_DISC_PROBE_CUE_IO;
    }
    source->file = file;
    source->file_size = (uint64_t)(unsigned long)end;
    source->data_offset = track->start_offset;
    source->data_bytes = selected_bytes;
    source->sector_bytes = track->sector_bytes;
    source->user_offset = track->user_offset;
    source->sector_count = (uint32_t)(selected_bytes / track->sector_bytes);
    schway__cue_track *first = &manifest.tracks[0];
    path = schway__cue_join_path(cue_path, manifest.files[first->file_index].name);
    if (path == NULL) {
        (void)fclose(source->file);
        source->file = NULL;
        return SCHWAY_HOST_DISC_PROBE_OUT_OF_MEMORY;
    }
    source->track1_file = disc_probe_open_readonly(path);
    free(path);
    if (source->track1_file == NULL) {
        (void)fclose(source->file);
        source->file = NULL;
        return SCHWAY_HOST_DISC_PROBE_CUE_IO;
    }
    source->track1_offset = first->start_offset;
    source->track1_bytes = track1_bytes;
    return SCHWAY_HOST_DISC_PROBE_OK;
}

static int schway__sha1_file_range(FILE *file, uint64_t offset, uint64_t size,
                                    uint8_t digest[20])
{
    uint8_t buffer[8192];
    schway__sha1 state;
    if (file == NULL || offset > (uint64_t)LONG_MAX || fseek(file, (long)offset, SEEK_SET) != 0) {
        return 0;
    }
    schway__sha1_init(&state);
    while (size != 0u) {
        size_t count = size > sizeof buffer ? sizeof buffer : (size_t)size;
        if (fread(buffer, 1u, count, file) != count) return 0;
        schway__sha1_update(&state, buffer, count);
        size -= count;
    }
    schway__sha1_final(&state, digest);
    return 1;
}

static int schway__path_is_cue(const char *path)
{
    const char *dot = strrchr(path, '.');
    const char *slash = strrchr(path, '/');
    const char *backslash = strrchr(path, '\\');
    const char *last = slash;
    if (backslash != NULL && (last == NULL || backslash > last)) last = backslash;
    return dot != NULL && (last == NULL || dot > last) &&
           schway__cue_word_is(dot, ".cue");
}

static int schway__path_is_unsupported(const char *path)
{
    const char *dot = strrchr(path, '.');
    const char *slash = strrchr(path, '/');
    const char *backslash = strrchr(path, '\\');
    const char *last = slash;
    if (backslash != NULL && (last == NULL || backslash > last)) last = backslash;
    if (dot == NULL || (last != NULL && dot < last)) return 0;
    return schway__cue_word_is(dot, ".chd") || schway__cue_word_is(dot, ".pbp") ||
           schway__cue_word_is(dot, ".ecm") || schway__cue_word_is(dot, ".ccd");
}

schway_host_disc_probe_status schway__disc_probe_image(const char *path,
                                                        schway_host_disc_probe *out)
{
    schway__iso_source source;
    schway_host_disc_probe candidate;
    schway_host_disc_probe_status status;
    FILE *file;
    long end;
    int is_cue;
    if (path == NULL || out == NULL) return SCHWAY_HOST_DISC_PROBE_IO;
    if (schway__path_is_unsupported(path)) return SCHWAY_HOST_DISC_PROBE_UNSUPPORTED;
    memset(&source, 0, sizeof source);
    is_cue = schway__path_is_cue(path);
    if (is_cue) {
        status = schway__cue_source(path, &source);
        if (status != SCHWAY_HOST_DISC_PROBE_OK) return status;
    } else {
        file = disc_probe_open_readonly(path);
        if (file == NULL) return SCHWAY_HOST_DISC_PROBE_NOT_ISO;
        if (fseek(file, 0L, SEEK_END) != 0 || (end = ftell(file)) < 0L) {
            (void)fclose(file);
            return SCHWAY_HOST_DISC_PROBE_NOT_ISO;
        }
        if ((uint64_t)(unsigned long)end > SCHWAY__ISO_MAX_BYTES ||
            (uint64_t)(unsigned long)end % SCHWAY__ISO_SECTOR_SIZE != 0u) {
            (void)fclose(file);
            return SCHWAY_HOST_DISC_PROBE_LIMIT;
        }
        if (fseek(file, 0L, SEEK_SET) != 0) {
            (void)fclose(file);
            return SCHWAY_HOST_DISC_PROBE_IO;
        }
        source.file = file;
        source.track1_file = file;
        source.file_size = (uint64_t)(unsigned long)end;
        source.data_bytes = source.file_size;
        source.track1_bytes = source.file_size;
        source.sector_bytes = SCHWAY__ISO_SECTOR_SIZE;
        source.sector_count = (uint32_t)(source.file_size / SCHWAY__ISO_SECTOR_SIZE);
    }
    memset(&candidate, 0, sizeof candidate);
    status = schway__iso_process(&source, &candidate);
    if (status == SCHWAY_HOST_DISC_PROBE_OK &&
        !schway__sha1_file_range(source.track1_file, source.track1_offset,
                                  source.track1_bytes, candidate.track1_sha1)) {
        status = SCHWAY_HOST_DISC_PROBE_IO;
    }
    if (source.track1_file != source.file && fclose(source.track1_file) != 0 &&
        status == SCHWAY_HOST_DISC_PROBE_OK) {
        status = SCHWAY_HOST_DISC_PROBE_IO;
    }
    if (fclose(source.file) != 0 && status == SCHWAY_HOST_DISC_PROBE_OK) {
        free(candidate.exe_bytes);
        return SCHWAY_HOST_DISC_PROBE_IO;
    }
    if (status == SCHWAY_HOST_DISC_PROBE_OK) *out = candidate;
    else free(candidate.exe_bytes);
    return status;
}

void schway__disc_probe_dispose(schway_host_disc_probe *probe)
{
    if (probe == NULL) return;
    free(probe->exe_bytes);
    memset(probe, 0, sizeof *probe);
}

const char *schway__disc_probe_status_string(schway_host_disc_probe_status status)
{
    switch (status) {
    case SCHWAY_HOST_DISC_PROBE_NOT_ISO: return "not a raw ISO9660 image";
    case SCHWAY_HOST_DISC_PROBE_IO: return "cannot read disc image";
    case SCHWAY_HOST_DISC_PROBE_LIMIT: return "disc image exceeds probe limits";
    case SCHWAY_HOST_DISC_PROBE_BAD_ISO: return "invalid ISO9660 metadata";
    case SCHWAY_HOST_DISC_PROBE_BAD_SYSTEM_CNF: return "invalid SYSTEM.CNF boot metadata";
    case SCHWAY_HOST_DISC_PROBE_NO_BOOT_EXE: return "disc boot executable not found";
    case SCHWAY_HOST_DISC_PROBE_BAD_BOOT_PATH: return "unsafe or invalid boot executable path";
    case SCHWAY_HOST_DISC_PROBE_OUT_OF_MEMORY: return "out of memory";
    case SCHWAY_HOST_DISC_PROBE_BAD_CUE: return "invalid CUE track metadata";
    case SCHWAY_HOST_DISC_PROBE_CUE_PATH: return "unsafe or escaping CUE FILE path";
    case SCHWAY_HOST_DISC_PROBE_CUE_IO: return "cannot read CUE track file";
    case SCHWAY_HOST_DISC_PROBE_CUE_MODE: return "unsupported CUE track mode";
    case SCHWAY_HOST_DISC_PROBE_CUE_NO_DATA: return "CUE has no data track";
    case SCHWAY_HOST_DISC_PROBE_UNSUPPORTED: return "unsupported disc image media";
    case SCHWAY_HOST_DISC_PROBE_OK: return "ok";
    }
    return "invalid disc probe status";
}
