/* Exceptions and interrupts through the CPU's one-instruction entry:
   schwaystation-test-exception <case>. White-box: the test sees src/, places
   words in RAM, runs them with schway__cpu_step_one and no replay log, and reads
   the registers, the delay latches, the fault record and the retirement stamp.
   Expected values are worked by hand from the psx-spx CPU Specifications page
   (COP0 registers, exception handling) and from docs/hardware/cop0.md. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

#define NOP_WORD 0u
#define SYSCALL_WORD 0x0000000Cu
#define BREAK_WORD 0x0000000Du
#define ADD_WORD(rd, rs, rt) (((rs) << 21) | ((rt) << 16) | ((rd) << 11) | 0x20u)
#define JR_WORD(rs) (((rs) << 21) | 0x08u)
#define MFC0_WORD(rt, rd) (0x40000000u | ((rt) << 16) | ((rd) << 11))
#define MTC0_WORD(rt, rd) (0x40800000u | ((rt) << 16) | ((rd) << 11))
#define RFE_WORD 0x42000010u
#define ITYPE(op, rs, rt, imm) (((op) << 26) | ((rs) << 21) | ((rt) << 16) | ((imm) & 0xFFFFu))
#define LH_WORD(rt, off, base) ITYPE(0x21u, base, rt, off)
#define LW_WORD(rt, off, base) ITYPE(0x23u, base, rt, off)
#define SW_WORD(rt, off, base) ITYPE(0x2Bu, base, rt, off)
/* COP1's first instruction word (BC1F with no offset needs no operands), a
   command of the GTE and a register move of it. */
#define COP1_WORD 0x44000000u
#define COP2_COMMAND 0x4A000001u
#define COP2_MFC2 0x48020000u

/* SR: IEc, the interrupt mask bit of the hardware line (CAUSE bit 10), and the
   exception vectors. */
#define SR_IEC 0x00000001u
#define SR_IM0 0x00000100u
#define SR_IM2 0x00000400u
#define SR_BEV 0x00400000u
#define SR_CU2 0x40000000u
#define VECTOR_RAM 0x80000080u
#define VECTOR_BEV 0xBFC00180u

/* The registers' addresses on the uncached segment: I_MASK, and a word of the
   unused expansion region 1, which raises a bus error on a read or a fetch. */
#define I_MASK_KSEG1 0xBF801074u
#define UNMAPPED_KSEG1 0xBF000000u

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

/* Kernel mode, interrupts off, RAM vectors: the state the guest sees once its
   start-up code has cleared BEV. */
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

/* The next instruction, wherever the last one left the program counter. */
static void next(schway *inst)
{
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

/* Lets I_STAT bit 0 through the mask, as a program that wants the vertical
   blank does. */
static void enable_vblank(schway *inst)
{
    schway__irq_write_mask(inst, 1u, 0xFFFFFFFFu);
}

/* A device edge, with I_MASK and SR open, is taken two retirements later: the
   edge (the vertical blank raised at the end of a frame) happens between
   instructions, the two instructions after it retire, and the interrupt is taken
   before the third, so EPC names the third and the handler returns to it
   (docs/hardware/cop0.md, Interrupts; interrupts return to EPC + 0, Entry). CAUSE
   bit 10 shows the line at once, and entry pushes the mode stack.
   Fault prevented: an interrupt that is never taken, one taken at the edge with
   no latency, or one that returns past an instruction. */
static void irq_latency_device(void)
{
    schway *inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = SR_IEC | SR_IM2;
    enable_vblank(inst);
    for (uint32_t i = 0u; i < 4u; i++) {
        put(inst, CODE + 4u * i, NOP_WORD);
    }
    CHECK_EQ_U64(inst->cpu.irq_stamp, 0u);
    schway__irq_raise(inst, 0u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_CAUSE) & 0x400u, 0x400u);
    CHECK_EQ_U64(inst->cpu.irq_stamp, 2u);
    step(inst, CODE);
    CHECK_EQ_HEX(inst->cpu.pc, CODE + 4u);
    CHECK_EQ_U64(inst->cpu.irq_stamp, 1u);
    CHECK_EQ_U64(exc_code(inst), 0u);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE + 8u);
    CHECK_EQ_U64(exc_code(inst), 0u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_CAUSE) & 0xC0000400u, 0x400u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_SR), SR_IM2 | 0x4u);
    CHECK_EQ_U64(inst->cpu.irq_stamp, 0u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    schway_destroy(inst);
}

/* A kernel-mode CPU with the line open, I_STAT bit 0 raised at once and the
   first of the instructions after it retired: the interrupt is due on the next
   instruction, which the caller places at CODE + 4 (the stamp is 1). */
static schway *due(void)
{
    schway *inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = SR_IEC | SR_IM2;
    enable_vblank(inst);
    put(inst, CODE, NOP_WORD);
    schway__irq_raise(inst, 0u);
    step(inst, CODE);
    CHECK_EQ_U64(inst->cpu.irq_stamp, 1u);
    return inst;
}

/* The instruction at pc, run once. */
static void run_word(schway *inst, uint32_t pc, uint32_t word)
{
    put(inst, pc, word);
    step(inst, pc);
}

