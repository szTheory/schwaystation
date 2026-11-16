/* CD-ROM instance reset and state serialization. */
#include <string.h>

#include "internal.h"
#include "state.h"

#define SCHWAY__CDROM_EVENT_AHEAD UINT64_C(105219072)

static void state_i16_array(struct schway__state_io *io, const char *name, int16_t *values,
                            uint32_t count)
{
    uint16_t raw[SCHWAY__CDROM_XA_TAPS * 2u];
    if (io->mode == SCHWAY__STATE_VISIT) {
        io->visitor(io->user, name, values, count, 2u);
        return;
    }
    for (uint32_t i = 0u; i < count; i++) {
        memcpy(&raw[i], &values[i], sizeof raw[i]);
    }
    schway__state_u16_array(io, name, raw, count, 0u, UINT16_MAX);
    if (io->mode == SCHWAY__STATE_APPLY) {
        for (uint32_t i = 0u; i < count; i++) {
            memcpy(&values[i], &raw[i], sizeof raw[i]);
        }
    }
}

void schway__cdrom_reset(schway *inst)
{
    memset(&inst->cdrom, 0, sizeof inst->cdrom);
    inst->cdrom.drive_speed = 1u;
    inst->cdrom.atv_active[0] = 0x80u;
    inst->cdrom.atv_active[2] = 0x80u;
    inst->cdrom.atv_pending[0] = 0x80u;
    inst->cdrom.atv_pending[2] = 0x80u;
}

static void cdrom_fifo_state_io(struct schway__state_io *io, struct schway__cdrom *cd)
{
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
    (void)schway__state_u32(io, "cdrom.interrupt_mask", &cd->interrupt_mask, 0u, 0x1Fu);
    if (io->mode == SCHWAY__STATE_VALIDATE) {
        schway__state_check(io, bank <= 3u);
        schway__state_check(io, parameter_head < SCHWAY__CDROM_FIFO_CAP &&
                                   parameter_count <= SCHWAY__CDROM_FIFO_CAP);
        schway__state_check(io, result_head < SCHWAY__CDROM_FIFO_CAP &&
                                   result_count <= SCHWAY__CDROM_FIFO_CAP);
        schway__state_check(io, interrupt_head < SCHWAY__CDROM_IRQ_CAP &&
                                   interrupt_count <= SCHWAY__CDROM_IRQ_CAP);
    }
}

struct schway__cdrom_state_values {
    uint32_t command_pending;
    uint32_t read_active;
    uint32_t play_active;
    uint32_t seek_active;
};

static void cdrom_command_state_io(struct schway__state_io *io, struct schway__cdrom *cd,
                                  struct schway__cdrom_state_values *values,
                                  const schway *inst)
{
    uint32_t command = schway__state_u32(io, "cdrom.command", &cd->command, 0u, UINT8_MAX);
    values->command_pending = schway__state_u32(io, "cdrom.command_pending",
                                                &cd->command_pending, 0u, 2u);
    (void)schway__state_u32(io, "cdrom.status", &cd->status, 0u, UINT8_MAX);
    (void)schway__state_u32(io, "cdrom.mode", &cd->mode, 0u, UINT8_MAX);
    (void)schway__state_u32(io, "cdrom.file", &cd->file, 0u, UINT8_MAX);
    (void)schway__state_u32(io, "cdrom.channel", &cd->channel, 0u, UINT8_MAX);
    (void)schway__state_u32(io, "cdrom.last_command", &cd->last_command, 0u, UINT8_MAX);
    uint32_t motor_on = schway__state_u32(io, "cdrom.motor_on", &cd->motor_on, 0u, 1u);
    (void)schway__state_u32(io, "cdrom.bfrd", &cd->bfrd, 0u, 1u);
    (void)schway__state_u32(io, "cdrom.sector_int1_pending", &cd->sector_int1_pending, 0u, 1u);
    (void)schway__state_u32(io, "cdrom.read_s", &cd->read_s, 0u, 1u);
    (void)schway__state_u32(io, "cdrom.sector_retry", &cd->sector_retry, 0u, 1u);
    values->read_active = schway__state_u32(io, "cdrom.read_active", &cd->read_active, 0u, 1u);
    values->play_active = schway__state_u32(io, "cdrom.play_active", &cd->play_active, 0u, 1u);
    values->seek_active = schway__state_u32(io, "cdrom.seek_active", &cd->seek_active, 0u, 1u);
    (void)schway__state_u32(io, "cdrom.paused", &cd->paused, 0u, 1u);
    (void)schway__state_u32(io, "cdrom.muted", &cd->muted, 0u, 1u);
    uint32_t drive_speed = schway__state_u32(io, "cdrom.drive_speed", &cd->drive_speed, 1u, 2u);
    (void)schway__state_u32(io, "cdrom.drive_speed_changed", &cd->drive_speed_changed, 0u, 1u);
    (void)schway__state_u32(io, "cdrom.unlock_enabled", &cd->unlock_enabled, 0u, 1u);
    (void)schway__state_u32(io, "cdrom.unlock_step", &cd->unlock_step, 0u, 6u);
    (void)schway__state_u32(io, "cdrom.current_lba", &cd->current_lba, 0u, UINT32_MAX);
    (void)schway__state_u32(io, "cdrom.target_lba", &cd->target_lba, 0u, UINT32_MAX);
    schway__state_u8_array(io, "cdrom.atv_pending", cd->atv_pending, 4u, 0u, UINT8_MAX);
    schway__state_u8_array(io, "cdrom.atv_active", cd->atv_active, 4u, 0u, UINT8_MAX);
    uint8_t q_candidate[12];
    uint8_t *q_bytes = cd->q_record;
    if (io->mode == SCHWAY__STATE_VALIDATE) {
        memcpy(q_candidate, io->r, sizeof q_candidate);
        q_bytes = q_candidate;
    }
    schway__state_u8_array(io, "cdrom.q_record", q_bytes, 12u, 0u, UINT8_MAX);
    uint32_t q_record_valid = schway__state_u32(io, "cdrom.q_record_valid",
                                                &cd->q_record_valid, 0u, 1u);
    if (io->mode == SCHWAY__STATE_VALIDATE) {
        schway__state_check(io, values->command_pending == 0u || command != 0u);
        schway__state_check(io, drive_speed == 1u || drive_speed == 2u);
        schway__state_check(io, values->seek_active == 0u || motor_on != 0u);
        schway__state_check(io, values->read_active == 0u || motor_on != 0u);
        schway__state_check(io, !(values->read_active != 0u && values->play_active != 0u));
        schway__state_check(io, q_record_valid <= 1u);
        schway__state_check(io, q_record_valid == 0u ||
                                   schway__cdrom_sector_q_valid(inst, q_bytes) != 0u);
    }
}

