/* Pinned MDEC block and status guests against the production caller path. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "sha256.h"
#include "internal.h"

#ifndef SCHWAY_MDEC_4BIT_EXE_SHA256
#error "CMake must bind the pinned 4-bit guest digest"
#endif
#ifndef SCHWAY_MDEC_8BIT_EXE_SHA256
#error "CMake must bind the pinned 8-bit guest digest"
#endif
#ifndef SCHWAY_MDEC_STATUS_EXE_SHA256
#error "CMake must bind the pinned status guest digest"
#endif
#ifndef SCHWAY_MDEC_4BIT_LOG_SHA256
#error "CMake must bind the pinned 4-bit output log digest"
#endif
#ifndef SCHWAY_MDEC_8BIT_LOG_SHA256
#error "CMake must bind the pinned 8-bit output log digest"
#endif
#ifndef SCHWAY_MDEC_STATUS_LOG_SHA256
#error "CMake must bind the pinned status log digest"
#endif

enum {
    REFERENCE_LOG_LIMIT = 150000u,
    REFERENCE_EXE_LIMIT = 1024u * 1024u,
    GUEST_TTY_LIMIT = 256u * 1024u,
    GUEST_RUN_LIMIT = 19200u,
    GUEST_RUN_CYCLES = 256u,
    BLOCK_ROWS = 8u,
    RAM_SIZE = 2u * 1024u * 1024u
};

typedef struct status_events {
    uint8_t values[4096];
    size_t count;
    size_t status_count;
    size_t input_count;
    size_t output_count;
} status_events;

typedef struct dma_trace_write {
    uint64_t cycle;
    uint32_t pc;
    uint32_t addr;
    uint32_t value;
} dma_trace_write;

typedef struct mdec_guest_snapshot {
    uint32_t valid;
    uint32_t command;
    uint32_t depth;
    uint32_t signed_output;
    uint32_t output_count;
    uint32_t dma0_started;
    uint32_t dma0_madr;
    uint32_t dma0_bcr;
    uint32_t dma0_chcr;
    uint32_t dma0_word_count;
    uint32_t dma0_words[32];
    uint32_t dma1_started;
    uint32_t dma1_madr;
    uint32_t dma1_bcr;
    uint32_t dma1_chcr;
    uint32_t dma1_write_count;
    dma_trace_write dma1_writes[32];
    uint32_t trace_head_seen;
    uint32_t trace_count_seen;
    uint32_t trace_initialized;
    uint32_t last_dma0_madr;
    uint32_t last_dma0_bcr;
    uint32_t last_dma_madr;
    uint32_t last_dma_bcr;
    uint8_t quant[SCHWAY__MDEC_TABLE_BYTES];
    int16_t scale[SCHWAY__MDEC_BLOCK_SAMPLES];
    int16_t block[SCHWAY__MDEC_BLOCK_SAMPLES];
    uint32_t output[SCHWAY__MDEC_OUTPUT_WORDS];
} mdec_guest_snapshot;

static void capture_mdec_snapshot(mdec_guest_snapshot *snapshot, const schway *inst)
{
    snapshot->valid = 1u;
    snapshot->command = inst->mdec.command;
    snapshot->depth = inst->mdec.depth;
    snapshot->signed_output = inst->mdec.signed_output;
    snapshot->output_count = inst->mdec.output_count;
    memcpy(snapshot->quant, inst->mdec.quant, sizeof snapshot->quant);
    memcpy(snapshot->scale, inst->mdec.scale, sizeof snapshot->scale);
    memcpy(snapshot->block, inst->mdec.block, sizeof snapshot->block);
    for (uint32_t i = 0u; i < inst->mdec.output_count; i++) {
        snapshot->output[i] = inst->mdec.output[(inst->mdec.output_head + i) %
                                                 SCHWAY__MDEC_OUTPUT_WORDS];
    }
}

static void capture_dma1_start(mdec_guest_snapshot *snapshot, const schway *inst)
{
    const struct schway__trace_ring *trace = &inst->trace;
    uint32_t scan_start = 0u;
    uint32_t scan_count = trace->count;
    if (snapshot->trace_initialized != 0u && trace->count < SCHWAY__TRACE_CAP &&
        trace->head == snapshot->trace_head_seen && trace->count >= snapshot->trace_count_seen) {
        scan_start = snapshot->trace_count_seen;
        scan_count = trace->count - scan_start;
    } else if (snapshot->trace_initialized != 0u && trace->count == SCHWAY__TRACE_CAP &&
               snapshot->trace_count_seen == SCHWAY__TRACE_CAP) {
        uint32_t added = (trace->head + SCHWAY__TRACE_CAP - snapshot->trace_head_seen) %
                         SCHWAY__TRACE_CAP;
        scan_start = SCHWAY__TRACE_CAP - added;
        scan_count = added;
    } else if (snapshot->trace_initialized != 0u && trace->count == SCHWAY__TRACE_CAP &&
               snapshot->trace_count_seen < SCHWAY__TRACE_CAP &&
               trace->count >= snapshot->trace_count_seen) {
        uint32_t overwritten =
            (trace->head + SCHWAY__TRACE_CAP - snapshot->trace_head_seen) % SCHWAY__TRACE_CAP;
        scan_count = SCHWAY__TRACE_CAP - snapshot->trace_count_seen + overwritten;
        if (scan_count > SCHWAY__TRACE_CAP) scan_count = SCHWAY__TRACE_CAP;
        scan_start = SCHWAY__TRACE_CAP - scan_count;
    }
    for (uint32_t i = scan_start; i < scan_start + scan_count; i++) {
        const struct schway__trace_rec *rec =
            &trace->rec[(trace->head + i) % SCHWAY__TRACE_CAP];
        if (SCHWAY__TRACE_EVENT(rec->meta) != SCHWAY_TRACE_WR) continue;
        if (rec->addr >= 0x1F801090u && rec->addr <= 0x1F80109Bu &&
            snapshot->dma1_write_count < 32u) {
            dma_trace_write *write = &snapshot->dma1_writes[snapshot->dma1_write_count++];
            write->cycle = rec->cycle;
            write->pc = rec->pc;
            write->addr = rec->addr;
            write->value = rec->value;
        }
        if (rec->addr == 0x1F801080u) {
            snapshot->last_dma0_madr = rec->value & SCHWAY__DMA_MADR_MASK;
        } else if (rec->addr == 0x1F801084u) {
            snapshot->last_dma0_bcr = rec->value;
        } else if (rec->addr == 0x1F801088u && (rec->value & 0x01000000u) != 0u &&
                   snapshot->dma0_started == 0u) {
            snapshot->dma0_started = 1u;
            snapshot->dma0_madr = snapshot->last_dma0_madr;
            snapshot->dma0_bcr = snapshot->last_dma0_bcr;
            snapshot->dma0_chcr = rec->value;
            uint32_t words = snapshot->dma0_bcr & 0xFFFFu;
            if (words <= 32u && snapshot->dma0_madr <= RAM_SIZE - words * 4u) {
                snapshot->dma0_word_count = words;
                for (uint32_t word = 0u; word < words; word++) {
                    uint32_t at = snapshot->dma0_madr + word * 4u;
                    snapshot->dma0_words[word] = (uint32_t)inst->bus.ram[at] |
                        ((uint32_t)inst->bus.ram[at + 1u] << 8) |
                        ((uint32_t)inst->bus.ram[at + 2u] << 16) |
                        ((uint32_t)inst->bus.ram[at + 3u] << 24);
                }
            }
        } else if (rec->addr == 0x1F801090u) {
            snapshot->last_dma_madr = rec->value & SCHWAY__DMA_MADR_MASK;
        } else if (rec->addr == 0x1F801094u) {
            snapshot->last_dma_bcr = rec->value;
        } else if (rec->addr == 0x1F801098u && (rec->value & 0x01000000u) != 0u &&
                   snapshot->dma1_started == 0u) {
            snapshot->dma1_started = 1u;
            snapshot->dma1_madr = snapshot->last_dma_madr;
            snapshot->dma1_bcr = snapshot->last_dma_bcr;
            snapshot->dma1_chcr = rec->value;
        }
    }
    snapshot->trace_head_seen = trace->head;
    snapshot->trace_count_seen = trace->count;
    snapshot->trace_initialized = 1u;
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

static uint8_t *read_bounded(const char *path, size_t limit, size_t *size_out)
{
    *size_out = 0u;
    FILE *file = fopen(path, "rb");
    if (file == NULL || fseek(file, 0, SEEK_END) != 0) {
        if (file != NULL) fclose(file);
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

static const uint8_t *find_bytes(const uint8_t *bytes, size_t size,
                                 const char *needle, size_t needle_size)
{
    if (needle_size > size) return NULL;
    for (size_t i = 0u; i <= size - needle_size; i++) {
        if (memcmp(bytes + i, needle, needle_size) == 0) return bytes + i;
    }
    return NULL;
}

static const uint8_t *next_line(const uint8_t *bytes, size_t size, size_t *at,
                                size_t *line_size)
{
    if (*at >= size) return NULL;
    size_t start = *at;
    size_t end = start;
    while (end < size && bytes[end] != '\n') end++;
    *line_size = end - start;
    if (*line_size != 0u && bytes[start + *line_size - 1u] == '\r') (*line_size)--;
    *at = end < size ? end + 1u : size;
    return bytes + start;
}

static void strip_tty_prefix(const uint8_t **line, size_t *length)
{
    while (*length >= 4u && memcmp(*line, "tty ", 4u) == 0) {
        *line += 4u;
        *length -= 4u;
    }
}

static int line_starts(const uint8_t *line, size_t length, const char *prefix)
{
    size_t prefix_size = strlen(prefix);
    return length >= prefix_size && memcmp(line, prefix, prefix_size) == 0;
}

static int run_guest(const uint8_t *exe, size_t exe_size,
                     uint8_t **tty_out, size_t *tty_size_out, schway **inst_out,
                     mdec_guest_snapshot *snapshot_out)
{
    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = reference_alloc;
    config.allocator.free = reference_free;
    schway *inst = NULL;
    if (schway_create(&config, &inst) != SCHWAY_STATUS_OK || inst == NULL) {
        schway_destroy(inst);
        return 0;
    }
    schway_profile profile;
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = SCHWAY_REGION_NTSC_U;
    profile.flags = SCHWAY_PROFILE_DEBUG_PORT;
    if (schway_set_profile(inst, &profile) != SCHWAY_STATUS_OK ||
        schway_load_exe(inst, exe, exe_size) != SCHWAY_STATUS_OK) {
        schway_destroy(inst);
        return 0;
    }

    uint8_t *tty_text = (uint8_t *)calloc(GUEST_TTY_LIMIT, 1u);
    if (tty_text == NULL) {
        schway_destroy(inst);
        return 0;
    }
    size_t tty_size = 0u;
    int complete = 0;
    if (snapshot_out != NULL) memset(snapshot_out, 0, sizeof *snapshot_out);
    schway_budget budget;
    memset(&budget, 0, sizeof budget);
    budget.size = (uint32_t)sizeof budget;
    budget.max_cycles = GUEST_RUN_CYCLES;
    budget.flags = SCHWAY_RUN_STOP_ON_FAULT | SCHWAY_RUN_TRACE;
    for (uint32_t step = 0u; step < GUEST_RUN_LIMIT; step++) {
        schway_run_result result;
        memset(&result, 0, sizeof result);
        result.size = (uint32_t)sizeof result;
        schway_status status = schway_run(inst, &budget, &result);
        if (snapshot_out != NULL) capture_dma1_start(snapshot_out, inst);
        if (snapshot_out != NULL &&
            (inst->mdec.block_pending != 0u || inst->mdec.output_count != 0u)) {
            capture_mdec_snapshot(snapshot_out, inst);
        }
        if (status != SCHWAY_STATUS_OK || result.fault_present != 0u ||
            (result.stop != SCHWAY_STOP_FRAME && result.stop != SCHWAY_STOP_BUDGET)) {
            fprintf(stderr, "MDEC_REFERENCE_GUEST_ERROR: run stopped status=%s stop=%u fault=%u\n",
                    schway_status_string(status), result.stop, result.fault_present);
            break;
        }
        uint8_t drained[4096];
        schway_tty tty;
        memset(&tty, 0, sizeof tty);
        tty.size = (uint32_t)sizeof tty;
        tty.bytes = drained;
        tty.capacity = (uint32_t)sizeof drained;
        status = schway_get_tty(inst, &tty);
        if (status != SCHWAY_STATUS_OK || tty.dropped != 0u ||
            tty.written > sizeof drained || tty.written > GUEST_TTY_LIMIT - tty_size) {
            fprintf(stderr, "MDEC_REFERENCE_GUEST_ERROR: TTY drain failed or exceeded its bound\n");
            break;
        }
        memcpy(tty_text + tty_size, drained, tty.written);
        tty_size += tty.written;
        if (find_bytes(tty_text, tty_size, "Done", 4u) != NULL) {
            complete = 1;
            break;
        }
    }
    if (complete == 0) {
        size_t status_lines = 0u;
        size_t input_lines = 0u;
        size_t output_lines = 0u;
        size_t tty_at = 0u;
        size_t line_size = 0u;
        const uint8_t *line = NULL;
        while ((line = next_line(tty_text, tty_size, &tty_at, &line_size)) != NULL) {
            strip_tty_prefix(&line, &line_size);
            if (line_starts(line, line_size, "==== MDEC_STATUS:")) status_lines++;
            else if (line_starts(line, line_size, "MDEC_DATA <-")) input_lines++;
            else if (line_starts(line, line_size, "MDEC_DATA ->")) output_lines++;
        }
        fprintf(stderr, "MDEC_REFERENCE_GUEST_ERROR: no Done marker within %u x %u-cycle run steps (tty-bytes=%zu)\n",
                (unsigned)GUEST_RUN_LIMIT, (unsigned)GUEST_RUN_CYCLES, tty_size);
        fprintf(stderr,
                "MDEC_REFERENCE_GUEST_EVENTS: status=%zu input=%zu output=%zu\n",
                status_lines, input_lines, output_lines);
        fprintf(stderr,
                "MDEC_REFERENCE_GUEST_STATE: pc=%08x ticks=%llu mdec=%08x active=%u pending=%u deadline=%llu in=%u rem=%u out=%u dma0=%08x/%08x/%08x dma1=%08x/%08x/%08x dreq=%02x\n",
                inst->cpu.pc, (unsigned long long)inst->ticks, inst->mdec.command,
                inst->mdec.command_active, inst->mdec.block_pending,
                (unsigned long long)inst->mdec.block_deadline, inst->mdec.input_count,
                inst->mdec.words_remaining, inst->mdec.output_count,
                inst->dma.chcr[0], inst->dma.madr[0], inst->dma.bcr[0],
                inst->dma.chcr[1], inst->dma.madr[1], inst->dma.bcr[1], inst->dma.dreq);
        uint32_t pc_word = 0u;
        uint32_t pc_word_available = 0u;
        if (schway__bus_device(inst->cpu.pc) == SCHWAY__DEV_RAM) {
            uint32_t pc_phys = inst->cpu.pc & 0x1FFFFFFFu;
            if (pc_phys <= RAM_SIZE - sizeof pc_word) {
                pc_word = (uint32_t)inst->bus.ram[pc_phys] |
                          ((uint32_t)inst->bus.ram[pc_phys + 1u] << 8) |
                          ((uint32_t)inst->bus.ram[pc_phys + 2u] << 16) |
                          ((uint32_t)inst->bus.ram[pc_phys + 3u] << 24);
                pc_word_available = 1u;
            }
        }
        if (pc_word_available != 0u) {
            fprintf(stderr,
                    "MDEC_REFERENCE_CPU_PATH: a0=%08x v0=%08x ra=%08x insn=%08x\n",
                    inst->cpu.r[4], inst->cpu.r[2], inst->cpu.r[31], pc_word);
        } else {
            fprintf(stderr,
                    "MDEC_REFERENCE_CPU_PATH: a0=%08x v0=%08x ra=%08x insn=unavailable\n",
                    inst->cpu.r[4], inst->cpu.r[2], inst->cpu.r[31]);
        }
        if ((inst->mdec.command == 0x20000020u || inst->mdec.command == 0x28000020u) &&
            inst->dma.madr[0] == 0x00012080u) {
            fprintf(stderr, "MDEC_REFERENCE_DMA0_SOURCE: command=%08x 00012000",
                    inst->mdec.command);
            for (uint32_t i = 0u; i < 32u; i++) {
                uint32_t at = 0x00012000u + i * 4u;
                uint32_t word = (uint32_t)inst->bus.ram[at] |
                                ((uint32_t)inst->bus.ram[at + 1u] << 8) |
                                ((uint32_t)inst->bus.ram[at + 2u] << 16) |
                                ((uint32_t)inst->bus.ram[at + 3u] << 24);
                fprintf(stderr, " %08x", word);
            }
            fputc('\n', stderr);
        }
        uint32_t last_status = UINT32_MAX;
        for (uint32_t i = 0u; i < inst->trace.count; i++) {
            const struct schway__trace_rec *rec =
                &inst->trace.rec[(inst->trace.head + i) % SCHWAY__TRACE_CAP];
            uint32_t is_status = rec->addr == 0x1F801824u && rec->value != last_status;
            if (rec->addr == 0x1F801824u) last_status = rec->value;
            if ((rec->addr >= 0x1F801820u && rec->addr <= 0x1F801823u) || is_status != 0u ||
                (rec->addr >= 0x1F801080u && rec->addr <= 0x1F80109Bu)) {
                fprintf(stderr, "MDEC_REFERENCE_BUS_TRACE: %llu %08x %u %08x %08x\n",
                        (unsigned long long)rec->cycle, rec->pc,
                        SCHWAY__TRACE_EVENT(rec->meta), rec->addr, rec->value);
            }
        }
        size_t tail_start = tty_size > 192u ? tty_size - 192u : 0u;
        fputs("MDEC_REFERENCE_TTY_TAIL: ", stderr);
        (void)fwrite(tty_text + tail_start, 1u, tty_size - tail_start, stderr);
        fputc('\n', stderr);
        free(tty_text);
        schway_destroy(inst);
        return 0;
    }
    *tty_out = tty_text;
    *tty_size_out = tty_size;
    *inst_out = inst;
    return 1;
}

static int parse_hex_row(const uint8_t *line, size_t line_size, uint32_t byte_count,
                         uint32_t *offset_out, uint8_t *bytes_out)
{
    char text[256];
    if (line_size == 0u || line_size >= sizeof text) return 0;
    memcpy(text, line, line_size);
    text[line_size] = '\0';
    char *at = text;
    while (*at == ' ' || *at == '\t') at++;
    char *end = NULL;
    unsigned long offset = strtoul(at, &end, 16);
    if (end == at || offset > UINT32_MAX) return 0;
    while (*end == ' ' || *end == '\t') end++;
    if (*end != ':') return 0;
    end++;
    *offset_out = (uint32_t)offset;
    for (uint32_t i = 0u; i < byte_count; i++) {
        while (*end == ' ' || *end == '\t') end++;
        char *value_end = NULL;
        unsigned long value = strtoul(end, &value_end, 16);
        if (value_end == end || (size_t)(value_end - end) != 2u || value > UINT8_MAX) return 0;
        bytes_out[i] = (uint8_t)value;
        end = value_end;
    }
    while (*end == ' ' || *end == '\t') end++;
    return *end == '\0';
}

static int parse_expected_block(const uint8_t *log, size_t log_size,
                                const char *mode, uint32_t row_bytes,
                                uint8_t *output, size_t output_size)
{
    char heading[64];
    int heading_size = snprintf(heading, sizeof heading, "8x8 block in %s mode:", mode);
    if (heading_size < 0 || (size_t)heading_size >= sizeof heading) return 0;
    const uint8_t *found = find_bytes(log, log_size, heading, (size_t)heading_size);
    if (found == NULL || output_size != (size_t)row_bytes * BLOCK_ROWS) return 0;
    size_t at = (size_t)(found - log) + (size_t)heading_size;
    size_t used = 0u;
    uint32_t row = 0u;
    while (at < log_size && row < BLOCK_ROWS) {
        size_t line_size = 0u;
        const uint8_t *line = next_line(log, log_size, &at, &line_size);
        if (line == NULL) break;
        uint32_t offset = 0u;
        uint8_t row_bytes_out[8];
        if (!parse_hex_row(line, line_size, row_bytes, &offset, row_bytes_out)) continue;
        if (offset != row * row_bytes || row_bytes > sizeof row_bytes_out ||
            row_bytes > output_size - used) return 0;
        memcpy(output + used, row_bytes_out, row_bytes);
        used += row_bytes;
        row++;
    }
    return row == BLOCK_ROWS && used == output_size;
}

static int parse_output_target(const uint8_t *log, size_t log_size, uint32_t *address_out)
{
    static const char marker[] = "mdec_readDecodedDma(addr=0x";
    const uint8_t *found = find_bytes(log, log_size, marker, sizeof marker - 1u);
    if (found == NULL) return 0;
    size_t offset = (size_t)(found - log);
    if (log_size - offset < sizeof marker - 1u + 8u) return 0;
    const uint8_t *digits = found + sizeof marker - 1u;
    uint32_t address = 0u;
    for (uint32_t i = 0u; i < 8u; i++) {
        uint8_t value = digits[i];
        uint32_t nibble;
        if (value >= '0' && value <= '9') nibble = (uint32_t)(value - '0');
        else if (value >= 'a' && value <= 'f') nibble = (uint32_t)(value - 'a') + 10u;
        else if (value >= 'A' && value <= 'F') nibble = (uint32_t)(value - 'A') + 10u;
        else return 0;
        address = (address << 4u) | nibble;
    }
    if (address < 0x80000000u || address > 0x9FFFFFFFu) return 0;
    *address_out = address & SCHWAY__DMA_MADR_MASK;
    return 1;
}

static int block_bytes_match(const uint8_t *actual, const uint8_t *expected, size_t size)
{
    return actual != NULL && expected != NULL && size != 0u && memcmp(actual, expected, size) == 0;
}

static uint32_t find_ram_bytes(const uint8_t *ram, size_t size, const uint8_t *bytes,
                               size_t byte_count)
{
    if (ram == NULL || bytes == NULL || byte_count == 0u || byte_count > size) return UINT32_MAX;
    for (size_t at = 0u; at <= size - byte_count; at++) {
        if (memcmp(ram + at, bytes, byte_count) == 0) return (uint32_t)at;
    }
    return UINT32_MAX;
}

static const char *exe_digest_for(const char *mode)
{
    if (strcmp(mode, "4bit") == 0) return SCHWAY_MDEC_4BIT_EXE_SHA256;
    if (strcmp(mode, "8bit") == 0) return SCHWAY_MDEC_8BIT_EXE_SHA256;
    return SCHWAY_MDEC_STATUS_EXE_SHA256;
}

static const char *log_digest_for(const char *mode)
{
    if (strcmp(mode, "4bit") == 0) return SCHWAY_MDEC_4BIT_LOG_SHA256;
    if (strcmp(mode, "8bit") == 0) return SCHWAY_MDEC_8BIT_LOG_SHA256;
    return SCHWAY_MDEC_STATUS_LOG_SHA256;
}

static int run_block_guest(const char *mode, const char *exe_path, const char *log_path)
{
    size_t exe_size = 0u;
    size_t log_size = 0u;
    uint8_t *exe = read_bounded(exe_path, REFERENCE_EXE_LIMIT, &exe_size);
    uint8_t *log = read_bounded(log_path, REFERENCE_LOG_LIMIT, &log_size);
    if (exe == NULL || log == NULL || !digest_matches(exe, exe_size, exe_digest_for(mode)) ||
        !digest_matches(log, log_size, log_digest_for(mode))) {
        fprintf(stderr, "MDEC_REFERENCE_IDENTITY_ERROR: %s guest or pinned output log is missing or changed\n",
                mode);
        free(exe);
        free(log);
        return 2;
    }
    uint8_t *tty = NULL;
    size_t tty_size = 0u;
    schway *inst = NULL;
    mdec_guest_snapshot snapshot;
    int guest_complete = run_guest(exe, exe_size, &tty, &tty_size, &inst, &snapshot);
    free(exe);
    size_t output_size = strcmp(mode, "4bit") == 0 ? 32u : 64u;
    uint32_t row_bytes = strcmp(mode, "4bit") == 0 ? 4u : 8u;
    uint32_t log_target_address = UINT32_MAX;
    uint8_t expected[64];
    int valid = guest_complete != 0 && parse_output_target(log, log_size, &log_target_address) &&
                find_bytes(tty, tty_size, strcmp(mode, "4bit") == 0 ? "mdec/4bit" : "mdec/8bit",
                           strcmp(mode, "4bit") == 0 ? sizeof "mdec/4bit" - 1u : sizeof "mdec/8bit" - 1u) != NULL &&
                parse_expected_block(log, log_size, mode, row_bytes, expected, output_size);
    if (valid != 0 && (log_target_address > RAM_SIZE ||
                       output_size > RAM_SIZE - log_target_address)) valid = 0;
    /* SCM.12's pinned main.cpp uses a stack buffer, and common/mdec.cpp writes
       its data pointer directly to DMA1 MADR. The log address is from the
       hardware run; compare bytes at this emulated run's captured DMA1 start. */
    uint32_t dma1_address = snapshot.dma1_madr;
    int dma1_target_valid = valid != 0 && snapshot.dma1_started != 0u &&
                            dma1_address <= RAM_SIZE && output_size <= RAM_SIZE - dma1_address;
    int output_matches = dma1_target_valid != 0 &&
                         block_bytes_match(inst->bus.ram + dma1_address, expected, output_size);
    if (valid != 0 && (!dma1_target_valid || !output_matches)) {
        if (!dma1_target_valid) {
            fprintf(stderr,
                    "MDEC_REFERENCE_DMA1_TARGET_ERROR: mode=%s log_target=%08x dma1_started=%u dma1_madr=%08x\n",
                    mode, log_target_address, snapshot.dma1_started, snapshot.dma1_madr);
        }
        if (dma1_target_valid != 0 && !output_matches) {
            const uint8_t *actual = inst->bus.ram + dma1_address;
            size_t mismatch = 0u;
            while (mismatch < output_size && actual[mismatch] == expected[mismatch]) mismatch++;
            fprintf(stderr,
                    "MDEC_REFERENCE_OUTPUT_MISMATCH: mode=%s byte=%zu expected=%02x actual=%02x\n",
                    mode, mismatch, (unsigned)expected[mismatch], (unsigned)actual[mismatch]);
            fputs("MDEC_REFERENCE_EXPECTED:", stderr);
            for (size_t i = 0u; i < output_size; i++) fprintf(stderr, " %02x", expected[i]);
            fputc('\n', stderr);
            fputs("MDEC_REFERENCE_ACTUAL:", stderr);
            for (size_t i = 0u; i < output_size; i++) fprintf(stderr, " %02x", actual[i]);
            fputc('\n', stderr);
        }
        fprintf(stderr,
                "MDEC_REFERENCE_FINAL_STATE: ticks=%llu command=%08x active=%u pending=%u in=%u rem=%u out=%u dreq=%02x dma1=%08x/%08x/%08x remain=%u phase=%u\n",
                (unsigned long long)inst->ticks, inst->mdec.command, inst->mdec.command_active,
                inst->mdec.block_pending, inst->mdec.input_count, inst->mdec.words_remaining,
                inst->mdec.output_count, inst->dma.dreq, inst->dma.chcr[1],
                inst->dma.madr[1], inst->dma.bcr[1], inst->dma.remain[1],
                inst->dma.phase[1]);
        fprintf(stderr, "MDEC_REFERENCE_SNAPSHOT: valid=%u command=%08x depth=%u signed=%u out=%u dma1_start=%u/%08x/%08x/%08x pinned_target=%08x\n",
                snapshot.valid, snapshot.command, snapshot.depth, snapshot.signed_output,
                snapshot.output_count, snapshot.dma1_started, snapshot.dma1_madr,
                snapshot.dma1_bcr, snapshot.dma1_chcr, log_target_address);
        fprintf(stderr, "MDEC_REFERENCE_DMA0_INPUT: started=%u madr=%08x bcr=%08x chcr=%08x words=%u",
                snapshot.dma0_started, snapshot.dma0_madr, snapshot.dma0_bcr,
                snapshot.dma0_chcr, snapshot.dma0_word_count);
        for (uint32_t i = 0u; i < snapshot.dma0_word_count; i++)
            fprintf(stderr, " %08x", snapshot.dma0_words[i]);
        fputc('\n', stderr);
        for (uint32_t i = 0u; i < snapshot.dma1_write_count; i++) {
            const dma_trace_write *write = &snapshot.dma1_writes[i];
            fprintf(stderr, "MDEC_REFERENCE_DMA1_REG: %llu pc=%08x addr=%08x value=%08x\n",
                    (unsigned long long)write->cycle, write->pc, write->addr, write->value);
        }
        fputs("MDEC_REFERENCE_FIFO:", stderr);
        for (uint32_t i = 0u; i < snapshot.output_count; i++)
            fprintf(stderr, " %08x", snapshot.output[i]);
        fputc('\n', stderr);
        uint8_t fifo_bytes[64] = {0};
        size_t fifo_size = snapshot.output_count * 4u;
        if (fifo_size > sizeof fifo_bytes) fifo_size = sizeof fifo_bytes;
        for (size_t i = 0u; i < fifo_size; i++)
            fifo_bytes[i] = (uint8_t)(snapshot.output[i / 4u] >> ((i % 4u) * 8u));
        fprintf(stderr, "MDEC_REFERENCE_EXPECTED_RAM: %08x FIFO_RAM: %08x\n",
                find_ram_bytes(inst->bus.ram, RAM_SIZE, expected, output_size),
                find_ram_bytes(inst->bus.ram, RAM_SIZE, fifo_bytes, fifo_size));
        uint32_t dma1_records = 0u;
        for (uint32_t i = 0u; i < inst->trace.count && dma1_records < 32u; i++) {
            const struct schway__trace_rec *rec =
                &inst->trace.rec[(inst->trace.head + i) % SCHWAY__TRACE_CAP];
            if (SCHWAY__TRACE_EVENT(rec->meta) == SCHWAY_TRACE_WR &&
                rec->addr >= 0x1F801090u && rec->addr <= 0x1F80109Bu) {
                fprintf(stderr, "MDEC_REFERENCE_DMA1_WRITE: %llu %08x %08x=%08x\n",
                        (unsigned long long)rec->cycle, rec->pc, rec->addr, rec->value);
                dma1_records++;
            }
        }
        fputs("MDEC_REFERENCE_BLOCK:", stderr);
        for (uint32_t i = 0u; i < SCHWAY__MDEC_BLOCK_SAMPLES; i++)
            fprintf(stderr, " %d", (int)snapshot.block[i]);
        fputc('\n', stderr);
        fputs("MDEC_REFERENCE_QUANT:", stderr);
        for (uint32_t i = 0u; i < SCHWAY__MDEC_BLOCK_SAMPLES; i++)
            fprintf(stderr, " %02x", snapshot.quant[i]);
        fputc('\n', stderr);
        fputs("MDEC_REFERENCE_SCALE:", stderr);
        for (uint32_t i = 0u; i < SCHWAY__MDEC_BLOCK_SAMPLES; i++)
            fprintf(stderr, " %04x", (uint16_t)snapshot.scale[i]);
        fputc('\n', stderr);
        valid = 0;
    }
    if (valid == 0 && guest_complete != 0) {
        fprintf(stderr, "MDEC_REFERENCE_BLOCK_ERROR: %s guest did not produce the pinned RAM bytes\n", mode);
        if (tty != NULL && find_bytes(tty, tty_size, "timeout", sizeof "timeout" - 1u) != NULL)
            fprintf(stderr, "MDEC_REFERENCE_GUEST_NOTE: guest TTY reports a DMA read timeout; its format placeholders do not expose values\n");
    }
    if (valid != 0) {
        printf("MDEC_REFERENCE_BLOCK_MATCH: mode=%s bytes=%zu log_target=%08x dma1_start=%08x guest=Done steps<=%u\n",
               mode, output_size, log_target_address, dma1_address, (unsigned)GUEST_RUN_LIMIT);
    }
    free(tty);
    free(log);
    schway_destroy(inst);
    return valid != 0 ? 0 : 1;
}

