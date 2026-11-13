/* The stub kernel and the PS-EXE boot path in the core: schwaystation-test-boot
   <case> [dir], dir holding the assembled fixtures. White-box: the test sees
   src/, so it can read the committed image array, RAM and the fault record. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../test_open.h"

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
    FILE *f = schway__test_open_file(path, "rb");
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
    FILE *f = schway__test_open_file(path, "rb");
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

static uint32_t mips_i(uint32_t op, uint32_t rs, uint32_t rt, int32_t imm)
{
    return (op << 26) | (rs << 21) | (rt << 16) | ((uint32_t)imm & 0xFFFFu);
}

static uint32_t mips_r(uint32_t rs, uint32_t rt, uint32_t rd, uint32_t fn)
{
    return (rs << 21) | (rt << 16) | (rd << 11) | fn;
}

static uint32_t mips_j(uint32_t op, uint32_t target)
{
    return (op << 26) | ((target >> 2) & 0x03FFFFFFu);
}

static void stub_irq_hook(void)
{
#define IRQ_ENTRY 0x80010400u
    enum {
        IRQ_ENTRY_WORD = 0xC0u
    };
    uint8_t exe[EXE_SIZE];
    uint32_t code[256] = {0u};
    size_t n = 0u;

    /* The guest registers its own callback context with B0(19h). */
    code[n++] = mips_i(0x0Fu, 0u, 4u, 0x8001); /* lui a0, 8001h */
    code[n++] = mips_i(0x0Du, 4u, 4u, 0x0300); /* ori a0, a0, 300h */
    code[n++] = mips_i(0x0Fu, 0u, 8u, 0x8000); /* lui t0, 8000h */
    code[n++] = mips_i(0x0Du, 8u, 8u, 0x00B0); /* ori t0, t0, B0h */
    code[n++] = mips_i(0x09u, 0u, 9u, 0x19);   /* addiu t1, zero, 19h */
    code[n++] = mips_r(8u, 0u, 31u, 0x09u);    /* jalr ra, t0 */
    code[n++] = 0u;                            /* nop: jalr delay slot */

    /* Allow only IRQ0 at the controller, then give every caller register a
       distinct value. The later polling loop uses k0 alone. */
    code[n++] = mips_i(0x0Fu, 0u, 8u, 0xBF80); /* lui t0, BF80h */
    code[n++] = mips_i(0x09u, 0u, 9u, 1);      /* addiu t1, zero, 1 */
    code[n++] = mips_i(0x2Bu, 8u, 9u, 0x1074); /* sw t1, 1074h(t0) */
    for (uint32_t reg = 1u; reg < 32u; reg++) {
        if (reg != 26u) {
            code[n++] = mips_i(0x09u, 0u, reg, (int32_t)(0x100u + reg));
            /* addiu rN, zero, 100h+N; k0 is deliberately left available. */
        }
    }
    code[n++] = mips_i(0x0Fu, 0u, 26u, 0x1357); /* lui k0, 1357h */
    code[n++] = mips_i(0x0Du, 26u, 26u, 0x9BDF); /* ori k0, k0, 9BDFh */
    code[n++] = mips_r(26u, 0u, 0u, 0x11u);      /* mthi k0 */
    code[n++] = mips_i(0x0Fu, 0u, 26u, 0x2468); /* lui k0, 2468h */
    code[n++] = mips_i(0x0Du, 26u, 26u, 0xACE0); /* ori k0, k0, ACE0h */
    code[n++] = mips_r(26u, 0u, 0u, 0x13u);      /* mtlo k0 */
    code[n++] = 0u;                            /* nop: HI/LO setup spacing */
    code[n++] = mips_i(0x0Du, 0u, 26u, 0x0401); /* ori k0, zero, IEc|IM2 */
    code[n++] = (0x10u << 26) | (4u << 21) | (26u << 16) | (12u << 11); /* mtc0 k0, SR */
    code[n++] = 0u;                            /* nop: COP0 write spacing */

    size_t wait = n;
    code[n++] = mips_i(4u, 0u, 0u, -1); /* wait: beq zero, zero, self */
    code[n++] = 0u;                      /* nop: branch delay slot */

    CHECK(n < IRQ_ENTRY_WORD);
    while (n < IRQ_ENTRY_WORD) {
        code[n++] = 0u;
    }

    /* The registered guest ISR snapshots its JmpBuf inputs, observes the
       masked pending bit, acknowledges only IRQ0, and calls B0(17h). */
    code[n++] = mips_i(0x0Fu, 0u, 26u, 0x8001); /* lui k0, 8001h */
    code[n++] = mips_i(0x2Bu, 26u, 31u, 0x0350); /* sw ra, observed+0(k0) */
    code[n++] = mips_i(0x2Bu, 26u, 2u, 0x0354);  /* sw v0, observed+4(k0) */
    code[n++] = mips_i(0x2Bu, 26u, 29u, 0x0358); /* sw sp, observed+8(k0) */
    code[n++] = mips_i(0x2Bu, 26u, 30u, 0x035C); /* sw fp, observed+12(k0) */
    for (uint32_t reg = 16u; reg <= 23u; reg++) {
        code[n++] = mips_i(0x2Bu, 26u, reg, (int32_t)(0x0360u + (reg - 16u) * 4u));
        /* sw sN, observed+16h+4*N(k0) */
    }
    code[n++] = mips_i(0x2Bu, 26u, 28u, 0x0380); /* sw gp, observed+30h(k0) */
    code[n++] = mips_i(0x0Fu, 0u, 26u, 0xBF80); /* lui k0, BF80h */
    code[n++] = mips_i(0x23u, 26u, 27u, 0x1070); /* lw k1, I_STAT(k0) */
    code[n++] = mips_i(0x23u, 26u, 8u, 0x1074);  /* lw t0, I_MASK(k0) */
    code[n++] = 0u;                             /* nop: load delay */
    code[n++] = mips_r(27u, 8u, 27u, 0x24u);     /* and k1, k1, t0 */
    code[n++] = mips_i(0x0Fu, 0u, 26u, 0x8001); /* lui k0, 8001h */
    code[n++] = mips_i(0x2Bu, 26u, 27u, 0x0384); /* sw k1, observed+34h(k0) */
    code[n++] = mips_i(0x0Fu, 0u, 26u, 0xBF80); /* lui k0, BF80h */
    code[n++] = mips_i(0x09u, 0u, 27u, -2);     /* addiu k1, zero, -2 */
    code[n++] = mips_i(0x2Bu, 26u, 27u, 0x1070); /* sw k1, I_STAT(k0) */
    code[n++] = mips_i(0x0Fu, 0u, 26u, 0x8001); /* lui k0, 8001h */
    code[n++] = mips_i(0x23u, 26u, 27u, 0x0340); /* lw k1, callback(k0) */
    code[n++] = 0u;                             /* nop: load delay */
    code[n++] = mips_i(0x09u, 27u, 27u, 1);     /* addiu k1, k1, 1 */
    code[n++] = mips_i(0x2Bu, 26u, 27u, 0x0340); /* sw k1, callback(k0) */
    code[n++] = mips_i(0x0Fu, 0u, 8u, 0x8000);  /* lui t0, 8000h */
    code[n++] = mips_i(0x0Du, 8u, 8u, 0x00B0);  /* ori t0, t0, B0h */
    code[n++] = mips_i(0x09u, 0u, 9u, 0x17);    /* addiu t1, zero, 17h */
    code[n++] = mips_r(8u, 0u, 31u, 0x09u);     /* jalr ra, t0 */
    code[n++] = 0u;                             /* nop: jalr delay slot */
    size_t isr_spin = n;
    code[n++] = mips_j(2u, 0x80010100u + (uint32_t)isr_spin * 4u); /* j isr_spin */
    code[n++] = 0u;                             /* nop: jump delay slot */

    build_exe(exe, code, n, 0u);
    const uint32_t jump_buffer[12] = {
        IRQ_ENTRY, 0x801FF000u, 0x12345000u,
        0x00001600u, 0x00001601u, 0x00001602u, 0x00001603u,
        0x00001604u, 0x00001605u, 0x00001606u, 0x00001607u,
        0x80012000u
    };
    for (size_t i = 0u; i < sizeof jump_buffer / sizeof jump_buffer[0]; i++) {
        put32(exe + 0x800u + 0x300u + 4u * i, jump_buffer[i]);
    }
    put32(exe + 0x800u + 0x340u, 0u);

    schway *inst = make();
    load(inst, exe, sizeof exe);
    schway_run_result r = run(inst, 10000u, 0u);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    CHECK_EQ_HEX(le32(inst->bus.ram + 0x10340u), 0u);
    const uint32_t wait_pc = 0x80010100u + (uint32_t)wait * 4u;
    for (uint32_t step = 0u; step < 8u && inst->cpu.pc != wait_pc; step++) {
        r = run(inst, 1u, 0u);
        CHECK_EQ_U64(r.stop, SCHWAY_STOP_BUDGET);
        CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    }
    const uint32_t interrupted_pc = inst->cpu.pc;
    CHECK_EQ_HEX(interrupted_pc, wait_pc);
    for (uint32_t reg = 1u; reg < 32u; reg++) {
        if (reg != 26u) {
            CHECK_EQ_HEX(inst->cpu.r[reg], 0x100u + reg);
        }
    }

    /* The test host supplies only an IRQ-controller edge; CPU entry and all
       hook work still run as guest instructions under schway_run. */
    schway__irq_raise(inst, 1u);
    schway__irq_raise(inst, 0u);
    uint32_t saved_epc = 0u;
    uint32_t was_stub = 0u;
    uint32_t entered = 0u;
    uint32_t returned = 0u;
    for (uint32_t step = 0u; step < 4096u && returned == 0u; step++) {
        r = run(inst, 1u, 0u);
        CHECK_EQ_U64(r.stop, SCHWAY_STOP_BUDGET);
        CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
        uint32_t pc = inst->cpu.pc;
        uint32_t in_stub = (pc >= 0xBFC00000u &&
                            pc < 0xBFC00000u + schway__stub_kernel_size) ||
                           (pc >= 0x80000080u && pc < 0x800000D0u);
        if (in_stub != 0u) {
            was_stub = 1u;
            entered = 1u;
            saved_epc = inst->cpu.cop0.r[SCHWAY__COP0_EPC];
        } else if (was_stub != 0u && entered != 0u && pc == saved_epc) {
            returned = 1u;
            was_stub = 0u;
        }
    }
    CHECK_EQ_U64(entered, 1u);
    CHECK_EQ_U64(returned, 1u);
    CHECK_EQ_U64(le32(inst->bus.ram + 0x10340u), 1u);
    CHECK_EQ_HEX(le32(inst->bus.ram + 0x10384u), 1u);
    CHECK_EQ_U64(inst->irq.stat & 1u, 0u);
    CHECK_EQ_U64(inst->irq.stat & 2u, 2u);
    CHECK_EQ_U64(inst->irq.mask & 1u, 1u);
    CHECK_EQ_U64(le32(inst->bus.ram + 0x304u), 0u);
    CHECK_EQ_HEX(saved_epc, wait_pc);
    CHECK_EQ_HEX(le32(inst->bus.ram + 0x398u), wait_pc);
    CHECK_EQ_HEX(inst->cpu.cop0.r[SCHWAY__COP0_EPC], wait_pc);
    CHECK_EQ_HEX(inst->cpu.hi, 0x13579BDFu);
    CHECK_EQ_HEX(inst->cpu.lo, 0x2468ACE0u);
    CHECK_EQ_HEX(inst->cpu.cop0.r[SCHWAY__COP0_SR], 0x0401u);
    for (uint32_t reg = 1u; reg < 32u; reg++) {
        if (reg != 26u) {
            CHECK_EQ_HEX(inst->cpu.r[reg], 0x100u + reg);
        }
    }
    CHECK_EQ_HEX(le32(inst->bus.ram + 0x10350u), IRQ_ENTRY);
    CHECK_EQ_HEX(le32(inst->bus.ram + 0x10354u), 1u);
    CHECK_EQ_HEX(le32(inst->bus.ram + 0x10358u), jump_buffer[1]);
    CHECK_EQ_HEX(le32(inst->bus.ram + 0x1035Cu), jump_buffer[2]);
    for (size_t i = 3u; i <= 10u; i++) {
        CHECK_EQ_HEX(le32(inst->bus.ram + 0x10350u + 4u * (i + 1u)), jump_buffer[i]);
    }
    CHECK_EQ_HEX(le32(inst->bus.ram + 0x10380u), jump_buffer[11]);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    schway_destroy(inst);
