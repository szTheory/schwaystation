/* The stub kernel and the PS-EXE boot path in the core: schwaystation-test-boot
   <case> [dir], dir holding the assembled fixtures. White-box: the test sees
   src/, so it can read the committed image array, RAM and the fault record. */
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

/* One little-endian word. */
static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

/* The bytes of dir/name; the caller frees them. A file that cannot be read
   is a failure and ends the test. */
static uint8_t *read_file(const char *dir, const char *name, size_t *n)
{
    char path[1024];
    int len = snprintf(path, sizeof path, "%s/%s", dir, name);
    CHECK(len > 0 && (size_t)len < sizeof path);
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "%s: cannot open\n", path);
        exit(1);
    }
    size_t cap = 1u << 16;
    uint8_t *buf = (uint8_t *)malloc(cap);
    if (buf == NULL) {
        exit(1);
    }
    *n = fread(buf, 1u, cap, f);
    CHECK(*n > 0u && *n < cap);
    fclose(f);
    return buf;
}

/* The mailbox: the 256 bytes at RAM 10000h, which a fixture places at its
   load address 80010000h. */
#define MB_STATUS 8u
#define MB_COUNT 10u
#define MB_RECORDS 16u
static const uint8_t *mailbox(const schway *inst)
{
    return inst->bus.ram + 0x10000u;
}

/* The mailbox is sealed: the magic, and the CRC-32 of bytes 10h to FFh equal
   to the word at 04h. The core's CRC-32 is not the fixture's bitwise routine,
   so the two agree only if both are right. */
static void check_sealed(const schway *inst, uint32_t count)
{
    const uint8_t *mb = mailbox(inst);
    CHECK(memcmp(mb, "SWMB", 4u) == 0);
    CHECK_EQ_HEX(mb[MB_STATUS], 0u);
    CHECK_EQ_U64(mb[9], 1u);
    CHECK_EQ_U64(mb[MB_COUNT], count);
    CHECK_EQ_HEX(le32(mb + 4u), schway__crc32_update(0u, mb + MB_RECORDS, 0x100u - MB_RECORDS));
}

/* The value of record n, 1 first, after its id and valid flag are checked. */
static uint32_t record(const schway *inst, uint32_t n)
{
    const uint8_t *r = mailbox(inst) + MB_RECORDS + 8u * (n - 1u);
    CHECK_EQ_U64((uint32_t)r[0] | ((uint32_t)r[1] << 8), n);
    CHECK_EQ_U64((uint32_t)r[2] | ((uint32_t)r[3] << 8), 1u);
    return le32(r + 4u);
}

static schway_run_result run_frame(schway *inst)
{
    schway_run_result r;
    memset(&r, 0, sizeof r);
    r.size = (uint32_t)sizeof r;
    CHECK_EQ_U64(schway_run_frame(inst, &r), SCHWAY_STATUS_OK);
    return r;
}

static schway_run_result run(schway *inst, uint64_t max_cycles, uint32_t flags)
{
    schway_budget b;
    memset(&b, 0, sizeof b);
    b.size = (uint32_t)sizeof b;
    b.flags = flags;
    b.max_cycles = max_cycles;
    schway_run_result r;
    memset(&r, 0, sizeof r);
    r.size = (uint32_t)sizeof r;
    CHECK_EQ_U64(schway_run(inst, &b, &r), SCHWAY_STATUS_OK);
    return r;
}

static void load(schway *inst, const uint8_t *exe, size_t n)
{
    CHECK_EQ_U64(schway_load_exe(inst, exe, n), SCHWAY_STATUS_OK);
}

/* The instance with the profile's debug-port flag set, so the exit port acts. */
static schway *make_debug(void)
{
    schway *inst = make();
    schway_profile p;
    memset(&p, 0, sizeof p);
    p.size = (uint32_t)sizeof p;
    p.region = SCHWAY_REGION_NTSC_U;
    p.flags = SCHWAY_PROFILE_DEBUG_PORT;
    CHECK_EQ_U64(schway_set_profile(inst, &p), SCHWAY_STATUS_OK);
    return inst;
}

