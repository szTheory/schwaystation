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

static void clear_fault(schway *inst)
{
    memset(&inst->fault, 0, sizeof inst->fault);
}

/* The memory-control registers read back within their documented bits, and
   every delay write rebuilds that port's six costs {read8, read16, read32,
   write8, write16, write32} from the published formula (psx-spx Memory
   Control; PSX-SYSTEM section 3). Hand-worked with COM_DELAY 00031125h
   (COM0 5, COM2 1, COM3 1):
   - power-on, every port 00000FFFh (delays 16, COM0, COM2 and COM3 selected, 8-bit):
     1ST = (4 + 1 = 5, below 6 so 6) + 16 + 2 = 24, SEQ = 4 + 1 + 18 = 23, so
     8-bit 24, 16-bit 24 + 23 = 47, 32-bit 24 + 3 * 23 = 93, for reads and writes;
   - BIOS 0013243Fh (read delay 4, write delay 16, COM2 selected, 8-bit): read
     1ST = (1, so 2) + 6 = 8, SEQ = 1 + 6 = 7, giving 8, 15, 29 (the 29 cycles of
     PSX-SYSTEM section 3); write 1ST = 2 + 18 = 20, SEQ = 1 + 18 = 19, giving 20, 39, 77;
   - SPU 200931E1h (read delay 15, write delay 2, COM0 selected, 16-bit bus): read
     1ST = (4, so 5) + 17 = 22, SEQ = 4 + 17 = 21, giving 22, 22, 43; write
     1ST = 5 + 4 = 9, SEQ = 4 + 4 = 8, giving 9, 9, 17;
   - then COM_DELAY 0 on the BIOS port: 1ST = (0, so 1) + 6 = 7, SEQ = 6, so the
     read costs are 7, 13, 25.
   Fault prevented: a delay register whose write leaves the costs stale, or a
   register that keeps bits the hardware does not have. */
