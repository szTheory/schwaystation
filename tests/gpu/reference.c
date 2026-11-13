/* Synthetic RGB555 controls and a whole-VRAM comparison for pinned captures.
   Python's bounded host adapter canonicalizes the capture; this file loads the
   pinned program through the public API and compares every VRAM word. */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "schwaystation.h"
#include "internal.h"
#include "bus.h"
#include "png.h"
#include "sha256.h"
#include "test_open.h"

enum {
    VRAM_WIDTH = 1024u,
    VRAM_HEIGHT = 512u,
    VRAM_WORDS = VRAM_WIDTH * VRAM_HEIGHT,
    RGB_BYTES = 3u,
    MAX_PNG_BYTES = 2u * 1024u * 1024u,
    MAX_EXE_BYTES = 2u * 1024u * 1024u + 0x800u,
    MAX_CAPTURE_ADVANCES = 32u,
    MAX_ALLOWANCE_BYTES = 512u,
    LINE_VARIANT_SAMPLES = 64u,
    TRANSPARENCY_VARIANT_SAMPLES = 447488u
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

typedef struct reference_allowances {
    uint32_t lines_used;
    uint32_t transparency_used;
} reference_allowances;

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

enum {
    ALLOWANCE_NONE = 0u,
    ALLOWANCE_LINES_ZERO_RGB3 = 1u,
    ALLOWANCE_TRANSPARENCY_OLD_CLEAR = 2u
};

static int parse_allowance_policy(const uint8_t *bytes, size_t size)
{
    if (bytes == NULL || size == 0u || size > MAX_ALLOWANCE_BYTES) {
        return 0;
    }

    uint32_t seen = 0u;
    size_t cursor = 0u;
    while (cursor < size) {
        size_t start = cursor;
        while (cursor < size && bytes[cursor] != (uint8_t)'\n') {
            cursor++;
        }
        size_t length = cursor - start;
        if (cursor < size) {
            cursor++;
        }
        if (length >= 128u || memchr(bytes + start, '\0', length) != NULL) {
            return 0;
        }

        char line[128];
        memcpy(line, bytes + start, length);
        line[length] = '\0';
        char *text = line;
        while (*text != '\0' && isspace((unsigned char)*text) != 0) {
            text++;
        }
        if (*text == '\0' || *text == '#') {
            continue;
        }

        char case_name[32];
        char variant[64];
        char extra;
        unsigned long count = 0ul;
        if (sscanf(text, "%31s %63s %lu %c", case_name, variant, &count, &extra) != 3) {
            return 0;
        }
        if (strcmp(case_name, "lines") == 0 && strcmp(variant, "zero-rgb3-cyan") == 0 &&
            count == (unsigned long)LINE_VARIANT_SAMPLES && (seen & 1u) == 0u) {
            seen |= 1u;
        } else if (strcmp(case_name, "transparency") == 0 &&
                   strcmp(variant, "old-white-clear") == 0 &&
                   count == (unsigned long)TRANSPARENCY_VARIANT_SAMPLES && (seen & 2u) == 0u) {
            seen |= 2u;
        } else {
            return 0;
        }
    }
    return seen == 3u;
}

static int read_allowance_policy(const char *path)
{
    size_t size = 0u;
    uint8_t *bytes = read_bounded_file(path, MAX_ALLOWANCE_BYTES, &size);
    int valid = parse_allowance_policy(bytes, size);
    free(bytes);
    return valid;
}

static int line_variant_word(size_t index, uint16_t *word_out)
{
    static const int8_t dither[4][4] = {
        {-4, 0, -3, 1}, {2, -2, 3, -1}, {-3, 1, -4, 0}, {3, -1, 2, -2}};
    uint32_t x = (uint32_t)(index % VRAM_WIDTH);
    uint32_t y = (uint32_t)(index / VRAM_WIDTH);
    if (y < 140u || y >= 172u) {
        return 0;
    }
    uint32_t row = y - 140u;
    uint32_t segment = 0u;
    if (x == 150u + row) {
        segment = 0u;
    } else if (x == 210u + row) {
        segment = 1u;
    } else {
        return 0;
    }

    uint32_t channel = ((255u * 4096u / 32u) * row + 2048u) / 4096u;
    int32_t corrected = (int32_t)channel + dither[y & 3u][x & 3u];
    if (corrected < 0) {
        corrected = 0;
    } else if (corrected > 255) {
        corrected = 255;
    }
    uint16_t five = (uint16_t)((uint32_t)corrected / 8u);
    uint16_t alternate = (uint16_t)((five << 5) | (five << 10));
    if (segment != 0u) {
        uint16_t destination = row == 0u ? 0x3DFFu : 0x7FFFu;
        uint16_t blended = 0u;
        for (uint32_t shift = 0u; shift <= 10u; shift += 5u) {
            uint32_t source_channel = (alternate >> shift) & 31u;
            uint32_t destination_channel = (destination >> shift) & 31u;
            blended |= (uint16_t)(((source_channel + destination_channel) / 2u) << shift);
        }
        alternate = blended;
    }
    *word_out = alternate;
    return 1;
}

static const char *reviewed_variant_sha256(const char *case_name)
{
    if (strcmp(case_name, "lines") == 0) {
        return "0e68352be9252cb6fe733e2f24db3cb70a82df14c175c1fe24334a2dfff239d1";
    }
    if (strcmp(case_name, "transparency") == 0) {
        return "82a71923f5d9bfaded9aef65f307d8023d4578df70639dd5fc254b7e885dc1e8";
    }
    return NULL;
}

static int reviewed_variant_fixture_matches(const char *case_name, const uint8_t *exe,
                                            size_t exe_size)
{
    const char *expected = reviewed_variant_sha256(case_name);
    if (expected == NULL || exe == NULL || exe_size == 0u) {
        return 0;
    }

    schway__sha256 hash;
    uint8_t digest[32];
    char hex[65];
    schway__sha256_init(&hash);
    schway__sha256_update(&hash, exe, exe_size);
    schway__sha256_final(&hash, digest);
    schway__sha256_hex(digest, hex);
    return strcmp(hex, expected) == 0;
}

static int capture_allowance_matches(const char *case_name, size_t index, uint16_t expected,
                                     uint16_t actual, int reviewed_variant_fixture,
                                     uint32_t *allowance_out)
{
    if (actual == expected) {
        return 0;
    }
    if (strcmp(case_name, "lines") == 0 && reviewed_variant_fixture != 0) {
        uint16_t alternate = 0u;
        if (line_variant_word(index, &alternate) && actual == alternate) {
            *allowance_out = ALLOWANCE_LINES_ZERO_RGB3;
            return 1;
        }
    }
    if (strcmp(case_name, "transparency") == 0 && reviewed_variant_fixture != 0) {
        uint32_t x = (uint32_t)(index % VRAM_WIDTH);
        uint32_t y = (uint32_t)(index / VRAM_WIDTH);
        if ((x >= 320u || y >= 240u) && expected == 0x7FFFu && actual == 0u) {
            *allowance_out = ALLOWANCE_TRANSPARENCY_OLD_CLEAR;
            return 1;
        }
    }
    return 0;
}

static int allowance_usage_valid(const char *case_name, const reference_allowances *allowances)
{
    if (strcmp(case_name, "lines") == 0) {
        return allowances->lines_used == LINE_VARIANT_SAMPLES;
    }
    if (strcmp(case_name, "transparency") == 0) {
        return allowances->transparency_used == TRANSPARENCY_VARIANT_SAMPLES;
    }
    return 1;
}

static int compare_capture(const char *case_name, const char *exe_path, const char *golden_path,
                           uint32_t advance_limit, const char *allowance_path)
{
    size_t exe_size = 0u;
    size_t golden_size = 0u;
    uint8_t *exe = read_bounded_file(exe_path, MAX_EXE_BYTES, &exe_size);
    uint8_t *golden = read_bounded_file(golden_path, VRAM_WORDS * 2u, &golden_size);
    reference_allowances allowances;
    memset(&allowances, 0, sizeof allowances);
    schway *inst = NULL;
    int exit_code = 2;

    if (!read_allowance_policy(allowance_path)) {
        fprintf(stderr, "GPU_REFERENCE_INPUT_ERROR: GPU allowance policy is missing or invalid\n");
        goto cleanup;
    }
    if (exe == NULL || exe_size == 0u) {
        fprintf(stderr, "GPU_REFERENCE_INPUT_ERROR: cannot read bounded %s executable\n", case_name);
        goto cleanup;
    }
    int reviewed_variant_fixture = 0;
    if (reviewed_variant_sha256(case_name) != NULL) {
        reviewed_variant_fixture = reviewed_variant_fixture_matches(case_name, exe, exe_size);
        if (!reviewed_variant_fixture) {
            fprintf(stderr, "GPU_REFERENCE_ALLOWANCE_ERROR: %s executable SHA-256 is not reviewed\n",
                    case_name);
            exit_code = 1;
            goto cleanup;
        }
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
        fprintf(stderr, "GPU_REFERENCE_INPUT_ERROR: %s executable was refused: %s\n", case_name,
                schway_status_string(status));
        goto cleanup;
    }

    uint64_t previous_frame = 0u;
    uint64_t previous_ticks = 0u;
    uint32_t last_stop = 0u;
    for (uint32_t advance = 0u; advance < advance_limit; advance++) {
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
                    "GPU_REFERENCE_CORE_ERROR: %s bounded run %u stopped unexpectedly "
                    "(stop=%u frame=%llu ticks=%llu fault=%u)\n",
                    case_name, advance + 1u, result.stop, (unsigned long long)result.frame,
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
            uint32_t allowance = ALLOWANCE_NONE;
            if (capture_allowance_matches(case_name, index, expected, actual,
                                          reviewed_variant_fixture, &allowance)) {
                if (allowance == ALLOWANCE_LINES_ZERO_RGB3) {
                    allowances.lines_used++;
                } else if (allowance == ALLOWANCE_TRANSPARENCY_OLD_CLEAR) {
                    allowances.transparency_used++;
                }
                continue;
            }
            if (mismatches == 0u) {
                first_mismatch = index;
                first_expected = expected;
                first_actual = actual;
            }
            mismatches++;
        }
    }

