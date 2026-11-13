/* The bus: guest RAM, the scratchpad, the BIOS window, memory control with its
   cost table, devices not built yet, gaps and the debug ports, and the state
   walk. The access path decodes the virtual address by segment, then by one
   switch on address class, and inside the I/O page by one switch on bits 4-11
   (docs/hardware/bus.md; ARCHITECTURE section 2). */
#include <string.h>

#include "state.h"

static const char name_ram[] = "bus.ram";
static const char name_scratch[] = "bus.scratch";
static const char name_memctl[] = "bus.memctl";
static const char name_ram_size[] = "bus.ram_size";
static const char name_cache_ctrl[] = "bus.cache_ctrl";
static const char name_spu_attr[] = "bus.spu_attr";
static const char name_unbuilt_n[] = "bus.unbuilt_n";
static const char name_bios_loaded[] = "bus.bios.loaded";
static const char name_bios_sha256[] = "bus.bios.sha256";
static const char name_last_dev[] = "bus.last_unbuilt.dev";
static const char name_last_addr[] = "bus.last_unbuilt.addr";
static const char name_last_width[] = "bus.last_unbuilt.width";
static const char name_last_pc[] = "bus.last_unbuilt.pc";
static const char name_irq_stat[] = "irq.stat";
static const char name_irq_mask[] = "irq.mask";

/* Memory-control registers, in address order from 1F801000h: DEV0 base, DEV8
   base, the delay/size registers of DEV0, DEV1, DEV2 (BIOS), DEV4 (SPU), DEV5
   (CD-ROM) and DEV8, then COM_DELAY. Each port's delay register is memctl[2 +
   port] (docs/hardware/bus.md; psx-spx Memory Control). */
#define MEMCTL_COM 8u

/* Power-on: the bases hold their boot values, every port uses the maximum
   delays (both fields 0Fh and all four COM periods selected), and COM_DELAY
   holds its boot value 00031125h (docs/hardware/bus.md; PSX-SYSTEM section 9). */
static uint32_t memctl_power_on(uint32_t i)
{
    switch (i) {
    case 0u:
        return 0x1F000000u;
    case 1u:
        return 0x1F802000u;
    case MEMCTL_COM:
        return 0x00031125u;
    default:
        return 0x00000FFFu;
    }
}

/* The cost, in CPU cycles, of reading or writing a port with its delay
   register dev and COM_DELAY com, as the formula of psx-spx Memory Control
   gives it (docs/hardware/bus.md):
     1ST = SEQ = 0, MIN = 0
     COM0 selected (bit 8):  1ST, SEQ += COM0 - 1
     COM2 selected (bit 10): 1ST, SEQ += COM2
     COM3 selected (bit 11): MIN = COM3
     if 1ST < 6: 1ST += 1
     1ST, SEQ += delay + 2, where delay is the field plus one
     1ST = max(1ST, MIN + 6), SEQ = max(SEQ, MIN + 2)
   A byte or halfword on a 16-bit bus is 1ST; a 32-bit access on an 8-bit bus
   is 1ST + 3 SEQ and on a 16-bit bus 1ST + SEQ. The hold period (COM1, bit 9)
   is not in the published formula and adds nothing. */
static void port_cost(struct schway__bus *b, uint32_t port)
{
    uint32_t dev = b->memctl[2u + port];
    uint32_t com = b->memctl[MEMCTL_COM];
    int32_t com0 = (int32_t)(com & 15u);
    int32_t com2 = (int32_t)((com >> 8) & 15u);
    int32_t com3 = (int32_t)((com >> 12) & 15u);
    for (uint32_t wr = 0; wr < 2u; wr++) {
        int32_t delay = (int32_t)(((wr != 0u ? dev : dev >> 4) & 15u) + 1u);
        int32_t first = 0;
        int32_t seq = 0;
        int32_t min = 0;
        if ((dev & 0x100u) != 0u) {
            first += com0 - 1;
            seq += com0 - 1;
        }
        if ((dev & 0x400u) != 0u) {
            first += com2;
            seq += com2;
        }
        if ((dev & 0x800u) != 0u) {
            min = com3;
        }
        if (first < 6) {
            first += 1;
        }
        first += delay + 2;
        seq += delay + 2;
        if (first < min + 6) {
            first = min + 6;
        }
        if (seq < min + 2) {
            seq = min + 2;
        }
        for (uint32_t k = 0; k < 3u; k++) {
            uint32_t width = k == 2u ? 4u : k + 1u;
            int32_t n = (dev & 0x1000u) != 0u ? (int32_t)((width + 1u) / 2u) : (int32_t)width;
            b->cost[port][wr * 3u + k] = (uint8_t)(first + (n - 1) * seq);
        }
    }
}

