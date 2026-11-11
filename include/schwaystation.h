/* schwaystation: a Sony PlayStation emulator core. Behaviour is described in docs/. */
#ifndef SCHWAYSTATION_H
#define SCHWAYSTATION_H

#include <stddef.h>
#include <stdint.h>

#include "schwaystation_version.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCHWAY_API_VERSION 4

typedef uint32_t schway_status;
#define SCHWAY_STATUS_OK 0u
#define SCHWAY_STATUS_ERR_ARGUMENT 1u
#define SCHWAY_STATUS_ERR_STRUCT_SIZE 2u
#define SCHWAY_STATUS_ERR_NO_MEMORY 3u
#define SCHWAY_STATUS_ERR_BUFFER_TOO_SMALL 4u
#define SCHWAY_STATUS_ERR_PROFILE_REJECTED 5u
#define SCHWAY_STATUS_ERR_PROFILE_LOCKED 6u
#define SCHWAY_STATUS_ERR_STATE_MAGIC 7u
#define SCHWAY_STATUS_ERR_STATE_VERSION 8u
#define SCHWAY_STATUS_ERR_STATE_CRC 9u
#define SCHWAY_STATUS_ERR_STATE_FIELD 10u
#define SCHWAY_STATUS_ERR_EXE_INVALID 11u
#define SCHWAY_STATUS_ERR_EXE_RANGE 12u

#define SCHWAY_REGION_NTSC_J 1u
#define SCHWAY_REGION_NTSC_U 2u
#define SCHWAY_REGION_PAL 3u

#define SCHWAY_STOP_FRAME 1u
#define SCHWAY_STOP_BUDGET 2u
#define SCHWAY_STOP_HOOK 4u
#define SCHWAY_STOP_FAULT 5u

#define SCHWAY_RUN_STOP_ON_FAULT 1u
#define SCHWAY_RUN_TRACE 2u

#define SCHWAY_SPACE_RAM 1u
#define SCHWAY_SPACE_SCRATCHPAD 2u

#define SCHWAY_MEMORY_RAM SCHWAY_SPACE_RAM
#define SCHWAY_MEMORY_SCRATCHPAD SCHWAY_SPACE_SCRATCHPAD
#define SCHWAY_MEMORY_VRAM 3u
#define SCHWAY_MEMORY_BIOS 4u
#define SCHWAY_MEMORY_CARD1 5u
#define SCHWAY_MEMORY_CARD2 6u

#define SCHWAY_CARD_SLOT_1 1u
#define SCHWAY_CARD_SLOT_2 2u
#define SCHWAY_MEMCARD_SIZE 131072u
#define SCHWAY_BIOS_SIZE 524288u
#define SCHWAY_BIOS_INFO_LOADED 1u

/* BIOS identity results. Status values are append-only. */
#define SCHWAY_BIOS_STATUS_UNKNOWN 0u
#define SCHWAY_BIOS_STATUS_RECOGNIZED 1u
#define SCHWAY_BIOS_REGION_UNKNOWN 0u
#define SCHWAY_BIOS_REGION_ANY 4u

/* Kinds of the first-fault record. */
#define SCHWAY_FAULT_NONE 0u
#define SCHWAY_FAULT_UNMAPPED_READ 1u
#define SCHWAY_FAULT_UNMAPPED_WRITE 2u
#define SCHWAY_FAULT_UNMAPPED_FETCH 3u
#define SCHWAY_FAULT_MISALIGNED_READ 4u
#define SCHWAY_FAULT_MISALIGNED_WRITE 5u
#define SCHWAY_FAULT_MISALIGNED_FETCH 6u
#define SCHWAY_FAULT_RESERVED_INSTRUCTION 7u
#define SCHWAY_FAULT_COP_UNUSABLE 8u
#define SCHWAY_FAULT_EXCEPTION_LOOP 9u
#define SCHWAY_FAULT_DMA 10u

/* Devices a fault names. */
#define SCHWAY_DEVICE_NONE 0u
#define SCHWAY_DEVICE_RAM 1u
#define SCHWAY_DEVICE_SCRATCH 2u
#define SCHWAY_DEVICE_BIOS 3u
#define SCHWAY_DEVICE_MEMCTL 4u
#define SCHWAY_DEVICE_IRQ 5u
#define SCHWAY_DEVICE_DMA 6u
#define SCHWAY_DEVICE_TIMER 7u
#define SCHWAY_DEVICE_CDROM 8u
#define SCHWAY_DEVICE_GPU 9u
#define SCHWAY_DEVICE_MDEC 10u
#define SCHWAY_DEVICE_SIO 11u
#define SCHWAY_DEVICE_SPU 12u
#define SCHWAY_DEVICE_EXP1 13u
#define SCHWAY_DEVICE_EXP2 14u
#define SCHWAY_DEVICE_EXP3 15u
#define SCHWAY_DEVICE_GTE 16u
#define SCHWAY_DEVICE_UNMAPPED 17u

