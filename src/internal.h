/* Declarations shared by the core's source files. Not installed. */
#ifndef SCHWAYSTATION_INTERNAL_H
#define SCHWAYSTATION_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include "schwaystation.h"

/* Internal version numbers reported by schway_get_versions. */
#define SCHWAY__ABI_VERSION 1u
#define SCHWAY__BEHAVIOUR_REVISION 1u
#define SCHWAY__STATE_FORMAT 1u

/* VRAM is 1024 x 512 halfwords (ARCHITECTURE section 4). */
#define SCHWAY__VRAM_W 1024u
#define SCHWAY__VRAM_H 512u

/* One frame is an exact rational number of CPU cycles: num / den, with den
   the GPU clocks per frame (ARCHITECTURE section 1). The same den is the
   denominator of the frame rate 53,693,175 / 897,619 (NTSC) and
   53,203,425 / 1,069,484 (PAL) after reduction. */
#define SCHWAY__NTSC_FRAME_NUM UINT64_C(405350378496)
#define SCHWAY__NTSC_FRAME_DEN UINT64_C(715909)
#define SCHWAY__PAL_FRAME_NUM UINT64_C(482961862656)
#define SCHWAY__PAL_FRAME_DEN UINT64_C(709379)

/* Serializable counter limits: ticks are capped at 2^62 (ARCHITECTURE
   section 1), and a frame takes at least 566,203 cycles. Loading and
   advancing use the same inclusive bounds. */
#define SCHWAY__STATE_TICKS_MAX (UINT64_C(1) << 62)
#define SCHWAY__STATE_FRAME_MAX (SCHWAY__STATE_TICKS_MAX / UINT64_C(566203))

/* The SPU makes one stereo pair every 768 CPU cycles (ARCHITECTURE
   section 1), so a frame holds 737 or 738 pairs (NTSC), 886 or 887 (PAL). */
#define SCHWAY__CYCLES_PER_PAIR 768u
#define SCHWAY__NTSC_MAX_PAIRS 738u
#define SCHWAY__PAL_MAX_PAIRS 887u

/* The audio ring holds four of the longest frames. */
#define SCHWAY__AUDIO_RING_PAIRS (4u * SCHWAY__PAL_MAX_PAIRS)

/* Display range and mode words, as GP1(05h)..GP1(08h) set them (psx-spx,
   GPU Display Control Commands). */
struct schway__display {
    uint32_t x1, x2, y1, y2;
    uint32_t dot_div;
    uint32_t field, interlaced, vres, hres;
    uint32_t start_x, start_y;
    uint32_t enabled, rgb24;
};

struct schway {
    schway_allocator allocator; /* the caller's, copied at create */
    size_t alloc_size;          /* the size given to alloc, given back to free */
    uint32_t region;            /* SCHWAY_REGION_* */
    uint32_t profile_locked;    /* 1 after the first run_frame */
    uint64_t frame;             /* frames run since power-on */
    uint64_t ticks;             /* CPU cycles run since power-on */
    uint64_t tick_acc;          /* the frame accumulator's remainder, < den */
    struct schway__display display;
    uint32_t audio_head;    /* oldest queued pair */
    uint32_t audio_count;   /* pairs queued */
    uint32_t audio_dropped; /* pairs lost since the last drain */
    int16_t audio[SCHWAY__AUDIO_RING_PAIRS][2];
    uint16_t vram[SCHWAY__VRAM_H][SCHWAY__VRAM_W];
};

/* Frame cycles per region, numerator and denominator. */
static inline uint64_t schway__instance_frame_num(uint32_t region)
{
    return region == SCHWAY_REGION_PAL ? SCHWAY__PAL_FRAME_NUM : SCHWAY__NTSC_FRAME_NUM;
}

static inline uint64_t schway__instance_frame_den(uint32_t region)
{
    return region == SCHWAY_REGION_PAL ? SCHWAY__PAL_FRAME_DEN : SCHWAY__NTSC_FRAME_DEN;
}

/* The public layouts, pinned on every target. P is the pointer size. */
#define SCHWAY__P sizeof(void *)
_Static_assert(sizeof(schway_status) == 4, "status size");

_Static_assert(offsetof(schway_allocator, alloc) == 0, "allocator.alloc");
_Static_assert(offsetof(schway_allocator, free) == SCHWAY__P, "allocator.free");
_Static_assert(offsetof(schway_allocator, user) == 2 * SCHWAY__P, "allocator.user");
_Static_assert(sizeof(schway_allocator) == 3 * SCHWAY__P, "allocator size");

