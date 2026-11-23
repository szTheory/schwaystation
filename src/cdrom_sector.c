/* Scheduled sector requests and the firmware-visible CD data window. */
#include <string.h>

#include "internal.h"
#include "cdxa.h"

#define SCHWAY__CDROM_SECTOR_PERIOD UINT64_C(451584)

static uint64_t sector_deadline(uint64_t now, uint64_t delta)
{
    if (now > SCHWAY__STATE_TICKS_MAX - delta) {
        return SCHWAY__STATE_TICKS_MAX;
    }
    return now + delta;
}

void schway__cdrom_bfrd_set(schway *inst, uint8_t value, uint64_t now)
{
    inst->cdrom.bfrd = (value >> 7) & 1u;
    if (inst->cdrom.bfrd != 0u) schway__dma_cdrom_ready(inst, now);
}

static uint32_t sector_bcd(uint8_t value, uint32_t maximum, uint32_t *out)
{
    if ((value & 0x0Fu) > 9u || (value >> 4) > 9u) return 0u;
    *out = (uint32_t)(value >> 4) * 10u + (value & 0x0Fu);
    return *out <= maximum;
}

static uint8_t sector_to_bcd(uint32_t value)
{
    return (uint8_t)(((value / 10u) << 4) | (value % 10u));
}

static uint32_t sector_track(const schway *inst, uint32_t lba, uint32_t *track_index)
{
    /* TOC index and lead-out values are zero-based LBAs. Setloc removes the
       150-frame MSF lead-in before a seek reaches this lookup (docs/hardware/cdrom.md). */
    uint64_t position = lba;
    for (uint32_t i = 0u; i < inst->disc_host.toc.track_count; i++) {
        const schway_disc_track *track = &inst->disc_host.toc.tracks[i];
        if (position >= track->index0 && position < track->leadout) {
            *track_index = i;
            return track->type;
        }
    }
    return 0u;
}

uint32_t schway__cdrom_sector_track_type_at(const schway *inst, uint32_t lba)
{
    uint32_t track_index = 0u;
    return inst->disc_host.attached != 0u ? sector_track(inst, lba, &track_index) : 0u;
}

/* Remove raw data-track seek headers while preserving streamed sector slots. */
void schway__cdrom_sector_discard_seek_headers(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    for (uint32_t i = 0u; i < cd->sector_count;) {
        uint32_t slot = (cd->sector_head + i) % SCHWAY__CDROM_SECTOR_CAP;
        uint32_t type = schway__cdrom_sector_track_type_at(inst, cd->sector_lba[slot]);
        if (cd->sector_size[slot] != SCHWAY__CDROM_SECTOR_BYTES || type == 0u ||
            type == SCHWAY_DISC_TRACK_AUDIO) {
            i++;
            continue;
        }
        for (uint32_t j = i; j + 1u < cd->sector_count; j++) {
            uint32_t dst = (cd->sector_head + j) % SCHWAY__CDROM_SECTOR_CAP;
            uint32_t src = (cd->sector_head + j + 1u) % SCHWAY__CDROM_SECTOR_CAP;
            memcpy(cd->sector[dst], cd->sector[src], SCHWAY__CDROM_SECTOR_BYTES);
            cd->sector_lba[dst] = cd->sector_lba[src];
            cd->sector_size[dst] = cd->sector_size[src];
        }
        cd->sector_count--;
        if (i == 0u) cd->sector_offset = 0u;
    }
}

static uint16_t sector_q_crc(const uint8_t q[12])
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

static uint32_t sector_q_valid(const uint8_t q[12], uint32_t track_count)
{
    uint32_t track, index, minute, second, frame;
    if ((q[0] & 0x0Fu) != 1u || q[6] != 0u ||
        (q[1] != 0xAAu && (sector_bcd(q[1], 99u, &track) == 0u ||
                            (track == 0u ? q[2] != 0u
                                         : track_count != 0u && track > track_count))) ||
        sector_bcd(q[2], 99u, &index) == 0u || index > 1u ||
        (q[1] == 0xAAu && index != 1u) ||
        sector_bcd(q[3], 99u, &minute) == 0u ||
        sector_bcd(q[4], 59u, &second) == 0u ||
        sector_bcd(q[5], 74u, &frame) == 0u ||
        sector_bcd(q[7], 99u, &minute) == 0u ||
        sector_bcd(q[8], 59u, &second) == 0u ||
        sector_bcd(q[9], 74u, &frame) == 0u) {
        return 0u;
    }
    uint16_t crc = sector_q_crc(q);
    return q[10] == (uint8_t)(crc >> 8) && q[11] == (uint8_t)crc;
}