static uint8_t event_kind(const uint8_t *line, size_t length)
{
    if (line_starts(line, length, "==== MDEC_STATUS:")) return 1u;
    if (line_starts(line, length, "MDEC_DATA <-")) return 2u;
    if (line_starts(line, length, "MDEC_DATA ->")) return 3u;
    return 0u;
}

static int collect_status_events(const uint8_t *log, size_t log_size, status_events *events)
{
    memset(events, 0, sizeof *events);
    const uint8_t *start = find_bytes(log, log_size, "mdec/step-by-step-log",
                                      sizeof "mdec/step-by-step-log" - 1u);
    if (start == NULL) return 0;
    size_t at = (size_t)(start - log);
    while (at < log_size) {
        size_t line_size = 0u;
        const uint8_t *line = next_line(log, log_size, &at, &line_size);
        if (line == NULL) break;
        strip_tty_prefix(&line, &line_size);
        if (line_starts(line, line_size, "Done")) break;
        uint8_t kind = event_kind(line, line_size);
        if (kind == 0u) continue;
        if (events->count >= sizeof events->values) return 0;
        if (kind == 1u) events->status_count++;
        else if (kind == 2u) events->input_count++;
        else events->output_count++;
        if (events->count == 0u || events->values[events->count - 1u] != kind)
            events->values[events->count++] = kind;
    }
    return events->status_count >= 100u && events->input_count == 256u &&
           events->output_count == 512u;
}

