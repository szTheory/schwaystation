/* The stub kernel in the core: schwaystation-test-boot <case> [dir]. White-box:
   the test sees src/, so it can read the committed image array and the bus. */
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
    } else {
        fprintf(stderr, "schwaystation-test-boot: unknown case %s\n", c);
        return 2;
    }
    CHECK_DONE();
}
