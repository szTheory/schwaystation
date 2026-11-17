/* Bounded replay of hostile command and parameter streams through CD-ROM MMIO. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus.h"
#include "cdrom.h"
#include "internal.h"
#include "schwaystation.h"

#define CDROM_BASE 0x1F801800u
#define INPUT_MAX (16u * 1024u)
#define RECORD_SIZE 4u
#define RUN_QUANTUM 128u

struct media {
    const uint8_t *bytes;
    size_t size;
    uint32_t reads;
};

static void broken(const char *what)
{
    fprintf(stderr, "fuzz.regress.cdrom: %s\n", what);
    abort();
}

static void *alloc_bytes(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void free_bytes(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static schway *make(void)
{
    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = alloc_bytes;
    config.allocator.free = free_bytes;
    schway *inst = NULL;
    if (schway_create(&config, &inst) != SCHWAY_STATUS_OK || inst == NULL) {
        broken("instance creation failed");
    }
    return inst;
}

static uint32_t read_sector(void *user, uint32_t lba, uint8_t sector[2352],
                            uint8_t subcode[96])
{
    struct media *media = (struct media *)user;
    media->reads++;
    for (uint32_t i = 0u; i < SCHWAY__CDROM_SECTOR_BYTES; i++) {
        size_t at = media->size == 0u ? 0u : (size_t)(((uint64_t)lba + i) % media->size);
        sector[i] = media->size == 0u ? 0u : media->bytes[at];
    }
    memset(subcode, 0, 96u);
    return 1u;
}

static uint32_t read_toc(void *user, schway_toc *out)
{
    (void)user;
    memset(out, 0, sizeof *out);
    out->size = (uint32_t)sizeof *out;
    out->track_count = 1u;
    out->tracks[0].size = (uint32_t)sizeof out->tracks[0];
    out->tracks[0].type = SCHWAY_DISC_TRACK_MODE2;
    out->tracks[0].index0 = 150u;
    out->tracks[0].index1 = 150u;
    out->tracks[0].leadout = 1000000u;
    return 1u;
}

static void attach_media(schway *inst, struct media *media)
{
    schway_disc disc;
    memset(&disc, 0, sizeof disc);
    disc.size = (uint32_t)sizeof disc;
    disc.user = media;
    disc.read_sector = read_sector;
    disc.toc = read_toc;
    if (schway_open_disc(inst, &disc) != SCHWAY_STATUS_OK) {
        broken("synthetic media was refused");
    }
}

static void write8(schway *inst, uint32_t offset, uint8_t value)
{
    uint32_t cycles = 0u;
    schway__sched_sync(inst);
    if (schway__bus_write(inst, CDROM_BASE + offset, 1u, value, &cycles) != 0u) {
        broken("mapped CD-ROM write was refused");
    }
}

static uint8_t read8(schway *inst, uint32_t offset)
{
    uint32_t cycles = 0u;
    uint32_t value = 0u;
    schway__sched_sync(inst);
    if (schway__bus_read(inst, CDROM_BASE + offset, 1u, &value, &cycles) != 0u) {
        broken("mapped CD-ROM read was refused");
    }
    return (uint8_t)value;
}

static uint64_t run_bounded(schway *inst, uint64_t cycles)
{
    uint64_t before = inst->ticks;
    schway_budget budget;
    schway_run_result result;
    memset(&budget, 0, sizeof budget);
    memset(&result, 0, sizeof result);
    budget.size = (uint32_t)sizeof budget;
    budget.max_cycles = cycles;
    budget.flags = SCHWAY_RUN_STOP_ON_FAULT;
    result.size = (uint32_t)sizeof result;
    if (schway_run(inst, &budget, &result) != SCHWAY_STATUS_OK ||
        (result.stop != SCHWAY_STOP_BUDGET && result.stop != SCHWAY_STOP_FRAME &&
         result.stop != SCHWAY_STOP_FAULT) ||
        result.ticks < before || result.ticks - before > cycles) {
        broken("public run exceeded the requested CD-ROM cycle budget");
    }
    return result.ticks - before;
}

static uint32_t execute_record(schway *inst, const uint8_t *record)
{
    switch (record[0] % 7u) {
    case 0u: {
        write8(inst, 0u, 0u);
        uint32_t count = 1u + record[1] % 20u;
        for (uint32_t i = 0u; i < count; i++) {
            write8(inst, 2u, (i & 1u) == 0u ? record[2] : record[3]);
        }
        return RUN_QUANTUM;
    }
    case 1u:
        write8(inst, 0u, 0u);
        write8(inst, 2u, record[2]);
        write8(inst, 2u, record[3]);
        write8(inst, 1u, record[1]);
        return 50000u;
    case 2u:
        write8(inst, 0u, 0u);
        write8(inst, 3u, record[1]);
        return RUN_QUANTUM;
    case 3u:
        if (schway_set_lid(inst, (uint32_t)(record[1] & 1u)) != SCHWAY_STATUS_OK) {
            broken("lid transition was refused");
        }
        return RUN_QUANTUM;
    case 4u:
        write8(inst, 0u, (uint8_t)(record[1] & 3u));
        (void)read8(inst, (uint32_t)(record[2] & 3u));
        (void)read8(inst, (uint32_t)(record[3] & 3u));
        return RUN_QUANTUM;
    case 5u:
        write8(inst, 0u, 1u);
        write8(inst, 2u, record[1]);
        write8(inst, 3u, record[2]);
        return RUN_QUANTUM;
    default:
        if (schway_reset(inst, SCHWAY_RESET_POWER) != SCHWAY_STATUS_OK) {
            broken("power reset was refused");
        }
        return RUN_QUANTUM;
    }
}

static void run_for(schway *inst, uint64_t requested, uint64_t total_limit,
                    uint64_t *elapsed)
{
    while (requested != 0u && *elapsed < total_limit) {
        uint64_t allowed = total_limit - *elapsed;
        if (allowed > requested) allowed = requested;
        uint64_t used = run_bounded(inst, allowed);
        *elapsed += used;
        if (used == 0u) return;
        requested -= used;
    }
}

static void check_bounds(const schway *inst)
{
    const struct schway__cdrom *cdrom = &inst->cdrom;
    if (cdrom->parameter_count > SCHWAY__CDROM_FIFO_CAP ||
        cdrom->result_count > SCHWAY__CDROM_FIFO_CAP ||
        cdrom->interrupt_count > SCHWAY__CDROM_IRQ_CAP ||
        cdrom->sector_count > SCHWAY__CDROM_SECTOR_CAP ||
        cdrom->audio_count > SCHWAY__CDROM_AUDIO_LIMIT ||
        cdrom->xa_audio_count > SCHWAY__CDROM_XA_AUDIO_CAP) {
        broken("CD-ROM FIFO or ring count exceeded its fixed capacity");
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size > INPUT_MAX) return 0;
    struct media media = {data, size, 0u};
    schway *inst = make();
    attach_media(inst, &media);

    uint64_t frame_cycles = schway__instance_frame_max(inst->region);
    uint64_t total_limit = frame_cycles * 2u;
    uint64_t elapsed = 0u;
    size_t bounded_size = size - size % RECORD_SIZE;
    for (size_t at = 0u; at < bounded_size; at += RECORD_SIZE) {
        uint32_t delay = execute_record(inst, data + at);
        run_for(inst, delay, total_limit, &elapsed);
        check_bounds(inst);
    }
    run_for(inst, total_limit - elapsed, total_limit, &elapsed);
    if (elapsed > total_limit) broken("total emulated time exceeded two frames");
    check_bounds(inst);
    schway_destroy(inst);
    return 0;
}

#define SCHWAY_REPLAY_NAME "fuzz.regress.cdrom"
#include "replay_main.h"
