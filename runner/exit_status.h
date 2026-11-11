/* The exit statuses of schwaystation-run, defined once. --help prints this
   table and docs/ repeats it. A status enters the table when the runner first
   returns it. */
#ifndef SCHWAYSTATION_RUNNER_EXIT_STATUS_H
#define SCHWAYSTATION_RUNNER_EXIT_STATUS_H

enum { SCHWAY__EXIT_OK = 0, SCHWAY__EXIT_FAILURE = 1, SCHWAY__EXIT_USAGE = 2,
       SCHWAY__EXIT_FIXTURE = 6, SCHWAY__EXIT_FAULT = 7, SCHWAY__EXIT_HANG = 8 };

typedef struct schway__exit_status {
    int code;
    const char *name;
    const char *meaning;
} schway__exit_status;

static const schway__exit_status schway__exit_statuses[] = {
    {SCHWAY__EXIT_OK, "done", "every frame ran, or SIGINT or SIGTERM ended the run after a whole frame"},
    {SCHWAY__EXIT_FAILURE, "failure", "a file or card could not be read or written, a PS-EXE is invalid, memory ran out or the core failed"},
    {SCHWAY__EXIT_USAGE, "usage", "the command line is invalid, so nothing ran"},
    {SCHWAY__EXIT_FIXTURE, "fixture", "the fixture reported failure: mailbox status FFh, a mailbox CRC mismatch or an exit port byte not 00h"},
    {SCHWAY__EXIT_FAULT, "fault", "the first fault stopped the run"},
    {SCHWAY__EXIT_HANG, "hang", "--stop-on mailbox saw no done or fault status by the frame cap"},
};

#endif /* SCHWAYSTATION_RUNNER_EXIT_STATUS_H */
