/* Multiply and divide timing: schwaystation-test-timing <case>. White-box like
   the exception tests: the test places words in RAM, fills the instruction cache
   with them so a fetch adds no cycles, runs them with schway__cpu_step_one and
   reads the cycle counter, HI, LO and the completion stamp. A multiply or divide
   issued at cycle counter t has HI and LO ready at t plus its latency, so
   latency is the stamp minus the counter before the instruction; an early read
   of HI or LO waits until then (PSX-SYSTEM section 2, MULTU/MULT and DIV/DIVU;
   psx-spx CPU Specifications, multiply and divide timing). */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

#define NOP_WORD 0u
#define SYSCALL_WORD 0x0000000Cu
#define MULT_WORD(rs, rt) (((rs) << 21) | ((rt) << 16) | 0x18u)
#define MULTU_WORD(rs, rt) (((rs) << 21) | ((rt) << 16) | 0x19u)
#define DIV_WORD(rs, rt) (((rs) << 21) | ((rt) << 16) | 0x1Au)
#define DIVU_WORD(rs, rt) (((rs) << 21) | ((rt) << 16) | 0x1Bu)
#define MFHI_WORD(rd) (((rd) << 11) | 0x10u)
#define MTHI_WORD(rs) (((rs) << 21) | 0x11u)
#define MFLO_WORD(rd) (((rd) << 11) | 0x12u)
#define MTLO_WORD(rs) (((rs) << 21) | 0x13u)

#define VECTOR_RAM 0x80000080u
#define CODE 0x80001000u

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
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = 0u;
    return inst;
}

/* One word of RAM through the uncached segment. */
static void put(schway *inst, uint32_t addr, uint32_t word)
{
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, 0xA0000000u | (addr & 0x1FFFFFFFu), 4u, word, &cycles), 0u);
}

/* Fills the instruction cache with n words from addr, after the words are
   written, so that each is fetched with no charge beyond its own cycle. */
static void prime(schway *inst, uint32_t addr, uint32_t n)
{
    for (uint32_t k = 0u; k < n; k++) {
        uint32_t word = 0u;
        uint32_t cycles = 0u;
        CHECK_EQ_U64(schway__cpu_icache_fetch(inst, addr + 4u * k, &word, &cycles), 0u);
    }
}

/* Runs the instruction at pc and returns the cycles it took. */
static uint64_t run(schway *inst, uint32_t pc)
{
    uint64_t before = inst->cpu.cycle;
    inst->cpu.pc = pc;
    schway__cpu_step_one(inst, NULL);
    return inst->cpu.cycle - before;
}

/* The latency of one multiply or divide, as the stamp the instruction sets
   minus the counter before it, with the result and the cycles it took. */
struct md {
    uint64_t latency, took;
    uint32_t hi, lo;
};

static struct md issue(uint32_t word, uint32_t rs, uint32_t rt)
{
    struct md m;
    schway *inst = make();
    uint64_t before;
    inst->cpu.r[1] = rs;
    inst->cpu.r[2] = rt;
    put(inst, CODE, word);
    prime(inst, CODE, 1u);
    before = inst->cpu.cycle;
    m.took = run(inst, CODE);
    m.latency = inst->cpu.muldiv_ready - before;
    m.hi = inst->cpu.hi;
    m.lo = inst->cpu.lo;
    schway_destroy(inst);
    return m;
}

/* MULT takes 6 cycles when its first operand, with its sign mirrored (rs XOR
   its sign bit spread over the word), is below 800h, and the instruction itself
   costs one (PSX-SYSTEM section 2, MULTU/MULT). 7FFh is the largest, and the
   mirror of -800h is 7FFh. The product is exact: 7FFh * 3 = 17FDh, and
   -800h * 3 = -1800h.
   Fault prevented: a multiply that is charged by its second operand, by a
   sign-blind rule that gives a small negative the longest time, or by a bound
   one off. */
