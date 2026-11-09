/* schway_run_frame, schway_run, schway_get_video and schway_get_audio: every
   check first, then the work. A run advances the exact cycle accumulator,
   queues the audio pairs and, at the frame boundary, redraws the test card
   into VRAM. */
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

/* Moves the instance forward by at most limit cycles and never past the end
   of the current frame, queueing one silent pair per 768-cycle boundary
   crossed. Reaching frame_end completes the frame: the frame count rises, the
   profile locks, the card is redrawn and the next frame's end is fixed from
   the accumulator's remainder, so the cumulative ticks stay exact
   (ARCHITECTURE sections 1 and 7). With no PS-EXE loaded no instruction runs,
   so a stop at any cycle leaves the state a later stop would pass through and
   N cycles then M equal N + M. With one loaded the CPU runs instead and stops
   at the first instruction boundary at or after the limit or the frame's end,
   so the advance can pass either by the rest of one instruction, and N cycles
   then M equal N + M only where N is a boundary of the one-shot run; a frame
   end passed inside an instruction completes at that instruction's end, and
   the vertical sync that ends the frame raises I_STAT bit 0 first (psx-spx,
   Interrupts). Refuses, with nothing changed, an advance that would leave the
   save format's inclusive bounds (ARCHITECTURE section 5), with the longest
   instruction's cycles as margin when the CPU runs. Returns OK, with *done set
   when the frame completed and *stop the CPU's stop reason. */
static schway_status advance(schway *inst, uint64_t limit, uint32_t flags, int *done,
                             uint32_t *stop)
{
    uint64_t room = inst->frame_end - inst->ticks;
    uint64_t cycles = limit < room ? limit : room;
    uint64_t before = inst->ticks;
    *stop = SCHWAY_STOP_BUDGET;
    if (inst->exe_loaded != 0u) {
        if (inst->ticks + cycles + SCHWAY__INSN_CYCLES_MAX > SCHWAY__STATE_TICKS_MAX ||
            inst->frame >= SCHWAY__STATE_FRAME_MAX) {
            return SCHWAY_STATUS_ERR_STATE_FIELD;
        }
        inst->profile_locked = 1u;
        *stop = schway__cpu_run(inst, cycles, flags);
        *done = inst->ticks >= inst->frame_end;
    } else {
        *done = cycles == room;
        if (inst->ticks + cycles > SCHWAY__STATE_TICKS_MAX ||
            (*done && inst->frame >= SCHWAY__STATE_FRAME_MAX)) {
            return SCHWAY_STATUS_ERR_STATE_FIELD;
        }
        inst->ticks += cycles;
    }
    /* One pair per 768-cycle boundary crossed. */
    audio_push_silence(inst, (uint32_t)(inst->ticks / SCHWAY__CYCLES_PER_PAIR -
                                        before / SCHWAY__CYCLES_PER_PAIR));
    inst->profile_locked = 1u;
    if (*done) {
        uint64_t num = schway__instance_frame_num(inst->region);
        uint64_t den = schway__instance_frame_den(inst->region);
        if (inst->exe_loaded != 0u) {
            schway__irq_raise(inst, 0u);
        }
        inst->frame++;
        inst->tick_acc = (inst->tick_acc + num) % den;
        inst->frame_end += (inst->tick_acc + num) / den;
        schway__testcard_draw(&inst->vram[0][0], (uint32_t)inst->frame);
    }
    return SCHWAY_STATUS_OK;
}

static void write_result(const schway *inst, schway_run_result *out, uint32_t stop)
{
    schway_run_result r;
    memset(&r, 0, sizeof r);
    r.size = out->size;
    r.stop = stop;
    r.frame = inst->frame;
    r.ticks = inst->ticks;
    r.exit_code = stop == SCHWAY_STOP_HOOK ? inst->exit_code : 0u;
    r.fault_present = inst->fault.kind != SCHWAY__FAULT_NONE;
    schway__instance_write_out(out, &r, (uint32_t)sizeof r);
}

/* The result's stop reason: a hook or a fault the CPU stopped on, else the
   frame's end, else the budget. */
static uint32_t stop_reason(uint32_t cpu_stop, int done)
{
    if (cpu_stop == SCHWAY_STOP_HOOK || cpu_stop == SCHWAY_STOP_FAULT) {
        return cpu_stop;
    }
    return done ? SCHWAY_STOP_FRAME : SCHWAY_STOP_BUDGET;
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
    int done;
    uint32_t cpu_stop;
    schway_status st = advance(inst, UINT64_MAX, 0u, &done, &cpu_stop);
    if (st != SCHWAY_STATUS_OK) {
        return st;
    }
    if (out != NULL) {
        write_result(inst, out, stop_reason(cpu_stop, done));
    }
    return SCHWAY_STATUS_OK;
}

/* A budget of 0 cycles, or a latched fault under STOP_ON_FAULT, stops before
   any cycle runs and changes nothing. */
schway_status schway_run(schway *inst, const schway_budget *budget, schway_run_result *out)
{
    if (inst == NULL || budget == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    schway_status st =
        schway__instance_check_size(budget, SCHWAY__BUDGET_SIZE_FIRST, (uint32_t)sizeof *budget);
    if (st != SCHWAY_STATUS_OK) {
        return st;
    }
    if ((budget->flags & ~SCHWAY_RUN_STOP_ON_FAULT) != 0u) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    if (out != NULL) {
        st = schway__instance_check_out_size(out, SCHWAY__RUN_RESULT_SIZE_FIRST);
        if (st != SCHWAY_STATUS_OK) {
            return st;
        }
    }
    uint32_t stop = SCHWAY_STOP_BUDGET;
    if ((budget->flags & SCHWAY_RUN_STOP_ON_FAULT) != 0u &&
        inst->fault.kind != SCHWAY__FAULT_NONE) {
        stop = SCHWAY_STOP_FAULT;
    } else if (budget->max_cycles != 0u) {
        int done;
        uint32_t cpu_stop;
        st = advance(inst, budget->max_cycles, budget->flags, &done, &cpu_stop);
        if (st != SCHWAY_STATUS_OK) {
            return st;
        }
        stop = stop_reason(cpu_stop, done);
    }
    if (out != NULL) {
        write_result(inst, out, stop);
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
