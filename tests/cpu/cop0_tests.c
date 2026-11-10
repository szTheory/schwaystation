/* COP0 and the exception entry through the CPU's one-instruction entry:
   schwaystation-test-cop0 <case>. White-box: the test sees src/, places words
   in RAM, runs them with schway__cpu_step_one and no replay log, and reads the
   registers, the delay latches and the fault record. Expected values are worked
   by hand from the psx-spx CPU Specifications page (COP0 registers, exception
   handling) and from docs/hardware/cop0.md. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

#define SYSCALL_WORD 0x0000000Cu
#define BREAK_WORD 0x0000000Du
#define ADD_WORD(rd, rs, rt) (((rs) << 21) | ((rt) << 16) | ((rd) << 11) | 0x20u)
#define ADDU_WORD(rd, rs, rt) (((rs) << 21) | ((rt) << 16) | ((rd) << 11) | 0x21u)
#define SUB_WORD(rd, rs, rt) (((rs) << 21) | ((rt) << 16) | ((rd) << 11) | 0x22u)
#define JR_WORD(rs) (((rs) << 21) | 0x08u)
#define JALR_WORD(rd, rs) (((rs) << 21) | ((rd) << 11) | 0x09u)
#define MFC0_WORD(rt, rd) (0x40000000u | ((rt) << 16) | ((rd) << 11))
#define MTC0_WORD(rt, rd) (0x40800000u | ((rt) << 16) | ((rd) << 11))
#define RFE_WORD 0x42000010u
#define ITYPE(op, rs, rt, imm) (((op) << 26) | ((rs) << 21) | ((rt) << 16) | ((imm) & 0xFFFFu))

/* SR with the coprocessor 1 bit, and the exception vectors. */
#define SR_CU1 0x20000000u
#define VECTOR_RAM 0x80000080u
#define VECTOR_BEV 0xBFC00180u

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
    /* Kernel mode, interrupts off, RAM vectors: the state the guest sees once
       its start-up code has cleared BEV. */
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = 0u;
    return inst;
}

/* Writes one word of RAM through the uncached segment. A test rewrites code in
   place, which a program would follow with a cache flush, so the instruction
   cache is emptied with it. */
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

/* The instruction at pc, run once. */
static void run_word(schway *inst, uint32_t pc, uint32_t word)
{
    put(inst, pc, word);
    step(inst, pc);
}

static uint32_t cop0(const schway *inst, uint32_t reg)
{
    return inst->cpu.cop0.r[reg];
}

static uint32_t exc_code(const schway *inst)
{
    return (cop0(inst, SCHWAY__COP0_CAUSE) >> 2) & 31u;
}

/* A delay-slot exception: BEQ r0, r0 (always taken) at 80001000h with a
   target of 80001010h, and SYSCALL in its slot. EPC is the branch, BD and BT
   are set, TAR is where the branch was going, and the pending branch is
   discarded: the next instruction comes from the vector. A branch that is not
   taken (BNE r0, r0) gives BD = 1, BT = 0 and the instruction after the slot
   in TAR (psx-spx CPU Specifications, COP0 registers TAR and CAUSE;
   docs/hardware/cop0.md, Entry).
   Fault prevented: an EPC that points at the slot, so the handler re-executes
   it, or a stale TAR or BT that sends a handler to the wrong place. */
static void entry_delay_slot(void)
{
    schway *inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_TAR] = 0xDEADBEEFu;
    put(inst, 0x80001004u, SYSCALL_WORD);
    run_word(inst, 0x80001000u, ITYPE(4u, 0u, 0u, 3u)); /* BEQ r0, r0, +3 */
    CHECK_EQ_U64(inst->cpu.branch_slot, 1u);
    CHECK_EQ_U64(inst->cpu.branch_take, 1u);
    CHECK_EQ_HEX(inst->cpu.branch_target, 0x80001010u);
    step(inst, 0x80001004u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), 0x80001000u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_CAUSE) & 0xC000007Cu, 0xC0000020u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_TAR), 0x80001010u);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
    CHECK_EQ_U64(inst->cpu.branch_slot | inst->cpu.branch_take | inst->cpu.branch_target, 0u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);

    /* Not taken. */
    inst->cpu.cop0.r[SCHWAY__COP0_CAUSE] = 0u;
    run_word(inst, 0x80002000u, ITYPE(5u, 0u, 0u, 3u)); /* BNE r0, r0, +3 */
    CHECK_EQ_U64(inst->cpu.branch_slot, 1u);
    CHECK_EQ_U64(inst->cpu.branch_take, 0u);
    put(inst, 0x80002004u, BREAK_WORD);
    step(inst, 0x80002004u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), 0x80002000u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_CAUSE) & 0xC000007Cu, 0x80000024u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_TAR), 0x80002008u);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
    schway_destroy(inst);
}

