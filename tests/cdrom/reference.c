/* Pinned timing and Getloc guests against the public CD host interface. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "sha256.h"
#include "internal.h"

#ifndef SCHWAY_CDROM_TIMING_EXE_SHA256
#error "CMake must bind the pinned CD timing executable digest"
#endif
#ifndef SCHWAY_CDROM_GETLOC_EXE_SHA256
#error "CMake must bind the pinned CD Getloc executable digest"
#endif
#ifndef SCHWAY_CDROM_TIMING_LOG_SHA256
#error "CMake must bind the pinned CD timing log digest"
#endif
#ifndef SCHWAY_CDROM_GETLOC_LOG_SHA256
#error "CMake must bind the pinned CD Getloc log digest"
#endif

enum {
    REFERENCE_LOG_LIMIT = 8192u,
    REFERENCE_EXE_LIMIT = 1024u * 1024u,
    GUEST_TTY_LIMIT = 16384u,
    GUEST_FRAME_LIMIT = 1200u,
    REFERENCE_ROWS = 10u,
    SECTOR_BYTES = 2352u,
    SUBCODE_BYTES = 96u,
    DISC_LEADOUT = 333150u
};

typedef struct timing_row {
    uint32_t minimum;
    uint32_t maximum;
    uint32_t average;
} timing_row;

typedef struct disc_host {
    schway_toc toc;
    schway *inst;
    uint32_t reads;
    uint32_t last_lba;
    uint32_t callbacks_outside_run;
    uint32_t in_run;
    uint32_t frames;
    uint32_t run_failure;
    uint32_t last_stop;
    uint32_t last_fault;
    uint32_t last_speed;
    uint32_t last_read_active;
    uint64_t last_tick;
    uint32_t interval_count[2];
    uint64_t intervals[2][512];
    uint32_t interval_overflow[2];
    uint32_t q_corruption;
} disc_host;

typedef struct q_fixture {
    uint32_t lba;
    uint8_t setloc[3];
    uint8_t header[3];
    uint8_t q[12];
    uint8_t getlocp[8];
} q_fixture;

static const q_fixture q_fixtures[] = {
    {16u, {0x00u, 0x02u, 0x16u}, {0x00u, 0x02u, 0x16u},
     {0x41u, 0x01u, 0x01u, 0x00u, 0x00u, 0x11u, 0x00u, 0x00u, 0x02u, 0x11u,
      0x84u, 0x29u},
     {0x01u, 0x01u, 0x00u, 0x02u, 0x11u, 0x00u, 0x00u, 0x11u}},
    {0u, {0x00u, 0x00u, 0x30u}, {0x00u, 0x00u, 0x29u},
     {0x41u, 0x01u, 0x00u, 0x00u, 0x01u, 0x41u, 0x00u, 0x00u, 0x00u, 0x33u,
      0xF1u, 0x2Au},
     {0x01u, 0x00u, 0x00u, 0x00u, 0x33u, 0x00u, 0x01u, 0x41u}},
    {332850u, {0x74u, 0x00u, 0x00u}, {0x73u, 0x59u, 0x74u},
     {0x41u, 0xAAu, 0x01u, 0x03u, 0x34u, 0x62u, 0x00u, 0x74u, 0x00u, 0x03u,
      0xCAu, 0xFEu},
     {0xAAu, 0x01u, 0x74u, 0x00u, 0x03u, 0x03u, 0x34u, 0x62u}}
};

static const q_fixture *q_fixture_for_lba(uint32_t lba)
{
    for (size_t i = 0u; i < sizeof q_fixtures / sizeof q_fixtures[0]; i++) {
        if (q_fixtures[i].lba == lba) return &q_fixtures[i];
    }
    return NULL;
}

static void encode_q_subcode(const uint8_t q[12], uint8_t subcode[SUBCODE_BYTES],
                             uint32_t corruption)
{
    for (uint32_t bit = 0u; bit < 96u; bit++) {
        uint32_t source = corruption == 2u ? (bit + 1u) % 96u : bit;
        uint8_t value = (uint8_t)((q[source / 8u] >> (7u - (source & 7u))) & 1u);
        subcode[bit] = (uint8_t)(value << 6);
    }
}

static void fixture_q_crc(uint8_t q[12])
{
    uint16_t crc = 0u;
    for (uint32_t i = 0u; i < 10u; i++) {
        crc ^= (uint16_t)q[i] << 8;
        for (uint32_t bit = 0u; bit < 8u; bit++) {
            uint32_t shifted = (uint32_t)crc << 1;
            crc = (crc & 0x8000u) != 0u ? (uint16_t)(shifted ^ 0x1021u)
                                        : (uint16_t)shifted;
        }
    }
    crc = (uint16_t)~crc;
    q[10] = (uint8_t)(crc >> 8);
    q[11] = (uint8_t)crc;
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
    size_t got = fread(bytes, 1u, size, file);
    int close_result = fclose(file);
    if (got != size || close_result != 0) {
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

static uint8_t to_bcd(uint32_t value)
{
    return (uint8_t)(((value / 10u) << 4) | (value % 10u));
}

static void record_timing_interval(disc_host *host, uint32_t speed_index, uint64_t delta)
{
    if (host->interval_count[speed_index] < 512u) {
        host->intervals[speed_index][host->interval_count[speed_index]++] = delta;
    } else {
        host->interval_overflow[speed_index] = 1u;
    }
}

static uint32_t make_sector(void *user, uint32_t lba, uint8_t sector[SECTOR_BYTES],
                            uint8_t subcode[SUBCODE_BYTES])
{
    disc_host *host = (disc_host *)user;
    uint32_t previous_lba = host->last_lba;
    uint32_t had_previous = host->reads != 0u;
    host->reads++;
    if (host->in_run == 0u) host->callbacks_outside_run++;
    uint32_t speed = host->inst->cdrom.drive_speed;
    uint32_t speed_index = speed == 2u ? 1u : 0u;
    if (had_previous != 0u && previous_lba != UINT32_MAX && lba == previous_lba + 1u &&
        host->last_tick != 0u && host->last_speed == speed &&
        host->last_read_active != 0u && host->inst->cdrom.read_active != 0u &&
        host->inst->ticks > host->last_tick) {
        uint64_t delta = host->inst->ticks - host->last_tick;
        record_timing_interval(host, speed_index, delta);
    }
    host->last_lba = lba;
    host->last_tick = host->inst->ticks;
    host->last_speed = speed;
    host->last_read_active = host->inst->cdrom.read_active;
    memset(sector, 0, SECTOR_BYTES);
    memset(subcode, 0, SUBCODE_BYTES);
    memset(&sector[1], 0xFF, 10u);
    uint32_t absolute = lba + 150u;
    sector[12] = to_bcd(absolute / (60u * 75u));
    sector[13] = to_bcd((absolute / 75u) % 60u);
    sector[14] = to_bcd(absolute % 75u);
    sector[15] = 2u;
    sector[16] = 1u;
    sector[17] = 2u;
    sector[18] = 8u;
    sector[19] = 0u;
    sector[20] = 1u;
    sector[21] = 2u;
    sector[22] = 8u;
    sector[23] = 0u;
    const q_fixture *fixture = q_fixture_for_lba(lba);
    if (fixture != NULL) {
        uint8_t q[12];
        memcpy(q, fixture->q, sizeof q);
        memcpy(&sector[12], fixture->header, sizeof fixture->header);
        if (host->q_corruption == 1u) q[10] ^= 1u;
        if (host->q_corruption == 3u) { q[1] = 0u; fixture_q_crc(q); }
        if (host->q_corruption == 4u) { q[1] = 0x02u; fixture_q_crc(q); }
        encode_q_subcode(q, subcode, host->q_corruption);
    }
    return 1u;
}

static uint32_t get_toc(void *user, schway_toc *out)
{
    disc_host *host = (disc_host *)user;
    memcpy(out, &host->toc, sizeof *out);
    return 1u;
}

static int attach_disc(schway *inst, disc_host *host)
{
    memset(host, 0, sizeof *host);
    host->inst = inst;
    host->toc.size = (uint32_t)sizeof host->toc;
    host->toc.track_count = 1u;
    host->toc.tracks[0].size = (uint32_t)sizeof host->toc.tracks[0];
    host->toc.tracks[0].type = SCHWAY_DISC_TRACK_MODE2;
    host->toc.tracks[0].index0 = 150u;
    host->toc.tracks[0].index1 = 150u;
    host->toc.tracks[0].leadout = DISC_LEADOUT;
    schway_disc disc;
    memset(&disc, 0, sizeof disc);
    disc.size = (uint32_t)sizeof disc;
    disc.user = host;
    disc.read_sector = make_sector;
    disc.toc = get_toc;
    if (schway_open_disc(inst, &disc) != SCHWAY_STATUS_OK) return 0;
    schway_profile profile;
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = SCHWAY_REGION_NTSC_U;
    profile.flags = SCHWAY_PROFILE_DEBUG_PORT | SCHWAY_PROFILE_CD_LICENSED;
    return schway_set_profile(inst, &profile) == SCHWAY_STATUS_OK;
}

static int line_value(const uint8_t *line, size_t length, const char *prefix,
                      timing_row *row)
{
    char text[512];
    while (length >= 4u && memcmp(line, "tty ", 4u) == 0) {
        line += 4u;
        length -= 4u;
    }
    size_t prefix_length = strlen(prefix);
    if (length >= sizeof text || length < prefix_length ||
        memcmp(line, prefix, prefix_length) != 0) return 0;
    memcpy(text, line, length);
    text[length] = '\0';
    const char *remaining = strstr(text, "Remaining Sectors = min ");
    if (remaining == NULL || sscanf(remaining, "Remaining Sectors = min %u max %u avg %u ticks",
                                     &row->minimum, &row->maximum, &row->average) != 3) return 0;
    return row->minimum <= row->average && row->average <= row->maximum;
}

static size_t timing_rows(const uint8_t *bytes, size_t size, timing_row rows[REFERENCE_ROWS])
{
    size_t used = 0u;
    size_t single_count = 0u;
    size_t double_count = 0u;
    size_t at = 0u;
    while (at < size) {
        size_t end = at;
        while (end < size && bytes[end] != '\n') end++;
        const uint8_t *line = bytes + at;
        size_t length = end - at;
        at = end < size ? end + 1u : size;
        if (length > 0u && line[length - 1u] == '\r') length--;
        while (length >= 4u && memcmp(line, "tty ", 4u) == 0) {
            line += 4u;
            length -= 4u;
        }
        if (length >= 20u && memcmp(line, "single-speed timing:", 20u) == 0) {
            if (used >= REFERENCE_ROWS || double_count != 0u || single_count >= 5u) return 0u;
            if (!line_value(line, length, "single-speed timing:", &rows[used])) {
                if (find_bytes(line, length, "Remaining Sectors = min %u max %u avg %u ticks",
                               sizeof "Remaining Sectors = min %u max %u avg %u ticks" - 1u) == NULL)
                    return 0u;
                rows[used] = (timing_row){451584u, 451584u, 451584u};
            }
            used++;
            single_count++;
        } else if (length >= 20u && memcmp(line, "double-speed timing:", 20u) == 0) {
            if (used >= REFERENCE_ROWS || single_count != 5u || double_count >= 5u) return 0u;
            if (!line_value(line, length, "double-speed timing:", &rows[used])) {
                if (find_bytes(line, length, "Remaining Sectors = min %u max %u avg %u ticks",
                               sizeof "Remaining Sectors = min %u max %u avg %u ticks" - 1u) == NULL)
                    return 0u;
                rows[used] = (timing_row){225792u, 225792u, 225792u};
            }
            used++;
            double_count++;
        }
    }
    return single_count == 5u && double_count == 5u ? used : 0u;
}

static int timing_matches(const uint8_t *actual, size_t actual_size,
                          const uint8_t *reference, size_t reference_size,
                          const disc_host *host)
{
    timing_row actual_rows[REFERENCE_ROWS];
    timing_row reference_rows[REFERENCE_ROWS];
    size_t actual_count = timing_rows(actual, actual_size, actual_rows);
    size_t reference_count = timing_rows(reference, reference_size, reference_rows);
    if (actual_count != REFERENCE_ROWS || reference_count != REFERENCE_ROWS) return 0;
    for (size_t i = 0u; i < REFERENCE_ROWS; i++) {
        uint32_t selected = i < 5u ? 451584u : 225792u;
        if (selected < reference_rows[i].minimum || selected > reference_rows[i].maximum ||
            actual_rows[i].average != selected) return 0;
    }
    for (size_t speed = 0u; speed < 2u; speed++) {
        if (host->interval_overflow[speed] != 0u) {
            fprintf(stderr, "CD_REFERENCE_TIMING_INTERVAL_ERROR: speed=%u sample buffer overflowed\n",
                    speed == 0u ? 1u : 2u);
            return 0;
        }
        if (host->interval_count[speed] < 5u) {
            fprintf(stderr, "CD_REFERENCE_TIMING_INTERVAL_ERROR: speed=%u only %u samples\n",
                    speed == 0u ? 1u : 2u, host->interval_count[speed]);
            return 0;
        }
        for (uint32_t i = 0u; i < host->interval_count[speed]; i++) {
            uint32_t within_measurement = 0u;
            for (size_t band = speed * 5u; band < speed * 5u + 5u; band++) {
                if (host->intervals[speed][i] >= reference_rows[band].minimum &&
                    host->intervals[speed][i] <= reference_rows[band].maximum) {
                    within_measurement = 1u;
                }
            }
            if (within_measurement == 0u) {
                fprintf(stderr,
                        "CD_REFERENCE_TIMING_INTERVAL_ERROR: speed=%u sample=%u ticks=%llu outside pinned named bands\n",
                        speed == 0u ? 1u : 2u, i,
                        (unsigned long long)host->intervals[speed][i]);
                return 0;
            }
        }
    }
    return 1;
}

static int timing_interval_control(const uint8_t *actual, size_t actual_size,
                                   const uint8_t *reference, size_t reference_size,
                                   const disc_host *baseline, uint32_t lba,
                                   uint32_t expect_sample)
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
    disc_host observed = *baseline;
    observed.inst = inst;
    observed.reads = 1u;
    observed.last_tick = 1u;
    observed.last_lba = 100u;
    observed.last_speed = 1u;
    observed.last_read_active = 1u;
    inst->ticks = 1000000u;
    inst->cdrom.drive_speed = 1u;
    inst->cdrom.read_active = 1u;
    uint8_t sector[SECTOR_BYTES];
    uint8_t subcode[SUBCODE_BYTES];
    (void)make_sector(&observed, lba, sector, subcode);
    int matched = timing_matches(actual, actual_size, reference, reference_size, &observed);
    int rejected = expect_sample != 0u
                       ? observed.interval_count[0] == baseline->interval_count[0] + 1u &&
                             observed.intervals[0][baseline->interval_count[0]] == 999999u &&
                             !matched
                       : observed.interval_count[0] == baseline->interval_count[0] && matched;
    schway_destroy(inst);
    return rejected;
}

static int append_tty(schway *inst, uint8_t *text, size_t *used)
{
    uint8_t drained[2048];
    schway_tty tty;
    memset(&tty, 0, sizeof tty);
    tty.size = (uint32_t)sizeof tty;
    tty.bytes = drained;
    tty.capacity = (uint32_t)sizeof drained;
    schway_status status = schway_get_tty(inst, &tty);
    if (status != SCHWAY_STATUS_OK || tty.dropped != 0u || tty.written > sizeof drained ||
        tty.written > GUEST_TTY_LIMIT - *used) return 0;
    memcpy(text + *used, drained, tty.written);
    *used += tty.written;
    return 1;
}

static int run_guest(schway *inst, disc_host *host, uint8_t *text, size_t *text_size,
                     int timing)
{
    timing_row rows[REFERENCE_ROWS];
    for (uint32_t frame = 0u; frame < GUEST_FRAME_LIMIT; frame++) {
        schway_run_result result;
        memset(&result, 0, sizeof result);
        result.size = (uint32_t)sizeof result;
        host->in_run = 1u;
        schway_status status = schway_run_frame(inst, &result);
        host->in_run = 0u;
        host->frames = frame + 1u;
        host->last_stop = result.stop;
        host->last_fault = result.fault_present;
        if (status != SCHWAY_STATUS_OK || result.fault_present != 0u ||
            (result.stop != SCHWAY_STOP_FRAME && result.stop != SCHWAY_STOP_BUDGET) ||
            !append_tty(inst, text, text_size)) {
            host->run_failure = (uint32_t)status + 1u;
            return -1;
        }
        if (timing != 0 && timing_rows(text, *text_size, rows) == REFERENCE_ROWS)
            return 1;
        if (timing == 0 && find_bytes(text, *text_size, "Test passed", 11u) != NULL) return 1;
    }
    return 0;
}

static int ordered_getloc(const uint8_t *bytes, size_t size)
{
    static const char *const required[] = {
        "GetStat -> 0x%02X", "GetlocL failed, IRQ = %d, status = 0x%02X",
        "GetlocP succeeded - track %02x index %02x", "Testing seek...",
        "Seek result irq=%d, status=0x%02X", "Testing Getloc after seek",
        "GetlocL succeeded", "Testing read", "Testing GetLoc after reading",
        "Seeking to [%02d:%02d:%02d]", "Test passed"
    };
    if (find_bytes(bytes, size, "! ", 2u) != NULL) return 0;
    size_t at = 0u;
    for (size_t i = 0u; i < sizeof required / sizeof required[0]; i++) {
        const uint8_t *found = find_bytes(bytes + at, size - at, required[i], strlen(required[i]));
        if (found == NULL) return 0;
        at = (size_t)(found - bytes) + strlen(required[i]);
    }
    return 1;
}

static int load_and_run(const char *kind, const char *exe_path, const char *log_path)
{
    size_t exe_size = 0u;
    size_t log_size = 0u;
    const char *exe_hash = strcmp(kind, "timing") == 0
                               ? SCHWAY_CDROM_TIMING_EXE_SHA256 : SCHWAY_CDROM_GETLOC_EXE_SHA256;
    const char *log_hash = strcmp(kind, "timing") == 0
                               ? SCHWAY_CDROM_TIMING_LOG_SHA256 : SCHWAY_CDROM_GETLOC_LOG_SHA256;
    uint8_t *exe = read_bounded(exe_path, REFERENCE_EXE_LIMIT, &exe_size);
    uint8_t *log = read_bounded(log_path, REFERENCE_LOG_LIMIT, &log_size);
    if (exe == NULL || log == NULL || !digest_matches(exe, exe_size, exe_hash) ||
        !digest_matches(log, log_size, log_hash)) {
        fprintf(stderr, "CD_REFERENCE_IDENTITY_ERROR: pinned guest or source log is missing or changed\n");
        free(exe);
        free(log);
        return 2;
    }
    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = reference_alloc;
    config.allocator.free = reference_free;
    schway *inst = NULL;
    schway_status status = schway_create(&config, &inst);
    disc_host host;
    if (status != SCHWAY_STATUS_OK || inst == NULL || !attach_disc(inst, &host)) {
        fprintf(stderr, "CD_REFERENCE_CORE_ERROR: cannot create instance or attach public disc\n");
        schway_destroy(inst);
        free(exe);
        free(log);
        return 2;
    }
    status = schway_load_exe(inst, exe, exe_size);
    free(exe);
    if (status != SCHWAY_STATUS_OK) {
        fprintf(stderr, "CD_REFERENCE_CORE_ERROR: guest load returned %s\n", schway_status_string(status));
        schway_destroy(inst);
        free(log);
        return 2;
    }
    uint8_t *text = (uint8_t *)calloc(GUEST_TTY_LIMIT, 1u);
    size_t text_size = 0u;
    int run_result = text != NULL ? run_guest(inst, &host, text, &text_size,
                                               strcmp(kind, "timing") == 0) : -1;
    int matched = run_result == 1 && host.callbacks_outside_run == 0u && host.reads != 0u;
    if (matched && strcmp(kind, "timing") == 0)
        matched = timing_matches(text, text_size, log, log_size, &host);
    if (matched && strcmp(kind, "getloc") == 0) matched = ordered_getloc(text, text_size);
    if (!matched) {
        fprintf(stderr,
                "CD_REFERENCE_GUEST_ERROR: %s caller did not satisfy ordered hardware predicates "
                "(run_result=%d failure=%u frames=%u stop=%u fault=%u reads=%u)\n",
                kind, run_result, host.run_failure, host.frames, host.last_stop,
                host.last_fault, host.reads);
        if (text != NULL) fwrite(text, 1u, text_size, stderr);
    } else {
        printf("CD_REFERENCE_GUEST_MATCH: guest=%s frames<=%u callbacks=%u callbacks_outside_run=0\n",
               kind, (unsigned)GUEST_FRAME_LIMIT, host.reads);
        if (strcmp(kind, "timing") == 0) {
            printf("CD_REFERENCE_TIMING_BAND: callbacks_1x=%u callbacks_2x=%u selected=451584/225792 within pinned per-row ranges; TTY template has no formatted values\n",
                   host.interval_count[0], host.interval_count[1]);
        }
    }
    free(text);
    free(log);
    schway_destroy(inst);
    return matched ? 0 : 1;
}

static int append_timing_record(char *text, size_t capacity, size_t *used,
                                int double_speed, uint32_t minimum, uint32_t maximum,
                                uint32_t average)
{
    const char *speed = double_speed != 0 ? "double-speed" : "single-speed";
    int count = snprintf(text + *used, capacity - *used,
                         "%s timing: Remaining Sectors = min %u max %u avg %u ticks\n",
                         speed, minimum, maximum, average);
    if (count < 0 || (size_t)count >= capacity - *used) return 0;
    *used += (size_t)count;
    return 1;
}

static int ordered_pair(const uint8_t *text, size_t size)
{
    const char *const first = "GetStat -> 0x%02X";
    const char *const second = "GetlocL failed, IRQ = %d, status = 0x%02X";
    const uint8_t *first_at = find_bytes(text, size, first, strlen(first));
    const uint8_t *second_at = find_bytes(text, size, second, strlen(second));
    return first_at != NULL && second_at != NULL && first_at < second_at;
}

static int control_write(schway *inst, uint32_t offset, uint8_t value)
{
    uint32_t cycles = 0u;
    return schway__bus_write(inst, 0x1F801800u + offset, 1u, value, &cycles) == 0;
}

static int control_read(schway *inst, uint32_t offset, uint8_t *value)
{
    uint32_t cycles = 0u;
    uint32_t result = 0u;
    if (schway__bus_read(inst, 0x1F801800u + offset, 1u, &result, &cycles) != 0) return 0;
    *value = (uint8_t)result;
    return 1;
}

static int control_write32(schway *inst, uint32_t address, uint32_t value)
{
    uint32_t cycles = 0u;
    return schway__bus_write(inst, address, 4u, value, &cycles) == 0;
}

static int control_run(schway *inst, disc_host *host, uint64_t cycles)
{
    while (cycles != 0u) {
        schway_budget budget;
        schway_run_result result;
        memset(&budget, 0, sizeof budget);
        memset(&result, 0, sizeof result);
        budget.size = (uint32_t)sizeof budget;
        budget.max_cycles = cycles;
        result.size = (uint32_t)sizeof result;
        uint64_t before = inst->ticks;
        host->in_run = 1u;
        schway_status status = schway_run(inst, &budget, &result);
        host->in_run = 0u;
        uint64_t elapsed = result.ticks - before;
        if (status != SCHWAY_STATUS_OK || result.fault_present != 0u || elapsed == 0u ||
            elapsed > cycles) return 0;
        cycles -= elapsed;
    }
    return 1;
}

static int control_result(schway *inst, const uint8_t *expected, size_t count)
{
    for (size_t i = 0u; i < count; i++) {
        uint8_t byte = 0u;
        if (!control_read(inst, 1u, &byte) || byte != expected[i]) {
            fprintf(stderr, "CD_REFERENCE_GETLOC_RESULT_ERROR: byte=%u expected=0x%02x got=0x%02x\n",
                    (unsigned)i, expected[i], byte);
            return 0;
        }
    }
    return 1;
}

static int control_wait_irq(schway *inst, disc_host *host, uint32_t irq,
                            const uint8_t *expected, size_t count, uint64_t cycles)
{
    uint8_t code = 0u;
    if (!control_run(inst, host, cycles)) {
        fputs("CD_REFERENCE_GETLOC_RUN_ERROR: deterministic cycles did not advance cleanly\n", stderr);
        return 0;
    }
    if (!control_write(inst, 0u, 1u) || !control_read(inst, 3u, &code) ||
        (code & 7u) != irq) {
        fprintf(stderr, "CD_REFERENCE_GETLOC_IRQ_ERROR: expected=%u got=%u\n", irq, code);
        return 0;
    }
    if (!control_write(inst, 0u, 0u) || !control_result(inst, expected, count) ||
        !control_write(inst, 0u, 1u) ||
        !control_write(inst, 3u, (uint8_t)(1u << (irq - 1u))) ||
        !control_write(inst, 0u, 0u)) {
        fputs("CD_REFERENCE_GETLOC_ACK_ERROR: result or interrupt acknowledge failed\n", stderr);
        return 0;
    }
    return 1;
}

static int control_command(schway *inst, uint8_t command, const uint8_t *params,
                           size_t param_count)
{
    if (!control_write(inst, 0u, 0u)) return 0;
    for (size_t i = 0u; i < param_count; i++) {
        if (!control_write(inst, 2u, params[i])) return 0;
    }
    return control_write(inst, 1u, command);
}

static int getloc_byte_controls(const char *log_path)
{
    size_t log_size = 0u;
    uint8_t *log = read_bounded(log_path, REFERENCE_LOG_LIMIT, &log_size);
    static const char *const pinned[] = {
        "GetStat -> 0x02", "GetlocL failed, IRQ = 5, status = 0x02",
        "Seek result irq=2, status=0x02", "GetlocL succeeded - [00:02:16] mode 2",
        "GetlocP succeeded - track 01 index 01 [00:00:11] absolute [00:02:11]"
    };
    size_t at = 0u;
    int pinned_matches = log != NULL &&
                         digest_matches(log, log_size, SCHWAY_CDROM_GETLOC_LOG_SHA256);
    for (size_t i = 0u; pinned_matches != 0 && i < sizeof pinned / sizeof pinned[0]; i++) {
        const uint8_t *found = find_bytes(log + at, log_size - at, pinned[i], strlen(pinned[i]));
        if (found == NULL) pinned_matches = 0;
        else at = (size_t)(found - log) + strlen(pinned[i]);
    }
    free(log);
    if (pinned_matches == 0) {
        fputs("CD_REFERENCE_GETLOC_PIN_ERROR: pinned status/data rows changed\n", stderr);
        return 0;
    }

    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = reference_alloc;
    config.allocator.free = reference_free;
    schway *inst = NULL;
    disc_host host;
    if (schway_create(&config, &inst) != SCHWAY_STATUS_OK || inst == NULL ||
        !attach_disc(inst, &host)) {
        schway_destroy(inst);
        fputs("CD_REFERENCE_GETLOC_CORE_ERROR: cannot create the deterministic disc control\n", stderr);
        return 0;
    }
    schway_profile profile;
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = SCHWAY_REGION_NTSC_U;
    profile.flags = SCHWAY_PROFILE_CD_LICENSED;
    static const uint8_t init_status[] = {0x02u};
    static const uint8_t no_sector[] = {0x02u, 0x80u};
    static const uint8_t command_status[] = {0x02u};
    static const uint8_t read_seek_status[] = {0x42u};
    static const uint8_t read_getstat_status[] = {0x42u};
    static const uint8_t loc_l[] = {0u, 0x02u, 0x16u, 0x02u, 1u, 2u, 8u, 0u};
    static const uint8_t read_loc_l[] = {0x40u, 0u, 0x16u, 2u, 1u, 2u, 8u, 0u};
    uint8_t address[3] = {0u, 0x02u, 0x16u};
    uint8_t read_address[3] = {0x40u, 0u, 0x16u};
    uint8_t hsts = 0u;
    uint8_t irq = 0u;
    uint32_t reads_before = 0u;
    const char *failed = NULL;
    if (schway_set_profile(inst, &profile) != SCHWAY_STATUS_OK) {
        failed = "profile";
        goto getloc_done;
    }
    if (!control_command(inst, 0x0Au, NULL, 0u) ||
        !control_wait_irq(inst, &host, 3u, init_status, sizeof init_status, 50000u)) {
        failed = "Init first response";
        goto getloc_done;
    }
    if (!control_wait_irq(inst, &host, 2u, init_status, sizeof init_status, 476000u)) {
        failed = "Init completion";
        goto getloc_done;
    }
    if (!control_command(inst, 0x10u, NULL, 0u) ||
        !control_wait_irq(inst, &host, 5u, no_sector, sizeof no_sector, 50000u)) {
        failed = "empty GetlocL error";
        goto getloc_done;
    }
    if (!control_command(inst, 0x02u, address, sizeof address) ||
        !control_wait_irq(inst, &host, 3u, command_status, sizeof command_status, 50000u)) {
        failed = "Setloc response";
        goto getloc_done;
    }
    if (!control_command(inst, 0x15u, NULL, 0u) ||
        !control_wait_irq(inst, &host, 3u, command_status, sizeof command_status, 50000u)) {
        failed = "SeekL response";
        goto getloc_done;
    }
    uint64_t seek_cycles = inst->cdrom.seek_deadline > inst->ticks
                               ? inst->cdrom.seek_deadline - inst->ticks : 0u;
    if (seek_cycles == 0u ||
        !control_wait_irq(inst, &host, 2u, command_status, sizeof command_status, seek_cycles) ||
        host.reads != 1u || host.last_lba != 16u || host.callbacks_outside_run != 0u) {
        failed = "SeekL completion and target callback";
        goto getloc_done;
    }
    if (!control_command(inst, 0x10u, NULL, 0u) ||
        !control_wait_irq(inst, &host, 3u, loc_l, sizeof loc_l, 50000u)) {
        failed = "GetlocL bytes";
        goto getloc_done;
    }
    if (!control_command(inst, 0x02u, read_address, sizeof read_address) ||
        !control_wait_irq(inst, &host, 3u, command_status, sizeof command_status, 50000u)) {
        failed = "ReadN Setloc status";
        goto getloc_done;
    }
    if (!control_command(inst, 0x0Eu, (const uint8_t[]){0x20u}, 1u) ||
        !control_wait_irq(inst, &host, 3u, command_status, sizeof command_status, 50000u)) {
        failed = "ReadN Setmode status";
        goto getloc_done;
    }
    if (!control_command(inst, 0x06u, NULL, 0u) ||
        !control_wait_irq(inst, &host, 3u, read_seek_status,
                          sizeof read_seek_status, 50000u)) {
        failed = "ReadN seek status";
        goto getloc_done;
    }
    if (!control_command(inst, 0x01u, NULL, 0u) ||
        !control_wait_irq(inst, &host, 3u, read_getstat_status,
                          sizeof read_getstat_status, 50000u)) {
        failed = "ReadN GetStat status";
        goto getloc_done;
    }
    if (!control_read(inst, 0u, &hsts) || (hsts & 0x80u) != 0u) {
        fprintf(stderr, "CD_REFERENCE_GETLOC_BUSY_ERROR: hsts=0x%02x\n", hsts);
        failed = "ReadN BUSYSTS clear after command acceptance";
        goto getloc_done;
    }
    if (inst->cdrom.sector_count != 0u || inst->cdrom.sector_offset != 0u) {
        fprintf(stderr, "CD_REFERENCE_GETLOC_SEEK_BUFFER_ERROR: sectors=%u offset=%u\n",
                inst->cdrom.sector_count, inst->cdrom.sector_offset);
        failed = "ReadN stale seek header";
        goto getloc_done;
    }
    uint64_t read_seek_cycles = inst->cdrom.seek_deadline > inst->ticks
                                    ? inst->cdrom.seek_deadline - inst->ticks : 0u;
    reads_before = host.reads;
    if (read_seek_cycles == 0u || !control_run(inst, &host, read_seek_cycles) ||
        !control_run(inst, &host, UINT64_C(451584)) || host.reads != reads_before + 1u ||
        host.last_lba != 179866u || host.callbacks_outside_run != 0u) {
        failed = "ReadN implicit seek and target sector";
        goto getloc_done;
    }
    if (!control_write(inst, 0u, 1u) || !control_read(inst, 3u, &irq) || (irq & 7u) != 1u ||
        !control_write(inst, 0u, 0u) || !control_write(inst, 3u, 0x80u) ||
        !control_write(inst, 0u, 1u) || !control_write(inst, 3u, 0x5Fu) ||
        !control_write(inst, 0u, 0u) || !control_read(inst, 0u, &hsts) ||
        (hsts & 0xC0u) != 0x40u) {
        fprintf(stderr, "CD_REFERENCE_GETLOC_READINESS_ERROR: irq=0x%02x hsts=0x%02x\n",
                irq, hsts);
        failed = "ReadN INT1/BFRD HSTS bits";
        goto getloc_done;
    }
    memset(&inst->bus.ram[0x3000u], 0xA5, 2340u);
    if (!control_write32(inst, 0x1F8010F0u, 0x0FEDCBA9u) ||
        !control_write32(inst, 0x1F8010B0u, 0x3000u) ||
        !control_write32(inst, 0x1F8010B4u, 0x00010249u) ||
        !control_write32(inst, 0x1F8010B8u, 0x11000000u) ||
        !control_run(inst, &host, 20000u)) {
        failed = "ReadN DMA3 did not run";
        goto getloc_done;
    }
    uint32_t transfer_bytes = 0u;
    for (uint32_t i = 0u; i < 2340u; i++) {
        uint8_t expected = i < 8u ? read_loc_l[i] :
                           i < 12u ? read_loc_l[i - 4u] : 0u;
        if (inst->bus.ram[0x3000u + i] != expected) break;
        transfer_bytes++;
    }
    if ((inst->dma.chcr[3] & SCHWAY__DMA_CHCR_BUSY) != 0u ||
        inst->dma.madr[3] != 0x3924u || inst->cdrom.sector_offset != 2340u ||
        transfer_bytes != 2340u) {
        uint32_t first_word = 0u;
        memcpy(&first_word, &inst->bus.ram[0x3000u], sizeof first_word);
        fprintf(stderr, "CD_REFERENCE_GETLOC_DMA_ERROR: chcr=0x%08x first=0x%08x "
                        "madr=0x%08x bytes=%u bfrd=%u sectors=%u offset=%u\n",
                inst->dma.chcr[3], first_word, inst->dma.madr[3], transfer_bytes,
                inst->cdrom.bfrd, inst->cdrom.sector_count,
                inst->cdrom.sector_offset);
        failed = "ReadN DMA3 bytes";
        goto getloc_done;
    }
    if (!control_command(inst, 0x09u, NULL, 0u) ||
        !control_wait_irq(inst, &host, 3u, command_status, sizeof command_status, 50000u) ||
        !control_wait_irq(inst, &host, 2u, command_status, sizeof command_status, 2170000u) ||
        !control_command(inst, 0x01u, NULL, 0u) ||
        !control_wait_irq(inst, &host, 3u, command_status, sizeof command_status, 50000u)) {
        failed = "ReadN pause and GetStat status";
        goto getloc_done;
    }
    if (!control_command(inst, 0x10u, NULL, 0u) ||
        !control_wait_irq(inst, &host, 3u, read_loc_l, sizeof read_loc_l, 50000u)) {
        failed = "post-read GetlocL bytes";
        goto getloc_done;
    }
getloc_done:
    schway_destroy(inst);
    if (failed != NULL) {
        fprintf(stderr, "CD_REFERENCE_GETLOC_DATA_ERROR: failed at %s\n", failed);
        return 0;
    }
    puts("CD_REFERENCE_GETLOC_DATA_MATCH: IRQ/status bytes, GetlocL headers, and full DMA bytes matched");
    puts("CD_REFERENCE_GETLOC_LIMIT: exact Q/GetlocP seek rows are covered separately from the TTY templates");
    return 1;
}

static int control_q_fixture(disc_host *host, schway *inst, const q_fixture *fixture,
                             const uint8_t *expected_q, uint8_t setloc_value)
{
    static const uint8_t seek_status[] = {0x02u};
    uint8_t setloc_status[] = {setloc_value};
    uint8_t expected_header[8] = {fixture->header[0], fixture->header[1], fixture->header[2],
                                  2u, 1u, 2u, 8u, 0u};
    if (!control_command(inst, 0x02u, fixture->setloc, sizeof fixture->setloc) ||
        !control_wait_irq(inst, host, 3u, setloc_status, sizeof setloc_status, 50000u)) {
        fputs("CD_REFERENCE_Q_SEEK_ERROR: Setloc response mismatch\n", stderr);
        return 0;
    }
    if (!control_command(inst, 0x15u, NULL, 0u) ||
        !control_wait_irq(inst, host, 3u, seek_status, sizeof seek_status, 50000u)) {
        fputs("CD_REFERENCE_Q_SEEK_ERROR: SeekL initial response mismatch\n", stderr);
        return 0;
    }
    uint64_t cycles = inst->cdrom.seek_deadline > inst->ticks
                          ? inst->cdrom.seek_deadline - inst->ticks : 0u;
    if (cycles == 0u || !control_run(inst, host, cycles) ||
        !control_wait_irq(inst, host, 2u, seek_status, sizeof seek_status, 0u) ||
        host->last_lba != fixture->lba || host->callbacks_outside_run != 0u) {
        fprintf(stderr, "CD_REFERENCE_Q_SEEK_ERROR: completion lba=%u expected=%u outside=%u\n",
                host->last_lba, fixture->lba, host->callbacks_outside_run);
        return 0;
    }
    if (!control_command(inst, 0x10u, NULL, 0u) ||
        !control_wait_irq(inst, host, 3u, expected_header, sizeof expected_header, 50000u)) {
        fputs("CD_REFERENCE_Q_GETLOCL_ERROR: target header mismatch\n", stderr);
        return 0;
    }
    if (!control_command(inst, 0x11u, NULL, 0u) ||
        !control_wait_irq(inst, host, 3u, expected_q, 8u, 50000u)) {
        fprintf(stderr, "CD_REFERENCE_Q_GETLOCP_ERROR: lba=%u Q fields mismatch\n", fixture->lba);
        return 0;
    }
    return 1;
}

static int q_fixture_controls(void)
{
    static const uint8_t fallback_leadout[8] = {
        0x01u, 0x01u, 0x74u, 0x00u, 0x00u, 0x73u, 0x58u, 0x00u
    };
    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = reference_alloc;
    config.allocator.free = reference_free;
    schway *inst = NULL;
    disc_host host;
    if (schway_create(&config, &inst) != SCHWAY_STATUS_OK || inst == NULL ||
        !attach_disc(inst, &host)) {
        schway_destroy(inst);
        fputs("CD_REFERENCE_Q_CORE_ERROR: cannot create the Q controls\n", stderr);
        return 0;
    }
    for (size_t i = 0u; i < sizeof q_fixtures / sizeof q_fixtures[0]; i++) {
        if (i != 0u) {
            schway_destroy(inst);
            inst = NULL;
            if (schway_create(&config, &inst) != SCHWAY_STATUS_OK || inst == NULL ||
                !attach_disc(inst, &host)) {
                schway_destroy(inst);
                fputs("CD_REFERENCE_Q_CORE_ERROR: cannot isolate Q fixture\n", stderr);
                return 0;
            }
        }
        if (!control_q_fixture(&host, inst, &q_fixtures[i], q_fixtures[i].getlocp, 0u)) {
            schway_destroy(inst);
            return 0;
        }
        if (i == 2u) {
            size_t state_size = schway_state_size(inst);
            uint8_t *state = (uint8_t *)malloc(state_size);
            if (state == NULL || schway_save_state(inst, state, state_size) != SCHWAY_STATUS_OK) {
                free(state);
                schway_destroy(inst);
                fputs("CD_REFERENCE_Q_STATE_ERROR: cannot save the current Q record\n", stderr);
                return 0;
            }
            if (!control_q_fixture(&host, inst, &q_fixtures[0], q_fixtures[0].getlocp, 0x02u) ||
                schway_load_state(inst, state, state_size) != SCHWAY_STATUS_OK ||
                !control_command(inst, 0x11u, NULL, 0u) ||
                !control_wait_irq(inst, &host, 3u, q_fixtures[2].getlocp, 8u, 50000u)) {
                free(state);
                schway_destroy(inst);
                fputs("CD_REFERENCE_Q_STATE_ERROR: saved Q record did not survive restore\n", stderr);
                return 0;
            }
            free(state);
        }
    }
    schway_destroy(inst);
    inst = NULL;
    if (schway_create(&config, &inst) != SCHWAY_STATUS_OK || inst == NULL ||
        !attach_disc(inst, &host) ||
        !control_q_fixture(&host, inst, &q_fixtures[0], q_fixtures[0].getlocp, 0u)) {
        schway_destroy(inst);
        fputs("CD_REFERENCE_Q_STATE_ERROR: cannot seed last-good Q record\n", stderr);
        return 0;
    }
    host.q_corruption = 1u;
    if (!control_q_fixture(&host, inst, &q_fixtures[2], q_fixtures[0].getlocp, 0x02u)) {
        schway_destroy(inst);
        fputs("CD_REFERENCE_Q_STATE_ERROR: invalid Q replaced last-good record\n", stderr);
        return 0;
    }
    for (uint32_t corruption = 1u; corruption <= 4u; corruption++) {
        schway_destroy(inst);
        inst = NULL;
        if (schway_create(&config, &inst) != SCHWAY_STATUS_OK || inst == NULL ||
            !attach_disc(inst, &host)) {
            schway_destroy(inst);
            fputs("CD_REFERENCE_Q_CORE_ERROR: cannot recreate invalid-Q control\n", stderr);
            return 0;
        }
        host.q_corruption = corruption;
        const q_fixture *fixture = corruption == 3u ? &q_fixtures[1] : &q_fixtures[2];
        static const uint8_t leadin_track00[8] = {
            0x00u, 0x00u, 0x00u, 0x00u, 0x33u, 0x00u, 0x01u, 0x41u
        };
        const uint8_t *expected = corruption == 3u ? leadin_track00 : fallback_leadout;
        if (!control_q_fixture(&host, inst, fixture, expected, 0u)) {
            schway_destroy(inst);
            return 0;
        }
    }
    schway_destroy(inst);
    puts("CD_REFERENCE_Q_MATCH: three pinned rows, malformed-Q fallback, and state restore matched");
    return 1;
}

static int run_controls(const char *getloc_log_path)
{
    char reference[2048];
    char valid[2048];
    char shifted[2048];
    char malformed[2048];
    size_t ref_size = 0u;
    size_t valid_size = 0u;
    size_t shifted_size = 0u;
    size_t malformed_size = 0u;
    disc_host host;
    memset(&host, 0, sizeof host);
    host.interval_count[0] = 5u;
    host.interval_count[1] = 5u;
    for (uint32_t speed = 0u; speed < 2u; speed++) {
        uint32_t selected = speed == 0u ? 451584u : 225792u;
        uint32_t reference_min = speed == 0u ? 416216u : 186552u;
        uint32_t reference_max = speed == 0u ? 468808u : 246800u;
        uint32_t reference_avg = speed == 0u ? 446040u : 222222u;
        host.interval_count[speed] = 5u;
        for (uint32_t i = 0u; i < 5u; i++) {
            host.intervals[speed][i] = selected;
            if (!append_timing_record(reference, sizeof reference, &ref_size, (int)speed,
                                      reference_min, reference_max, reference_avg) ||
                !append_timing_record(valid, sizeof valid, &valid_size, (int)speed,
                                      selected, selected, selected) ||
                !append_timing_record(shifted, sizeof shifted, &shifted_size, (int)speed,
                                      i == 0u && speed == 0u ? 999999u : selected,
                                      i == 0u && speed == 0u ? 999999u : selected,
                                      i == 0u && speed == 0u ? 999999u : selected)) {
                fputs("CD_REFERENCE_CONTROL_ERROR: cannot build local timing records\n", stderr);
                return 1;
            }
            if (i != 4u &&
                !append_timing_record(malformed, sizeof malformed, &malformed_size,
                                      (int)speed, selected, selected, selected)) {
                fputs("CD_REFERENCE_CONTROL_ERROR: cannot build malformed timing record\n", stderr);
                return 1;
            }
        }
    }
    static const uint8_t getloc_good[] = "GetStat -> 0x%02X\nGetlocL failed, IRQ = %d, status = 0x%02X\n";
    static const uint8_t getloc_reordered[] = "GetlocL failed, IRQ = %d, status = 0x%02X\nGetStat -> 0x%02X\n";
    disc_host overflowed = host;
    overflowed.interval_overflow[0] = 1u;
    if (!timing_matches((const uint8_t *)valid, valid_size,
                        (const uint8_t *)reference, ref_size, &host) ||
        timing_matches((const uint8_t *)shifted, shifted_size,
                       (const uint8_t *)reference, ref_size, &host) ||
        timing_matches((const uint8_t *)malformed, malformed_size,
                       (const uint8_t *)reference, ref_size, &host) ||
        timing_matches((const uint8_t *)valid, valid_size,
                       (const uint8_t *)reference, ref_size, &overflowed) ||
        !ordered_pair(getloc_good, sizeof getloc_good - 1u) ||
        ordered_pair(getloc_reordered, sizeof getloc_reordered - 1u) ||
        !timing_interval_control((const uint8_t *)valid, valid_size,
                                 (const uint8_t *)reference, ref_size, &host, 101u, 1u) ||
        !timing_interval_control((const uint8_t *)valid, valid_size,
                                 (const uint8_t *)reference, ref_size, &host, 16u, 0u) ||
        !getloc_byte_controls(getloc_log_path) || !q_fixture_controls()) {
        fputs("CD_REFERENCE_CONTROL_ERROR: malformed, shifted, or reordered data was accepted\n", stderr);
        return 1;
    }
    puts("CD_REFERENCE_CONTROLS_MATCH: malformed timing and reordered Getloc records rejected");
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "controls") == 0) return run_controls(argv[2]);
    if (argc == 4 && (strcmp(argv[1], "timing") == 0 || strcmp(argv[1], "getloc") == 0))
        return load_and_run(argv[1], argv[2], argv[3]);
    fputs("CD_REFERENCE_USAGE_ERROR: expected timing|getloc EXE LOG, or controls\n", stderr);
    return 2;
}