/* Events of a trace record. */
#define SCHWAY_TRACE_EXEC 1u
#define SCHWAY_TRACE_RD 2u
#define SCHWAY_TRACE_WR 3u
#define SCHWAY_TRACE_IRQ 4u
#define SCHWAY_TRACE_FAULT 5u

#define SCHWAY_RESET_POWER 1u

#define SCHWAY_PROFILE_DEBUG_PORT 1u
#define SCHWAY_PROFILE_GPU_V0 4u

#define SCHWAY_INPUT_DEVICE_NONE 0u
#define SCHWAY_INPUT_DEVICE_DIGITAL 1u
#define SCHWAY_INPUT_DEVICE_DUALSHOCK 2u

#define SCHWAY_LICENCE_NONE 0u
#define SCHWAY_LICENCE_JAPAN 1u
#define SCHWAY_LICENCE_EUROPE 2u
#define SCHWAY_LICENCE_NORTH_AMERICA 3u

#define SCHWAY_EXE_LINT_LOW_64K 1u
#define SCHWAY_EXE_LINT_TRAILING 2u

typedef struct schway schway;

typedef struct schway_allocator {
    void *(*alloc)(void *user, size_t size); /* max_align_t-aligned */
    void (*free)(void *user, void *ptr, size_t size);
    void *user;
} schway_allocator;

typedef struct schway_config {
    uint32_t size;
    uint32_t reserved0;
    schway_allocator allocator; /* alloc and free required */
} schway_config;

typedef struct schway_versions {
    uint32_t size;
    uint32_t library_major, library_minor, library_patch;
    uint32_t api;
    uint32_t abi;
    uint32_t behaviour_revision;
    uint32_t state_format;
} schway_versions;

typedef struct schway_profile {
    uint32_t size;
    uint32_t region; /* SCHWAY_REGION_* */
    uint32_t flags;  /* SCHWAY_PROFILE_* bits */
} schway_profile;

typedef struct schway_exe_info {
    uint32_t size;
    uint32_t pc0, gp0, t_addr, t_size, d_addr, d_size, b_addr, b_size, s_addr, s_size;
    uint32_t licence_area; /* SCHWAY_LICENCE_* */
    uint32_t lint;         /* SCHWAY_EXE_LINT_* bits */
    uint32_t hash_len;     /* t_size + 0800h */
} schway_exe_info;

typedef struct schway_info {
    uint32_t size;
    uint32_t width, height;
    uint32_t fps_num, fps_den;
    uint32_t sample_rate_num, sample_rate_den;
    uint32_t max_audio_frames; /* stereo pairs in the longest frame */
    uint32_t region;
} schway_info;

typedef struct schway_run_result {
    uint32_t size;
    uint32_t stop; /* SCHWAY_STOP_* */
    uint64_t frame;
    uint64_t ticks;
    uint32_t exit_code;
    uint32_t fault_present; /* 1 once a fault is latched */
    uint32_t lag; /* 1 if the current frame has no completed pad poll */
    uint32_t rumble[2]; /* per-port large << 8 | small */
} schway_run_result;

typedef struct schway_input {
    uint32_t size;
    uint32_t device[2]; /* SCHWAY_INPUT_DEVICE_* */
    uint16_t buttons[2]; /* bit set means pressed; the serial reply is active low */
    uint8_t axes[2][4]; /* right X, right Y, left X, left Y; centre is 128 */
    uint8_t analog_button[2]; /* 0 released, 1 pressed */
    uint16_t reserved0;
} schway_input;

typedef struct schway_budget {
    uint32_t size;
    uint32_t flags; /* SCHWAY_RUN_* bits */
    uint64_t max_cycles;
} schway_budget;

typedef struct schway_fault {
    uint32_t size;
    uint32_t kind, device, width; /* SCHWAY_FAULT_*, SCHWAY_DEVICE_*, bytes */
    uint64_t frame, ticks;
    uint32_t line, dot, pc, opcode, addr, note;
} schway_fault;

