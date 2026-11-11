/* Pad replies and rumble follow .planning/preparation/PSX-PERIPHERALS-AND-SAVES.md. */
#include "internal.h"

#include <string.h>

#include "state.h"

#define PAD_WATCHDOG_TICKS UINT64_C(33868800)

enum { PAD_IDLE, PAD_COMMAND, PAD_ID_END, PAD_PAYLOAD };

static const char name_buttons[] = "pad.buttons";
static const char name_axes[] = "pad.axes";
static const char name_present[] = "pad.present";
static const char name_device[] = "pad.device";
static const char name_phase[] = "pad.phase";
static const char name_command[] = "pad.command";
static const char name_data_index[] = "pad.data_index";
static const char name_config_mode[] = "pad.config_mode";
static const char name_response_mode[] = "pad.response_mode";
static const char name_analog_mode[] = "pad.analog_mode";
static const char name_analog_button[] = "pad.analog_button";
static const char name_analog_lock[] = "pad.analog_lock";
static const char name_selector[] = "pad.selector";
static const char name_old_rumble[] = "pad.old_rumble";
static const char name_rumble_locked[] = "pad.rumble_locked";
static const char name_rumble_map_valid[] = "pad.rumble_map_valid";
static const char name_rumble_large[] = "pad.rumble_large";
static const char name_rumble_small[] = "pad.rumble_small";
static const char name_rumble_stage_large[] = "pad.rumble_stage_large";
static const char name_rumble_stage_small[] = "pad.rumble_stage_small";
static const char name_rumble_map[] = "pad.rumble_map";
static const char name_rumble_map_stage[] = "pad.rumble_map_stage";
static const char name_frame_polls[] = "pad.frame_polls";
static const char name_frame_latched[] = "pad.frame_latched";
static const char name_watchdog0[] = "pad.watchdog0";
static const char name_watchdog1[] = "pad.watchdog1";
static const char name_transfers[] = "pad.transfers";

static uint64_t deadline_after(uint64_t now, uint64_t delta)
{
    return now > SCHWAY__STATE_TICKS_MAX - delta ? SCHWAY__STATE_TICKS_MAX : now + delta;
}

void schway__pad_reset(schway *inst)
{
    memset(&inst->pad, 0, sizeof inst->pad);
    for (uint32_t port = 0u; port < 2u; port++) {
        inst->pad.buttons[port] = UINT16_MAX;
        inst->pad.present[port] = 1u;
        inst->pad.device[port] = SCHWAY_INPUT_DEVICE_DIGITAL;
        inst->pad.rumble_map_valid[port] = 1u;
        for (uint32_t i = 0u; i < 6u; i++) {
            inst->pad.rumble_map[port][i] = 0xFFu;
            inst->pad.rumble_map_stage[port][i] = 0xFFu;
        }
    }
}

void schway__pad_abort(schway *inst, uint32_t port)
{
    if (port < 2u) {
        inst->pad.phase[port] = PAD_IDLE;
    } else {
        inst->pad.phase[0] = PAD_IDLE;
        inst->pad.phase[1] = PAD_IDLE;
    }
}

void schway__pad_set_buttons(schway *inst, uint32_t port, uint16_t buttons)
{
    if (port < 2u) {
        inst->pad.buttons[port] = buttons;
    }
}

void schway__pad_set_connected(schway *inst, uint32_t port, uint32_t connected)
{
    if (port < 2u) {
        inst->pad.present[port] = connected != 0u ? 1u : 0u;
        inst->pad.device[port] = connected != 0u ? SCHWAY_INPUT_DEVICE_DIGITAL
                                                 : SCHWAY_INPUT_DEVICE_NONE;
        if (inst->pad.present[port] == 0u) {
            inst->pad.phase[port] = PAD_IDLE;
        }
    }
}

