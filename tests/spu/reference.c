/* Pinned SPU transfer log validation and the first-upload guest runner. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "sha256.h"
#include "../check.h"

#ifndef SCHWAY_SPU_REFERENCE_EXE_SHA256
#error "CMake must bind the pinned SPU executable digest"
#endif
#ifndef SCHWAY_SPU_REFERENCE_LOG_SHA256
#error "CMake must bind the pinned SPU log digest"
#endif

enum {
    MEMORY_TRANSFER_CASE_COUNT = 7u,
    FIRST_UPLOAD_CASE_COUNT = 4u,
    REFERENCE_LOG_MAX_BYTES = 4096u,
    REFERENCE_EXE_MAX_BYTES = 1024u * 1024u,
    FIRST_UPLOAD_TTY_MAX_BYTES = 8192u,
    FIRST_UPLOAD_FRAME_LIMIT = 600u
};

static const char *const memory_transfer_cases[MEMORY_TRANSFER_CASE_COUNT] = {
    "testDtcRegister",
    "testManualWriteToSpuRam",
    "testDMAReadFromSpuRam",
    "testDMAWriteToSpuRam",
    "testDMAWriteTiming",
    "testDMAReadTiming",
    "testControlBitsAreCopiedToStatusRegister",
};

static const char pinned_memory_transfer_log[] =
    "spu/memory-transfer\n"
    "Test SPU memory access using DMA and regular IO.\n"
    "\n"
    "pass - testDtcRegister\n"
    "pass - testManualWriteToSpuRam\n"
    "pass - testDMAReadFromSpuRam\n"
    "pass - testDMAWriteToSpuRam\n"
    "pass - testDMAWriteTiming\n"
    "pass - testDMAReadTiming\n"
    "pass - testControlBitsAreCopiedToStatusRegister\n"
    "Done.\n";

static uint8_t *read_bounded_file(const char *path, size_t limit, size_t *size_out)
{
    *size_out = 0u;
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    long end = ftell(file);
    if (end <= 0 || (unsigned long)end > limit || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    size_t size = (size_t)end;
    uint8_t *bytes = (uint8_t *)malloc(size);
    if (bytes == NULL) {
        fclose(file);
        return NULL;
    }
    size_t read_size = fread(bytes, 1u, size, file);
    int close_result = fclose(file);
    if (read_size != size || close_result != 0) {
        free(bytes);
        return NULL;
    }
    *size_out = size;
    return bytes;
}

static int digest_matches(const uint8_t *bytes, size_t size, const char *expected)
{
    schway__sha256 hash;
    uint8_t digest[32];
    char hex[65];
    schway__sha256_init(&hash);
    schway__sha256_update(&hash, bytes, size);
    schway__sha256_final(&hash, digest);
    schway__sha256_hex(digest, hex);
    return strcmp(hex, expected) == 0;
}

static int line_equals(const uint8_t *line, size_t length, const char *expected)
{
    size_t expected_length = strlen(expected);
    return length == expected_length && memcmp(line, expected, length) == 0;
}

static void remove_tty_prefix(const uint8_t **line, size_t *length)
{
    while (*length >= 4u && memcmp(*line, "tty ", 4u) == 0) {
        *line += 4u;
        *length -= 4u;
    }
}

/* The exact case names and order come from JaCzekanski/ps1-tests at f727802fead11f1daa7549285548392ef87749cb.
   Only that hardware-log shape can pass; repeatability or PCM bytes do not. */
static int memory_transfer_log_matches(const uint8_t *bytes, size_t size, int exit_code)
{
    if (exit_code != 0 || bytes == NULL || size == 0u || size > REFERENCE_LOG_MAX_BYTES) {
        return 0;
    }

    size_t at = 0u;
    size_t matched = 0u;
    uint32_t headers = 0u;
    int done = 0;
    while (at < size) {
        size_t end = at;
        while (end < size && bytes[end] != '\n') {
            end++;
        }
        size_t length = end - at;
        const uint8_t *line = bytes + at;
        at = end < size ? end + 1u : size;
        if (length > 0u && line[length - 1u] == '\r') {
            length--;
        }
        remove_tty_prefix(&line, &length);
        if (length == 0u) {
            continue;
        }
        if (done != 0) {
            return 0;
        }
        if (headers == 0u) {
            if (!line_equals(line, length, "spu/memory-transfer")) {
                return 0;
            }
            headers++;
            continue;
        }
        if (headers == 1u) {
            if (!line_equals(line, length, "Test SPU memory access using DMA and regular IO.")) {
                return 0;
            }
            headers++;
            continue;
        }
        if (matched < MEMORY_TRANSFER_CASE_COUNT) {
            const char *expected = memory_transfer_cases[matched];
            size_t expected_length = strlen(expected);
            if (length != 7u + expected_length || memcmp(line, "pass - ", 7u) != 0 ||
                memcmp(line + 7u, expected, expected_length) != 0) {
                return 0;
            }
            matched++;
            continue;
        }
        if (!line_equals(line, length, "Done.")) {
            return 0;
        }
        done = 1;
    }
    return headers == 2u && matched == MEMORY_TRANSFER_CASE_COUNT && done != 0;
}

