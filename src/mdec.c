/* The bounded MDEC owner decodes coefficient blocks and publishes DMA output
   on deterministic deadlines (PSX-SPU-CDROM-AND-MDEC, section 4). */
#include <limits.h>
#include <string.h>

#include "internal.h"
#include "state.h"

#define SCHWAY__MDEC_BLOCK_CYCLES 1000u
#define SCHWAY__MDEC_STATUS_ADDR 0x1F801824u
#define SCHWAY__MDEC_DATA_ADDR 0x1F801820u
#define SCHWAY__MDEC_DMA_IN_ENABLE (1u << 30)
#define SCHWAY__MDEC_DMA_OUT_ENABLE (1u << 29)

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
    state_i16_array(io, "mdec.chroma_cr", mdec->chroma_cr, SCHWAY__MDEC_BLOCK_SAMPLES);
    state_i16_array(io, "mdec.chroma_cb", mdec->chroma_cb, SCHWAY__MDEC_BLOCK_SAMPLES);
    schway__state_u8_array(io, "mdec.color_stage", mdec->color_stage,
                           (uint32_t)sizeof mdec->color_stage, 0u, UINT8_MAX);
    uint32_t input_head = schway__state_u32(io, "mdec.input_head", &mdec->input_head, 0u,
                                            SCHWAY__MDEC_INPUT_WORDS - 1u);
    uint32_t input_count = schway__state_u32(io, "mdec.input_count", &mdec->input_count, 0u,
                                             SCHWAY__MDEC_INPUT_WORDS);
    uint32_t input_half = schway__state_u32(io, "mdec.input_half", &mdec->input_half, 0u, 1u);
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
    uint32_t parse_scan = schway__state_u32(io, "mdec.parse_scan", &mdec->parse_scan, 0u, 63u);
    uint32_t parse_qscale = schway__state_u32(io, "mdec.parse_qscale", &mdec->parse_qscale,
                                               0u, 63u);
    uint32_t parse_have_dc = schway__state_u32(io, "mdec.parse_have_dc",
                                               &mdec->parse_have_dc, 0u, 1u);
    (void)schway__state_u32(io, "mdec.signed_output", &mdec->signed_output, 0u, 1u);
    (void)schway__state_u32(io, "mdec.bit15", &mdec->bit15, 0u, 1u);
    uint32_t control = schway__state_u32(io, "mdec.control", &mdec->control, 0u, UINT32_MAX);
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
                                   input_count <= SCHWAY__MDEC_INPUT_WORDS && input_half <= 1u &&
                                   (input_count != 0u || input_half == 0u));
        schway__state_check(io, output_head < SCHWAY__MDEC_OUTPUT_WORDS &&
                                   output_count <= SCHWAY__MDEC_OUTPUT_WORDS);
        schway__state_check(io, block_index <= 5u && depth <= 3u);
        schway__state_check(io, parse_scan <= 63u && parse_qscale <= 63u && parse_have_dc <= 1u);
        schway__state_check(io, parse_have_dc != 0u ||
                                   (parse_scan == 0u && parse_qscale == 0u));
        schway__state_check(io, parse_have_dc == 0u ||
                                   (command_active != 0u && block_pending == 0u));
        schway__state_check(io, command_words <= 0xFFFFu && words_remaining <= 0xFFFFu);
        schway__state_check(io, command_active == 0u || command_words != 0u ||
                                   words_remaining != 0u);
        schway__state_check(io, (control & ~(SCHWAY__MDEC_DMA_IN_ENABLE |
                                             SCHWAY__MDEC_DMA_OUT_ENABLE)) == 0u);
        uint32_t output_words = depth == 0u ? 8u : depth == 1u ? 16u :
                                depth == 2u && (block_index == 1u || block_index == 3u) ? 96u :
                                depth == 3u && block_index < 4u ? 32u : 0u;
        uint32_t output_block_fits = output_words == 0u ||
                                     output_count <= SCHWAY__MDEC_OUTPUT_WORDS - output_words;
        schway__state_check(io, block_pending == 0u
                                   ? block_deadline == 0u
                                   : (block_deadline == 0u
                                          ? output_words != 0u && output_block_fits == 0u
                                          : block_deadline >= ticks && block_deadline <= max));
    }
}

