/* A PNG encoder for 8-bit RGB images with no compression library: the image
   data goes into stored (uncompressed) deflate blocks. The whole file is
   built in memory, so the runner writes it in one call. */
#ifndef SCHWAYSTATION_RUNNER_PNG_H
#define SCHWAYSTATION_RUNNER_PNG_H

#include <stddef.h>
#include <stdint.h>

/* Continues an Adler-32 (RFC 1950 section 8) over n bytes at p. Start with
   adler = 1. */
uint32_t schway__png_adler32(uint32_t adler, const uint8_t *p, size_t n);

/* The size in bytes of the PNG of a width x height image, or 0 when either
   side is 0 or the image is too large for one IDAT chunk. */
size_t schway__png_size(uint32_t width, uint32_t height);

/* Encodes a width x height image into out, which has room for cap bytes.
   rgb holds the rows top first, each width pixels of R, G, B bytes, with no
   padding. Returns the number of bytes written, or 0 when the size is
   refused by schway__png_size or cap is too small. */
size_t schway__png_write(uint8_t *out, size_t cap, const uint8_t *rgb, uint32_t width,
                            uint32_t height);

#endif /* SCHWAYSTATION_RUNNER_PNG_H */
