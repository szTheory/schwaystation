/* Host-side region hints shared by the runner and its probe. */
#ifndef SCHWAYSTATION_RUNNER_REGION_H
#define SCHWAYSTATION_RUNNER_REGION_H

#include <stdint.h>

typedef struct schway__region_resolution {
    uint32_t region;
    uint32_t serial_region;
    uint32_t disc_region;
    uint32_t exe_region;
    uint32_t disagreements;
} schway__region_resolution;

#define SCHWAY__REGION_DISAGREES_SERIAL_DISC (1u << 0)
#define SCHWAY__REGION_DISAGREES_SERIAL_EXE (1u << 1)
#define SCHWAY__REGION_DISAGREES_DISC_EXE (1u << 2)

uint32_t schway__region_from_licence_area(uint32_t licence_area);
schway__region_resolution schway__region_resolve(const char *serial,
                                                  uint32_t disc_licence_area,
                                                  uint32_t exe_licence_area);

#endif /* SCHWAYSTATION_RUNNER_REGION_H */
