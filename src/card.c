/* Memory-card reads follow the prepared card protocol, including bounded
   sector addressing and instance-owned raw spans. */
#include "internal.h"

#include <string.h>

#include "state.h"

enum { CARD_IDLE, CARD_COMMAND, CARD_READ, CARD_ID, CARD_WRITE, CARD_POCKET };

#define CARD_STATE_NAMES(prefix)                                                                    \
    prefix ".pending_error", prefix ".phase", prefix ".command", prefix ".read_step",            \
        prefix ".sector", prefix ".checksum", prefix ".invalid_sector", prefix ".write_step",   \
        prefix ".write_bad_checksum", prefix ".write_data", prefix ".pocket_index",             \
        prefix ".pocket_flags", prefix ".pocket_data", prefix ".pocket_address",                \
        prefix ".pocket_length", prefix ".pocket_step", prefix ".pocket_payload"

static void reset_slot_protocol(struct schway__card_slot *slot)
{
    slot->phase = CARD_IDLE;
    slot->command = 0u;
    slot->read_step = 0u;
    slot->sector = 0u;
    slot->checksum = 0u;
    slot->invalid_sector = 0u;
    slot->write_step = 0u;
    slot->write_bad_checksum = 0u;
    memset(slot->write_data, 0, sizeof slot->write_data);
    memset(slot->pocket_data, 0, sizeof slot->pocket_data);
    slot->pocket_address = 0u;
    slot->pocket_length = 0u;
    slot->pocket_step = 0u;
    memset(slot->pocket_payload, 0, sizeof slot->pocket_payload);
}

static uint8_t frame_checksum(const uint8_t *frame)
{
    uint8_t checksum = 0u;
    for (uint32_t i = 0u; i < SCHWAY__CARD_SECTOR_SIZE - 1u; i++) {
        checksum ^= frame[i];
    }
    return checksum;
}

static uint32_t pocket_le16(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8);
}

static uint32_t pocket_le32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static int pocket_file_chain(const struct schway__card_slot *slot, uint32_t index,
                             uint8_t blocks[15], uint32_t *block_count,
                             uint32_t *file_size)
{
    uint32_t visited = 0u;
    uint32_t block = index;
    const uint8_t *first = NULL;
    uint32_t count = 0u;
    if (index == 0u || index > 15u) {
        return 0;
    }
    for (;;) {
        const uint8_t *directory;
        uint32_t next;
        uint8_t expected_state;
        if (block == 0u || block > 15u || count >= 15u ||
            (visited & (1u << block)) != 0u) {
            return 0;
        }
        visited |= 1u << block;
        directory = &slot->bytes[(size_t)block * SCHWAY__CARD_SECTOR_SIZE];
        if (frame_checksum(directory) != directory[127]) {
            return 0;
        }
        if (count == 0u) {
            uint32_t size;
            first = directory;
            size = pocket_le32(&directory[4]);
            if (size < 0x2000u || size > 0x1E000u || (size & 0x1FFFu) != 0u) {
                return 0;
            }
            *file_size = size;
            *block_count = size / 0x2000u;
        }
        expected_state = count == 0u ? 0x51u : 0x52u;
        if (count + 1u == *block_count) {
            expected_state = count == 0u ? 0x51u : 0x53u;
        }
        if (directory[0] != expected_state) {
            return 0;
        }
        blocks[count++] = (uint8_t)block;
        next = pocket_le16(&directory[8]);
        if (count == *block_count) {
            if (next != 0xFFFFu) {
                return 0;
            }
            return first != NULL;
        }
        if (next == 0xFFFFu || next > 14u) {
            return 0;
        }
        block = next + 1u;
    }
}