/* An EXE built in memory: a 2048-byte header and one sector of text at
   80010000h, entry point 80010100h, with the code from there. A non-zero
   handler address makes the sector start with the mailbox magic and that
   handler word, which is how the stub finds an image's handler. Every word of
   the code is written at its call, beside its mnemonic. */
#define EXE_SIZE 4096u
#define CODE_AT 0x100u
static void build_exe(uint8_t *exe, const uint32_t *code, size_t words, uint32_t handler)
{
    memset(exe, 0, EXE_SIZE);
    memcpy(exe, "PS-X EXE", 8u);
    put32(exe + 0x10, 0x80010100u); /* pc0 */
    put32(exe + 0x18, 0x80010000u); /* t_addr */
    put32(exe + 0x1C, 0x800u);      /* t_size */
    if (handler != 0u) {
        memcpy(exe + 0x800, "SWMB", 4u);
        put32(exe + 0x800 + 0x0C, handler);
    }
    for (size_t i = 0; i < words; i++) {
        put32(exe + 0x800 + CODE_AT + 4u * i, code[i]);
    }
}

/* hello.exe runs from reset through the stub to its own end: the mailbox is
   sealed with status 00h, and its two records are what the stub set up, gp
   from the header's gp0 and sp from its s_addr + s_size. Both come from the
   file's own header here, so a stub that ignores pc0, gp0 or s_addr fails.
   Fault prevented: an image that never starts, or starts with the wrong
   global pointer or stack. */
static void hello_boot(const char *dir)
{
    size_t n = 0u;
    uint8_t *exe = read_file(dir, "hello.exe", &n);
    schway *inst = make();
    load(inst, exe, n);
    for (int f = 0; f < 3 && mailbox(inst)[MB_STATUS] != 0u; f++) {
        schway_run_result r = run_frame(inst);
        CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    }
    check_sealed(inst, 2u);
    CHECK_EQ_HEX(record(inst, 1u), le32(exe + 0x14));
    CHECK_EQ_HEX(record(inst, 2u), le32(exe + 0x30) + le32(exe + 0x34));
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    schway_destroy(inst);
    free(exe);
}

/* Drains the TTY into buf (capacity cap) and returns the bytes written;
   *dropped is what the drain reported lost. */
static uint32_t drain_tty(schway *inst, uint8_t *buf, uint32_t cap, uint32_t *dropped)
{
    schway_tty t;
    memset(&t, 0, sizeof t);
    t.size = (uint32_t)sizeof t;
    t.bytes = buf;
    t.capacity = cap;
    CHECK_EQ_U64(schway_get_tty(inst, &t), SCHWAY_STATUS_OK);
    if (dropped != NULL) {
        *dropped = t.dropped;
    }
    return t.written;
}

/* hello.exe with the debug-port flag set prints a line through the port at
   1F802080h and then a line through the kernel's putchar, A(3Ch): the stub's
   own routine stores the byte to the port. The host drains both lines in that
   order, and a second drain finds nothing.
   Fault prevented: a TTY byte that is lost, reordered or reaches the ring by
   only one of its two sources. */