/* An exception outside a delay slot: ADD of 7FFFFFFFh and 1 overflows, which
   is ExcCode 0Ch, leaves rd as it was, returns to the instruction itself,
   leaves BD, BT, TAR and BadVaddr alone, and pushes the SR mode stack: the
   six low bits 101101b become 110100b (current pair cleared, old pair lost)
   and everything above them is kept. With BEV set the vector is BFC00180h.
   ADDU of the same operands wraps and takes nothing, and SUB of
   80000000h - 1 overflows the other way (psx-spx CPU Specifications,
   exception handling; docs/hardware/cop0.md, SR, CAUSE and Entry).
   Fault prevented: a missing or mis-shifted mode stack, a changed rd after a
   trap, or a BadVaddr or TAR written by an exception that is not an address
   error or a delay slot. */
static void entry_plain(void)
{
    schway *inst = make();
    inst->cpu.r[1] = 0x7FFFFFFFu;
    inst->cpu.r[2] = 1u;
    inst->cpu.r[3] = 0x12345678u;
    inst->cpu.cop0.r[SCHWAY__COP0_BADVADDR] = 0x0BADC0DEu;
    inst->cpu.cop0.r[SCHWAY__COP0_TAR] = 0x0A0B0C0Du;
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = 0x0040FF2Du;
    inst->cpu.cop0.r[SCHWAY__COP0_CAUSE] = 0x00000700u;
    run_word(inst, 0x80003000u, ADD_WORD(3u, 1u, 2u));
    CHECK_EQ_U64(exc_code(inst), 0x0Cu);
    CHECK_EQ_HEX(inst->cpu.r[3], 0x12345678u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), 0x80003000u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_CAUSE), 0x00000730u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_SR), 0x0040FF34u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_BADVADDR), 0x0BADC0DEu);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_TAR), 0x0A0B0C0Du);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_BEV);

    run_word(inst, 0x80003010u, ADDU_WORD(3u, 1u, 2u));
    CHECK_EQ_HEX(inst->cpu.r[3], 0x80000000u);
    CHECK_EQ_HEX(inst->cpu.pc, 0x80003014u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), 0x80003000u);

    inst->cpu.r[1] = 0x80000000u;
    run_word(inst, 0x80003020u, SUB_WORD(4u, 1u, 2u));
    CHECK_EQ_U64(exc_code(inst), 0x0Cu);
    CHECK_EQ_HEX(inst->cpu.r[4], 0u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), 0x80003020u);
    /* ADDI with the same overflow rule. */
    inst->cpu.r[1] = 0x7FFFFFFFu;
    run_word(inst, 0x80003030u, ITYPE(8u, 1u, 5u, 1u));
    CHECK_EQ_U64(exc_code(inst), 0x0Cu);
    CHECK_EQ_HEX(inst->cpu.r[5], 0u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), 0x80003030u);
    schway_destroy(inst);
}

/* MTC0 to SR keeps the bits of the mask F04FFF3Fh and nothing else, and MFC0
   reads the result back through its delay slot (docs/hardware/cop0.md, SR
   and Registers).
   Fault prevented: a writable bit the CPU does not have, such as bit 7 or the
   timer-shutdown bit. */
