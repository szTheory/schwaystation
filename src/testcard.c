/* The built-in test card: a pure integer function of (x, y, frame) with no
   table, font or state, redrawn whole into VRAM every frame. Colour words
   are 15-bit 0BBBBBGGGGGRRRRR with bit 15 the mask bit (psx-spx, GPU Video
   Memory). docs/ explains what each band reveals. First matching rule wins. */
#include "internal.h"

#define CARD_W 320u
#define CARD_H 240u

#define WHITE 0x7FFFu
#define BLACK 0x0000u
#define RED 0x001Fu
#define GREEN 0x03E0u
#define MID_GREY 0x4210u
#define MASK_BIT 0x8000u

uint16_t schway__testcard_pixel(uint32_t x, uint32_t y, uint32_t frame)
{
    /* A white block in the top-left corner only: a mirror, flip or transpose
       moves it. */
    if (x < 8u && y < 8u) {
        return WHITE;
    }
    /* A one-dot white frame on the edges: a crop, offset or wrong size
       shows. */
    if (x == 0u || x == CARD_W - 1u || y == 0u || y == CARD_H - 1u) {
        return WHITE;
    }
    /* The row number in a strip on the left: a wrong pitch or a skipped or
       repeated row shows. */
    if (x >= 1u && x <= 8u) {
        return (uint16_t)((y & 31u) | ((y >> 5) << 5));
    }
    /* Ten dots per level, all 32 levels across the card. */
    uint32_t level = x / 10u;
    uint32_t grey = level | (level << 5) | (level << 10);
    if (y <= 16u) {
        return RED;
    }
    if (y <= 48u) {
        return (uint16_t)level;
    }
    if (y <= 80u) {
        return (uint16_t)(level << 5);
    }
    if (y <= 112u) {
        return (uint16_t)(level << 10);
    }
    if (y <= 144u) {
        return (uint16_t)grey;
    }
    /* The same grey with the mask bit: converts equal, hashes different. */
    if (y <= 176u) {
        return (uint16_t)(grey | MASK_BIT);
    }
    /* A one-dot checker: scaling or filtering blurs it. */
    if (y <= 192u) {
        return ((x ^ y) & 1u) != 0u ? WHITE : BLACK;
    }
    /* The frame counter, most significant bit on the left, eight dots per
       bit. */
    if (y <= 208u) {
        if (x >= 32u && x < 288u) {
            uint32_t bit = 31u - (x - 32u) / 8u;
            return ((frame >> bit) & 1u) != 0u ? WHITE : BLACK;
        }
        return MID_GREY;
    }
    return GREEN;
}

void schway__testcard_draw(uint16_t *vram, uint32_t frame)
{
    for (uint32_t y = 0; y < CARD_H; y++) {
        uint16_t *row = vram + (size_t)y * SCHWAY__VRAM_W;
        for (uint32_t x = 0; x < CARD_W; x++) {
            row[x] = schway__testcard_pixel(x, y, frame);
        }
    }
}