    if (!allowance_usage_valid(case_name, &allowances)) {
        uint32_t used = strcmp(case_name, "lines") == 0 ? allowances.lines_used
                                                         : allowances.transparency_used;
        uint32_t required = strcmp(case_name, "lines") == 0 ? LINE_VARIANT_SAMPLES
                                                              : TRANSPARENCY_VARIANT_SAMPLES;
        fprintf(stderr,
                "GPU_REFERENCE_ALLOWANCE_ERROR: %s used %u of %u exact source-variant pixels\n",
                case_name, used, required);
        exit_code = 1;
        goto cleanup;
    }

    if (mismatches != 0u) {
        fprintf(stderr,
                "GPU_REFERENCE_RASTER_MISMATCH: %s: %u of %u words; first=%zu expected=%04x actual=%04x; advances=%u ticks=%llu\n",
                case_name, mismatches, VRAM_WORDS, first_mismatch, first_expected, first_actual,
                advance_limit, (unsigned long long)previous_ticks);
        exit_code = 1;
        goto cleanup;
    }

    printf("GPU_REFERENCE_CAPTURE_MATCH: case=%s words=%u advances=%u stop=%s ticks=%llu tolerance=0",
           case_name, VRAM_WORDS, advance_limit,
           last_stop == SCHWAY_STOP_FRAME ? "frame" : "budget",
           (unsigned long long)previous_ticks);
    if (strcmp(case_name, "lines") == 0) {
        printf(" allowance=zero-rgb3-cyan:%u", allowances.lines_used);
    } else if (strcmp(case_name, "transparency") == 0) {
        printf(" allowance=old-white-clear:%u", allowances.transparency_used);
    }
    putchar('\n');
    exit_code = 0;

cleanup:
    schway_destroy(inst);
    free(exe);
    free(golden);
    return exit_code;
}

