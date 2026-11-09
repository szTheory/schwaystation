/* The fixture mailbox the runner reads for --stop-on mailbox: 256 bytes at RAM 10000h, where a fixture
   places it (docs/runner.md describes the layout and the lines printed). Runner-only: the core has no
   mailbox. */
#ifndef SCHWAYSTATION_RUNNER_MAILBOX_H
#define SCHWAYSTATION_RUNNER_MAILBOX_H

#include <stdio.h>

#include "schwaystation.h"

enum {
    SCHWAY__MAILBOX_PENDING = 0, /* no magic, or a status that is neither done nor fault */
    SCHWAY__MAILBOX_DONE = 1,    /* status 00h; the lines are printed */
    SCHWAY__MAILBOX_FAULT = 2,   /* status FFh; the lines are printed */
    SCHWAY__MAILBOX_BAD_CRC = 3  /* status 00h with a CRC that does not match; one line is printed */
};

/* Reads the mailbox with schway_peek and returns one of the values above. Done and fault print
   "mailbox SWMB status=<hh> records=<n>" and one "mailbox <id>=<value>" line per record to out. */
int schway__mailbox_check(const schway *inst, FILE *out);

#endif /* SCHWAYSTATION_RUNNER_MAILBOX_H */
