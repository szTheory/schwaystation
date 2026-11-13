/* Memory-card storage and bounded SIO command phases. Not installed. */
#ifndef SCHWAYSTATION_CARD_H
#define SCHWAYSTATION_CARD_H

#include <stdint.h>

#include "schwaystation.h"

struct schway__state_io;

#define SCHWAY__CARD_SIZE SCHWAY_MEMCARD_SIZE
#define SCHWAY__CARD_SECTOR_SIZE 128u
#define SCHWAY__CARD_SECTOR_COUNT 1024u
#define SCHWAY__CARD_ACK_FROM_TX 1500u
#define SCHWAY__CARD_READ_PAUSE_FROM_TX 31000u
#define SCHWAY__CARD_WRITE_FINAL_STEP 136u

enum {
    SCHWAY__CARD_BYTE_UNHANDLED = 0u,
    SCHWAY__CARD_BYTE_ACK,
    SCHWAY__CARD_BYTE_READ_PAUSE,
    SCHWAY__CARD_BYTE_NO_ACK
};

struct schway__card_slot {
    uint8_t bytes[SCHWAY__CARD_SIZE];
    uint64_t generation;
    uint32_t flags;
    uint32_t phase;
    uint32_t command;
    uint32_t read_step;
    uint32_t sector;
    uint32_t checksum;
    uint32_t invalid_sector;
    uint32_t write_step;
    uint32_t write_bad_checksum;
    uint8_t write_data[SCHWAY__CARD_SECTOR_SIZE];
};

struct schway__card {
    struct schway__card_slot slot[2];
};

void schway__card_init(schway *inst);
void schway__card_reset(schway *inst);
void schway__card_abort(schway *inst, uint32_t port);
void schway__card_state_io(struct schway__state_io *io, schway *inst);
uint32_t schway__card_byte(schway *inst, uint32_t port, uint8_t sent, uint8_t *received);

#endif /* SCHWAYSTATION_CARD_H */