/* LH of an address that is not a multiple of 2 is an address error on a load
   (AdEL, ExcCode 4): BadVaddr is the address, EPC the instruction, the first
   fault is recorded and rt, which the load never reached, keeps its value with
   no load pending. The bus is not touched (psx-spx CPU Specifications,
   exception handling; docs/hardware/cpu.md, Loads and stores).
   Fault prevented: a misaligned half-word read that returns data, or one whose
   handler reads the wrong BadVaddr. */
static void adel_lh(void)
{
    schway *inst = make();
    inst->cpu.r[1] = DATA + 1u;
    inst->cpu.r[2] = 0x1234u;
    run_word(inst, CODE, LH_WORD(2u, 0u, 1u));
    CHECK_EQ_U64(exc_code(inst), 4u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_BADVADDR), DATA + 1u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
    CHECK_EQ_HEX(inst->cpu.r[2], 0x1234u);
    CHECK_EQ_U64(inst->cpu.load_reg, SCHWAY__CPU_NO_LOAD);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_MISALIGNED_READ);
    schway_destroy(inst);
}

/* LW of an address with either low bit set is AdEL too, with BadVaddr the whole
   address (psx-spx CPU Specifications, exception handling).
   Fault prevented: a word read at 2 mod 4 that is treated as aligned. */
static void adel_lw(void)
{
    schway *inst = make();
    inst->cpu.r[1] = DATA + 2u;
    inst->cpu.r[2] = 0x1234u;
    run_word(inst, CODE, LW_WORD(2u, 0u, 1u));
    CHECK_EQ_U64(exc_code(inst), 4u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_BADVADDR), DATA + 2u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE);
    CHECK_EQ_HEX(inst->cpu.r[2], 0x1234u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_MISALIGNED_READ);
    schway_destroy(inst);
}

/* SW of a misaligned address is an address error on a store (AdES, ExcCode 5),
   BadVaddr the address, and nothing is written: the word at the aligned address
   is as it was (psx-spx CPU Specifications, exception handling).
   Fault prevented: a misaligned store that lands in RAM before it traps. */
static void ades_sw(void)
{
    schway *inst = make();
    inst->cpu.r[1] = DATA + 3u;
    inst->cpu.r[2] = 0xA5A5A5A5u;
    put(inst, DATA, 0x11223344u);
    run_word(inst, CODE, SW_WORD(2u, 0u, 1u));
    CHECK_EQ_U64(exc_code(inst), 5u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_BADVADDR), DATA + 3u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE);
    CHECK_EQ_U64(inst->cpu.wq_count, 0u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_MISALIGNED_WRITE);
    inst->cpu.r[1] = DATA;
    inst->cpu.r[2] = 0u;
    run_word(inst, CODE + 0x10u, LW_WORD(2u, 0u, 1u));
    run_word(inst, CODE + 0x14u, NOP_WORD);
    CHECK_EQ_HEX(inst->cpu.r[2], 0x11223344u);
    schway_destroy(inst);
}

/* ADD of 7FFFFFFFh and 1 overflows: ExcCode 0Ch, rd as it was, EPC the
   instruction, no first fault and no BadVaddr (psx-spx CPU Specifications,
   exception handling; docs/hardware/cop0.md, CAUSE).
   Fault prevented: a wrapped sum written to rd, or a first fault a handler
   that fixes the overflow would hide. */
static void overflow_add(void)
{
    schway *inst = make();
    inst->cpu.r[1] = 0x7FFFFFFFu;
    inst->cpu.r[2] = 1u;
    inst->cpu.r[3] = 0xCAFEF00Du;
    inst->cpu.cop0.r[SCHWAY__COP0_BADVADDR] = 0x0BADC0DEu;
    run_word(inst, CODE, ADD_WORD(3u, 1u, 2u));
    CHECK_EQ_U64(exc_code(inst), 0x0Cu);
    CHECK_EQ_HEX(inst->cpu.r[3], 0xCAFEF00Du);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_BADVADDR), 0x0BADC0DEu);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    schway_destroy(inst);
}

/* An opcode that is not an instruction is a reserved-instruction exception
   (ExcCode 0Ah) with EPC the instruction and a first fault; so is a SPECIAL
   function that is not one (psx-spx CPU Specifications, opcode map;
   docs/hardware/cop0.md, CAUSE).
   Fault prevented: an undefined encoding that retires as a no-operation. */
static void reserved_instruction(void)
{
    schway *inst = make();
    run_word(inst, CODE, 0xFC000000u);
    CHECK_EQ_U64(exc_code(inst), 0x0Au);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_RESERVED_INSTRUCTION);
    CHECK_EQ_HEX(inst->fault.pc, CODE);
    schway_destroy(inst);
    inst = make();
    run_word(inst, CODE, 0x0000003Fu);
    CHECK_EQ_U64(exc_code(inst), 0x0Au);
    schway_destroy(inst);
}

/* A COP1 instruction with SR.CU1 clear is a coprocessor-unusable exception
   (ExcCode 0Bh) with CE = 1 in CAUSE bits 29-28, EPC the instruction, and a first
   fault, since the console has no COP1 (psx-spx CPU Specifications, exception
   handling; docs/hardware/cop0.md, Coprocessor unusable).
   Fault prevented: a missing coprocessor that executes, or a CE that names
   the wrong one. */