static int status_order_matches(const uint8_t *expected, size_t expected_size,
                                const uint8_t *actual, size_t actual_size)
{
    status_events expected_events;
    status_events actual_events;
    if (!collect_status_events(expected, expected_size, &expected_events) ||
        !collect_status_events(actual, actual_size, &actual_events) ||
        expected_events.count != actual_events.count) return 0;
    for (size_t i = 0u; i < expected_events.count; i++) {
        if (expected_events.values[i] != actual_events.values[i]) return 0;
    }
    return 1;
}

static int run_status_guest(const char *exe_path, const char *log_path)
{
    size_t exe_size = 0u;
    size_t log_size = 0u;
    uint8_t *exe = read_bounded(exe_path, REFERENCE_EXE_LIMIT, &exe_size);
    uint8_t *log = read_bounded(log_path, REFERENCE_LOG_LIMIT, &log_size);
    if (exe == NULL || log == NULL || !digest_matches(exe, exe_size, SCHWAY_MDEC_STATUS_EXE_SHA256) ||
        !digest_matches(log, log_size, SCHWAY_MDEC_STATUS_LOG_SHA256)) {
        fprintf(stderr, "MDEC_REFERENCE_IDENTITY_ERROR: status guest or pinned log is missing or changed\n");
        free(exe);
        free(log);
        return 2;
    }
    uint8_t *tty = NULL;
    size_t tty_size = 0u;
    schway *inst = NULL;
    int guest_complete = run_guest(exe, exe_size, &tty, &tty_size, &inst, NULL);
    free(exe);
    int matches = guest_complete != 0 &&
                  find_bytes(tty, tty_size, "mdec/step-by-step-log",
                             sizeof "mdec/step-by-step-log" - 1u) != NULL &&
                  status_order_matches(log, log_size, tty, tty_size);
    if (!matches) {
        fprintf(stderr, "MDEC_REFERENCE_STATUS_ORDER_ERROR: actual guest status/data event order differs from the pinned trace\n");
    } else {
        printf("MDEC_REFERENCE_STATUS_MATCH: ordered status/input/output records; cycle timing not inferred\n");
    }
    free(tty);
    free(log);
    schway_destroy(inst);
    return matches ? 0 : 1;
}

