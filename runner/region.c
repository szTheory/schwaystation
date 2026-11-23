/* Serial, disc licence and PS-EXE region hints stay on the host side. */
#include "region.h"

#include <stddef.h>
#include <string.h>

#include "schwaystation.h"

typedef struct schway__region_prefix {
    const char *prefix;
    size_t size;
    uint32_t region;
} schway__region_prefix;

static const schway__region_prefix schway__region_prefixes[] = {
    {"SLPS", 4u, SCHWAY_REGION_NTSC_J},
    {"SLPM", 4u, SCHWAY_REGION_NTSC_J},
    {"SCPS", 4u, SCHWAY_REGION_NTSC_J},
    {"SCPM", 4u, SCHWAY_REGION_NTSC_J},
    {"SIPS", 4u, SCHWAY_REGION_NTSC_J},
    {"PCPX", 4u, SCHWAY_REGION_NTSC_J},
    {"PAPX", 4u, SCHWAY_REGION_NTSC_J},
    {"PBPX", 4u, SCHWAY_REGION_NTSC_J},
    {"SLES", 4u, SCHWAY_REGION_PAL},
    {"SCES", 4u, SCHWAY_REGION_PAL},
    {"SCED", 4u, SCHWAY_REGION_PAL},
    {"SLED", 4u, SCHWAY_REGION_PAL},
    {"SLUS", 4u, SCHWAY_REGION_NTSC_U},
    {"SCUS", 4u, SCHWAY_REGION_NTSC_U},
    {"LSP", 3u, SCHWAY_REGION_NTSC_U},
    {"PUPX", 4u, SCHWAY_REGION_NTSC_U},
    {"SPUS", 4u, SCHWAY_REGION_NTSC_U}
};

static unsigned char schway__region_ascii_upper(unsigned char value)
{
    if (value >= (unsigned char)'a' && value <= (unsigned char)'z') {
        return (unsigned char)(value - (unsigned char)'a' + (unsigned char)'A');
    }
    return value;
}

static uint32_t schway__region_from_serial(const char *serial)
{
    if (serial == NULL) return 0u;
    size_t serial_size = strlen(serial);
    for (size_t i = 0u; i < sizeof schway__region_prefixes / sizeof schway__region_prefixes[0]; i++) {
        const schway__region_prefix *entry = &schway__region_prefixes[i];
        if (serial_size <= entry->size ||
            (serial[entry->size] != '-' && serial[entry->size] != '_')) {
            continue;
        }
        size_t j = 0u;
        while (j < entry->size &&
               schway__region_ascii_upper((unsigned char)serial[j]) ==
                   (unsigned char)entry->prefix[j]) {
            j++;
        }
        if (j == entry->size) return entry->region;
    }
    return 0u;
}

uint32_t schway__region_from_licence_area(uint32_t licence_area)
{
    switch (licence_area) {
    case SCHWAY_LICENCE_JAPAN: return SCHWAY_REGION_NTSC_J;
    case SCHWAY_LICENCE_EUROPE: return SCHWAY_REGION_PAL;
    case SCHWAY_LICENCE_NORTH_AMERICA: return SCHWAY_REGION_NTSC_U;
    default: return 0u;
    }
}

static void schway__region_note_disagreement(schway__region_resolution *result,
                                              uint32_t left, uint32_t right,
                                              uint32_t flag)
{
    if (left != 0u && right != 0u && left != right) result->disagreements |= flag;
}

schway__region_resolution schway__region_resolve(const char *serial,
                                                  uint32_t disc_licence_area,
                                                  uint32_t exe_licence_area)
{
    schway__region_resolution result = {0u, 0u, 0u, 0u, 0u};
    result.serial_region = schway__region_from_serial(serial);
    result.disc_region = schway__region_from_licence_area(disc_licence_area);
    result.exe_region = schway__region_from_licence_area(exe_licence_area);
    if (result.serial_region != 0u) result.region = result.serial_region;
    else if (result.disc_region != 0u) result.region = result.disc_region;
    else if (result.exe_region != 0u) result.region = result.exe_region;
    else result.region = SCHWAY_REGION_NTSC_U;
    schway__region_note_disagreement(&result, result.serial_region, result.disc_region,
                                     SCHWAY__REGION_DISAGREES_SERIAL_DISC);
    schway__region_note_disagreement(&result, result.serial_region, result.exe_region,
                                     SCHWAY__REGION_DISAGREES_SERIAL_EXE);
    schway__region_note_disagreement(&result, result.disc_region, result.exe_region,
                                     SCHWAY__REGION_DISAGREES_DISC_EXE);
    return result;
}
