/* The runner's PNG encoder, checked by a parse that shares no code with it
   (chunk CRCs with a bitwise CRC-32, stored blocks reassembled by hand and
   Adler-32 by its definition), and the shared 15-bit converter, checked by
   known answers. Cases:

     png             a 2 x 2 fixture and the checksums' published values
     png_file PATH   PATH, written by schwaystation-run --frames 5
                     --png 5:PATH, is the card's frame 5: 320 x 240 RGB,
                     every pixel the converter's output for the card's word
     xrgb8888        the converter's known answers */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"
#include "schwaystation.h"
#include "xrgb8888.h"
#include "../../runner/crc32.h"
#include "../../runner/png.h"
#include "../check.h"

/* CRC-32 one bit at a time (PNG specification, Annex D), sharing nothing
   with the encoder's table. */
static uint32_t bitwise_crc(const uint8_t *p, size_t n)
{
    uint32_t c = 0xffffffffu;
    for (size_t i = 0; i < n; i++) {
        c ^= p[i];
        for (int k = 0; k < 8; k++) {
            c = (c & 1u) != 0u ? 0xedb88320u ^ (c >> 1) : c >> 1;
        }
    }
    return ~c;
}

static uint32_t be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

/* Parses the PNG in d (n bytes) as a width x height RGB image and copies its
   decoded image data, one filter byte and 3 * width bytes per row, into raw.
   Returns 0 when every structural check passed. */
