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

static void send_command(schway *inst, uint8_t command, const uint8_t data[6], uint8_t id)
{
    CHECK_EQ_HEX(exchange(inst, 0x01u, 1), 0xFFu);
    CHECK_EQ_HEX(exchange(inst, command, 1), id);
    CHECK_EQ_HEX(exchange(inst, 0x00u, 1), 0x5Au);
    for (uint32_t i = 0u; i < 6u; i++) {
        (void)exchange(inst, data[i], i != 5u);
    }
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

int main(int argc, char **argv)
{
    if (argc != 2 || strcmp(argv[1], "input_latch_and_rumble") != 0) {
        return 2;
    }
    input_latch_and_rumble();
    CHECK_DONE();
}