/* These command sequences translate ps1-tests gpu/{gp0-e1,mask-bit,gpustat}/main.cpp
   at f727802fead11f1daa7549285548392ef87749cb. Ordered pass names come from the
   corresponding pinned psx.log files. Expected values remain the test source's
   independent assertions; none are learned from emulator output. */
#define GPU_LOG_MAX_BYTES 65536u
#define GPU_GP0 0x1F801810u
#define GPU_GP1 0x1F801814u
#define DMA_MADR(ch) (0x1F801080u + 0x10u * (ch))
#define DMA_BCR(ch) (DMA_MADR(ch) + 4u)
#define DMA_CHCR(ch) (DMA_MADR(ch) + 8u)
#define DMA_DPCR 0x1F8010F0u
#define DMA_DPCR_GPU_ENABLED 0x07654B21u
#define GPU_E1_MASK 0x000087FFu
#define GPU_STAT_MASK ((1u << 25) | (1u << 26) | (1u << 27) | (1u << 28) | (3u << 29))

typedef struct gpu_log_spec {
    const char *name;
    const char *header;
    const char *const *predicates;
    size_t predicate_count;
} gpu_log_spec;

static const char *const gp0_e1_predicates[] = {
    "testWriteZerosToE1",
    "testWriteOnesToE1",
    "testWriteOnesToE1WithTextureDisable",
    "testTexturedPolygons",
    "testTexturedPolygonsTextureDisable",
    "testTexturedPolygonsDoesNotChangeOtherBits",
    "testTextureDisableBitIsIgnoredWhenNotAllowed",
    "testTextureDisableBitIsWrittenWhenAllowed",
    "testUnsetAllowTextureDisablePreservesBit",
    "testUnsetAllowTextureDisableClearsBitAfterWrite",
};

static const char *const mask_bit_predicates[] = {
    "testWriteAndRead",
    "testSetBit",
    "testCheckMaskBit",
    "testCheckIsMaskBitStickyManually",
    "testCheckIsMaskBitStickySetBit",
};

static const char *const gpustat_predicates[] = {
    "testGpustat_CPU_beforeRender",
    "testGpustat_CPU_dringRender",
    "testGpustat_CPU_afterRender",
    "testGpustat_linkedListDMA_beforeRender",
    "testGpustat_LinkedListDMA_duringRender",
    "testGpustat_LinkedListDA_afterRender",
    "testGpustat_SyncModeDMA_DMAdirection1_beforeRender",
    "testGpustat_SyncModeDMA_DMAdirection1_duringRender",
    "testGpustat_SyncModeDMA_DMAdirection1_afterRender",
    "testGpustat_SyncModeDMA_DMAdirection2_beforeRender",
    "testGpustat_SyncModeDMA_DMAdirection2_duringRender",
    "testGpustat_SyncModeDMA_DMAdirection2_afterRender",
};

static const char *const gpustat_log_prelude[] = {
    "Bits in GPUSTAT ecription:",
    "Bit 25 - DMA Request ",
    "  - When DMA Direction == 0 (Off) - always 0",
    "  - When DMA Direction == 1 (FIFO) - ?",
    "  - When DMA Direction == 2 (CPUtoGP0) - same as Bit 28",
    "  - When DMA Direction == 3 (GPUREADtoCPU) - same as Bt 27",
    "Bit 26 - Ready to receive Cmd Word",
    "Bit 27 - Ready to send VRAM to CPU",
    "Bit 28 - Ready to receive DMA Block",
    "Bits 29-30 - DMA Direction (0 - Off, 1 - FIFO, 2 - CPUtoGP0, 3 - GPUREADtoCPU)",
};

static const gpu_log_spec gpu_log_specs[] = {
    {"gp0-e1", "gpu/gp0-e1", gp0_e1_predicates,
     sizeof gp0_e1_predicates / sizeof gp0_e1_predicates[0]},
    {"mask-bit", "gpu/mask-bit", mask_bit_predicates,
     sizeof mask_bit_predicates / sizeof mask_bit_predicates[0]},
    {"gpustat", "gpu/gpustat", gpustat_predicates,
     sizeof gpustat_predicates / sizeof gpustat_predicates[0]},
};

static const gpu_log_spec *find_gpu_log_spec(const char *name)
{
    for (size_t i = 0u; i < sizeof gpu_log_specs / sizeof gpu_log_specs[0]; i++) {
        if (strcmp(name, gpu_log_specs[i].name) == 0) {
            return &gpu_log_specs[i];
        }
    }
    return NULL;
}

static int append_control_text(char *buffer, size_t capacity, size_t *used, const char *text)
{
    int length = snprintf(buffer + *used, capacity - *used, "%s", text);
    if (length < 0 || (size_t)length >= capacity - *used) {
        return 0;
    }
    *used += (size_t)length;
    return 1;
}

/* A valid hardware log has one identifying header, its ordered pass list, and
   one Done. line. gpustat also has descriptive text before its pass list. */
