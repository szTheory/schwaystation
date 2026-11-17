#include <string.h> /* CD-ROM registers and command state: docs/hardware/cdrom.md. */

#include "internal.h"
#include "state.h"

#define SCHWAY__CDROM_ACK_CYCLES UINT64_C(50000)

#define SCHWAY__CDROM_IRQ2 2u

static uint64_t deadline_after(uint64_t now, uint64_t delta)
{
    if (now > SCHWAY__STATE_TICKS_MAX - delta) {
        return SCHWAY__STATE_TICKS_MAX;
    }
    return now + delta;
}

static uint8_t status_read(const struct schway__cdrom *cd)
{
    uint8_t value = (uint8_t)(cd->bank & 3u);
    if (cd->parameter_count == 0u) {
        value |= 1u << 3;
    }
    if (cd->parameter_count < SCHWAY__CDROM_FIFO_CAP) {
        value |= 1u << 4;
    }
    if (cd->result_count != 0u) {
        value |= 1u << 5;
    }
    if (cd->bfrd != 0u && cd->sector_count != 0u &&
        cd->sector_offset < cd->sector_size[cd->sector_head]) value |= 1u << 6;
    /* BUSYSTS clears when the sub-CPU accepts a command, independent of drive activity (SCM.02). */
    if (cd->command_pending == 1u) {
        value |= 1u << 7;
    }
    return value;
}

static uint8_t current_interrupt(const struct schway__cdrom *cd)
{
    if (cd->interrupt_count == 0u) {
        return 0u;
    }
    return cd->interrupt[cd->interrupt_head];
}

static void update_irq(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t code = current_interrupt(cd);
    if (code >= 1u && code <= 5u && (cd->interrupt_mask & (1u << (code - 1u))) != 0u) {
        schway__irq_raise(inst, SCHWAY__CDROM_IRQ2);
    }
}

static void push_result(struct schway__cdrom *cd, uint8_t value)
{
    if (cd->result_count < SCHWAY__CDROM_FIFO_CAP) {
        uint32_t tail = (cd->result_head + cd->result_count) % SCHWAY__CDROM_FIFO_CAP;
        cd->result[tail] = value;
        cd->result_count++;
    }
}

void schway__cdrom_push_interrupt(schway *inst, uint8_t code)
{
    struct schway__cdrom *cd = &inst->cdrom;
    if (code >= 1u && code <= 5u && cd->interrupt_count < SCHWAY__CDROM_IRQ_CAP) {
        uint32_t tail = (cd->interrupt_head + cd->interrupt_count) % SCHWAY__CDROM_IRQ_CAP;
        cd->interrupt[tail] = code;
        cd->interrupt_count++;
        update_irq(inst);
    }
}

static void acknowledge_interrupt(schway *inst, uint8_t value)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t code = current_interrupt(cd);
    if (code >= 1u && code <= 5u && (value & (1u << (code - 1u))) != 0u) {
        cd->interrupt[cd->interrupt_head] = 0u;
        cd->interrupt_head = (cd->interrupt_head + 1u) % SCHWAY__CDROM_IRQ_CAP;
        cd->interrupt_count--;
        update_irq(inst);
        schway__cdrom_sector_ack(inst);
    }
}

static uint8_t hint_status(const struct schway__cdrom *cd)
{
    uint8_t value = current_interrupt(cd) & 7u;
    if (cd->result_count == 0u) {
        value |= 1u << 3;
    }
    if (cd->parameter_count < SCHWAY__CDROM_FIFO_CAP) {
        value |= 1u << 4;
    }
    return value | 0xE0u;
}

static uint8_t pop_result(struct schway__cdrom *cd)
{
    if (cd->result_count == 0u) {
        return 0u;
    }
    uint8_t value = cd->result[cd->result_head];
    cd->result[cd->result_head] = 0u;
    cd->result_head = (cd->result_head + 1u) % SCHWAY__CDROM_FIFO_CAP;
    cd->result_count--;
    return value;
}

static uint32_t bcd_valid(uint8_t value)
{
    return (value & 0x0Fu) <= 9u && (value >> 4) <= 9u;
}

static uint32_t bcd_value(uint8_t value)
{
    return ((uint32_t)(value >> 4) * 10u) + (value & 0x0Fu);
}

static uint8_t to_bcd(uint32_t value)
{
    value %= 100u;
    return (uint8_t)(((value / 10u) << 4) | (value % 10u));
}

static void set_motor(struct schway__cdrom *cd, uint32_t on)
{
    cd->motor_on = on;
    if (on != 0u) {
        cd->status |= 1u << 1;
    } else {
        cd->status &= ~(1u << 1);
    }
}

static void response(schway *inst, const uint8_t *bytes, uint32_t count, uint8_t interrupt)
{
    for (uint32_t i = 0u; i < count; i++) {
        push_result(&inst->cdrom, bytes[i]);
    }
    schway__cdrom_push_interrupt(inst, interrupt);
}

static void response_status(schway *inst, uint8_t interrupt)
{
    uint8_t status = (uint8_t)inst->cdrom.status;
    response(inst, &status, 1u, interrupt);
}

