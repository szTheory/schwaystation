/* Host-provided disc callbacks are installed here; sectors are consumed only
   from the run loop once CD-ROM reads are connected. */
#include "internal.h"

schway_status schway_open_disc(schway *inst, const schway_disc *disc)
{
    (void)inst;
    (void)disc;
    return SCHWAY_STATUS_ERR_ARGUMENT;
}

schway_status schway_set_lid(schway *inst, uint32_t open)
{
    (void)inst;
    (void)open;
    return SCHWAY_STATUS_ERR_ARGUMENT;
}

schway_status schway_swap_disc(schway *inst, const schway_disc *disc)
{
    (void)inst;
    (void)disc;
    return SCHWAY_STATUS_ERR_ARGUMENT;
}