static int gpu_log_matches(const gpu_log_spec *spec, const uint8_t *bytes, size_t size,
                           int report)
{
    size_t at = 0u;
    size_t matched = 0u;
    size_t prelude = 0u;
    int header_seen = 0;
    int done_seen = 0;
    while (at < size) {
        size_t end = at;
        while (end < size && bytes[end] != '\n') {
            end++;
        }
        size_t length = end - at;
        if (length != 0u && bytes[at + length - 1u] == '\r') {
            length--;
        }
        const uint8_t *line = bytes + at;
        at = end < size ? end + 1u : size;
        if (length == 0u) {
            continue;
        }
        if (!header_seen) {
            header_seen = length == strlen(spec->header) &&
                          memcmp(line, spec->header, length) == 0;
            if (!header_seen) {
                if (report) {
                    fprintf(stderr, "GPU_REFERENCE_LOG_ERROR: %s has no %s header\n",
                            spec->name, spec->header);
                }
                return 0;
            }
            continue;
        }
        if (length == 5u && memcmp(line, "Done.", 5u) == 0) {
            if (done_seen || matched != spec->predicate_count ||
                (strcmp(spec->name, "gpustat") == 0 &&
                 prelude != sizeof gpustat_log_prelude / sizeof gpustat_log_prelude[0]) ||
                at != size) {
                if (report) {
                    fprintf(stderr, "GPU_REFERENCE_LOG_ERROR: %s has a missing, duplicate, or trailing predicate\n",
                            spec->name);
                }
                return 0;
            }
            done_seen = 1;
            continue;
        }
        if (length >= 7u && memcmp(line, "fail - ", 7u) == 0) {
            if (report) {
                fprintf(stderr, "GPU_REFERENCE_LOG_ERROR: %s records a hardware failure\n",
                        spec->name);
            }
            return 0;
        }
        if (strcmp(spec->name, "gpustat") == 0 && matched == 0u &&
            prelude < sizeof gpustat_log_prelude / sizeof gpustat_log_prelude[0]) {
            const char *expected = gpustat_log_prelude[prelude];
            size_t expected_size = strlen(expected);
            if (length != expected_size || memcmp(line, expected, expected_size) != 0) {
                if (report) {
                    fprintf(stderr, "GPU_REFERENCE_LOG_ERROR: gpustat prelude line %zu changed\n",
                            prelude + 1u);
                }
                return 0;
            }
            prelude++;
            continue;
        }
        if (length >= 7u && memcmp(line, "pass - ", 7u) == 0) {
            const char *expected = matched < spec->predicate_count ?
                                       spec->predicates[matched] : NULL;
            size_t expected_size = expected == NULL ? 0u : strlen(expected);
            if (expected == NULL || length != 7u + expected_size ||
                memcmp(line + 7u, expected, expected_size) != 0) {
                if (report) {
                    fprintf(stderr, "GPU_REFERENCE_LOG_ERROR: %s predicate %zu is absent or out of order\n",
                            spec->name, matched + 1u);
                }
                return 0;
            }
            matched++;
            continue;
        }
        if (report) {
            fprintf(stderr, "GPU_REFERENCE_LOG_ERROR: %s contains an unexpected line\n",
                    spec->name);
        }
        return 0;
    }
    if (!header_seen || !done_seen) {
        if (report) {
            fprintf(stderr, "GPU_REFERENCE_LOG_ERROR: %s is missing its header or Done.\n",
                    spec->name);
        }
        return 0;
    }
    return 1;
}

/* Control fixtures are deliberately synthesized only to test refusal paths;
   actual log tests always read the pinned reference file. */
static size_t make_gpu_log_control(const gpu_log_spec *spec, char *buffer, size_t capacity,
                                   uint32_t fault)
{
    size_t used = 0u;
    if (!append_control_text(buffer, capacity, &used, spec->header) ||
        !append_control_text(buffer, capacity, &used, "\n")) {
        return 0u;
    }
    if (strcmp(spec->name, "gpustat") == 0) {
        for (size_t i = 0u; i < sizeof gpustat_log_prelude / sizeof gpustat_log_prelude[0]; i++) {
            if (!append_control_text(buffer, capacity, &used, gpustat_log_prelude[i]) ||
                !append_control_text(buffer, capacity, &used, "\n")) {
                return 0u;
            }
        }
    }
    if (fault == 3u &&
        (!append_control_text(buffer, capacity, &used, "fail - ") ||
         !append_control_text(buffer, capacity, &used, spec->predicates[0]) ||
         !append_control_text(buffer, capacity, &used, "\n"))) {
        return 0u;
    }
    for (size_t i = 0u; i < spec->predicate_count; i++) {
        size_t index = i;
        if (fault == 1u && i == 0u) {
            index = 1u;
        } else if (fault == 1u && i == 1u) {
            index = 0u;
        }
        if (fault == 2u && i + 1u == spec->predicate_count) {
            continue;
        }
        if (fault == 4u && i == 1u &&
            (!append_control_text(buffer, capacity, &used, "pass - testNotInHardwareLog\n"))) {
            return 0u;
        }
        if (!append_control_text(buffer, capacity, &used, "pass - ") ||
            !append_control_text(buffer, capacity, &used, spec->predicates[index]) ||
            !append_control_text(buffer, capacity, &used, "\n")) {
            return 0u;
        }
    }
    if (!append_control_text(buffer, capacity, &used, "Done.\n")) {
        return 0u;
    }
    return used;
}

static void check_gpu_reference_value(const char *log_case, const char *predicate,
                                      uint32_t actual, uint32_t expected, uint32_t mask)
{
    if ((actual & mask) != (expected & mask)) {
        fprintf(stderr,
                "GPU_REFERENCE_REGISTER_MISMATCH: %s/%s expected=%08x actual=%08x mask=%08x\n",
                log_case, predicate, expected, actual, mask);
        failures++;
    }
}

static void log_gpu_write(schway *inst, uint32_t port, uint32_t value)
{
    uint32_t cycles = 0u;
    CHECK(schway__bus_write(inst, port, 4u, value, &cycles) == 0);
}

static uint32_t log_gpu_read(schway *inst, uint32_t port)
{
    uint32_t cycles = 0u;
    uint32_t value = 0u;
    CHECK(schway__bus_read(inst, port, 4u, &value, &cycles) == 0);
    return value;
}

static schway *new_log_gpu(void)
{
    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = reference_alloc;
    config.allocator.free = reference_free;
    schway *inst = NULL;
    schway_status status = schway_create(&config, &inst);
    if (status != SCHWAY_STATUS_OK || inst == NULL) {
        fprintf(stderr, "GPU_REFERENCE_CORE_ERROR: log fixture creation failed: %s\n",
                schway_status_string(status));
        failures++;
        return NULL;
    }
    schway_profile profile;
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = SCHWAY_REGION_NTSC_U;
    profile.flags = 0u;
    status = schway_set_profile(inst, &profile);
    if (status != SCHWAY_STATUS_OK) {
        fprintf(stderr, "GPU_REFERENCE_CORE_ERROR: log fixture profile rejected: %s\n",
                schway_status_string(status));
        failures++;
        schway_destroy(inst);
        return NULL;
    }
    return inst;
}

