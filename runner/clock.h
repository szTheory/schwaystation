/* The runner's monotonic clock for --bench. Runner-only: the core reads no
   clock. */
#ifndef SCHWAYSTATION_RUNNER_CLOCK_H
#define SCHWAYSTATION_RUNNER_CLOCK_H

#include <stdint.h>

/* A monotonic reading in nanoseconds, or 0 when the clock cannot be read.
   Only the difference between two readings means anything. */
uint64_t schway__clock_ns(void);

#endif /* SCHWAYSTATION_RUNNER_CLOCK_H */
