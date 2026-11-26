/* Shared, host-only provider for bounded PlayStation disc images. */
#ifndef SCHWAYSTATION_HOST_DISC_IMAGE_H
#define SCHWAYSTATION_HOST_DISC_IMAGE_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "schwaystation.h"
#include "disc_patch.h"

typedef enum schway_disc_image_status {
    SCHWAY_DISC_IMAGE_OK = 0,
    SCHWAY_DISC_IMAGE_BAD_ARGUMENT = 1,
    SCHWAY_DISC_IMAGE_IO = 2,
    SCHWAY_DISC_IMAGE_BAD_CUE = 3,
    SCHWAY_DISC_IMAGE_BAD_IMAGE = 4,
    SCHWAY_DISC_IMAGE_LIMIT = 5,
    SCHWAY_DISC_IMAGE_OUT_OF_MEMORY = 6,
    SCHWAY_DISC_IMAGE_UNSUPPORTED = 7,
    SCHWAY_DISC_IMAGE_BAD_M3U = 8,
    SCHWAY_DISC_IMAGE_BAD_SYSCNF = 9
} schway_disc_image_status;

typedef struct schway_disc_image_error {
    uint32_t size;
    uint32_t status;
    uint32_t line;
    char message[96];
} schway_disc_image_error;

typedef struct schway_disc_image schway_disc_image;

#define SCHWAY_DISC_IMAGE_PLAYLIST_LIMIT 32u
#define SCHWAY_DISC_SYSTEM_CNF_MAX_BYTES 2047u
#define SCHWAY_DISC_SYSTEM_CNF_COMPONENT_LIMIT 16u
#define SCHWAY_DISC_SYSTEM_CNF_COMPONENT_BYTES 64u

#define SCHWAY_DISC_SYSTEM_CNF_HAVE_TCB 1u
#define SCHWAY_DISC_SYSTEM_CNF_HAVE_EVENT 2u
#define SCHWAY_DISC_SYSTEM_CNF_HAVE_STACK 4u

typedef struct schway_disc_system_cnf {
    uint32_t size;
    uint32_t component_count;
    uint32_t present;
    uint32_t tcb;
    uint32_t event;
    uint32_t stack;
    char components[SCHWAY_DISC_SYSTEM_CNF_COMPONENT_LIMIT]
                 [SCHWAY_DISC_SYSTEM_CNF_COMPONENT_BYTES];
    char boot_name[SCHWAY_DISC_SYSTEM_CNF_COMPONENT_BYTES];
} schway_disc_system_cnf;

static inline unsigned char schway__disc_image_cnf_upper(unsigned char value)
{
    if (value >= (unsigned char)'a' && value <= (unsigned char)'z') {
        return (unsigned char)(value - (unsigned char)'a' + (unsigned char)'A');
    }
    return value;
}

static inline int schway__disc_image_cnf_key_is(const uint8_t *bytes, size_t size,
                                                const char *key)
{
    size_t i = 0u;
    while (i < size && key[i] != '\0' &&
           schway__disc_image_cnf_upper(bytes[i]) == (unsigned char)key[i]) i++;
    return i == size && key[i] == '\0';
}

static inline int schway__disc_image_cnf_number(const uint8_t *bytes, size_t size,
                                                unsigned int default_base,
                                                uint32_t *out)
{
    unsigned int base = default_base;
    uint32_t value = 0u;
    size_t first = 0u;
    size_t last = size;
    if (size >= 2u && bytes[0] == (uint8_t)'0' &&
        (bytes[1] == (uint8_t)'x' || bytes[1] == (uint8_t)'X')) {
        base = 16u;
        first = 2u;
    }
    if (last > first && (bytes[last - 1u] == (uint8_t)'h' ||
                         bytes[last - 1u] == (uint8_t)'H')) {
        base = 16u;
        last--;
    }
    if (first == last) return 0;
    for (size_t i = first; i < last; i++) {
        unsigned char c = bytes[i];
        unsigned int digit;
        if (c >= (unsigned char)'0' && c <= (unsigned char)'9') {
            digit = (unsigned int)(c - (unsigned char)'0');
        } else if (base == 16u && c >= (unsigned char)'A' && c <= (unsigned char)'F') {
            digit = 10u + (unsigned int)(c - (unsigned char)'A');
        } else if (base == 16u && c >= (unsigned char)'a' && c <= (unsigned char)'f') {
            digit = 10u + (unsigned int)(c - (unsigned char)'a');
        } else {
            return 0;
        }
        if (digit >= base || value > (UINT32_MAX - digit) / base) return 0;
        value = value * base + digit;
    }
    *out = value;
    return 1;
}

