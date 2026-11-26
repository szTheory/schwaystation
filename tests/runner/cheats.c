#include <schwaystation.h>
#include "cheats.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *test_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void test_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static int create_test_core(schway **out)
{
    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = test_alloc;
    config.allocator.free = test_free;
    *out = NULL;
    if (schway_create(&config, out) != SCHWAY_STATUS_OK || *out == NULL) {
        fputs("could not create core for host cheat check\n", stderr);
        return 1;
    }
    return 0;
}

static int expect_ram(schway *inst, uint32_t offset, uint32_t width, uint32_t expected)
{
    uint32_t actual = UINT32_MAX;
    if (schway_peek(inst, SCHWAY_SPACE_RAM, offset, width, &actual) != SCHWAY_STATUS_OK ||
        actual != expected) {
        fprintf(stderr, "RAM at 0x%X width %u: expected 0x%X, got 0x%X\n", offset, width,
                expected, actual);
        return 1;
    }
    return 0;
}

static int released_memory_read(void)
{
    schway *inst = NULL;
    if (create_test_core(&inst) != 0) return 1;
    uint32_t value = UINT32_MAX;
    const schway_status status = schway_peek(
        inst, SCHWAY_SPACE_RAM, 0u, 4u, &value);
    schway_destroy(inst);
    if (status != SCHWAY_STATUS_OK || value != 0u) {
        fputs("released RAM memory-read check failed\n", stderr);
        return 1;
    }
    return 0;
}

static int parser_byte_write(void)
{
    static const char valid_code[] = "30000010 00A5";
    static const char invalid_code[] = "90000010 00EE";
    schway *inst = NULL;
    if (create_test_core(&inst) != 0) return 1;

    schway__host_cheats cheats;
    schway__host_cheats_reset(&cheats);
    if (!schway__host_cheats_replace(&cheats, 0u, true, valid_code, sizeof valid_code - 1u)) {
        fputs("expected a valid GameShark byte write to be accepted\n", stderr);
        schway_destroy(inst);
        return 1;
    }
    if (!schway__host_cheats_apply(&cheats, inst)) {
        fputs("expected a valid host cheat to use the checked RAM write\n", stderr);
        schway_destroy(inst);
        return 1;
    }

    if (expect_ram(inst, 0x10u, 1u, 0xA5u) != 0) {
        schway_destroy(inst);
        return 1;
    }

    schway__host_cheats before;
    memcpy(&before, &cheats, sizeof before);
    if (schway__host_cheats_replace(&cheats, 0u, true, invalid_code, sizeof invalid_code - 1u)) {
        fputs("expected unsupported replacement to be rejected\n", stderr);
        schway_destroy(inst);
        return 1;
    }
    if (memcmp(&before, &cheats, sizeof before) != 0 ||
        expect_ram(inst, 0x10u, 1u, 0xA5u) != 0) {
        fputs("rejected replacement changed the live table or RAM\n", stderr);
        schway_destroy(inst);
        return 1;
    }
    schway_destroy(inst);
    return 0;
}

static int parser_supported_subset(void)
{
    static const char codes[] = "30000010 00a5\r\n80000012 1234\n30000020+00Ff";
    schway__host_cheats cheats;
    schway__host_cheats_reset(&cheats);
    if (!schway__host_cheats_replace(&cheats, 0u, true, codes, sizeof codes - 1u)) {
        fputs("expected byte and halfword writes with CRLF and plus separators to parse\n", stderr);
        return 1;
    }
    if (cheats.total_lines != 3u || cheats.entries[0].line_count != 3u ||
        cheats.entries[0].lines[0].width != 1u || cheats.entries[1].line_count != 0u ||
        cheats.entries[0].lines[1].width != 2u || cheats.entries[0].lines[1].offset != 0x12u ||
        cheats.entries[0].lines[1].value != 0x1234u) {
        fputs("normalized writes did not preserve widths, values, and textual order\n", stderr);
        return 1;
    }

    schway *inst = NULL;
    if (create_test_core(&inst) != 0) return 1;
    if (!schway__host_cheats_apply(&cheats, inst) || expect_ram(inst, 0x10u, 1u, 0xA5u) != 0 ||
        expect_ram(inst, 0x12u, 2u, 0x1234u) != 0 ||
        expect_ram(inst, 0x20u, 1u, 0xFFu) != 0) {
        schway_destroy(inst);
        return 1;
    }
    schway_destroy(inst);
    return 0;
}

