#include <stdio.h>
#include <string.h>

static int check_schema(const char *path)
{
    static const char header[] = "serial,md5,track1_sha1,title,region,media,predicate,status,evidence,date\n";
    char line[512];
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "compat.ratchet.rows: cannot open ledger\n");
        return 1;
    }
    if (fgets(line, sizeof(line), file) == NULL || strcmp(line, header) != 0) {
        fprintf(stderr, "compat.ratchet.rows: expected the ten meaningful ledger columns\n");
        (void)fclose(file);
        return 1;
    }
    (void)fclose(file);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3 || strcmp(argv[1], "schema") != 0) {
        fprintf(stderr, "usage: compat_tests schema <ledger.csv>\n");
        return 2;
    }
    return check_schema(argv[2]);
}
