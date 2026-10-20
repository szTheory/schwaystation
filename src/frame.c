/* schway_run_frame, schway_get_video and schway_get_audio: every check
   first, then the work. A frame advances the exact cycle accumulator, queues
   the frame's audio pairs and redraws the test card into VRAM. */
#include <string.h>

#include "internal.h"

void schway__frame_audio_clear(schway *inst)
{
    inst->audio_head = 0u;
    inst->audio_count = 0u;
    inst->audio_dropped = 0u;
    memset(inst->audio, 0, sizeof inst->audio);
}

/* Queues n silent pairs; pairs that do not fit in the ring are dropped and
   counted. */
static void audio_push_silence(schway *inst, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        if (inst->audio_count == SCHWAY__AUDIO_RING_PAIRS) {
            inst->audio_dropped++;
            continue;
        }
        uint32_t slot = (inst->audio_head + inst->audio_count) % SCHWAY__AUDIO_RING_PAIRS;
        inst->audio[slot][0] = 0;
        inst->audio[slot][1] = 0;
        inst->audio_count++;
    }
}

schway_status schway_run_frame(schway *inst, schway_run_result *out)
{
    if (inst == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    if (out != NULL) {
        schway_status st = schway__instance_check_out_size(out, SCHWAY__RUN_RESULT_SIZE_FIRST);
        if (st != SCHWAY_STATUS_OK) {
            return st;
        }
    }
    /* The frame is num / den cycles; the remainder carries to the next
       frame so cumulative ticks stay exact (ARCHITECTURE section 1). */
    uint64_t den = schway__instance_frame_den(inst->region);
    uint64_t before = inst->ticks;
    inst->tick_acc += schway__instance_frame_num(inst->region);
    inst->ticks += inst->tick_acc / den;
    inst->tick_acc %= den;
    /* One pair per 768-cycle boundary crossed during the frame. */
    audio_push_silence(inst, (uint32_t)(inst->ticks / SCHWAY__CYCLES_PER_PAIR -
                                        before / SCHWAY__CYCLES_PER_PAIR));
    inst->profile_locked = 1u;
    inst->frame++;
    schway__testcard_draw(&inst->vram[0][0], (uint32_t)inst->frame);
    if (out != NULL) {
        schway_run_result r;
        memset(&r, 0, sizeof r);
        r.size = out->size;
        r.stop = SCHWAY_STOP_FRAME;
        r.frame = inst->frame;
        r.ticks = inst->ticks;
        schway__instance_write_out(out, &r, (uint32_t)sizeof r);
    }
    return SCHWAY_STATUS_OK;
}

/* Scan-out of the display area: width (((X2 - X1) / divider) + 2) & ~3 and
   height Y2 - Y1, from VRAM at (start_x, start_y) (ARCHITECTURE section 4). */
schway_status schway_get_video(const schway *inst, schway_video *out)
{
    if (inst == NULL || out == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    schway_status st = schway__instance_check_out_size(out, SCHWAY__VIDEO_SIZE_FIRST);
    if (st != SCHWAY_STATUS_OK) {
        return st;
    }
    const struct schway__display *d = &inst->display;
    schway_video v;
    memset(&v, 0, sizeof v);
    v.size = out->size;
    v.frame = inst->frame;
    v.pixels = &inst->vram[d->start_y][d->start_x];
    v.width = (((d->x2 - d->x1) / d->dot_div) + 2u) & ~3u;
    v.height = d->y2 - d->y1;
    v.pitch_bytes = SCHWAY__VRAM_W * 2u;
    v.bpp = 16u;
    v.field = d->field;
    v.interlaced = d->interlaced;
    v.vres = d->vres;
    v.hres = d->hres;
    v.dot_div = d->dot_div;
    v.region = inst->region;
    v.x1 = d->x1;
    v.y1 = d->y1;
    v.start_x = d->start_x;
    v.start_y = d->start_y;
    v.enabled = d->enabled;
    v.rgb24 = d->rgb24;
    schway__instance_write_out(out, &v, (uint32_t)sizeof v);
    return SCHWAY_STATUS_OK;
}

schway_status schway_get_audio(schway *inst, schway_audio *io)
{
    if (inst == NULL || io == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    schway_status st =
        schway__instance_check_size(io, SCHWAY__AUDIO_SIZE_FIRST, (uint32_t)sizeof *io);
    if (st != SCHWAY_STATUS_OK) {
        return st;
    }
    if (io->samples == NULL && io->capacity != 0u) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    uint32_t n = io->capacity < inst->audio_count ? io->capacity : inst->audio_count;
    for (uint32_t i = 0; i < n; i++) {
        uint32_t slot = (inst->audio_head + i) % SCHWAY__AUDIO_RING_PAIRS;
        io->samples[2u * i] = inst->audio[slot][0];
        io->samples[2u * i + 1u] = inst->audio[slot][1];
    }
    inst->audio_head = (inst->audio_head + n) % SCHWAY__AUDIO_RING_PAIRS;
    inst->audio_count -= n;
    io->written = n;
    io->dropped = inst->audio_dropped;
    inst->audio_dropped = 0u;
    return SCHWAY_STATUS_OK;
}
