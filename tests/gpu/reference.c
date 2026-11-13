/* Synthetic RGB555 controls and a whole-VRAM comparison for pinned captures.
   Python's bounded host adapter canonicalizes the capture; this file loads the
   pinned program through the public API and compares every VRAM word. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "png.h"
#include "test_open.h"

enum {
    VRAM_WIDTH = 1024u,
    VRAM_HEIGHT = 512u,
    VRAM_WORDS = VRAM_WIDTH * VRAM_HEIGHT,
    RGB_BYTES = 3u,
    MAX_PNG_BYTES = 2u * 1024u * 1024u,
    MAX_EXE_BYTES = 2u * 1024u * 1024u + 0x800u,
    CAPTURE_RUN_ADVANCES = 1u
};

enum {
    REFERENCE_INVALID = -1,
    REFERENCE_MISMATCH = 0,
    REFERENCE_MATCH = 1
};

typedef struct reference_fixture {
    uint16_t *words;
    uint8_t *rgb;
    uint8_t *png;
    size_t png_size;
} reference_fixture;

static unsigned failures;

#define CHECK(expr)                                                                            \
    do {                                                                                       \
        if (!(expr)) {                                                                         \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr);                       \
            failures++;                                                                        \
        }                                                                                      \
    } while (0)

static uint32_t read_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

/* Bitwise CRC-32 is independent of the table used by the PNG writer. */
static uint32_t reference_crc32(const uint8_t *p, size_t n)
{
    uint32_t crc = 0xffffffffu;
    for (size_t i = 0u; i < n; i++) {
        crc ^= p[i];
        for (uint32_t bit = 0u; bit < 8u; bit++) {
            crc = (crc & 1u) != 0u ? 0xedb88320u ^ (crc >> 1) : crc >> 1;
        }
    }
    return ~crc;
}

static uint32_t reference_adler32(const uint8_t *p, size_t n)
{
    uint32_t a = 1u;
    uint32_t b = 0u;
    for (size_t i = 0u; i < n; i++) {
        a = (a + p[i]) % 65521u;
        b = (b + a) % 65521u;
    }
    return (b << 16) | a;
}

static int chunk_crc_matches(const uint8_t *chunk, uint32_t length)
{
    return read_be32(chunk + 8u + length) == reference_crc32(chunk + 4u, 4u + length);
}

/* Reads the canonical 1024x512 RGB8/filter-0/stored-DEFLATE subset generated
   by runner/png.c and compares every pixel after RGB555 quantization. */