static void finish_command(struct schway__cdrom *cd)
{
    cd->command = 0u;
    cd->command_pending = 0u;
    cd->command_event = 0u;
    cd->command_deadline = 0u;
}

static void cancel_sector_event(struct schway__cdrom *cd)
{
    cd->sector_event = 0u;
    cd->sector_deadline = 0u;
}

static void schedule_second(struct schway__cdrom *cd, uint64_t now, uint64_t delay)
{
    cd->command_pending = 2u;
    cd->command_event = 1u;
    cd->command_deadline = deadline_after(now, delay);
}

static void command_error(schway *inst, uint8_t error)
{
    struct schway__cdrom *cd = &inst->cdrom;
    if (error != 0x80u || (cd->command != 0x10u && cd->command != 0x11u)) cd->status |= 1u;
    uint8_t bytes[2] = {(uint8_t)cd->status, error};
    response(inst, bytes, 2u, 5u);
    finish_command(cd);
}

static void command_invalid(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    cd->status |= 1u;
    const uint8_t bytes[2] = {0x11u, 0x40u};
    response(inst, bytes, 2u, 5u);
    finish_command(cd);
}

static void consume_parameters(struct schway__cdrom *cd, uint32_t count)
{
    for (uint32_t i = 0u; i < count; i++) {
        cd->parameter[cd->parameter_head] = 0u;
        cd->parameter_head = (cd->parameter_head + 1u) % SCHWAY__CDROM_FIFO_CAP;
    }
    cd->parameter_count -= count;
    if (cd->parameter_count == 0u) {
        cd->parameter_head = 0u;
    }
}

static uint32_t parameters(struct schway__cdrom *cd, uint32_t minimum, uint32_t maximum,
                           uint8_t values[SCHWAY__CDROM_FIFO_CAP])
{
    if (cd->parameter_count < minimum || cd->parameter_count > maximum) {
        return 0u;
    }
    for (uint32_t i = 0u; i < cd->parameter_count; i++) {
        values[i] = cd->parameter[(cd->parameter_head + i) % SCHWAY__CDROM_FIFO_CAP];
    }
    return 1u;
}

static uint32_t disc_type(const schway *inst)
{
    if (inst->disc_host.attached == 0u || inst->disc_host.toc.track_count == 0u) {
        return 0u;
    }
    return inst->disc_host.toc.tracks[0].type;
}

static uint32_t disc_region_code(const schway *inst)
{
    if ((inst->profile_flags & SCHWAY_PROFILE_CD_LICENSED) != 0u) {
        return inst->region;
    }
    if (inst->disc_host.attached == 0u) {
        return 0u;
    }

    uint8_t sector[SCHWAY__CDROM_SECTOR_BYTES];
    uint8_t subcode[96];
    if (inst->disc_host.callbacks.read_sector(inst->disc_host.callbacks.user, 4u, sector,
                                              subcode) == 0u) {
        return 0u;
    }
    const uint8_t *license = &sector[0x38u];
    if (memcmp(license, "Sony Computer Entertainment Inc.", 32u) == 0) {
        return SCHWAY_REGION_NTSC_J;
    }
    if (memcmp(license, "Sony Computer Entertainment Amer", 32u) == 0) {
        return SCHWAY_REGION_NTSC_U;
    }
    if (memcmp(license, "Sony Computer Entertainment Euro", 32u) == 0) {
        return SCHWAY_REGION_PAL;
    }
    return 0u;
}

static uint8_t region_letter(uint32_t region)
{
    if (region == SCHWAY_REGION_NTSC_J) {
        return (uint8_t)'I';
    }
    if (region == SCHWAY_REGION_NTSC_U) {
        return (uint8_t)'A';
    }
    return (uint8_t)'E';
}

