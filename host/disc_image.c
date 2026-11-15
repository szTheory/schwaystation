/* RED-phase scaffold for the shared, host-only disc image provider. */
#include "disc_image.h"

#include <string.h>

schway_disc_image_status schway_disc_image_open(const char *path,
                                                schway_disc_image **out,
                                                schway_disc_image_error *error)
{
    (void)path;
    if (out != NULL) *out = NULL;
    if (error != NULL) {
        memset(error, 0, sizeof *error);
        error->size = (uint32_t)sizeof *error;
        error->status = SCHWAY_DISC_IMAGE_BAD_IMAGE;
        (void)strncpy(error->message, "disc image provider not implemented",
                      sizeof error->message - 1u);
    }
    return SCHWAY_DISC_IMAGE_BAD_IMAGE;
}

void schway_disc_image_close(schway_disc_image *image)
{
    (void)image;
}

const schway_disc *schway_disc_image_disc(const schway_disc_image *image)
{
    (void)image;
    return NULL;
}

const char *schway_disc_image_status_string(schway_disc_image_status status)
{
    switch (status) {
    case SCHWAY_DISC_IMAGE_OK: return "ok";
    case SCHWAY_DISC_IMAGE_BAD_ARGUMENT: return "invalid argument";
    case SCHWAY_DISC_IMAGE_IO: return "disc image I/O error";
    case SCHWAY_DISC_IMAGE_BAD_CUE: return "BAD_CUE";
    case SCHWAY_DISC_IMAGE_BAD_IMAGE: return "BAD_IMAGE";
    case SCHWAY_DISC_IMAGE_LIMIT: return "disc image exceeds limits";
    case SCHWAY_DISC_IMAGE_OUT_OF_MEMORY: return "out of memory";
    case SCHWAY_DISC_IMAGE_UNSUPPORTED: return "unsupported disc image";
    }
    return "unknown disc image status";
}
