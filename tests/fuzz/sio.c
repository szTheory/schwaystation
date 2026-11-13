/* Hostile SIO0 byte streams: bounded FIFO, ACK edges, aborts and watchdog. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"
#include "schwaystation.h"

#define JOY_DATA 0x1F801040u
#define JOY_STAT 0x1F801044u
#define JOY_MODE 0x1F801048u
#define JOY_CTRL 0x1F80104Au
#define JOY_BAUD 0x1F80104Eu
#define I_STAT 0x1F801070u

static void broken(const char *what)
{
    fprintf(stderr, "fuzz.regress.sio: %s\n", what);
    abort();
}

static void *alloc_bytes(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void free_bytes(void *user, void *ptr, size_t size)
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
    cfg.allocator.alloc = alloc_bytes;
    cfg.allocator.free = free_bytes;
    schway *inst = NULL;
    if (schway_create(&cfg, &inst) != SCHWAY_STATUS_OK || inst == NULL) {
        broken("instance creation failed");
    }
    return inst;
}

static void write_mmio(schway *inst, uint32_t address, uint32_t width, uint32_t value)
{
    uint32_t cycles = 0u;
    if (schway__bus_write(inst, address, width, value, &cycles) != 0u) {
        broken("SIO bus write was refused");
    }
}

static uint32_t read_mmio(schway *inst, uint32_t address, uint32_t width)
{
    uint32_t cycles = 0u;
    uint32_t value = 0u;
    if (schway__bus_read(inst, address, width, &value, &cycles) != 0u) {
        broken("SIO bus read was refused");
    }
    return value;
}

static void advance(schway *inst, uint64_t tick)
{
    inst->ticks = tick;
    schway__sched_sync(inst);
    schway__sched_run(inst);
}

static uint8_t exchange(schway *inst, uint8_t sent, uint8_t expected, int ack)
{
    uint64_t start = inst->ticks;
    write_mmio(inst, JOY_DATA, 1u, sent);
    advance(inst, start + 1087u);
    if ((read_mmio(inst, JOY_STAT, 4u) & 2u) != 0u) {
        broken("RX byte arrived before 1088 ticks");
    }
    advance(inst, start + 1088u);
    uint32_t status = read_mmio(inst, JOY_STAT, 4u);
    uint8_t received = (uint8_t)read_mmio(inst, JOY_DATA, 1u);
    if ((status & 2u) == 0u || received != expected) {
        fprintf(stderr, "sent %02x received %02x expected %02x at %llu status %08x\n", sent,
                received, expected, (unsigned long long)start, status);
        broken("scheduled RX byte or independent reply changed");
    }
    if (ack) {
        advance(inst, start + 1425u);
        if ((inst->irq.stat & 0x80u) != 0u) {
            broken("IRQ7 arrived before the ACK boundary");
        }
        advance(inst, start + 1426u);
        if ((inst->irq.stat & 0x80u) == 0u || inst->sio.ack_low == 0u) {
            broken("ACK/IRQ7 did not assert at its scheduled boundary");
        }
        write_mmio(inst, I_STAT, 4u, 0u);
        write_mmio(inst, JOY_CTRL, 2u, 0x1013u);
        advance(inst, start + 1494u);
        write_mmio(inst, JOY_CTRL, 2u, 0x1003u);
    } else if (inst->sio.ack_pending != 0u || (inst->irq.stat & 0x80u) != 0u) {
        broken("final pad byte incorrectly scheduled an ACK");
    }
    return (uint8_t)expected;
}

static void configure(schway *inst)
{
    write_mmio(inst, JOY_MODE, 2u, 0x000Cu);
    write_mmio(inst, JOY_BAUD, 2u, 0x0088u);
    write_mmio(inst, JOY_CTRL, 2u, 0x1003u);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    schway *inst = make();
    configure(inst);
    if (size != 0u && (data[0] & 1u) != 0u) {
        /* Incomplete TX then deselect: neither the stale byte nor ACK may land. */
        write_mmio(inst, JOY_DATA, 1u, 0x01u);
        advance(inst, 500u);
        write_mmio(inst, JOY_CTRL, 2u, 0x1001u);
        advance(inst, 2000u);
        if (inst->sio.tx_pending != 0u || inst->sio.ack_pending != 0u || inst->sio.rx_count != 0u ||
            (inst->irq.stat & 0x80u) != 0u) {
            broken("deselect left a scheduled byte or ACK behind");
        }
    } else {
        exchange(inst, 0x01u, 0xFFu, 1);
        exchange(inst, 0x42u, 0x41u, 1);
        exchange(inst, 0x00u, 0x5Au, 1);
        exchange(inst, 0x00u, 0xFFu, 1);
        exchange(inst, 0x00u, 0xFFu, 0);
        if (inst->sio.rx_count > SCHWAY__SIO_RX_CAP || inst->pad.transfers != 1u) {
            broken("pad poll count or FIFO bound changed");
        }
        uint64_t deadline = inst->pad.watchdog_deadline[0];
        if (deadline != 0u) {
            schway__pad_update_watchdog(inst, deadline - 1u);
            if (inst->pad.config_mode[0] == 0u) {
                broken("pad watchdog fired early");
            }
            schway__pad_update_watchdog(inst, deadline);
            if (inst->pad.config_mode[0] != 0u || inst->pad.analog_mode[0] != 0u) {
                broken("pad watchdog did not close config and analog modes");
            }
        }
    }
    if (inst->sio.rx_count != 0u || (read_mmio(inst, JOY_STAT, 4u) & 2u) != 0u) {
        broken("completed SIO input left the RX FIFO nonempty");
    }
    (void)read_mmio(inst, JOY_DATA, 1u);
    if (inst->sio.rx_count != 0u) {
        broken("an empty FIFO read changed its count");
    }

    /* Drive the config command on a latched DualShock, then hit both watchdog
     * edges. */
    schway_input input;
    memset(&input, 0, sizeof input);
    input.size = (uint32_t)sizeof input;
    input.device[0] = SCHWAY_INPUT_DEVICE_DUALSHOCK;
    for (uint32_t axis = 0u; axis < 4u; ++axis) {
        input.axes[0][axis] = 128u;
    }
    if (schway_set_input(inst, &input) != SCHWAY_STATUS_OK ||
        schway_run_frame(inst, NULL) != SCHWAY_STATUS_OK) {
        broken("DualShock input could not be latched");
    }
    configure(inst);
    exchange(inst, 0x01u, 0xFFu, 1);
    exchange(inst, 0x43u, 0x73u, 1);
    exchange(inst, 0x00u, 0x5Au, 1);
    exchange(inst, 0x01u, 0xFFu, 1);
    exchange(inst, 0x00u, 0xFFu, 1);
    exchange(inst, 0x00u, 128u, 1);
    exchange(inst, 0x00u, 128u, 1);
    exchange(inst, 0x00u, 128u, 1);
    exchange(inst, 0x00u, 128u, 0);
    uint64_t deadline = inst->pad.watchdog_deadline[0];
    if (inst->pad.config_mode[0] == 0u || deadline < inst->ticks ||
        deadline - inst->ticks != UINT64_C(33868800)) {
        broken("DualShock config watchdog was not scheduled at one second");
    }
    schway__pad_update_watchdog(inst, deadline - 1u);
    if (inst->pad.config_mode[0] == 0u) {
        broken("DualShock watchdog fired early");
    }
    schway__pad_update_watchdog(inst, deadline);
    if (inst->pad.config_mode[0] != 0u || inst->pad.analog_mode[0] != 0u ||
        inst->pad.rumble_locked[0] == 0u) {
        broken("DualShock watchdog did not restore the digital locked state");
    }

    uint64_t before_run = inst->ticks;
    schway_budget budget;
    schway_run_result result;
    memset(&budget, 0, sizeof budget);
    memset(&result, 0, sizeof result);
    budget.size = (uint32_t)sizeof budget;
    budget.max_cycles = 32u;
    result.size = (uint32_t)sizeof result;
    if (schway_run(inst, &budget, &result) != SCHWAY_STATUS_OK || result.ticks < before_run ||
        result.ticks - before_run > 32u) {
        broken("public run exceeded its SIO replay bound");
    }
    schway_destroy(inst);
    return 0;
}

#define SCHWAY_REPLAY_NAME "fuzz.regress.sio"
#include "replay_main.h"
