/* CD-ROM controller register tests: schwaystation-test-cdrom <case>.
   Each case drives the real MMIO and scheduler, then checks device-visible
   registers and the instance's saved device state. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

#define CDROM 0x1F801800u
#define I_STAT 0x1F801070u

struct test_disc {
    schway_toc toc;
    const char *license;
    uint32_t reads;
};

static void *test_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void test_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static schway *make(void)
{
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) {
        exit(1);
    }
    return inst;
}

static uint32_t read_sector(void *user, uint32_t lba, uint8_t sector[2352],
                            uint8_t subcode[96])
{
    struct test_disc *disc = (struct test_disc *)user;
    disc->reads++;
    memset(sector, 0, 2352u);
    memset(subcode, 0, 96u);
    memset(&sector[1], 0xFF, 10u);
    sector[15] = 2u;
    sector[16] = 1u;
    sector[17] = 2u;
    sector[18] = 8u;
    sector[20] = 1u;
    sector[21] = 2u;
    sector[22] = 8u;
    if (lba == 4u && disc->license != NULL) {
        (void)memcpy(&sector[56], disc->license, 32u);
    }
    return 1u;
}

static uint32_t read_toc(void *user, schway_toc *out)
{
    struct test_disc *disc = (struct test_disc *)user;
    (void)memcpy(out, &disc->toc, sizeof *out);
    return 1u;
}

static void attach_disc(schway *inst, struct test_disc *host, uint32_t track_type,
                        const char *license)
{
    memset(host, 0, sizeof *host);
    host->license = license;
    host->toc.size = (uint32_t)sizeof host->toc;
    host->toc.track_count = 1u;
    host->toc.tracks[0].size = (uint32_t)sizeof host->toc.tracks[0];
    host->toc.tracks[0].type = track_type;
    host->toc.tracks[0].index0 = 150u;
    host->toc.tracks[0].index1 = 150u;
    host->toc.tracks[0].leadout = 1000000u;
    schway_disc disc;
    memset(&disc, 0, sizeof disc);
    disc.size = (uint32_t)sizeof disc;
    disc.user = host;
    disc.read_sector = read_sector;
    disc.toc = read_toc;
    CHECK_EQ_U64(schway_open_disc(inst, &disc), SCHWAY_STATUS_OK);
}

static void attach_two_track_disc(schway *inst, struct test_disc *host,
                                 uint32_t first_type, uint32_t second_type,
                                 const char *license)
{
    memset(host, 0, sizeof *host);
    host->license = license;
    host->toc.size = (uint32_t)sizeof host->toc;
    host->toc.track_count = 2u;
    for (uint32_t i = 0u; i < 2u; i++) {
        schway_disc_track *track = &host->toc.tracks[i];
        track->size = (uint32_t)sizeof *track;
        track->type = i == 0u ? first_type : second_type;
        track->index0 = i == 0u ? 150u : 300u;
        track->index1 = track->index0;
        track->leadout = i == 0u ? 300u : 1000000u;
    }
    schway_disc disc;
    memset(&disc, 0, sizeof disc);
    disc.size = (uint32_t)sizeof disc;
    disc.user = host;
    disc.read_sector = read_sector;
    disc.toc = read_toc;
    CHECK_EQ_U64(schway_open_disc(inst, &disc), SCHWAY_STATUS_OK);
}

static void profile(schway *inst, uint32_t region, uint32_t flags)
{
    schway_profile value;
    memset(&value, 0, sizeof value);
    value.size = (uint32_t)sizeof value;
    value.region = region;
    value.flags = flags;
    CHECK_EQ_U64(schway_set_profile(inst, &value), SCHWAY_STATUS_OK);
}

static void wr8(schway *inst, uint32_t offset, uint8_t value)
{
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, CDROM + offset, 1u, value, &cycles), 0u);
}

static uint8_t rd8(schway *inst, uint32_t offset)
{
    uint32_t cycles = 0u;
    uint32_t value = 0u;
    CHECK_EQ_U64(schway__bus_read(inst, CDROM + offset, 1u, &value, &cycles), 0u);
    return (uint8_t)value;
}

static uint32_t rd32(schway *inst, uint32_t address)
{
    uint32_t cycles = 0u;
    uint32_t value = 0u;
    CHECK_EQ_U64(schway__bus_read(inst, address, 4u, &value, &cycles), 0u);
    return value;
}

static void run_cycles(schway *inst, uint64_t cycles)
{
    while (cycles != 0u) {
        schway_budget budget;
        memset(&budget, 0, sizeof budget);
        budget.size = (uint32_t)sizeof budget;
        budget.max_cycles = cycles;
        schway_run_result result;
        memset(&result, 0, sizeof result);
        result.size = (uint32_t)sizeof result;
        uint64_t before = inst->ticks;
        CHECK_EQ_U64(schway_run(inst, &budget, &result), SCHWAY_STATUS_OK);
        uint64_t elapsed = result.ticks - before;
        CHECK(elapsed != 0u);
        CHECK(elapsed <= cycles);
        if (elapsed == 0u || elapsed > cycles) {
            return;
        }
        cycles -= elapsed;
    }
}

static void fire_command_event(schway *inst)
{
    uint64_t deadline = inst->cdrom.command_deadline;
    CHECK_EQ_U64(inst->cdrom.command_event, 1u);
    CHECK(deadline != 0u);
    inst->run_active = 1u;
    schway__cdrom_event(inst, deadline);
    inst->run_active = 0u;
}

static uint8_t hint_status(schway *inst)
{
    wr8(inst, 0u, 1u);
    return rd8(inst, 3u);
}

static void acknowledge(schway *inst, uint8_t interrupt)
{
    wr8(inst, 0u, 1u);
    wr8(inst, 3u, (uint8_t)(1u << (interrupt - 1u)));
}

static void command(schway *inst, uint8_t value)
{
    wr8(inst, 0u, 0u);
    wr8(inst, 1u, value);
}

static void param(schway *inst, uint8_t value)
{
    wr8(inst, 0u, 0u);
    wr8(inst, 2u, value);
}

static void expect_first_ack(schway *inst)
{
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    (void)rd8(inst, 1u);
    acknowledge(inst, 3u);
}

static void run_unlock_part(schway *inst, uint8_t value, const char *parameters)
{
    uint32_t count = parameters == NULL ? 0u : (uint32_t)strlen(parameters);
    for (uint32_t i = 0u; i < count; i++) {
        param(inst, (uint8_t)parameters[i]);
    }
    command(inst, value);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x11u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x40u);
    acknowledge(inst, 5u);
}

static void unlock_disc(schway *inst)
{
    run_unlock_part(inst, 0x50u, NULL);
    run_unlock_part(inst, 0x51u, "Licensed by");
    run_unlock_part(inst, 0x52u, "Sony");
    run_unlock_part(inst, 0x53u, "Computer");
    run_unlock_part(inst, 0x54u, "Entertainment");
    run_unlock_part(inst, 0x55u, "(Europe)");
    run_unlock_part(inst, 0x56u, NULL);
}

static void test_command(void)
{
    schway *inst = make();
    wr8(inst, 0u, 1u);
    wr8(inst, 2u, 0u); /* Keep INT3 masked until its status is pending. */
    command(inst, 0x01u); /* Nop. */
    run_cycles(inst, 50000u);

    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    CHECK_EQ_U64(rd32(inst, I_STAT) & (1u << 2), 0u);
    wr8(inst, 2u, 0x04u); /* Enabling a pending INT3 raises IRQ2. */
    CHECK((rd32(inst, I_STAT) & (1u << 2)) != 0u);
    CHECK_EQ_U64(rd8(inst, 1u), 0u); /* Nop returns the current status. */
    acknowledge(inst, 3u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 0u);

    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, I_STAT, 4u, 0u, &cycles), 0u);
    CHECK_EQ_U64(rd32(inst, I_STAT) & (1u << 2), 0u);
    schway_destroy(inst);
}