static int compare_reference_png(const uint16_t *words, size_t word_count,
                                 const uint8_t *png, size_t png_size)
{
    static const uint8_t signature[8] = {0x89u, 'P', 'N', 'G', '\r', '\n', 0x1au, '\n'};
    enum { RAW_ROW_BYTES = 1u + VRAM_WIDTH * RGB_BYTES, RAW_BYTES = VRAM_HEIGHT * RAW_ROW_BYTES };
    if (words == NULL || word_count != VRAM_WORDS || png == NULL || png_size < 57u ||
        png_size > MAX_PNG_BYTES || memcmp(png, signature, sizeof signature) != 0) {
        return REFERENCE_INVALID;
    }

    size_t at = sizeof signature;
    if (read_be32(png + at) != 13u || memcmp(png + at + 4u, "IHDR", 4u) != 0 ||
        !chunk_crc_matches(png + at, 13u)) {
        return REFERENCE_INVALID;
    }
    const uint8_t *ihdr = png + at + 8u;
    if (read_be32(ihdr) != VRAM_WIDTH || read_be32(ihdr + 4u) != VRAM_HEIGHT || ihdr[8] != 8u ||
        ihdr[9] != 2u || ihdr[10] != 0u || ihdr[11] != 0u || ihdr[12] != 0u) {
        return REFERENCE_INVALID;
    }
    at += 25u;
    if (at + 12u > png_size || memcmp(png + at + 4u, "IDAT", 4u) != 0) {
        return REFERENCE_INVALID;
    }
    uint32_t idat_size = read_be32(png + at);
    if ((size_t)idat_size > png_size - at - 12u || !chunk_crc_matches(png + at, idat_size)) {
        return REFERENCE_INVALID;
    }
    const uint8_t *zlib = png + at + 8u;
    at += 12u + (size_t)idat_size;
    if (at + 12u != png_size || read_be32(png + at) != 0u ||
        memcmp(png + at + 4u, "IEND", 4u) != 0 || !chunk_crc_matches(png + at, 0u)) {
        return REFERENCE_INVALID;
    }

    if (idat_size < 11u || zlib[0] != 0x78u || zlib[1] != 0x01u) {
        return REFERENCE_INVALID;
    }
    uint8_t *raw = (uint8_t *)malloc(RAW_BYTES);
    if (raw == NULL) {
        return REFERENCE_INVALID;
    }
    size_t pos = 2u;
    size_t produced = 0u;
    size_t deflate_end = (size_t)idat_size - 4u;
    int final = 0;
    while (!final) {
        if (pos + 5u > deflate_end || (zlib[pos] & 0xfeu) != 0u) {
            free(raw);
            return REFERENCE_INVALID;
        }
        final = zlib[pos] & 1u;
        uint32_t block_size = (uint32_t)zlib[pos + 1u] | ((uint32_t)zlib[pos + 2u] << 8);
        uint32_t complement = (uint32_t)zlib[pos + 3u] | ((uint32_t)zlib[pos + 4u] << 8);
        pos += 5u;
        if (complement != (~block_size & 0xffffu) || block_size > RAW_BYTES - produced ||
            (size_t)block_size > deflate_end - pos) {
            free(raw);
            return REFERENCE_INVALID;
        }
        memcpy(raw + produced, zlib + pos, block_size);
        produced += block_size;
        pos += block_size;
    }
    if (produced != RAW_BYTES || pos != deflate_end ||
        read_be32(zlib + idat_size - 4u) != reference_adler32(raw, RAW_BYTES)) {
        free(raw);
        return REFERENCE_INVALID;
    }

    uint32_t mismatches = 0u;
    for (uint32_t y = 0u; y < VRAM_HEIGHT; y++) {
        const uint8_t *row = raw + (size_t)y * RAW_ROW_BYTES;
        if (row[0] != 0u) {
            free(raw);
            return REFERENCE_INVALID;
        }
        for (uint32_t x = 0u; x < VRAM_WIDTH; x++) {
            uint16_t word = (uint16_t)(words[(size_t)y * VRAM_WIDTH + x] & 0x7fffu);
            const uint8_t *pixel = row + 1u + (size_t)x * RGB_BYTES;
            mismatches += (uint32_t)((pixel[0] >> 3) != (word & 31u));
            mismatches += (uint32_t)((pixel[1] >> 3) != ((word >> 5) & 31u));
            mismatches += (uint32_t)((pixel[2] >> 3) != ((word >> 10) & 31u));
        }
    }
    free(raw);
    return mismatches == 0u ? REFERENCE_MATCH : REFERENCE_MISMATCH;
}

/* Independent color-known-answer pattern. It covers every RGB555 level and
   deliberately mixes the non-color mask bit into the word domain. */
static uint16_t known_word(uint32_t x, uint32_t y)
{
    if (y == 0u && x == 0u) return 0x001fu;
    if (y == 0u && x == 1u) return 0x03e0u;
    if (y == 0u && x == 2u) return 0x7c00u;
    uint32_t red = (x + y) & 31u;
    uint32_t green = (3u * x + 5u * y) & 31u;
    uint32_t blue = (7u * x + 11u * y) & 31u;
    uint16_t mask = ((x ^ y) & 1u) != 0u ? 0x8000u : 0u;
    return (uint16_t)(mask | red | (green << 5) | (blue << 10));
}

static uint8_t expand5(uint32_t value)
{
    return (uint8_t)((value << 3) | (value >> 2));
}

static int encode_fixture(reference_fixture *fixture, uint32_t width, uint32_t height);

