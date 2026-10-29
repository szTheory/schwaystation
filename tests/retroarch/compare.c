/* compare EXPECTED.bmp ACTUAL.bmp

   Exits 0 when both BMPs have the same size and every pixel's red, green
   and blue are equal, printing the size. Exits 1 on a size mismatch or a
   differing pixel, naming the first one in row order with both values.
   Exits 2 on wrong arguments or a file bmp.c cannot read. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int bmp_read(const char *path, uint32_t *w, uint32_t *h, uint8_t **rgb);

static unsigned compare_hex(const uint8_t *p)
{
    return (unsigned)p[0] << 16 | (unsigned)p[1] << 8 | (unsigned)p[2];
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: compare EXPECTED.bmp ACTUAL.bmp\n");
        return 2;
    }
    uint32_t w[2], h[2];
    uint8_t *rgb[2] = {NULL, NULL};
    int rc = 0;
    for (int i = 0; i < 2 && rc == 0; i++) {
        if (bmp_read(argv[i + 1], &w[i], &h[i], &rgb[i]) != 0) {
            fprintf(stderr, "compare: cannot read %s as a BMP\n", argv[i + 1]);
            rc = 2;
        }
    }
    if (rc == 0 && (w[0] != w[1] || h[0] != h[1])) {
        printf("size differs: expected %ux%u, actual %ux%u\n", (unsigned)w[0], (unsigned)h[0],
               (unsigned)w[1], (unsigned)h[1]);
        rc = 1;
    }
    for (uint32_t y = 0; rc == 0 && y < h[0]; y++) {
        for (uint32_t x = 0; rc == 0 && x < w[0]; x++) {
            const uint8_t *a = rgb[0] + ((size_t)y * w[0] + x) * 3u;
            const uint8_t *b = rgb[1] + ((size_t)y * w[0] + x) * 3u;
            if (a[0] != b[0] || a[1] != b[1] || a[2] != b[2]) {
                printf("first difference at %u,%u: expected %06X, actual %06X\n", (unsigned)x,
                       (unsigned)y, compare_hex(a), compare_hex(b));
                rc = 1;
            }
        }
    }
    if (rc == 0)
        printf("%ux%u, every pixel equal\n", (unsigned)w[0], (unsigned)h[0]);
    free(rgb[0]);
    free(rgb[1]);
    return rc;
}
