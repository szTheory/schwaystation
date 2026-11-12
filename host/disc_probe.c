/* Read bounded ISO9660 metadata for runner identity reporting. This code is
   host-only; it never submits sectors to the emulated console. */
#include "disc_probe.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCHWAY__ISO_SECTOR_SIZE 2048u
#define SCHWAY__ISO_MAX_BYTES 1073741824u
#define SCHWAY__ISO_DESCRIPTOR_LIMIT 64u
#define SCHWAY__ISO_DIRECTORY_MAX 65536u
#define SCHWAY__ISO_RECORD_LIMIT 4096u
#define SCHWAY__ISO_PATH_COMPONENTS 16u
#define SCHWAY__ISO_EXE_MAX 0x200800u
#define SCHWAY__SYSTEM_CNF_MAX 2047u

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

static int schway__iso_read_at(FILE *file, uint64_t offset, void *out, size_t size)
{
    if (offset > (uint64_t)LONG_MAX || fseek(file, (long)offset, SEEK_SET) != 0) {
        return 0;
    }
    return fread(out, 1u, size, file) == size;
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
    FILE *file, uint32_t volume_sectors, const schway__iso_record *directory,
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
    if (!schway__iso_read_at(file, (uint64_t)directory->extent * SCHWAY__ISO_SECTOR_SIZE,
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
    FILE *file, uint32_t volume_sectors, const schway__iso_record *root,
    char components[SCHWAY__ISO_PATH_COMPONENTS][64], size_t component_count,
    schway__iso_record *boot)
{
    schway__iso_record directory = *root;
    for (size_t i = 0u; i < component_count; i++) {
        schway__iso_record child;
        schway__iso_find_status status = schway__iso_find_in_directory(
            file, volume_sectors, &directory, components[i], &child);
        if (status != SCHWAY__ISO_FIND_OK) return status;
        if (i + 1u < component_count) {
            if ((child.flags & 2u) == 0u) return SCHWAY__ISO_FIND_MISSING;
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
    FILE *file, const schway__iso_record *record,
    schway_host_disc_probe *result)
{
    result->exe_bytes = (uint8_t *)malloc(record->size);
    if (result->exe_bytes == NULL) return SCHWAY_HOST_DISC_PROBE_OUT_OF_MEMORY;
    if (!schway__iso_read_at(file, (uint64_t)record->extent * SCHWAY__ISO_SECTOR_SIZE,
                             result->exe_bytes, record->size)) {
        free(result->exe_bytes);
        result->exe_bytes = NULL;
        return SCHWAY_HOST_DISC_PROBE_IO;
    }
    result->exe_size = record->size;
    return SCHWAY_HOST_DISC_PROBE_OK;
}

static schway_host_disc_probe_status schway__iso_process(
    FILE *file, uint64_t file_size, schway_host_disc_probe *result)
{
    uint8_t descriptor[SCHWAY__ISO_SECTOR_SIZE];
    uint8_t pvd[SCHWAY__ISO_SECTOR_SIZE];
    uint32_t volume_sectors = 0u;
    uint32_t actual_sectors;
    uint32_t descriptor_sector = 16u;
    int have_pvd = 0;
    int have_terminator = 0;
    schway__iso_record root;
    if (!schway__iso_read_at(file, 16u * SCHWAY__ISO_SECTOR_SIZE, descriptor, sizeof descriptor) ||
        memcmp(descriptor + 1u, "CD001", 5u) != 0) {
        return SCHWAY_HOST_DISC_PROBE_NOT_ISO;
    }
    if (file_size > SCHWAY__ISO_MAX_BYTES || file_size % SCHWAY__ISO_SECTOR_SIZE != 0u) {
        return SCHWAY_HOST_DISC_PROBE_LIMIT;
    }
    actual_sectors = (uint32_t)(file_size / SCHWAY__ISO_SECTOR_SIZE);
    for (uint32_t i = 0u; i < SCHWAY__ISO_DESCRIPTOR_LIMIT; i++, descriptor_sector++) {
        if (!schway__iso_read_at(file, (uint64_t)descriptor_sector * SCHWAY__ISO_SECTOR_SIZE,
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
        file, volume_sectors, &root, "SYSTEM.CNF", &system_cnf);
    schway__iso_record boot;
    char components[SCHWAY__ISO_PATH_COMPONENTS][64];
    size_t component_count = 0u;
    if (found == SCHWAY__ISO_FIND_MEMORY) return SCHWAY_HOST_DISC_PROBE_OUT_OF_MEMORY;
    if (found == SCHWAY__ISO_FIND_BAD) return SCHWAY_HOST_DISC_PROBE_BAD_ISO;
    if (found == SCHWAY__ISO_FIND_MISSING) {
        found = schway__iso_find_in_directory(file, volume_sectors, &root, "PSX.EXE", &boot);
        if (found == SCHWAY__ISO_FIND_MISSING) return SCHWAY_HOST_DISC_PROBE_NO_BOOT_EXE;
        if (found == SCHWAY__ISO_FIND_MEMORY) return SCHWAY_HOST_DISC_PROBE_OUT_OF_MEMORY;
        if (found != SCHWAY__ISO_FIND_OK || (boot.flags & 2u) != 0u || boot.size == 0u ||
            boot.size > SCHWAY__ISO_EXE_MAX) return SCHWAY_HOST_DISC_PROBE_BAD_ISO;
        memcpy(result->boot_name, "PSX.EXE", sizeof "PSX.EXE");
    } else {
        if ((system_cnf.flags & 2u) != 0u || system_cnf.size > SCHWAY__SYSTEM_CNF_MAX ||
            system_cnf.size == 0u) return SCHWAY_HOST_DISC_PROBE_BAD_SYSTEM_CNF;
        uint8_t system_bytes[SCHWAY__SYSTEM_CNF_MAX];
        if (!schway__iso_read_at(file, (uint64_t)system_cnf.extent * SCHWAY__ISO_SECTOR_SIZE,
                                 system_bytes, system_cnf.size)) return SCHWAY_HOST_DISC_PROBE_BAD_ISO;
        int parse_status = schway__iso_parse_system_cnf(system_bytes, system_cnf.size,
                                                        components, &component_count);
        if (parse_status < 0) return SCHWAY_HOST_DISC_PROBE_BAD_SYSTEM_CNF;
        for (size_t i = 0u; i + 1u < component_count; i++) {
            if (strlen(components[i]) > 63u) return SCHWAY_HOST_DISC_PROBE_BAD_BOOT_PATH;
        }
        found = schway__iso_resolve_boot(file, volume_sectors, &root, components,
                                         component_count, &boot);
        if (found == SCHWAY__ISO_FIND_MEMORY) return SCHWAY_HOST_DISC_PROBE_OUT_OF_MEMORY;
        if (found == SCHWAY__ISO_FIND_MISSING) return SCHWAY_HOST_DISC_PROBE_NO_BOOT_EXE;
        if (found != SCHWAY__ISO_FIND_OK) return SCHWAY_HOST_DISC_PROBE_BAD_BOOT_PATH;
        const char *last = components[component_count - 1u];
        size_t last_size = strlen(last);
        size_t canonical_size = schway__iso_name_size((const uint8_t *)last, last_size);
        if (canonical_size == 0u || canonical_size > 63u) return SCHWAY_HOST_DISC_PROBE_BAD_BOOT_PATH;
        memcpy(result->boot_name, last, canonical_size);
        result->boot_name[canonical_size] = '\0';
    }
    (void)schway__iso_make_identity(result->boot_name, result->serial);
    return schway__iso_read_exe(file, &boot, result);
}

schway_host_disc_probe_status schway__disc_probe_iso(const char *path,
                                                      schway_host_disc_probe *out)
{
    FILE *file;
    long end;
    schway_host_disc_probe candidate;
    schway_host_disc_probe_status status;
    if (path == NULL || out == NULL) return SCHWAY_HOST_DISC_PROBE_IO;
    file = fopen(path, "rb");
    if (file == NULL) return SCHWAY_HOST_DISC_PROBE_NOT_ISO;
    if (fseek(file, 0L, SEEK_END) != 0 || (end = ftell(file)) < 0L) {
        (void)fclose(file);
        return SCHWAY_HOST_DISC_PROBE_NOT_ISO;
    }
    memset(&candidate, 0, sizeof candidate);
    status = schway__iso_process(file, (uint64_t)(unsigned long)end, &candidate);
    if (fclose(file) != 0 && status == SCHWAY_HOST_DISC_PROBE_OK) {
        free(candidate.exe_bytes);
        return SCHWAY_HOST_DISC_PROBE_IO;
    }
    if (status == SCHWAY_HOST_DISC_PROBE_OK) *out = candidate;
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
    case SCHWAY_HOST_DISC_PROBE_OK: return "ok";
    }
    return "invalid disc probe status";
}
