/* The physical bus: guest RAM, the scratchpad, memory control and the state
   walk. The access path decodes the virtual address by segment, then by one
   switch on address class. */
#include <string.h>

#include "state.h"

static const char name_ram[] = "bus.ram";
static const char name_scratch[] = "bus.scratch";
static const char name_memctl[] = "bus.memctl";
static const char name_ram_size[] = "bus.ram_size";
static const char name_cache_ctrl[] = "bus.cache_ctrl";
static const char name_unbuilt_n[] = "bus.unbuilt_n";
static const char name_last_dev[] = "bus.last_unbuilt.dev";
static const char name_last_addr[] = "bus.last_unbuilt.addr";
static const char name_last_width[] = "bus.last_unbuilt.width";
static const char name_last_pc[] = "bus.last_unbuilt.pc";
static const char name_irq_stat[] = "irq.stat";
static const char name_irq_mask[] = "irq.mask";

void schway__bus_reset(schway *inst)
{
    struct schway__bus *b = &inst->bus;
    memset(b->ram, 0, sizeof b->ram);
    memset(b->scratch, 0, sizeof b->scratch);
    memset(b->memctl, 0, sizeof b->memctl);
    b->ram_size = SCHWAY__RAM_SIZE_POWER_ON;
    b->cache_ctrl = 0u;
    memset(b->unbuilt_n, 0, sizeof b->unbuilt_n);
    memset(b->last_unbuilt, 0, sizeof b->last_unbuilt);
}

/* The counters and the last access are state, and the host's BIOS is not.
   The device and width of the last access are bounded, so a state never
   names a device the build does not know. */
void schway__bus_state_io(struct schway__state_io *io, schway *inst)
{
    struct schway__bus *b = &inst->bus;
    schway__state_u8_array(io, name_ram, b->ram, SCHWAY__RAM_SIZE, 0u, UINT8_MAX);
    schway__state_u8_array(io, name_scratch, b->scratch, SCHWAY__SCRATCH_SIZE, 0u, UINT8_MAX);
    schway__state_u32_array(io, name_memctl, b->memctl, SCHWAY__MEMCTL_REGS, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_ram_size, &b->ram_size, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_cache_ctrl, &b->cache_ctrl, 0u, UINT32_MAX);
    schway__state_u32_array(io, name_unbuilt_n, b->unbuilt_n, SCHWAY__DEV_COUNT, 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_last_dev, &b->last_unbuilt[0], 0u, SCHWAY__DEV_COUNT - 1u);
    (void)schway__state_u32(io, name_last_addr, &b->last_unbuilt[1], 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_last_width, &b->last_unbuilt[2], 0u, 4u);
    (void)schway__state_u32(io, name_last_pc, &b->last_unbuilt[3], 0u, UINT32_MAX);
    (void)schway__state_u32(io, name_irq_stat, &inst->irq.stat, 0u, SCHWAY__IRQ_BITS);
    (void)schway__state_u32(io, name_irq_mask, &inst->irq.mask, 0u, SCHWAY__IRQ_BITS);
}

/* Address classes of the physical map (PSX-SYSTEM section 3). */
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
   (PSX-SYSTEM section 3, RAM_SIZE; psx-spx Memory Control, DRAM_CTRL). */
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
    if (schway__bus_segment(addr) == (uint32_t)SCHWAY__SEG_KSEG2) {
        return gap(inst, kind, SCHWAY__DEV_UNMAPPED, addr, width);
    }
    uint32_t p = addr & 0x1FFFFFFFu;
    switch (classify(p)) {
    case CLS_RAM:
        return ram_access(inst, kind, addr, p, width, value, cycles);
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
