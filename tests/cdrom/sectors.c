/* Sector delivery through the CD host callback and guest-visible RDDATA. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

#define CDROM 0x1F801800u
#define DMA_DPCR 0x1F8010F0u
#define DMA_MADR3 0x1F8010B0u
#define DMA_BCR3 0x1F8010B4u
#define DMA_CHCR3 0x1F8010B8u
#define SECTOR_PERIOD 451584u

int schway__cdrom_sector_test(const char *name);

struct sector_host {
    schway_toc toc;
    schway *inst;
    uint32_t reads;
    uint32_t bad_q_calls;
    uint32_t fail_lba;
    uint32_t fail_once;
    uint32_t malformed_lba;
    uint32_t bad_q_lba;
    uint32_t q_present;
    uint8_t q[12];
    uint32_t xa_audio;
    uint32_t raw_audio;
    uint32_t callbacks_allowed;
    uint32_t callbacks_outside_run;
};

static void *sector_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void sector_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static schway *sector_make(void)
{
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = sector_alloc;
    cfg.allocator.free = sector_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) {
        exit(1);
    }
    return inst;
}

static uint32_t sector_read(void *user, uint32_t lba, uint8_t sector[2352],
                            uint8_t subcode[96])
{
    struct sector_host *host = (struct sector_host *)user;
    host->reads++;
    if (host->callbacks_allowed == 0u) {
        host->callbacks_outside_run++;
    }
    memset(sector, 0, 2352u);
    memset(subcode, 0, 96u);
    if (host->raw_audio != 0u) {
        for (uint32_t i = 0u; i < 2352u; i++) sector[i] = (uint8_t)(lba + i);
        return 1u;
    }
    if (lba != host->malformed_lba) {
        memset(&sector[1], 0xFF, 10u);
    }
    for (uint32_t i = 24u; i < 2352u; i++) {
        sector[i] = (uint8_t)(lba + i);
    }
    sector[15] = 2u;
    sector[16] = 1u;
    sector[17] = 2u;
    sector[18] = host->xa_audio != 0u ? 0x44u : 8u;
    sector[19] = 0u;
    sector[20] = 1u;
    sector[21] = 2u;
    sector[22] = sector[18];
    sector[23] = 0u;
    if (host->q_present != 0u) {
        for (uint32_t bit = 0u; bit < 96u; bit++) {
            if ((host->q[bit / 8u] & (1u << (7u - bit % 8u))) != 0u) {
                subcode[bit] |= 0x40u;
            }
        }
    }
    if (host->fail_lba == lba && (host->fail_once == 0u || host->reads == 1u)) return 0u;
    return 1u;
}

static uint32_t sector_bad_q(void *user, uint32_t lba)
{
    struct sector_host *host = (struct sector_host *)user;
    host->bad_q_calls++;
    if (host->callbacks_allowed == 0u) {
        host->callbacks_outside_run++;
    }
    return lba == host->bad_q_lba ? 1u : 0u;
}

static uint32_t sector_toc(void *user, schway_toc *out)
{
    struct sector_host *host = (struct sector_host *)user;
    memcpy(out, &host->toc, sizeof *out);
    return 1u;
}

static void sector_attach_type(schway *inst, struct sector_host *host, uint32_t track_type)
{
    memset(host, 0, sizeof *host);
    host->inst = inst;
    host->fail_lba = UINT32_MAX;
    host->malformed_lba = UINT32_MAX;
    host->bad_q_lba = UINT32_MAX;
    host->toc.size = (uint32_t)sizeof host->toc;
    host->toc.track_count = 1u;
    host->toc.tracks[0].size = (uint32_t)sizeof host->toc.tracks[0];
    host->toc.tracks[0].type = track_type;
    host->toc.tracks[0].index0 = 150u;
    host->toc.tracks[0].index1 = 150u;
    host->toc.tracks[0].leadout = 10000u;
    schway_disc disc;
    memset(&disc, 0, sizeof disc);
    disc.size = (uint32_t)sizeof disc;
    disc.user = host;
    disc.read_sector = sector_read;
    disc.toc = sector_toc;
    disc.bad_q = sector_bad_q;
    CHECK_EQ_U64(schway_open_disc(inst, &disc), SCHWAY_STATUS_OK);
    schway_profile profile;
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = SCHWAY_REGION_NTSC_U;
    profile.flags = SCHWAY_PROFILE_CD_LICENSED;
    CHECK_EQ_U64(schway_set_profile(inst, &profile), SCHWAY_STATUS_OK);
}

static void sector_attach(schway *inst, struct sector_host *host)
{
    sector_attach_type(inst, host, SCHWAY_DISC_TRACK_MODE2);
}

static void sector_wr8(schway *inst, uint32_t offset, uint8_t value)
{
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, CDROM + offset, 1u, value, &cycles), 0u);
}

static void sector_wr32(schway *inst, uint32_t address, uint32_t value)
{
    uint32_t cycles = 0u;
    schway__sched_sync(inst);
    CHECK_EQ_U64(schway__bus_write(inst, address, 4u, value, &cycles), 0u);
}

static uint32_t sector_ram_word(const schway *inst, uint32_t address)
{
    return (uint32_t)inst->bus.ram[address] |
           ((uint32_t)inst->bus.ram[address + 1u] << 8) |
           ((uint32_t)inst->bus.ram[address + 2u] << 16) |
           ((uint32_t)inst->bus.ram[address + 3u] << 24);
}

static void sector_bfrd(schway *inst, uint8_t value)
{
    sector_wr8(inst, 0u, 0u);
    sector_wr8(inst, 3u, value != 0u ? 0x80u : 0u);
}

static uint8_t sector_rd8(schway *inst, uint32_t offset)
{
    uint32_t cycles = 0u;
    uint32_t value = 0u;
    CHECK_EQ_U64(schway__bus_read(inst, CDROM + offset, 1u, &value, &cycles), 0u);
    return (uint8_t)value;
}

static void sector_run(schway *inst, uint64_t cycles, struct sector_host *host)
{
    uint64_t start = inst->ticks;
    host->callbacks_allowed = 1u;
    while (inst->ticks - start < cycles) {
        schway_budget budget;
        memset(&budget, 0, sizeof budget);
        budget.size = (uint32_t)sizeof budget;
        budget.max_cycles = cycles - (inst->ticks - start);
        schway_run_result result;
        memset(&result, 0, sizeof result);
        result.size = (uint32_t)sizeof result;
        uint64_t before = inst->ticks;
        CHECK_EQ_U64(schway_run(inst, &budget, &result), SCHWAY_STATUS_OK);
        if (inst->ticks == before && result.stop != SCHWAY_STOP_FRAME) {
            break;
        }
    }
    host->callbacks_allowed = 0u;
}

static void sector_param(schway *inst, uint8_t value)
{
    sector_wr8(inst, 0u, 0u);
    sector_wr8(inst, 2u, value);
}

static void sector_ack3(schway *inst)
{
    sector_wr8(inst, 0u, 1u);
    sector_wr8(inst, 3u, 4u);
}

static uint8_t sector_int_code(schway *inst)
{
    sector_wr8(inst, 0u, 1u);
    return sector_rd8(inst, 3u) & 7u;
}

static void sector_command(schway *inst, struct sector_host *host, uint8_t value)
{
    sector_wr8(inst, 0u, 0u);
    sector_wr8(inst, 1u, value);
    sector_run(inst, 50000u, host);
    sector_ack3(inst);
}

static void sector_start_read(schway *inst, struct sector_host *host)
{
    sector_param(inst, 0u);
    sector_param(inst, 2u);
    sector_param(inst, 0u);
    sector_command(inst, host, 0x02u);
    sector_wr8(inst, 0u, 0u);
    sector_wr8(inst, 3u, 0x80u);
    sector_command(inst, host, 0x06u);
}

static void sector_setmode(schway *inst, struct sector_host *host, uint8_t mode)
{
    sector_param(inst, mode);
    sector_command(inst, host, 0x0Eu);
}

static void sector_setfilter(schway *inst, struct sector_host *host, uint8_t file,
                             uint8_t channel)
{
    sector_param(inst, file);
    sector_param(inst, channel);
    sector_command(inst, host, 0x0Du);
}

static void sector_start_read_s(schway *inst, struct sector_host *host)
{
    sector_param(inst, 0u);
    sector_param(inst, 2u);
    sector_param(inst, 0u);
    sector_command(inst, host, 0x02u);
    sector_wr8(inst, 0u, 0u);
    sector_wr8(inst, 3u, 0x80u);
    sector_command(inst, host, 0x1Bu);
}

static void sector_start_read_pending(schway *inst, struct sector_host *host)
{
    sector_param(inst, 0u);
    sector_param(inst, 2u);
    sector_param(inst, 0u);
    sector_command(inst, host, 0x02u);
    sector_wr8(inst, 0u, 0u);
    sector_wr8(inst, 3u, 0x80u);
    sector_wr8(inst, 0u, 0u);
    sector_wr8(inst, 1u, 0x06u);
    sector_run(inst, 50000u, host);
}

static void test_sector_timing_and_data(void)
{
    schway *inst = sector_make();
    struct sector_host host;
    sector_attach(inst, &host);
    uint32_t peek = 0u;
    CHECK_EQ_U64(schway_peek(inst, SCHWAY_SPACE_RAM, 0u, 1u, &peek), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(host.reads, 0u);
    sector_start_read(inst, &host);
    uint64_t start = inst->ticks;
    sector_run(inst, SECTOR_PERIOD - 1u, &host);
    CHECK_EQ_U64(host.reads, 0u);
    sector_run(inst, 1u, &host);
    CHECK_EQ_U64(host.reads, 1u);
    CHECK_EQ_U64(host.callbacks_outside_run, 0u);
    sector_bfrd(inst, 0u);
    CHECK_EQ_U64(sector_rd8(inst, 2u), 0u);
    CHECK_EQ_U64(inst->cdrom.sector_offset, 0u);
    sector_bfrd(inst, 1u);
    CHECK_EQ_U64(sector_rd8(inst, 2u), 0x18u);
    CHECK_EQ_U64(sector_rd8(inst, 2u), 0x19u);
    CHECK_EQ_U64(sector_rd8(inst, 2u), 0x1Au);
    CHECK_EQ_U64(sector_rd8(inst, 2u), 0x1Bu);
    for (uint32_t i = 4u; i < 2048u; i++) {
        (void)sector_rd8(inst, 2u);
    }
    CHECK_EQ_U64(sector_rd8(inst, 2u), 0xF8u);
    CHECK_EQ_U64(inst->ticks, start + SECTOR_PERIOD);
    schway_destroy(inst);

    /* CPU RDDATA and DMA3 consume the callback-delivered sector from one
       cursor; a partial CPU read leaves the next byte for DMA. */
    inst = sector_make();
    sector_attach(inst, &host);
    sector_start_read(inst, &host);
    sector_run(inst, SECTOR_PERIOD, &host);
    CHECK_EQ_U64(host.reads, 1u);
    CHECK_EQ_U64(sector_rd8(inst, 2u), 0x18u);
    CHECK_EQ_U64(sector_rd8(inst, 2u), 0x19u);
    CHECK_EQ_U64(sector_rd8(inst, 2u), 0x1Au);
    sector_wr32(inst, DMA_DPCR, 0x0FEDCBA9u);
    sector_wr32(inst, DMA_MADR3, 0x4000u);
    sector_wr32(inst, DMA_BCR3, 4u);
    sector_wr32(inst, DMA_CHCR3, 0x11000000u);
    CHECK_EQ_HEX(sector_ram_word(inst, 0x4000u), 0x1E1D1C1Bu);
    CHECK_EQ_HEX(sector_ram_word(inst, 0x4004u), 0x2221201Fu);
    CHECK_EQ_HEX(sector_ram_word(inst, 0x4008u), 0x26252423u);
    CHECK_EQ_HEX(sector_ram_word(inst, 0x400Cu), 0x2A292827u);
    CHECK_EQ_U64(inst->cdrom.sector_offset, 19u);
    schway_destroy(inst);
}