static int pocket_map_byte(const struct schway__card_slot *slot, uint32_t index,
                           uint32_t address, uint32_t *offset)
{
    if (address >= 0x08000000u && address <= 0x08FFFFFFu) {
        uint32_t physical = address & 0x1FFFFu;
        if (offset == NULL) {
            return 1;
        }
        *offset = physical;
        return 1;
    }
    if (address >= 0x02000000u && address < 0x02020000u) {
        uint8_t blocks[15];
        uint32_t block_count = 0u;
        uint32_t file_size = 0u;
        uint32_t file_offset = address - 0x02000000u;
        if (file_offset >= 0x20000u ||
            !pocket_file_chain(slot, index, blocks, &block_count, &file_size) ||
            file_offset >= file_size) {
            return 0;
        }
        if (offset != NULL) {
            uint32_t block_number = file_offset / 0x2000u;
            if (block_number >= block_count) {
                return 0;
            }
            *offset = (uint32_t)blocks[block_number] * 0x2000u + (file_offset & 0x1FFFu);
        }
        return 1;
    }
    return 0;
}

static int pocket_transfer_valid(const struct schway__card_slot *slot, uint32_t index,
                                 uint32_t command, uint32_t address, uint32_t length)
{
    uint64_t end = (uint64_t)address + length;
    if (length > SCHWAY__CARD_POCKET_TRANSFER_MAX || end > UINT32_MAX + (uint64_t)1u) {
        return 0;
    }
    /* PSX-SPX PocketStation FUNC 01h: writes over two bytes need word alignment. */
    if (command == 0x5Cu && length > 2u && (address & 3u) != 0u) {
        return 0;
    }
    if (length != 0u && !pocket_map_byte(slot, index, (uint32_t)(end - 1u), NULL)) {
        return 0;
    }
    if (length == 0u && !pocket_map_byte(slot, index, address, NULL)) {
        return 0;
    }
    if (address >= 0x08000000u && address <= 0x08FFFFFFu) {
        if (end > 0x09000000u || ((address & 0x1FFFFu) + length) > 0x20000u) {
            return 0;
        }
    } else if (address >= 0x02000000u && address < 0x02020000u) {
        if (end > 0x02020000u) {
            return 0;
        }
    } else {
        return 0;
    }
    for (uint32_t i = 0u; i < length; i++) {
        if (!pocket_map_byte(slot, index, address + i, NULL)) {
            return 0;
        }
    }
    return 1;
}

static int pending_payload_valid(const struct schway__state_io *io, uint32_t phase,
                                uint32_t sector, uint32_t checksum, uint32_t write_step)
{
    const uint8_t *data = io->r;
    uint32_t data_count = 0u;
    uint8_t expected_checksum = 0u;
    if (phase == CARD_WRITE) {
        if (write_step == 4u) {
            expected_checksum = (uint8_t)(sector >> 8);
        } else if (write_step >= 5u) {
            data_count = write_step - 5u;
            if (data_count > SCHWAY__CARD_SECTOR_SIZE) {
                data_count = SCHWAY__CARD_SECTOR_SIZE;
            }
            expected_checksum = (uint8_t)((sector >> 8) ^ sector);
        }
    }
    int valid = 1;
    for (uint32_t i = 0u; i < SCHWAY__CARD_SECTOR_SIZE; i++) {
        if (i < data_count) {
            expected_checksum = (uint8_t)(expected_checksum ^ data[i]);
        } else if (data[i] != 0u) {
            valid = 0;
        }
    }
    return valid != 0 && (phase != CARD_WRITE || checksum == expected_checksum);
}

static uint32_t pocket_data_count(uint32_t command, uint32_t read_step)
{
    if (read_step <= 1u) {
        return 0u;
    }
    if (command == 0x59u) {
        return read_step - 1u;
    }
    if ((command == 0x5Bu || command == 0x5Cu) && read_step >= 3u && read_step <= 7u) {
        return read_step - 2u;
    }
    if (command == 0x5Du || command == 0x5Eu || command == 0x5Fu) {
        return read_step - 1u;
    }
    return 0u;
}

static int pocket_payload_valid(const struct schway__state_io *io, uint32_t phase,
                                uint32_t command, uint32_t read_step)
{
    const uint8_t *data = io->r;
    uint32_t count = phase == CARD_POCKET ? pocket_data_count(command, read_step) : 0u;
    int valid = 1;
    for (uint32_t i = 0u; i < 6u; i++) {
        if (i >= count && data[i] != 0u) {
            valid = 0;
        }
    }
    return valid;
}

