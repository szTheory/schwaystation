/* Per-voice integer envelope step helper.
   Hardware references: .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md §1.4. */
#include "internal.h"
#include "spu.h"

void schway__spu_envelope_rate(uint32_t rate, uint32_t decreasing, int32_t *step,
                               uint32_t *ticks)
{
    (void)rate;
    (void)decreasing;
    *step = 0;
    *ticks = 0u;
}

void schway__spu_envelope_step(schway *inst, uint32_t voice)
{
    (void)inst;
    (void)voice;
}
