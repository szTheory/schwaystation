#ifndef SCHWAYSTATION_HOST_DISC_CHD_H
#define SCHWAYSTATION_HOST_DISC_CHD_H

#include "disc_chd_metadata.h"

typedef struct schway__disc_chd schway__disc_chd;

schway_disc_image_status schway__disc_chd_open(const char *path,
                                                schway__disc_chd **out,
                                                schway_disc_image_error *error);
void schway__disc_chd_close(schway__disc_chd *reader);
schway_disc_image_status schway__disc_chd_toc(const schway__disc_chd *reader,
                                               schway_toc *out);
uint32_t schway__disc_chd_map_lba(const schway__disc_chd_metadata_map *map,
                                  uint32_t lba, uint32_t *frame,
                                  uint32_t *track_type,
                                  uint32_t *virtual_pregap);
uint32_t schway__disc_chd_read_sector(void *user, uint32_t lba,
                                      uint8_t sector[2352], uint8_t subcode[96]);

#endif