static void port_costs(struct schway__bus *b)
{
    for (uint32_t port = 0; port < SCHWAY__PORT_COUNT; port++) {
        port_cost(b, port);
    }
}

void schway__bus_reset(schway *inst)
{
    struct schway__bus *b = &inst->bus;
    memset(b->ram, 0, sizeof b->ram);
    memset(b->scratch, 0, sizeof b->scratch);
    for (uint32_t i = 0; i < SCHWAY__MEMCTL_REGS; i++) {
        b->memctl[i] = memctl_power_on(i);
    }
    port_costs(b);
    b->ram_size = SCHWAY__RAM_SIZE_POWER_ON;
    b->cache_ctrl = 0u;
    b->spu_attr = 0u;
    memset(b->unbuilt_n, 0, sizeof b->unbuilt_n);
    memset(b->last_unbuilt, 0, sizeof b->last_unbuilt);
}

/* The counters and the last access are state. BIOS bytes remain host-owned;
   a state stores and validates only the loaded-image identity. The device and
   width of the last access are bounded, so a state never names an unknown device. */
void schway__bus_state_io(struct schway__state_io *io, schway *inst)
{
    struct schway__bus *b = &inst->bus;
    schway__state_u8_array(io, name_ram, b->ram, SCHWAY__RAM_SIZE, 0u, UINT8_MAX);
    schway__state_u8_array(io, name_scratch, b->scratch, SCHWAY__SCRATCH_SIZE, 0u, UINT8_MAX);
    schway__state_u32_array(io, name_memctl, b->memctl, SCHWAY__MEMCTL_REGS, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_ram_size, &b->ram_size, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_cache_ctrl, &b->cache_ctrl, 0u, UINT32_MAX);
    schway__state_u16_array(io, name_spu_attr, &b->spu_attr, 1u, 0u, UINT16_MAX);
    uint32_t bios_loaded = schway__state_u32(io, name_bios_loaded, &b->bios_loaded, 0u, 1u);
    schway__state_check(io, bios_loaded == b->bios_loaded);
    schway__state_match_u8_array(io, name_bios_sha256, b->bios_sha256,
                                 (uint32_t)sizeof b->bios_sha256);
    schway__state_u32_array(io, name_unbuilt_n, b->unbuilt_n, SCHWAY__DEV_COUNT, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_last_dev, &b->last_unbuilt[0], 0u, SCHWAY__DEV_COUNT - 1u);
    (void)schway__state_u32(io, name_last_addr, &b->last_unbuilt[1], 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_last_width, &b->last_unbuilt[2], 0u, 4u);
    (void)schway__state_u32(io, name_last_pc, &b->last_unbuilt[3], 0u, UINT32_MAX);
    uint32_t irq_stat = schway__state_u32(io, name_irq_stat, &inst->irq.stat, 0u, SCHWAY__IRQ_BITS);
    uint32_t irq_mask = schway__state_u32(io, name_irq_mask, &inst->irq.mask, 0u, SCHWAY__IRQ_BITS);
    if (io->mode == SCHWAY__STATE_VALIDATE) {
        io->irq_stat = irq_stat;
        io->irq_mask = irq_mask;
    }
    /* The cost table is derived from the delay registers, so a load rebuilds it. */
    if (io->mode == SCHWAY__STATE_APPLY) {
        port_costs(b);
    }
}

/* Address classes of the physical map (docs/hardware/bus.md). */
enum { CLS_GAP, CLS_RAM, CLS_SCRATCH, CLS_IO, CLS_EXP2, CLS_EXP1, CLS_BIOS, CLS_EXP3 };