static void sr_mask(void)
{
    schway *inst = make();
    inst->cpu.r[1] = 0xFFFFFFFFu;
    run_word(inst, 0x80001000u, MTC0_WORD(1u, SCHWAY__COP0_SR));
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_SR), 0xF04FFF3Fu);
    run_word(inst, 0x80001004u, MFC0_WORD(2u, SCHWAY__COP0_SR));
    run_word(inst, 0x80001008u, 0u);
    CHECK_EQ_HEX(inst->cpu.r[2], 0xF04FFF3Fu);
    inst->cpu.r[1] = 0u;
    run_word(inst, 0x8000100Cu, MTC0_WORD(1u, SCHWAY__COP0_SR));
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_SR), 0u);
    schway_destroy(inst);
}

/* MTC0 to CAUSE changes bits 8-9 only: the exception code, the coprocessor
   field, BT, BD and the interrupt line at bit 10 stay. TAR, BadVaddr, EPC and
   PRID ignore writes (docs/hardware/cop0.md, Registers).
   Fault prevented: a CAUSE write that clears a pending exception code or the
   interrupt line, or a handler-visible register a guest can overwrite. */
static void cause_writable(void)
{
    schway *inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_CAUSE] = 0xB0000428u;
    inst->cpu.cop0.r[SCHWAY__COP0_EPC] = 0x80004000u;
    inst->cpu.cop0.r[SCHWAY__COP0_TAR] = 0x80004004u;
    inst->cpu.cop0.r[SCHWAY__COP0_BADVADDR] = 0x80004008u;
    inst->cpu.r[1] = 0xFFFFFFFFu;
    run_word(inst, 0x80001000u, MTC0_WORD(1u, SCHWAY__COP0_CAUSE));
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_CAUSE), 0xB0000728u);
    inst->cpu.r[1] = 0u;
    run_word(inst, 0x80001004u, MTC0_WORD(1u, SCHWAY__COP0_CAUSE));
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_CAUSE), 0xB0000428u);
    run_word(inst, 0x80001008u, MTC0_WORD(1u, SCHWAY__COP0_EPC));
    run_word(inst, 0x8000100Cu, MTC0_WORD(1u, SCHWAY__COP0_TAR));
    run_word(inst, 0x80001010u, MTC0_WORD(1u, SCHWAY__COP0_BADVADDR));
    run_word(inst, 0x80001014u, MTC0_WORD(1u, SCHWAY__COP0_PRID));
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), 0x80004000u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_TAR), 0x80004004u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_BADVADDR), 0x80004008u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_PRID), 2u);
    schway_destroy(inst);
}

/* RFE shifts SR bits 2-5 into 0-3 at once, leaves bits 4-5 as they are and
   everything above them untouched, and does not jump: 101101b becomes
   101011b (docs/hardware/cop0.md, SR and RFE). CU0 is set so that the
   user mode the second step runs in may still use COP0.
   Fault prevented: a return from exception that loses the previous mode or
   interrupt-enable pair, or one that moves the program counter. */
static void rfe(void)
{
    schway *inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = 0x1040FF2Du;
    inst->cpu.cop0.r[SCHWAY__COP0_EPC] = 0x80005000u;
    run_word(inst, 0x80001000u, RFE_WORD);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_SR), 0x1040FF2Bu);
    CHECK_EQ_HEX(inst->cpu.pc, 0x80001004u);
    run_word(inst, 0x80001004u, RFE_WORD);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_SR), 0x1040FF2Au);
    schway_destroy(inst);
}

/* CE is set for a coprocessor-unusable exception only, to the coprocessor
   number, and the other exceptions write 0 there; BadVaddr is written by an
   address error and by nothing else (docs/hardware/cop0.md, CAUSE and
   Registers).
   Fault prevented: a stale CE after an unrelated exception, or a BadVaddr
   that follows every exception. */
