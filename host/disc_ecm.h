#ifndef SCHWAYSTATION_HOST_DISC_ECM_H
#define SCHWAYSTATION_HOST_DISC_ECM_H

#include "disc_image.h"

typedef struct schway__disc_ecm schway__disc_ecm;

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
uint32_t schway__disc_ecm_read_sector(void *user, uint32_t lba,
                                      uint8_t sector[2352], uint8_t subcode[96]);

#endif
