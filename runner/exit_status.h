/* The exit statuses of schwaystation-run, defined once. --help prints this
   table and docs/ repeats it. A status enters the table when the runner first
   returns it. */
#ifndef SCHWAYSTATION_RUNNER_EXIT_STATUS_H
#define SCHWAYSTATION_RUNNER_EXIT_STATUS_H

enum { SCHWAY__EXIT_OK = 0, SCHWAY__EXIT_FAILURE = 1, SCHWAY__EXIT_USAGE = 2 };

typedef struct schway__exit_status {
    int code;
    const char *name;
    const char *meaning;
} schway__exit_status;

static const schway__exit_status schway__exit_statuses[] = {
    {SCHWAY__EXIT_OK, "done", "every frame ran"},
    {SCHWAY__EXIT_FAILURE, "failure", "memory ran out or the core failed"},
    {SCHWAY__EXIT_USAGE, "usage", "the command line is invalid, so nothing ran"},
};

#endif /* SCHWAYSTATION_RUNNER_EXIT_STATUS_H */
