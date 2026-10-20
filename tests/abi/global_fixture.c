/* Negative control for abi.globals: one mutable file-scope integer, written
   by a function, which the check must report as writable data. No
   floating-point type appears, so this builds under -mgeneral-regs-only. */
#include <stdint.h>

uint32_t schway__abi_global_fixture(uint32_t step);

static uint32_t counter;

uint32_t schway__abi_global_fixture(uint32_t step)
{
    counter += step;
    return counter;
}
