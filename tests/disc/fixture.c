/* Real OpenBIOS boot, CD-ROM controller responses and guest DMA3 sector data. */
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "disc_image.h"
#include "internal.h"
#include "sha256.h"

#define DISC_LBA_FIRST 32u
#define DISC_SECTOR_COUNT 16u
#define DISC_SECTOR_BYTES 2048u
#define DISC_DATA_BYTES (DISC_SECTOR_COUNT * DISC_SECTOR_BYTES)
#define DISC_BUFFER_OFFSET 0x40000u
#define MDEC_OUTPUT_OFFSET 0x61000u
#define AV_CAPTURE_FRAME 900u
#define AV_FRAME_BYTES (16u * 16u * 3u)
#define MAILBOX_OFFSET 0x10000u
#define MAILBOX_RECORDS_OFFSET 0x10u
#define MAILBOX_SIZE 0x100u

struct fixture_disc {
    schway_disc provider;
    schway *active_instance;
    uint32_t inside_run;
    uint32_t outside_run_calls;
    uint32_t bad_q_calls;
    uint32_t bad_q_hits;
    uint32_t read_calls;
    uint32_t sector_reads[256];
    uint32_t bad_q_seen[64];
    uint32_t xa_mode_seen, xa_file_seen, xa_channel_seen;
    uint32_t xa_sector_shape_seen;
};

struct audio_capture {
    int16_t *samples;
    size_t pairs;
    size_t capacity;
    uint32_t saw_xa_decode;
    uint32_t saw_xa_nonzero_queue;
    uint32_t saw_cdda_nonzero_queue;
    uint32_t nonzero;
};

static const uint32_t fixture_bad_q_lbas[] = {
    32u, 33u, 34u, 35u, 36u, 37u, 38u, 39u,
};

static void *fixture_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void fixture_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static uint8_t *read_file(const char *path, size_t *size)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL || fseek(file, 0L, SEEK_END) != 0) {
        if (file != NULL) (void)fclose(file);
        return NULL;
    }
    long end = ftell(file);
    if (end <= 0L || fseek(file, 0L, SEEK_SET) != 0) {
        (void)fclose(file);
        return NULL;
    }
    uint8_t *bytes = (uint8_t *)malloc((size_t)end);
    if (bytes == NULL) {
        (void)fclose(file);
        return NULL;
    }
    if (fread(bytes, 1u, (size_t)end, file) != (size_t)end || fclose(file) != 0) {
        free(bytes);
        return NULL;
    }
    *size = (size_t)end;
    return bytes;
}

static uint32_t fixture_read_sector(void *user, uint32_t lba,
                                    uint8_t sector[2352], uint8_t subcode[96])
{
    struct fixture_disc *host = (struct fixture_disc *)user;
    if (host->inside_run == 0u) host->outside_run_calls++;
    host->read_calls++;
    if (lba < sizeof host->sector_reads / sizeof host->sector_reads[0]) {
        host->sector_reads[lba]++;
    }
    uint32_t ok = host->provider.read_sector(host->provider.user, lba, sector, subcode);
    if (ok != 0u && lba >= 51u && lba <= 53u && host->active_instance != NULL) {
        host->xa_mode_seen = host->active_instance->cdrom.mode;
        host->xa_file_seen = host->active_instance->cdrom.file;
        host->xa_channel_seen = host->active_instance->cdrom.channel;
        host->xa_sector_shape_seen = sector[15] == 2u && sector[16] == 7u &&
                                     sector[17] == 3u && (sector[18] & 0x44u) == 0x44u &&
                                     sector[19] == 1u &&
                                     memcmp(&sector[16], &sector[20], 4u) == 0;
    }
    return ok;
}

static uint32_t fixture_read_toc(void *user, schway_toc *out)
{
    struct fixture_disc *host = (struct fixture_disc *)user;
    return host->provider.toc(host->provider.user, out);
}

static uint32_t fixture_bad_q(void *user, uint32_t lba)
{
    struct fixture_disc *host = (struct fixture_disc *)user;
    if (host->inside_run == 0u) host->outside_run_calls++;
    host->bad_q_calls++;
    if (lba < 64u) host->bad_q_seen[lba]++;
    for (size_t i = 0u; i < sizeof fixture_bad_q_lbas / sizeof fixture_bad_q_lbas[0]; i++) {
        if (fixture_bad_q_lbas[i] == lba) {
            host->bad_q_hits++;
            return 1u;
        }
    }
    return 0u;
}

static int identity_for_disc(struct fixture_disc *host, schway_disc *disc)
{
    static const char serial[] = "SWAY_000.01";
    schway__sha256 hash;
    uint8_t sector[2352];
    uint8_t subcode[96];
    schway__sha256_init(&hash);
    for (uint32_t lba = 0u; lba < 16u; lba++) {
        if (host->provider.read_sector(host->provider.user, lba, sector, subcode) == 0u) {
            return 0;
        }
        schway__sha256_update(&hash, sector + 24u, DISC_SECTOR_BYTES);
    }
    schway__sha256_update(&hash, serial, sizeof serial - 1u);
    disc->identity_size = 32u;
    schway__sha256_final(&hash, disc->identity_sha256);
    return 1;
}

static int expected_bios_digest(const char *path, uint8_t expected[32])
{
    FILE *file = fopen(path, "rb");
    char hex[66];
    if (file == NULL) return 0;
    int ok = fgets(hex, (int)sizeof hex, file) != NULL && fclose(file) == 0;
    if (!ok || strlen(hex) != 65u || hex[64] != '\n') return 0;
    hex[64] = '\0';
    for (uint32_t i = 0u; i < 32u; i++) {
        unsigned int value = 0u;
        if (sscanf(hex + i * 2u, "%2x", &value) != 1) return 0;
        expected[i] = (uint8_t)value;
    }
    return 1;
}

