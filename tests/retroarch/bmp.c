/* Reads an uncompressed BMP into RGB24, top row first. Test code for the
   hosted RetroArch job, never linked into the library.

   Accepts what sips writes: a 40, 108 or 124-byte info header, 24 bits per
   pixel uncompressed, or 32 bits per pixel uncompressed or with the
   standard red, green and blue byte masks; rows bottom-up (positive height)
   or top-down (negative height), each padded to four bytes. Every field is
   checked against the file's size before it is read. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define BMP_FILE_LIMIT (16ul * 1024ul * 1024ul)
#define BMP_DIM_LIMIT 16384u

static uint32_t bmp_get(const uint8_t *b, size_t off, unsigned bytes)
{
    uint32_t v = 0;
    for (unsigned i = 0; i < bytes; i++)
        v |= (uint32_t)b[off + i] << (8u * i);
    return v;
}

static int bmp_parse(const uint8_t *b, size_t n, uint32_t *w, uint32_t *h, uint8_t **rgb)
{
    if (n < 70 || b[0] != 'B' || b[1] != 'M')
        return 2;
    uint32_t offset = bmp_get(b, 10, 4), header = bmp_get(b, 14, 4);
    uint32_t width = bmp_get(b, 18, 4), height = bmp_get(b, 22, 4);
    uint32_t planes = bmp_get(b, 26, 2), bpp = bmp_get(b, 28, 2), comp = bmp_get(b, 30, 4);
    if ((header != 40u && header != 108u && header != 124u) || planes != 1u ||
        offset < 14u + header || offset > n)
        return 2;
    int top_down = (height & 0x80000000u) != 0u;
    uint32_t rows = top_down ? 0u - height : height;
    if (width == 0u || width > BMP_DIM_LIMIT || rows == 0u || rows > BMP_DIM_LIMIT)
        return 2;
    int masks = bpp == 32u && comp == 3u;
    if (masks && (bmp_get(b, 54, 4) != 0x00FF0000u || bmp_get(b, 58, 4) != 0x0000FF00u ||
                  bmp_get(b, 62, 4) != 0x000000FFu))
        return 2;
    if (!masks && !((bpp == 24u || bpp == 32u) && comp == 0u))
        return 2;
    uint64_t stride = ((uint64_t)width * bpp + 31u) / 32u * 4u;
    if ((uint64_t)offset + stride * rows > (uint64_t)n)
        return 2;
    uint8_t *out = malloc((size_t)width * rows * 3u);
    if (out == NULL)
        return 2;
    for (uint32_t y = 0; y < rows; y++) {
        const uint8_t *src = b + offset + (size_t)stride * (top_down ? y : rows - 1u - y);
        uint8_t *dst = out + (size_t)y * width * 3u;
        for (uint32_t x = 0; x < width; x++) {
            /* Stored blue, green, red, then an unused byte at 32 bits. */
            const uint8_t *p = src + (size_t)x * (bpp / 8u);
            dst[x * 3u + 0u] = p[2];
            dst[x * 3u + 1u] = p[1];
            dst[x * 3u + 2u] = p[0];
        }
    }
    *w = width;
    *h = rows;
    *rgb = out;
    return 0;
}

/* Returns 0 with *rgb allocated (the caller frees it), or 2 when the file
   cannot be read or is not a BMP of the accepted kind. */
int bmp_read(const char *path, uint32_t *w, uint32_t *h, uint8_t **rgb)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return 2;
    int rc = 2;
    long end = -1;
    if (fseek(f, 0, SEEK_END) == 0)
        end = ftell(f);
    if (end > 0 && (unsigned long)end <= BMP_FILE_LIMIT && fseek(f, 0, SEEK_SET) == 0) {
        size_t n = (size_t)end;
        uint8_t *buf = malloc(n);
        if (buf != NULL && fread(buf, 1, n, f) == n)
            rc = bmp_parse(buf, n, w, h, rgb);
        free(buf);
    }
    fclose(f);
    return rc;
}