static void log_gpu_run_cycles(schway *inst, uint64_t cycles)
{
    uint64_t target = inst->ticks + cycles;
    while (inst->ticks < target) {
        schway_budget budget;
        schway_run_result result;
        memset(&budget, 0, sizeof budget);
        memset(&result, 0, sizeof result);
        budget.size = (uint32_t)sizeof budget;
        budget.max_cycles = target - inst->ticks;
        result.size = (uint32_t)sizeof result;
        uint64_t before = inst->ticks;
        schway_status status = schway_run(inst, &budget, &result);
        if (status != SCHWAY_STATUS_OK || inst->ticks <= before ||
            result.stop == SCHWAY_STOP_FAULT || result.fault_present != 0u) {
            fprintf(stderr,
                    "GPU_REFERENCE_CORE_ERROR: log fixture advance failed (%s, stop=%u)\n",
                    schway_status_string(status), result.stop);
            failures++;
            return;
        }
    }
}

/* A CPU read on the I/O bus first waits for a running DMA unit. The first bus
   access asks the normal bus path to establish that hold; public schway_run
   advances only to its exact scheduler deadline before the operation is read. */
static uint32_t log_gpu_read_after_dma_stall(schway *inst, uint32_t port)
{
    unsigned failures_before = failures;
    for (;;) {
        uint32_t cycles = 0u;
        uint32_t value = 0u;
        CHECK(schway__bus_read(inst, port, 4u, &value, &cycles) == 0);
        uint64_t hold_until = inst->dma.stall_until;
        if (hold_until <= inst->ticks) {
            return value;
        }
        log_gpu_run_cycles(inst, hold_until - inst->ticks);
        if (failures != failures_before) {
            return value;
        }
    }
}

static uint32_t log_gpu_stat_bits(schway *inst)
{
    return log_gpu_read(inst, GPU_GP1) & GPU_STAT_MASK;
}

static uint32_t gpu_status_expected(uint32_t request, uint32_t command_ready,
                                    uint32_t vram_ready, uint32_t block_ready,
                                    uint32_t direction)
{
    return (request << 25) | (command_ready << 26) | (vram_ready << 27) |
           (block_ready << 28) | (direction << 29);
}

static void check_gpustat(const char *predicate, schway *inst, uint32_t request,
                          uint32_t command_ready, uint32_t vram_ready, uint32_t block_ready,
                          uint32_t direction)
{
    check_gpu_reference_value("gpustat", predicate, log_gpu_stat_bits(inst),
                              gpu_status_expected(request, command_ready, vram_ready,
                                                 block_ready, direction),
                              GPU_STAT_MASK);
}

static void write_vertex(schway *inst, uint32_t x, uint32_t y)
{
    log_gpu_write(inst, GPU_GP0, (x & 0x7FFu) | ((y & 0x7FFu) << 16));
}

static void write_uv(schway *inst, uint32_t u, uint32_t v, uint32_t high)
{
    log_gpu_write(inst, GPU_GP0, (u & 0xFFu) | ((v & 0xFFu) << 8) | ((high & 0xFFFFu) << 16));
}

static void draw_source_ft4(schway *inst, uint32_t tpage)
{
    /* ps1-tests setPolyFT4 + setUVWH + setXY4: unshaded textured quad. */
    log_gpu_write(inst, GPU_GP0, 0x2C808080u);
    write_vertex(inst, 0u, 0u);
    write_uv(inst, 0u, 0u, 0u); /* CLUT from vertex one */
    write_vertex(inst, 0u, 32u);
    write_uv(inst, 0u, 0u, tpage); /* texture page from vertex two */
    write_vertex(inst, 32u, 0u);
    write_uv(inst, 255u, 255u, 0u);
    write_vertex(inst, 32u, 32u);
    write_uv(inst, 255u, 255u, 0u);
}

static void run_gp0_e1_operations(void)
{
    for (size_t i = 0u; i < sizeof gp0_e1_predicates / sizeof gp0_e1_predicates[0]; i++) {
        schway *inst = new_log_gpu();
        if (inst == NULL) {
            return;
        }
        uint32_t expected = 0u;
        if (i == 0u) {
            log_gpu_write(inst, GPU_GP1, 0x09000000u);
            log_gpu_write(inst, GPU_GP0, 0xE1000000u);
        } else if (i == 1u || i == 2u) {
            uint32_t allow = i == 2u ? 1u : 0u;
            log_gpu_write(inst, GPU_GP1, 0x09000000u | allow);
            log_gpu_write(inst, GPU_GP0, 0xE1000FFFu);
            expected = allow != 0u ? 0x000087FFu : 0x000007FFu;
        } else if (i == 3u || i == 4u) {
            uint32_t allow = i == 4u ? 1u : 0u;
            log_gpu_write(inst, GPU_GP1, 0x09000000u | allow);
            log_gpu_write(inst, GPU_GP0, 0xE1000000u);
            draw_source_ft4(inst, 0xFFFFu);
            expected = allow != 0u ? 0x000081FFu : 0x000001FFu;
        } else if (i == 5u) {
            log_gpu_write(inst, GPU_GP1, 0x09000001u);
            log_gpu_write(inst, GPU_GP0, 0xE1000FFFu);
            draw_source_ft4(inst, 0u);
            expected = 0x00000600u;
        } else if (i == 6u) {
            log_gpu_write(inst, GPU_GP1, 0x09000000u);
            log_gpu_write(inst, GPU_GP0, 0xE1000800u);
        } else if (i == 7u || i == 8u) {
            log_gpu_write(inst, GPU_GP1, 0x09000001u);
            log_gpu_write(inst, GPU_GP0, 0xE1000800u);
            if (i == 8u) {
                log_gpu_write(inst, GPU_GP1, 0x09000000u);
            }
            expected = 0x00008000u;
        } else {
            log_gpu_write(inst, GPU_GP1, 0x09000001u);
            log_gpu_write(inst, GPU_GP0, 0xE1000800u);
            log_gpu_write(inst, GPU_GP1, 0x09000000u);
            log_gpu_write(inst, GPU_GP0, 0xE1000000u);
        }
        uint32_t actual = log_gpu_read(inst, GPU_GP1) & GPU_E1_MASK;
        check_gpu_reference_value("gp0-e1", gp0_e1_predicates[i], actual, expected, GPU_E1_MASK);
        schway_destroy(inst);
    }
}