static schway *fixture_create(const char *bios_path, const uint8_t expected_digest[32],
                              const schway_disc *disc)
{
    size_t bios_size = 0u;
    uint8_t *bios = read_file(bios_path, &bios_size);
    if (bios == NULL || bios_size != SCHWAY_BIOS_SIZE) {
        fprintf(stderr, "disc fixture: OpenBIOS input has the wrong size\n");
        free(bios);
        return NULL;
    }
    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = fixture_alloc;
    config.allocator.free = fixture_free;
    schway *inst = NULL;
    schway_status status = schway_create(&config, &inst);
    if (status != SCHWAY_STATUS_OK || inst == NULL) {
        fprintf(stderr, "disc fixture: create failed: %s\n", schway_status_string(status));
        free(bios);
        return NULL;
    }
    schway_profile profile;
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = SCHWAY_REGION_NTSC_U;
    profile.flags = SCHWAY_PROFILE_CD_LICENSED | SCHWAY_PROFILE_DEBUG_PORT;
    status = schway_set_profile(inst, &profile);
    if (status == SCHWAY_STATUS_OK) status = schway_load_bios(inst, bios, bios_size);
    free(bios);
    if (status == SCHWAY_STATUS_OK) {
        schway_bios_info info;
        memset(&info, 0, sizeof info);
        info.size = (uint32_t)sizeof info;
        status = schway_get_bios_info(inst, &info);
        if (status == SCHWAY_STATUS_OK && memcmp(info.sha256, expected_digest, 32u) != 0) {
            status = SCHWAY_STATUS_ERR_ARGUMENT;
        }
    }
    if (status == SCHWAY_STATUS_OK) status = schway_open_disc(inst, disc);
    if (status == SCHWAY_STATUS_OK &&
        (schway__cdrom_sector_track_type_at(inst, 16u) != SCHWAY_DISC_TRACK_MODE2 ||
         schway__cdrom_sector_track_type_at(inst, DISC_LBA_FIRST) != SCHWAY_DISC_TRACK_MODE2)) {
        fprintf(stderr, "disc fixture: TOC does not classify boot/read sectors as Mode 2\n");
        status = SCHWAY_STATUS_ERR_ARGUMENT;
    }
    if (status != SCHWAY_STATUS_OK) {
        fprintf(stderr, "disc fixture: setup failed: %s\n", schway_status_string(status));
        schway_destroy(inst);
        return NULL;
    }
    return inst;
}

static int capture_audio_frame(schway *inst, struct audio_capture *capture)
{
    int16_t samples[SCHWAY__AUDIO_RING_PAIRS * 2u];
    schway_audio audio;
    memset(&audio, 0, sizeof audio);
    audio.size = (uint32_t)sizeof audio;
    audio.samples = samples;
    audio.capacity = SCHWAY__AUDIO_RING_PAIRS;
    if (schway_get_audio(inst, &audio) != SCHWAY_STATUS_OK || audio.dropped != 0u) {
        fprintf(stderr, "disc fixture: native PCM drain failed or dropped samples\n");
        return 0;
    }
    if (capture->pairs + audio.written > capture->capacity) {
        size_t capacity = capture->capacity == 0u ? 16384u : capture->capacity;
        while (capacity < capture->pairs + audio.written) capacity *= 2u;
        if (capacity > SIZE_MAX / (2u * sizeof *capture->samples)) return 0;
        int16_t *grown = (int16_t *)realloc(capture->samples,
                                             capacity * 2u * sizeof *capture->samples);
        if (grown == NULL) return 0;
        capture->samples = grown;
        capture->capacity = capacity;
    }
    for (uint32_t i = 0u; i < audio.written * 2u; i++) {
        capture->samples[capture->pairs * 2u + i] = samples[i];
        if (samples[i] != 0) capture->nonzero = 1u;
    }
    capture->pairs += audio.written;
    for (uint32_t i = 0u; i < inst->cdrom.xa_audio_count; i++) {
        uint32_t slot = (inst->cdrom.xa_audio_head + i) % SCHWAY__CDROM_XA_AUDIO_CAP;
        if (inst->cdrom.xa_audio[slot][0] != 0u ||
            inst->cdrom.xa_audio[slot][1] != 0u) {
            capture->saw_xa_nonzero_queue = 1u;
        }
    }
    for (uint32_t channel = 0u; channel < 2u; channel++) {
        for (uint32_t tap = 0u; tap < 2u; tap++) {
            if (inst->cdrom.xa_adpcm_history[channel][tap] != 0) capture->saw_xa_decode = 1u;
        }
    }
    for (uint32_t i = 0u; i < inst->cdrom.audio_count; i++) {
        uint32_t slot = (inst->cdrom.audio_head + i) % SCHWAY__CDROM_AUDIO_CAP;
        if (inst->cdrom.audio[slot][0] != 0u || inst->cdrom.audio[slot][1] != 0u) {
            capture->saw_cdda_nonzero_queue = 1u;
            break;
        }
    }
    return 1;
}

static int run_frames(schway *inst, struct fixture_disc *host, uint32_t first,
                      uint32_t last, uint32_t press_frame, struct audio_capture *capture)
{
    for (uint32_t frame = first; frame <= last; frame++) {
        schway_input input;
        memset(&input, 0, sizeof input);
        input.size = (uint32_t)sizeof input;
        input.device[0] = SCHWAY_INPUT_DEVICE_DIGITAL;
        if (frame == press_frame) input.buttons[0] = UINT16_C(0x4000);
        schway_status status = schway_set_input(inst, &input);
        if (status != SCHWAY_STATUS_OK) return 0;
        schway_run_result result;
        memset(&result, 0, sizeof result);
        result.size = (uint32_t)sizeof result;
        host->inside_run = 1u;
        host->active_instance = inst;
        status = schway_run_frame(inst, &result);
        host->active_instance = NULL;
        host->inside_run = 0u;
        if (status != SCHWAY_STATUS_OK || result.stop == SCHWAY_STOP_FAULT ||
            result.fault_present != 0u) {
            fprintf(stderr,
                    "disc fixture: frame %u stopped with %s/%u fault=%u pc=%08x op=%08x addr=%08x\n",
                    frame, schway_status_string(status), result.stop, inst->fault.kind,
                    inst->fault.pc, inst->fault.opcode, inst->fault.addr);
            return 0;
        }
        if (capture != NULL && !capture_audio_frame(inst, capture)) return 0;
        const uint8_t *mailbox = inst->bus.ram + MAILBOX_OFFSET;
        if (memcmp(mailbox, "SWMB", 4u) == 0 && mailbox[8] == 0xFFu) {
            fprintf(stderr, "disc fixture: guest rejected the STR frame header at frame %u\n", frame);
            return 0;
        }
    }
    return 1;
}

