#ifndef SCHWAY_TEST_RC_COMPAT_H
#define SCHWAY_TEST_RC_COMPAT_H

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static inline int schway__test_rc_strncasecmp(const char *left, const char *right, size_t size)
{
    for (size_t i = 0u; i < size; i++) {
        int a = tolower((unsigned char)left[i]);
        int b = tolower((unsigned char)right[i]);
        if (a != b || a == 0 || b == 0) return a - b;
    }
    return 0;
}

static inline int schway__test_rc_strcasecmp(const char *left, const char *right)
{
    while (*left != '\0' && *right != '\0') {
        int a = tolower((unsigned char)*left++);
        int b = tolower((unsigned char)*right++);
        if (a != b) return a - b;
    }
    return (unsigned char)*left - (unsigned char)*right;
}

#define strncasecmp schway__test_rc_strncasecmp
#define strcasecmp schway__test_rc_strcasecmp

#endif