static void memctl_costs(void)
{
    schway *inst = make();
    struct schway__bus *b = &inst->bus;
    static const uint8_t reset_cost[6] = {24, 47, 93, 24, 47, 93};
    for (uint32_t port = 0; port < SCHWAY__PORT_COUNT; port++) {
        CHECK(memcmp(b->cost[port], reset_cost, sizeof reset_cost) == 0);
    }
    uint32_t v = 0u;
    CHECK_EQ_U64(rd(inst, 0xBF801000u, 4u, &v), 0u);
    CHECK_EQ_HEX(v, 0x1F000000u);
    CHECK_EQ_U64(rd(inst, 0xBF801004u, 4u, &v), 0u);
    CHECK_EQ_HEX(v, 0x1F802000u);
    CHECK_EQ_U64(rd(inst, 0xBF801020u, 4u, &v), 0u);
    CHECK_EQ_HEX(v, 0x00031125u);

    /* BIOS port. */
    CHECK_EQ_U64(wr(inst, 0xBF801010u, 4u, 0x0013243Fu), 0u);
    static const uint8_t bios_cost[6] = {8, 15, 29, 20, 39, 77};
    CHECK(memcmp(b->cost[SCHWAY__PORT_BIOS], bios_cost, sizeof bios_cost) == 0);
    CHECK(memcmp(b->cost[SCHWAY__PORT_SPU], reset_cost, sizeof reset_cost) == 0);
    /* The same value again gives the same table. */
    uint8_t before[SCHWAY__PORT_COUNT][6];
    memcpy(before, b->cost, sizeof before);
    CHECK_EQ_U64(wr(inst, 0xBF801010u, 4u, 0x0013243Fu), 0u);
    CHECK(memcmp(before, b->cost, sizeof before) == 0);
    CHECK_EQ_U64(rd(inst, 0xBF801010u, 4u, &v), 0u);
    CHECK_EQ_HEX(v, 0x0013243Fu);

    /* SPU port, and the charge the bus makes for a read through it. */
    CHECK_EQ_U64(wr(inst, 0xBF801014u, 4u, 0x200931E1u), 0u);
    static const uint8_t spu_cost[6] = {22, 22, 43, 9, 9, 17};
    CHECK(memcmp(b->cost[SCHWAY__PORT_SPU], spu_cost, sizeof spu_cost) == 0);
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_read(inst, 0xBF801C00u, 4u, &v, &cycles), 0u);
    CHECK_EQ_U64(cycles, 43u);
    cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, 0xBF801C00u, 2u, 0u, &cycles), 0u);
    CHECK_EQ_U64(cycles, 9u);

    /* COM_DELAY rebuilds every port. */
    CHECK_EQ_U64(wr(inst, 0xBF801020u, 4u, 0u), 0u);
    static const uint8_t bios_com0[3] = {7, 13, 25};
    CHECK(memcmp(b->cost[SCHWAY__PORT_BIOS], bios_com0, sizeof bios_com0) == 0);

    /* Readback keeps the documented bits: the delay registers have no bits
       21-23 and no address-error flag (bit 28), the bases fix bits 24-31 at
       1Fh, COM_DELAY keeps 20 bits and RAM_SIZE 16. */
    CHECK_EQ_U64(wr(inst, 0xBF801010u, 4u, 0xFFFFFFFFu), 0u);
    CHECK_EQ_U64(rd(inst, 0xBF801010u, 4u, &v), 0u);
    CHECK_EQ_HEX(v, 0xEF1FFFFFu);
    CHECK_EQ_U64(wr(inst, 0xBF801000u, 4u, 0x12345678u), 0u);
    CHECK_EQ_U64(rd(inst, 0xBF801000u, 4u, &v), 0u);
    CHECK_EQ_HEX(v, 0x1F345678u);
    CHECK_EQ_U64(wr(inst, 0xBF801020u, 4u, 0xFFFFFFFFu), 0u);
    CHECK_EQ_U64(rd(inst, 0xBF801020u, 4u, &v), 0u);
    CHECK_EQ_HEX(v, 0x000FFFFFu);
    CHECK_EQ_U64(rd(inst, 0xBF801060u, 4u, &v), 0u);
    CHECK_EQ_HEX(v, 0x0B88u);
    CHECK_EQ_U64(wr(inst, 0xBF801060u, 4u, 0xFFFFFFFFu), 0u);
    CHECK_EQ_U64(rd(inst, 0xBF801060u, 4u, &v), 0u);
    CHECK_EQ_HEX(v, 0xFFFFu);
    /* A byte write changes its byte alone. */
    CHECK_EQ_U64(wr(inst, 0xBF801061u, 1u, 0xABu), 0u);
    CHECK_EQ_U64(rd(inst, 0xBF801060u, 4u, &v), 0u);
    CHECK_EQ_HEX(v, 0xABFFu);
    CHECK_EQ_U64(rd(inst, 0xBF801062u, 2u, &v), 0u);
    CHECK_EQ_HEX(v, 0u);
    CHECK_EQ_U64(rd(inst, 0xBF801061u, 1u, &v), 0u);
    CHECK_EQ_HEX(v, 0xABu);
    /* The hole after COM_DELAY is a gap, and nothing above faulted yet. */
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    CHECK(rd(inst, 0xBF801024u, 4u, &v) != 0);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_READ);
    drop(inst);
}

/* The scratchpad is data in KUSEG and KSEG0, a gap in KSEG1, and never
   executed from (PSX-SYSTEM section 2, D-cache).
   Fault prevented: a KSEG1 alias that answers, a fetch that runs from the
   scratchpad, or an access that runs off its 1 KiB. */
