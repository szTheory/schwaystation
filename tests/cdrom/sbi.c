/* Good-Q ReadN/GetlocP control for the CD-ROM MMIO path. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "disc_image.h"
#include "internal.h"
#include "../check.h"
#include "../test_open.h"

#define CDROM 0x1F801800u
#define SECTOR_PERIOD UINT64_C(451584)

struct sbi_host {
    schway_toc toc;
    uint32_t reads;
    uint32_t q_lba;
};

static uint8_t sbi_bcd(uint32_t value)
{
    return (uint8_t)(((value / 10u) << 4) | (value % 10u));
}

static void *sbi_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void sbi_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static uint16_t q_crc(const uint8_t q[12])
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
    return (uint16_t)~crc;
}

static uint32_t sbi_read_sector(void *user, uint32_t lba, uint8_t sector[2352],
                                uint8_t subcode[96])
{
    struct sbi_host *host = (struct sbi_host *)user;
    uint8_t q[12] = {0x41u, 0x01u, 0x01u, 0x00u, 0x00u, 0x00u,
                     0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u};
    (void)lba;
    host->reads++;
    memset(sector, 0, 2352u);
    memset(subcode, 0, 96u);
    uint32_t absolute = host->q_lba + 150u;
    q[7] = sbi_bcd(absolute / (60u * 75u));
    q[8] = sbi_bcd((absolute / 75u) % 60u);
    q[9] = sbi_bcd(absolute % 75u);
    memset(sector + 1u, 0xFF, 10u);
    sector[15] = 2u;
    sector[16] = 1u;
    sector[17] = 2u;
    sector[18] = 8u;
    sector[20] = 1u;
    sector[21] = 2u;
    sector[22] = 8u;
    uint16_t crc = q_crc(q);
    q[10] = (uint8_t)(crc >> 8);
    q[11] = (uint8_t)crc;
    for (uint32_t bit = 0u; bit < 96u; bit++) {
        if ((q[bit / 8u] & (1u << (7u - bit % 8u))) != 0u) {
            subcode[bit] |= 0x40u;
        }
    }
    return 1u;
}

static uint32_t sbi_read_toc(void *user, schway_toc *out)
{
    struct sbi_host *host = (struct sbi_host *)user;
    memcpy(out, &host->toc, sizeof *out);
    return 1u;
}

static void sbi_wr8(schway *inst, uint32_t offset, uint8_t value)
{
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, CDROM + offset, 1u, value, &cycles), 0u);
}

static uint8_t sbi_rd8(schway *inst, uint32_t offset)
{
    uint32_t cycles = 0u;
    uint32_t value = 0u;
    CHECK_EQ_U64(schway__bus_read(inst, CDROM + offset, 1u, &value, &cycles), 0u);
    return (uint8_t)value;
}

static void sbi_run(schway *inst, uint64_t cycles)
{
    while (cycles != 0u) {
        schway_budget budget;
        schway_run_result result;
        memset(&budget, 0, sizeof budget);
        budget.size = (uint32_t)sizeof budget;
        budget.max_cycles = cycles;
        memset(&result, 0, sizeof result);
        result.size = (uint32_t)sizeof result;
        uint64_t before = inst->ticks;
        CHECK_EQ_U64(schway_run(inst, &budget, &result), SCHWAY_STATUS_OK);
        uint64_t elapsed = result.ticks - before;
        CHECK(elapsed != 0u && elapsed <= cycles);
        if (elapsed == 0u || elapsed > cycles) return;
        cycles -= elapsed;
    }
}

static uint8_t sbi_irq(schway *inst)
{
    sbi_wr8(inst, 0u, 1u);
    return (uint8_t)(sbi_rd8(inst, 3u) & 7u);
}

static void sbi_ack(schway *inst, uint8_t irq)
{
    sbi_wr8(inst, 0u, 1u);
    sbi_wr8(inst, 3u, (uint8_t)(1u << (irq - 1u)));
}

static void sbi_read_one(schway *inst, struct sbi_host *host)
{
    sbi_wr8(inst, 0u, 0u);
    sbi_wr8(inst, 2u, 0u);
    sbi_wr8(inst, 0u, 0u);
    sbi_wr8(inst, 2u, 0x02u);
    sbi_wr8(inst, 0u, 0u);
    sbi_wr8(inst, 2u, 0u);
    sbi_wr8(inst, 0u, 0u);
    sbi_wr8(inst, 1u, 0x02u);
    sbi_run(inst, 50000u);
    CHECK_EQ_U64(sbi_irq(inst), 3u);
    (void)sbi_rd8(inst, 1u);
    sbi_ack(inst, 3u);
    sbi_wr8(inst, 0u, 0u);
    sbi_wr8(inst, 1u, 0x06u);
    sbi_run(inst, 50000u);
    CHECK_EQ_U64(sbi_irq(inst), 3u);
    (void)sbi_rd8(inst, 1u);
    sbi_ack(inst, 3u);
    sbi_run(inst, SECTOR_PERIOD);
    CHECK_EQ_U64(host->reads, 1u);
    CHECK_EQ_U64(sbi_irq(inst), 1u);
    sbi_ack(inst, 1u);
}

static void sbi_getlocp(schway *inst, uint8_t bytes[8])
{
    sbi_wr8(inst, 0u, 0u);
    sbi_wr8(inst, 1u, 0x11u);
    sbi_run(inst, 50000u);
    CHECK_EQ_U64(sbi_irq(inst), 3u);
    for (uint32_t i = 0u; i < 8u; i++) bytes[i] = sbi_rd8(inst, 1u);
    sbi_ack(inst, 3u);
}

static void sbi_init_host(struct sbi_host *host, uint32_t q_lba)
{
    memset(host, 0, sizeof *host);
    host->q_lba = q_lba;
    host->toc.size = (uint32_t)sizeof host->toc;
    host->toc.track_count = 1u;
    host->toc.tracks[0].size = (uint32_t)sizeof host->toc.tracks[0];
    host->toc.tracks[0].type = SCHWAY_DISC_TRACK_MODE2;
    host->toc.tracks[0].leadout = 1000u;
}

static schway_disc sbi_make_disc(struct sbi_host *host)
{
    schway_disc disc;
    memset(&disc, 0, sizeof disc);
    disc.size = (uint32_t)sizeof disc;
    disc.user = host;
    disc.read_sector = sbi_read_sector;
    disc.toc = sbi_read_toc;
    return disc;
}

static void sbi_set_profile(schway *inst)
{
    schway_profile profile;
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = SCHWAY_REGION_NTSC_U;
    profile.disc_flags = SCHWAY_PROFILE_CD_LICENSED;
    CHECK_EQ_U64(schway_set_profile(inst, &profile), SCHWAY_STATUS_OK);
}

static void test_good_q_readn_getlocp(void)
{
    struct sbi_host host;
    sbi_init_host(&host, 0u);
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = sbi_alloc;
    cfg.allocator.free = sbi_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) return;
    schway_disc disc = sbi_make_disc(&host);
    CHECK_EQ_U64(schway_open_disc(inst, &disc), SCHWAY_STATUS_OK);
    sbi_set_profile(inst);

    sbi_wr8(inst, 0u, 0u);
    sbi_wr8(inst, 1u, 0x06u);
    sbi_run(inst, 50000u);
    CHECK_EQ_U64(sbi_irq(inst), 3u);
    (void)sbi_rd8(inst, 1u);
    sbi_ack(inst, 3u);
    sbi_run(inst, SECTOR_PERIOD);
    CHECK_EQ_U64(host.reads, 1u);
    CHECK_EQ_U64(inst->cdrom.q_record_valid, 1u);
    CHECK_EQ_U64(sbi_irq(inst), 1u);
    sbi_ack(inst, 1u);

    sbi_wr8(inst, 0u, 0u);
    sbi_wr8(inst, 1u, 0x11u);
    sbi_run(inst, 50000u);
    CHECK_EQ_U64(sbi_irq(inst), 3u);
    static const uint8_t expected[8] = {0x01u, 0x01u, 0x00u, 0x02u,
                                        0x00u, 0x00u, 0x00u, 0x00u};
    for (uint32_t i = 0u; i < sizeof expected; i++) {
        CHECK_EQ_U64(sbi_rd8(inst, 1u), expected[i]);
    }
    sbi_ack(inst, 3u);
    schway_destroy(inst);
}

static void test_disc_swap_clears_q_position(void)
{
    struct sbi_host first;
    struct sbi_host second;
    sbi_init_host(&first, 75u);
    sbi_init_host(&second, 200u);
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = sbi_alloc;
    cfg.allocator.free = sbi_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) return;
    schway_disc first_disc = sbi_make_disc(&first);
    schway_disc second_disc = sbi_make_disc(&second);
    CHECK_EQ_U64(schway_open_disc(inst, &first_disc), SCHWAY_STATUS_OK);
    sbi_set_profile(inst);
    sbi_read_one(inst, &first);
    CHECK_EQ_U64(inst->cdrom.q_record_valid, 1u);
    CHECK_EQ_U64(inst->cdrom.current_lba, 75u);
    uint8_t position[8];
    static const uint8_t first_position[8] = {
        0x01u, 0x01u, 0x00u, 0x03u, 0x00u, 0x00u, 0x00u, 0x00u
    };
    sbi_getlocp(inst, position);
    for (uint32_t i = 0u; i < sizeof position; i++) {
        CHECK_EQ_U64(position[i], first_position[i]);
    }

    CHECK_EQ_U64(schway_set_lid(inst, 1u), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(sbi_irq(inst), 5u);
    (void)sbi_rd8(inst, 1u);
    (void)sbi_rd8(inst, 1u);
    sbi_ack(inst, 5u);
    CHECK_EQ_U64(schway_swap_disc(inst, &second_disc), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(inst->cdrom.q_record_valid, 0u);
    CHECK_EQ_U64(inst->cdrom.current_lba, 0u);
    CHECK_EQ_U64(schway_set_lid(inst, 0u), SCHWAY_STATUS_OK);

    static const uint8_t cleared_position[8] = {
        0x01u, 0x01u, 0x00u, 0x02u, 0x00u, 0x00u, 0x00u, 0x00u
    };
    sbi_getlocp(inst, position);
    for (uint32_t i = 0u; i < sizeof position; i++) {
        CHECK_EQ_U64(position[i], cleared_position[i]);
    }
    sbi_read_one(inst, &second);
    CHECK_EQ_U64(inst->cdrom.q_record_valid, 1u);
    CHECK_EQ_U64(inst->cdrom.current_lba, 200u);
    static const uint8_t second_position[8] = {
        0x01u, 0x01u, 0x00u, 0x04u, 0x50u, 0x00u, 0x00u, 0x00u
    };
    sbi_getlocp(inst, position);
    for (uint32_t i = 0u; i < sizeof position; i++) {
        CHECK_EQ_U64(position[i], second_position[i]);
    }
    schway_destroy(inst);
}

static int sbi_make_path(char path[1024], const char *directory, const char *name)
{
    int size = snprintf(path, 1024u, "%s/%s", directory, name);
    return size >= 0 && (size_t)size < 1024u;
}

static int sbi_write_bytes(const char *path, const uint8_t *bytes, size_t size)
{
    FILE *file = schway__test_open_file(path, "wb");
    int failed;
    if (file == NULL) return 0;
    failed = fwrite(bytes, 1u, size, file) != size;
    if (fclose(file) != 0) failed = 1;
    return failed == 0;
}

static int sbi_write_text(const char *path, const char *text)
{
    return sbi_write_bytes(path, (const uint8_t *)text, strlen(text));
}

static uint64_t sbi_file_size(const char *path);

static int sbi_check_overlay(const char *path, const uint32_t *bad_lbas,
                             uint32_t bad_count)
{
    static const uint8_t header[4] = {'S', 'B', 'I', 0u};
    if (sbi_file_size(path) != 4u + (uint64_t)bad_count * 7u) return 0;
    FILE *file = schway__test_open_file(path, "rb");
    uint8_t actual_header[sizeof header];
    int ok = file != NULL;
    if (!ok) return 0;
    ok = fread(actual_header, 1u, sizeof actual_header, file) == sizeof actual_header &&
         memcmp(actual_header, header, sizeof header) == 0;
    for (uint32_t i = 0u; ok != 0 && i < bad_count; i++) {
        uint32_t absolute = bad_lbas[i] + 150u;
        uint8_t expected[7] = {
            sbi_bcd(absolute / (60u * 75u)),
            sbi_bcd((absolute / 75u) % 60u),
            sbi_bcd(absolute % 75u), 0x02u, 0xFAu, 0xFAu, 0xFAu
        };
        uint8_t actual[sizeof expected];
        ok = fread(actual, 1u, sizeof actual, file) == sizeof actual &&
             memcmp(actual, expected, sizeof expected) == 0;
    }
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static uint64_t sbi_file_size(const char *path)
{
    FILE *file = schway__test_open_file(path, "rb");
    long size;
    if (file == NULL) return 0u;
    int ok = fseek(file, 0L, SEEK_END) == 0;
    size = ok != 0 ? ftell(file) : -1L;
    if (fclose(file) != 0 || size < 0L) return 0u;
    return (uint64_t)size;
}

static int sbi_copy_file(const char *source, const char *destination)
{
    uint8_t buffer[4096];
    FILE *in = schway__test_open_file(source, "rb");
    FILE *out = schway__test_open_file(destination, "wb");
    int ok = in != NULL && out != NULL;
    if (!ok) goto done;
    for (;;) {
        size_t count = fread(buffer, 1u, sizeof buffer, in);
        if (count != 0u && fwrite(buffer, 1u, count, out) != count) {
            ok = 0;
            break;
        }
        if (count < sizeof buffer) {
            if (ferror(in) != 0) ok = 0;
            break;
        }
    }
done:
    if (in != NULL && fclose(in) != 0) ok = 0;
    if (out != NULL && fclose(out) != 0) ok = 0;
    return ok;
}

static int sbi_write_q(const char *path, uint32_t lba)
{
    uint32_t absolute = lba + 150u;
    uint8_t q[12] = {0x41u, 0x01u, 0x01u, 0u, 0u, 0u,
                     0u, 0u, 0u, 0u, 0u, 0u};
    q[5] = sbi_bcd(lba % 75u);
    q[4] = sbi_bcd((lba / 75u) % 60u);
    q[3] = sbi_bcd(lba / (60u * 75u));
    q[7] = sbi_bcd(absolute / (60u * 75u));
    q[8] = sbi_bcd((absolute / 75u) % 60u);
    q[9] = sbi_bcd(absolute % 75u);
    uint16_t crc = q_crc(q);
    q[10] = (uint8_t)(crc >> 8);
    q[11] = (uint8_t)crc;
    FILE *file = schway__test_open_file(path, "r+b");
    if (file == NULL) return 0;
    long offset = (long)((uint64_t)lba * UINT64_C(96) + UINT64_C(12));
    int ok = fseek(file, offset, SEEK_SET) == 0 &&
             fwrite(q, 1u, sizeof q, file) == sizeof q;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static int sbi_write_data_marker(const char *path, uint32_t lba, uint8_t marker)
{
    uint8_t data[2048];
    memset(data, marker, sizeof data);
    FILE *file = schway__test_open_file(path, "r+b");
    if (file == NULL) return 0;
    long sector = (long)((uint64_t)lba * UINT64_C(2352));
    uint8_t data_submode = 0x28u;
    int ok = fseek(file, sector + 18L, SEEK_SET) == 0 &&
             fwrite(&data_submode, 1u, 1u, file) == 1u &&
             fseek(file, sector + 22L, SEEK_SET) == 0 &&
             fwrite(&data_submode, 1u, 1u, file) == 1u &&
             fseek(file, sector + 24L, SEEK_SET) == 0 &&
             fwrite(data, 1u, sizeof data, file) == sizeof data;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static int sbi_write_overlay(const char *path, const uint32_t *bad_lbas,
                             uint32_t bad_count)
{
    static const uint8_t header[4] = {'S', 'B', 'I', 0u};
    FILE *file = schway__test_open_file(path, "wb");
    if (file == NULL) return 0;
    int ok = fwrite(header, 1u, sizeof header, file) == sizeof header;
    for (uint32_t i = 0u; ok != 0 && i < bad_count; i++) {
        uint32_t absolute = bad_lbas[i] + 150u;
        uint8_t record[7] = {
            sbi_bcd(absolute / (60u * 75u)),
            sbi_bcd((absolute / 75u) % 60u),
            sbi_bcd(absolute % 75u), 0x02u, 0xFAu, 0xFAu, 0xFAu
        };
        ok = fwrite(record, 1u, sizeof record, file) == sizeof record;
    }
    if (fclose(file) != 0) ok = 0;
    return ok != 0 && sbi_check_overlay(path, bad_lbas, bad_count);
}

static int sbi_copy_child(const char *source_directory, const char *directory,
                          const char *stem, uint8_t marker,
                          const uint32_t *bad_lbas, uint32_t bad_count)
{
    static const char *const extensions[] = {"ccd", "img", "sub"};
    char source_name[128];
    char destination_name[128];
    char source[1024];
    char destination[1024];
    char sub[1024];
    char image[1024];
    char overlay[1024];
    static const uint32_t q_lbas[] = {1u, 2u, 3u, 4u};
    for (uint32_t i = 0u; i < sizeof extensions / sizeof extensions[0]; i++) {
        int source_size = snprintf(source_name, sizeof source_name,
                                   "disc_boot_legacy.%s", extensions[i]);
        int destination_size = snprintf(destination_name, sizeof destination_name,
                                        "%s.%s", stem, extensions[i]);
        if (source_size < 0 || (size_t)source_size >= sizeof source_name ||
            destination_size < 0 || (size_t)destination_size >= sizeof destination_name ||
            !sbi_make_path(source, source_directory, source_name) ||
            !sbi_make_path(destination, directory, destination_name) ||
            !sbi_copy_file(source, destination)) return 0;
        if (strcmp(extensions[i], "sub") == 0) {
            memcpy(sub, destination, strlen(destination) + 1u);
        }
        if (strcmp(extensions[i], "img") == 0) {
            memcpy(image, destination, strlen(destination) + 1u);
        }
    }
    int overlay_size = snprintf(destination_name, sizeof destination_name, "%s.sbi", stem);
    if (overlay_size < 0 || (size_t)overlay_size >= sizeof destination_name ||
        !sbi_make_path(overlay, directory, destination_name)) return 0;
    for (uint32_t i = 0u; i < sizeof q_lbas / sizeof q_lbas[0]; i++) {
        uint32_t lba = q_lbas[i];
        if (!sbi_write_q(sub, lba) ||
            !sbi_write_data_marker(image, lba, (uint8_t)(marker + (uint8_t)lba))) return 0;
    }
    return sbi_write_overlay(overlay, bad_lbas, bad_count);
}

static void sbi_remove_child(const char *directory, const char *stem)
{
    static const char *const extensions[] = {"ccd", "img", "sub", "sbi"};
    char name[128];
    char path[1024];
    for (uint32_t i = 0u; i < sizeof extensions / sizeof extensions[0]; i++) {
        int size = snprintf(name, sizeof name, "%s.%s", stem, extensions[i]);
        if (size >= 0 && (size_t)size < sizeof name &&
            sbi_make_path(path, directory, name)) (void)remove(path);
    }
}

static int sbi_setloc(schway *inst, uint32_t lba)
{
    uint32_t absolute = lba + 150u;
    sbi_wr8(inst, 0u, 0u);
    sbi_wr8(inst, 2u, sbi_bcd(absolute / (60u * 75u)));
    sbi_wr8(inst, 0u, 0u);
    sbi_wr8(inst, 2u, sbi_bcd((absolute / 75u) % 60u));
    sbi_wr8(inst, 0u, 0u);
    sbi_wr8(inst, 2u, sbi_bcd(absolute % 75u));
    sbi_wr8(inst, 0u, 0u);
    sbi_wr8(inst, 1u, 0x02u);
    sbi_run(inst, 50000u);
    uint8_t irq = sbi_irq(inst);
    CHECK_EQ_U64(irq, 3u);
    if (irq != 3u || inst->cdrom.result_count == 0u) return 0;
    (void)sbi_rd8(inst, 1u);
    sbi_ack(inst, 3u);
    CHECK_EQ_U64(inst->cdrom.target_lba, lba);
    return inst->cdrom.target_lba == lba;
}

static int sbi_init_drive(schway *inst)
{
    sbi_wr8(inst, 0u, 0u);
    sbi_wr8(inst, 1u, 0x0Au);
    sbi_run(inst, 50000u);
    uint8_t irq = sbi_irq(inst);
    CHECK_EQ_U64(irq, 3u);
    if (irq != 3u || inst->cdrom.result_count == 0u) return 0;
    CHECK_EQ_U64(sbi_rd8(inst, 1u), 0x02u);
    sbi_ack(inst, 3u);
    sbi_run(inst, 476000u);
    irq = sbi_irq(inst);
    CHECK_EQ_U64(irq, 2u);
    if (irq != 2u || inst->cdrom.result_count == 0u) return 0;
    CHECK_EQ_U64(sbi_rd8(inst, 1u), 0x02u);
    sbi_ack(inst, 2u);

    sbi_wr8(inst, 0u, 0u);
    sbi_wr8(inst, 2u, 0u);
    sbi_wr8(inst, 0u, 0u);
    sbi_wr8(inst, 1u, 0x0Eu);
    sbi_run(inst, 50000u);
    irq = sbi_irq(inst);
    CHECK_EQ_U64(irq, 3u);
    if (irq != 3u || inst->cdrom.result_count == 0u) return 0;
    CHECK_EQ_U64(sbi_rd8(inst, 1u), 0x02u);
    sbi_ack(inst, 3u);
    return 1;
}

static int sbi_start_readn(schway *inst)
{
    sbi_wr8(inst, 0u, 0u);
    sbi_wr8(inst, 3u, 0x80u);
    sbi_wr8(inst, 0u, 0u);
    sbi_wr8(inst, 1u, 0x06u);
    sbi_run(inst, 50000u);
    uint8_t irq = sbi_irq(inst);
    CHECK_EQ_U64(irq, 3u);
    if (irq != 3u || inst->cdrom.result_count == 0u) return 0;
    (void)sbi_rd8(inst, 1u);
    sbi_ack(inst, 3u);
    CHECK_EQ_U64(inst->cdrom.read_active, 1u);
    return inst->cdrom.read_active == 1u;
}

static int sbi_wait_sector(schway *inst)
{
    if (inst->cdrom.seek_event != 0u && inst->cdrom.seek_deadline > inst->ticks) {
        sbi_run(inst, inst->cdrom.seek_deadline - inst->ticks);
    }
    if (inst->cdrom.sector_event != 0u && inst->cdrom.sector_deadline > inst->ticks) {
        sbi_run(inst, inst->cdrom.sector_deadline - inst->ticks);
    }
    uint8_t irq = sbi_irq(inst);
    CHECK_EQ_U64(irq, 1u);
    if (irq != 1u) return 0;
    sbi_ack(inst, 1u);
    return 1;
}

static int sbi_read_payload(schway *inst, uint8_t marker)
{
    uint8_t payload[2048];
    uint8_t expected[sizeof payload];
    memset(expected, marker, sizeof expected);
    for (uint32_t i = 0u; i < sizeof payload; i++) payload[i] = sbi_rd8(inst, 2u);
    int matches = memcmp(payload, expected, sizeof payload) == 0;
    CHECK(matches);
    return matches;
}

static int sbi_check_getlocp(schway *inst, const uint8_t expected[8])
{
    sbi_wr8(inst, 0u, 0u);
    sbi_wr8(inst, 1u, 0x11u);
    sbi_run(inst, 50000u);
    uint8_t irq = sbi_irq(inst);
    CHECK_EQ_U64(irq, 3u);
    if (irq != 3u || inst->cdrom.result_count < 9u) return 0;
    (void)sbi_rd8(inst, 1u);
    int matches = 1;
    for (uint32_t i = 0u; i < 8u; i++) {
        uint8_t value = sbi_rd8(inst, 1u);
        CHECK_EQ_U64(value, expected[i]);
        if (value != expected[i]) matches = 0;
    }
    sbi_ack(inst, 3u);
    return matches;
}

static void test_selected_provider_command_sequence(const char *directory,
                                                    const char *fixture_directory)
{
    static const uint32_t bad_a[] = {3u, 4u};
    static const uint32_t bad_b[] = {3u};
    static const char playlist[] = "schway-sbi-command-a.ccd\n"
                                   "schway-sbi-command-b.ccd\n";
    static const char stem_a[] = "schway-sbi-command-a";
    static const char stem_b[] = "schway-sbi-command-b";
    char path[1024];
    char name[128];
    char source_img[1024];
    char source_sub[1024];
    char image_a[1024];
    char sub_b[1024];
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    const schway_disc *disc_a = NULL;
    const schway_disc *disc_b = NULL;
    schway_toc toc_a;
    schway_toc toc_b;
    schway_config cfg;
    schway *inst = NULL;
    int name_size = snprintf(name, sizeof name, "%s.m3u", "schway-sbi-command");
    CHECK(name_size >= 0 && (size_t)name_size < sizeof name);
    if (name_size < 0 || (size_t)name_size >= sizeof name ||
        !sbi_make_path(path, directory, name)) return;
    char playlist_path[1024];
    memcpy(playlist_path, path, sizeof playlist_path);

    CHECK(sbi_make_path(source_img, fixture_directory, "disc_boot_legacy.img"));
    CHECK(sbi_make_path(source_sub, fixture_directory, "disc_boot_legacy.sub"));
    CHECK_EQ_U64(sbi_file_size(source_img) / 2352u, 222u);
    CHECK_EQ_U64(sbi_file_size(source_sub) / 96u, 222u);
    CHECK_EQ_U64(sbi_file_size(source_img) % 2352u, 0u);
    CHECK_EQ_U64(sbi_file_size(source_sub) % 96u, 0u);
    CHECK(sbi_copy_child(fixture_directory, directory, stem_a, 0xA0u, bad_a, 2u));
    CHECK(sbi_copy_child(fixture_directory, directory, stem_b, 0xB0u, bad_b, 1u));
    CHECK(sbi_make_path(image_a, directory, "schway-sbi-command-a.img"));
    CHECK(sbi_make_path(sub_b, directory, "schway-sbi-command-b.sub"));
    CHECK_EQ_U64(sbi_file_size(image_a), sbi_file_size(source_img));
    CHECK_EQ_U64(sbi_file_size(sub_b), sbi_file_size(source_sub));
    CHECK(sbi_write_text(playlist_path, playlist));
    memset(&error, 0, sizeof error);
    error.size = (uint32_t)sizeof error;
    CHECK_EQ_U64(schway_disc_image_open(playlist_path, &image, &error),
                 SCHWAY_DISC_IMAGE_OK);
    if (image == NULL) goto cleanup;
    CHECK_EQ_U64(schway_disc_image_select(image, 0u, &disc_a), SCHWAY_DISC_IMAGE_OK);
    CHECK_EQ_U64(schway_disc_image_select(image, 1u, &disc_b), SCHWAY_DISC_IMAGE_OK);
    CHECK(disc_a != NULL && disc_b != NULL && disc_a != disc_b);
    if (disc_a == NULL || disc_b == NULL) goto cleanup;
    CHECK(disc_a->bad_q != NULL && disc_b->bad_q != NULL);
    CHECK_EQ_U64(disc_a->bad_q(disc_a->user, 3u), 1u);
    CHECK_EQ_U64(disc_a->bad_q(disc_a->user, 4u), 1u);
    CHECK_EQ_U64(disc_b->bad_q(disc_b->user, 3u), 1u);
    CHECK_EQ_U64(disc_b->bad_q(disc_b->user, 4u), 0u);
    memset(&toc_a, 0, sizeof toc_a);
    memset(&toc_b, 0, sizeof toc_b);
    toc_a.size = (uint32_t)sizeof toc_a;
    toc_b.size = (uint32_t)sizeof toc_b;
    CHECK(disc_a->toc(disc_a->user, &toc_a) == 1u);
    CHECK(disc_b->toc(disc_b->user, &toc_b) == 1u);
    CHECK_EQ_U64(toc_a.track_count, 2u);
    CHECK_EQ_U64(toc_b.track_count, 2u);
    CHECK_EQ_U64(toc_a.tracks[0].leadout, 64u);
    CHECK_EQ_U64(toc_b.tracks[0].leadout, 64u);
    CHECK_EQ_U64(toc_a.tracks[1].leadout, 222u);
    CHECK_EQ_U64(toc_b.tracks[1].leadout, 222u);

    uint8_t sector[2352];
    uint8_t subcode[96];
    uint8_t q[12] = {0u};
    CHECK(disc_b->read_sector(disc_b->user, 3u, sector, subcode) == 1u);
    CHECK_EQ_HEX(sector[24], 0xB3u);
    for (uint32_t i = 0u; i < 96u; i++) {
        q[i / 8u] |= (uint8_t)(((subcode[i] >> 6) & 1u) << (7u - (i & 7u)));
    }
    CHECK_EQ_U64(q[1], 0x01u);
    CHECK_EQ_U64(q[2], 0x01u);
    CHECK_EQ_U64(q[7], 0x00u);
    CHECK_EQ_U64(q[8], 0x02u);
    CHECK_EQ_U64(q[9], 0x03u);

    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = sbi_alloc;
    cfg.allocator.free = sbi_free;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) goto cleanup;
    CHECK_EQ_U64(schway_open_disc(inst, disc_a), SCHWAY_STATUS_OK);
    sbi_set_profile(inst);
    if (!sbi_init_drive(inst) || !sbi_setloc(inst, 1u) ||
        !sbi_start_readn(inst) || !sbi_wait_sector(inst) ||
        !sbi_read_payload(inst, 0xA1u)) goto cleanup;
    static const uint8_t position_a[8] = {
        0x01u, 0x01u, 0x00u, 0x02u, 0x01u, 0x00u, 0x00u, 0x01u
    };
    if (!sbi_check_getlocp(inst, position_a)) goto cleanup;

    CHECK_EQ_U64(schway_set_lid(inst, 1u), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(sbi_irq(inst), 5u);
    CHECK_EQ_U64(sbi_rd8(inst, 1u) & 0x80u, 0u);
    CHECK_EQ_U64(sbi_rd8(inst, 1u), 0x08u);
    sbi_ack(inst, 5u);
    CHECK_EQ_U64(schway_swap_disc(inst, disc_b), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_set_lid(inst, 0u), SCHWAY_STATUS_OK);
    static const uint8_t cleared_position[8] = {
        0x01u, 0x01u, 0x00u, 0x02u, 0x00u, 0x00u, 0x00u, 0x00u
    };
    if (!sbi_check_getlocp(inst, cleared_position)) goto cleanup;

    if (!sbi_setloc(inst, 2u) || !sbi_start_readn(inst) ||
        !sbi_wait_sector(inst)) goto cleanup;
    CHECK_EQ_U64(inst->cdrom.current_lba, 2u);
    if (!sbi_read_payload(inst, 0xB2u)) goto cleanup;
    CHECK_EQ_U64(inst->cdrom.target_lba, 3u);
    if (!sbi_wait_sector(inst)) goto cleanup;
    CHECK_EQ_U64(inst->cdrom.current_lba, 2u);
    CHECK_EQ_U64(inst->cdrom.target_lba, 4u);
    if (!sbi_read_payload(inst, 0xB3u)) goto cleanup;
    static const uint8_t position_b_good[8] = {
        0x01u, 0x01u, 0x00u, 0x02u, 0x02u, 0x00u, 0x00u, 0x02u
    };
    if (!sbi_check_getlocp(inst, position_b_good) || !sbi_wait_sector(inst)) goto cleanup;
    CHECK_EQ_U64(inst->cdrom.current_lba, 4u);
    CHECK_EQ_U64(inst->cdrom.target_lba, 5u);
    if (!sbi_read_payload(inst, 0xB4u)) goto cleanup;
    static const uint8_t position_b_only[8] = {
        0x01u, 0x01u, 0x00u, 0x02u, 0x04u, 0x00u, 0x00u, 0x04u
    };
    (void)sbi_check_getlocp(inst, position_b_only);

cleanup:
    schway_destroy(inst);
    schway_disc_image_close(image);
    sbi_remove_child(directory, stem_a);
    sbi_remove_child(directory, stem_b);
    (void)snprintf(name, sizeof name, "%s.m3u", "schway-sbi-command");
    if (sbi_make_path(path, directory, name)) (void)remove(path);
}

int main(int argc, char **argv)
{
    CHECK_EQ_U64(argc, 3u);
    test_good_q_readn_getlocp();
    test_disc_swap_clears_q_position();
    if (argc == 3) test_selected_provider_command_sequence(argv[1], argv[2]);
    CHECK_DONE();
}
