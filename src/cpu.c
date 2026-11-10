/* The CPU: power-on values, the state walk and the one-instruction entry. */
#include <string.h>

#include "state.h"

static const char name_r[] = "cpu.r";
static const char name_hi[] = "cpu.hi";
static const char name_lo[] = "cpu.lo";
static const char name_pc[] = "cpu.pc";
static const char name_cop0[] = "cop0.r";
static const char name_load_reg[] = "cpu.load_reg";
static const char name_load_value[] = "cpu.load_value";
static const char name_branch_slot[] = "cpu.branch_slot";
static const char name_branch_take[] = "cpu.branch_take";
static const char name_branch_target[] = "cpu.branch_target";
static const char name_exc_epc[] = "cpu.exc_epc";
static const char name_exc_code[] = "cpu.exc_code";
static const char name_exc_repeat[] = "cpu.exc_repeat";
static const char name_muldiv_ready[] = "cpu.muldiv_ready";
static const char name_cycle[] = "cpu.cycle";
static const char name_wq_count[] = "cpu.wq_count";
static const char name_irq_stamp[] = "cpu.irq_stamp";
static const char name_icache_tag[] = "cpu.icache_tag";
static const char name_icache_word[] = "cpu.icache_word";
static const char name_icache_valid[] = "cpu.icache_valid";

/* General registers, HI and LO are 0 and the program counter is the BIOS
   entry: the machine starts at BFC00000h (ARCHITECTURE section 7,
   Power-on). */
void schway__cpu_reset(schway *inst)
{
    struct schway__cpu *c = &inst->cpu;
    memset(c->r, 0, sizeof c->r);
    c->hi = 0u;
    c->lo = 0u;
    c->pc = SCHWAY__CPU_RESET_PC;
    c->load_reg = SCHWAY__CPU_NO_LOAD;
    c->load_value = 0u;
    c->branch_slot = 0u;
    c->branch_take = 0u;
    c->branch_target = 0u;
    c->exc_epc = 0u;
    c->exc_code = 0u;
    c->exc_repeat = 0u;
    c->muldiv_ready = 0u;
    c->cycle = 0u;
    /* Every line starts invalid; the words are never read before a fill
       writes them, and are zero so the state is the same on every run
       (PSX-SYSTEM section 9, CPU, I-cache). */
    c->wq_count = 0u;
    c->irq_stamp = 0u;
    c->block_end = 0u;
    memset(c->icache_tag, 0, sizeof c->icache_tag);
    memset(c->icache_word, 0, sizeof c->icache_word);
    memset(c->icache_valid, 0, sizeof c->icache_valid);
    c->log = NULL;
    schway__cop0_reset(inst);
}

uint64_t schway__cpu_state_io(struct schway__state_io *io, schway *inst)
{
    struct schway__cpu *c = &inst->cpu;
    schway__state_u32_array(io, name_r, c->r, 32u, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_hi, &c->hi, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_lo, &c->lo, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_pc, &c->pc, 0u, UINT32_MAX);
    schway__state_u32_array(io, name_cop0, c->cop0.r, 32u, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_load_reg, &c->load_reg, 0u, SCHWAY__CPU_NO_LOAD);
    (void)schway__state_u32(io, name_load_value, &c->load_value, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_branch_slot, &c->branch_slot, 0u, 1u);
    (void)schway__state_u32(io, name_branch_take, &c->branch_take, 0u, 1u);
    (void)schway__state_u32(io, name_branch_target, &c->branch_target, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_exc_epc, &c->exc_epc, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_exc_code, &c->exc_code, 0u, 31u);
    (void)schway__state_u32(io, name_exc_repeat, &c->exc_repeat, 0u, UINT32_MAX);
    (void)schway__state_u64(io, name_muldiv_ready, &c->muldiv_ready, 0u, UINT64_MAX);
    uint64_t cycle = schway__state_u64(io, name_cycle, &c->cycle, 0u, UINT64_MAX);
    (void)schway__state_u32(io, name_wq_count, &c->wq_count, 0u, SCHWAY__WQ_DEPTH);
    (void)schway__state_u32(io, name_irq_stamp, &c->irq_stamp, 0u, SCHWAY__IRQ_LATENCY_EDGE);
    schway__state_u32_array(io, name_icache_tag, c->icache_tag, SCHWAY__ICACHE_LINES, 0u,
                            0xFFFFFu);
    schway__state_u32_array(io, name_icache_word, &c->icache_word[0][0], 4u * SCHWAY__ICACHE_LINES,
                            0u, UINT32_MAX);
    schway__state_u8_array(io, name_icache_valid, c->icache_valid, SCHWAY__ICACHE_LINES, 0u, 15u);
    return cycle;
}

/* The instruction cache (PSX-SYSTEM section 2, I-cache; psx-spx Memory Map,
   Cache Control). Direct mapped: address bits 11:4 pick the line, physical bits
   31:12 are its tag, and each of its four words has a valid bit. A word that
   hits is returned as it is, so a store to the same RAM never reaches it. A miss
   on a different tag takes the line over, with no word valid; a miss on the
   line's own tag (a word that is not valid) keeps the valid words it has. Either
   way the fill reads the missed word and the words after it up to the line's
   end, and cache-control IBLKSZ is ignored. The missed word costs what the bus
   charges for it and each further word one cycle, the burst rate of RAM (4
   cycles plus one per word; ARCHITECTURE section 2, Costs), an approximation for
   other devices. A bus error on a later word ends the fill there: those words
   stay invalid and the bus's first-fault record is put back, because the CPU
   never asked for them. */
