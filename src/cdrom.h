/* Bounded CD-ROM controller state and its device contracts. Not installed. */
#ifndef SCHWAYSTATION_CDROM_H
#define SCHWAYSTATION_CDROM_H

#include <stdint.h>

#include "schwaystation.h"
#include "cdxa.h"

struct schway__state_io;

/* The controller has 16-byte parameter/result FIFOs. Its internal sector
   buffer is eight raw sectors; all limits follow the pinned CD-ROM hardware record (SCM.02). */
#define SCHWAY__CDROM_FIFO_CAP 16u
#define SCHWAY__CDROM_IRQ_CAP 8u
#define SCHWAY__CDROM_SECTOR_CAP 8u
#define SCHWAY__CDROM_SECTOR_BYTES 2352u
#define SCHWAY__CDROM_AUDIO_CAP 2048u
#define SCHWAY__CDROM_AUDIO_LIMIT (2u * 588u)
#define SCHWAY__CDROM_XA_AUDIO_CAP (SCHWAY__CDXA_MAX_OUTPUT_FRAMES + 1u)
#define SCHWAY__CDROM_XA_TAPS 29u

struct schway__cdrom {
    uint8_t parameter[SCHWAY__CDROM_FIFO_CAP];
    uint8_t result[SCHWAY__CDROM_FIFO_CAP];
    uint8_t interrupt[SCHWAY__CDROM_IRQ_CAP];
    uint32_t bank;
    uint32_t parameter_head, parameter_count;
    uint32_t result_head, result_count;
    uint32_t interrupt_head, interrupt_count;
    uint32_t interrupt_mask;
    uint32_t command, command_pending;
    uint32_t status, mode, file, channel, last_command;
    uint32_t motor_on, read_active, play_active, seek_active;
    uint32_t paused, muted, drive_speed, drive_speed_changed, unlock_enabled, unlock_step;
    uint32_t bfrd, sector_int1_pending, read_s, sector_retry;
    uint32_t current_lba, target_lba;
    uint8_t atv_pending[4], atv_active[4];
    uint8_t q_record[12];
    uint32_t q_record_valid;
    uint32_t sector_head, sector_count, sector_offset;
    uint8_t sector[SCHWAY__CDROM_SECTOR_CAP][SCHWAY__CDROM_SECTOR_BYTES];
    uint32_t sector_lba[SCHWAY__CDROM_SECTOR_CAP];
    uint32_t sector_size[SCHWAY__CDROM_SECTOR_CAP];
    int16_t xa_history[2][SCHWAY__CDROM_XA_TAPS];
    int16_t xa_adpcm_history[2][2];
    uint32_t xa_phase;
    uint16_t audio[SCHWAY__CDROM_AUDIO_CAP][2];
    uint32_t audio_head, audio_count;
    uint16_t xa_audio[SCHWAY__CDROM_XA_AUDIO_CAP][2];
    uint32_t xa_audio_head, xa_audio_count;
    uint32_t command_event, sector_event, seek_event;
    uint64_t command_deadline, sector_deadline, seek_deadline;
};

void schway__cdrom_reset(schway *inst);
void schway__cdrom_lid_changed(schway *inst);
void schway__cdrom_state_io(struct schway__state_io *io, schway *inst, uint64_t ticks);

/* Read/write one byte at a CD-ROM port. The I/O wrapper preserves byte lanes
   for wider bus accesses and catches due events before touching registers. */
uint8_t schway__cdrom_read(schway *inst, uint32_t address, uint64_t now);
void schway__cdrom_write(schway *inst, uint32_t address, uint8_t value, uint64_t now);
void schway__cdrom_io(schway *inst, uint32_t wr, uint32_t address, uint32_t width,
                      uint32_t *value, uint32_t *cycles);

uint64_t schway__cdrom_next(const schway *inst);
void schway__cdrom_event(schway *inst, uint64_t now);
uint8_t schway__cdrom_sector_read(schway *inst);
void schway__cdrom_sector_schedule(schway *inst, uint64_t now);
void schway__cdrom_sector_event(schway *inst, uint64_t now);
void schway__cdrom_audio_push(schway *inst, int16_t left, int16_t right);
void schway__cdrom_audio_pop(schway *inst, int16_t *left, int16_t *right);
void schway__cdrom_audio_cdda_sector(schway *inst, const uint8_t *sector);
void schway__cdrom_xa_audio_push(schway *inst, int16_t left, int16_t right);
void schway__cdrom_xa_audio_pop(schway *inst, int16_t *left, int16_t *right);
uint32_t schway__cdrom_sector_track_type_at(const schway *inst, uint32_t lba);
void schway__cdrom_sector_discard_seek_headers(schway *inst);
void schway__cdrom_bfrd_set(schway *inst, uint8_t value, uint64_t now);
void schway__cdrom_sector_seek(schway *inst);
void schway__cdrom_sector_getlocp(schway *inst, uint8_t bytes[8]);
uint32_t schway__cdrom_sector_q_valid(const schway *inst, const uint8_t q[12]);
void schway__cdrom_sector_ack(schway *inst);
void schway__cdrom_push_interrupt(schway *inst, uint8_t code);

#endif /* SCHWAYSTATION_CDROM_H */