void schway__pad_frame_start(schway *inst)
{
    if (inst->pad.frame_latched != 0u) {
        return;
    }
    inst->pad.frame_polls = 0u;
    if (inst->pad.pending_valid != 0u) {
        for (uint32_t port = 0u; port < 2u; port++) {
            uint32_t device = inst->pad.pending_input.device[port];
            uint8_t pressed = inst->pad.pending_input.analog_button[port];
            if (device != inst->pad.device[port]) {
                if (device == SCHWAY_INPUT_DEVICE_DUALSHOCK) {
                    inst->pad.analog_mode[port] = 1u;
                    inst->pad.analog_lock[port] = 0u;
                    inst->pad.rumble_locked[port] = 0u;
                    inst->pad.old_rumble[port] = 1u;
                } else {
                    inst->pad.analog_mode[port] = 0u;
                    inst->pad.config_mode[port] = 0u;
                    inst->pad.watchdog_deadline[port] = 0u;
                    inst->pad.old_rumble[port] = 0u;
                    inst->pad.rumble_large[port] = 0u;
                    inst->pad.rumble_small[port] = 0u;
                }
                inst->pad.device[port] = (uint8_t)device;
                inst->pad.present[port] = device != SCHWAY_INPUT_DEVICE_NONE ? 1u : 0u;
                inst->pad.phase[port] = PAD_IDLE;
            }
            uint8_t prior_pressed = inst->pad.analog_button[port];
            if (device == SCHWAY_INPUT_DEVICE_DUALSHOCK && prior_pressed == 0u &&
                pressed != 0u && inst->pad.analog_lock[port] == 0u) {
                inst->pad.analog_mode[port] ^= 1u;
            }
            inst->pad.analog_button[port] = pressed;
            inst->pad.buttons[port] = (uint16_t)~inst->pad.pending_input.buttons[port];
            memcpy(inst->pad.axes[port], inst->pad.pending_input.axes[port],
                   sizeof inst->pad.axes[port]);
        }
        inst->pad.pending_valid = 0u;
    }
    inst->pad.frame_latched = 1u;
}

void schway__pad_frame_end(schway *inst)
{
    inst->pad.frame_latched = 0u;
}

void schway__pad_update_watchdog(schway *inst, uint64_t now)
{
    for (uint32_t port = 0u; port < 2u; port++) {
        if (inst->pad.config_mode[port] != 0u &&
            inst->pad.watchdog_deadline[port] != 0u &&
            now >= inst->pad.watchdog_deadline[port]) {
            inst->pad.config_mode[port] = 0u;
            inst->pad.analog_mode[port] = 0u;
            inst->pad.rumble_locked[port] = 1u;
            inst->pad.rumble_large[port] = 0u;
            inst->pad.rumble_small[port] = 0u;
            inst->pad.watchdog_deadline[port] = 0u;
        }
    }
}

