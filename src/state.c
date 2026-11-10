/* Save states and the public field visitor (ARCHITECTURE section 5).

   Layout. A 20-byte header, then the body:
     header  magic (8 bytes), state format (u32), total size (u32), body
             CRC-32 (u32)
     body    instance.frame, instance.region, instance.ticks,
             instance.tick_acc, instance.frame_end, GPU and VRAM, CPU and cop0
             registers, the geometry engine, instance.profile_flags,
             instance.exe_loaded, the bus (RAM, scratchpad, memory control,
             unbuilt-device counters), irq, dma, the root counters and the
             first-fault record
   One field list drives every pass (state.h), so they cannot drift apart. A
   struct is never copied as a whole: padding, pointers and the allocator
   never reach the bytes. The audio ring is not saved; a load empties it.
   The body CRC-32 covers every byte after the header. */
#include <string.h>

#include "state.h"

#define H_FORMAT 8u
#define H_TOTAL 12u
#define H_BODY_CRC 16u

/* PNG-style magic: a high-bit byte, "PSX", CR LF, DOS end of file, LF. A
   text-mode transfer that strips the high bit or rewrites line ends breaks
   it. */
static const uint8_t state_magic[8] = {0x8Au, 0x50u, 0x53u, 0x58u, 0x0Du, 0x0Au, 0x1Au, 0x0Au};

/* Field names handed to the visitor; static, so a host may keep them. */
static const char name_frame[] = "instance.frame";
static const char name_region[] = "instance.region";
static const char name_ticks[] = "instance.ticks";
static const char name_tick_acc[] = "instance.tick_acc";
static const char name_frame_end[] = "instance.frame_end";
static const char name_profile_flags[] = "instance.profile_flags";
static const char name_exe_loaded[] = "instance.exe_loaded";
static const char name_fault_kind[] = "fault.kind";
static const char name_fault_device[] = "fault.device";
static const char name_fault_width[] = "fault.width";
static const char name_fault_note[] = "fault.note";
static const char name_fault_frame[] = "fault.frame";
static const char name_fault_ticks[] = "fault.ticks";
static const char name_fault_line[] = "fault.line";
static const char name_fault_dot[] = "fault.dot";
static const char name_fault_pc[] = "fault.pc";
static const char name_fault_opcode[] = "fault.opcode";
static const char name_fault_addr[] = "fault.addr";
static const char name_header[] = "header";

/* One header field whose only valid value is v. */
static void fixed(struct schway__state_io *io, uint64_t v, uint32_t width)
{
    (void)schway__state_field_io(io, name_header, v, width, v, v);
}

static void header_io(struct schway__state_io *io, uint32_t total)
{
    for (uint32_t i = 0; i < sizeof state_magic; i++) {
        fixed(io, state_magic[i], 1u);
    }
    fixed(io, SCHWAY__STATE_FORMAT, 4u);
    fixed(io, total, 4u);
    /* Checked against the body before VALIDATE runs; any value passes here. */
    (void)schway__state_field_io(io, name_header, 0u, 4u, 0u, UINT32_MAX);
}

/* The first-fault record: the kind and device are the known ones. */
static void fault_io(struct schway__state_io *io, struct schway__fault *f)
{
    (void)schway__state_u32(io, name_fault_kind, &f->kind, SCHWAY__FAULT_NONE,
                            SCHWAY__FAULT_DMA);
    (void)schway__state_u32(io, name_fault_device, &f->device, 0u, SCHWAY__DEV_COUNT - 1u);
    (void)schway__state_u32(io, name_fault_width, &f->width, 0u, 4u);
    (void)schway__state_u32(io, name_fault_note, &f->note, 0u, UINT32_MAX);
    (void)schway__state_u64(io, name_fault_frame, &f->frame, 0u, SCHWAY__STATE_FRAME_MAX);
    (void)schway__state_u64(io, name_fault_ticks, &f->ticks, 0u, SCHWAY__STATE_TICKS_MAX);
    (void)schway__state_u32(io, name_fault_line, &f->line, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_fault_dot, &f->dot, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_fault_pc, &f->pc, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_fault_opcode, &f->opcode, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_fault_addr, &f->addr, 0u, UINT32_MAX);
}

static void body_io(struct schway__state_io *io, schway *inst)
{
    uint64_t den = schway__instance_frame_den(inst->region);
    (void)schway__state_u64(io, name_frame, &inst->frame, 0u, SCHWAY__STATE_FRAME_MAX);
    /* A state loads only into an instance of the same region. */
    (void)schway__state_u32(io, name_region, &inst->region, inst->region, inst->region);
    uint64_t ticks = schway__state_u64(io, name_ticks, &inst->ticks, 0u, SCHWAY__STATE_TICKS_MAX);
    (void)schway__state_u64(io, name_tick_acc, &inst->tick_acc, 0u, den - 1u);
    /* The frame ends at or after the tick the state is at, and within one
       frame of it, so a run from a loaded state is bounded. */
    uint64_t frame_end =
        schway__state_u64(io, name_frame_end, &inst->frame_end, 0u,
                          SCHWAY__STATE_TICKS_MAX + schway__instance_frame_max(inst->region));
    schway__state_check(io, ticks <= frame_end &&
                                frame_end - ticks <= schway__instance_frame_max(inst->region));
    schway__gpu_state_io(io, inst);
    uint64_t cycle = schway__cpu_state_io(io, inst);
    schway__gte_state_io(io, inst, cycle);
    uint32_t profile_flags = schway__state_u32(io, name_profile_flags, &inst->profile_flags, 0u,
                                                7u);
    schway__state_check(io, (profile_flags & ~(SCHWAY_PROFILE_DEBUG_PORT |
                                                SCHWAY_PROFILE_GPU_V0)) == 0u);
    (void)schway__state_u32(io, name_exe_loaded, &inst->exe_loaded, 0u, 1u);
    schway__bus_state_io(io, inst);
    schway__state_check(io, ((io->cpu_cause & 0x00000400u) != 0u) ==
                                ((io->irq_stat & io->irq_mask & SCHWAY__IRQ_BITS) != 0u));
    schway__dma_state_io(io, inst, ticks);
    schway__timers_state_io(io, inst);
    fault_io(io, &inst->fault);
}