static void test_pending_interrupt_and_bad_q(void)
{
    schway *inst = sector_make();
    struct sector_host host;
    sector_attach(inst, &host);
    host.bad_q_lba = 0u;
    sector_start_read_pending(inst, &host);
    inst->cdrom.current_lba = 77u;
    uint64_t deadline = inst->ticks + SECTOR_PERIOD;
    sector_run(inst, SECTOR_PERIOD, &host);
    CHECK_EQ_U64(host.reads, 1u);
    CHECK_EQ_U64(host.bad_q_calls, 1u);
    CHECK_EQ_U64(inst->cdrom.current_lba, 77u);
    CHECK_EQ_U64(sector_int_code(inst), 3u);
    sector_run(inst, SECTOR_PERIOD, &host);
    CHECK_EQ_U64(host.reads, 2u);
    CHECK_EQ_U64(host.bad_q_calls, 2u);
    CHECK_EQ_U64(inst->cdrom.current_lba, 1u);
    sector_ack3(inst);
    CHECK_EQ_U64(sector_int_code(inst), 1u);
    CHECK_EQ_U64(inst->ticks, deadline + SECTOR_PERIOD);
    schway_destroy(inst);
}

static void test_q_xa_filter_retry_and_audio_play(void)
{
    schway *inst = sector_make();
    struct sector_host host;
    sector_attach(inst, &host);
    host.q_present = 1u;
    const uint8_t q[12] = {0x41u, 0x01u, 0x01u, 0u, 0u, 0u, 0u,
                           0u, 0x03u, 0u, 0x1Bu, 0x03u};
    memcpy(host.q, q, sizeof q);
    sector_start_read(inst, &host);
    sector_run(inst, SECTOR_PERIOD, &host);
    CHECK_EQ_U64(inst->cdrom.current_lba, 75u);
    schway_destroy(inst);

    inst = sector_make();
    sector_attach(inst, &host);
    host.xa_audio = 1u;
    sector_setmode(inst, &host, 0x48u);
    sector_setfilter(inst, &host, 9u, 9u);
    sector_start_read(inst, &host);
    sector_run(inst, SECTOR_PERIOD, &host);
    CHECK_EQ_U64(host.reads, 1u);
    CHECK_EQ_U64(inst->cdrom.sector_count, 0u);
    CHECK_EQ_U64(inst->cdrom.current_lba, 0u);
    CHECK_EQ_U64(sector_int_code(inst), 0u);
    schway_destroy(inst);

    inst = sector_make();
    sector_attach(inst, &host);
    sector_setmode(inst, &host, 0x08u);
    sector_setfilter(inst, &host, 9u, 9u);
    sector_start_read(inst, &host);
    sector_run(inst, SECTOR_PERIOD * 2u, &host);
    CHECK_EQ_U64(host.reads, 2u);
    CHECK_EQ_U64(inst->cdrom.target_lba, 1u);
    CHECK_EQ_U64(inst->cdrom.sector_count, 0u);
    schway_destroy(inst);

    inst = sector_make();
    sector_attach(inst, &host);
    sector_setmode(inst, &host, 0x08u);
    sector_setfilter(inst, &host, 9u, 9u);
    sector_start_read_s(inst, &host);
    sector_run(inst, SECTOR_PERIOD, &host);
    CHECK_EQ_U64(host.reads, 1u);
    CHECK_EQ_U64(inst->cdrom.target_lba, 1u);
    CHECK_EQ_U64(inst->cdrom.sector_count, 0u);
    schway_destroy(inst);

    inst = sector_make();
    sector_attach_type(inst, &host, SCHWAY_DISC_TRACK_AUDIO);
    host.raw_audio = 1u;
    sector_command(inst, &host, 0x03u);
    sector_run(inst, SECTOR_PERIOD, &host);
    CHECK_EQ_U64(host.reads, 1u);
    CHECK_EQ_U64(inst->cdrom.play_active, 1u);
    CHECK_EQ_U64(inst->cdrom.sector_size[inst->cdrom.sector_head], 2352u);
    CHECK_EQ_U64(sector_int_code(inst), 0u);
    schway_destroy(inst);
}

