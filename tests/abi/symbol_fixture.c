/* Negative control for abi.symbols: one external function whose name is
   outside the schway_ namespace, which the check must report. No
   floating-point type appears, so this builds under -mgeneral-regs-only. */
#include <stdint.h>

uint32_t abi_symbol_fixture(uint32_t x);

uint32_t abi_symbol_fixture(uint32_t x)
{
    return x + 1u;
}