static void ce_only_cpu(void)
{
    schway *inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_BADVADDR] = 0x0BADC0DEu;
    inst->cpu.cop0.r[SCHWAY__COP0_CAUSE] = 0x30000000u;
    run_word(inst, 0x80001000u, SYSCALL_WORD);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_CAUSE) >> 28, 0u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_BADVADDR), 0x0BADC0DEu);
    run_word(inst, 0x80001004u, 0x44000000u); /* COP1, CU1 clear */
    CHECK_EQ_U64(exc_code(inst), 0x0Bu);
    CHECK_EQ_U64((cop0(inst, SCHWAY__COP0_CAUSE) >> 28) & 3u, 1u);
    run_word(inst, 0x80001008u, 0x4C000000u); /* COP3, CU3 clear */
    CHECK_EQ_U64((cop0(inst, SCHWAY__COP0_CAUSE) >> 28) & 3u, 3u);
    /* COP0 in user mode with CU0 clear: CpU, CE 0. */
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = 0x00000002u;
    run_word(inst, 0x8000100Cu, RFE_WORD);
    CHECK_EQ_U64(exc_code(inst), 0x0Bu);
    CHECK_EQ_U64((cop0(inst, SCHWAY__COP0_CAUSE) >> 28) & 3u, 0u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_BADVADDR), 0x0BADC0DEu);
    schway_destroy(inst);
}

/* SR.BEV picks the general vector: BFC00180h while it is 1, 80000080h while
   it is 0 (docs/hardware/cop0.md, Entry).
   Fault prevented: a handler that is never entered because the vector reads
   the wrong BEV state. */
static void bev_vectors(void)
{
    schway *inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = 0x00400000u;
    run_word(inst, 0x80001000u, SYSCALL_WORD);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_BEV);
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = 0u;
    run_word(inst, 0x80001000u, SYSCALL_WORD);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
    schway_destroy(inst);
}

/* COP1 and COP3 with their SR.CUz bit clear raise CpU, and latch a first
   fault because the console has neither; with the bit set they do nothing
   (docs/hardware/cop0.md, SR and Coprocessor unusable).
   Fault prevented: a guest that touches a missing coprocessor running on as if
   it were there, or a first-fault record that stays empty. */
static void cop1_cop3_unusable(void)
{
    schway *inst = make();
    run_word(inst, 0x80001000u, 0x44000000u);
    CHECK_EQ_U64(exc_code(inst), 0x0Bu);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_COP_UNUSABLE);
    CHECK_EQ_HEX(inst->fault.pc, 0x80001000u);
    CHECK_EQ_HEX(inst->fault.opcode, 0x44000000u);
    schway_destroy(inst);

    inst = make();
    run_word(inst, 0x80001000u, 0x4C000000u);
    CHECK_EQ_U64(exc_code(inst), 0x0Bu);
    CHECK_EQ_U64((cop0(inst, SCHWAY__COP0_CAUSE) >> 28) & 3u, 3u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_COP_UNUSABLE);
    schway_destroy(inst);

    inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = SR_CU1;
    run_word(inst, 0x80001000u, 0x44000000u);
    CHECK_EQ_HEX(inst->cpu.pc, 0x80001004u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    CHECK_EQ_U64(exc_code(inst), 0u);
    schway_destroy(inst);

    /* The loads and stores of the two coprocessors follow the same rule: LWC1
       and SWC1 give CE 1, LWC3 and SWC3 give CE 3, and with the bit set they
       do nothing, even with 8 in the base field where a branch would sit. */
    static const struct {
        uint32_t word, ce;
    } mem[] = {{0xC4000000u, 1u}, {0xE4000000u, 1u}, {0xCC000000u, 3u}, {0xEC000000u, 3u}};
    for (size_t i = 0; i < sizeof mem / sizeof mem[0]; i++) {
        inst = make();
        run_word(inst, 0x80001000u, mem[i].word);
        CHECK_EQ_U64(exc_code(inst), 0x0Bu);
        CHECK_EQ_U64((cop0(inst, SCHWAY__COP0_CAUSE) >> 28) & 3u, mem[i].ce);
        CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_COP_UNUSABLE);
        schway_destroy(inst);

        inst = make();
        inst->cpu.cop0.r[SCHWAY__COP0_SR] = SR_CU1 | 0x80000000u;
        run_word(inst, 0x80001000u, mem[i].word | 0x01000000u);
        CHECK_EQ_HEX(inst->cpu.pc, 0x80001004u);
        CHECK_EQ_U64(inst->cpu.branch_slot, 0u);
        CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
        schway_destroy(inst);
    }
}

