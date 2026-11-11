/* RED-phase link scaffold. The BIOS behavior test must fail on the empty info
   returned here until the real per-instance loader is implemented. */
#include <string.h>

#include "internal.h"

schway_status schway_load_bios(schway *inst, const void *bytes, size_t size)
{
    if (inst == NULL || bytes == NULL || size != SCHWAY_BIOS_SIZE) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    return SCHWAY_STATUS_OK;
}

schway_status schway_get_bios_info(const schway *inst, schway_bios_info *out)
{
    if (inst == NULL || out == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    schway_status status = schway__instance_check_out_size(out, (uint32_t)sizeof *out);
    if (status != SCHWAY_STATUS_OK) {
        return status;
    }
    schway_bios_info info;
    memset(&info, 0, sizeof info);
    info.size = out->size;
    schway__instance_write_out(out, &info, (uint32_t)sizeof info);
    return SCHWAY_STATUS_OK;
}