static uint32_t isqrt_u64(uint64_t value)
{
    uint64_t bit = UINT64_C(1) << 62;
    uint64_t result = 0u;
    while (bit > value) {
        bit >>= 2;
    }
    while (bit != 0u) {
        if (value >= result + bit) {
            value -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }
    return (uint32_t)result;
}

static uint32_t seek_ticks(uint32_t from, uint32_t to)
{
    uint64_t a_lba = (uint64_t)from + 150u;
    uint64_t b_lba = (uint64_t)to + 150u;
    if (a_lba > UINT32_MAX) {
        a_lba = UINT32_MAX;
    }
    if (b_lba > UINT32_MAX) {
        b_lba = UINT32_MAX;
    }
    uint32_t a = isqrt_u64(a_lba << 32);
    uint32_t b = isqrt_u64(b_lba << 32);
    uint32_t radial = a > b ? a - b : b - a;
    uint32_t distance = from > to ? from - to : to - from;
    uint32_t radial_root = isqrt_u64((uint64_t)radial << 16);
    uint64_t ticks_q16 = (UINT64_C(3) << 16) + radial / 5u +
                         (UINT64_C(5) * radial_root) / 2u +
                         (((uint64_t)distance << 16) / 60000u);
    uint32_t ticks = (uint32_t)((ticks_q16 / 2u) >> 16);
    if (ticks < 1u) {
        ticks = 1u;
    }
    if (ticks > 150u) {
        ticks = 150u;
    }
    return ticks;
}

static uint64_t seek_delay_cycles(const struct schway__cdrom *cd, uint32_t target)
{
    uint32_t ticks = seek_ticks(cd->current_lba, target);
    uint64_t cycles = (uint64_t)ticks * UINT64_C(451584);
    cycles = (cycles + cd->drive_speed - 1u) / cd->drive_speed;
    cycles += (UINT64_C(3) * UINT64_C(451584) + cd->drive_speed - 1u) / cd->drive_speed;
    if (cd->drive_speed_changed != 0u) {
        cycles += (UINT64_C(80) * UINT64_C(451584) + cd->drive_speed - 1u) / cd->drive_speed;
    }
    return cycles;
}

static void update_status_activity(struct schway__cdrom *cd)
{
    cd->status &= ~((1u << 7) | (1u << 6) | (1u << 5));
    if (cd->play_active != 0u) {
        cd->status |= 1u << 7;
    } else if (cd->seek_active != 0u) {
        cd->status |= 1u << 6;
    } else if (cd->read_active != 0u) {
        cd->status |= 1u << 5;
    }
}

static void observe_lid(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    if (inst->disc_host.lid_open == 0u || (cd->status & (1u << 4)) != 0u) {
        return;
    }
    cd->status |= (1u << 4) | 1u;
    cd->read_active = 0u;
    cd->play_active = 0u;
    cd->seek_active = 0u;
    cd->command = 0u;
    cd->command_pending = 0u;
    cd->command_event = 0u;
    cd->command_deadline = 0u;
    cancel_sector_event(cd);
    cd->seek_event = 0u;
    cd->seek_deadline = 0u;
    cd->audio_head = 0u;
    cd->audio_count = 0u;
    cd->xa_audio_head = 0u;
    cd->xa_audio_count = 0u;
    update_status_activity(cd);
    uint8_t bytes[2] = {(uint8_t)cd->status, 0x08u};
    response(inst, bytes, 2u, 5u);
}

void schway__cdrom_lid_changed(schway *inst)
{
    observe_lid(inst);
}

static void begin_seek(schway *inst, uint32_t position, uint64_t now, uint32_t play)
{
    struct schway__cdrom *cd = &inst->cdrom;
    cd->read_active = 0u;
    cd->play_active = play;
    cd->seek_active = 1u;
    cd->target_lba = position;
    if (play != 0u) {
        cd->status |= 1u << 7;
    }
    update_status_activity(cd);
    cd->seek_event = 1u;
    cd->seek_deadline = deadline_after(now, seek_delay_cycles(cd, position));
    cd->drive_speed_changed = 0u;
    cancel_sector_event(cd);
    cd->command_pending = 2u;
}

static void complete_get_id(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t bytes[SCHWAY__CDROM_FIFO_CAP] = {0u};
    uint32_t type = disc_type(inst);
    uint32_t region = 0u;
    if (inst->disc_host.lid_open == 0u && type != 0u && type != SCHWAY_DISC_TRACK_AUDIO) {
        region = disc_region_code(inst);
    }
    uint32_t match = region == inst->region;
    if (inst->disc_host.lid_open != 0u) {
        bytes[0] = 0x11u;
        bytes[1] = 0x80u;
        response(inst, bytes, 8u, 5u);
    } else if (type == 0u) {
        bytes[0] = 0x08u;
        bytes[1] = 0x40u;
        response(inst, bytes, 8u, 5u);
    } else if (type == SCHWAY_DISC_TRACK_AUDIO) {
        bytes[0] = 0x0Au;
        bytes[1] = 0x90u;
        response(inst, bytes, 8u, 5u);
    } else if (region == 0u && (inst->profile_flags & SCHWAY_PROFILE_CD_LICENSED) == 0u) {
        bytes[0] = 0x0Au;
        bytes[1] = 0x80u;
        bytes[2] = type == SCHWAY_DISC_TRACK_MODE2 ? 0x20u : 0u;
        response(inst, bytes, 8u, 5u);
    } else if (match != 0u) {
        bytes[0] = 0x02u;
        bytes[2] = type == SCHWAY_DISC_TRACK_MODE2 ? 0x20u : 0u;
        bytes[4] = 'S';
        bytes[5] = 'C';
        bytes[6] = 'E';
        bytes[7] = region_letter(inst->region);
        response(inst, bytes, 8u, 2u);
    } else {
        bytes[0] = 0x0Au;
        bytes[1] = 0x80u;
        bytes[2] = type == SCHWAY_DISC_TRACK_MODE2 ? 0x20u : 0u;
        response(inst, bytes, 8u, 5u);
    }
    finish_command(cd);
}

static void complete_second_response(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    switch (cd->command) {
    case 0x07u:
    case 0x09u:
    case 0x0Au:
    case 0x16u:
        response_status(inst, 2u);
        finish_command(cd);
        break;
    case 0x15u:
        if ((cd->status & (1u << 2)) != 0u) {
            uint8_t error[2] = {(uint8_t)cd->status, 0x04u};
            response(inst, error, 2u, 5u);
        } else {
            response_status(inst, 2u);
        }
        finish_command(cd);
        break;
    case 0x08u:
        set_motor(cd, 0u);
        response_status(inst, 2u);
        finish_command(cd);
        break;
    case 0x1Au:
        complete_get_id(inst);
        break;
    default:
        response_status(inst, 2u);
        finish_command(cd);
        break;
    }
}

static uint32_t command_parameters(schway *inst, uint8_t minimum, uint8_t maximum,
                                   uint8_t values[SCHWAY__CDROM_FIFO_CAP])
{
    if (parameters(&inst->cdrom, minimum, maximum, values) == 0u) {
        command_error(inst, 0x20u);
        return 0u;
    }
    return 1u;
}

static uint32_t parameters_match(const struct schway__cdrom *cd, const char *expected,
                                 uint32_t count)
{
    if (cd->parameter_count != count) {
        return 0u;
    }
    for (uint32_t i = 0u; i < count; i++) {
        if (cd->parameter[(cd->parameter_head + i) % SCHWAY__CDROM_FIFO_CAP] !=
            (uint8_t)expected[i]) {
            return 0u;
        }
    }
    return 1u;
}

static void command_unlock(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    static const char licensed_by[] = "Licensed by";
    static const char sony[] = "Sony";
    static const char computer[] = "Computer";
    static const char entertainment[] = "Entertainment";
    static const char america[] = "of America";
    static const char europe[] = "(Europe)";
    uint32_t expected_command = UINT32_MAX;
    const char *expected = "";
    uint32_t count = 0u;

    if ((inst->profile_flags & SCHWAY_PROFILE_CD_UNLOCK) != 0u &&
        inst->region != SCHWAY_REGION_NTSC_J) {
        switch (cd->unlock_step) {
        case 0u: expected_command = 0x50u; break;
        case 1u: expected_command = 0x51u; expected = licensed_by; count = 11u; break;
        case 2u: expected_command = 0x52u; expected = sony; count = 4u; break;
        case 3u: expected_command = 0x53u; expected = computer; count = 8u; break;
        case 4u: expected_command = 0x54u; expected = entertainment; count = 13u; break;
        case 5u:
            expected_command = 0x55u;
            expected = inst->region == SCHWAY_REGION_NTSC_U ? america : europe;
            count = inst->region == SCHWAY_REGION_NTSC_U ? 10u : 8u;
            break;
        case 6u: expected_command = 0x56u; break;
        default: break;
        }
    }

    uint32_t valid = cd->command == expected_command &&
                     parameters_match(cd, expected, count) != 0u;
    if (valid != 0u) {
        consume_parameters(cd, count);
        cd->unlock_step++;
        if (cd->command == 0x56u) {
            cd->unlock_step = 0u;
            cd->unlock_enabled = 1u;
        }
    } else {
        cd->unlock_step = 0u;
    }
    command_invalid(inst);
}


static void begin_nop(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    if (inst->disc_host.lid_open == 0u) {
        cd->status &= ~(1u << 4);
    }
    response_status(inst, 3u);
    finish_command(cd);
}

static void begin_setloc(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t p[SCHWAY__CDROM_FIFO_CAP] = {0u};
    if (command_parameters(inst, 3u, 3u, p) == 0u) {
        return;
    }
    if (!bcd_valid(p[0]) || !bcd_valid(p[1]) || !bcd_valid(p[2]) ||
        bcd_value(p[1]) >= 60u || bcd_value(p[2]) >= 75u) {
        command_error(inst, 0x10u);
        return;
    }
    uint32_t target = (bcd_value(p[0]) * 60u + bcd_value(p[1])) * 75u + bcd_value(p[2]);
    cd->target_lba = target > 150u ? target - 150u : 0u;
    cd->status &= ~(1u << 3);
    consume_parameters(cd, 3u);
    response_status(inst, 3u);
    finish_command(cd);
}

static void begin_play(schway *inst, uint64_t now)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t p[SCHWAY__CDROM_FIFO_CAP] = {0u};
    if (command_parameters(inst, 0u, 1u, p) == 0u) {
        return;
    }
    uint32_t pcount = cd->parameter_count;
    if (pcount != 0u && !bcd_valid(p[0])) {
        command_error(inst, 0x10u);
        return;
    }
    if (disc_type(inst) == 0u || inst->disc_host.lid_open != 0u) {
        command_error(inst, 0x80u);
        return;
    }
    if (pcount != 0u && bcd_value(p[0]) > inst->disc_host.toc.track_count) {
        command_error(inst, 0x10u);
        return;
    }
    if (pcount != 0u && bcd_value(p[0]) != 0u) {
        uint32_t index1 = inst->disc_host.toc.tracks[bcd_value(p[0]) - 1u].index1;
        cd->target_lba = index1;
    }
    consume_parameters(cd, pcount);
    cd->paused = 0u;
    cd->read_active = 0u;
    cancel_sector_event(cd);
    cd->play_active = 1u;
    set_motor(cd, 1u);
    update_status_activity(cd);
    schway__cdrom_sector_schedule(inst, now);
    response_status(inst, 3u);
    finish_command(cd);
}