static const uint8_t *fixture_ram(schway *inst)
{
    schway_span ram;
    if (schway_get_memory(inst, SCHWAY_MEMORY_RAM, &ram) != SCHWAY_STATUS_OK ||
        ram.data == NULL || ram.size < DISC_BUFFER_OFFSET + DISC_DATA_BYTES) {
        return NULL;
    }
    return (const uint8_t *)ram.data;
}

static int guest_started(schway *inst)
{
    const uint8_t *ram = fixture_ram(inst);
    if (ram == NULL) return 0;
    const uint8_t *mailbox = ram + MAILBOX_OFFSET;
    uint32_t first = (uint32_t)mailbox[MAILBOX_RECORDS_OFFSET] |
                     ((uint32_t)mailbox[MAILBOX_RECORDS_OFFSET + 1u] << 8) |
                     ((uint32_t)mailbox[MAILBOX_RECORDS_OFFSET + 2u] << 16) |
                     ((uint32_t)mailbox[MAILBOX_RECORDS_OFFSET + 3u] << 24);
    uint32_t value = (uint32_t)mailbox[MAILBOX_RECORDS_OFFSET + 4u] |
                     ((uint32_t)mailbox[MAILBOX_RECORDS_OFFSET + 5u] << 8) |
                     ((uint32_t)mailbox[MAILBOX_RECORDS_OFFSET + 6u] << 16) |
                     ((uint32_t)mailbox[MAILBOX_RECORDS_OFFSET + 7u] << 24);
    return memcmp(mailbox, "SWMB", 4u) == 0 && mailbox[10] >= 1u &&
           first == UINT32_C(0x00010001) && value == UINT32_C(0x424F4F54);
}

static uint32_t read_le32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static int load_expected_records(const char *path, uint32_t expected[8])
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) return 0;
    char line[1024];
    uint32_t seen = 0u;
    while (fgets(line, (int)sizeof line, file) != NULL) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        char *fields[4];
        char *cursor = line;
        int malformed = 0;
        for (size_t i = 0u; i < 4u; i++) {
            fields[i] = cursor;
            char *comma = strchr(cursor, ',');
            if (i < 3u && comma == NULL) {
                malformed = 1;
                break;
            }
            if (comma != NULL) {
                *comma = '\0';
                cursor = comma + 1;
            }
        }
        if (malformed != 0 || strcmp(fields[0], "disc_boot") != 0) continue;
        if (fields[1][0] < '0' || fields[1][0] > '9') continue;
        char *id_end = NULL;
        unsigned long id = strtoul(fields[1], &id_end, 10);
        if (id_end == fields[1] || *id_end != '\0' || id == 0ul || id > 8ul ||
            strcmp(fields[2], "*") != 0 || strlen(fields[3]) != 8u) {
            (void)fclose(file);
            return 0;
        }
        char *value_end = NULL;
        unsigned long value = strtoul(fields[3], &value_end, 16);
        if (value_end == fields[3] || *value_end != '\0' || value > UINT32_MAX ||
            (seen & (UINT32_C(1) << (id - 1ul))) != 0u) {
            (void)fclose(file);
            return 0;
        }
        expected[id - 1ul] = (uint32_t)value;
        seen |= UINT32_C(1) << (id - 1ul);
    }
    int ok = fclose(file) == 0 && seen == UINT32_C(0xFF);
    return ok;
}

struct av_expectations {
    char audio_none[65];
    char frame_none[65];
    char audio_press[65];
    char frame_press[65];
};

static int copy_hash(const char *value, char output[65])
{
    if (strlen(value) != 64u) return 0;
    for (uint32_t i = 0u; i < 64u; i++) {
        char c = value[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return 0;
    }
    memcpy(output, value, 65u);
    return 1;
}

static int load_expected_av(const char *path, struct av_expectations *expected)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) return 0;
    memset(expected, 0, sizeof *expected);
    char line[1024];
    uint32_t seen = 0u;
    while (fgets(line, (int)sizeof line, file) != NULL) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        char *fields[4];
        char *cursor = line;
        int malformed = 0;
        for (size_t i = 0u; i < 4u; i++) {
            fields[i] = cursor;
            char *comma = strchr(cursor, ',');
            if (i < 3u && comma == NULL) {
                malformed = 1;
                break;
            }
            if (comma != NULL) {
                *comma = '\0';
                cursor = comma + 1;
            }
        }
        if (malformed != 0 || strcmp(fields[0], "disc_boot") != 0 ||
            (strncmp(fields[1], "audio-900.", 10u) != 0 &&
             strncmp(fields[1], "frame-900.", 10u) != 0)) {
            continue;
        }
        uint32_t bit;
        char *destination;
        if (strcmp(fields[1], "audio-900.none") == 0) {
            bit = 1u;
            destination = expected->audio_none;
        } else if (strcmp(fields[1], "frame-900.none") == 0) {
            bit = 2u;
            destination = expected->frame_none;
        } else if (strcmp(fields[1], "audio-900.press300") == 0) {
            bit = 4u;
            destination = expected->audio_press;
        } else if (strcmp(fields[1], "frame-900.press300") == 0) {
            bit = 8u;
            destination = expected->frame_press;
        } else {
            (void)fclose(file);
            return 0;
        }
        if (strcmp(fields[2], "ntsc-u") != 0 || copy_hash(fields[3], destination) == 0 ||
            (seen & bit) != 0u) {
            (void)fclose(file);
            return 0;
        }
        seen |= bit;
    }
    return fclose(file) == 0 && seen == 15u;
}

