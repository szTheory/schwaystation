/* The load delay and the interlocks around it: schwaystation-test-delay <case>.
   White-box like the exception tests: the test places words in RAM, runs them
   with schway__cpu_step_one and no replay log, and reads the registers and the
   delay latches. Expected values are worked by hand from the psx-spx CPU
   Specifications page (load delay, COP0 registers) and from the Delay latches
   and Loads and stores sections of docs/hardware/cpu.md. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

#define NOP_WORD 0u
#define ADDU_WORD(rd, rs, rt) (((rs) << 21) | ((rt) << 16) | ((rd) << 11) | 0x21u)
#define JR_WORD(rs) (((rs) << 21) | 0x08u)
#define MFC0_WORD(rt, rd) (0x40000000u | ((rt) << 16) | ((rd) << 11))
#define RFE_WORD 0x42000010u
#define ITYPE(op, rs, rt, imm) (((op) << 26) | ((rs) << 21) | ((rt) << 16) | ((imm) & 0xFFFFu))
#define ORI_WORD(rt, rs, imm) ITYPE(0x0Du, rs, rt, imm)
#define LW_WORD(rt, off, base) ITYPE(0x23u, base, rt, off)
#define LWL_WORD(rt, off, base) ITYPE(0x22u, base, rt, off)
#define LWR_WORD(rt, off, base) ITYPE(0x26u, base, rt, off)

#define SR_IEC 0x00000001u
#define SR_IM2 0x00000400u
#define VECTOR_RAM 0x80000080u
#define CODE 0x80001000u
#define DATA 0x80002000u

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

/* Kernel mode, interrupts off, RAM vectors. */
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

/* Writes one word of RAM through the uncached segment, emptying the
   instruction cache so a rewritten word is fetched again. */
static void put(schway *inst, uint32_t addr, uint32_t word)
{
    uint32_t cycles = 0u;
    memset(inst->cpu.icache_valid, 0, sizeof inst->cpu.icache_valid);
    CHECK_EQ_U64(schway__bus_write(inst, 0xA0000000u | (addr & 0x1FFFFFFFu), 4u, word, &cycles), 0u);
}

static void step(schway *inst, uint32_t pc)
{
    inst->cpu.pc = pc;
    schway__cpu_step_one(inst, NULL);
}

static void next(schway *inst)
{
    schway__cpu_step_one(inst, NULL);
}

static uint32_t cop0(const schway *inst, uint32_t reg)
{
    return inst->cpu.cop0.r[reg];
}

/* LW lands one instruction late: the instruction in its delay slot reads the
   register as it was, and the one after reads the loaded word. r2 starts at 1,
   the word is 600Dh; ADDU r3 in the slot copies the old r2 and ADDU r4 after it
   the new one (psx-spx CPU Specifications, load delay; docs/hardware/cpu.md,
   Delay latches).
   Fault prevented: a load that is visible to the next instruction, or one that
   lands a second late. */
static void load_delay(void)
{
    schway *inst = make();
    inst->cpu.r[1] = DATA;
    inst->cpu.r[2] = 1u;
    put(inst, DATA, 0x600Du);
    put(inst, CODE, LW_WORD(2u, 0u, 1u));
    put(inst, CODE + 4u, ADDU_WORD(3u, 2u, 0u));
    put(inst, CODE + 8u, ADDU_WORD(4u, 2u, 0u));
    step(inst, CODE);
    CHECK_EQ_HEX(inst->cpu.r[2], 1u);
    CHECK_EQ_U64(inst->cpu.load_reg, 2u);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.r[3], 1u);
    CHECK_EQ_HEX(inst->cpu.r[2], 0x600Du);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.r[4], 0x600Du);
    CHECK_EQ_U64(inst->cpu.load_reg, SCHWAY__CPU_NO_LOAD);
    schway_destroy(inst);
}