void schway__pad_state_io(struct schway__state_io *io, schway *inst)
{
    uint8_t saved_present[2] = {inst->pad.present[0], inst->pad.present[1]};
    uint8_t saved_device[2] = {inst->pad.device[0], inst->pad.device[1]};
    uint8_t saved_config[2] = {inst->pad.config_mode[0], inst->pad.config_mode[1]};
    uint8_t saved_map[12];
    uint8_t saved_map_stage[12];
    schway__state_u16_array(io, name_buttons, inst->pad.buttons, 2u, 0u, UINT16_MAX);
    schway__state_u8_array(io, name_axes, &inst->pad.axes[0][0], 8u, 0u, UINT8_MAX);
    const uint8_t *read_bytes = io->mode == SCHWAY__STATE_VALIDATE ? io->r : NULL;
    schway__state_u8_array(io, name_present, inst->pad.present, 2u, 0u, 1u);
    if (read_bytes != NULL) {
        memcpy(saved_present, read_bytes, sizeof saved_present);
    }
    read_bytes = io->mode == SCHWAY__STATE_VALIDATE ? io->r : NULL;
    schway__state_u8_array(io, name_device, inst->pad.device, 2u, SCHWAY_INPUT_DEVICE_NONE,
                           SCHWAY_INPUT_DEVICE_DUALSHOCK);
    if (read_bytes != NULL) {
        memcpy(saved_device, read_bytes, sizeof saved_device);
    }
    schway__state_u8_array(io, name_phase, inst->pad.phase, 2u, PAD_IDLE, PAD_PAYLOAD);
    schway__state_u8_array(io, name_command, inst->pad.command, 2u, 0u, UINT8_MAX);
    schway__state_u8_array(io, name_data_index, inst->pad.data_index, 2u, 0u, 6u);
    read_bytes = io->mode == SCHWAY__STATE_VALIDATE ? io->r : NULL;
    schway__state_u8_array(io, name_config_mode, inst->pad.config_mode, 2u, 0u, 1u);
    if (read_bytes != NULL) {
        memcpy(saved_config, read_bytes, sizeof saved_config);
    }
    schway__state_u8_array(io, name_response_mode, inst->pad.response_mode, 2u, 0u, 1u);
    schway__state_u8_array(io, name_analog_mode, inst->pad.analog_mode, 2u, 0u, 1u);
    schway__state_u8_array(io, name_analog_button, inst->pad.analog_button, 2u, 0u, 1u);
    schway__state_u8_array(io, name_analog_lock, inst->pad.analog_lock, 2u, 0u, 1u);
    schway__state_u8_array(io, name_selector, inst->pad.selector, 2u, 0u, UINT8_MAX);
    schway__state_u8_array(io, name_old_rumble, inst->pad.old_rumble, 2u, 0u, 1u);
    schway__state_u8_array(io, name_rumble_locked, inst->pad.rumble_locked, 2u, 0u, 1u);
    schway__state_u8_array(io, name_rumble_map_valid, inst->pad.rumble_map_valid, 2u, 0u, 1u);
    schway__state_u8_array(io, name_rumble_large, inst->pad.rumble_large, 2u, 0u, UINT8_MAX);
    schway__state_u8_array(io, name_rumble_small, inst->pad.rumble_small, 2u, 0u, 1u);
    schway__state_u8_array(io, name_rumble_stage_large, inst->pad.rumble_stage_large, 2u, 0u,
                           UINT8_MAX);
    schway__state_u8_array(io, name_rumble_stage_small, inst->pad.rumble_stage_small, 2u, 0u, 1u);
    read_bytes = io->mode == SCHWAY__STATE_VALIDATE ? io->r : NULL;
    schway__state_u8_array(io, name_rumble_map, &inst->pad.rumble_map[0][0], 12u, 0u,
                           UINT8_MAX);
    if (read_bytes != NULL) {
        memcpy(saved_map, read_bytes, sizeof saved_map);
    } else {
        memcpy(saved_map, &inst->pad.rumble_map[0][0], sizeof saved_map);
    }
    read_bytes = io->mode == SCHWAY__STATE_VALIDATE ? io->r : NULL;
    schway__state_u8_array(io, name_rumble_map_stage, &inst->pad.rumble_map_stage[0][0], 12u, 0u,
                           UINT8_MAX);
    if (read_bytes != NULL) {
        memcpy(saved_map_stage, read_bytes, sizeof saved_map_stage);
    } else {
        memcpy(saved_map_stage, &inst->pad.rumble_map_stage[0][0], sizeof saved_map_stage);
    }
    schway__state_u8_array(io, name_frame_polls, &inst->pad.frame_polls, 1u, 0u, 3u);
    schway__state_u8_array(io, name_frame_latched, &inst->pad.frame_latched, 1u, 0u, 1u);
    uint64_t saved_watchdog[2];
    saved_watchdog[0] = schway__state_u64(io, name_watchdog0,
                                          &inst->pad.watchdog_deadline[0], 0u,
                                          SCHWAY__STATE_TICKS_MAX);
    saved_watchdog[1] = schway__state_u64(io, name_watchdog1,
                                          &inst->pad.watchdog_deadline[1], 0u,
                                          SCHWAY__STATE_TICKS_MAX);
    (void)schway__state_u64(io, name_transfers, &inst->pad.transfers, 0u,
                            SCHWAY__STATE_TICKS_MAX);
    for (uint32_t port = 0u; port < 2u; port++) {
        for (uint32_t i = 0u; i < 6u; i++) {
            uint8_t value = saved_map[port * 6u + i];
            uint8_t staged = saved_map_stage[port * 6u + i];
            schway__state_check(io, value == 0u || value == 1u || value == 0xFFu);
            schway__state_check(io, staged == 0u || staged == 1u || staged == 0xFFu);
        }
        schway__state_check(io, saved_present[port] ==
                                    (saved_device[port] != SCHWAY_INPUT_DEVICE_NONE ? 1u : 0u));
        schway__state_check(io, saved_config[port] == 0u ||
                                    (saved_device[port] == SCHWAY_INPUT_DEVICE_DUALSHOCK &&
                                     saved_watchdog[port] != 0u));
        schway__state_check(io, saved_config[port] != 0u || saved_watchdog[port] == 0u);
    }
}

