/* The executor: fetch, decode and execute one instruction. cpu.c includes it
   twice (see the second part below), so the one-instruction entry and the run
   loop run the same code. The CPU is a MIPS I R3000A (PSX-SYSTEM section 2).
   Integer only. */

#if SCHWAY_TRACE == 0
/* Shared by both copies of the loop: nothing here writes a trace record, so it is
   compiled once, with the untraced copy. */

/* One access of the replay bus. The access is recorded, and a read or fetch
   returns the value the expected list holds at the same position (zero past
   its end, which the caller's comparison reports as a missing cycle). */
static uint32_t cpu_log_access(struct schway__cpu_log *log, uint32_t kind, uint32_t size,
                               uint32_t addr, uint32_t value)
{
    uint32_t k = log->got_n + log->overflow;
    if (kind != SCHWAY__CYCLE_WRITE) {
        value = k < log->expect_n ? log->expect[k].value : 0u;
    }
    if (log->got_n < sizeof log->got / sizeof log->got[0]) {
        struct schway__cpu_cycle *g = &log->got[log->got_n++];
        g->kind = kind;
        g->size = size;
        g->addr = addr;
        g->value = value;
    } else {
        log->overflow++;
    }
    return value;
}

/* A KSEG1 access, data or instruction, waits for the write queue to empty
   first, so uncached memory and devices see the stores in order: each store
   still queued is charged its drain, and an empty queue adds nothing (PSX-SYSTEM
   section 2, Cache isolation, write queue). */
static void cpu_wq_drain(struct schway__cpu *c, uint32_t *cycles)
{
    *cycles += c->wq_count * SCHWAY__WQ_DRAIN_CYCLES;
    c->wq_count = 0u;
}

/* A store that reached the bus joins the write queue, unless it went to the
   scratchpad, which is not behind the queue. A fifth store waits for the oldest
   to leave, which costs one drain, and the queue stays full. The data is
   already on the bus, so only the wait is modelled. */
static void cpu_wq_push(struct schway__cpu *c, uint32_t addr, uint32_t *cycles)
{
    if ((addr & 0x1FFFFC00u) == 0x1F800000u) {
        return;
    }
    if (c->wq_count >= SCHWAY__WQ_DEPTH) {
        *cycles += SCHWAY__WQ_DRAIN_CYCLES;
    } else {
        c->wq_count++;
    }
}

/* The instruction fetch front-end. With a log attached it replays it, with no
   cache and no queue. Without one, a fetch in KUSEG or KSEG0 goes through the
   instruction cache, except in the BIOS window, which is never cached; a fetch
   in KSEG1 drains the write queue and reaches the bus. The bus decodes the
   segment of the virtual address (PSX-SYSTEM section 2, Segments and I-cache;
   section 3, C4), and its charge adds to *cycles. A bus error sets *err and the
   word reads as zero; the bus has already latched it as the first fault. */
static uint32_t cpu_fetch(schway *inst, uint32_t vaddr, uint32_t *cycles, uint32_t *err)
{
    uint32_t word = 0u;
    uint32_t seg = schway__bus_segment(vaddr);
    if (inst->cpu.log != NULL) {
        return cpu_log_access(inst->cpu.log, SCHWAY__CYCLE_FETCH, 4u, vaddr, 0u);
    }
    if ((seg == (uint32_t)SCHWAY__SEG_KUSEG || seg == (uint32_t)SCHWAY__SEG_KSEG0) &&
        ((vaddr & 0x1FFFFFFFu) >> 19) != 0x3F8u) {
        *err = schway__cpu_icache_fetch(inst, vaddr, &word, cycles);
        return word;
    }
    if (seg == (uint32_t)SCHWAY__SEG_KSEG1) {
        cpu_wq_drain(&inst->cpu, cycles);
    }
    *err = schway__bus_fetch(inst, vaddr, &word, cycles) != 0 ? 1u : 0u;
    return word;
}

/* Writes general register n. A write to register 0 is discarded. A load
   still pending for the same register is cancelled: the later write wins
   (PSX-SYSTEM section 2, Load delay). */
static void cpu_set(struct schway__cpu *c, uint32_t n, uint32_t v)
{
    if (c->load_reg == n) {
        c->load_reg = SCHWAY__CPU_NO_LOAD;
        c->load_value = 0u;
    }
    if (n != 0u) {
        c->r[n] = v;
    }
}

/* Arithmetic shift right and signed compare, written on unsigned values so
   no implementation-defined conversion is involved. */
static uint32_t cpu_sra(uint32_t x, uint32_t sa)
{
    return (x & 0x80000000u) != 0u ? ~(~x >> sa) : x >> sa;
}

static uint32_t cpu_slt(uint32_t a, uint32_t b)
{
    return (a ^ 0x80000000u) < (b ^ 0x80000000u) ? 1u : 0u;
}

/* A branch as the instruction after it needs it: slot says the next
   instruction is a delay slot, take that the branch goes, target where. */
struct cpu_branch {
    uint32_t slot, take, target;
};

static void cpu_branch_set(struct cpu_branch *b, uint32_t take, uint32_t target)
{
    b->slot = 1u;
    b->take = take != 0u ? 1u : 0u;
    b->target = target;
}

/* Whether an exception of this code, raised by the instruction at pc, ranks
   below an interrupt that is due on the same instruction. The order is data
   AdEL, AdES and DBE, overflow, interrupt, then SYSCALL, BREAK, RI and CpU,
   then the fetch's AdEL and IBE (PSX-SYSTEM section 2, Priority). A data AdEL
   has an aligned pc; the fetch's has an odd one. */
static uint32_t cpu_below_interrupt(uint32_t code, uint32_t pc)
{
    return code == (uint32_t)SCHWAY__EXC_SYS || code == (uint32_t)SCHWAY__EXC_BP ||
                   code == (uint32_t)SCHWAY__EXC_RI || code == (uint32_t)SCHWAY__EXC_CPU ||
                   code == (uint32_t)SCHWAY__EXC_IBE ||
                   (code == (uint32_t)SCHWAY__EXC_ADEL && (pc & 3u) != 0u)
               ? 1u
               : 0u;
}

