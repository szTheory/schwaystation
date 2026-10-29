/* The only file with a branch per operating system for the peak resident
   set. Each branch converts the platform's unit to KiB, rounding up. */

/* struct rusage's ru_maxrss is outside POSIX, and macOS hides it when the
   runner's _POSIX_C_SOURCE is defined unless the Darwin extensions are on;
   glibc and musl always declare it. Defined before any system header. */
#if defined(__APPLE__)
#define _DARWIN_C_SOURCE 1
#endif

#include "rss.h"

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>

/* PeakWorkingSetSize is in bytes; K32GetProcessMemoryInfo is exported by
   KERNEL32.dll, so no PSAPI.dll import is added (Microsoft Learn,
   "GetProcessMemoryInfo function", remarks on PSAPI_VERSION). */
uint64_t schway__rss_max_kib(void)
{
    PROCESS_MEMORY_COUNTERS pmc;
    if (!K32GetProcessMemoryInfo(GetCurrentProcess(), &pmc, (DWORD)sizeof pmc)) {
        return 0u;
    }
    return ((uint64_t)pmc.PeakWorkingSetSize + 1023u) / 1024u;
}

#elif defined(__wasi__)

/* WASI has no call that reports resident memory. */
uint64_t schway__rss_max_kib(void)
{
    return 0u;
}

#else

#include <sys/resource.h>

/* ru_maxrss is in KiB on Linux (getrusage(2)) and in bytes on macOS
   (getrusage(2) of the macOS manual). */
uint64_t schway__rss_max_kib(void)
{
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) != 0 || ru.ru_maxrss < 0) {
        return 0u;
    }
    uint64_t v = (uint64_t)ru.ru_maxrss;
#ifdef __APPLE__
    v = (v + 1023u) / 1024u;
#endif
    return v;
}

#endif