static void tty_hello(const char *dir)
{
    size_t n = 0u;
    uint8_t *exe = read_file(dir, "hello.exe", &n);
    schway *inst = make_debug();
    load(inst, exe, n);
    for (int f = 0; f < 3 && mailbox(inst)[MB_STATUS] != 0u; f++) {
        schway_run_result r = run_frame(inst);
        CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    }
    check_sealed(inst, 2u);
    static const char want[] = "hello port\nhello putchar\n";
    uint8_t got[64];
    uint32_t dropped = 99u;
    uint32_t w = drain_tty(inst, got, sizeof got, &dropped);
    CHECK_EQ_U64(w, sizeof want - 1u);
    CHECK(w == sizeof want - 1u && memcmp(got, want, w) == 0);
    CHECK_EQ_U64(dropped, 0u);
    CHECK_EQ_U64(drain_tty(inst, got, sizeof got, &dropped), 0u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    schway_destroy(inst);
    free(exe);
}

/* 5,000 bytes written to the port in one frame fill the 4 KiB ring: the first
   4,096 stay in the order written, the other 904 are dropped, and the first
   drain reports 904 whatever its capacity. A partial drain keeps the order of
   the rest and reports nothing more, and a drain of an empty ring reports 0.
   The byte written is the low byte of a counter that counts down from 5,000,
   so a reordered or repeated byte differs from its place.
   Fault prevented: a ring that overwrites old bytes, loses the drop count, or
   reports it again after a drain. */
static void tty_ring(void)
{
    uint8_t exe[EXE_SIZE];
    const uint32_t code[] = {
        0x3C08BF80u, /* 0x100: lui t0, 0xBF80 */
        0x34091388u, /* 0x104: ori t1, zero, 5000 */
        0xA1092080u, /* 0x108: sb t1, 0x2080(t0): 1F802080h in KSEG1 */
        0x2529FFFFu, /* 0x10C: addiu t1, t1, -1 */
        0x1520FFFDu, /* 0x110: bne t1, zero, 80010108h */
        0x00000000u, /* 0x114: nop */
        0x08004046u, /* 0x118: j 80010118h: itself */
        0x00000000u, /* 0x11C: nop */
    };
    build_exe(exe, code, sizeof code / sizeof code[0], 0u);
    schway *inst = make_debug();
    load(inst, exe, sizeof exe);
    schway_run_result r = run_frame(inst);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    static uint8_t got[SCHWAY__TTY_SIZE];
    uint32_t dropped = 0u;
    uint32_t first = drain_tty(inst, got, 1000u, &dropped);
    CHECK_EQ_U64(first, 1000u);
    CHECK_EQ_U64(dropped, 904u);
    uint32_t rest = drain_tty(inst, got + first, sizeof got - first, &dropped);
    CHECK_EQ_U64(rest, SCHWAY__TTY_SIZE - 1000u);
    CHECK_EQ_U64(dropped, 0u);
    for (uint32_t i = 0u; i < SCHWAY__TTY_SIZE; i++) {
        if (got[i] != (uint8_t)(5000u - i)) {
            fprintf(stderr, "tty byte %u is %02x, expected %02x\n", (unsigned)i, (unsigned)got[i],
                    (unsigned)(uint8_t)(5000u - i));
            check_failures++;
            break;
        }
    }
    CHECK_EQ_U64(drain_tty(inst, got, sizeof got, &dropped), 0u);
    CHECK_EQ_U64(dropped, 0u);
    schway_destroy(inst);
}

/* hello.exe on an instance whose debug-port flag is clear, as on a retail
   host: it runs to its end, the ring stays empty, and the two ports' writes are
   not a fault and not a count against any unbuilt device.
   Fault prevented: debug ports decoded on a retail profile. */
static void tty_flag_off(const char *dir)
{
    size_t n = 0u;
    uint8_t *exe = read_file(dir, "hello.exe", &n);
    schway *inst = make();
    load(inst, exe, n);
    for (int f = 0; f < 3 && mailbox(inst)[MB_STATUS] != 0u; f++) {
        schway_run_result r = run_frame(inst);
        CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    }
    check_sealed(inst, 2u);
    uint8_t got[64];
    uint32_t dropped = 99u;
    CHECK_EQ_U64(drain_tty(inst, got, sizeof got, &dropped), 0u);
    CHECK_EQ_U64(dropped, 0u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    for (uint32_t i = 0u; i < SCHWAY__DEV_COUNT; i++) {
        CHECK_EQ_U64(inst->bus.unbuilt_n[i], 0u);
    }
    CHECK_EQ_U64(inst->exit_pending, 0u);
    schway_destroy(inst);
    free(exe);
}

/* The exception record the stub's handler writes at 80000210h: Cause, EPC,
   BadVaddr and SR. */
static void check_exc_record(const schway *inst, uint32_t cause, uint32_t epc)
{
    const uint8_t *rec = inst->bus.ram + 0x210u;
    CHECK_EQ_HEX(le32(rec), cause);
    CHECK_EQ_HEX(le32(rec + 4u), epc);
    CHECK_EQ_HEX(le32(rec + 8u), 0u);
    CHECK_EQ_HEX(le32(rec + 12u), 0u);
}

/* BREAK with no handler registered: the stub's own handler records Cause (Bp,
   09h in bits 2-6), the BREAK's address, BadVaddr and SR, sets the mailbox
   word at 08h to FFh and stops in its loop. BREAK is never a first fault.
   Fault prevented: a default handler that records the wrong registers or the
   wrong address, or does not mark the mailbox. */
static void default_handler(const char *dir)
{
    size_t n = 0u;
    uint8_t *exe = read_file(dir, "exc_default.exe", &n);
    schway *inst = make();
    load(inst, exe, n);
    schway_run_result r = run_frame(inst);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    check_exc_record(inst, 0x24u, 0x80010100u);
    CHECK_EQ_HEX(le32(mailbox(inst) + 8u), 0xFFu);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    schway_destroy(inst);
    free(exe);
}

/* The same BREAK with a handler registered at 0Ch of the mailbox: the stub
   records and marks the mailbox, then jumps there; the handler stores Cause as
   record 1, seals the mailbox and sets status 00h over the stub's FFh.
   Fault prevented: a stub that does not hand the exception to the image's
   handler, or hands it over with the wrong registers. */
static void registered_handler(const char *dir)
{
    size_t n = 0u;
    uint8_t *exe = read_file(dir, "exc_handler.exe", &n);
    schway *inst = make();
    load(inst, exe, n);
    schway_run_result r = run_frame(inst);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    check_exc_record(inst, 0x24u, 0x80010100u);
    check_sealed(inst, 1u);
    CHECK_EQ_HEX(record(inst, 1u), 0x24u);
    schway_destroy(inst);
    free(exe);
}

/* A jump to the first address past the stub image is a bus error on the
   fetch: the first fault is an unmapped fetch of the BIOS at that address, the
   run reports FAULT under STOP_ON_FAULT with the program counter at the
   exception vector, and the exception is IBE at that address, which the
   stub's handler records without BadVaddr. An IBE at an address that is not an
   exception vector is not a double fault.
   Fault prevented: a runaway into the unmapped rest of the BIOS window that
   slides on as no-operations. */
static void past_stub(void)
{
    const uint32_t target = 0xBFC00000u + schway__stub_kernel_size;
    uint8_t exe[EXE_SIZE];
    const uint32_t code[] = {
        0x3C08BFC0u,                     /* lui t0, 0xBFC0 */
        0x35080000u | (target & 0xFFFFu), /* ori t0, t0, size of the image */
        0x01000008u,                     /* jr t0 */
        0x00000000u,                     /* nop */
    };
    build_exe(exe, code, sizeof code / sizeof code[0], 0u);
    schway *inst = make();
    load(inst, exe, sizeof exe);
    schway_run_result r = run(inst, 100000u, SCHWAY_RUN_STOP_ON_FAULT);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FAULT);
    CHECK_EQ_U64(r.fault_present, 1u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_FETCH);
    CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_BIOS);
    CHECK_EQ_HEX(inst->fault.addr, target);
    CHECK_EQ_HEX(inst->cpu.pc, 0x80000080u);
    r = run_frame(inst);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    check_exc_record(inst, 0x18u, target);
    schway_destroy(inst);
}