static void write_vram_pixel(schway *inst, uint32_t x, uint32_t y, uint16_t value)
{
    log_gpu_write(inst, GPU_GP0, 0xA0000000u);
    log_gpu_write(inst, GPU_GP0, (y << 16) | x);
    log_gpu_write(inst, GPU_GP0, 0x00010001u);
    log_gpu_write(inst, GPU_GP0, value);
}

static uint16_t read_vram_pixel(schway *inst, uint32_t x, uint32_t y)
{
    log_gpu_write(inst, GPU_GP0, 0xC0000000u);
    log_gpu_write(inst, GPU_GP0, (y << 16) | x);
    log_gpu_write(inst, GPU_GP0, 0x00010001u);
    return (uint16_t)log_gpu_read(inst, GPU_GP0);
}

static void set_mask_bit(schway *inst, uint32_t set, uint32_t check)
{
    log_gpu_write(inst, GPU_GP0, 0xE6000000u | (set & 1u) | ((check & 1u) << 1));
}

static void run_mask_bit_operations(void)
{
    static const uint16_t expected[] = {0x1234u, 0x8000u, 0x8000u, 0x0456u, 0x0456u};
    static const uint32_t coordinates[][2] = {{32u, 32u}, {33u, 32u}, {34u, 32u},
                                               {35u, 32u}, {36u, 32u}};
    for (size_t i = 0u; i < sizeof mask_bit_predicates / sizeof mask_bit_predicates[0]; i++) {
        schway *inst = new_log_gpu();
        if (inst == NULL) {
            return;
        }
        uint32_t x = coordinates[i][0];
        uint32_t y = coordinates[i][1];
        if (i == 0u) {
            set_mask_bit(inst, 0u, 0u);
            write_vram_pixel(inst, x, y, 0x1234u);
        } else if (i == 1u) {
            set_mask_bit(inst, 1u, 0u);
            write_vram_pixel(inst, x, y, 0u);
        } else if (i == 2u) {
            set_mask_bit(inst, 0u, 0u);
            write_vram_pixel(inst, x, y, 0x8000u);
            set_mask_bit(inst, 0u, 1u);
            write_vram_pixel(inst, x, y, 0x1234u);
        } else if (i == 3u) {
            set_mask_bit(inst, 0u, 0u);
            write_vram_pixel(inst, x, y, 0x8123u);
            write_vram_pixel(inst, x, y, 0x0456u);
        } else {
            set_mask_bit(inst, 1u, 0u);
            write_vram_pixel(inst, x, y, 0u);
            set_mask_bit(inst, 0u, 0u);
            write_vram_pixel(inst, x, y, 0x0456u);
        }
        uint16_t actual = read_vram_pixel(inst, x, y);
        check_gpu_reference_value("mask-bit", mask_bit_predicates[i], actual, expected[i], 0xFFFFu);
        schway_destroy(inst);
    }
}

static void draw_source_tile(schway *inst, uint32_t x, uint32_t y, uint32_t rgb)
{
    log_gpu_write(inst, GPU_GP0, 0x60000000u | (rgb & 0x00FFFFFFu));
    log_gpu_write(inst, GPU_GP0, (y << 16) | x);
    log_gpu_write(inst, GPU_GP0, (128u << 16) | 128u);
}

static void write_ram_word(schway *inst, uint32_t address, uint32_t value)
{
    inst->bus.ram[address] = (uint8_t)value;
    inst->bus.ram[address + 1u] = (uint8_t)(value >> 8);
    inst->bus.ram[address + 2u] = (uint8_t)(value >> 16);
    inst->bus.ram[address + 3u] = (uint8_t)(value >> 24);
}

static void enable_gpu_dma(schway *inst, uint32_t direction)
{
    /* DMA::masterEnable(GPU, true), followed by writeGP1(4, direction). */
    log_gpu_write(inst, DMA_DPCR, DMA_DPCR_GPU_ENABLED);
    log_gpu_write(inst, GPU_GP1, 0x04000000u | direction);
}

static void start_gpu_dma(schway *inst, uint32_t address, uint32_t bcr, uint32_t chcr)
{
    /* setupDmaGpu writes MADR, BCR, then CHCR after the direction snapshot. */
    log_gpu_write(inst, DMA_MADR(2u), address);
    log_gpu_write(inst, DMA_BCR(2u), bcr);
    log_gpu_write(inst, DMA_CHCR(2u), chcr);
}

static void write_dma_linked_tiles(schway *inst)
{
    write_ram_word(inst, 0x1000u, 0x03001010u);
    write_ram_word(inst, 0x1004u, 0x60008000u);
    write_ram_word(inst, 0x1008u, 0u);
    write_ram_word(inst, 0x100Cu, 0x00800080u);
    write_ram_word(inst, 0x1010u, 0x03FFFFFFu);
    write_ram_word(inst, 0x1014u, 0x60800000u);
    write_ram_word(inst, 0x1018u, 0x00000100u);
    write_ram_word(inst, 0x101Cu, 0x00800080u);
}

static void write_dma_sync_tiles(schway *inst)
{
    static const uint32_t words[8] = {
        0u, 0x60008080u, 0x00800000u, 0x00800080u,
        0u, 0x60808000u, 0x00800080u, 0x00800080u,
    };
    for (size_t i = 0u; i < sizeof words / sizeof words[0]; i++) {
        write_ram_word(inst, 0x2000u + (uint32_t)i * 4u, words[i]);
    }
}