static void cop1_unusable(void)
{
    schway *inst = make();
    run_word(inst, CODE, COP1_WORD);
    CHECK_EQ_U64(exc_code(inst), 0x0Bu);
    CHECK_EQ_U64((cop0(inst, SCHWAY__COP0_CAUSE) >> 28) & 3u, 1u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_COP_UNUSABLE);
    schway_destroy(inst);
}

/* The common checks of SYSCALL and BREAK outside a delay slot: ExcCode 8 or 9,
   EPC the instruction itself, BD and BT clear, no first fault and no
   BadVaddr (psx-spx CPU Specifications, exception handling). */
static void trap_epc(uint32_t word, uint32_t code)
{
    schway *inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_BADVADDR] = 0x0BADC0DEu;
    run_word(inst, CODE, word);
    CHECK_EQ_U64(exc_code(inst), code);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_CAUSE) & 0xC0000000u, 0u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_BADVADDR), 0x0BADC0DEu);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    schway_destroy(inst);
}

/* In a delay slot the exception returns to the branch: EPC is the branch, BD is
   set, BT says the branch was taken and TAR is where it was going (psx-spx CPU
   Specifications, exception handling; docs/hardware/cop0.md, Entry). A taken BEQ r0, r0 at CODE with a target of CODE + 10h, the trap in
   its slot. */
static void trap_bd(uint32_t word, uint32_t code)
{
    schway *inst = make();
    put(inst, CODE + 4u, word);
    run_word(inst, CODE, ITYPE(4u, 0u, 0u, 3u));
    next(inst);
    CHECK_EQ_U64(exc_code(inst), code);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_CAUSE) & 0xC0000000u, 0xC0000000u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_TAR), CODE + 0x10u);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
    schway_destroy(inst);
}

/* Fault prevented: an EPC for SYSCALL that skips it, so a handler returns
   past the call, or a BD or first fault left over. */
static void syscall_epc(void)
{
    trap_epc(SYSCALL_WORD, 8u);
}

/* Fault prevented: a SYSCALL in a delay slot whose EPC names the slot, so the
   return re-executes only the slot and the branch is lost. */
static void syscall_bd(void)
{
    trap_bd(SYSCALL_WORD, 8u);
}

/* Fault prevented: as for SYSCALL, with ExcCode 9. */
static void break_epc(void)
{
    trap_epc(BREAK_WORD, 9u);
}

static void break_bd(void)
{
    trap_bd(BREAK_WORD, 9u);
}

/* With the line asserted, an interrupt is taken only while SR.IEc and the
   interrupt mask bit of the line, SR bit 10, are both set, and I_MASK lets I_STAT
   through to CAUSE bit 10. With one of the three clear nothing is armed, however
   many instructions run, and a read of CAUSE or SR never arms one. The level
   stays visible in CAUSE bit 10 while SR holds it off (docs/hardware/cop0.md, SR,
   CAUSE and Interrupts).
   Fault prevented: an interrupt taken in spite of IEc, IM2 or I_MASK, or one
   armed by looking at a register. */
static void irq_masked_sr(void)
{
    static const uint32_t sr[] = {SR_IM2, SR_IEC, 0u};
    for (uint32_t k = 0u; k < 4u; k++) {
        schway *inst = make();
        inst->cpu.cop0.r[SCHWAY__COP0_SR] = k < 3u ? sr[k] : (SR_IEC | SR_IM2);
        if (k < 3u) {
            enable_vblank(inst);
        }
        put(inst, CODE, MFC0_WORD(1u, SCHWAY__COP0_CAUSE));
        put(inst, CODE + 4u, MFC0_WORD(2u, SCHWAY__COP0_SR));
        put(inst, CODE + 8u, NOP_WORD);
        put(inst, CODE + 12u, NOP_WORD);
        schway__irq_raise(inst, 0u);
        CHECK_EQ_U64(inst->cpu.irq_stamp, 0u);
        /* The last round leaves I_MASK clear: no line at all. */
        CHECK_EQ_U64((cop0(inst, SCHWAY__COP0_CAUSE) >> 10) & 1u, k < 3u ? 1u : 0u);
        step(inst, CODE);
        for (uint32_t i = 1u; i < 4u; i++) {
            next(inst);
        }
        CHECK_EQ_HEX(inst->cpu.pc, CODE + 16u);
        CHECK_EQ_U64(exc_code(inst), 0u);
        CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), 0u);
        CHECK_EQ_U64(inst->cpu.irq_stamp, 0u);
        schway_destroy(inst);
    }
}

/* SR.BEV picks the general vector, for an exception and for an interrupt alike:
   BFC00180h while it is set and 80000080h while it is clear (psx-spx CPU
   Specifications, exception handling; docs/hardware/cop0.md, Entry).
   Fault prevented: a handler that is never entered because the vector ignores
   BEV, for the interrupt path as for the instruction path. */