static void mult_6(void)
{
    struct md m = issue(MULT_WORD(1u, 2u), 0x7FFu, 3u);
    CHECK_EQ_U64(m.latency, 6u);
    CHECK_EQ_U64(m.took, 1u);
    CHECK_EQ_HEX(m.lo, 0x17FDu);
    CHECK_EQ_HEX(m.hi, 0u);
    m = issue(MULT_WORD(1u, 2u), 0u, 3u);
    CHECK_EQ_U64(m.latency, 6u);
    m = issue(MULT_WORD(1u, 2u), 0xFFFFF800u, 3u);
    CHECK_EQ_U64(m.latency, 6u);
    CHECK_EQ_HEX(m.lo, 0xFFFFE800u);
    CHECK_EQ_HEX(m.hi, 0xFFFFFFFFu);
    m = issue(MULT_WORD(1u, 2u), 0xFFFFFFFFu, 0xFFFFFFFFu);
    CHECK_EQ_U64(m.latency, 6u);
    CHECK_EQ_HEX(m.lo, 1u);
    CHECK_EQ_HEX(m.hi, 0u);
}

/* From 800h up to FFFFFh, by the same mirrored rs, MULT takes 9 cycles; -801h
   mirrors to 800h and -100000h to FFFFFh (PSX-SYSTEM section 2, MULTU/MULT).
   Fault prevented: a middle class that starts or ends one value off, on either
   sign. */
static void mult_9(void)
{
    static const uint32_t rs[] = {0x800u, 0xFFFFFu, 0xFFFFF7FFu, 0xFFF00000u};
    for (size_t i = 0u; i < sizeof rs / sizeof rs[0]; i++) {
        struct md m = issue(MULT_WORD(1u, 2u), rs[i], 1u);
        CHECK_EQ_U64(m.latency, 9u);
        CHECK_EQ_U64(m.took, 1u);
        CHECK_EQ_HEX(m.lo, rs[i]);
    }
}

/* Anything from 100000h up, mirrored, takes 13 cycles: 100000h, 7FFFFFFFh,
   -100001h and -80000000h (PSX-SYSTEM section 2, MULTU/MULT).
   Fault prevented: a longest class that begins early or never applies to a
   negative first operand. */
static void mult_13(void)
{
    static const uint32_t rs[] = {0x100000u, 0x7FFFFFFFu, 0xFFEFFFFFu, 0x80000000u};
    for (size_t i = 0u; i < sizeof rs / sizeof rs[0]; i++) {
        struct md m = issue(MULT_WORD(1u, 2u), rs[i], 1u);
        CHECK_EQ_U64(m.latency, 13u);
        CHECK_EQ_U64(m.took, 1u);
    }
}

/* MULTU classifies rs as it is, with no mirror: FFFFFFFFh is the longest class,
   where MULT of the same bits is the shortest, and FFFFF800h is the longest
   too (PSX-SYSTEM section 2, MULTU/MULT). The product is unsigned:
   FFFFFFFFh * 1 is 00000000FFFFFFFFh.
   Fault prevented: an unsigned multiply that takes the signed classification. */
static void multu_raw(void)
{
    struct md m = issue(MULTU_WORD(1u, 2u), 0xFFFFFFFFu, 1u);
    CHECK_EQ_U64(m.latency, 13u);
    CHECK_EQ_HEX(m.lo, 0xFFFFFFFFu);
    CHECK_EQ_HEX(m.hi, 0u);
    m = issue(MULT_WORD(1u, 2u), 0xFFFFFFFFu, 1u);
    CHECK_EQ_U64(m.latency, 6u);
    m = issue(MULTU_WORD(1u, 2u), 0xFFFFF800u, 1u);
    CHECK_EQ_U64(m.latency, 13u);
    m = issue(MULTU_WORD(1u, 2u), 0x7FFu, 1u);
    CHECK_EQ_U64(m.latency, 6u);
    m = issue(MULTU_WORD(1u, 2u), 0x800u, 1u);
    CHECK_EQ_U64(m.latency, 9u);
    m = issue(MULTU_WORD(1u, 2u), 0x100000u, 1u);
    CHECK_EQ_U64(m.latency, 13u);
}

