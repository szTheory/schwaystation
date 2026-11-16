/* CD audio enters the native SPU sample path through a bounded device queue. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"
#include "cdrom.h"
#include "spu.h"
#include "../check.h"

#define CDROM_BASE 0x1F801800u
#define SPU_ATTR 0x1F801DAAu
#define SPU_AVOLL 0x1F801DB0u
#define SPU_AVOLR 0x1F801DB2u
#define SPU_RAM_CTRL 0x1F801DACu
#define CDDA_SECTOR_CYCLES UINT64_C(451584)

static void *audio_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void audio_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static schway *audio_make(void)
{
    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = audio_alloc;
    config.allocator.free = audio_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&config, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) {
        exit(1);
    }
    return inst;
}

struct audio_disc {
    schway_toc toc;
    uint32_t reads;
    uint32_t last_lba;
    uint32_t xa;
};

static void audio_make_xa_sector(uint8_t sector[SCHWAY__CDROM_SECTOR_BYTES])
{
    memset(sector, 0, SCHWAY__CDROM_SECTOR_BYTES);
    memset(&sector[1], 0xFF, 10u);
    sector[15] = 2u;
    sector[16] = 1u;
    sector[17] = 2u;
    sector[18] = 0x6Cu;
    sector[19] = 1u;
    memcpy(&sector[20], &sector[16], 4u);
    for (uint32_t portion = 0u; portion < 18u; portion++) {
        uint8_t *chunk = &sector[24u + portion * 128u];
        memcpy(chunk, &chunk[4], 4u);
        memcpy(&chunk[12], &chunk[8], 4u);
        memset(&chunk[16], 0x11, 112u);
    }
}

static uint32_t audio_read_sector(void *user, uint32_t lba, uint8_t sector[2352],
                                  uint8_t subcode[96])
{
    struct audio_disc *disc = (struct audio_disc *)user;
    disc->reads++;
    disc->last_lba = lba;
    if (disc->xa != 0u) {
        audio_make_xa_sector(sector);
        memset(subcode, 0, 96u);
        return 1u;
    }
    for (uint32_t frame = 0u; frame < 588u; frame++) {
        uint32_t offset = frame * 4u;
        uint16_t left = (uint16_t)(0x1234u + lba);
        sector[offset] = (uint8_t)left;
        sector[offset + 1u] = (uint8_t)(left >> 8);
        sector[offset + 2u] = 0xDCu;
        sector[offset + 3u] = 0xFEu;
    }
    memset(subcode, 0, 96u);
    return 1u;
}

static uint32_t audio_read_toc(void *user, schway_toc *toc)
{
    struct audio_disc *disc = (struct audio_disc *)user;
    memcpy(toc, &disc->toc, sizeof *toc);
    return 1u;
}

static void audio_attach_disc_type(schway *inst, struct audio_disc *host, uint32_t track_type)
{
    memset(host, 0, sizeof *host);
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
    disc.read_sector = audio_read_sector;
    disc.toc = audio_read_toc;
    CHECK_EQ_U64(schway_open_disc(inst, &disc), SCHWAY_STATUS_OK);
}

static void audio_attach_disc(schway *inst, struct audio_disc *host)
{
    audio_attach_disc_type(inst, host, SCHWAY_DISC_TRACK_AUDIO);
}

static void audio_attach_xa_disc(schway *inst, struct audio_disc *host)
{
    audio_attach_disc_type(inst, host, SCHWAY_DISC_TRACK_MODE2);
    host->xa = 1u;
}

static void audio_run_cycles(schway *inst, uint64_t cycles)
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
        if (elapsed == 0u || elapsed > cycles) {
            return;
        }
        cycles -= elapsed;
    }
}

static int16_t signed_sample(uint16_t value)
{
    return value < 0x8000u ? (int16_t)value : (int16_t)((int32_t)value - 0x10000);
}

static uint16_t read_cd_audio_word(const schway *inst, uint32_t pair, uint32_t channel)
{
    uint32_t slot = (inst->cdrom.audio_head + pair) % SCHWAY__CDROM_AUDIO_CAP;
    return inst->cdrom.audio[slot][channel];
}

static void cdda_queue_drains_at_native_tick(void)
{
    schway *inst = audio_make();
    schway__cdrom_audio_push(inst, 1200, -2400);
    inst->spu.reg[(SPU_ATTR - 0x1F801C00u) >> 1] = 0x0001u;
    inst->spu.reg[(SPU_AVOLL - 0x1F801C00u) >> 1] = 0x7FFFu;
    inst->spu.reg[(SPU_AVOLR - 0x1F801C00u) >> 1] = 0x7FFFu;
    inst->spu.reg[(SPU_RAM_CTRL - 0x1F801C00u) >> 1] = 0x0004u;

    schway__spu_event(inst, inst->spu.next_tick);

    CHECK_EQ_U64(inst->cdrom.audio_count, 0u);
    CHECK_EQ_U64(inst->audio_count, 1u);
    CHECK_EQ_U64((uint16_t)inst->audio[inst->audio_head][0], 1199u);
    CHECK_EQ_U64((uint16_t)inst->audio[inst->audio_head][1], (uint16_t)-2400);
    CHECK_EQ_U64(schway__spu_ram_peek(inst, 0u), 1200u);
    CHECK_EQ_U64(schway__spu_ram_peek(inst, 0x400u), (uint16_t)-2400);

    schway__spu_event(inst, inst->spu.next_tick);
    CHECK_EQ_U64(inst->audio_count, 2u);
    CHECK_EQ_U64((uint16_t)inst->audio[(inst->audio_head + 1u) % SCHWAY__AUDIO_RING_PAIRS][0], 0u);
    CHECK_EQ_U64(schway__spu_ram_peek(inst, 2u), 0u);

    schway__cdrom_audio_push(inst, 1111, 2222);
    inst->cdrom.muted = 1u;
    schway__spu_event(inst, inst->spu.next_tick);
    CHECK_EQ_U64(inst->cdrom.audio_count, 0u);
    CHECK_EQ_U64((uint16_t)inst->audio[(inst->audio_head + 2u) % SCHWAY__AUDIO_RING_PAIRS][0], 0u);
    CHECK_EQ_U64(schway__spu_ram_peek(inst, 4u), 1111u);
    schway_destroy(inst);
}

static void cdda_atv_applies_atomically_and_clips(void)
{
    schway *inst = audio_make();
    schway__cdrom_write(inst, CDROM_BASE, 2u, 0u);
    schway__cdrom_write(inst, CDROM_BASE + 2u, 0u, 0u);
    schway__cdrom_write(inst, CDROM_BASE + 3u, 0x80u, 0u);
    schway__cdrom_write(inst, CDROM_BASE, 3u, 0u);
    schway__cdrom_write(inst, CDROM_BASE + 1u, 0u, 0u);
    schway__cdrom_write(inst, CDROM_BASE + 2u, 0x80u, 0u);
    schway__cdrom_audio_push(inst, 1000, 2000);
    CHECK_EQ_U64(signed_sample(read_cd_audio_word(inst, 0u, 0u)), 1000u);
    CHECK_EQ_U64(signed_sample(read_cd_audio_word(inst, 0u, 1u)), 2000u);

    schway__cdrom_write(inst, CDROM_BASE + 3u, 0x20u, 0u);
    schway__cdrom_audio_push(inst, 1000, 2000);
    CHECK_EQ_U64(signed_sample(read_cd_audio_word(inst, 1u, 0u)), 2000u);
    CHECK_EQ_U64(signed_sample(read_cd_audio_word(inst, 1u, 1u)), 1000u);

    schway__cdrom_write(inst, CDROM_BASE, 2u, 0u);
    schway__cdrom_write(inst, CDROM_BASE + 2u, 0xFFu, 0u);
    schway__cdrom_write(inst, CDROM_BASE + 3u, 0xFFu, 0u);
    schway__cdrom_write(inst, CDROM_BASE, 3u, 0u);
    schway__cdrom_write(inst, CDROM_BASE + 1u, 0xFFu, 0u);
    schway__cdrom_write(inst, CDROM_BASE + 2u, 0xFFu, 0u);
    schway__cdrom_write(inst, CDROM_BASE + 3u, 0x20u, 0u);
    schway__cdrom_audio_push(inst, 30000, 30000);
    CHECK_EQ_U64(read_cd_audio_word(inst, 2u, 0u), INT16_MAX);
    CHECK_EQ_U64(read_cd_audio_word(inst, 2u, 1u), INT16_MAX);
    schway_destroy(inst);
}

static void cdda_source_and_native_rates(uint32_t speed)
{
    schway *inst = audio_make();
    struct audio_disc host;
    audio_attach_disc(inst, &host);
    inst->spu.reg[(SPU_ATTR - 0x1F801C00u) >> 1] = 0x0001u;
    inst->spu.reg[(SPU_AVOLL - 0x1F801C00u) >> 1] = 0x7FFFu;
    inst->spu.reg[(SPU_AVOLR - 0x1F801C00u) >> 1] = 0x7FFFu;
    inst->cdrom.play_active = 1u;
    inst->cdrom.motor_on = 1u;
    inst->cdrom.drive_speed = speed;
    schway__cdrom_sector_schedule(inst, 0u);

    uint64_t period = CDDA_SECTOR_CYCLES / speed;
    audio_run_cycles(inst, period);
    CHECK_EQ_U64(host.reads, 1u);
    CHECK_EQ_U64(host.last_lba, 0u);
    CHECK_EQ_U64(inst->cdrom.current_lba, 0u);
    CHECK_EQ_U64(inst->cdrom.target_lba, 1u);
    CHECK_EQ_U64(inst->cdrom.audio_count, speed == 1u ? 0u : 294u);
    CHECK_EQ_U64(inst->cdrom.audio[0][0], 0x1234u);
    CHECK_EQ_U64(inst->cdrom.audio[0][1], 0xFEDCu);
    CHECK_EQ_U64(inst->audio_count, period / 768u);
    uint32_t pcm_last = (inst->audio_head + inst->audio_count - 1u) % SCHWAY__AUDIO_RING_PAIRS;
    CHECK_EQ_U64((uint16_t)inst->audio[pcm_last][0], 4659u);
    CHECK_EQ_U64((uint16_t)inst->audio[pcm_last][1], (uint16_t)(int16_t)-292);

    audio_run_cycles(inst, period);
    CHECK_EQ_U64(host.reads, 2u);
    CHECK_EQ_U64(host.last_lba, 1u);
    CHECK_EQ_U64(inst->cdrom.target_lba, 2u);
    CHECK_EQ_U64(inst->audio_count, 2u * (period / 768u));
    if (speed == 1u) {
        CHECK_EQ_U64(inst->cdrom.audio_count, 587u);
    } else {
        audio_run_cycles(inst, 3u * period);
        CHECK_EQ_U64(host.reads, 5u);
        CHECK_EQ_U64(host.last_lba, 4u);
        CHECK_EQ_U64(inst->cdrom.target_lba, 5u);
        CHECK(inst->cdrom.audio_count <= SCHWAY__CDROM_AUDIO_LIMIT);
        CHECK_EQ_U64(inst->audio_count, 5u * (period / 768u));
    }
    schway_destroy(inst);
}

static void cdda_two_speed_overrun_drops_newest(void)
{
    schway *inst = audio_make();
    struct audio_disc host;
    audio_attach_disc(inst, &host);
    inst->cdrom.play_active = 1u;
    inst->cdrom.motor_on = 1u;
    inst->cdrom.drive_speed = 2u;
    inst->spu.next_tick = UINT64_MAX;
    schway__cdrom_sector_schedule(inst, 0u);

    uint64_t period = CDDA_SECTOR_CYCLES / 2u;
    for (uint32_t sector = 0u; sector < 3u; sector++) {
        audio_run_cycles(inst, period);
    }
    CHECK_EQ_U64(host.reads, 3u);
    CHECK_EQ_U64(host.last_lba, 2u);
    CHECK_EQ_U64(inst->cdrom.target_lba, 3u);
    CHECK_EQ_U64(inst->cdrom.audio_count, SCHWAY__CDROM_AUDIO_LIMIT);
    CHECK_EQ_U64(read_cd_audio_word(inst, SCHWAY__CDROM_AUDIO_LIMIT - 1u, 0u), 0x1235u);
    CHECK_EQ_U64(read_cd_audio_word(inst, SCHWAY__CDROM_AUDIO_LIMIT - 1u, 1u), 0xFEDCu);

    schway__cdrom_audio_push(inst, 123, 456);
    CHECK_EQ_U64(inst->cdrom.audio_count, SCHWAY__CDROM_AUDIO_LIMIT);
    CHECK_EQ_U64(read_cd_audio_word(inst, SCHWAY__CDROM_AUDIO_LIMIT - 1u, 0u), 0x1235u);
    schway_destroy(inst);
}

static void cdda_two_speed_probe(void)
{
    cdda_source_and_native_rates(1u);
    cdda_source_and_native_rates(2u);
    cdda_two_speed_overrun_drops_newest();
}

static void xa_sector_reaches_native_audio(void)
{
    schway *inst = audio_make();
    struct audio_disc host;
    audio_attach_xa_disc(inst, &host);
    inst->cdrom.mode = 0x40u;
    inst->cdrom.read_active = 1u;
    inst->cdrom.motor_on = 1u;
    inst->cdrom.drive_speed = 2u;
    inst->spu.reg[(SPU_ATTR - 0x1F801C00u) >> 1] = 0x0001u;
    inst->spu.reg[(SPU_AVOLL - 0x1F801C00u) >> 1] = 0x7FFFu;
    inst->spu.reg[(SPU_AVOLR - 0x1F801C00u) >> 1] = 0x7FFFu;
    schway__cdrom_sector_schedule(inst, 0u);

    audio_run_cycles(inst, CDDA_SECTOR_CYCLES / 2u);
    CHECK_EQ_U64(host.reads, 1u);
    CHECK_EQ_U64(host.last_lba, 0u);
    CHECK_EQ_U64(inst->cdrom.sector_count, 0u);
    for (uint32_t tick = 0u; tick < 64u; tick++) {
        schway__spu_event(inst, inst->spu.next_tick);
    }
    uint32_t nonzero = 0u;
    for (uint32_t i = 0u; i < inst->audio_count; i++) {
        uint32_t slot = (inst->audio_head + i) % SCHWAY__AUDIO_RING_PAIRS;
        if (inst->audio[slot][0] != 0 || inst->audio[slot][1] != 0) {
            nonzero = 1u;
        }
    }
    CHECK_EQ_U64(nonzero, 1u);
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    if (argc != 2 || (strcmp(argv[1], "cdda") != 0 && strcmp(argv[1], "xa") != 0)) {
        return 2;
    }
    if (strcmp(argv[1], "cdda") == 0) {
        cdda_queue_drains_at_native_tick();
        cdda_atv_applies_atomically_and_clips();
        cdda_two_speed_probe();
    } else {
        xa_sector_reaches_native_audio();
    }
    return check_failures == 0 ? 0 : 1;
}