static void bev_vectors(void)
{
    for (uint32_t bev = 0u; bev < 2u; bev++) {
        uint32_t want = bev != 0u ? VECTOR_BEV : VECTOR_RAM;
        schway *inst = make();
        inst->cpu.cop0.r[SCHWAY__COP0_SR] = bev != 0u ? SR_BEV : 0u;
        run_word(inst, CODE, SYSCALL_WORD);
        CHECK_EQ_HEX(inst->cpu.pc, want);
        schway_destroy(inst);

        inst = make();
        inst->cpu.cop0.r[SCHWAY__COP0_SR] = (bev != 0u ? SR_BEV : 0u) | SR_IEC | SR_IM2;
        enable_vblank(inst);
        schway__irq_raise(inst, 0u);
        step(inst, CODE);
        next(inst);
        CHECK_EQ_HEX(inst->cpu.pc, want);
        CHECK_EQ_U64(exc_code(inst), 0u);
        schway_destroy(inst);
    }
}

/* A load from an address no device answers is a data bus error (DBE, ExcCode 7)
   with EPC the instruction, no BadVaddr, no load pending and rt untouched, and
   the first fault names it. The expansion region 1 is unused here (psx-spx
   Memory Map; docs/hardware/cpu.md, Loads and stores).
   Fault prevented: a read from nowhere that returns zero without a trap, or one
   that writes BadVaddr, which only address errors do. */
static void dbe_unmapped_read(void)
{
    schway *inst = make();
    inst->cpu.r[1] = UNMAPPED_KSEG1;
    inst->cpu.r[2] = 0x1234u;
    inst->cpu.cop0.r[SCHWAY__COP0_BADVADDR] = 0x0BADC0DEu;
    run_word(inst, CODE, LW_WORD(2u, 0u, 1u));
    CHECK_EQ_U64(exc_code(inst), 7u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_BADVADDR), 0x0BADC0DEu);
    CHECK_EQ_HEX(inst->cpu.r[2], 0x1234u);
    CHECK_EQ_U64(inst->cpu.load_reg, SCHWAY__CPU_NO_LOAD);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_READ);
    schway_destroy(inst);
}

/* An instruction fetch from an address no device answers is an instruction bus
   error (IBE, ExcCode 6): EPC is that address, there is no BadVaddr, and the
   first fault names the fetch (docs/hardware/cpu.md, Instruction fetch).
   Fault prevented: a fetch from nowhere that executes zeros, or reports a data
   error. */
static void ibe_unmapped_fetch(void)
{
    schway *inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_BADVADDR] = 0x0BADC0DEu;
    step(inst, UNMAPPED_KSEG1);
    CHECK_EQ_U64(exc_code(inst), 6u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), UNMAPPED_KSEG1);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_BADVADDR), 0x0BADC0DEu);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_FETCH);
    schway_destroy(inst);
}

/* When an interrupt and an exception of the instruction itself fall on the same
   instruction the order is data AdEL, AdES and DBE, then overflow, then the
   interrupt, then SYSCALL, BREAK, RI and CpU, then the fetch's AdEL and IBE
   (docs/hardware/cop0.md, Interrupts). The interrupt is due on the instruction at
   CODE + 4 here (the stamp is 1 as it starts). Below it the interrupt is taken
   at that instruction, which runs again after the return; above it the
   exception is taken, the entry clears IEc so the interrupt is not taken yet,
   and the line stays true in CAUSE for the return from exception to see.
   Fault prevented: an interrupt that hides a data error or an overflow, or one
   that a SYSCALL, a reserved instruction or a fetch error hides. */
static void priority(void)
{
    /* Ranked below the interrupt: INT wins, EPC is the instruction. */
    static const uint32_t below[] = {SYSCALL_WORD, BREAK_WORD, 0xFC000000u, COP1_WORD};
    for (size_t i = 0u; i < sizeof below / sizeof below[0]; i++) {
        schway *inst = due();
        run_word(inst, CODE + 4u, below[i]);
        CHECK_EQ_U64(exc_code(inst), 0u);
        CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE + 4u);
        CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
        schway_destroy(inst);
    }
    /* A fetch error and a misaligned fetch rank below it too. */
    schway *inst = due();
    step(inst, UNMAPPED_KSEG1);
    CHECK_EQ_U64(exc_code(inst), 0u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), UNMAPPED_KSEG1);
    schway_destroy(inst);
    inst = due();
    step(inst, CODE + 2u);
    CHECK_EQ_U64(exc_code(inst), 0u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE + 2u);
    schway_destroy(inst);

    /* Ranked above it: the exception wins and the interrupt waits. */
    static const struct {
        uint32_t word, code, r1;
    } above[] = {
        {ADD_WORD(3u, 1u, 1u), 0x0Cu, 0x7FFFFFFFu},
        {LW_WORD(2u, 0u, 1u), 4u, DATA + 2u},
        {SW_WORD(2u, 0u, 1u), 5u, DATA + 1u},
        {LW_WORD(2u, 0u, 1u), 7u, UNMAPPED_KSEG1},
    };
    for (size_t i = 0u; i < sizeof above / sizeof above[0]; i++) {
        inst = due();
        inst->cpu.r[1] = above[i].r1;
        run_word(inst, CODE + 4u, above[i].word);
        CHECK_EQ_U64(exc_code(inst), above[i].code);
        CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE + 4u);
        CHECK_EQ_U64(cop0(inst, SCHWAY__COP0_SR) & SR_IEC, 0u);
        CHECK_EQ_U64(inst->cpu.irq_stamp, 0u);
        CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_CAUSE) & 0x400u, 0x400u);
        schway_destroy(inst);
    }
}

