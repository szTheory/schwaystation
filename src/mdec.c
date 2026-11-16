/* The bounded MDEC owner. Block decode is supplied by its first callers; this
   slice establishes storage, MMIO and the deterministic block deadline
   contract (PSX-SPU-CDROM-AND-MDEC, section 4). */
#include <limits.h>
#include <string.h>

#include "internal.h"
#include "state.h"

#define SCHWAY__MDEC_BLOCK_CYCLES 1000u

static void state_i16_array(struct schway__state_io *io, const char *name, int16_t *values,
                            uint32_t count)
{
    uint16_t raw[SCHWAY__MDEC_BLOCK_SAMPLES];
    if (io->mode == SCHWAY__STATE_VISIT) {
        io->visitor(io->user, name, values, count, 2u);
        return;
    }
    for (uint32_t i = 0; i < count; i++) {
        memcpy(&raw[i], &values[i], sizeof raw[i]);
    }
    schway__state_u16_array(io, name, raw, count, 0u, UINT16_MAX);
    if (io->mode == SCHWAY__STATE_APPLY) {
        for (uint32_t i = 0; i < count; i++) {
            memcpy(&values[i], &raw[i], sizeof raw[i]);
        }
    }
}

static void state_i32_array(struct schway__state_io *io, const char *name, int32_t *values,
                            uint32_t count)
{
    uint32_t raw[SCHWAY__MDEC_BLOCK_SAMPLES];
    if (io->mode == SCHWAY__STATE_VISIT) {
        io->visitor(io->user, name, values, count, 4u);
        return;
    }
    for (uint32_t i = 0; i < count; i++) {
        memcpy(&raw[i], &values[i], sizeof raw[i]);
    }
    schway__state_u32_array(io, name, raw, count, 0u, UINT32_MAX);
    if (io->mode == SCHWAY__STATE_APPLY) {
        for (uint32_t i = 0; i < count; i++) {
            memcpy(&values[i], &raw[i], sizeof raw[i]);
        }
    }
}

void schway__mdec_reset(schway *inst)
{
    memset(&inst->mdec, 0, sizeof inst->mdec);
    inst->mdec.block_index = 4u;
    schway__dma_dreq(inst, 0u, 0u);
    schway__dma_dreq(inst, 1u, 0u);
}

