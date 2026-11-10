/* API version 3, as first released. FROZEN: once released this file never
   changes, except that the version check is a lower bound because a later
   API version keeps every version-3 name. It checks every constant added in
   version 3, so a rename, a removal or a changed value fails here. */
#include "schwaystation.h"
#include "../check.h"

int main(void)
{
    CHECK(SCHWAY_API_VERSION >= 3);
    CHECK_EQ_U64(SCHWAY_FAULT_DMA, 10);
    CHECK_DONE();
}