/* The exit port: with the debug-port flag set a byte written to 1F802082h
   stops the run with HOOK and the byte in exit_code, once; the next run goes
   on to the frame's end. With the flag clear the write is ignored and the run
   is a frame.
   Fault prevented: an exit that is lost, repeated or reported without its code. */
static void exit_port(void)
{
    uint8_t exe[EXE_SIZE];
    const uint32_t code[] = {
        0x3C08BF80u, /* lui t0, 0xBF80 */
        0x3409002Au, /* ori t1, zero, 0x2A */
        0xA1092082u, /* sb t1, 0x2082(t0): 1F802082h in KSEG1 */
        0x08004043u, /* j 8001010Ch: itself */
        0x00000000u, /* nop */
    };
    build_exe(exe, code, sizeof code / sizeof code[0], 0u);
    schway *inst = make_debug();
    load(inst, exe, sizeof exe);
    schway_run_result r = run_frame(inst);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_HOOK);
    CHECK_EQ_U64(r.exit_code, 0x2Au);
    CHECK(r.ticks < inst->frame_end);
    r = run_frame(inst);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    CHECK_EQ_U64(r.exit_code, 0u);
    CHECK_EQ_U64(r.frame, 1u);
    schway_destroy(inst);

    inst = make();
    load(inst, exe, sizeof exe);
    r = run_frame(inst);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    CHECK_EQ_U64(r.frame, 1u);
    schway_destroy(inst);
}