/* An interrupt taken with stores queued: three stores (the first before the
   edge, two after it, the second over the same word as the first of those) are
   in the write queue when the interrupt is entered. The queue is kept, RAM holds
   the stores in store order (the later store over the same word wins), and the
   handler's first uncached read finds that data and empties the queue
   (docs/hardware/bus.md, Instruction cache and Write queue).
   Fault prevented: an interrupt entry that drops or reorders queued stores, or
   one that leaves them queued behind a handler's uncached read. */
static void irq_with_queued_stores(void)
{
    schway *inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = SR_IEC | SR_IM2;
    enable_vblank(inst);
    inst->cpu.r[1] = DATA;
    inst->cpu.r[2] = 0x11u;
    inst->cpu.r[3] = 0x22u;
    inst->cpu.r[4] = 0x33u;
    inst->cpu.r[5] = 0xA0000000u | DATA;
    put(inst, CODE, SW_WORD(4u, 8u, 1u));
    put(inst, CODE + 4u, SW_WORD(2u, 0u, 1u));
    put(inst, CODE + 8u, SW_WORD(3u, 0u, 1u));
    put(inst, CODE + 12u, NOP_WORD);
    put(inst, VECTOR_RAM, LW_WORD(6u, 0u, 5u));
    step(inst, CODE);
    schway__irq_raise(inst, 0u);
    next(inst);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE + 12u);
    CHECK_EQ_U64(exc_code(inst), 0u);
    CHECK_EQ_U64(inst->cpu.wq_count, 3u);
    next(inst);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.r[6], 0x22u);
    CHECK_EQ_U64(inst->cpu.wq_count, 0u);
    inst->cpu.r[1] = 0u;
    put(inst, VECTOR_RAM + 8u, LW_WORD(7u, 8u, 5u));
    step(inst, VECTOR_RAM + 8u);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.r[7], 0x33u);
    schway_destroy(inst);
}

/* RTPS, the command word used below, keeps the engine busy 15 cycles. */
#define RTPS_BUSY 15u

/* The interrupt entry hands the interrupted instruction to the one hook for the
   GTE, and a command there executes before the handler's first instruction and
   again when the return makes it run. An interrupt due before a command: the
   entry sets the engine's stamp 15 cycles after the entry's own cycle, the
   handler runs, and after the return the command's own issue sets it again, from
   the cycle it issues on, with the wait for the entry's command in front of it
   (docs/hardware/cop0.md, Interrupts). With SR.CU2 clear
   the instruction would raise CpU, so the entry executes nothing.
   Fault prevented: an entry that does not run the pending command, one that runs
   it but loses the second run after the return, or one that runs a command the
   CPU could not issue. */
static void irq_gte_double_execute(void)
{
    schway *inst = make();
    uint64_t at_entry, before, wait;
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = SR_CU2 | SR_IEC | SR_IM2;
    enable_vblank(inst);
    put(inst, CODE, NOP_WORD);
    put(inst, CODE + 4u, NOP_WORD);
    put(inst, CODE + 8u, COP2_COMMAND);
    put(inst, VECTOR_RAM, MFC0_WORD(26u, SCHWAY__COP0_EPC));
    put(inst, VECTOR_RAM + 4u, NOP_WORD);
    put(inst, VECTOR_RAM + 8u, JR_WORD(26u));
    put(inst, VECTOR_RAM + 12u, RFE_WORD);
    CHECK_EQ_U64(inst->gte.ready, 0u);
    schway__irq_raise(inst, 0u);
    step(inst, CODE);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE + 8u);
    CHECK_EQ_U64(exc_code(inst), 0u);
    /* The command ran at the entry: the stamp is set, before any handler code. */
    CHECK_EQ_U64(inst->gte.ready, inst->cpu.cycle + RTPS_BUSY);
    at_entry = inst->gte.ready;
    CHECK_EQ_U64(inst->bus.unbuilt_n[SCHWAY__DEV_GTE], 0u);
    /* The handler acknowledges the vertical blank and returns to EPC. */
    schway__irq_write_stat(inst, 0u, 0xFFFFFFFFu);
    for (uint32_t i = 0u; i < 4u; i++) {
        next(inst);
    }
    CHECK_EQ_HEX(inst->cpu.pc, CODE + 8u);
    CHECK_EQ_U64(inst->gte.ready, at_entry);
    /* It runs again now, behind what is left of the entry's. */
    before = inst->cpu.cycle;
    wait = at_entry > before ? at_entry - before : 0u;
    next(inst);
    CHECK_EQ_HEX(inst->cpu.pc, CODE + 12u);
    CHECK_EQ_U64(inst->gte.ready, before + wait + RTPS_BUSY);
    schway_destroy(inst);

    /* With CU2 clear the entry runs nothing. */
    inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = SR_IEC | SR_IM2;
    enable_vblank(inst);
    put(inst, CODE, NOP_WORD);
    put(inst, CODE + 4u, NOP_WORD);
    put(inst, CODE + 8u, COP2_COMMAND);
    schway__irq_raise(inst, 0u);
    step(inst, CODE);
    next(inst);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE + 8u);
    CHECK_EQ_U64(inst->gte.ready, 0u);
    schway_destroy(inst);

    /* The hook itself, called directly: a word that is not a command runs
       nothing. A command word runs only with ExcCode 0. */
    inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = SR_CU2;
    CHECK_EQ_U64(schway__cop0_irq_entry_cop2(inst, COP2_MFC2), 0u);
    CHECK_EQ_U64(schway__cop0_irq_entry_cop2(inst, NOP_WORD), 0u);
    CHECK_EQ_U64(schway__cop0_irq_entry_cop2(inst, 0x4A000000u ^ 0x02000000u), 0u);
    CHECK_EQ_U64(inst->gte.ready, 0u);
    inst->cpu.cop0.r[SCHWAY__COP0_CAUSE] = 8u << 2;
    CHECK_EQ_U64(schway__cop0_irq_entry_cop2(inst, COP2_COMMAND), 0u);
    CHECK_EQ_U64(inst->gte.ready, 0u);
    inst->cpu.cop0.r[SCHWAY__COP0_CAUSE] = 0u;
    CHECK_EQ_U64(schway__cop0_irq_entry_cop2(inst, COP2_COMMAND), 1u);
    CHECK_EQ_U64(inst->gte.ready, inst->cpu.cycle + RTPS_BUSY);
    schway_destroy(inst);
}

