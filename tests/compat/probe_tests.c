/* Small ISO9660 fixtures for the host-only compatibility probe tests. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROBE_TEST_SECTOR_SIZE 2048u
#define PROBE_TEST_IMAGE_SECTORS 24u

static void put16_le_be(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
    out[2] = (uint8_t)(value >> 8);
    out[3] = (uint8_t)value;
}

static void put32_le_be(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8);
    out[2] = (uint8_t)(value >> 16);
    out[3] = (uint8_t)(value >> 24);
    out[4] = (uint8_t)(value >> 24);
    out[5] = (uint8_t)(value >> 16);
    out[6] = (uint8_t)(value >> 8);
    out[7] = (uint8_t)value;
}

static size_t write_record(uint8_t *out, size_t offset, uint32_t extent,
                           uint32_t size, uint8_t flags,
                           const uint8_t *name, size_t name_size)
{
    size_t record_size = 33u + name_size + ((name_size & 1u) == 0u ? 1u : 0u);
    uint8_t *record = out + offset;
    if (name_size > 255u || record_size > PROBE_TEST_SECTOR_SIZE - offset) {
        return 0u;
    }
    record[0] = (uint8_t)record_size;
    record[1] = 0u;
    put32_le_be(record + 2u, extent);
    put32_le_be(record + 10u, size);
    record[25] = flags;
    put16_le_be(record + 28u, 1u);
    record[32] = (uint8_t)name_size;
    memcpy(record + 33u, name, name_size);
    return record_size;
}

static size_t write_named_record(uint8_t *out, size_t offset, uint32_t extent,
                                 uint32_t size, uint8_t flags, const char *name)
{
    return write_record(out, offset, extent, size, flags,
                        (const uint8_t *)name, strlen(name));
}

static void write_executable(uint8_t *out)
{
    memset(out, 0, 4096u);
    memcpy(out, "PS-X EXE", 8u);
    out[0x10u] = 0x00u;
    out[0x11u] = 0x00u;
    out[0x12u] = 0x01u;
    out[0x13u] = 0x80u;
    out[0x18u] = 0x00u;
    out[0x19u] = 0x00u;
    out[0x1Au] = 0x01u;
    out[0x1Bu] = 0x80u;
    out[0x1Cu] = 0x00u;
    out[0x1Du] = 0x08u;
}

static int make_synthetic_iso(const char *path, const char *mode)
{
    static const uint8_t dot[] = {0u};
    static const uint8_t dotdot[] = {1u};
    static const char system_cnf[] = "BOOT = cdrom:\\SWAY_000.01;1\r\n";
    static const char traversal_system_cnf[] = "BOOT = cdrom:\\..\\SWAY_000.01;1\r\n";
    static const uint8_t pvd_id[] = {'C', 'D', '0', '0', '1'};
    uint8_t image[PROBE_TEST_SECTOR_SIZE * PROBE_TEST_IMAGE_SECTORS];
    uint8_t *pvd = image + 16u * PROBE_TEST_SECTOR_SIZE;
    uint8_t *terminator = image + 17u * PROBE_TEST_SECTOR_SIZE;
    uint8_t *root = image + 18u * PROBE_TEST_SECTOR_SIZE;
    uint8_t *exe = image + 20u * PROBE_TEST_SECTOR_SIZE;
    size_t offset = 0u;
    FILE *file;
    const char *system_contents = strcmp(mode, "traversal") == 0 ?
                                  traversal_system_cnf : system_cnf;

    memset(image, 0, sizeof image);
    pvd[0] = 1u;
    memcpy(pvd + 1u, pvd_id, sizeof pvd_id);
    pvd[6] = 1u;
    put32_le_be(pvd + 80u, PROBE_TEST_IMAGE_SECTORS);
    put16_le_be(pvd + 120u, 1u);
    put16_le_be(pvd + 124u, 1u);
    put16_le_be(pvd + 128u, PROBE_TEST_SECTOR_SIZE);
    put32_le_be(pvd + 132u, 0u);
    (void)write_record(pvd, 156u, 18u, PROBE_TEST_SECTOR_SIZE, 2u, dot, sizeof dot);

    terminator[0] = 255u;
    memcpy(terminator + 1u, pvd_id, sizeof pvd_id);
    terminator[6] = 1u;

    offset += write_record(root, offset, 18u, PROBE_TEST_SECTOR_SIZE, 2u, dot, sizeof dot);
    offset += write_record(root, offset, 18u, PROBE_TEST_SECTOR_SIZE, 2u, dotdot, sizeof dotdot);
    if (strcmp(mode, "fallback") != 0 && strcmp(mode, "missing") != 0) {
        offset += write_named_record(root, offset, 19u, (uint32_t)strlen(system_contents), 0u,
                                     "SYSTEM.CNF;1");
    }
    if (strcmp(mode, "missing") != 0) {
        const char *boot_file = strcmp(mode, "fallback") == 0 ? "PSX.EXE" : "SWAY_000.01;1";
        uint32_t boot_extent = strcmp(mode, "bad_extent") == 0 ? PROBE_TEST_IMAGE_SECTORS : 20u;
        offset += write_named_record(root, offset, boot_extent, 4096u, 0u, boot_file);
    }
    if (offset == 0u) {
        return 1;
    }
    if (strcmp(mode, "fallback") != 0 && strcmp(mode, "missing") != 0) {
        memcpy(image + 19u * PROBE_TEST_SECTOR_SIZE, system_contents, strlen(system_contents));
    }
    if (strcmp(mode, "missing") != 0) write_executable(exe);

    file = fopen(path, "wb");
    if (file == NULL) {
        return 1;
    }
    int failed = fwrite(image, 1u, sizeof image, file) != sizeof image;
    if (fclose(file) != 0) {
        failed = 1;
    }
    return failed;
}

static int write_iso_sector(FILE *file, const uint8_t *iso, size_t sector,
                            size_t sector_size, size_t data_offset, int mode2)
{
    uint8_t raw[2352];
    if (sector_size == PROBE_TEST_SECTOR_SIZE) {
        return fwrite(iso + sector * PROBE_TEST_SECTOR_SIZE, 1u,
                      PROBE_TEST_SECTOR_SIZE, file) == PROBE_TEST_SECTOR_SIZE ? 0 : 1;
    }
    memset(raw, 0, sizeof raw);
    if (sector_size == 2352u) {
        raw[0] = 0u;
        memset(raw + 1u, 0xFF, 10u);
        raw[11] = 0u;
        raw[15] = mode2 ? 2u : 1u;
        if (mode2) {
            raw[16] = 0u;
            raw[17] = 0u;
            raw[18] = 8u;
            raw[19] = 0u;
            memcpy(raw + 20u, raw + 16u, 4u);
        }
    } else if (sector_size == 2336u) {
        raw[2] = 8u;
        raw[6] = 8u;
    }
    memcpy(raw + data_offset, iso + sector * PROBE_TEST_SECTOR_SIZE,
           PROBE_TEST_SECTOR_SIZE);
    return fwrite(raw, 1u, sector_size, file) == sector_size ? 0 : 1;
}

static void write_msf(FILE *cue, unsigned frames)
{
    unsigned minute = frames / (60u * 75u);
    unsigned second = (frames / 75u) % 60u;
    unsigned frame = frames % 75u;
    (void)fprintf(cue, "%02u:%02u:%02u", minute, second, frame);
}

static int make_cue_fixture(const char *cue_path, const char *mode)
{
    uint8_t iso[PROBE_TEST_SECTOR_SIZE * PROBE_TEST_IMAGE_SECTORS];
    char *source_path = NULL;
    char *bin_path = NULL;
    size_t cue_path_size = strlen(cue_path);
    FILE *source = NULL;
    FILE *bin = NULL;
    FILE *cue = NULL;
    int failed = 0;
    size_t sector_size = PROBE_TEST_SECTOR_SIZE;
    size_t data_offset = 0u;
    int mode2 = 0;
    int track99 = strcmp(mode, "cue_track99") == 0;

    if (strcmp(mode, "cue_mode1_2352") == 0) {
        sector_size = 2352u;
        data_offset = 16u;
    } else if (strcmp(mode, "cue_mode2_2352") == 0) {
        sector_size = 2352u;
        data_offset = 24u;
        mode2 = 1;
    } else if (strcmp(mode, "cue_mode2_2336") == 0) {
        sector_size = 2336u;
        data_offset = 8u;
        mode2 = 1;
    } else if (strcmp(mode, "cue_2048") != 0 && !track99) {
        return 1;
    }

    source_path = (char *)malloc(cue_path_size + 16u);
    bin_path = (char *)malloc(cue_path_size + 16u);
    if (source_path == NULL || bin_path == NULL) {
        failed = 1;
        goto done;
    }
    (void)snprintf(source_path, cue_path_size + 16u, "%s.source", cue_path);
    const char *slash = strrchr(cue_path, '/');
    const char *backslash = strrchr(cue_path, '\\');
    const char *last = slash;
    if (backslash != NULL && (last == NULL || backslash > last)) last = backslash;
    size_t directory_size = last == NULL ? 0u : (size_t)(last - cue_path + 1);
    memcpy(bin_path, cue_path, directory_size);
    (void)snprintf(bin_path + directory_size, cue_path_size + 16u - directory_size,
                   "synthetic.bin");
    if (make_synthetic_iso(source_path, "synthetic") != 0) {
        failed = 1;
        goto done;
    }
    source = fopen(source_path, "rb");
    bin = fopen(bin_path, "wb");
    if (source == NULL || bin == NULL ||
        fread(iso, 1u, sizeof iso, source) != sizeof iso) {
        failed = 1;
        goto done;
    }
    if (fclose(source) != 0) failed = 1;
    source = NULL;

    if (track99) {
        uint8_t audio_sector[2352] = {0};
        for (unsigned i = 0u; i < 98u && !failed; i++) {
            if (fwrite(audio_sector, 1u, sizeof audio_sector, bin) != sizeof audio_sector) failed = 1;
        }
        for (size_t i = 0u; i < PROBE_TEST_IMAGE_SECTORS && !failed; i++) {
            failed = write_iso_sector(bin, iso, i, PROBE_TEST_SECTOR_SIZE, 0u, 0);
        }
    } else {
        for (size_t i = 0u; i < PROBE_TEST_IMAGE_SECTORS && !failed; i++) {
            failed = write_iso_sector(bin, iso, i, sector_size, data_offset, mode2);
        }
    }
    if (fclose(bin) != 0) failed = 1;
    bin = NULL;
    if (failed) goto done;

    cue = fopen(cue_path, "wb");
    if (cue == NULL) {
        failed = 1;
        goto done;
    }
    if (!track99) {
        const char *cue_mode = "MODE1/2048";
        if (strcmp(mode, "cue_mode1_2352") == 0) cue_mode = "MODE1/2352";
        else if (strcmp(mode, "cue_mode2_2352") == 0) cue_mode = "MODE2/2352";
        else if (strcmp(mode, "cue_mode2_2336") == 0) cue_mode = "MODE2/2336";
        (void)fprintf(cue, "FILE \"synthetic.bin\" BINARY\nTRACK 01 %s\nINDEX 01 00:00:00\n",
                      cue_mode);
    } else {
        (void)fprintf(cue, "FILE \"synthetic.bin\" BINARY\n");
        for (unsigned i = 0u; i < 98u; i++) {
            (void)fprintf(cue, "TRACK %02u AUDIO\nINDEX 01 ", i + 1u);
            write_msf(cue, i);
            (void)fprintf(cue, "\n");
        }
        (void)fprintf(cue, "TRACK 99 MODE1/2048\nINDEX 01 ");
        write_msf(cue, 98u);
        (void)fprintf(cue, "\n");
    }
    if (fclose(cue) != 0) failed = 1;
    cue = NULL;

done:
    if (source != NULL && fclose(source) != 0) failed = 1;
    if (bin != NULL && fclose(bin) != 0) failed = 1;
    if (cue != NULL && fclose(cue) != 0) failed = 1;
    if (source_path != NULL) (void)remove(source_path);
    free(source_path);
    free(bin_path);
    return failed;
}

int main(int argc, char **argv)
{
    if (argc == 3 && (strcmp(argv[1], "cue_2048") == 0 ||
                      strcmp(argv[1], "cue_mode1_2352") == 0 ||
                      strcmp(argv[1], "cue_mode2_2352") == 0 ||
                      strcmp(argv[1], "cue_mode2_2336") == 0 ||
                      strcmp(argv[1], "cue_track99") == 0)) {
        return make_cue_fixture(argv[2], argv[1]);
    }
    if (argc != 3 || (strcmp(argv[1], "synthetic") != 0 &&
                      strcmp(argv[1], "fallback") != 0 &&
                      strcmp(argv[1], "missing") != 0 &&
                      strcmp(argv[1], "bad_extent") != 0 &&
                      strcmp(argv[1], "traversal") != 0)) {
        (void)fprintf(stderr, "usage: schwaystation-test-probe <synthetic|fallback|missing|bad_extent|traversal|cue_2048|cue_mode1_2352|cue_mode2_2352|cue_mode2_2336|cue_track99> <image>\n");
        return 2;
    }
    return make_synthetic_iso(argv[2], argv[1]);
}
