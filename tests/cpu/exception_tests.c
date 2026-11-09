/* Exceptions and interrupts through the CPU's one-instruction entry:
   schwaystation-test-exception <case>. White-box: the test sees src/, places
   words in RAM, runs them with schway__cpu_step_one and no replay log, and reads
   the registers, the delay latches, the fault record and the retirement stamp.
   Expected values are worked by hand from the psx-spx CPU Specifications page
   (COP0 registers, exception handling) and from sections 2 and 6 of
   .planning/preparation/PSX-SYSTEM.md. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

#define NOP_WORD 0u

/* SR: IEc, the interrupt mask bit of the hardware line (CAUSE bit 10), and the
   exception vectors. */
#define SR_IEC 0x00000001u
#define SR_IM2 0x00000400u
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
   (PSX-SYSTEM section 6; interrupts return to EPC + 0, section 2, EPC). CAUSE
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

int main(int argc, char **argv)
{
    static const struct {
        const char *name;
        void (*fn)(void);
    } cases[] = {
        {"irq_latency_device", irq_latency_device},
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