/* DIV takes 36 cycles whatever the operands, a zero divisor and -80000000h by -1
   included, and never traps; the results are the documented ones: 7 / 2 is 3
   remainder 1, -7 / 2 is -3 remainder -1, a zero divisor gives LO = -1 and
   HI = rs (LO = +1 for a negative rs), and the one overflowing quotient wraps
   (PSX-SYSTEM section 2, DIV/DIVU).
   Fault prevented: a divide whose time depends on its operands, or one that
   traps on zero. */
static void div_36(void)
{
    struct md m = issue(DIV_WORD(1u, 2u), 7u, 2u);
    CHECK_EQ_U64(m.latency, 36u);
    CHECK_EQ_U64(m.took, 1u);
    CHECK_EQ_HEX(m.lo, 3u);
    CHECK_EQ_HEX(m.hi, 1u);
    m = issue(DIV_WORD(1u, 2u), 0xFFFFFFF9u, 2u);
    CHECK_EQ_U64(m.latency, 36u);
    CHECK_EQ_HEX(m.lo, 0xFFFFFFFDu);
    CHECK_EQ_HEX(m.hi, 0xFFFFFFFFu);
    m = issue(DIV_WORD(1u, 2u), 5u, 0u);
    CHECK_EQ_U64(m.latency, 36u);
    CHECK_EQ_HEX(m.lo, 0xFFFFFFFFu);
    CHECK_EQ_HEX(m.hi, 5u);
    m = issue(DIV_WORD(1u, 2u), 0xFFFFFFFBu, 0u);
    CHECK_EQ_U64(m.latency, 36u);
    CHECK_EQ_HEX(m.lo, 1u);
    CHECK_EQ_HEX(m.hi, 0xFFFFFFFBu);
    m = issue(DIV_WORD(1u, 2u), 0x80000000u, 0xFFFFFFFFu);
    CHECK_EQ_U64(m.latency, 36u);
    CHECK_EQ_HEX(m.lo, 0x80000000u);
    CHECK_EQ_HEX(m.hi, 0u);
}

/* DIVU takes 36 cycles too: 7 / 2, and a zero divisor gives LO = FFFFFFFFh and
   HI = rs; FFFFFFFFh / 2 is 7FFFFFFFh remainder 1 (PSX-SYSTEM section 2,
   DIV/DIVU).
   Fault prevented: an unsigned divide with a different time or a signed
   reading of a large dividend. */
static void divu_36(void)
{
    struct md m = issue(DIVU_WORD(1u, 2u), 7u, 2u);
    CHECK_EQ_U64(m.latency, 36u);
    CHECK_EQ_HEX(m.lo, 3u);
    CHECK_EQ_HEX(m.hi, 1u);
    m = issue(DIVU_WORD(1u, 2u), 5u, 0u);
    CHECK_EQ_U64(m.latency, 36u);
    CHECK_EQ_HEX(m.lo, 0xFFFFFFFFu);
    CHECK_EQ_HEX(m.hi, 5u);
    m = issue(DIVU_WORD(1u, 2u), 0xFFFFFFFFu, 2u);
    CHECK_EQ_U64(m.latency, 36u);
    CHECK_EQ_HEX(m.lo, 0x7FFFFFFFu);
    CHECK_EQ_HEX(m.hi, 1u);
}