static void begin_scan(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t p[SCHWAY__CDROM_FIFO_CAP] = {0u};
    if (command_parameters(inst, 0u, 0u, p) == 0u) {
        return;
    }
    if (cd->play_active == 0u) {
        command_error(inst, 0x80u);
        return;
    }
    response_status(inst, 3u);
    finish_command(cd);
}

static void begin_read(schway *inst, uint64_t now)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t p[SCHWAY__CDROM_FIFO_CAP] = {0u};
    if (command_parameters(inst, 0u, 0u, p) == 0u) {
        return;
    }
    if (disc_type(inst) == 0u || inst->disc_host.lid_open != 0u) {
        command_error(inst, 0x80u);
        return;
    }
    uint32_t region = disc_region_code(inst);
    if ((inst->profile_flags & SCHWAY_PROFILE_CD_LICENSED) == 0u &&
        (cd->mode & 1u) == 0u && (region == 0u || region != inst->region) &&
        cd->unlock_enabled == 0u) {
        command_error(inst, 0x40u);
        return;
    }
    /* Seek headers support GetlocL; RDDATA must start with streamed data (SCM.02). */
    schway__cdrom_sector_discard_seek_headers(inst);
    cd->paused = 0u;
    cd->play_active = 0u;
    cancel_sector_event(cd);
    cd->read_active = 1u;
    cd->read_s = cd->command == 0x1Bu;
    cd->sector_retry = 0u;
    set_motor(cd, 1u);
    update_status_activity(cd);
    if (cd->target_lba != cd->current_lba) {
        begin_seek(inst, cd->target_lba, now, 0u);
        cd->read_active = 1u;
        update_status_activity(cd);
    } else {
        schway__cdrom_sector_schedule(inst, now);
    }
    response_status(inst, 3u);
    finish_command(cd);
}