/* With SR.Isc set, a data access in KUSEG, KSEG0 or KSEG1 goes to the
   instruction cache and never reaches the bus; KSEG2, the cache-control word,
   still does. The line is the one address bits 11:4 name, the word bits 3:2
   (PSX-SYSTEM section 2, Cache isolation, write queue). A load returns
   that word's bytes whether or not the tag matches. A store writes the tag, the
   physical address bits 31:12 of the datum, and sets the four valid bits to the
   datum's low four bits when cache-control TAG (bit 2) and IS1 (bit 11) are both
   set, and otherwise writes the bytes it covers into the word; that is how the
   BIOS' FlushCache empties the cache. The bits of rt above the access width go
   out too, as in any store, which is why a tag store reads the whole datum. */
static uint32_t cpu_isolated(const schway *inst, uint32_t addr)
{
    return (inst->cpu.cop0.r[SCHWAY__COP0_SR] & SCHWAY__SR_ISC) != 0u &&
                   schway__bus_segment(addr) != (uint32_t)SCHWAY__SEG_KSEG2
               ? 1u
               : 0u;
}

static uint32_t cpu_isolated_read(schway *inst, uint32_t addr, uint32_t size)
{
    uint32_t w = inst->cpu.icache_word[(addr >> 4) & 0xFFu][(addr >> 2) & 3u];
    w >>= 8u * (addr & 3u);
    return size == 4u ? w : w & ((1u << (8u * size)) - 1u);
}

static void cpu_isolated_write(schway *inst, uint32_t addr, uint32_t size, uint32_t value)
{
    struct schway__cpu *c = &inst->cpu;
    uint32_t line = (addr >> 4) & 0xFFu;
    uint32_t ctrl = inst->bus.cache_ctrl;
    if ((ctrl & 0x4u) != 0u && (ctrl & 0x800u) != 0u) {
        c->icache_tag[line] = value >> 12;
        c->icache_valid[line] = (uint8_t)(value & 0xFu);
    } else {
        uint32_t shift = 8u * (addr & 3u);
        uint32_t mask = (size == 4u ? 0xFFFFFFFFu : (1u << (8u * size)) - 1u) << shift;
        uint32_t *w = &c->icache_word[line][(addr >> 2) & 3u];
        *w = (*w & ~mask) | ((value << shift) & mask);
    }
}

/* Issues a load: the register is written one instruction later. A load still
   pending for the same register is dropped, the later one winning (PSX-SYSTEM
   section 2, Load delay). */
static void cpu_load_issue(struct schway__cpu *c, uint32_t n, uint32_t v, uint32_t *load_reg,
                           uint32_t *load_value)
{
    if (c->load_reg == n) {
        c->load_reg = SCHWAY__CPU_NO_LOAD;
        c->load_value = 0u;
    }
    *load_reg = n;
    *load_value = v;
}

/* The effective address of a load or store: the base register plus the
   sign-extended immediate. */
static uint32_t cpu_address(const struct schway__cpu *c, uint32_t op)
{
    return c->r[(op >> 21) & 31u] + (((op & 0xFFFFu) ^ 0x8000u) - 0x8000u);
}

/* The cycles an early read of HI or LO waits: until the running multiply or
   divide is done, and never more than the longest one takes, whatever a loaded
   state says. The read issues at the cycle counter plus what the instruction
   spent before its own issue cycle (PSX-SYSTEM section 2, MULTU/MULT). */
static uint32_t cpu_muldiv_wait(const struct schway__cpu *c, uint32_t cycles)
{
    uint64_t at = c->cycle + (cycles - 1u);
    uint64_t wait = c->muldiv_ready > at ? c->muldiv_ready - at : 0u;
    return wait > SCHWAY__MULDIV_MAX ? SCHWAY__MULDIV_MAX : (uint32_t)wait;
}

/* Starts a multiply or divide that takes n cycles from the issue; one that is
   still running is replaced and the new one's time is all that counts. */
static void cpu_muldiv_start(struct schway__cpu *c, uint32_t cycles, uint32_t n)
{
    c->muldiv_ready = c->cycle + (cycles - 1u) + n;
}

/* MULT and MULTU: the 64-bit product into HI:LO, in 6, 9 or 13 cycles by how
   many bits the first operand needs. MULT classifies rs with its sign mirrored
   (rs XOR its sign bit spread over the word), MULTU on rs as it is. The
   product is built from magnitudes so no signed conversion is involved. */
static void cpu_mult(struct schway__cpu *c, uint32_t rs, uint32_t rt, uint32_t sign,
                     uint32_t cycles)
{
    uint32_t neg_rs = sign != 0u && (rs & 0x80000000u) != 0u ? 1u : 0u;
    uint32_t neg_rt = sign != 0u && (rt & 0x80000000u) != 0u ? 1u : 0u;
    uint32_t m = neg_rs != 0u ? ~rs : rs;
    uint64_t p = (uint64_t)(neg_rs != 0u ? 0u - rs : rs) * (neg_rt != 0u ? 0u - rt : rt);
    if (neg_rs != neg_rt) {
        p = 0u - p;
    }
    c->lo = (uint32_t)p;
    c->hi = (uint32_t)(p >> 32);
    cpu_muldiv_start(c, cycles, m < 0x800u ? 6u : (m < 0x100000u ? 9u : 13u));
}

/* DIV and DIVU, 36 cycles, no trap. A zero divisor gives LO = -1 (or +1 for
   DIV of a negative rs) and HI = rs; DIV of -80000000h by -1 gives LO =
   80000000h and HI = 0 (PSX-SYSTEM section 2, DIV/DIVU). */
static void cpu_div(struct schway__cpu *c, uint32_t rs, uint32_t rt, uint32_t sign,
                    uint32_t cycles)
{
    uint32_t neg_rs = sign != 0u && (rs & 0x80000000u) != 0u ? 1u : 0u;
    uint32_t neg_rt = sign != 0u && (rt & 0x80000000u) != 0u ? 1u : 0u;
    if (rt == 0u) {
        c->lo = neg_rs != 0u ? 1u : 0xFFFFFFFFu;
        c->hi = rs;
    } else if (sign != 0u && rs == 0x80000000u && rt == 0xFFFFFFFFu) {
        c->lo = 0x80000000u;
        c->hi = 0u;
    } else {
        uint32_t a = neg_rs != 0u ? 0u - rs : rs;
        uint32_t b = neg_rt != 0u ? 0u - rt : rt;
        uint32_t q = a / b;
        uint32_t r = a % b;
        c->lo = neg_rs != neg_rt ? 0u - q : q;
        c->hi = neg_rs != 0u ? 0u - r : r;
    }
    cpu_muldiv_start(c, cycles, SCHWAY__MULDIV_MAX);
}