static schway *make_pal_data_disc(struct test_disc *host)
{
    schway *inst = make();
    attach_two_track_disc(inst, host, SCHWAY_DISC_TRACK_MODE2, SCHWAY_DISC_TRACK_MODE2,
                          "Sony Computer Entertainment Euro");
    profile(inst, SCHWAY_REGION_PAL, 0u);
    return inst;
}

static void setloc_start(schway *inst)
{
    param(inst, 0u);
    param(inst, 2u);
    param(inst, 0u);
    command(inst, 0x02u);
    expect_first_ack(inst);
}

static void test_command_settings(void)
{
    struct test_disc host;
    schway *inst = make_pal_data_disc(&host);
    param(inst, 0x80u);
    command(inst, 0x0Eu);
    expect_first_ack(inst);
    CHECK_EQ_U64(inst->cdrom.mode, 0x80u);
    CHECK_EQ_U64(inst->cdrom.drive_speed, 2u);
    param(inst, 7u);
    param(inst, 2u);
    command(inst, 0x0Du);
    expect_first_ack(inst);
    CHECK_EQ_U64(inst->cdrom.file, 7u);
    CHECK_EQ_U64(inst->cdrom.channel, 2u);
    command(inst, 0x0Fu);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    CHECK_EQ_U64(rd8(inst, 1u), 0u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x80u);
    CHECK_EQ_U64(rd8(inst, 1u), 0u);
    CHECK_EQ_U64(rd8(inst, 1u), 7u);
    CHECK_EQ_U64(rd8(inst, 1u), 2u);
    acknowledge(inst, 3u);
    command(inst, 0x0Bu);
    expect_first_ack(inst);
    CHECK_EQ_U64(inst->cdrom.muted, 1u);
    command(inst, 0x0Cu);
    expect_first_ack(inst);
    CHECK_EQ_U64(inst->cdrom.muted, 0u);
    command(inst, 0x13u);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    CHECK_EQ_U64(rd8(inst, 1u), 0u);
    CHECK_EQ_U64(rd8(inst, 1u), 1u);
    CHECK_EQ_U64(rd8(inst, 1u), 2u);
    acknowledge(inst, 3u);
    param(inst, 1u);
    command(inst, 0x14u);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    CHECK_EQ_U64(rd8(inst, 1u), 0u);
    CHECK_EQ_U64(rd8(inst, 1u), 0u);
    CHECK_EQ_U64(rd8(inst, 1u), 4u);
    acknowledge(inst, 3u);
    setloc_start(inst);
    CHECK_EQ_U64(inst->cdrom.target_lba, 0u);
    schway_destroy(inst);
}