static void begin_motor_on(schway *inst, uint64_t now)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t p[SCHWAY__CDROM_FIFO_CAP] = {0u};
    if (command_parameters(inst, 0u, 0u, p) == 0u) {
        return;
    }
    if (cd->motor_on != 0u) {
        command_error(inst, 0x20u);
        return;
    }
    if (disc_type(inst) == 0u || inst->disc_host.lid_open != 0u) {
        command_error(inst, 0x80u);
        return;
    }
    set_motor(cd, 1u);
    response_status(inst, 3u);
    schedule_second(cd, now, UINT64_C(476000));
}

static void begin_stop(schway *inst, uint64_t now)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t p[SCHWAY__CDROM_FIFO_CAP] = {0u};
    if (command_parameters(inst, 0u, 0u, p) == 0u) {
        return;
    }
    cd->read_active = 0u;
    cd->play_active = 0u;
    cd->paused = 0u;
    cancel_sector_event(cd);
    cd->seek_active = 0u;
    cd->seek_event = 0u;
    cd->seek_deadline = 0u;
    update_status_activity(cd);
    response_status(inst, 3u);
    uint64_t delay = cd->motor_on == 0u ? UINT64_C(0x1D7B) :
                     cd->drive_speed == 1u ? UINT64_C(0xD38ACA) : UINT64_C(0x18A6076);
    schedule_second(cd, now, delay);
}

static void begin_pause(schway *inst, uint64_t now)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t p[SCHWAY__CDROM_FIFO_CAP] = {0u};
    if (command_parameters(inst, 0u, 0u, p) == 0u) {
        return;
    }
    uint32_t was_paused = cd->paused;
    cd->paused = 1u;
    cd->read_active = 0u;
    cd->play_active = 0u;
    cancel_sector_event(cd);
    update_status_activity(cd);
    response_status(inst, 3u);
    uint64_t delay = was_paused != 0u ? UINT64_C(7666) :
                     UINT64_C(2170000) >> (cd->drive_speed - 1u);
    schedule_second(cd, now, delay);
}

static void begin_init(schway *inst, uint64_t now)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t p[SCHWAY__CDROM_FIFO_CAP] = {0u};
    if (command_parameters(inst, 0u, 0u, p) == 0u) {
        return;
    }
    cd->mode = 0x20u;
    cd->drive_speed_changed = cd->drive_speed != 1u;
    cd->drive_speed = 1u;
    cd->read_active = 0u;
    cd->play_active = 0u;
    cd->seek_active = 0u;
    cd->paused = 0u;
    cancel_sector_event(cd);
    cd->seek_event = 0u;
    cd->seek_deadline = 0u;
    set_motor(cd, 1u);
    update_status_activity(cd);
    response_status(inst, 3u);
    schedule_second(cd, now, UINT64_C(476000));
}

static void begin_mute(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t p[SCHWAY__CDROM_FIFO_CAP] = {0u};
    if (command_parameters(inst, 0u, 0u, p) == 0u) {
        return;
    }
    cd->muted = cd->command == 0x0Bu;
    response_status(inst, 3u);
    finish_command(cd);
}

static void begin_setfilter(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t p[SCHWAY__CDROM_FIFO_CAP] = {0u};
    if (command_parameters(inst, 2u, 2u, p) == 0u) {
        return;
    }
    cd->file = p[0];
    cd->channel = p[1];
    consume_parameters(cd, 2u);
    response_status(inst, 3u);
    finish_command(cd);
}

static void begin_setmode(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t p[SCHWAY__CDROM_FIFO_CAP] = {0u};
    if (command_parameters(inst, 1u, 1u, p) == 0u) {
        return;
    }
    cd->mode = p[0];
    uint32_t target = (p[0] & 0x80u) != 0u ? 2u : 1u;
    cd->drive_speed_changed = cd->drive_speed != target;
    cd->drive_speed = target;
    consume_parameters(cd, 1u);
    response_status(inst, 3u);
    finish_command(cd);
}