static uint32_t classify(uint32_t p)
{
    if (p < 0x00800000u) {
        return CLS_RAM;
    }
    if ((p >> 10) == 0x7E000u) {
        return CLS_SCRATCH;
    }
    if ((p >> 12) == 0x1F801u) {
        return CLS_IO;
    }
    if ((p >> 13) == 0xFC01u) {
        return CLS_EXP2;
    }
    if ((p >> 23) == 0x3Eu) {
        return CLS_EXP1;
    }
    if ((p >> 19) == 0x3F8u) {
        return CLS_BIOS;
    }
    if ((p >> 21) == 0xFDu) {
        return CLS_EXP3;
    }
    return CLS_GAP;
}

/* The device a virtual address belongs to, for the trace. It decodes the same
   map as bus_access and the I/O page's switch, and changes nothing. */
uint32_t schway__bus_device(uint32_t addr)
{
    if (schway__bus_segment(addr) == (uint32_t)SCHWAY__SEG_KSEG2) {
        return addr == 0xFFFE0130u ? (uint32_t)SCHWAY__DEV_MEMCTL : (uint32_t)SCHWAY__DEV_UNMAPPED;
    }
    uint32_t p = addr & 0x1FFFFFFFu;
    switch (classify(p)) {
    case CLS_RAM:
        return SCHWAY__DEV_RAM;
    case CLS_SCRATCH:
        return SCHWAY__DEV_SCRATCH;
    case CLS_EXP2:
        return SCHWAY__DEV_EXP2;
    case CLS_EXP1:
        return SCHWAY__DEV_EXP1;
    case CLS_BIOS:
        return SCHWAY__DEV_BIOS;
    case CLS_EXP3:
        return SCHWAY__DEV_EXP3;
    case CLS_IO: {
        uint32_t slot = (p >> 4) & 0xFFu;
        if (slot <= 0x02u || slot == 0x06u) {
            return SCHWAY__DEV_MEMCTL;
        }
        if (slot == 0x04u || slot == 0x05u) {
            return SCHWAY__DEV_SIO;
        }
        if (slot == 0x07u) {
            return SCHWAY__DEV_IRQ;
        }
        if (slot >= 0x08u && slot <= 0x0Fu) {
            return SCHWAY__DEV_DMA;
        }
        if (slot >= 0x10u && slot <= 0x12u) {
            return SCHWAY__DEV_TIMER;
        }
        if (slot == 0x80u) {
            return SCHWAY__DEV_CDROM;
        }
        if (slot == 0x81u) {
            return SCHWAY__DEV_GPU;
        }
        if (slot == 0x82u) {
            return SCHWAY__DEV_MDEC;
        }
        return slot >= 0xC0u ? (uint32_t)SCHWAY__DEV_SPU : (uint32_t)SCHWAY__DEV_UNMAPPED;
    }
    default:
        return SCHWAY__DEV_UNMAPPED;
    }
}

enum { K_READ, K_WRITE, K_FETCH };

/* A bus error: the first one is kept as the first fault, naming the device
   the address belongs to. */
static int gap(schway *inst, uint32_t kind, uint32_t dev, uint32_t addr, uint32_t width)
{
    uint32_t f = kind == K_WRITE   ? SCHWAY__FAULT_UNMAPPED_WRITE
                 : kind == K_FETCH ? SCHWAY__FAULT_UNMAPPED_FETCH
                                   : SCHWAY__FAULT_UNMAPPED_READ;
    schway__fault_latch(inst, f, dev, width, inst->cpu.pc, 0u, addr);
    return 1;
}

/* Where byte a of the 8 MiB RAM window lands in the 2 MiB of RAM, or
   UINT32_MAX for a bus error. The two banks follow each other: /RAS0 holds
   1, 2, 4 or 8 MiB from bits 9 and 11 of RAM_SIZE, /RAS1 the same from bits
   8 and 11 when bit 10 enables it, and a bank larger than the RAM mirrors it
   (docs/hardware/bus.md, Memory control; psx-spx Memory Control, DRAM_CTRL). */
