/* PS-EXE validation, probing and side-loading. One validator serves the probe
   and the loader, so a file the probe accepts is a file the loader accepts.
   The header layout is psx-spx, "PSX Executable Formats": the signature at
   00h, then little-endian words pc0 at 10h, gp0 14h, t_addr 18h, t_size 1Ch,
   d_addr 20h, d_size 24h, b_addr 28h, b_size 2Ch, s_addr 30h and s_size 34h,
   and the region text at 4Ch; the text segment follows the 0800h-byte header. */
#include <string.h>

#include "internal.h"

#define EXE_HEADER 0x800u
#define EXE_LICENCE_OFFSET 0x4Cu

/* Where the loader leaves the launch block for the boot stub: pc0, gp0,
   s_addr and s_size as little-endian words, in kernel RAM clear of the
   exception and call vectors at 80h to FFh. */
#define EXE_LAUNCH_BLOCK 0x200u

static const uint8_t exe_magic[8] = {'P', 'S', '-', 'X', ' ', 'E', 'X', 'E'};

/* The region texts a licensed executable carries at 4Ch (PSXDEV-EXTRACTS). */
static const char licence_japan[] = "Sony Computer Entertainment Inc. for Japan area";
static const char licence_europe[] = "Sony Computer Entertainment Inc. for Europe area";
static const char licence_america[] = "Sony Computer Entertainment Inc. for North America area";

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void put_le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static uint32_t licence_of(const uint8_t *b)
{
    const uint8_t *text = b + EXE_LICENCE_OFFSET;
    if (memcmp(text, licence_japan, sizeof licence_japan - 1u) == 0) {
        return SCHWAY_LICENCE_JAPAN;
    }
    if (memcmp(text, licence_europe, sizeof licence_europe - 1u) == 0) {
        return SCHWAY_LICENCE_EUROPE;
    }
    if (memcmp(text, licence_america, sizeof licence_america - 1u) == 0) {
        return SCHWAY_LICENCE_NORTH_AMERICA;
    }
    return SCHWAY_LICENCE_NONE;
}

/* The physical address of a KUSEG, KSEG0 or KSEG1 virtual address, or
   UINT64_MAX in KSEG2, which maps no RAM. */
static uint64_t physical(uint32_t addr)
{
    return schway__bus_segment(addr) >= 6u ? UINT64_MAX : (uint64_t)(addr & 0x1FFFFFFFu);
}

/* Checks the whole header against the file's n bytes, in 64 bits, before any
   caller writes: n in 0800h to 0800h + 2 MiB, text a whole number of 0800h
   sectors that fits the file and RAM, word-aligned addresses, a non-zero
   entry point and a stack that stays in RAM. On OK fills info. */
static schway_status validate(const uint8_t *b, uint64_t n, schway_exe_info *info)
{
    if (n < EXE_HEADER || memcmp(b, exe_magic, sizeof exe_magic) != 0) {
        return SCHWAY_STATUS_ERR_EXE_INVALID;
    }
    if (n > (uint64_t)EXE_HEADER + SCHWAY__RAM_SIZE) {
        return SCHWAY_STATUS_ERR_EXE_RANGE;
    }
    schway_exe_info e;
    memset(&e, 0, sizeof e);
    e.pc0 = le32(b + 0x10);
    e.gp0 = le32(b + 0x14);
    e.t_addr = le32(b + 0x18);
    e.t_size = le32(b + 0x1C);
    e.d_addr = le32(b + 0x20);
    e.d_size = le32(b + 0x24);
    e.b_addr = le32(b + 0x28);
    e.b_size = le32(b + 0x2C);
    e.s_addr = le32(b + 0x30);
    e.s_size = le32(b + 0x34);
    if ((e.t_size & (EXE_HEADER - 1u)) != 0u || (uint64_t)e.t_size > n - EXE_HEADER ||
        (e.t_addr & 3u) != 0u || e.pc0 == 0u || (e.pc0 & 3u) != 0u) {
        return SCHWAY_STATUS_ERR_EXE_INVALID;
    }
    uint64_t t_phys = physical(e.t_addr);
    if (t_phys == UINT64_MAX || t_phys + e.t_size > SCHWAY__RAM_SIZE) {
        return SCHWAY_STATUS_ERR_EXE_RANGE;
    }
    if (e.s_addr != 0u) {
        uint64_t s_phys = physical(e.s_addr);
        if (s_phys == UINT64_MAX || s_phys + e.s_size > SCHWAY__RAM_SIZE) {
            return SCHWAY_STATUS_ERR_EXE_RANGE;
        }
    }
    e.licence_area = licence_of(b);
    /* The kernel area (the low 64 KiB) and bytes past the text are noted, not
       refused. */
    if (e.t_size != 0u && t_phys < 0x10000u) {
        e.lint |= SCHWAY_EXE_LINT_LOW_64K;
    }
    if (n > (uint64_t)EXE_HEADER + e.t_size) {
        e.lint |= SCHWAY_EXE_LINT_TRAILING;
    }
    e.hash_len = e.t_size + EXE_HEADER;
    *info = e;
    return SCHWAY_STATUS_OK;
}

schway_status schway_probe_exe(const void *bytes, size_t n, schway_exe_info *out)
{
    if (bytes == NULL || n == 0u || out == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    schway_status st = schway__instance_check_out_size(out, SCHWAY__EXE_INFO_SIZE_FIRST);
    if (st != SCHWAY_STATUS_OK) {
        return st;
    }
    schway_exe_info info;
    st = validate((const uint8_t *)bytes, (uint64_t)n, &info);
    if (st != SCHWAY_STATUS_OK) {
        return st;
    }
    info.size = out->size;
    schway__instance_write_out(out, &info, (uint32_t)sizeof info);
    return SCHWAY_STATUS_OK;
}

/* Side-loads before the first frame: the text segment at its physical
   address and the launch block, and nothing else. Validation finishes before
   the first write, so a refused file changes nothing. A power reset erases
   both; the host loads again. */
schway_status schway_load_exe(schway *inst, const void *bytes, size_t n)
{
    if (inst == NULL || bytes == NULL || n == 0u || inst->frame != 0u) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    const uint8_t *b = (const uint8_t *)bytes;
    schway_exe_info e;
    schway_status st = validate(b, (uint64_t)n, &e);
    if (st != SCHWAY_STATUS_OK) {
        return st;
    }
    uint8_t *ram = inst->bus.ram;
    memcpy(&ram[(size_t)physical(e.t_addr)], b + EXE_HEADER, e.t_size);
    put_le32(&ram[EXE_LAUNCH_BLOCK], e.pc0);
    put_le32(&ram[EXE_LAUNCH_BLOCK + 4u], e.gp0);
    put_le32(&ram[EXE_LAUNCH_BLOCK + 8u], e.s_addr);
    put_le32(&ram[EXE_LAUNCH_BLOCK + 12u], e.s_size);
    inst->exe_loaded = 1u;
    return SCHWAY_STATUS_OK;
}
