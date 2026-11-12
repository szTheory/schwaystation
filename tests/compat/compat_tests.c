#include <stdio.h>
#include <string.h>

#include "../test_open.h"

#define COMPAT_FIELD_COUNT 10
#define COMPAT_LINE_SIZE 2048
#define COMPAT_MAX_ROWS 64

static int valid_date(const char *date)
{
    static const size_t separators[] = {4, 7};
    size_t i;

    if (strlen(date) != 10 || date[4] != '-' || date[7] != '-') {
        return 0;
    }
    for (i = 0; i < 10; ++i) {
        if (i != separators[0] && i != separators[1] &&
            (date[i] < '0' || date[i] > '9')) {
            return 0;
        }
    }
    return 1;
}

static int split_row(char *line, char *fields[COMPAT_FIELD_COUNT])
{
    char *cursor = line;
    size_t i;

    for (i = 0; i < COMPAT_FIELD_COUNT; ++i) {
        char *comma;
        fields[i] = cursor;
        if (strchr(cursor, '"') != NULL) {
            return 0;
        }
        comma = strchr(cursor, ',');
        if (i + 1 == COMPAT_FIELD_COUNT) {
            return comma == NULL;
        }
        if (comma == NULL) {
            return 0;
        }
        *comma = '\0';
        cursor = comma + 1;
    }
    return 0;
}

static int valid_row(char *line, char *key, size_t key_size)
{
    char *fields[COMPAT_FIELD_COUNT];
    int written;

    if (!split_row(line, fields)) {
        return 0;
    }
    for (size_t i = 0; i < COMPAT_FIELD_COUNT; ++i) {
        if (fields[i][0] == '\0') {
            return 0;
        }
    }
    if (strcmp(fields[7], "unknown") != 0 && strcmp(fields[7], "fail") != 0 &&
        strcmp(fields[7], "pass") != 0) {
        return 0;
    }
    if (!valid_date(fields[9])) {
        return 0;
    }
    written = snprintf(key, key_size, "%s|%s|%s", fields[0], fields[1], fields[6]);
    return written > 0 && (size_t)written < key_size;
}

static int check_schema(const char *path)
{
    static const char header[] = "serial,md5,track1_sha1,title,region,media,predicate,status,evidence,date\n";
    char line[COMPAT_LINE_SIZE];
    char keys[COMPAT_MAX_ROWS][COMPAT_LINE_SIZE];
    size_t row_count = 0;
    FILE *file = schway__test_open_file(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "compat.ratchet.rows: cannot open ledger\n");
        return 1;
    }
    if (fgets(line, sizeof(line), file) == NULL || strcmp(line, header) != 0) {
        fprintf(stderr, "compat.ratchet.rows: expected the ten meaningful ledger columns\n");
        (void)fclose(file);
        return 1;
    }
    while (fgets(line, sizeof(line), file) != NULL) {
        char key[COMPAT_LINE_SIZE];
        size_t length = strlen(line);
        int has_newline = length > 0 && line[length - 1] == '\n';

        if (has_newline) {
            line[--length] = '\0';
        }
        if (length > 0 && line[length - 1] == '\r') {
            line[--length] = '\0';
        }
        if (length == 0 || (!has_newline && !feof(file)) ||
            !valid_row(line, key, sizeof(key)) || row_count == COMPAT_MAX_ROWS) {
            fprintf(stderr, "compat.ratchet.rows: malformed or oversized ledger row\n");
            (void)fclose(file);
            return 1;
        }
        for (size_t i = 0; i < row_count; ++i) {
            if (strcmp(keys[i], key) == 0) {
                fprintf(stderr, "compat.ratchet.rows: duplicate serial+md5+predicate key\n");
                (void)fclose(file);
                return 1;
            }
        }
        (void)snprintf(keys[row_count], sizeof(keys[row_count]), "%s", key);
        ++row_count;
    }
    if (ferror(file) || row_count == 0) {
        fprintf(stderr, "compat.ratchet.rows: ledger has no valid rows\n");
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