static int decode(const uint8_t *d, size_t n, uint32_t width, uint32_t height, uint8_t *raw)
{
    static const uint8_t signature[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    int before = check_failures;
    uint32_t raw_size = height * (1u + 3u * width);
    CHECK(n >= 8u && memcmp(d, signature, 8) == 0);
    if (n < 8u) {
        return -1;
    }

    /* Exactly IHDR, IDAT, IEND, each with a valid CRC. */
    static const char *const want[3] = {"IHDR", "IDAT", "IEND"};
    const uint8_t *chunk_data[3] = {NULL, NULL, NULL};
    uint32_t chunk_len[3] = {0, 0, 0};
    size_t at = 8;
    for (int c = 0; c < 3; c++) {
        CHECK(at + 12u <= n);
        if (at + 12u > n) {
            return -1;
        }
        uint32_t len = be32(d + at);
        CHECK(len <= n - at - 12u);
        if (len > n - at - 12u) {
            return -1;
        }
        CHECK(memcmp(d + at + 4, want[c], 4) == 0);
        CHECK_EQ_HEX(be32(d + at + 8 + len), bitwise_crc(d + at + 4, 4u + (size_t)len));
        chunk_data[c] = d + at + 8;
        chunk_len[c] = len;
        at += 12u + len;
    }
    CHECK_EQ_U64(at, n);

    /* IHDR: width, height, depth 8, colour type 2, compression, filter and
       interlace 0 (PNG section 11.2.1). */
    uint8_t ihdr[13] = {0, 0, 0, 0, 0, 0, 0, 0, 8, 2, 0, 0, 0};
    for (int i = 0; i < 4; i++) {
        ihdr[i] = (uint8_t)(width >> (24 - 8 * i));
        ihdr[4 + i] = (uint8_t)(height >> (24 - 8 * i));
    }
    CHECK_EQ_U64(chunk_len[0], 13u);
    CHECK(chunk_len[0] == 13u && memcmp(chunk_data[0], ihdr, 13) == 0);
    CHECK_EQ_U64(chunk_len[2], 0u);

    /* IDAT: zlib header 78 01, stored blocks, Adler-32. */
    const uint8_t *z = chunk_data[1];
    uint32_t zn = chunk_len[1];
    CHECK(zn >= 6u && z[0] == 0x78 && z[1] == 0x01);
    uint32_t pos = 2, got = 0, blocks = 0;
    int final = 0;
    while (!final && pos + 5u <= zn) {
        CHECK((z[pos] & 0xfeu) == 0u); /* BTYPE 00, no other bits */
        final = z[pos] & 1;
        uint32_t len = z[pos + 1] | ((uint32_t)z[pos + 2] << 8);
        uint32_t nlen = z[pos + 3] | ((uint32_t)z[pos + 4] << 8);
        CHECK_EQ_HEX(nlen, ~len & 0xffffu);
        pos += 5u;
        if (pos + len > zn || got + len > raw_size) {
            CHECK(0);
            return -1;
        }
        memcpy(raw + got, z + pos, len);
        pos += len;
        got += len;
        blocks++;
    }
    CHECK(final);
    CHECK_EQ_U64(got, raw_size);
    CHECK_EQ_U64(blocks, (raw_size + 65534u) / 65535u);
    CHECK_EQ_U64(pos + 4u, zn);
    if (pos + 4u != zn) {
        return -1;
    }
    uint32_t s1 = 1, s2 = 0;
    for (uint32_t i = 0; i < got; i++) {
        s1 = (s1 + raw[i]) % 65521u;
        s2 = (s2 + s1) % 65521u;
    }
    CHECK_EQ_HEX(be32(z + zn - 4u), (s2 << 16) | s1);

    /* Filter type 0 (None) on every row. */
    uint32_t bad = 0;
    for (uint32_t y = 0; y < height; y++) {
        bad += (uint32_t)(raw[y * (1u + 3u * width)] != 0u);
    }
    CHECK_EQ_U64(bad, 0u);
    return check_failures == before ? 0 : -1;
}

/* A 2 x 2 image round-trips through the encoder; sizes it cannot hold are
   refused. */
static void png(void)
{
    /* Published check values: CRC-32 of "123456789" (PNG specification,
       Annex D and the CRC catalogue), Adler-32 of "Wikipedia". */
    CHECK_EQ_HEX(schway__runner_crc32(0, (const uint8_t *)"123456789", 9), 0xCBF43926u);
    CHECK_EQ_HEX(bitwise_crc((const uint8_t *)"123456789", 9), 0xCBF43926u);
    CHECK_EQ_HEX(schway__png_adler32(1, (const uint8_t *)"Wikipedia", 9), 0x11E60398u);

    static const uint8_t rgb[12] = {0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
                                    0x00, 0x00, 0xff, 0x11, 0x22, 0x33};
    /* Signature 8, IHDR 25, IDAT 12 + 2 + 5 + 14 + 4, IEND 12. */
    CHECK_EQ_U64(schway__png_size(2, 2), 82u);
    CHECK_EQ_U64(schway__png_size(0, 2), 0u);
    CHECK_EQ_U64(schway__png_size(2, 0), 0u);
    CHECK_EQ_U64(schway__png_size(65536u, 65536u), 0u);

    uint8_t out[83];
    memset(out, 0xA5, sizeof out);
    CHECK_EQ_U64(schway__png_write(out, 81, rgb, 2, 2), 0u);
    CHECK_EQ_U64(schway__png_write(out, sizeof out, rgb, 2, 2), 82u);
    CHECK_EQ_HEX(out[82], 0xA5u);

    uint8_t raw[14];
    memset(raw, 0, sizeof raw);
    if (decode(out, 82, 2, 2, raw) == 0) {
        CHECK(memcmp(raw + 1, rgb, 6) == 0);
        CHECK(memcmp(raw + 8, rgb + 6, 6) == 0);
    }
}

/* Reads the file at path into a heap buffer; *n is its size. */
static uint8_t *read_file(const char *path, size_t limit, size_t *n)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return NULL;
    }
    uint8_t *buf = (uint8_t *)malloc(limit + 1u);
    *n = buf == NULL ? 0u : fread(buf, 1, limit + 1u, f);
    fclose(f);
    return buf;
}

/* The runner's PNG of frame 5 is the card's frame 5 through the converter,
   whose rule the xrgb8888 case pins on its own. */
static void png_file(const char *path)
{
    enum { W = 320, H = 240, FRAME = 5 };
    size_t limit = schway__png_size(W, H);
    size_t n = 0;
    uint8_t *d = read_file(path, limit, &n);
    uint8_t *raw = (uint8_t *)malloc((size_t)H * (1u + 3u * W));
    CHECK(d != NULL && raw != NULL);
    if (d != NULL && raw != NULL && decode(d, n, W, H, raw) == 0) {
        uint32_t bad = 0;
        for (uint32_t y = 0; y < H; y++) {
            const uint8_t *row = raw + y * (1u + 3u * W) + 1u;
            for (uint32_t x = 0; x < W; x++) {
                uint32_t got = ((uint32_t)row[3u * x] << 16) | ((uint32_t)row[3u * x + 1u] << 8) |
                               row[3u * x + 2u];
                uint32_t want = schway_host_xrgb8888_pixel(schway__testcard_pixel(x, y, FRAME));
                bad += (uint32_t)(got != want);
            }
        }
        CHECK_EQ_U64(bad, 0u);
    }
    free(d);
    free(raw);
}