static void begin_getparam(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t p[SCHWAY__CDROM_FIFO_CAP] = {0u};
    uint8_t bytes[5] = {(uint8_t)cd->status, (uint8_t)cd->mode, 0u,
                        (uint8_t)cd->file, (uint8_t)cd->channel};
    if (command_parameters(inst, 0u, 0u, p) != 0u) {
        response(inst, bytes, 5u, 3u);
        finish_command(cd);
    }
}

static void begin_getloc_l(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t p[SCHWAY__CDROM_FIFO_CAP] = {0u};
    uint8_t bytes[8];
    if (command_parameters(inst, 0u, 0u, p) == 0u) {
        return;
    }
    if (cd->sector_count == 0u || cd->seek_active != 0u ||
        (cd->status & (1u << 2)) != 0u ||
        schway__cdrom_sector_track_type_at(inst, cd->current_lba) == 0u ||
        schway__cdrom_sector_track_type_at(inst, cd->current_lba) == SCHWAY_DISC_TRACK_AUDIO) {
        command_error(inst, 0x80u);
        return;
    }
    uint32_t newest = (cd->sector_head + cd->sector_count - 1u) % SCHWAY__CDROM_SECTOR_CAP;
    for (uint32_t i = 0u; i < 8u; i++) {
        bytes[i] = cd->sector[newest][12u + i];
    }
    response(inst, bytes, 8u, 3u);
    finish_command(cd);
}

static void begin_getloc_p(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t p[SCHWAY__CDROM_FIFO_CAP] = {0u};
    uint8_t bytes[8];
    if (command_parameters(inst, 0u, 0u, p) == 0u) {
        return;
    }
    if (disc_type(inst) == 0u || (cd->status & (1u << 2)) != 0u ||
        schway__cdrom_sector_track_type_at(inst, cd->current_lba) == 0u) {
        command_error(inst, 0x80u);
        return;
    }
    schway__cdrom_sector_getlocp(inst, bytes);
    response(inst, bytes, 8u, 3u);
    finish_command(cd);
}

static void begin_setsession(schway *inst, uint64_t now)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t p[SCHWAY__CDROM_FIFO_CAP] = {0u};
    if (command_parameters(inst, 1u, 1u, p) == 0u) {
        return;
    }
    if (p[0] == 0u) {
        command_error(inst, 0x10u);
        return;
    }
    consume_parameters(cd, 1u);
    response_status(inst, 3u);
    schedule_second(cd, now, UINT64_C(476000));
}

static void begin_gettn(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t p[SCHWAY__CDROM_FIFO_CAP] = {0u};
    uint8_t bytes[3] = {(uint8_t)cd->status, 1u,
                        to_bcd(inst->disc_host.toc.track_count)};
    if (command_parameters(inst, 0u, 0u, p) == 0u) {
        return;
    }
    if (disc_type(inst) == 0u) {
        command_error(inst, 0x80u);
        return;
    }
    response(inst, bytes, 3u, 3u);
    finish_command(cd);
}

static void begin_gettd(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t p[SCHWAY__CDROM_FIFO_CAP] = {0u};
    if (command_parameters(inst, 1u, 1u, p) == 0u) {
        return;
    }
    if (!bcd_valid(p[0]) || bcd_value(p[0]) > inst->disc_host.toc.track_count) {
        command_error(inst, 0x10u);
        return;
    }
    if (disc_type(inst) == 0u) {
        command_error(inst, 0x80u);
        return;
    }
    uint32_t track_index = bcd_value(p[0]);
    uint32_t target = track_index == 0u
                          ? inst->disc_host.toc.tracks[inst->disc_host.toc.track_count - 1u].leadout
                          : inst->disc_host.toc.tracks[track_index - 1u].index1;
    uint64_t absolute = (uint64_t)target + 150u;
    uint8_t bytes[3] = {(uint8_t)cd->status,
                        to_bcd((uint32_t)(absolute / (60u * 75u))),
                        to_bcd((uint32_t)((absolute / 75u) % 60u))};
    consume_parameters(cd, 1u);
    response(inst, bytes, 3u, 3u);
    finish_command(cd);
}

static void begin_seek_command(schway *inst, uint64_t now)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t p[SCHWAY__CDROM_FIFO_CAP] = {0u};
    if (command_parameters(inst, 0u, 0u, p) == 0u) {
        return;
    }
    if (disc_type(inst) == 0u || inst->disc_host.lid_open != 0u) {
        command_error(inst, 0x80u);
        return;
    }
    cd->status &= ~(1u << 2);
    cd->read_active = 0u;
    cd->play_active = 0u;
    cancel_sector_event(cd);
    update_status_activity(cd);
    uint32_t target = cd->command == 0x16u
                          ? (cd->target_lba > 25u ? cd->target_lba - 25u : 0u)
                          : cd->target_lba;
    set_motor(cd, 1u);
    response_status(inst, 3u);
    if (cd->command == 0x15u &&
        (schway__cdrom_sector_track_type_at(inst, target) == 0u ||
         schway__cdrom_sector_track_type_at(inst, target) == SCHWAY_DISC_TRACK_AUDIO)) {
        cd->status |= 1u << 2;
        schedule_second(cd, now, UINT64_C(67737600));
        return;
    }
    begin_seek(inst, target, now, 0u);
}

