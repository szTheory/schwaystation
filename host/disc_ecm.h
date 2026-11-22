#ifndef SCHWAYSTATION_HOST_DISC_ECM_H
#define SCHWAYSTATION_HOST_DISC_ECM_H

#include "disc_image.h"

typedef struct schway__disc_ecm schway__disc_ecm;

typedef struct schway__disc_ecm_read_metrics {
    uint32_t size;
    uint32_t checkpoint_count;
    uint32_t decoded_forward_bytes;
    uint32_t record_headers;
} schway__disc_ecm_read_metrics;

schway_disc_image_status schway__disc_ecm_open(const char *path,
                                                schway__disc_ecm **out,
                                                schway_disc_image_error *error);
void schway__disc_ecm_close(schway__disc_ecm *reader);
schway_disc_image_status schway__disc_ecm_toc(const schway__disc_ecm *reader,
                                               schway_toc *out);
schway_disc_image_status schway__disc_ecm_decoded_size(
    const schway__disc_ecm *reader, uint64_t *out);
schway_disc_image_status schway__disc_ecm_read_bytes(
    schway__disc_ecm *reader, uint64_t offset, uint8_t *bytes, size_t size);
schway_disc_image_status schway__disc_ecm_read_metrics_get(
    const schway__disc_ecm *reader, schway__disc_ecm_read_metrics *out);
int schway__disc_ecm_test_limits(uint64_t file_bytes, uint64_t decoded_bytes,
                                 uint32_t records, uint32_t checkpoints);
schway_disc_image_status schway__disc_ecm_test_open_fail_index_alloc(
    const char *path, schway__disc_ecm **out, schway_disc_image_error *error);
uint32_t schway__disc_ecm_read_sector(void *user, uint32_t lba,
                                      uint8_t sector[2352], uint8_t subcode[96]);

#endif