static int make_fixture(reference_fixture *fixture, uint32_t width, uint32_t height)
{
    memset(fixture, 0, sizeof *fixture);
    if (width == 0u || height == 0u || (size_t)width > SIZE_MAX / RGB_BYTES / height) {
        return 0;
    }
    size_t pixels = (size_t)width * height;
    fixture->words = (uint16_t *)calloc(VRAM_WORDS, sizeof *fixture->words);
    fixture->rgb = (uint8_t *)malloc(pixels * RGB_BYTES);
    fixture->png_size = schway__png_size(width, height);
    fixture->png = fixture->png_size == 0u ? NULL : (uint8_t *)malloc(fixture->png_size);
    if (fixture->words == NULL || fixture->rgb == NULL || fixture->png == NULL) {
        return 0;
    }
    uint32_t rows = height < VRAM_HEIGHT ? height : VRAM_HEIGHT;
    uint32_t columns = width < VRAM_WIDTH ? width : VRAM_WIDTH;
    for (uint32_t y = 0u; y < rows; y++) {
        for (uint32_t x = 0u; x < columns; x++) {
            fixture->words[(size_t)y * VRAM_WIDTH + x] = known_word(x, y);
        }
    }
    for (uint32_t y = 0u; y < height; y++) {
        for (uint32_t x = 0u; x < width; x++) {
            uint16_t word = x < VRAM_WIDTH && y < VRAM_HEIGHT ?
                                fixture->words[(size_t)y * VRAM_WIDTH + x] : 0u;
            size_t at = ((size_t)y * width + x) * RGB_BYTES;
            fixture->rgb[at] = expand5(word & 31u);
            fixture->rgb[at + 1u] = expand5((word >> 5) & 31u);
            fixture->rgb[at + 2u] = expand5((word >> 10) & 31u);
        }
    }
    return encode_fixture(fixture, width, height);
}

static int encode_fixture(reference_fixture *fixture, uint32_t width, uint32_t height)
{
    size_t written = schway__png_write(fixture->png, fixture->png_size, fixture->rgb, width, height);
    return written == fixture->png_size;
}

static void free_fixture(reference_fixture *fixture)
{
    free(fixture->words);
    free(fixture->rgb);
    free(fixture->png);
    memset(fixture, 0, sizeof *fixture);
}

static void *reference_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void reference_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static uint8_t *read_bounded_file(const char *path, size_t limit, size_t *size_out)
{
    FILE *file = schway__test_open_file(path, "rb");
    if (file == NULL) {
        return NULL;
    }

    if (fseek(file, 0L, SEEK_END) != 0) {
        (void)fclose(file);
        return NULL;
    }
    long file_size = ftell(file);
    if (file_size < 0 || (uint64_t)file_size > (uint64_t)limit ||
        fseek(file, 0L, SEEK_SET) != 0) {
        (void)fclose(file);
        return NULL;
    }

    size_t size = (size_t)file_size;
    uint8_t *bytes = (uint8_t *)malloc(size == 0u ? 1u : size);
    if (bytes == NULL) {
        (void)fclose(file);
        return NULL;
    }
    size_t count = fread(bytes, 1u, size, file);
    int extra = fgetc(file);
    int read_error = ferror(file);
    int close_error = fclose(file);
    if (count != size || extra != EOF || read_error != 0 || close_error != 0) {
        free(bytes);
        return NULL;
    }
    *size_out = size;
    return bytes;
}

static uint16_t read_le16(const uint8_t *bytes)
{
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}

