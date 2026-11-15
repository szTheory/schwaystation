/* The bounded CD-ROM owner. Command and sector algorithms are layered on this
   state by their first callers; this slice establishes the owned storage,
   MMIO path and deadline contract (PSX-SPU-CDROM-AND-MDEC, sections 2.1-2.4). */
#include <string.h>

#include "internal.h"
#include "state.h"

#define SCHWAY__CDROM_EVENT_AHEAD UINT64_C(67737600)

static void state_i16_array(struct schway__state_io *io, const char *name, int16_t *values,
                            uint32_t count)
{
    uint16_t raw[SCHWAY__CDROM_XA_TAPS * 2u];
    if (io->mode == SCHWAY__STATE_VISIT) {
        io->visitor(io->user, name, values, count, 2u);
        return;
    }
    for (uint32_t i = 0; i < count; i++) {
        memcpy(&raw[i], &values[i], sizeof raw[i]);
    }
    schway__state_u16_array(io, name, raw, count, 0u, UINT16_MAX);
    if (io->mode == SCHWAY__STATE_APPLY) {
        for (uint32_t i = 0; i < count; i++) {
            memcpy(&values[i], &raw[i], sizeof raw[i]);
        }
    }
}

void schway__cdrom_reset(schway *inst)
{
    memset(&inst->cdrom, 0, sizeof inst->cdrom);
}

/* Every CD deadline is a CPU tick, bounded to two seconds ahead of the state
   clock. Idle deadlines are zero and are ignored unless their event flag is
   set. */
