/* schwaystation-run --probe: what a PS-EXE file is, without running it. */
#ifndef SCHWAYSTATION_RUNNER_PROBE_H
#define SCHWAYSTATION_RUNNER_PROBE_H

#include <stdio.h>

/* Reads the file at path (at most 0800h + 2 MiB are kept), asks the core's
   validator about it and prints the report described in docs/runner.md to
   out, one key=value line per field. Returns 0 on success. When the file is
   unreadable, empty or not a valid PS-EXE it prints
   "schwaystation-run: <path>: <reason>" to standard error, prints nothing to
   out and returns 1. No instance is created. */
int schway__probe_print(FILE *out, const char *path);

#endif /* SCHWAYSTATION_RUNNER_PROBE_H */