static void test_command_media(void)
{
    struct test_disc host;
    schway *inst = make_pal_data_disc(&host);
    setloc_start(inst);
    command(inst, 0x07u);
    expect_first_ack(inst);
    CHECK_EQ_U64(inst->cdrom.motor_on, 1u);
    run_cycles(inst, 476000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 2u);
    (void)rd8(inst, 1u);
    acknowledge(inst, 2u);
    command(inst, 0x06u);
    expect_first_ack(inst);
    CHECK_EQ_U64(inst->cdrom.read_active, 1u);
    CHECK_EQ_U64(inst->cdrom.status & (1u << 5), 1u << 5);
    command(inst, 0x09u);
    expect_first_ack(inst);
    CHECK_EQ_U64(inst->cdrom.read_active, 0u);
    run_cycles(inst, 2170000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 2u);
    (void)rd8(inst, 1u);
    acknowledge(inst, 2u);
    param(inst, 2u);
    command(inst, 0x03u);
    expect_first_ack(inst);
    CHECK_EQ_U64(inst->cdrom.target_lba, 150u);
    CHECK_EQ_U64(inst->cdrom.play_active, 1u);
    command(inst, 0x04u);
    expect_first_ack(inst);
    command(inst, 0x05u);
    expect_first_ack(inst);
    CHECK_EQ_U64(inst->cdrom.play_active, 1u);
    schway_destroy(inst);
}

static void test_command_seek_position(void)
{
    struct test_disc host;
    schway *inst = make_pal_data_disc(&host);
    param(inst, 0x80u);
    command(inst, 0x0Eu);
    expect_first_ack(inst);
    setloc_start(inst);
    command(inst, 0x15u);
    expect_first_ack(inst);
    CHECK_EQ_U64(inst->cdrom.seek_active, 1u);
    uint64_t delay = inst->cdrom.seek_deadline - inst->ticks;
    CHECK_EQ_U64(delay, (UINT64_C(84) * UINT64_C(451584)) / 2u);
    run_cycles(inst, delay - 1u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 0u);
    run_cycles(inst, 1u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 2u);
    (void)rd8(inst, 1u);
    acknowledge(inst, 2u);
    CHECK_EQ_U64(inst->cdrom.seek_active, 0u);
    CHECK_EQ_U64(host.reads, 1u);
    CHECK_EQ_U64(inst->cdrom.q_record_valid, 1u);
    CHECK_EQ_HEX(inst->cdrom.q_record[1], 0x01u);
    CHECK_EQ_HEX(inst->cdrom.q_record[2], 0x01u);
    CHECK_EQ_HEX(inst->cdrom.q_record[7], 0x00u);
    CHECK_EQ_HEX(inst->cdrom.q_record[8], 0x02u);
    CHECK_EQ_HEX(inst->cdrom.q_record[9], 0x00u);
    CHECK_EQ_U64(inst->cdrom.sector_count, 1u);
    command(inst, 0x10u);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    for (uint32_t i = 0u; i < 3u; i++) (void)rd8(inst, 1u);
    CHECK_EQ_U64(rd8(inst, 1u), 2u);
    for (uint32_t i = 0u; i < 4u; i++) (void)rd8(inst, 1u);
    acknowledge(inst, 3u);
    command(inst, 0x11u);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    CHECK_EQ_U64(rd8(inst, 1u), 1u);
    CHECK_EQ_U64(rd8(inst, 1u), 1u);
    for (uint32_t i = 0u; i < 6u; i++) {
        (void)rd8(inst, 1u);
    }
    acknowledge(inst, 3u);
    schway_destroy(inst);
}

static void test_command_read_seeks_to_setloc(void)
{
    struct test_disc host;
    schway *inst = make_pal_data_disc(&host);
    profile(inst, SCHWAY_REGION_PAL, SCHWAY_PROFILE_CD_LICENSED);
    param(inst, 0x40u);
    param(inst, 0u);
    param(inst, 0x16u);
    command(inst, 0x02u);
    expect_first_ack(inst);
    CHECK_EQ_U64(inst->cdrom.target_lba, 179866u);
    command(inst, 0x06u);
    expect_first_ack(inst);
    CHECK_EQ_U64(inst->cdrom.read_active, 1u);
    CHECK_EQ_U64(inst->cdrom.seek_active, 1u);
    CHECK_EQ_U64(inst->cdrom.seek_event, 1u);
    CHECK_EQ_U64(inst->cdrom.sector_event, 0u);
    CHECK_EQ_U64(inst->cdrom.status & 0x62u, 0x42u);
    uint64_t delay = inst->cdrom.seek_deadline - inst->ticks;
    run_cycles(inst, delay);
    CHECK_EQ_U64(inst->cdrom.seek_active, 0u);
    CHECK_EQ_U64(inst->cdrom.read_active, 1u);
    CHECK_EQ_U64(inst->cdrom.status & 0x62u, 0x22u);
    CHECK_EQ_U64(inst->cdrom.sector_event, 1u);
    CHECK_EQ_U64(host.reads, 0u);
    run_cycles(inst, 451584u);
    CHECK_EQ_U64(host.reads, 1u);
    CHECK_EQ_U64(inst->cdrom.sector_count, 1u);
    CHECK_EQ_U64(inst->cdrom.sector_lba[inst->cdrom.sector_head], 179866u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 1u);
    wr8(inst, 0u, 0u);
    wr8(inst, 3u, 0x80u);
    CHECK_EQ_U64(rd8(inst, 0u) & 0xC0u, 0x40u);
    schway_destroy(inst);
}