static int pocket_transfer_payload_valid(const struct schway__state_io *io,
                                         const struct schway__card_slot *slot,
                                         uint32_t phase, uint32_t command,
                                         uint32_t read_step, uint32_t index,
                                         uint32_t address, uint32_t length, uint32_t step)
{
    const uint8_t *data = io->r;
    int active = phase == CARD_POCKET && (command == 0x5Bu || command == 0x5Cu);
    int valid = 1;
    if (active != 0 && read_step >= 8u) {
        valid = length <= SCHWAY__CARD_POCKET_TRANSFER_MAX && step <= length &&
                read_step == 8u + step && pocket_transfer_valid(slot, index, command, address, length);
    } else {
        valid = address == 0u && length == 0u && step == 0u;
    }
    for (uint32_t i = 0u; i < SCHWAY__CARD_POCKET_TRANSFER_MAX; i++) {
        int staged = active != 0 && command == 0x5Cu && read_step >= 8u && i < step;
        if (staged == 0 && data[i] != 0u) {
            valid = 0;
        }
    }
    return valid;
}

static int pocket_phase_valid(uint32_t command, uint32_t read_step)
{
    switch (command) {
    case 0x58u: return read_step <= 2u;
    case 0x59u: return read_step <= 6u;
    case 0x5Au: return read_step <= 18u;
    case 0x5Bu:
    case 0x5Cu: return read_step <= 136u;
    case 0x5Du: return read_step <= 3u;
    case 0x5Eu: return read_step <= 3u;
    case 0x5Fu: return read_step <= 1u;
    default: return 0;
    }
}

static int protocol_fields_valid(uint32_t phase, uint32_t command, uint32_t read_step,
                                 uint32_t sector, uint32_t checksum, uint32_t invalid_sector,
                                 uint32_t write_step, uint32_t write_bad_checksum)
{
    if (phase == CARD_IDLE || phase == CARD_COMMAND) {
        return command == 0u && read_step == 0u && sector == 0u && checksum == 0u &&
               invalid_sector == 0u && write_step == 0u && write_bad_checksum == 0u;
    }
    if (phase == CARD_READ) {
        int address_valid = 0;
        if (read_step < 4u) {
            address_valid = sector == 0u && checksum == 0u;
        } else if (read_step == 4u) {
            address_valid = (sector & 0xFFu) == 0u && checksum == (sector >> 8);
        } else if (read_step <= 8u) {
            address_valid = checksum == (uint8_t)((sector >> 8) ^ sector);
        } else {
            address_valid = 1;
        }
        int sector_valid = invalid_sector == 0u;
        if (read_step >= 5u && sector >= SCHWAY__CARD_SECTOR_COUNT) {
            sector_valid = invalid_sector == 1u && read_step <= 7u;
        } else if (read_step >= 5u) {
            sector_valid = invalid_sector == 0u;
        }
        return command == 0x52u && read_step <= 137u && address_valid && sector_valid &&
               write_step == 0u && write_bad_checksum == 0u;
    }
    if (phase == CARD_ID) {
        return command == 0x53u && read_step <= 7u && sector == 0u && checksum == 0u &&
               invalid_sector == 0u && write_step == 0u && write_bad_checksum == 0u;
    }
    if (phase == CARD_WRITE) {
        int address_valid = 0;
        if (write_step < 4u) {
            address_valid = sector == 0u && checksum == 0u;
        } else if (write_step == 4u) {
            address_valid = (sector & 0xFFu) == 0u && checksum == (sector >> 8);
        } else {
            address_valid = sector < SCHWAY__CARD_SECTOR_COUNT;
        }
        return command == 0x57u && read_step == 0u && address_valid && invalid_sector == 0u &&
               write_step <= SCHWAY__CARD_WRITE_FINAL_STEP &&
               (write_bad_checksum == 0u || write_step >= 134u);
    }
    if (phase == CARD_POCKET) {
        return pocket_phase_valid(command, read_step) && sector == 0u && checksum == 0u &&
               invalid_sector == 0u && write_step == 0u && write_bad_checksum == 0u;
    }
    return 0;
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
    slot->pocket_index = 0u;
    slot->pocket_flags = 0u;
    reset_slot_protocol(slot);
}