/* psx-spx, MDEC bitstream: quantized coefficients arrive in zigzag order. */
static const uint8_t mdec_zigzag[64] = {
    0u, 1u, 8u, 16u, 9u, 2u, 3u, 10u, 17u, 24u, 32u, 25u, 18u, 11u, 4u, 5u,
    12u, 19u, 26u, 33u, 40u, 48u, 41u, 34u, 27u, 20u, 13u, 6u, 7u, 14u, 21u, 28u,
    35u, 42u, 49u, 56u, 57u, 50u, 43u, 36u, 29u, 22u, 15u, 23u, 30u, 37u, 44u, 51u,
    58u, 59u, 52u, 45u, 38u, 31u, 39u, 46u, 53u, 60u, 61u, 54u, 47u, 55u, 62u, 63u,
};

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

static int32_t mdec_clip9(int64_t value)
{
    value %= 512;
    if (value >= 256) {
        value -= 512;
    } else if (value < -256) {
        value += 512;
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
                   mdec->input_count == 0u
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
    mdec->input_half = 0u;
    mdec->command_active = 0u;
    mdec->words_remaining = 0u;
    mdec->block_index = 4u;
    mdec->parse_scan = 0u;
    mdec->parse_qscale = 0u;
    mdec->parse_have_dc = 0u;
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
            int64_t value = mdec_clip9(mdec_floor_div(sum + 0x8000, 0x10000));
            mdec->block[y * 8u + x] = (int16_t)mdec_clip8(value);
        }
    }
}

static int mdec_finish_block(schway *inst, const int16_t coefficients[64],
                             uint32_t consumed_words, uint32_t next_half, uint64_t now)
{
    struct schway__mdec *mdec = &inst->mdec;
    mdec_input_consume(mdec, consumed_words);
    mdec->input_half = next_half;
    mdec_idct(mdec, coefficients);
    mdec->parse_scan = 0u;
    mdec->parse_qscale = 0u;
    mdec->parse_have_dc = 0u;
    if (now > UINT64_MAX - SCHWAY__MDEC_BLOCK_CYCLES) {
        mdec_abort_block(inst);
        return -1;
    }
    if (mdec->depth < 2u) {
        mdec->block_index = 0u;
    }
    mdec->block_pending = 1u;
    mdec->block_deadline = now + SCHWAY__MDEC_BLOCK_CYCLES;
    mdec_update_requests(inst);
    return 1;
}

/* Continue one block across bounded FIFO refills. The parser consumes complete
   input words after saving coefficient state, so a legal dense block never
   needs to fit in the 32-word FIFO. */
static int mdec_parse_block(schway *inst, uint64_t now)
{
    struct schway__mdec *mdec = &inst->mdec;

    for (uint32_t word_index = 0u; word_index < mdec->input_count; word_index++) {
        uint32_t fifo_index = (mdec->input_head + word_index) % SCHWAY__MDEC_INPUT_WORDS;
        uint32_t word = mdec->input[fifo_index];
        uint32_t first_half = word_index == 0u ? mdec->input_half : 0u;
        for (uint32_t half = first_half; half < 2u; half++) {
            uint32_t value = half == 0u ? word & 0xFFFFu : word >> 16;
            if (mdec->parse_have_dc == 0u && value == 0xFE00u) {
                continue;
            }
            if (mdec->parse_have_dc == 0u) {
                memset(mdec->block, 0, sizeof mdec->block);
                mdec->parse_qscale = value >> 10;
                int32_t dc = mdec_signed10(value);
                uint32_t quant_offset = mdec->depth >= 2u && mdec->block_index >= 4u ? 64u : 0u;
                int64_t dequant = mdec->parse_qscale == 0u ? (int64_t)dc * 2
                                                            : (int64_t)dc * mdec->quant[quant_offset];
                mdec->block[0] = mdec_clip11(dequant);
                mdec->parse_have_dc = 1u;
                continue;
            }
            if (value == 0xFE00u) {
                return mdec_finish_block(inst, mdec->block,
                                         word_index + (half == 0u ? 0u : 1u),
                                         half == 0u ? 1u : 0u, now);
            }
            uint32_t next = mdec->parse_scan + ((value >> 10) & 0x3Fu) + 1u;
            if (next >= SCHWAY__MDEC_BLOCK_SAMPLES) {
                /* A run that reaches the end of the 64 coefficients closes
                   the block; ares uses the same rule for real MDEC streams. */
                return mdec_finish_block(inst, mdec->block,
                                         word_index + (half == 0u ? 0u : 1u),
                                         half == 0u ? 1u : 0u, now);
            }
            int32_t ac = mdec_signed10(value);
            uint32_t quant_offset = mdec->depth >= 2u && mdec->block_index >= 4u ? 64u : 0u;
            uint32_t coefficient = next;
            int64_t dequant;
            if (mdec->parse_qscale == 0u) {
                /* Zero-scale blocks bypass quantization and zigzag (MDEC guide, Commands and input). */
                dequant = (int64_t)ac * 2;
            } else {
                coefficient = mdec_zigzag[next];
                dequant = (int64_t)ac * mdec->quant[quant_offset + next] *
                          mdec->parse_qscale;
                dequant = (dequant + 4) / 8;
            }
            mdec->block[coefficient] = mdec_clip11(dequant);
            mdec->parse_scan = next;
        }
    }

    /* The pinned mono caller ends exactly after coefficient 63 without EOB. */
    if (mdec->command_words == 32u && mdec->words_remaining == 0u &&
        mdec->parse_have_dc != 0u && mdec->parse_scan == 63u) {
        return mdec_finish_block(inst, mdec->block, mdec->input_count, 0u, now);
    }
    uint32_t consumed_words = mdec->input_count;
    mdec_input_consume(mdec, consumed_words);
    mdec->input_half = 0u;
    if (mdec->words_remaining == 0u) {
        if (mdec->parse_have_dc != 0u) {
            mdec_abort_block(inst);
            return -1;
        }
        mdec->command_active = 0u;
        mdec->block_index = 4u;
        mdec_update_requests(inst);
        return 1;
    }
    return 0;
}

