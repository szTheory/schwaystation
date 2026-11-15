/* The bounded MDEC owner. Block decode is supplied by its first callers; this
   slice establishes storage, MMIO and the deterministic block deadline
   contract (PSX-SPU-CDROM-AND-MDEC, section 4). */
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
    }
}

uint32_t schway__mdec_read(schway *inst, uint32_t address, uint64_t now)
{
    if (schway__mdec_next(inst) <= now) {
        schway__mdec_event(inst, now);
    }
    (void)address;
    return 0u;
}

void schway__mdec_write(schway *inst, uint32_t address, uint32_t value, uint64_t now)
{
    if (schway__mdec_next(inst) <= now) {
        schway__mdec_event(inst, now);
    }
    (void)address;
    (void)value;
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
        uint32_t old = schway__mdec_read(inst, aligned, now);
        uint32_t merged = (old & ~(mask << shift)) | ((*value & mask) << shift);
        schway__mdec_write(inst, aligned, merged, now);
    } else {
        *value = (schway__mdec_read(inst, aligned, now) >> shift) & mask;
    }
    if (schway__mdec_next(inst) <= now) {
        schway__mdec_event(inst, now);
    }
    schway__sched_recalc(inst);
}