static uint32_t ram_locate(uint32_t rs, uint32_t a)
{
    uint32_t size0 = UINT32_C(0x100000) << (((rs >> 8) & 2u) | ((rs >> 11) & 1u));
    uint32_t keep0 = (size0 < SCHWAY__RAM_SIZE ? size0 : SCHWAY__RAM_SIZE) - 1u;
    if (a < size0) {
        return a & keep0;
    }
    if (((rs >> 10) & 1u) != 0u) {
        uint32_t size1 = UINT32_C(0x100000) << (((rs >> 7) & 2u) | ((rs >> 11) & 1u));
        uint32_t keep1 = (size1 < SCHWAY__RAM_SIZE ? size1 : SCHWAY__RAM_SIZE) - 1u;
        if (a - size0 < size1) {
            return (keep0 + 1u + ((a - size0) & keep1)) & (SCHWAY__RAM_SIZE - 1u);
        }
    }
    return UINT32_MAX;
}

/* The DMA engine's path to RAM: one aligned word at physical word address
   addr in the 8 MiB RAM window, assembled and stored little-endian a byte at a
   time. It raises no fault, charges no cycle, counts no unbuilt access and never
   waits on a DMA unit. An address that RAM_SIZE leaves without RAM drops the
   word, reads nothing and returns non-zero; it does not touch DICR (a DMA
   word past RAM is not modelled as a bus error; docs/hardware/dma.md). The low
   two address bits are ignored, as MADR's are. */
int schway__bus_ram_word(schway *inst, uint32_t addr, uint32_t *word, uint32_t write)
{
    uint32_t off = ram_locate(inst->bus.ram_size, addr & ~UINT32_C(3));
    if (off == UINT32_MAX) {
        return 1;
    }
    uint8_t *ram = inst->bus.ram;
    if (write != 0u) {
        for (uint32_t i = 0; i < 4u; i++) {
            ram[off + i] = (uint8_t)(*word >> (8u * i));
        }
        return 0;
    }
    uint32_t v = 0u;
    for (uint32_t i = 0; i < 4u; i++) {
        v |= (uint32_t)ram[off + i] << (8u * i);
    }
    *word = v;
    return 0;
}

/* RAM costs 4 cycles plus one per word read, and 1 per write (ARCHITECTURE
   section 2, Costs). */
#define RAM_READ_COST 5u
#define RAM_WRITE_COST 1u

static int ram_access(schway *inst, uint32_t kind, uint32_t addr, uint32_t p, uint32_t width,
                      uint32_t *value, uint32_t *cycles)
{
    uint32_t rs = inst->bus.ram_size;
    uint32_t off[4];
    /* An aligned access sits inside one bank; an unaligned one may cross
       into the next bank or off the end of the window, so each byte is
       located on its own. */
    if ((p & (width - 1u)) == 0u) {
        off[0] = ram_locate(rs, p);
        if (off[0] == UINT32_MAX) {
            return gap(inst, kind, SCHWAY__DEV_RAM, addr, width);
        }
        for (uint32_t i = 1; i < width; i++) {
            off[i] = off[0] + i;
        }
    } else {
        for (uint32_t i = 0; i < width; i++) {
            off[i] = ram_locate(rs, p + i);
            if (off[i] == UINT32_MAX) {
                return gap(inst, kind, SCHWAY__DEV_RAM, addr, width);
            }
        }
    }
    uint8_t *ram = inst->bus.ram;
    if (kind == K_WRITE) {
        for (uint32_t i = 0; i < width; i++) {
            ram[off[i]] = (uint8_t)(*value >> (8u * i));
        }
        *cycles += RAM_WRITE_COST;
        return 0;
    }
    uint32_t v = 0u;
    for (uint32_t i = 0; i < width; i++) {
        v |= (uint32_t)ram[off[i]] << (8u * i);
    }
    *value = v;
    *cycles += RAM_READ_COST;
    return 0;
}

/* The bytes of a register an access covers: the lane of width bytes at the
   address's offset in its 32-bit register. */
static uint32_t lane_shift(uint32_t addr)
{
    return 8u * (addr & 3u);
}

static uint32_t lane_mask(uint32_t width)
{
    return width >= 4u ? 0xFFFFFFFFu : (UINT32_C(1) << (8u * width)) - 1u;
}

static uint32_t reg_get(uint32_t reg, uint32_t addr, uint32_t width)
{
    return (reg >> lane_shift(addr)) & lane_mask(width);
}

static uint32_t reg_put(uint32_t old, uint32_t addr, uint32_t width, uint32_t value)
{
    uint32_t m = lane_mask(width) << lane_shift(addr);
    return (old & ~m) | ((value << lane_shift(addr)) & m);
}