/* A reserved instruction is a first fault: with STOP_ON_FAULT the run stops at
   the end of that instruction, the program counter already at the vector, and
   with the flag clear the same program runs to the frame's end.
   Fault prevented: a stop that comes late, after the handler has run, or never. */
static void stop_on_fault(void)
{
    uint8_t exe[EXE_SIZE];
    const uint32_t code[] = {0xFFFFFFFFu /* opcode 3Fh: reserved */};
    build_exe(exe, code, 1u, 0u);
    schway *inst = make();
    load(inst, exe, sizeof exe);
    schway_run_result r = run(inst, 100000u, SCHWAY_RUN_STOP_ON_FAULT);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FAULT);
    CHECK_EQ_U64(r.fault_present, 1u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_RESERVED_INSTRUCTION);
    CHECK_EQ_HEX(inst->fault.pc, 0x80010100u);
    CHECK_EQ_HEX(inst->cpu.pc, 0x80000080u);
    CHECK(r.ticks < 100000u);
    /* A fault already latched stops the next run before any cycle runs. */
    uint64_t ticks = r.ticks;
    r = run(inst, 100000u, SCHWAY_RUN_STOP_ON_FAULT);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FAULT);
    CHECK_EQ_U64(r.ticks, ticks);
    schway_destroy(inst);

    inst = make();
    load(inst, exe, sizeof exe);
    r = run_frame(inst);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    CHECK_EQ_U64(r.fault_present, 1u);
    schway_destroy(inst);
}

/* An instruction fetch that fails at an exception vector is a double fault: the
   run stops with FAULT at once, whatever the flags. The test shrinks the BIOS
   window to end below the vector once the image is running, sets SR.BEV so
   BREAK goes to BFC00180h, and runs without STOP_ON_FAULT.
   Fault prevented: a guest that loses its exception vector spinning for ever
   in the exception path, or a double fault the host is never told about. */
static void double_fault(void)
{
    uint8_t exe[EXE_SIZE];
    const uint32_t code[] = {
        0x40086000u, /* mfc0 t0, sr */
        0x3C090040u, /* lui t1, 0x0040: the load delay slot */
        0x01094025u, /* or t0, t0, t1: BEV */
        0x40886000u, /* mtc0 t0, sr */
        0x00000000u, /* nop */
        0x0000000Du, /* break */
    };
    build_exe(exe, code, sizeof code / sizeof code[0], 0u);
    schway *inst = make();
    load(inst, exe, sizeof exe);
    for (int i = 0; i < 4000 && (inst->cpu.pc >> 16) != 0x8001u; i++) {
        (void)run(inst, 1u, 0u); /* one instruction at a time, to stop at the image's entry */
    }
    CHECK_EQ_HEX(inst->cpu.pc >> 16, 0x8001u);
    inst->bus.bios_size = 0x180u;
    schway_run_result r = run_frame(inst);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FAULT);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_FETCH);
    CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_BIOS);
    CHECK_EQ_HEX(inst->fault.addr, 0xBFC00180u);
    CHECK(r.ticks < inst->frame_end);
    schway_destroy(inst);
}

