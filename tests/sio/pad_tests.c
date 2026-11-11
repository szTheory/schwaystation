/* DualShock input and rumble tests through the real SIO0 register path. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

#define JOY_DATA 0x1F801040u
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
    memcpy(expected, inst->pad.rumble_map[0], sizeof expected);
    CHECK_EQ_U64(schway_load_state(inst, saved, state_size), SCHWAY_STATUS_OK);
    for (uint32_t i = 2u; i < 6u; i++) {
        (void)exchange(inst, mapping[i], i != 5u);
    }
    for (uint32_t i = 0u; i < 6u; i++) {
        CHECK_EQ_HEX(inst->pad.rumble_map[0][i], expected[i]);
    }
    free(saved);
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        return 2;
    }
    if (strcmp(argv[1], "input_latch_and_rumble") == 0) {
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
    } else {
        return 2;
    }
    CHECK_DONE();
}