/* With the coprocessor usable, BCzF always goes and BCzT never does, since
   the condition line is never true; a taken one has its delay slot and its
   target like any branch (docs/hardware/cop0.md, SR and Entry).
   Fault prevented: a coprocessor branch that falls through where it must go,
   or the reverse. */
static void cop_branches(void)
{
    schway *inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = SR_CU1;
    run_word(inst, 0x80001000u, 0x45000000u | 5u); /* BC1F +5 */
    CHECK_EQ_U64(inst->cpu.branch_slot, 1u);
    CHECK_EQ_U64(inst->cpu.branch_take, 1u);
    CHECK_EQ_HEX(inst->cpu.branch_target, 0x80001018u);
    run_word(inst, 0x80001004u, 0u);
    CHECK_EQ_HEX(inst->cpu.pc, 0x80001018u);
    run_word(inst, 0x80002000u, 0x45010000u | 5u); /* BC1T +5 */
    CHECK_EQ_U64(inst->cpu.branch_slot, 1u);
    CHECK_EQ_U64(inst->cpu.branch_take, 0u);
    run_word(inst, 0x80002004u, 0u);
    CHECK_EQ_HEX(inst->cpu.pc, 0x80002008u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    schway_destroy(inst);
}

/* An encoding that is no instruction raises RI (ExcCode 0Ah) and latches a
   first fault with the pc and the opcode; so do MFC0 and MTC0 of the COP0
   registers that do not exist, 0-2, 4 and 10, while register 3 and 16 are
   accepted (docs/hardware/cop0.md, Registers).
   Fault prevented: an undefined opcode that retires as a no-op, or a COP0
   register number that answers when the CPU has none. */
static void reserved_instruction(void)
{
    static const uint32_t bad[] = {0x50000000u,       MFC0_WORD(1u, 0u), MTC0_WORD(1u, 10u),
                                   MTC0_WORD(1u, 4u), MFC0_WORD(1u, 2u), MTC0_WORD(1u, 1u),
                                   0x0000003Fu};
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        schway *inst = make();
        inst->cpu.r[1] = 0x55AA55AAu;
        run_word(inst, 0x80001000u, bad[i]);
        CHECK_EQ_U64(exc_code(inst), 0x0Au);
        CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), 0x80001000u);
        CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_RESERVED_INSTRUCTION);
        CHECK_EQ_U64(inst->fault.width, 4u);
        CHECK_EQ_HEX(inst->fault.pc, 0x80001000u);
        CHECK_EQ_HEX(inst->fault.opcode, bad[i]);
        CHECK_EQ_HEX(inst->cpu.r[1], 0x55AA55AAu);
        schway_destroy(inst);
    }
    schway *inst = make();
    run_word(inst, 0x80001000u, MTC0_WORD(1u, 3u));
    run_word(inst, 0x80001004u, MFC0_WORD(2u, 16u));
    CHECK_EQ_U64(exc_code(inst), 0u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    schway_destroy(inst);
}

/* SYSCALL, BREAK, an overflowing ADD and a coprocessor-unusable exception on
   COP0 are architectural exceptions a handler takes, so none of them leaves a
   first fault (ARCHITECTURE section 6 and docs/hardware/cop0.md, Which exceptions
   are a first fault).
   Fault prevented: a first-fault record filled by an exception that is part of
   normal guest behaviour, which would hide the first real fault. */
static void no_fault_for_traps(void)
{
    schway *inst = make();
    inst->cpu.r[1] = 0x7FFFFFFFu;
    inst->cpu.r[2] = 1u;
    run_word(inst, 0x80001000u, SYSCALL_WORD);
    run_word(inst, 0x80001004u, BREAK_WORD);
    run_word(inst, 0x80001008u, ADD_WORD(3u, 1u, 2u));
    CHECK_EQ_U64(exc_code(inst), 0x0Cu);
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = 0x00000002u;
    run_word(inst, 0x8000100Cu, MFC0_WORD(1u, SCHWAY__COP0_SR));
    CHECK_EQ_U64(exc_code(inst), 0x0Bu);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    schway_destroy(inst);
}