/* The pixel word is 0BBBBBGGGGGRRRRR with bit 15 the mask bit, not colour
   (ARCHITECTURE section 4; psx-spx, GPU Video Memory and colour format), and
   the host output is XRGB8888 with X zero. Each 5-bit channel widens to 8
   bits by copying its top 3 bits below it, so 0, 1, 15, 16 and 31 give 0, 8,
   123, 132 and 255. */
static void xrgb8888(void)
{
    static const uint32_t five[5] = {0, 1, 15, 16, 31};
    static const uint32_t eight[5] = {0, 8, 123, 132, 255};
    for (int i = 0; i < 5; i++) {
        CHECK_EQ_HEX(schway_host_xrgb8888_pixel((uint16_t)five[i]), eight[i] << 16);
        CHECK_EQ_HEX(schway_host_xrgb8888_pixel((uint16_t)(five[i] << 5)), eight[i] << 8);
        CHECK_EQ_HEX(schway_host_xrgb8888_pixel((uint16_t)(five[i] << 10)), eight[i]);
    }
    CHECK_EQ_HEX(schway_host_xrgb8888_pixel(0x7FFFu), 0x00FFFFFFu);
    CHECK_EQ_HEX(schway_host_xrgb8888_pixel(0x8000u), 0x00000000u);
    CHECK_EQ_HEX(schway_host_xrgb8888_pixel(0xFFFFu), 0x00FFFFFFu);

    /* The card's mask-twin band (row 150) is the grey band (row 120) with
       bit 15 set: the words differ, the converted colours do not. */
    uint32_t words_differ = 0, colours_differ = 0;
    for (uint32_t x = 9; x < 319; x++) {
        uint16_t grey = schway__testcard_pixel(x, 120, 0);
        uint16_t twin = schway__testcard_pixel(x, 150, 0);
        words_differ += (uint32_t)(grey != twin);
        colours_differ +=
            (uint32_t)(schway_host_xrgb8888_pixel(grey) != schway_host_xrgb8888_pixel(twin));
    }
    CHECK_EQ_U64(words_differ, 310u);
    CHECK_EQ_U64(colours_differ, 0u);

    /* The frame converter reads rows pitch_bytes apart and writes rows
       out_pitch_pixels apart, leaving the padding alone. */
    uint16_t src[2][4] = {{0x001Fu, 0x03E0u, 0xBEEFu, 0xBEEFu}, {0x7C00u, 0x8000u, 0xBEEFu, 0xBEEFu}};
    uint32_t dst[2][3] = {{1, 1, 1}, {1, 1, 1}};
    schway_video v;
    memset(&v, 0, sizeof v);
    v.size = (uint32_t)sizeof v;
    v.width = 2;
    v.height = 2;
    v.pitch_bytes = (uint32_t)sizeof src[0];
    v.bpp = 16;
    v.pixels = &src[0][0];
    schway_host_xrgb8888_frame(&v, &dst[0][0], 3);
    CHECK_EQ_HEX(dst[0][0], 0x00FF0000u);
    CHECK_EQ_HEX(dst[0][1], 0x0000FF00u);
    CHECK_EQ_HEX(dst[0][2], 1u);
    CHECK_EQ_HEX(dst[1][0], 0x000000FFu);
    CHECK_EQ_HEX(dst[1][1], 0x00000000u);
    CHECK_EQ_HEX(dst[1][2], 1u);
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "png") == 0) {
        png();
    } else if (argc == 3 && strcmp(argv[1], "png_file") == 0) {
        png_file(argv[2]);
    } else if (argc == 2 && strcmp(argv[1], "xrgb8888") == 0) {
        xrgb8888();
    } else {
        fprintf(stderr, "usage: schwaystation-test-runner png | png_file PATH | xrgb8888\n");
        return 2;
    }
    CHECK_DONE();
}