static inline int schway__disc_image_cnf_component(const uint8_t *bytes, size_t size,
                                                   char out[SCHWAY_DISC_SYSTEM_CNF_COMPONENT_BYTES],
                                                   size_t *canonical_size)
{
    size_t name_size = size;
    size_t version = size;
    if (size == 0u) return 0;
    for (size_t i = 0u; i < size; i++) {
        unsigned char c = bytes[i];
        if (c == (unsigned char)';') {
            version = i;
            name_size = i;
            break;
        }
        if (!((c >= (unsigned char)'A' && c <= (unsigned char)'Z') ||
              (c >= (unsigned char)'a' && c <= (unsigned char)'z') ||
              (c >= (unsigned char)'0' && c <= (unsigned char)'9') ||
              c == (unsigned char)'_' || c == (unsigned char)'-' ||
              c == (unsigned char)'.')) return 0;
    }
    if (version != size) {
        if (version == 0u || version + 1u == size) return 0;
        for (size_t i = version + 1u; i < size; i++) {
            if (bytes[i] < (uint8_t)'0' || bytes[i] > (uint8_t)'9') return 0;
        }
    }
    if (name_size == 0u || name_size >= SCHWAY_DISC_SYSTEM_CNF_COMPONENT_BYTES ||
        (name_size == 1u && bytes[0] == (uint8_t)'.') ||
        (name_size == 2u && bytes[0] == (uint8_t)'.' && bytes[1] == (uint8_t)'.')) return 0;
    memcpy(out, bytes, name_size);
    out[name_size] = '\0';
    *canonical_size = name_size;
    return 1;
}

static inline int schway__disc_image_cnf_ram_stack(uint32_t address)
{
    return address < UINT32_C(0x00200000) ||
           (address >= UINT32_C(0x80000000) && address <= UINT32_C(0x801fffff)) ||
           (address >= UINT32_C(0xa0000000) && address <= UINT32_C(0xa01fffff));
}