/* Signed overflow of a + b = sum, and of a - b = diff, on unsigned words
   (MIPS I: ADD, ADDI and SUB trap and leave rd unchanged). */
static uint32_t cpu_add_overflows(uint32_t a, uint32_t b, uint32_t sum)
{
    return (~(a ^ b) & (a ^ sum) & 0x80000000u) != 0u ? 1u : 0u;
}

static uint32_t cpu_sub_overflows(uint32_t a, uint32_t b, uint32_t diff)
{
    return ((a ^ b) & (a ^ diff) & 0x80000000u) != 0u ? 1u : 0u;
}

#endif /* SCHWAY_TRACE == 0 */

/* The functions below reach a bus access or an exception entry, so they exist
   twice: this file is included with SCHWAY_TRACE 0 and then 1. In the traced
   copy every name gains _traced and TRACE(...) writes one record into the
   instance's ring; in the other TRACE is empty, so the untraced copy holds no
   trace code and no branch for one. schway__cpu_run picks the copy once per
   call, from the budget's trace flag (ARCHITECTURE section 6). */
#if SCHWAY_TRACE
#define cpu_irq_enter cpu_irq_enter_traced
#define cpu_exception cpu_exception_traced
#define cpu_reserved cpu_reserved_traced
#define cpu_unusable cpu_unusable_traced
#define cpu_data_check cpu_data_check_traced
#define cpu_bus_read cpu_bus_read_traced
#define cpu_bus_write cpu_bus_write_traced
#define cpu_load_int cpu_load_int_traced
#define cpu_load_part cpu_load_part_traced
#define cpu_store_int cpu_store_int_traced
#define cpu_store_part cpu_store_part_traced
#define cpu_cop2 cpu_cop2_traced
#define cpu_step cpu_step_traced
#define cpu_block cpu_block_traced
/* The cycle an access at this point of the instruction happens on: the
   instruction's start plus the cycles it has spent before the access. */
#define TRACE_BEGIN(at, spent) uint64_t at = trace_now(inst) + (uint64_t)((spent) - 1u)
#define TRACE(...) trace_push(inst, __VA_ARGS__)
/* A block's start, from which every record's cycle is counted. */
#define TRACE_START() (inst->trace.t0 = inst->ticks, inst->trace.c0 = inst->cpu.cycle)
/* The record of a fault latched in the block just run. */
#define TRACE_FAULT(latched, at)                                                                   \
    do {                                                                                           \
        if (latched) {                                                                             \
            const struct schway__fault *tf_ = &inst->fault;                                        \
            TRACE(SCHWAY_TRACE_FAULT, at, tf_->pc, tf_->opcode, tf_->addr, tf_->width,            \
                  tf_->device, tf_->kind, tf_->note);                                              \
        }                                                                                          \
    } while (0)
#else
#define TRACE_BEGIN(at, spent) ((void)0)
#define TRACE(...) ((void)0)
#define TRACE_START() ((void)0)
#define TRACE_FAULT(latched, at) ((void)0)
#endif

/* Takes the interrupt before the instruction at pc, ExcCode 0. The load issued
   by the instruction before lands first, so the handler sees it (PSX-SYSTEM
   section 2, Load delay). Interrupts return to EPC + 0: the instruction at pc,
   or the branch before it when pc is a delay slot, which the entry records as
   BD with TAR the branch's destination (PSX-SYSTEM section 2, EPC). The entry
   calls the one hook for the GTE with the word at EPC, read without charge and
   without a first fault; with a replay log attached there is no memory to read
   and the word is 0. The stores in the write queue stay queued: the first KSEG1
   access after the entry drains them. */
static void cpu_irq_enter(schway *inst, uint32_t pc)
{
    struct schway__cpu *c = &inst->cpu;
    uint32_t slot = c->branch_slot;
    uint32_t take = c->branch_take;
    uint32_t epc = slot != 0u ? pc - 4u : pc;
    uint32_t target = take != 0u ? c->branch_target : pc + 4u;
    uint32_t word = 0u;
    if (c->load_reg != SCHWAY__CPU_NO_LOAD) {
        if (c->load_reg != 0u) {
            c->r[c->load_reg] = c->load_value;
        }
        c->load_reg = SCHWAY__CPU_NO_LOAD;
        c->load_value = 0u;
    }
    if (c->log == NULL) {
        struct schway__fault held = inst->fault;
        uint32_t sink = 0u;
        (void)schway__bus_fetch(inst, epc & ~3u, &word, &sink);
        inst->fault = held;
    }
    TRACE(SCHWAY_TRACE_IRQ, trace_now(inst), epc, word, 0u, 0u, (uint32_t)SCHWAY__DEV_NONE,
          c->cop0.r[SCHWAY__COP0_CAUSE], 0u);
    schway__cop0_exception(inst, SCHWAY__EXC_INT, epc, slot, take, target, 0u, 0u, 0u);
    (void)schway__cop0_irq_entry_cop2(inst, word);
}

/* Takes an exception for the instruction at pc. A delay-slot instruction
   returns to the branch before it, and its TAR is where the branch goes, or
   the instruction after the slot when it was not taken (PSX-SYSTEM section 2,
   EPC and Branch delay). Returns 1 so the caller can note that the program
   counter now holds the vector. An interrupt that is due on this very
   instruction wins over the exceptions that rank below it: it is taken at pc and
   the instruction runs again after the return. The ones that rank above it
   (data address errors, bus errors on data, overflow) are taken, and the
   interrupt waits for the return from exception. */
static uint32_t cpu_exception(schway *inst, uint32_t pc, uint32_t code, uint32_t ce,
                              uint32_t has_badvaddr, uint32_t badvaddr)
{
    struct schway__cpu *c = &inst->cpu;
    uint32_t slot = c->branch_slot;
    if (c->irq_stamp == 1u && schway__cop0_irq_pending(inst) != 0u &&
        cpu_below_interrupt(code, pc) != 0u) {
        cpu_irq_enter(inst, pc);
        return 1u;
    }
    uint32_t take = c->branch_take;
    uint32_t target = take != 0u ? c->branch_target : pc + 4u;
    schway__cop0_exception(inst, code, slot != 0u ? pc - 4u : pc, slot, take, target, badvaddr,
                           has_badvaddr, ce);
    return 1u;
}

