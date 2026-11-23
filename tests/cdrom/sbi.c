/* Good-Q ReadN/GetlocP control for the CD-ROM MMIO path. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "../check.h"

#define CDROM 0x1F801800u
#define SECTOR_PERIOD UINT64_C(451584)

struct sbi_host {
    schway_toc toc;
    uint32_t reads;
};

static void *sbi_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void sbi_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static uint16_t q_crc(const uint8_t q[12])
{
    uint16_t crc = 0u;
    for (uint32_t i = 0u; i < 10u; i++) {
        crc ^= (uint16_t)q[i] << 8;
        for (uint32_t bit = 0u; bit < 8u; bit++) {
            uint32_t shifted = (uint32_t)crc << 1;
            crc = (crc & 0x8000u) != 0u ? (uint16_t)(shifted ^ 0x1021u)
                                        : (uint16_t)shifted;
        }
    }
    return (uint16_t)~crc;
}

static uint32_t sbi_read_sector(void *user, uint32_t lba, uint8_t sector[2352],
                                uint8_t subcode[96])
{
    struct sbi_host *host = (struct sbi_host *)user;
    uint8_t q[12] = {0x41u, 0x01u, 0x01u, 0x00u, 0x00u, 0x00u,
                     0x00u, 0x00u, 0x02u, 0x00u, 0x00u, 0x00u};
    (void)lba;
    host->reads++;
    memset(sector, 0, 2352u);
    memset(subcode, 0, 96u);
    memset(sector + 1u, 0xFF, 10u);
    sector[15] = 2u;
    sector[16] = 1u;
    sector[17] = 2u;
    sector[18] = 8u;
    sector[20] = 1u;
    sector[21] = 2u;
    sector[22] = 8u;
    uint16_t crc = q_crc(q);
    q[10] = (uint8_t)(crc >> 8);
    q[11] = (uint8_t)crc;
    for (uint32_t bit = 0u; bit < 96u; bit++) {
        if ((q[bit / 8u] & (1u << (7u - bit % 8u))) != 0u) {
            subcode[bit] |= 0x40u;
        }
    }
    return 1u;
}

static uint32_t sbi_read_toc(void *user, schway_toc *out)
{
    struct sbi_host *host = (struct sbi_host *)user;
    memcpy(out, &host->toc, sizeof *out);
    return 1u;
}

static void sbi_wr8(schway *inst, uint32_t offset, uint8_t value)
{
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, CDROM + offset, 1u, value, &cycles), 0u);
}

static uint8_t sbi_rd8(schway *inst, uint32_t offset)
{
    uint32_t cycles = 0u;
    uint32_t value = 0u;
    CHECK_EQ_U64(schway__bus_read(inst, CDROM + offset, 1u, &value, &cycles), 0u);
    return (uint8_t)value;
}

static void sbi_run(schway *inst, uint64_t cycles)
{
    while (cycles != 0u) {
        schway_budget budget;
        schway_run_result result;
        memset(&budget, 0, sizeof budget);
        budget.size = (uint32_t)sizeof budget;
        budget.max_cycles = cycles;
        memset(&result, 0, sizeof result);
        result.size = (uint32_t)sizeof result;
        uint64_t before = inst->ticks;
        CHECK_EQ_U64(schway_run(inst, &budget, &result), SCHWAY_STATUS_OK);
        uint64_t elapsed = result.ticks - before;
        CHECK(elapsed != 0u && elapsed <= cycles);
        if (elapsed == 0u || elapsed > cycles) return;
        cycles -= elapsed;
    }
}

static uint8_t sbi_irq(schway *inst)
{
    sbi_wr8(inst, 0u, 1u);
    return (uint8_t)(sbi_rd8(inst, 3u) & 7u);
}

static void sbi_ack(schway *inst, uint8_t irq)
{
    sbi_wr8(inst, 0u, 1u);
    sbi_wr8(inst, 3u, (uint8_t)(1u << (irq - 1u)));
}

static void test_good_q_readn_getlocp(void)
{
    struct sbi_host host;
    memset(&host, 0, sizeof host);
    host.toc.size = (uint32_t)sizeof host.toc;
    host.toc.track_count = 1u;
    host.toc.tracks[0].size = (uint32_t)sizeof host.toc.tracks[0];
    host.toc.tracks[0].type = SCHWAY_DISC_TRACK_MODE2;
    host.toc.tracks[0].leadout = 1000u;
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = sbi_alloc;
    cfg.allocator.free = sbi_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) return;
    schway_disc disc;
    memset(&disc, 0, sizeof disc);
    disc.size = (uint32_t)sizeof disc;
    disc.user = &host;
    disc.read_sector = sbi_read_sector;
    disc.toc = sbi_read_toc;
    CHECK_EQ_U64(schway_open_disc(inst, &disc), SCHWAY_STATUS_OK);
    schway_profile profile;
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = SCHWAY_REGION_NTSC_U;
    profile.disc_flags = SCHWAY_PROFILE_CD_LICENSED;
    CHECK_EQ_U64(schway_set_profile(inst, &profile), SCHWAY_STATUS_OK);

    sbi_wr8(inst, 0u, 0u);
    sbi_wr8(inst, 1u, 0x06u);
    sbi_run(inst, 50000u);
    CHECK_EQ_U64(sbi_irq(inst), 3u);
    (void)sbi_rd8(inst, 1u);
    sbi_ack(inst, 3u);
    sbi_run(inst, SECTOR_PERIOD);
    CHECK_EQ_U64(host.reads, 1u);
    CHECK_EQ_U64(inst->cdrom.q_record_valid, 1u);
    CHECK_EQ_U64(sbi_irq(inst), 1u);
    sbi_ack(inst, 1u);

    sbi_wr8(inst, 0u, 0u);
    sbi_wr8(inst, 1u, 0x11u);
    sbi_run(inst, 50000u);
    CHECK_EQ_U64(sbi_irq(inst), 3u);
    static const uint8_t expected[8] = {0x01u, 0x01u, 0x00u, 0x02u,
                                        0x00u, 0x00u, 0x00u, 0x00u};
    for (uint32_t i = 0u; i < sizeof expected; i++) {
        CHECK_EQ_U64(sbi_rd8(inst, 1u), expected[i]);
    }
    sbi_ack(inst, 3u);
    schway_destroy(inst);
}

int main(void)
{
    test_good_q_readn_getlocp();
    CHECK_DONE();
}
