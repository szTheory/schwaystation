#include "clock.h"

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

/* QueryPerformanceCounter is a monotonic tick count and
   QueryPerformanceFrequency its rate (Microsoft Learn, "Acquiring
   high-resolution time stamps"). Whole seconds are split from the remainder
   so the multiplication cannot overflow. */
uint64_t schway__clock_ns(void)
{
    LARGE_INTEGER f;
    LARGE_INTEGER t;
    if (!QueryPerformanceFrequency(&f) || f.QuadPart <= 0 || !QueryPerformanceCounter(&t)) {
        return 0u;
    }
    uint64_t freq = (uint64_t)f.QuadPart;
    uint64_t ticks = (uint64_t)t.QuadPart;
    return (ticks / freq) * 1000000000u + (ticks % freq) * 1000000000u / freq;
}

#else

#include <time.h>

/* CLOCK_MONOTONIC never jumps with the wall clock (POSIX.1-2017,
   clock_gettime); WASI libc maps it to the monotonic clock of
   wasi:clocks. */
uint64_t schway__clock_ns(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0u;
    }
    return (uint64_t)ts.tv_sec * 1000000000u + (uint64_t)ts.tv_nsec;
}

#endif