/* An encoding that is not an instruction: RI, and a first fault. */
static uint32_t cpu_reserved(schway *inst, uint32_t pc, uint32_t op)
{
    schway__fault_latch(inst, SCHWAY__FAULT_RESERVED_INSTRUCTION, SCHWAY__DEV_NONE, 4u, pc, op,
                        0u);
    return cpu_exception(inst, pc, SCHWAY__EXC_RI, 0u, 0u, 0u);
}

/* A coprocessor instruction whose SR.CUz bit is clear: CpU with CE the
   coprocessor number. Only COP1 and COP3, which the console does not have,
   are a first fault; COP0 and COP2 are real, and a handler may take the
   exception (PSX-SYSTEM section 2, Address and bus errors; ARCHITECTURE
   section 6). */
static uint32_t cpu_unusable(schway *inst, uint32_t pc, uint32_t op, uint32_t first_fault)
{
    if (first_fault != 0u) {
        schway__fault_latch(inst, SCHWAY__FAULT_COP_UNUSABLE, SCHWAY__DEV_NONE, 4u, pc, op, 0u);
    }
    return cpu_exception(inst, pc, SCHWAY__EXC_CPU, (op >> 26) & 3u, 0u, 0u);
}

/* The data access front-end, checks first. A half or a word access at an
   address that is not a multiple of its size is an address error, AdEL for a
   load and AdES for a store, with BadVaddr the address; it is a first fault
   and never reaches the bus. User mode (SR.KUc = 1) may touch only KUSEG, so
   an address with bit 31 set is the same address error, which a handler may
   take. LWL, LWR, SWL and SWR pass size 1 and never fault on alignment
   (PSX-SYSTEM section 2, Address and bus errors and Segments). Returns 1 when
   it took the exception. */
static uint32_t cpu_data_check(schway *inst, uint32_t pc, uint32_t op, uint32_t addr,
                               uint32_t size, uint32_t store)
{
    uint32_t sr = inst->cpu.cop0.r[SCHWAY__COP0_SR];
    uint32_t code = store != 0u ? (uint32_t)SCHWAY__EXC_ADES : (uint32_t)SCHWAY__EXC_ADEL;
    if ((addr & (size - 1u)) != 0u) {
        schway__fault_latch(inst, store != 0u ? SCHWAY__FAULT_MISALIGNED_WRITE
                                              : SCHWAY__FAULT_MISALIGNED_READ,
                            SCHWAY__DEV_NONE, size, pc, op, addr);
        return cpu_exception(inst, pc, code, 0u, 1u, addr);
    }
    if ((sr & SCHWAY__SR_KUC) != 0u && (addr & 0x80000000u) != 0u) {
        return cpu_exception(inst, pc, code, 0u, 1u, addr);
    }
    return 0u;
}

/* One access on the bus with the address already checked: the virtual
   address goes to the bus, which decodes the segment (KSEG2 gets the full
   address), and its charge adds to *cycles. A KSEG1 read drains the write queue
   first. A bus error is DBE and carries no BadVaddr; the bus has already
   latched it as the first fault (PSX-SYSTEM section 2, Address and bus errors).
   Returns 1 when it took the exception. A read yields the low size bytes of the
   datum. */
static uint32_t cpu_bus_read(schway *inst, uint32_t pc, uint32_t addr, uint32_t size,
                             uint32_t *value, uint32_t *cycles)
{
    uint32_t v = 0u;
    TRACE_BEGIN(at, *cycles);
    if (inst->cpu.log != NULL) {
        v = cpu_log_access(inst->cpu.log, SCHWAY__CYCLE_READ, size, addr, 0u);
    } else if (cpu_isolated(inst, addr) != 0u) {
        v = cpu_isolated_read(inst, addr, size);
        TRACE(SCHWAY_TRACE_RD, at, pc, 0u, addr, size, (uint32_t)SCHWAY__DEV_NONE, v, 0u);
    } else {
        if (schway__bus_segment(addr) == (uint32_t)SCHWAY__SEG_KSEG1) {
            cpu_wq_drain(&inst->cpu, cycles);
        }
        uint32_t err = schway__bus_read(inst, addr, size, &v, cycles) != 0 ? 1u : 0u;
        TRACE(SCHWAY_TRACE_RD, at, pc, 0u, addr, size, schway__bus_device(addr), v, 0u);
        if (err != 0u) {
            return cpu_exception(inst, pc, SCHWAY__EXC_DBE, 0u, 0u, 0u);
        }
    }
    *value = size == 4u ? v : v & ((1u << (8u * size)) - 1u);
    return 0u;
}

/* The store counterpart: replayed with a log, sent to the cache when isolated,
   otherwise drained first when it is in KSEG1, written to the bus and then
   queued. */
static uint32_t cpu_bus_write(schway *inst, uint32_t pc, uint32_t addr, uint32_t size,
                              uint32_t value, uint32_t *cycles)
{
    TRACE_BEGIN(at, *cycles);
    if (inst->cpu.log != NULL) {
        (void)cpu_log_access(inst->cpu.log, SCHWAY__CYCLE_WRITE, size, addr, value);
    } else if (cpu_isolated(inst, addr) != 0u) {
        cpu_isolated_write(inst, addr, size, value);
        TRACE(SCHWAY_TRACE_WR, at, pc, 0u, addr, size, (uint32_t)SCHWAY__DEV_NONE,
              size == 4u ? value : value & ((1u << (8u * size)) - 1u), 0u);
    } else {
        if (schway__bus_segment(addr) == (uint32_t)SCHWAY__SEG_KSEG1) {
            cpu_wq_drain(&inst->cpu, cycles);
        }
        uint32_t err = schway__bus_write(inst, addr, size, value, cycles) != 0 ? 1u : 0u;
        TRACE(SCHWAY_TRACE_WR, at, pc, 0u, addr, size, schway__bus_device(addr),
              size == 4u ? value : value & ((1u << (8u * size)) - 1u), 0u);
        if (err != 0u) {
            return cpu_exception(inst, pc, SCHWAY__EXC_DBE, 0u, 0u, 0u);
        }
        cpu_wq_push(&inst->cpu, addr, cycles);
    }
    return 0u;
}

