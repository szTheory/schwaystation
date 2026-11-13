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

/* Loaded content counts a public frame when the GPU line counter wraps. */
void schway__frame_gpu_wrap(schway *inst)
{
    if (inst->exe_loaded == 0u) {
        return;
    }
    uint64_t num = schway__instance_frame_num(inst->region);
    uint64_t den = schway__instance_frame_den(inst->region);
    inst->frame++;
    inst->tick_acc = (inst->tick_acc + num) % den;
    inst->frame_end += (inst->tick_acc + num) / den;
}

/* Keep the saved-state horizon valid when GP1 restarts a guest's beam.
   Each public call still keeps a separate, immutable deadline. */
void schway__frame_gpu_reset(schway *inst, uint64_t tick)
{
    if (inst->exe_loaded != 0u) {
        inst->frame_end = tick + schway__instance_frame_max(inst->region);
    }
}

/* Moves the instance forward by at most limit cycles. The scheduled SPU queues
   one pair per 768-cycle boundary crossed. No-content calls stop at the synthetic
   frame_end and redraw the test card. Loaded content stops at the GPU field
   wrap, with an independent maximum-field call bound so video resets cannot
   hold a host call forever. With neither a BIOS nor PS-EXE loaded no guest
   instruction runs, so a stop at any cycle leaves the state a later stop would
   pass through and N cycles then M equal N + M. With a BIOS or PS-EXE loaded,
   the CPU stops at the first instruction boundary at or after the limit or the field wrap,
   so the advance can pass either by the rest of one instruction, and N cycles
   then M equal N + M only where N is a boundary of the one-shot run; a frame
   wrap passed inside an instruction completes at that instruction's end.
   Refuses, with nothing changed, an advance that would leave the
   save format's inclusive bounds (ARCHITECTURE section 5), with the longest
   instruction's cycles as margin when the CPU runs. Returns OK, with *done set
   when the frame completed and *stop the CPU's stop reason. */
