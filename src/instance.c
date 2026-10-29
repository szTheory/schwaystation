/* Instance lifetime, profile and reset, the version and info queries, status
   text and the size-tag helpers. */
#include <string.h>

#include "internal.h"

/* The largest input size tag accepted, far above any struct's planned growth,
   so a garbage tag is refused without reading past the caller's object. */
#define SIZE_TAG_MAX 4096u

static uint32_t size_tag(const void *s)
{
    uint32_t size;
    memcpy(&size, s, sizeof size); /* the tag is untrusted: read it alone first */
    return size;
}

schway_status schway__instance_check_size(const void *s, uint32_t first, uint32_t ours)
{
    const uint8_t *bytes = (const uint8_t *)s;
    uint32_t size = size_tag(s);
    if (size < first || size > SIZE_TAG_MAX) {
        return SCHWAY_STATUS_ERR_STRUCT_SIZE;
    }
    /* A newer caller's struct is accepted only if the fields this build does
       not know are all zero. */
    for (uint32_t i = ours; i < size; i++) {
        if (bytes[i] != 0u) {
            return SCHWAY_STATUS_ERR_STRUCT_SIZE;
        }
    }
    return SCHWAY_STATUS_OK;
}

schway_status schway__instance_check_out_size(const void *s, uint32_t first)
{
    return size_tag(s) < first ? SCHWAY_STATUS_ERR_STRUCT_SIZE : SCHWAY_STATUS_OK;
}

void schway__instance_write_out(void *out, const void *full, uint32_t ours)
{
    uint32_t size = size_tag(out);
    memcpy(out, full, size < ours ? size : ours);
}

/* The power-on display: GP1(06h) X1 = 260h, X2 = C60h and GP1(07h)
   Y1 = 10h, Y2 = 100h, 320-dot mode (divider 8), 240 lines, progressive,
   15-bit, display enabled (psx-spx, GPU Display Control Commands). */
static void power_on(schway *inst)
{
    inst->frame = 0u;
    inst->ticks = 0u;
    inst->tick_acc = 0u;
    struct schway__display *d = &inst->display;
    memset(d, 0, sizeof *d);
    d->x1 = 0x260u;
    d->x2 = 0xC60u;
    d->y1 = 0x10u;
    d->y2 = 0x100u;
    d->dot_div = 8u;
    d->vres = 240u;
    d->hres = 320u;
    d->enabled = 1u;
    schway__frame_audio_clear(inst);
    memset(inst->vram, 0, sizeof inst->vram);
}

