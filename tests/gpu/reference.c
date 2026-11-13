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
                "usage: schwaystation-test-gpu-reference quantization|controls|allowance_controls ALLOW|capture CASE EXE RGB555LE ADVANCES ALLOW\n");
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