static int mailbox_records_are_unique(const uint8_t *mailbox)
{
    uint8_t seen[9] = {0u};
    for (uint32_t i = 0u; i < mailbox[10]; i++) {
        const uint8_t *record = mailbox + MAILBOX_RECORDS_OFFSET + i * 8u;
        uint32_t key = read_le32(record);
        uint32_t id = key & UINT32_C(0xFFFF);
        if ((key >> 16) != 1u || id == 0u || id > 8u || seen[id] != 0u) return 0;
        seen[id] = 1u;
    }
    for (uint32_t id = 1u; id <= 8u; id++) {
        if (seen[id] == 0u) return 0;
    }
    return 1;
}

static uint32_t mailbox_record(const uint8_t *mailbox, uint16_t id, int *found)
{
    uint32_t count = mailbox[10];
    for (uint32_t i = 0u; i < count && i < 30u; i++) {
        const uint8_t *record = mailbox + MAILBOX_RECORDS_OFFSET + i * 8u;
        uint32_t key = read_le32(record);
        if ((key & 0xFFFFu) == id && (key >> 16) == 1u) {
            *found = 1;
            return read_le32(record + 4u);
        }
    }
    *found = 0;
    return 0u;
}

static uint32_t expected_crc32(const uint8_t *bytes, size_t size)
{
    uint32_t crc = UINT32_MAX;
    for (size_t i = 0u; i < size; i++) {
        crc ^= bytes[i];
        for (uint32_t bit = 0u; bit < 8u; bit++) {
            crc = (crc >> 1) ^ ((crc & 1u) != 0u ? UINT32_C(0xEDB88320) : 0u);
        }
    }
    return ~crc;
}

static void digest_audio(const struct audio_capture *capture, char digest[65])
{
    schway__sha256 state;
    uint8_t bytes[1024];
    uint8_t output[32];
    size_t used = 0u;
    schway__sha256_init(&state);
    for (size_t sample = 0u; sample < capture->pairs * 2u; sample++) {
        uint16_t value = (uint16_t)capture->samples[sample];
        bytes[used++] = (uint8_t)value;
        bytes[used++] = (uint8_t)(value >> 8);
        if (used == sizeof bytes) {
            schway__sha256_update(&state, bytes, used);
            used = 0u;
        }
    }
    if (used != 0u) schway__sha256_update(&state, bytes, used);
    schway__sha256_final(&state, output);
    schway__sha256_hex(output, digest);
}

static void digest_frame(const uint8_t pixels[AV_FRAME_BYTES], char digest[65])
{
    uint8_t output[32];
    schway__sha256 hash;
    schway__sha256_init(&hash);
    schway__sha256_update(&hash, pixels, AV_FRAME_BYTES);
    schway__sha256_final(&hash, output);
    schway__sha256_hex(output, digest);
}

static int capture_frame(schway *inst, uint8_t pixels[AV_FRAME_BYTES])
{
    schway_video video;
    memset(&video, 0, sizeof video);
    video.size = (uint32_t)sizeof video;
    if (schway_get_video(inst, &video) != SCHWAY_STATUS_OK || video.pixels == NULL ||
        video.enabled == 0u || video.width != 16u || video.height != 16u ||
        video.pitch_bytes != 48u || video.bpp != 24u || video.rgb24 == 0u) {
        fprintf(stderr, "disc fixture: decoded frame is not a 16x16 native RGB24 image\n");
        return 0;
    }
    const uint8_t *ram = fixture_ram(inst);
    if (ram == NULL || memcmp(video.pixels, ram + MDEC_OUTPUT_OFFSET, AV_FRAME_BYTES) != 0) {
        fprintf(stderr, "disc fixture: displayed pixels differ from MDEC DMA output\n");
        return 0;
    }
    memcpy(pixels, video.pixels, AV_FRAME_BYTES);
    uint8_t min[3] = {UINT8_MAX, UINT8_MAX, UINT8_MAX};
    uint8_t max[3] = {0u, 0u, 0u};
    uint32_t channel_distinct = 0u;
    for (uint32_t i = 0u; i < AV_FRAME_BYTES; i += 3u) {
        for (uint32_t channel = 0u; channel < 3u; channel++) {
            uint8_t value = pixels[i + channel];
            if (value < min[channel]) min[channel] = value;
            if (value > max[channel]) max[channel] = value;
        }
        if (pixels[i] != pixels[i + 1u] || pixels[i + 1u] != pixels[i + 2u]) {
            channel_distinct = 1u;
        }
    }
    if (channel_distinct == 0u || min[0] == max[0] || min[1] == max[1] || min[2] == max[2]) {
        fprintf(stderr,
                "disc fixture: decoded image lost its per-channel color pattern "
                "(ranges R=%u..%u G=%u..%u B=%u..%u, first=%u/%u/%u)\n",
                min[0], max[0], min[1], max[1], min[2], max[2],
                pixels[0], pixels[1], pixels[2]);
        return 0;
    }
    return 1;
}

static void audio_capture_free(struct audio_capture *capture)
{
    free(capture->samples);
    memset(capture, 0, sizeof *capture);
}

static int derive_data(struct fixture_disc *host, uint8_t expected[DISC_DATA_BYTES],
                       uint32_t *crc)
{
    uint8_t sector[2352];
    uint8_t subcode[96];
    for (uint32_t i = 0u; i < DISC_SECTOR_COUNT; i++) {
        if (host->provider.read_sector(host->provider.user, DISC_LBA_FIRST + i,
                                       sector, subcode) == 0u) return 0;
        memcpy(expected + i * DISC_SECTOR_BYTES, sector + 24u, DISC_SECTOR_BYTES);
    }
    *crc = expected_crc32(expected, DISC_DATA_BYTES);
    return 1;
}

