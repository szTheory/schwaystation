/* The loader fuzz seeds, built in memory so no seed byte is committed. A seed
   is a whole PS-EXE file; the replay and the nightly corpus both start from
   these. */
#ifndef SCHWAYSTATION_FUZZ_SEEDS_H
#define SCHWAYSTATION_FUZZ_SEEDS_H

#include <stddef.h>
#include <stdint.h>

/* One byte past the largest file the loader accepts (0800h + 2 MiB), the
   room seeds_build may fill. */
#define SEEDS_SIZE_MAX 0x200801u

uint32_t seeds_count(void);

/* A file-name stem for seed i: letters, digits and underscores. */
const char *seeds_name(uint32_t i);

/* Writes seed i into buf (SEEDS_SIZE_MAX bytes) and returns its length. */
size_t seeds_build(uint32_t i, uint8_t *buf);

#endif /* SCHWAYSTATION_FUZZ_SEEDS_H */
