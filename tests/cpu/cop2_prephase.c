/* COP2 before the GTE exists: schwaystation-test-cop2 <case>. White-box like
   the COP0 tests: the test places words in RAM, runs them with
   schway__cpu_step_one and no replay log, and reads the registers, the delay
   latches, the fault record and the unbuilt-device counters. The expected
   behaviour is the documented one for a CPU whose coprocessor 2 is not there
   yet (PSX-SYSTEM section 2, SR bits and Address and bus errors). */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

#define SR_CU2 0x40000000u
#define VECTOR_RAM 0x80000080u
#define CODE 0x80001000u
#define DATA 0x80002000u

/* COP2 is opcode 12h with the sub-operation in the rs field; LWC2 and SWC2
   are 32h and 3Ah. */
#define COP2_WORD(sub, rt, rd) (0x48000000u | ((sub) << 21) | ((rt) << 16) | ((rd) << 11))
#define MFC2_SUB 0u
#define CFC2_SUB 2u
#define MTC2_SUB 4u
#define CTC2_SUB 6u
#define BC2_SUB 8u
#define COP2_COMMAND 0x4A000001u
#define LWC2_WORD(base, rt, off) ((0x32u << 26) | ((base) << 21) | ((rt) << 16) | ((off) & 0xFFFFu))
#define SWC2_WORD(base, rt, off) ((0x3Au << 26) | ((base) << 21) | ((rt) << 16) | ((off) & 0xFFFFu))

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

/* A kernel-mode CPU with interrupts off and the RAM vectors; sr is the
   status register under test. */
static schway *make(uint32_t sr)
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
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = sr;
    return inst;
}

/* One word of RAM, written and read through the uncached segment. */
static void put(schway *inst, uint32_t addr, uint32_t word)
{
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, 0xA0000000u | (addr & 0x1FFFFFFFu), 4u, word, &cycles), 0u);
}

static uint32_t get(schway *inst, uint32_t addr)
{
    uint32_t word = 0u;
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_read(inst, 0xA0000000u | (addr & 0x1FFFFFFFu), 4u, &word, &cycles), 0u);
    return word;
}

/* The instruction at pc, run once. */
static void run_word(schway *inst, uint32_t pc, uint32_t word)
{
    put(inst, pc, word);
    inst->cpu.pc = pc;
    schway__cpu_step_one(inst, NULL);
}

static uint32_t cop0(const schway *inst, uint32_t reg)
{
    return inst->cpu.cop0.r[reg];
}

static uint32_t exc_code(const schway *inst)
{
    return (cop0(inst, SCHWAY__COP0_CAUSE) >> 2) & 31u;
}

static uint32_t gte_count(const schway *inst)
{
    return inst->bus.unbuilt_n[SCHWAY__DEV_GTE];
}

/* With SR.CU2 clear every COP2 form, the register moves, the condition
   branches and the commands, raises CpU with CE 2 at the instruction and
   latches no fault: a guest handler may take it. LWC2 and SWC2 check the
   address first, so a misaligned one is an address error and an aligned one is
   CpU with its memory untouched.
   Fault prevented: a missing GTE that answers as if it were there, a first
   fault recorded for an exception a guest can handle, or a store that reaches
   RAM before the coprocessor check. */