enum control_fault {
    CONTROL_VALID,
    CONTROL_MISSING,
    CONTROL_DUPLICATE,
    CONTROL_REORDERED,
    CONTROL_FAILED
};

static int append_bytes(char *buffer, size_t capacity, size_t *used,
                        const char *bytes, size_t length)
{
    if (*used > capacity || length > capacity - *used) {
        return 0;
    }
    memcpy(buffer + *used, bytes, length);
    *used += length;
    return 1;
}

static int append_control_line(char *buffer, size_t capacity, size_t *used,
                               const char *line)
{
    size_t length = strlen(line);
    return append_bytes(buffer, capacity, used, line, length) &&
           append_bytes(buffer, capacity, used, "\n", 1u);
}

static size_t make_control_log(char *buffer, size_t capacity, enum control_fault fault)
{
    size_t used = 0u;
    if (!append_control_line(buffer, capacity, &used, "spu/memory-transfer") ||
        !append_control_line(buffer, capacity, &used, "Test SPU memory access using DMA and regular IO.") ||
        !append_control_line(buffer, capacity, &used, "")) {
        return 0u;
    }
    for (size_t i = 0u; i < MEMORY_TRANSFER_CASE_COUNT; i++) {
        if (fault == CONTROL_MISSING && i + 1u == MEMORY_TRANSFER_CASE_COUNT) {
            continue;
        }
        size_t index = i;
        if (fault == CONTROL_DUPLICATE && i + 1u == MEMORY_TRANSFER_CASE_COUNT) {
            index--;
        } else if (fault == CONTROL_REORDERED && i < 2u) {
            index = 1u - i;
        }
        if (fault == CONTROL_FAILED && i == 1u &&
            !append_control_line(buffer, capacity, &used, "fail - testManualWriteToSpuRam")) {
            return 0u;
        }
        char line[96];
        int count = snprintf(line, sizeof line, "pass - %s", memory_transfer_cases[index]);
        if (count < 0 || (size_t)count >= sizeof line ||
            !append_control_line(buffer, capacity, &used, line)) {
            return 0u;
        }
    }
    if (!append_control_line(buffer, capacity, &used, "Done.")) {
        return 0u;
    }
    return used;
}

static int run_predicate_controls(void)
{
    char buffer[REFERENCE_LOG_MAX_BYTES];
    CHECK(memory_transfer_log_matches((const uint8_t *)pinned_memory_transfer_log,
                                      sizeof pinned_memory_transfer_log - 1u, 0));
    CHECK(!memory_transfer_log_matches(NULL, 0u, 0));

    const enum control_fault rejected[] = {
        CONTROL_MISSING,
        CONTROL_DUPLICATE,
        CONTROL_REORDERED,
        CONTROL_FAILED,
    };
    for (size_t i = 0u; i < sizeof rejected / sizeof rejected[0]; i++) {
        size_t size = make_control_log(buffer, sizeof buffer, rejected[i]);
        CHECK(size != 0u && !memory_transfer_log_matches((const uint8_t *)buffer, size, 0));
    }
    static const uint8_t pcm_like[] = {0x00u, 0x80u, 0x00u, 0x80u};
    CHECK(!memory_transfer_log_matches(pcm_like, sizeof pcm_like, 0));
    CHECK_DONE();
}

static int run_pinned_log(const char *path)
{
    size_t size = 0u;
    uint8_t *bytes = read_bounded_file(path, REFERENCE_LOG_MAX_BYTES, &size);
    if (bytes == NULL) {
        fprintf(stderr, "SPU_REFERENCE_INPUT_ERROR: cannot read bounded memory-transfer log\n");
        return 2;
    }
    if (!digest_matches(bytes, size, SCHWAY_SPU_REFERENCE_LOG_SHA256)) {
        fprintf(stderr, "SPU_REFERENCE_IDENTITY_ERROR: memory-transfer log digest does not match pins.txt\n");
        free(bytes);
        return 2;
    }
    if (!memory_transfer_log_matches(bytes, size, 0)) {
        fprintf(stderr, "SPU_REFERENCE_LOG_ERROR: pinned memory-transfer log is incomplete or malformed\n");
        free(bytes);
        return 1;
    }
    free(bytes);
    puts("SPU_REFERENCE_LOG_MATCH: case=memory-transfer cases=7 ordered=1 completion=Done");
    return 0;
}

static int first_upload_progress(const uint8_t *bytes, size_t size)
{
    size_t at = 0u;
    size_t matched = 0u;
    while (at < size) {
        size_t end = at;
        while (end < size && bytes[end] != '\n') {
            end++;
        }
        if (end == size) {
            break;
        }
        const uint8_t *line = bytes + at;
        size_t length = end - at;
        at = end + 1u;
        if (length > 0u && line[length - 1u] == '\r') {
            length--;
        }
        remove_tty_prefix(&line, &length);
        if (length >= 7u && memcmp(line, "fail - ", 7u) == 0) {
            return -1;
        }
        if (length < 7u || memcmp(line, "pass - ", 7u) != 0) {
            continue;
        }
        if (matched >= FIRST_UPLOAD_CASE_COUNT) {
            return 1;
        }
        const char *expected = memory_transfer_cases[matched];
        if (!line_equals(line + 7u, length - 7u, expected)) {
            return -1;
        }
        matched++;
    }
    return matched == FIRST_UPLOAD_CASE_COUNT ? 1 : 0;
}

