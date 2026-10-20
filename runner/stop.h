/* The runner's stop on SIGINT or SIGTERM. Runner-only: the core never sees
   a signal. */
#ifndef SCHWAYSTATION_RUNNER_STOP_H
#define SCHWAYSTATION_RUNNER_STOP_H

/* Installs the handlers. Call once, after the command line is valid. */
void schway__stop_install(void);

/* 1 once SIGINT or SIGTERM has arrived, else 0. */
int schway__stop_requested(void);

#endif /* SCHWAYSTATION_RUNNER_STOP_H */