/* Shared, allocation-free SYSTEM.CNF parser for the image provider and probe. */
static inline schway_disc_image_status schway_disc_image_parse_system_cnf(
    const uint8_t *bytes, size_t size, schway_disc_system_cnf *out)
{
    schway_disc_system_cnf candidate;
    size_t line = 0u;
    uint32_t seen = 0u;
    if (bytes == NULL || out == NULL || size == 0u ||
        size > SCHWAY_DISC_SYSTEM_CNF_MAX_BYTES) return SCHWAY_DISC_IMAGE_BAD_SYSCNF;
    for (size_t i = 0u; i < size; i++) {
        uint8_t c = bytes[i];
        if ((c < 0x20u && c != (uint8_t)'\r' && c != (uint8_t)'\n' &&
             c != (uint8_t)'\t') || c > 0x7eu) return SCHWAY_DISC_IMAGE_BAD_SYSCNF;
    }
    memset(&candidate, 0, sizeof candidate);
    candidate.size = (uint32_t)sizeof candidate;
    while (line < size) {
        size_t end = line;
        size_t first;
        size_t last;
        size_t key_end;
        size_t value_start;
        uint32_t bit = 0u;
        while (end < size && bytes[end] != (uint8_t)'\r' && bytes[end] != (uint8_t)'\n') end++;
        first = line;
        last = end;
        while (first < last && (bytes[first] == (uint8_t)' ' || bytes[first] == (uint8_t)'\t')) first++;
        while (last > first && (bytes[last - 1u] == (uint8_t)' ' || bytes[last - 1u] == (uint8_t)'\t')) last--;
        if (first < last && bytes[first] != (uint8_t)'#') {
            key_end = first;
            while (key_end < last && bytes[key_end] != (uint8_t)' ' &&
                   bytes[key_end] != (uint8_t)'\t' && bytes[key_end] != (uint8_t)'=') key_end++;
            if (schway__disc_image_cnf_key_is(bytes + first, key_end - first, "BOOT")) bit = 8u;
            else if (schway__disc_image_cnf_key_is(bytes + first, key_end - first, "TCB")) bit = SCHWAY_DISC_SYSTEM_CNF_HAVE_TCB;
            else if (schway__disc_image_cnf_key_is(bytes + first, key_end - first, "EVENT")) bit = SCHWAY_DISC_SYSTEM_CNF_HAVE_EVENT;
            else if (schway__disc_image_cnf_key_is(bytes + first, key_end - first, "STACK")) bit = SCHWAY_DISC_SYSTEM_CNF_HAVE_STACK;
            if (bit != 0u) {
                while (key_end < last && (bytes[key_end] == (uint8_t)' ' || bytes[key_end] == (uint8_t)'\t')) key_end++;
                if (key_end >= last || bytes[key_end] != (uint8_t)'=' || (seen & bit) != 0u) {
                    return SCHWAY_DISC_IMAGE_BAD_SYSCNF;
                }
                value_start = key_end + 1u;
                while (value_start < last && (bytes[value_start] == (uint8_t)' ' || bytes[value_start] == (uint8_t)'\t')) value_start++;
                size_t value_end = value_start;
                while (value_end < last && bytes[value_end] != (uint8_t)' ' && bytes[value_end] != (uint8_t)'\t') value_end++;
                if (value_start == value_end) return SCHWAY_DISC_IMAGE_BAD_SYSCNF;
                if (bit == 8u) {
                    if (value_end - value_start < 7u ||
                        schway__disc_image_cnf_upper(bytes[value_start]) != (unsigned char)'C' ||
                        schway__disc_image_cnf_upper(bytes[value_start + 1u]) != (unsigned char)'D' ||
                        schway__disc_image_cnf_upper(bytes[value_start + 2u]) != (unsigned char)'R' ||
                        schway__disc_image_cnf_upper(bytes[value_start + 3u]) != (unsigned char)'O' ||
                        schway__disc_image_cnf_upper(bytes[value_start + 4u]) != (unsigned char)'M' ||
                        bytes[value_start + 5u] != (uint8_t)':') return SCHWAY_DISC_IMAGE_BAD_SYSCNF;
                    size_t path = value_start + 6u;
                    if (path < value_end && (bytes[path] == (uint8_t)'/' || bytes[path] == (uint8_t)'\\')) path++;
                    while (path < value_end) {
                        size_t component_end = path;
                        size_t canonical_size;
                        if (candidate.component_count >= SCHWAY_DISC_SYSTEM_CNF_COMPONENT_LIMIT) return SCHWAY_DISC_IMAGE_BAD_SYSCNF;
                        while (component_end < value_end && bytes[component_end] != (uint8_t)'/' &&
                               bytes[component_end] != (uint8_t)'\\') component_end++;
                        if (!schway__disc_image_cnf_component(bytes + path, component_end - path,
                                candidate.components[candidate.component_count], &canonical_size)) {
                            return SCHWAY_DISC_IMAGE_BAD_SYSCNF;
                        }
                        candidate.component_count++;
                        if (component_end == value_end) break;
                        path = component_end + 1u;
                        if (path == value_end) return SCHWAY_DISC_IMAGE_BAD_SYSCNF;
                    }
                    if (candidate.component_count == 0u) return SCHWAY_DISC_IMAGE_BAD_SYSCNF;
                    size_t final_index = (size_t)candidate.component_count - 1u;
                    size_t final_size = strlen(candidate.components[final_index]);
                    size_t canonical_size = final_size;
                    for (size_t i = 0u; i < final_size; i++) {
                        if (candidate.components[final_index][i] == ';') {
                            canonical_size = i;
                            break;
                        }
                    }
                    if (canonical_size == 0u || canonical_size >= sizeof candidate.boot_name) return SCHWAY_DISC_IMAGE_BAD_SYSCNF;
                    memcpy(candidate.boot_name, candidate.components[final_index], canonical_size);
                    candidate.boot_name[canonical_size] = '\0';
                } else {
                    uint32_t *number = bit == SCHWAY_DISC_SYSTEM_CNF_HAVE_TCB ? &candidate.tcb :
                                       bit == SCHWAY_DISC_SYSTEM_CNF_HAVE_EVENT ? &candidate.event :
                                       &candidate.stack;
                    unsigned int base = bit == SCHWAY_DISC_SYSTEM_CNF_HAVE_STACK ? 16u : 10u;
                    if (!schway__disc_image_cnf_number(bytes + value_start, value_end - value_start,
                                                       base, number)) return SCHWAY_DISC_IMAGE_BAD_SYSCNF;
                    if ((bit == SCHWAY_DISC_SYSTEM_CNF_HAVE_TCB && *number > 0x40u) ||
                        (bit == SCHWAY_DISC_SYSTEM_CNF_HAVE_EVENT && *number > 0x100u) ||
                        (bit == SCHWAY_DISC_SYSTEM_CNF_HAVE_STACK && !schway__disc_image_cnf_ram_stack(*number))) {
                        return SCHWAY_DISC_IMAGE_BAD_SYSCNF;
                    }
                    candidate.present |= bit;
                }
                seen |= bit;
            }
        }
        line = end;
        while (line < size && (bytes[line] == (uint8_t)'\r' || bytes[line] == (uint8_t)'\n')) line++;
    }
    if ((seen & 8u) == 0u) return SCHWAY_DISC_IMAGE_BAD_SYSCNF;
    *out = candidate;
    return SCHWAY_DISC_IMAGE_OK;
}