static void mdec_try_decode(schway *inst, uint64_t now)
{
    struct schway__mdec *mdec = &inst->mdec;
    if (mdec->command_active == 0u || mdec->block_pending != 0u || mdec->input_count == 0u ||
        mdec->depth > 3u ||
        (mdec->depth >= 2u && mdec->block_index == 4u && mdec->output_count != 0u)) {
        return;
    }
    (void)mdec_parse_block(inst, now);
}

static uint32_t mdec_output_words(const struct schway__mdec *mdec)
{
    if (mdec->depth == 0u) {
        return 8u;
    }
    if (mdec->depth == 1u) {
        return 16u;
    }
    if (mdec->depth == 2u) {
        return mdec->block_index == 1u || mdec->block_index == 3u ? 96u : 0u;
    }
    return mdec->block_index < 4u ? 32u : 0u;
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

/* MDEC color conversion uses signed 8-bit Y, Cr and Cb samples, clips each
   channel to -128..127, then XORs bit 7 for unsigned output (PSX-SPU-CDROM-
   AND-MDEC, section 4). Coefficients are the locked Q16 values from that
   section; integer floor division keeps negative products portable. */
static void mdec_color_pixel(const struct schway__mdec *mdec, uint32_t block,
                             uint32_t x, uint32_t y, uint8_t rgb[3])
{
    uint32_t block_x = (block & 1u) * 8u;
    uint32_t block_y = (block >> 1u) * 8u;
    uint32_t chroma_index = ((block_y + y) / 2u) * 8u + (block_x + x) / 2u;
    int32_t luminance = mdec->block[y * 8u + x];
    int32_t cr = mdec->chroma_cr[chroma_index];
    int32_t cb = mdec->chroma_cb[chroma_index];
    int64_t red = luminance + mdec_floor_div((int64_t)cr * 91881, 65536);
    int64_t green = luminance - mdec_floor_div((int64_t)cb * 22524, 65536) -
                    mdec_floor_div((int64_t)cr * 46811, 65536);
    int64_t blue = luminance + mdec_floor_div((int64_t)cb * 116130, 65536);
    rgb[0] = (uint8_t)mdec_clip8(red);
    rgb[1] = (uint8_t)mdec_clip8(green);
    rgb[2] = (uint8_t)mdec_clip8(blue);
    if (mdec->signed_output == 0u) {
        rgb[0] ^= 0x80u;
        rgb[1] ^= 0x80u;
        rgb[2] ^= 0x80u;
    }
}

static uint32_t mdec_color_pixel_15(const struct schway__mdec *mdec, uint32_t block,
                                    uint32_t x, uint32_t y)
{
    uint32_t block_x = (block & 1u) * 8u;
    uint32_t block_y = (block >> 1u) * 8u;
    uint32_t chroma_index = ((block_y + y) / 2u) * 8u + (block_x + x) / 2u;
    int32_t luminance = mdec->block[y * 8u + x];
    int32_t cr = mdec->chroma_cr[chroma_index];
    int32_t cb = mdec->chroma_cb[chroma_index];
    int32_t red = mdec_clip8(luminance + mdec_floor_div((int64_t)cr * 91881, 65536));
    int32_t green = mdec_clip8(luminance - mdec_floor_div((int64_t)cb * 22524, 65536) -
                               mdec_floor_div((int64_t)cr * 46811, 65536));
    int32_t blue = mdec_clip8(luminance + mdec_floor_div((int64_t)cb * 116130, 65536));
    uint32_t pixel = mdec->bit15 << 15u;
    uint32_t channels[3] = {(uint8_t)red, (uint8_t)green, (uint8_t)blue};
    if (mdec->signed_output == 0u) {
        channels[0] ^= 0x80u;
        channels[1] ^= 0x80u;
        channels[2] ^= 0x80u;
    }
    for (uint32_t channel = 0u; channel < 3u; channel++) {
        pixel |= (channels[channel] >> 3u) << (channel * 5u);
    }
    return pixel;
}

static void mdec_output_byte(struct schway__mdec *mdec, uint32_t byte_offset, uint8_t value)
{
    uint32_t logical_byte = mdec->output_count * 4u + byte_offset;
    uint32_t index = (mdec->output_head + logical_byte / 4u) % SCHWAY__MDEC_OUTPUT_WORDS;
    uint32_t shift = (logical_byte & 3u) * 8u;
    uint32_t mask = UINT32_C(0xFF) << shift;
    mdec->output[index] = (mdec->output[index] & ~mask) | ((uint32_t)value << shift);
}

static void mdec_stage_color_block(struct schway__mdec *mdec)
{
    for (uint32_t y = 0u; y < 8u; y++) {
        for (uint32_t x = 0u; x < 8u; x++) {
            uint8_t rgb[3];
            mdec_color_pixel(mdec, mdec->block_index, x, y, rgb);
            uint32_t offset = (y * 8u + x) * 3u;
            mdec->color_stage[offset] = rgb[0];
            mdec->color_stage[offset + 1u] = rgb[1];
            mdec->color_stage[offset + 2u] = rgb[2];
        }
    }
}

static void mdec_publish_15bit_block(struct schway__mdec *mdec)
{
    uint32_t byte_offset = 0u;
    for (uint32_t y = 0u; y < 8u; y++) {
        for (uint32_t x = 0u; x < 8u; x++) {
            uint32_t pixel_word = mdec_color_pixel_15(mdec, mdec->block_index, x, y);
            uint16_t pixel = (uint16_t)pixel_word;
            mdec_output_byte(mdec, byte_offset++, (uint8_t)pixel);
            mdec_output_byte(mdec, byte_offset++, (uint8_t)(pixel >> 8u));
        }
    }
    mdec->output_count += 32u;
}

/* The 24-bit stream is packed as an 8-row, 16-pixel strip. Y1/Y2 and Y3/Y4
   are interleaved by row so DMA1 can feed a native GPU image upload directly
   (PSX-SPU-CDROM-AND-MDEC, section 4). */
static void mdec_publish_24bit_pair(struct schway__mdec *mdec)
{
    for (uint32_t y = 0u; y < 8u; y++) {
        for (uint32_t side = 0u; side < 2u; side++) {
            for (uint32_t x = 0u; x < 8u; x++) {
                uint8_t rgb[3];
                uint32_t pixel_offset = (y * 8u + x) * 3u;
                if (side == 0u) {
                    rgb[0] = mdec->color_stage[pixel_offset];
                    rgb[1] = mdec->color_stage[pixel_offset + 1u];
                    rgb[2] = mdec->color_stage[pixel_offset + 2u];
                } else {
                    mdec_color_pixel(mdec, mdec->block_index, x, y, rgb);
                }
                uint32_t row_pixel = y * 16u + side * 8u + x;
                for (uint32_t channel = 0u; channel < 3u; channel++) {
                    mdec_output_byte(mdec, row_pixel * 3u + channel, rgb[channel]);
                }
            }
        }
    }
    mdec->output_count += 96u;
}

static void mdec_publish_block(schway *inst)
{
    struct schway__mdec *mdec = &inst->mdec;
    if (mdec->depth >= 2u) {
        if (mdec->block_index == 4u) {
            memcpy(mdec->chroma_cr, mdec->block, sizeof mdec->chroma_cr);
        } else if (mdec->block_index == 5u) {
            memcpy(mdec->chroma_cb, mdec->block, sizeof mdec->chroma_cb);
        } else if (mdec->depth == 3u) {
            mdec_publish_15bit_block(mdec);
        } else if ((mdec->block_index & 1u) == 0u) {
            mdec_stage_color_block(mdec);
        } else {
            mdec_publish_24bit_pair(mdec);
        }
        return;
    }
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
                if (mdec->signed_output == 0u) {
                    value = (value + 8u) >> 4u;
                    if (value > 15u) {
                        value = 15u;
                    }
                } else {
                    value = (value >> 4u) & 0x0Fu;
                }
                word |= value << (4u * i);
            } else {
                word |= value << (8u * i);
            }
        }
        uint32_t tail = (mdec->output_head + mdec->output_count) % SCHWAY__MDEC_OUTPUT_WORDS;
        mdec->output[tail] = word;
        mdec->output_count++;
    }
}