_Static_assert(offsetof(schway_config, size) == 0, "config.size");
_Static_assert(offsetof(schway_config, reserved0) == 4, "config.reserved0");
_Static_assert(offsetof(schway_config, allocator) == 8, "config.allocator");
_Static_assert(sizeof(schway_config) == 8 + 3 * SCHWAY__P, "config size");

_Static_assert(offsetof(schway_versions, size) == 0, "versions.size");
_Static_assert(offsetof(schway_versions, library_major) == 4, "versions.library_major");
_Static_assert(offsetof(schway_versions, library_minor) == 8, "versions.library_minor");
_Static_assert(offsetof(schway_versions, library_patch) == 12, "versions.library_patch");
_Static_assert(offsetof(schway_versions, api) == 16, "versions.api");
_Static_assert(offsetof(schway_versions, abi) == 20, "versions.abi");
_Static_assert(offsetof(schway_versions, behaviour_revision) == 24, "versions.behaviour_revision");
_Static_assert(offsetof(schway_versions, state_format) == 28, "versions.state_format");
_Static_assert(sizeof(schway_versions) == 32, "versions size");

_Static_assert(offsetof(schway_profile, size) == 0, "profile.size");
_Static_assert(offsetof(schway_profile, region) == 4, "profile.region");
_Static_assert(sizeof(schway_profile) == 8, "profile size");

_Static_assert(offsetof(schway_info, size) == 0, "info.size");
_Static_assert(offsetof(schway_info, width) == 4, "info.width");
_Static_assert(offsetof(schway_info, height) == 8, "info.height");
_Static_assert(offsetof(schway_info, fps_num) == 12, "info.fps_num");
_Static_assert(offsetof(schway_info, fps_den) == 16, "info.fps_den");
_Static_assert(offsetof(schway_info, sample_rate_num) == 20, "info.sample_rate_num");
_Static_assert(offsetof(schway_info, sample_rate_den) == 24, "info.sample_rate_den");
_Static_assert(offsetof(schway_info, max_audio_frames) == 28, "info.max_audio_frames");
_Static_assert(offsetof(schway_info, region) == 32, "info.region");
_Static_assert(sizeof(schway_info) == 36, "info size");

_Static_assert(offsetof(schway_run_result, size) == 0, "run_result.size");
_Static_assert(offsetof(schway_run_result, stop) == 4, "run_result.stop");
_Static_assert(offsetof(schway_run_result, frame) == 8, "run_result.frame");
_Static_assert(offsetof(schway_run_result, ticks) == 16, "run_result.ticks");
_Static_assert(sizeof(schway_run_result) == 24, "run_result size");

_Static_assert(offsetof(schway_video, size) == 0, "video.size");
_Static_assert(offsetof(schway_video, reserved0) == 4, "video.reserved0");
_Static_assert(offsetof(schway_video, frame) == 8, "video.frame");
_Static_assert(offsetof(schway_video, pixels) == 16, "video.pixels");
_Static_assert(offsetof(schway_video, width) == 16 + SCHWAY__P, "video.width");
_Static_assert(offsetof(schway_video, height) == 20 + SCHWAY__P, "video.height");
_Static_assert(offsetof(schway_video, pitch_bytes) == 24 + SCHWAY__P, "video.pitch_bytes");
_Static_assert(offsetof(schway_video, bpp) == 28 + SCHWAY__P, "video.bpp");
_Static_assert(offsetof(schway_video, field) == 32 + SCHWAY__P, "video.field");
_Static_assert(offsetof(schway_video, interlaced) == 36 + SCHWAY__P, "video.interlaced");
_Static_assert(offsetof(schway_video, vres) == 40 + SCHWAY__P, "video.vres");
_Static_assert(offsetof(schway_video, hres) == 44 + SCHWAY__P, "video.hres");
_Static_assert(offsetof(schway_video, dot_div) == 48 + SCHWAY__P, "video.dot_div");
_Static_assert(offsetof(schway_video, region) == 52 + SCHWAY__P, "video.region");
_Static_assert(offsetof(schway_video, x1) == 56 + SCHWAY__P, "video.x1");
_Static_assert(offsetof(schway_video, y1) == 60 + SCHWAY__P, "video.y1");
_Static_assert(offsetof(schway_video, start_x) == 64 + SCHWAY__P, "video.start_x");
_Static_assert(offsetof(schway_video, start_y) == 68 + SCHWAY__P, "video.start_y");
_Static_assert(offsetof(schway_video, enabled) == 72 + SCHWAY__P, "video.enabled");
_Static_assert(offsetof(schway_video, rgb24) == 76 + SCHWAY__P, "video.rgb24");
_Static_assert(offsetof(schway_video, reserved1) == 80 + SCHWAY__P, "video.reserved1");
_Static_assert(sizeof(schway_video) == 80 + 2 * SCHWAY__P, "video size");
_Static_assert(sizeof(schway_video) % 8 == 0, "video size is a multiple of 8");

