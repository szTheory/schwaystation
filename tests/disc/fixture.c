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
#define MAILBOX_OFFSET 0x10000u
#define MAILBOX_RECORDS_OFFSET 0x10u
#define MAILBOX_SIZE 0x100u

struct fixture_disc {
    schway_disc provider;
    uint32_t inside_run;
    uint32_t outside_run_calls;
    uint32_t bad_q_calls;
    uint32_t bad_q_hits;
    uint32_t read_calls;
    uint32_t sector_reads[64];
    uint32_t bad_q_seen[64];
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
    if (lba < 64u) host->sector_reads[lba]++;
    return host->provider.read_sector(host->provider.user, lba, sector, subcode);
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

static int run_frames(schway *inst, struct fixture_disc *host, uint32_t first,
                      uint32_t last, uint32_t press_frame)
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
        status = schway_run_frame(inst, &result);
        host->inside_run = 0u;
        if (status != SCHWAY_STATUS_OK || result.stop == SCHWAY_STOP_FAULT ||
            result.fault_present != 0u) {
            fprintf(stderr, "disc fixture: frame %u stopped with %s/%u\n",
                    frame, schway_status_string(status), result.stop);
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
    int ok = run_frames(inst, host, 1u, 600u, 0u) && guest_started(inst);
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
    ok = run_frames(inst, host, 1u, 300u, 0u) &&
         schway_save_state(inst, checkpoint, state_size) == SCHWAY_STATUS_OK &&
         run_frames(inst, host, 301u, 600u, 0u) && guest_started(inst) &&
         run_frames(inst, host, 601u, 900u, 0u) &&
         verify_guest(inst, host, expected_data, expected_crc, expected_records, first_mailbox) &&
         schway_save_state(inst, final_state, state_size) == SCHWAY_STATUS_OK;
    if (ok == 0) {
        fprintf(stderr, "disc fixture: no-input boot/read acceptance failed\n");
        goto cleanup;
    }
    if (schway_load_state(inst, checkpoint, state_size) != SCHWAY_STATUS_OK ||
        !run_frames(inst, host, 301u, 900u, 0u) ||
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
    ok = run_frames(inst, host, 1u, 600u, 300u) && guest_started(inst) &&
         run_frames(inst, host, 601u, 900u, 300u) &&
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

int main(int argc, char **argv)
{
    if (argc != 7 || (strcmp(argv[1], "boot") != 0 && strcmp(argv[1], "read") != 0)) {
        fprintf(stderr, "usage: schwaystation-test-disc-fixture boot|read BIOS EXPECTED CUE SBI CSV\n");
        return 2;
    }
    uint32_t expected_records[8] = {0u};
    if (!load_expected_records(argv[6], expected_records)) {
        fprintf(stderr, "disc fixture: expected CSV rows are missing or malformed\n");
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
    int ok = strcmp(argv[1], "boot") == 0
                 ? run_boot_case(argv[2], expected_digest, &disc, &host)
                 : run_read_case(argv[2], expected_digest, &disc, &host, expected_records);
    if (host.outside_run_calls != 0u) {
        fprintf(stderr, "disc fixture: %u host callbacks ran outside a core run\n",
                host.outside_run_calls);
        ok = 0;
    }
    schway_disc_image_close(image);
    if (ok == 0) return 1;
    printf("disc fixture %s passed: OpenBIOS, SYSTEM.CNF, TOC, GetID, ReadN DMA3, SBI callback\n",
           argv[1]);
    return 0;
}