void schway__mdec_state_io(struct schway__state_io *io, schway *inst, uint64_t ticks)
{
    struct schway__mdec *mdec = &inst->mdec;
    schway__state_u32_array(io, "mdec.input", mdec->input, SCHWAY__MDEC_INPUT_WORDS, 0u,
                            UINT32_MAX);
    schway__state_u32_array(io, "mdec.output", mdec->output, SCHWAY__MDEC_OUTPUT_WORDS, 0u,
                            UINT32_MAX);
    schway__state_u8_array(io, "mdec.quant", mdec->quant, SCHWAY__MDEC_TABLE_BYTES, 0u,
                           UINT8_MAX);
    state_i16_array(io, "mdec.scale", mdec->scale, SCHWAY__MDEC_BLOCK_SAMPLES);
    state_i16_array(io, "mdec.block", mdec->block, SCHWAY__MDEC_BLOCK_SAMPLES);
    state_i32_array(io, "mdec.work", mdec->work, SCHWAY__MDEC_BLOCK_SAMPLES);
    uint32_t input_head = schway__state_u32(io, "mdec.input_head", &mdec->input_head, 0u,
                                            SCHWAY__MDEC_INPUT_WORDS - 1u);
    uint32_t input_count = schway__state_u32(io, "mdec.input_count", &mdec->input_count, 0u,
                                             SCHWAY__MDEC_INPUT_WORDS);
    uint32_t output_head = schway__state_u32(io, "mdec.output_head", &mdec->output_head, 0u,
                                             SCHWAY__MDEC_OUTPUT_WORDS - 1u);
    uint32_t output_count = schway__state_u32(io, "mdec.output_count", &mdec->output_count, 0u,
                                              SCHWAY__MDEC_OUTPUT_WORDS);
    (void)schway__state_u32(io, "mdec.command", &mdec->command, 0u, UINT32_MAX);
    uint32_t command_words = schway__state_u32(io, "mdec.command_words", &mdec->command_words,
                                               0u, 0xFFFFu);
    uint32_t words_remaining = schway__state_u32(io, "mdec.words_remaining",
                                                 &mdec->words_remaining, 0u, 0xFFFFu);
    uint32_t block_index = schway__state_u32(io, "mdec.block_index", &mdec->block_index, 0u, 5u);
    uint32_t depth = schway__state_u32(io, "mdec.depth", &mdec->depth, 0u, 3u);
    (void)schway__state_u32(io, "mdec.signed_output", &mdec->signed_output, 0u, 1u);
    (void)schway__state_u32(io, "mdec.bit15", &mdec->bit15, 0u, 1u);
    (void)schway__state_u32(io, "mdec.control", &mdec->control, 0u, 3u);
    uint32_t command_active = schway__state_u32(io, "mdec.command_active",
                                                &mdec->command_active, 0u, 1u);
    uint32_t block_pending = schway__state_u32(io, "mdec.block_pending", &mdec->block_pending,
                                               0u, 1u);
    uint64_t block_deadline = schway__state_u64(io, "mdec.block_deadline",
                                                &mdec->block_deadline, 0u,
                                                SCHWAY__STATE_TICKS_MAX);
    uint64_t max = ticks > SCHWAY__STATE_TICKS_MAX - SCHWAY__MDEC_BLOCK_CYCLES
                       ? SCHWAY__STATE_TICKS_MAX
                       : ticks + SCHWAY__MDEC_BLOCK_CYCLES;

    if (io->mode == SCHWAY__STATE_VALIDATE) {
        schway__state_check(io, input_head < SCHWAY__MDEC_INPUT_WORDS &&
                                   input_count <= SCHWAY__MDEC_INPUT_WORDS);
        schway__state_check(io, output_head < SCHWAY__MDEC_OUTPUT_WORDS &&
                                   output_count <= SCHWAY__MDEC_OUTPUT_WORDS);
        schway__state_check(io, block_index <= 5u && depth <= 3u);
        schway__state_check(io, command_words <= 0xFFFFu && words_remaining <= 0xFFFFu);
        schway__state_check(io, command_active == 0u || command_words != 0u ||
                                   words_remaining != 0u);
        schway__state_check(io, block_pending == 0u
                                   ? block_deadline == 0u
                                   : block_deadline >= ticks && block_deadline <= max);
    }
}

/* psx-spx, MDEC bitstream: quantized coefficients arrive in zigzag order. */
static const uint8_t mdec_zigzag[64] = {
    0u, 1u, 8u, 16u, 9u, 2u, 3u, 10u, 17u, 24u, 32u, 25u, 18u, 11u, 4u, 5u,
    12u, 19u, 26u, 33u, 40u, 48u, 41u, 34u, 27u, 20u, 13u, 6u, 7u, 14u, 21u, 28u,
    35u, 42u, 49u, 56u, 57u, 50u, 43u, 36u, 29u, 22u, 15u, 23u, 30u, 37u, 44u, 51u,
    58u, 59u, 52u, 45u, 38u, 31u, 39u, 46u, 53u, 60u, 61u, 54u, 47u, 55u, 62u, 63u,
};

#define SCHWAY__MDEC_STATUS_ADDR 0x1F801824u
#define SCHWAY__MDEC_DATA_ADDR 0x1F801820u
#define SCHWAY__MDEC_DMA_IN_ENABLE (1u << 30)
#define SCHWAY__MDEC_DMA_OUT_ENABLE (1u << 29)

static int32_t mdec_signed10(uint32_t value)
{
    int32_t result = (int32_t)(value & 0x3FFu);
    return (result & 0x200) != 0 ? result - 0x400 : result;
}