static uint8_t normal_id(const struct schway__pad *pad, uint32_t port)
{
    if (pad->device[port] == SCHWAY_INPUT_DEVICE_DUALSHOCK &&
        pad->analog_mode[port] != 0u) {
        return 0x73u;
    }
    return 0x41u;
}

static uint8_t poll_size(const struct schway__pad *pad, uint32_t port)
{
    return pad->config_mode[port] != 0u ||
                   (pad->device[port] == SCHWAY_INPUT_DEVICE_DUALSHOCK &&
                    pad->analog_mode[port] != 0u)
               ? 6u
               : 2u;
}

static void refresh_watchdog(schway *inst, uint32_t port, uint64_t now)
{
    if (inst->pad.config_mode[port] != 0u) {
        inst->pad.watchdog_deadline[port] = deadline_after(now, PAD_WATCHDOG_TICKS);
    }
}

static uint8_t poll_reply(const struct schway__pad *pad, uint32_t port, uint32_t index)
{
    if (index == 0u) {
        return (uint8_t)(pad->buttons[port] & 0xFFu);
    }
    if (index == 1u) {
        return (uint8_t)(pad->buttons[port] >> 8);
    }
    return pad->axes[port][index - 2u];
}

static uint8_t command_reply(const struct schway__pad *pad, uint32_t port, uint32_t index)
{
    switch (pad->command[port]) {
    case 0x42u:
        return poll_reply(pad, port, index);
    case 0x43u:
        if (pad->response_mode[port] != 0u) {
            return 0u;
        }
        return poll_reply(pad, port, index);
    case 0x44u:
        return index == 2u ? pad->selector[port] : 0u;
    case 0x45u:
        switch (index) {
        case 0u:
            return 0x01u;
        case 1u:
            return 0x02u;
        case 2u:
            return pad->analog_mode[port] != 0u ? 1u : 0u;
        case 3u:
            return 0x02u;
        case 4u:
            return 0x01u;
        default:
            return 0u;
        }
    case 0x46u:
        if (pad->selector[port] > 1u) {
            return 0xFFu;
        }
        switch (index) {
        case 2u:
            return 0x01u;
        case 3u:
            return pad->selector[port] == 0u ? 0x02u : 0x01u;
        case 4u:
            return pad->selector[port] == 0u ? 0u : 1u;
        case 5u:
            return pad->selector[port] == 0u ? 10u : 20u;
        default:
            return 0u;
        }
    case 0x47u:
        switch (index) {
        case 2u:
            return 0x02u;
        case 4u:
            return 0x01u;
        default:
            return 0u;
        }
    case 0x4Cu:
        if (pad->selector[port] > 1u) {
            return 0xFFu;
        }
        return index == 3u ? (pad->selector[port] == 0u ? 0x04u : 0x07u) : 0u;
    case 0x4Du:
        return pad->rumble_map[port][index];
    default:
        return 0xFFu;
    }
}