static void scratchpad(void)
{
    schway *inst = make();
    uint32_t v = 0u;
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, 0x1F800000u, 4u, 0x01020304u, &cycles), 0u);
    CHECK_EQ_U64(cycles, 0u);
    CHECK_EQ_HEX(inst->bus.scratch[0], 0x04u);
    CHECK_EQ_HEX(inst->bus.scratch[3], 0x01u);
    CHECK_EQ_U64(rd(inst, 0x9F800000u, 4u, &v), 0u);
    CHECK_EQ_HEX(v, 0x01020304u);
    CHECK_EQ_U64(rd(inst, 0x1F800002u, 2u, &v), 0u);
    CHECK_EQ_HEX(v, 0x0102u);
    CHECK_EQ_U64(wr(inst, 0x9F8003FFu, 1u, 0x5Au), 0u);
    CHECK_EQ_HEX(inst->bus.scratch[0x3FF], 0x5Au);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);

    /* KSEG1 is a gap that names the scratchpad. */
    CHECK(rd(inst, 0xBF800000u, 4u, &v) != 0);
    CHECK_EQ_U64(v, 0u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_READ);
    CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_SCRATCH);
    clear_fault(inst);
    CHECK(wr(inst, 0xBF800000u, 4u, 0xFFFFFFFFu) != 0);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_WRITE);
    CHECK_EQ_HEX(inst->bus.scratch[0], 0x04u);
    /* A fetch is a bus error in every segment. */
    clear_fault(inst);
    CHECK(schway__bus_fetch(inst, 0x9F800000u, &v, &cycles) != 0);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_FETCH);
    CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_SCRATCH);
    CHECK(schway__bus_fetch(inst, 0x1F800000u, &v, &cycles) != 0);
    /* The end of the 1 KiB: a word that starts inside and ends outside is an
       error, and the next address is a hole in no device. */
    clear_fault(inst);
    CHECK(rd(inst, 0x1F8003FEu, 4u, &v) != 0);
    CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_SCRATCH);
    clear_fault(inst);
    CHECK(rd(inst, 0x1F800400u, 1u, &v) != 0);
    CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_UNMAPPED);
    drop(inst);
}

/* The BIOS window reads the image mapped into it: the stub kernel after
   create, and a host's image once one is mapped. The image survives a power
   reset, a write to the ROM is dropped, and its cost follows the DEV2
   register.
   Fault prevented: a window that reads past the image, answers with nothing
   mapped, or loses the image at reset. */
static void bios_window(void)
{
    schway *inst = make();
    uint32_t v = 0u;
    CHECK(inst->bus.bios == schway__stub_kernel_image);
    CHECK_EQ_U64(rd(inst, 0xBFC00000u, 4u, &v), 0u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    /* With nothing mapped the window is a gap. */
    inst->bus.bios = NULL;
    CHECK(rd(inst, 0xBFC00000u, 4u, &v) != 0);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_READ);
    CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_BIOS);
    clear_fault(inst);

    static const uint8_t rom[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    inst->bus.bios = rom;
    inst->bus.bios_size = 16u;
    CHECK_EQ_U64(rd(inst, 0xBFC00000u, 4u, &v), 0u);
    CHECK_EQ_HEX(v, 0x33221100u);
    CHECK_EQ_U64(rd(inst, 0x9FC00004u, 4u, &v), 0u);
    CHECK_EQ_HEX(v, 0x77665544u);
    CHECK_EQ_U64(rd(inst, 0x1FC00008u, 2u, &v), 0u);
    CHECK_EQ_HEX(v, 0x9988u);
    CHECK_EQ_U64(rd(inst, 0xBFC0000Fu, 1u, &v), 0u);
    CHECK_EQ_HEX(v, 0xFFu);
    CHECK_EQ_U64(schway__bus_fetch(inst, 0xBFC0000Cu, &v, NULL), 0u);
    CHECK_EQ_HEX(v, 0xFFEEDDCCu);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    /* Past the image, and a word that ends past it, are gaps. */
    CHECK(rd(inst, 0xBFC00010u, 1u, &v) != 0);
    CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_BIOS);
    CHECK(rd(inst, 0xBFC0000Eu, 4u, &v) != 0);
    /* The window is 512 KiB: the next address is in no device. */
    clear_fault(inst);
    CHECK(rd(inst, 0xBFC80000u, 1u, &v) != 0);
    CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_UNMAPPED);
    /* A write to the ROM changes nothing and is no error. */
    clear_fault(inst);
    CHECK_EQ_U64(wr(inst, 0xBFC00000u, 4u, 0xFFFFFFFFu), 0u);
    CHECK_EQ_HEX(rom[0], 0x00u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    /* The charge: the maximum delays at power-on, then 8 + 7 + 7 + 7 = 29
       cycles for a word once DEV2 holds the boot value. */
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_read(inst, 0xBFC00000u, 4u, &v, &cycles), 0u);
    CHECK_EQ_U64(cycles, 93u);
    CHECK_EQ_U64(wr(inst, 0xBF801010u, 4u, 0x0013243Fu), 0u);
    cycles = 0u;
    CHECK_EQ_U64(schway__bus_read(inst, 0xBFC00000u, 4u, &v, &cycles), 0u);
    CHECK_EQ_U64(cycles, 29u);
    /* The image is the host's: a power reset keeps the pointer. */
    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    CHECK(inst->bus.bios == rom);
    CHECK_EQ_U64(inst->bus.bios_size, 16u);
    drop(inst);
}

