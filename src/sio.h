/* SIO0 registers, bounded RX FIFO and scheduled transport state. Not installed. */
#ifndef SCHWAYSTATION_SIO_H
#define SCHWAYSTATION_SIO_H

#include <stdint.h>

#include "schwaystation.h"

struct schway__state_io;

#define SCHWAY__SIO_RX_CAP 8u

struct schway__sio {
    uint16_t mode, ctrl, baud;
    uint8_t rx[SCHWAY__SIO_RX_CAP];
    uint8_t rx_head, rx_count, last_rx;
    uint8_t tx_data, tx_port;
    uint8_t tx_pending, ack_pending, ack_low;
    uint8_t parity_error, irq;
    uint64_t byte_deadline, ack_deadline, release_deadline;
};

void schway__sio_reset(schway *inst);
void schway__sio_state_io(struct schway__state_io *io, schway *inst, uint64_t ticks);
int schway__sio_io(schway *inst, uint32_t wr, uint32_t addr, uint32_t width,
                   uint32_t *value, uint32_t *cycles);
int schway__sio_peek(const schway *inst, uint32_t addr, uint32_t width, uint32_t *value);
uint64_t schway__sio_next(const schway *inst);
void schway__sio_event(schway *inst, uint64_t now);

#endif /* SCHWAYSTATION_SIO_H */
