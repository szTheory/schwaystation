#include "stop.h"

#include <signal.h>

/* Written only by the handler and read by the frame loop: ISO C17 7.14.1.1
   allows a handler to store to a volatile sig_atomic_t and nothing more. */
static volatile sig_atomic_t stop_flag;

static void on_signal(int sig)
{
    (void)sig;
    stop_flag = 1;
}

/* A signal the process inherited as ignored (nohup, a background job)
   stays ignored. */
static void catch_signal(int sig)
{
    if (signal(sig, on_signal) == SIG_IGN) {
        (void)signal(sig, SIG_IGN);
    }
}

void schway__stop_install(void)
{
    catch_signal(SIGINT);
    catch_signal(SIGTERM);
}

int schway__stop_requested(void)
{
    return stop_flag != 0;
}
