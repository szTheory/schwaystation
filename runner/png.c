/* PNG encoder, from the PNG specification (ISO/IEC 15948, W3C PNG Third
   Edition) and RFC 1950/1951. The file is the signature, IHDR, one IDAT and
   IEND, with no colour, gamma or aspect chunks. IDAT holds a zlib stream
   whose deflate data is stored blocks of at most 65,535 bytes (RFC 1951
   section 3.2.4), so no compressor is needed. Every multi-byte field is
   written byte by byte, so the file does not depend on the host's byte
   order. */
#include "png.h"

#include "crc32.h"

/* RFC 1951 section 3.2.4: LEN is 16 bits. */
#define STORED_MAX 65535u
/* RFC 1950 section 8: the largest prime below 65536. */
#define ADLER_BASE 65521u
/* PNG section 5.3: a chunk's length is at most 2^31 - 1. */
#define CHUNK_MAX 0x7fffffffu

uint32_t schway__png_adler32(uint32_t adler, const uint8_t *p, size_t n)
{
    uint32_t s1 = adler & 0xffffu;
    uint32_t s2 = adler >> 16;
    for (size_t i = 0; i < n; i++) {
        s1 = (s1 + p[i]) % ADLER_BASE;
        s2 = (s2 + s1) % ADLER_BASE;
    }
    return (s2 << 16) | s1;
}

/* Raw image data: one filter byte plus 3 bytes per pixel per row. */
static uint64_t raw_size(uint32_t width, uint32_t height)
{
    return (uint64_t)height * (1u + 3u * (uint64_t)width);
}

/* IDAT data: the 2-byte zlib header, 5 bytes per stored block, the raw data
   and the 4-byte Adler-32. */
static uint64_t idat_size(uint32_t width, uint32_t height)
{
    uint64_t raw = raw_size(width, height);
    return 2u + 5u * ((raw + STORED_MAX - 1u) / STORED_MAX) + raw + 4u;
}

size_t schway__png_size(uint32_t width, uint32_t height)
{
    if (width == 0u || height == 0u || width > CHUNK_MAX || height > CHUNK_MAX ||
        idat_size(width, height) > CHUNK_MAX) {
        return 0;
    }
    /* Signature, then IHDR, IDAT and IEND, each with 12 bytes of length,
       type and CRC. With IDAT at most 2^31 - 1 bytes the total fits in 32
       bits, so in any size_t. */
    return (size_t)(8u + (12u + 13u) + (12u + idat_size(width, height)) + 12u);
}

static void put32(uint8_t *b, uint32_t v)
{
    b[0] = (uint8_t)(v >> 24);
    b[1] = (uint8_t)(v >> 16);
    b[2] = (uint8_t)(v >> 8);
    b[3] = (uint8_t)v;
}

/* Appends n bytes at out + *at. The caller has checked the total size. */
static void emit(uint8_t *out, size_t *at, const uint8_t *p, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        out[*at + i] = p[i];
    }
    *at += n;
}

/* Appends a whole chunk whose data is n bytes at data. */
static void chunk(uint8_t *out, size_t *at, const char type[4], const uint8_t *data, uint32_t n)
{
    size_t start = *at;
    uint8_t b[8];
    put32(b, n);
    for (int i = 0; i < 4; i++) {
        b[4 + i] = (uint8_t)type[i];
    }
    emit(out, at, b, 8);
    emit(out, at, data, n);
    put32(b, schway__runner_crc32(0, out + start + 4u, 4u + (size_t)n));
    emit(out, at, b, 4);
}

size_t schway__png_write(uint8_t *out, size_t cap, const uint8_t *rgb, uint32_t width,
                            uint32_t height)
{
    size_t total = schway__png_size(width, height);
    if (total == 0u || cap < total) {
        return 0;
    }
    size_t at = 0;
    static const uint8_t signature[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    emit(out, &at, signature, 8);

    /* IHDR: width, height, bit depth 8, colour type 2 (RGB), compression 0,
       filter method 0, interlace 0 (PNG section 11.2.1). */
    uint8_t ihdr[13];
    put32(ihdr, width);
    put32(ihdr + 4, height);
    ihdr[8] = 8;
    ihdr[9] = 2;
    ihdr[10] = 0;
    ihdr[11] = 0;
    ihdr[12] = 0;
    chunk(out, &at, "IHDR", ihdr, 13);

    /* IDAT: length and type, then the zlib header: CM 8 with a 256-byte
       window (0x78) and FLEVEL 0 with FCHECK making 0x7801 a multiple of 31
       (RFC 1950 section 2.2). */
    size_t idat_start = at;
    uint8_t b[8];
    put32(b, (uint32_t)idat_size(width, height));
    b[4] = 'I';
    b[5] = 'D';
    b[6] = 'A';
    b[7] = 'T';
    emit(out, &at, b, 8);
    static const uint8_t zlib_header[2] = {0x78, 0x01};
    emit(out, &at, zlib_header, 2);

    /* Stored blocks. A block header is BFINAL (1 on the last block) with
       BTYPE 00, then LEN and its complement NLEN, little-endian. Filter
       type 0 (None) stores each row's bytes as they are. */
    size_t row_bytes = 3u * (size_t)width;
    uint32_t raw_left = (uint32_t)raw_size(width, height);
    uint32_t block_left = 0;
    uint32_t adler = 1;
    for (uint32_t y = 0; y < height; y++) {
        static const uint8_t filter_none = 0;
        const uint8_t *row = rgb + (size_t)y * row_bytes;
        for (size_t i = 0; i <= row_bytes; i++) {
            if (block_left == 0u) {
                uint32_t len = raw_left < STORED_MAX ? raw_left : STORED_MAX;
                uint32_t nlen = ~len & 0xffffu;
                uint8_t h[5] = {(uint8_t)(len == raw_left ? 1u : 0u), (uint8_t)len,
                                (uint8_t)(len >> 8), (uint8_t)nlen, (uint8_t)(nlen >> 8)};
                emit(out, &at, h, 5);
                block_left = len;
            }
            const uint8_t *p = i == 0u ? &filter_none : row + i - 1u;
            adler = schway__png_adler32(adler, p, 1);
            out[at++] = *p;
            block_left--;
            raw_left--;
        }
    }
    put32(b, adler);
    emit(out, &at, b, 4);
    put32(b, schway__runner_crc32(0, out + idat_start + 4u, at - idat_start - 4u));
    emit(out, &at, b, 4);

    chunk(out, &at, "IEND", NULL, 0);
    return at;
}