static int16_t mdec_signed16(uint32_t value)
{
    int32_t result = (int32_t)(value & 0xFFFFu);
    if ((result & 0x8000) != 0) {
        result -= 0x10000;
    }
    return (int16_t)result;
}

static int16_t mdec_clip11(int64_t value)
{
    if (value > 1023) {
        value = 1023;
    } else if (value < -1024) {
        value = -1024;
    }
    return (int16_t)value;
}

static int32_t mdec_clip8(int64_t value)
{
    if (value > 127) {
        return 127;
    }
    if (value < -128) {
        return -128;
    }
    return (int32_t)value;
}

static int64_t mdec_floor_div(int64_t value, int64_t divisor)
{
    if (value >= 0) {
        return value / divisor;
    }
    return -(((-value) + divisor - 1) / divisor);
}

static uint32_t mdec_input_request(const schway *inst)
{
    const struct schway__mdec *mdec = &inst->mdec;
    return (mdec->control & SCHWAY__MDEC_DMA_IN_ENABLE) != 0u &&
                   mdec->command_active != 0u && mdec->block_pending == 0u &&
                   mdec->input_count < SCHWAY__MDEC_INPUT_WORDS
               ? 1u
               : 0u;
}

static uint32_t mdec_output_request(const schway *inst)
{
    const struct schway__mdec *mdec = &inst->mdec;
    return (mdec->control & SCHWAY__MDEC_DMA_OUT_ENABLE) != 0u && mdec->output_count != 0u
               ? 1u
               : 0u;
}

static void mdec_update_requests(schway *inst)
{
    schway__dma_dreq(inst, 0u, mdec_input_request(inst));
    schway__dma_dreq(inst, 1u, mdec_output_request(inst));
    schway__sched_recalc(inst);
}

static void mdec_input_consume(struct schway__mdec *mdec, uint32_t words)
{
    while (words != 0u && mdec->input_count != 0u) {
        mdec->input_head = (mdec->input_head + 1u) % SCHWAY__MDEC_INPUT_WORDS;
        mdec->input_count--;
        words--;
    }
}

static void mdec_abort_block(schway *inst)
{
    struct schway__mdec *mdec = &inst->mdec;
    mdec->input_head = 0u;
    mdec->input_count = 0u;
    mdec->command_active = 0u;
    mdec->words_remaining = 0u;
    mdec->block_index = 4u;
    mdec_update_requests(inst);
}

static void mdec_idct(struct schway__mdec *mdec, const int16_t coefficients[64])
{
    for (uint32_t y = 0u; y < 8u; y++) {
        for (uint32_t x = 0u; x < 8u; x++) {
            int64_t sum = 0;
            for (uint32_t z = 0u; z < 8u; z++) {
                sum += (int64_t)coefficients[y + z * 8u] * mdec->scale[x + z * 8u];
            }
            mdec->work[y * 8u + x] = (int32_t)mdec_floor_div(sum + 0x8000, 0x10000);
        }
    }
    for (uint32_t y = 0u; y < 8u; y++) {
        for (uint32_t x = 0u; x < 8u; x++) {
            int64_t sum = 0;
            for (uint32_t z = 0u; z < 8u; z++) {
                sum += (int64_t)mdec->work[y + z * 8u] * mdec->scale[x + z * 8u];
            }
            int64_t value = mdec_floor_div(sum + 0x8000, 0x10000);
            if (value > 255) {
                value = 255;
            } else if (value < -256) {
                value = -256;
            }
            mdec->block[y * 8u + x] = (int16_t)mdec_clip8(value);
        }
    }
}

/* Read one bounded block from the input FIFO. A full FIFO or an exhausted
   command without EOB is malformed and is discarded. */