static uint8_t to_bcd(uint32_t value)
{
    value %= 100u;
    return (uint8_t)(((value / 10u) << 4) | (value % 10u));
}

static uint32_t expected_gettd(const schway_disc_track *track)
{
    uint64_t absolute = (uint64_t)track->index1 + 150u;
    uint32_t minute = (uint32_t)(absolute / (60u * 75u));
    uint32_t second = (uint32_t)((absolute / 75u) % 60u);
    return UINT32_C(0x02) | ((uint32_t)to_bcd(minute) << 8) |
           ((uint32_t)to_bcd(second) << 16);
}

static int verify_guest(schway *inst, struct fixture_disc *host,
                        const uint8_t expected_data[DISC_DATA_BYTES], uint32_t expected_crc,
                        const uint32_t expected_records[8],
                        uint8_t mailbox_copy[MAILBOX_SIZE])
{
    const uint8_t *ram = fixture_ram(inst);
    if (ram == NULL) return 0;
    const uint8_t *mailbox = ram + MAILBOX_OFFSET;
    if (memcmp(mailbox, "SWMB", 4u) != 0 || mailbox[8] != 0u || mailbox[10] != 8u) {
        fprintf(stderr, "disc fixture: mailbox incomplete (status=%02x records=%u)\n",
                mailbox[8], mailbox[10]);
        fprintf(stderr,
                "disc fixture: pc=%08x ra=%08x frame=%llu cd=%02x bank=%u cmd=%02x/%02x "
                "pending=%u params=%u results=%u/%u/%02x irqs=%u/%u/%u mask=%02x irq=%03x/%03x\n",
                inst->cpu.pc, inst->cpu.r[31], (unsigned long long)inst->frame,
                inst->cdrom.status, inst->cdrom.bank, inst->cdrom.last_command,
                inst->cdrom.command, inst->cdrom.command_pending, inst->cdrom.parameter_count,
                inst->cdrom.result_head, inst->cdrom.result_count,
                inst->cdrom.result_count == 0u ? 0u : inst->cdrom.result[inst->cdrom.result_head],
                inst->cdrom.interrupt_head, inst->cdrom.interrupt_count,
                inst->cdrom.interrupt_count == 0u ? 0u :
                    inst->cdrom.interrupt[inst->cdrom.interrupt_head],
                inst->cdrom.interrupt_mask, inst->irq.stat, inst->irq.mask);
        fprintf(stderr,
                "disc fixture: DMA0/1 chcr=%08x/%08x bcr=%08x/%08x remain=%u/%u MDEC "
                "active=%u command=%08x words=%u input=%u output=%u block=%u deadline=%llu\n",
                inst->dma.chcr[0], inst->dma.chcr[1], inst->dma.bcr[0], inst->dma.bcr[1],
                inst->dma.remain[0], inst->dma.remain[1], inst->mdec.command_active,
                inst->mdec.command, inst->mdec.words_remaining, inst->mdec.input_count,
                inst->mdec.output_count, inst->mdec.block_pending,
                (unsigned long long)inst->mdec.block_deadline);
        fprintf(stderr,
                "disc fixture: read target=%u host LBA 51..63=%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u\n",
                inst->cdrom.target_lba, host->sector_reads[51u], host->sector_reads[52u],
                host->sector_reads[53u], host->sector_reads[54u], host->sector_reads[55u],
                host->sector_reads[56u], host->sector_reads[57u], host->sector_reads[58u],
                host->sector_reads[59u], host->sector_reads[60u], host->sector_reads[61u],
                host->sector_reads[62u], host->sector_reads[63u]);
        return 0;
    }
    uint32_t mailbox_crc = schway__crc32_update(0u, mailbox + MAILBOX_RECORDS_OFFSET,
                                                MAILBOX_SIZE - MAILBOX_RECORDS_OFFSET);
    if (mailbox_crc != read_le32(mailbox + 4u)) {
        fprintf(stderr, "disc fixture: mailbox CRC mismatch\n");
        return 0;
    }
    if (!mailbox_records_are_unique(mailbox)) {
        fprintf(stderr, "disc fixture: mailbox has duplicate, unknown or missing record IDs\n");
        return 0;
    }
    static const uint32_t expected_id_words[2] = {
        UINT32_C(0x00200002), UINT32_C(0x41454353),
    };
    schway_toc toc;
    memset(&toc, 0, sizeof toc);
    toc.size = (uint32_t)sizeof toc;
    if (host->provider.toc(host->provider.user, &toc) == 0u || toc.track_count != 2u) {
        fprintf(stderr, "disc fixture: unexpected TOC track count\n");
        return 0;
    }
    const uint32_t derived_values[8] = {
        UINT32_C(0x424F4F54),
        UINT32_C(0x00020102),
        expected_gettd(&toc.tracks[0]),
        expected_gettd(&toc.tracks[1]),
        expected_id_words[0],
        expected_id_words[1],
        DISC_SECTOR_COUNT,
        expected_crc,
    };
    for (uint16_t id = 1u; id <= 8u; id++) {
        int found = 0;
        uint32_t got = mailbox_record(mailbox, id, &found);
        if (expected_records[id - 1u] != derived_values[id - 1u]) {
            fprintf(stderr, "disc fixture: CSV row %u disagrees with independent source value\n",
                    (unsigned)id);
            return 0;
        }
        if (found == 0 || got != expected_records[id - 1u]) {
            fprintf(stderr, "disc fixture: record %u got %08x expected %08x\n",
                    (unsigned)id, got, expected_records[id - 1u]);
            return 0;
        }
    }
    if (memcmp(ram + DISC_BUFFER_OFFSET, expected_data, DISC_DATA_BYTES) != 0) {
        fprintf(stderr, "disc fixture: DMA3 sector data differs from generated image\n");
        return 0;
    }
    for (uint32_t lba = DISC_LBA_FIRST; lba < DISC_LBA_FIRST + DISC_SECTOR_COUNT; lba++) {
        if (host->sector_reads[lba] == 0u) {
            fprintf(stderr, "disc fixture: host provider did not read LBA %u\n", lba);
            return 0;
        }
    }
    for (size_t i = 0u; i < sizeof fixture_bad_q_lbas / sizeof fixture_bad_q_lbas[0]; i++) {
        if (host->bad_q_seen[fixture_bad_q_lbas[i]] == 0u) {
            fprintf(stderr, "disc fixture: bad-Q callback missed LBA %u\n", fixture_bad_q_lbas[i]);
            return 0;
        }
    }
    if (host->bad_q_hits < sizeof fixture_bad_q_lbas / sizeof fixture_bad_q_lbas[0] ||
        host->outside_run_calls != 0u) {
        fprintf(stderr, "disc fixture: bad-Q or callback run-boundary check failed\n");
        return 0;
    }
    memcpy(mailbox_copy, mailbox, MAILBOX_SIZE);
    return 1;
}

