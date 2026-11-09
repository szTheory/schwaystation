/* Declarations shared by the core's source files. Not installed. */
#ifndef SCHWAYSTATION_INTERNAL_H
#define SCHWAYSTATION_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include "schwaystation.h"
#include "bus.h"
#include "cpu.h"

/* Internal version numbers reported by schway_get_versions. */
#define SCHWAY__ABI_VERSION 1u
#define SCHWAY__BEHAVIOUR_REVISION 2u
#define SCHWAY__STATE_FORMAT 2u

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

/* The first event since power-on that the model calls impossible or
   unmodelled (ARCHITECTURE section 6). Only the first is kept: schway__fault_latch
   writes it while kind is NONE. */
enum {
    SCHWAY__FAULT_NONE = 0,
    SCHWAY__FAULT_UNMAPPED_READ,
    SCHWAY__FAULT_UNMAPPED_WRITE,
    SCHWAY__FAULT_UNMAPPED_FETCH,
    SCHWAY__FAULT_MISALIGNED_READ,
    SCHWAY__FAULT_MISALIGNED_WRITE,
    SCHWAY__FAULT_MISALIGNED_FETCH,
    SCHWAY__FAULT_RESERVED_INSTRUCTION,
    SCHWAY__FAULT_COP_UNUSABLE,
    SCHWAY__FAULT_EXCEPTION_LOOP
};

struct schway__fault {
    uint32_t kind;   /* SCHWAY__FAULT_* */
    uint32_t device; /* enum schway__dev */
    uint32_t width;  /* bytes, 0 to 4 */
    uint32_t note;
    uint64_t frame, ticks;
    uint32_t line, dot;
    uint32_t pc, opcode, addr;
};

/* Bytes the guest wrote to the TTY, for the host to drain: 4 KiB, and
   outside the state. A full ring drops the new byte and counts it. */
#define SCHWAY__TTY_SIZE 4096u

struct schway__tty {
    uint8_t buf[SCHWAY__TTY_SIZE];
    uint32_t head, count, dropped;
};