static size_t line_end(const uint8_t *bytes, size_t size, size_t start)
{
    size_t end = start;
    while (end < size && bytes[end] != '\n') end++;
    return end < size ? end + 1u : size;
}

static int make_status_mutation(const uint8_t *source, size_t size, int drop,
                                uint8_t *output, size_t capacity, size_t *output_size)
{
    size_t status_start = SIZE_MAX;
    size_t status_end = 0u;
    size_t data_start = SIZE_MAX;
    size_t data_end = 0u;
    size_t at = 0u;
    while (at < size) {
        size_t start = at;
        size_t end = line_end(source, size, start);
        size_t length = end - start;
        if (length != 0u && source[start + length - 1u] == '\n') length--;
        const uint8_t *line = source + start;
        strip_tty_prefix(&line, &length);
        if (status_start == SIZE_MAX && line_starts(line, length, "==== MDEC_STATUS:")) {
            status_start = start;
            status_end = end;
        } else if (status_start != SIZE_MAX && data_start == SIZE_MAX &&
                   line_starts(line, length, "MDEC_DATA <-")) {
            data_start = start;
            data_end = end;
            break;
        }
        at = end;
    }
    if (status_start == SIZE_MAX || data_start == SIZE_MAX || data_start < status_end ||
        capacity < size) return 0;
    size_t used = 0u;
    memcpy(output, source, status_start);
    used += status_start;
    if (drop != 0) {
        size_t rest = status_end;
        memcpy(output + used, source + rest, size - rest);
        used += size - rest;
    } else {
        memcpy(output + used, source + data_start, data_end - data_start);
        used += data_end - data_start;
        memcpy(output + used, source + status_start, status_end - status_start);
        used += status_end - status_start;
        memcpy(output + used, source + status_end, data_start - status_end);
        used += data_start - status_end;
        memcpy(output + used, source + data_end, size - data_end);
        used += size - data_end;
    }
    *output_size = used;
    return 1;
}

