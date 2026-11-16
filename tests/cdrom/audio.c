/* CD audio enters the native SPU sample path through a bounded device queue. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"
#include "cdrom.h"
#include "cdxa.h"
#include "spu.h"
#include "../check.h"

#define CDROM_BASE 0x1F801800u
#define SPU_ATTR 0x1F801DAAu
#define SPU_AVOLL 0x1F801DB0u
#define SPU_AVOLR 0x1F801DB2u
#define SPU_RAM_CTRL 0x1F801DACu
#define SPU_NON 0x1F801D94u
#define SPU_EON0 0x1F801D98u
#define SPU_EVOL_L 0x1F801D84u
#define SPU_EVOL_R 0x1F801D86u
#define SPU_MAIN_VOL_L 0x1F801D80u
#define SPU_MAIN_VOL_R 0x1F801D82u
#define SPU_IIR 0x1F801DC4u
#define SPU_COMB1 0x1F801DC6u
#define SPU_WALL 0x1F801DCEu
#define SPU_LSAME 0x1F801DD4u
#define SPU_RSAME 0x1F801DD6u
#define SPU_LCOMB1 0x1F801DD8u
#define SPU_RCOMB1 0x1F801DDAu
#define SPU_LDIFF 0x1F801DE4u
#define SPU_RDIFF 0x1F801DE6u
#define SPU_LSAME_DELAY 0x1F801DE0u
#define SPU_RSAME_DELAY 0x1F801DE2u
#define SPU_LDIFF_DELAY 0x1F801DF0u
#define SPU_RDIFF_DELAY 0x1F801DF2u
#define SPU_APF1_VOLUME 0x1F801DD0u
#define SPU_APF2_VOLUME 0x1F801DD2u
#define SPU_LAPF1 0x1F801DF4u
#define SPU_RAPF1 0x1F801DF6u
#define SPU_LAPF2 0x1F801DF8u
#define SPU_RAPF2 0x1F801DFAu
#define SPU_VLIN 0x1F801DFCu
#define SPU_VRIN 0x1F801DFEu
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
    uint32_t xa_every;
    uint32_t xa_reads;
    uint8_t xa_file, xa_channel, xa_submode, xa_coding, xa_sample;
};

static void audio_make_xa_sector(uint8_t sector[SCHWAY__CDROM_SECTOR_BYTES],
                                 const struct audio_disc *disc)
{
    memset(sector, 0, SCHWAY__CDROM_SECTOR_BYTES);
    memset(&sector[1], 0xFF, 10u);
    sector[15] = 2u;
    sector[16] = disc->xa_file;
    sector[17] = disc->xa_channel;
    sector[18] = disc->xa_submode;
    sector[19] = disc->xa_coding;
    memcpy(&sector[20], &sector[16], 4u);
    for (uint32_t portion = 0u; portion < 18u; portion++) {
        uint8_t *chunk = &sector[24u + portion * 128u];
        memcpy(chunk, &chunk[4], 4u);
        memcpy(&chunk[12], &chunk[8], 4u);
        memset(&chunk[16], disc->xa_sample, 112u);
    }
}

static void audio_make_data_sector(uint8_t sector[SCHWAY__CDROM_SECTOR_BYTES])
{
    memset(sector, 0, SCHWAY__CDROM_SECTOR_BYTES);
    memset(&sector[1], 0xFF, 10u);
    sector[15] = 2u;
    sector[16] = 1u;
    sector[17] = 2u;
    sector[18] = 0x08u;
    memcpy(&sector[20], &sector[16], 4u);
}

static uint32_t audio_read_sector(void *user, uint32_t lba, uint8_t sector[2352],
                                  uint8_t subcode[96])
{
    struct audio_disc *disc = (struct audio_disc *)user;
    disc->reads++;
    disc->last_lba = lba;
    if (disc->xa != 0u &&
        (disc->xa_every == 0u || (lba % disc->xa_every) == 0u)) {
        audio_make_xa_sector(sector, disc);
        disc->xa_reads++;
        memset(subcode, 0, 96u);
        return 1u;
    }
    if (disc->xa != 0u) {
        audio_make_data_sector(sector);
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
    host->xa_file = 1u;
    host->xa_channel = 2u;
    host->xa_submode = 0x6Cu;
    host->xa_coding = 1u;
    host->xa_sample = 0x11u;
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

static uint32_t audio_spu_reg_index(uint32_t address)
{
    return (address - 0x1F801C00u) >> 1;
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
    CHECK_EQ_U64(host.xa_reads, 1u);
    CHECK_EQ_U64(host.last_lba, 0u);
    CHECK_EQ_U64(inst->cdrom.sector_count, 0u);
    CHECK(inst->cdrom.xa_audio_count != 0u);
    CHECK(inst->cdrom.xa_adpcm_history[0][0] != 0);
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

static void audio_drain_host_output(schway *inst)
{
    int16_t samples[SCHWAY__AUDIO_RING_PAIRS][2];
    schway_audio audio;
    memset(&audio, 0, sizeof audio);
    audio.size = (uint32_t)sizeof audio;
    audio.samples = &samples[0][0];
    audio.capacity = SCHWAY__AUDIO_RING_PAIRS;
    CHECK_EQ_U64(schway_get_audio(inst, &audio), SCHWAY_STATUS_OK);
}

static void xa_prepare_read(schway *inst, struct audio_disc *host, uint32_t every,
                            uint8_t coding)
{
    audio_attach_xa_disc(inst, host);
    host->xa_every = every;
    host->xa_coding = coding;
    inst->cdrom.mode = 0x40u;
    inst->cdrom.read_active = 1u;
    inst->cdrom.motor_on = 1u;
    inst->cdrom.drive_speed = 2u;
    inst->spu.reg[(SPU_ATTR - 0x1F801C00u) >> 1] = 0x0001u;
    inst->spu.reg[(SPU_AVOLL - 0x1F801C00u) >> 1] = 0x7FFFu;
    inst->spu.reg[(SPU_AVOLR - 0x1F801C00u) >> 1] = 0x7FFFu;
    schway__cdrom_sector_schedule(inst, 0u);
}

static void xa_interleaved_cadence(uint32_t coding, uint32_t interleave,
                                   uint32_t packet_frames)
{
    schway *inst = audio_make();
    struct audio_disc host;
    xa_prepare_read(inst, &host, interleave, (uint8_t)coding);
    uint64_t period = CDDA_SECTOR_CYCLES / 2u;
    uint32_t xa_reads = 0u;
    for (uint32_t sector = 0u; sector <= interleave; sector++) {
        uint32_t pending = inst->cdrom.xa_audio_count;
        audio_run_cycles(inst, period);
        if (host.xa_reads != xa_reads) {
            xa_reads = host.xa_reads;
            CHECK_EQ_U64(xa_reads, sector == 0u ? 1u : 2u);
            CHECK_EQ_U64(inst->cdrom.xa_audio_count,
                         pending + packet_frames -
                             (sector == 0u ? (uint32_t)(period / 768u) : 1u));
        } else {
            CHECK_EQ_U64(inst->cdrom.xa_audio_count,
                         pending > period / 768u ? pending - (uint32_t)(period / 768u) : 0u);
        }
        audio_drain_host_output(inst);
    }
    CHECK_EQ_U64(host.reads, interleave + 1u);
    CHECK_EQ_U64(host.xa_reads, 2u);
    CHECK(inst->cdrom.xa_audio_count <= SCHWAY__CDROM_XA_AUDIO_CAP);
    schway_destroy(inst);
}

static void xa_interleave_and_boundary_capacity(void)
{
    xa_interleaved_cadence(1u, 8u, 2352u);
    xa_interleaved_cadence(4u, 32u, SCHWAY__CDXA_MAX_OUTPUT_FRAMES);

    schway *inst = audio_make();
    struct audio_disc host;
    xa_prepare_read(inst, &host, 0u, 4u);
    uint64_t deadline = inst->cdrom.sector_deadline;
    schway__cdrom_xa_audio_push(inst, 123, -456);
    inst->spu.next_tick = deadline;
    inst->run_active = 1u;
    inst->ticks = deadline;
    schway__sched_run(inst);
    CHECK_EQ_U64(host.xa_reads, 1u);
    CHECK_EQ_U64(inst->cdrom.xa_audio_head, 1u);
    CHECK_EQ_U64(inst->cdrom.xa_audio_count, SCHWAY__CDXA_MAX_OUTPUT_FRAMES);
    uint32_t last = (inst->cdrom.xa_audio_head + inst->cdrom.xa_audio_count - 1u) %
                    SCHWAY__CDROM_XA_AUDIO_CAP;
    CHECK_EQ_U64(inst->cdrom.xa_audio[last][0],
                 (uint16_t)inst->xa_decode_scratch[2u * (SCHWAY__CDXA_MAX_OUTPUT_FRAMES - 1u)]);
    CHECK_EQ_U64(inst->cdrom.xa_audio[last][1],
                 (uint16_t)inst->xa_decode_scratch[2u * (SCHWAY__CDXA_MAX_OUTPUT_FRAMES - 1u) + 1u]);
    schway_destroy(inst);
}

static void xa_filter_and_mute_controls(void)
{
    schway *accepted = audio_make();
    struct audio_disc accepted_host;
    xa_prepare_read(accepted, &accepted_host, 0u, 1u);
    accepted->cdrom.mode = 0x48u;
    accepted->cdrom.file = 1u;
    accepted->cdrom.channel = 2u;
    audio_run_cycles(accepted, CDDA_SECTOR_CYCLES / 2u);
    CHECK_EQ_U64(accepted_host.xa_reads, 1u);
    CHECK(accepted->cdrom.xa_audio_count != 0u);
    CHECK_EQ_U64(accepted->cdrom.sector_count, 0u);
    schway_destroy(accepted);

    schway *rejected = audio_make();
    struct audio_disc rejected_host;
    xa_prepare_read(rejected, &rejected_host, 0u, 1u);
    rejected->cdrom.mode = 0u;
    audio_run_cycles(rejected, CDDA_SECTOR_CYCLES / 2u);
    CHECK_EQ_U64(rejected_host.xa_reads, 1u);
    CHECK_EQ_U64(rejected->cdrom.xa_audio_count, 0u);
    CHECK_EQ_U64(rejected->cdrom.sector_count, 1u);
    schway_destroy(rejected);

    schway *muted = audio_make();
    struct audio_disc muted_host;
    xa_prepare_read(muted, &muted_host, 0u, 1u);
    muted->cdrom.muted = 1u;
    muted->spu.reg[(SPU_RAM_CTRL - 0x1F801C00u) >> 1] = 0x0004u;
    audio_run_cycles(muted, CDDA_SECTOR_CYCLES / 2u);
    uint32_t captured = 0u;
    uint32_t audible = 0u;
    for (uint32_t i = 0u; i < muted->audio_count; i++) {
        uint32_t slot = (muted->audio_head + i) % SCHWAY__AUDIO_RING_PAIRS;
        if (muted->audio[slot][0] != 0 || muted->audio[slot][1] != 0) audible = 1u;
    }
    for (uint32_t pair = 0u; pair < muted->spu.capture_cursor; pair++) {
        if (schway__spu_ram_peek(muted, pair * 2u) != 0u ||
            schway__spu_ram_peek(muted, 0x400u + pair * 2u) != 0u) captured = 1u;
    }
    CHECK_EQ_U64(audible, 0u);
    CHECK_EQ_U64(captured, 1u);
    CHECK(muted->cdrom.xa_adpcm_history[0][0] != 0);
    CHECK(muted->cdrom.xa_audio_count != 0u);
    schway_destroy(muted);
}

static void xa_file_channel_mismatch_retries_then_skips(void)
{
    schway *inst = audio_make();
    struct audio_disc host;
    xa_prepare_read(inst, &host, 0u, 1u);
    inst->cdrom.mode = 0x48u;
    inst->cdrom.file = 9u;
    inst->cdrom.channel = 2u;

    audio_run_cycles(inst, CDDA_SECTOR_CYCLES / 2u);
    CHECK_EQ_U64(host.reads, 1u);
    CHECK_EQ_U64(host.xa_reads, 1u);
    CHECK_EQ_U64(inst->cdrom.xa_audio_count, 0u);
    CHECK_EQ_U64(inst->cdrom.sector_count, 0u);
    CHECK_EQ_U64(inst->cdrom.sector_retry, 1u);
    CHECK_EQ_U64(inst->cdrom.target_lba, 0u);

    audio_run_cycles(inst, CDDA_SECTOR_CYCLES / 2u);
    CHECK_EQ_U64(host.reads, 2u);
    CHECK_EQ_U64(host.xa_reads, 2u);
    CHECK_EQ_U64(host.last_lba, 0u);
    CHECK_EQ_U64(inst->cdrom.xa_audio_count, 0u);
    CHECK_EQ_U64(inst->cdrom.sector_count, 0u);
    CHECK_EQ_U64(inst->cdrom.sector_retry, 0u);
    CHECK_EQ_U64(inst->cdrom.target_lba, 1u);
    schway_destroy(inst);
}

static void xa_reset_lid_and_instance_controls(void)
{
    schway *first = audio_make();
    schway *second = audio_make();
    struct audio_disc first_host, second_host;
    xa_prepare_read(first, &first_host, 0u, 1u);
    xa_prepare_read(second, &second_host, 0u, 1u);
    second_host.xa_sample = 0x22u;
    audio_run_cycles(first, CDDA_SECTOR_CYCLES / 2u);
    audio_run_cycles(second, CDDA_SECTOR_CYCLES / 2u);
    CHECK(first->cdrom.xa_adpcm_history[0][0] != second->cdrom.xa_adpcm_history[0][0]);
    CHECK(first->cdrom.xa_audio_count != 0u && second->cdrom.xa_audio_count != 0u);

    schway__cdrom_audio_push(first, 100, 200);
    schway__cdrom_reset(first);
    CHECK_EQ_U64(first->cdrom.audio_count, 0u);
    CHECK_EQ_U64(first->cdrom.xa_audio_count, 0u);
    CHECK_EQ_U64(first->cdrom.xa_audio_head, 0u);

    schway__cdrom_audio_push(second, 100, 200);
    second->disc_host.lid_open = 1u;
    schway__cdrom_lid_changed(second);
    CHECK_EQ_U64(second->cdrom.audio_count, 0u);
    CHECK_EQ_U64(second->cdrom.xa_audio_count, 0u);
    schway_destroy(second);
    schway_destroy(first);
}

static void xa_and_cdda_mix_at_native_tick(void)
{
    schway *inst = audio_make();
    schway__cdrom_audio_push(inst, 1200, -2400);
    schway__cdrom_xa_audio_push(inst, 2400, 4800);
    inst->spu.reg[(SPU_ATTR - 0x1F801C00u) >> 1] = 0x0001u;
    inst->spu.reg[(SPU_AVOLL - 0x1F801C00u) >> 1] = 0x7FFFu;
    inst->spu.reg[(SPU_AVOLR - 0x1F801C00u) >> 1] = 0x7FFFu;
    schway__spu_event(inst, inst->spu.next_tick);
    CHECK_EQ_U64(inst->cdrom.audio_count, 0u);
    CHECK_EQ_U64(inst->cdrom.xa_audio_count, 0u);
    CHECK_EQ_U64((uint16_t)inst->audio[inst->audio_head][0], 3599u);
    CHECK_EQ_U64((uint16_t)inst->audio[inst->audio_head][1], 2399u);
    schway_destroy(inst);
}

static void xa_setup_voice_reverb(schway *inst, uint32_t send_cd)
{
    inst->spu.reg[audio_spu_reg_index(SPU_ATTR)] = (uint16_t)(0xC081u | (send_cd != 0u ? 4u : 0u));
    inst->spu.reg[audio_spu_reg_index(SPU_NON)] = 1u;
    inst->spu.reg[audio_spu_reg_index(SPU_EON0)] = 1u;
    inst->spu.reg[audio_spu_reg_index(SPU_EVOL_L)] = 0x7FFFu;
    inst->spu.reg[audio_spu_reg_index(SPU_EVOL_R)] = 0x7FFFu;
    inst->spu.reg[audio_spu_reg_index(SPU_IIR)] = 0x7FFFu;
    inst->spu.reg[audio_spu_reg_index(SPU_COMB1)] = 0x7FFFu;
    inst->spu.reg[audio_spu_reg_index(SPU_WALL)] = 0u;
    inst->spu.reg[audio_spu_reg_index(SPU_LSAME)] = 2u;
    inst->spu.reg[audio_spu_reg_index(SPU_RSAME)] = 2u;
    inst->spu.reg[audio_spu_reg_index(SPU_LCOMB1)] = 2u;
    inst->spu.reg[audio_spu_reg_index(SPU_RCOMB1)] = 2u;
    inst->spu.reg[audio_spu_reg_index(SPU_LSAME_DELAY)] = 0u;
    inst->spu.reg[audio_spu_reg_index(SPU_RSAME_DELAY)] = 0u;
    inst->spu.reg[audio_spu_reg_index(SPU_LDIFF)] = 2u;
    inst->spu.reg[audio_spu_reg_index(SPU_RDIFF)] = 2u;
    inst->spu.reg[audio_spu_reg_index(SPU_LDIFF_DELAY)] = 0u;
    inst->spu.reg[audio_spu_reg_index(SPU_RDIFF_DELAY)] = 0u;
    inst->spu.reg[audio_spu_reg_index(SPU_APF1_VOLUME)] = 0u;
    inst->spu.reg[audio_spu_reg_index(SPU_APF2_VOLUME)] = 0u;
    inst->spu.reg[audio_spu_reg_index(SPU_LAPF1)] = 2u;
    inst->spu.reg[audio_spu_reg_index(SPU_RAPF1)] = 2u;
    inst->spu.reg[audio_spu_reg_index(SPU_LAPF2)] = 2u;
    inst->spu.reg[audio_spu_reg_index(SPU_RAPF2)] = 2u;
    inst->spu.reg[audio_spu_reg_index(SPU_VLIN)] = 0x7FFFu;
    inst->spu.reg[audio_spu_reg_index(SPU_VRIN)] = 0x7FFFu;
    inst->spu.reg[audio_spu_reg_index(SPU_MAIN_VOL_L)] = 0x3FFFu;
    inst->spu.reg[audio_spu_reg_index(SPU_MAIN_VOL_R)] = 0x3FFFu;
    inst->spu.reg[audio_spu_reg_index(SPU_AVOLL)] = 0x7FFFu;
    inst->spu.reg[audio_spu_reg_index(SPU_AVOLR)] = 0x7FFFu;
    inst->spu.reg[audio_spu_reg_index(SPU_RAM_CTRL)] = 4u;
    inst->spu.noise_lfsr = 0x2000u;
    inst->spu.noise_phase = 0u;
    inst->spu.voice[0].envelope = 0x4000;
    inst->spu.voice[0].reg[0] = 0x3FFFu;
    inst->spu.voice[0].reg[1] = 0x3FFFu;
}

static void xa_cdda_voice_reverb_mix_at_native_tick(void)
{
    schway *direct = audio_make();
    schway *wet = audio_make();
    xa_setup_voice_reverb(direct, 0u);
    xa_setup_voice_reverb(wet, 1u);
    schway *instances[2] = {direct, wet};
    for (uint32_t i = 0u; i < 2u; i++) {
        schway__cdrom_audio_push(instances[i], 1200, -2400);
        schway__cdrom_xa_audio_push(instances[i], 2400, 4800);
        schway__spu_event(instances[i], instances[i]->spu.next_tick);
    }

    CHECK_EQ_U64(direct->cdrom.audio_count, 0u);
    CHECK_EQ_U64(direct->cdrom.xa_audio_count, 0u);
    CHECK_EQ_U64(wet->cdrom.audio_count, 0u);
    CHECK_EQ_U64(wet->cdrom.xa_audio_count, 0u);
    CHECK(direct->spu.voice[0].output != 0 && wet->spu.voice[0].output != 0);
    CHECK_EQ_U64(schway__spu_ram_peek(wet, 0u), 3600u);
    CHECK_EQ_U64(schway__spu_ram_peek(wet, 0x400u), 2400u);
    CHECK(memcmp(direct->spu.reverb_fir, wet->spu.reverb_fir,
                 sizeof direct->spu.reverb_fir) != 0);

    for (uint32_t i = 0u; i < 2u; i++) {
        for (uint32_t tick = 1u; tick < 96u; tick++) {
            schway__spu_event(instances[i], instances[i]->spu.next_tick);
        }
    }

    uint32_t differs = 0u;
    for (uint32_t pair = 0u; pair < 96u; pair++) {
        uint32_t direct_slot = (direct->audio_head + pair) % SCHWAY__AUDIO_RING_PAIRS;
        uint32_t wet_slot = (wet->audio_head + pair) % SCHWAY__AUDIO_RING_PAIRS;
        differs |= direct->audio[direct_slot][0] != wet->audio[wet_slot][0] ||
                   direct->audio[direct_slot][1] != wet->audio[wet_slot][1];
    }
    CHECK_EQ_U64(differs, 1u);
    schway_destroy(wet);
    schway_destroy(direct);
}

int main(int argc, char **argv)
{
    if (argc != 2 || (strcmp(argv[1], "cdda") != 0 && strcmp(argv[1], "xa") != 0 &&
                      strcmp(argv[1], "combined") != 0)) {
        return 2;
    }
    if (strcmp(argv[1], "cdda") == 0) {
        cdda_queue_drains_at_native_tick();
        cdda_atv_applies_atomically_and_clips();
        cdda_two_speed_probe();
    } else if (strcmp(argv[1], "xa") == 0) {
        xa_sector_reaches_native_audio();
        xa_interleave_and_boundary_capacity();
        xa_filter_and_mute_controls();
        xa_file_channel_mismatch_retries_then_skips();
        xa_reset_lid_and_instance_controls();
    } else {
        xa_and_cdda_mix_at_native_tick();
        xa_cdda_voice_reverb_mix_at_native_tick();
    }
    return check_failures == 0 ? 0 : 1;
}
