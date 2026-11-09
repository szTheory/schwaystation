/* The bus and the interrupt registers through the module's own interface:
   schwaystation-test-bus <case>. White-box: the test sees src/, so it can
   drive the bus directly, load a BIOS image and read the fault record. */
#include <stdint.h>
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

static void drop(schway *inst)
{
    schway_destroy(inst);
}

/* One access: the status, with the value read in *v. */
static int rd(schway *inst, uint32_t addr, uint32_t width, uint32_t *v)
{
    uint32_t cycles = 0u;
    return schway__bus_read(inst, addr, width, v, &cycles);
}

static int wr(schway *inst, uint32_t addr, uint32_t width, uint32_t v)
{
    uint32_t cycles = 0u;
    return schway__bus_write(inst, addr, width, v, &cycles);
}

/* A word written through KSEG1 reads back through KUSEG, KSEG0 and every RAM
   mirror, a write through the third mirror reads back through the others, and
   RAM_SIZE decides where the banks end. Hand-worked from the DRAM_CTRL bank
   rule (/RAS0 size = 1 MiB << (bit 9 * 2 + bit 11), /RAS1 likewise from bits 8
   and 11, enabled by bit 10): 0B88h is bits 9 and 11 set, so an 8 MiB bank
   mirrors the 2 MiB four times; 0888h is bit 11 alone, a 2 MiB bank; 0000h is
   a 1 MiB bank; 0C88h is two 2 MiB banks, and the one on /RAS1 maps back onto
   the same 2 MiB.
   Fault prevented: a mirror that reads zero or another address, a window past
   the banks that answers, or an access that misses the first fault. */
static void ram_mirrors(void)
{
    schway *inst = make();
    uint32_t v = 0u;
    CHECK_EQ_HEX(inst->bus.ram_size, 0x0B88u);
    CHECK_EQ_U64(wr(inst, 0xA0001000u, 4u, 0x11223344u), 0u);
    static const uint32_t same[] = {0x00001000u, 0x80001000u, 0xA0001000u, 0x00201000u,
                                    0x80401000u, 0xA0601000u};
    for (size_t i = 0; i < sizeof same / sizeof same[0]; i++) {
        v = 0u;
        CHECK_EQ_U64(rd(inst, same[i], 4u, &v), 0u);
        CHECK_EQ_HEX(v, 0x11223344u);
    }
    /* A byte written through the third mirror shows in the first, second and
       fourth. */
    CHECK_EQ_U64(wr(inst, 0x00402000u, 1u, 0xABu), 0u);
    static const uint32_t third[] = {0x00002000u, 0x00202000u, 0x00602000u};
    for (size_t i = 0; i < sizeof third / sizeof third[0]; i++) {
        CHECK_EQ_U64(rd(inst, third[i], 1u, &v), 0u);
        CHECK_EQ_HEX(v, 0xABu);
    }
    /* Past 8 MiB is a bus error in no device, and the first fault is kept. */
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    CHECK(rd(inst, 0x00800000u, 4u, &v) != 0);
    CHECK_EQ_U64(v, 0u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_READ);
    CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_UNMAPPED);
    CHECK_EQ_HEX(inst->fault.addr, 0x00800000u);
    CHECK(wr(inst, 0x00800000u, 4u, 1u) != 0);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_READ);
    /* A word that starts inside the window and ends outside it is an error
       and writes nothing. */
    CHECK(wr(inst, 0x007FFFFEu, 4u, 0xFFFFFFFFu) != 0);
    CHECK_EQ_U64(rd(inst, 0x001FFFFEu, 2u, &v), 0u);
    CHECK_EQ_HEX(v, 0u);
    /* A word across a 2 MiB mirror boundary inside the window reads on into
       the next mirror, which is the start of RAM. */
    CHECK_EQ_U64(wr(inst, 0x0000u, 4u, 0x44332211u), 0u);
    CHECK_EQ_U64(wr(inst, 0x1FFFFEu, 2u, 0x8877u), 0u);
    CHECK_EQ_U64(rd(inst, 0x1FFFFEu, 4u, &v), 0u);
    CHECK_EQ_HEX(v, 0x22118877u);

    /* A 2 MiB bank: no mirror, and 200000h is a bus error. */
    inst->bus.ram_size = 0x0888u;
    memset(&inst->fault, 0, sizeof inst->fault);
    CHECK_EQ_U64(rd(inst, 0x001FFFFCu, 4u, &v), 0u);
    CHECK(rd(inst, 0x00200000u, 4u, &v) != 0);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_READ);
    CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_RAM);
    /* A 1 MiB bank wraps its own megabyte. */
    inst->bus.ram_size = 0x0000u;
    CHECK_EQ_U64(wr(inst, 0x000FFFFCu, 4u, 0x01020304u), 0u);
    CHECK_EQ_U64(rd(inst, 0x000FFFFCu, 4u, &v), 0u);
    CHECK_EQ_HEX(v, 0x01020304u);
    CHECK(rd(inst, 0x00100000u, 4u, &v) != 0);
    /* Two enabled banks of 2 MiB: 400000h is the second bank, which maps onto
       the same RAM. */
    inst->bus.ram_size = 0x0C88u;
    CHECK_EQ_U64(wr(inst, 0x00000100u, 4u, 0x5555AAAAu), 0u);
    CHECK_EQ_U64(rd(inst, 0x00200100u, 4u, &v), 0u);
    CHECK_EQ_HEX(v, 0x5555AAAAu);
    CHECK(rd(inst, 0x00400000u, 4u, &v) != 0);

    /* The charge: RAM reads cost 4 plus a word, writes 1. */
    inst->bus.ram_size = SCHWAY__RAM_SIZE_POWER_ON;
    uint32_t cycles = 10u;
    CHECK_EQ_U64(schway__bus_read(inst, 0x80000000u, 4u, &v, &cycles), 0u);
    CHECK_EQ_U64(cycles, 15u);
    CHECK_EQ_U64(schway__bus_write(inst, 0x80000000u, 4u, 0u, &cycles), 0u);
    CHECK_EQ_U64(cycles, 16u);
    drop(inst);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-bus <case>\n");
        return 2;
    }
    if (strcmp(argv[1], "ram_mirrors") == 0) {
        ram_mirrors();
    } else {
        fprintf(stderr, "schwaystation-test-bus: unknown case %s\n", argv[1]);
        return 2;
    }
    CHECK_DONE();
}
