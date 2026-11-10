/* API version 3, as first released. FROZEN: once released this file never
   changes, except that the version check is a lower bound because a later
   API version keeps every version-3 name. It checks every constant added in
   version 3 and reads the new fault kind back through the version-1 fault
   record from a program that causes it, so a rename, a removal or a changed
   value fails to build or fails here. */
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "../check.h"

static void *v3_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void v3_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static void put32(unsigned char *p, uint32_t v)
{
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
    p[2] = (unsigned char)(v >> 16);
    p[3] = (unsigned char)(v >> 24);
}

int main(void)
{
    CHECK(SCHWAY_API_VERSION >= 3);
    CHECK_EQ_U64(SCHWAY_FAULT_DMA, 10);

    /* A PS-EXE whose program writes the word 11000601h to the control register
       of DMA channel 2 (1F8010A8h): the start bits with the reserved
       synchronisation mode 3, then spins. */
    static unsigned char exe[0x1000];
    memset(exe, 0, sizeof exe);
    memcpy(exe, "PS-X EXE", 8);
    put32(exe + 0x10, 0x80010000u);
    put32(exe + 0x18, 0x80010000u);
    put32(exe + 0x1C, 0x800u);
    put32(exe + 0x30, 0x801FFF00u);
    memcpy(exe + 0x4C, "Sony Computer Entertainment Inc. for Europe area", 48);
    put32(exe + 0x800, 0x3C081F80u);  /* lui   t0, 1F80h        */
    put32(exe + 0x804, 0x3C091100u);  /* lui   t1, 1100h        */
    put32(exe + 0x808, 0x35290601u);  /* ori   t1, t1, 0601h    */
    put32(exe + 0x80C, 0xAD0910A8u);  /* sw    t1, 10A8h(t0)    */
    put32(exe + 0x810, 0x08004004u);  /* j     80010010h        */
    put32(exe + 0x814, 0u);           /* nop                    */

    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = v3_alloc;
    cfg.allocator.free = v3_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    CHECK(inst != NULL);
    if (inst == NULL) {
        CHECK_DONE();
    }
    CHECK_EQ_U64(schway_load_exe(inst, exe, sizeof exe), SCHWAY_STATUS_OK);

    schway_budget budget;
    memset(&budget, 0, sizeof budget);
    budget.size = (uint32_t)sizeof budget;
    budget.flags = SCHWAY_RUN_STOP_ON_FAULT;
    budget.max_cycles = 100000u;
    schway_run_result res;
    memset(&res, 0, sizeof res);
    res.size = (uint32_t)sizeof res;
    CHECK_EQ_U64(schway_run(inst, &budget, &res), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(res.stop, SCHWAY_STOP_FAULT);
    CHECK_EQ_U64(res.fault_present, 1u);

    schway_fault fault;
    memset(&fault, 0, sizeof fault);
    fault.size = (uint32_t)sizeof fault;
    CHECK_EQ_U64(schway_get_fault(inst, &fault), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(fault.kind, SCHWAY_FAULT_DMA);
    CHECK_EQ_U64(fault.device, SCHWAY_DEVICE_DMA);
    CHECK_EQ_U64(fault.width, 4u);
    CHECK_EQ_HEX(fault.addr, 0x1F8010A8u);
    CHECK_EQ_HEX(fault.opcode, 0x11000601u);
    CHECK_EQ_HEX(fault.pc, 0x8001000Cu);
    schway_destroy(inst);
    CHECK_DONE();
}