/* A hostile image returns from every call with a stop reason: a jump to
   itself, and a load from an unmapped address in a loop whose handler returns
   to it, so the CPU takes a bus error on every pass. Each runs a whole frame,
   and then one more, in bounded time.
   Fault prevented: a run that never returns for a looping guest. */
static void bounded(void)
{
    uint8_t exe[EXE_SIZE];
    const uint32_t spin[] = {
        0x08004040u, /* j 80010100h */
        0x00000000u, /* nop */
    };
    build_exe(exe, spin, sizeof spin / sizeof spin[0], 0u);
    schway *inst = make();
    load(inst, exe, sizeof exe);
    for (int i = 0; i < 2; i++) {
        schway_run_result r = run_frame(inst);
        CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
        CHECK_EQ_U64(r.frame, (unsigned)i + 1u);
    }
    schway_destroy(inst);

    /* The handler is at 80010110h, registered in the mailbox word the sector starts with. */
    const uint32_t hammer[] = {
        0x3C081F80u, /* 0x100: lui t0, 0x1F80 */
        0x8D090400u, /* 0x104: lw t1, 0x400(t0): a gap, so a bus error */
        0x08004041u, /* 0x108: j 80010104h */
        0x00000000u, /* 0x10C: nop */
        0x401A7000u, /* 0x110: mfc0 k0, epc */
        0x00000000u, /* 0x114: nop */
        0x275A0004u, /* 0x118: addiu k0, k0, 4 */
        0x03400008u, /* 0x11C: jr k0 */
        0x42000010u, /* 0x120: rfe */
    };
    build_exe(exe, hammer, sizeof hammer / sizeof hammer[0], 0x80010110u);
    inst = make();
    load(inst, exe, sizeof exe);
    for (int i = 0; i < 2; i++) {
        schway_run_result r = run_frame(inst);
        CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
        CHECK_EQ_U64(r.frame, (unsigned)i + 1u);
    }
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_READ);
    schway_destroy(inst);
}

/* The state, the frame, the ticks and the drained audio of an instance. */
struct snapshot {
    uint8_t *state;
    size_t size;
    uint64_t frame, ticks;
    uint32_t pairs;
    int16_t pcm[2 * 1024];
};

static void snap(schway *inst, struct snapshot *s)
{
    s->size = schway_state_size(inst);
    s->state = (uint8_t *)malloc(s->size);
    if (s->state == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(schway_save_state(inst, s->state, s->size), SCHWAY_STATUS_OK);
    s->frame = inst->frame;
    s->ticks = inst->ticks;
    schway_audio a;
    memset(&a, 0, sizeof a);
    a.size = (uint32_t)sizeof a;
    a.samples = s->pcm;
    a.capacity = 1024u;
    memset(s->pcm, 0, sizeof s->pcm);
    CHECK_EQ_U64(schway_get_audio(inst, &a), SCHWAY_STATUS_OK);
    s->pairs = a.written;
}

/* With a program loaded a run stops at an instruction boundary at or after its
   budget, so running T cycles and then M equals running T + M when T is where
   a run of B cycles stopped: the state bytes, the frame, the ticks and the
   audio match. The odd budget B stops at T >= B, and a loop of divides, whose
   instructions take many cycles, makes T > B. A frame end passed inside such an
   instruction completes at its end.
   Fault prevented: a run that carries overshoot in the state, or whose stop
   points differ with how the cycles are split. */
static void split_one(const uint8_t *exe, size_t n, int expect_overshoot)
{
    const uint64_t budget = 100001u;
    schway *a = make();
    load(a, exe, n);
    schway_run_result r = run(a, budget, 0u);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_BUDGET);
    const uint64_t t = r.ticks;
    CHECK(t >= budget);
    CHECK(t - budget < SCHWAY__INSN_CYCLES_MAX);
    if (expect_overshoot) {
        CHECK(t > budget);
    }
    schway_destroy(a);

    schway *split = make();
    schway *whole = make();
    load(split, exe, n);
    load(whole, exe, n);
    r = run(split, t, 0u);
    CHECK_EQ_U64(r.ticks, t);
    r = run(split, budget, 0u);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_BUDGET);
    r = run(whole, t + budget, 0u);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_BUDGET);
    struct snapshot sa, sb;
    snap(split, &sa);
    snap(whole, &sb);
    CHECK_EQ_U64(sa.ticks, sb.ticks);
    CHECK_EQ_U64(sa.frame, sb.frame);
    CHECK(sa.size == sb.size && memcmp(sa.state, sb.state, sa.size) == 0);
    CHECK_EQ_U64(sa.pairs, sb.pairs);
    CHECK(memcmp(sa.pcm, sb.pcm, sizeof sa.pcm) == 0);
    CHECK_EQ_U64(sa.pairs, sa.ticks / 768u);
    free(sa.state);
    free(sb.state);
    schway_destroy(split);
    schway_destroy(whole);
}