/* An interrupt taken when the next instruction is the delay slot of a branch
   returns to the branch, and it is the slot's instruction that is pending: a GTE
   command in the slot executes at the entry, EPC naming the branch with BD set,
   and again after the return when the slot runs. A word that is not a command in
   the slot executes nothing, whatever the branch is.
   Fault prevented: a hook handed the word at EPC, which is the branch, so a
   command in a slot is never run at the entry or a branch is read as the
   command. */
static void irq_gte_delay_slot(void)
{
    schway *inst = make();
    uint64_t at_entry, before, wait;
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = SR_CU2 | SR_IEC | SR_IM2;
    enable_vblank(inst);
    put(inst, CODE, NOP_WORD);
    put(inst, CODE + 4u, ITYPE(4u, 0u, 0u, 4u)); /* BEQ r0, r0, +4: always taken */
    put(inst, CODE + 8u, COP2_COMMAND);
    put(inst, VECTOR_RAM, MFC0_WORD(26u, SCHWAY__COP0_EPC));
    put(inst, VECTOR_RAM + 4u, NOP_WORD);
    put(inst, VECTOR_RAM + 8u, JR_WORD(26u));
    put(inst, VECTOR_RAM + 12u, RFE_WORD);
    schway__irq_raise(inst, 0u);
    step(inst, CODE);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE + 4u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_CAUSE) & 0x80000000u, 0x80000000u);
    CHECK_EQ_U64(inst->gte.ready, inst->cpu.cycle + RTPS_BUSY);
    at_entry = inst->gte.ready;
    /* Return to the branch: it runs, then its slot's command runs again. */
    schway__irq_write_stat(inst, 0u, 0xFFFFFFFFu);
    for (uint32_t i = 0u; i < 4u; i++) {
        next(inst);
    }
    CHECK_EQ_HEX(inst->cpu.pc, CODE + 4u);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.pc, CODE + 8u);
    CHECK_EQ_U64(inst->gte.ready, at_entry);
    before = inst->cpu.cycle;
    wait = at_entry > before ? at_entry - before : 0u;
    next(inst);
    CHECK_EQ_U64(inst->gte.ready, before + wait + RTPS_BUSY);
    schway_destroy(inst);

    /* A plain word in the slot: nothing executes at the entry. */
    inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = SR_CU2 | SR_IEC | SR_IM2;
    enable_vblank(inst);
    put(inst, CODE, NOP_WORD);
    put(inst, CODE + 4u, ITYPE(4u, 0u, 0u, 4u));
    put(inst, CODE + 8u, NOP_WORD);
    schway__irq_raise(inst, 0u);
    step(inst, CODE);
    next(inst);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE + 4u);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
    CHECK_EQ_U64(inst->gte.ready, 0u);
    schway_destroy(inst);
}

/* The entry reads the pending word the way the CPU would fetch it and leaves no
   trace of the read: a command held in the instruction cache runs even when the
   RAM behind it has been overwritten with a no-operation, a command in uncached
   RAM runs with the cache empty, and neither read counts an unbuilt access,
   moves the last-access record or latches a fault.
   Fault prevented: an entry that reads the bus, so it sees the new RAM word where
   the CPU would run the cached one, or that records the peek as a first fault or
   a device access. */
