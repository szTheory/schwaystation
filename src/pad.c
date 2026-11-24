/* Pad replies and rumble follow .planning/preparation/PSX-PERIPHERALS-AND-SAVES.md. */
#include "internal.h"

#include <string.h>

#include "state.h"

#define PAD_WATCHDOG_TICKS UINT64_C(33868800)

enum { PAD_IDLE, PAD_COMMAND, PAD_ID_END, PAD_PAYLOAD };
enum { PAD_MOUSE_DELTA_MIN = 32640u, PAD_MOUSE_DELTA_MAX = 32895u };

static const char name_buttons[] = "pad.buttons";
static const char name_axes[] = "pad.axes";
static const char name_mouse_dx[] = "pad.mouse_dx";
static const char name_mouse_dy[] = "pad.mouse_dy";
static const char name_mouse_startup_ack[] = "pad.mouse_startup_ack";
static const char name_negcon_twist[] = "pad.negcon_twist";
static const char name_negcon_i[] = "pad.negcon_i";
static const char name_negcon_ii[] = "pad.negcon_ii";
static const char name_negcon_l[] = "pad.negcon_l";
static const char name_jogcon_dial[] = "pad.jogcon_dial";
static const char name_jogcon_flags[] = "pad.jogcon_flags";
static const char name_multitap[] = "pad.multitap";
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
static const char name_watchdogs[SCHWAY__PAD_ENDPOINT_COUNT][sizeof "pad.watchdog0"] = {
    "pad.watchdog0", "pad.watchdog1", "pad.watchdog2", "pad.watchdog3",
    "pad.watchdog4", "pad.watchdog5", "pad.watchdog6", "pad.watchdog7"};
static const char name_transfers[] = "pad.transfers";

static uint64_t deadline_after(uint64_t now, uint64_t delta)
{
    return now > SCHWAY__STATE_TICKS_MAX - delta ? SCHWAY__STATE_TICKS_MAX : now + delta;
}

void schway__pad_reset(schway *inst)
{
    memset(&inst->pad, 0, sizeof inst->pad);
    for (uint32_t endpoint = 0u; endpoint < SCHWAY__PAD_ENDPOINT_COUNT; endpoint++) {
        inst->pad.buttons[endpoint] = UINT16_MAX;
        inst->pad.present[endpoint] = endpoint < 2u ? 1u : 0u;
        inst->pad.device[endpoint] = endpoint < 2u ? SCHWAY_INPUT_DEVICE_DIGITAL
                                                   : SCHWAY_INPUT_DEVICE_NONE;
        inst->pad.mouse_dx[endpoint] = UINT16_C(32768);
        inst->pad.mouse_dy[endpoint] = UINT16_C(32768);
        inst->pad.rumble_map_valid[endpoint] = 1u;
        for (uint32_t i = 0u; i < 6u; i++) {
            uint32_t index = endpoint * 6u + i;
            inst->pad.rumble_map[index] = 0xFFu;
            inst->pad.rumble_map_stage[index] = 0xFFu;
        }
    }
}

void schway__pad_abort(schway *inst, uint32_t port)
{
    if (port < 2u) {
        inst->pad.phase[port] = PAD_IDLE;
        inst->pad.command[port] = 0u;
        inst->pad.data_index[port] = 0u;
        for (uint32_t socket = 1u; socket < 4u; socket++) {
            uint32_t endpoint = 2u + port * 3u + socket - 1u;
            inst->pad.phase[endpoint] = PAD_IDLE;
            inst->pad.command[endpoint] = 0u;
            inst->pad.data_index[endpoint] = 0u;
        }
    } else {
        for (uint32_t endpoint = 0u; endpoint < SCHWAY__PAD_ENDPOINT_COUNT; endpoint++) {
            inst->pad.phase[endpoint] = PAD_IDLE;
            inst->pad.command[endpoint] = 0u;
            inst->pad.data_index[endpoint] = 0u;
        }
    }
}

void schway__pad_set_buttons(schway *inst, uint32_t port, uint16_t buttons)
{
    if (port < SCHWAY__PAD_ENDPOINT_COUNT) {
        inst->pad.buttons[port] = buttons;
    }
}