static uint32_t sector_q_lba(const uint8_t q[12], uint32_t track_count, uint32_t *lba)
{
    uint32_t track, index, minute, second, frame;
    if (q[1] == 0xAAu || sector_q_valid(q, track_count) == 0u ||
        sector_bcd(q[1], 99u, &track) == 0u || track > track_count ||
        sector_bcd(q[2], 99u, &index) == 0u ||
        sector_bcd(q[7], 99u, &minute) == 0u ||
        sector_bcd(q[8], 59u, &second) == 0u ||
        sector_bcd(q[9], 74u, &frame) == 0u) {
        return 0u;
    }
    uint64_t absolute = (uint64_t)minute * 60u * 75u + (uint64_t)second * 75u + frame;
    if (absolute < 150u || absolute - 150u > UINT32_MAX) return 0u;
    *lba = (uint32_t)(absolute - 150u);
    return 1u;
}

uint32_t schway__cdrom_sector_q_valid(const schway *inst, const uint8_t q[12])
{
    return sector_q_valid(q, inst->disc_host.toc.track_count);
}

static void sector_make_q(const schway *inst, uint32_t lba, uint8_t q[12])
{
    uint32_t track_index = 0u;
    uint32_t type = sector_track(inst, lba, &track_index);
    uint64_t absolute = (uint64_t)lba + 150u;
    const schway_disc_track *track = track_index < inst->disc_host.toc.track_count
                                         ? &inst->disc_host.toc.tracks[track_index]
                                         : NULL;
    uint32_t index = 1u;
    uint64_t relative = 0u;
    if (track != NULL && lba < track->index1) {
        index = 0u;
        relative = (uint64_t)track->index1 - lba;
    } else if (track != NULL) {
        relative = (uint64_t)lba - track->index1;
    }
    uint32_t am = (uint32_t)(absolute / (60u * 75u));
    uint32_t as = (uint32_t)((absolute / 75u) % 60u);
    uint32_t af = (uint32_t)(absolute % 75u);
    uint32_t rm = (uint32_t)(relative / (60u * 75u));
    uint32_t rs = (uint32_t)((relative / 75u) % 60u);
    uint32_t rf = (uint32_t)(relative % 75u);
    memset(q, 0, 12u);
    q[0] = type == SCHWAY_DISC_TRACK_AUDIO ? 0x01u : 0x41u;
    q[1] = sector_to_bcd(track_index + 1u);
    q[2] = sector_to_bcd(index);
    q[3] = sector_to_bcd(rm);
    q[4] = sector_to_bcd(rs);
    q[5] = sector_to_bcd(rf);
    q[7] = sector_to_bcd(am);
    q[8] = sector_to_bcd(as);
    q[9] = sector_to_bcd(af);
    uint16_t crc = sector_q_crc(q);
    q[10] = (uint8_t)(crc >> 8);
    q[11] = (uint8_t)crc;
}

static uint32_t sector_location(schway *inst, uint32_t lba, const uint8_t subcode[96],
                                uint32_t bad_q, uint32_t *position)
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint32_t track_index = 0u;
    if (sector_track(inst, lba, &track_index) == 0u) {
        *position = cd->current_lba;
        return cd->q_record_valid;
    }
    if (bad_q != 0u) {
        *position = cd->current_lba;
        return cd->q_record_valid;
    }
    uint8_t q[12] = {0u};
    uint32_t present = 0u;
    for (uint32_t i = 0u; i < 96u; i++) {
        q[i / 8u] |= (uint8_t)(((subcode[i] >> 6) & 1u) << (7u - (i & 7u)));
        present |= (subcode[i] >> 6) & 1u;
    }
    if (present != 0u && sector_q_valid(q, inst->disc_host.toc.track_count) != 0u) {
        memcpy(cd->q_record, q, sizeof q);
        cd->q_record_valid = 1u;
        if (sector_q_lba(q, inst->disc_host.toc.track_count, position) != 0u) return 1u;
        *position = lba;
        return 1u;
    }
    if (cd->q_record_valid != 0u) {
        *position = cd->current_lba;
        return 1u;
    }
    *position = lba;
    return 1u;
}