static void irq_entry_peek(void)
{
    static const uint32_t bases[] = {CODE, 0xA0001000u};
    for (size_t i = 0u; i < sizeof bases / sizeof bases[0]; i++) {
        uint32_t base = bases[i];
        schway *inst = make();
        uint32_t unbuilt_n[SCHWAY__DEV_COUNT];
        uint32_t last[4];
        struct schway__fault fault;
        uint32_t cycles = 0u;
        inst->cpu.cop0.r[SCHWAY__COP0_SR] = SR_CU2 | SR_IEC | SR_IM2;
        enable_vblank(inst);
        put(inst, base, NOP_WORD);
        put(inst, base + 4u, NOP_WORD);
        put(inst, base + 8u, COP2_COMMAND);
        put(inst, base + 12u, NOP_WORD);
        if (i == 0u) {
            /* Fill the line, then change the RAM under it, behind the cache. */
            for (uint32_t k = 0u; k < 4u; k++) {
                uint32_t word = 0u;
                CHECK_EQ_U64(schway__cpu_icache_fetch(inst, base + 4u * k, &word, &cycles), 0u);
            }
            CHECK_EQ_U64(schway__bus_write(inst, 0xA0000000u | (base & 0x1FFFFFFFu) | 8u, 4u,
                                           NOP_WORD, &cycles),
                         0u);
        }
        schway__bus_unbuilt(inst, SCHWAY__DEV_SPU, 0x1F801C00u, 4u, 0u, 0u);
        memcpy(unbuilt_n, inst->bus.unbuilt_n, sizeof unbuilt_n);
        memcpy(last, inst->bus.last_unbuilt, sizeof last);
        fault = inst->fault;
        schway__irq_raise(inst, 0u);
        step(inst, base);
        next(inst);
        CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
        CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), base + 8u);
        CHECK_EQ_U64(inst->gte.ready, inst->cpu.cycle + RTPS_BUSY);
        CHECK_EQ_U64(memcmp(unbuilt_n, inst->bus.unbuilt_n, sizeof unbuilt_n), 0);
        CHECK_EQ_U64(memcmp(last, inst->bus.last_unbuilt, sizeof last), 0);
        CHECK_EQ_U64(memcmp(&fault, &inst->fault, sizeof fault), 0);
        schway_destroy(inst);
    }
}

/* The latencies below are an inference, not a measurement: the published
   behaviour is that a newly asserted interrupt is delayed by about two
   instructions, and nothing measured on silicon separates a delay of one from
   two for the sources below (docs/hardware/cop0.md, Interrupts). Each is pinned
   as the documented value, and a change to one needs a dated line in
   tests/diag/allow.txt that names the evidence for it.

   MTC0 to CAUSE that raises a software interrupt bit the mask lets through is
   taken one retirement later, after the MTC0 itself, so EPC is the address after
   it: pc + 4.
   Fault prevented: a CAUSE write whose interrupt is taken at the write, or two
   instructions on. */
static void doc_cause_write(void)
{
    schway *inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = SR_IEC | SR_IM0;
    inst->cpu.r[1] = 0x100u;
    run_word(inst, CODE, MTC0_WORD(1u, SCHWAY__COP0_CAUSE));
    CHECK_EQ_U64(exc_code(inst), 0u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE + 4u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_CAUSE) & 0xC0000100u, 0x100u);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
    schway_destroy(inst);
}

/* The latency here is an inference, unmeasured on silicon (docs/hardware/cop0.md,
   Interrupts); a change needs a dated line in tests/diag/allow.txt naming its
   evidence. MTC0 to SR that enables a line that is already pending is taken two
   retirements later, the MTC0 and the instruction after it, so EPC is pc + 8.
   Fault prevented: an SR write whose pending interrupt is taken at once, or
   never. */
static void doc_sr_write(void)
{
    schway *inst = make();
    enable_vblank(inst);
    schway__irq_raise(inst, 0u);
    CHECK_EQ_U64(inst->cpu.irq_stamp, 0u);
    inst->cpu.r[1] = SR_IEC | SR_IM2;
    put(inst, CODE + 4u, NOP_WORD);
    run_word(inst, CODE, MTC0_WORD(1u, SCHWAY__COP0_SR));
    CHECK_EQ_U64(inst->cpu.irq_stamp, 1u);
    CHECK_EQ_U64(exc_code(inst), 0u);
    CHECK_EQ_HEX(inst->cpu.pc, CODE + 4u);
    next(inst);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE + 8u);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
    schway_destroy(inst);
}

/* The latency here is an inference, unmeasured on silicon (docs/hardware/cop0.md,
   Interrupts); a change needs a dated line in tests/diag/allow.txt naming its
   evidence. A store to I_MASK that lets a latched source through is taken two
   retirements later, the store and the instruction after it, so EPC is pc + 8.
   Fault prevented: an I_MASK write whose interrupt is taken at the store, or
   ignored. */
static void doc_imask_write(void)
{
    schway *inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = SR_IEC | SR_IM2;
    schway__irq_raise(inst, 0u);
    CHECK_EQ_U64(inst->cpu.irq_stamp, 0u);
    inst->cpu.r[1] = 1u;
    inst->cpu.r[2] = I_MASK_KSEG1;
    put(inst, CODE + 4u, NOP_WORD);
    run_word(inst, CODE, SW_WORD(1u, 0u, 2u));
    CHECK_EQ_U64(inst->irq.mask, 1u);
    CHECK_EQ_U64(inst->cpu.irq_stamp, 1u);
    CHECK_EQ_U64(exc_code(inst), 0u);
    next(inst);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE + 8u);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
    schway_destroy(inst);
}

