/* Host-only patch candidate implementation. */
#include "disc_patch.h"

#include <stdio.h>
#include <string.h>

static void schway__disc_patch_error_set(schway_disc_patch_error *error,
                                         schway_disc_patch_status status,
                                         const char *message)
{
    if (error == NULL) return;
    memset(error, 0, sizeof *error);
    error->size = (uint32_t)sizeof *error;
    error->status = (uint32_t)status;
    (void)snprintf(error->message, sizeof error->message, "%s", message);
}

schway_disc_patch_status schway_disc_patch_load(
    const schway_disc_patch_source *source, schway_disc_patch_format format,
    const uint8_t *patch_bytes, size_t patch_size,
    const uint8_t expected_source_sha256[SCHWAY_DISC_PATCH_SHA256_BYTES],
    schway_disc_patch_candidate **out, schway_disc_patch_report *report,
    schway_disc_patch_error *error)
{
    (void)source;
    (void)format;
    (void)patch_bytes;
    (void)patch_size;
    (void)expected_source_sha256;
    if (out != NULL) *out = NULL;
    if (report != NULL) memset(report, 0, sizeof *report);
    schway__disc_patch_error_set(error, SCHWAY_DISC_PATCH_UNSUPPORTED,
                                 "patch candidate implementation pending");
    return SCHWAY_DISC_PATCH_UNSUPPORTED;
}

void schway_disc_patch_candidate_destroy(schway_disc_patch_candidate *candidate)
{
    (void)candidate;
}

uint32_t schway_disc_patch_candidate_matches_source(
    const schway_disc_patch_candidate *candidate,
    const schway_disc_patch_source *source)
{
    (void)candidate;
    (void)source;
    return 0u;
}

schway_disc_patch_status schway_disc_patch_candidate_apply(
    const schway_disc_patch_candidate *candidate, uint64_t offset,
    uint8_t *bytes, size_t count)
{
    (void)candidate;
    (void)offset;
    (void)bytes;
    (void)count;
    return SCHWAY_DISC_PATCH_UNSUPPORTED;
}