void schway__card_init(schway *inst)
{
    for (uint32_t i = 0u; i < SCHWAY__CARD_ENDPOINT_COUNT; i++) {
        initialize_slot(&inst->card.slot[i]);
    }
}

void schway__card_reset(schway *inst)
{
    for (uint32_t i = 0u; i < SCHWAY__CARD_ENDPOINT_COUNT; i++) {
        struct schway__card_slot *slot = &inst->card.slot[i];
        slot->flags = 0x08u;
        slot->pocket_index = 0u;
        slot->pocket_flags = 0u;
        reset_slot_protocol(slot);
    }
}

void schway__card_abort(schway *inst, uint32_t endpoint)
{
    if (endpoint < SCHWAY__CARD_ENDPOINT_COUNT) {
        reset_slot_protocol(&inst->card.slot[endpoint]);
    } else if (endpoint == SCHWAY__CARD_ENDPOINT_ALL) {
        for (uint32_t i = 0u; i < SCHWAY__CARD_ENDPOINT_COUNT; i++) {
            reset_slot_protocol(&inst->card.slot[i]);
        }
    } else {
        return;
    }
}

static void slot_state_io(struct schway__state_io *io, struct schway__card_slot *slot,
                          const char *error_name, const char *phase_name, const char *command_name,
                          const char *read_step_name, const char *sector_name,
                          const char *checksum_name, const char *invalid_sector_name,
                          const char *write_step_name, const char *write_bad_checksum_name,
                          const char *write_data_name, const char *pocket_index_name,
                          const char *pocket_flags_name, const char *pocket_data_name,
                          const char *pocket_address_name, const char *pocket_length_name,
                          const char *pocket_step_name, const char *pocket_payload_name)
{
    uint32_t current_error = (slot->flags >> 2) & 1u;
    uint32_t error_pending = schway__state_u32(io, error_name, &current_error, 0u, 1u);
    uint32_t phase = schway__state_u32(io, phase_name, &slot->phase, CARD_IDLE, CARD_POCKET);
    if (io->mode == SCHWAY__STATE_APPLY) {
        /* Fresh-card metadata stays with the receiver; only the pending reply rewinds. */
        slot->flags = (slot->flags & ~0x04u) | (error_pending << 2);
    }
    uint32_t pocket_index = schway__state_u32(io, pocket_index_name, &slot->pocket_index, 0u, 15u);
    uint32_t pocket_flags = schway__state_u32(io, pocket_flags_name, &slot->pocket_flags, 0u, 15u);
    uint32_t pocket_address =
        schway__state_u32(io, pocket_address_name, &slot->pocket_address, 0u, UINT32_MAX);
    uint32_t pocket_length = schway__state_u32(io, pocket_length_name, &slot->pocket_length,
                                                0u, SCHWAY__CARD_POCKET_TRANSFER_MAX);
    uint32_t pocket_step = schway__state_u32(io, pocket_step_name, &slot->pocket_step,
                                              0u, SCHWAY__CARD_POCKET_TRANSFER_MAX);
    uint32_t command = schway__state_u32(io, command_name, &slot->command, 0u, UINT8_MAX);
    uint32_t read_step = schway__state_u32(io, read_step_name, &slot->read_step, 0u, 137u);
    uint32_t sector = schway__state_u32(io, sector_name, &slot->sector, 0u, UINT16_MAX);
    uint32_t checksum = schway__state_u32(io, checksum_name, &slot->checksum, 0u, UINT8_MAX);
    uint32_t invalid_sector =
        schway__state_u32(io, invalid_sector_name, &slot->invalid_sector, 0u, 1u);
    uint32_t write_step = schway__state_u32(io, write_step_name, &slot->write_step, 0u,
                                             SCHWAY__CARD_WRITE_FINAL_STEP);
    uint32_t write_bad_checksum = schway__state_u32(io, write_bad_checksum_name,
                                                     &slot->write_bad_checksum, 0u, 1u);
    int payload_valid = 1;
    int pocket_valid = 1;
    if (io->mode == SCHWAY__STATE_VALIDATE) {
        payload_valid = pending_payload_valid(io, phase, sector, checksum, write_step);
    }
    schway__state_u8_array(io, write_data_name, slot->write_data,
                           SCHWAY__CARD_SECTOR_SIZE, 0u, UINT8_MAX);
    if (io->mode == SCHWAY__STATE_VALIDATE) {
        pocket_valid = pocket_payload_valid(io, phase, command, read_step);
    }
    schway__state_u8_array(io, pocket_data_name, slot->pocket_data,
                           6u, 0u, UINT8_MAX);
    int transfer_valid = 1;
    if (io->mode == SCHWAY__STATE_VALIDATE) {
        transfer_valid = pocket_transfer_payload_valid(io, slot, phase, command, read_step,
                                                        pocket_index, pocket_address,
                                                        pocket_length, pocket_step);
    }
    schway__state_u8_array(io, pocket_payload_name, slot->pocket_payload,
                           SCHWAY__CARD_POCKET_TRANSFER_MAX, 0u, UINT8_MAX);
    if (io->mode == SCHWAY__STATE_VALIDATE) {
        schway__state_check(io, (error_pending == 0u || phase <= CARD_COMMAND) &&
                                    payload_valid != 0 && pocket_valid != 0 &&
                                    transfer_valid != 0 && pocket_step <= pocket_length &&
                                    pocket_index <= 15u && pocket_flags <= 15u &&
                                    protocol_fields_valid(phase, command, read_step, sector,
                                                          checksum, invalid_sector, write_step,
                                                          write_bad_checksum));
    }
}

