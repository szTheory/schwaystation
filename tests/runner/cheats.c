#include <schwaystation.h>
#include "../../host/cheats.h"

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

static int released_memory_read(void)
{
    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = test_alloc;
    config.allocator.free = test_free;

    schway *inst = NULL;
    if (schway_create(&config, &inst) != SCHWAY_STATUS_OK || inst == NULL) {
        fputs("could not create core for released memory-read check\n", stderr);
        return 1;
    }
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
    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = test_alloc;
    config.allocator.free = test_free;

    schway *inst = NULL;
    if (schway_create(&config, &inst) != SCHWAY_STATUS_OK || inst == NULL) {
        fputs("could not create core for host cheat check\n", stderr);
        return 1;
    }

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

    uint32_t value = 0u;
    if (schway_peek(inst, SCHWAY_SPACE_RAM, 0x10u, 1u, &value) != SCHWAY_STATUS_OK ||
        value != 0xA5u) {
        fprintf(stderr, "expected RAM byte 0x10 to be 0xA5, got 0x%02X\n", value);
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
        schway_peek(inst, SCHWAY_SPACE_RAM, 0x10u, 1u, &value) != SCHWAY_STATUS_OK ||
        value != 0xA5u) {
        fputs("rejected replacement changed the live table or RAM\n", stderr);
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
    if (strcmp(argv[1], "parser") == 0) return parser_byte_write();
    if (strcmp(argv[1], "bounds") == 0 || strcmp(argv[1], "order") == 0) {
        fprintf(stderr, "cheat test case is not implemented: %s\n", argv[1]);
        return 1;
    }
    fprintf(stderr, "unknown cheat test case: %s\n", argv[1]);
    return 2;
}