static void finish_payload(schway *inst, uint32_t port, uint64_t now)
{
    struct schway__pad *pad = &inst->pad;
    uint8_t command = pad->command[port];
    if (command == 0x42u) {
        if (pad->rumble_locked[port] == 0u && pad->config_mode[port] != 0u) {
            pad->rumble_large[port] = pad->rumble_stage_large[port];
            pad->rumble_small[port] = pad->rumble_stage_small[port];
        } else if (pad->rumble_locked[port] == 0u && pad->old_rumble[port] != 0u &&
                   pad->analog_mode[port] == 0u) {
            uint8_t large = pad->rumble_stage_large[port];
            uint8_t small = pad->rumble_stage_small[port];
            pad->rumble_large[port] = large >= 0x40u && large <= 0x7Fu && (small & 1u) != 0u
                                          ? 0xFFu
                                          : 0u;
            pad->rumble_small[port] = 0u;
        }
        uint8_t bit = (uint8_t)(1u << port);
        pad->frame_polls |= bit;
        if (pad->transfers < SCHWAY__STATE_TICKS_MAX) {
            pad->transfers++;
        }
        refresh_watchdog(inst, port, now);
    } else if (command == 0x4Du) {
        if (pad->rumble_map_valid[port] != 0u) {
            for (uint32_t i = 0u; i < 6u; i++) {
                pad->rumble_map[port][i] = pad->rumble_map_stage[port][i];
            }
        }
        refresh_watchdog(inst, port, now);
    }
    pad->phase[port] = PAD_IDLE;
    pad->data_index[port] = 0u;
}

static uint8_t pad_payload(schway *inst, uint32_t port, uint8_t sent, uint64_t now)
{
    struct schway__pad *pad = &inst->pad;
    uint32_t index = pad->data_index[port];
    uint8_t received = command_reply(pad, port, index);
    uint8_t command = pad->command[port];
    if (command == 0x42u) {
        if (pad->config_mode[port] != 0u) {
            uint8_t mapping = pad->rumble_map[port][index];
            if (mapping == 0u) {
                pad->rumble_stage_small[port] = sent & 1u;
            } else if (mapping == 1u) {
                pad->rumble_stage_large[port] = sent;
            }
        } else if (pad->old_rumble[port] != 0u && pad->analog_mode[port] == 0u) {
            if (index == 0u) {
                pad->rumble_stage_large[port] = sent;
            } else if (index == 1u) {
                pad->rumble_stage_small[port] = sent;
            }
        }
    } else if (command == 0x43u) {
        if (index == 0u) {
            if (sent == 1u) {
                pad->config_mode[port] = 1u;
                pad->watchdog_deadline[port] = deadline_after(now, PAD_WATCHDOG_TICKS);
            } else if (sent == 0u) {
                pad->config_mode[port] = 0u;
                pad->watchdog_deadline[port] = 0u;
            }
        }
        refresh_watchdog(inst, port, now);
    } else if (command == 0x4Du) {
        if (sent == 0u || sent == 1u || sent == 0xFFu) {
            pad->rumble_map_stage[port][index] = sent;
        } else {
            pad->rumble_map_valid[port] = 0u;
            received = 0xFFu;
        }
        refresh_watchdog(inst, port, now);
    } else if (command == 0x44u) {
        if (index == 0u) {
            if (sent <= 1u) {
                pad->analog_mode[port] = sent;
            } else {
                pad->selector[port] = 1u;
            }
        } else if (index == 1u) {
            if (sent <= 3u) {
                pad->analog_lock[port] = sent == 3u ? 1u : 0u;
            } else {
                pad->selector[port] = 1u;
            }
        }
        refresh_watchdog(inst, port, now);
    } else if (command == 0x46u || command == 0x4Cu) {
        if (index == 0u) {
            pad->selector[port] = sent <= 1u ? sent : 0xFFu;
        }
        refresh_watchdog(inst, port, now);
    } else if (command == 0x45u || command == 0x47u) {
        refresh_watchdog(inst, port, now);
    }
    pad->data_index[port]++;
    return received;
}