/* LB, LBU, LH, LHU and LW: size bytes at the effective address into rt
   through the load delay, sign-extended when sign is set. A load that faults
   leaves rt untouched. Returns 1 when it trapped. */
static uint32_t cpu_load_int(schway *inst, uint32_t pc, uint32_t op, uint32_t size,
                             uint32_t sign, uint32_t *cycles, uint32_t *load_reg,
                             uint32_t *load_value)
{
    struct schway__cpu *c = &inst->cpu;
    uint32_t addr = cpu_address(c, op);
    uint32_t v = 0u;
    if (cpu_data_check(inst, pc, op, addr, size, 0u) != 0u ||
        cpu_bus_read(inst, pc, addr, size, &v, cycles) != 0u) {
        return 1u;
    }
    if (sign != 0u && size < 4u) {
        uint32_t top = 1u << (8u * size - 1u);
        v = (v ^ top) - top;
    }
    cpu_load_issue(c, (op >> 16) & 31u, v, load_reg, load_value);
    return 0u;
}

/* LWL and LWR: one read of the aligned word that holds the address, whose
   bytes from the address toward the high end (LWL) or the low end (LWR) of
   the register replace the same bytes of rt and leave the others. The merge
   starts from the value a load issued by the previous instruction is about to
   write, so a pair on one register needs no delay between them (PSX-SYSTEM
   section 2, Stores, LWL/LWR). Neither faults on alignment. */
static uint32_t cpu_load_part(schway *inst, uint32_t pc, uint32_t op, uint32_t left,
                              uint32_t *cycles, uint32_t *load_reg, uint32_t *load_value)
{
    struct schway__cpu *c = &inst->cpu;
    uint32_t addr = cpu_address(c, op);
    uint32_t rt_n = (op >> 16) & 31u;
    uint32_t off = addr & 3u;
    uint32_t w = 0u;
    uint32_t cur, v;
    if (cpu_data_check(inst, pc, op, addr, 1u, 0u) != 0u ||
        cpu_bus_read(inst, pc, addr & ~3u, 4u, &w, cycles) != 0u) {
        return 1u;
    }
    cur = c->load_reg == rt_n ? c->load_value : c->r[rt_n];
    v = left != 0u ? (cur & (0x00FFFFFFu >> (8u * off))) | (w << (24u - 8u * off))
                   : (cur & (0xFFFFFF00u << (24u - 8u * off))) | (w >> (8u * off));
    cpu_load_issue(c, rt_n, v, load_reg, load_value);
    return 0u;
}

/* SB, SH and SW: the low size bytes of rt at the effective address. All 32
   bits of rt go on the bus whatever the width (PSX-SYSTEM section 2, Stores).
   Returns 1 when it trapped. */
static uint32_t cpu_store_int(schway *inst, uint32_t pc, uint32_t op, uint32_t size,
                              uint32_t *cycles)
{
    struct schway__cpu *c = &inst->cpu;
    uint32_t addr = cpu_address(c, op);
    if (cpu_data_check(inst, pc, op, addr, size, 1u) != 0u) {
        return 1u;
    }
    return cpu_bus_write(inst, pc, addr, size, c->r[(op >> 16) & 31u], cycles);
}

/* SWL and SWR: the bytes of the aligned word from its low end up to the
   address (SWL) or from the address to its high end (SWR) take the matching
   bytes of rt, and the others stay. They go out as the largest aligned
   chunks of 4, 2 or 1 bytes, lowest first. Neither faults on alignment. */
static uint32_t cpu_store_part(schway *inst, uint32_t pc, uint32_t op, uint32_t left,
                               uint32_t *cycles)
{
    struct schway__cpu *c = &inst->cpu;
    uint32_t addr = cpu_address(c, op);
    uint32_t rt = c->r[(op >> 16) & 31u];
    uint32_t off = addr & 3u;
    uint32_t lo = left != 0u ? 0u : off;
    uint32_t hi = left != 0u ? off : 3u;
    if (cpu_data_check(inst, pc, op, addr, 1u, 1u) != 0u) {
        return 1u;
    }
    while (lo <= hi) {
        uint32_t len = hi - lo + 1u;
        uint32_t size = (lo & 3u) == 0u && len >= 4u ? 4u : ((lo & 1u) == 0u && len >= 2u ? 2u : 1u);
        uint32_t byte = left != 0u ? 3u - off + lo : lo - off; /* the byte of rt that lands at lo */
        if (cpu_bus_write(inst, pc, (addr & ~3u) + lo, size, rt >> (8u * byte), cycles) != 0u) {
            return 1u;
        }
        lo += size;
    }
    return 0u;
}

/* COP2, LWC2 and SWC2 before the GTE exists. An address error on LWC2 or SWC2
   comes first. With SR.CU2 clear every form raises CpU with CE 2 and latches
   no fault. With it set, MFC2 and CFC2 read 0 through the load delay, MTC2,
   CTC2 and the commands do nothing, BC2 never goes (the condition line is
   never true), LWC2 does the real bus read and drops it and SWC2 the real bus
   write of 0; each counts as an access to the unbuilt GTE (ARCHITECTURE
   section 6; PSX-SYSTEM section 2, SR bits). Returns 1 when it trapped. */