schway_status schway_get_versions(schway_versions *out)
{
    if (out == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    schway_status st = schway__instance_check_out_size(out, SCHWAY__VERSIONS_SIZE_FIRST);
    if (st != SCHWAY_STATUS_OK) {
        return st;
    }
    schway_versions v;
    memset(&v, 0, sizeof v);
    v.size = out->size;
    v.library_major = SCHWAYSTATION_VERSION_MAJOR;
    v.library_minor = SCHWAYSTATION_VERSION_MINOR;
    v.library_patch = SCHWAYSTATION_VERSION_PATCH;
    v.api = SCHWAY_API_VERSION;
    v.abi = SCHWAY__ABI_VERSION;
    v.behaviour_revision = SCHWAY__BEHAVIOUR_REVISION;
    v.state_format = SCHWAY__STATE_FORMAT;
    schway__instance_write_out(out, &v, (uint32_t)sizeof v);
    return SCHWAY_STATUS_OK;
}

schway_status schway_create(const schway_config *cfg, schway **out)
{
    if (cfg == NULL || out == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    schway_status st =
        schway__instance_check_size(cfg, SCHWAY__CONFIG_SIZE_FIRST, (uint32_t)sizeof *cfg);
    if (st != SCHWAY_STATUS_OK) {
        return st;
    }
    /* Both hooks are required: the core has no allocator of its own. */
    schway_allocator a = cfg->allocator;
    if (a.alloc == NULL || a.free == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    schway *inst = (schway *)a.alloc(a.user, sizeof *inst);
    if (inst == NULL) {
        return SCHWAY_STATUS_ERR_NO_MEMORY;
    }
    memset(inst, 0, sizeof *inst);
    inst->allocator = a;
    inst->alloc_size = sizeof *inst;
    inst->region = SCHWAY_REGION_NTSC_U;
    power_on(inst);
    *out = inst;
    return SCHWAY_STATUS_OK;
}

void schway_destroy(schway *inst)
{
    if (inst == NULL) {
        return;
    }
    schway_allocator a = inst->allocator;
    a.free(a.user, inst, inst->alloc_size);
}

schway_status schway_set_profile(schway *inst, const schway_profile *profile)
{
    if (inst == NULL || profile == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    schway_status st = schway__instance_check_size(profile, SCHWAY__PROFILE_SIZE_FIRST,
                                                   (uint32_t)sizeof *profile);
    if (st != SCHWAY_STATUS_OK) {
        return st;
    }
    if (profile->region < SCHWAY_REGION_NTSC_J || profile->region > SCHWAY_REGION_PAL) {
        return SCHWAY_STATUS_ERR_PROFILE_REJECTED;
    }
    if (inst->profile_locked) {
        return SCHWAY_STATUS_ERR_PROFILE_LOCKED;
    }
    inst->region = profile->region;
    return SCHWAY_STATUS_OK;
}

/* POWER returns the instance to its state after create, keeping the
   profile. */
schway_status schway_reset(schway *inst, uint32_t kind)
{
    if (inst == NULL || kind != SCHWAY_RESET_POWER) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    power_on(inst);
    return SCHWAY_STATUS_OK;
}

schway_status schway_get_info(const schway *inst, schway_info *out)
{
    if (inst == NULL || out == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    schway_status st = schway__instance_check_out_size(out, SCHWAY__INFO_SIZE_FIRST);
    if (st != SCHWAY_STATUS_OK) {
        return st;
    }
    schway_info info;
    memset(&info, 0, sizeof info);
    info.size = out->size;
    info.width = 320u;
    info.height = 240u;
    /* 33,868,800 cycles per second over the frame's cycles (ARCHITECTURE
       section 1). */
    if (inst->region == SCHWAY_REGION_PAL) {
        info.fps_num = 53203425u;
        info.fps_den = 1069484u;
        info.max_audio_frames = SCHWAY__PAL_MAX_PAIRS;
    } else {
        info.fps_num = 53693175u;
        info.fps_den = 897619u;
        info.max_audio_frames = SCHWAY__NTSC_MAX_PAIRS;
    }
    info.sample_rate_num = 44100u;
    info.sample_rate_den = 1u;
    info.region = inst->region;
    schway__instance_write_out(out, &info, (uint32_t)sizeof info);
    return SCHWAY_STATUS_OK;
}

/* Literals behind a switch, never a pointer table: the core keeps no
   relocated data. */
static const char text_ok[] = "ok";
static const char text_argument[] = "invalid argument";
static const char text_struct_size[] = "struct size tag not accepted";
static const char text_no_memory[] = "out of memory";
static const char text_buffer_too_small[] = "buffer too small";
static const char text_profile_rejected[] = "profile rejected";
static const char text_profile_locked[] = "profile locked after the first frame";
static const char text_state_magic[] = "not a state";
static const char text_state_version[] = "state format not supported";
static const char text_state_crc[] = "state checksum mismatch";
static const char text_state_field[] = "state field out of range";
static const char text_unknown[] = "unknown status";

const char *schway_status_string(schway_status status)
{
    switch (status) {
    case SCHWAY_STATUS_OK:
        return text_ok;
    case SCHWAY_STATUS_ERR_ARGUMENT:
        return text_argument;
    case SCHWAY_STATUS_ERR_STRUCT_SIZE:
        return text_struct_size;
    case SCHWAY_STATUS_ERR_NO_MEMORY:
        return text_no_memory;
    case SCHWAY_STATUS_ERR_BUFFER_TOO_SMALL:
        return text_buffer_too_small;
    case SCHWAY_STATUS_ERR_PROFILE_REJECTED:
        return text_profile_rejected;
    case SCHWAY_STATUS_ERR_PROFILE_LOCKED:
        return text_profile_locked;
    case SCHWAY_STATUS_ERR_STATE_MAGIC:
        return text_state_magic;
    case SCHWAY_STATUS_ERR_STATE_VERSION:
        return text_state_version;
    case SCHWAY_STATUS_ERR_STATE_CRC:
        return text_state_crc;
    case SCHWAY_STATUS_ERR_STATE_FIELD:
        return text_state_field;
    default:
        return text_unknown;
    }
}