/* The same EPC and ExcCode 1,000 times in a row is an exception loop and a
   first fault whose note is the count. A different EPC or ExcCode starts the
   count again, and an interrupt (ExcCode 0) is neither counted nor breaks a
   run. 999 BREAKs at one address leave nothing; so do 999, a BREAK elsewhere
   and 999 more (ARCHITECTURE section 6, First fault).
   Fault prevented: a guest spinning in its own handler that runs to the time
   limit unexplained, or a counter that is never reset and flags a healthy
   program. */
static void exception_loop(void)
{
    schway *inst = make();
    for (uint32_t i = 0u; i < 999u; i++) {
        run_word(inst, 0x80001000u, BREAK_WORD);
    }
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    run_word(inst, 0x80001000u, BREAK_WORD);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_EXCEPTION_LOOP);
    CHECK_EQ_U64(inst->fault.note, 1000u);
    CHECK_EQ_HEX(inst->fault.pc, 0x80001000u);
    schway_destroy(inst);

    inst = make();
    for (uint32_t i = 0u; i < 999u; i++) {
        run_word(inst, 0x80001000u, BREAK_WORD);
    }
    run_word(inst, 0x80001004u, BREAK_WORD);
    for (uint32_t i = 0u; i < 999u; i++) {
        run_word(inst, 0x80001000u, BREAK_WORD);
    }
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    schway_destroy(inst);

    /* Another ExcCode at the same EPC resets the count; an interrupt entry
       between two BREAKs does not. */
    inst = make();
    for (uint32_t i = 0u; i < 999u; i++) {
        run_word(inst, 0x80001000u, BREAK_WORD);
    }
    run_word(inst, 0x80001000u, SYSCALL_WORD);
    run_word(inst, 0x80001000u, BREAK_WORD);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    schway_destroy(inst);

    inst = make();
    for (uint32_t i = 0u; i < 999u; i++) {
        run_word(inst, 0x80001000u, BREAK_WORD);
    }
    schway__cop0_exception(inst, SCHWAY__EXC_INT, 0x80001000u, 0u, 0u, 0u, 0u, 0u, 0u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    run_word(inst, 0x80001000u, BREAK_WORD);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_EXCEPTION_LOOP);
    CHECK_EQ_U64(inst->fault.note, 1000u);
    schway_destroy(inst);
}

/* MFC0 has one delay slot: the instruction after it still reads the old
   register, and the value lands as that instruction ends. A write to the same
   register by the next instruction cancels the load (docs/hardware/cpu.md,
   Delay latches, and docs/hardware/cop0.md, Registers).
   Fault prevented: a coprocessor read that is visible at once, or one that
   lands over a newer write. */
static void mfc0_delay(void)
{
    schway *inst = make();
    inst->cpu.r[5] = 0x11111111u;
    inst->cpu.cop0.r[SCHWAY__COP0_EPC] = 0xCAFE0000u;
    run_word(inst, 0x80001000u, MFC0_WORD(5u, SCHWAY__COP0_EPC));
    CHECK_EQ_U64(inst->cpu.load_reg, 5u);
    CHECK_EQ_HEX(inst->cpu.load_value, 0xCAFE0000u);
    CHECK_EQ_HEX(inst->cpu.r[5], 0x11111111u);
    run_word(inst, 0x80001004u, ADDU_WORD(6u, 5u, 0u));
    CHECK_EQ_HEX(inst->cpu.r[6], 0x11111111u);
    CHECK_EQ_HEX(inst->cpu.r[5], 0xCAFE0000u);
    run_word(inst, 0x80001008u, ADDU_WORD(7u, 5u, 0u));
    CHECK_EQ_HEX(inst->cpu.r[7], 0xCAFE0000u);

    run_word(inst, 0x8000100Cu, MFC0_WORD(5u, SCHWAY__COP0_EPC));
    run_word(inst, 0x80001010u, ITYPE(9u, 0u, 5u, 7u)); /* ADDIU r5, r0, 7 */
    CHECK_EQ_HEX(inst->cpu.r[5], 7u);
    CHECK_EQ_U64(inst->cpu.load_reg, SCHWAY__CPU_NO_LOAD);
    schway_destroy(inst);
}