/* Counts an access to a device not built yet and keeps the last one (the
   device, address, width and program counter). The access itself reads 0 and
   drops a write; the caller does that. */
void schway__bus_unbuilt(schway *inst, uint32_t dev, uint32_t addr, uint32_t width, uint32_t wr,
                         uint32_t val)
{
    struct schway__bus *b = &inst->bus;
    (void)wr;
    (void)val;
    if (dev < SCHWAY__DEV_COUNT && b->unbuilt_n[dev] != UINT32_MAX) {
        b->unbuilt_n[dev]++;
    }
    b->last_unbuilt[0] = dev;
    b->last_unbuilt[1] = addr;
    b->last_unbuilt[2] = width;
    b->last_unbuilt[3] = inst->cpu.pc;
}

/* Adds a port's charge for one access. */
static void charge_port(const struct schway__bus *b, uint32_t port, uint32_t kind, uint32_t width,
                        uint32_t *cycles)
{
    *cycles += b->cost[port][(kind == K_WRITE ? 3u : 0u) + (width == 4u ? 2u : width - 1u)];
}

/* Registers with no delay register of their own cost one cycle (approximate;
   ARCHITECTURE section 2, Costs). */
#define IO_FLAT_COST 1u

static void spu_catch_up(schway *inst, uint32_t *cycles)
{
    uint64_t tick = schway__sched_now(inst, *cycles);
    if (schway__spu_next(inst) <= tick) {
        schway__spu_event(inst, tick);
    }
}

static uint32_t spu_write_halfword(schway *inst, uint32_t address, uint16_t value,
                                   uint32_t *cycles)
{
    uint32_t aligned = address & ~1u;
    if (!schway__spu_reg_supported(aligned)) {
        return 0u;
    }
    spu_catch_up(inst, cycles);
    if (aligned == 0x1F801DAAu) {
        uint64_t tick = schway__sched_now(inst, *cycles);
        schway__dma_event(inst, tick);
        inst->bus.spu_attr = value;
        schway__dma_spu_control_changed(inst, tick);
    }
    schway__spu_reg_write(inst, aligned, value);
    return 1u;
}

static uint16_t spu_read_halfword(schway *inst, uint32_t address, uint32_t *cycles,
                                  uint32_t *supported)
{
    uint32_t aligned = address & ~1u;
    if (!schway__spu_reg_supported(aligned)) {
        *supported = 0u;
        return 0u;
    }
    *supported = 1u;
    spu_catch_up(inst, cycles);
    return schway__spu_reg_read(inst, aligned);
}

static void spu_io(schway *inst, uint32_t kind, uint32_t address, uint32_t width,
                   uint32_t *value, uint32_t *cycles)
{
    uint32_t unbuilt = 0u;
    if (kind == K_WRITE) {
        if (width == 1u) {
            if ((address & 1u) == 0u) {
                unbuilt = spu_write_halfword(inst, address, (uint16_t)*value, cycles) == 0u;
            } else if (!schway__spu_reg_supported(address)) {
                unbuilt = 1u;
            }
            if (unbuilt != 0u) {
                schway__bus_unbuilt(inst, SCHWAY__DEV_SPU, address, width, 1u, *value);
            }
            return;
        }
        for (uint32_t offset = 0u; offset < width; offset += 2u) {
            if (spu_write_halfword(inst, address + offset,
                                   (uint16_t)(*value >> (offset * 8u)), cycles) == 0u) {
                unbuilt = 1u;
            }
        }
        if (unbuilt != 0u) {
            schway__bus_unbuilt(inst, SCHWAY__DEV_SPU, address, width, 1u, *value);
        }
        return;
    }

    uint32_t result = 0u;
    if (width == 1u) {
        uint32_t supported = 0u;
        uint16_t halfword = spu_read_halfword(inst, address, cycles, &supported);
        unbuilt = supported == 0u;
        result = (halfword >> ((address & 1u) * 8u)) & 0xFFu;
    } else {
        for (uint32_t offset = 0u; offset < width; offset += 2u) {
            uint32_t supported = 0u;
            uint16_t halfword = spu_read_halfword(inst, address + offset, cycles, &supported);
            if (supported == 0u) {
                unbuilt = 1u;
            }
            result |= (uint32_t)halfword << (offset * 8u);
        }
    }
    *value = result;
    if (unbuilt != 0u) {
        schway__bus_unbuilt(inst, SCHWAY__DEV_SPU, address, width, 0u, 0u);
    }
}

