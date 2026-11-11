/* Controller input and its serial transaction. Not installed. */
#ifndef SCHWAYSTATION_PAD_H
#define SCHWAYSTATION_PAD_H

#include <stdint.h>

#include "schwaystation.h"

struct schway__state_io;

struct schway__pad {
    uint16_t buttons[2];
    uint8_t axes[2][4];
    uint8_t present[2];
    uint8_t device[2];
    uint8_t phase[2];
    uint8_t command[2];
    uint8_t data_index[2];
    uint8_t config_mode[2];
    uint8_t response_mode[2];
    uint8_t analog_mode[2];
    uint8_t analog_button[2];
    uint8_t analog_lock[2];
    uint8_t selector[2];
    uint8_t old_rumble[2];
    uint8_t rumble_locked[2];
    uint8_t rumble_map_valid[2];
    uint8_t rumble_large[2];
    uint8_t rumble_small[2];
    uint8_t rumble_stage_large[2];
    uint8_t rumble_stage_small[2];
    uint8_t rumble_map[2][6];
    uint8_t rumble_map_stage[2][6];
    uint8_t frame_polls;
    uint8_t frame_latched;
    uint64_t watchdog_deadline[2];
    uint64_t transfers;
    schway_input pending_input; /* host-owned input is deliberately not saved */
    uint8_t pending_valid;
};

void schway__pad_reset(schway *inst);
void schway__pad_abort(schway *inst, uint32_t port);
void schway__pad_set_buttons(schway *inst, uint32_t port, uint16_t buttons);
void schway__pad_set_connected(schway *inst, uint32_t port, uint32_t connected);
void schway__pad_frame_start(schway *inst);
void schway__pad_frame_end(schway *inst);
void schway__pad_update_watchdog(schway *inst, uint64_t now);
void schway__pad_state_io(struct schway__state_io *io, schway *inst);
uint32_t schway__pad_byte(schway *inst, uint32_t port, uint8_t sent, uint64_t now,
                          uint8_t *received);

#endif /* SCHWAYSTATION_PAD_H */
