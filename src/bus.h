/* The physical bus, guest memory and the interrupt registers: state structs
   and the module's declarations. Not installed. The instance embeds the
   structs, so each module's source file is edited alone. */
#ifndef SCHWAYSTATION_BUS_H
#define SCHWAYSTATION_BUS_H

#include <stdint.h>

#include "schwaystation.h"

struct schway__state_io;

/* The device an access reached, named in the first-fault record and counted
   for unbuilt devices. */
enum schway__dev {
    SCHWAY__DEV_NONE = 0,
    SCHWAY__DEV_RAM,
    SCHWAY__DEV_SCRATCH,
    SCHWAY__DEV_BIOS,
    SCHWAY__DEV_MEMCTL,
    SCHWAY__DEV_IRQ,
    SCHWAY__DEV_DMA,
    SCHWAY__DEV_TIMER,
    SCHWAY__DEV_CDROM,
    SCHWAY__DEV_GPU,
    SCHWAY__DEV_MDEC,
    SCHWAY__DEV_SIO,
    SCHWAY__DEV_SPU,
    SCHWAY__DEV_EXP1,
    SCHWAY__DEV_EXP2,
    SCHWAY__DEV_EXP3,
    SCHWAY__DEV_GTE,
    SCHWAY__DEV_UNMAPPED,
    SCHWAY__DEV_COUNT
};

/* Main RAM is 2 MiB, the scratchpad 1 KiB (ARCHITECTURE section 2). */
#define SCHWAY__RAM_SIZE 0x200000u
#define SCHWAY__SCRATCH_SIZE 0x400u

/* Memory control: the nine registers at 1F801000h-1F801020h and RAM_SIZE at
   1F801060h (PSX-SYSTEM section 3). */
#define SCHWAY__MEMCTL_REGS 9u

/* RAM_SIZE after a power reset: a 2 MiB RAM in an 8 MiB bank, mirrored four
   times, as the BIOS sets it (PSX-SYSTEM section 3). */
#define SCHWAY__RAM_SIZE_POWER_ON 0x0B88u

/* The ports with a delay register of their own: the first and third
   expansion regions, the BIOS ROM, the SPU, the CD-ROM and the second
   expansion region (DEV0, DEV1, DEV2, DEV4, DEV5, DEV8). */
enum schway__port {
    SCHWAY__PORT_EXP1 = 0,
    SCHWAY__PORT_EXP3,
    SCHWAY__PORT_BIOS,
    SCHWAY__PORT_SPU,
    SCHWAY__PORT_CDROM,
    SCHWAY__PORT_EXP2,
    SCHWAY__PORT_COUNT
};

/* Bit 0 of the profile flags: the guest's writes to the debug ports at
   1F802080h and 1F802082h take effect. */
#define SCHWAY__PROFILE_DEBUG_PORT 1u

struct schway__bus {
    uint8_t ram[SCHWAY__RAM_SIZE];
    uint8_t scratch[SCHWAY__SCRATCH_SIZE];
    uint32_t memctl[SCHWAY__MEMCTL_REGS];
    /* CPU cycles per port for {read8, read16, read32, write8, write16,
       write32}, derived from memctl and rebuilt by a delay-register write and
       by a state load: not part of the state. */
    uint8_t cost[SCHWAY__PORT_COUNT][6];
    uint32_t ram_size;   /* RAM_SIZE, 1F801060h */
    uint32_t cache_ctrl; /* cache control, FFFE0130h */
    /* Accesses to devices not built yet, per device, and the last one: its
       device, address, width and the program counter. */
    uint32_t unbuilt_n[SCHWAY__DEV_COUNT];
    uint32_t last_unbuilt[4];
    /* The host's BIOS image: outside the state. */
    const uint8_t *bios;
    uint32_t bios_size;
};

/* I_STAT and I_MASK, bits 0-10 (PSX-SYSTEM section 6). */
#define SCHWAY__IRQ_BITS 0x7FFu

struct schway__irq {
    uint32_t stat, mask;
};

/* Power-on values: RAM, scratchpad, memory control, counters and interrupt
   registers are zero. The host's BIOS pointer is kept. */
void schway__bus_reset(schway *inst);
void schway__irq_reset(schway *inst);

/* The bus and irq fields of the state walk (state.h). */
void schway__bus_state_io(struct schway__state_io *io, schway *inst);

/* The segment of a virtual address: the top three bits, with 0-3 all KUSEG
   and 6-7 both KSEG2 (PSX-SYSTEM section 2, Segments). KUSEG, KSEG0 and KSEG1
   map to the physical address addr & 1FFFFFFFh. */
enum { SCHWAY__SEG_KUSEG = 0, SCHWAY__SEG_KSEG0 = 4, SCHWAY__SEG_KSEG1 = 5, SCHWAY__SEG_KSEG2 = 6 };

static inline uint32_t schway__bus_segment(uint32_t addr)
{
    uint32_t top = addr >> 29;
    return top < 4u ? (uint32_t)SCHWAY__SEG_KUSEG : (top > 6u ? 6u : top);
}

/* One access to virtual address addr of width 1, 2 or 4 bytes, assembled
   little-endian. Returns 0 when done, with the cost in CPU cycles added to
   *cycles, and non-zero for a bus error, with *value left 0. */
int schway__bus_read(schway *inst, uint32_t addr, uint32_t width, uint32_t *value,
                     uint32_t *cycles);
int schway__bus_write(schway *inst, uint32_t addr, uint32_t width, uint32_t value,
                      uint32_t *cycles);
/* An instruction fetch of the word at addr. */
int schway__bus_fetch(schway *inst, uint32_t addr, uint32_t *word, uint32_t *cycles);

/* The device the physical map gives to virtual address addr, for a trace
   record: a SCHWAY__DEV_* value, UNMAPPED for a gap. It decodes only and
   touches nothing. */
uint32_t schway__bus_device(uint32_t addr);

/* Counts an access to a device not built yet, by device, and keeps the last
   one. The caller reads 0 or drops the write. */
void schway__bus_unbuilt(schway *inst, uint32_t dev, uint32_t addr, uint32_t width, uint32_t wr,
                         uint32_t val);

/* Sets interrupt bit number bit (0-10) in I_STAT; other numbers are ignored.
   A bit already set stays as it is. The line is nonzero while an enabled bit
   is set, and CAUSE bit 10 follows it at each change. */
void schway__irq_raise(schway *inst, uint32_t bit);

/* A write to I_STAT or I_MASK: lane holds the data in place, and mask names
   the bytes the access covers. A 0 bit written to I_STAT clears it and a 1
   leaves it; I_MASK keeps the bits written, 0-10 only. */
void schway__irq_write_stat(schway *inst, uint32_t lane, uint32_t mask);
void schway__irq_write_mask(schway *inst, uint32_t lane, uint32_t mask);
uint32_t schway__irq_line(const schway *inst);

#endif /* SCHWAYSTATION_BUS_H */
