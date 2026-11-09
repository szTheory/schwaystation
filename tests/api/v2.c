/* API version 2, as first released. FROZEN: once released this file never
   changes, except that the version check is a lower bound because a later
   API version keeps every version-2 name. It calls every function added in
   version 2, assigns every field and checks every constant it added, so a
   rename, a removal or a changed value fails to build or fails here. */
#include <string.h>

#include <stdlib.h>

#include "schwaystation.h"
#include "../check.h"

static void *v2_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void v2_free(void *user, void *ptr, size_t size)
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
    CHECK(SCHWAY_API_VERSION >= 2);
    CHECK_EQ_U64(SCHWAY_STOP_FRAME, 1);
    CHECK_EQ_U64(SCHWAY_STOP_BUDGET, 2);
    CHECK_EQ_U64(SCHWAY_STOP_HOOK, 4);
    CHECK_EQ_U64(SCHWAY_STOP_FAULT, 5);
    CHECK_EQ_U64(SCHWAY_RUN_STOP_ON_FAULT, 1);
    CHECK_EQ_U64(SCHWAY_RUN_TRACE, 2);
    CHECK_EQ_U64(SCHWAY_TRACE_EXEC, 1);
    CHECK_EQ_U64(SCHWAY_TRACE_RD, 2);
    CHECK_EQ_U64(SCHWAY_TRACE_WR, 3);
    CHECK_EQ_U64(SCHWAY_TRACE_IRQ, 4);
    CHECK_EQ_U64(SCHWAY_TRACE_FAULT, 5);
    CHECK_EQ_U64(SCHWAY_SPACE_RAM, 1);
    CHECK_EQ_U64(SCHWAY_SPACE_SCRATCHPAD, 2);
    CHECK_EQ_U64(SCHWAY_FAULT_NONE, 0);
    CHECK_EQ_U64(SCHWAY_FAULT_UNMAPPED_READ, 1);
    CHECK_EQ_U64(SCHWAY_FAULT_UNMAPPED_WRITE, 2);
    CHECK_EQ_U64(SCHWAY_FAULT_UNMAPPED_FETCH, 3);
    CHECK_EQ_U64(SCHWAY_FAULT_MISALIGNED_READ, 4);
    CHECK_EQ_U64(SCHWAY_FAULT_MISALIGNED_WRITE, 5);
    CHECK_EQ_U64(SCHWAY_FAULT_MISALIGNED_FETCH, 6);
    CHECK_EQ_U64(SCHWAY_FAULT_RESERVED_INSTRUCTION, 7);
    CHECK_EQ_U64(SCHWAY_FAULT_COP_UNUSABLE, 8);
    CHECK_EQ_U64(SCHWAY_FAULT_EXCEPTION_LOOP, 9);
    CHECK_EQ_U64(SCHWAY_DEVICE_NONE, 0);
    CHECK_EQ_U64(SCHWAY_DEVICE_RAM, 1);
    CHECK_EQ_U64(SCHWAY_DEVICE_SCRATCH, 2);
    CHECK_EQ_U64(SCHWAY_DEVICE_BIOS, 3);
    CHECK_EQ_U64(SCHWAY_DEVICE_MEMCTL, 4);
    CHECK_EQ_U64(SCHWAY_DEVICE_IRQ, 5);
    CHECK_EQ_U64(SCHWAY_DEVICE_DMA, 6);
    CHECK_EQ_U64(SCHWAY_DEVICE_TIMER, 7);
    CHECK_EQ_U64(SCHWAY_DEVICE_CDROM, 8);
    CHECK_EQ_U64(SCHWAY_DEVICE_GPU, 9);
    CHECK_EQ_U64(SCHWAY_DEVICE_MDEC, 10);
    CHECK_EQ_U64(SCHWAY_DEVICE_SIO, 11);
    CHECK_EQ_U64(SCHWAY_DEVICE_SPU, 12);
    CHECK_EQ_U64(SCHWAY_DEVICE_EXP1, 13);
    CHECK_EQ_U64(SCHWAY_DEVICE_EXP2, 14);
    CHECK_EQ_U64(SCHWAY_DEVICE_EXP3, 15);
    CHECK_EQ_U64(SCHWAY_DEVICE_GTE, 16);
    CHECK_EQ_U64(SCHWAY_DEVICE_UNMAPPED, 17);
    CHECK_EQ_U64(SCHWAY_STATUS_ERR_EXE_INVALID, 11);
    CHECK_EQ_U64(SCHWAY_STATUS_ERR_EXE_RANGE, 12);
    CHECK_EQ_U64(SCHWAY_PROFILE_DEBUG_PORT, 1);
    CHECK_EQ_U64(SCHWAY_LICENCE_NONE, 0);
    CHECK_EQ_U64(SCHWAY_LICENCE_JAPAN, 1);
    CHECK_EQ_U64(SCHWAY_LICENCE_EUROPE, 2);
    CHECK_EQ_U64(SCHWAY_LICENCE_NORTH_AMERICA, 3);
    CHECK_EQ_U64(SCHWAY_EXE_LINT_LOW_64K, 1);
    CHECK_EQ_U64(SCHWAY_EXE_LINT_TRAILING, 2);
    for (schway_status st = SCHWAY_STATUS_ERR_EXE_INVALID; st <= SCHWAY_STATUS_ERR_EXE_RANGE;
         st++) {
        const char *text = schway_status_string(st);
        CHECK(text != NULL && text[0] != '\0');
        CHECK(strcmp(text, schway_status_string(SCHWAY_STATUS_ERR_EXE_RANGE + 1u)) != 0);
    }

    /* A PS-EXE header and one text sector: entry 80010000h, global pointer
       80020000h, text at 80010000h, stack at 801FFF00h. */
    static unsigned char exe[0x1000];
    memset(exe, 0, sizeof exe);
    memcpy(exe, "PS-X EXE", 8);
    put32(exe + 0x10, 0x80010000u);
    put32(exe + 0x14, 0x80020000u);
    put32(exe + 0x18, 0x80010000u);
    put32(exe + 0x1C, 0x800u);
    put32(exe + 0x20, 0x11u);
    put32(exe + 0x24, 0x22u);
    put32(exe + 0x28, 0x33u);
    put32(exe + 0x2C, 0x44u);
    put32(exe + 0x30, 0x801FFF00u);
    put32(exe + 0x34, 0u);
    memcpy(exe + 0x4C, "Sony Computer Entertainment Inc. for Europe area", 48);

    schway_exe_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    info.pc0 = 0u;
    info.gp0 = 0u;
    info.t_addr = 0u;
    info.t_size = 0u;
    info.d_addr = 0u;
    info.d_size = 0u;
    info.b_addr = 0u;
    info.b_size = 0u;
    info.s_addr = 0u;
    info.s_size = 0u;
    info.licence_area = 0u;
    info.lint = 0u;
    info.hash_len = 0u;
    CHECK_EQ_U64(schway_probe_exe(exe, sizeof exe, &info), SCHWAY_STATUS_OK);
    CHECK_EQ_HEX(info.pc0, 0x80010000u);
    CHECK_EQ_HEX(info.gp0, 0x80020000u);
    CHECK_EQ_HEX(info.t_addr, 0x80010000u);
    CHECK_EQ_HEX(info.t_size, 0x800u);
    CHECK_EQ_HEX(info.d_addr, 0x11u);
    CHECK_EQ_HEX(info.d_size, 0x22u);
    CHECK_EQ_HEX(info.b_addr, 0x33u);
    CHECK_EQ_HEX(info.b_size, 0x44u);
    CHECK_EQ_HEX(info.s_addr, 0x801FFF00u);
    CHECK_EQ_HEX(info.s_size, 0u);
    CHECK_EQ_U64(info.licence_area, SCHWAY_LICENCE_EUROPE);
    CHECK_EQ_U64(info.lint, 0);
    CHECK_EQ_HEX(info.hash_len, 0x1000u);

    CHECK_EQ_U64(schway_probe_exe(NULL, sizeof exe, &info), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_probe_exe(exe, 0u, &info), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_probe_exe(exe, sizeof exe, NULL), SCHWAY_STATUS_ERR_ARGUMENT);

    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = v2_alloc;
    cfg.allocator.free = v2_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    CHECK(inst != NULL);
    if (inst == NULL) {
        CHECK_DONE();
    }
    schway_profile profile;
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = SCHWAY_REGION_NTSC_U;
    profile.flags = SCHWAY_PROFILE_DEBUG_PORT;
    CHECK_EQ_U64(schway_set_profile(inst, &profile), SCHWAY_STATUS_OK);
    profile.flags = 2u;
    CHECK_EQ_U64(schway_set_profile(inst, &profile), SCHWAY_STATUS_ERR_PROFILE_REJECTED);

    CHECK_EQ_U64(schway_load_exe(inst, exe, sizeof exe), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_load_exe(NULL, exe, sizeof exe), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_load_exe(inst, exe, 0u), SCHWAY_STATUS_ERR_ARGUMENT);
    exe[0] = 'X';
    CHECK_EQ_U64(schway_load_exe(inst, exe, sizeof exe), SCHWAY_STATUS_ERR_EXE_INVALID);
    CHECK_EQ_U64(schway_probe_exe(exe, sizeof exe, &info), SCHWAY_STATUS_ERR_EXE_INVALID);
    exe[0] = 'P';
    put32(exe + 0x18, 0xC0000000u);
    CHECK_EQ_U64(schway_load_exe(inst, exe, sizeof exe), SCHWAY_STATUS_ERR_EXE_RANGE);
    CHECK_EQ_U64(schway_run_frame(inst, NULL), SCHWAY_STATUS_OK);
    put32(exe + 0x18, 0x80010000u);
    CHECK_EQ_U64(schway_load_exe(inst, exe, sizeof exe), SCHWAY_STATUS_ERR_ARGUMENT);

    /* A budgeted run stops at its cycle limit, with the two result fields
       added after the first release. The program is running, so it stops at
       the first instruction boundary at or after the limit. */
    schway_run_result res;
    memset(&res, 0, sizeof res);
    res.size = (uint32_t)sizeof res;
    CHECK(sizeof res >= 32u);
    CHECK_EQ_U64(schway_run_frame(inst, &res), SCHWAY_STATUS_OK);
    uint64_t ticks = res.ticks;
    schway_budget budget;
    memset(&budget, 0, sizeof budget);
    budget.size = (uint32_t)sizeof budget;
    budget.flags = SCHWAY_RUN_STOP_ON_FAULT;
    budget.max_cycles = 1000u;
    CHECK_EQ_U64(schway_run(inst, &budget, &res), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(res.stop, SCHWAY_STOP_BUDGET);
    CHECK(res.ticks >= ticks + 1000u && res.ticks < ticks + 1000u + 64u);
    CHECK_EQ_U64(res.frame, 2u);
    CHECK_EQ_U64(res.exit_code, 0u);
    CHECK_EQ_U64(res.fault_present, 0u);
    CHECK_EQ_U64(schway_run(inst, &budget, NULL), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_run(NULL, &budget, &res), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_run(inst, NULL, &res), SCHWAY_STATUS_ERR_ARGUMENT);
    budget.flags = 4u;
    CHECK_EQ_U64(schway_run(inst, &budget, &res), SCHWAY_STATUS_ERR_ARGUMENT);
    budget.flags = 0u;
    budget.size = 8u;
    CHECK_EQ_U64(schway_run(inst, &budget, &res), SCHWAY_STATUS_ERR_STRUCT_SIZE);

    /* Peek reads RAM and the scratchpad without touching them, and the fault
       record is empty until something latches one. */
    uint32_t value = 0xFFFFFFFFu;
    CHECK_EQ_U64(schway_peek(inst, SCHWAY_SPACE_RAM, 0x10000u, 4u, &value), SCHWAY_STATUS_OK);
    CHECK_EQ_HEX(value, 0u);
    CHECK_EQ_U64(schway_peek(inst, SCHWAY_SPACE_SCRATCHPAD, 0x3FCu, 4u, &value),
                 SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_peek(inst, 3u, 0u, 1u, &value), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_peek(inst, SCHWAY_SPACE_RAM, 1u, 2u, &value), SCHWAY_STATUS_ERR_ARGUMENT);
    schway_fault fault;
    memset(&fault, 0xA5, sizeof fault);
    fault.size = (uint32_t)sizeof fault;
    fault.kind = 1u;
    fault.device = 1u;
    fault.width = 1u;
    fault.frame = 1u;
    fault.ticks = 1u;
    fault.line = 1u;
    fault.dot = 1u;
    fault.pc = 1u;
    fault.opcode = 1u;
    fault.addr = 1u;
    fault.note = 1u;
    CHECK_EQ_U64(schway_get_fault(inst, &fault), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(fault.kind, SCHWAY_FAULT_NONE);
    CHECK_EQ_U64(fault.device | fault.width | fault.frame | fault.ticks, 0u);
    CHECK_EQ_U64(fault.line | fault.dot | fault.pc | fault.opcode | fault.addr | fault.note, 0u);
    CHECK_EQ_U64(schway_get_fault(NULL, &fault), SCHWAY_STATUS_ERR_ARGUMENT);

    /* The TTY drain assigns every field and takes the bytes the guest wrote
       to the debug port; none was written here, so it reports nothing. */
    uint8_t tty_buf[16];
    schway_tty tty;
    memset(&tty, 0xA5, sizeof tty);
    tty.size = (uint32_t)sizeof tty;
    tty.reserved0 = 0u;
    tty.bytes = tty_buf;
    tty.capacity = (uint32_t)sizeof tty_buf;
    tty.written = 1u;
    tty.dropped = 1u;
    CHECK_EQ_U64(schway_get_tty(inst, &tty), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(tty.written, 0u);
    CHECK_EQ_U64(tty.dropped, 0u);
    tty.bytes = NULL;
    CHECK_EQ_U64(schway_get_tty(inst, &tty), SCHWAY_STATUS_ERR_ARGUMENT);
    tty.capacity = 0u;
    CHECK_EQ_U64(schway_get_tty(inst, &tty), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_get_tty(NULL, &tty), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_get_tty(inst, NULL), SCHWAY_STATUS_ERR_ARGUMENT);
    tty.size = 8u;
    CHECK_EQ_U64(schway_get_tty(inst, &tty), SCHWAY_STATUS_ERR_STRUCT_SIZE);

    /* A traced run leaves records to read from the oldest. No record is held
       before one, the program is running, and an index past the end is refused. */
    schway_trace trace;
    memset(&trace, 0xA5, sizeof trace);
    trace.size = (uint32_t)sizeof trace;
    CHECK(sizeof trace == 64u);
    CHECK_EQ_U64(schway_get_trace(inst, 0u, &trace), SCHWAY_STATUS_ERR_ARGUMENT);
    budget.size = (uint32_t)sizeof budget;
    budget.flags = SCHWAY_RUN_TRACE;
    budget.max_cycles = 2000u;
    CHECK_EQ_U64(schway_run(inst, &budget, &res), SCHWAY_STATUS_OK);
    trace.event = 0u;
    trace.frame = 1u;
    trace.cycle = 1u;
    trace.line = 1u;
    trace.dot = 1u;
    trace.pc = 1u;
    trace.opcode = 1u;
    trace.addr = 1u;
    trace.width = 1u;
    trace.device = 1u;
    trace.value = 1u;
    trace.note = 1u;
    trace.count = 0u;
    CHECK_EQ_U64(schway_get_trace(inst, 0u, &trace), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(trace.size, sizeof trace);
    CHECK_EQ_U64(trace.event, SCHWAY_TRACE_EXEC);
    CHECK_EQ_U64(trace.count > 0u, 1u);
    CHECK_EQ_U64(trace.width | trace.device | trace.note | trace.addr | trace.value, 0u);
    CHECK(trace.frame <= res.frame && trace.cycle > 0u && trace.pc != 0u);
    CHECK(trace.line < 400u && trace.dot < 4000u);
    CHECK_EQ_U64(schway_get_trace(inst, trace.count, &trace), SCHWAY_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(schway_get_trace(NULL, 0u, &trace), SCHWAY_STATUS_ERR_ARGUMENT);
    schway_destroy(inst);
    CHECK_DONE();
}