static uint32_t cpu_cop2(schway *inst, uint32_t pc, uint32_t op, uint32_t next, uint32_t *cycles,
                         uint32_t *load_reg, uint32_t *load_value, struct cpu_branch *br)
{
    struct schway__cpu *c = &inst->cpu;
    uint32_t opc = op >> 26;
    uint32_t rs_n = (op >> 21) & 31u;
    uint32_t rt_n = (op >> 16) & 31u;
    uint32_t rd_n = (op >> 11) & 31u;
    uint32_t addr = cpu_address(c, op);
    uint32_t v = 0u;
    if (opc != 0x12u && cpu_data_check(inst, pc, op, addr, 4u, opc == 0x3Au ? 1u : 0u) != 0u) {
        return 1u;
    }
    if ((c->cop0.r[SCHWAY__COP0_SR] & SCHWAY__SR_CU2) == 0u) {
        return cpu_unusable(inst, pc, op, 0u);
    }
    if (opc == 0x32u) { /* LWC2 */
        if (cpu_bus_read(inst, pc, addr, 4u, &v, cycles) != 0u) {
            return 1u;
        }
        schway__bus_unbuilt(inst, SCHWAY__DEV_GTE, addr, 4u, 0u, 0u);
    } else if (opc == 0x3Au) { /* SWC2 */
        if (cpu_bus_write(inst, pc, addr, 4u, 0u, cycles) != 0u) {
            return 1u;
        }
        schway__bus_unbuilt(inst, SCHWAY__DEV_GTE, addr, 4u, 1u, 0u);
    } else if (rs_n == 0u || rs_n == 2u) { /* MFC2, CFC2 */
        cpu_load_issue(c, rt_n, 0u, load_reg, load_value);
        schway__bus_unbuilt(inst, SCHWAY__DEV_GTE, rs_n == 2u ? 32u + rd_n : rd_n, 4u, 0u, 0u);
    } else if (rs_n == 4u || rs_n == 6u) { /* MTC2, CTC2 */
        schway__bus_unbuilt(inst, SCHWAY__DEV_GTE, rs_n == 6u ? 32u + rd_n : rd_n, 4u, 1u,
                            c->r[rt_n]);
    } else if (rs_n == 8u) { /* BC2F, BC2T */
        cpu_branch_set(br, (rt_n & 1u) == 0u ? 1u : 0u, next + (((op & 0xFFFFu) ^ 0x8000u) - 0x8000u) * 4u);
        schway__bus_unbuilt(inst, SCHWAY__DEV_GTE, 0u, 4u, 0u, 0u);
    } else if ((rs_n & 0x10u) != 0u) { /* a GTE command */
        schway__bus_unbuilt(inst, SCHWAY__DEV_GTE, op & 0x01FFFFFFu, 4u, 1u, 0u);
    }
    return 0u;
}

/* Executes the instruction at pc and returns the cycles it took: one to
   issue, plus what the bus charged. The program counter moves to pc + 4, or
   to the pending branch target when the instruction is its delay slot, or to
   the exception vector when the instruction takes one; the load issued by the
   previous instruction lands as this one ends. */
