#include "cheats.h"

#include <limits.h>
#include <string.h>

static bool schway__host_cheat_write_valid(const schway__host_cheat_write *write)
{
    if (write == NULL || (write->width != 1u && write->width != 2u) ||
        write->offset > 0x1FFFFFu ||
        (write->width == 2u && ((write->offset & 1u) != 0u || write->offset > 0x1FFFFEu))) {
        return false;
    }
    return write->width != 1u || write->value <= UINT8_MAX;
}

static bool schway__host_cheats_zero_write(const schway__host_cheat_write *write)
{
    return write->offset == 0u && write->value == 0u && write->width == 0u;
}

static bool schway__host_cheat_is_separator(char byte)
{
    return byte == ' ' || byte == '\t' || byte == '\r' || byte == '\n';
}

static int schway__host_cheat_hex(char byte)
{
    if (byte >= '0' && byte <= '9') return byte - '0';
    if (byte >= 'a' && byte <= 'f') return byte - 'a' + 10;
    if (byte >= 'A' && byte <= 'F') return byte - 'A' + 10;
    return -1;
}

static bool schway__host_cheat_parse_hex(const char *text, size_t size, size_t *position,
                                         size_t digits, uint32_t *value)
{
    uint32_t parsed = 0u;
    if (text == NULL || position == NULL || value == NULL || digits == 0u ||
        *position > size || digits > size - *position) {
        return false;
    }
    for (size_t i = 0u; i < digits; i++) {
        const int nibble = schway__host_cheat_hex(text[*position + i]);
        if (nibble < 0) return false;
        parsed = (parsed << 4u) | (uint32_t)nibble;
    }
    *position += digits;
    *value = parsed;
    return true;
}

static bool schway__host_cheat_parse_one(const char *text, size_t text_size,
                                         schway__host_cheat_write *write)
{
    if (text == NULL || write == NULL || text_size == 0u ||
        text_size > SCHWAY_HOST_CHEAT_ENTRY_TEXT_MAX) {
        return false;
    }

    size_t position = 0u;
    while (position < text_size && schway__host_cheat_is_separator(text[position])) position++;
    uint32_t code = 0u;
    if (!schway__host_cheat_parse_hex(text, text_size, &position, 8u, &code) ||
        position == text_size || !schway__host_cheat_is_separator(text[position])) {
        return false;
    }
    while (position < text_size && schway__host_cheat_is_separator(text[position])) position++;
    uint32_t data = 0u;
    if (!schway__host_cheat_parse_hex(text, text_size, &position, 4u, &data)) return false;
    while (position < text_size && schway__host_cheat_is_separator(text[position])) position++;
    if (position != text_size) return false;

    const uint32_t type = code >> 24u;
    const uint32_t offset = code & 0x00FFFFFFu;
    if (type != 0x30u || offset > 0x1FFFFFu || (data & 0xFF00u) != 0u) return false;

    write->width = 1u;
    write->offset = offset;
    write->value = (uint16_t)data;
    return schway__host_cheat_write_valid(write);
}

void schway__host_cheats_reset(schway__host_cheats *cheats)
{
    if (cheats != NULL) memset(cheats, 0, sizeof *cheats);
}

bool schway__host_cheats_validate(const schway__host_cheats *cheats)
{
    if (cheats == NULL || cheats->total_lines > SCHWAY_HOST_CHEAT_TOTAL_LINES) return false;

    uint32_t total_lines = 0u;
    for (uint32_t index = 0u; index < SCHWAY_HOST_CHEAT_ENTRY_COUNT; index++) {
        const schway__host_cheat_entry *entry = &cheats->entries[index];
        if (entry->enabled > 1u || entry->line_count > SCHWAY_HOST_CHEAT_LINES_PER_ENTRY ||
            entry->line_count > SCHWAY_HOST_CHEAT_TOTAL_LINES - total_lines) {
            return false;
        }
        total_lines += entry->line_count;
        for (uint32_t line = 0u; line < SCHWAY_HOST_CHEAT_LINES_PER_ENTRY; line++) {
            const schway__host_cheat_write *write = &entry->lines[line];
            if (line < entry->line_count) {
                if (!schway__host_cheat_write_valid(write)) return false;
            } else if (!schway__host_cheats_zero_write(write)) {
                return false;
            }
        }
    }
    return total_lines == cheats->total_lines;
}

bool schway__host_cheats_replace(schway__host_cheats *cheats, uint32_t index, bool enabled,
                                 const char *text, size_t text_size)
{
    if (!schway__host_cheats_validate(cheats) || index >= SCHWAY_HOST_CHEAT_ENTRY_COUNT ||
        text == NULL || text_size == 0u || text_size > SCHWAY_HOST_CHEAT_ENTRY_TEXT_MAX) {
        return false;
    }

    schway__host_cheat_entry candidate;
    memset(&candidate, 0, sizeof candidate);
    if (!schway__host_cheat_parse_one(text, text_size, &candidate.lines[0])) return false;
    candidate.enabled = enabled ? 1u : 0u;
    candidate.line_count = 1u;

    const schway__host_cheat_entry *previous = &cheats->entries[index];
    const uint32_t total_lines = cheats->total_lines - previous->line_count + candidate.line_count;
    if (total_lines > SCHWAY_HOST_CHEAT_TOTAL_LINES) return false;

    schway__host_cheats updated = *cheats;
    updated.entries[index] = candidate;
    updated.total_lines = total_lines;
    if (!schway__host_cheats_validate(&updated)) return false;
    *cheats = updated;
    return true;
}

bool schway__host_cheats_apply(const schway__host_cheats *cheats, schway *inst)
{
    if (!schway__host_cheats_validate(cheats) || inst == NULL) return false;

    for (uint32_t index = 0u; index < SCHWAY_HOST_CHEAT_ENTRY_COUNT; index++) {
        const schway__host_cheat_entry *entry = &cheats->entries[index];
        if (entry->enabled == 0u) continue;
        for (uint32_t line = 0u; line < entry->line_count; line++) {
            const schway__host_cheat_write *write = &entry->lines[line];
            if (schway_write_ram(inst, write->offset, write->width, write->value) !=
                SCHWAY_STATUS_OK) {
                return false;
            }
        }
    }
    return true;
}

bool schway__host_cheats_encode(const schway__host_cheats *cheats, uint8_t *bytes,
                                size_t bytes_size)
{
    (void)cheats;
    (void)bytes;
    (void)bytes_size;
    return false;
}

bool schway__host_cheats_decode(schway__host_cheats *cheats, const uint8_t *bytes,
                                size_t bytes_size)
{
    (void)cheats;
    (void)bytes;
    (void)bytes_size;
    return false;
}
