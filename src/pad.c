/* The digital pad protocol and active-low buttons follow the peripheral
   preparation document, section one (.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md). */
#include "internal.h"

#include "state.h"

static const char name_buttons[] = "pad.buttons";
static const char name_present[] = "pad.present";
static const char name_phase[] = "pad.phase";
static const char name_transfers[] = "pad.transfers";

enum { PAD_IDLE, PAD_COMMAND, PAD_ID_END, PAD_BUTTONS_LOW, PAD_BUTTONS_HIGH };

void schway__pad_reset(schway *inst)
{
    for (uint32_t port = 0u; port < 2u; port++) {
        inst->pad.buttons[port] = UINT16_MAX;
        inst->pad.present[port] = 1u;
        inst->pad.phase[port] = PAD_IDLE;
    }
    inst->pad.transfers = 0u;
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
        if (inst->pad.present[port] == 0u) {
            inst->pad.phase[port] = PAD_IDLE;
        }
    }
}

void schway__pad_state_io(struct schway__state_io *io, schway *inst)
{
    schway__state_u16_array(io, name_buttons, inst->pad.buttons, 2u, 0u, UINT16_MAX);
    schway__state_u8_array(io, name_present, inst->pad.present, 2u, 0u, 1u);
    schway__state_u8_array(io, name_phase, inst->pad.phase, 2u, PAD_IDLE, PAD_BUTTONS_HIGH);
    (void)schway__state_u64(io, name_transfers, &inst->pad.transfers, 0u,
                            SCHWAY__STATE_TICKS_MAX);
}

uint32_t schway__pad_byte(schway *inst, uint32_t port, uint8_t sent, uint8_t *received)
{
    if (port >= 2u || inst->pad.present[port] == 0u) {
        *received = 0xFFu;
        return 0u;
    }

    uint8_t *phase = &inst->pad.phase[port];
    *received = 0xFFu;
    if (sent == 0x01u) {
        *phase = PAD_COMMAND;
        return 1u;
    }

    switch (*phase) {
    case PAD_COMMAND:
        if (sent == 0x42u) {
            *received = 0x41u;
            *phase = PAD_ID_END;
            return 1u;
        }
        *phase = PAD_IDLE;
        return 0u;
    case PAD_ID_END:
        *received = 0x5Au;
        *phase = PAD_BUTTONS_LOW;
        return 1u;
    case PAD_BUTTONS_LOW:
        *received = (uint8_t)(inst->pad.buttons[port] & 0xFFu);
        *phase = PAD_BUTTONS_HIGH;
        return 1u;
    case PAD_BUTTONS_HIGH:
        *received = (uint8_t)(inst->pad.buttons[port] >> 8);
        *phase = PAD_IDLE;
        if (inst->pad.transfers < SCHWAY__STATE_TICKS_MAX) {
            inst->pad.transfers++;
        }
        return 0u;
    default:
        *phase = PAD_IDLE;
        return 0u;
    }
}
