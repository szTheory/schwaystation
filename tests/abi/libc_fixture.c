/* Negative control for abi.undefined: a library that calls into libc. The
   call to abort leaves the undefined symbol abort on every target. No
   floating-point type appears, so this builds under -mgeneral-regs-only. */
#include <stdlib.h>

void schway__abi_libc_fixture(int fail);

void schway__abi_libc_fixture(int fail)
{
    if (fail) {
        abort();
    }
}
