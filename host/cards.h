/* Host-side raw memory-card file operations; the core never opens files. */
#ifndef SCHWAYSTATION_HOST_CARDS_H
#define SCHWAYSTATION_HOST_CARDS_H

#include <stddef.h>
#include <stdint.h>

#include "schwaystation.h"

typedef enum schway_host_card_status {
    SCHWAY_HOST_CARD_OK = 0,
    SCHWAY_HOST_CARD_ERR_ARGUMENT = 1,
    SCHWAY_HOST_CARD_ERR_SIZE = 2,
    SCHWAY_HOST_CARD_ERR_IO = 3,
    SCHWAY_HOST_CARD_ERR_NOT_FOUND = 4
} schway_host_card_status;

typedef enum schway_host_card_format {
    SCHWAY_HOST_CARD_FORMAT_RAW = 0,
    SCHWAY_HOST_CARD_FORMAT_GME = 1,
    SCHWAY_HOST_CARD_FORMAT_VGS = 2,
    SCHWAY_HOST_CARD_FORMAT_PSV = 3
} schway_host_card_format;

enum {
    SCHWAY_HOST_CARD_LINT_BAD_HEADER = 1u,
    SCHWAY_HOST_CARD_LINT_BAD_CHECKSUM = 2u
};

typedef struct schway_host_card_lint {
    uint32_t flags;
    uint32_t bad_frame_checksums;
} schway_host_card_lint;

schway_host_card_status schway_host_card_import_raw(const char *path, void *out,
                                                     size_t out_size,
                                                     schway_host_card_lint *lint);
/* Imports raw and approved wrapped images, leaving out and lint unchanged on refusal. */
schway_host_card_status schway_host_card_import_file(const char *path, void *out,
                                                      size_t out_size,
                                                      schway_host_card_lint *lint);
/* Builds slot names from a selected content basename or serial component. */
schway_host_card_status schway_host_card_paths_for_basename(const char *directory,
                                                             const char *basename,
                                                             char *slot1, size_t slot1_size,
                                                             char *slot2, size_t slot2_size);
schway_host_card_status schway__host_card_paths_for_identity(const char *directory,
                                                              const char *identity,
                                                              char *slot1, size_t slot1_size,
                                                              char *slot2, size_t slot2_size);
schway_host_card_status schway__host_card_path_for_socket(const char *directory,
                                                           const char *component,
                                                           uint32_t port, uint32_t socket,
                                                           char *out, size_t out_size);
schway_host_card_status schway__host_card_identity_path_for_socket(
    const char *directory, const char *identity, uint32_t port, uint32_t socket,
    char *out, size_t out_size);
schway_host_card_status schway_host_card_save_atomic(const char *path, const void *bytes,
                                                      size_t size);
schway_host_card_status schway_host_card_export_atomic(const char *path, const void *bytes,
                                                        size_t size,
                                                        schway_host_card_format format);

#if defined(SCHWAY_HOST_CARD_TESTING)
typedef enum schway_host_card_test_failure {
    SCHWAY_HOST_CARD_TEST_FAILURE_NONE = 0,
    SCHWAY_HOST_CARD_TEST_FAILURE_FLUSH = 1,
    SCHWAY_HOST_CARD_TEST_FAILURE_RENAME = 2
} schway_host_card_test_failure;

schway_host_card_status schway_host_card_test_save_atomic(const char *path, const void *bytes,
                                                           size_t size,
                                                           schway_host_card_test_failure failure);
schway_host_card_status schway_host_card_test_export_atomic(
    const char *path, const void *bytes, size_t size, schway_host_card_format format,
    schway_host_card_test_failure failure);
#endif

#endif /* SCHWAYSTATION_HOST_CARDS_H */