#undef IRQ_ENTRY
}

static const uint32_t IRQ_FIXTURE_ENTRY = 0x80010400u;
static const uint32_t IRQ_FIXTURE_WAIT_LIMIT = 4096u;

typedef struct irq_fixture {
    uint8_t exe[EXE_SIZE];
    uint32_t wait_pc;
    uint32_t return_pc;
} irq_fixture;

/* A smaller shared guest for the repeated-return, live-state and exception
   controls below. Its expected values are fixed by the guest, not read back
   from the stub's private save area. */
static void build_irq_fixture(irq_fixture *f)
{
    uint32_t code[256] = {0u};
    size_t n = 0u;
    code[n++] = mips_i(0x0Fu, 0u, 4u, 0x8001);
    code[n++] = mips_i(0x0Du, 4u, 4u, 0x0300);
    code[n++] = mips_i(0x0Fu, 0u, 8u, 0x8000);
    code[n++] = mips_i(0x0Du, 8u, 8u, 0x00B0);
    code[n++] = mips_i(0x09u, 0u, 9u, 0x19);
    code[n++] = mips_r(8u, 0u, 31u, 0x09u);
    code[n++] = 0u;
    code[n++] = mips_i(0x0Fu, 0u, 8u, 0xBF80);
    code[n++] = mips_i(0x09u, 0u, 9u, 1);
    code[n++] = mips_i(0x2Bu, 8u, 9u, 0x1074);
    for (uint32_t reg = 1u; reg < 32u; reg++) {
        if (reg != 26u) {
            code[n++] = mips_i(0x09u, 0u, reg, (int32_t)(0x100u + reg));
        }
    }
    code[n++] = mips_i(0x0Fu, 0u, 26u, 0x1357);
    code[n++] = mips_i(0x0Du, 26u, 26u, 0x9BDF);
    code[n++] = mips_r(26u, 0u, 0u, 0x11u);
    code[n++] = mips_i(0x0Fu, 0u, 26u, 0x2468);
    code[n++] = mips_i(0x0Du, 26u, 26u, 0xACE0);
    code[n++] = mips_r(26u, 0u, 0u, 0x13u);
    code[n++] = 0u;
    code[n++] = mips_i(0x0Du, 0u, 26u, 0x0401);
    code[n++] = (0x10u << 26) | (4u << 21) | (26u << 16) | (12u << 11);
    code[n++] = 0u;
    size_t wait = n;
    code[n++] = mips_i(4u, 0u, 0u, -1);
    code[n++] = 0u;
    while (n < 0xC0u) {
        code[n++] = 0u;
    }
    code[n++] = mips_i(0x0Fu, 0u, 26u, 0x8001);
    code[n++] = mips_i(0x2Bu, 26u, 31u, 0x0350);
    code[n++] = mips_i(0x2Bu, 26u, 2u, 0x0354);
    code[n++] = mips_i(0x2Bu, 26u, 29u, 0x0358);
    code[n++] = mips_i(0x2Bu, 26u, 30u, 0x035C);
    for (uint32_t reg = 16u; reg <= 23u; reg++) {
        code[n++] = mips_i(0x2Bu, 26u, reg, (int32_t)(0x0360u + (reg - 16u) * 4u));
    }
    code[n++] = mips_i(0x2Bu, 26u, 28u, 0x0380);
    code[n++] = mips_i(0x0Fu, 0u, 26u, 0xBF80);
    code[n++] = mips_i(0x23u, 26u, 27u, 0x1070);
    code[n++] = mips_i(0x23u, 26u, 8u, 0x1074);
    code[n++] = 0u;
    code[n++] = mips_r(27u, 8u, 27u, 0x24u);
    code[n++] = mips_i(0x0Fu, 0u, 26u, 0x8001);
    code[n++] = mips_i(0x2Bu, 26u, 27u, 0x0384);
    code[n++] = mips_i(0x0Fu, 0u, 26u, 0xBF80);
    code[n++] = mips_i(0x09u, 0u, 27u, -2);
    code[n++] = mips_i(0x2Bu, 26u, 27u, 0x1070);
    code[n++] = mips_i(0x0Fu, 0u, 26u, 0x8001);
    code[n++] = mips_i(0x23u, 26u, 27u, 0x0340);
    code[n++] = 0u;
    code[n++] = mips_i(0x09u, 27u, 27u, 1);
    code[n++] = mips_i(0x2Bu, 26u, 27u, 0x0340);
    f->return_pc = 0x80010100u + (uint32_t)n * 4u;
    code[n++] = mips_i(0x0Fu, 0u, 8u, 0x8000);
    code[n++] = mips_i(0x0Du, 8u, 8u, 0x00B0);
    code[n++] = mips_i(0x09u, 0u, 9u, 0x17);
    code[n++] = mips_r(8u, 0u, 31u, 0x09u);
    code[n++] = 0u;
    size_t spin = n;
    code[n++] = mips_j(2u, 0x80010100u + (uint32_t)spin * 4u);
    code[n++] = 0u;
    build_exe(f->exe, code, n, 0u);
    f->wait_pc = 0x80010100u + (uint32_t)wait * 4u;
    const uint32_t jump_buffer[12] = {
        IRQ_FIXTURE_ENTRY, 0x801FF000u, 0x12345000u,
        0x00001600u, 0x00001601u, 0x00001602u, 0x00001603u,
        0x00001604u, 0x00001605u, 0x00001606u, 0x00001607u, 0x80012000u
    };
    for (size_t i = 0u; i < 12u; i++) {
        put32(f->exe + 0xB00u + 4u * i, jump_buffer[i]);
    }
    put32(f->exe + 0xB40u, 0u);
}