void schway__pad_set_connected(schway *inst, uint32_t port, uint32_t connected)
{
    if (port < SCHWAY__PAD_ENDPOINT_COUNT) {
        inst->pad.present[port] = connected != 0u ? 1u : 0u;
        inst->pad.device[port] = connected != 0u ? SCHWAY_INPUT_DEVICE_DIGITAL
                                                 : SCHWAY_INPUT_DEVICE_NONE;
        inst->pad.mouse_startup_ack[port] = 0u;
        inst->pad.mouse_dx[port] = UINT16_C(32768);
        inst->pad.mouse_dy[port] = UINT16_C(32768);
        if (inst->pad.present[port] == 0u) {
            inst->pad.phase[port] = PAD_IDLE;
            inst->pad.command[port] = 0u;
            inst->pad.data_index[port] = 0u;
        }
    }
}

static uint16_t latched_mouse_delta(int16_t input)
{
    int32_t value = input;
    if (value < -128) {
        value = -128;
    } else if (value > 127) {
        value = 127;
    }
    return (uint16_t)(value - INT16_MIN);
}

void schway__pad_frame_start(schway *inst)
{
    if (inst->pad.frame_latched != 0u) {
        return;
    }
    inst->pad.frame_polls = 0u;
    if (inst->pad.pending_valid != 0u) {
        for (uint32_t port = 0u; port < 2u; port++) {
            inst->pad.multitap[port] = (uint8_t)inst->pad.pending_input.multitap[port];
            for (uint32_t socket = 0u; socket < 4u; socket++) {
                uint32_t endpoint = socket == 0u ? port : 2u + port * 3u + socket - 1u;
                const schway_controller_input *controller =
                    &inst->pad.pending_input.controllers[port][socket];
                uint8_t tap_enabled = inst->pad.multitap[port];
                const schway_controller_input *controls =
                    tap_enabled != 0u || socket == 0u ? controller : NULL;
                uint32_t device = tap_enabled != 0u
                                      ? controller->device
                                      : (socket == 0u ? inst->pad.pending_input.device[port]
                                                      : SCHWAY_INPUT_DEVICE_NONE);
                uint16_t buttons = tap_enabled != 0u
                                       ? controller->buttons
                                       : (socket == 0u
                                              ? inst->pad.pending_input.buttons[port]
                                              : 0u);
                const uint8_t *axes = tap_enabled != 0u
                                          ? controller->axes
                                          : (socket == 0u
                                                 ? inst->pad.pending_input.axes[port]
                                                 : NULL);
                uint8_t pressed = tap_enabled != 0u
                                      ? controller->analog_button
                                      : (socket == 0u
                                             ? inst->pad.pending_input.analog_button[port]
                                             : 0u);
                if (device != inst->pad.device[endpoint]) {
                    if (device == SCHWAY_INPUT_DEVICE_DUALSHOCK) {
                        inst->pad.analog_mode[endpoint] = 1u;
                        inst->pad.analog_lock[endpoint] = 0u;
                        inst->pad.rumble_locked[endpoint] = 0u;
                        inst->pad.old_rumble[endpoint] = 1u;
                    } else {
                        inst->pad.analog_mode[endpoint] = 0u;
                        inst->pad.config_mode[endpoint] = 0u;
                        inst->pad.watchdog_deadline[endpoint] = 0u;
                        inst->pad.old_rumble[endpoint] = 0u;
                        inst->pad.rumble_large[endpoint] = 0u;
                        inst->pad.rumble_small[endpoint] = 0u;
                    }
                    inst->pad.device[endpoint] = (uint8_t)device;
                    inst->pad.present[endpoint] =
                        device != SCHWAY_INPUT_DEVICE_NONE ? 1u : 0u;
                    inst->pad.phase[endpoint] = PAD_IDLE;
                    inst->pad.command[endpoint] = 0u;
                    inst->pad.data_index[endpoint] = 0u;
                    inst->pad.mouse_startup_ack[endpoint] =
                        device == SCHWAY_INPUT_DEVICE_MOUSE ? 1u : 0u;
                }
                uint8_t prior_pressed = inst->pad.analog_button[endpoint];
                if (device == SCHWAY_INPUT_DEVICE_DUALSHOCK && prior_pressed == 0u &&
                    pressed != 0u && inst->pad.analog_lock[endpoint] == 0u) {
                    inst->pad.analog_mode[endpoint] ^= 1u;
                }
                inst->pad.analog_button[endpoint] = pressed;
                inst->pad.buttons[endpoint] = device == SCHWAY_INPUT_DEVICE_MOUSE
                                                  ? (uint16_t)(0xF0FFu |
                                                               (0x0C00u & (uint16_t)~buttons))
                                                  : (uint16_t)~buttons;
                if (axes != NULL) {
                    memcpy(&inst->pad.axes[endpoint * 4u], axes, 4u);
                } else {
                    memset(&inst->pad.axes[endpoint * 4u], 0, 4u);
                }
                inst->pad.mouse_dx[endpoint] = device == SCHWAY_INPUT_DEVICE_MOUSE &&
                                                       controls != NULL
                                                   ? latched_mouse_delta(controls->mouse_dx)
                                                   : UINT16_C(32768);
                inst->pad.mouse_dy[endpoint] = device == SCHWAY_INPUT_DEVICE_MOUSE &&
                                                       controls != NULL
                                                   ? latched_mouse_delta(controls->mouse_dy)
                                                   : UINT16_C(32768);
                inst->pad.negcon_twist[endpoint] =
                    controls != NULL ? controls->negcon_twist : 0u;
                inst->pad.negcon_i[endpoint] = controls != NULL ? controls->negcon_i : 0u;
                inst->pad.negcon_ii[endpoint] = controls != NULL ? controls->negcon_ii : 0u;
                inst->pad.negcon_l[endpoint] = controls != NULL ? controls->negcon_l : 0u;
                inst->pad.jogcon_dial[endpoint] =
                    controls != NULL ? controls->jogcon_dial : 0u;
                inst->pad.jogcon_flags[endpoint] =
                    controls != NULL ? controls->jogcon_flags : 0u;
            }
        }
        inst->pad.pending_valid = 0u;
    } else {
        for (uint32_t endpoint = 0u; endpoint < SCHWAY__PAD_ENDPOINT_COUNT; endpoint++) {
            inst->pad.mouse_dx[endpoint] = UINT16_C(32768);
            inst->pad.mouse_dy[endpoint] = UINT16_C(32768);
        }
    }
    inst->pad.frame_latched = 1u;
}

