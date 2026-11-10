/* The trace record formatter and the names of events, devices and fault kinds. The runner uses it for
   --trace first-fault; a later trace mode formats through the same function. */
#include "trace.h"

#include <stdio.h>

const char *schway__trace_event_name(uint32_t event)
{
    switch (event) {
    case SCHWAY_TRACE_EXEC:
        return "exec";
    case SCHWAY_TRACE_RD:
        return "rd";
    case SCHWAY_TRACE_WR:
        return "wr";
    case SCHWAY_TRACE_IRQ:
        return "irq";
    case SCHWAY_TRACE_FAULT:
        return "fault";
    default:
        return "unknown";
    }
}

const char *schway__trace_device_name(uint32_t device)
{
    switch (device) {
    case SCHWAY_DEVICE_NONE:
        return "none";
    case SCHWAY_DEVICE_RAM:
        return "ram";
    case SCHWAY_DEVICE_SCRATCH:
        return "scratch";
    case SCHWAY_DEVICE_BIOS:
        return "bios";
    case SCHWAY_DEVICE_MEMCTL:
        return "memctl";
    case SCHWAY_DEVICE_IRQ:
        return "irq";
    case SCHWAY_DEVICE_DMA:
        return "dma";
    case SCHWAY_DEVICE_TIMER:
        return "timer";
    case SCHWAY_DEVICE_CDROM:
        return "cdrom";
    case SCHWAY_DEVICE_GPU:
        return "gpu";
    case SCHWAY_DEVICE_MDEC:
        return "mdec";
    case SCHWAY_DEVICE_SIO:
        return "sio";
    case SCHWAY_DEVICE_SPU:
        return "spu";
    case SCHWAY_DEVICE_EXP1:
        return "exp1";
    case SCHWAY_DEVICE_EXP2:
        return "exp2";
    case SCHWAY_DEVICE_EXP3:
        return "exp3";
    case SCHWAY_DEVICE_GTE:
        return "gte";
    case SCHWAY_DEVICE_UNMAPPED:
        return "unmapped";
    default:
        return "unknown";
    }
}

const char *schway__trace_kind_name(uint32_t kind)
{
    switch (kind) {
    case SCHWAY_FAULT_NONE:
        return "none";
    case SCHWAY_FAULT_UNMAPPED_READ:
        return "unmapped_read";
    case SCHWAY_FAULT_UNMAPPED_WRITE:
        return "unmapped_write";
    case SCHWAY_FAULT_UNMAPPED_FETCH:
        return "unmapped_fetch";
    case SCHWAY_FAULT_MISALIGNED_READ:
        return "misaligned_read";
    case SCHWAY_FAULT_MISALIGNED_WRITE:
        return "misaligned_write";
    case SCHWAY_FAULT_MISALIGNED_FETCH:
        return "misaligned_fetch";
    case SCHWAY_FAULT_RESERVED_INSTRUCTION:
        return "reserved_instruction";
    case SCHWAY_FAULT_COP_UNUSABLE:
        return "cop_unusable";
    case SCHWAY_FAULT_EXCEPTION_LOOP:
        return "exception_loop";
    case SCHWAY_FAULT_DMA:
        return "dma";
    default:
        return "unknown";
    }
}

int schway__trace_write_record(char *buf, size_t cap, const schway_trace *r)
{
    if (buf == NULL || r == NULL || cap == 0u) {
        return -1;
    }
    int n = snprintf(buf, cap, "%llu\t%lu\t%lu\t%llu\t0x%08lx\t0x%08lx\t%s\t0x%08lx\t%lu\t%s\t0x%08lx\t%lu",
                     (unsigned long long)r->frame, (unsigned long)r->line, (unsigned long)r->dot,
                     (unsigned long long)r->cycle, (unsigned long)r->pc, (unsigned long)r->opcode,
                     schway__trace_event_name(r->event), (unsigned long)r->addr,
                     (unsigned long)r->width, schway__trace_device_name(r->device),
                     (unsigned long)r->value, (unsigned long)r->note);
    return n < 0 || (size_t)n >= cap ? -1 : n;
}