/* A memory-control register, by index into memctl: a write keeps the bits the
   register has, and a delay write or COM_DELAY write rebuilds the costs. */
static void memctl_access(schway *inst, uint32_t kind, uint32_t idx, uint32_t addr, uint32_t width,
                          uint32_t *value)
{
    struct schway__bus *b = &inst->bus;
    if (kind != K_WRITE) {
        *value = reg_get(b->memctl[idx], addr, width);
        return;
    }
    uint32_t v = reg_put(b->memctl[idx], addr, width, *value);
    if (idx < 2u) {
        v = 0x1F000000u | (v & 0x00FFFFFFu);
    } else if (idx == MEMCTL_COM) {
        v &= 0x000FFFFFu;
    } else {
        v &= 0xEF1FFFFFu;
    }
    b->memctl[idx] = v;
    if (idx == MEMCTL_COM) {
        port_costs(b);
    } else if (idx >= 2u) {
        port_cost(b, idx - 2u);
    }
}

/* One access to the I/O page at physical address p. Documented devices not
   built yet are one switch case each (they read 0 and drop writes); the holes
   between registers are bus errors. */
static int io_access(schway *inst, uint32_t kind, uint32_t addr, uint32_t p, uint32_t width,
                     uint32_t *value, uint32_t *cycles)
{
    uint32_t slot = (p >> 4) & 0xFFu;
    uint32_t wr = kind == K_WRITE ? 1u : 0u;
    uint32_t v = wr != 0u ? *value : 0u;
    uint32_t port = SCHWAY__PORT_COUNT;
    switch (slot) {
    case 0x00u:
    case 0x01u:
        memctl_access(inst, kind, (p >> 2) & 7u, p, width, value);
        break;
    case 0x02u:
        if ((p & 0xCu) != 0u) {
            return gap(inst, kind, SCHWAY__DEV_UNMAPPED, addr, width);
        }
        memctl_access(inst, kind, MEMCTL_COM, p, width, value);
        break;
    case 0x04u:
    case 0x05u:
        if (p >= 0x1F801040u && p < 0x1F801050u) {
            if (schway__sio_io(inst, wr, p, width, value, cycles) != 0) {
                return gap(inst, kind, SCHWAY__DEV_UNMAPPED, addr, width);
            }
            break;
        }
        schway__bus_unbuilt(inst, SCHWAY__DEV_SIO, p, width, wr, v);
        break;
    case 0x06u:
        if ((p & 0xCu) != 0u) {
            return gap(inst, kind, SCHWAY__DEV_UNMAPPED, addr, width);
        }
        if (wr != 0u) {
            inst->bus.ram_size = reg_put(inst->bus.ram_size, p, width, v) & 0xFFFFu;
        } else {
            *value = reg_get(inst->bus.ram_size, p, width);
        }
        break;
    case 0x07u:
        if ((p & 0x8u) != 0u) {
            return gap(inst, kind, SCHWAY__DEV_UNMAPPED, addr, width);
        }
        /* The root counters count lazily, so an I_STAT access first brings
           them up to the tick of this access: an interrupt edge due by then
           is in the register a read returns, and what happened while a bit
           was set is behind a write that acknowledges it. */
        if ((p & 4u) == 0u) {
            uint64_t tick = schway__sched_now(inst, *cycles);
            schway__gpu_sync(inst, tick);
            schway__timers_sync(inst, tick);
        }
        if (wr == 0u) {
            *value = reg_get((p & 4u) != 0u ? inst->irq.mask : inst->irq.stat, p, width);
        } else if ((p & 4u) != 0u) {
            schway__irq_write_mask(inst, v << lane_shift(p), lane_mask(width) << lane_shift(p));
        } else {
            schway__irq_write_stat(inst, v << lane_shift(p), lane_mask(width) << lane_shift(p));
        }
        break;
    case 0x08u:
    case 0x09u:
    case 0x0Au:
    case 0x0Bu:
    case 0x0Cu:
    case 0x0Du:
    case 0x0Eu:
    case 0x0Fu:
        schway__dma_io(inst, wr, p, width, value, cycles);
        break;
    case 0x10u:
    case 0x11u:
    case 0x12u:
        schway__timers_io(inst, wr, p, width, value, cycles);
        break;
    case 0x80u:
        if ((p & 0xCu) != 0u) {
            return gap(inst, kind, SCHWAY__DEV_UNMAPPED, addr, width);
        }
        schway__bus_unbuilt(inst, SCHWAY__DEV_CDROM, p, width, wr, v);
        port = SCHWAY__PORT_CDROM;
        break;
    case 0x81u:
        if ((p & 0x8u) != 0u) {
            return gap(inst, kind, SCHWAY__DEV_UNMAPPED, addr, width);
        }
        if (schway__gpu_io(inst, wr, p, width, value, cycles) != 0) {
            return gap(inst, kind, SCHWAY__DEV_UNMAPPED, addr, width);
        }
        return 0;
    case 0x82u:
        if ((p & 0x8u) != 0u) {
            return gap(inst, kind, SCHWAY__DEV_UNMAPPED, addr, width);
        }
        schway__bus_unbuilt(inst, SCHWAY__DEV_MDEC, p, width, wr, v);
        break;
    default:
        if (slot < 0xC0u) {
            return gap(inst, kind, SCHWAY__DEV_UNMAPPED, addr, width);
        }
        spu_io(inst, kind, p, width, value, cycles);
        port = SCHWAY__PORT_SPU;
        break;
    }
    if (port < SCHWAY__PORT_COUNT) {
        charge_port(&inst->bus, port, kind, width, cycles);
    } else {
        *cycles += IO_FLAT_COST;
    }
    return 0;
}

