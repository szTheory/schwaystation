/* Sweeps the I/O page, 1F801000h-1F801FFFh, at widths 1, 2 and 4 through KSEG1
   and classifies every address by its effect: owned (answers, no count, no
   fault), unbuilt (reads 0, drops a write, counts, no fault) or a gap (bus
   error and a first fault in no device). Each class is also checked against
   the documented ranges, written out here independently of the decoder, so an
   address in two classes or none, or on the wrong side of an edge such as
   1F80105Fh against 1F801060h, fails. Prints the three counts. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

enum { OWNED, UNBUILT, GAP };

/* The documented map, as offsets in the page (docs/hardware/bus.md, Segments and the physical map):
   memory control 000h-023h, RAM_SIZE 060h-063h, I_STAT and I_MASK 070h-077h,
   the DMA registers 080h-0FFh and the timers 100h-12Fh are owned; SIO
   040h-05Fh, CD-ROM 800h-803h, MDEC 820h-827h and SPU C00h-FFFh are
   unbuilt; the GPU owns 810h-817h. */
static int expected(uint32_t off)
{
    if (off <= 0x023u || (off >= 0x060u && off <= 0x063u) || (off >= 0x070u && off <= 0x077u) ||
        (off >= 0x080u && off <= 0x12Fu) || (off >= 0x810u && off <= 0x817u)) {
        return OWNED;
    }
    if ((off >= 0x040u && off <= 0x05Fu) || (off >= 0x800u && off <= 0x803u) ||
        (off >= 0x820u && off <= 0x827u) || off >= 0xC00u) {
        return UNBUILT;
    }
    return GAP;
}

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

/* What one access did to the instance. */
struct effect {
    int err;
    uint32_t value;
    uint32_t counted; /* sum of the unbuilt counters */
    uint32_t fault_kind, fault_device;
};

static uint32_t counted(const schway *inst)
{
    uint32_t n = 0u;
    for (uint32_t i = 0; i < SCHWAY__DEV_COUNT; i++) {
        n += inst->bus.unbuilt_n[i];
    }
    return n;
}

int main(void)
{
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    schway *inst = NULL;
    if (schway_create(&cfg, &inst) != SCHWAY_STATUS_OK || inst == NULL) {
        return 1;
    }

    /* The state an I/O access can touch, saved at power-on and put back
       before every access, so each one starts from a power reset (a full
       reset also clears 3 MiB of RAM and VRAM, which this page never reaches). */
    uint32_t memctl[SCHWAY__MEMCTL_REGS];
    uint8_t cost[SCHWAY__PORT_COUNT][6];
    memcpy(memctl, inst->bus.memctl, sizeof memctl);
    memcpy(cost, inst->bus.cost, sizeof cost);
    uint32_t ram_size = inst->bus.ram_size;

    uint32_t totals[2][3] = {{0u, 0u, 0u}, {0u, 0u, 0u}};
    static const uint32_t widths[3] = {1u, 2u, 4u};
    for (uint32_t wr = 0u; wr < 2u; wr++) {
        for (uint32_t w = 0u; w < 3u; w++) {
            for (uint32_t off = 0u; off < 0x1000u; off++) {
                memcpy(inst->bus.memctl, memctl, sizeof memctl);
                memcpy(inst->bus.cost, cost, sizeof cost);
                inst->bus.ram_size = ram_size;
                inst->bus.cache_ctrl = 0u;
                memset(inst->bus.unbuilt_n, 0, sizeof inst->bus.unbuilt_n);
                memset(inst->bus.last_unbuilt, 0, sizeof inst->bus.last_unbuilt);
                memset(&inst->fault, 0, sizeof inst->fault);
                inst->cpu.pc = 0x80010000u;
                schway__irq_reset(inst);
                schway__dma_reset(inst);
                schway__timers_reset(inst);
                inst->cpu.cop0.r[SCHWAY__COP0_CAUSE] = 0u;

                uint32_t addr = 0xBF801000u + off;
                struct effect e;
                e.value = 0xFFFFFFFFu;
                uint32_t cycles = 0u;
                e.err = wr != 0u ? schway__bus_write(inst, addr, widths[w], 0u, &cycles)
                                 : schway__bus_read(inst, addr, widths[w], &e.value, &cycles);
                e.counted = counted(inst);
                e.fault_kind = inst->fault.kind;
                e.fault_device = inst->fault.device;

                int cls;
                if (e.err != 0) {
                    cls = GAP;
                    CHECK_EQ_U64(e.fault_kind,
                                 wr != 0u ? SCHWAY__FAULT_UNMAPPED_WRITE : SCHWAY__FAULT_UNMAPPED_READ);
                    CHECK_EQ_U64(e.fault_device, SCHWAY__DEV_UNMAPPED);
                    CHECK_EQ_U64(e.counted, 0u);
                    CHECK_EQ_U64(cycles, 0u);
                    if (wr == 0u) {
                        CHECK_EQ_U64(e.value, 0u);
                    }
                } else if (e.counted != 0u) {
                    cls = UNBUILT;
                    CHECK_EQ_U64(e.counted, 1u);
                    CHECK_EQ_U64(e.fault_kind, SCHWAY__FAULT_NONE);
                    CHECK_EQ_HEX(inst->bus.last_unbuilt[1], addr & 0x1FFFFFFFu);
                    CHECK_EQ_U64(inst->bus.last_unbuilt[2], widths[w]);
                    CHECK_EQ_HEX(inst->bus.last_unbuilt[3], 0x80010000u);
                    CHECK(cycles != 0u);
                    if (wr == 0u) {
                        CHECK_EQ_U64(e.value, 0u);
                    }
                } else {
                    cls = OWNED;
                    CHECK_EQ_U64(e.fault_kind, SCHWAY__FAULT_NONE);
                    CHECK(cycles != 0u);
                }
                if (cls != expected(off)) {
                    fprintf(stderr, "bus_unbuilt: 1F801%03X width %u %s is class %d, expected %d\n",
                            (unsigned)off, (unsigned)widths[w], wr != 0u ? "write" : "read", cls,
                            expected(off));
                    check_failures++;
                }
                totals[wr][cls]++;
            }
        }
    }
    CHECK(memcmp(totals[0], totals[1], sizeof totals[0]) == 0);
    printf("bus_unbuilt: owned=%u unbuilt=%u gap=%u\n", (unsigned)totals[0][OWNED],
           (unsigned)totals[0][UNBUILT], (unsigned)totals[0][GAP]);
    CHECK_EQ_U64(totals[0][OWNED] + totals[0][UNBUILT] + totals[0][GAP], 3u * 0x1000u);
    schway_destroy(inst);
    CHECK_DONE();
}