static int mdec_parse_block(schway *inst, uint64_t now)
{
    struct schway__mdec *mdec = &inst->mdec;
    int16_t coefficients[SCHWAY__MDEC_BLOCK_SAMPLES] = {0};
    uint32_t scan = 0u;
    uint32_t have_dc = 0u;
    uint32_t qscale = 0u;

    for (uint32_t word_index = 0u; word_index < mdec->input_count; word_index++) {
        uint32_t fifo_index = (mdec->input_head + word_index) % SCHWAY__MDEC_INPUT_WORDS;
        uint32_t word = mdec->input[fifo_index];
        for (uint32_t half = 0u; half < 2u; half++) {
            uint32_t value = half == 0u ? word & 0xFFFFu : word >> 16;
            if (have_dc == 0u && value == 0xFE00u) {
                continue;
            }
            if (have_dc == 0u) {
                qscale = value >> 10;
                int32_t dc = mdec_signed10(value);
                int64_t dequant = qscale == 0u ? (int64_t)dc * 2
                                                : (int64_t)dc * mdec->quant[0];
                coefficients[0] = mdec_clip11(dequant);
                have_dc = 1u;
                continue;
            }
            if (value == 0xFE00u) {
                mdec_input_consume(mdec, word_index + 1u);
                mdec_idct(mdec, coefficients);
                if (now > UINT64_MAX - SCHWAY__MDEC_BLOCK_CYCLES) {
                    mdec_abort_block(inst);
                    return -1;
                }
                mdec->block_index = 0u;
                mdec->block_pending = 1u;
                mdec->block_deadline = now + SCHWAY__MDEC_BLOCK_CYCLES;
                mdec_update_requests(inst);
                return 1;
            }
            uint32_t next = scan + ((value >> 10) & 0x3Fu) + 1u;
            if (next >= SCHWAY__MDEC_BLOCK_SAMPLES) {
                mdec_abort_block(inst);
                return -1;
            }
            int32_t ac = mdec_signed10(value);
            int64_t dequant = (int64_t)ac * mdec->quant[next] * qscale;
            coefficients[mdec_zigzag[next]] = mdec_clip11((dequant + 4) / 8);
            scan = next;
        }
    }

    if (mdec->input_count >= SCHWAY__MDEC_INPUT_WORDS || mdec->words_remaining == 0u) {
        mdec_abort_block(inst);
        return -1;
    }
    return 0;
}

static void mdec_try_decode(schway *inst, uint64_t now)
{
    struct schway__mdec *mdec = &inst->mdec;
    if (mdec->command_active == 0u || mdec->block_pending != 0u || mdec->input_count == 0u ||
        mdec->depth > 1u) {
        return;
    }
    (void)mdec_parse_block(inst, now);
}

static uint32_t mdec_output_words(const struct schway__mdec *mdec)
{
    return mdec->depth == 0u ? 8u : 16u;
}

static uint32_t mdec_pixel(const struct schway__mdec *mdec, uint32_t pixel)
{
    int32_t sample = mdec_clip8(mdec->block[pixel]);
    uint8_t value = (uint8_t)sample;
    if (mdec->signed_output == 0u) {
        value ^= 0x80u;
    }
    return value;
}

static void mdec_publish_block(schway *inst)
{
    struct schway__mdec *mdec = &inst->mdec;
    uint32_t words = mdec_output_words(mdec);
    if (words > SCHWAY__MDEC_OUTPUT_WORDS - mdec->output_count) {
        return;
    }
    for (uint32_t word_index = 0u; word_index < words; word_index++) {
        uint32_t word = 0u;
        uint32_t pixels = mdec->depth == 0u ? 8u : 4u;
        for (uint32_t i = 0u; i < pixels; i++) {
            uint32_t pixel = word_index * pixels + i;
            uint32_t value = mdec_pixel(mdec, pixel);
            if (mdec->depth == 0u) {
                word |= ((value >> 4) & 0x0Fu) << (4u * i);
            } else {
                word |= value << (8u * i);
            }
        }
        uint32_t tail = (mdec->output_head + mdec->output_count) % SCHWAY__MDEC_OUTPUT_WORDS;
        mdec->output[tail] = word;
        mdec->output_count++;
    }
}

