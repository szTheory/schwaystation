/* Internal XA-ADPCM decoder interface. */
#ifndef SCHWAYSTATION_CDXA_H
#define SCHWAYSTATION_CDXA_H

#include <stdint.h>

#include "schwaystation.h"

#define SCHWAY__CDXA_RAW_SECTOR_BYTES 2352u
#define SCHWAY__CDXA_MAX_DECODED_FRAMES 4032u
#define SCHWAY__CDXA_MAX_OUTPUT_FRAMES 9408u

/* Decode one complete raw Form2 XA audio sector at its encoded sample rate.
   Returns 1 on success; malformed input or insufficient capacity returns 0. */
uint32_t schway__cdxa_decode_adpcm(schway *inst, const uint8_t *sector, uint32_t length,
                                   int16_t *decoded, uint32_t decoded_capacity,
                                   uint32_t *decoded_frames, uint32_t *low_rate);

/* Resample decoded stereo input to the PCM output rate, retaining filter history in inst. */
uint32_t schway__cdxa_resample(schway *inst, const int16_t *decoded,
                               uint32_t decoded_frames, uint32_t low_rate,
                               int16_t *output, uint32_t output_capacity,
                               uint32_t *output_frames);

/* Decode and resample one complete raw Form2 XA audio sector. */
uint32_t schway__cdxa_decode(schway *inst, const uint8_t *sector, uint32_t length,
                             int16_t *output, uint32_t output_capacity,
                             uint32_t *output_frames);

#endif /* SCHWAYSTATION_CDXA_H */