/* The byte at the BIOS window, assembled little-endian; a gap while no
   image is mapped or the access runs past it. A write to the ROM is dropped. */
static int bios_access(schway *inst, uint32_t kind, uint32_t addr, uint32_t p, uint32_t width,
                       uint32_t *value, uint32_t *cycles)
{
    const struct schway__bus *b = &inst->bus;
    uint32_t off = p - 0x1FC00000u;
    if (b->bios == NULL || off >= b->bios_size || width > b->bios_size - off) {
        return gap(inst, kind, SCHWAY__DEV_BIOS, addr, width);
    }
    if (kind != K_WRITE) {
        uint32_t v = 0u;
        for (uint32_t i = 0; i < width; i++) {
            v |= (uint32_t)b->bios[off + i] << (8u * i);
        }
        *value = v;
    }
    charge_port(b, SCHWAY__PORT_BIOS, kind, width, cycles);
    return 0;
}

/* The second expansion region has two profile-controlled debug ports and one
   source-identified POST byte sink; every other access is a gap. */
static int exp2_access(schway *inst, uint32_t kind, uint32_t addr, uint32_t p, uint32_t width,
                       uint32_t *value, uint32_t *cycles)
{
    if (p == 0x1F802041u && kind == K_WRITE && width == 1u) {
        /* OpenBIOS writes this byte before startup; this ignored compatibility
           sink is documented with its pinned source in docs/hardware/bus.md. */
    } else if (p != 0x1F802080u && p != 0x1F802082u) {
        return gap(inst, kind, SCHWAY__DEV_EXP2, addr, width);
    }
    if (p != 0x1F802041u && kind == K_WRITE && width == 1u &&
        (inst->profile_flags & SCHWAY__PROFILE_DEBUG_PORT) != 0u) {
        if (p == 0x1F802080u) {
            schway__tty_push(inst, (uint8_t)*value);
        } else {
            inst->exit_pending = 1u;
            inst->exit_code = *value & 0xFFu;
        }
    }
    charge_port(&inst->bus, SCHWAY__PORT_EXP2, kind, width, cycles);
    return 0;
}

/* One access. For a write, *value holds the data. Guest bytes are
   little-endian whatever the host is, so every access is assembled from
   single bytes. */