static void begin_test(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t p[SCHWAY__CDROM_FIFO_CAP] = {0u};
    uint8_t bytes[SCHWAY__CDROM_FIFO_CAP] = {0u};
    if (command_parameters(inst, 1u, 1u, p) == 0u) {
        return;
    }
    if (p[0] == 0x20u) {
        bytes[0] = inst->region == SCHWAY_REGION_NTSC_J ? 0x96u : 0x97u;
        bytes[1] = inst->region == SCHWAY_REGION_NTSC_J ? 0x09u : 0x01u;
        bytes[2] = inst->region == SCHWAY_REGION_NTSC_J ? 0x12u : 0x10u;
        bytes[3] = 0xC2u;
        consume_parameters(cd, 1u);
        response(inst, bytes, 4u, 3u);
        finish_command(cd);
        return;
    }
    if (p[0] == 0x22u) {
        const char *region = inst->region == SCHWAY_REGION_NTSC_J ? "for Japan" :
                             inst->region == SCHWAY_REGION_NTSC_U ? "for U/C" : "for Europe";
        uint32_t count = inst->region == SCHWAY_REGION_NTSC_J ? 9u :
                         inst->region == SCHWAY_REGION_NTSC_U ? 7u : 10u;
        memcpy(bytes, region, count);
        consume_parameters(cd, 1u);
        response(inst, bytes, count, 3u);
        finish_command(cd);
        return;
    }
    command_invalid(inst);
}

static void begin_getid(schway *inst, uint64_t now)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t p[SCHWAY__CDROM_FIFO_CAP] = {0u};
    if (command_parameters(inst, 0u, 0u, p) != 0u) {
        response_status(inst, 3u);
        schedule_second(cd, now, UINT64_C(0x4A00));
    }
}

static void begin_reset(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t p[SCHWAY__CDROM_FIFO_CAP] = {0u};
    if (command_parameters(inst, 0u, 0u, p) == 0u) {
        return;
    }
    cd->mode = 0x20u;
    cd->drive_speed_changed = cd->drive_speed != 1u;
    cd->drive_speed = 1u;
    cd->unlock_enabled = 0u;
    cd->unlock_step = 0u;
    cd->status &= ~((1u << 2) | (1u << 3) | 1u);
    cd->read_active = 0u;
    cd->play_active = 0u;
    cd->seek_active = 0u;
    cd->seek_event = 0u;
    cd->seek_deadline = 0u;
    cancel_sector_event(cd);
    cd->paused = 0u;
    cd->sector_head = 0u;
    cd->sector_count = 0u;
    cd->sector_offset = 0u;
    set_motor(cd, 0u);
    update_status_activity(cd);
    response_status(inst, 3u);
    finish_command(cd);
}

static void begin_command(schway *inst, uint64_t now)
{
    switch (inst->cdrom.command) {
    case 0x01u: begin_nop(inst); return;
    case 0x02u: begin_setloc(inst); return;
    case 0x03u: begin_play(inst, now); return;
    case 0x04u: case 0x05u: begin_scan(inst); return;
    case 0x06u: case 0x1Bu: begin_read(inst, now); return;
    case 0x07u: begin_motor_on(inst, now); return;
    case 0x08u: begin_stop(inst, now); return;
    case 0x09u: begin_pause(inst, now); return;
    case 0x0Au: begin_init(inst, now); return;
    case 0x0Bu: case 0x0Cu: begin_mute(inst); return;
    case 0x0Du: begin_setfilter(inst); return;
    case 0x0Eu: begin_setmode(inst); return;
    case 0x0Fu: begin_getparam(inst); return;
    case 0x10u: begin_getloc_l(inst); return;
    case 0x11u: begin_getloc_p(inst); return;
    case 0x12u: begin_setsession(inst, now); return;
    case 0x13u: begin_gettn(inst); return;
    case 0x14u: begin_gettd(inst); return;
    case 0x15u: case 0x16u: begin_seek_command(inst, now); return;
    case 0x19u: begin_test(inst); return;
    case 0x1Au: begin_getid(inst, now); return;
    case 0x1Cu: begin_reset(inst); return;
    case 0x50u: case 0x51u: case 0x52u: case 0x53u:
    case 0x54u: case 0x55u: case 0x56u:
        command_unlock(inst);
        return;
    case 0x57u:
        inst->cdrom.unlock_enabled = 0u;
        inst->cdrom.unlock_step = 0u;
        command_invalid(inst);
        return;
    default:
        command_invalid(inst);
        return;
    }
}

uint64_t schway__cdrom_next(const schway *inst)
{
    const struct schway__cdrom *cd = &inst->cdrom;
    uint64_t next = UINT64_MAX;
    if (cd->command_event != 0u && cd->command_deadline < next) {
        next = cd->command_deadline;
    }
    if (cd->sector_event != 0u && cd->sector_deadline < next) {
        next = cd->sector_deadline;
    }
    if (cd->seek_event != 0u && cd->seek_deadline < next) {
        next = cd->seek_deadline;
    }
    return next;
}

