/* The stub kernel and the PS-EXE boot path in the core: schwaystation-test-boot
   <case> [dir], dir holding the assembled fixtures. White-box: the test sees
   src/, so it can read the committed image array, RAM and the fault record. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

static void *test_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void test_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static schway *make(void)
{
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) {
        exit(1);
    }
    return inst;
}

/* With no BIOS from the host, power-on maps the stub at BFC00000h: every word
   of the image reads back as the array holds it, the BEV vector at BFC00180h
   lies inside the image and starts with a jump, and the first word past the
   image is a bus error that latches an unmapped read of the BIOS.
   Fault prevented: a window that maps another image, reads the bytes in the
   wrong order, or lets a runaway read on past the image. */
static void stub_window(void)
{
    schway *inst = make();
    const uint32_t size = schway__stub_kernel_size;
    CHECK(size > 0x188u && size % 4u == 0u);
    CHECK(inst->bus.bios == schway__stub_kernel_image);
    CHECK_EQ_U64(inst->bus.bios_size, size);
    for (uint32_t off = 0u; off < size; off += 4u) {
        uint32_t want = (uint32_t)schway__stub_kernel_image[off] |
                        ((uint32_t)schway__stub_kernel_image[off + 1u] << 8) |
                        ((uint32_t)schway__stub_kernel_image[off + 2u] << 16) |
                        ((uint32_t)schway__stub_kernel_image[off + 3u] << 24);
        uint32_t got = 0u;
        CHECK_EQ_U64(schway__bus_read(inst, 0xBFC00000u + off, 4u, &got, NULL), 0u);
        CHECK_EQ_HEX(got, want);
    }
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    uint32_t vector = 0u;
    CHECK_EQ_U64(schway__bus_fetch(inst, 0xBFC00180u, &vector, NULL), 0u);
    CHECK_EQ_U64(vector >> 26, 2u); /* J */
    /* Past the image, by a read and by a fetch, inside the 512 KiB window. */
    uint32_t v = 0u;
    CHECK(schway__bus_read(inst, 0xBFC00000u + size, 4u, &v, NULL) != 0);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_READ);
    CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_BIOS);
    CHECK_EQ_HEX(inst->fault.addr, 0xBFC00000u + size);
    CHECK(schway__bus_fetch(inst, 0xBFC00000u + size, &v, NULL) != 0);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_READ); /* the first fault stays */
    schway_destroy(inst);
}

/* The committed array is the assembled stub: stub.bin in the directory
   argument holds the same bytes, and the first difference is printed by its
   offset.
   Fault prevented: a committed stub that no longer matches its source. */
static void stub_drift(const char *dir)
{
    char path[1024];
    int n = snprintf(path, sizeof path, "%s/stub.bin", dir);
    CHECK(n > 0 && (size_t)n < sizeof path);
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "%s: cannot open\n", path);
        check_failures++;
        return;
    }
    uint8_t buf[4096];
    size_t got = fread(buf, 1u, sizeof buf, f);
    int more = fgetc(f);
    fclose(f);
    if (more != EOF) {
        fprintf(stderr, "%s: longer than %zu bytes\n", path, sizeof buf);
        check_failures++;
        return;
    }
    size_t common = got < schway__stub_kernel_size ? got : schway__stub_kernel_size;
    for (size_t i = 0u; i < common; i++) {
        if (buf[i] != schway__stub_kernel_image[i]) {
            fprintf(stderr, "%s: differs from src/stub_kernel.c at offset %zu: %02x != %02x\n", path,
                    i, (unsigned)buf[i], (unsigned)schway__stub_kernel_image[i]);
            check_failures++;
            return;
        }
    }
    if (got != schway__stub_kernel_size) {
        fprintf(stderr, "%s: %zu bytes, src/stub_kernel.c holds %u; they agree up to offset %zu\n",
                path, got, (unsigned)schway__stub_kernel_size, common);
        check_failures++;
    }
}

/* One little-endian word. */
static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* The bytes of dir/name; the caller frees them. A file that cannot be read
   is a failure and ends the test. */