typedef struct schway_video {
    uint32_t size;
    uint32_t reserved0;
    uint64_t frame;
    const void *pixels; /* into the instance; valid until the next run, load or reset */
    uint32_t width, height, pitch_bytes, bpp;
    uint32_t field, interlaced, vres, hres, dot_div, region;
    uint32_t x1, y1, start_x, start_y, enabled, rgb24;
    uint32_t reserved1;
} schway_video;

/* Read-only view of a contiguous byte span owned by an instance. */
typedef struct schway_span {
    const void *data;
    size_t size; /* bytes */
} schway_span;

typedef struct schway_bios_info {
    uint32_t size;
    uint32_t flags; /* SCHWAY_BIOS_INFO_* */
    uint8_t sha256[32];
    uint32_t status; /* SCHWAY_BIOS_STATUS_* */
    uint32_t region; /* SCHWAY_REGION_* or SCHWAY_BIOS_REGION_* */
} schway_bios_info;

typedef struct schway_audio {
    uint32_t size;
    uint32_t reserved0;
    int16_t *samples;  /* in: room for capacity stereo pairs */
    uint32_t capacity; /* in: stereo pairs */
    uint32_t written;  /* out: stereo pairs */
    uint32_t dropped;  /* out: pairs lost since the last drain */
} schway_audio;

typedef struct schway_tty {
    uint32_t size;
    uint32_t reserved0;
    uint8_t *bytes;    /* in: room for capacity bytes */
    uint32_t capacity; /* in: bytes */
    uint32_t written;  /* out: bytes */
    uint32_t dropped;  /* out: bytes lost since the last drain */
} schway_tty;

typedef struct schway_trace {
    uint32_t size;
    uint32_t event; /* SCHWAY_TRACE_* */
    uint64_t frame, cycle;
    uint32_t line, dot, pc, opcode, addr, width, device, value, note;
    uint32_t count; /* records held; every record carries it */
} schway_trace;

/* Called once per state field: count values of width bytes (1, 2, 4 or 8) in host order. */
typedef void (*schway_state_visitor)(void *user, const char *name, const void *data,
                                     uint32_t count, uint32_t width);

schway_status schway_get_versions(schway_versions *out);
schway_status schway_create(const schway_config *cfg, schway **out);
void schway_destroy(schway *inst); /* NULL-safe */
schway_status schway_set_profile(schway *inst, const schway_profile *profile);
schway_status schway_reset(schway *inst, uint32_t kind); /* SCHWAY_RESET_* */
schway_status schway_run_frame(schway *inst, schway_run_result *out); /* out may be NULL */
schway_status schway_run(schway *inst, const schway_budget *budget, schway_run_result *out);
schway_status schway_set_input(schway *inst, const schway_input *input);
schway_status schway_peek(const schway *inst, uint32_t space, uint32_t addr, uint32_t width,
                          uint32_t *value);
schway_status schway_get_fault(const schway *inst, schway_fault *out);
schway_status schway_get_info(const schway *inst, schway_info *out);
schway_status schway_get_video(const schway *inst, schway_video *out);
schway_status schway_get_vram(const schway *inst, schway_span *out);
schway_status schway_get_memory(const schway *inst, uint32_t memory, schway_span *out);
schway_status schway_load_bios(schway *inst, const void *bytes, size_t size);
schway_status schway_get_bios_info(const schway *inst, schway_bios_info *out);
schway_status schway_load_memcard(schway *inst, uint32_t slot, const void *bytes, size_t size);
schway_status schway_memcard_generation(const schway *inst, uint32_t slot, uint64_t *out);
schway_status schway_get_audio(schway *inst, schway_audio *io);
schway_status schway_get_tty(schway *inst, schway_tty *io);
schway_status schway_get_trace(const schway *inst, uint32_t index, schway_trace *out);
size_t schway_state_size(const schway *inst);
schway_status schway_save_state(const schway *inst, void *buf, size_t size);
schway_status schway_load_state(schway *inst, const void *buf, size_t size);
schway_status schway_state_visit(const schway *inst, schway_state_visitor visitor, void *user);
schway_status schway_probe_exe(const void *bytes, size_t n, schway_exe_info *out);
schway_status schway_load_exe(schway *inst, const void *bytes, size_t n);
const char *schway_status_string(schway_status status); /* never NULL */

#ifdef __cplusplus
}
#endif

#endif /* SCHWAYSTATION_H */
