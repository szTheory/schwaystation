/* Checks the local library report without exposing any input path. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../test_open.h"

#define LIBRARY_HEADER "serial,md5,track1_sha1,title,region,media,predicate,status,evidence,date"
#define LIBRARY_LINE 2048
#define INVENTORY_HEADER "ordinal,entry_sha256,content_sha256,status,predicate,profile,evidence"
#define INVENTORY_LINE 8192

static int parse_count(const char *text, unsigned *value)
{
    char *end = NULL;
    unsigned long parsed = strtoul(text, &end, 10);
    if (end == text || *end != '\0' || parsed > 32768ul) return 0;
    *value = (unsigned)parsed;
    return 1;
}

static int valid_sha256(const char *text)
{
    if (strlen(text) != 64u) return 0;
    for (size_t i = 0u; i < 64u; i++) {
        char value = text[i];
        if (!((value >= '0' && value <= '9') || (value >= 'a' && value <= 'f'))) return 0;
    }
    return 1;
}

static int inventory_csv_field(char **cursor, char *field, size_t capacity)
{
    char *position = *cursor;
    size_t length = 0u;
    int quoted = *position == '"';
    int closed = !quoted;
    if (quoted) position++;
    while (*position != '\0') {
        if (quoted && *position == '"') {
            if (position[1] == '"') {
                if (length + 1u >= capacity) return 0;
                field[length++] = '"';
                position += 2;
                continue;
            }
            position++;
            closed = 1;
            break;
        }
        if (!quoted && *position == ',') break;
        if (length + 1u >= capacity) return 0;
        field[length++] = *position++;
    }
    if (!closed || (quoted && *position != ',' && *position != '\0')) return 0;
    field[length] = '\0';
    if (*position == ',') {
        *cursor = position + 1;
    } else {
        *cursor = position;
    }
    return 1;
}

static int check_inventory(const char *path, unsigned expected_entries,
                           unsigned expected_pass, unsigned expected_fail,
                           unsigned expected_unknown)
{
    char line[INVENTORY_LINE];
    FILE *file = schway__test_open_file(path, "rb");
    if (file == NULL || fgets(line, sizeof line, file) == NULL) return 1;
    line[strcspn(line, "\r\n")] = '\0';
    if (strcmp(line, INVENTORY_HEADER) != 0) {
        (void)fclose(file);
        return 1;
    }
    unsigned entries = 0u;
    unsigned pass = 0u;
    unsigned fail = 0u;
    unsigned unknown = 0u;
    while (fgets(line, sizeof line, file) != NULL) {
        size_t length = strlen(line);
        if (length == sizeof line - 1u && line[length - 1u] != '\n' && !feof(file)) {
            (void)fclose(file);
            return 1;
        }
        line[strcspn(line, "\r\n")] = '\0';
        char *cursor = line;
        char ordinal[16];
        char entry_sha[65];
        char content_sha[65];
        char status[16];
        char predicate[INVENTORY_LINE];
        char profile[INVENTORY_LINE];
        char evidence[INVENTORY_LINE];
        if (!inventory_csv_field(&cursor, ordinal, sizeof ordinal) ||
            !inventory_csv_field(&cursor, entry_sha, sizeof entry_sha) ||
            !inventory_csv_field(&cursor, content_sha, sizeof content_sha) ||
            !inventory_csv_field(&cursor, status, sizeof status) ||
            !inventory_csv_field(&cursor, predicate, sizeof predicate) ||
            !inventory_csv_field(&cursor, profile, sizeof profile) ||
            !inventory_csv_field(&cursor, evidence, sizeof evidence) || *cursor != '\0') {
            (void)fclose(file);
            return 1;
        }
        unsigned number = 0u;
        if (!parse_count(ordinal, &number) || number != entries + 1u ||
            !valid_sha256(entry_sha) || !valid_sha256(content_sha)) {
            (void)fclose(file);
            return 1;
        }
        if (strcmp(status, "pass") == 0) pass++;
        else if (strcmp(status, "fail") == 0) fail++;
        else if (strcmp(status, "unknown") == 0) unknown++;
        else {
            (void)fclose(file);
            return 1;
        }
        entries++;
    }
    int failed = ferror(file) != 0;
    if (fclose(file) != 0) failed = 1;
    if (failed || entries != expected_entries || pass != expected_pass ||
        fail != expected_fail || unknown != expected_unknown ||
        pass + fail + unknown != entries) return 1;
    return 0;
}

static int valid_date(const char *date)
{
    if (strlen(date) != 10u || date[4] != '-' || date[7] != '-') return 0;
    for (size_t i = 0u; i < 10u; i++) {
        if (i != 4u && i != 7u && (date[i] < '0' || date[i] > '9')) return 0;
    }
    int year = (date[0] - '0') * 1000 + (date[1] - '0') * 100 +
               (date[2] - '0') * 10 + date[3] - '0';
    int month = (date[5] - '0') * 10 + date[6] - '0';
    int day = (date[8] - '0') * 10 + date[9] - '0';
    static const int month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) return 0;
    int maximum = month_days[month - 1];
    if (month == 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) maximum++;
    return day >= 1 && day <= maximum;
}

static int valid_row(char *line, char *key, size_t capacity)
{
    char *field[10];
    char *cursor = line;
    int written;
    for (size_t i = 0u; i < 10u; i++) {
        field[i] = cursor;
        if (strchr(cursor, '"') != NULL) return 0;
        char *comma = strchr(cursor, ',');
        if (i == 9u) {
            if (comma != NULL) return 0;
        } else {
            if (comma == NULL) return 0;
            *comma = '\0';
            cursor = comma + 1;
        }
        if (field[i][0] == '\0') return 0;
    }
    if (strcmp(field[7], "pass") != 0 && strcmp(field[7], "unknown") != 0) return 0;
    if (!valid_date(field[9])) return 0;
    written = snprintf(key, capacity, "%s|%s|%s", field[0], field[1], field[6]);
    return written > 0 && (size_t)written < capacity;
}

static int check_report(const char *path, const char *mode, const char *track_sha1)
{
    char line[LIBRARY_LINE];
    char previous[LIBRARY_LINE] = "";
    unsigned rows = 0u;
    int identity = 0;
    int boot = 0;
    int play = 0;
    FILE *file = schway__test_open_file(path, "rb");
    if (file == NULL || fgets(line, sizeof line, file) == NULL) return 1;
    line[strcspn(line, "\r\n")] = '\0';
    if (strcmp(line, LIBRARY_HEADER) != 0) { (void)fclose(file); return 1; }
    while (fgets(line, sizeof line, file) != NULL) {
        char key[LIBRARY_LINE];
        char raw[LIBRARY_LINE];
        size_t length = strlen(line);
        if (length == sizeof line - 1u && line[length - 1u] != '\n' && !feof(file)) {
            (void)fclose(file);
            return 1;
        }
        if (length != 0u && line[length - 1u] == '\n') line[--length] = '\0';
        if (length != 0u && line[length - 1u] == '\r') line[--length] = '\0';
        (void)snprintf(raw, sizeof raw, "%s", line);
        if (length == 0u || !valid_row(line, key, sizeof key) ||
            (rows != 0u && strcmp(previous, key) > 0)) {
            (void)fclose(file);
            return 1;
        }
        if (rows != 0u && strcmp(previous, key) == 0) { (void)fclose(file); return 1; }
        (void)snprintf(previous, sizeof previous, "%s", key);
        if (strstr(raw, "PRIVATE_SENTINEL") != NULL ||
            strstr(raw, "12481bed29c6c0d54a6f776ef5cec981") == NULL) {
            (void)fclose(file);
            return 1;
        }
        if (strstr(raw, "SWAY-00001,") == NULL || strstr(raw, track_sha1) == NULL ||
            strstr(raw, "Schway Synthetic Metadata Fixture") == NULL ||
            strstr(raw, ",ntsc-u,cue/bin,") == NULL || strstr(raw, "2026-11-12") == NULL) {
            (void)fclose(file);
            return 1;
        }
        if (strstr(raw, ",metadata.identity,pass,synthetic:iso9660-known-answer,") != NULL) identity++;
        if (strstr(raw, ",boots@600,unknown,unavailable,") != NULL) boot++;
        if (strstr(raw, ",plays@600,unknown,unavailable,") != NULL) play++;
        rows++;
    }
    int failed = ferror(file) != 0;
    if (fclose(file) != 0) failed = 1;
    if (failed) return 1;
    if (strcmp(mode, "empty") == 0 || strcmp(mode, "unlisted") == 0) return rows == 0u ? 0 : 1;
    if (strcmp(mode, "identity") == 0) return rows == 3u && identity == 1 && boot == 1 && play == 1 ? 0 : 1;
    return 2;
}

int main(int argc, char **argv)
{
    if (argc == 7 && strcmp(argv[1], "inventory") == 0) {
        unsigned entries;
        unsigned pass;
        unsigned fail;
        unsigned unknown;
        if (!parse_count(argv[3], &entries) || !parse_count(argv[4], &pass) ||
            !parse_count(argv[5], &fail) || !parse_count(argv[6], &unknown)) return 2;
        return check_inventory(argv[2], entries, pass, fail, unknown);
    }
    if (argc != 4) {
        (void)fprintf(stderr, "usage: schwaystation-test-library <identity|empty|unlisted> <results.csv> <track-sha1> | inventory <report> <entries> <pass> <fail> <unknown>\n");
        return 2;
    }
    return check_report(argv[2], argv[1], argv[3]);
}
