/* Controller input and its serial transaction. Not installed. */
#ifndef SCHWAYSTATION_PAD_H
#define SCHWAYSTATION_PAD_H

#include <stdint.h>

#include "schwaystation.h"

struct schway__state_io;

enum { SCHWAY__PAD_ENDPOINT_COUNT = 8, SCHWAY__PAD_RUMBLE_MAP_SIZE = 48 };

struct schway__pad {
    uint16_t buttons[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t axes[4u * SCHWAY__PAD_ENDPOINT_COUNT];
    uint16_t mouse_dx[SCHWAY__PAD_ENDPOINT_COUNT]; /* signed input + 32768 */
    uint16_t mouse_dy[SCHWAY__PAD_ENDPOINT_COUNT]; /* signed input + 32768 */
    uint8_t negcon_twist[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t negcon_i[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t negcon_ii[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t negcon_l[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t mouse_startup_ack[SCHWAY__PAD_ENDPOINT_COUNT];
    uint16_t jogcon_dial[SCHWAY__PAD_ENDPOINT_COUNT];
    uint16_t jogcon_flags[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t present[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t device[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t phase[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t command[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t data_index[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t config_mode[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t response_mode[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t analog_mode[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t analog_button[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t analog_lock[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t selector[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t old_rumble[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t rumble_locked[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t rumble_map_valid[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t rumble_large[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t rumble_small[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t rumble_stage_large[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t rumble_stage_small[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t rumble_map[SCHWAY__PAD_RUMBLE_MAP_SIZE];
    uint8_t rumble_map_stage[SCHWAY__PAD_RUMBLE_MAP_SIZE];
    uint8_t multitap[2];
    uint8_t frame_polls;
    uint8_t frame_latched;
    uint64_t watchdog_deadline[SCHWAY__PAD_ENDPOINT_COUNT];
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
uint32_t schway__pad_mouse_ack_low(const schway *inst);

#endif /* SCHWAYSTATION_PAD_H */