static int mdec_block_fits(const struct schway__mdec *mdec)
{
    return mdec_output_words(mdec) <= SCHWAY__MDEC_OUTPUT_WORDS - mdec->output_count;
}

static void mdec_complete_block(schway *inst, uint64_t now)
{
    struct schway__mdec *mdec = &inst->mdec;
    mdec_publish_block(inst);
    mdec->block_pending = 0u;
    mdec->block_deadline = 0u;
    if (mdec->depth >= 2u) {
        if (mdec->block_index == 4u) {
            mdec->block_index = 5u;
        } else if (mdec->block_index == 5u) {
            mdec->block_index = 0u;
        } else if (mdec->block_index == 3u) {
            mdec->block_index = 4u;
        } else {
            mdec->block_index++;
        }
    }
    if (mdec->input_count != 0u) {
        mdec_try_decode(inst, now);
    } else if (mdec->words_remaining == 0u) {
        mdec->command_active = 0u;
        mdec->block_index = 4u;
    }
    mdec_update_requests(inst);
}

static void mdec_resume_waiting_block(schway *inst, uint64_t now)
{
    struct schway__mdec *mdec = &inst->mdec;
    if (mdec->block_pending != 0u && mdec->block_deadline == 0u && mdec_block_fits(mdec)) {
        mdec_complete_block(inst, now);
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
    mdec->input_half = 0u;
    mdec->command_words = 0u;
    mdec->words_remaining = 0u;
    mdec->block_index = 4u;
    mdec->command_active = 0u;
    mdec->parse_scan = 0u;
    mdec->parse_qscale = 0u;
    mdec->parse_have_dc = 0u;
    /* Command mirror fields follow the STAT behavior in docs/hardware/mdec.md. */
    mdec->depth = (command >> 27) & 3u;
    mdec->signed_output = (command >> 26) & 1u;
    mdec->bit15 = (command >> 25) & 1u;
    if (opcode == 1u) {
        mdec->command_words = command & 0xFFFFu;
        mdec->words_remaining = mdec->command_words;
        mdec->command_active = mdec->command_words != 0u && mdec->depth <= 3u ? 1u : 0u;
        mdec->block_index = mdec->command_active != 0u
                                ? (mdec->depth >= 2u ? 4u : 0u)
                                : 4u;
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
    uint32_t opcode = mdec->command >> 29;
    if (opcode == 0u || opcode >= 4u) {
        status |= mdec->command & 0xFFFFu;
    } else if (mdec->command_words != 0u) {
        /* The hardware exposes the parameter counter minus one, including
           0xFFFF after a nonempty command consumes its final word (SCM.30). */
        status |= (mdec->words_remaining - 1u) & 0xFFFFu;
    }
    return status;
}

uint64_t schway__mdec_next(const schway *inst)
{
    return inst->mdec.block_pending != 0u && inst->mdec.block_deadline != 0u
               ? inst->mdec.block_deadline
               : UINT64_MAX;
}

void schway__mdec_event(schway *inst, uint64_t now)
{
    struct schway__mdec *mdec = &inst->mdec;
    if (mdec->block_pending != 0u && mdec->block_deadline != 0u &&
        mdec->block_deadline <= now) {
        if (!mdec_block_fits(mdec)) {
            mdec->block_deadline = 0u;
            mdec_update_requests(inst);
            return;
        }
        mdec_complete_block(inst, now);
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
        mdec_resume_waiting_block(inst, now);
        if (mdec->output_count == 0u && mdec->block_pending == 0u) {
            mdec_try_decode(inst, now);
        }
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
