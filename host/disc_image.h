/* Shared, host-only provider for bounded PlayStation disc images. */
#ifndef SCHWAYSTATION_HOST_DISC_IMAGE_H
#define SCHWAYSTATION_HOST_DISC_IMAGE_H

#include <stdint.h>

#include "schwaystation.h"

typedef enum schway_disc_image_status {
    SCHWAY_DISC_IMAGE_OK = 0,
    SCHWAY_DISC_IMAGE_BAD_ARGUMENT = 1,
    SCHWAY_DISC_IMAGE_IO = 2,
    SCHWAY_DISC_IMAGE_BAD_CUE = 3,
    SCHWAY_DISC_IMAGE_BAD_IMAGE = 4,
    SCHWAY_DISC_IMAGE_LIMIT = 5,
    SCHWAY_DISC_IMAGE_OUT_OF_MEMORY = 6,
    SCHWAY_DISC_IMAGE_UNSUPPORTED = 7
} schway_disc_image_status;

typedef struct schway_disc_image_error {
    uint32_t size;
    uint32_t status;
    uint32_t line;
    char message[96];
} schway_disc_image_error;

typedef struct schway_disc_image schway_disc_image;

schway_disc_image_status schway_disc_image_open(const char *path,
                                                schway_disc_image **out,
                                                schway_disc_image_error *error);
void schway_disc_image_close(schway_disc_image *image);
const schway_disc *schway_disc_image_disc(const schway_disc_image *image);
const char *schway_disc_image_status_string(schway_disc_image_status status);

#endif /* SCHWAYSTATION_HOST_DISC_IMAGE_H */