void schway__card_state_io(struct schway__state_io *io, schway *inst)
{
    slot_state_io(io, &inst->card.slot[0], CARD_STATE_NAMES("card.slot1"));
    slot_state_io(io, &inst->card.slot[1], CARD_STATE_NAMES("card.port1.socketB"));
    slot_state_io(io, &inst->card.slot[2], CARD_STATE_NAMES("card.port1.socketC"));
    slot_state_io(io, &inst->card.slot[3], CARD_STATE_NAMES("card.port1.socketD"));
    slot_state_io(io, &inst->card.slot[4], CARD_STATE_NAMES("card.slot2"));
    slot_state_io(io, &inst->card.slot[5], CARD_STATE_NAMES("card.port2.socketB"));
    slot_state_io(io, &inst->card.slot[6], CARD_STATE_NAMES("card.port2.socketC"));
    slot_state_io(io, &inst->card.slot[7], CARD_STATE_NAMES("card.port2.socketD"));
}

static uint32_t begin_command(struct schway__card_slot *slot, uint8_t sent, uint8_t *received)
{
    slot->command = sent;
    slot->read_step = 0u;
    slot->sector = 0u;
    slot->checksum = 0u;
    slot->invalid_sector = 0u;
    slot->write_step = 0u;
    slot->write_bad_checksum = 0u;
    if (sent == 0x52u) {
        slot->phase = CARD_READ;
        *received = (uint8_t)slot->flags;
        slot->flags &= ~0x04u;
        return SCHWAY__CARD_BYTE_ACK;
    }
    if (sent == 0x53u) {
        slot->phase = CARD_ID;
        *received = (uint8_t)slot->flags;
        slot->flags &= ~0x04u;
        return SCHWAY__CARD_BYTE_ACK;
    }
    if (sent == 0x57u) {
        slot->phase = CARD_WRITE;
        *received = (uint8_t)slot->flags;
        slot->flags &= ~0x04u;
        return SCHWAY__CARD_BYTE_ACK;
    }
    if (sent == 0x58u || sent == 0x59u || sent == 0x5Au || sent == 0x5Bu ||
        sent == 0x5Cu || sent == 0x5Du ||
        sent == 0x5Eu || sent == 0x5Fu) {
        slot->phase = CARD_POCKET;
        *received = (uint8_t)slot->flags;
        slot->flags &= ~0x04u;
        return SCHWAY__CARD_BYTE_ACK;
    }
    reset_slot_protocol(slot);
    *received = 0xFFu;
    return SCHWAY__CARD_BYTE_NO_ACK;
}

