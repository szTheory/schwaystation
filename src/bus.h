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

/* Memory control: the registers at 1F801000h-1F801020h (eight) and RAM_SIZE
   at 1F801060h (PSX-SYSTEM section 3). */
#define SCHWAY__MEMCTL_REGS 9u

struct schway__bus {
    uint8_t ram[SCHWAY__RAM_SIZE];
    uint8_t scratch[SCHWAY__SCRATCH_SIZE];
    uint32_t memctl[SCHWAY__MEMCTL_REGS];
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

/* One access to physical address paddr of width 1, 2 or 4 bytes, assembled
   little-endian. Returns 0 when done, with the cost in CPU cycles added to
   *cycles, and non-zero for a bus error, with *value left 0. */
int schway__bus_read(schway *inst, uint32_t paddr, uint32_t width, uint32_t *value,
                     uint32_t *cycles);
int schway__bus_write(schway *inst, uint32_t paddr, uint32_t width, uint32_t value,
                      uint32_t *cycles);
/* An instruction fetch of the word at paddr. */
int schway__bus_fetch(schway *inst, uint32_t paddr, uint32_t *word, uint32_t *cycles);

/* Sets interrupt bit number bit (0-10) in I_STAT; other numbers are ignored.
   The line is nonzero while an enabled bit is set. */
void schway__irq_raise(schway *inst, uint32_t bit);
uint32_t schway__irq_line(const schway *inst);

#endif /* SCHWAYSTATION_BUS_H */