static int compare_capture(const char *exe_path, const char *golden_path)
{
    size_t exe_size = 0u;
    size_t golden_size = 0u;
    uint8_t *exe = read_bounded_file(exe_path, MAX_EXE_BYTES, &exe_size);
    uint8_t *golden = read_bounded_file(golden_path, VRAM_WORDS * 2u, &golden_size);
    schway *inst = NULL;
    int exit_code = 2;

    if (exe == NULL || exe_size == 0u) {
        fprintf(stderr, "GPU_REFERENCE_INPUT_ERROR: cannot read bounded clipping executable\n");
        goto cleanup;
    }
    if (golden == NULL || golden_size != VRAM_WORDS * 2u) {
        fprintf(stderr, "GPU_REFERENCE_INPUT_ERROR: canonical VRAM file has the wrong length\n");
        goto cleanup;
    }

    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = reference_alloc;
    config.allocator.free = reference_free;
    schway_status status = schway_create(&config, &inst);
    if (status != SCHWAY_STATUS_OK || inst == NULL) {
        fprintf(stderr, "GPU_REFERENCE_CORE_ERROR: schway_create returned %s\n",
                schway_status_string(status));
        goto cleanup;
    }

    status = schway_load_exe(inst, exe, exe_size);
    if (status != SCHWAY_STATUS_OK) {
        fprintf(stderr, "GPU_REFERENCE_INPUT_ERROR: clipping executable was refused: %s\n",
                schway_status_string(status));
        goto cleanup;
    }

    uint64_t previous_frame = 0u;
    uint64_t previous_ticks = 0u;
    uint32_t last_stop = 0u;
    for (uint32_t advance = 0u; advance < CAPTURE_RUN_ADVANCES; advance++) {
        schway_run_result result;
        memset(&result, 0, sizeof result);
        result.size = (uint32_t)sizeof result;
        status = schway_run_frame(inst, &result);
        /* This program resets the GPU during video setup, then busy-waits in
           BIOS VSync(0). The documented independent field bound may therefore
           return BUDGET before a wrap; only FRAME and that bounded stop are
           valid, and each call must make progress without a guest fault. */
        if (status != SCHWAY_STATUS_OK ||
            (result.stop != SCHWAY_STOP_FRAME && result.stop != SCHWAY_STOP_BUDGET) ||
            result.frame < previous_frame ||
            (result.stop == SCHWAY_STOP_FRAME && result.frame == previous_frame) ||
            result.ticks <= previous_ticks || result.fault_present != 0u) {
            fprintf(stderr,
                    "GPU_REFERENCE_CORE_ERROR: bounded run %u stopped unexpectedly "
                    "(stop=%u frame=%llu ticks=%llu fault=%u)\n",
                    advance + 1u, result.stop, (unsigned long long)result.frame,
                    (unsigned long long)result.ticks, result.fault_present);
            goto cleanup;
        }
        previous_frame = result.frame;
        previous_ticks = result.ticks;
        last_stop = result.stop;
    }

    schway_span span;
    memset(&span, 0, sizeof span);
    status = schway_get_vram(inst, &span);
    if (status != SCHWAY_STATUS_OK || span.data == NULL || span.size != VRAM_WORDS * 2u) {
        fprintf(stderr, "GPU_REFERENCE_CORE_ERROR: complete VRAM span is unavailable\n");
        goto cleanup;
    }

    const uint16_t *vram = (const uint16_t *)span.data;
    uint32_t mismatches = 0u;
    size_t first_mismatch = VRAM_WORDS;
    uint16_t first_expected = 0u;
    uint16_t first_actual = 0u;
    for (size_t index = 0u; index < VRAM_WORDS; index++) {
        uint16_t expected = read_le16(golden + index * 2u);
        uint16_t actual = (uint16_t)(vram[index] & 0x7fffu);
        if (actual != expected) {
            if (mismatches == 0u) {
                first_mismatch = index;
                first_expected = expected;
                first_actual = actual;
            }
            mismatches++;
        }
    }

    if (mismatches != 0u) {
        fprintf(stderr,
                "GPU_REFERENCE_RASTER_MISMATCH: %u of %u words; first=%zu expected=%04x actual=%04x\n",
                mismatches, VRAM_WORDS, first_mismatch, first_expected, first_actual);
        exit_code = 1;
        goto cleanup;
    }

    printf("GPU_REFERENCE_CAPTURE_MATCH: words=%u advances=%u stop=%s ticks=%llu tolerance=0\n",
           VRAM_WORDS, CAPTURE_RUN_ADVANCES,
           last_stop == SCHWAY_STOP_FRAME ? "frame" : "budget",
           (unsigned long long)previous_ticks);
    exit_code = 0;

cleanup:
    schway_destroy(inst);
    free(exe);
    free(golden);
    return exit_code;
}