/* These bounded replies paraphrase the published transcripts in
   PSX-PERIPHERALS-AND-SAVES.md, section 2.8, Phase 8 storage transcripts. */
static uint32_t pocket_byte(struct schway__card_slot *slot, uint8_t sent, uint8_t *received)
{
    uint32_t step = slot->read_step;
    if (!pocket_phase_valid(slot->command, step)) {
        *received = 0xFFu;
        reset_slot_protocol(slot);
        return SCHWAY__CARD_BYTE_NO_ACK;
    }
    if (slot->command == 0x58u) {
        if (step == 0u) {
            *received = 0x02u;
        } else {
            *received = 0x01u;
        }
        if (step == 2u) {
            reset_slot_protocol(slot);
            return SCHWAY__CARD_BYTE_NO_ACK;
        }
        slot->read_step++;
        return SCHWAY__CARD_BYTE_ACK;
    }
    if (slot->command == 0x59u) {
        if (step == 0u) {
            *received = 0x06u;
        } else if (step == 1u) {
            slot->pocket_data[0] = sent;
            *received = (uint8_t)(slot->pocket_index >> 8);
        } else if (step == 2u) {
            slot->pocket_data[1] = sent;
            *received = (uint8_t)slot->pocket_index;
        } else {
            uint32_t data_index = step - 1u;
            slot->pocket_data[data_index] = sent;
            *received = 0u;
        }
        if (step == 6u) {
            uint32_t new_index = ((uint32_t)slot->pocket_data[0] << 8) | slot->pocket_data[1];
            if (new_index <= 15u) {
                slot->pocket_index = new_index;
            }
            reset_slot_protocol(slot);
            return SCHWAY__CARD_BYTE_NO_ACK;
        }
        slot->read_step++;
        return SCHWAY__CARD_BYTE_ACK;
    }
    if (slot->command == 0x5Au) {
        static const uint8_t date_time[8] = {1u, 1u, 0x99u, 0x19u, 0u, 0u, 0u, 1u};
        if (step == 0u) {
            *received = 0x12u;
        } else if (step == 1u) {
            *received = (uint8_t)(slot->pocket_index >> 8);
        } else if (step == 2u) {
            *received = (uint8_t)slot->pocket_index;
        } else if (step <= 6u) {
            static const uint8_t flag_bits[4] = {0u, 1u, 3u, 2u};
            uint32_t bit = flag_bits[step - 3u];
            *received = (uint8_t)((slot->pocket_flags >> bit) & 1u);
        } else if (step <= 10u) {
            *received = 0u;
        } else {
            *received = date_time[step - 11u];
        }
        if (step == 18u) {
            reset_slot_protocol(slot);
            return SCHWAY__CARD_BYTE_NO_ACK;
        }
        slot->read_step++;
        return SCHWAY__CARD_BYTE_ACK;
    }
    if (slot->command == 0x5Bu || slot->command == 0x5Cu) {
        if (step == 0u) {
            if (sent != 0x01u) {
                *received = 0xFFu;
                reset_slot_protocol(slot);
                return SCHWAY__CARD_BYTE_NO_ACK;
            }
            *received = 0xFFu;
        } else if (step == 1u) {
            if (sent != 0u) {
                *received = 0xFFu;
                reset_slot_protocol(slot);
                return SCHWAY__CARD_BYTE_NO_ACK;
            }
            *received = 0x05u;
        } else if (step >= 2u && step <= 6u) {
            slot->pocket_data[step - 2u] = sent;
            *received = 0u;
        } else if (step == 7u) {
            uint32_t address;
            uint32_t length;
            if (sent != 0u) {
                *received = 0xFFu;
                reset_slot_protocol(slot);
                return SCHWAY__CARD_BYTE_NO_ACK;
            }
            address = pocket_le32(slot->pocket_data);
            length = slot->pocket_data[4];
            if (length > SCHWAY__CARD_POCKET_TRANSFER_MAX) {
                length = SCHWAY__CARD_POCKET_TRANSFER_MAX;
            }
            if (!pocket_transfer_valid(slot, slot->pocket_index, slot->command, address, length)) {
                *received = 0xFFu;
                reset_slot_protocol(slot);
                return SCHWAY__CARD_BYTE_NO_ACK;
            }
            slot->pocket_address = address;
            slot->pocket_length = length;
            slot->pocket_step = 0u;
            memset(slot->pocket_data, 0, sizeof slot->pocket_data);
            memset(slot->pocket_payload, 0, sizeof slot->pocket_payload);
            *received = (uint8_t)length;
        } else if (slot->pocket_step < slot->pocket_length) {
            uint32_t offset;
            if (!pocket_map_byte(slot, slot->pocket_index,
                                 slot->pocket_address + slot->pocket_step, &offset)) {
                *received = 0xFFu;
                reset_slot_protocol(slot);
                return SCHWAY__CARD_BYTE_NO_ACK;
            }
            if (slot->command == 0x5Bu) {
                if (sent != 0u) {
                    *received = 0xFFu;
                    reset_slot_protocol(slot);
                    return SCHWAY__CARD_BYTE_NO_ACK;
                }
                *received = slot->bytes[offset];
            } else {
                slot->pocket_payload[slot->pocket_step] = sent;
                *received = 0u;
            }
            slot->pocket_step++;
        } else {
            if (sent != 0u || !pocket_transfer_valid(slot, slot->pocket_index, slot->command,
                                                     slot->pocket_address, slot->pocket_length)) {
                *received = 0xFFu;
                reset_slot_protocol(slot);
                return SCHWAY__CARD_BYTE_NO_ACK;
            }
            if (slot->command == 0x5Cu && slot->pocket_length != 0u) {
                uint32_t offsets[SCHWAY__CARD_POCKET_TRANSFER_MAX];
                for (uint32_t i = 0u; i < slot->pocket_length; i++) {
                    if (!pocket_map_byte(slot, slot->pocket_index,
                                         slot->pocket_address + i, &offsets[i])) {
                        *received = 0xFFu;
                        reset_slot_protocol(slot);
                        return SCHWAY__CARD_BYTE_NO_ACK;
                    }
                }
                for (uint32_t i = 0u; i < slot->pocket_length; i++) {
                    slot->bytes[offsets[i]] = slot->pocket_payload[i];
                }
                slot->generation++;
                slot->flags &= ~0x08u;
            }
            *received = 0xFFu;
            reset_slot_protocol(slot);
            return SCHWAY__CARD_BYTE_NO_ACK;
        }
        slot->read_step++;
        return SCHWAY__CARD_BYTE_ACK;
    }
    if (slot->command == 0x5Du) {
        if (step == 0u) {
            *received = 0x03u;
        } else {
            slot->pocket_data[step - 1u] = sent;
            *received = 0u;
        }
        if (step == 3u) {
            reset_slot_protocol(slot);
            return SCHWAY__CARD_BYTE_NO_ACK;
        }
        slot->read_step++;
        return SCHWAY__CARD_BYTE_ACK;
    }
    if (slot->command == 0x5Eu) {
        static const uint8_t flag_bits[3] = {1u, 3u, 2u};
        if (step == 0u) {
            *received = 0x03u;
        } else {
            uint32_t bit = flag_bits[step - 1u];
            slot->pocket_data[step - 1u] = sent;
            *received = (uint8_t)((slot->pocket_flags >> bit) & 1u);
        }
        if (step == 3u) {
            int valid = 1;
            for (uint32_t i = 0u; i < 3u; i++) {
                if (slot->pocket_data[i] > 1u) {
                    valid = 0;
                }
            }
            if (valid != 0) {
                for (uint32_t i = 0u; i < 3u; i++) {
                    uint32_t bit = flag_bits[i];
                    uint32_t mask = 1u << bit;
                    slot->pocket_flags = (slot->pocket_flags & ~mask) |
                                         ((uint32_t)slot->pocket_data[i] << bit);
                }
            }
            reset_slot_protocol(slot);
            return SCHWAY__CARD_BYTE_NO_ACK;
        }
        slot->read_step++;
        return SCHWAY__CARD_BYTE_ACK;
    }
    if (slot->command == 0x5Fu) {
        if (step == 0u) {
            *received = 0x01u;
            slot->read_step++;
            return SCHWAY__CARD_BYTE_ACK;
        }
        slot->pocket_data[0] = sent;
        *received = (uint8_t)(slot->pocket_flags & 1u);
        if (sent <= 1u) {
            slot->pocket_flags = (slot->pocket_flags & ~1u) | sent;
        }
        reset_slot_protocol(slot);
        return SCHWAY__CARD_BYTE_NO_ACK;
    }
    *received = 0xFFu;
    reset_slot_protocol(slot);
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

/* The byte sequence follows .planning/preparation/PSX-PERIPHERALS-AND-SAVES.md section 3.3.
   Staging and rejecting bad checksums before changing live bytes are model
   choices so card spans and generation counters remain atomic. */
static uint32_t write_byte(struct schway__card_slot *slot, uint8_t sent, uint8_t *received)
{
    uint32_t step = slot->write_step;
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
        slot->sector |= sent;
        slot->checksum ^= sent;
        if (slot->sector >= SCHWAY__CARD_SECTOR_COUNT) {
            slot->flags |= 0x04u;
            *received = 0xFFu;
            reset_slot_protocol(slot);
            return SCHWAY__CARD_BYTE_NO_ACK;
        }
        *received = 0u;
    } else if (step < 133u) {
        slot->write_data[step - 5u] = sent;
        slot->checksum ^= sent;
        *received = 0u;
    } else if (step == 133u) {
        slot->write_bad_checksum = sent != (uint8_t)slot->checksum;
        *received = 0u;
    } else if (step == 134u) {
        *received = 0x5Cu;
    } else if (step == 135u) {
        *received = 0x5Du;
    } else {
        if (slot->write_bad_checksum != 0u) {
            slot->flags |= 0x04u;
            *received = 0x4Eu;
        } else {
            size_t offset = (size_t)slot->sector * SCHWAY__CARD_SECTOR_SIZE;
            memcpy(&slot->bytes[offset], slot->write_data, sizeof slot->write_data);
            slot->generation++;
            slot->flags &= ~0x08u;
            *received = 0x47u;
        }
        reset_slot_protocol(slot);
        return SCHWAY__CARD_BYTE_NO_ACK;
    }
    slot->write_step++;
    return SCHWAY__CARD_BYTE_ACK;
}

uint32_t schway__card_byte(schway *inst, uint32_t endpoint, uint8_t sent, uint8_t *received)
{
    if (endpoint >= SCHWAY__CARD_ENDPOINT_COUNT || received == NULL) {
        return SCHWAY__CARD_BYTE_UNHANDLED;
    }
    struct schway__card_slot *slot = &inst->card.slot[endpoint];
    if (slot->phase == CARD_IDLE) {
        if (sent != (uint8_t)(0x81u + endpoint % 4u)) {
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
    if (slot->phase == CARD_ID) {
        return id_byte(slot, received);
    }
    if (slot->phase == CARD_WRITE) {
        return write_byte(slot, sent, received);
    }
    if (slot->phase == CARD_POCKET) {
        return pocket_byte(slot, sent, received);
    }
    return SCHWAY__CARD_BYTE_UNHANDLED;
}
