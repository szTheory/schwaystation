/* The runner's trace record formatter: one golden line per event, the names of every device and fault
   kind, and the refusals. A change to the line layout is a change to docs/runner.md. */
#include <stdint.h>
#include <string.h>

#include "../../runner/trace.h"
#include "../check.h"

/* Formats r and compares the line with want, byte for byte. */
static void golden(const schway_trace *r, const char *want)
{
    char buf[SCHWAY__TRACE_LINE_MAX];
    int n = schway__trace_write_record(buf, sizeof buf, r);
    CHECK_EQ_U64(n, strlen(want));
    if (n >= 0 && strcmp(buf, want) != 0) {
        fprintf(stderr, "%s:%d: the line is\n  %s\nand the golden line is\n  %s\n", __FILE__, __LINE__,
                buf, want);
        check_failures++;
    }
}

static schway_trace record(uint32_t event)
{
    schway_trace r;
    memset(&r, 0, sizeof r);
    r.size = (uint32_t)sizeof r;
    r.event = event;
    r.frame = 3u;
    r.cycle = 1693851u;
    r.line = 12u;
    r.dot = 345u;
    r.pc = 0x80010100u;
    r.opcode = 0x8D090000u;
    r.count = 4096u;
    return r;
}

int main(void)
{
    schway_trace r = record(SCHWAY_TRACE_EXEC);
    golden(&r, "3\t12\t345\t1693851\t0x80010100\t0x8d090000\texec\t0x00000000\t0\tnone\t0x00000000\t0");

    r = record(SCHWAY_TRACE_RD);
    r.addr = 0x80020004u;
    r.width = 4u;
    r.device = SCHWAY_DEVICE_RAM;
    r.value = 0xDEADBEEFu;
    golden(&r, "3\t12\t345\t1693851\t0x80010100\t0x8d090000\trd\t0x80020004\t4\tram\t0xdeadbeef\t0");

    r = record(SCHWAY_TRACE_WR);
    r.addr = 0xBF801810u;
    r.width = 2u;
    r.device = SCHWAY_DEVICE_GPU;
    r.value = 0x0000ABCDu;
    golden(&r, "3\t12\t345\t1693851\t0x80010100\t0x8d090000\twr\t0xbf801810\t2\tgpu\t0x0000abcd\t0");

    r = record(SCHWAY_TRACE_IRQ);
    r.value = 0x00000400u;
    golden(&r, "3\t12\t345\t1693851\t0x80010100\t0x8d090000\tirq\t0x00000000\t0\tnone\t0x00000400\t0");

    /* A fault record: value is the kind, an exception loop's count is in note, and the largest frame
       and cycle values still fit the line. */
    r = record(SCHWAY_TRACE_FAULT);
    r.frame = UINT64_MAX;
    r.cycle = UINT64_MAX;
    r.line = UINT32_MAX;
    r.dot = UINT32_MAX;
    r.pc = UINT32_MAX;
    r.opcode = UINT32_MAX;
    r.addr = UINT32_MAX;
    r.width = UINT32_MAX;
    r.device = SCHWAY_DEVICE_UNMAPPED;
    r.value = SCHWAY_FAULT_EXCEPTION_LOOP;
    r.note = 1000u;
    golden(&r,
           "18446744073709551615\t4294967295\t4294967295\t18446744073709551615\t0xffffffff\t0xffffffff\t"
           "fault\t0xffffffff\t4294967295\tunmapped\t0x00000009\t1000");
    r.device = SCHWAY_DEVICE_NONE;
    r.frame = 0u;
    r.cycle = 0u;
    r.line = r.dot = r.pc = r.opcode = r.addr = r.width = r.note = 0u;
    r.value = SCHWAY_FAULT_MISALIGNED_READ;
    golden(&r, "0\t0\t0\t0\t0x00000000\t0x00000000\tfault\t0x00000000\t0\tnone\t0x00000004\t0");

    /* Every device and every fault kind has its name; a value outside the enum is "unknown". */
    static const char *const devices[] = {"none", "ram", "scratch", "bios", "memctl", "irq", "dma",
                                          "timer", "cdrom", "gpu", "mdec", "sio", "spu", "exp1",
                                          "exp2", "exp3", "gte", "unmapped"};
    for (uint32_t i = 0; i < sizeof devices / sizeof devices[0]; i++) {
        CHECK(strcmp(schway__trace_device_name(i), devices[i]) == 0);
    }
    CHECK(strcmp(schway__trace_device_name(SCHWAY_DEVICE_UNMAPPED + 1u), "unknown") == 0);
    static const char *const kinds[] = {"none", "unmapped_read", "unmapped_write", "unmapped_fetch",
                                        "misaligned_read", "misaligned_write", "misaligned_fetch",
                                        "reserved_instruction", "cop_unusable", "exception_loop", "dma"};
    for (uint32_t i = 0; i < sizeof kinds / sizeof kinds[0]; i++) {
        CHECK(strcmp(schway__trace_kind_name(i), kinds[i]) == 0);
    }
    CHECK(strcmp(schway__trace_kind_name(SCHWAY_FAULT_DMA + 1u), "unknown") == 0);
    CHECK(strcmp(schway__trace_event_name(0u), "unknown") == 0);

    /* A buffer one byte too small, a NULL buffer and a NULL record are refused, never overrun. */
    char small[8];
    r = record(SCHWAY_TRACE_EXEC);
    CHECK_EQ_U64(schway__trace_write_record(small, sizeof small, &r) < 0, 1u);
    CHECK_EQ_U64(schway__trace_write_record(NULL, 100u, &r) < 0, 1u);
    CHECK_EQ_U64(schway__trace_write_record(small, sizeof small, NULL) < 0, 1u);
    CHECK_DONE();
}
