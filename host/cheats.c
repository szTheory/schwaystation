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

static bool schway__host_cheats_zero_bytes(const uint8_t *bytes, size_t size)
{
    for (size_t i = 0u; i < size; i++) {
        if (bytes[i] != 0u) return false;
    }
    return true;
}

static void schway__host_cheats_put_u16(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8u);
}

static void schway__host_cheats_put_u32(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8u);
    bytes[2] = (uint8_t)(value >> 16u);
    bytes[3] = (uint8_t)(value >> 24u);
}

static uint16_t schway__host_cheats_get_u16(const uint8_t *bytes)
{
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8u));
}

static uint32_t schway__host_cheats_get_u32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8u) | ((uint32_t)bytes[2] << 16u) |
           ((uint32_t)bytes[3] << 24u);
}

static bool schway__host_cheat_is_separator(char byte)
{
    return byte == ' ' || byte == '\t' || byte == '\r' || byte == '\n' || byte == '\v' ||
           byte == '\f' || byte == '+';
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

static bool schway__host_cheat_parse_text(const char *text, size_t text_size,
                                          schway__host_cheat_entry *entry)
{
    if (text == NULL || entry == NULL || text_size == 0u ||
        text_size > SCHWAY_HOST_CHEAT_ENTRY_TEXT_MAX) {
        return false;
    }

    memset(entry, 0, sizeof *entry);
    size_t position = 0u;
    uint32_t line_count = 0u;
    while (position < text_size) {
        while (position < text_size && schway__host_cheat_is_separator(text[position])) position++;
        if (position == text_size) break;
        if (line_count >= SCHWAY_HOST_CHEAT_LINES_PER_ENTRY) return false;

        uint32_t code = 0u;
        if (!schway__host_cheat_parse_hex(text, text_size, &position, 8u, &code) ||
            position == text_size || !schway__host_cheat_is_separator(text[position])) {
            return false;
        }
        while (position < text_size && schway__host_cheat_is_separator(text[position])) position++;
        uint32_t data = 0u;
        if (!schway__host_cheat_parse_hex(text, text_size, &position, 4u, &data) ||
            (position < text_size && !schway__host_cheat_is_separator(text[position]))) {
            return false;
        }

        const uint32_t type = code >> 24u;
        const uint32_t offset = code & 0x00FFFFFFu;
        schway__host_cheat_write write;
        memset(&write, 0, sizeof write);
        if (type == 0x30u) {
            if ((data & 0xFF00u) != 0u) return false;
            write.width = 1u;
        } else if (type == 0x80u) {
            write.width = 2u;
        } else {
            return false;
        }
        write.offset = offset;
        write.value = (uint16_t)data;
        if (!schway__host_cheat_write_valid(&write)) return false;
        entry->lines[line_count] = write;
        line_count++;
    }

    if (line_count == 0u) return false;
    entry->line_count = line_count;
    return true;
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
    if (!schway__host_cheat_parse_text(text, text_size, &candidate)) return false;
    candidate.enabled = enabled ? 1u : 0u;

    const schway__host_cheat_entry *previous = &cheats->entries[index];
    const uint32_t other_lines = cheats->total_lines - previous->line_count;
    if (candidate.line_count > SCHWAY_HOST_CHEAT_TOTAL_LINES - other_lines) return false;
    const uint32_t total_lines = other_lines + candidate.line_count;

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
    if (bytes == NULL || bytes_size != SCHWAY_HOST_CHEAT_SERIALIZED_SIZE ||
        !schway__host_cheats_validate(cheats)) {
        return false;
    }

    memset(bytes, 0, bytes_size);
    memcpy(bytes, "SCHT", 4u);
    bytes[4] = SCHWAY_HOST_CHEAT_FORMAT_VERSION;
    schway__host_cheats_put_u16(bytes + 6u, (uint16_t)SCHWAY_HOST_CHEAT_ENTRY_COUNT);
    schway__host_cheats_put_u32(bytes + 8u, cheats->total_lines);

    size_t entry_position = 16u;
    for (uint32_t index = 0u; index < SCHWAY_HOST_CHEAT_ENTRY_COUNT; index++) {
        const schway__host_cheat_entry *entry = &cheats->entries[index];
        bytes[entry_position] = entry->enabled;
        schway__host_cheats_put_u32(bytes + entry_position + 4u, entry->line_count);
        size_t line_position = entry_position + 8u;
        for (uint32_t line = 0u; line < SCHWAY_HOST_CHEAT_LINES_PER_ENTRY; line++) {
            if (line < entry->line_count) {
                const schway__host_cheat_write *write = &entry->lines[line];
                bytes[line_position] = write->width;
                schway__host_cheats_put_u32(bytes + line_position + 2u, write->offset);
                schway__host_cheats_put_u16(bytes + line_position + 6u, write->value);
            }
            line_position += 8u;
        }
        entry_position += 8u + SCHWAY_HOST_CHEAT_LINES_PER_ENTRY * 8u;
    }
    return true;
}

bool schway__host_cheats_decode(schway__host_cheats *cheats, const uint8_t *bytes,
                                size_t bytes_size)
{
    if (cheats == NULL || bytes == NULL || bytes_size != SCHWAY_HOST_CHEAT_SERIALIZED_SIZE ||
        memcmp(bytes, "SCHT", 4u) != 0 || bytes[4] != SCHWAY_HOST_CHEAT_FORMAT_VERSION ||
        bytes[5] != 0u || schway__host_cheats_get_u16(bytes + 6u) !=
                                  SCHWAY_HOST_CHEAT_ENTRY_COUNT ||
        !schway__host_cheats_zero_bytes(bytes + 12u, 4u)) {
        return false;
    }

    const uint32_t encoded_total = schway__host_cheats_get_u32(bytes + 8u);
    if (encoded_total > SCHWAY_HOST_CHEAT_TOTAL_LINES) return false;

    schway__host_cheats candidate;
    schway__host_cheats_reset(&candidate);
    uint32_t total_lines = 0u;
    size_t entry_position = 16u;
    for (uint32_t index = 0u; index < SCHWAY_HOST_CHEAT_ENTRY_COUNT; index++) {
        schway__host_cheat_entry *entry = &candidate.entries[index];
        const uint8_t *entry_bytes = bytes + entry_position;
        if (entry_bytes[0] > 1u || !schway__host_cheats_zero_bytes(entry_bytes + 1u, 3u)) {
            return false;
        }
        const uint32_t line_count = schway__host_cheats_get_u32(entry_bytes + 4u);
        if (line_count > SCHWAY_HOST_CHEAT_LINES_PER_ENTRY ||
            line_count > SCHWAY_HOST_CHEAT_TOTAL_LINES - total_lines) {
            return false;
        }
        entry->enabled = entry_bytes[0];
        entry->line_count = line_count;
        total_lines += line_count;

        size_t line_position = entry_position + 8u;
        for (uint32_t line = 0u; line < SCHWAY_HOST_CHEAT_LINES_PER_ENTRY; line++) {
            const uint8_t *line_bytes = bytes + line_position;
            if (line < line_count) {
                schway__host_cheat_write *write = &entry->lines[line];
                if (line_bytes[1] != 0u) return false;
                write->width = line_bytes[0];
                write->offset = schway__host_cheats_get_u32(line_bytes + 2u);
                write->value = schway__host_cheats_get_u16(line_bytes + 6u);
                if (!schway__host_cheat_write_valid(write)) return false;
            } else if (!schway__host_cheats_zero_bytes(line_bytes, 8u)) {
                return false;
            }
            line_position += 8u;
        }
        entry_position += 8u + SCHWAY_HOST_CHEAT_LINES_PER_ENTRY * 8u;
    }

    if (total_lines != encoded_total) return false;
    candidate.total_lines = total_lines;
    if (!schway__host_cheats_validate(&candidate)) return false;
    *cheats = candidate;
    return true;
}
