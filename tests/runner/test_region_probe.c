#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "region.h"
#include "schwaystation.h"

typedef struct region_case {
    const char *name;
    const char *serial;
    uint32_t disc_area;
    uint32_t exe_area;
    uint32_t expected_region;
    uint32_t expected_disagreements;
} region_case;

int main(void)
{
    static const region_case cases[] = {
        {"serial", "slps-00001", SCHWAY_LICENCE_NONE, SCHWAY_LICENCE_NONE,
         SCHWAY_REGION_NTSC_J, 0u},
        {"disc licence", "SLKA-00001", SCHWAY_LICENCE_EUROPE, SCHWAY_LICENCE_NONE,
         SCHWAY_REGION_PAL, 0u},
        {"exe marker", NULL, SCHWAY_LICENCE_NONE, SCHWAY_LICENCE_NORTH_AMERICA,
         SCHWAY_REGION_NTSC_U, 0u},
        {"serial priority and all conflicts", "SLUS-00001", SCHWAY_LICENCE_JAPAN,
         SCHWAY_LICENCE_EUROPE, SCHWAY_REGION_NTSC_U,
         SCHWAY__REGION_DISAGREES_SERIAL_DISC | SCHWAY__REGION_DISAGREES_SERIAL_EXE |
             SCHWAY__REGION_DISAGREES_DISC_EXE},
        {"disc priority", NULL, SCHWAY_LICENCE_EUROPE, SCHWAY_LICENCE_JAPAN,
         SCHWAY_REGION_PAL, SCHWAY__REGION_DISAGREES_DISC_EXE},
        {"unknown serial and no hints", "SLKA-00001", SCHWAY_LICENCE_NONE,
         SCHWAY_LICENCE_NONE, SCHWAY_REGION_NTSC_U, 0u}
    };
    int failed = 0;
    for (size_t i = 0u; i < sizeof cases / sizeof cases[0]; i++) {
        schway__region_resolution result = schway__region_resolve(
            cases[i].serial, cases[i].disc_area, cases[i].exe_area);
        if (result.region != cases[i].expected_region ||
            result.disagreements != cases[i].expected_disagreements) {
            fprintf(stderr, "%s: region %u (expected %u), conflicts 0x%08lx (expected 0x%08lx)\n",
                    cases[i].name, (unsigned)result.region,
                    (unsigned)cases[i].expected_region,
                    (unsigned long)result.disagreements,
                    (unsigned long)cases[i].expected_disagreements);
            failed = 1;
        }
    }
    return failed;
}