static void sector_pop_oldest(struct schway__cdrom *cd)
{
    if (cd->sector_count != 0u) {
        cd->sector_head = (cd->sector_head + 1u) % SCHWAY__CDROM_SECTOR_CAP;
        cd->sector_count--;
    }
    cd->sector_offset = 0u;
}

void schway__cdrom_sector_ack(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    if (cd->interrupt_count == 0u && cd->sector_int1_pending != 0u) {
        cd->sector_int1_pending = 0u;
        schway__cdrom_push_interrupt(inst, 1u);
    }
}

static void sector_failed(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    cd->read_active = 0u;
    cd->play_active = 0u;
    cd->sector_event = 0u;
    cd->sector_deadline = 0u;
    cd->sector_retry = 0u;
    cd->status = (cd->status | 1u) & ~((1u << 5) | (1u << 7));
    if (cd->result_count < SCHWAY__CDROM_FIFO_CAP) {
        uint32_t tail = (cd->result_head + cd->result_count) % SCHWAY__CDROM_FIFO_CAP;
        cd->result[tail] = (uint8_t)cd->status;
        cd->result_count++;
    }
    if (cd->result_count < SCHWAY__CDROM_FIFO_CAP) {
        uint32_t tail = (cd->result_head + cd->result_count) % SCHWAY__CDROM_FIFO_CAP;
        cd->result[tail] = 0x80u;
        cd->result_count++;
    }
    schway__cdrom_push_interrupt(inst, 5u);
}

void schway__cdrom_sector_schedule(schway *inst, uint64_t now)
{
    struct schway__cdrom *cd = &inst->cdrom;
    if ((cd->read_active == 0u && cd->play_active == 0u) || cd->seek_active != 0u ||
        inst->disc_host.attached == 0u || inst->disc_host.lid_open != 0u) {
        cd->sector_event = 0u;
        cd->sector_deadline = 0u;
        return;
    }
    uint64_t period = SCHWAY__CDROM_SECTOR_PERIOD >> (cd->drive_speed - 1u);
    cd->sector_event = 1u;
    cd->sector_deadline = sector_deadline(now, period);
}

static uint32_t sector_is_well_formed(const uint8_t *sector)
{
    if (sector[0] != 0u || sector[11] != 0u || sector[15] < 1u || sector[15] > 2u) {
        return 0u;
    }
    for (uint32_t i = 1u; i < 11u; i++) {
        if (sector[i] != 0xFFu) {
            return 0u;
        }
    }
    if (sector[15] == 2u && memcmp(&sector[16], &sector[20], 4u) != 0) {
        return 0u;
    }
    return 1u;
}

static void sector_retry_or_fail(schway *inst, uint64_t now)
{
    struct schway__cdrom *cd = &inst->cdrom;
    if (cd->read_active != 0u && cd->read_s == 0u && cd->sector_retry == 0u) {
        cd->sector_retry = 1u;
        schway__cdrom_sector_schedule(inst, now);
    } else {
        sector_failed(inst);
    }
}

static void sector_advance(schway *inst, uint32_t lba, uint32_t bad_q, uint32_t position)
{
    if (bad_q == 0u) inst->cdrom.current_lba = position;
    if (lba != UINT32_MAX) inst->cdrom.target_lba = lba + 1u;
}

static void sector_interrupt(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    if (cd->interrupt_count == 0u) schway__cdrom_push_interrupt(inst, 1u);
    else cd->sector_int1_pending = 1u;
}