/* A write to the loaded register, by the instruction in the delay slot, cancels
   the load: the later write wins and the loaded word never arrives. A second
   load of the same register in the slot does the same to the first
   (docs/hardware/cpu.md, Delay latches).
   Fault prevented: a late load that lands over the newer value the program
   wrote. */
static void write_cancels_pending(void)
{
    schway *inst = make();
    inst->cpu.r[1] = DATA;
    inst->cpu.r[2] = 1u;
    put(inst, DATA, 0x600Du);
    put(inst, DATA + 4u, 0x7777u);
    put(inst, CODE, LW_WORD(2u, 0u, 1u));
    put(inst, CODE + 4u, ORI_WORD(2u, 0u, 5u));
    put(inst, CODE + 8u, NOP_WORD);
    put(inst, CODE + 12u, NOP_WORD);
    step(inst, CODE);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.r[2], 5u);
    CHECK_EQ_U64(inst->cpu.load_reg, SCHWAY__CPU_NO_LOAD);
    next(inst);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.r[2], 5u);

    inst->cpu.r[2] = 1u;
    put(inst, CODE + 0x20u, LW_WORD(2u, 0u, 1u));
    put(inst, CODE + 0x24u, LW_WORD(2u, 4u, 1u));
    put(inst, CODE + 0x28u, NOP_WORD);
    step(inst, CODE + 0x20u);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.r[2], 1u);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.r[2], 0x7777u);
    schway_destroy(inst);
}

/* A load that fails, an address error or a bus error, leaves rt as it was and
   issues nothing, and a load an instruction earlier on another register still
   completes as the failing one ends (docs/hardware/cpu.md, Loads and stores: a
   failed load leaves rt untouched).
   Fault prevented: a load that writes zero or stale data into rt when it
   traps, or one that leaves an older load pending past the exception. */
static void failed_load_untouched(void)
{
    static const uint32_t bad[] = {DATA + 1u, 0xBF000000u};
    for (size_t i = 0u; i < sizeof bad / sizeof bad[0]; i++) {
        schway *inst = make();
        inst->cpu.r[1] = DATA;
        inst->cpu.r[4] = bad[i];
        inst->cpu.r[2] = 0x1234u;
        inst->cpu.r[3] = 0u;
        put(inst, DATA, 0x600Du);
        put(inst, CODE, LW_WORD(3u, 0u, 1u));
        put(inst, CODE + 4u, LW_WORD(2u, 0u, 4u));
        put(inst, VECTOR_RAM, NOP_WORD);
        step(inst, CODE);
        next(inst);
        CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
        CHECK_EQ_HEX(inst->cpu.r[3], 0x600Du);
        CHECK_EQ_HEX(inst->cpu.r[2], 0x1234u);
        CHECK_EQ_U64(inst->cpu.load_reg, SCHWAY__CPU_NO_LOAD);
        next(inst);
        CHECK_EQ_HEX(inst->cpu.r[2], 0x1234u);
        schway_destroy(inst);
    }
}

/* A load whose interrupt is taken right after it completes before the handler
   runs: the handler sees the register loaded, and the instruction the interrupt
   cut off, which would have read the old value, reads the new one when it runs
   after the return (docs/hardware/cpu.md, Delay latches: an interrupt between the
   two changes this).
   Fault prevented: a load left pending into the handler, which the handler's
   first instruction then lands, or a load that is lost by the entry. */