/* An early read of HI or LO stalls until the result is ready: with k
   instructions between the multiply and the read, the read costs 1 plus what is
   left of the latency, and nothing once it has elapsed. MFLO straight after a
   13-cycle MULT takes 13 cycles, MFHI straight after a divide takes 36, and
   after 4 no-operations the MULT's read takes 9 (13 less the 1 for its own issue
   and 4 elapsed, plus its own cycle); after 20 it takes 1 (PSX-SYSTEM section 2,
   MULTU/MULT: reading HI/LO early stalls until done).
   Fault prevented: a read that returns before the result is there at no cost, or
   one that waits longer than the result takes. */
static void early_read_stalls(void)
{
    schway *inst = make();
    inst->cpu.r[1] = 0x100000u;
    inst->cpu.r[2] = 2u;
    put(inst, CODE, MULT_WORD(1u, 2u));
    put(inst, CODE + 4u, MFLO_WORD(3u));
    prime(inst, CODE, 2u);
    CHECK_EQ_U64(run(inst, CODE), 1u);
    CHECK_EQ_U64(run(inst, CODE + 4u), 13u);
    CHECK_EQ_HEX(inst->cpu.r[3], 0x200000u);
    schway_destroy(inst);

    inst = make();
    inst->cpu.r[1] = 100u;
    inst->cpu.r[2] = 7u;
    put(inst, CODE, DIV_WORD(1u, 2u));
    put(inst, CODE + 4u, MFHI_WORD(3u));
    prime(inst, CODE, 2u);
    CHECK_EQ_U64(run(inst, CODE), 1u);
    CHECK_EQ_U64(run(inst, CODE + 4u), 36u);
    CHECK_EQ_HEX(inst->cpu.r[3], 2u);
    schway_destroy(inst);

    for (uint32_t k = 4u; k <= 20u; k += 16u) {
        inst = make();
        inst->cpu.r[1] = 0x100000u;
        inst->cpu.r[2] = 2u;
        put(inst, CODE, MULT_WORD(1u, 2u));
        for (uint32_t i = 0u; i < k; i++) {
            put(inst, CODE + 4u + 4u * i, NOP_WORD);
        }
        put(inst, CODE + 4u + 4u * k, MFLO_WORD(3u));
        prime(inst, CODE, k + 2u);
        (void)run(inst, CODE);
        for (uint32_t i = 0u; i < k; i++) {
            (void)run(inst, CODE + 4u + 4u * i);
        }
        CHECK_EQ_U64(run(inst, CODE + 4u + 4u * k), k == 4u ? 9u : 1u);
        CHECK_EQ_HEX(inst->cpu.r[3], 0x200000u);
        schway_destroy(inst);
    }
}

/* A multiply or divide issued while another runs replaces it: the stamp is the
   new operation's own latency from its own issue, never the longer of the two. A
   13-cycle MULT then a DIV is ready 36 after the DIV, and a DIV then a 6-cycle
   MULT is ready 6 after the MULT, so a read right after it stalls 6 cycles
   (PSX-SYSTEM section 2, MULTU/MULT).
   Fault prevented: a new multiply that waits for the old one, or one whose
   stamp keeps the old, later completion. */
static void replace_while_busy(void)
{
    schway *inst = make();
    uint64_t before;
    inst->cpu.r[1] = 0x100000u;
    inst->cpu.r[2] = 2u;
    inst->cpu.r[4] = 0x7FFu;
    put(inst, CODE, MULT_WORD(1u, 2u));
    put(inst, CODE + 4u, DIV_WORD(1u, 2u));
    put(inst, CODE + 8u, MULT_WORD(4u, 2u));
    put(inst, CODE + 12u, MFLO_WORD(3u));
    prime(inst, CODE, 4u);
    (void)run(inst, CODE);
    CHECK_EQ_U64(inst->cpu.muldiv_ready - (inst->cpu.cycle - 1u), 13u);
    before = inst->cpu.cycle;
    (void)run(inst, CODE + 4u);
    CHECK_EQ_U64(inst->cpu.muldiv_ready - before, 36u);
    before = inst->cpu.cycle;
    (void)run(inst, CODE + 8u);
    CHECK_EQ_U64(inst->cpu.muldiv_ready - before, 6u);
    CHECK_EQ_U64(run(inst, CODE + 12u), 6u);
    CHECK_EQ_HEX(inst->cpu.r[3], 0xFFEu);
    schway_destroy(inst);
}

