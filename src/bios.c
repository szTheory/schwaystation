#include <string.h>

#include "internal.h"
#include "sha256.h"

schway_status schway_load_bios(schway *inst, const void *bytes, size_t size)
{
    if (inst == NULL || bytes == NULL || size != SCHWAY__BIOS_SIZE || inst->exe_loaded != 0u ||
        inst->profile_locked != 0u || inst->frame != 0u || inst->ticks != 0u) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }

    schway__sha256 hash;
    uint8_t digest[32];
    schway__sha256_init(&hash);
    schway__sha256_update(&hash, bytes, size);
    schway__sha256_final(&hash, digest);

    struct schway__bus *bus = &inst->bus;
    memcpy(bus->bios_storage, bytes, SCHWAY__BIOS_SIZE);
    memcpy(bus->bios_sha256, digest, sizeof digest);
    bus->bios_loaded = 1u;
    bus->bios = bus->bios_storage;
    bus->bios_size = SCHWAY__BIOS_SIZE;
    inst->exe_loaded = 1u;
    return SCHWAY_STATUS_OK;
}

schway_status schway_get_bios_info(const schway *inst, schway_bios_info *out)
{
    if (inst == NULL || out == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    schway_status status = schway__instance_check_out_size(out, SCHWAY__BIOS_INFO_SIZE_FIRST);
    if (status != SCHWAY_STATUS_OK) {
        return status;
    }
    schway_bios_info info;
    memset(&info, 0, sizeof info);
    info.size = out->size;
    if (inst->bus.bios_loaded != 0u) {
        info.flags = SCHWAY_BIOS_INFO_LOADED;
        memcpy(info.sha256, inst->bus.bios_sha256, sizeof info.sha256);
    }
    schway__instance_write_out(out, &info, (uint32_t)sizeof info);
    return SCHWAY_STATUS_OK;
}