static int run_boot_case(const char *bios_path, const uint8_t digest[32],
                         const schway_disc *disc, struct fixture_disc *host)
{
    schway *inst = fixture_create(bios_path, digest, disc);
    if (inst == NULL) return 0;
    int ok = run_frames(inst, host, 1u, 600u, 0u, NULL) && guest_started(inst);
    if (ok == 0) {
        const uint8_t *ram = fixture_ram(inst);
        const uint8_t *mailbox = ram == NULL ? NULL : ram + MAILBOX_OFFSET;
        fprintf(stderr,
                "disc fixture: SYSTEM.CNF guest did not boot by frame 600 "
                "(pc=%08x frames=%llu reads=%u bios=%u exe=%u cd=%02x target=%u "
                "mailbox=%02x%02x%02x%02x)\n",
                inst->cpu.pc, (unsigned long long)inst->frame, host->read_calls,
                inst->bus.bios_loaded, inst->exe_loaded, inst->cdrom.status,
                inst->cdrom.target_lba,
                mailbox == NULL ? 0u : mailbox[0], mailbox == NULL ? 0u : mailbox[1],
                mailbox == NULL ? 0u : mailbox[2], mailbox == NULL ? 0u : mailbox[3]);
        fprintf(stderr,
                "disc fixture: fault=%u at %08x address=%08x; guest mailbox=%02x%02x%02x%02x\n",
                inst->fault.kind, inst->fault.pc, inst->fault.addr,
                mailbox == NULL ? 0u : mailbox[0], mailbox == NULL ? 0u : mailbox[1],
                mailbox == NULL ? 0u : mailbox[2], mailbox == NULL ? 0u : mailbox[3]);
    }
    schway_destroy(inst);
    return ok;
}

static int run_read_case(const char *bios_path, const uint8_t digest[32],
                         const schway_disc *disc, struct fixture_disc *host,
                         const uint32_t expected_records[8])
{
    uint8_t expected_data[DISC_DATA_BYTES];
    uint32_t expected_crc = 0u;
    if (!derive_data(host, expected_data, &expected_crc)) {
        fprintf(stderr, "disc fixture: could not derive generated-sector reference\n");
        return 0;
    }
    schway *inst = fixture_create(bios_path, digest, disc);
    if (inst == NULL) return 0;
    size_t state_size = schway_state_size(inst);
    uint8_t *checkpoint = (uint8_t *)malloc(state_size);
    uint8_t *final_state = (uint8_t *)malloc(state_size);
    uint8_t *replay_state = (uint8_t *)malloc(state_size);
    uint8_t first_mailbox[MAILBOX_SIZE];
    uint8_t press_mailbox[MAILBOX_SIZE];
    int ok = checkpoint != NULL && final_state != NULL && replay_state != NULL;
    if (ok == 0) {
        fprintf(stderr, "disc fixture: checkpoint allocation failed\n");
        goto cleanup;
    }
    ok = run_frames(inst, host, 1u, 300u, 0u, NULL) &&
         schway_save_state(inst, checkpoint, state_size) == SCHWAY_STATUS_OK &&
         run_frames(inst, host, 301u, 600u, 0u, NULL) && guest_started(inst) &&
         run_frames(inst, host, 601u, 900u, 0u, NULL) &&
         verify_guest(inst, host, expected_data, expected_crc, expected_records, first_mailbox) &&
         schway_save_state(inst, final_state, state_size) == SCHWAY_STATUS_OK;
    if (ok == 0) {
        fprintf(stderr, "disc fixture: no-input boot/read acceptance failed\n");
        goto cleanup;
    }
    if (schway_load_state(inst, checkpoint, state_size) != SCHWAY_STATUS_OK ||
        !run_frames(inst, host, 301u, 900u, 0u, NULL) ||
        !verify_guest(inst, host, expected_data, expected_crc, expected_records, press_mailbox) ||
        schway_save_state(inst, replay_state, state_size) != SCHWAY_STATUS_OK ||
        memcmp(first_mailbox, press_mailbox, MAILBOX_SIZE) != 0 ||
        memcmp(final_state, replay_state, state_size) != 0) {
        fprintf(stderr, "disc fixture: split-run replay diverged\n");
        ok = 0;
        goto cleanup;
    }
    schway_destroy(inst);
    inst = fixture_create(bios_path, digest, disc);
    if (inst == NULL) {
        ok = 0;
        goto cleanup;
    }
    ok = run_frames(inst, host, 1u, 600u, 300u, NULL) && guest_started(inst) &&
         run_frames(inst, host, 601u, 900u, 300u, NULL) &&
         verify_guest(inst, host, expected_data, expected_crc, expected_records, press_mailbox) &&
         memcmp(first_mailbox, press_mailbox, MAILBOX_SIZE) == 0;
    if (ok == 0) fprintf(stderr, "disc fixture: press@300 boot/read acceptance failed\n");

cleanup:
    schway_destroy(inst);
    free(checkpoint);
    free(final_state);
    free(replay_state);
    return ok;
}

static int audio_sector_coverage(const struct fixture_disc *host);