static void cdrom_buffer_state_io(struct schway__state_io *io, struct schway__cdrom *cd)
{
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
    state_i16_array(io, "cdrom.xa_adpcm_history", &cd->xa_adpcm_history[0][0], 4u);
    uint32_t xa_phase = schway__state_u32(io, "cdrom.xa_phase", &cd->xa_phase, 0u, 5u);
    schway__state_u16_array(io, "cdrom.audio", &cd->audio[0][0], 2u * SCHWAY__CDROM_AUDIO_CAP,
                            0u, UINT16_MAX);
    uint32_t audio_head = schway__state_u32(io, "cdrom.audio_head", &cd->audio_head, 0u,
                                            SCHWAY__CDROM_AUDIO_CAP - 1u);
    uint32_t audio_count = schway__state_u32(io, "cdrom.audio_count", &cd->audio_count, 0u,
                                             SCHWAY__CDROM_AUDIO_LIMIT);
    schway__state_u16_array(io, "cdrom.xa_audio", &cd->xa_audio[0][0],
                            2u * SCHWAY__CDROM_XA_AUDIO_CAP, 0u, UINT16_MAX);
    uint32_t xa_audio_head = schway__state_u32(io, "cdrom.xa_audio_head", &cd->xa_audio_head,
                                               0u, SCHWAY__CDROM_XA_AUDIO_CAP - 1u);
    uint32_t xa_audio_count = schway__state_u32(io, "cdrom.xa_audio_count", &cd->xa_audio_count,
                                                0u, SCHWAY__CDROM_XA_AUDIO_CAP);
    if (io->mode == SCHWAY__STATE_VALIDATE) {
        schway__state_check(io, sector_head < SCHWAY__CDROM_SECTOR_CAP &&
                                   sector_count <= SCHWAY__CDROM_SECTOR_CAP &&
                                   sector_offset <= SCHWAY__CDROM_SECTOR_BYTES);
        schway__state_check(io, audio_head < SCHWAY__CDROM_AUDIO_CAP &&
                                   audio_count <= SCHWAY__CDROM_AUDIO_LIMIT && xa_phase < 6u &&
                                   xa_audio_head < SCHWAY__CDROM_XA_AUDIO_CAP &&
                                   xa_audio_count <= SCHWAY__CDROM_XA_AUDIO_CAP);
    }
}

static void cdrom_event_state_io(struct schway__state_io *io, struct schway__cdrom *cd,
                                 const struct schway__cdrom_state_values *values,
                                 uint64_t ticks)
{
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
                       ? SCHWAY__STATE_TICKS_MAX : ticks + SCHWAY__CDROM_EVENT_AHEAD;
    if (io->mode == SCHWAY__STATE_VALIDATE) {
        schway__state_check(io, command_event == 0u
                                   ? command_deadline == 0u
                                   : command_deadline >= ticks && command_deadline <= max);
        schway__state_check(io, sector_event == 0u
                                   ? sector_deadline == 0u
                                   : sector_deadline >= ticks && sector_deadline <= max);
        schway__state_check(io, seek_event == 0u
                                   ? seek_deadline == 0u
                                   : seek_deadline >= ticks && seek_deadline <= max);
        schway__state_check(io, command_event == 0u || values->command_pending != 0u);
        schway__state_check(io, values->command_pending != 2u || command_event != 0u || seek_event != 0u);
        schway__state_check(io, sector_event == 0u || values->read_active != 0u ||
                                   values->play_active != 0u);
        schway__state_check(io, seek_event == 0u || values->seek_active != 0u);
    }
}

void schway__cdrom_state_io(struct schway__state_io *io, schway *inst, uint64_t ticks)
{
    struct schway__cdrom *cd = &inst->cdrom;
    struct schway__cdrom_state_values values = {0u};
    cdrom_fifo_state_io(io, cd);
    cdrom_command_state_io(io, cd, &values, inst);
    cdrom_buffer_state_io(io, cd);
    cdrom_event_state_io(io, cd, &values, ticks);
}