static schway *irq_fixture_load(const irq_fixture *f)
{
    schway *inst = make();
    load(inst, f->exe, sizeof f->exe);
    return inst;
}

static void irq_fixture_wait(schway *inst, const irq_fixture *f)
{
    schway_run_result r = run(inst, 10000u, 0u);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    for (uint32_t step = 0u; step < 8u && inst->cpu.pc != f->wait_pc; step++) {
        r = run(inst, 1u, 0u);
        CHECK_EQ_U64(r.stop, SCHWAY_STOP_BUDGET);
        CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    }
    CHECK_EQ_HEX(inst->cpu.pc, f->wait_pc);
    CHECK_EQ_U64(le32(inst->bus.ram + 0x10340u), 0u);
}

static int irq_fixture_run_to_pc(schway *inst, uint32_t pc)
{
    for (uint32_t step = 0u; step < IRQ_FIXTURE_WAIT_LIMIT && inst->cpu.pc != pc; step++) {
        schway_run_result r = run(inst, 1u, 0u);
        CHECK_EQ_U64(r.stop, SCHWAY_STOP_BUDGET);
        CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    }
    return inst->cpu.pc == pc;
}

static int irq_fixture_returned(schway *inst, const irq_fixture *f, uint32_t count)
{
    for (uint32_t step = 0u; step < IRQ_FIXTURE_WAIT_LIMIT; step++) {
        if (inst->cpu.pc == f->wait_pc && le32(inst->bus.ram + 0x10340u) == count) {
            return 1;
        }
        schway_run_result r = run(inst, 1u, 0u);
        CHECK_EQ_U64(r.stop, SCHWAY_STOP_BUDGET);
        CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    }
    return inst->cpu.pc == f->wait_pc && le32(inst->bus.ram + 0x10340u) == count;
}