static uint32_t cpu_step(schway *inst)
{
    struct schway__cpu *c = &inst->cpu;
    uint32_t cycles = 1u;
    uint32_t pc = c->pc;
    uint32_t misaligned = (pc & 3u) != 0u ? 1u : 0u;
    uint32_t fetch_err = 0u;
    uint32_t op = misaligned != 0u ? 0u : cpu_fetch(inst, pc, &cycles, &fetch_err);
    uint32_t next = c->branch_take != 0u ? c->branch_target : pc + 4u;
    uint32_t rs_n = (op >> 21) & 31u;
    uint32_t rt_n = (op >> 16) & 31u;
    uint32_t rd_n = (op >> 11) & 31u;
    uint32_t sa = (op >> 6) & 31u;
    uint32_t rs = c->r[rs_n];
    uint32_t rt = c->r[rt_n];
    uint32_t imm = op & 0xFFFFu;
    uint32_t simm = (imm ^ 0x8000u) - 0x8000u;
    uint32_t neg = (rs & 0x80000000u) != 0u ? 1u : 0u;
    uint32_t sr = c->cop0.r[SCHWAY__COP0_SR];
    uint32_t trapped = 0u;
    /* COP2, LWC2 and SWC2: the GTE's instructions, which cpu_cop2 handles
       until the GTE does. */
    uint32_t is_cop2 = (op >> 26) == 0x12u || (op >> 26) == 0x32u || (op >> 26) == 0x3Au ? 1u : 0u;
    uint32_t sum, value;
    struct cpu_branch br = {0u, 0u, 0u};
    uint32_t load_reg = SCHWAY__CPU_NO_LOAD;
    uint32_t load_value = 0u;

    TRACE(SCHWAY_TRACE_EXEC, trace_now(inst), pc, op, 0u, 0u, (uint32_t)SCHWAY__DEV_NONE, 0u, 0u);
    if (misaligned != 0u) {
        /* A fetch from an address that is not a multiple of 4 is an address
           error on the fetch, EPC and BadVaddr the address (PSX-SYSTEM
           section 2, Branch delay: JR and JALR to a misaligned target). */
        schway__fault_latch(inst, SCHWAY__FAULT_MISALIGNED_FETCH, SCHWAY__DEV_NONE, 4u, pc, 0u,
                            pc);
        trapped = cpu_exception(inst, pc, SCHWAY__EXC_ADEL, 0u, 1u, pc);
    } else if (fetch_err != 0u) {
        /* A bus error on the fetch is IBE with no BadVaddr (PSX-SYSTEM section
           2, Address and bus errors). At an exception vector it cannot be
           recovered: the handler would fault again, so the run loop stops. */
        trapped = cpu_exception(inst, pc, SCHWAY__EXC_IBE, 0u, 0u, 0u);
        if (pc == SCHWAY__VECTOR_BEV || pc == SCHWAY__VECTOR_RAM) {
            c->block_end |= SCHWAY__BLOCK_DOUBLE_FAULT;
        }
    } else if (is_cop2 != 0u) {
        trapped = cpu_cop2(inst, pc, op, next, &cycles, &load_reg, &load_value, &br);
    } else {
        switch (op >> 26) {
        case 0x00u: /* SPECIAL */
            switch (op & 0x3Fu) {
            case 0x00u: /* SLL */
                cpu_set(c, rd_n, rt << sa);
                break;
            case 0x02u: /* SRL */
                cpu_set(c, rd_n, rt >> sa);
                break;
            case 0x03u: /* SRA */
                cpu_set(c, rd_n, cpu_sra(rt, sa));
                break;
            case 0x04u: /* SLLV */
                cpu_set(c, rd_n, rt << (rs & 31u));
                break;
            case 0x06u: /* SRLV */
                cpu_set(c, rd_n, rt >> (rs & 31u));
                break;
            case 0x07u: /* SRAV */
                cpu_set(c, rd_n, cpu_sra(rt, rs & 31u));
                break;
            case 0x08u: /* JR */
                cpu_branch_set(&br, 1u, rs);
                break;
            case 0x09u: /* JALR */
                cpu_branch_set(&br, 1u, rs);
                cpu_set(c, rd_n, next + 4u);
                break;
            case 0x10u: /* MFHI */
                cycles += cpu_muldiv_wait(c, cycles);
                cpu_set(c, rd_n, c->hi);
                break;
            case 0x11u: /* MTHI */
                c->hi = rs;
                c->muldiv_ready = 0u;
                break;
            case 0x12u: /* MFLO */
                cycles += cpu_muldiv_wait(c, cycles);
                cpu_set(c, rd_n, c->lo);
                break;
            case 0x13u: /* MTLO */
                c->lo = rs;
                c->muldiv_ready = 0u;
                break;
            case 0x18u: /* MULT */
                cpu_mult(c, rs, rt, 1u, cycles);
                break;
            case 0x19u: /* MULTU */
                cpu_mult(c, rs, rt, 0u, cycles);
                break;
            case 0x1Au: /* DIV */
                cpu_div(c, rs, rt, 1u, cycles);
                break;
            case 0x1Bu: /* DIVU */
                cpu_div(c, rs, rt, 0u, cycles);
                break;
            case 0x0Cu: /* SYSCALL */
                trapped = cpu_exception(inst, pc, SCHWAY__EXC_SYS, 0u, 0u, 0u);
                break;
            case 0x0Du: /* BREAK */
                trapped = cpu_exception(inst, pc, SCHWAY__EXC_BP, 0u, 0u, 0u);
                break;
            case 0x20u: /* ADD */
                sum = rs + rt;
                if (cpu_add_overflows(rs, rt, sum) != 0u) {
                    trapped = cpu_exception(inst, pc, SCHWAY__EXC_OVF, 0u, 0u, 0u);
                } else {
                    cpu_set(c, rd_n, sum);
                }
                break;
            case 0x21u: /* ADDU */
                cpu_set(c, rd_n, rs + rt);
                break;
            case 0x22u: /* SUB */
                sum = rs - rt;
                if (cpu_sub_overflows(rs, rt, sum) != 0u) {
                    trapped = cpu_exception(inst, pc, SCHWAY__EXC_OVF, 0u, 0u, 0u);
                } else {
                    cpu_set(c, rd_n, sum);
                }
                break;
            case 0x23u: /* SUBU */
                cpu_set(c, rd_n, rs - rt);
                break;
            case 0x24u: /* AND */
                cpu_set(c, rd_n, rs & rt);
                break;
            case 0x25u: /* OR */
                cpu_set(c, rd_n, rs | rt);
                break;
            case 0x26u: /* XOR */
                cpu_set(c, rd_n, rs ^ rt);
                break;
            case 0x27u: /* NOR */
                cpu_set(c, rd_n, ~(rs | rt));
                break;
            case 0x2Au: /* SLT */
                cpu_set(c, rd_n, cpu_slt(rs, rt));
                break;
            case 0x2Bu: /* SLTU */
                cpu_set(c, rd_n, rs < rt ? 1u : 0u);
                break;
            default:
                trapped = cpu_reserved(inst, pc, op);
                break;
            }
            break;
        case 0x01u: /* BCondZ: BLTZ, BGEZ, BLTZAL, BGEZAL */
            /* Bit 16 of the instruction selects "greater or equal" over
               "less than". Only 10h and 11h link; every other value of the
               field acts as the plain compare. The link register is written
               whether or not the branch goes (PSX-SYSTEM section 2, Branch
               delay). */
            cpu_branch_set(&br, (rt_n & 1u) != 0u ? 1u - neg : neg, next + (simm << 2));
            if (rt_n == 0x10u || rt_n == 0x11u) {
                cpu_set(c, 31u, next + 4u);
            }
            break;
        case 0x02u: /* J */
            cpu_branch_set(&br, 1u, (next & 0xF0000000u) | ((op & 0x03FFFFFFu) << 2));
            break;
        case 0x03u: /* JAL */
            cpu_branch_set(&br, 1u, (next & 0xF0000000u) | ((op & 0x03FFFFFFu) << 2));
            cpu_set(c, 31u, next + 4u);
            break;
        case 0x04u: /* BEQ */
            cpu_branch_set(&br, rs == rt ? 1u : 0u, next + (simm << 2));
            break;
        case 0x05u: /* BNE */
            cpu_branch_set(&br, rs != rt ? 1u : 0u, next + (simm << 2));
            break;
        case 0x06u: /* BLEZ */
            cpu_branch_set(&br, neg != 0u || rs == 0u ? 1u : 0u, next + (simm << 2));
            break;
        case 0x07u: /* BGTZ */
            cpu_branch_set(&br, neg == 0u && rs != 0u ? 1u : 0u, next + (simm << 2));
            break;
        case 0x08u: /* ADDI */
            sum = rs + simm;
            if (cpu_add_overflows(rs, simm, sum) != 0u) {
                trapped = cpu_exception(inst, pc, SCHWAY__EXC_OVF, 0u, 0u, 0u);
            } else {
                cpu_set(c, rt_n, sum);
            }
            break;
        case 0x09u: /* ADDIU */
            cpu_set(c, rt_n, rs + simm);
            break;
        case 0x0Au: /* SLTI */
            cpu_set(c, rt_n, cpu_slt(rs, simm));
            break;
        case 0x0Bu: /* SLTIU */
            cpu_set(c, rt_n, rs < simm ? 1u : 0u);
            break;
        case 0x0Cu: /* ANDI */
            cpu_set(c, rt_n, rs & imm);
            break;
        case 0x0Du: /* ORI */
            cpu_set(c, rt_n, rs | imm);
            break;
        case 0x0Eu: /* XORI */
            cpu_set(c, rt_n, rs ^ imm);
            break;
        case 0x0Fu: /* LUI */
            cpu_set(c, rt_n, imm << 16);
            break;
        case 0x20u: /* LB */
            trapped = cpu_load_int(inst, pc, op, 1u, 1u, &cycles, &load_reg, &load_value);
            break;
        case 0x21u: /* LH */
            trapped = cpu_load_int(inst, pc, op, 2u, 1u, &cycles, &load_reg, &load_value);
            break;
        case 0x22u: /* LWL */
            trapped = cpu_load_part(inst, pc, op, 1u, &cycles, &load_reg, &load_value);
            break;
        case 0x23u: /* LW */
            trapped = cpu_load_int(inst, pc, op, 4u, 0u, &cycles, &load_reg, &load_value);
            break;
        case 0x24u: /* LBU */
            trapped = cpu_load_int(inst, pc, op, 1u, 0u, &cycles, &load_reg, &load_value);
            break;
        case 0x25u: /* LHU */
            trapped = cpu_load_int(inst, pc, op, 2u, 0u, &cycles, &load_reg, &load_value);
            break;
        case 0x26u: /* LWR */
            trapped = cpu_load_part(inst, pc, op, 0u, &cycles, &load_reg, &load_value);
            break;
        case 0x28u: /* SB */
            trapped = cpu_store_int(inst, pc, op, 1u, &cycles);
            break;
        case 0x29u: /* SH */
            trapped = cpu_store_int(inst, pc, op, 2u, &cycles);
            break;
        case 0x2Au: /* SWL */
            trapped = cpu_store_part(inst, pc, op, 1u, &cycles);
            break;
        case 0x2Bu: /* SW */
            trapped = cpu_store_int(inst, pc, op, 4u, &cycles);
            break;
        case 0x2Eu: /* SWR */
            trapped = cpu_store_part(inst, pc, op, 0u, &cycles);
            break;
        case 0x10u: /* COP0 */
            if ((sr & SCHWAY__SR_KUC) != 0u && (sr & SCHWAY__SR_CU0) == 0u) {
                trapped = cpu_unusable(inst, pc, op, 0u);
            } else if (rs_n == 0u) { /* MFC0: one delay slot, as a load */
                if (schway__cop0_mfc0(inst, rd_n, &value) != 0u) {
                    cpu_load_issue(c, rt_n, value, &load_reg, &load_value);
                } else {
                    trapped = cpu_reserved(inst, pc, op);
                }
            } else if (rs_n == 4u) { /* MTC0 */
                if (schway__cop0_mtc0(inst, rd_n, rt) == 0u) {
                    trapped = cpu_reserved(inst, pc, op);
                }
            } else if ((rs_n & 0x10u) != 0u && (op & 0x3Fu) == 0x10u) { /* RFE */
                schway__cop0_rfe(inst);
            } else {
                trapped = cpu_reserved(inst, pc, op);
            }
            break;
        case 0x11u: /* COP1 */
        case 0x13u: /* COP3 */
        case 0x31u: /* LWC1 */
        case 0x33u: /* LWC3 */
        case 0x39u: /* SWC1 */
        case 0x3Bu: /* SWC3 */
            /* With the coprocessor's SR.CUz bit set, BCzF always goes and
               BCzT never does, as the condition line is never true; every
               other instruction does nothing (PSX-SYSTEM section 2, SR
               bits). */
            if ((sr & (((op >> 26) & 3u) == 1u ? SCHWAY__SR_CU1 : SCHWAY__SR_CU3)) == 0u) {
                trapped = cpu_unusable(inst, pc, op, 1u);
            } else if (rs_n == 8u && (op >> 31) == 0u) {
                cpu_branch_set(&br, (rt_n & 1u) == 0u ? 1u : 0u, next + (simm << 2));
            }
            break;
        default:
            trapped = cpu_reserved(inst, pc, op);
            break;
        }
    }

    /* The load issued by the previous instruction lands, and this
       instruction's own load, if any, is now the pending one. An exception
       has already moved the program counter to the vector and emptied the
       branch record. */
    if (c->load_reg != SCHWAY__CPU_NO_LOAD) {
        if (c->load_reg != 0u) {
            c->r[c->load_reg] = c->load_value;
        }
    }
    c->load_reg = load_reg;
    c->load_value = load_value;
    c->branch_slot = br.slot;
    c->branch_take = br.take;
    c->branch_target = br.target;
    if (trapped == 0u) {
        c->pc = next;
    }
    c->cycle += cycles;
    /* The retirement stamp: this instruction retired, so one retirement of the
       latency is counted, and at the last one the interrupt is taken if the line
       is still true. It is taken before the next instruction runs. */
    if (c->irq_stamp != 0u) {
        c->irq_stamp--;
        if (c->irq_stamp == 0u && schway__cop0_irq_pending(inst) != 0u) {
            cpu_irq_enter(inst, c->pc);
        }
    }
    return cycles;
}