static int bus_access(schway *inst, uint32_t kind, uint32_t addr, uint32_t width,
                      uint32_t *value, uint32_t *cycles)
{
    uint32_t sink = 0u;
    if (cycles == NULL) {
        cycles = &sink;
    }
    if (kind != K_WRITE) {
        *value = 0u;
    }
    if (width != 1u && width != 2u && width != 4u) {
        return 1;
    }
    /* KSEG2 answers only the cache-control word, by word reads and writes. */
    if (schway__bus_segment(addr) == (uint32_t)SCHWAY__SEG_KSEG2) {
        if (addr != 0xFFFE0130u || width != 4u || kind == K_FETCH) {
            return gap(inst, kind, SCHWAY__DEV_UNMAPPED, addr, width);
        }
        if (kind == K_WRITE) {
            inst->bus.cache_ctrl = *value;
        } else {
            *value = inst->bus.cache_ctrl;
        }
        return 0;
    }
    uint32_t p = addr & 0x1FFFFFFFu;
    uint32_t cls = classify(p);
    /* A read or an instruction-fetch miss that reaches RAM, the I/O page, an
       expansion region or the BIOS waits for a running DMA unit, so the engine
       is told first, at the tick of this access. A store does not tell it: it
       goes into the 4-deep write queue and only the fifth queued store waits,
       which the CPU's queue push reports. The scratchpad, the cache-control
       word and a gap are not bus accesses that wait. */
    if (kind != K_WRITE && cls != CLS_SCRATCH && cls != CLS_GAP) {
        schway__dma_bus_access(inst, schway__sched_now(inst, *cycles));
    }
    switch (cls) {
    case CLS_RAM:
        return ram_access(inst, kind, addr, p, width, value, cycles);
    case CLS_SCRATCH: {
        /* The D-cache SRAM: data in KUSEG and KSEG0, a gap in KSEG1, and
           never fetched from (docs/hardware/bus.md, The scratchpad). */
        uint32_t off = p & (SCHWAY__SCRATCH_SIZE - 1u);
        if (schway__bus_segment(addr) == (uint32_t)SCHWAY__SEG_KSEG1 || kind == K_FETCH ||
            width > SCHWAY__SCRATCH_SIZE - off) {
            return gap(inst, kind, SCHWAY__DEV_SCRATCH, addr, width);
        }
        uint8_t *sp = &inst->bus.scratch[off];
        if (kind == K_WRITE) {
            for (uint32_t i = 0; i < width; i++) {
                sp[i] = (uint8_t)(*value >> (8u * i));
            }
        } else {
            uint32_t v = 0u;
            for (uint32_t i = 0; i < width; i++) {
                v |= (uint32_t)sp[i] << (8u * i);
            }
            *value = v;
        }
        return 0;
    }
    case CLS_IO:
        return io_access(inst, kind, addr, p, width, value, cycles);
    case CLS_EXP2:
        return exp2_access(inst, kind, addr, p, width, value, cycles);
    case CLS_EXP1:
    case CLS_EXP3: {
        /* Reads and fetches find nothing and are bus errors; a write is
           dropped (docs/hardware/bus.md, Segments and the physical map). */
        uint32_t dev = cls == CLS_EXP1 ? SCHWAY__DEV_EXP1 : SCHWAY__DEV_EXP3;
        if (kind != K_WRITE) {
            return gap(inst, kind, dev, addr, width);
        }
        charge_port(&inst->bus, dev == SCHWAY__DEV_EXP1 ? SCHWAY__PORT_EXP1 : SCHWAY__PORT_EXP3,
                    kind, width, cycles);
        return 0;
    }
    case CLS_BIOS:
        return bios_access(inst, kind, addr, p, width, value, cycles);
    default:
        return gap(inst, kind, SCHWAY__DEV_UNMAPPED, addr, width);
    }
}

int schway__bus_read(schway *inst, uint32_t addr, uint32_t width, uint32_t *value,
                     uint32_t *cycles)
{
    return bus_access(inst, K_READ, addr, width, value, cycles);
}

int schway__bus_write(schway *inst, uint32_t addr, uint32_t width, uint32_t value,
                      uint32_t *cycles)
{
    return bus_access(inst, K_WRITE, addr, width, &value, cycles);
}

int schway__bus_fetch(schway *inst, uint32_t addr, uint32_t *word, uint32_t *cycles)
{
    return bus_access(inst, K_FETCH, addr, 4u, word, cycles);
}
