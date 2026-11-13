/* Replay main shared by the fuzz harnesses, seeded from szTheory/muhtari
   d698364b180d tests/fuzz/replay_main.h (same author, MIT). Included at the
   end of a harness that defines LLVMFuzzerTestOneInput. Built only with
   SCHWAY_FUZZ_REPLAY, so the libFuzzer binary keeps libFuzzer's own main.

   Usage: <replay> [file...] runs every in-memory seed from seeds.c, then each
   file in order; a file ending in .hex holds the bytes as hexadecimal text
   (whitespace and #-comments to the end of a line are skipped), so a
   minimised crasher is committed as text, never as bytes.
   <replay> --write DIR writes each seed as DIR/<name>.exe instead, for the
   libFuzzer corpus. SCHWAY_REPLAY_NAME prefixes the diagnostics. */
#ifndef SCHWAYSTATION_FUZZ_REPLAY_MAIN_H
#define SCHWAYSTATION_FUZZ_REPLAY_MAIN_H

#ifdef SCHWAY_FUZZ_REPLAY
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../test_open.h"

#include "seeds.h"

#ifndef SCHWAY_REPLAY_NAME
#define SCHWAY_REPLAY_NAME "fuzz.regress"
#endif

/* The most a file may hold: a hex text takes two characters a byte. */
#define REPLAY_FILE_MAX (2ul * SEEDS_SIZE_MAX + 4096ul)

static int hex_digit(int c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

/* Decodes hex text in place; returns the byte count, or -1 on a bad
   character or an odd digit count. */
static long hex_decode(uint8_t *text, size_t length)
{
    size_t out = 0;
    int high = -1;
    for (size_t i = 0; i < length; i++) {
        int c = text[i];
        if (c == '#') {
            while (i < length && text[i] != '\n') {
                i++;
            }
            continue;
        }
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            continue;
        }
        int d = hex_digit(c);
        if (d < 0) {
            return -1;
        }
        if (high < 0) {
            high = d;
        } else {
            text[out++] = (uint8_t)((high << 4) | d);
            high = -1;
        }
    }
    return high < 0 ? (long)out : -1;
}

static int replay_file(const char *path)
{
    FILE *file = schway__test_open_file(path, "rb");
    long length;
    uint8_t *bytes;
    size_t n = strlen(path);
    if (file == NULL) {
        fprintf(stderr, SCHWAY_REPLAY_NAME ": cannot open corpus input: %s\n", path);
        return 0;
    }
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0 ||
        fseek(file, 0, SEEK_SET) != 0 || (unsigned long)length > REPLAY_FILE_MAX) {
        fprintf(stderr, SCHWAY_REPLAY_NAME ": invalid corpus input: %s\n", path);
        (void)fclose(file);
        return 0;
    }
    bytes = (uint8_t *)malloc(length == 0 ? 1u : (size_t)length);
    if (bytes == NULL) {
        fprintf(stderr, SCHWAY_REPLAY_NAME ": out of memory reading input: %s\n", path);
        (void)fclose(file);
        return 0;
    }
    if ((size_t)length != 0u && fread(bytes, 1u, (size_t)length, file) != (size_t)length) {
        fprintf(stderr, SCHWAY_REPLAY_NAME ": short read: %s\n", path);
        free(bytes);
        (void)fclose(file);
        return 0;
    }
    if (fclose(file) != 0) {
        fprintf(stderr, SCHWAY_REPLAY_NAME ": close failed: %s\n", path);
        free(bytes);
        return 0;
    }
    if (n > 4u && strcmp(path + n - 4u, ".hex") == 0) {
        length = hex_decode(bytes, (size_t)length);
        if (length < 0) {
            fprintf(stderr, SCHWAY_REPLAY_NAME ": not hexadecimal text: %s\n", path);
            free(bytes);
            return 0;
        }
    }
    (void)LLVMFuzzerTestOneInput(bytes, (size_t)length);
    free(bytes);
    return 1;
}

/* Runs every in-memory seed, or with dir writes each as a file; returns the
   number handled, 0 on failure. */
static uint32_t replay_seeds(const char *dir)
{
    uint8_t *buf = (uint8_t *)malloc(SEEDS_SIZE_MAX);
    if (buf == NULL) {
        fputs(SCHWAY_REPLAY_NAME ": out of memory building seeds\n", stderr);
        return 0;
    }
    uint32_t i = 0;
    for (; i < seeds_count(); i++) {
        size_t n = seeds_build(i, buf);
        if (dir == NULL) {
            (void)LLVMFuzzerTestOneInput(buf, n);
            continue;
        }
        char path[1024];
        int len = snprintf(path, sizeof path, "%s/%s.exe", dir, seeds_name(i));
        FILE *file = len > 0 && (size_t)len < sizeof path ? schway__test_open_file(path, "wb") : NULL;
        if (file == NULL || fwrite(buf, 1u, n, file) != n || fclose(file) != 0) {
            fprintf(stderr, SCHWAY_REPLAY_NAME ": cannot write seed %s/%s.exe\n", dir,
                    seeds_name(i));
            free(buf);
            return 0;
        }
    }
    free(buf);
    return i;
}

int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "--write") == 0) {
        uint32_t wrote = replay_seeds(argv[2]);
        if (wrote != 0u) {
            printf(SCHWAY_REPLAY_NAME ": wrote %lu seeds\n", (unsigned long)wrote);
        }
        return wrote == 0u ? 1 : 0;
    }
    unsigned long inputs = replay_seeds(NULL);
    if (inputs == 0u) {
        return 1;
    }
    for (int i = 1; i < argc; ++i) {
        if (!replay_file(argv[i])) {
            return 1;
        }
        inputs++;
    }
    printf(SCHWAY_REPLAY_NAME ": %lu inputs\n", inputs);
    return 0;
}
#endif

#endif