static void split_with_exe(const char *dir)
{
    size_t n = 0u;
    uint8_t *hello = read_file(dir, "hello.exe", &n);
    split_one(hello, n, 0);
    free(hello);

    uint8_t exe[EXE_SIZE];
    const uint32_t divides[] = {
        0x34080064u, /* 0x100: ori t0, zero, 100 */
        0x34090007u, /* 0x104: ori t1, zero, 7 */
        0x0109001Au, /* 0x108: div t0, t1: 36 cycles */
        0x00005012u, /* 0x10C: mflo t2: waits for the divide */
        0x08004042u, /* 0x110: j 80010108h */
        0x00000000u, /* 0x114: nop */
    };
    build_exe(exe, divides, sizeof divides / sizeof divides[0], 0u);
    split_one(exe, sizeof exe, 1);

    /* The frame end inside a divide: the frame completes at the instruction's
       end, no later than one instruction past it, and the next frame's end
       does not move. */
    schway *inst = make();
    load(inst, exe, sizeof exe);
    const uint64_t end = inst->frame_end;
    schway_run_result r = run_frame(inst);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    CHECK_EQ_U64(r.frame, 1u);
    CHECK(r.ticks >= end && r.ticks - end < SCHWAY__INSN_CYCLES_MAX);
    CHECK(inst->ticks <= inst->frame_end);
    schway *ref = make();
    (void)run_frame(ref);
    CHECK_EQ_U64(inst->frame_end, ref->frame_end);
    schway_destroy(ref);
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
    } else if (strcmp(c, "stub_drift") == 0 && argc == 3) {
        stub_drift(argv[2]);
    } else if (strcmp(c, "hello") == 0 && argc == 3) {
        hello_boot(argv[2]);
    } else if (strcmp(c, "tty_hello") == 0 && argc == 3) {
        tty_hello(argv[2]);
    } else if (strcmp(c, "tty_ring") == 0) {
        tty_ring();
    } else if (strcmp(c, "tty_flag_off") == 0 && argc == 3) {
        tty_flag_off(argv[2]);
    } else if (strcmp(c, "default_handler") == 0 && argc == 3) {
        default_handler(argv[2]);
    } else if (strcmp(c, "registered_handler") == 0 && argc == 3) {
        registered_handler(argv[2]);
    } else if (strcmp(c, "past_stub") == 0) {
        past_stub();
    } else if (strcmp(c, "exit_port") == 0) {
        exit_port();
    } else if (strcmp(c, "stop_on_fault") == 0) {
        stop_on_fault();
    } else if (strcmp(c, "double_fault") == 0) {
        double_fault();
    } else if (strcmp(c, "bounded") == 0) {
        bounded();
    } else if (strcmp(c, "split_with_exe") == 0 && argc == 3) {
        split_with_exe(argv[2]);
    } else {
        fprintf(stderr, "schwaystation-test-boot: unknown case %s\n", c);
        return 2;
    }
    CHECK_DONE();
}