/* Runs instructions until the block ends and returns the cycles they took. A
   block ends after a branch's delay slot, after any instruction that set
   block_end (an exception entry, which covers SYSCALL and BREAK, RFE, a COP0
   write, a write to I_STAT or I_MASK, a double fault), when the next
   instruction lies in another 4 KiB page, after 128 instructions, after the
   exit port was written, after an instruction that latched a first fault, and
   at the first instruction boundary at or after limit cycles. The cycles are
   added to inst->ticks once, at the end, so what the instructions did not
   change, the state of the other devices, is as of the block's start
   (PSX-SYSTEM section 6; a fault latched in the block carries the ticks of its
   start). */
static uint64_t cpu_block(schway *inst, uint64_t limit)
{
    struct schway__cpu *c = &inst->cpu;
    uint32_t page = c->pc >> 12;
    uint32_t fault = inst->fault.kind;
    uint64_t ran = 0u;
    c->block_end = 0u;
    TRACE_START();
    for (uint32_t n = 0u; n < SCHWAY__BLOCK_MAX_INSNS; n++) {
        uint32_t slot = c->branch_slot;
        TRACE_BEGIN(began, 1u);
        ran += cpu_step(inst);
        if (slot != 0u || c->block_end != 0u || ran >= limit || (c->pc >> 12) != page ||
            inst->exit_pending != 0u || inst->fault.kind != fault) {
            /* The first fault, when this block latched it, ends the tail: its own
               record, from the fault record, on the cycle its instruction began. */
            TRACE_FAULT(inst->fault.kind != fault, began);
            break;
        }
    }
    inst->ticks += ran;
    return ran;
}

#undef TRACE
#undef TRACE_BEGIN
#undef TRACE_START
#undef TRACE_FAULT
#if SCHWAY_TRACE
#undef cpu_irq_enter
#undef cpu_exception
#undef cpu_reserved
#undef cpu_unusable
#undef cpu_data_check
#undef cpu_bus_read
#undef cpu_bus_write
#undef cpu_load_int
#undef cpu_load_part
#undef cpu_store_int
#undef cpu_store_part
#undef cpu_cop2
#undef cpu_step
#undef cpu_block
#endif
