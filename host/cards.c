/* Host-only file operations for raw memory cards. */
#include "cards.h"

schway_host_card_status schway_host_card_import_raw(const char *path, void *out,
                                                     size_t out_size,
                                                     schway_host_card_lint *lint)
{
    (void)path;
    (void)out;
    (void)out_size;
    (void)lint;
    return SCHWAY_HOST_CARD_ERR_IO;
}

schway_host_card_status schway_host_card_save_atomic(const char *path, const void *bytes,
                                                      size_t size)
{
    (void)path;
    (void)bytes;
    (void)size;
    return SCHWAY_HOST_CARD_ERR_IO;
}

#if defined(SCHWAY_HOST_CARD_TESTING)
schway_host_card_status schway_host_card_test_save_atomic(const char *path, const void *bytes,
                                                           size_t size,
                                                           schway_host_card_test_failure failure)
{
    (void)path;
    (void)bytes;
    (void)size;
    (void)failure;
    return SCHWAY_HOST_CARD_ERR_IO;
}
#endif