static uint8_t *read_file(const char *dir, const char *name, size_t *n)
{
    char path[1024];
    int len = snprintf(path, sizeof path, "%s/%s", dir, name);
    CHECK(len > 0 && (size_t)len < sizeof path);
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "%s: cannot open\n", path);
        exit(1);
    }
    size_t cap = 1u << 16;
    uint8_t *buf = (uint8_t *)malloc(cap);
    if (buf == NULL) {
        exit(1);
    }
    *n = fread(buf, 1u, cap, f);
    CHECK(*n > 0u && *n < cap);
    fclose(f);
    return buf;
}

/* The mailbox: the 256 bytes at RAM 10000h, which a fixture places at its
   load address 80010000h. */
#define MB_STATUS 8u
#define MB_COUNT 10u
#define MB_RECORDS 16u
static const uint8_t *mailbox(const schway *inst)
{
    return inst->bus.ram + 0x10000u;
}

/* The mailbox is sealed: the magic, and the CRC-32 of bytes 10h to FFh equal
   to the word at 04h. The core's CRC-32 is not the fixture's bitwise routine,
   so the two agree only if both are right. */
static void check_sealed(const schway *inst, uint32_t count)
{
    const uint8_t *mb = mailbox(inst);
    CHECK(memcmp(mb, "SWMB", 4u) == 0);
    CHECK_EQ_HEX(mb[MB_STATUS], 0u);
    CHECK_EQ_U64(mb[9], 1u);
    CHECK_EQ_U64(mb[MB_COUNT], count);
    CHECK_EQ_HEX(le32(mb + 4u), schway__crc32_update(0u, mb + MB_RECORDS, 0x100u - MB_RECORDS));
}

/* The value of record n, 1 first, after its id and valid flag are checked. */
static uint32_t record(const schway *inst, uint32_t n)
{
    const uint8_t *r = mailbox(inst) + MB_RECORDS + 8u * (n - 1u);
    CHECK_EQ_U64((uint32_t)r[0] | ((uint32_t)r[1] << 8), n);
    CHECK_EQ_U64((uint32_t)r[2] | ((uint32_t)r[3] << 8), 1u);
    return le32(r + 4u);
}

static schway_run_result run_frame(schway *inst)
{
    schway_run_result r;
    memset(&r, 0, sizeof r);
    r.size = (uint32_t)sizeof r;
    CHECK_EQ_U64(schway_run_frame(inst, &r), SCHWAY_STATUS_OK);
    return r;
}

static void load(schway *inst, const uint8_t *exe, size_t n)
{
    CHECK_EQ_U64(schway_load_exe(inst, exe, n), SCHWAY_STATUS_OK);
}

/* hello.exe runs from reset through the stub to its own end: the mailbox is
   sealed with status 00h, and its two records are what the stub set up, gp
   from the header's gp0 and sp from its s_addr + s_size. Both come from the
   file's own header here, so a stub that ignores pc0, gp0 or s_addr fails.
   Fault prevented: an image that never starts, or starts with the wrong
   global pointer or stack. */
static void hello_boot(const char *dir)
{
    size_t n = 0u;
    uint8_t *exe = read_file(dir, "hello.exe", &n);
    schway *inst = make();
    load(inst, exe, n);
    for (int f = 0; f < 3 && mailbox(inst)[MB_STATUS] != 0u; f++) {
        schway_run_result r = run_frame(inst);
        CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    }
    check_sealed(inst, 2u);
    CHECK_EQ_HEX(record(inst, 1u), le32(exe + 0x14));
    CHECK_EQ_HEX(record(inst, 2u), le32(exe + 0x30) + le32(exe + 0x34));
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    schway_destroy(inst);
    free(exe);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: schwaystation-test-boot <case> [dir]\n");
        return 2;
    }
    const char *c = argv[1];
    if (strcmp(c, "stub_window") == 0) {
        stub_window();
    } else if (strcmp(c, "stub_drift") == 0 && argc == 3) {
        stub_drift(argv[2]);
    } else if (strcmp(c, "hello") == 0 && argc == 3) {
        hello_boot(argv[2]);
    } else {
        fprintf(stderr, "schwaystation-test-boot: unknown case %s\n", c);
        return 2;
    }
    CHECK_DONE();
}