static schway_status advance(schway *inst, uint64_t limit, uint32_t flags, int *done,
                             uint32_t *stop)
{
    uint64_t room = inst->frame_end - inst->ticks;
    uint64_t max_field = schway__instance_frame_max(inst->region);
    uint64_t cycles = limit;
    if (inst->exe_loaded != 0u) {
        cycles = cycles < max_field ? cycles : max_field;
    } else {
        cycles = cycles < room ? cycles : room;
    }
    uint64_t starting_frame = inst->frame;
    *stop = SCHWAY_STOP_BUDGET;
    if (inst->exe_loaded != 0u) {
        if (inst->ticks + cycles + SCHWAY__INSN_CYCLES_MAX > SCHWAY__STATE_TICKS_MAX ||
            inst->frame >= SCHWAY__STATE_FRAME_MAX) {
            return SCHWAY_STATUS_ERR_STATE_FIELD;
        }
        schway__pad_frame_start(inst);
        inst->profile_locked = 1u;
        *stop = schway__cpu_run(inst, cycles, flags);
        *done = inst->frame != starting_frame;
    } else {
        *done = cycles == room;
        if (inst->ticks + cycles > SCHWAY__STATE_TICKS_MAX ||
            (*done && inst->frame >= SCHWAY__STATE_FRAME_MAX)) {
            return SCHWAY_STATUS_ERR_STATE_FIELD;
        }
        schway__pad_frame_start(inst);
        /* No program runs and no device is stepped; only an event already due
           runs, at its own deadline, and with none armed the loop is one
           compare. */
        uint64_t target = inst->ticks + cycles;
        while (inst->ticks < target) {
            if (inst->sched.next <= inst->ticks) {
                schway__sched_run(inst);
            }
            uint64_t step = target - inst->ticks;
            if (inst->sched.next > inst->ticks && inst->sched.next - inst->ticks < step) {
                step = inst->sched.next - inst->ticks;
            }
            inst->ticks += step;
        }
        schway__sched_sync(inst);
    }
    /* A deadline that falls on or before the tick the advance ended on runs
       now, not at the start of the next advance, so a state saved here never
       holds an event behind its own tick. */
    if (inst->sched.next <= inst->ticks) {
        schway__sched_run(inst);
    }
    schway__pad_update_watchdog(inst, inst->ticks);
    if (inst->exe_loaded != 0u) {
        *done = inst->frame != starting_frame;
    }
    inst->profile_locked = 1u;
    if (*done && inst->exe_loaded == 0u) {
        uint64_t num = schway__instance_frame_num(inst->region);
        uint64_t den = schway__instance_frame_den(inst->region);
        inst->frame++;
        inst->tick_acc = (inst->tick_acc + num) % den;
        inst->frame_end += (inst->tick_acc + num) / den;
        schway__testcard_draw(&inst->vram[0][0], (uint32_t)inst->frame);
    }
    if (*done) {
        schway__pad_frame_end(inst);
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
    r.lag = inst->pad.frame_polls == 0u ? 1u : 0u;
    for (uint32_t port = 0u; port < 2u; port++) {
        r.rumble[port] = ((uint32_t)inst->pad.rumble_large[port] << 8) |
                         inst->pad.rumble_small[port];
    }
    schway__instance_write_out(out, &r, (uint32_t)sizeof r);
}

schway_status schway_set_input(schway *inst, const schway_input *input)
{
    if (inst == NULL || input == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    schway_status st = schway__instance_check_size(input, SCHWAY__INPUT_SIZE_FIRST,
                                                    (uint32_t)sizeof *input);
    if (st != SCHWAY_STATUS_OK) {
        return st;
    }
    if (input->reserved0 != 0u) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    for (uint32_t port = 0u; port < 2u; port++) {
        if (input->device[port] > SCHWAY_INPUT_DEVICE_DUALSHOCK ||
            input->analog_button[port] > 1u) {
            return SCHWAY_STATUS_ERR_ARGUMENT;
        }
    }
    memcpy(&inst->pad.pending_input, input, sizeof *input);
    inst->pad.pending_valid = 1u;
    return SCHWAY_STATUS_OK;
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
    if ((budget->flags & ~(SCHWAY_RUN_STOP_ON_FAULT | SCHWAY_RUN_TRACE)) != 0u) {
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

/* Scan-out of the display area uses native field height and packed rows
   (PSX-GPU-AND-GTE sections four point five and eight point one). */
schway_status schway_get_video(const schway *inst, schway_video *out)
{
    if (inst == NULL || out == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    schway_status st = schway__instance_check_out_size(out, SCHWAY__VIDEO_SIZE_FIRST);
    if (st != SCHWAY_STATUS_OK) {
        return st;
    }
    struct schway__display display = inst->display;
    if (inst->exe_loaded != 0u) {
        schway__gpu_display(inst, &display);
    }
    const struct schway__display *d = &display;
    schway_video v;
    memset(&v, 0, sizeof v);
    v.size = out->size;
    v.frame = inst->frame;
    v.pixels = inst->video_scanout;
    v.bpp = d->rgb24 != 0u ? 24u : 16u;
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

    if (d->dot_div != 0u && d->x2 >= d->x1 && d->y2 >= d->y1) {
        uint32_t width = (((d->x2 - d->x1) / d->dot_div) + 2u) & ~3u;
        uint32_t height = d->y2 - d->y1;
        if (width != 0u && width <= SCHWAY__VRAM_W && height != 0u &&
            height <= SCHWAY__VRAM_H) {
            uint32_t bytes_per_pixel = d->rgb24 != 0u ? 3u : 2u;
            v.width = width;
            v.height = height;
            v.pitch_bytes = width * bytes_per_pixel;

            if (d->enabled == 0u) {
                memset(inst->video_scanout, 0, (size_t)v.pitch_bytes * v.height);
            } else if (d->rgb24 == 0u) {
                for (uint32_t y = 0u; y < height; y++) {
                    uint32_t source_y = (d->start_y + y) & (SCHWAY__VRAM_H - 1u);
                    uint16_t *dst = (uint16_t *)(void *)(inst->video_scanout +
                                                         (size_t)y * v.pitch_bytes);
                    for (uint32_t x = 0u; x < width; x++) {
                        uint32_t source_x = (d->start_x + x) & (SCHWAY__VRAM_W - 1u);
                        dst[x] = inst->vram[source_y][source_x];
                    }
                }
            } else {
                for (uint32_t y = 0u; y < height; y++) {
                    uint32_t source_y = (d->start_y + y) & (SCHWAY__VRAM_H - 1u);
                    uint8_t *dst = inst->video_scanout + (size_t)y * v.pitch_bytes;
                    for (uint32_t x = 0u; x < width; x++) {
                        for (uint32_t channel = 0u; channel < 3u; channel++) {
                            uint32_t byte_x = (d->start_x * 2u + x * 3u + channel) %
                                              (SCHWAY__VRAM_W * 2u);
                            uint16_t word = inst->vram[source_y][byte_x / 2u];
                            dst[3u * x + channel] =
                                (uint8_t)(word >> ((byte_x & 1u) * 8u));
                        }
                    }
                }
            }
        }
    }
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