/* BLTZAL and BGEZAL write the return address, the address after the delay
   slot, whether or not the branch goes (docs/hardware/cpu.md, Branches and jumps).
   Fault prevented: a call that loses its return address on the not-taken
   side. */
static void branch_links(void)
{
    schway *inst = make();
    inst->cpu.r[1] = 0x80000000u; /* negative */
    inst->cpu.r[31] = 0x12345678u;
    run_word(inst, 0x80001000u, ITYPE(1u, 1u, 0x10u, 4u)); /* BLTZAL r1, +4: taken */
    CHECK_EQ_U64(inst->cpu.branch_take, 1u);
    CHECK_EQ_HEX(inst->cpu.r[31], 0x80001008u);
    run_word(inst, 0x80001004u, 0u); /* the slot, which ends the pending branch */
    inst->cpu.r[31] = 0x12345678u;
    run_word(inst, 0x80002000u, ITYPE(1u, 1u, 0x11u, 4u)); /* BGEZAL r1, +4: not taken */
    CHECK_EQ_U64(inst->cpu.branch_take, 0u);
    CHECK_EQ_HEX(inst->cpu.r[31], 0x80002008u);
    run_word(inst, 0x80002004u, 0u);
    inst->cpu.r[31] = 0x12345678u;
    run_word(inst, 0x80003000u, ITYPE(1u, 1u, 0x00u, 4u)); /* BLTZ: no link */
    CHECK_EQ_HEX(inst->cpu.r[31], 0x12345678u);
    schway_destroy(inst);
}

/* JR and JALR to a misaligned target run the delay slot, then raise an address
   error on the fetch: ExcCode 4, and EPC and BadVaddr both the bad target
   (docs/hardware/cop0.md, Address errors on the fetch). JALR writes the return address first.
   Fault prevented: a jump to an odd address that executes whatever word the
   truncated address names. */
static void jr_misaligned(void)
{
    schway *inst = make();
    inst->cpu.r[1] = 0x80004002u;
    run_word(inst, 0x80001000u, JR_WORD(1u));
    run_word(inst, 0x80001004u, ITYPE(9u, 0u, 2u, 9u)); /* ADDIU r2, r0, 9 in the slot */
    CHECK_EQ_HEX(inst->cpu.r[2], 9u);
    CHECK_EQ_HEX(inst->cpu.pc, 0x80004002u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    step(inst, inst->cpu.pc);
    CHECK_EQ_U64(exc_code(inst), 4u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), 0x80004002u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_BADVADDR), 0x80004002u);
    CHECK_EQ_U64(cop0(inst, SCHWAY__COP0_CAUSE) >> 31, 0u);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_MISALIGNED_FETCH);
    schway_destroy(inst);

    inst = make();
    inst->cpu.r[1] = 0x80004001u;
    run_word(inst, 0x80001000u, JALR_WORD(3u, 1u));
    CHECK_EQ_HEX(inst->cpu.r[3], 0x80001008u);
    run_word(inst, 0x80001004u, 0u);
    step(inst, inst->cpu.pc);
    CHECK_EQ_U64(exc_code(inst), 4u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_BADVADDR), 0x80004001u);
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    static const struct {
        const char *name;
        void (*fn)(void);
    } cases[] = {
        {"entry_delay_slot", entry_delay_slot},
        {"entry_plain", entry_plain},
        {"sr_mask", sr_mask},
        {"cause_writable", cause_writable},
        {"rfe", rfe},
        {"ce_only_cpu", ce_only_cpu},
        {"bev_vectors", bev_vectors},
        {"cop1_cop3_unusable", cop1_cop3_unusable},
        {"cop_branches", cop_branches},
        {"reserved_instruction", reserved_instruction},
        {"no_fault_for_traps", no_fault_for_traps},
        {"exception_loop", exception_loop},
        {"mfc0_delay", mfc0_delay},
        {"branch_links", branch_links},
        {"jr_misaligned", jr_misaligned},
    };
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-cop0 <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].fn();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "schwaystation-test-cop0: unknown case %s\n", argv[1]);
    return 2;
}
