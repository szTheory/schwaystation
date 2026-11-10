/* The runner's trace and fault text: the one formatter for a trace record and the names the fault line
   uses (docs/runner.md describes the lines). Runner-only: the core hands over numbers. */
#ifndef SCHWAYSTATION_RUNNER_TRACE_H
#define SCHWAYSTATION_RUNNER_TRACE_H

#include <stddef.h>
#include <stdint.h>

#include "schwaystation.h"

/* The longest line schway__trace_write_record can write, its NUL included. */
#define SCHWAY__TRACE_LINE_MAX 160u

/* Writes record r to buf as twelve tab-separated fields, with no line feed and a NUL at the end:
     frame line dot cycle pc opcode event addr width device value note
   frame, line, dot, cycle, width and note are decimal; pc, opcode, addr and value are "0x" and eight
   lowercase hex digits. Returns the length written, or -1 when buf (cap bytes) is too small or r is
   NULL. */
int schway__trace_write_record(char *buf, size_t cap, const schway_trace *r);

/* Names of the SCHWAY_TRACE_*, SCHWAY_DEVICE_* and SCHWAY_FAULT_* values: "exec", "rd", "wr", "irq" and
   "fault"; "none", "ram", "scratch", ... "unmapped"; "unmapped_read", ... "exception_loop". A value
   outside the enum is "unknown". Never NULL. */
const char *schway__trace_event_name(uint32_t event);
const char *schway__trace_device_name(uint32_t device);
const char *schway__trace_kind_name(uint32_t kind);

#endif /* SCHWAYSTATION_RUNNER_TRACE_H */
