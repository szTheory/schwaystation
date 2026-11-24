/* DualShock input and rumble tests through the real SIO0 register path. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

#define JOY_DATA 0x1F801040u
#define JOY_STAT 0x1F801044u
#define JOY_MODE 0x1F801048u
#define JOY_CTRL 0x1F80104Au
#define JOY_BAUD 0x1F80104Eu

static void *test_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void test_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static schway *make(void)
{
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) {
        exit(1);
    }
    return inst;
}

static void write_mmio(schway *inst, uint32_t addr, uint32_t width, uint32_t value)
{
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, addr, width, value, &cycles), 0u);
}

static uint32_t read_mmio(schway *inst, uint32_t addr, uint32_t width)
{
    uint32_t cycles = 0u;
    uint32_t value = 0u;
    CHECK_EQ_U64(schway__bus_read(inst, addr, width, &value, &cycles), 0u);
    return value;
}

static void advance_to(schway *inst, uint64_t tick)
{
    inst->ticks = tick;
    schway__sched_sync(inst);
    schway__sched_run(inst);
    schway__spu_catch_up(inst, inst->ticks);
}

static uint8_t exchange(schway *inst, uint8_t sent, int ack)
{
    uint64_t start = inst->ticks;
    write_mmio(inst, JOY_DATA, 1u, sent);
    advance_to(inst, start + 1088u);
    uint8_t received = (uint8_t)read_mmio(inst, JOY_DATA, 1u);
    if (ack != 0) {
        advance_to(inst, start + 1426u);
        advance_to(inst, start + 1494u);
    }
    return received;
}

static void configure_sio(schway *inst)
{
    write_mmio(inst, JOY_MODE, 2u, 0x000Cu);
    write_mmio(inst, JOY_BAUD, 2u, 0x0088u);
    write_mmio(inst, JOY_CTRL, 2u, 0x0003u);
}

static void poll(schway *inst, uint8_t id, const uint8_t *payload, uint32_t n,
                const uint8_t *motor_bytes)
{
    CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x42u, 1), id);
    CHECK_EQ_HEX(exchange(inst, 0x00u, 1), 0x5Au);
    for (uint32_t i = 0u; i < n; i++) {
        uint8_t sent = motor_bytes != NULL ? motor_bytes[i] : 0u;
        CHECK_EQ_HEX(exchange(inst, sent, i + 1u < n), payload[i]);
    }
}

static void send_command_capture(schway *inst, uint8_t command, const uint8_t data[6], uint8_t id,
                                 uint8_t reply[6])
{
    CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, command, 1), id);
    CHECK_EQ_HEX(exchange(inst, 0x00u, 1), 0x5Au);
    for (uint32_t i = 0u; i < 6u; i++) {
        uint8_t value = exchange(inst, data[i], i != 5u);
        if (reply != NULL) {
            reply[i] = value;
        }
    }
}

static void send_command(schway *inst, uint8_t command, const uint8_t data[6], uint8_t id)
{
    send_command_capture(inst, command, data, id, NULL);
}

static void set_dualshock_input(schway *inst, uint8_t analog_button)
{
    schway_input input;
    memset(&input, 0, sizeof input);
    input.size = (uint32_t)sizeof input;
    input.device[0] = SCHWAY_INPUT_DEVICE_DUALSHOCK;
    for (uint32_t axis = 0u; axis < 4u; axis++) {
        input.axes[0][axis] = 128u;
    }
    input.analog_button[0] = analog_button;
    CHECK_EQ_U64(schway_set_input(inst, &input), SCHWAY_STATUS_OK);
}

static void start_dualshock(schway *inst, uint8_t analog_button)
{
    set_dualshock_input(inst, analog_button);
    CHECK_EQ_U64(schway_run_frame(inst, NULL), SCHWAY_STATUS_OK);
}

static void next_input_frame(schway *inst, uint8_t analog_button)
{
    set_dualshock_input(inst, analog_button);
    CHECK_EQ_U64(schway_run_frame(inst, NULL), SCHWAY_STATUS_OK);
}

static uint32_t endpoint_index(uint32_t port, uint32_t socket)
{
    return socket == 0u ? port : 2u + port * 3u + socket - 1u;
}

static uint8_t direct_endpoint_id(schway *inst, uint32_t endpoint)
{
    uint8_t received = 0u;
    CHECK_EQ_U64(schway__pad_byte(inst, endpoint, 0x01u, inst->ticks, &received), 1u);
    CHECK_EQ_HEX(received, 0xFFu);
    CHECK_EQ_U64(schway__pad_byte(inst, endpoint, 0x42u, inst->ticks, &received), 1u);
    uint8_t id = received;
    CHECK_EQ_U64(schway__pad_byte(inst, endpoint, 0x00u, inst->ticks, &received), 1u);
    CHECK_EQ_HEX(received, 0x5Au);
    uint32_t count = id == 0x73u ? 6u : 2u;
    for (uint32_t i = 0u; i < count; i++) {
        uint32_t ack = schway__pad_byte(inst, endpoint, 0x00u, inst->ticks, &received);
        CHECK_EQ_U64(ack, i + 1u < count ? 1u : 0u);
    }
    return id;
}

static void eight_endpoint_input_latch(void)
{
    schway *inst = make();
    schway_input first;
    memset(&first, 0, sizeof first);
    first.size = (uint32_t)sizeof first;
    first.multitap[0] = 1u;
    first.multitap[1] = 1u;
    first.device[0] = SCHWAY_INPUT_DEVICE_DIGITAL;
    first.buttons[0] = UINT16_C(0x00FF);
    for (uint32_t port = 0u; port < 2u; port++) {
        for (uint32_t socket = 0u; socket < 4u; socket++) {
            uint32_t endpoint = endpoint_index(port, socket);
            schway_controller_input *controller = &first.controllers[port][socket];
            controller->device = endpoint == 0u ? SCHWAY_INPUT_DEVICE_DIGITAL
                                  : endpoint == 2u ? SCHWAY_INPUT_DEVICE_DUALSHOCK
                                                   : endpoint % 7u;
            controller->buttons = (uint16_t)(UINT16_C(0x0101) * (endpoint + 1u));
            for (uint32_t axis = 0u; axis < 4u; axis++) {
                controller->axes[axis] = (uint8_t)(10u * (axis + 1u) + endpoint);
            }
            controller->analog_button = (uint8_t)(endpoint & 1u);
            controller->mouse_dx = (int16_t)((int32_t)endpoint - 4);
            controller->mouse_dy = (int16_t)(4 - (int32_t)endpoint);
            controller->negcon_twist = (uint8_t)(20u + endpoint);
            controller->negcon_i = (uint8_t)(30u + endpoint);
            controller->negcon_ii = (uint8_t)(40u + endpoint);
            controller->negcon_l = (uint8_t)(50u + endpoint);
            controller->jogcon_dial = (uint16_t)(100u + endpoint);
            controller->jogcon_flags = (uint16_t)(endpoint & 3u);
        }
    }
    CHECK_EQ_U64(schway_set_input(inst, &first), SCHWAY_STATUS_OK);
    schway__pad_frame_start(inst);
    CHECK_EQ_U64(inst->pad.pending_valid, 0u);
    CHECK_EQ_U64(inst->pad.multitap[0], 1u);
    CHECK_EQ_U64(inst->pad.multitap[1], 1u);
    for (uint32_t port = 0u; port < 2u; port++) {
        for (uint32_t socket = 0u; socket < 4u; socket++) {
            uint32_t endpoint = endpoint_index(port, socket);
            const schway_controller_input *controller = &first.controllers[port][socket];
            CHECK_EQ_U64(inst->pad.device[endpoint], controller->device);
            CHECK_EQ_U64(inst->pad.present[endpoint],
                         controller->device != SCHWAY_INPUT_DEVICE_NONE ? 1u : 0u);
            CHECK_EQ_HEX(inst->pad.buttons[endpoint], (uint16_t)~controller->buttons);
            CHECK(memcmp(&inst->pad.axes[endpoint * 4u], controller->axes, 4u) == 0);
            CHECK_EQ_U64(inst->pad.analog_button[endpoint], controller->analog_button);
            CHECK_EQ_U64(inst->pad.mouse_dx[endpoint],
                         (uint16_t)((int32_t)controller->mouse_dx - INT16_MIN));
            CHECK_EQ_U64(inst->pad.mouse_dy[endpoint],
                         (uint16_t)((int32_t)controller->mouse_dy - INT16_MIN));
            CHECK_EQ_U64(inst->pad.negcon_twist[endpoint], controller->negcon_twist);
            CHECK_EQ_U64(inst->pad.negcon_i[endpoint], controller->negcon_i);
            CHECK_EQ_U64(inst->pad.negcon_ii[endpoint], controller->negcon_ii);
            CHECK_EQ_U64(inst->pad.negcon_l[endpoint], controller->negcon_l);
            CHECK_EQ_U64(inst->pad.jogcon_dial[endpoint], controller->jogcon_dial);
            CHECK_EQ_U64(inst->pad.jogcon_flags[endpoint], controller->jogcon_flags);
        }
    }
    CHECK_EQ_HEX(direct_endpoint_id(inst, 0u), 0x41u);
    CHECK_EQ_HEX(direct_endpoint_id(inst, 2u), 0x73u);

    schway_input next;
    memset(&next, 0, sizeof next);
    next.size = (uint32_t)sizeof next;
    next.device[0] = SCHWAY_INPUT_DEVICE_DIGITAL;
    next.device[1] = SCHWAY_INPUT_DEVICE_DUALSHOCK;
    next.buttons[0] = UINT16_C(0xA55A);
    next.buttons[1] = UINT16_C(0xC33C);
    next.axes[0][0] = 11u;
    next.axes[1][0] = 22u;
    next.controllers[0][0].mouse_dx = -100;
    next.controllers[0][0].mouse_dy = 100;
    next.controllers[0][0].negcon_twist = 1u;
    next.controllers[0][0].negcon_i = 2u;
    next.controllers[0][0].negcon_ii = 3u;
    next.controllers[0][0].negcon_l = 4u;
    next.controllers[0][0].jogcon_dial = 5u;
    next.controllers[0][0].jogcon_flags = 3u;
    next.controllers[1][0].mouse_dx = -200;
    next.controllers[1][0].mouse_dy = 200;
    for (uint32_t port = 0u; port < 2u; port++) {
        for (uint32_t socket = 1u; socket < 4u; socket++) {
            next.controllers[port][socket].device = SCHWAY_INPUT_DEVICE_MOUSE;
            next.controllers[port][socket].buttons = UINT16_MAX;
            next.controllers[port][socket].mouse_dx = 123;
            next.controllers[port][socket].negcon_twist = 99u;
        }
    }
    CHECK_EQ_U64(schway_set_input(inst, &next), SCHWAY_STATUS_OK);
    schway__pad_frame_start(inst);
    CHECK_EQ_U64(inst->pad.pending_valid, 1u);
    CHECK_EQ_U64(inst->pad.multitap[0], 1u);
    CHECK_EQ_HEX(inst->pad.buttons[2], (uint16_t)~first.controllers[0][1].buttons);

    schway__pad_frame_end(inst);
    schway__pad_frame_start(inst);
    CHECK_EQ_U64(inst->pad.pending_valid, 0u);
    CHECK_EQ_U64(inst->pad.multitap[0], 0u);
    CHECK_EQ_U64(inst->pad.multitap[1], 0u);
    CHECK_EQ_U64(inst->pad.device[0], SCHWAY_INPUT_DEVICE_DIGITAL);
    CHECK_EQ_U64(inst->pad.device[1], SCHWAY_INPUT_DEVICE_DUALSHOCK);
    CHECK_EQ_HEX(inst->pad.buttons[0], (uint16_t)~next.buttons[0]);
    CHECK_EQ_HEX(inst->pad.buttons[1], (uint16_t)~next.buttons[1]);
    CHECK_EQ_U64(inst->pad.axes[0], next.axes[0][0]);
    CHECK_EQ_U64(inst->pad.axes[4], next.axes[1][0]);
    CHECK_EQ_U64(inst->pad.mouse_dx[0], (uint16_t)(-100 - INT16_MIN));
    CHECK_EQ_U64(inst->pad.mouse_dy[0], (uint16_t)(100 - INT16_MIN));
    CHECK_EQ_U64(inst->pad.negcon_twist[0], 1u);
    CHECK_EQ_U64(inst->pad.jogcon_dial[0], 5u);
    CHECK_EQ_U64(inst->pad.jogcon_flags[0], 3u);
    for (uint32_t endpoint = 2u; endpoint < SCHWAY__PAD_ENDPOINT_COUNT; endpoint++) {
        CHECK_EQ_U64(inst->pad.device[endpoint], SCHWAY_INPUT_DEVICE_NONE);
        CHECK_EQ_U64(inst->pad.present[endpoint], 0u);
        CHECK_EQ_HEX(inst->pad.buttons[endpoint], UINT16_MAX);
        CHECK_EQ_U64(inst->pad.mouse_dx[endpoint], UINT16_C(32768));
        CHECK_EQ_U64(inst->pad.negcon_twist[endpoint], 0u);
        CHECK_EQ_U64(inst->pad.jogcon_dial[endpoint], 0u);
    }
    schway_destroy(inst);
}

static void enter_config(schway *inst)
{
    const uint8_t enter[] = {1u, 0u, 0u, 0u, 0u, 0u};
    send_command(inst, 0x43u, enter, 0x73u);
}

static schway_run_result run_one_cycle(schway *inst)
{
    schway_budget budget;
    memset(&budget, 0, sizeof budget);
    budget.size = (uint32_t)sizeof budget;
    budget.max_cycles = 1u;
    schway_run_result result;
    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    CHECK_EQ_U64(schway_run(inst, &budget, &result), SCHWAY_STATUS_OK);
    return result;
}

static void input_latch_and_rumble(void)
{
    schway *inst = make();
    configure_sio(inst);

    schway_budget budget;
    memset(&budget, 0, sizeof budget);
    budget.size = (uint32_t)sizeof budget;
    budget.max_cycles = 1u;
    CHECK_EQ_U64(schway_run(inst, &budget, NULL), SCHWAY_STATUS_OK);

    schway_input input;
    memset(&input, 0, sizeof input);
    input.size = (uint32_t)sizeof input;
    input.device[0] = SCHWAY_INPUT_DEVICE_DUALSHOCK;
    input.buttons[0] = 0x0001u;
    input.axes[0][0] = 10u;
    input.axes[0][1] = 20u;
    input.axes[0][2] = 30u;
    input.axes[0][3] = 40u;
    CHECK_EQ_U64(schway_set_input(inst, &input), SCHWAY_STATUS_OK);

    const uint8_t digital[] = {0xFFu, 0xFFu};
    poll(inst, 0x41u, digital, 2u, NULL);

    CHECK_EQ_U64(schway_run_frame(inst, NULL), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_run(inst, &budget, NULL), SCHWAY_STATUS_OK);

    const uint8_t analog[] = {0xFEu, 0xFFu, 10u, 20u, 30u, 40u};
    poll(inst, 0x73u, analog, 6u, NULL);

    const uint8_t enter_config[] = {0x01u, 0u, 0u, 0u, 0u, 0u};
    send_command(inst, 0x43u, enter_config, 0x73u);
    CHECK_EQ_U64(inst->pad.config_mode[0], 1u);

    const uint8_t mapping[] = {0u, 1u, 0xFFu, 0xFFu, 0xFFu, 0xFFu};
    send_command(inst, 0x4Du, mapping, 0xF3u);

    const uint8_t actuators[] = {1u, 255u, 0u, 0u, 0u, 0u};
    poll(inst, 0xF3u, analog, 6u, actuators);

    schway_run_result result;
    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    CHECK_EQ_U64(schway_run(inst, &budget, &result), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(result.lag, 0u);
    CHECK_EQ_HEX(result.rumble[0], 0xFF01u);
    schway_destroy(inst);
}

static void config_commands(void)
{
    schway *inst = make();
    configure_sio(inst);
    start_dualshock(inst, 0u);

    uint8_t reply[6];
    const uint8_t enter[] = {1u, 0u, 0u, 0u, 0u, 0u};
    const uint8_t normal[] = {0xFFu, 0xFFu, 128u, 128u, 128u, 128u};
    send_command_capture(inst, 0x43u, enter, 0x73u, reply);
    for (uint32_t i = 0u; i < 6u; i++) {
        CHECK_EQ_HEX(reply[i], normal[i]);
    }

    const uint8_t led_lock[] = {1u, 3u, 0u, 0u, 0u, 0u};
    const uint8_t zeros[] = {0u, 0u, 0u, 0u, 0u, 0u};
    send_command_capture(inst, 0x44u, led_lock, 0xF3u, reply);
    for (uint32_t i = 0u; i < 6u; i++) {
        CHECK_EQ_HEX(reply[i], zeros[i]);
    }
    CHECK_EQ_U64(inst->pad.analog_mode[0], 1u);
    CHECK_EQ_U64(inst->pad.analog_lock[0], 1u);

    const uint8_t model_query[] = {0u, 0u, 0u, 0u, 0u, 0u};
    const uint8_t model[] = {1u, 2u, 1u, 2u, 1u, 0u};
    send_command_capture(inst, 0x45u, model_query, 0xF3u, reply);
    for (uint32_t i = 0u; i < 6u; i++) {
        CHECK_EQ_HEX(reply[i], model[i]);
    }

    const uint8_t actuator0[] = {0u, 0u, 0u, 0u, 0u, 0u};
    const uint8_t actuator0_reply[] = {0u, 0u, 1u, 2u, 0u, 10u};
    send_command_capture(inst, 0x46u, actuator0, 0xF3u, reply);
    for (uint32_t i = 0u; i < 6u; i++) {
        CHECK_EQ_HEX(reply[i], actuator0_reply[i]);
    }
    const uint8_t actuator1[] = {1u, 0u, 0u, 0u, 0u, 0u};
    const uint8_t actuator1_reply[] = {0u, 0u, 1u, 1u, 1u, 20u};
    send_command_capture(inst, 0x46u, actuator1, 0xF3u, reply);
    for (uint32_t i = 0u; i < 6u; i++) {
        CHECK_EQ_HEX(reply[i], actuator1_reply[i]);
    }

    const uint8_t constant47[] = {0u, 0u, 0u, 0u, 0u, 0u};
    const uint8_t constant47_reply[] = {0u, 0u, 2u, 0u, 1u, 0u};
    send_command_capture(inst, 0x47u, constant47, 0xF3u, reply);
    for (uint32_t i = 0u; i < 6u; i++) {
        CHECK_EQ_HEX(reply[i], constant47_reply[i]);
    }

    const uint8_t query4c0[] = {0u, 0u, 0u, 0u, 0u, 0u};
    const uint8_t query4c0_reply[] = {0u, 0u, 0u, 4u, 0u, 0u};
    send_command_capture(inst, 0x4Cu, query4c0, 0xF3u, reply);
    for (uint32_t i = 0u; i < 6u; i++) {
        CHECK_EQ_HEX(reply[i], query4c0_reply[i]);
    }
    const uint8_t query4c1[] = {1u, 0u, 0u, 0u, 0u, 0u};
    const uint8_t query4c1_reply[] = {0u, 0u, 0u, 7u, 0u, 0u};
    send_command_capture(inst, 0x4Cu, query4c1, 0xF3u, reply);
    for (uint32_t i = 0u; i < 6u; i++) {
        CHECK_EQ_HEX(reply[i], query4c1_reply[i]);
    }

    const uint8_t mapping[] = {0u, 1u, 0xFFu, 0xFFu, 0xFFu, 0xFFu};
    const uint8_t disabled[] = {0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu};
    send_command_capture(inst, 0x4Du, mapping, 0xF3u, reply);
    for (uint32_t i = 0u; i < 6u; i++) {
        CHECK_EQ_HEX(reply[i], disabled[i]);
    }
    const uint8_t mapping2[] = {1u, 0u, 0xFFu, 0xFFu, 0xFFu, 0xFFu};
    send_command_capture(inst, 0x4Du, mapping2, 0xF3u, reply);
    for (uint32_t i = 0u; i < 6u; i++) {
        CHECK_EQ_HEX(reply[i], mapping[i]);
    }

    const uint8_t leave[] = {0u, 0u, 0u, 0u, 0u, 0u};
    send_command_capture(inst, 0x43u, leave, 0xF3u, reply);
    for (uint32_t i = 0u; i < 6u; i++) {
        CHECK_EQ_HEX(reply[i], zeros[i]);
    }
    CHECK_EQ_U64(inst->pad.config_mode[0], 0u);
    CHECK_EQ_U64(inst->pad.old_rumble[0], 0u);
    CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x42u, 1), 0x73u);
    schway_destroy(inst);
}

static void analog_button_edges(void)
{
    schway *inst = make();
    configure_sio(inst);
    start_dualshock(inst, 0u);
    enter_config(inst);
    const uint8_t lock[] = {1u, 3u, 0u, 0u, 0u, 0u};
    send_command(inst, 0x44u, lock, 0xF3u);
    next_input_frame(inst, 1u);
    CHECK_EQ_U64(inst->pad.analog_mode[0], 1u);
    next_input_frame(inst, 1u);
    CHECK_EQ_U64(inst->pad.analog_mode[0], 1u);

    const uint8_t unlock_digital[] = {0u, 2u, 0u, 0u, 0u, 0u};
    send_command(inst, 0x44u, unlock_digital, 0xF3u);
    CHECK_EQ_U64(inst->pad.analog_mode[0], 0u);
    next_input_frame(inst, 1u);
    CHECK_EQ_U64(inst->pad.analog_mode[0], 0u);
    next_input_frame(inst, 0u);
    next_input_frame(inst, 1u);
    CHECK_EQ_U64(inst->pad.analog_mode[0], 1u);
    next_input_frame(inst, 1u);
    CHECK_EQ_U64(inst->pad.analog_mode[0], 1u);
    schway_destroy(inst);
}

static void old_rumble(void)
{
    schway *inst = make();
    configure_sio(inst);
    start_dualshock(inst, 1u);
    CHECK_EQ_U64(inst->pad.analog_mode[0], 0u);
    (void)run_one_cycle(inst);

    const uint8_t buttons[] = {0xFFu, 0xFFu};
    const uint8_t on[] = {0x40u, 0x01u};
    poll(inst, 0x41u, buttons, 2u, on);
    schway_run_result result = run_one_cycle(inst);
    CHECK_EQ_HEX(result.rumble[0], 0xFF00u);
    CHECK_EQ_U64(result.lag, 0u);

    const uint8_t off[] = {0u, 0u};
    poll(inst, 0x41u, buttons, 2u, off);
    result = run_one_cycle(inst);
    CHECK_EQ_HEX(result.rumble[0], 0u);
    schway_destroy(inst);
}

static void watchdog_boundary(void)
{
    schway *inst = make();
    configure_sio(inst);
    start_dualshock(inst, 0u);
    enter_config(inst);
    uint64_t deadline = inst->pad.watchdog_deadline[0];
    CHECK(deadline >= inst->ticks);
    CHECK_EQ_U64(deadline - inst->ticks, UINT64_C(33868800));
    schway__pad_update_watchdog(inst, deadline - 1u);
    CHECK_EQ_U64(inst->pad.config_mode[0], 1u);
    CHECK_EQ_U64(inst->pad.rumble_locked[0], 0u);
    schway__pad_update_watchdog(inst, deadline);
    CHECK_EQ_U64(inst->pad.config_mode[0], 0u);
    CHECK_EQ_U64(inst->pad.analog_mode[0], 0u);
    CHECK_EQ_U64(inst->pad.rumble_locked[0], 1u);
    CHECK_EQ_U64(inst->pad.rumble_large[0], 0u);
    CHECK_EQ_U64(inst->pad.rumble_small[0], 0u);
    schway__pad_update_watchdog(inst, deadline + 1u);
    CHECK_EQ_U64(inst->pad.config_mode[0], 0u);
    CHECK_EQ_U64(inst->pad.rumble_locked[0], 1u);
    schway_destroy(inst);
}

static void state_split_config(void)
{
    schway *inst = make();
    configure_sio(inst);
    start_dualshock(inst, 0u);
    enter_config(inst);
    (void)run_one_cycle(inst);

    const uint8_t mapping[] = {0u, 1u, 0xFFu, 0xFFu, 0xFFu, 0xFFu};
    CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x4Du, 1), 0xF3u);
    CHECK_EQ_HEX(exchange(inst, 0x00u, 1), 0x5Au);
    CHECK_EQ_HEX(exchange(inst, mapping[0], 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, mapping[1], 1), 0xFFu);

    size_t state_size = schway_state_size(inst);
    uint8_t *saved = malloc(state_size);
    CHECK(saved != NULL);
    if (saved == NULL) {
        schway_destroy(inst);
        return;
    }
    CHECK_EQ_U64(schway_save_state(inst, saved, state_size), SCHWAY_STATUS_OK);
    for (uint32_t i = 2u; i < 6u; i++) {
        (void)exchange(inst, mapping[i], i != 5u);
    }
    uint8_t expected[6];
    memcpy(expected, inst->pad.rumble_map, sizeof expected);
    CHECK_EQ_U64(schway_load_state(inst, saved, state_size), SCHWAY_STATUS_OK);
    for (uint32_t i = 2u; i < 6u; i++) {
        (void)exchange(inst, mapping[i], i != 5u);
    }
    for (uint32_t i = 0u; i < 6u; i++) {
        CHECK_EQ_HEX(inst->pad.rumble_map[i], expected[i]);
    }
    free(saved);
    schway_destroy(inst);
}

static void configure_sio_port(schway *inst, uint32_t port)
{
    write_mmio(inst, JOY_MODE, 2u, 0x000Cu);
    write_mmio(inst, JOY_BAUD, 2u, 0x0088u);
    write_mmio(inst, JOY_CTRL, 2u, (uint16_t)(0x0003u | (port != 0u ? 0x2000u : 0u)));
}

static void latch_input(schway *inst, schway_input *input)
{
    input->size = (uint32_t)sizeof *input;
    CHECK_EQ_U64(schway_set_input(inst, input), SCHWAY_STATUS_OK);
    schway__pad_frame_end(inst);
    schway__pad_frame_start(inst);
}

static uint8_t signed_mouse_byte(int16_t value)
{
    int32_t clamped = value;
    if (clamped < -128) {
        clamped = -128;
    } else if (clamped > 127) {
        clamped = 127;
    }
    return (uint8_t)(int8_t)clamped;
}

static void set_direct_flight(schway_input *input, uint32_t port, uint16_t buttons,
                              const uint8_t axes[4])
{
    input->device[port] = SCHWAY_INPUT_DEVICE_DUAL_ANALOG_FLIGHT;
    input->buttons[port] = buttons;
    memcpy(input->axes[port], axes, 4u);
}

static void flight_direct_vectors(void)
{
    static const uint8_t endpoints[] = {0u, 128u, 255u};
    static const uint8_t button_bits[] = {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u,
                                          8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u};
    for (uint32_t port = 0u; port < 2u; port++) {
        schway *inst = make();
        configure_sio_port(inst, port);
        for (uint32_t axis = 0u; axis < 4u; axis++) {
            for (uint32_t value = 0u; value < sizeof endpoints; value++) {
                uint8_t axes[] = {17u, 31u, 47u, 63u};
                axes[axis] = endpoints[value];
                schway_input input;
                memset(&input, 0, sizeof input);
                set_direct_flight(&input, port, 0u, axes);
                latch_input(inst, &input);
                uint8_t payload[] = {0xFFu, 0xFFu, axes[0], axes[1], axes[2], axes[3]};
                poll(inst, 0x53u, payload, 6u, NULL);
            }
        }
        for (uint32_t bit = 0u; bit < sizeof button_bits; bit++) {
            uint16_t pressed = (uint16_t)(1u << button_bits[bit]);
            const uint8_t axes[] = {0u, 128u, 255u, 64u};
            schway_input input;
            memset(&input, 0, sizeof input);
            set_direct_flight(&input, port, pressed, axes);
            latch_input(inst, &input);
            uint16_t active_low = (uint16_t)~pressed;
            uint8_t payload[] = {(uint8_t)active_low, (uint8_t)(active_low >> 8),
                                 axes[0], axes[1], axes[2], axes[3]};
            poll(inst, 0x53u, payload, 6u, NULL);
        }

        const uint8_t old_axes[] = {1u, 2u, 3u, 4u};
        schway_input input;
        memset(&input, 0, sizeof input);
        set_direct_flight(&input, port, 1u << 13, old_axes);
        latch_input(inst, &input);
        schway_input next;
        memset(&next, 0, sizeof next);
        const uint8_t new_axes[] = {5u, 6u, 7u, 8u};
        set_direct_flight(&next, port, 1u << 9, new_axes);
        next.size = (uint32_t)sizeof next;
        CHECK_EQ_U64(schway_set_input(inst, &next), SCHWAY_STATUS_OK);
        uint16_t old_buttons = (uint16_t)~(uint16_t)(1u << 13);
        uint8_t old_payload[] = {(uint8_t)old_buttons, (uint8_t)(old_buttons >> 8),
                                 old_axes[0], old_axes[1], old_axes[2], old_axes[3]};
        poll(inst, 0x53u, old_payload, 6u, NULL);
        schway__pad_frame_end(inst);
        schway__pad_frame_start(inst);
        uint16_t new_buttons = (uint16_t)~(uint16_t)(1u << 9);
        uint8_t new_payload[] = {(uint8_t)new_buttons, (uint8_t)(new_buttons >> 8),
                                 new_axes[0], new_axes[1], new_axes[2], new_axes[3]};
        poll(inst, 0x53u, new_payload, 6u, NULL);

        for (uint32_t command = 0u; command < 7u; command++) {
            static const uint8_t unsupported[] = {0x43u, 0x44u, 0x45u, 0x46u,
                                                  0x47u, 0x4Cu, 0x4Du};
            uint8_t received = 0u;
            CHECK_EQ_U64(schway__pad_byte(inst, port, 0x01u, inst->ticks, &received), 1u);
            CHECK_EQ_U64(schway__pad_byte(inst, port, unsupported[command], inst->ticks,
                                          &received), 0u);
            CHECK_EQ_HEX(received, 0xFFu);
            CHECK_EQ_U64(inst->pad.phase[port], 0u);
            CHECK_EQ_U64(inst->pad.config_mode[port], 0u);
            CHECK_EQ_U64(inst->pad.rumble_large[port], 0u);
            CHECK_EQ_U64(inst->pad.rumble_small[port], 0u);
        }

        CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
        configure_sio_port(inst, port);
        poll(inst, 0x41u, (const uint8_t[]){0xFFu, 0xFFu}, 2u, NULL);
        schway_destroy(inst);
    }
}

static void flight_tap_vectors(void)
{
    schway *inst = make();
    schway_input input;
    memset(&input, 0, sizeof input);
    input.multitap[1] = 1u;
    input.controllers[1][0].device = SCHWAY_INPUT_DEVICE_DIGITAL;
    input.controllers[1][3].device = SCHWAY_INPUT_DEVICE_DUAL_ANALOG_FLIGHT;
    input.controllers[1][3].buttons = (uint16_t)((1u << 3) | (1u << 11) | (1u << 14));
    input.controllers[1][3].axes[0] = 0u;
    input.controllers[1][3].axes[1] = 128u;
    input.controllers[1][3].axes[2] = 255u;
    input.controllers[1][3].axes[3] = 64u;
    latch_input(inst, &input);
    configure_sio_port(inst, 1u);
    CHECK_EQ_HEX(exchange(inst, 0x04u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x42u, 1), 0x53u);
    CHECK_EQ_HEX(exchange(inst, 0x00u, 1), 0x5Au);
    uint16_t buttons = (uint16_t)~input.controllers[1][3].buttons;
    uint8_t payload[] = {(uint8_t)buttons, (uint8_t)(buttons >> 8), 0u, 128u, 255u, 64u};
    for (uint32_t i = 0u; i < sizeof payload; i++) {
        CHECK_EQ_HEX(exchange(inst, 0u, i + 1u < sizeof payload), payload[i]);
    }
    schway_destroy(inst);

    inst = make();
    memset(&input, 0, sizeof input);
    input.multitap[0] = 1u;
    input.controllers[0][0].device = SCHWAY_INPUT_DEVICE_DIGITAL;
    input.controllers[0][3].device = SCHWAY_INPUT_DEVICE_DUAL_ANALOG_FLIGHT;
    input.controllers[0][3].buttons = (uint16_t)((1u << 7) | (1u << 12));
    input.controllers[0][3].axes[0] = 255u;
    input.controllers[0][3].axes[1] = 0u;
    input.controllers[0][3].axes[2] = 128u;
    input.controllers[0][3].axes[3] = 1u;
    latch_input(inst, &input);
    configure_sio_port(inst, 0u);
    CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x42u, 1), 0x41u);
    CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0x5Au);
    CHECK_EQ_HEX(exchange(inst, 0x00u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x00u, 0), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x42u, 1), 0x80u);
    CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0x5Au);
    uint16_t flight_buttons = (uint16_t)~input.controllers[0][3].buttons;
    uint8_t flight_record[] = {0x53u, 0x5Au, (uint8_t)flight_buttons,
                               (uint8_t)(flight_buttons >> 8), 255u, 0u, 128u, 1u};
    for (uint32_t i = 0u; i < 32u; i++) {
        uint8_t expected = i < 8u ? (const uint8_t[]){0x41u, 0x5Au, 0xFFu, 0xFFu,
                                                       0xFFu, 0xFFu, 0xFFu, 0xFFu}[i]
                                   : i >= 24u ? flight_record[i - 24u] : 0xFFu;
        CHECK_EQ_HEX(exchange(inst, 0x01u, i < 31u), expected);
    }
    schway_destroy(inst);
}

static void special_flight(void)
{
    flight_direct_vectors();
    flight_tap_vectors();
}

static void set_direct_jogcon(schway_input *input, uint32_t port, uint16_t buttons,
                              uint16_t dial, uint16_t flags)
{
    input->device[port] = SCHWAY_INPUT_DEVICE_JOGCON;
    input->buttons[port] = buttons;
    input->controllers[port][0].device = SCHWAY_INPUT_DEVICE_JOGCON;
    input->controllers[port][0].buttons = buttons;
    input->controllers[port][0].jogcon_dial = dial;
    input->controllers[port][0].jogcon_flags = flags;
}

static void jogcon_direct_vectors(void)
{
    static const uint16_t dial_patterns[] = {
        0x0000u, 0x0001u, 0x1234u, 0x1235u, 0x7FFEu, 0x8001u, 0xFFFEu, 0xFFFFu};
    static const uint8_t button_bits[] = {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u,
                                          8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u};
    for (uint32_t port = 0u; port < 2u; port++) {
        schway *inst = make();
        configure_sio_port(inst, port);
        for (uint32_t i = 0u; i < sizeof dial_patterns / sizeof dial_patterns[0]; i++) {
            schway_input input;
            memset(&input, 0, sizeof input);
            set_direct_jogcon(&input, port, (uint16_t)((1u << 3) | (1u << 13)),
                              dial_patterns[i], (uint16_t)(i & 3u));
            latch_input(inst, &input);
            uint16_t buttons = (uint16_t)~input.buttons[port];
            uint16_t dial = (uint16_t)(dial_patterns[i] & UINT16_C(0xFFFE));
            uint16_t flags = (uint16_t)(i & 3u);
            uint8_t payload[] = {(uint8_t)buttons, (uint8_t)(buttons >> 8),
                                 (uint8_t)dial, (uint8_t)(dial >> 8),
                                 (uint8_t)flags, (uint8_t)(flags >> 8)};
            poll(inst, 0xE3u, payload, 6u, NULL);
            if (i == 0u || i == 7u) {
                poll(inst, 0xE3u, payload, 6u, NULL);
            }
        }
        for (uint32_t i = 0u; i < sizeof button_bits; i++) {
            schway_input input;
            memset(&input, 0, sizeof input);
            uint16_t pressed = (uint16_t)(1u << button_bits[i]);
            set_direct_jogcon(&input, port, pressed, 0xA55Bu, (uint16_t)(i & 3u));
            latch_input(inst, &input);
            uint16_t active_low = (uint16_t)~pressed;
            uint16_t dial = UINT16_C(0xA55A);
            uint16_t flags = (uint16_t)(i & 3u);
            uint8_t payload[] = {(uint8_t)active_low, (uint8_t)(active_low >> 8),
                                 (uint8_t)dial, (uint8_t)(dial >> 8),
                                 (uint8_t)flags, (uint8_t)(flags >> 8)};
            poll(inst, 0xE3u, payload, 6u, NULL);
        }

        schway_input saved_input;
        memset(&saved_input, 0, sizeof saved_input);
        set_direct_jogcon(&saved_input, port, 1u << 7, 0x8001u, 2u);
        latch_input(inst, &saved_input);
        size_t state_size = schway_state_size(inst);
        uint8_t *saved = malloc(state_size);
        CHECK(saved != NULL);
        if (saved != NULL) {
            CHECK_EQ_U64(schway_save_state(inst, saved, state_size), SCHWAY_STATUS_OK);
            schway_input changed;
            memset(&changed, 0, sizeof changed);
            set_direct_jogcon(&changed, port, 1u << 9, 0x1235u, 1u);
            latch_input(inst, &changed);
            CHECK_EQ_U64(schway_load_state(inst, saved, state_size), SCHWAY_STATUS_OK);
            uint16_t active_low = (uint16_t)~(uint16_t)(1u << 7);
            uint16_t dial = UINT16_C(0x8000);
            uint8_t payload[] = {(uint8_t)active_low, (uint8_t)(active_low >> 8),
                                 (uint8_t)dial, (uint8_t)(dial >> 8), 2u, 0u};
            poll(inst, 0xE3u, payload, 6u, NULL);
            free(saved);
        }

        schway_input changed;
        memset(&changed, 0, sizeof changed);
        set_direct_jogcon(&changed, port, 1u << 11, 0x4001u, 1u);
        latch_input(inst, &changed);
        schway_input pending;
        memset(&pending, 0, sizeof pending);
        set_direct_jogcon(&pending, port, 1u << 4, 0xC003u, 2u);
        pending.size = (uint32_t)sizeof pending;
        CHECK_EQ_U64(schway_set_input(inst, &pending), SCHWAY_STATUS_OK);
        uint16_t old_buttons = (uint16_t)~(uint16_t)(1u << 11);
        uint16_t old_dial = UINT16_C(0x4000);
        uint8_t old_payload[] = {(uint8_t)old_buttons, (uint8_t)(old_buttons >> 8),
                                 (uint8_t)old_dial, (uint8_t)(old_dial >> 8), 1u, 0u};
        poll(inst, 0xE3u, old_payload, 6u, NULL);
        schway__pad_frame_end(inst);
        schway__pad_frame_start(inst);
        uint16_t new_buttons = (uint16_t)~(uint16_t)(1u << 4);
        uint16_t new_dial = UINT16_C(0xC002);
        uint8_t new_payload[] = {(uint8_t)new_buttons, (uint8_t)(new_buttons >> 8),
                                 (uint8_t)new_dial, (uint8_t)(new_dial >> 8), 2u, 0u};
        poll(inst, 0xE3u, new_payload, 6u, NULL);

        static const uint8_t unsupported[] = {
            0x00u, 0x43u, 0x44u, 0x45u, 0x46u, 0x47u, 0x4Cu, 0x4Du, 0x99u, 0xFFu};
        inst->pad.rumble_large[port] = 0xA5u;
        inst->pad.rumble_small[port] = 1u;
        inst->pad.old_rumble[port] = 1u;
        inst->pad.rumble_map[port * 6u] = 1u;
        uint16_t latched_buttons = inst->pad.buttons[port];
        uint16_t jogcon_dial = inst->pad.jogcon_dial[port];
        uint16_t jogcon_flags = inst->pad.jogcon_flags[port];
        uint8_t response_mode = inst->pad.response_mode[port];
        uint8_t analog_button = inst->pad.analog_button[port];
        uint8_t analog_lock = inst->pad.analog_lock[port];
        uint8_t selector = inst->pad.selector[port];
        uint8_t rumble_locked = inst->pad.rumble_locked[port];
        uint8_t rumble_map_valid = inst->pad.rumble_map_valid[port];
        uint8_t rumble_stage_large = inst->pad.rumble_stage_large[port];
        uint8_t rumble_stage_small = inst->pad.rumble_stage_small[port];
        uint64_t watchdog_deadline = inst->pad.watchdog_deadline[port];
        uint8_t rumble_map[6];
        uint8_t rumble_map_stage[6];
        memcpy(rumble_map, &inst->pad.rumble_map[port * 6u], sizeof rumble_map);
        memcpy(rumble_map_stage, &inst->pad.rumble_map_stage[port * 6u],
               sizeof rumble_map_stage);
        uint64_t transfers = inst->pad.transfers;
        uint8_t frame_polls = inst->pad.frame_polls;
        for (uint32_t i = 0u; i < sizeof unsupported; i++) {
            schway__pad_abort(inst, port);
            uint8_t received = 0u;
            CHECK_EQ_U64(schway__pad_byte(inst, port, 0x01u, inst->ticks, &received), 1u);
            CHECK_EQ_HEX(received, 0xFFu);
            CHECK_EQ_U64(schway__pad_byte(inst, port, unsupported[i], inst->ticks,
                                          &received), 0u);
            CHECK_EQ_HEX(received, 0xFFu);
            CHECK_EQ_U64(inst->pad.phase[port], 0u);
            CHECK_EQ_U64(inst->pad.command[port], 0u);
            CHECK_EQ_U64(inst->pad.data_index[port], 0u);
            CHECK_EQ_U64(inst->pad.device[port], SCHWAY_INPUT_DEVICE_JOGCON);
            CHECK_EQ_U64(inst->pad.present[port], 1u);
            CHECK_EQ_U64(inst->pad.buttons[port], latched_buttons);
            CHECK_EQ_U64(inst->pad.jogcon_dial[port], jogcon_dial);
            CHECK_EQ_U64(inst->pad.jogcon_flags[port], jogcon_flags);
            CHECK_EQ_U64(inst->pad.config_mode[port], 0u);
            CHECK_EQ_U64(inst->pad.response_mode[port], response_mode);
            CHECK_EQ_U64(inst->pad.analog_mode[port], 0u);
            CHECK_EQ_U64(inst->pad.analog_button[port], analog_button);
            CHECK_EQ_U64(inst->pad.analog_lock[port], analog_lock);
            CHECK_EQ_U64(inst->pad.selector[port], selector);
            CHECK_EQ_U64(inst->pad.rumble_large[port], 0xA5u);
            CHECK_EQ_U64(inst->pad.rumble_small[port], 1u);
            CHECK_EQ_U64(inst->pad.old_rumble[port], 1u);
            CHECK_EQ_U64(inst->pad.rumble_locked[port], rumble_locked);
            CHECK_EQ_U64(inst->pad.rumble_map_valid[port], rumble_map_valid);
            CHECK_EQ_U64(inst->pad.rumble_stage_large[port], rumble_stage_large);
            CHECK_EQ_U64(inst->pad.rumble_stage_small[port], rumble_stage_small);
            CHECK_EQ_U64(inst->pad.watchdog_deadline[port], watchdog_deadline);
            CHECK_EQ_U64(inst->pad.rumble_map[port * 6u], 1u);
            CHECK(memcmp(rumble_map, &inst->pad.rumble_map[port * 6u], sizeof rumble_map) == 0);
            CHECK(memcmp(rumble_map_stage, &inst->pad.rumble_map_stage[port * 6u],
                         sizeof rumble_map_stage) == 0);
            CHECK_EQ_U64(inst->pad.transfers, transfers);
            CHECK_EQ_U64(inst->pad.frame_polls, frame_polls);
        }

        schway_input digital;
        memset(&digital, 0, sizeof digital);
        digital.device[port] = SCHWAY_INPUT_DEVICE_DIGITAL;
        digital.buttons[port] = 1u << 3;
        latch_input(inst, &digital);
        poll(inst, 0x41u, (const uint8_t[]){0xF7u, 0xFFu}, 2u, NULL);
        CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
        configure_sio_port(inst, port);
        poll(inst, 0x41u, (const uint8_t[]){0xFFu, 0xFFu}, 2u, NULL);
        schway_destroy(inst);
    }
}

static void jogcon_tap_vectors(void)
{
    schway *inst = make();
    schway_input input;
    memset(&input, 0, sizeof input);
    input.multitap[1] = 1u;
    input.controllers[1][0].device = SCHWAY_INPUT_DEVICE_DIGITAL;
    input.controllers[1][3].device = SCHWAY_INPUT_DEVICE_JOGCON;
    input.controllers[1][3].buttons = (uint16_t)((1u << 3) | (1u << 7) | (1u << 11));
    input.controllers[1][3].jogcon_dial = 0xA55Bu;
    input.controllers[1][3].jogcon_flags = 3u;
    latch_input(inst, &input);
    configure_sio_port(inst, 1u);
    CHECK_EQ_HEX(exchange(inst, 0x04u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x42u, 1), 0xE3u);
    CHECK_EQ_HEX(exchange(inst, 0x00u, 1), 0x5Au);
    uint16_t buttons = (uint16_t)~input.controllers[1][3].buttons;
    uint16_t dial = UINT16_C(0xA55A);
    uint8_t method2[] = {(uint8_t)buttons, (uint8_t)(buttons >> 8),
                         (uint8_t)dial, (uint8_t)(dial >> 8), 3u, 0u};
    for (uint32_t i = 0u; i < sizeof method2; i++) {
        CHECK_EQ_HEX(exchange(inst, 0u, i + 1u < sizeof method2), method2[i]);
    }
    schway_destroy(inst);

    inst = make();
    memset(&input, 0, sizeof input);
    input.multitap[0] = 1u;
    input.controllers[0][0].device = SCHWAY_INPUT_DEVICE_DIGITAL;
    input.controllers[0][3].device = SCHWAY_INPUT_DEVICE_JOGCON;
    input.controllers[0][3].buttons = (uint16_t)((1u << 3) | (1u << 5) | (1u << 12));
    input.controllers[0][3].jogcon_dial = 0x7FFFu;
    input.controllers[0][3].jogcon_flags = 1u;
    latch_input(inst, &input);
    configure_sio_port(inst, 0u);
    CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x42u, 1), 0x41u);
    CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0x5Au);
    CHECK_EQ_HEX(exchange(inst, 0x00u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x00u, 0), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x42u, 1), 0x80u);
    CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0x5Au);
    uint8_t first_record[] = {0x41u, 0x5Au, 0xFFu, 0xFFu,
                              0xFFu, 0xFFu, 0xFFu, 0xFFu};
    uint16_t method1_buttons = (uint16_t)~input.controllers[0][3].buttons;
    uint16_t method1_dial = UINT16_C(0x7FFE);
    uint8_t jogcon_record[] = {0xE3u, 0x5Au, (uint8_t)method1_buttons,
                               (uint8_t)(method1_buttons >> 8), (uint8_t)method1_dial,
                               (uint8_t)(method1_dial >> 8), 1u, 0u};
    for (uint32_t i = 0u; i < 32u; i++) {
        uint8_t expected = i < 8u ? first_record[i]
                                   : i >= 24u ? jogcon_record[i - 24u] : 0xFFu;
        CHECK_EQ_HEX(exchange(inst, 0x01u, i < 31u), expected);
    }
    schway_destroy(inst);
}

static void special_jogcon(void)
{
    jogcon_direct_vectors();
    jogcon_tap_vectors();
}

static void set_direct_mouse(schway_input *input, uint32_t port, uint16_t buttons,
                             int16_t dx, int16_t dy)
{
    input->device[port] = SCHWAY_INPUT_DEVICE_MOUSE;
    input->buttons[port] = buttons;
    input->controllers[port][0].device = SCHWAY_INPUT_DEVICE_MOUSE;
    input->controllers[port][0].buttons = buttons;
    input->controllers[port][0].mouse_dx = dx;
    input->controllers[port][0].mouse_dy = dy;
}

static void mouse_direct_vectors(void)
{
    static const int16_t values[] = {INT16_MIN, -129, -128, -1, 0, 1, 127, 128, INT16_MAX};
    schway *inst = make();
    configure_sio_port(inst, 0u);
    for (uint32_t i = 0u; i < sizeof values / sizeof values[0]; i++) {
        schway_input input;
        memset(&input, 0, sizeof input);
        input.device[0] = SCHWAY_INPUT_DEVICE_DIGITAL;
        set_direct_mouse(&input, 0u, 1u << 10, values[i], values[i]);
        latch_input(inst, &input);
        uint8_t payload[] = {0xFFu, 0xF8u, signed_mouse_byte(values[i]),
                             signed_mouse_byte(values[i])};
        poll(inst, 0x12u, payload, 4u, NULL);
        if (i == 0u || i == 4u || i == 8u) {
            poll(inst, 0x12u, payload, 4u, NULL);
        }
    }

    schway_input input;
    memset(&input, 0, sizeof input);
    input.device[0] = SCHWAY_INPUT_DEVICE_DIGITAL;
    set_direct_mouse(&input, 0u, 0u, 64, -64);
    latch_input(inst, &input);
    (void)exchange(inst, 0x01u, 1);
    (void)exchange(inst, 0x42u, 1);
    (void)exchange(inst, 0x00u, 1);
    (void)exchange(inst, 0x00u, 1);
    schway__pad_abort(inst, 0u);
    uint8_t payload[] = {0xFFu, 0xFCu, 64u, 0xC0u};
    poll(inst, 0x12u, payload, 4u, NULL);

    schway__pad_frame_end(inst);
    schway__pad_frame_start(inst);
    uint8_t stopped[] = {0xFFu, 0xFCu, 0x00u, 0x00u};
    poll(inst, 0x12u, stopped, 4u, NULL);
    CHECK_EQ_U64(inst->pad.mouse_dx[0], UINT16_C(32768));
    CHECK_EQ_U64(inst->pad.mouse_dy[0], UINT16_C(32768));
    schway__pad_set_connected(inst, 0u, 0u);
    CHECK_EQ_U64(inst->pad.phase[0], 0u);
    CHECK_EQ_U64(inst->pad.mouse_dx[0], UINT16_C(32768));
    CHECK_EQ_U64(inst->pad.mouse_dy[0], UINT16_C(32768));
    schway__pad_reset(inst);
    CHECK_EQ_U64(inst->pad.phase[0], 0u);
    CHECK_EQ_U64(inst->pad.mouse_dx[0], UINT16_C(32768));
    CHECK_EQ_U64(inst->pad.mouse_dy[0], UINT16_C(32768));
    schway_destroy(inst);
}

static void mouse_tap_and_startup_ack(void)
{
    schway *inst = make();
    schway_input input;
    memset(&input, 0, sizeof input);
    input.device[0] = SCHWAY_INPUT_DEVICE_DIGITAL;
    input.device[1] = SCHWAY_INPUT_DEVICE_DIGITAL;
    set_direct_mouse(&input, 1u, 1u << 10, 9, -9);
    latch_input(inst, &input);
    configure_sio_port(inst, 0u);
    size_t state_size = schway_state_size(inst);
    uint8_t *saved = malloc(state_size);
    CHECK(saved != NULL);
    if (saved == NULL) {
        schway_destroy(inst);
        return;
    }
    CHECK_EQ_U64(schway_save_state(inst, saved, state_size), SCHWAY_STATUS_OK);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & 0x80u) != 0u);
    (void)exchange(inst, 0x01u, 1);
    CHECK_EQ_U64(inst->sio.tx_pending, 0u);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & 0x80u) != 0u);
    configure_sio_port(inst, 1u);
    (void)exchange(inst, 0x01u, 1);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & 0x80u) == 0u);
    CHECK_EQ_U64(schway_load_state(inst, saved, state_size), SCHWAY_STATUS_OK);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & 0x80u) != 0u);
    configure_sio_port(inst, 1u);
    (void)exchange(inst, 0x01u, 1);
    CHECK((read_mmio(inst, JOY_STAT, 4u) & 0x80u) == 0u);
    free(saved);

    memset(&input, 0, sizeof input);
    input.multitap[1] = 1u;
    input.controllers[1][3].device = SCHWAY_INPUT_DEVICE_MOUSE;
    input.controllers[1][3].buttons = 1u << 10;
    input.controllers[1][3].mouse_dx = 127;
    input.controllers[1][3].mouse_dy = -128;
    latch_input(inst, &input);
    configure_sio_port(inst, 1u);
    CHECK_EQ_HEX(exchange(inst, 0x04u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x42u, 1), 0x12u);
    CHECK_EQ_HEX(exchange(inst, 0x00u, 1), 0x5Au);
    uint8_t method2[] = {0xFFu, 0xF8u, 0x7Fu, 0x80u};
    for (uint32_t i = 0u; i < sizeof method2; i++) {
        CHECK_EQ_HEX(exchange(inst, 0x00u, i + 1u < sizeof method2), method2[i]);
    }
    schway_destroy(inst);

    inst = make();
    memset(&input, 0, sizeof input);
    input.multitap[0] = 1u;
    input.controllers[0][0].device = SCHWAY_INPUT_DEVICE_DIGITAL;
    input.controllers[0][3].device = SCHWAY_INPUT_DEVICE_MOUSE;
    input.controllers[0][3].buttons = 1u << 10;
    input.controllers[0][3].mouse_dx = 127;
    input.controllers[0][3].mouse_dy = -128;
    latch_input(inst, &input);
    configure_sio_port(inst, 0u);
    CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x42u, 1), 0x41u);
    CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0x5Au);
    CHECK_EQ_HEX(exchange(inst, 0x00u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x00u, 0), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x42u, 1), 0x80u);
    CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0x5Au);
    uint8_t first_record[] = {0x41u, 0x5Au, 0xFFu, 0xFFu,
                              0xFFu, 0xFFu, 0xFFu, 0xFFu};
    uint8_t mouse_record[] = {0x12u, 0x5Au, 0xFFu, 0xF8u, 0x7Fu, 0x80u, 0xFFu, 0xFFu};
    for (uint32_t i = 0u; i < 32u; i++) {
        uint8_t expected = i < 8u ? first_record[i]
                                   : i >= 24u ? mouse_record[i - 24u] : 0xFFu;
        CHECK_EQ_HEX(exchange(inst, 0x01u, i < 31u), expected);
    }
    schway_destroy(inst);
}

static void special_mouse(void)
{
    mouse_direct_vectors();
    mouse_tap_and_startup_ack();
}

static void set_direct_negcon(schway_input *input, uint32_t port, uint16_t buttons,
                              uint8_t twist, uint8_t value_i, uint8_t value_ii, uint8_t value_l)
{
    schway_controller_input *controller = &input->controllers[port][0];
    input->device[port] = SCHWAY_INPUT_DEVICE_NEGCON;
    input->buttons[port] = buttons;
    controller->device = SCHWAY_INPUT_DEVICE_NEGCON;
    controller->buttons = buttons;
    controller->negcon_twist = twist;
    controller->negcon_i = value_i;
    controller->negcon_ii = value_ii;
    controller->negcon_l = value_l;
}

static void negcon_direct_vectors(void)
{
    static const uint8_t button_bits[] = {3u, 4u, 5u, 6u, 7u, 11u, 12u, 13u};
    static const uint8_t raw_values[] = {0u, 128u, 255u};
    for (uint32_t port = 0u; port < 2u; port++) {
        schway *inst = make();
        configure_sio_port(inst, port);
        for (uint32_t i = 0u; i < sizeof raw_values; i++) {
            schway_input input;
            memset(&input, 0, sizeof input);
            set_direct_negcon(&input, port, 0u, raw_values[i], raw_values[i],
                              raw_values[i], raw_values[i]);
            latch_input(inst, &input);
            uint8_t payload[] = {0xFFu, 0xFFu, raw_values[i], raw_values[i],
                                 raw_values[i], raw_values[i]};
            poll(inst, 0x23u, payload, 6u, NULL);
        }
        for (uint32_t i = 0u; i < sizeof button_bits; i++) {
            uint16_t pressed = (uint16_t)(1u << button_bits[i]);
            schway_input input;
            memset(&input, 0, sizeof input);
            set_direct_negcon(&input, port, pressed, 0x80u, 0x11u, 0x22u, 0x33u);
            latch_input(inst, &input);
            uint16_t active_low = (uint16_t)~pressed;
            uint8_t payload[] = {(uint8_t)active_low, (uint8_t)(active_low >> 8),
                                 0x80u, 0x11u, 0x22u, 0x33u};
            poll(inst, 0x23u, payload, 6u, NULL);
        }
        for (uint32_t axis = 0u; axis < 3u; axis++) {
            for (uint32_t i = 0u; i < sizeof raw_values; i++) {
                uint8_t controls[] = {0x31u, 0x52u, 0x73u};
                controls[axis] = raw_values[i];
                schway_input input;
                memset(&input, 0, sizeof input);
                set_direct_negcon(&input, port, 0u, 0xA5u,
                                  controls[0], controls[1], controls[2]);
                latch_input(inst, &input);
                uint8_t payload[] = {0xFFu, 0xFFu, 0xA5u,
                                     controls[0], controls[1], controls[2]};
                poll(inst, 0x23u, payload, 6u, NULL);
            }
        }

        CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0xFFu);
        CHECK_EQ_HEX(exchange(inst, 0x99u, 0), 0xFFu);
        CHECK_EQ_U64(inst->sio.ack_pending, 0u);
        CHECK_EQ_U64(inst->sio.ack_low, 0u);
        CHECK_EQ_U64(inst->pad.phase[port], 0u);
        schway_destroy(inst);
    }
}

static void negcon_tap_methods(void)
{
    schway *inst = make();
    schway_input input;
    memset(&input, 0, sizeof input);
    input.multitap[1] = 1u;
    input.controllers[1][3].device = SCHWAY_INPUT_DEVICE_NEGCON;
    input.controllers[1][3].buttons = (1u << 3) | (1u << 7) | (1u << 11) | (1u << 13);
    input.controllers[1][3].negcon_twist = 0xFEu;
    input.controllers[1][3].negcon_i = 0x12u;
    input.controllers[1][3].negcon_ii = 0xA5u;
    input.controllers[1][3].negcon_l = 0x80u;
    latch_input(inst, &input);
    configure_sio_port(inst, 1u);
    CHECK_EQ_HEX(exchange(inst, 0x04u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x42u, 1), 0x23u);
    CHECK_EQ_HEX(exchange(inst, 0x00u, 1), 0x5Au);
    uint16_t method2_buttons = (uint16_t)~input.controllers[1][3].buttons;
    uint8_t method2[] = {(uint8_t)method2_buttons,
                         (uint8_t)(method2_buttons >> 8), 0xFEu, 0x12u, 0xA5u, 0x80u};
    for (uint32_t i = 0u; i < sizeof method2; i++) {
        CHECK_EQ_HEX(exchange(inst, 0x01u, i + 1u < sizeof method2), method2[i]);
    }
    schway_destroy(inst);

    inst = make();
    memset(&input, 0, sizeof input);
    input.multitap[0] = 1u;
    input.controllers[0][0].device = SCHWAY_INPUT_DEVICE_DIGITAL;
    input.controllers[0][3].device = SCHWAY_INPUT_DEVICE_NEGCON;
    input.controllers[0][3].buttons = (1u << 3) | (1u << 5) | (1u << 12);
    input.controllers[0][3].negcon_twist = 0x7Bu;
    input.controllers[0][3].negcon_i = 0x01u;
    input.controllers[0][3].negcon_ii = 0x80u;
    input.controllers[0][3].negcon_l = 0xFFu;
    latch_input(inst, &input);
    configure_sio_port(inst, 0u);
    CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x42u, 1), 0x41u);
    CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0x5Au);
    CHECK_EQ_HEX(exchange(inst, 0x00u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x00u, 0), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, 0x42u, 1), 0x80u);
    CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0x5Au);
    uint8_t first_record[] = {0x41u, 0x5Au, 0xFFu, 0xFFu,
                              0xFFu, 0xFFu, 0xFFu, 0xFFu};
    uint16_t negcon_buttons = (uint16_t)~input.controllers[0][3].buttons;
    uint8_t negcon_record[] = {0x23u, 0x5Au, (uint8_t)negcon_buttons,
                               (uint8_t)(negcon_buttons >> 8), 0x7Bu, 0x01u, 0x80u, 0xFFu};
    for (uint32_t i = 0u; i < 32u; i++) {
        uint8_t expected = i < 8u ? first_record[i]
                                   : i >= 24u ? negcon_record[i - 24u] : 0xFFu;
        CHECK_EQ_HEX(exchange(inst, 0x01u, i < 31u), expected);
    }
    schway_destroy(inst);
}

static void negcon_instance_isolation(void)
{
    schway *instances[2] = {make(), make()};
    const uint8_t twists[2] = {0x17u, 0xE9u};
    for (uint32_t i = 0u; i < 2u; i++) {
        schway_input input;
        memset(&input, 0, sizeof input);
        set_direct_negcon(&input, 0u, 0u, twists[i], (uint8_t)(i + 1u),
                          (uint8_t)(i + 2u), (uint8_t)(i + 3u));
        latch_input(instances[i], &input);
        configure_sio_port(instances[i], 0u);
        uint8_t payload[] = {0xFFu, 0xFFu, twists[i],
                             (uint8_t)(i + 1u), (uint8_t)(i + 2u), (uint8_t)(i + 3u)};
        poll(instances[i], 0x23u, payload, 6u, NULL);
    }
    schway_destroy(instances[0]);
    schway_destroy(instances[1]);
}

static void special_negcon(void)
{
    negcon_direct_vectors();
    negcon_tap_methods();
    negcon_instance_isolation();
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        return 2;
    }
    if (strcmp(argv[1], "input_latch_and_rumble") == 0) {
        eight_endpoint_input_latch();
        input_latch_and_rumble();
    } else if (strcmp(argv[1], "config_commands") == 0) {
        config_commands();
    } else if (strcmp(argv[1], "analog_button_edges") == 0) {
        analog_button_edges();
    } else if (strcmp(argv[1], "old_rumble") == 0) {
        old_rumble();
    } else if (strcmp(argv[1], "watchdog_boundary") == 0) {
        watchdog_boundary();
    } else if (strcmp(argv[1], "state_split_config") == 0) {
        state_split_config();
    } else if (strcmp(argv[1], "special_mouse") == 0) {
        special_mouse();
    } else if (strcmp(argv[1], "special_negcon") == 0) {
        special_negcon();
    } else if (strcmp(argv[1], "special_flight") == 0) {
        special_flight();
    } else if (strcmp(argv[1], "special_jogcon") == 0) {
        special_jogcon();
    } else {
        return 2;
    }
    CHECK_DONE();
}
