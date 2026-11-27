/* Private runner-side accounting for clrmamepro DAT game occurrences. */
#ifndef SCHWAYSTATION_HOST_DAT_INVENTORY_H
#define SCHWAYSTATION_HOST_DAT_INVENTORY_H

#include <stdint.h>

typedef enum schway__dat_inventory_status {
    SCHWAY__DAT_INVENTORY_OK = 0,
    SCHWAY__DAT_INVENTORY_IO,
    SCHWAY__DAT_INVENTORY_INVALID,
    SCHWAY__DAT_INVENTORY_LIMIT,
    SCHWAY__DAT_INVENTORY_NO_MEMORY,
    SCHWAY__DAT_INVENTORY_EVIDENCE,
    SCHWAY__DAT_INVENTORY_OUTPUT
} schway__dat_inventory_status;

typedef struct schway__dat_inventory_result {
    char source_sha256[65];
    uint64_t entries;
    uint64_t pass;
    uint64_t fail;
    uint64_t unknown;
    uint64_t missing_serial;
    uint64_t duplicate_serial_entries;
} schway__dat_inventory_result;

/* Reads a bounded DAT, validates optional exact-occurrence evidence, and
   atomically writes the optional private occurrence report. No paths, titles
   or per-occurrence identifiers are emitted to standard output. */
schway__dat_inventory_status schway__dat_inventory_run(
    const char *dat_path, const char *evidence_path, const char *private_output_path,
    schway__dat_inventory_result *result);

const char *schway__dat_inventory_status_string(schway__dat_inventory_status status);

#endif
