#ifndef SCHWAYSTATION_HOST_CHEATS_H
#define SCHWAYSTATION_HOST_CHEATS_H

#include <schwaystation.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SCHWAY_HOST_CHEAT_ENTRY_COUNT 32u
#define SCHWAY_HOST_CHEAT_LINES_PER_ENTRY 64u
#define SCHWAY_HOST_CHEAT_TOTAL_LINES 256u
#define SCHWAY_HOST_CHEAT_ENTRY_TEXT_MAX 1024u
#define SCHWAY_HOST_CHEAT_SERIALIZED_SIZE (16u + SCHWAY_HOST_CHEAT_ENTRY_COUNT * \
                                           (8u + SCHWAY_HOST_CHEAT_LINES_PER_ENTRY * 8u))

typedef struct schway__host_cheat_write {
    uint32_t offset;
    uint16_t value;
    uint8_t width;
} schway__host_cheat_write;

typedef struct schway__host_cheat_entry {
    uint32_t line_count;
    uint8_t enabled;
    schway__host_cheat_write lines[SCHWAY_HOST_CHEAT_LINES_PER_ENTRY];
} schway__host_cheat_entry;

typedef struct schway__host_cheats {
    uint32_t total_lines;
    schway__host_cheat_entry entries[SCHWAY_HOST_CHEAT_ENTRY_COUNT];
} schway__host_cheats;

void schway__host_cheats_reset(schway__host_cheats *cheats);
bool schway__host_cheats_validate(const schway__host_cheats *cheats);
bool schway__host_cheats_replace(schway__host_cheats *cheats, uint32_t index, bool enabled,
                                 const char *text, size_t text_size);
bool schway__host_cheats_apply(const schway__host_cheats *cheats, schway *inst);
bool schway__host_cheats_encode(const schway__host_cheats *cheats, uint8_t *bytes,
                                size_t bytes_size);
bool schway__host_cheats_decode(schway__host_cheats *cheats, const uint8_t *bytes,
                                size_t bytes_size);

#endif
