#include <stdio.h>
#include <string.h>

static int known_case(const char *name)
{
    static const char *const cases[] = {
        "mapping", "ppf1", "ppf2", "ppf3", "bps", "atomic", "bounds"};
    const size_t count = sizeof cases / sizeof cases[0];
    for (size_t i = 0u; i < count; i++) {
        if (strcmp(name, cases[i]) == 0) return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3 || argv[1][0] == '\0' || argv[2][0] == '\0' ||
        strlen(argv[2]) >= 4096u) {
        fputs("usage: schwaystation-test-disc-patch CASE BUILD_WORK_ROOT\n", stderr);
        return 2;
    }
    if (!known_case(argv[1])) {
        fprintf(stderr, "unknown patch test case: %s\n", argv[1]);
        return 2;
    }
    fprintf(stderr, "patch test case is not implemented: %s\n", argv[1]);
    return 1;
}