void schway__cdrom_event(schway *inst, uint64_t now)
{
    struct schway__cdrom *cd = &inst->cdrom;
    observe_lid(inst);
    if (inst->run_active == 0u) return;
    if (cd->command_event != 0u && cd->command_deadline <= now) {
        cd->command_event = 0u;
        cd->command_deadline = 0u;
        if (cd->command_pending == 1u) {
            begin_command(inst, now);
        } else if (cd->command_pending == 2u) {
            complete_second_response(inst);
        }
    }
    if (cd->sector_event != 0u && cd->sector_deadline <= now) {
        schway__cdrom_sector_event(inst, now);
    }
    if (cd->seek_event != 0u && cd->seek_deadline <= now) {
        cd->seek_event = 0u;
        cd->seek_deadline = 0u;
        cd->current_lba = cd->target_lba;
        cd->seek_active = 0u;
        update_status_activity(cd);
        if (cd->read_active != 0u) {
            schway__cdrom_sector_schedule(inst, now);
        } else {
            schway__cdrom_sector_seek(inst);
        }
        if (cd->command_pending == 2u && cd->command_event == 0u) {
            complete_second_response(inst);
        }
    }
}
uint8_t schway__cdrom_read(schway *inst, uint32_t address, uint64_t now)
{
    observe_lid(inst);
    if (schway__cdrom_next(inst) <= now) {
        schway__cdrom_event(inst, now);
    }
    struct schway__cdrom *cd = &inst->cdrom;
    switch (address & 3u) {
    case 0u:
        return status_read(cd);
    case 1u:
        return pop_result(cd);
    case 2u:
        return schway__cdrom_sector_read(inst);
    case 3u:
        if ((cd->bank & 1u) == 0u) {
            return (uint8_t)cd->interrupt_mask;
        }
        return hint_status(cd);
    default:
        return 0u;
    }
}

void schway__cdrom_write(schway *inst, uint32_t address, uint8_t value, uint64_t now)
{
    observe_lid(inst);
    if (schway__cdrom_next(inst) <= now) {
        schway__cdrom_event(inst, now);
    }
    struct schway__cdrom *cd = &inst->cdrom;
    uint32_t offset = address & 3u;
    if (offset == 0u) {
        cd->bank = value & 3u;
        return;
    }
    switch (cd->bank) {
    case 0u:
        if (offset == 1u) {
            if (cd->command_pending == 0u && cd->interrupt_count == 0u) {
                cd->command = value;
                cd->last_command = value;
                cd->command_pending = 1u;
                cd->command_event = 1u;
                cd->command_deadline = deadline_after(
                    now, value == 0x1Cu ? UINT64_C(0x400000) : SCHWAY__CDROM_ACK_CYCLES);
            }
        } else if (offset == 2u && cd->parameter_count < SCHWAY__CDROM_FIFO_CAP) {
            uint32_t tail =
                (cd->parameter_head + cd->parameter_count) % SCHWAY__CDROM_FIFO_CAP;
            cd->parameter[tail] = value;
            cd->parameter_count++;
        } else if (offset == 3u) schway__cdrom_bfrd_set(inst, value, now);
        break;
    case 1u:
        if (offset == 2u) {
            cd->interrupt_mask = value & 0x1Fu;
            update_irq(inst);
        } else if (offset == 3u) {
            acknowledge_interrupt(inst, value);
            if ((value & (1u << 6)) != 0u) {
                memset(cd->parameter, 0, sizeof cd->parameter);
                cd->parameter_head = 0u;
                cd->parameter_count = 0u;
            }
            if ((value & (1u << 7)) != 0u) {
                schway__cdrom_reset(inst);
            }
        }
        break;
    case 2u:
        if (offset == 2u) {
            cd->atv_pending[0] = value;
        } else if (offset == 3u) {
            cd->atv_pending[1] = value;
        }
        break;
    case 3u:
        if (offset == 1u) {
            cd->atv_pending[2] = value;
        } else if (offset == 2u) {
            cd->atv_pending[3] = value;
        } else if (offset == 3u && (value & 0x20u) != 0u) {
            memcpy(cd->atv_active, cd->atv_pending, sizeof cd->atv_active);
        }
        break;
    default:
        break;
    }
}

void schway__cdrom_io(schway *inst, uint32_t wr, uint32_t address, uint32_t width,
                      uint32_t *value, uint32_t *cycles)
{
    uint32_t sink = 0u;
    if (cycles == NULL) {
        cycles = &sink;
    }
    uint64_t now = schway__sched_now(inst, *cycles);
    if (wr != 0u) {
        for (uint32_t i = 0u; i < width; i++) {
            schway__cdrom_write(inst, address + i, (uint8_t)(*value >> (i * 8u)), now);
        }
    } else {
        uint32_t result = 0u;
        for (uint32_t i = 0u; i < width; i++) {
            result |= (uint32_t)schway__cdrom_read(inst, address + i, now) << (i * 8u);
        }
        *value = result;
    }
    if (schway__cdrom_next(inst) <= now) {
        schway__cdrom_event(inst, now);
    }
    schway__sched_recalc(inst);
}
