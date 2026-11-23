/* The loader corpus remains the default; GPU/SIO/card replays compile a small
   subsystem corpus through the same interface. */
#ifndef SCHWAYSTATION_FUZZ_SEEDS_H
#define SCHWAYSTATION_FUZZ_SEEDS_H

#include <stddef.h>
#include <stdint.h>

/* The room seeds_build may fill for its selected campaign. */
#if defined(SCHWAY_FUZZ_CUE_SEEDS) || defined(SCHWAY_FUZZ_M3U_SEEDS)
#define SEEDS_SIZE_MAX (64u * 1024u)
#elif defined(SCHWAY_FUZZ_CHD_SEEDS)
#define SEEDS_SIZE_MAX (16u * 1024u)
#elif defined(SCHWAY_FUZZ_CNF_SEEDS)
#define SEEDS_SIZE_MAX 4096u
#elif defined(SCHWAY_FUZZ_PBP_SEEDS)
#define SEEDS_SIZE_MAX 256u
#elif defined(SCHWAY_FUZZ_CDROM_SEEDS)
#define SEEDS_SIZE_MAX (16u * 1024u)
#elif defined(SCHWAY_FUZZ_SBI_SEEDS)
#define SEEDS_SIZE_MAX 65536u
#elif defined(SCHWAY_FUZZ_MDEC_SEEDS)
#define SEEDS_SIZE_MAX (256u * 1024u)
#else
/* One byte past the largest executable accepted by the loader. */
#define SEEDS_SIZE_MAX 0x200801u
#endif

uint32_t seeds_count(void);

/* A file-name stem for seed i: letters, digits and underscores. */
const char *seeds_name(uint32_t i);

/* Writes seed i into buf (SEEDS_SIZE_MAX bytes) and returns its length. */
size_t seeds_build(uint32_t i, uint8_t *buf);

#endif /* SCHWAYSTATION_FUZZ_SEEDS_H */