/* The expansion regions: reads of the first and third are bus errors naming
   them and their writes are dropped; the second answers only its two debug
   ports, and those act only while the profile's debug-port flag is set.
   Fault prevented: an expansion read that returns data, a debug port that
   writes with the flag clear, or a neighbour of a port that answers. */
static void exp_regions(void)
{
    schway *inst = make();
    uint32_t v = 0u;
    static const uint32_t widths[3] = {1u, 2u, 4u};
    for (size_t i = 0; i < 3u; i++) {
        clear_fault(inst);
        CHECK(rd(inst, 0xBF000000u, widths[i], &v) != 0);
        CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_READ);
        CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_EXP1);
        CHECK_EQ_U64(inst->fault.width, widths[i]);
        clear_fault(inst);
        CHECK(rd(inst, 0xBFBFFFFCu & ~(widths[i] - 1u), widths[i], &v) != 0);
        CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_EXP3);
    }
    clear_fault(inst);
    CHECK(rd(inst, 0x9F7FFFFFu, 1u, &v) != 0);
    CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_EXP1);
    CHECK_EQ_U64(wr(inst, 0xBF000000u, 4u, 1u), 0u);
    CHECK_EQ_U64(wr(inst, 0xBFA00000u, 1u, 1u), 0u);
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, 0xBF000000u, 1u, 1u, &cycles), 0u);
    CHECK_EQ_U64(cycles, 24u);
    /* The holes either side of the regions are in no device. */
    clear_fault(inst);
    CHECK(rd(inst, 0xBF800800u, 1u, &v) != 0);
    CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_UNMAPPED);
    clear_fault(inst);
    CHECK(rd(inst, 0xBFC80000u, 1u, &v) != 0);
    CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_UNMAPPED);

    /* The second expansion region: every address but two is a gap. */
    clear_fault(inst);
    CHECK(rd(inst, 0xBF802000u, 1u, &v) != 0);
    CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_EXP2);
    CHECK(wr(inst, 0xBF802041u, 1u, 1u) != 0);
    CHECK(rd(inst, 0xBF802081u, 1u, &v) != 0);
    CHECK(rd(inst, 0xBF803FFFu, 1u, &v) != 0);
    clear_fault(inst);
    CHECK(rd(inst, 0xBF804000u, 1u, &v) != 0);
    CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_UNMAPPED);

    /* Flag clear: the ports answer and do nothing. */
    clear_fault(inst);
    CHECK_EQ_U64(wr(inst, 0xBF802080u, 1u, 'A'), 0u);
    CHECK_EQ_U64(wr(inst, 0xBF802082u, 1u, 7u), 0u);
    CHECK_EQ_U64(rd(inst, 0xBF802080u, 1u, &v), 0u);
    CHECK_EQ_U64(inst->tty.count, 0u);
    CHECK_EQ_U64(inst->exit_pending, 0u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    /* Flag set: a byte reaches the TTY ring, the exit code is kept, and a
       wider write does neither. */
    inst->profile_flags = SCHWAY__PROFILE_DEBUG_PORT;
    CHECK_EQ_U64(wr(inst, 0xBF802080u, 1u, 'A'), 0u);
    CHECK_EQ_U64(wr(inst, 0xBF802080u, 1u, 'B'), 0u);
    CHECK_EQ_U64(inst->tty.count, 2u);
    CHECK_EQ_HEX(inst->tty.buf[0], 'A');
    CHECK_EQ_HEX(inst->tty.buf[1], 'B');
    CHECK_EQ_U64(wr(inst, 0xBF802080u, 2u, 'C'), 0u);
    CHECK_EQ_U64(wr(inst, 0xBF802080u, 4u, 'C'), 0u);
    CHECK_EQ_U64(inst->tty.count, 2u);
    CHECK_EQ_U64(inst->exit_pending, 0u);
    CHECK_EQ_U64(wr(inst, 0xBF802082u, 1u, 0x1FFu), 0u);
    CHECK_EQ_U64(inst->exit_pending, 1u);
    CHECK_EQ_U64(inst->exit_code, 0xFFu);
    CHECK_EQ_U64(wr(inst, 0xBF802082u, 1u, 0u), 0u);
    CHECK_EQ_U64(inst->exit_code, 0u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    drop(inst);
}

/* KSEG2 answers only the cache-control word, by word access; every other
   KSEG2 address is a bus error, and KUSEG, KSEG0 and KSEG1 all map to the
   address masked to 1FFFFFFFh.
   Fault prevented: a segment that classifies wrong, a KSEG2 hole that
   answers, or a KUSEG address that misses its alias. */
static void kseg2(void)
{
    schway *inst = make();
    uint32_t v = 0u;
    static const uint32_t seg_in[8] = {0x00000000u, 0x7FFFFFFFu, 0x80000000u, 0x9FFFFFFFu,
                                       0xA0000000u, 0xBFFFFFFFu, 0xC0000000u, 0xFFFFFFFFu};
    static const uint32_t seg_out[8] = {SCHWAY__SEG_KUSEG, SCHWAY__SEG_KUSEG, SCHWAY__SEG_KSEG0,
                                        SCHWAY__SEG_KSEG0, SCHWAY__SEG_KSEG1, SCHWAY__SEG_KSEG1,
                                        SCHWAY__SEG_KSEG2, SCHWAY__SEG_KSEG2};
    for (size_t i = 0; i < 8u; i++) {
        CHECK_EQ_U64(schway__bus_segment(seg_in[i]), seg_out[i]);
    }
    CHECK_EQ_U64(wr(inst, 0xFFFE0130u, 4u, 0x00000804u), 0u);
    CHECK_EQ_HEX(inst->bus.cache_ctrl, 0x804u);
    CHECK_EQ_U64(rd(inst, 0xFFFE0130u, 4u, &v), 0u);
    CHECK_EQ_HEX(v, 0x804u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    /* Word access only, and no fetch. */
    CHECK(rd(inst, 0xFFFE0130u, 2u, &v) != 0);
    CHECK(rd(inst, 0xFFFE0130u, 1u, &v) != 0);
    CHECK(wr(inst, 0xFFFE0130u, 2u, 0u) != 0);
    CHECK(schway__bus_fetch(inst, 0xFFFE0130u, &v, NULL) != 0);
    CHECK_EQ_HEX(inst->bus.cache_ctrl, 0x804u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_READ);
    CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_UNMAPPED);
    CHECK_EQ_HEX(inst->fault.addr, 0xFFFE0130u);
    /* The neighbours and the rest of KSEG2 are errors. */
    static const uint32_t holes[5] = {0xFFFE0134u, 0xFFFE012Cu, 0xC0000000u, 0xE0000000u,
                                      0xFFFFFFFCu};
    for (size_t i = 0; i < 5u; i++) {
        clear_fault(inst);
        CHECK(rd(inst, holes[i], 4u, &v) != 0);
        CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_READ);
        CHECK_EQ_HEX(inst->fault.addr, holes[i]);
        CHECK(wr(inst, holes[i], 4u, 1u) != 0);
    }
    /* A KUSEG address past 512 MiB masks onto the physical map: RAM at 100h
       and the memory-control register at 1F801000h. */
    CHECK_EQ_U64(wr(inst, 0x60000100u, 4u, 0xCAFEF00Du), 0u);
    CHECK_EQ_U64(rd(inst, 0x00000100u, 4u, &v), 0u);
    CHECK_EQ_HEX(v, 0xCAFEF00Du);
    CHECK_EQ_U64(rd(inst, 0x7F801000u, 4u, &v), 0u);
    CHECK_EQ_HEX(v, 0x1F000000u);
    /* A KUSEG hole is a gap. */
    clear_fault(inst);
    CHECK(rd(inst, 0x30000000u, 4u, &v) != 0);
    CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_UNMAPPED);
    drop(inst);
}