static void test_readn_status_acceptance(void)
{
    struct test_disc host;
    schway *inst = make_pal_data_disc(&host);
    param(inst, 0u);
    param(inst, 2u);
    param(inst, 0x16u);
    command(inst, 0x02u);
    expect_first_ack(inst);
    command(inst, 0x06u);
    CHECK_EQ_U64(rd8(inst, 0u) & 0x80u, 0x80u);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x42u);
    acknowledge(inst, 3u);
    CHECK_EQ_U64(rd8(inst, 0u) & 0x80u, 0u);
    command(inst, 0x01u);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x42u);
    acknowledge(inst, 3u);
    schway_destroy(inst);
}

static void test_read_discards_only_seek_header(void)
{
    struct test_disc host;
    schway *inst = make_pal_data_disc(&host);
    inst->cdrom.current_lba = 16u;
    inst->cdrom.sector_head = 7u;
    inst->cdrom.sector_count = 3u;
    inst->cdrom.sector_offset = 2u;
    inst->cdrom.sector_lba[7] = 15u;
    inst->cdrom.sector_size[7] = 2048u;
    inst->cdrom.sector[7][0] = 0xA1u;
    inst->cdrom.sector_lba[0] = 16u;
    inst->cdrom.sector_size[0] = SCHWAY__CDROM_SECTOR_BYTES;
    inst->cdrom.sector[0][0] = 0xB2u;
    inst->cdrom.sector_lba[1] = 17u;
    inst->cdrom.sector_size[1] = 2048u;
    inst->cdrom.sector[1][0] = 0xC3u;

    schway__cdrom_sector_discard_seek_headers(inst);
    CHECK_EQ_U64(inst->cdrom.sector_head, 7u);
    CHECK_EQ_U64(inst->cdrom.sector_count, 2u);
    CHECK_EQ_U64(inst->cdrom.sector_offset, 2u);
    CHECK_EQ_U64(inst->cdrom.sector_lba[7], 15u);
    CHECK_EQ_U64(inst->cdrom.sector[7][0], 0xA1u);
    CHECK_EQ_U64(inst->cdrom.sector_lba[0], 17u);
    CHECK_EQ_U64(inst->cdrom.sector_size[0], 2048u);
    CHECK_EQ_U64(inst->cdrom.sector[0][0], 0xC3u);
    schway_destroy(inst);
}

static void test_getloc_rejects_seek_error(void)
{
    struct test_disc host;
    schway *inst = make_pal_data_disc(&host);
    param(inst, 0u);
    param(inst, 2u);
    param(inst, 0x16u);
    command(inst, 0x02u);
    expect_first_ack(inst);
    command(inst, 0x15u);
    expect_first_ack(inst);
    uint64_t delay = inst->cdrom.seek_deadline - inst->ticks;
    run_cycles(inst, delay);
    CHECK_EQ_U64(hint_status(inst) & 7u, 2u);
    (void)rd8(inst, 1u);
    acknowledge(inst, 2u);
    CHECK_EQ_U64(inst->cdrom.sector_count, 1u);
    uint8_t cached_header[8];
    memcpy(cached_header, &inst->cdrom.sector[inst->cdrom.sector_head][12], sizeof cached_header);

    inst->disc_host.toc.tracks[1].leadout = 335000u;
    param(inst, 0x74u);
    param(inst, 0x30u);
    param(inst, 0u);
    command(inst, 0x02u);
    expect_first_ack(inst);
    command(inst, 0x15u);
    expect_first_ack(inst);
    fire_command_event(inst);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x06u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x04u);
    acknowledge(inst, 5u);
    CHECK_EQ_U64(inst->cdrom.sector_count, 1u);
    CHECK_EQ_U64(inst->cdrom.sector_lba[inst->cdrom.sector_head], 16u);
    CHECK(memcmp(cached_header, &inst->cdrom.sector[inst->cdrom.sector_head][12],
                 sizeof cached_header) == 0);

    command(inst, 0x10u);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x06u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x80u);
    acknowledge(inst, 5u);
    CHECK_EQ_U64(inst->cdrom.sector_count, 1u);
    CHECK_EQ_U64(inst->cdrom.sector_lba[inst->cdrom.sector_head], 16u);
    CHECK(memcmp(cached_header, &inst->cdrom.sector[inst->cdrom.sector_head][12],
                 sizeof cached_header) == 0);
    command(inst, 0x11u);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x06u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x80u);
    acknowledge(inst, 5u);
    CHECK_EQ_U64(inst->cdrom.sector_count, 1u);
    CHECK_EQ_U64(inst->cdrom.sector_lba[inst->cdrom.sector_head], 16u);
    CHECK(memcmp(cached_header, &inst->cdrom.sector[inst->cdrom.sector_head][12],
                 sizeof cached_header) == 0);

    param(inst, 0u);
    param(inst, 2u);
    param(inst, 0x16u);
    command(inst, 0x02u);
    expect_first_ack(inst);
    command(inst, 0x15u);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x02u);
    acknowledge(inst, 3u);
    CHECK_EQ_U64(inst->cdrom.status & (1u << 2), 0u);
    schway_destroy(inst);
}