uint32_t schway__pad_byte(schway *inst, uint32_t port, uint8_t sent, uint64_t now,
                          uint8_t *received)
{
    schway__pad_update_watchdog(inst, now);
    if (port >= 2u || inst->pad.present[port] == 0u) {
        *received = 0xFFu;
        return 0u;
    }

    struct schway__pad *pad = &inst->pad;
    *received = 0xFFu;
    if (sent == 0x01u && pad->phase[port] == PAD_IDLE) {
        pad->phase[port] = PAD_COMMAND;
        pad->command[port] = 0u;
        pad->data_index[port] = 0u;
        return 1u;
    }
    switch (pad->phase[port]) {
    case PAD_COMMAND:
        if (sent == 0x42u) {
            pad->command[port] = sent;
            pad->response_mode[port] = pad->config_mode[port];
            pad->rumble_stage_large[port] = 0u;
            pad->rumble_stage_small[port] = 0u;
            *received = pad->config_mode[port] != 0u ? 0xF3u : normal_id(pad, port);
            pad->phase[port] = PAD_ID_END;
            if (pad->config_mode[port] != 0u) {
                pad->old_rumble[port] = 0u;
            }
            refresh_watchdog(inst, port, now);
            return 1u;
        }
        if (sent == 0x43u && pad->device[port] == SCHWAY_INPUT_DEVICE_DUALSHOCK) {
            pad->command[port] = sent;
            pad->response_mode[port] = pad->config_mode[port];
            pad->old_rumble[port] = 0u;
            pad->rumble_large[port] = 0u;
            pad->rumble_small[port] = 0u;
            *received = pad->config_mode[port] != 0u ? 0xF3u : normal_id(pad, port);
            pad->phase[port] = PAD_ID_END;
            refresh_watchdog(inst, port, now);
            return 1u;
        }
        if (pad->device[port] == SCHWAY_INPUT_DEVICE_DUALSHOCK &&
            pad->config_mode[port] != 0u &&
            (sent == 0x44u || sent == 0x45u || sent == 0x46u || sent == 0x47u ||
             sent == 0x4Cu || sent == 0x4Du)) {
            pad->command[port] = sent;
            pad->response_mode[port] = 1u;
            pad->old_rumble[port] = 0u;
            pad->rumble_large[port] = 0u;
            pad->rumble_small[port] = 0u;
            *received = 0xF3u;
            pad->phase[port] = PAD_ID_END;
            pad->selector[port] = 0u;
            if (sent == 0x4Du) {
                pad->rumble_map_valid[port] = 1u;
                memcpy(pad->rumble_map_stage[port], pad->rumble_map[port],
                       sizeof pad->rumble_map_stage[port]);
            }
            refresh_watchdog(inst, port, now);
            return 1u;
        }
        pad->phase[port] = PAD_IDLE;
        return 0u;
    case PAD_ID_END:
        *received = 0x5Au;
        if (pad->command[port] == 0x42u) {
            pad->phase[port] = PAD_PAYLOAD;
        } else if (pad->command[port] == 0x43u || pad->command[port] == 0x44u ||
                   pad->command[port] == 0x45u || pad->command[port] == 0x46u ||
                   pad->command[port] == 0x47u || pad->command[port] == 0x4Cu ||
                   pad->command[port] == 0x4Du) {
            pad->phase[port] = PAD_PAYLOAD;
        } else {
            pad->phase[port] = PAD_IDLE;
            return 0u;
        }
        pad->data_index[port] = 0u;
        return 1u;
    case PAD_PAYLOAD: {
        uint8_t count = pad->command[port] == 0x42u ? poll_size(pad, port) : 6u;
        *received = pad_payload(inst, port, sent, now);
        if (pad->data_index[port] >= count) {
            finish_payload(inst, port, now);
            return 0u;
        }
        return 1u;
    }
    default:
        pad->phase[port] = PAD_IDLE;
        return 0u;
    }
}