/* The latency here is an inference, unmeasured on silicon (docs/hardware/cop0.md,
   Interrupts); a change needs a dated line in tests/diag/allow.txt naming its
   evidence. A CAUSE write in the delay slot of a taken branch is taken after the
   slot, with the branch gone: EPC is the branch's destination and BD is 0 (the
   entry never splits a branch from its slot by naming the slot).
   Fault prevented: an interrupt that returns into the middle of a branch, or
   one that sets BD for an instruction that is not a delay slot any more. */
static void doc_cause_in_delay_slot(void)
{
    schway *inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = SR_IEC | SR_IM0;
    inst->cpu.r[1] = 0x100u;
    put(inst, CODE + 4u, MTC0_WORD(1u, SCHWAY__COP0_CAUSE));
    run_word(inst, CODE, ITYPE(4u, 0u, 0u, 3u)); /* BEQ r0, r0, +3: to CODE + 10h */
    next(inst);
    CHECK_EQ_U64(exc_code(inst), 0u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), CODE + 0x10u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_CAUSE) & 0xC0000000u, 0u);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
    schway_destroy(inst);
}

/* RFE looks at the line again at once: a handler that returns without
   acknowledging the source is entered again after the RFE, not after two more
   instructions, and EPC is the instruction after the RFE (docs/hardware/cop0.md,
   Interrupts and SR).
   Fault prevented: a return from exception that runs the interrupted program
   for a while with an interrupt it should take, or never takes it. */
static void irq_rfe(void)
{
    schway *inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = SR_IEC | SR_IM2;
    enable_vblank(inst);
    schway__irq_raise(inst, 0u);
    step(inst, CODE);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
    CHECK_EQ_U64(cop0(inst, SCHWAY__COP0_SR) & SR_IEC, 0u);
    run_word(inst, VECTOR_RAM, RFE_WORD);
    CHECK_EQ_U64(exc_code(inst), 0u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), VECTOR_RAM + 4u);
    CHECK_EQ_HEX(inst->cpu.pc, VECTOR_RAM);
    schway_destroy(inst);
}

/* An armed interrupt is taken only if the line is still true when it is due:
   acknowledging the source in between takes it back (docs/hardware/cop0.md, Interrupts).
   Fault prevented: an interrupt taken for a source the handler or the program
   had already cleared. */
static void irq_dropped_before_entry(void)
{
    schway *inst = make();
    inst->cpu.cop0.r[SCHWAY__COP0_SR] = SR_IEC | SR_IM2;
    enable_vblank(inst);
    for (uint32_t i = 0u; i < 4u; i++) {
        put(inst, CODE + 4u * i, NOP_WORD);
    }
    schway__irq_raise(inst, 0u);
    step(inst, CODE);
    CHECK_EQ_U64(inst->cpu.irq_stamp, 1u);
    schway__irq_write_stat(inst, 0u, 0xFFFFFFFFu);
    CHECK_EQ_U64(inst->cpu.irq_stamp, 0u);
    next(inst);
    next(inst);
    CHECK_EQ_HEX(inst->cpu.pc, CODE + 12u);
    CHECK_EQ_U64(exc_code(inst), 0u);
    CHECK_EQ_HEX(cop0(inst, SCHWAY__COP0_EPC), 0u);
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    static const struct {
        const char *name;
        void (*fn)(void);
    } cases[] = {
        {"irq_latency_device", irq_latency_device},
        {"adel_lh", adel_lh},
        {"adel_lw", adel_lw},
        {"ades_sw", ades_sw},
        {"overflow_add", overflow_add},
        {"reserved_instruction", reserved_instruction},
        {"cop1_unusable", cop1_unusable},
        {"syscall_epc", syscall_epc},
        {"syscall_bd", syscall_bd},
        {"break_epc", break_epc},
        {"break_bd", break_bd},
        {"irq_masked_sr", irq_masked_sr},
        {"bev_vectors", bev_vectors},
        {"dbe_unmapped_read", dbe_unmapped_read},
        {"ibe_unmapped_fetch", ibe_unmapped_fetch},
        {"priority", priority},
        {"irq_with_queued_stores", irq_with_queued_stores},
        {"irq_gte_double_execute", irq_gte_double_execute},
        {"irq_gte_delay_slot", irq_gte_delay_slot},
        {"irq_entry_peek", irq_entry_peek},
        {"doc_cause_write", doc_cause_write},
        {"doc_sr_write", doc_sr_write},
        {"doc_imask_write", doc_imask_write},
        {"doc_cause_in_delay_slot", doc_cause_in_delay_slot},
        {"irq_rfe", irq_rfe},
        {"irq_dropped_before_entry", irq_dropped_before_entry},
    };
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-exception <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            cases[i].fn();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "schwaystation-test-exception: unknown case %s\n", argv[1]);
    return 2;
}