static void irq_fixture_check_context(schway *inst, const irq_fixture *f)
{
    CHECK_EQ_HEX(inst->cpu.pc, f->wait_pc);
    CHECK_EQ_HEX(inst->cpu.cop0.r[SCHWAY__COP0_EPC], f->wait_pc);
    CHECK_EQ_HEX(inst->cpu.cop0.r[SCHWAY__COP0_SR], 0x0401u);
    CHECK_EQ_HEX(inst->cpu.hi, 0x13579BDFu);
    CHECK_EQ_HEX(inst->cpu.lo, 0x2468ACE0u);
    for (uint32_t reg = 1u; reg < 32u; reg++) {
        if (reg != 26u) {
            CHECK_EQ_HEX(inst->cpu.r[reg], 0x100u + reg);
        }
    }
}

static uint8_t *irq_fixture_save(schway *inst, size_t *size)
{
    *size = schway_state_size(inst);
    uint8_t *state = (uint8_t *)malloc(*size);
    CHECK(state != NULL);
    if (state == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(schway_save_state(inst, state, *size), SCHWAY_STATUS_OK);
    return state;
}

static schway *irq_fixture_load_state(const uint8_t *state, size_t size)
{
    schway *inst = make();
    CHECK_EQ_U64(schway_load_state(inst, state, size), SCHWAY_STATUS_OK);
    return inst;
}

static void irq_fixture_compare(schway *a, schway *b)
{
    CHECK_EQ_U64(le32(a->bus.ram + 0x10340u), le32(b->bus.ram + 0x10340u));
    CHECK_EQ_U64(a->irq.stat, b->irq.stat);
    CHECK_EQ_U64(a->irq.mask, b->irq.mask);
    CHECK(memcmp(&a->cpu, &b->cpu, sizeof a->cpu) == 0);
    CHECK(memcmp(a->bus.ram, b->bus.ram, sizeof a->bus.ram) == 0);
    size_t a_size = 0u;
    size_t b_size = 0u;
    uint8_t *a_state = irq_fixture_save(a, &a_size);
    uint8_t *b_state = irq_fixture_save(b, &b_size);
    CHECK_EQ_U64(a_size, b_size);
    CHECK(a_size == b_size && memcmp(a_state, b_state, a_size) == 0);
    free(a_state);
    free(b_state);
}

static void irq_fixture_write(schway *inst, uint32_t addr, uint32_t word)
{
    uint32_t cycles = 0u;
    memset(inst->cpu.icache_valid, 0, sizeof inst->cpu.icache_valid);
    CHECK_EQ_U64(schway__bus_write(inst, 0xA0000000u | (addr & 0x1FFFFFFFu), 4u, word, &cycles), 0u);
}

static void irq_fixture_expect_stop(schway *inst)
{
    schway_run_result r = run(inst, 10000u, 0u);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    CHECK_EQ_U64(le32(inst->bus.ram + 0x10340u), 0u);
}

static void stub_irq_context(void)
{
    irq_fixture f;
    build_irq_fixture(&f);
    schway *inst = irq_fixture_load(&f);
    irq_fixture_wait(inst, &f);
    schway__irq_raise(inst, 1u);
    schway__irq_raise(inst, 0u);
    CHECK(irq_fixture_returned(inst, &f, 1u));
    irq_fixture_check_context(inst, &f);
    CHECK_EQ_U64(inst->irq.stat & 1u, 0u);
    CHECK_EQ_U64(inst->irq.stat & 2u, 2u);
    schway__irq_raise(inst, 0u);
    CHECK(irq_fixture_returned(inst, &f, 2u));
    irq_fixture_check_context(inst, &f);
    CHECK_EQ_U64(inst->irq.stat & 1u, 0u);
    CHECK_EQ_U64(inst->irq.stat & 2u, 2u);
    schway_destroy(inst);
}

static void stub_irq_state(void)
{
    irq_fixture f;
    build_irq_fixture(&f);
    schway *source = irq_fixture_load(&f);
    irq_fixture_wait(source, &f);
    size_t registered_size = 0u;
    uint8_t *registered = irq_fixture_save(source, &registered_size);
    schway *straight = irq_fixture_load_state(registered, registered_size);
    schway *resumed = irq_fixture_load_state(registered, registered_size);
    free(registered);
    schway_destroy(source);

    schway__irq_raise(straight, 1u);
    schway__irq_raise(straight, 0u);
    CHECK(irq_fixture_run_to_pc(straight, f.return_pc));
    CHECK_EQ_U64(le32(straight->bus.ram + 0x10384u), 1u);
    CHECK_EQ_U64(le32(straight->bus.ram + 0x304u), 1u);
    CHECK_EQ_U64(straight->irq.stat & 1u, 0u);
    CHECK_EQ_U64(straight->irq.stat & 2u, 2u);
    size_t active_size = 0u;
    uint8_t *active = irq_fixture_save(straight, &active_size);
    schway_destroy(resumed);
    resumed = irq_fixture_load_state(active, active_size);
    free(active);

    CHECK(irq_fixture_returned(straight, &f, 1u));
    CHECK(irq_fixture_returned(resumed, &f, 1u));
    irq_fixture_compare(straight, resumed);
    schway__irq_raise(straight, 0u);
    schway__irq_raise(resumed, 0u);
    CHECK(irq_fixture_returned(straight, &f, 2u));
    CHECK(irq_fixture_returned(resumed, &f, 2u));
    irq_fixture_compare(straight, resumed);

    CHECK_EQ_U64(schway_reset(straight, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(le32(straight->bus.ram + 0x300u), 0u);
    CHECK_EQ_U64(le32(straight->bus.ram + 0x304u), 0u);
    schway_destroy(resumed);
    schway_destroy(straight);
}

static void stub_irq_controls(void)
{
    irq_fixture f;
    build_irq_fixture(&f);

    for (uint32_t with_handler = 0u; with_handler < 2u; with_handler++) {
        irq_fixture swmb = f;
        memcpy(swmb.exe + 0x800u, "SWMB", 4u);
        put32(swmb.exe + 0x80Cu, with_handler != 0u ? f.wait_pc : 0u);
        schway *inst = irq_fixture_load(&swmb);
        irq_fixture_wait(inst, &swmb);
        schway__irq_raise(inst, 0u);
        irq_fixture_expect_stop(inst);
        CHECK_EQ_HEX(mailbox(inst)[MB_STATUS], 0xFFu);
        CHECK_EQ_HEX(le32(inst->bus.ram + 0x10300u), IRQ_FIXTURE_ENTRY);
        schway_destroy(inst);
    }

    /* Registered state is cleared on reset; clearing it in a live guest
       provides the unregistered IRQ control without changing exception code. */
    schway *inst = irq_fixture_load(&f);
    irq_fixture_wait(inst, &f);
    put32(inst->bus.ram + 0x300u, 0u);
    schway__irq_raise(inst, 0u);
    irq_fixture_expect_stop(inst);
    CHECK_EQ_HEX(le32(inst->bus.ram + 0x214u), f.wait_pc);
    schway_destroy(inst);

    /* BREAK, unsupported SYS and inactive B0(17h) remain stopped exceptions. */
    inst = irq_fixture_load(&f);
    irq_fixture_wait(inst, &f);
    irq_fixture_write(inst, f.wait_pc, 0x0000000Du);
    irq_fixture_expect_stop(inst);
    CHECK_EQ_HEX(le32(inst->bus.ram + 0x210u), 0x24u);
    CHECK_EQ_HEX(le32(inst->bus.ram + 0x214u), f.wait_pc);
    schway_destroy(inst);

    inst = irq_fixture_load(&f);
    irq_fixture_wait(inst, &f);
    irq_fixture_write(inst, f.wait_pc, mips_i(0x0Du, 0u, 4u, 3));
    irq_fixture_write(inst, f.wait_pc + 4u, 0x0000000Cu);
    irq_fixture_write(inst, f.wait_pc + 8u, mips_i(0x09u, 0u, 16u, 1));
    irq_fixture_expect_stop(inst);
    CHECK_EQ_HEX(le32(inst->bus.ram + 0x210u), 0x20u);
    CHECK_EQ_HEX(le32(inst->bus.ram + 0x214u), f.wait_pc + 4u);
    CHECK_EQ_HEX(inst->cpu.r[16], 0x110u);
    schway_destroy(inst);

    inst = irq_fixture_load(&f);
    irq_fixture_wait(inst, &f);
    irq_fixture_write(inst, f.wait_pc, mips_i(0x0Fu, 0u, 8u, 0x8000));
    irq_fixture_write(inst, f.wait_pc + 4u, mips_i(0x0Du, 8u, 8u, 0x00B0));
    irq_fixture_write(inst, f.wait_pc + 8u, mips_i(0x09u, 0u, 9u, 0x17));
    irq_fixture_write(inst, f.wait_pc + 12u, mips_r(8u, 0u, 31u, 0x09u));
    irq_fixture_write(inst, f.wait_pc + 16u, 0u);
    irq_fixture_write(inst, f.wait_pc + 20u, mips_i(0x09u, 0u, 16u, 1));
    irq_fixture_expect_stop(inst);
    CHECK_EQ_HEX(le32(inst->bus.ram + 0x210u), 0x20u);
    CHECK_EQ_U64(le32(inst->bus.ram + 0x304u), 0u);
    CHECK_EQ_HEX(inst->cpu.r[16], 0x110u);
    schway_destroy(inst);
}

/* With no SWMB mailbox, the stub handles only BIOS SYS(01h) and SYS(02h):
   EnterCriticalSection tests and clears the saved IEp/IM2 bits, ExitCriticalSection
   sets them, and both return through RFE. A valid SWMB mailbox still takes the
   existing registered-handler path first; any other service remains stopped.
   Fault prevented: a real EXE's startup syscall being mistaken for an unhandled
   exception, or the small compatibility path swallowing fixture exceptions. */
static void stub_syscalls(void)
{
    const uint32_t code[] = {
        0x34080401u, /* ori t0, zero, 0x0401: IEc and IM2 enabled */
        0x40886000u, /* mtc0 t0, SR */
        0x00000000u, /* COP0 write spacing */
        0x34040001u, /* ori a0, zero, SYS(01h) */
        0x0000000Cu, /* syscall */
        0x00408021u, /* addu s0, v0, zero: save EnterCriticalSection result */
        0x34025678u, /* ori v0, zero, 0x5678 */
        0x34040002u, /* ori a0, zero, SYS(02h) */
        0x0000000Cu, /* syscall */
        0x00408821u, /* addu s1, v0, zero: SYS(02h) preserves v0 */
        0x34080001u, /* ori t0, zero, IEc without IM2 */
        0x40886000u, /* mtc0 t0, SR */
        0x00000000u, /* COP0 write spacing */
        0x34040001u, /* ori a0, zero, SYS(01h) */
        0x0000000Cu, /* syscall */
        0x00409021u, /* addu s2, v0, zero: only one saved bit was set */
        0x1000FFFFu, /* beq zero, zero, self */
        0x00000000u, /* delay slot */
    };
    uint8_t exe[EXE_SIZE];
    build_exe(exe, code, sizeof code / sizeof code[0], 0u);
    schway *inst = make();
    load(inst, exe, sizeof exe);
    schway_run_result r = run_frame(inst);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    CHECK_EQ_HEX(inst->cpu.r[16], 1u);
    CHECK_EQ_HEX(inst->cpu.r[17], 0x5678u);
    CHECK_EQ_HEX(inst->cpu.r[18], 0u);
    CHECK_EQ_HEX(inst->cpu.cop0.r[SCHWAY__COP0_SR], 0u);
    schway_destroy(inst);

    /* A valid SWMB mailbox keeps its existing priority over SYS(01h). The
       registered handler lives at 80010120h and loops without changing v0. */
    const uint32_t mailbox_code[] = {
        0x34021234u, /* ori v0, zero, 0x1234 */
        0x34040001u, /* ori a0, zero, SYS(01h) */
        0x0000000Cu, /* syscall */
        0x1000FFFFu, /* not reached: beq zero, zero, self */
        0x00000000u,
        0x00000000u,
        0x00000000u,
        0x00000000u,
        0x1000FFFFu, /* registered handler: beq zero, zero, self */
        0x00000000u,
    };
    build_exe(exe, mailbox_code, sizeof mailbox_code / sizeof mailbox_code[0], 0x80010120u);
    inst = make();
    load(inst, exe, sizeof exe);
    r = run_frame(inst);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    CHECK_EQ_HEX(le32(mailbox(inst) + MB_STATUS), 0xFFu);
    CHECK_EQ_HEX(inst->cpu.r[2], 0x1234u);
    CHECK(inst->cpu.pc == 0x80010120u || inst->cpu.pc == 0x80010124u);
    check_exc_record(inst, 0x20u, 0x80010108u);
    schway_destroy(inst);

    /* An unknown service selector keeps the existing recorded-and-stopped
       behavior when there is no registered image handler. */
    const uint32_t unknown_code[] = {
        0x34040003u, /* ori a0, zero, unsupported SYS(03h) */
        0x0000000Cu, /* syscall */
        0x1000FFFFu, /* caller continuation, which must not be reached */
        0x00000000u,
    };
    build_exe(exe, unknown_code, sizeof unknown_code / sizeof unknown_code[0], 0u);
    inst = make();
    load(inst, exe, sizeof exe);
    r = run_frame(inst);
    CHECK_EQ_U64(r.stop, SCHWAY_STOP_FRAME);
    check_exc_record(inst, 0x20u, 0x80010104u);
    CHECK(inst->cpu.pc >= 0xBFC00000u &&
          inst->cpu.pc < 0xBFC00000u + schway__stub_kernel_size);
    schway_destroy(inst);

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

/* Copies the public frame view so later runs cannot change the saved image. */
static size_t copy_motion_video(const schway *inst, uint32_t frame, uint8_t **copy)
{
    schway_video video;
    memset(&video, 0, sizeof video);
    video.size = (uint32_t)sizeof video;
    CHECK_EQ_U64(schway_get_video(inst, &video), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(video.frame, frame);
    CHECK_EQ_U64(video.width, 256u);
    CHECK_EQ_U64(video.height, 240u);
    CHECK_EQ_U64(video.pitch_bytes, 512u);
    CHECK_EQ_U64(video.bpp, 16u);
    CHECK_EQ_U64(video.dot_div, 10u);
    CHECK_EQ_U64(video.region, SCHWAY_REGION_NTSC_U);
    CHECK_EQ_U64(video.enabled, 1u);
    CHECK_EQ_U64(video.rgb24, 0u);
    size_t size = (size_t)video.pitch_bytes * video.height;
    CHECK(video.pixels != NULL && size != 0u);
    *copy = (uint8_t *)malloc(size);
    if (*copy == NULL || video.pixels == NULL) {
        exit(1);
    }
    memcpy(*copy, video.pixels, size);
    return size;
}

static void check_motion_pixels(const uint8_t *pixels, uint32_t drawn)
{
    const uint16_t *words = (const uint16_t *)pixels;
    for (uint32_t x = 0u; x < drawn; x++) {
        CHECK_EQ_HEX(words[100u * 256u + x], 0x001Fu);
    }
    CHECK_EQ_HEX(words[100u * 256u + drawn], 0u);
}

/* The assembled program draws one new GP0 pixel per beam interrupt. Check the
   visible pixels at three points and prove a frame-150 save can resume to the
   same frame and complete state as straight execution.
   Fault prevented: GPU commands that do not affect scan-out, a stationary
   primitive, or VRAM/frame state lost by loading a saved state. */
static void gpu_motion(const char *dir)
{
    size_t exe_size = 0u;
    uint8_t *exe = read_file(dir, "gpu_motion.exe", &exe_size);
    schway *straight = make();
    load(straight, exe, exe_size);
    free(exe);

    uint8_t *frame60 = NULL;
    uint8_t *frame120 = NULL;
    uint8_t *frame180 = NULL;
    size_t frame_size = 0u;
    size_t state_size = schway_state_size(straight);
    uint8_t *state150 = (uint8_t *)malloc(state_size);
    if (state150 == NULL) {
        exit(1);
    }
    for (uint32_t frame = 1u; frame <= 150u; frame++) {
        schway_run_result result = run_frame(straight);
        CHECK_EQ_U64(result.stop, SCHWAY_STOP_FRAME);
        CHECK_EQ_U64(result.frame, frame);
        if (frame == 60u) {
            frame_size = copy_motion_video(straight, frame, &frame60);
        } else if (frame == 120u) {
            CHECK_EQ_U64(copy_motion_video(straight, frame, &frame120), frame_size);
        }
    }
    CHECK_EQ_U64(schway_save_state(straight, state150, state_size), SCHWAY_STATUS_OK);

    schway *resumed = make();
    CHECK_EQ_U64(schway_load_state(resumed, state150, state_size), SCHWAY_STATUS_OK);
    for (uint32_t frame = 151u; frame <= 180u; frame++) {
        schway_run_result result = run_frame(resumed);
        CHECK_EQ_U64(result.stop, SCHWAY_STOP_FRAME);
        CHECK_EQ_U64(result.frame, frame);
    }
    size_t resumed_size = copy_motion_video(resumed, 180u, &frame180);

    for (uint32_t frame = 151u; frame <= 180u; frame++) {
        schway_run_result result = run_frame(straight);
        CHECK_EQ_U64(result.stop, SCHWAY_STOP_FRAME);
        CHECK_EQ_U64(result.frame, frame);
    }
    uint8_t *straight180 = NULL;
    CHECK_EQ_U64(copy_motion_video(straight, 180u, &straight180), resumed_size);
    CHECK_EQ_HEX(straight->dma.madr[2], 0x001100Cu);
    CHECK_EQ_HEX(straight->vram[100u][179u], 0x001Fu);
    CHECK(memcmp(frame180, straight180, resumed_size) == 0);
    CHECK(memcmp(frame60, frame120, frame_size) != 0);
    CHECK(memcmp(frame120, frame180, frame_size) != 0);

    uint8_t *straight_state = (uint8_t *)malloc(state_size);
    uint8_t *resumed_state = (uint8_t *)malloc(state_size);
    if (straight_state == NULL || resumed_state == NULL) {
        exit(1);
    }
    CHECK_EQ_U64(schway_save_state(straight, straight_state, state_size), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_save_state(resumed, resumed_state, state_size), SCHWAY_STATUS_OK);
    CHECK(memcmp(straight_state, resumed_state, state_size) == 0);

    check_motion_pixels(frame60, 60u);
    check_motion_pixels(frame120, 120u);
    check_motion_pixels(frame180, 180u);

    free(straight_state);
    free(resumed_state);
    free(frame60);
    free(frame120);
    free(frame180);
    free(straight180);
    free(state150);
    schway_destroy(resumed);
    schway_destroy(straight);
}

/* The whole of a file of at most 1 MiB, NUL-terminated after its n bytes. */
static uint8_t *read_whole(const char *path, size_t *n)
{
    const size_t cap = (size_t)1u << 20;
    FILE *f = schway__test_open_file(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "%s: cannot open\n", path);
        exit(1);
    }
    uint8_t *buf = (uint8_t *)malloc(cap + 1u);
    if (buf == NULL) {
        exit(1);
    }
    *n = fread(buf, 1u, cap, f);
    CHECK(*n > 0u && *n < cap);
    buf[*n] = 0u;
    fclose(f);
    return buf;
}

/* The expected values of the gte_vectors fixture and the vector file are two paths to one value: every
   gte_vectors row of the CSV is hw, its citation ends in the case number, and its value is the CRC-32 of that
   case's 64 output words (little-endian bytes) in the vector file (a u32 count, then 516-byte records: the
   command word, 64 inputs, 64 outputs). A value that drifted, a case that is not in the file and a CSV with
   no gte_vectors row all fail.
   Fault prevented: an expected value that drifted from the vector file it claims to come from. */
static void gte_vectors_guard(const char *bin_path, const char *csv_path)
{
    const size_t record = 516u;
    const size_t out_at = 4u + 64u * 4u; /* the outputs follow the command word and the inputs */
    size_t bn = 0u;
    size_t cn = 0u;
    uint8_t *bin = read_whole(bin_path, &bn);
    char *csv = (char *)read_whole(csv_path, &cn);
    CHECK(bn >= 4u);
    const uint32_t cases = bn >= 4u ? le32(bin) : 0u;
    CHECK(cases > 0u && bn == 4u + (size_t)cases * record);
    uint32_t rows = 0u;
    char *line = csv;
    while (*line != '\0') {
        char *end = strchr(line, '\n');
        if (end != NULL) {
            *end = '\0';
        }
        if (strncmp(line, "gte_vectors,", 12u) == 0) {
            char *field[6];
            uint32_t nf = 0u;
            char *p = line;
            while (nf < 6u) {
                field[nf++] = p;
                p = strchr(p, ',');
                if (p == NULL) {
                    break;
                }
                *p++ = '\0';
            }
            CHECK_EQ_U64(nf, 6u);
            if (nf == 6u) {
                CHECK(strchr(field[5], ',') == NULL); /* a seventh field: the judge splits on commas */
                char *tail = NULL;
                const unsigned long want = strtoul(field[3], &tail, 16);
                CHECK(field[3][0] != '\0' && *tail == '\0' && strlen(field[3]) == 8u);
                CHECK(strcmp(field[4], "hw") == 0);
                const char *at = strstr(field[5], " test ");
                const unsigned long n = at != NULL ? strtoul(at + 6, &tail, 10) : 0ul;
                CHECK(at != NULL && *tail == '\0' && n >= 1ul && n <= cases);
                if (at != NULL && n >= 1ul && n <= cases && bn == 4u + (size_t)cases * record) {
                    const uint8_t *words = bin + 4u + (size_t)(n - 1ul) * record + out_at;
                    const uint32_t got = schway__crc32_update(0u, words, 256u);
                    if (got != (uint32_t)want) {
                        fprintf(stderr, "%s: case %lu: the row says %08lx, the vector file's outputs give %08x\n",
                                field[1], n, want, (unsigned)got);
                        check_failures++;
                    }
                }
            }
            rows++;
        }
        if (end == NULL) {
            break;
        }
        line = end + 1;
    }
    CHECK(rows > 0u);
    free(bin);
    free(csv);
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
    } else if (strcmp(c, "stub_syscalls") == 0) {
        stub_syscalls();
    } else if (strcmp(c, "stub_irq_hook") == 0) {
        stub_irq_hook();
    } else if (strcmp(c, "stub_irq_context") == 0) {
        stub_irq_context();
    } else if (strcmp(c, "stub_irq_state") == 0) {
        stub_irq_state();
    } else if (strcmp(c, "stub_irq_controls") == 0) {
        stub_irq_controls();
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
    } else if (strcmp(c, "gpu_motion") == 0 && argc == 3) {
        gpu_motion(argv[2]);
    } else if (strcmp(c, "gte_vectors_guard") == 0 && argc == 4) {
        gte_vectors_guard(argv[2], argv[3]);
    } else {
        fprintf(stderr, "schwaystation-test-boot: unknown case %s\n", c);
        return 2;
    }
    CHECK_DONE();
}