static void test_speed_overrun_and_bad_reads(void)
{
    schway *inst = sector_make();
    struct sector_host host;
    sector_attach(inst, &host);
    sector_param(inst, 0xA0u);
    sector_command(inst, &host, 0x0Eu);
    sector_start_read(inst, &host);
    sector_run(inst, SECTOR_PERIOD / 2u - 1u, &host);
    CHECK_EQ_U64(host.reads, 0u);
    sector_run(inst, 1u, &host);
    CHECK_EQ_U64(host.reads, 1u);
    CHECK_EQ_U64(inst->cdrom.sector_size[inst->cdrom.sector_head], 2340u);
    CHECK_EQ_U64(sector_rd8(inst, 2u), inst->cdrom.sector[inst->cdrom.sector_head][12]);
    sector_run(inst, (SECTOR_PERIOD / 2u) * 8u, &host);
    CHECK_EQ_U64(host.reads, 9u);
    CHECK_EQ_U64(inst->cdrom.sector_count, SCHWAY__CDROM_SECTOR_CAP);
    CHECK_EQ_U64(inst->cdrom.sector_lba[inst->cdrom.sector_head], 1u);
    uint32_t newest = (inst->cdrom.sector_head + inst->cdrom.sector_count - 1u) %
                      SCHWAY__CDROM_SECTOR_CAP;
    CHECK_EQ_U64(inst->cdrom.sector_lba[newest], 8u);
    schway_destroy(inst);

    inst = sector_make();
    sector_attach(inst, &host);
    host.fail_lba = 0u;
    sector_start_read(inst, &host);
    sector_run(inst, SECTOR_PERIOD * 2u, &host);
    CHECK_EQ_U64(host.reads, 2u);
    CHECK_EQ_U64(inst->cdrom.read_active, 0u);
    sector_run(inst, SECTOR_PERIOD, &host);
    CHECK_EQ_U64(host.reads, 2u);
    CHECK_EQ_U64(sector_int_code(inst), 5u);
    schway_destroy(inst);

    inst = sector_make();
    sector_attach(inst, &host);
    host.fail_lba = 0u;
    sector_start_read_s(inst, &host);
    sector_run(inst, SECTOR_PERIOD, &host);
    CHECK_EQ_U64(host.reads, 1u);
    CHECK_EQ_U64(inst->cdrom.read_active, 0u);
    schway_destroy(inst);

    inst = sector_make();
    sector_attach(inst, &host);
    host.malformed_lba = 0u;
    sector_start_read(inst, &host);
    sector_run(inst, SECTOR_PERIOD * 2u, &host);
    CHECK_EQ_U64(host.reads, 2u);
    CHECK_EQ_U64(inst->cdrom.sector_count, 0u);
    CHECK_EQ_U64(inst->cdrom.read_active, 0u);
    schway_destroy(inst);
}

int schway__cdrom_sector_test(const char *name)
{
    if (strcmp(name, "sectors") == 0) {
        test_sector_timing_and_data();
        test_speed_overrun_and_bad_reads();
    } else if (strcmp(name, "delivery") == 0) {
        test_pending_interrupt_and_bad_q();
        test_q_xa_filter_retry_and_audio_play();
    } else {
        fprintf(stderr, "unknown CD-ROM sector case: %s\n", name);
        return 2;
    }
    CHECK_DONE();
}