static int run_av_variant(const char *bios_path, const uint8_t digest[32],
                          const schway_disc *disc, struct fixture_disc *host,
                          const uint32_t expected_records[8],
                          const uint8_t expected_data[DISC_DATA_BYTES], uint32_t expected_crc,
                          uint32_t press_frame, uint32_t check_split,
                          struct audio_capture *capture, uint8_t pixels[AV_FRAME_BYTES])
{
    schway *inst = fixture_create(bios_path, digest, disc);
    if (inst == NULL) return 0;
    size_t state_size = schway_state_size(inst);
    uint8_t *checkpoint = (uint8_t *)malloc(state_size);
    uint8_t *final_state = (uint8_t *)malloc(state_size);
    uint8_t *replay_state = (uint8_t *)malloc(state_size);
    uint8_t mailbox[MAILBOX_SIZE];
    uint8_t replay_mailbox[MAILBOX_SIZE];
    uint8_t replay_pixels[AV_FRAME_BYTES];
    struct audio_capture replay_capture;
    memset(&replay_capture, 0, sizeof replay_capture);
    int ok = checkpoint != NULL && final_state != NULL && replay_state != NULL;
    size_t split_pairs = 0u;
    if (!ok) {
        fprintf(stderr, "disc fixture: AV checkpoint allocation failed\n");
        goto cleanup;
    }
    ok = run_frames(inst, host, 1u, 300u, press_frame, capture);
    split_pairs = capture->pairs;
    if (ok != 0 && check_split != 0u) {
        ok = schway_save_state(inst, checkpoint, state_size) == SCHWAY_STATUS_OK;
    }
    if (ok != 0) {
        ok = run_frames(inst, host, 301u, 600u, press_frame, capture) && guest_started(inst) &&
             run_frames(inst, host, 601u, AV_CAPTURE_FRAME, press_frame, capture) &&
             verify_guest(inst, host, expected_data, expected_crc, expected_records, mailbox) &&
             capture_frame(inst, pixels) &&
             schway_save_state(inst, final_state, state_size) == SCHWAY_STATUS_OK;
    }
    if (ok == 0) {
        fprintf(stderr, "disc fixture: AV acceptance failed (press frame %u)\n", press_frame);
        goto cleanup;
    }
    if (capture->pairs == 0u || capture->nonzero == 0u ||
        capture->saw_xa_decode == 0u || capture->saw_xa_nonzero_queue == 0u ||
        capture->saw_cdda_nonzero_queue == 0u || audio_sector_coverage(host) == 0 ||
        host->xa_mode_seen != 0x48u || host->xa_file_seen != 7u ||
        host->xa_channel_seen != 3u || host->xa_sector_shape_seen == 0u) {
        fprintf(stderr,
                "disc fixture: audio source predicate failed "
                "(pairs=%zu PCM=%u XA decode/nonzero queue=%u/%u CDDA nonzero queue=%u "
                "XA sectors=%u/%u/%u CDDA sectors=%u/%u/%u/%u/%u/%u/%u/%u "
                "mode/filter=%02x/%u/%u shape=%u)\n",
                capture->pairs, capture->nonzero, capture->saw_xa_decode,
                capture->saw_xa_nonzero_queue, capture->saw_cdda_nonzero_queue,
                host->sector_reads[51u], host->sector_reads[52u],
                host->sector_reads[53u],
                host->sector_reads[214u], host->sector_reads[215u],
                host->sector_reads[216u], host->sector_reads[217u],
                host->sector_reads[218u], host->sector_reads[219u],
                host->sector_reads[220u], host->sector_reads[221u],
                host->xa_mode_seen,
                host->xa_file_seen, host->xa_channel_seen, host->xa_sector_shape_seen);
        ok = 0;
        goto cleanup;
    }
    if (check_split != 0u) {
        ok = schway_load_state(inst, checkpoint, state_size) == SCHWAY_STATUS_OK &&
             run_frames(inst, host, 301u, AV_CAPTURE_FRAME, press_frame, &replay_capture) &&
             verify_guest(inst, host, expected_data, expected_crc, expected_records,
                          replay_mailbox) &&
             capture_frame(inst, replay_pixels) &&
             schway_save_state(inst, replay_state, state_size) == SCHWAY_STATUS_OK;
        if (ok == 0 || capture->pairs < split_pairs ||
            replay_capture.pairs != capture->pairs - split_pairs ||
            memcmp(replay_capture.samples, capture->samples + split_pairs * 2u,
                   replay_capture.pairs * 2u * sizeof *capture->samples) != 0 ||
            memcmp(replay_pixels, pixels, AV_FRAME_BYTES) != 0 ||
            memcmp(mailbox, replay_mailbox, MAILBOX_SIZE) != 0 ||
            memcmp(final_state, replay_state, state_size) != 0) {
            fprintf(stderr, "disc fixture: split AV replay diverged\n");
            ok = 0;
        }
    }

cleanup:
    audio_capture_free(&replay_capture);
    free(checkpoint);
    free(final_state);
    free(replay_state);
    schway_destroy(inst);
    return ok;
}

static int audio_sector_coverage(const struct fixture_disc *host)
{
    for (uint32_t lba = 51u; lba <= 53u; lba++) {
        if (host->sector_reads[lba] == 0u) return 0;
    }
    for (uint32_t lba = 214u; lba <= 221u; lba++) {
        if (host->sector_reads[lba] == 0u) return 0;
    }
    return 1;
}

