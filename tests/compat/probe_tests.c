/* Small ISO9660 fixtures for the host-only compatibility probe tests. */
#include <stdint.h>
#include <stdio.h>
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

int main(int argc, char **argv)
{
    if (argc != 3 || (strcmp(argv[1], "synthetic") != 0 &&
                      strcmp(argv[1], "fallback") != 0 &&
                      strcmp(argv[1], "missing") != 0 &&
                      strcmp(argv[1], "bad_extent") != 0 &&
                      strcmp(argv[1], "traversal") != 0)) {
        (void)fprintf(stderr, "usage: schwaystation-test-probe <synthetic|fallback|missing|bad_extent|traversal> <image.iso>\n");
        return 2;
    }
    return make_synthetic_iso(argv[2], argv[1]);
}
