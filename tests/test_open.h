/* Test file operations use the portable C interface except on MSVC, whose
   secure CRT requires fopen_s and reports fopen as C4996 under /WX. */
#ifndef SCHWAYSTATION_TEST_OPEN_H
#define SCHWAYSTATION_TEST_OPEN_H

#include <stdio.h>

static inline FILE *schway__test_open_file(const char *path, const char *mode)
{
#if defined(_MSC_VER)
    FILE *file = NULL;
    if (fopen_s(&file, path, mode) != 0) {
        return NULL;
    }
    return file;
#else
    return fopen(path, mode);
#endif
}

#endif