static void cu2_clear(void)
{
    static const uint32_t forms[] = {
        COP2_WORD(MFC2_SUB, 3u, 4u), COP2_WORD(CFC2_SUB, 3u, 4u), COP2_WORD(MTC2_SUB, 3u, 4u),
        COP2_WORD(CTC2_SUB, 3u, 4u), (COP2_WORD(BC2_SUB, 0u, 0u) | 3u), (COP2_WORD(BC2_SUB, 1u, 0u) | 3u),
        COP2_COMMAND,
        LWC2_WORD(1u, 5u, 0u),       SWC2_WORD(1u, 5u, 0u),
    };
    for (size_t i = 0; i < sizeof forms / sizeof forms[0]; i++) {
        schway *inst = make(0u);
        inst->cpu.r[1] = DATA;
        inst->cpu.r[3] = 0x1234u;
        put(inst, DATA, 0xCAFEBABEu);
        run_word(inst, CODE, forms[i]);
        CHECK_EQ_U64(exc_code(inst), 0x0Bu);
        CHECK_EQ_U64((cop0(inst, SCHWAY__COP0_CAUSE) >> 28) & 3u, 2u);
        CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE);
        CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
        CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
        CHECK_EQ_U64(gte_count(inst), 0u);
        CHECK_EQ_U64(inst->cpu.load_reg, SCHWAY__CPU_NO_LOAD);
        CHECK_EQ_U64(inst->cpu.branch_slot, 0u);
        CHECK_EQ_HEX(inst->cpu.r[3], 0x1234u);
        CHECK_EQ_HEX(get(inst, DATA), 0xCAFEBABEu);
        schway_destroy(inst);
    }

    /* Alignment first: LWC2 and SWC2 at an odd address are address errors, not
       CpU, with BadVaddr and the matching first fault, and no CE. */
    schway *inst = make(0u);
    inst->cpu.r[1] = DATA + 2u;
    run_word(inst, CODE, LWC2_WORD(1u, 5u, 0u));
    CHECK_EQ_U64(exc_code(inst), 0x04u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_BADVADDR), DATA + 2u);
    CHECK_EQ_U64((cop0(inst, SCHWAY__COP0_CAUSE) >> 28) & 3u, 0u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_MISALIGNED_READ);
    schway_destroy(inst);

    inst = make(0u);
    inst->cpu.r[1] = DATA + 1u;
    put(inst, DATA, 0xCAFEBABEu);
    run_word(inst, CODE, SWC2_WORD(1u, 5u, 0u));
    CHECK_EQ_U64(exc_code(inst), 0x05u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_BADVADDR), DATA + 1u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_MISALIGNED_WRITE);
    CHECK_EQ_HEX(get(inst, DATA), 0xCAFEBABEu);
    schway_destroy(inst);
}

/* With SR.CU2 set nothing raises. MFC2 and CFC2 give 0 through the one-slot
   load delay; MTC2, CTC2 and the commands do nothing; BC2F goes and BC2T does
   not; LWC2 does the real bus read and drops the data, so a read of a bus gap
   is still a bus error; SWC2 does the real write of 0. Each counts once as an
   access to the unbuilt GTE. An odd address is still an address error.
   Fault prevented: a register move that lands at once or never, a GTE access
   that is not counted (a hang polling it would go unexplained), a load or
   store that skips the bus, or a coprocessor check that hides an alignment
   error. */