static void run_gpustat_operations(void)
{
    schway *cpu = new_log_gpu();
    if (cpu == NULL) {
        return;
    }
    log_gpu_write(cpu, GPU_GP1, 0x04000000u);
    check_gpustat(gpustat_predicates[0], cpu, 0u, 1u, 0u, 1u, 0u);
    draw_source_tile(cpu, 0u, 0u, 0x00000080u);
    check_gpustat(gpustat_predicates[1], cpu, 0u, 0u, 0u, 1u, 0u);
    log_gpu_run_cycles(cpu, 1000000u);
    check_gpustat(gpustat_predicates[2], cpu, 0u, 1u, 0u, 1u, 0u);
    schway_destroy(cpu);

    schway *linked = new_log_gpu();
    if (linked == NULL) {
        return;
    }
    write_dma_linked_tiles(linked);
    enable_gpu_dma(linked, 2u);
    check_gpustat(gpustat_predicates[3], linked, 1u, 1u, 0u, 1u, 2u);
    start_gpu_dma(linked, 0x1000u, 0u, 0x01000401u);
    check_gpu_reference_value("gpustat", gpustat_predicates[4],
                              log_gpu_read_after_dma_stall(linked, GPU_GP1) & GPU_STAT_MASK,
                              gpu_status_expected(0u, 0u, 0u, 0u, 2u), GPU_STAT_MASK);
    log_gpu_run_cycles(linked, 100000u);
    check_gpustat(gpustat_predicates[5], linked, 1u, 1u, 0u, 1u, 2u);
    schway_destroy(linked);

    for (uint32_t direction = 1u; direction <= 2u; direction++) {
        size_t first = direction == 1u ? 6u : 9u;
        schway *sync = new_log_gpu();
        if (sync == NULL) {
            return;
        }
        write_dma_sync_tiles(sync);
        enable_gpu_dma(sync, direction);
        check_gpustat(gpustat_predicates[first], sync, 1u, 1u, 0u, 1u, direction);
        start_gpu_dma(sync, 0x2000u, 0x00010008u, 0x01000201u);
        check_gpu_reference_value("gpustat", gpustat_predicates[first + 1u],
                                  log_gpu_read_after_dma_stall(sync, GPU_GP1) & GPU_STAT_MASK,
                                  gpu_status_expected(direction == 1u ? 1u : 0u,
                                                      0u, 0u, 0u, direction),
                                  GPU_STAT_MASK);
        log_gpu_run_cycles(sync, 100000u);
        check_gpustat(gpustat_predicates[first + 2u], sync, 1u, 1u, 0u, 1u, direction);
        schway_destroy(sync);
    }
}

static int run_pinned_gpu_log(const char *name, const char *path)
{
    const gpu_log_spec *spec = find_gpu_log_spec(name);
    if (spec == NULL) {
        fprintf(stderr, "GPU_REFERENCE_INPUT_ERROR: unknown hardware log '%s'\n", name);
        return 2;
    }
    size_t size = 0u;
    uint8_t *bytes = read_bounded_file(path, GPU_LOG_MAX_BYTES, &size);
    if (bytes == NULL || size == 0u) {
        fprintf(stderr, "GPU_REFERENCE_INPUT_ERROR: cannot read bounded %s hardware log\n", name);
        free(bytes);
        return 2;
    }
    int valid = gpu_log_matches(spec, bytes, size, 1);
    free(bytes);
    if (!valid) {
        return 1;
    }
    printf("GPU_REFERENCE_LOG_MATCH: case=%s predicates=%zu ordered=1\n",
           spec->name, spec->predicate_count);
    fflush(stdout);
    unsigned before = failures;
    if (strcmp(name, "gp0-e1") == 0) {
        run_gp0_e1_operations();
    } else if (strcmp(name, "mask-bit") == 0) {
        run_mask_bit_operations();
    } else {
        run_gpustat_operations();
    }
    if (failures != before) {
        return 1;
    }
    printf("GPU_REFERENCE_OPERATION_MATCH: case=%s assertions=%zu\n",
           spec->name, spec->predicate_count);
    return 0;
}

static int run_gpu_predicate_controls(void)
{
    char buffer[GPU_LOG_MAX_BYTES];
    unsigned before = failures;
    for (size_t i = 0u; i < sizeof gpu_log_specs / sizeof gpu_log_specs[0]; i++) {
        const gpu_log_spec *spec = &gpu_log_specs[i];
        size_t size = make_gpu_log_control(spec, buffer, sizeof buffer, 0u);
        CHECK(size != 0u);
        CHECK(gpu_log_matches(spec, (const uint8_t *)buffer, size, 0));
        if (i == 0u) {
            size = make_gpu_log_control(spec, buffer, sizeof buffer, 2u);
            CHECK(size != 0u && !gpu_log_matches(spec, (const uint8_t *)buffer, size, 0));
        } else if (i == 1u) {
            size = make_gpu_log_control(spec, buffer, sizeof buffer, 1u);
            CHECK(size != 0u && !gpu_log_matches(spec, (const uint8_t *)buffer, size, 0));
        } else {
            size = make_gpu_log_control(spec, buffer, sizeof buffer, 3u);
            CHECK(size != 0u && !gpu_log_matches(spec, (const uint8_t *)buffer, size, 0));
        }
        size = make_gpu_log_control(spec, buffer, sizeof buffer, 4u);
        CHECK(size != 0u && !gpu_log_matches(spec, (const uint8_t *)buffer, size, 0));
    }
    size_t missing_size = 0u;
    CHECK(read_bounded_file("gpu-reference/does-not-exist/psx.log", GPU_LOG_MAX_BYTES,
                            &missing_size) == NULL);
    if (failures != before) {
        return 1;
    }
    puts("GPU_REFERENCE_PREDICATE_CONTROLS: all three selectors reject missing, bad, and unordered logs");
    return 0;
}

static int parse_advance_limit(const char *text, uint32_t *out)
{
    char *end = NULL;
    errno = 0;
    unsigned long value = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value == 0u || value > MAX_CAPTURE_ADVANCES) {
        return 0;
    }
    *out = (uint32_t)value;
    return 1;
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

