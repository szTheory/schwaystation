#include "cheats.h"

#include <string.h>

void schway__host_cheats_reset(schway__host_cheats *cheats)
{
    if (cheats != NULL) memset(cheats, 0, sizeof *cheats);
}

bool schway__host_cheats_validate(const schway__host_cheats *cheats)
{
    (void)cheats;
    return false;
}

bool schway__host_cheats_replace(schway__host_cheats *cheats, uint32_t index, bool enabled,
                                 const char *text, size_t text_size)
{
    (void)cheats;
    (void)index;
    (void)enabled;
    (void)text;
    (void)text_size;
    return false;
}

bool schway__host_cheats_apply(const schway__host_cheats *cheats, schway *inst)
{
    (void)cheats;
    (void)inst;
    return false;
}

bool schway__host_cheats_encode(const schway__host_cheats *cheats, uint8_t *bytes,
                                size_t bytes_size)
{
    (void)cheats;
    (void)bytes;
    (void)bytes_size;
    return false;
}

bool schway__host_cheats_decode(schway__host_cheats *cheats, const uint8_t *bytes,
                                size_t bytes_size)
{
    (void)cheats;
    (void)bytes;
    (void)bytes_size;
    return false;
}