static void test_seek_error_reset(void)
{
    struct test_disc host;
    schway *inst = make_pal_data_disc(&host);
    inst->disc_host.toc.tracks[1].leadout = 335000u;
    param(inst, 0x74u);
    param(inst, 0x30u);
    param(inst, 0u);
    command(inst, 0x02u);
    expect_first_ack(inst);
    command(inst, 0x15u);
    expect_first_ack(inst);
    fire_command_event(inst);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x06u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x04u);
    acknowledge(inst, 5u);
    CHECK((inst->cdrom.status & (1u << 2)) != 0u);

    command(inst, 0x1Cu);
    fire_command_event(inst);
    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    CHECK((inst->cdrom.status & (1u << 2)) == 0u);
    (void)rd8(inst, 1u);
    acknowledge(inst, 3u);
    schway_destroy(inst);
}

static void test_command_getlocl_far_seek(void)
{
    struct test_disc host;
    schway *inst = make_pal_data_disc(&host);
    param(inst, 0x80u);
    command(inst, 0x0Eu);
    expect_first_ack(inst);
    setloc_start(inst);
    command(inst, 0x15u);
    expect_first_ack(inst);
    uint64_t delay = inst->cdrom.seek_deadline - inst->ticks;
    run_cycles(inst, delay);
    CHECK_EQ_U64(hint_status(inst) & 7u, 2u);
    (void)rd8(inst, 1u);
    acknowledge(inst, 2u);

    inst->cdrom.sector_head = 6u;
    inst->cdrom.sector_count = 2u;
    inst->cdrom.sector[6][12] = 0x10u;
    for (uint32_t i = 0u; i < 8u; i++) {
        inst->cdrom.sector[7][12u + i] = (uint8_t)(0x20u + i);
    }
    command(inst, 0x10u);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    for (uint32_t i = 0u; i < 8u; i++) {
        CHECK_EQ_U64(rd8(inst, 1u), 0x20u + i);
    }
    acknowledge(inst, 3u);

    param(inst, 0x99u);
    param(inst, 0x59u);
    param(inst, 0x74u);
    command(inst, 0x02u);
    expect_first_ack(inst);
    command(inst, 0x15u);
    expect_first_ack(inst);
    delay = inst->cdrom.seek_deadline - inst->ticks;
    CHECK_EQ_U64(delay, (UINT64_C(106) * UINT64_C(451584)) / 2u);
    run_cycles(inst, delay - 1u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 0u);
    run_cycles(inst, 1u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 2u);
    (void)rd8(inst, 1u);
    acknowledge(inst, 2u);
    schway_destroy(inst);
}

