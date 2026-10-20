/* Negative control for core.no_host_clock: a library that reads the host's
   wall clock through time(NULL), which the check must report. No
   floating-point type appears, so this builds under -mgeneral-regs-only. */
#include <stdint.h>
#include <time.h>

uint32_t schway__abi_clock_fixture(void);

uint32_t schway__abi_clock_fixture(void)
{
    return (uint32_t)time(NULL);
}