_Static_assert(offsetof(schway_audio, size) == 0, "audio.size");
_Static_assert(offsetof(schway_audio, reserved0) == 4, "audio.reserved0");
_Static_assert(offsetof(schway_audio, samples) == 8, "audio.samples");
_Static_assert(offsetof(schway_audio, capacity) == 8 + SCHWAY__P, "audio.capacity");
_Static_assert(offsetof(schway_audio, written) == 12 + SCHWAY__P, "audio.written");
_Static_assert(offsetof(schway_audio, dropped) == 16 + SCHWAY__P, "audio.dropped");
_Static_assert(sizeof(schway_audio) == 16 + 2 * SCHWAY__P, "audio size");

/* The size of each struct as first released. A caller's size tag below it
   is refused; each is pinned to the layout above, so appending a field later
   cannot move it. */
#define SCHWAY__CONFIG_SIZE_FIRST ((uint32_t)(8 + 3 * SCHWAY__P))
#define SCHWAY__VERSIONS_SIZE_FIRST 32u
#define SCHWAY__PROFILE_SIZE_FIRST 8u
#define SCHWAY__INFO_SIZE_FIRST 36u
#define SCHWAY__RUN_RESULT_SIZE_FIRST 24u
#define SCHWAY__VIDEO_SIZE_FIRST ((uint32_t)(80 + 2 * SCHWAY__P))
#define SCHWAY__AUDIO_SIZE_FIRST ((uint32_t)(16 + 2 * SCHWAY__P))
_Static_assert(SCHWAY__CONFIG_SIZE_FIRST == sizeof(schway_config), "config first size");
_Static_assert(SCHWAY__VERSIONS_SIZE_FIRST == sizeof(schway_versions), "versions first size");
_Static_assert(SCHWAY__PROFILE_SIZE_FIRST == sizeof(schway_profile), "profile first size");
_Static_assert(SCHWAY__INFO_SIZE_FIRST == sizeof(schway_info), "info first size");
_Static_assert(SCHWAY__RUN_RESULT_SIZE_FIRST == sizeof(schway_run_result), "run_result first size");
_Static_assert(SCHWAY__VIDEO_SIZE_FIRST == sizeof(schway_video), "video first size");
_Static_assert(SCHWAY__AUDIO_SIZE_FIRST == sizeof(schway_audio), "audio first size");

/* Input structs: OK when the size tag at s is at least first and at most
   4096, and every byte past ours is zero; ERR_STRUCT_SIZE otherwise. */
schway_status schway__instance_check_size(const void *s, uint32_t first, uint32_t ours);

/* Output structs: OK when the size tag at s is at least first, with no upper
   bound; ERR_STRUCT_SIZE otherwise. */
schway_status schway__instance_check_out_size(const void *s, uint32_t first);

/* Writes min(caller's size tag, ours) bytes of the fully built struct full to
   out; bytes past ours are left as the caller set them. */
void schway__instance_write_out(void *out, const void *full, uint32_t ours);

/* Empties the audio ring and its dropped count. */
void schway__frame_audio_clear(schway *inst);

/* CRC-32 as in Ethernet and PNG: reflected polynomial 0xEDB88320, initial
   value and final XOR 0xFFFFFFFF. Continue a running CRC by passing it back
   as crc; start from 0. */
uint32_t schway__crc32_update(uint32_t crc, const uint8_t *p, size_t n);

/* The test card's 15-bit colour word at display dot (x, y) for a frame
   counter value. */
uint16_t schway__testcard_pixel(uint32_t x, uint32_t y, uint32_t frame);

/* Draws the whole 320 x 240 card at VRAM (0, 0); vram is the 1024 x 512
   array, row-major. */
void schway__testcard_draw(uint16_t *vram, uint32_t frame);

#endif /* SCHWAYSTATION_INTERNAL_H */
