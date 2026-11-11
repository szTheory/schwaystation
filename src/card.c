/* Memory-card reads follow the prepared card protocol, including bounded
   sector addressing and instance-owned raw spans. */
#include "internal.h"

#include <string.h>

#include "state.h"

enum { CARD_IDLE, CARD_COMMAND, CARD_READ, CARD_ID };

static const char card1_phase[] = "card.slot1.phase";
static const char card1_command[] = "card.slot1.command";
static const char card1_read_step[] = "card.slot1.read_step";
static const char card1_sector[] = "card.slot1.sector";
static const char card1_checksum[] = "card.slot1.checksum";
static const char card1_invalid_sector[] = "card.slot1.invalid_sector";
static const char card2_phase[] = "card.slot2.phase";
static const char card2_command[] = "card.slot2.command";
static const char card2_read_step[] = "card.slot2.read_step";
static const char card2_sector[] = "card.slot2.sector";
static const char card2_checksum[] = "card.slot2.checksum";
static const char card2_invalid_sector[] = "card.slot2.invalid_sector";

static void reset_slot_protocol(struct schway__card_slot *slot)
{
    slot->phase = CARD_IDLE;
    slot->command = 0u;
    slot->read_step = 0u;
    slot->sector = 0u;
    slot->checksum = 0u;
    slot->invalid_sector = 0u;
}

static uint8_t frame_checksum(const uint8_t *frame)
{
    uint8_t checksum = 0u;
    for (uint32_t i = 0u; i < SCHWAY__CARD_SECTOR_SIZE - 1u; i++) {
        checksum ^= frame[i];
    }
    return checksum;
}

static void initialize_slot(struct schway__card_slot *slot)
{
    memset(slot->bytes, 0xFF, sizeof slot->bytes);
    uint8_t *header = &slot->bytes[0];
    memset(header, 0, SCHWAY__CARD_SECTOR_SIZE);
    header[0] = 0x4Du;
    header[1] = 0x43u;
    header[127] = frame_checksum(header);
    for (uint32_t i = 1u; i < 16u; i++) {
        uint8_t *directory = &slot->bytes[(size_t)i * SCHWAY__CARD_SECTOR_SIZE];
        memset(directory, 0, SCHWAY__CARD_SECTOR_SIZE);
        directory[0] = 0xA0u;
        directory[8] = 0xFFu;
        directory[9] = 0xFFu;
        directory[127] = frame_checksum(directory);
    }
    memcpy(&slot->bytes[63u * SCHWAY__CARD_SECTOR_SIZE], header, SCHWAY__CARD_SECTOR_SIZE);
    slot->generation = 0u;
    slot->flags = 0x08u;
    reset_slot_protocol(slot);
}

void schway__card_init(schway *inst)
{
    initialize_slot(&inst->card.slot[0]);
    initialize_slot(&inst->card.slot[1]);
}

void schway__card_reset(schway *inst)
{
    for (uint32_t i = 0u; i < 2u; i++) {
        struct schway__card_slot *slot = &inst->card.slot[i];
        slot->flags = 0x08u;
        reset_slot_protocol(slot);
    }
}

void schway__card_abort(schway *inst, uint32_t port)
{
    if (port < 2u) {
        reset_slot_protocol(&inst->card.slot[port]);
    } else {
        reset_slot_protocol(&inst->card.slot[0]);
        reset_slot_protocol(&inst->card.slot[1]);
    }
}

static void slot_state_io(struct schway__state_io *io, struct schway__card_slot *slot,
                          const char *phase_name, const char *command_name,
                          const char *read_step_name, const char *sector_name,
                          const char *checksum_name, const char *invalid_sector_name)
{
    uint32_t phase = schway__state_u32(io, phase_name, &slot->phase, CARD_IDLE, CARD_ID);
    uint32_t command = schway__state_u32(io, command_name, &slot->command, 0u, UINT8_MAX);
    uint32_t read_step = schway__state_u32(io, read_step_name, &slot->read_step, 0u, 137u);
    uint32_t sector = schway__state_u32(io, sector_name, &slot->sector, 0u, UINT16_MAX);
    uint32_t checksum = schway__state_u32(io, checksum_name, &slot->checksum, 0u, UINT8_MAX);
    uint32_t invalid_sector =
        schway__state_u32(io, invalid_sector_name, &slot->invalid_sector, 0u, 1u);
    if (io->mode == SCHWAY__STATE_VALIDATE) {
        schway__state_check(io, (phase != CARD_IDLE ||
                                 (command == 0u && read_step == 0u && sector == 0u &&
                                  checksum == 0u && invalid_sector == 0u)) &&
                                (phase != CARD_COMMAND ||
                                 (command == 0u && read_step == 0u)) &&
                                (phase != CARD_READ ||
                                 (command == 0x52u && read_step <= 137u &&
                                  invalid_sector == (read_step < 5u ? 0u :
                                      sector >= SCHWAY__CARD_SECTOR_COUNT ? 1u : 0u))) &&
                                (phase != CARD_ID || (command == 0x53u && read_step <= 7u)));
    }
}