static int run_av_case(const char *mode, const char *bios_path,
                       const uint8_t digest[32], const schway_disc *disc,
                       struct fixture_disc *host, const uint32_t expected_records[8],
                       const struct av_expectations *expected)
{
    uint8_t expected_data[DISC_DATA_BYTES];
    uint32_t expected_crc = 0u;
    if (!derive_data(host, expected_data, &expected_crc)) {
        fprintf(stderr, "disc fixture: could not derive AV sector reference\n");
        return 0;
    }
    struct audio_capture none;
    struct audio_capture press;
    memset(&none, 0, sizeof none);
    memset(&press, 0, sizeof press);
    uint8_t none_pixels[AV_FRAME_BYTES];
    uint8_t press_pixels[AV_FRAME_BYTES];
    int ok = run_av_variant(bios_path, digest, disc, host, expected_records,
                            expected_data, expected_crc, 0u, 1u, &none, none_pixels);
    if (ok != 0) {
        ok = run_av_variant(bios_path, digest, disc, host, expected_records,
                            expected_data, expected_crc, 300u, 0u, &press, press_pixels);
    }
    if (ok != 0) {
        char audio_none[65];
        char frame_none[65];
        char audio_press[65];
        char frame_press[65];
        digest_audio(&none, audio_none);
        digest_frame(none_pixels, frame_none);
        digest_audio(&press, audio_press);
        digest_frame(press_pixels, frame_press);
        printf("CAPTURE audio-900.none=%s (%zu pairs)\n", audio_none, none.pairs);
        printf("CAPTURE frame-900.none=%s\n", frame_none);
        printf("CAPTURE audio-900.press300=%s (%zu pairs)\n", audio_press, press.pairs);
        printf("CAPTURE frame-900.press300=%s\n", frame_press);
        if (strcmp(mode, "audio") == 0 &&
            (strcmp(audio_none, expected->audio_none) != 0 ||
             strcmp(audio_press, expected->audio_press) != 0)) {
            fprintf(stderr, "disc fixture: PCM regression pin changed\n");
            ok = 0;
        }
        if (strcmp(mode, "fmv") == 0 &&
            (strcmp(frame_none, expected->frame_none) != 0 ||
             strcmp(frame_press, expected->frame_press) != 0)) {
            fprintf(stderr, "disc fixture: RGB24 frame regression pin changed\n");
            ok = 0;
        }
    }
    audio_capture_free(&none);
    audio_capture_free(&press);
    return ok;
}

int main(int argc, char **argv)
{
    if (argc != 7 || (strcmp(argv[1], "boot") != 0 && strcmp(argv[1], "read") != 0 &&
                      strcmp(argv[1], "audio") != 0 && strcmp(argv[1], "fmv") != 0 &&
                      strcmp(argv[1], "record") != 0)) {
        fprintf(stderr, "usage: schwaystation-test-disc-fixture boot|read|audio|fmv|record BIOS EXPECTED CUE SBI CSV\n");
        return 2;
    }
    uint32_t expected_records[8] = {0u};
    if (!load_expected_records(argv[6], expected_records)) {
        fprintf(stderr, "disc fixture: expected CSV rows are missing or malformed\n");
        return 1;
    }
    struct av_expectations expected_av;
    memset(&expected_av, 0, sizeof expected_av);
    if ((strcmp(argv[1], "audio") == 0 || strcmp(argv[1], "fmv") == 0) &&
        !load_expected_av(argv[6], &expected_av)) {
        fprintf(stderr, "disc fixture: audiovisual CSV pins are missing or malformed\n");
        return 1;
    }
    uint8_t expected_digest[32];
    if (!expected_bios_digest(argv[3], expected_digest)) {
        fprintf(stderr, "disc fixture: OpenBIOS digest record is malformed\n");
        return 1;
    }
    size_t sbi_size = 0u;
    uint8_t *sbi = read_file(argv[5], &sbi_size);
    if (sbi == NULL || sbi_size != 116u || memcmp(sbi, "SBId", 4u) != 0) {
        fprintf(stderr, "disc fixture: handwritten SBI fixture is missing or malformed\n");
        free(sbi);
        return 1;
    }
    free(sbi);
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    memset(&error, 0, sizeof error);
    error.size = (uint32_t)sizeof error;
    if (schway_disc_image_open(argv[4], &image, &error) != SCHWAY_DISC_IMAGE_OK || image == NULL) {
        fprintf(stderr, "disc fixture: cannot open cue image: %s\n", error.message);
        return 1;
    }
    const schway_disc *source = schway_disc_image_disc(image);
    if (source == NULL || source->read_sector == NULL || source->toc == NULL) {
        fprintf(stderr, "disc fixture: cue provider has no sector/TOC callbacks\n");
        schway_disc_image_close(image);
        return 1;
    }
    struct fixture_disc host;
    memset(&host, 0, sizeof host);
    host.provider = *source;
    schway_disc disc;
    memset(&disc, 0, sizeof disc);
    disc.size = (uint32_t)sizeof disc;
    disc.user = &host;
    disc.read_sector = fixture_read_sector;
    disc.toc = fixture_read_toc;
    disc.bad_q = fixture_bad_q;
    if (!identity_for_disc(&host, &disc)) {
        fprintf(stderr, "disc fixture: could not identify the first sixteen sectors\n");
        schway_disc_image_close(image);
        return 1;
    }
    int ok;
    if (strcmp(argv[1], "boot") == 0) {
        ok = run_boot_case(argv[2], expected_digest, &disc, &host);
    } else if (strcmp(argv[1], "read") == 0) {
        ok = run_read_case(argv[2], expected_digest, &disc, &host, expected_records);
    } else {
        ok = run_av_case(argv[1], argv[2], expected_digest, &disc, &host,
                         expected_records, &expected_av);
    }
    if (host.outside_run_calls != 0u) {
        fprintf(stderr, "disc fixture: %u host callbacks ran outside a core run\n",
                host.outside_run_calls);
        ok = 0;
    }
    schway_disc_image_close(image);
    if (ok == 0) return 1;
    if (strcmp(argv[1], "audio") == 0 || strcmp(argv[1], "fmv") == 0 ||
        strcmp(argv[1], "record") == 0) {
        printf("disc fixture %s passed: XA/CD-DA PCM, STR MDEC/GPU RGB24, split-state replay\n",
               argv[1]);
    } else {
        printf("disc fixture %s passed: OpenBIOS, SYSTEM.CNF, TOC, GetID, ReadN DMA3, SBI callback\n",
               argv[1]);
    }
    return 0;
}
