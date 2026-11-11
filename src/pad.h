/* The digital controller state and its serial transaction. Not installed. */
#ifndef SCHWAYSTATION_PAD_H
#define SCHWAYSTATION_PAD_H

#include <stdint.h>

#include "schwaystation.h"

struct schway__state_io;

struct schway__pad {
    uint16_t buttons[2];
    uint8_t present[2];
    uint8_t phase[2];
    uint64_t transfers;
};

void schway__pad_reset(schway *inst);
void schway__pad_abort(schway *inst, uint32_t port);
void schway__pad_set_buttons(schway *inst, uint32_t port, uint16_t buttons);
void schway__pad_set_connected(schway *inst, uint32_t port, uint32_t connected);
void schway__pad_state_io(struct schway__state_io *io, schway *inst);
uint32_t schway__pad_byte(schway *inst, uint32_t port, uint8_t sent, uint8_t *received);

#endif /* SCHWAYSTATION_PAD_H */