static void sector_store(struct schway__cdrom *cd, const uint8_t sector[2352], uint32_t lba,
                         uint32_t size)
{
    if (cd->sector_count != 0u && cd->sector_offset >= cd->sector_size[cd->sector_head]) {
        sector_pop_oldest(cd);
    }
    if (cd->sector_count == SCHWAY__CDROM_SECTOR_CAP) sector_pop_oldest(cd);
    uint32_t tail = (cd->sector_head + cd->sector_count) % SCHWAY__CDROM_SECTOR_CAP;
    memcpy(cd->sector[tail], sector, SCHWAY__CDROM_SECTOR_BYTES);
    cd->sector_lba[tail] = lba;
    cd->sector_size[tail] = size;
    cd->sector_count++;
}

void schway__cdrom_sector_seek(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    if (inst->run_active == 0u || inst->disc_host.attached == 0u ||
        inst->disc_host.lid_open != 0u || inst->disc_host.callbacks.read_sector == NULL) {
        return;
    }
    uint32_t lba = cd->target_lba;
    uint32_t track_index = 0u;
    uint32_t track_type = sector_track(inst, lba, &track_index);
    (void)track_index;
    if (track_type == 0u || track_type == SCHWAY_DISC_TRACK_AUDIO) {
        cd->current_lba = lba;
        return;
    }
    uint8_t sector[SCHWAY__CDROM_SECTOR_BYTES] = {0u};
    uint8_t subcode[96] = {0u};
    if (inst->disc_host.callbacks.read_sector(inst->disc_host.callbacks.user, lba,
                                              sector, subcode) == 0u ||
        sector_is_well_formed(sector) == 0u) {
        cd->current_lba = lba;
        return;
    }
    uint32_t bad_q = inst->disc_host.callbacks.bad_q != NULL
                         ? inst->disc_host.callbacks.bad_q(inst->disc_host.callbacks.user, lba)
                         : 0u;
    uint32_t position = lba;
    if (sector_location(inst, lba, subcode, bad_q, &position) != 0u || bad_q == 0u) {
        cd->current_lba = position;
    }
    /* GetlocL exposes the completed SeekL target header (SCM.02; docs/hardware/cdrom.md#command-behavior). */
    sector_store(cd, sector, lba, SCHWAY__CDROM_SECTOR_BYTES);
}

void schway__cdrom_sector_getlocp(schway *inst, uint8_t bytes[8])
{
    struct schway__cdrom *cd = &inst->cdrom;
    uint8_t fallback[12];
    if (cd->q_record_valid == 0u) sector_make_q(inst, cd->current_lba, fallback);
    const uint8_t *q = cd->q_record_valid != 0u ? cd->q_record : fallback;
    bytes[0] = q[1];
    bytes[1] = q[2];
    bytes[2] = q[7];
    bytes[3] = q[8];
    bytes[4] = q[9];
    bytes[5] = q[3];
    bytes[6] = q[4];
    bytes[7] = q[5];
}

