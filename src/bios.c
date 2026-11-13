#include <string.h>

#include "internal.h"
#include "md5.h"
#include "sha256.h"

struct schway__bios_digest_row {
    uint8_t md5[16];
    uint8_t sha1[20];
    uint32_t region;
};

#ifdef SCHWAY_BIOS_TEST_SYNTHETIC
/* Synthetic all-zero test image; this key is never compiled into production. */
static const struct schway__bios_digest_row schway__bios_digest_rows[] = {
    {{0x59u, 0x07u, 0x15u, 0x90u, 0x09u, 0x9du, 0x21u, 0xddu,
      0x43u, 0x98u, 0x96u, 0x59u, 0x23u, 0x38u, 0xbfu, 0x95u},
     {0x6au, 0x52u, 0x1eu, 0x1du, 0x2au, 0x63u, 0x2cu, 0x26u, 0xe5u, 0x3bu,
      0x83u, 0xd2u, 0xccu, 0x4bu, 0x0eu, 0xdeu, 0xcfu, 0xc1u, 0xe6u, 0x8cu},
     SCHWAY_REGION_NTSC_U},
};
#else
static const struct schway__bios_digest_row schway__bios_digest_rows[] = {
#include "bios_table.inc"
};
#endif

static uint32_t schway__bios_region(const uint8_t *bytes)
{
    schway__md5 hash;
    uint8_t digest[16];
    schway__md5_init(&hash);
    schway__md5_update(&hash, bytes, SCHWAY__BIOS_SIZE);
    schway__md5_final(&hash, digest);
    for (size_t i = 0u; i < sizeof schway__bios_digest_rows / sizeof schway__bios_digest_rows[0]; i++) {
        if (memcmp(digest, schway__bios_digest_rows[i].md5, sizeof digest) == 0) {
            return schway__bios_digest_rows[i].region;
        }
    }
    return SCHWAY_BIOS_REGION_UNKNOWN;
}

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
        info.region = schway__bios_region(inst->bus.bios_storage);
        if (info.region != SCHWAY_BIOS_REGION_UNKNOWN) {
            info.flags |= SCHWAY_BIOS_INFO_RECOGNIZED;
            info.status = SCHWAY_BIOS_STATUS_RECOGNIZED;
        }
    }
    schway__instance_write_out(out, &info, (uint32_t)sizeof info);
    return SCHWAY_STATUS_OK;
}
