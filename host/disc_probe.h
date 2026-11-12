/* Bounded, host-only ISO9660 boot metadata extraction. */
#ifndef SCHWAYSTATION_HOST_DISC_PROBE_H
#define SCHWAYSTATION_HOST_DISC_PROBE_H

#include <stddef.h>
#include <stdint.h>

typedef enum schway_host_disc_probe_status {
    SCHWAY_HOST_DISC_PROBE_OK = 0,
    SCHWAY_HOST_DISC_PROBE_NOT_ISO = 1,
    SCHWAY_HOST_DISC_PROBE_IO = 2,
    SCHWAY_HOST_DISC_PROBE_LIMIT = 3,
    SCHWAY_HOST_DISC_PROBE_BAD_ISO = 4,
    SCHWAY_HOST_DISC_PROBE_BAD_SYSTEM_CNF = 5,
    SCHWAY_HOST_DISC_PROBE_NO_BOOT_EXE = 6,
    SCHWAY_HOST_DISC_PROBE_BAD_BOOT_PATH = 7,
    SCHWAY_HOST_DISC_PROBE_OUT_OF_MEMORY = 8,
    SCHWAY_HOST_DISC_PROBE_BAD_CUE = 9,
    SCHWAY_HOST_DISC_PROBE_CUE_PATH = 10,
    SCHWAY_HOST_DISC_PROBE_CUE_IO = 11,
    SCHWAY_HOST_DISC_PROBE_CUE_MODE = 12,
    SCHWAY_HOST_DISC_PROBE_CUE_NO_DATA = 13
} schway_host_disc_probe_status;

typedef struct schway_host_disc_probe {
    char serial[16];
    char boot_name[64];
    uint8_t track1_sha1[20];
    uint8_t *exe_bytes;
    size_t exe_size;
} schway_host_disc_probe;

/* Success transfers an allocated executable buffer to out. Failure leaves
   out unchanged. NOT_ISO means the caller may try another file format. */
schway_host_disc_probe_status schway__disc_probe_image(const char *path,
                                                        schway_host_disc_probe *out);
void schway__disc_probe_dispose(schway_host_disc_probe *probe);
const char *schway__disc_probe_status_string(schway_host_disc_probe_status status);

#endif /* SCHWAYSTATION_HOST_DISC_PROBE_H */
