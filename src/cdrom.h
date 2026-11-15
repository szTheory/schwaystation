/* Bounded CD-ROM controller state and its device contracts. Not installed. */
#ifndef SCHWAYSTATION_CDROM_H
#define SCHWAYSTATION_CDROM_H

#include <stdint.h>

#include "schwaystation.h"

struct schway__state_io;

/* The controller has 16-byte parameter/result FIFOs. Its internal sector
   buffer is eight raw sectors; all limits follow PSX-SPU-CDROM-AND-MDEC, 2.1
   and 2.3. */
#define SCHWAY__CDROM_FIFO_CAP 16u
#define SCHWAY__CDROM_IRQ_CAP 8u
#define SCHWAY__CDROM_SECTOR_CAP 8u
#define SCHWAY__CDROM_SECTOR_BYTES 2352u
#define SCHWAY__CDROM_AUDIO_CAP 2048u
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
    uint32_t current_lba, target_lba;
    uint32_t sector_head, sector_count, sector_offset;
    uint8_t sector[SCHWAY__CDROM_SECTOR_CAP][SCHWAY__CDROM_SECTOR_BYTES];
    uint32_t sector_lba[SCHWAY__CDROM_SECTOR_CAP];
    uint32_t sector_size[SCHWAY__CDROM_SECTOR_CAP];
    int16_t xa_history[2][SCHWAY__CDROM_XA_TAPS];
    uint32_t xa_phase;
    uint16_t audio[SCHWAY__CDROM_AUDIO_CAP][2];
    uint32_t audio_head, audio_count;
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

#endif /* SCHWAYSTATION_CDROM_H */