/* I_STAT: a bit sets on the 0 to 1 edge through the raise entry and a bit
   already set stays; a written 0 clears and a written 1 leaves; narrow writes
   reach their own bytes; bits above 10 do not exist. Reads see bits 0-10.
   Fault prevented: an acknowledge that clears the wrong bits, or a source
   number past 10 that sets a bit.
   Probes: idempotency (raising a set bit, writing the same value twice). */
static void irq_edge(void)
{
    schway *inst = make();
    uint32_t v = 0u;
    schway__irq_raise(inst, 2u);
    CHECK_EQ_HEX(inst->irq.stat, 0x4u);
    schway__irq_raise(inst, 2u);
    CHECK_EQ_HEX(inst->irq.stat, 0x4u);
    for (uint32_t bit = 0u; bit < 11u; bit++) {
        schway__irq_raise(inst, bit);
    }
    CHECK_EQ_HEX(inst->irq.stat, 0x7FFu);
    schway__irq_raise(inst, 11u);
    schway__irq_raise(inst, 31u);
    schway__irq_raise(inst, 0xFFFFFFFFu);
    CHECK_EQ_HEX(inst->irq.stat, 0x7FFu);
    CHECK_EQ_U64(rd(inst, 0xBF801070u, 4u, &v), 0u);
    CHECK_EQ_HEX(v, 0x7FFu);
    CHECK_EQ_U64(rd(inst, 0xBF801071u, 1u, &v), 0u);
    CHECK_EQ_HEX(v, 0x07u);
    /* Written 1 leaves. */
    CHECK_EQ_U64(wr(inst, 0xBF801070u, 4u, 0xFFFFFFFFu), 0u);
    CHECK_EQ_HEX(inst->irq.stat, 0x7FFu);
    /* Written 0 clears, and the same write twice changes nothing more. */
    CHECK_EQ_U64(wr(inst, 0xBF801070u, 4u, 0xFFFFFFFBu), 0u);
    CHECK_EQ_HEX(inst->irq.stat, 0x7FBu);
    CHECK_EQ_U64(wr(inst, 0xBF801070u, 4u, 0xFFFFFFFBu), 0u);
    CHECK_EQ_HEX(inst->irq.stat, 0x7FBu);
    /* A byte at 1F801071h reaches bits 8-15 only; a halfword reaches 0-15. */
    CHECK_EQ_U64(wr(inst, 0xBF801071u, 1u, 0x0u), 0u);
    CHECK_EQ_HEX(inst->irq.stat, 0x0FBu);
    CHECK_EQ_U64(wr(inst, 0xBF801070u, 2u, 0xF0F0u), 0u);
    CHECK_EQ_HEX(inst->irq.stat, 0x0F0u);
    CHECK_EQ_U64(wr(inst, 0xBF801070u, 4u, 0u), 0u);
    CHECK_EQ_HEX(inst->irq.stat, 0u);
    CHECK_EQ_HEX(inst->irq.mask, 0u);
    /* The word after I_MASK is a hole. */
    CHECK(rd(inst, 0xBF801078u, 4u, &v) != 0);
    drop(inst);
}

