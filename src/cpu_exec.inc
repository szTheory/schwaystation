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

/* The instruction fetch front-end. With a log attached it replays it;
   without, it reaches the bus with the virtual address, whose segment the bus
   decodes (PSX-SYSTEM section 2, Segments; section 3, C4), and adds its
   charge to *cycles. A bus error reads as zero. */
static uint32_t cpu_fetch(schway *inst, uint32_t vaddr, uint32_t *cycles)
{
    uint32_t word = 0u;
    if (inst->cpu.log != NULL) {
        return cpu_log_access(inst->cpu.log, SCHWAY__CYCLE_FETCH, 4u, vaddr, 0u);
    }
    (void)schway__bus_fetch(inst, vaddr, &word, cycles);
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
   address), and its charge adds to *cycles. A bus error is DBE and carries no
   BadVaddr; the bus has already latched it as the first fault (PSX-SYSTEM
   section 2, Address and bus errors). Returns 1 when it took the exception.
   A read yields the low size bytes of the datum. */
static uint32_t cpu_bus_read(schway *inst, uint32_t pc, uint32_t addr, uint32_t size,
                             uint32_t *value, uint32_t *cycles)
{
    uint32_t v = 0u;
    if (inst->cpu.log != NULL) {
        v = cpu_log_access(inst->cpu.log, SCHWAY__CYCLE_READ, size, addr, 0u);
    } else if (schway__bus_read(inst, addr, size, &v, cycles) != 0u) {
        return cpu_exception(inst, pc, SCHWAY__EXC_DBE, 0u, 0u, 0u);
    }
    *value = size == 4u ? v : v & ((1u << (8u * size)) - 1u);
    return 0u;
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

/* LW: the word at rs + the sign-extended immediate, into rt through the load
   delay. A load that faults leaves rt untouched. Returns 1 when it trapped. */
static uint32_t cpu_load_word(schway *inst, uint32_t pc, uint32_t op, uint32_t *cycles,
                              uint32_t *load_reg, uint32_t *load_value)
{
    struct schway__cpu *c = &inst->cpu;
    uint32_t addr = c->r[(op >> 21) & 31u] + (((op & 0xFFFFu) ^ 0x8000u) - 0x8000u);
    uint32_t v = 0u;
    if (cpu_data_check(inst, pc, op, addr, 4u, 0u) != 0u ||
        cpu_bus_read(inst, pc, addr, 4u, &v, cycles) != 0u) {
        return 1u;
    }
    cpu_load_issue(c, (op >> 16) & 31u, v, load_reg, load_value);
    return 0u;
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
    uint32_t op = misaligned != 0u ? 0u : cpu_fetch(inst, pc, &cycles);
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
    uint32_t sum, value;
    struct cpu_branch br = {0u, 0u, 0u};
    uint32_t load_reg = SCHWAY__CPU_NO_LOAD;
    uint32_t load_value = 0u;

    if (misaligned != 0u) {
        /* A fetch from an address that is not a multiple of 4 is an address
           error on the fetch, EPC and BadVaddr the address (PSX-SYSTEM
           section 2, Branch delay: JR and JALR to a misaligned target). */
        schway__fault_latch(inst, SCHWAY__FAULT_MISALIGNED_FETCH, SCHWAY__DEV_NONE, 4u, pc, 0u,
                            pc);
        trapped = cpu_exception(inst, pc, SCHWAY__EXC_ADEL, 0u, 1u, pc);
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
        case 0x23u: /* LW */
            trapped = cpu_load_word(inst, pc, op, &cycles, &load_reg, &load_value);
            break;
        case 0x10u: /* COP0 */
            if ((sr & SCHWAY__SR_KUC) != 0u && (sr & SCHWAY__SR_CU0) == 0u) {
                trapped = cpu_unusable(inst, pc, op, 0u);
            } else if (rs_n == 0u) { /* MFC0: one delay slot, as a load */
                if (schway__cop0_mfc0(inst, rd_n, &value) != 0u) {
                    if (c->load_reg == rt_n) {
                        c->load_reg = SCHWAY__CPU_NO_LOAD;
                        c->load_value = 0u;
                    }
                    load_reg = rt_n;
                    load_value = value;
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
            /* With the coprocessor's SR.CUz bit set, BCzF always goes and
               BCzT never does, as the condition line is never true; every
               other instruction does nothing (PSX-SYSTEM section 2, SR
               bits). */
            if ((sr & ((op >> 26) == 0x11u ? SCHWAY__SR_CU1 : SCHWAY__SR_CU3)) == 0u) {
                trapped = cpu_unusable(inst, pc, op, 1u);
            } else if (rs_n == 8u) {
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
    return cycles;
}