static void mdec_set_table_word(struct schway__mdec *mdec, uint32_t value)
{
    uint32_t index = mdec->command_words - mdec->words_remaining;
    if ((mdec->command >> 29) == 2u) {
        uint32_t offset = (mdec->command & 1u) != 0u && index >= 16u ? 64u : 0u;
        uint32_t table_word = index & 15u;
        for (uint32_t byte = 0u; byte < 4u; byte++) {
            mdec->quant[offset + table_word * 4u + byte] = (uint8_t)(value >> (byte * 8u));
        }
    } else {
        uint32_t scale_index = index * 2u;
        mdec->scale[scale_index] = mdec_signed16(value);
        mdec->scale[scale_index + 1u] = mdec_signed16(value >> 16);
    }
    mdec->words_remaining--;
    if (mdec->words_remaining == 0u) {
        mdec->command_active = 0u;
        mdec->block_index = 4u;
    }
}

static void mdec_start_command(schway *inst, uint32_t command)
{
    struct schway__mdec *mdec = &inst->mdec;
    uint32_t opcode = command >> 29;
    mdec->command = command;
    mdec->input_head = 0u;
    mdec->input_count = 0u;
    mdec->command_words = 0u;
    mdec->words_remaining = 0u;
    mdec->block_index = 4u;
    mdec->command_active = 0u;
    if (opcode == 1u) {
        mdec->depth = (command >> 27) & 3u;
        mdec->signed_output = (command >> 26) & 1u;
        mdec->bit15 = (command >> 25) & 1u;
        mdec->command_words = command & 0xFFFFu;
        mdec->words_remaining = mdec->command_words;
        mdec->command_active = mdec->command_words != 0u && mdec->depth <= 1u ? 1u : 0u;
        mdec->block_index = mdec->command_active != 0u ? 0u : 4u;
    } else if (opcode == 2u || opcode == 3u) {
        mdec->command_words = opcode == 2u ? (((command & 1u) != 0u) ? 32u : 16u) : 32u;
        mdec->words_remaining = mdec->command_words;
        mdec->command_active = 1u;
    }
    mdec_update_requests(inst);
}

static uint32_t mdec_status(const schway *inst)
{
    const struct schway__mdec *mdec = &inst->mdec;
    uint32_t status = 0u;
    if (mdec->output_count == 0u) {
        status |= 1u << 31;
    }
    if (mdec->input_count == SCHWAY__MDEC_INPUT_WORDS) {
        status |= 1u << 30;
    }
    if (mdec->command_active != 0u || mdec->block_pending != 0u) {
        status |= 1u << 29;
    }
    if (mdec_input_request(inst) != 0u) {
        status |= 1u << 28;
    }
    if (mdec_output_request(inst) != 0u) {
        status |= 1u << 27;
    }
    status |= (mdec->depth & 3u) << 25;
    status |= (mdec->signed_output & 1u) << 24;
    status |= (mdec->bit15 & 1u) << 23;
    status |= (mdec->block_index & 7u) << 16;
    if (mdec->words_remaining != 0u) {
        status |= (mdec->words_remaining - 1u) & 0xFFFFu;
    }
    return status;
}

uint64_t schway__mdec_next(const schway *inst)
{
    return inst->mdec.block_pending != 0u ? inst->mdec.block_deadline : UINT64_MAX;
}

void schway__mdec_event(schway *inst, uint64_t now)
{
    struct schway__mdec *mdec = &inst->mdec;
    if (mdec->block_pending != 0u && mdec->block_deadline <= now) {
        mdec->block_pending = 0u;
        mdec->block_deadline = 0u;
        mdec_publish_block(inst);
        if (mdec->output_count == 0u) {
            mdec_abort_block(inst);
            return;
        }
        if (mdec->input_count != 0u) {
            mdec_try_decode(inst, now);
        } else if (mdec->words_remaining == 0u) {
            mdec->command_active = 0u;
            mdec->block_index = 4u;
        }
        mdec_update_requests(inst);
    }
}

