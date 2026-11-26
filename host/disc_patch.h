/* Host-only, transactional disc patch candidates. */
#ifndef SCHWAYSTATION_HOST_DISC_PATCH_H
#define SCHWAYSTATION_HOST_DISC_PATCH_H

#include <stddef.h>
#include <stdint.h>

#define SCHWAY_DISC_PATCH_SHA256_BYTES 32u
#define SCHWAY_DISC_PATCH_MAX_BYTES (16u * 1024u * 1024u)
#define SCHWAY_DISC_PATCH_MAX_RECORDS 262144u

typedef enum schway_disc_patch_format {
    SCHWAY_DISC_PATCH_FORMAT_PPF1 = 1,
    SCHWAY_DISC_PATCH_FORMAT_PPF2 = 2,
    SCHWAY_DISC_PATCH_FORMAT_PPF3 = 3,
    SCHWAY_DISC_PATCH_FORMAT_BPS = 4
} schway_disc_patch_format;

typedef enum schway_disc_patch_status {
    SCHWAY_DISC_PATCH_OK = 0,
    SCHWAY_DISC_PATCH_BAD_ARGUMENT = 1,
    SCHWAY_DISC_PATCH_UNSUPPORTED = 2,
    SCHWAY_DISC_PATCH_BAD_PATCH = 3,
    SCHWAY_DISC_PATCH_BAD_SOURCE = 4,
    SCHWAY_DISC_PATCH_BAD_IDENTITY = 5,
    SCHWAY_DISC_PATCH_BOUNDS = 6,
    SCHWAY_DISC_PATCH_LIMIT = 7,
    SCHWAY_DISC_PATCH_IO = 8,
    SCHWAY_DISC_PATCH_OUT_OF_MEMORY = 9
} schway_disc_patch_status;

typedef uint32_t (*schway_disc_patch_read_fn)(void *user, uint64_t offset,
                                              uint8_t *bytes, size_t count);

typedef struct schway_disc_patch_source {
    uint32_t size;
    uint32_t sector_bytes;
    uint64_t byte_count;
    schway_disc_patch_read_fn read;
    void *user;
} schway_disc_patch_source;

typedef struct schway_disc_patch_report {
    uint32_t size;
    uint32_t format;
    uint32_t checks;
    uint32_t records;
    uint64_t source_bytes;
    uint64_t patch_bytes;
    uint8_t source_sha256[SCHWAY_DISC_PATCH_SHA256_BYTES];
} schway_disc_patch_report;

#define SCHWAY_DISC_PATCH_CHECK_SOURCE_SHA256 1u
#define SCHWAY_DISC_PATCH_CHECK_PPF2_SOURCE_SIZE 2u
#define SCHWAY_DISC_PATCH_CHECK_PPF2_BLOCK 4u
#define SCHWAY_DISC_PATCH_CHECK_PPF3_BLOCK 8u
#define SCHWAY_DISC_PATCH_CHECK_BPS_SOURCE_CRC 16u
#define SCHWAY_DISC_PATCH_CHECK_BPS_TARGET_CRC 32u
#define SCHWAY_DISC_PATCH_CHECK_BPS_PATCH_CRC 64u

typedef struct schway_disc_patch_error {
    uint32_t size;
    uint32_t status;
    uint64_t offset;
    char message[96];
} schway_disc_patch_error;

typedef struct schway_disc_patch_candidate schway_disc_patch_candidate;

/* The supplied source is immutable for the candidate's lifetime. PPF formats
   require expected_source_sha256; BPS uses its format-native CRC checks. */
schway_disc_patch_status schway_disc_patch_load(
    const schway_disc_patch_source *source, schway_disc_patch_format format,
    const uint8_t *patch_bytes, size_t patch_size,
    const uint8_t expected_source_sha256[SCHWAY_DISC_PATCH_SHA256_BYTES],
    schway_disc_patch_candidate **out, schway_disc_patch_report *report,
    schway_disc_patch_error *error);

/* On success, the candidate remains owned by the caller until transferred to
   an image. Applying overlays never changes the supplied source buffer. */
void schway_disc_patch_candidate_destroy(schway_disc_patch_candidate *candidate);
uint32_t schway_disc_patch_candidate_matches_source(
    const schway_disc_patch_candidate *candidate,
    const schway_disc_patch_source *source);
schway_disc_patch_status schway_disc_patch_candidate_apply(
    const schway_disc_patch_candidate *candidate, uint64_t offset,
    uint8_t *bytes, size_t count);

#endif /* SCHWAYSTATION_HOST_DISC_PATCH_H */