static void test_command_stop_reset(void)
{
    struct test_disc host;
    schway *inst = make_pal_data_disc(&host);
    command(inst, 0x07u);
    expect_first_ack(inst);
    run_cycles(inst, 476000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 2u);
    (void)rd8(inst, 1u);
    acknowledge(inst, 2u);
    param(inst, 0x80u);
    command(inst, 0x0Eu);
    expect_first_ack(inst);
    command(inst, 0x08u);
    expect_first_ack(inst);
    CHECK_EQ_U64(inst->cdrom.play_active, 0u);
    run_cycles(inst, UINT64_C(0x18A6076));
    CHECK_EQ_U64(hint_status(inst) & 7u, 2u);
    (void)rd8(inst, 1u);
    acknowledge(inst, 2u);
    command(inst, 0x1Cu);
    run_cycles(inst, 0x3FFFFFu);
    CHECK_EQ_U64(hint_status(inst) & 7u, 0u);
    run_cycles(inst, 1u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    (void)rd8(inst, 1u);
    acknowledge(inst, 3u);
    CHECK_EQ_U64(inst->cdrom.mode, 0x20u);
    CHECK_EQ_U64(inst->cdrom.motor_on, 0u);
    schway_destroy(inst);
}

static void test_command_matrix(void)
{
    test_command_settings();
    test_command_media();
    test_command_seek_position();
    test_command_read_seeks_to_setloc();
    test_readn_status_acceptance();
    test_read_discards_only_seek_header();
    test_getloc_rejects_seek_error();
    test_seek_error_reset();
    test_command_getlocl_far_seek();
    test_command_stop_reset();
}

static void test_timing_ack(void)
{
    schway *inst = make();
    command(inst, 0x01u);
    run_cycles(inst, 49999u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 0u);
    CHECK_EQ_U64(rd32(inst, I_STAT) & (1u << 2), 0u);
    run_cycles(inst, 1u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    CHECK_EQ_U64(rd8(inst, 1u), 0u);
    schway_destroy(inst);
}

static void test_timing_init(void)
{
    schway *inst = make();
    command(inst, 0x0Au);
    command(inst, 0x0Au);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    (void)rd8(inst, 1u);
    acknowledge(inst, 3u);
    CHECK_EQ_U64(inst->cdrom.command_pending, 2u);
    CHECK_EQ_U64(inst->cdrom.motor_on, 1u);
    run_cycles(inst, 475999u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 0u);
    run_cycles(inst, 1u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 2u);
    (void)rd8(inst, 1u);
    acknowledge(inst, 2u);
    schway_destroy(inst);
}

static void test_timing_pause_1x(void)
{
    schway *inst = make();
    command(inst, 0x09u);
    run_cycles(inst, 50000u);
    (void)rd8(inst, 1u);
    acknowledge(inst, 3u);
    run_cycles(inst, 2169999u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 0u);
    run_cycles(inst, 1u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 2u);
    (void)rd8(inst, 1u);
    acknowledge(inst, 2u);
    command(inst, 0x09u);
    expect_first_ack(inst);
    run_cycles(inst, 7665u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 0u);
    run_cycles(inst, 1u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 2u);
    (void)rd8(inst, 1u);
    acknowledge(inst, 2u);
    schway_destroy(inst);
}

static void test_timing_pause_2x(void)
{
    schway *inst = make();
    command(inst, 0x0Au);
    expect_first_ack(inst);
    run_cycles(inst, 476000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 2u);
    (void)rd8(inst, 1u);
    acknowledge(inst, 2u);
    param(inst, 0x80u);
    command(inst, 0x0Eu);
    expect_first_ack(inst);
    command(inst, 0x09u);
    expect_first_ack(inst);
    run_cycles(inst, 1084999u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 0u);
    run_cycles(inst, 1u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 2u);
    (void)rd8(inst, 1u);
    acknowledge(inst, 2u);
    schway_destroy(inst);
}

static void test_timing(void)
{
    test_timing_ack();
    test_timing_init();
    test_timing_pause_1x();
    test_timing_pause_2x();
}

static void test_bounds(void)
{
    schway *inst = make();
    wr8(inst, 0u, 0u);
    for (uint32_t i = 0u; i < SCHWAY__CDROM_FIFO_CAP + 1u; i++) {
        wr8(inst, 2u, (uint8_t)i);
    }
    CHECK_EQ_U64(inst->cdrom.parameter_count, SCHWAY__CDROM_FIFO_CAP);
    CHECK_EQ_U64(rd8(inst, 0u) & 0x18u, 0u); /* Not empty, no more write room. */

    wr8(inst, 0u, 1u);
    wr8(inst, 3u, 0x40u); /* Clear parameter FIFO. */
    wr8(inst, 0u, 0u);
    CHECK_EQ_U64(rd8(inst, 0u) & 0x18u, 0x18u);
    CHECK_EQ_U64(rd8(inst, 1u), 0u); /* Empty result reads are zero. */

    for (uint32_t i = 0u; i < SCHWAY__CDROM_FIFO_CAP; i++) {
        command(inst, 0x01u);
        run_cycles(inst, 50000u);
        acknowledge(inst, 3u);
    }
    CHECK_EQ_U64(inst->cdrom.result_count, SCHWAY__CDROM_FIFO_CAP);
    for (uint32_t i = 0u; i < 10u; i++) {
        CHECK_EQ_U64(rd8(inst, 1u), 0u);
    }
    CHECK_EQ_U64(inst->cdrom.result_head, 10u);
    for (uint32_t i = 0u; i < 10u; i++) {
        command(inst, 0x01u);
        run_cycles(inst, 50000u);
        acknowledge(inst, 3u);
    }
    CHECK_EQ_U64(inst->cdrom.result_count, SCHWAY__CDROM_FIFO_CAP);
    for (uint32_t i = 0u; i < SCHWAY__CDROM_FIFO_CAP; i++) {
        CHECK_EQ_U64(rd8(inst, 1u), 0u);
    }
    CHECK_EQ_U64(inst->cdrom.result_head, 10u);
    CHECK_EQ_U64(inst->cdrom.result_count, 0u);
    schway_destroy(inst);
}

static void test_mixed_track_seek(void)
{
    struct test_disc host;
    schway *inst = make();
    attach_two_track_disc(inst, &host, SCHWAY_DISC_TRACK_MODE2, SCHWAY_DISC_TRACK_AUDIO, NULL);
    param(inst, 0u);
    param(inst, 4u);
    param(inst, 0u);
    command(inst, 0x02u);
    expect_first_ack(inst);
    CHECK_EQ_U64(inst->cdrom.target_lba, 150u);
    command(inst, 0x15u);
    expect_first_ack(inst);
    run_cycles(inst, 67737599u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 0u);
    run_cycles(inst, 1u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    (void)rd8(inst, 1u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x04u);
    schway_destroy(inst);
}

static void test_region_no_disc_audio(void)
{
    struct test_disc host;
    schway *inst = make();
    command(inst, 0x1Au);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    CHECK_EQ_U64(rd8(inst, 1u), 0u);
    acknowledge(inst, 3u);
    run_cycles(inst, 0x4A00u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x08u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x40u);
    for (uint32_t i = 0u; i < 6u; i++) {
        CHECK_EQ_U64(rd8(inst, 1u), 0u);
    }
    schway_destroy(inst);

    inst = make();
    attach_disc(inst, &host, SCHWAY_DISC_TRACK_AUDIO, NULL);
    command(inst, 0x1Au);
    expect_first_ack(inst);
    run_cycles(inst, 0x4A00u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x0Au);
    CHECK_EQ_U64(rd8(inst, 1u), 0x90u);
    for (uint32_t i = 0u; i < 6u; i++) {
        CHECK_EQ_U64(rd8(inst, 1u), 0u);
    }
    acknowledge(inst, 5u);
    command(inst, 0x15u);
    expect_first_ack(inst);
    run_cycles(inst, 67737599u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 0u);
    run_cycles(inst, 1u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK((rd8(inst, 1u) & 4u) != 0u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x04u);
    schway_destroy(inst);
}

static void test_region_unlicensed(void)
{
    struct test_disc host;
    schway *inst = make();
    attach_disc(inst, &host, SCHWAY_DISC_TRACK_MODE2, NULL);
    command(inst, 0x1Au);
    expect_first_ack(inst);
    run_cycles(inst, 0x4A00u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x0Au);
    CHECK_EQ_U64(rd8(inst, 1u), 0x80u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x20u);
    for (uint32_t i = 0u; i < 5u; i++) {
        CHECK_EQ_U64(rd8(inst, 1u), 0u);
    }
    acknowledge(inst, 5u);
    command(inst, 0x07u);
    expect_first_ack(inst);
    run_cycles(inst, 476000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 2u);
    (void)rd8(inst, 1u);
    acknowledge(inst, 2u);
    command(inst, 0x06u);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x03u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x40u);
    schway_destroy(inst);
}

static void test_region_image_matches(void)
{
    struct test_disc host;
    schway *inst = make();
    attach_disc(inst, &host, SCHWAY_DISC_TRACK_MODE2,
                "Sony Computer Entertainment Euro");
    profile(inst, SCHWAY_REGION_PAL, 0u);
    command(inst, 0x1Au);
    expect_first_ack(inst);
    run_cycles(inst, 0x4A00u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 2u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x02u);
    CHECK_EQ_U64(rd8(inst, 1u), 0u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x20u);
    CHECK_EQ_U64(rd8(inst, 1u), 0u);
    CHECK_EQ_U64(rd8(inst, 1u), 'S');
    CHECK_EQ_U64(rd8(inst, 1u), 'C');
    CHECK_EQ_U64(rd8(inst, 1u), 'E');
    CHECK_EQ_U64(rd8(inst, 1u), 'E');
    CHECK_EQ_U64(host.reads, 1u);
    schway_destroy(inst);
}

static void test_region_image_mismatches(void)
{
    struct test_disc host;
    schway *inst = make();
    attach_disc(inst, &host, SCHWAY_DISC_TRACK_MODE2,
                "Sony Computer Entertainment Amer");
    profile(inst, SCHWAY_REGION_PAL, 0u);
    command(inst, 0x1Au);
    expect_first_ack(inst);
    run_cycles(inst, 0x4A00u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x0Au);
    CHECK_EQ_U64(rd8(inst, 1u), 0x80u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x20u);
    schway_destroy(inst);
}

static void test_region_unlock(void)
{
    struct test_disc host;
    schway *inst = make();
    attach_disc(inst, &host, SCHWAY_DISC_TRACK_MODE2,
                "Sony Computer Entertainment Amer");
    profile(inst, SCHWAY_REGION_PAL, SCHWAY_PROFILE_CD_UNLOCK);
    unlock_disc(inst);
    CHECK_EQ_U64(inst->cdrom.unlock_enabled, 1u);
    command(inst, 0x1Au);
    expect_first_ack(inst);
    run_cycles(inst, 0x4A00u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x0Au);
    CHECK_EQ_U64(rd8(inst, 1u), 0x80u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x20u);
    for (uint32_t i = 0u; i < 5u; i++) {
        CHECK_EQ_U64(rd8(inst, 1u), 0u);
    }
    acknowledge(inst, 5u);
    command(inst, 0x06u);
    expect_first_ack(inst);
    CHECK_EQ_U64(inst->cdrom.read_active, 1u);
    schway_destroy(inst);
}

static void test_region_unlock_lock(void)
{
    struct test_disc host;
    schway *inst = make();
    attach_disc(inst, &host, SCHWAY_DISC_TRACK_MODE2,
                "Sony Computer Entertainment Amer");
    profile(inst, SCHWAY_REGION_PAL, SCHWAY_PROFILE_CD_UNLOCK);
    unlock_disc(inst);
    CHECK_EQ_U64(inst->cdrom.unlock_enabled, 1u);
    command(inst, 0x57u);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x11u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x40u);
    CHECK_EQ_U64(inst->cdrom.unlock_enabled, 0u);
    acknowledge(inst, 5u);
    command(inst, 0x07u);
    expect_first_ack(inst);
    run_cycles(inst, 476000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 2u);
    (void)rd8(inst, 1u);
    acknowledge(inst, 2u);
    command(inst, 0x06u);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x03u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x40u);
    CHECK_EQ_U64(inst->cdrom.read_active, 0u);
    schway_destroy(inst);
}

static void test_region_unlock_partial_rejected(void)
{
    struct test_disc host;
    schway *inst = make();
    attach_disc(inst, &host, SCHWAY_DISC_TRACK_MODE2,
                "Sony Computer Entertainment Amer");
    profile(inst, SCHWAY_REGION_PAL, SCHWAY_PROFILE_CD_UNLOCK);
    run_unlock_part(inst, 0x50u, NULL);
    command(inst, 0x07u);
    expect_first_ack(inst);
    run_cycles(inst, 476000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 2u);
    (void)rd8(inst, 1u);
    acknowledge(inst, 2u);
    command(inst, 0x06u);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x03u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x40u);
    CHECK_EQ_U64(inst->cdrom.read_active, 0u);
    schway_destroy(inst);
}

