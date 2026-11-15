/* Host-provided disc metadata stays outside serialized device state. */
#include <string.h>

#include "internal.h"

static schway_status disc_prepare(const schway_disc *disc, schway_disc *callbacks,
                                 schway_toc *toc)
{
    if (disc == NULL || callbacks == NULL || toc == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    schway_status status = schway__instance_check_size(
        disc, SCHWAY__DISC_SIZE_FIRST, (uint32_t)sizeof *disc);
    if (status != SCHWAY_STATUS_OK) {
        return status;
    }
    if (disc->reserved0 != 0u || disc->read_sector == NULL || disc->toc == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }

    *callbacks = *disc;
    memset(toc, 0, sizeof *toc);
    toc->size = (uint32_t)sizeof *toc;
    if (callbacks->toc(callbacks->user, toc) == 0u) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    if (toc->size != sizeof *toc || toc->reserved0 != 0u) {
        return SCHWAY_STATUS_ERR_STRUCT_SIZE;
    }
    if (toc->track_count == 0u || toc->track_count > SCHWAY_DISC_MAX_TRACKS) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }

    for (uint32_t i = 0u; i < SCHWAY_DISC_MAX_TRACKS; i++) {
        const schway_disc_track *track = &toc->tracks[i];
        if (i >= toc->track_count) {
            schway_disc_track zero;
            memset(&zero, 0, sizeof zero);
            if (memcmp(track, &zero, sizeof zero) != 0) {
                return SCHWAY_STATUS_ERR_STRUCT_SIZE;
            }
            continue;
        }
        if (track->size != sizeof *track || track->reserved0 != 0u) {
            return SCHWAY_STATUS_ERR_STRUCT_SIZE;
        }
        if ((track->type != SCHWAY_DISC_TRACK_AUDIO && track->type != SCHWAY_DISC_TRACK_MODE1 &&
             track->type != SCHWAY_DISC_TRACK_MODE2) ||
            track->index0 > track->index1 || track->leadout <= track->index1 ||
            track->pregap > track->index1 - track->index0) {
            return SCHWAY_STATUS_ERR_ARGUMENT;
        }
        if (i != 0u) {
            const schway_disc_track *previous = &toc->tracks[i - 1u];
            if (track->index0 < previous->leadout || track->index1 <= previous->index1) {
                return SCHWAY_STATUS_ERR_ARGUMENT;
            }
        }
    }
    return SCHWAY_STATUS_OK;
}

static void disc_apply(schway *inst, const schway_disc *callbacks, const schway_toc *toc)
{
    inst->disc_host.callbacks = *callbacks;
    inst->disc_host.toc = *toc;
    inst->disc_host.attached = 1u;
}

schway_status schway_open_disc(schway *inst, const schway_disc *disc)
{
    if (inst == NULL || disc == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    if (inst->disc_host.attached != 0u) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    schway_disc callbacks;
    schway_toc toc;
    schway_status status = disc_prepare(disc, &callbacks, &toc);
    if (status != SCHWAY_STATUS_OK) {
        return status;
    }
    disc_apply(inst, &callbacks, &toc);
    return SCHWAY_STATUS_OK;
}

schway_status schway_set_lid(schway *inst, uint32_t open)
{
    if (inst == NULL || open > 1u) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    uint32_t changed = inst->disc_host.lid_open != open;
    inst->disc_host.lid_open = open;
    if (changed != 0u && open != 0u) {
        schway__cdrom_lid_changed(inst);
    }
    return SCHWAY_STATUS_OK;
}

schway_status schway_swap_disc(schway *inst, const schway_disc *disc)
{
    if (inst == NULL || disc == NULL) {
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    if (inst->disc_host.lid_open == 0u) {
        return SCHWAY_STATUS_ERR_LID_CLOSED;
    }
    schway_disc callbacks;
    schway_toc toc;
    schway_status status = disc_prepare(disc, &callbacks, &toc);
    if (status != SCHWAY_STATUS_OK) {
        return status;
    }
    disc_apply(inst, &callbacks, &toc);
    return SCHWAY_STATUS_OK;
}