/* The level the CPU sees is (I_STAT & I_MASK) != 0. I_MASK keeps bits 0-10,
   and is untouched by raising or acknowledging.
   Fault prevented: a line that ignores the mask, or a mask that keeps bits
   the register does not have.
   Probe: empty (nothing raised, nothing enabled). */
static void irq_line(void)
{
    schway *inst = make();
    uint32_t v = 0u;
    CHECK_EQ_U64(schway__irq_line(inst), 0u);
    schway__irq_raise(inst, 3u);
    CHECK_EQ_U64(schway__irq_line(inst), 0u);
    CHECK_EQ_U64(wr(inst, 0xBF801074u, 4u, 0x8u), 0u);
    CHECK_EQ_U64(schway__irq_line(inst), 1u);
    CHECK_EQ_U64(wr(inst, 0xBF801074u, 4u, 0x4u), 0u);
    CHECK_EQ_U64(schway__irq_line(inst), 0u);
    CHECK_EQ_U64(wr(inst, 0xBF801074u, 4u, 0xFFFFFFFFu), 0u);
    CHECK_EQ_HEX(inst->irq.mask, 0x7FFu);
    CHECK_EQ_U64(rd(inst, 0xBF801074u, 4u, &v), 0u);
    CHECK_EQ_HEX(v, 0x7FFu);
    CHECK_EQ_U64(schway__irq_line(inst), 1u);
    CHECK_EQ_HEX(inst->irq.stat, 0x8u);
    /* A byte write to the upper byte keeps the lower. */
    CHECK_EQ_U64(wr(inst, 0xBF801075u, 1u, 0u), 0u);
    CHECK_EQ_HEX(inst->irq.mask, 0x0FFu);
    CHECK_EQ_U64(wr(inst, 0xBF801070u, 4u, 0u), 0u);
    CHECK_EQ_U64(schway__irq_line(inst), 0u);
    drop(inst);
}

