#ifndef SCHWAYSTATION_HOST_DISC_PBP_H
#define SCHWAYSTATION_HOST_DISC_PBP_H

#include "disc_image.h"

typedef struct schway__disc_pbp schway__disc_pbp;

schway_disc_image_status schway__disc_pbp_open(const char *path,
                                                schway__disc_pbp **out,
                                                schway_disc_image_error *error);
void schway__disc_pbp_close(schway__disc_pbp *reader);
schway_disc_image_status schway__disc_pbp_toc(const schway__disc_pbp *reader,
                                               schway_toc *out);
uint32_t schway__disc_pbp_read_sector(void *user, uint32_t lba,
                                      uint8_t sector[2352], uint8_t subcode[96]);

#endif