void schway__cdrom_sector_event(schway *inst, uint64_t now)
{
    struct schway__cdrom *cd = &inst->cdrom;
    cd->sector_event = 0u;
    cd->sector_deadline = 0u;
    if (inst->run_active == 0u || (cd->read_active == 0u && cd->play_active == 0u) ||
        cd->seek_active != 0u || inst->disc_host.attached == 0u ||
        inst->disc_host.lid_open != 0u) {
        return;
    }

    uint32_t lba = cd->target_lba;
    uint8_t sector[SCHWAY__CDROM_SECTOR_BYTES] = {0u};
    uint8_t subcode[96] = {0u};
    if (inst->disc_host.callbacks.read_sector == NULL ||
        inst->disc_host.callbacks.read_sector(inst->disc_host.callbacks.user, lba, sector,
                                              subcode) == 0u) {
        sector_retry_or_fail(inst, now);
        return;
    }

    uint32_t track_index = 0u;
    uint32_t track_type = sector_track(inst, lba, &track_index);
    uint32_t audio_track = track_type == SCHWAY_DISC_TRACK_AUDIO;
    if (audio_track == 0u && sector_is_well_formed(sector) == 0u) {
        sector_retry_or_fail(inst, now);
        return;
    }

    uint32_t bad_q = inst->disc_host.callbacks.bad_q != NULL
                         ? inst->disc_host.callbacks.bad_q(inst->disc_host.callbacks.user, lba)
                         : 0u;
    uint32_t position = lba;
    (void)sector_location(inst, lba, subcode, bad_q, &position);
    sector_advance(inst, lba, bad_q, position);

    if (audio_track != 0u && cd->play_active == 0u) {
        cd->sector_retry = 0u;
        schway__cdrom_sector_schedule(inst, now);
        return;
    }

    uint32_t mode2 = audio_track == 0u && sector[15] == 2u;
    uint32_t xa_audio = mode2 != 0u && (sector[18] & 0x44u) == 0x44u;
    uint32_t filter_match = (cd->mode & 0x08u) == 0u ||
                            (sector[16] == cd->file && sector[17] == cd->channel);
    if (xa_audio != 0u && (cd->mode & 0x40u) != 0u && filter_match != 0u) {
        uint32_t decoded_frames = 0u;
        if (schway__cdxa_decode(inst, sector, SCHWAY__CDROM_SECTOR_BYTES,
                                inst->xa_decode_scratch,
                                SCHWAY__CDXA_MAX_OUTPUT_FRAMES, &decoded_frames) != 0u) {
            for (uint32_t frame = 0u; frame < decoded_frames; frame++) {
                schway__cdrom_xa_audio_push(inst, inst->xa_decode_scratch[2u * frame],
                                            inst->xa_decode_scratch[2u * frame + 1u]);
            }
            cd->sector_retry = 0u;
            schway__cdrom_sector_schedule(inst, now);
            return;
        }
    }

    if (mode2 != 0u && (sector[18] & 0x08u) == 0u) {
        cd->sector_retry = 0u;
        schway__cdrom_sector_schedule(inst, now);
        return;
    }

    if (cd->play_active != 0u) {
        if ((cd->mode & 0x04u) != 0u && (lba & 0x0Fu) == 0x0Fu) sector_interrupt(inst);
        if (audio_track != 0u) {
            schway__cdrom_audio_cdda_sector(inst, sector);
            sector_store(cd, sector, lba, SCHWAY__CDROM_SECTOR_BYTES);
            schway__dma_cdrom_ready(inst, now);
        }
        cd->sector_retry = 0u;
        schway__cdrom_sector_schedule(inst, now);
        return;
    }

    if (mode2 != 0u && (cd->mode & 0x08u) != 0u && filter_match == 0u) {
        if (cd->read_s == 0u && cd->sector_retry == 0u) {
            cd->sector_retry = 1u;
            cd->target_lba = lba;
            schway__cdrom_sector_schedule(inst, now);
            return;
        }
        cd->sector_retry = 0u;
        schway__cdrom_sector_schedule(inst, now);
        return;
    }

    sector_store(cd, sector, lba, (cd->mode & 0x20u) != 0u ? 2340u : 2048u);
    schway__dma_cdrom_ready(inst, now);
    cd->sector_retry = 0u;
    sector_interrupt(inst);
    schway__cdrom_sector_schedule(inst, now);
}

uint8_t schway__cdrom_sector_read(schway *inst)
{
    struct schway__cdrom *cd = &inst->cdrom;
    if (cd->bfrd == 0u || cd->sector_count == 0u) {
        return 0u;
    }
    uint32_t index = cd->sector_head;
    uint32_t size = cd->sector_size[index];
    uint32_t data_start = size == SCHWAY__CDROM_SECTOR_BYTES ? 0u : size == 2340u ? 12u : 24u;
    if (cd->sector_offset >= size) {
        return cd->sector[index][size == 2340u ? 0x920u : 0x7F8u];
    }
    uint8_t value = cd->sector[index][data_start + cd->sector_offset];
    cd->sector_offset++;
    if (cd->sector_offset == size && cd->sector_count > 1u) {
        sector_pop_oldest(cd);
    }
    return value;
}