uint32_t schway__cpu_icache_fetch(schway *inst, uint32_t vaddr, uint32_t *word, uint32_t *cycles)
{
    struct schway__cpu *c = &inst->cpu;
    uint32_t p = vaddr & 0x1FFFFFFFu;
    uint32_t line = (p >> 4) & 0xFFu;
    uint32_t w = (p >> 2) & 3u;
    uint32_t tag = p >> 12;
    uint32_t base = vaddr & ~0xFu;
    uint32_t got = 0u;
    if (c->icache_tag[line] == tag && (c->icache_valid[line] & (1u << w)) != 0u) {
        *word = c->icache_word[line][w];
        return 0u;
    }
    if (schway__bus_fetch(inst, vaddr & ~3u, &got, cycles) != 0) {
        *word = 0u;
        return 1u;
    }
    if (c->icache_tag[line] != tag) {
        c->icache_tag[line] = tag;
        c->icache_valid[line] = 0u;
    }
    c->icache_word[line][w] = got;
    c->icache_valid[line] = (uint8_t)(c->icache_valid[line] | (1u << w));
    struct schway__fault held = inst->fault;
    for (uint32_t k = w + 1u; k < 4u; k++) {
        uint32_t sink = 0u;
        if (schway__bus_fetch(inst, base + 4u * k, &c->icache_word[line][k], &sink) != 0) {
            inst->fault = held;
            break;
        }
        c->icache_valid[line] = (uint8_t)(c->icache_valid[line] | (1u << k));
        *cycles += 1u;
    }
    *word = got;
    return 0u;
}

/* The trace ring (ARCHITECTURE section 6): the instance keeps the last
   SCHWAY__TRACE_CAP records in a ring outside the state and every hash, written
   only by the traced copy of the executor. A record's cycle is the tick the
   block started on plus the cycles the CPU has spent since, so t0 and c0 are
   the block's start; the frame is kept as its low 32 bits and widened when a
   record is read. A full ring overwrites its oldest record. */
static uint64_t trace_now(const schway *inst)
{
    return inst->trace.t0 + (inst->cpu.cycle - inst->trace.c0);
}

static void trace_push(schway *inst, uint32_t event, uint64_t cycle, uint32_t pc, uint32_t opcode,
                       uint32_t addr, uint32_t width, uint32_t device, uint32_t value,
                       uint32_t note)
{
    struct schway__trace_ring *ring = &inst->trace;
    uint32_t slot = ring->head;
    if (ring->count < SCHWAY__TRACE_CAP) {
        slot = (ring->head + ring->count) % SCHWAY__TRACE_CAP;
        ring->count++;
    } else {
        ring->head = (ring->head + 1u) % SCHWAY__TRACE_CAP;
    }
    struct schway__trace_rec *r = &ring->rec[slot];
    r->cycle = cycle;
    r->frame = (uint32_t)inst->frame;
    r->pc = pc;
    r->opcode = opcode;
    r->addr = addr;
    r->value = value;
    r->meta = SCHWAY__TRACE_META(event, width, device, note);
}

/* The executor, twice: untraced first, so the shared helpers live in that copy,
   then traced. The traced copy is the same code with its records. */
#define SCHWAY_TRACE 0
#include "cpu_exec.inc"
#undef SCHWAY_TRACE
#define SCHWAY_TRACE 1
#include "cpu_exec.inc"
#undef SCHWAY_TRACE

uint32_t schway__cpu_run(schway *inst, uint64_t cycles, uint32_t flags)
{
    uint64_t room = inst->frame_end - inst->ticks;
    uint64_t limit = cycles < room ? cycles : room;
    uint64_t ran = 0u;
    uint32_t stop = SCHWAY_STOP_BUDGET;
    /* The copy is chosen here, once for the whole call. */
    uint64_t (*block)(schway *, uint64_t) =
        (flags & SCHWAY_RUN_TRACE) != 0u ? cpu_block_traced : cpu_block;
    while (ran < limit) {
        ran += block(inst, limit - ran);
        if ((inst->cpu.block_end & SCHWAY__BLOCK_DOUBLE_FAULT) != 0u) {
            stop = SCHWAY_STOP_FAULT;
            break;
        }
        if (inst->exit_pending != 0u) {
            /* The report consumes the write; exit_code stays for the host. */
            inst->exit_pending = 0u;
            stop = SCHWAY_STOP_HOOK;
            break;
        }
        if ((flags & SCHWAY_RUN_STOP_ON_FAULT) != 0u && inst->fault.kind != SCHWAY__FAULT_NONE) {
            stop = SCHWAY_STOP_FAULT;
            break;
        }
    }
    inst->cpu.block_end = 0u;
    return stop;
}

void schway__cpu_step_one(schway *inst, struct schway__cpu_log *log)
{
    inst->cpu.log = log;
    (void)cpu_step(inst);
    inst->cpu.log = NULL;
}