static int rejected_replacement_preserves_table(schway__host_cheats *cheats, uint32_t index,
                                                const char *text, size_t text_size)
{
    schway__host_cheats before;
    memcpy(&before, cheats, sizeof before);
    if (schway__host_cheats_replace(cheats, index, true, text, text_size)) {
        fputs("expected malformed or unsupported cheat replacement to be rejected\n", stderr);
        return 1;
    }
    if (memcmp(&before, cheats, sizeof before) != 0) {
        fputs("rejected cheat replacement changed the table\n", stderr);
        return 1;
    }
    return 0;
}

static int append_repeated_code(char *text, size_t capacity, uint32_t count, size_t *size)
{
    size_t used = 0u;
    for (uint32_t line = 0u; line < count; line++) {
        const int written = snprintf(text + used, capacity - used, "30000010 0001\n");
        if (written < 0 || (size_t)written >= capacity - used) return 1;
        used += (size_t)written;
    }
    *size = used;
    return 0;
}

static int parser_bounds(void)
{
    static const char lower[] = "30000000 00AB";
    static const char upper_halfword[] = "801FFFFE BEEF\n301FFFFF 005A";
    static const char unsupported[] = "D0000010 0001";
    static const char high_byte[] = "30000010 0101";
    static const char odd_halfword[] = "80000001 1234";
    static const char past_ram[] = "80200000 1234";
    static const char surplus_code_digits[] = "300000100 00A5";
    static const char surplus_value_digits[] = "30000010 00A50";
    static const char surplus[] = "30000010 00A5 extra";
    static const char comment[] = "30000010 00A5 # comment";
    static const char invalid_last_line[] = "30000010 00A5\nD0000010 0001";
    static const char empty[] = "";
    static const char whitespace_only[] = " \t\r\n";
    schway__host_cheats cheats;
    schway__host_cheats_reset(&cheats);
    if (!schway__host_cheats_replace(&cheats, 0u, true, lower, sizeof lower - 1u) ||
        !schway__host_cheats_replace(&cheats, 1u, true, upper_halfword,
                                     sizeof upper_halfword - 1u)) {
        fputs("expected both main RAM boundaries and an aligned halfword boundary to parse\n",
              stderr);
        return 1;
    }

    schway *inst = NULL;
    if (create_test_core(&inst) != 0) return 1;
    if (!schway__host_cheats_apply(&cheats, inst) || expect_ram(inst, 0u, 1u, 0xABu) != 0 ||
        expect_ram(inst, 0x1FFFFFu, 1u, 0x5Au) != 0 ||
        expect_ram(inst, 0x1FFFFEu, 2u, 0x5AEFu) != 0) {
        schway_destroy(inst);
        return 1;
    }

    const struct {
        const char *text;
        size_t size;
    } invalid[] = {
        {unsupported, sizeof unsupported - 1u},
        {high_byte, sizeof high_byte - 1u},
        {odd_halfword, sizeof odd_halfword - 1u},
        {past_ram, sizeof past_ram - 1u},
        {surplus_code_digits, sizeof surplus_code_digits - 1u},
        {surplus_value_digits, sizeof surplus_value_digits - 1u},
        {surplus, sizeof surplus - 1u},
        {comment, sizeof comment - 1u},
        {invalid_last_line, sizeof invalid_last_line - 1u},
        {empty, 0u},
        {whitespace_only, sizeof whitespace_only - 1u},
    };
    for (size_t i = 0u; i < sizeof invalid / sizeof invalid[0]; i++) {
        if (rejected_replacement_preserves_table(&cheats, 0u, invalid[i].text,
                                                 invalid[i].size) != 0 ||
            expect_ram(inst, 0u, 1u, 0xABu) != 0) {
            schway_destroy(inst);
            return 1;
        }
    }
    if (rejected_replacement_preserves_table(&cheats, 32u, lower, sizeof lower - 1u) != 0 ||
        rejected_replacement_preserves_table(&cheats, 0u, NULL, 0u) != 0) {
        schway_destroy(inst);
        return 1;
    }
    schway_destroy(inst);

    char line_text[65u * 14u + 1u];
    size_t line_text_size = 0u;
    schway__host_cheats_reset(&cheats);
    if (append_repeated_code(line_text, sizeof line_text, 64u, &line_text_size) != 0 ||
        !schway__host_cheats_replace(&cheats, 0u, true, line_text, line_text_size) ||
        cheats.total_lines != 64u ||
        append_repeated_code(line_text, sizeof line_text, 65u, &line_text_size) != 0 ||
        rejected_replacement_preserves_table(&cheats, 0u, line_text, line_text_size) != 0) {
        fputs("per-entry 64-line bound was not enforced transactionally\n", stderr);
        return 1;
    }

    char byte_text[SCHWAY_HOST_CHEAT_ENTRY_TEXT_MAX + 2u];
    static const char one_code[] = "30000010 00A5";
    memcpy(byte_text, one_code, sizeof one_code - 1u);
    memset(byte_text + sizeof one_code - 1u, ' ', SCHWAY_HOST_CHEAT_ENTRY_TEXT_MAX + 1u -
                                                        (sizeof one_code - 1u));
    schway__host_cheats_reset(&cheats);
    if (!schway__host_cheats_replace(&cheats, 0u, true, byte_text,
                                     SCHWAY_HOST_CHEAT_ENTRY_TEXT_MAX) ||
        rejected_replacement_preserves_table(&cheats, 0u, byte_text,
                                             SCHWAY_HOST_CHEAT_ENTRY_TEXT_MAX + 1u) != 0) {
        fputs("per-entry 1,024-byte input bound was not enforced\n", stderr);
        return 1;
    }

    schway__host_cheats_reset(&cheats);
    for (uint32_t index = 0u; index < SCHWAY_HOST_CHEAT_ENTRY_COUNT; index++) {
        if (!schway__host_cheats_replace(&cheats, index, true, one_code, sizeof one_code - 1u)) {
            fputs("expected all 32 indexed entries to be accepted\n", stderr);
            return 1;
        }
    }
    if (cheats.total_lines != 32u ||
        rejected_replacement_preserves_table(&cheats, SCHWAY_HOST_CHEAT_ENTRY_COUNT, one_code,
                                             sizeof one_code - 1u) != 0) {
        fputs("entry index bound was not enforced\n", stderr);
        return 1;
    }

    schway__host_cheats_reset(&cheats);
    for (uint32_t index = 0u; index < 4u; index++) {
        if (append_repeated_code(line_text, sizeof line_text, 64u, &line_text_size) != 0 ||
            !schway__host_cheats_replace(&cheats, index, index != 1u, line_text,
                                         line_text_size)) {
            fputs("expected the global 256-write bound to admit its final write\n", stderr);
            return 1;
        }
    }
    if (cheats.total_lines != SCHWAY_HOST_CHEAT_TOTAL_LINES ||
        rejected_replacement_preserves_table(&cheats, 4u, one_code, sizeof one_code - 1u) != 0) {
        fputs("global 256-write bound was not enforced across entries\n", stderr);
        return 1;
    }
    return 0;
}

