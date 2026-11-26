#include <schwaystation.h>

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

int main(int argc, char **argv)
{
    if (argc != 2) {
        fputs("usage: schwaystation-test-cheats CASE\n", stderr);
        return 2;
    }
    if (released_memory_read() != 0) return 1;
    if (strcmp(argv[1], "memory-read") == 0) return 0;
    if (strcmp(argv[1], "parser") == 0 || strcmp(argv[1], "bounds") == 0 ||
        strcmp(argv[1], "order") == 0) {
        fprintf(stderr, "cheat test case is not implemented: %s\n", argv[1]);
        return 1;
    }
    fprintf(stderr, "unknown cheat test case: %s\n", argv[1]);
    return 2;
}