static void test_region_unlock_rejected_in_japan(void)
{
    schway *inst = make();
    profile(inst, SCHWAY_REGION_NTSC_J, SCHWAY_PROFILE_CD_UNLOCK);
    command(inst, 0x50u);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x11u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x40u);
    CHECK_EQ_U64(inst->cdrom.unlock_enabled, 0u);
    schway_destroy(inst);
}

static void test_region_profile_override(void)
{
    struct test_disc host;
    schway *inst = make();
    attach_disc(inst, &host, SCHWAY_DISC_TRACK_MODE2, NULL);
    profile(inst, SCHWAY_REGION_NTSC_U, SCHWAY_PROFILE_CD_LICENSED);
    command(inst, 0x1Au);
    expect_first_ack(inst);
    run_cycles(inst, 0x4A00u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 2u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x02u);
    CHECK_EQ_U64(rd8(inst, 1u), 0u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x20u);
    CHECK_EQ_U64(rd8(inst, 1u), 0u);
    CHECK_EQ_U64(rd8(inst, 1u), 'S');
    CHECK_EQ_U64(rd8(inst, 1u), 'C');
    CHECK_EQ_U64(rd8(inst, 1u), 'E');
    CHECK_EQ_U64(rd8(inst, 1u), 'A');
    schway_destroy(inst);
}