static int parser_order(void)
{
    static const char low_index[] = "30000010 0011\n30000010 0033";
    static const char high_index[] = "30000010 0044";
    static const char disabled[] = "30000020 00AA";
    static const char enabled[] = "30000020 0055";
    schway__host_cheats cheats;
    schway__host_cheats_reset(&cheats);
    if (!schway__host_cheats_replace(&cheats, 3u, true, high_index, sizeof high_index - 1u) ||
        !schway__host_cheats_replace(&cheats, 0u, true, low_index, sizeof low_index - 1u)) {
        fputs("expected ordered overlapping byte writes to parse\n", stderr);
        return 1;
    }

    schway *inst = NULL;
    if (create_test_core(&inst) != 0) return 1;
    if (!schway__host_cheats_apply(&cheats, inst) || expect_ram(inst, 0x10u, 1u, 0x44u) != 0) {
        schway_destroy(inst);
        return 1;
    }
    if (!schway__host_cheats_replace(&cheats, 4u, true, enabled, sizeof enabled - 1u) ||
        !schway__host_cheats_apply(&cheats, inst) ||
        expect_ram(inst, 0x20u, 1u, 0x55u) != 0 ||
        !schway__host_cheats_replace(&cheats, 4u, false, disabled, sizeof disabled - 1u) ||
        !schway__host_cheats_apply(&cheats, inst) ||
        expect_ram(inst, 0x20u, 1u, 0x55u) != 0) {
        fputs("disabled entries must stop writes without restoring prior RAM\n", stderr);
        schway_destroy(inst);
        return 1;
    }
    schway_destroy(inst);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fputs("usage: schwaystation-test-cheats CASE\n", stderr);
        return 2;
    }
    if (released_memory_read() != 0) return 1;
    if (strcmp(argv[1], "memory-read") == 0) return 0;
    if (strcmp(argv[1], "parser") == 0) {
        if (parser_byte_write() != 0) return 1;
        return parser_supported_subset();
    }
    if (strcmp(argv[1], "bounds") == 0) return parser_bounds();
    if (strcmp(argv[1], "order") == 0) return parser_order();
    if (strcmp(argv[1], "unknown") == 0) {
        fprintf(stderr, "cheat test case is not implemented: %s\n", argv[1]);
        return 1;
    }
    fprintf(stderr, "unknown cheat test case: %s\n", argv[1]);
    return 2;
}