static void load_before_irq(void)
{
    schway *inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = SR_IEC | SR_IM2;
    schway__irq_write_mask(inst, 1u, 0xFFFFFFFFu);
    inst->cpu.r[1] = DATA;
    inst->cpu.r[2] = 1u;
    put(inst, DATA, 0x600Du);
    put(inst, CODE, NOP_WORD);
    put(inst, CODE + 4u, LW_WORD(2u, 0u, 1u));
    put(inst, CODE + 8u, ADDU_WORD(3u, 2u, 0u));
    put(inst, VECTOR_RAM, MFC0_WORD(26u, SCHWAY__COP0_EPC));
    put(inst, VECTOR_RAM + 4u, NOP_WORD);
    put(inst, VECTOR_RAM + 8u, JR_WORD(26u));
    put(inst, VECTOR_RAM + 12u, RFE_WORD);
    schway__irq_raise(inst, 0u);
    step(inst, CODE);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE + 8u);
    CHECK_EQ_HEX(inst->cpu.r[2], 0x600Du);
    CHECK_EQ_U64(inst->cpu.load_reg, SCHWAY__CPU_NO_LOAD);
    /* The handler acknowledges the source and returns. */
    schway__irq_write_stat(inst, 0u, 0xFFFFFFFFu);
    for (uint32_t i = 0u; i < 4u; i++) {
        next(inst);
    }
    CHECK_EQ_HEX(inst->cpu.pc, CODE + 8u);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.r[3], 0x600Du);
    schway_destroy(inst);
}

/* LWR then LWL on one register load a word from an unaligned address with no
   instruction between them: the second merges into the value the first is
   about to write, not the register's old contents, and the first load, which
   the second replaces, never lands on its own. The word at DATA + 1 is 44332211h
   (docs/hardware/cpu.md, Loads and stores: lane merges with no delay needed
   between the pair).
   Fault prevented: a pair that needs a nop between its halves, or one whose
   second half merges into stale register contents. */
static void lwl_lwr_interlock(void)
{
    schway *inst = make();
    inst->cpu.r[1] = DATA;
    inst->cpu.r[2] = 0xA5A5A5A5u;
    put(inst, DATA, 0x332211AAu);
    put(inst, DATA + 4u, 0x00000044u);
    put(inst, CODE, LWR_WORD(2u, 1u, 1u));
    put(inst, CODE + 4u, LWL_WORD(2u, 4u, 1u));
    put(inst, CODE + 8u, ADDU_WORD(3u, 2u, 0u));
    put(inst, CODE + 12u, NOP_WORD);
    step(inst, CODE);
    CHECK_EQ_HEX(inst->cpu.r[2], 0xA5A5A5A5u);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.r[2], 0xA5A5A5A5u);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.r[3], 0xA5A5A5A5u);
    CHECK_EQ_HEX(inst->cpu.r[2], 0x44332211u);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.r[2], 0x44332211u);
    schway_destroy(inst);
}

/* MFC0 has a delay slot of one instruction like a load: the register moves
   after the next instruction, which reads the old value, and a write to the same
   register by that next instruction wins (docs/hardware/cop0.md, Registers).
   Fault prevented: a COP0 read visible one instruction early, or landing over a
   newer write to its register. */
static void mfc0_delay(void)
{
    schway *inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = SR_IM2;
    inst->cpu.r[2] = 7u;
    put(inst, CODE, MFC0_WORD(2u, SCHWAY__COP0_SR));
    put(inst, CODE + 4u, ADDU_WORD(3u, 2u, 0u));
    put(inst, CODE + 8u, ADDU_WORD(4u, 2u, 0u));
    step(inst, CODE);
    CHECK_EQ_HEX(inst->cpu.r[2], 7u);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.r[3], 7u);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.r[4], SR_IM2);

    put(inst, CODE + 0x10u, MFC0_WORD(2u, SCHWAY__COP0_SR));
    put(inst, CODE + 0x14u, ORI_WORD(2u, 0u, 9u));
    put(inst, CODE + 0x18u, NOP_WORD);
    step(inst, CODE + 0x10u);
    next(inst);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.r[2], 9u);
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    static const struct {
        const char *name;
        void (*fn)(void);
    } cases[] = {
        {"load_delay", load_delay},
        {"write_cancels_pending", write_cancels_pending},
        {"failed_load_untouched", failed_load_untouched},
        {"load_before_irq", load_before_irq},
        {"lwl_lwr_interlock", lwl_lwr_interlock},
        {"mfc0_delay", mfc0_delay},
    };
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-delay <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].fn();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "schwaystation-test-delay: unknown case %s\n", argv[1]);
    return 2;
}
