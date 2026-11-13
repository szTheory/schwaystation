/* Behavior checks for the pinned SPU transfer-log matcher. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../check.h"

static const char valid_memory_transfer_log[] =
    "spu/memory-transfer\n"
    "Test SPU memory access using DMA and regular IO.\n"
    "\n"
    "pass - testDtcRegister\n"
    "pass - testManualWriteToSpuRam\n"
    "pass - testDMAReadFromSpuRam\n"
    "pass - testDMAWriteToSpuRam\n"
    "pass - testDMAWriteTiming\n"
    "pass - testDMAReadTiming\n"
    "pass - testControlBitsAreCopiedToStatusRegister\n"
    "Done.\n";

/* RED baseline: a complete ordered hardware transcript is not yet admitted. */
static int memory_transfer_log_matches(const uint8_t *bytes, size_t size, int exit_code)
{
    (void)bytes;
    (void)size;
    (void)exit_code;
    return 0;
}

static int run_predicate_controls(void)
{
    CHECK(memory_transfer_log_matches((const uint8_t *)valid_memory_transfer_log,
                                      sizeof valid_memory_transfer_log - 1u, 0));
    CHECK_DONE();
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "predicate_controls") == 0) {
        return run_predicate_controls();
    }
    return 2;
}
