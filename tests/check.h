/* Comparison macros for the tests. No framework: each test is a program
   whose main ends with CHECK_DONE(), and CTest reads its exit status.
   A failed check prints file:line and the values to stderr and the program
   carries on, so one run shows every failure. */
#ifndef SCHWAYSTATION_CHECK_H
#define SCHWAYSTATION_CHECK_H

#include <stdio.h>

static int check_failures = 0;

/* cond must be true. */
#define CHECK(cond)                                                                                \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #cond);               \
            check_failures++;                                                                      \
        }                                                                                          \
    } while (0)

/* a == b, compared and printed as unsigned decimal. */
#define CHECK_EQ_U64(a, b)                                                                         \
    do {                                                                                           \
        unsigned long long check_a_ = (unsigned long long)(a);                                     \
        unsigned long long check_b_ = (unsigned long long)(b);                                     \
        if (check_a_ != check_b_) {                                                                \
            fprintf(stderr, "%s:%d: %s == %s failed: %llu != %llu\n", __FILE__, __LINE__, #a, #b,  \
                    check_a_, check_b_);                                                           \
            check_failures++;                                                                      \
        }                                                                                          \
    } while (0)

/* a == b, compared as unsigned and printed in hex. */
#define CHECK_EQ_HEX(a, b)                                                                         \
    do {                                                                                           \
        unsigned long long check_a_ = (unsigned long long)(a);                                     \
        unsigned long long check_b_ = (unsigned long long)(b);                                     \
        if (check_a_ != check_b_) {                                                                \
            fprintf(stderr, "%s:%d: %s == %s failed: 0x%llx != 0x%llx\n", __FILE__, __LINE__, #a,  \
                    #b, check_a_, check_b_);                                                       \
            check_failures++;                                                                      \
        }                                                                                          \
    } while (0)

/* Ends main: exit status 1 if any check failed, else 0. */
#define CHECK_DONE() return check_failures != 0 ? 1 : 0

#endif /* SCHWAYSTATION_CHECK_H */