/* The whole state, in the order the format fixes. */
static void state_io(struct schway__state_io *io, schway *inst, uint32_t total)
{
    header_io(io, total);
    body_io(io, inst);
}

/* SIZE, WRITE, RECORD and VISIT only read through the field pointers, and an
   instance is never a const object, so walking a const instance is defined.
   The pointer is copied, not cast, to say so once. */
static schway *walkable(const schway *inst)
{
    schway *w;
    memcpy(&w, &inst, sizeof w);
    return w;
}

static uint32_t get_u32(const uint8_t *b)
{
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) |
           ((uint32_t)b[3] << 24);
}

static uint32_t body_crc(const uint8_t *b, size_t size)
{
    return schway__crc32_update(0u, b + SCHWAY__STATE_HEADER_SIZE,
                                size - SCHWAY__STATE_HEADER_SIZE);
}

size_t schway_state_size(const schway *inst)
{
    if (inst == NULL) {
        return 0u;
    }
    struct schway__state_io io;
    memset(&io, 0, sizeof io);
    io.mode = SCHWAY__STATE_SIZE;
    state_io(&io, walkable(inst), 0u);
    return io.n;
}

size_t schway__state_record(const schway *inst, struct schway__state_field *out, size_t cap)
{
    size_t size = schway_state_size(inst);
    if (size == 0u) {
        return 0u;
    }
    struct schway__state_io io;
    memset(&io, 0, sizeof io);
    io.mode = SCHWAY__STATE_RECORD;
    io.rec = out;
    io.rec_cap = cap;
    state_io(&io, walkable(inst), (uint32_t)size);
    return io.rec_n;
}

void schway__state_seal(void *buf, size_t size)
{
    uint8_t *b = (uint8_t *)buf;
    if (b == NULL || size < SCHWAY__STATE_HEADER_SIZE) {
        return;
    }
    uint32_t crc = body_crc(b, size);
    for (uint32_t i = 0; i < 4u; i++) {
        b[H_BODY_CRC + i] = (uint8_t)(crc >> (8u * i));
    }
}

schway_status schway_save_state(const schway *inst, void *buf, size_t size)
{
    if (inst == NULL || buf == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    size_t need = schway_state_size(inst);
    if (size < need) {
        return SCHWAY_STATUS_ERR_BUFFER_TOO_SMALL;
    }
    struct schway__state_io io;
    memset(&io, 0, sizeof io);
    io.mode = SCHWAY__STATE_WRITE;
    io.w = (uint8_t *)buf;
    state_io(&io, walkable(inst), (uint32_t)need);
    schway__state_seal(buf, need);
    return SCHWAY_STATUS_OK;
}

schway_status schway_load_state(schway *inst, const void *buf, size_t size)
{
    const uint8_t *b = (const uint8_t *)buf;
    if (inst == NULL || b == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    if (size < sizeof state_magic || memcmp(b, state_magic, sizeof state_magic) != 0) {
        return SCHWAY_STATUS_ERR_STATE_MAGIC;
    }
    if (size < SCHWAY__STATE_HEADER_SIZE) {
        return SCHWAY_STATUS_ERR_STATE_FIELD;
    }
    if (get_u32(b + H_FORMAT) != SCHWAY__STATE_FORMAT) {
        return SCHWAY_STATUS_ERR_STATE_VERSION;
    }
    size_t need = schway_state_size(inst);
    if (size != need || get_u32(b + H_TOTAL) != (uint32_t)need) {
        return SCHWAY_STATUS_ERR_STATE_FIELD;
    }
    if (get_u32(b + H_BODY_CRC) != body_crc(b, size)) {
        return SCHWAY_STATUS_ERR_STATE_CRC;
    }
    /* VALIDATE walks the same fields over the same bytes that APPLY reads
       and writes nothing, so a refused state has changed nothing. */
    struct schway__state_io io;
    memset(&io, 0, sizeof io);
    io.mode = SCHWAY__STATE_VALIDATE;
    io.r = b;
    state_io(&io, inst, (uint32_t)need);
    if (io.bad) {
        return SCHWAY_STATUS_ERR_STATE_FIELD;
    }
    /* APPLY cannot fail. */
    io.mode = SCHWAY__STATE_APPLY;
    io.r = b;
    state_io(&io, inst, (uint32_t)need);
    schway__sched_sync(inst);
    schway__sched_recalc(inst);
    schway__frame_audio_clear(inst);
    return SCHWAY_STATUS_OK;
}

schway_status schway_state_visit(const schway *inst, schway_state_visitor visitor, void *user)
{
    if (inst == NULL || visitor == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    struct schway__state_io io;
    memset(&io, 0, sizeof io);
    io.mode = SCHWAY__STATE_VISIT;
    io.visitor = visitor;
    io.user = user;
    body_io(&io, walkable(inst));
    return SCHWAY_STATUS_OK;
}
