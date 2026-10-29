/* Peak resident memory for the --bench report. Runner-only. */
#ifndef SCHWAYSTATION_RUNNER_RSS_H
#define SCHWAYSTATION_RUNNER_RSS_H

#include <stdint.h>

/* The peak resident set of this process so far, in KiB rounded up, or 0
   when the platform has no such figure (WASI) or the query fails. */
uint64_t schway__rss_max_kib(void);

#endif /* SCHWAYSTATION_RUNNER_RSS_H */