/* CAUSE bit 10 follows the level at the write or raise that changes it, with
   no delay, so a read right after each change sees it; CAUSE bits 8-9 are
   software's and stay.
   Fault prevented: a CAUSE bit that lags the registers, or one that clears
   the software latches.
   Probes: idempotency (the same write twice) and empty (mask 0 hides a raise). */
static void irq_cause_bit(void)
{
    schway *inst = make();
    uint32_t *cause = &inst->cpu.cop0.r[SCHWAY__COP0_CAUSE];
    *cause = 0x00000300u;
#define BIT10() ((*cause >> 10) & 1u)
    CHECK_EQ_U64(BIT10(), 0u);
    schway__irq_raise(inst, 2u);
    CHECK_EQ_U64(BIT10(), 0u); /* mask 0 hides it */
    CHECK_EQ_U64(wr(inst, 0xBF801074u, 4u, 0x4u), 0u);
    CHECK_EQ_U64(BIT10(), 1u);
    CHECK_EQ_U64(wr(inst, 0xBF801074u, 4u, 0x4u), 0u);
    CHECK_EQ_U64(BIT10(), 1u);
    CHECK_EQ_U64(wr(inst, 0xBF801070u, 4u, ~0x4u), 0u);
    CHECK_EQ_U64(BIT10(), 0u);
    schway__irq_raise(inst, 2u);
    CHECK_EQ_U64(BIT10(), 1u);
    schway__irq_raise(inst, 2u); /* raising a set bit changes nothing */
    CHECK_EQ_U64(BIT10(), 1u);
    CHECK_EQ_U64(wr(inst, 0xBF801074u, 4u, 0u), 0u);
    CHECK_EQ_U64(BIT10(), 0u);
    schway__irq_raise(inst, 9u);
    CHECK_EQ_U64(BIT10(), 0u);
    CHECK_EQ_U64(wr(inst, 0xBF801074u, 4u, 0x200u), 0u);
    CHECK_EQ_U64(BIT10(), 1u);
    CHECK_EQ_U64(wr(inst, 0xBF801070u, 4u, 0u), 0u);
    CHECK_EQ_U64(BIT10(), 0u);
    /* Bits 8-9 and the rest of CAUSE were never touched. */
    CHECK_EQ_HEX(*cause, 0x00000300u);
#undef BIT10
    drop(inst);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-bus <case>\n");
        return 2;
    }
    const char *c = argv[1];
    if (strcmp(c, "ram_mirrors") == 0) {
        ram_mirrors();
    } else if (strcmp(c, "memctl_costs") == 0) {
        memctl_costs();
    } else if (strcmp(c, "scratchpad") == 0) {
        scratchpad();
    } else if (strcmp(c, "bios_window") == 0) {
        bios_window();
    } else if (strcmp(c, "exp_regions") == 0) {
        exp_regions();
    } else if (strcmp(c, "kseg2") == 0) {
        kseg2();
    } else if (strcmp(c, "irq_edge") == 0) {
        irq_edge();
    } else if (strcmp(c, "irq_line") == 0) {
        irq_line();
    } else if (strcmp(c, "irq_cause_bit") == 0) {
        irq_cause_bit();
    } else {
        fprintf(stderr, "schwaystation-test-bus: unknown case %s\n", c);
        return 2;
    }
    CHECK_DONE();
}