struct schway {
    schway_allocator allocator; /* the caller's, copied at create */
    size_t alloc_size;          /* the size given to alloc, given back to free */
    uint32_t region;            /* SCHWAY_REGION_* */
    uint32_t profile_locked;    /* 1 after the first run_frame */
    uint64_t frame;             /* frames run since power-on */
    uint64_t ticks;             /* CPU cycles run since power-on */
    uint64_t tick_acc;          /* the frame accumulator's remainder at the frame's start, < den */
    uint64_t frame_end;         /* the tick the current frame ends on, >= ticks */
    struct schway__display display;
    uint32_t audio_head;    /* oldest queued pair */
    uint32_t audio_count;   /* pairs queued */
    uint32_t audio_dropped; /* pairs lost since the last drain */
    int16_t audio[SCHWAY__AUDIO_RING_PAIRS][2];
    uint16_t vram[SCHWAY__VRAM_H][SCHWAY__VRAM_W];
    uint32_t profile_flags;     /* the profile's flags, bit 0 the debug port */
    uint32_t exe_loaded;        /* 1 after a PS-EXE was side-loaded */
    struct schway__cpu cpu;
    struct schway__bus bus;
    struct schway__irq irq;
    struct schway__fault fault;
    /* Not saved: the host drains these, and a power reset empties them. */
    struct schway__tty tty;
    uint32_t exit_pending;      /* the guest wrote the exit port */
    uint32_t exit_code;         /* the byte it wrote */
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

/* The most cycles one frame takes: the frame's cycles rounded up. */
static inline uint64_t schway__instance_frame_max(uint32_t region)
{
    uint64_t den = schway__instance_frame_den(region);
    return (schway__instance_frame_num(region) + den - 1u) / den;
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
_Static_assert(offsetof(schway_profile, flags) == 8, "profile.flags");
_Static_assert(sizeof(schway_profile) == 12, "profile size");
_Static_assert(SCHWAY__PROFILE_DEBUG_PORT == SCHWAY_PROFILE_DEBUG_PORT, "debug port flag");

_Static_assert(offsetof(schway_exe_info, size) == 0, "exe_info.size");
_Static_assert(offsetof(schway_exe_info, pc0) == 4, "exe_info.pc0");
_Static_assert(offsetof(schway_exe_info, gp0) == 8, "exe_info.gp0");
_Static_assert(offsetof(schway_exe_info, t_addr) == 12, "exe_info.t_addr");
_Static_assert(offsetof(schway_exe_info, t_size) == 16, "exe_info.t_size");
_Static_assert(offsetof(schway_exe_info, d_addr) == 20, "exe_info.d_addr");
_Static_assert(offsetof(schway_exe_info, d_size) == 24, "exe_info.d_size");
_Static_assert(offsetof(schway_exe_info, b_addr) == 28, "exe_info.b_addr");
_Static_assert(offsetof(schway_exe_info, b_size) == 32, "exe_info.b_size");
_Static_assert(offsetof(schway_exe_info, s_addr) == 36, "exe_info.s_addr");
_Static_assert(offsetof(schway_exe_info, s_size) == 40, "exe_info.s_size");
_Static_assert(offsetof(schway_exe_info, licence_area) == 44, "exe_info.licence_area");
_Static_assert(offsetof(schway_exe_info, lint) == 48, "exe_info.lint");
_Static_assert(offsetof(schway_exe_info, hash_len) == 52, "exe_info.hash_len");
_Static_assert(sizeof(schway_exe_info) == 56, "exe_info size");

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
_Static_assert(offsetof(schway_run_result, exit_code) == 24, "run_result.exit_code");
_Static_assert(offsetof(schway_run_result, fault_present) == 28, "run_result.fault_present");
_Static_assert(sizeof(schway_run_result) == 32, "run_result size");

_Static_assert(offsetof(schway_budget, size) == 0, "budget.size");
_Static_assert(offsetof(schway_budget, flags) == 4, "budget.flags");
_Static_assert(offsetof(schway_budget, max_cycles) == 8, "budget.max_cycles");
_Static_assert(sizeof(schway_budget) == 16, "budget size");

_Static_assert(offsetof(schway_fault, size) == 0, "fault.size");
_Static_assert(offsetof(schway_fault, kind) == 4, "fault.kind");
_Static_assert(offsetof(schway_fault, device) == 8, "fault.device");
_Static_assert(offsetof(schway_fault, width) == 12, "fault.width");
_Static_assert(offsetof(schway_fault, frame) == 16, "fault.frame");
_Static_assert(offsetof(schway_fault, ticks) == 24, "fault.ticks");
_Static_assert(offsetof(schway_fault, line) == 32, "fault.line");
_Static_assert(offsetof(schway_fault, dot) == 36, "fault.dot");
_Static_assert(offsetof(schway_fault, pc) == 40, "fault.pc");
_Static_assert(offsetof(schway_fault, opcode) == 44, "fault.opcode");
_Static_assert(offsetof(schway_fault, addr) == 48, "fault.addr");
_Static_assert(offsetof(schway_fault, note) == 52, "fault.note");
_Static_assert(sizeof(schway_fault) == 56, "fault size");

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

_Static_assert(offsetof(schway_tty, size) == 0, "tty.size");
_Static_assert(offsetof(schway_tty, reserved0) == 4, "tty.reserved0");
_Static_assert(offsetof(schway_tty, bytes) == 8, "tty.bytes");
_Static_assert(offsetof(schway_tty, capacity) == 8 + SCHWAY__P, "tty.capacity");
_Static_assert(offsetof(schway_tty, written) == 12 + SCHWAY__P, "tty.written");
_Static_assert(offsetof(schway_tty, dropped) == 16 + SCHWAY__P, "tty.dropped");
_Static_assert(sizeof(schway_tty) == 16 + 2 * SCHWAY__P, "tty size");

/* The size of each struct as first released. A caller's size tag below it
   is refused; each is pinned to the layout above, so appending a field later
   cannot move it. */
#define SCHWAY__CONFIG_SIZE_FIRST ((uint32_t)(8 + 3 * SCHWAY__P))
#define SCHWAY__VERSIONS_SIZE_FIRST 32u
#define SCHWAY__PROFILE_SIZE_FIRST 8u /* a literal: appended fields leave it alone */
#define SCHWAY__INFO_SIZE_FIRST 36u
#define SCHWAY__RUN_RESULT_SIZE_FIRST 24u /* a literal: appended fields leave it alone */
#define SCHWAY__BUDGET_SIZE_FIRST 16u
#define SCHWAY__VIDEO_SIZE_FIRST ((uint32_t)(80 + 2 * SCHWAY__P))
#define SCHWAY__AUDIO_SIZE_FIRST ((uint32_t)(16 + 2 * SCHWAY__P))
#define SCHWAY__EXE_INFO_SIZE_FIRST 56u
#define SCHWAY__FAULT_SIZE_FIRST 56u
#define SCHWAY__TTY_SIZE_FIRST ((uint32_t)(16 + 2 * SCHWAY__P))
_Static_assert(SCHWAY__CONFIG_SIZE_FIRST == sizeof(schway_config), "config first size");
_Static_assert(SCHWAY__VERSIONS_SIZE_FIRST == sizeof(schway_versions), "versions first size");
_Static_assert(SCHWAY__PROFILE_SIZE_FIRST == offsetof(schway_profile, flags), "profile first size");
_Static_assert(SCHWAY__INFO_SIZE_FIRST == sizeof(schway_info), "info first size");
_Static_assert(SCHWAY__RUN_RESULT_SIZE_FIRST == offsetof(schway_run_result, exit_code),
               "run_result first size");
_Static_assert(SCHWAY__BUDGET_SIZE_FIRST == sizeof(schway_budget), "budget first size");
_Static_assert(SCHWAY__VIDEO_SIZE_FIRST == sizeof(schway_video), "video first size");
_Static_assert(SCHWAY__AUDIO_SIZE_FIRST == sizeof(schway_audio), "audio first size");
_Static_assert(SCHWAY__EXE_INFO_SIZE_FIRST == sizeof(schway_exe_info), "exe_info first size");
_Static_assert(SCHWAY__FAULT_SIZE_FIRST == sizeof(schway_fault), "fault first size");
_Static_assert(SCHWAY__TTY_SIZE_FIRST == sizeof(schway_tty), "tty first size");

/* The public fault kinds and devices are the internal ones, number for number. */
_Static_assert(SCHWAY_FAULT_NONE == SCHWAY__FAULT_NONE, "fault none");
_Static_assert(SCHWAY_FAULT_UNMAPPED_READ == SCHWAY__FAULT_UNMAPPED_READ, "fault unmapped read");
_Static_assert(SCHWAY_FAULT_UNMAPPED_WRITE == SCHWAY__FAULT_UNMAPPED_WRITE, "fault unmapped write");
_Static_assert(SCHWAY_FAULT_UNMAPPED_FETCH == SCHWAY__FAULT_UNMAPPED_FETCH, "fault unmapped fetch");
_Static_assert(SCHWAY_FAULT_MISALIGNED_READ == SCHWAY__FAULT_MISALIGNED_READ, "fault misaligned read");
_Static_assert(SCHWAY_FAULT_MISALIGNED_WRITE == SCHWAY__FAULT_MISALIGNED_WRITE,
               "fault misaligned write");
_Static_assert(SCHWAY_FAULT_MISALIGNED_FETCH == SCHWAY__FAULT_MISALIGNED_FETCH,
               "fault misaligned fetch");
_Static_assert(SCHWAY_FAULT_RESERVED_INSTRUCTION == SCHWAY__FAULT_RESERVED_INSTRUCTION,
               "fault reserved instruction");
_Static_assert(SCHWAY_FAULT_COP_UNUSABLE == SCHWAY__FAULT_COP_UNUSABLE, "fault cop unusable");
_Static_assert(SCHWAY_FAULT_EXCEPTION_LOOP == SCHWAY__FAULT_EXCEPTION_LOOP, "fault exception loop");
_Static_assert(SCHWAY_DEVICE_NONE == SCHWAY__DEV_NONE, "device none");
_Static_assert(SCHWAY_DEVICE_RAM == SCHWAY__DEV_RAM, "device ram");
_Static_assert(SCHWAY_DEVICE_SCRATCH == SCHWAY__DEV_SCRATCH, "device scratch");
_Static_assert(SCHWAY_DEVICE_BIOS == SCHWAY__DEV_BIOS, "device bios");
_Static_assert(SCHWAY_DEVICE_MEMCTL == SCHWAY__DEV_MEMCTL, "device memctl");
_Static_assert(SCHWAY_DEVICE_IRQ == SCHWAY__DEV_IRQ, "device irq");
_Static_assert(SCHWAY_DEVICE_DMA == SCHWAY__DEV_DMA, "device dma");
_Static_assert(SCHWAY_DEVICE_TIMER == SCHWAY__DEV_TIMER, "device timer");
_Static_assert(SCHWAY_DEVICE_CDROM == SCHWAY__DEV_CDROM, "device cdrom");
_Static_assert(SCHWAY_DEVICE_GPU == SCHWAY__DEV_GPU, "device gpu");
_Static_assert(SCHWAY_DEVICE_MDEC == SCHWAY__DEV_MDEC, "device mdec");
_Static_assert(SCHWAY_DEVICE_SIO == SCHWAY__DEV_SIO, "device sio");
_Static_assert(SCHWAY_DEVICE_SPU == SCHWAY__DEV_SPU, "device spu");
_Static_assert(SCHWAY_DEVICE_EXP1 == SCHWAY__DEV_EXP1, "device exp1");
_Static_assert(SCHWAY_DEVICE_EXP2 == SCHWAY__DEV_EXP2, "device exp2");
_Static_assert(SCHWAY_DEVICE_EXP3 == SCHWAY__DEV_EXP3, "device exp3");
_Static_assert(SCHWAY_DEVICE_GTE == SCHWAY__DEV_GTE, "device gte");
_Static_assert(SCHWAY_DEVICE_UNMAPPED == SCHWAY__DEV_UNMAPPED, "device unmapped");

/* Input structs: OK when the size tag at s is at least first and at most
   4096, and every byte past ours is zero; ERR_STRUCT_SIZE otherwise. */
schway_status schway__instance_check_size(const void *s, uint32_t first, uint32_t ours);

/* Output structs: OK when the size tag at s is at least first, with no upper
   bound; ERR_STRUCT_SIZE otherwise. */
schway_status schway__instance_check_out_size(const void *s, uint32_t first);

/* Writes min(caller's size tag, ours) bytes of the fully built struct full to
   out; bytes past ours are left as the caller set them. */
void schway__instance_write_out(void *out, const void *full, uint32_t ours);

_Static_assert(sizeof(struct schway__cop0) == 128, "cop0 size");
/* The scalar state of the cpu ends with exc_repeat at 312; the write-queue
   count and the cache arrays follow, then the non-state log pointer, which is
   aligned differently on 32-bit and 64-bit targets. */
_Static_assert(offsetof(struct schway__cpu, exc_repeat) == 312, "cpu state size");
_Static_assert(sizeof(struct schway__irq) == 8, "irq size");
_Static_assert(SCHWAY__DEV_COUNT == 18, "device count");
_Static_assert(offsetof(struct schway__fault, frame) == 16, "fault.frame");
_Static_assert(offsetof(struct schway__fault, line) == 32, "fault.line");
_Static_assert(offsetof(struct schway__fault, addr) == 48, "fault.addr");

/* Latches the first-fault record at the current frame and ticks, only while
   none is held. The line and dot stay 0 until the GPU keeps a beam. */
void schway__fault_latch(schway *inst, uint32_t kind, uint32_t device, uint32_t width,
                         uint32_t pc, uint32_t opcode, uint32_t addr);

/* Queues one TTY byte; a full ring drops it. */
void schway__tty_push(schway *inst, uint8_t byte);

/* Empties the audio ring and its dropped count. */
void schway__frame_audio_clear(schway *inst);

/* CRC-32 as in Ethernet and PNG: reflected polynomial 0xEDB88320, initial
   value and final XOR 0xFFFFFFFF. Continue a running CRC by passing it back
   as crc; start from 0. */
uint32_t schway__crc32_update(uint32_t crc, const uint8_t *p, size_t n);

/* The stub kernel, the BIOS image mapped at BFC00000h while the host gives
   none, and its size in bytes. The array is generated from
   tests/fixtures/src/stub/stub.asm. */
extern const uint8_t schway__stub_kernel_image[];
extern const uint32_t schway__stub_kernel_size;

/* The test card's 15-bit colour word at display dot (x, y) for a frame
   counter value. */
uint16_t schway__testcard_pixel(uint32_t x, uint32_t y, uint32_t frame);

/* Draws the whole 320 x 240 card at VRAM (0, 0); vram is the 1024 x 512
   array, row-major. */
void schway__testcard_draw(uint16_t *vram, uint32_t frame);

#endif /* SCHWAYSTATION_INTERNAL_H */