void schway__card_state_io(struct schway__state_io *io, schway *inst)
{
    slot_state_io(io, &inst->card.slot[0], card1_phase, card1_command, card1_read_step,
                  card1_sector, card1_checksum, card1_invalid_sector);
    slot_state_io(io, &inst->card.slot[1], card2_phase, card2_command, card2_read_step,
                  card2_sector, card2_checksum, card2_invalid_sector);
}

static uint32_t begin_command(struct schway__card_slot *slot, uint8_t sent, uint8_t *received)
{
    slot->command = sent;
    slot->read_step = 0u;
    slot->sector = 0u;
    slot->checksum = 0u;
    slot->invalid_sector = 0u;
    if (sent == 0x52u) {
        slot->phase = CARD_READ;
        *received = (uint8_t)slot->flags;
        return SCHWAY__CARD_BYTE_ACK;
    }
    if (sent == 0x53u) {
        slot->phase = CARD_ID;
        *received = (uint8_t)slot->flags;
        return SCHWAY__CARD_BYTE_ACK;
    }
    reset_slot_protocol(slot);
    *received = 0xFFu;
    return SCHWAY__CARD_BYTE_NO_ACK;
}

static uint32_t read_byte(struct schway__card_slot *slot, uint8_t sent, uint8_t *received)
{
    uint32_t step = slot->read_step;
    if (step == 0u) {
        *received = 0x5Au;
    } else if (step == 1u) {
        *received = 0x5Du;
    } else if (step == 2u) {
        *received = 0u;
    } else if (step == 3u) {
        *received = 0u;
        slot->sector = (uint32_t)sent << 8;
        slot->checksum = sent;
    } else if (step == 4u) {
        *received = 0x5Cu;
        slot->sector |= sent;
        slot->checksum ^= sent;
        slot->invalid_sector = slot->sector >= SCHWAY__CARD_SECTOR_COUNT ? 1u : 0u;
    } else if (step == 5u) {
        *received = 0x5Du;
    } else if (step == 6u) {
        *received = slot->invalid_sector != 0u ? 0xFFu : (uint8_t)(slot->sector >> 8);
    } else if (step == 7u) {
        *received = slot->invalid_sector != 0u ? 0xFFu : (uint8_t)slot->sector;
    } else if (step < 136u) {
        uint32_t byte = step - 8u;
        *received = slot->bytes[slot->sector * SCHWAY__CARD_SECTOR_SIZE + byte];
        slot->checksum ^= *received;
    } else if (step == 136u) {
        *received = (uint8_t)slot->checksum;
    } else {
        *received = 0x47u;
        reset_slot_protocol(slot);
        return SCHWAY__CARD_BYTE_NO_ACK;
    }
    slot->read_step++;
    if (slot->invalid_sector != 0u && step == 7u) {
        reset_slot_protocol(slot);
        return SCHWAY__CARD_BYTE_NO_ACK;
    }
    if (step == 6u) {
        return SCHWAY__CARD_BYTE_READ_PAUSE;
    }
    return SCHWAY__CARD_BYTE_ACK;
}

static uint32_t id_byte(struct schway__card_slot *slot, uint8_t *received)
{
    switch (slot->read_step) {
    case 0u:
        *received = 0x5Au;
        break;
    case 1u:
        *received = 0x5Du;
        break;
    case 2u:
        *received = 0x5Cu;
        break;
    case 3u:
        *received = 0x5Du;
        break;
    case 4u:
        *received = 0x04u;
        break;
    case 5u:
    case 6u:
        *received = 0u;
        break;
    default:
        *received = 0x80u;
        reset_slot_protocol(slot);
        return SCHWAY__CARD_BYTE_NO_ACK;
    }
    slot->read_step++;
    return SCHWAY__CARD_BYTE_ACK;
}

uint32_t schway__card_byte(schway *inst, uint32_t port, uint8_t sent, uint8_t *received)
{
    if (port >= 2u || received == NULL) {
        return SCHWAY__CARD_BYTE_UNHANDLED;
    }
    struct schway__card_slot *slot = &inst->card.slot[port];
    if (slot->phase == CARD_IDLE) {
        if (sent != 0x81u) {
            return SCHWAY__CARD_BYTE_UNHANDLED;
        }
        slot->phase = CARD_COMMAND;
        slot->command = 0u;
        slot->read_step = 0u;
        slot->sector = 0u;
        slot->checksum = 0u;
        slot->invalid_sector = 0u;
        *received = 0xFFu;
        return SCHWAY__CARD_BYTE_ACK;
    }
    if (slot->phase == CARD_COMMAND) {
        return begin_command(slot, sent, received);
    }
    if (slot->phase == CARD_READ) {
        return read_byte(slot, sent, received);
    }
    return id_byte(slot, received);
}