uint32_t schway__mdec_read(schway *inst, uint32_t address, uint64_t now)
{
    if (schway__mdec_next(inst) <= now) {
        schway__mdec_event(inst, now);
    }
    if ((address & ~3u) == SCHWAY__MDEC_STATUS_ADDR) {
        return mdec_status(inst);
    }
    uint32_t value = 0u;
    if ((address & ~3u) == SCHWAY__MDEC_DATA_ADDR && inst->mdec.output_count != 0u) {
        struct schway__mdec *mdec = &inst->mdec;
        value = mdec->output[mdec->output_head];
        mdec->output_head = (mdec->output_head + 1u) % SCHWAY__MDEC_OUTPUT_WORDS;
        mdec->output_count--;
        mdec_update_requests(inst);
    }
    return value;
}

void schway__mdec_write(schway *inst, uint32_t address, uint32_t value, uint64_t now)
{
    if (schway__mdec_next(inst) <= now) {
        schway__mdec_event(inst, now);
    }
    uint32_t aligned = address & ~3u;
    if (aligned == SCHWAY__MDEC_STATUS_ADDR) {
        if ((value & 0x80000000u) != 0u) {
            schway__mdec_reset(inst);
        }
        inst->mdec.control = value & (SCHWAY__MDEC_DMA_IN_ENABLE | SCHWAY__MDEC_DMA_OUT_ENABLE);
        mdec_update_requests(inst);
        return;
    }
    if (aligned != SCHWAY__MDEC_DATA_ADDR) {
        return;
    }
    struct schway__mdec *mdec = &inst->mdec;
    if (mdec->command_active == 0u) {
        mdec_start_command(inst, value);
        return;
    }
    uint32_t opcode = mdec->command >> 29;
    if (opcode == 2u || opcode == 3u) {
        if (mdec->words_remaining != 0u) {
            mdec_set_table_word(mdec, value);
        }
    } else if (opcode == 1u && mdec->input_count < SCHWAY__MDEC_INPUT_WORDS) {
        uint32_t tail = (mdec->input_head + mdec->input_count) % SCHWAY__MDEC_INPUT_WORDS;
        mdec->input[tail] = value;
        mdec->input_count++;
        if (mdec->words_remaining != 0u) {
            mdec->words_remaining--;
        }
        mdec_try_decode(inst, now);
    }
    mdec_update_requests(inst);
}

void schway__mdec_dma_write(schway *inst, uint32_t word, uint64_t now)
{
    schway__mdec_write(inst, SCHWAY__MDEC_DATA_ADDR, word, now);
}

uint32_t schway__mdec_dma_read(schway *inst, uint64_t now)
{
    return schway__mdec_read(inst, SCHWAY__MDEC_DATA_ADDR, now);
}

void schway__mdec_io(schway *inst, uint32_t wr, uint32_t address, uint32_t width,
                     uint32_t *value, uint32_t *cycles)
{
    uint32_t sink = 0u;
    if (cycles == NULL) {
        cycles = &sink;
    }
    uint64_t now = schway__sched_now(inst, *cycles);
    uint32_t aligned = address & ~3u;
    uint32_t shift = (address & 3u) * 8u;
    uint32_t mask = width == 4u ? UINT32_MAX : (UINT32_C(1) << (8u * width)) - 1u;
    if (wr != 0u) {
        uint32_t merged;
        if (aligned == SCHWAY__MDEC_DATA_ADDR) {
            merged = (*value & mask) << shift;
        } else {
            uint32_t old = inst->mdec.control;
            uint32_t shifted_mask = mask << shift;
            uint32_t shifted_value = (*value & mask) << shift;
            merged = (old & ~shifted_mask) | shifted_value;
            if ((shifted_value & 0x80000000u) != 0u) {
                merged |= 0x80000000u;
            }
        }
        schway__mdec_write(inst, aligned, merged, now);
    } else {
        *value = (schway__mdec_read(inst, aligned, now) >> shift) & mask;
    }
    if (schway__mdec_next(inst) <= now) {
        schway__mdec_event(inst, now);
    }
    schway__sched_recalc(inst);
}