static int run_negative_controls(const char *log_4bit_path, const char *log_8bit_path,
                                 const char *status_log_path)
{
    size_t log4_size = 0u;
    size_t log8_size = 0u;
    size_t status_size = 0u;
    uint8_t *log4 = read_bounded(log_4bit_path, REFERENCE_LOG_LIMIT, &log4_size);
    uint8_t *log8 = read_bounded(log_8bit_path, REFERENCE_LOG_LIMIT, &log8_size);
    uint8_t *status = read_bounded(status_log_path, REFERENCE_LOG_LIMIT, &status_size);
    if (log4 == NULL || log8 == NULL || status == NULL ||
        !digest_matches(log4, log4_size, SCHWAY_MDEC_4BIT_LOG_SHA256) ||
        !digest_matches(log8, log8_size, SCHWAY_MDEC_8BIT_LOG_SHA256) ||
        !digest_matches(status, status_size, SCHWAY_MDEC_STATUS_LOG_SHA256)) {
        fprintf(stderr, "MDEC_REFERENCE_CONTROL_ERROR: pinned control inputs are missing or changed\n");
        free(log4);
        free(log8);
        free(status);
        return 2;
    }
    uint8_t mutated[64];
    uint8_t expected4[32];
    uint8_t expected8[64];
    int block4_parsed = parse_expected_block(log4, log4_size, "4bit", 4u, expected4, sizeof expected4);
    int block8_parsed = parse_expected_block(log8, log8_size, "8bit", 8u, expected8, sizeof expected8);
    int regression_identity_mapped =
        strcmp(exe_digest_for("8bit-regression"), SCHWAY_MDEC_8BIT_EXE_SHA256) == 0 &&
        strcmp(log_digest_for("8bit-regression"), SCHWAY_MDEC_8BIT_LOG_SHA256) == 0;
    memcpy(mutated, expected4, sizeof expected4);
    mutated[0] ^= 1u;
    int corrupted_block_rejected = block4_parsed != 0 &&
                                   !block_bytes_match(mutated, expected4, sizeof expected4);
    uint8_t *mutated_status = (uint8_t *)malloc(status_size);
    size_t mutated_status_size = 0u;
    int reordered_rejected = mutated_status != NULL &&
                             make_status_mutation(status, status_size, 0, mutated_status,
                                                  status_size, &mutated_status_size) &&
                             !status_order_matches(status, status_size,
                                                   mutated_status, mutated_status_size);
    int dropped_rejected = mutated_status != NULL &&
                           make_status_mutation(status, status_size, 1, mutated_status,
                                                status_size, &mutated_status_size) &&
                           !status_order_matches(status, status_size,
                                                 mutated_status, mutated_status_size);
    if (block4_parsed == 0 || block8_parsed == 0 || !regression_identity_mapped ||
        !corrupted_block_rejected ||
        !reordered_rejected || !dropped_rejected) {
        fprintf(stderr, "MDEC_REFERENCE_CONTROL_ERROR: regression identity or byte/order control failed\n");
    } else {
        printf("MDEC_REFERENCE_CONTROLS_OK: one-byte output mutation and reordered/dropped status events rejected\n");
    }
    free(mutated_status);
    free(log4);
    free(log8);
    free(status);
    return block4_parsed != 0 && block8_parsed != 0 && regression_identity_mapped &&
                   corrupted_block_rejected &&
                   reordered_rejected && dropped_rejected ? 0 : 1;
}

int schway_mdec_color_reference_run(int argc, char **argv);

int main(int argc, char **argv)
{
    if (argc >= 2 && (strncmp(argv[1], "color", 5u) == 0))
        return schway_mdec_color_reference_run(argc, argv);
    if (argc == 4 && (strcmp(argv[1], "4bit") == 0 || strcmp(argv[1], "8bit") == 0))
        return run_block_guest(argv[1], argv[2], argv[3]);
    if (argc == 4 && strcmp(argv[1], "status") == 0)
        return run_status_guest(argv[2], argv[3]);
    if (argc == 5 && strcmp(argv[1], "controls") == 0)
        return run_negative_controls(argv[2], argv[3], argv[4]);
    fprintf(stderr, "MDEC_REFERENCE_USAGE_ERROR: expected 4bit|8bit|status EXE LOG or controls LOG4 LOG8 STATUS_LOG\n");
    return 2;
}