static int run_first_upload(const char *assets)
{
    char exe_path[1024];
    int path_length = snprintf(exe_path, sizeof exe_path,
                               "%s/spu/memory-transfer/memory-transfer.exe", assets);
    if (path_length < 0 || (size_t)path_length >= sizeof exe_path) {
        fprintf(stderr, "SPU_REFERENCE_INPUT_ERROR: asset path is too long\n");
        return 2;
    }
    size_t exe_size = 0u;
    uint8_t *exe = read_bounded_file(exe_path, REFERENCE_EXE_MAX_BYTES, &exe_size);
    if (exe == NULL || !digest_matches(exe, exe_size, SCHWAY_SPU_REFERENCE_EXE_SHA256)) {
        fprintf(stderr, "SPU_REFERENCE_IDENTITY_ERROR: memory-transfer executable is missing or changed\n");
        free(exe);
        return 2;
    }

    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    schway *inst = NULL;
    schway_status status = schway_create(&config, &inst);
    if (status != SCHWAY_STATUS_OK || inst == NULL) {
        fprintf(stderr, "SPU_REFERENCE_CORE_ERROR: schway_create returned %s\n",
                schway_status_string(status));
        free(exe);
        return 2;
    }

    schway_profile profile;
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = SCHWAY_REGION_NTSC_U;
    profile.flags = SCHWAY_PROFILE_DEBUG_PORT;
    status = schway_set_profile(inst, &profile);
    if (status == SCHWAY_STATUS_OK) {
        status = schway_load_exe(inst, exe, exe_size);
    }
    free(exe);
    if (status != SCHWAY_STATUS_OK) {
        fprintf(stderr, "SPU_REFERENCE_CORE_ERROR: guest load returned %s\n",
                schway_status_string(status));
        schway_destroy(inst);
        return 2;
    }

    uint8_t tty_text[FIRST_UPLOAD_TTY_MAX_BYTES];
    size_t tty_size = 0u;
    for (uint32_t frame = 0u; frame < FIRST_UPLOAD_FRAME_LIMIT; frame++) {
        schway_run_result result;
        memset(&result, 0, sizeof result);
        result.size = (uint32_t)sizeof result;
        status = schway_run_frame(inst, &result);
        if (status != SCHWAY_STATUS_OK || result.fault_present != 0u ||
            (result.stop != SCHWAY_STOP_FRAME && result.stop != SCHWAY_STOP_BUDGET)) {
            fprintf(stderr, "SPU_REFERENCE_CORE_ERROR: bounded guest run stopped unexpectedly\n");
            schway_destroy(inst);
            return 1;
        }

        uint8_t drained[1024];
        schway_tty tty;
        memset(&tty, 0, sizeof tty);
        tty.size = (uint32_t)sizeof tty;
        tty.bytes = drained;
        tty.capacity = (uint32_t)sizeof drained;
        status = schway_get_tty(inst, &tty);
        if (status != SCHWAY_STATUS_OK || tty.dropped != 0u ||
            tty.written > sizeof drained || tty.written > sizeof tty_text - tty_size) {
            fprintf(stderr, "SPU_REFERENCE_CORE_ERROR: TTY output overflowed its bounded capture\n");
            schway_destroy(inst);
            return 1;
        }
        memcpy(tty_text + tty_size, drained, tty.written);
        tty_size += tty.written;
        int progress = first_upload_progress(tty_text, tty_size);
        if (progress > 0) {
            schway_destroy(inst);
            puts("SPU_REFERENCE_FIRST_UPLOAD: accepted four manual/DMA transfer cases; full seven-case log not evaluated");
            return 0;
        }
        if (progress < 0) {
            fprintf(stderr, "SPU_REFERENCE_GUEST_ERROR: first manual/DMA transfer cases are missing or failing\n");
            schway_destroy(inst);
            return 1;
        }
    }
    fprintf(stderr, "SPU_REFERENCE_GUEST_ERROR: first manual/DMA transfer cases did not complete within %u frames\n",
            (unsigned)FIRST_UPLOAD_FRAME_LIMIT);
    schway_destroy(inst);
    return 1;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "predicate_controls") == 0) {
        return run_predicate_controls();
    }
    if (argc == 3 && strcmp(argv[1], "log") == 0) {
        return run_pinned_log(argv[2]);
    }
    if (argc == 5 && strcmp(argv[1], "memory-transfer") == 0 &&
        strcmp(argv[2], "--first-upload") == 0 && strcmp(argv[3], "--assets") == 0) {
        return run_first_upload(argv[4]);
    }
    fprintf(stderr,
            "usage: schwaystation-test-spu-reference predicate_controls|log PSX.LOG|memory-transfer --first-upload --assets DIR\n");
    return 2;
}