static void cu2_set(void)
{
    /* MFC2 and CFC2: the value 0 arrives one instruction later. */
    for (uint32_t sub = MFC2_SUB; sub <= CFC2_SUB; sub += 2u) {
        schway *inst = make(SR_CU2);
        inst->cpu.r[3] = 0x1234u;
        run_word(inst, CODE, COP2_WORD(sub, 3u, 4u));
        CHECK_EQ_HEX(inst->cpu.pc, CODE + 4u);
        CHECK_EQ_U64(exc_code(inst), 0u);
        CHECK_EQ_HEX(inst->cpu.r[3], 0x1234u);
        CHECK_EQ_U64(inst->cpu.load_reg, 3u);
        CHECK_EQ_HEX(inst->cpu.load_value, 0u);
        CHECK_EQ_U64(gte_count(inst), 1u);
        CHECK_EQ_U64(inst->bus.last_unbuilt[0], SCHWAY__DEV_GTE);
        run_word(inst, CODE + 4u, 0u); /* SLL r0, r0, 0 */
        CHECK_EQ_HEX(inst->cpu.r[3], 0u);
        CHECK_EQ_U64(inst->cpu.load_reg, SCHWAY__CPU_NO_LOAD);
        schway_destroy(inst);
    }

    /* MTC2, CTC2 and a command change nothing the CPU can see. */
    static const uint32_t quiet[] = {COP2_WORD(MTC2_SUB, 3u, 4u), COP2_WORD(CTC2_SUB, 3u, 4u),
                                     COP2_COMMAND};
    for (size_t i = 0; i < sizeof quiet / sizeof quiet[0]; i++) {
        schway *inst = make(SR_CU2);
        inst->cpu.r[3] = 0x1234u;
        run_word(inst, CODE, quiet[i]);
        CHECK_EQ_HEX(inst->cpu.pc, CODE + 4u);
        CHECK_EQ_U64(exc_code(inst), 0u);
        CHECK_EQ_HEX(inst->cpu.r[3], 0x1234u);
        CHECK_EQ_U64(inst->cpu.load_reg, SCHWAY__CPU_NO_LOAD);
        CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
        CHECK_EQ_U64(gte_count(inst), 1u);
        schway_destroy(inst);
    }

    /* BC2F goes, BC2T does not; both have a delay slot. */
    schway *inst = make(SR_CU2);
    run_word(inst, CODE, (COP2_WORD(BC2_SUB, 0u, 0u) | 3u) /* BC2F +3 */);
    CHECK_EQ_U64(inst->cpu.branch_slot, 1u);
    CHECK_EQ_U64(inst->cpu.branch_take, 1u);
    CHECK_EQ_HEX(inst->cpu.branch_target, CODE + 4u + 12u);
    schway_destroy(inst);
    inst = make(SR_CU2);
    run_word(inst, CODE, (COP2_WORD(BC2_SUB, 1u, 0u) | 3u) /* BC2T +3 */);
    CHECK_EQ_U64(inst->cpu.branch_slot, 1u);
    CHECK_EQ_U64(inst->cpu.branch_take, 0u);
    schway_destroy(inst);

    /* LWC2: the read happens, the data is dropped, and nothing but the count
       changes. */
    inst = make(SR_CU2);
    inst->cpu.r[1] = DATA;
    inst->cpu.r[5] = 0x77u;
    put(inst, DATA, 0xCAFEBABEu);
    run_word(inst, CODE, LWC2_WORD(1u, 5u, 0u));
    CHECK_EQ_HEX(inst->cpu.pc, CODE + 4u);
    CHECK_EQ_U64(exc_code(inst), 0u);
    CHECK_EQ_HEX(inst->cpu.r[5], 0x77u);
    CHECK_EQ_U64(inst->cpu.load_reg, SCHWAY__CPU_NO_LOAD);
    CHECK_EQ_U64(gte_count(inst), 1u);
    CHECK_EQ_HEX(inst->bus.last_unbuilt[1], DATA);
    schway_destroy(inst);

    /* The read is real: an address no device answers is a bus error (ExcCode
       7) that carries no BadVaddr, and the GTE count is not touched. */
    inst = make(SR_CU2);
    inst->cpu.r[1] = 0xBF000000u;
    run_word(inst, CODE, LWC2_WORD(1u, 5u, 0u));
    CHECK_EQ_U64(exc_code(inst), 0x07u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_BADVADDR), 0u);
    CHECK_EQ_U64(gte_count(inst), 0u);
    schway_destroy(inst);

    /* SWC2 writes the word 0. */
    inst = make(SR_CU2);
    inst->cpu.r[1] = DATA;
    put(inst, DATA, 0xCAFEBABEu);
    run_word(inst, CODE, SWC2_WORD(1u, 5u, 0u));
    CHECK_EQ_HEX(inst->cpu.pc, CODE + 4u);
    CHECK_EQ_HEX(get(inst, DATA), 0u);
    CHECK_EQ_U64(gte_count(inst), 1u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    schway_destroy(inst);

    /* An odd address is an address error before the coprocessor is asked. */
    inst = make(SR_CU2);
    inst->cpu.r[1] = DATA + 2u;
    put(inst, DATA, 0xCAFEBABEu);
    run_word(inst, CODE, SWC2_WORD(1u, 5u, 0u));
    CHECK_EQ_U64(exc_code(inst), 0x05u);
    CHECK_EQ_HEX(get(inst, DATA), 0xCAFEBABEu);
    CHECK_EQ_U64(gte_count(inst), 0u);
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    static const struct {
        const char *name;
        void (*fn)(void);
    } cases[] = {
        {"cu2_clear", cu2_clear},
        {"cu2_set", cu2_set},
    };
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-cop2 <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].fn();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "schwaystation-test-cop2: unknown case %s\n", argv[1]);
    return 2;
}
