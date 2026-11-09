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

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: schwaystation-test-boot <case> [dir]\n");
        return 2;
    }
    const char *c = argv[1];
    if (strcmp(c, "stub_window") == 0) {
        stub_window();
    } else {
        fprintf(stderr, "schwaystation-test-boot: unknown case %s\n", c);
        return 2;
    }
    CHECK_DONE();
}