static void quantization(void)
{
    reference_fixture fixture;
    int made = make_fixture(&fixture, VRAM_WIDTH, VRAM_HEIGHT);
    CHECK(made);
    if (!made) {
        free_fixture(&fixture);
        return;
    }
    CHECK(compare_reference_png(fixture.words, VRAM_WORDS, fixture.png, fixture.png_size) ==
          REFERENCE_MATCH);
    free_fixture(&fixture);
}

static void controls(void)
{
    quantization();
    reference_fixture fixture;
    int made = make_fixture(&fixture, VRAM_WIDTH, VRAM_HEIGHT);
    CHECK(made);
    if (!made) {
        free_fixture(&fixture);
        return;
    }

    /* Fixed primary colors pin channel order and the 15-bit word layout. */
    CHECK(fixture.words[0] == 0x001fu && fixture.rgb[0] == 255u && fixture.rgb[1] == 0u &&
          fixture.rgb[2] == 0u);
    CHECK(fixture.words[1] == 0x03e0u && fixture.rgb[3] == 0u && fixture.rgb[4] == 255u &&
          fixture.rgb[5] == 0u);
    CHECK(fixture.words[2] == 0x7c00u && fixture.rgb[6] == 0u && fixture.rgb[7] == 0u &&
          fixture.rgb[8] == 255u);
    CHECK(compare_reference_png(fixture.words, VRAM_WORDS, fixture.png, fixture.png_size) ==
          REFERENCE_MATCH);

    /* Bit 15 is a mask flag, not part of the RGB555 reference color. */
    fixture.words[3] ^= 0x8000u;
    CHECK(compare_reference_png(fixture.words, VRAM_WORDS, fixture.png, fixture.png_size) ==
          REFERENCE_MATCH);
    fixture.words[3] ^= 0x8000u;

    /* An eight-step red change crosses one RGB555 bucket and must be visible. */
    fixture.rgb[0] = (uint8_t)(fixture.rgb[0] - 8u);
    CHECK(encode_fixture(&fixture, VRAM_WIDTH, VRAM_HEIGHT));
    CHECK(compare_reference_png(fixture.words, VRAM_WORDS, fixture.png, fixture.png_size) ==
          REFERENCE_MISMATCH);
    fixture.rgb[0] = 255u;
    CHECK(encode_fixture(&fixture, VRAM_WIDTH, VRAM_HEIGHT));

    reference_fixture wrong_dimensions;
    int wrong_made = make_fixture(&wrong_dimensions, VRAM_WIDTH - 1u, VRAM_HEIGHT);
    CHECK(wrong_made);
    if (wrong_made) {
        CHECK(compare_reference_png(fixture.words, VRAM_WORDS, wrong_dimensions.png,
                                    wrong_dimensions.png_size) == REFERENCE_INVALID);
    }
    free_fixture(&wrong_dimensions);
    CHECK(compare_reference_png(fixture.words, VRAM_WORDS, NULL, 0u) == REFERENCE_INVALID);
    free_fixture(&fixture);
}

int main(int argc, char **argv)
{
    if (argc == 4 && strcmp(argv[1], "capture") == 0) {
        return compare_capture(argv[2], argv[3]);
    }
    if (argc != 2) {
        fprintf(stderr,
                "usage: schwaystation-test-gpu-reference quantization|controls|capture EXE RGB555LE\n");
        return 2;
    }
    if (strcmp(argv[1], "quantization") == 0) {
        quantization();
    } else if (strcmp(argv[1], "controls") == 0) {
        controls();
    } else {
        fprintf(stderr, "unknown GPU reference case: %s\n", argv[1]);
        return 2;
    }
    return failures == 0u ? 0 : 1;
}
