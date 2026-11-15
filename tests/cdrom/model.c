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
    uint32_t track_type;
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
    if (lba == 4u && disc->license != NULL) {
        (void)memcpy(&sector[64], disc->license, 4u);
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
    host->track_type = track_type;
    host->license = license;
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

static void test_timing(void)
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

    /* Init drops duplicate commands while active, then completes at its fixed
       second-response deadline. */
    inst = make();
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

    /* Pause uses the selected 1x/2x delay; a repeated paused Pause is short. */
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

    /* Changing Setmode to 2x halves the unpaused Pause baseline. */
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

static void test_region(void)
{
    schway *inst = make();
    command(inst, 0x1Au); /* GetID with no installed disc. */
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

    /* Audio media fails distinctly. */
    struct test_disc host;
    inst = make();
    attach_disc(inst, &host, SCHWAY_DISC_TRACK_AUDIO, NULL);
    command(inst, 0x1Au);
    expect_first_ack(inst);
    run_cycles(inst, 0x4A00u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x0Au);
    CHECK_EQ_U64(rd8(inst, 1u), 0x90u);
    schway_destroy(inst);

    /* The host sector's region mark is compared with the machine profile. */
    inst = make();
    attach_disc(inst, &host, SCHWAY_DISC_TRACK_MODE2, "SCEE");
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
    CHECK(host.reads != 0u);
    schway_destroy(inst);

    inst = make();
    attach_disc(inst, &host, SCHWAY_DISC_TRACK_MODE2, "SCEA");
    profile(inst, SCHWAY_REGION_PAL, 0u);
    command(inst, 0x1Au);
    expect_first_ack(inst);
    run_cycles(inst, 0x4A00u);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x0Au);
    CHECK_EQ_U64(rd8(inst, 1u), 0x80u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x20u);
    schway_destroy(inst);

    /* Licensed override supplies the profile region when the image has no
       discoverable marker; no fixture-only switch is needed. */
    inst = make();
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

    /* Regional firmware constants are visible through Test 20h/22h. */
    inst = make();
    profile(inst, SCHWAY_REGION_NTSC_J, 0u);
    param(inst, 0x20u);
    command(inst, 0x19u);
    expect_first_ack(inst);
    CHECK_EQ_U64(rd8(inst, 1u), 0x96u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x09u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x12u);
    schway_destroy(inst);

    inst = make();
    profile(inst, SCHWAY_REGION_NTSC_U, SCHWAY_PROFILE_CD_UNLOCK);
    param(inst, 0x22u);
    command(inst, 0x19u);
    expect_first_ack(inst);
    CHECK_EQ_U64(rd8(inst, 1u), 'U');
    CHECK_EQ_U64(rd8(inst, 1u), '/');
    CHECK_EQ_U64(rd8(inst, 1u), 'C');
    schway_destroy(inst);

    /* Lid open is an unsolicited error; Nop clears sticky shell-open only
       after the host closes the lid. */
    inst = make();
    CHECK_EQ_U64(schway_set_lid(inst, 1u), SCHWAY_STATUS_OK);
    (void)hint_status(inst);
    CHECK_EQ_U64(hint_status(inst) & 7u, 5u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x11u);
    CHECK_EQ_U64(rd8(inst, 1u), 0x08u);
    acknowledge(inst, 5u);
    CHECK_EQ_U64(schway_set_lid(inst, 0u), SCHWAY_STATUS_OK);
    command(inst, 0x01u);
    expect_first_ack(inst);
    CHECK_EQ_U64(inst->cdrom.status & 0x10u, 0u);
    schway_destroy(inst);

    /* Invalid BCD Setloc reports INT5 and leaves its parameters available. */
    inst = make();
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
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-cdrom <case>\n");
        return 2;
    }
    if (strcmp(argv[1], "command") == 0) {
        test_command();
    } else if (strcmp(argv[1], "timing") == 0) {
        test_timing();
    } else if (strcmp(argv[1], "bounds") == 0) {
        test_bounds();
    } else if (strcmp(argv[1], "region") == 0) {
        test_region();
    } else {
        fprintf(stderr, "unknown CD-ROM case: %s\n", argv[1]);
        return 2;
    }
    CHECK_DONE();
}
