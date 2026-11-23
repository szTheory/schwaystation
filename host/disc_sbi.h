/* Bounded host-side parser for PlayStation SBI sidecars. */
#ifndef SCHWAYSTATION_HOST_DISC_SBI_H
#define SCHWAYSTATION_HOST_DISC_SBI_H

#include <stddef.h>
#include <stdint.h>

/* Defensive implementation bounds; these are not SBI format limits. */
#define SCHWAY_DISC_SBI_MAX_BYTES 65536u
#define SCHWAY_DISC_SBI_MAX_RECORDS 8192u

typedef enum schway_disc_sbi_status {
    SCHWAY_DISC_SBI_OK = 0,
    SCHWAY_DISC_SBI_BAD_ARGUMENT = 1,
    SCHWAY_DISC_SBI_BAD_HEADER = 2,
    SCHWAY_DISC_SBI_TRUNCATED = 3,
    SCHWAY_DISC_SBI_BAD_RECORD = 4,
    SCHWAY_DISC_SBI_OUT_OF_RANGE = 5,
    SCHWAY_DISC_SBI_LIMIT = 6,
    SCHWAY_DISC_SBI_OUT_OF_MEMORY = 7
} schway_disc_sbi_status;

typedef struct schway_disc_sbi {
    uint32_t *lbas;
    size_t count;
} schway_disc_sbi;

/* `out` must be empty. On failure its value is unchanged. `lba_count` is the
   exclusive end of the selected image's addressable LBA range. */
schway_disc_sbi_status schway_disc_sbi_parse(const uint8_t *bytes, size_t size,
                                              uint32_t lba_count,
                                              schway_disc_sbi *out);
void schway_disc_sbi_clear(schway_disc_sbi *overlay);

#endif /* SCHWAYSTATION_HOST_DISC_SBI_H */