void schway__pad_frame_end(schway *inst)
{
    inst->pad.frame_latched = 0u;
}

void schway__pad_update_watchdog(schway *inst, uint64_t now)
{
    for (uint32_t port = 0u; port < SCHWAY__PAD_ENDPOINT_COUNT; port++) {
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
    uint8_t saved_present[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t saved_device[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t saved_phase[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t saved_command[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t saved_data_index[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t saved_config[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t saved_analog[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t saved_mouse_startup_ack[SCHWAY__PAD_ENDPOINT_COUNT];
    uint8_t saved_multitap[2];
    uint8_t saved_map[SCHWAY__PAD_RUMBLE_MAP_SIZE];
    uint8_t saved_map_stage[SCHWAY__PAD_RUMBLE_MAP_SIZE];
    const uint8_t *read_bytes = io->mode == SCHWAY__STATE_VALIDATE ? io->r : NULL;
    memcpy(saved_present, inst->pad.present, sizeof saved_present);
    memcpy(saved_device, inst->pad.device, sizeof saved_device);
    memcpy(saved_config, inst->pad.config_mode, sizeof saved_config);
    memcpy(saved_mouse_startup_ack, inst->pad.mouse_startup_ack,
           sizeof saved_mouse_startup_ack);
    schway__state_u16_array(io, name_buttons, inst->pad.buttons,
                            SCHWAY__PAD_ENDPOINT_COUNT, 0u, UINT16_MAX);
    schway__state_u8_array(io, name_axes, inst->pad.axes,
                           4u * SCHWAY__PAD_ENDPOINT_COUNT, 0u, UINT8_MAX);
    schway__state_u16_array(io, name_mouse_dx, inst->pad.mouse_dx,
                            SCHWAY__PAD_ENDPOINT_COUNT, PAD_MOUSE_DELTA_MIN,
                            PAD_MOUSE_DELTA_MAX);
    schway__state_u16_array(io, name_mouse_dy, inst->pad.mouse_dy,
                            SCHWAY__PAD_ENDPOINT_COUNT, PAD_MOUSE_DELTA_MIN,
                            PAD_MOUSE_DELTA_MAX);
    read_bytes = io->mode == SCHWAY__STATE_VALIDATE ? io->r : NULL;
    schway__state_u8_array(io, name_mouse_startup_ack, inst->pad.mouse_startup_ack,
                           SCHWAY__PAD_ENDPOINT_COUNT, 0u, 1u);
    if (read_bytes != NULL) {
        memcpy(saved_mouse_startup_ack, read_bytes, sizeof saved_mouse_startup_ack);
    }
    schway__state_u8_array(io, name_negcon_twist, inst->pad.negcon_twist,
                           SCHWAY__PAD_ENDPOINT_COUNT, 0u, UINT8_MAX);
    schway__state_u8_array(io, name_negcon_i, inst->pad.negcon_i,
                           SCHWAY__PAD_ENDPOINT_COUNT, 0u, UINT8_MAX);
    schway__state_u8_array(io, name_negcon_ii, inst->pad.negcon_ii,
                           SCHWAY__PAD_ENDPOINT_COUNT, 0u, UINT8_MAX);
    schway__state_u8_array(io, name_negcon_l, inst->pad.negcon_l,
                           SCHWAY__PAD_ENDPOINT_COUNT, 0u, UINT8_MAX);
    schway__state_u16_array(io, name_jogcon_dial, inst->pad.jogcon_dial,
                            SCHWAY__PAD_ENDPOINT_COUNT, 0u, UINT16_MAX);
    schway__state_u16_array(io, name_jogcon_flags, inst->pad.jogcon_flags,
                            SCHWAY__PAD_ENDPOINT_COUNT, 0u, 3u);
    read_bytes = io->mode == SCHWAY__STATE_VALIDATE ? io->r : NULL;
    schway__state_u8_array(io, name_multitap, inst->pad.multitap, 2u, 0u, 1u);
    if (read_bytes != NULL) {
        memcpy(saved_multitap, read_bytes, sizeof saved_multitap);
        memcpy(io->pad_multitap, read_bytes, sizeof io->pad_multitap);
    } else {
        memcpy(saved_multitap, inst->pad.multitap, sizeof saved_multitap);
    }
    read_bytes = io->mode == SCHWAY__STATE_VALIDATE ? io->r : NULL;
    schway__state_u8_array(io, name_present, inst->pad.present,
                           SCHWAY__PAD_ENDPOINT_COUNT, 0u, 1u);
    if (read_bytes != NULL) {
        memcpy(saved_present, read_bytes, sizeof saved_present);
    }
    read_bytes = io->mode == SCHWAY__STATE_VALIDATE ? io->r : NULL;
    schway__state_u8_array(io, name_device, inst->pad.device,
                           SCHWAY__PAD_ENDPOINT_COUNT, SCHWAY_INPUT_DEVICE_NONE,
                           SCHWAY_INPUT_DEVICE_DUAL_ANALOG_FLIGHT);
    if (read_bytes != NULL) {
        memcpy(saved_device, read_bytes, sizeof saved_device);
    }
    read_bytes = io->mode == SCHWAY__STATE_VALIDATE ? io->r : NULL;
    schway__state_u8_array(io, name_phase, inst->pad.phase,
                           SCHWAY__PAD_ENDPOINT_COUNT, PAD_IDLE, PAD_PAYLOAD);
    if (read_bytes != NULL) {
        memcpy(saved_phase, read_bytes, sizeof saved_phase);
    } else {
        memcpy(saved_phase, inst->pad.phase, sizeof saved_phase);
    }
    read_bytes = io->mode == SCHWAY__STATE_VALIDATE ? io->r : NULL;
    schway__state_u8_array(io, name_command, inst->pad.command,
                           SCHWAY__PAD_ENDPOINT_COUNT, 0u, UINT8_MAX);
    if (read_bytes != NULL) {
        memcpy(saved_command, read_bytes, sizeof saved_command);
    } else {
        memcpy(saved_command, inst->pad.command, sizeof saved_command);
    }
    read_bytes = io->mode == SCHWAY__STATE_VALIDATE ? io->r : NULL;
    schway__state_u8_array(io, name_data_index, inst->pad.data_index,
                           SCHWAY__PAD_ENDPOINT_COUNT, 0u, 6u);
    if (read_bytes != NULL) {
        memcpy(saved_data_index, read_bytes, sizeof saved_data_index);
    } else {
        memcpy(saved_data_index, inst->pad.data_index, sizeof saved_data_index);
    }
    read_bytes = io->mode == SCHWAY__STATE_VALIDATE ? io->r : NULL;
    schway__state_u8_array(io, name_config_mode, inst->pad.config_mode,
                           SCHWAY__PAD_ENDPOINT_COUNT, 0u, 1u);
    if (read_bytes != NULL) {
        memcpy(saved_config, read_bytes, sizeof saved_config);
    }
    schway__state_u8_array(io, name_response_mode, inst->pad.response_mode,
                           SCHWAY__PAD_ENDPOINT_COUNT, 0u, 1u);
    read_bytes = io->mode == SCHWAY__STATE_VALIDATE ? io->r : NULL;
    schway__state_u8_array(io, name_analog_mode, inst->pad.analog_mode,
                           SCHWAY__PAD_ENDPOINT_COUNT, 0u, 1u);
    if (read_bytes != NULL) {
        memcpy(saved_analog, read_bytes, sizeof saved_analog);
    } else {
        memcpy(saved_analog, inst->pad.analog_mode, sizeof saved_analog);
    }
    schway__state_u8_array(io, name_analog_button, inst->pad.analog_button,
                           SCHWAY__PAD_ENDPOINT_COUNT, 0u, 1u);
    schway__state_u8_array(io, name_analog_lock, inst->pad.analog_lock,
                           SCHWAY__PAD_ENDPOINT_COUNT, 0u, 1u);
    schway__state_u8_array(io, name_selector, inst->pad.selector,
                           SCHWAY__PAD_ENDPOINT_COUNT, 0u, UINT8_MAX);
    schway__state_u8_array(io, name_old_rumble, inst->pad.old_rumble,
                           SCHWAY__PAD_ENDPOINT_COUNT, 0u, 1u);
    schway__state_u8_array(io, name_rumble_locked, inst->pad.rumble_locked,
                           SCHWAY__PAD_ENDPOINT_COUNT, 0u, 1u);
    schway__state_u8_array(io, name_rumble_map_valid, inst->pad.rumble_map_valid,
                           SCHWAY__PAD_ENDPOINT_COUNT, 0u, 1u);
    schway__state_u8_array(io, name_rumble_large, inst->pad.rumble_large,
                           SCHWAY__PAD_ENDPOINT_COUNT, 0u, UINT8_MAX);
    schway__state_u8_array(io, name_rumble_small, inst->pad.rumble_small,
                           SCHWAY__PAD_ENDPOINT_COUNT, 0u, 1u);
    schway__state_u8_array(io, name_rumble_stage_large, inst->pad.rumble_stage_large,
                           SCHWAY__PAD_ENDPOINT_COUNT, 0u, UINT8_MAX);
    schway__state_u8_array(io, name_rumble_stage_small, inst->pad.rumble_stage_small,
                           SCHWAY__PAD_ENDPOINT_COUNT, 0u, 1u);
    read_bytes = io->mode == SCHWAY__STATE_VALIDATE ? io->r : NULL;
    schway__state_u8_array(io, name_rumble_map, inst->pad.rumble_map,
                           SCHWAY__PAD_RUMBLE_MAP_SIZE, 0u, UINT8_MAX);
    if (read_bytes != NULL) {
        memcpy(saved_map, read_bytes, sizeof saved_map);
    } else {
        memcpy(saved_map, inst->pad.rumble_map, sizeof saved_map);
    }
    read_bytes = io->mode == SCHWAY__STATE_VALIDATE ? io->r : NULL;
    schway__state_u8_array(io, name_rumble_map_stage, inst->pad.rumble_map_stage,
                           SCHWAY__PAD_RUMBLE_MAP_SIZE, 0u, UINT8_MAX);
    if (read_bytes != NULL) {
        memcpy(saved_map_stage, read_bytes, sizeof saved_map_stage);
    } else {
        memcpy(saved_map_stage, inst->pad.rumble_map_stage, sizeof saved_map_stage);
    }
    schway__state_u8_array(io, name_frame_polls, &inst->pad.frame_polls, 1u, 0u, UINT8_MAX);
    schway__state_u8_array(io, name_frame_latched, &inst->pad.frame_latched, 1u, 0u, 1u);
    uint64_t saved_watchdog[SCHWAY__PAD_ENDPOINT_COUNT];
    for (uint32_t endpoint = 0u; endpoint < SCHWAY__PAD_ENDPOINT_COUNT; endpoint++) {
        saved_watchdog[endpoint] = schway__state_u64(
            io, name_watchdogs[endpoint], &inst->pad.watchdog_deadline[endpoint], 0u,
            SCHWAY__STATE_TICKS_MAX);
    }
    (void)schway__state_u64(io, name_transfers, &inst->pad.transfers, 0u,
                            SCHWAY__STATE_TICKS_MAX);
    for (uint32_t port = 0u; port < SCHWAY__PAD_ENDPOINT_COUNT; port++) {
        uint32_t physical_port = port < 2u ? port : (port < 5u ? 0u : 1u);
        int tap_socket = port >= 2u;
        for (uint32_t i = 0u; i < 6u; i++) {
            uint8_t value = saved_map[port * 6u + i];
            uint8_t staged = saved_map_stage[port * 6u + i];
            schway__state_check(io, value == 0u || value == 1u || value == 0xFFu);
            schway__state_check(io, staged == 0u || staged == 1u || staged == 0xFFu);
        }
        schway__state_check(io, saved_present[port] ==
                                    (saved_device[port] != SCHWAY_INPUT_DEVICE_NONE ? 1u : 0u));
        schway__state_check(io, saved_mouse_startup_ack[port] == 0u ||
                                    saved_device[port] == SCHWAY_INPUT_DEVICE_MOUSE);
        schway__state_check(io, saved_config[port] == 0u ||
                                    (saved_device[port] == SCHWAY_INPUT_DEVICE_DUALSHOCK &&
                                     saved_watchdog[port] != 0u));
        schway__state_check(io, saved_config[port] != 0u || saved_watchdog[port] == 0u);
        schway__state_check(io, saved_phase[port] == PAD_IDLE || saved_present[port] != 0u);
        if (saved_phase[port] == PAD_COMMAND) {
            schway__state_check(io, saved_command[port] == 0u &&
                                       saved_data_index[port] == 0u);
        } else if (saved_phase[port] == PAD_ID_END) {
            schway__state_check(io, saved_data_index[port] == 0u);
        } else if (saved_phase[port] == PAD_PAYLOAD) {
            uint32_t count = saved_command[port] == 0x42u
                                 ? (saved_config[port] != 0u ||
                                            (saved_device[port] == SCHWAY_INPUT_DEVICE_DUALSHOCK &&
                                             saved_analog[port] != 0u) ||
                                            saved_device[port] == SCHWAY_INPUT_DEVICE_NEGCON ||
                                            saved_device[port] == SCHWAY_INPUT_DEVICE_JOGCON ||
                                            saved_device[port] ==
                                                SCHWAY_INPUT_DEVICE_DUAL_ANALOG_FLIGHT
                                        ? 6u
                                        : saved_device[port] == SCHWAY_INPUT_DEVICE_MOUSE ? 4u
                                                                                           : 2u)
                                 : 6u;
            schway__state_check(io, saved_data_index[port] < count);
        }
        schway__state_check(io, saved_command[port] == 0u ||
                                   saved_command[port] == 0x42u ||
                                   (saved_device[port] == SCHWAY_INPUT_DEVICE_DUALSHOCK &&
                                    (saved_command[port] == 0x43u ||
                                     (saved_config[port] != 0u &&
                                      (saved_command[port] == 0x44u ||
                                       saved_command[port] == 0x45u ||
                                       saved_command[port] == 0x46u ||
                                       saved_command[port] == 0x47u ||
                                       saved_command[port] == 0x4Cu ||
                                       saved_command[port] == 0x4Du)))));
        schway__state_check(io, tap_socket == 0 || saved_multitap[physical_port] != 0u ||
                                   (saved_device[port] == SCHWAY_INPUT_DEVICE_NONE &&
                                    saved_present[port] == 0u &&
                                    saved_phase[port] == PAD_IDLE &&
                                    saved_data_index[port] == 0u));
    }
}

static uint8_t normal_id(const struct schway__pad *pad, uint32_t port)
{
    if (pad->device[port] == SCHWAY_INPUT_DEVICE_DUAL_ANALOG_FLIGHT) {
        return 0x53u;
    }
    if (pad->device[port] == SCHWAY_INPUT_DEVICE_JOGCON) {
        return 0xE3u;
    }
    if (pad->device[port] == SCHWAY_INPUT_DEVICE_MOUSE) {
        return 0x12u;
    }
    if (pad->device[port] == SCHWAY_INPUT_DEVICE_NEGCON) {
        return 0x23u;
    }
    if (pad->device[port] == SCHWAY_INPUT_DEVICE_DUALSHOCK &&
        pad->analog_mode[port] != 0u) {
        return 0x73u;
    }
    return 0x41u;
}

static uint8_t poll_size(const struct schway__pad *pad, uint32_t port)
{
    if (pad->config_mode[port] != 0u ||
        (pad->device[port] == SCHWAY_INPUT_DEVICE_DUALSHOCK &&
         pad->analog_mode[port] != 0u) ||
        pad->device[port] == SCHWAY_INPUT_DEVICE_NEGCON ||
        pad->device[port] == SCHWAY_INPUT_DEVICE_JOGCON ||
        pad->device[port] == SCHWAY_INPUT_DEVICE_DUAL_ANALOG_FLIGHT) {
        return 6u;
    }
    return pad->device[port] == SCHWAY_INPUT_DEVICE_MOUSE ? 4u : 2u;
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
    if (pad->device[port] == SCHWAY_INPUT_DEVICE_MOUSE) {
        uint16_t encoded = index == 2u ? pad->mouse_dx[port] : pad->mouse_dy[port];
        return (uint8_t)((int32_t)encoded - 32768);
    }
    if (pad->device[port] == SCHWAY_INPUT_DEVICE_NEGCON) {
        switch (index) {
        case 2u:
            return pad->negcon_twist[port];
        case 3u:
            return pad->negcon_i[port];
        case 4u:
            return pad->negcon_ii[port];
        default:
            return pad->negcon_l[port];
        }
    }
    if (pad->device[port] == SCHWAY_INPUT_DEVICE_JOGCON) {
        uint16_t dial = (uint16_t)(pad->jogcon_dial[port] & UINT16_C(0xFFFE));
        switch (index) {
        case 2u:
            return (uint8_t)dial;
        case 3u:
            return (uint8_t)(dial >> 8);
        case 4u:
            return (uint8_t)(pad->jogcon_flags[port] & 3u);
        default:
            return 0u;
        }
    }
    return pad->axes[port * 4u + index - 2u];
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
            return (uint8_t)(pad->selector[port] == 0u ? 0u : 1u);
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
        return pad->rumble_map[port * 6u + index];
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
                uint32_t index = port * 6u + i;
                pad->rumble_map[index] = pad->rumble_map_stage[index];
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
            uint8_t mapping = pad->rumble_map[port * 6u + index];
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
            pad->rumble_map_stage[port * 6u + index] = sent;
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
    if (port >= SCHWAY__PAD_ENDPOINT_COUNT || inst->pad.present[port] == 0u) {
        *received = 0xFFu;
        return 0u;
    }

    struct schway__pad *pad = &inst->pad;
    *received = 0xFFu;
    if (sent == 0x01u && pad->phase[port] == PAD_IDLE) {
        if (pad->device[port] == SCHWAY_INPUT_DEVICE_MOUSE) {
            pad->mouse_startup_ack[port] = 0u;
        }
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
                uint32_t map_offset = port * 6u;
                memcpy(&pad->rumble_map_stage[map_offset], &pad->rumble_map[map_offset], 6u);
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

uint32_t schway__pad_mouse_ack_low(const schway *inst)
{
    for (uint32_t endpoint = 0u; endpoint < SCHWAY__PAD_ENDPOINT_COUNT; endpoint++) {
        if (inst->pad.mouse_startup_ack[endpoint] != 0u) {
            return 1u;
        }
    }
    return 0u;
}
