/* The executor: fetch, decode and execute one instruction. cpu.c includes it,
   so the one-instruction entry and the run loop run the same code. The CPU is
   a MIPS I R3000A (PSX-SYSTEM section 2). Integer only. */

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

/* The access front-ends. With a log attached they replay it; without, they
   reach the bus with the virtual address, whose segment the bus decodes
   (PSX-SYSTEM section 2, Segments; section 3, C4), and add its charge to
   *cycles. A bus error reads as zero. */
static uint32_t cpu_fetch(schway *inst, uint32_t vaddr, uint32_t *cycles)
{
    uint32_t word = 0u;
    if (inst->cpu.log != NULL) {
        return cpu_log_access(inst->cpu.log, SCHWAY__CYCLE_FETCH, 4u, vaddr, 0u);
    }
    (void)schway__bus_fetch(inst, vaddr, &word, cycles);
    return word;
}

static inline uint32_t cpu_read(schway *inst, uint32_t vaddr, uint32_t size, uint32_t *cycles)
{
    uint32_t value = 0u;
    if (inst->cpu.log != NULL) {
        return cpu_log_access(inst->cpu.log, SCHWAY__CYCLE_READ, size, vaddr, 0u);
    }
    (void)schway__bus_read(inst, vaddr, size, &value, cycles);
    return value;
}

static inline void cpu_write(schway *inst, uint32_t vaddr, uint32_t size, uint32_t value,
                             uint32_t *cycles)
{
    if (inst->cpu.log != NULL) {
        (void)cpu_log_access(inst->cpu.log, SCHWAY__CYCLE_WRITE, size, vaddr, value);
        return;
    }
    (void)schway__bus_write(inst, vaddr, size, value, cycles);
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

/* Takes an exception for the instruction at pc. A delay-slot instruction
   returns to the branch before it, and its TAR is where the branch goes, or
   the instruction after the slot when it was not taken (PSX-SYSTEM section 2,
   EPC and Branch delay). Returns 1 so the caller can note that the program
   counter now holds the vector. */
static uint32_t cpu_exception(schway *inst, uint32_t pc, uint32_t code, uint32_t ce,
                              uint32_t has_badvaddr, uint32_t badvaddr)
{
    struct schway__cpu *c = &inst->cpu;
    uint32_t slot = c->branch_slot;
    uint32_t take = c->branch_take;
    uint32_t target = take != 0u ? c->branch_target : pc + 4u;
    schway__cop0_exception(inst, code, slot != 0u ? pc - 4u : pc, slot, take, target, badvaddr,
                           has_badvaddr, ce);
    return 1u;
}

/* Executes the instruction at pc and returns the cycles it took: one to
   issue, plus what the bus charged. The program counter moves to pc + 4, or
   to the pending branch target when the instruction is its delay slot; the
   load issued by the previous instruction lands as this one ends. */
static uint32_t cpu_step(schway *inst)
{
    struct schway__cpu *c = &inst->cpu;
    uint32_t cycles = 1u;
    uint32_t pc = c->pc;
    uint32_t op = cpu_fetch(inst, pc, &cycles);
    uint32_t next = c->branch_take != 0u ? c->branch_target : pc + 4u;
    uint32_t rs_n = (op >> 21) & 31u;
    uint32_t rt_n = (op >> 16) & 31u;
    uint32_t rd_n = (op >> 11) & 31u;
    uint32_t sa = (op >> 6) & 31u;
    uint32_t rs = c->r[rs_n];
    uint32_t rt = c->r[rt_n];
    uint32_t imm = op & 0xFFFFu;
    uint32_t simm = (imm ^ 0x8000u) - 0x8000u;
    uint32_t trapped = 0u;

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
        case 0x0Cu: /* SYSCALL */
            trapped = cpu_exception(inst, pc, SCHWAY__EXC_SYS, 0u, 0u, 0u);
            break;
        case 0x21u: /* ADDU */
            cpu_set(c, rd_n, rs + rt);
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
            schway__fault_latch(inst, SCHWAY__FAULT_RESERVED_INSTRUCTION, SCHWAY__DEV_NONE, 0u,
                                pc, op, 0u);
            break;
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
    default:
        schway__fault_latch(inst, SCHWAY__FAULT_RESERVED_INSTRUCTION, SCHWAY__DEV_NONE, 0u, pc,
                            op, 0u);
        break;
    }

    /* The load issued by the previous instruction lands, and this
       instruction issues none. Nothing decoded here starts a branch. An
       exception has already moved the program counter to the vector. */
    if (c->load_reg != SCHWAY__CPU_NO_LOAD) {
        if (c->load_reg != 0u) {
            c->r[c->load_reg] = c->load_value;
        }
        c->load_reg = SCHWAY__CPU_NO_LOAD;
        c->load_value = 0u;
    }
    c->branch_slot = 0u;
    c->branch_take = 0u;
    c->branch_target = 0u;
    if (trapped == 0u) {
        c->pc = next;
    }
    return cycles;
}
