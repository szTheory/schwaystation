/* CD-XA decoder implementation. */
#include "cdxa.h"

uint32_t schway__cdxa_decode(schway *inst, const uint8_t *sector, uint32_t length,
                             int16_t *output, uint32_t output_capacity,
                             uint32_t *output_frames)
{
    (void)inst;
    (void)sector;
    (void)length;
    (void)output;
    (void)output_capacity;
    if (output_frames != 0) {
        *output_frames = 0u;
    }
    return 0u;
}