static void test_region_firmware(void)
{
    schway *inst = make();
    profile(inst, SCHWAY_REGION_NTSC_J, 0u);
    param(inst, 0x20u);
    command(inst, 0x19u);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x96u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x09u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x12u);
    CHECK_EQ_U64(rd8(inst, 1u), 0xC2u);
    schway_destroy(inst);

    inst = make();
    profile(inst, SCHWAY_REGION_NTSC_U, SCHWAY_PROFILE_CD_UNLOCK);
    param(inst, 0x22u);
    command(inst, 0x19u);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    CHECK_EQ_U64(rd8(inst, 1u), 'f');
    CHECK_EQ_U64(rd8(inst, 1u), 'o');
    CHECK_EQ_U64(rd8(inst, 1u), 'r');
    CHECK_EQ_U64(rd8(inst, 1u), ' ');
    CHECK_EQ_U64(rd8(inst, 1u), 'U');
    CHECK_EQ_U64(rd8(inst, 1u), '/');
    CHECK_EQ_U64(rd8(inst, 1u), 'C');
    schway_destroy(inst);

    inst = make();
    profile(inst, SCHWAY_REGION_PAL, 0u);
    param(inst, 0x22u);
    command(inst, 0x19u);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 3u);
    CHECK_EQ_U64(inst->cdrom.result_count, 10u);
    static const uint8_t europe[10] = {'f', 'o', 'r', ' ', 'E', 'u', 'r', 'o', 'p', 'e'};
    for (uint32_t i = 0u; i < sizeof europe; i++) {
        CHECK_EQ_U64(rd8(inst, 1u), europe[i]);
    }
    schway_destroy(inst);
}

static void test_region_lid(void)
{
    schway *inst = make();
    CHECK_EQ_U64(schway_set_lid(inst, 1u), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(inst->cdrom.interrupt_count, 1u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x11u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x08u);
    acknowledge(inst, 5u);
    CHECK_EQ_U64(schway_set_lid(inst, 0u), SCHWAY_STATUS_OK);
    command(inst, 0x01u);
    expect_first_ack(inst);
    CHECK_EQ_U64(inst->cdrom.status & 0x10u, 0u);
    schway_destroy(inst);
}

static void test_region_invalid_commands(void)
{
    schway *inst = make();
    param(inst, 0x00u);
    param(inst, 0x6Au);
    param(inst, 0x00u);
    command(inst, 0x02u);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x01u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x10u);
    CHECK_EQ_U64(inst->cdrom.parameter_count, 3u);
    schway_destroy(inst);

    inst = make();
    param(inst, 1u);
    command(inst, 0x13u);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 1u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x20u);
    CHECK_EQ_U64(inst->cdrom.parameter_count, 1u);
    schway_destroy(inst);

    inst = make();
    command(inst, 0x17u);
    run_cycles(inst, 50000u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x11u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x40u);
    schway_destroy(inst);
}

static void test_region(void)
{
    test_region_no_disc_audio();
    test_region_unlicensed();
    test_region_image_matches();
    test_region_image_mismatches();
    test_region_unlock();
    test_region_unlock_partial_rejected();
    test_region_unlock_lock();
    test_region_unlock_rejected_in_japan();
    test_region_profile_override();
    test_region_firmware();
    test_region_lid();
    test_region_invalid_commands();
    test_mixed_track_seek();
}

int schway__cdrom_sector_test(const char *name);

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-cdrom <case>\n");
        return 2;
    }
    if (strcmp(argv[1], "command") == 0) {
        test_command();
        test_command_matrix();
    } else if (strcmp(argv[1], "timing") == 0) {
        test_timing();
    } else if (strcmp(argv[1], "bounds") == 0) {
        test_bounds();
    } else if (strcmp(argv[1], "region") == 0) {
        test_region();
    } else if (strcmp(argv[1], "sectors") == 0 || strcmp(argv[1], "delivery") == 0) {
        return schway__cdrom_sector_test(argv[1]);
    } else {
        fprintf(stderr, "unknown CD-ROM case: %s\n", argv[1]);
        return 2;
    }
    CHECK_DONE();
}