static void allowance_controls(const char *allowance_path)
{
    static const char valid_policy[] =
        "# case variant exact-use-count\n"
        "lines zero-rgb3-cyan 64\n"
        "transparency old-white-clear 447488\n";
    static const char missing_variant[] = "lines zero-rgb3-cyan 64\n";
    static const char wrong_count[] =
        "lines zero-rgb3-cyan 63\ntransparency old-white-clear 447488\n";
    static const char duplicate_variant[] =
        "lines zero-rgb3-cyan 64\nlines zero-rgb3-cyan 64\n"
        "transparency old-white-clear 447488\n";
    static const char trailing_field[] =
        "lines zero-rgb3-cyan 64 ignored\ntransparency old-white-clear 447488\n";
    static const uint8_t changed_fixture[] = {0x53u, 0x43u, 0x48u, 0x57u};
    reference_allowances allowances;
    CHECK(read_allowance_policy(allowance_path));
    CHECK(!read_allowance_policy(""));
    CHECK(parse_allowance_policy((const uint8_t *)valid_policy, sizeof valid_policy - 1u));
    CHECK(!parse_allowance_policy((const uint8_t *)missing_variant, sizeof missing_variant - 1u));
    CHECK(!parse_allowance_policy((const uint8_t *)wrong_count, sizeof wrong_count - 1u));
    CHECK(!parse_allowance_policy((const uint8_t *)duplicate_variant, sizeof duplicate_variant - 1u));
    CHECK(!parse_allowance_policy((const uint8_t *)trailing_field, sizeof trailing_field - 1u));
    CHECK(!reviewed_variant_fixture_matches("lines", changed_fixture, sizeof changed_fixture));
    CHECK(!reviewed_variant_fixture_matches("transparency", changed_fixture,
                                            sizeof changed_fixture));
    CHECK(!reviewed_variant_fixture_matches("quad", changed_fixture, sizeof changed_fixture));

    size_t line_index = (size_t)140u * VRAM_WIDTH + 150u;
    uint16_t line_alternate = 0u;
    CHECK(line_variant_word(line_index, &line_alternate));
    uint32_t matched_allowance = ALLOWANCE_NONE;
    CHECK(capture_allowance_matches("lines", line_index,
                                    (uint16_t)(line_alternate ^ 1u), line_alternate, 1,
                                    &matched_allowance));
    CHECK(matched_allowance == ALLOWANCE_LINES_ZERO_RGB3);
    CHECK(!capture_allowance_matches("lines", line_index,
                                     (uint16_t)(line_alternate ^ 1u), line_alternate, 0,
                                     &matched_allowance));
    CHECK(!capture_allowance_matches("quad", line_index,
                                     (uint16_t)(line_alternate ^ 1u), line_alternate, 1,
                                     &matched_allowance));
    CHECK(!capture_allowance_matches("lines", line_index + 1u,
                                     (uint16_t)(line_alternate ^ 1u), line_alternate, 1,
                                     &matched_allowance));
    CHECK(!capture_allowance_matches("lines", line_index,
                                     (uint16_t)(line_alternate ^ 1u),
                                     (uint16_t)(line_alternate ^ 1u), 1,
                                     &matched_allowance));

    size_t semi_index = (size_t)140u * VRAM_WIDTH + 210u;
    uint16_t semi_alternate = 0u;
    CHECK(line_variant_word(semi_index, &semi_alternate));
    CHECK(semi_alternate == 0x1CEFu);

    size_t outside_index = 320u;
    CHECK(capture_allowance_matches("transparency", outside_index, 0x7FFFu, 0u, 1,
                                    &matched_allowance));
    CHECK(matched_allowance == ALLOWANCE_TRANSPARENCY_OLD_CLEAR);
    CHECK(!capture_allowance_matches("transparency", outside_index, 0x7FFFu, 0u, 0,
                                     &matched_allowance));
    size_t visible_index = (size_t)239u * VRAM_WIDTH + 319u;
    CHECK(!capture_allowance_matches("transparency", visible_index, 0x7FFFu, 0u, 1,
                                     &matched_allowance));
    CHECK(!capture_allowance_matches("transparency", outside_index, 0x7FFFu, 1u, 1,
                                     &matched_allowance));
    CHECK(!capture_allowance_matches("quad", outside_index, 0x7FFFu, 0u, 0,
                                     &matched_allowance));

    memset(&allowances, 0, sizeof allowances);
    CHECK(!allowance_usage_valid("lines", &allowances));
    allowances.lines_used = LINE_VARIANT_SAMPLES;
    CHECK(allowance_usage_valid("lines", &allowances));
    allowances.lines_used--;
    CHECK(!allowance_usage_valid("lines", &allowances));
    memset(&allowances, 0, sizeof allowances);
    allowances.transparency_used = TRANSPARENCY_VARIANT_SAMPLES;
    CHECK(allowance_usage_valid("transparency", &allowances));
    allowances.transparency_used--;
    CHECK(!allowance_usage_valid("transparency", &allowances));
}

int main(int argc, char **argv)
{
    if (argc == 4 && strcmp(argv[1], "log") == 0) {
        return run_pinned_gpu_log(argv[2], argv[3]);
    }
    if (argc == 2 && strcmp(argv[1], "predicate_controls") == 0) {
        return run_gpu_predicate_controls();
    }
    if (argc == 7 && strcmp(argv[1], "capture") == 0) {
        uint32_t advance_limit = 0u;
        if (!parse_advance_limit(argv[5], &advance_limit)) {
            fprintf(stderr, "GPU_REFERENCE_INPUT_ERROR: advance bound must be between 1 and %u\n",
                    (unsigned)MAX_CAPTURE_ADVANCES);
            return 2;
        }
        return compare_capture(argv[2], argv[3], argv[4], advance_limit, argv[6]);
    }
    if (argc == 3 && strcmp(argv[1], "allowance_controls") == 0) {
        allowance_controls(argv[2]);
        return failures == 0u ? 0 : 1;
    }
    if (argc != 2) {
        fprintf(stderr,
                "usage: schwaystation-test-gpu-reference quantization|controls|predicate_controls|log CASE PSX.LOG|allowance_controls ALLOW|capture CASE EXE RGB555LE ADVANCES ALLOW\n");
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