schway_disc_image_status schway_disc_image_open(const char *path,
                                                schway_disc_image **out,
                                                schway_disc_image_error *error);
/* Describes the one supported raw CUE track for a patch candidate. */
schway_disc_patch_status schway_disc_image_patch_source(
    schway_disc_image *image, schway_disc_patch_source *out,
    schway_disc_patch_error *error);
/* Atomically transfers a validated candidate and preserves the old overlay
   when the candidate does not belong to this exact raw source. */
schway_disc_patch_status schway_disc_image_patch_replace(
    schway_disc_image *image, schway_disc_patch_candidate **candidate,
    schway_disc_patch_error *error);
/* Shared by CUE and CHD providers for absent virtual-pregap sectors. */
void schway__disc_image_make_virtual_sector(uint32_t track_type, uint32_t lba,
                                            uint8_t sector[2352]);
void schway_disc_image_close(schway_disc_image *image);
const schway_disc *schway_disc_image_disc(const schway_disc_image *image);
uint32_t schway_disc_image_count(const schway_disc_image *image);
schway_disc_image_status schway_disc_image_select(const schway_disc_image *image,
                                                  uint32_t index,
                                                  const schway_disc **out);
const char *schway_disc_image_path(const schway_disc_image *image, uint32_t index);
const char *schway_disc_image_label(const schway_disc_image *image, uint32_t index);
const char *schway_disc_image_status_string(schway_disc_image_status status);

#endif /* SCHWAYSTATION_HOST_DISC_IMAGE_H */