void schway__cdrom_state_io(struct schway__state_io *io, schway *inst, uint64_t ticks)
{
    struct schway__cdrom *cd = &inst->cdrom;
    schway__state_u8_array(io, "cdrom.parameter", cd->parameter, SCHWAY__CDROM_FIFO_CAP, 0u,
                           UINT8_MAX);
    schway__state_u8_array(io, "cdrom.result", cd->result, SCHWAY__CDROM_FIFO_CAP, 0u,
                           UINT8_MAX);
    schway__state_u8_array(io, "cdrom.interrupt", cd->interrupt, SCHWAY__CDROM_IRQ_CAP, 0u,
                           5u);
    uint32_t bank = schway__state_u32(io, "cdrom.bank", &cd->bank, 0u, 3u);
    uint32_t parameter_head = schway__state_u32(io, "cdrom.parameter_head", &cd->parameter_head,
                                                0u, SCHWAY__CDROM_FIFO_CAP - 1u);
    uint32_t parameter_count = schway__state_u32(io, "cdrom.parameter_count",
                                                 &cd->parameter_count, 0u,
                                                 SCHWAY__CDROM_FIFO_CAP);
    uint32_t result_head = schway__state_u32(io, "cdrom.result_head", &cd->result_head, 0u,
                                             SCHWAY__CDROM_FIFO_CAP - 1u);
    uint32_t result_count = schway__state_u32(io, "cdrom.result_count", &cd->result_count, 0u,
                                              SCHWAY__CDROM_FIFO_CAP);
    uint32_t interrupt_head = schway__state_u32(io, "cdrom.interrupt_head", &cd->interrupt_head,
                                                0u, SCHWAY__CDROM_IRQ_CAP - 1u);
    uint32_t interrupt_count = schway__state_u32(io, "cdrom.interrupt_count",
                                                 &cd->interrupt_count, 0u,
                                                 SCHWAY__CDROM_IRQ_CAP);
    uint32_t command = schway__state_u32(io, "cdrom.command", &cd->command, 0u, UINT8_MAX);
    uint32_t command_pending = schway__state_u32(io, "cdrom.command_pending",
                                                &cd->command_pending, 0u, 1u);
    (void)schway__state_u32(io, "cdrom.status", &cd->status, 0u, UINT8_MAX);
    (void)schway__state_u32(io, "cdrom.mode", &cd->mode, 0u, UINT8_MAX);
    (void)schway__state_u32(io, "cdrom.file", &cd->file, 0u, UINT8_MAX);
    (void)schway__state_u32(io, "cdrom.channel", &cd->channel, 0u, UINT8_MAX);
    (void)schway__state_u32(io, "cdrom.last_command", &cd->last_command, 0u, UINT8_MAX);
    uint32_t motor_on = schway__state_u32(io, "cdrom.motor_on", &cd->motor_on, 0u, 1u);
    uint32_t read_active = schway__state_u32(io, "cdrom.read_active", &cd->read_active, 0u, 1u);
    uint32_t play_active = schway__state_u32(io, "cdrom.play_active", &cd->play_active, 0u, 1u);
    uint32_t seek_active = schway__state_u32(io, "cdrom.seek_active", &cd->seek_active, 0u, 1u);
    (void)schway__state_u32(io, "cdrom.current_lba", &cd->current_lba, 0u, UINT32_MAX);
    (void)schway__state_u32(io, "cdrom.target_lba", &cd->target_lba, 0u, UINT32_MAX);
    uint32_t sector_head = schway__state_u32(io, "cdrom.sector_head", &cd->sector_head, 0u,
                                             SCHWAY__CDROM_SECTOR_CAP - 1u);
    uint32_t sector_count = schway__state_u32(io, "cdrom.sector_count", &cd->sector_count, 0u,
                                              SCHWAY__CDROM_SECTOR_CAP);
    uint32_t sector_offset = schway__state_u32(io, "cdrom.sector_offset", &cd->sector_offset, 0u,
                                               SCHWAY__CDROM_SECTOR_BYTES);
    schway__state_u8_array(io, "cdrom.sector", &cd->sector[0][0],
                           SCHWAY__CDROM_SECTOR_CAP * SCHWAY__CDROM_SECTOR_BYTES, 0u, UINT8_MAX);
    schway__state_u32_array(io, "cdrom.sector_lba", cd->sector_lba, SCHWAY__CDROM_SECTOR_CAP,
                            0u, UINT32_MAX);
    schway__state_u32_array(io, "cdrom.sector_size", cd->sector_size, SCHWAY__CDROM_SECTOR_CAP,
                            0u, SCHWAY__CDROM_SECTOR_BYTES);
    state_i16_array(io, "cdrom.xa_history", &cd->xa_history[0][0], 2u * SCHWAY__CDROM_XA_TAPS);
    uint32_t xa_phase = schway__state_u32(io, "cdrom.xa_phase", &cd->xa_phase, 0u, 5u);
    schway__state_u16_array(io, "cdrom.audio", &cd->audio[0][0], 2u * SCHWAY__CDROM_AUDIO_CAP,
                            0u, UINT16_MAX);
    uint32_t audio_head = schway__state_u32(io, "cdrom.audio_head", &cd->audio_head, 0u,
                                            SCHWAY__CDROM_AUDIO_CAP - 1u);
    uint32_t audio_count = schway__state_u32(io, "cdrom.audio_count", &cd->audio_count, 0u,
                                             SCHWAY__CDROM_AUDIO_CAP);
    uint32_t command_event = schway__state_u32(io, "cdrom.command_event", &cd->command_event,
                                               0u, 1u);
    uint32_t sector_event = schway__state_u32(io, "cdrom.sector_event", &cd->sector_event, 0u,
                                              1u);
    uint32_t seek_event = schway__state_u32(io, "cdrom.seek_event", &cd->seek_event, 0u, 1u);
    uint64_t command_deadline = schway__state_u64(io, "cdrom.command_deadline",
                                                  &cd->command_deadline, 0u,
                                                  SCHWAY__STATE_TICKS_MAX);
    uint64_t sector_deadline = schway__state_u64(io, "cdrom.sector_deadline",
                                                 &cd->sector_deadline, 0u,
                                                 SCHWAY__STATE_TICKS_MAX);
    uint64_t seek_deadline = schway__state_u64(io, "cdrom.seek_deadline", &cd->seek_deadline,
                                               0u, SCHWAY__STATE_TICKS_MAX);

    uint64_t max = ticks > SCHWAY__STATE_TICKS_MAX - SCHWAY__CDROM_EVENT_AHEAD
                       ? SCHWAY__STATE_TICKS_MAX
                       : ticks + SCHWAY__CDROM_EVENT_AHEAD;
    if (io->mode == SCHWAY__STATE_VALIDATE) {
        schway__state_check(io, bank <= 3u);
        schway__state_check(io, parameter_head < SCHWAY__CDROM_FIFO_CAP &&
                                   parameter_count <= SCHWAY__CDROM_FIFO_CAP);
        schway__state_check(io, result_head < SCHWAY__CDROM_FIFO_CAP &&
                                   result_count <= SCHWAY__CDROM_FIFO_CAP);
        schway__state_check(io, interrupt_head < SCHWAY__CDROM_IRQ_CAP &&
                                   interrupt_count <= SCHWAY__CDROM_IRQ_CAP);
        schway__state_check(io, sector_head < SCHWAY__CDROM_SECTOR_CAP &&
                                   sector_count <= SCHWAY__CDROM_SECTOR_CAP &&
                                   sector_offset <= SCHWAY__CDROM_SECTOR_BYTES);
        schway__state_check(io, audio_head < SCHWAY__CDROM_AUDIO_CAP &&
                                   audio_count <= SCHWAY__CDROM_AUDIO_CAP && xa_phase < 6u);
        schway__state_check(io, command_pending == 0u || command != 0u);
        schway__state_check(io, seek_active == 0u || motor_on != 0u);
        schway__state_check(io, read_active == 0u || motor_on != 0u);
        schway__state_check(io, !(read_active != 0u && play_active != 0u));
        schway__state_check(io, command_event == 0u
                                   ? command_deadline == 0u
                                   : command_deadline >= ticks && command_deadline <= max);
        schway__state_check(io, sector_event == 0u
                                   ? sector_deadline == 0u
                                   : sector_deadline >= ticks && sector_deadline <= max);
        schway__state_check(io, seek_event == 0u
                                   ? seek_deadline == 0u
                                   : seek_deadline >= ticks && seek_deadline <= max);
        schway__state_check(io, command_event == 0u || command_pending != 0u);
        schway__state_check(io, sector_event == 0u || read_active != 0u || play_active != 0u);
        schway__state_check(io, seek_event == 0u || seek_active != 0u);
    }
}