/* MTLO and MTHI set the stamp to 0: the running multiply is over as far as a
   read can tell, so a read straight after takes 1 cycle and gives the written
   value. Only that stall is pinned (PSX-SYSTEM section 2, MULTU/MULT).
   Fault prevented: a write to HI or LO that a running multiply then overwrites
   or that a read waits behind. */
static void mtlo_mthi_stamp_zero(void)
{
    for (uint32_t k = 0u; k < 2u; k++) {
        schway *inst = make();
        inst->cpu.r[1] = 0x100000u;
        inst->cpu.r[2] = 2u;
        inst->cpu.r[5] = 0x5A5Au;
        put(inst, CODE, MULT_WORD(1u, 2u));
        put(inst, CODE + 4u, k == 0u ? MTLO_WORD(5u) : MTHI_WORD(5u));
        put(inst, CODE + 8u, k == 0u ? MFLO_WORD(3u) : MFHI_WORD(3u));
        prime(inst, CODE, 3u);
        (void)run(inst, CODE);
        CHECK(inst->cpu.muldiv_ready != 0u);
        (void)run(inst, CODE + 4u);
        CHECK_EQ_U64(inst->cpu.muldiv_ready, 0u);
        CHECK_EQ_U64(run(inst, CODE + 8u), 1u);
        CHECK_EQ_HEX(inst->cpu.r[3], 0x5A5Au);
        schway_destroy(inst);
    }
}

/* An exception leaves HI, LO and the stamp alone: SYSCALL taken while a
   multiply runs changes none of them, and a read in the handler still stalls to
   the original completion (PSX-SYSTEM section 2, EPC and COP0 registers: the
   entry changes only COP0 and the program counter).
   Fault prevented: an exception entry that clears the multiply unit or
   forgets the result, so a handler reads zero. */
static void survives_exception(void)
{
    schway *inst = make();
    uint64_t ready;
    inst->cpu.r[1] = 0x100000u;
    inst->cpu.r[2] = 3u;
    put(inst, CODE, MULT_WORD(1u, 2u));
    put(inst, CODE + 4u, SYSCALL_WORD);
    put(inst, VECTOR_RAM, MFLO_WORD(3u));
    prime(inst, CODE, 2u);
    prime(inst, VECTOR_RAM, 1u);
    (void)run(inst, CODE);
    ready = inst->cpu.muldiv_ready;
    (void)run(inst, CODE + 4u);
    CHECK_EQ_U64(inst->cpu.cop0.r[SCHWAY__COP0_CAUSE] >> 2 & 31u, 8u);
    CHECK_EQ_HEX(inst->cpu.lo, 0x300000u);
    CHECK_EQ_HEX(inst->cpu.hi, 0u);
    CHECK_EQ_U64(inst->cpu.muldiv_ready, ready);
    (void)run(inst, VECTOR_RAM);
    CHECK_EQ_U64(inst->cpu.cycle, ready + 1u);
    CHECK_EQ_HEX(inst->cpu.r[3], 0x300000u);
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    static const struct {
        const char *name;
        void (*fn)(void);
    } cases[] = {
        {"mult_6", mult_6},
        {"mult_9", mult_9},
        {"mult_13", mult_13},
        {"multu_raw", multu_raw},
        {"div_36", div_36},
        {"divu_36", divu_36},
        {"early_read_stalls", early_read_stalls},
        {"replace_while_busy", replace_while_busy},
        {"mtlo_mthi_stamp_zero", mtlo_mthi_stamp_zero},
        {"survives_exception", survives_exception},
    };
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-timing <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].fn();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "schwaystation-test-timing: unknown case %s\n", argv[1]);
    return 2;
}
