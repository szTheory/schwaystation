/* The physical bus: guest RAM, the scratchpad, memory control and the state
   walk. Only RAM answers so far; every other address is a bus error until
   the bus decodes the rest of the map. */
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
    b->ram_size = 0u;
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

/* True when width bytes at paddr lie inside main RAM, 0 to 1FFFFFh. */
static int in_ram(uint32_t paddr, uint32_t width)
{
    return width >= 1u && width <= 4u && paddr < SCHWAY__RAM_SIZE &&
           width <= SCHWAY__RAM_SIZE - paddr;
}

/* Guest bytes are little-endian whatever the host is, so every access is
   assembled from single bytes. Costs are charged by the timing model (cycles
   are left unchanged here). */
int schway__bus_read(schway *inst, uint32_t paddr, uint32_t width, uint32_t *value,
                     uint32_t *cycles)
{
    (void)cycles;
    *value = 0u;
    if (!in_ram(paddr, width)) {
        return 1;
    }
    const uint8_t *p = &inst->bus.ram[paddr];
    uint32_t v = 0u;
    for (uint32_t i = 0; i < width; i++) {
        v |= (uint32_t)p[i] << (8u * i);
    }
    *value = v;
    return 0;
}

int schway__bus_write(schway *inst, uint32_t paddr, uint32_t width, uint32_t value,
                      uint32_t *cycles)
{
    (void)cycles;
    if (!in_ram(paddr, width)) {
        return 1;
    }
    uint8_t *p = &inst->bus.ram[paddr];
    for (uint32_t i = 0; i < width; i++) {
        p[i] = (uint8_t)(value >> (8u * i));
    }
    return 0;
}

int schway__bus_fetch(schway *inst, uint32_t paddr, uint32_t *word, uint32_t *cycles)
{
    return schway__bus_read(inst, paddr, 4u, word, cycles);
}