uint64_t schway__cdrom_next(const schway *inst)
{
    const struct schway__cdrom *cd = &inst->cdrom;
    uint64_t next = UINT64_MAX;
    if (cd->command_event != 0u && cd->command_deadline < next) {
        next = cd->command_deadline;
    }
    if (cd->sector_event != 0u && cd->sector_deadline < next) {
        next = cd->sector_deadline;
    }
    if (cd->seek_event != 0u && cd->seek_deadline < next) {
        next = cd->seek_deadline;
    }
    return next;
}

/* Clear every due phase so one event dispatch always advances the device's
   deadline. Command effects are supplied by the command-model slice. */
void schway__cdrom_event(schway *inst, uint64_t now)
{
    struct schway__cdrom *cd = &inst->cdrom;
    if (cd->command_event != 0u && cd->command_deadline <= now) {
        cd->command_event = 0u;
        cd->command_deadline = 0u;
    }
    if (cd->sector_event != 0u && cd->sector_deadline <= now) {
        cd->sector_event = 0u;
        cd->sector_deadline = 0u;
    }
    if (cd->seek_event != 0u && cd->seek_deadline <= now) {
        cd->seek_event = 0u;
        cd->seek_deadline = 0u;
    }
}

uint8_t schway__cdrom_read(schway *inst, uint32_t address, uint64_t now)
{
    if (schway__cdrom_next(inst) <= now) {
        schway__cdrom_event(inst, now);
    }
    (void)address;
    return 0u;
}

void schway__cdrom_write(schway *inst, uint32_t address, uint8_t value, uint64_t now)
{
    if (schway__cdrom_next(inst) <= now) {
        schway__cdrom_event(inst, now);
    }
    (void)address;
    (void)value;
}

void schway__cdrom_io(schway *inst, uint32_t wr, uint32_t address, uint32_t width,
                      uint32_t *value, uint32_t *cycles)
{
    uint32_t sink = 0u;
    if (cycles == NULL) {
        cycles = &sink;
    }
    uint64_t now = schway__sched_now(inst, *cycles);
    if (wr != 0u) {
        for (uint32_t i = 0u; i < width; i++) {
            schway__cdrom_write(inst, address + i, (uint8_t)(*value >> (i * 8u)), now);
        }
    } else {
        uint32_t result = 0u;
        for (uint32_t i = 0u; i < width; i++) {
            result |= (uint32_t)schway__cdrom_read(inst, address + i, now) << (i * 8u);
        }
        *value = result;
    }
    if (schway__cdrom_next(inst) <= now) {
        schway__cdrom_event(inst, now);
    }
    schway__sched_recalc(inst);
}
