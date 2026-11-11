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

void schway__fault_latch(schway *inst, uint32_t kind, uint32_t device, uint32_t width,
                         uint32_t pc, uint32_t opcode, uint32_t addr)
{
    struct schway__fault *f = &inst->fault;
    if (f->kind != SCHWAY__FAULT_NONE) {
        return;
    }
    f->kind = kind;
    f->device = device;
    f->width = width;
    f->note = 0u;
    f->frame = inst->frame;
    f->ticks = schway__sched_now(inst, 1u);
    f->line = 0u;
    f->dot = 0u;
    f->pc = pc;
    f->opcode = opcode;
    f->addr = addr;
}

/* Reads the RAM or scratchpad bytes at offset addr of the space, as a
   little-endian value of width 1, 2 or 4, aligned. It touches nothing: no
   fault, no counter, no cycle (ARCHITECTURE section 6). */
schway_status schway_peek(const schway *inst, uint32_t space, uint32_t addr, uint32_t width,
                          uint32_t *value)
{
    if (inst == NULL || value == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    const uint8_t *mem;
    uint32_t size;
    if (space == SCHWAY_SPACE_RAM) {
        mem = inst->bus.ram;
        size = SCHWAY__RAM_SIZE;
    } else if (space == SCHWAY_SPACE_SCRATCHPAD) {
        mem = inst->bus.scratch;
        size = SCHWAY__SCRATCH_SIZE;
    } else {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    if ((width != 1u && width != 2u && width != 4u) || addr % width != 0u || addr >= size ||
        width > size - addr) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    uint32_t v = 0u;
    for (uint32_t i = 0; i < width; i++) {
        v |= (uint32_t)mem[addr + i] << (8u * i);
    }
    *value = v;
    return SCHWAY_STATUS_OK;
}

/* The first-fault record, or all zero while none is latched. */
schway_status schway_get_fault(const schway *inst, schway_fault *out)
{
    if (inst == NULL || out == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    schway_status st = schway__instance_check_out_size(out, SCHWAY__FAULT_SIZE_FIRST);
    if (st != SCHWAY_STATUS_OK) {
        return st;
    }
    const struct schway__fault *f = &inst->fault;
    schway_fault r;
    memset(&r, 0, sizeof r);
    r.size = out->size;
    r.kind = f->kind;
    r.device = f->device;
    r.width = f->width;
    r.frame = f->frame;
    r.ticks = f->ticks;
    r.line = f->line;
    r.dot = f->dot;
    r.pc = f->pc;
    r.opcode = f->opcode;
    r.addr = f->addr;
    r.note = f->note;
    schway__instance_write_out(out, &r, (uint32_t)sizeof r);
    return SCHWAY_STATUS_OK;
}

void schway__tty_push(schway *inst, uint8_t byte)
{
    struct schway__tty *t = &inst->tty;
    if (t->count >= SCHWAY__TTY_SIZE) {
        if (t->dropped != UINT32_MAX) {
            t->dropped++;
        }
        return;
    }
    t->buf[(t->head + t->count) % SCHWAY__TTY_SIZE] = byte;
    t->count++;
}

schway_status schway_get_tty(schway *inst, schway_tty *io)
{
    if (inst == NULL || io == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    schway_status st =
        schway__instance_check_size(io, SCHWAY__TTY_SIZE_FIRST, (uint32_t)sizeof *io);
    if (st != SCHWAY_STATUS_OK) {
        return st;
    }
    if (io->bytes == NULL && io->capacity != 0u) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    struct schway__tty *t = &inst->tty;
    uint32_t n = io->capacity < t->count ? io->capacity : t->count;
    for (uint32_t i = 0; i < n; i++) {
        io->bytes[i] = t->buf[(t->head + i) % SCHWAY__TTY_SIZE];
    }
    t->head = (t->head + n) % SCHWAY__TTY_SIZE;
    t->count -= n;
    io->written = n;
    io->dropped = t->dropped;
    t->dropped = 0u;
    return SCHWAY_STATUS_OK;
}

/* GPU clocks per scan line (psx-spx, GPU Timings): NTSC lines are 3413 video
   clocks, PAL lines 3406. */
#define CLOCKS_PER_LINE_NTSC 3413u
#define CLOCKS_PER_LINE_PAL 3406u

/* Record index of the ring, counted from the oldest, as the public struct.
   frame is widened from the 32 bits the ring keeps. line and dot come from the
   cycles since the record's frame started, converted to GPU clocks (11 / 7 of
   the CPU's) and split by the region's clocks per line, until the GPU keeps a
   beam of its own; dot is the clock within the line. A frame's first cycle is
   the exact floor of frame * num / den, built so no product passes 64 bits
   (ARCHITECTURE section 1). A record from before a state load can lie outside
   its frame, and then line and dot are 0. */
schway_status schway_get_trace(const schway *inst, uint32_t index, schway_trace *out)
{
    if (inst == NULL || out == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    schway_status st = schway__instance_check_out_size(out, SCHWAY__TRACE_SIZE_FIRST);
    if (st != SCHWAY_STATUS_OK) {
        return st;
    }
    const struct schway__trace_ring *ring = &inst->trace;
    if (index >= ring->count) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    const struct schway__trace_rec *rec =
        &ring->rec[(ring->head + index) % SCHWAY__TRACE_CAP];
    uint64_t frame = (inst->frame & ~UINT64_C(0xFFFFFFFF)) | rec->frame;
    if (frame > inst->frame && frame >= UINT64_C(0x100000000)) {
        frame -= UINT64_C(0x100000000);
    }
    uint64_t num = schway__instance_frame_num(inst->region);
    uint64_t den = schway__instance_frame_den(inst->region);
    uint64_t start = frame * (num / den) + frame * (num % den) / den;
    uint32_t line = 0u;
    uint32_t dot = 0u;
    if (rec->cycle >= start && rec->cycle - start < schway__instance_frame_max(inst->region)) {
        uint64_t clocks = (rec->cycle - start) * 11u / 7u;
        uint32_t per_line =
            inst->region == SCHWAY_REGION_PAL ? CLOCKS_PER_LINE_PAL : CLOCKS_PER_LINE_NTSC;
        line = (uint32_t)(clocks / per_line);
        dot = (uint32_t)(clocks % per_line);
    }
    schway_trace r;
    memset(&r, 0, sizeof r);
    r.size = out->size;
    r.event = SCHWAY__TRACE_EVENT(rec->meta);
    r.frame = frame;
    r.cycle = rec->cycle;
    r.line = line;
    r.dot = dot;
    r.pc = rec->pc;
    r.opcode = rec->opcode;
    r.addr = rec->addr;
    r.width = SCHWAY__TRACE_WIDTH(rec->meta);
    r.device = SCHWAY__TRACE_DEVICE(rec->meta);
    r.value = rec->value;
    r.note = SCHWAY__TRACE_NOTE(rec->meta);
    r.count = ring->count;
    schway__instance_write_out(out, &r, (uint32_t)sizeof r);
    return SCHWAY_STATUS_OK;
}

/* The power-on display: GP1(06h) X1 = 260h, X2 = C60h and GP1(07h)
   Y1 = 10h, Y2 = 100h, 320-dot mode (divider 8), 240 lines, progressive,
   15-bit, display enabled (psx-spx, GPU Display Control Commands). */
static void power_on(schway *inst)
{
    inst->frame = 0u;
    inst->ticks = 0u;
    inst->tick_acc = 0u;
    inst->frame_end = schway__instance_frame_num(inst->region) /
                      schway__instance_frame_den(inst->region);
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
    schway__cpu_reset(inst);
    schway__gte_reset(&inst->gte);
    schway__bus_reset(inst);
    schway__irq_reset(inst);
    schway__gpu_reset(inst);
    schway__dma_reset(inst);
    schway__timers_reset(inst);
    schway__pad_reset(inst);
    schway__sio_reset(inst);
    schway__card_reset(inst);
    schway__sched_sync(inst);
    schway__sched_recalc(inst);
    /* With no BIOS image the BIOS window holds the stub kernel; a mapped
       image, the host's, is kept across a power reset. */
    if (inst->bus.bios == NULL) {
        inst->bus.bios = schway__stub_kernel_image;
        inst->bus.bios_size = schway__stub_kernel_size;
    }
    /* The profile, and its flags, survive a power reset. */
    inst->exe_loaded = 0u;
    memset(&inst->fault, 0, sizeof inst->fault);
    memset(&inst->tty, 0, sizeof inst->tty);
    memset(&inst->trace, 0, sizeof inst->trace);
    inst->exit_pending = 0u;
    inst->exit_code = 0u;
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
    size_t alloc_size = sizeof(schway) + (size_t)SCHWAY__VIDEO_MAX_BYTES;
    schway *inst = (schway *)a.alloc(a.user, alloc_size);
    if (inst == NULL) {
        return SCHWAY_STATUS_ERR_NO_MEMORY;
    }
    memset(inst, 0, alloc_size);
    inst->allocator = a;
    inst->alloc_size = alloc_size;
    inst->video_scanout = (uint8_t *)(void *)(inst + 1);
    inst->region = SCHWAY_REGION_NTSC_U;
    schway__card_init(inst);
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
    /* A caller whose struct ends before flags predates them: none set. */
    uint32_t flags = profile->size >= sizeof *profile ? profile->flags : 0u;
    if (profile->region < SCHWAY_REGION_NTSC_J || profile->region > SCHWAY_REGION_PAL ||
        (flags & ~(SCHWAY_PROFILE_DEBUG_PORT | SCHWAY_PROFILE_GPU_V0)) != 0u) {
        return SCHWAY_STATUS_ERR_PROFILE_REJECTED;
    }
    if (inst->profile_locked) {
        return SCHWAY_STATUS_ERR_PROFILE_LOCKED;
    }
    inst->region = profile->region;
    inst->profile_flags = flags;
    /* The first frame is the region's; nothing has run before the first
       frame locks the profile. */
    if (inst->ticks == 0u) {
        inst->frame_end = schway__instance_frame_num(inst->region) /
                          schway__instance_frame_den(inst->region);
    }
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

schway_status schway_get_vram(const schway *inst, schway_span *out)
{
    return schway_get_memory(inst, SCHWAY_MEMORY_VRAM, out);
}

schway_status schway_get_memory(const schway *inst, uint32_t memory, schway_span *out)
{
    if (inst == NULL || out == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    schway_span span;
    switch (memory) {
    case SCHWAY_MEMORY_RAM:
        span.data = inst->bus.ram;
        span.size = sizeof inst->bus.ram;
        break;
    case SCHWAY_MEMORY_SCRATCHPAD:
        span.data = inst->bus.scratch;
        span.size = sizeof inst->bus.scratch;
        break;
    case SCHWAY_MEMORY_VRAM:
        span.data = &inst->vram[0][0];
        span.size = sizeof inst->vram;
        break;
    case SCHWAY_MEMORY_BIOS:
        span.data = inst->bus.bios;
        span.size = inst->bus.bios_size;
        break;
    case SCHWAY_MEMORY_CARD1:
        span.data = inst->card.slot[0].bytes;
        span.size = sizeof inst->card.slot[0].bytes;
        break;
    case SCHWAY_MEMORY_CARD2:
        span.data = inst->card.slot[1].bytes;
        span.size = sizeof inst->card.slot[1].bytes;
        break;
    default:
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    *out = span;
    return SCHWAY_STATUS_OK;
}

schway_status schway_load_memcard(schway *inst, uint32_t slot, const void *bytes, size_t size)
{
    if (inst == NULL || bytes == NULL || size > SCHWAY_MEMCARD_SIZE ||
        (slot != SCHWAY_CARD_SLOT_1 && slot != SCHWAY_CARD_SLOT_2) ||
        inst->profile_locked != 0u || inst->frame != 0u || inst->ticks != 0u) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    uint32_t index = slot == SCHWAY_CARD_SLOT_1 ? 0u : 1u;
    memmove(inst->card.slot[index].bytes, bytes, size);
    if (size < SCHWAY_MEMCARD_SIZE) {
        memset(inst->card.slot[index].bytes + size, 0xFF, SCHWAY_MEMCARD_SIZE - size);
    }
    inst->card.slot[index].generation = 0u;
    schway__card_abort(inst, index);
    return SCHWAY_STATUS_OK;
}

schway_status schway_memcard_generation(const schway *inst, uint32_t slot, uint64_t *out)
{
    if (inst == NULL || out == NULL ||
        (slot != SCHWAY_CARD_SLOT_1 && slot != SCHWAY_CARD_SLOT_2)) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    uint32_t index = slot == SCHWAY_CARD_SLOT_1 ? 0u : 1u;
    *out = inst->card.slot[index].generation;
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
static const char text_exe_invalid[] = "PS-EXE invalid";
static const char text_exe_range[] = "PS-EXE outside RAM";
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
    case SCHWAY_STATUS_ERR_EXE_INVALID:
        return text_exe_invalid;
    case SCHWAY_STATUS_ERR_EXE_RANGE:
        return text_exe_range;
    default:
        return text_unknown;
    }
}
