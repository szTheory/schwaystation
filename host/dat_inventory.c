/* Bounded, host-only clrmamepro occurrence accounting for the runner. */
#include "dat_inventory.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "../runner/sha256.h"

#define SCHWAY__DAT_MAX_BYTES (32u * 1024u * 1024u)
#define SCHWAY__DAT_MAX_TOKEN 8192u
#define SCHWAY__DAT_MAX_NESTING 64u
#define SCHWAY__DAT_MAX_ENTRIES 32768u
#define SCHWAY__DAT_EVIDENCE_MAX (4u * 1024u * 1024u)
#define SCHWAY__DAT_EVIDENCE_LINE_MAX 8192u
#define SCHWAY__DAT_CONTEXT_MAX 128u
#define SCHWAY__DAT_EVIDENCE_VALUE_MAX 4096u
#define SCHWAY__DAT_PRIVATE_PATH_MAX 2048u
#define SCHWAY__DAT_PIN "12c21cf79f1e20eff6ac68b80875816c717d0a7a"

typedef enum schway__dat_token_type {
    SCHWAY__DAT_TOKEN_EOF = 0,
    SCHWAY__DAT_TOKEN_WORD,
    SCHWAY__DAT_TOKEN_STRING,
    SCHWAY__DAT_TOKEN_OPEN,
    SCHWAY__DAT_TOKEN_CLOSE,
    SCHWAY__DAT_TOKEN_INVALID
} schway__dat_token_type;

typedef struct schway__dat_token {
    schway__dat_token_type type;
    size_t start;
    size_t end;
    const unsigned char *text;
    size_t length;
} schway__dat_token;

typedef struct schway__dat_entry {
    char entry_sha256[65];
    char content_sha256[65];
    char serial_sha256[65];
    char status;
    size_t evidence_offset;
    size_t evidence_length;
    int has_serial;
    int serial_seen;
    int has_evidence;
} schway__dat_entry;

typedef struct schway__dat_parser {
    const unsigned char *bytes;
    size_t length;
    size_t position;
    schway__dat_entry *entries;
    size_t entry_count;
    size_t entry_capacity;
} schway__dat_parser;

static void schway__dat_hash_hex(const unsigned char *bytes, size_t length, char output[65])
{
    schway__sha256 hash;
    unsigned char digest[32];
    schway__sha256_init(&hash);
    schway__sha256_update(&hash, bytes, length);
    schway__sha256_final(&hash, digest);
    schway__sha256_hex(digest, output);
}

static FILE *schway__dat_open_file(const char *path, const char *mode)
{
#if defined(_MSC_VER)
    FILE *file = NULL;
    int open_error = fopen_s(&file, path, mode);
    if (open_error != 0) {
        errno = open_error;
        return NULL;
    }
    return file;
#else
    return fopen(path, mode);
#endif
}

static int schway__dat_read_file(const char *path, size_t limit,
                                 unsigned char **bytes_out, size_t *length_out)
{
    FILE *file = schway__dat_open_file(path, "rb");
    if (file == NULL) return -1;
    if (fseek(file, 0L, SEEK_END) != 0) {
        (void)fclose(file);
        return -1;
    }
    long end = ftell(file);
    if (end < 0L) {
        (void)fclose(file);
        return -1;
    }
    if ((uint64_t)end > (uint64_t)limit) {
        (void)fclose(file);
        return -2;
    }
    if (end == 0L || fseek(file, 0L, SEEK_SET) != 0) {
        (void)fclose(file);
        return -3;
    }
    size_t length = (size_t)end;
    unsigned char *bytes = (unsigned char *)malloc(length + 1u);
    if (bytes == NULL) {
        (void)fclose(file);
        return -4;
    }
    size_t read = fread(bytes, 1u, length, file);
    int extra = fgetc(file);
    int failed = ferror(file) != 0 || read != length || extra != EOF;
    if (fclose(file) != 0) failed = 1;
    if (failed) {
        free(bytes);
        return -1;
    }
    bytes[length] = 0u;
    *bytes_out = bytes;
    *length_out = length;
    return 0;
}

static int schway__dat_next_token(schway__dat_parser *parser, schway__dat_token *token)
{
    const unsigned char *bytes = parser->bytes;
    size_t position = parser->position;
    for (;;) {
        while (position < parser->length && isspace((unsigned char)bytes[position])) position++;
        if (position + 1u < parser->length && bytes[position] == '/' &&
            bytes[position + 1u] == '/') {
            position += 2u;
            while (position < parser->length && bytes[position] != '\n' &&
                   bytes[position] != '\r') position++;
            continue;
        }
        break;
    }
    token->start = position;
    token->end = position;
    token->text = bytes + position;
    token->length = 0u;
    if (position == parser->length) {
        token->type = SCHWAY__DAT_TOKEN_EOF;
        parser->position = position;
        return 1;
    }
    if (bytes[position] == 0u) {
        token->type = SCHWAY__DAT_TOKEN_INVALID;
        return 0;
    }
    if (bytes[position] == '(') {
        token->type = SCHWAY__DAT_TOKEN_OPEN;
        token->end = ++position;
        parser->position = position;
        return 1;
    }
    if (bytes[position] == ')') {
        token->type = SCHWAY__DAT_TOKEN_CLOSE;
        token->end = ++position;
        parser->position = position;
        return 1;
    }
    if (bytes[position] == '"') {
        size_t start = ++position;
        while (position < parser->length) {
            if (bytes[position] == 0u) {
                token->type = SCHWAY__DAT_TOKEN_INVALID;
                return 0;
            }
            if (bytes[position] == '\\') {
                if (position + 1u >= parser->length || bytes[position + 1u] == 0u) {
                    token->type = SCHWAY__DAT_TOKEN_INVALID;
                    return 0;
                }
                position += 2u;
                continue;
            }
            if (bytes[position] == '"') {
                token->type = SCHWAY__DAT_TOKEN_STRING;
                token->text = bytes + start;
                token->length = position - start;
                token->end = position + 1u;
                parser->position = position + 1u;
                return token->length <= SCHWAY__DAT_MAX_TOKEN;
            }
            position++;
        }
        token->type = SCHWAY__DAT_TOKEN_INVALID;
        return 0;
    }
    size_t start = position;
    while (position < parser->length && !isspace((unsigned char)bytes[position]) &&
           bytes[position] != '(' && bytes[position] != ')' && bytes[position] != '"' &&
           bytes[position] != 0u) {
        position++;
    }
    if (position == start || position - start > SCHWAY__DAT_MAX_TOKEN) {
        token->type = SCHWAY__DAT_TOKEN_INVALID;
        return 0;
    }
    token->type = SCHWAY__DAT_TOKEN_WORD;
    token->text = bytes + start;
    token->length = position - start;
    token->end = position;
    parser->position = position;
    return 1;
}

static int schway__dat_token_is(const schway__dat_token *token, const char *text)
{
    size_t length = strlen(text);
    return token->type == SCHWAY__DAT_TOKEN_WORD && token->length == length &&
           memcmp(token->text, text, length) == 0;
}

static int schway__dat_hash_serial(const schway__dat_token *token, char serial_sha256[65],
                                   int *has_serial)
{
    schway__sha256 hash;
    unsigned char digest[32];
    schway__sha256_init(&hash);
    size_t written = 0u;
    for (size_t i = 0u; i < token->length; i++) {
        unsigned char value = token->text[i];
        if (value == '\\') {
            if (i + 1u >= token->length) return 0;
            value = token->text[++i];
        }
        if (value < 0x20u || value == 0x7fu) return 0;
        schway__sha256_update(&hash, &value, 1u);
        written++;
    }
    schway__sha256_final(&hash, digest);
    schway__sha256_hex(digest, serial_sha256);
    *has_serial = written != 0u;
    return 1;
}

static int schway__dat_reserve_entry(schway__dat_parser *parser)
{
    if (parser->entry_count >= SCHWAY__DAT_MAX_ENTRIES) return 0;
    if (parser->entry_count < parser->entry_capacity) return 1;
    size_t next = parser->entry_capacity == 0u ? 16u : parser->entry_capacity * 2u;
    if (next > SCHWAY__DAT_MAX_ENTRIES) next = SCHWAY__DAT_MAX_ENTRIES;
    if (next > SIZE_MAX / sizeof(*parser->entries)) return 0;
    schway__dat_entry *entries =
        (schway__dat_entry *)realloc(parser->entries, next * sizeof(*entries));
    if (entries == NULL) return -1;
    parser->entries = entries;
    parser->entry_capacity = next;
    return 1;
}

static int schway__dat_parse_group(schway__dat_parser *parser, unsigned depth,
                                   schway__dat_entry *game, size_t *end_out)
{
    if (depth > SCHWAY__DAT_MAX_NESTING) return 0;
    for (;;) {
        schway__dat_token key;
        if (!schway__dat_next_token(parser, &key) || key.type == SCHWAY__DAT_TOKEN_EOF) return 0;
        if (key.type == SCHWAY__DAT_TOKEN_CLOSE) {
            *end_out = parser->position;
            return 1;
        }
        if (key.type != SCHWAY__DAT_TOKEN_WORD) return 0;
        schway__dat_token value;
        if (!schway__dat_next_token(parser, &value)) return 0;
        if (value.type == SCHWAY__DAT_TOKEN_OPEN) {
            size_t child_end = 0u;
            if (!schway__dat_parse_group(parser, depth + 1u, game, &child_end)) return 0;
            if (game != NULL && depth == 1u &&
                (schway__dat_token_is(&key, "rom") || schway__dat_token_is(&key, "disk"))) {
                schway__sha256 content;
                /* The content digest is finalized after each content block and
                   folded into the previous digest to preserve block order. */
                unsigned char prior[32];
                char prior_hex[65];
                if (game->content_sha256[0] == '\0') {
                    schway__dat_hash_hex(parser->bytes + key.start, child_end - key.start,
                                         game->content_sha256);
                } else {
                    schway__sha256_init(&content);
                    schway__sha256_update(&content, game->content_sha256, 64u);
                    schway__sha256_update(&content, parser->bytes + key.start,
                                          child_end - key.start);
                    schway__sha256_final(&content, prior);
                    schway__sha256_hex(prior, prior_hex);
                    (void)memcpy(game->content_sha256, prior_hex, sizeof prior_hex);
                }
            }
        } else if ((value.type == SCHWAY__DAT_TOKEN_WORD ||
                    value.type == SCHWAY__DAT_TOKEN_STRING) &&
                   game != NULL && depth == 1u && schway__dat_token_is(&key, "serial")) {
            if (game->serial_seen ||
                !schway__dat_hash_serial(&value, game->serial_sha256, &game->has_serial)) return 0;
            game->serial_seen = 1;
        } else if (value.type != SCHWAY__DAT_TOKEN_WORD &&
                   value.type != SCHWAY__DAT_TOKEN_STRING) {
            return 0;
        }
    }
}

static int schway__dat_parse(schway__dat_parser *parser)
{
    if (parser->length >= 3u && parser->bytes[0] == 0xefu &&
        parser->bytes[1] == 0xbbu && parser->bytes[2] == 0xbfu) {
        parser->position = 3u;
    }
    for (;;) {
        schway__dat_token name;
        if (!schway__dat_next_token(parser, &name)) return 0;
        if (name.type == SCHWAY__DAT_TOKEN_EOF) return parser->entry_count != 0u;
        if (name.type != SCHWAY__DAT_TOKEN_WORD) return 0;
        schway__dat_token open;
        if (!schway__dat_next_token(parser, &open) || open.type != SCHWAY__DAT_TOKEN_OPEN) return 0;
        if (schway__dat_token_is(&name, "game")) {
            int reserved = schway__dat_reserve_entry(parser);
            if (reserved <= 0) return reserved < 0 ? -1 : -2;
            schway__dat_entry entry;
            (void)memset(&entry, 0, sizeof entry);
            entry.status = 'u';
            size_t end = 0u;
            if (!schway__dat_parse_group(parser, 1u, &entry, &end)) return 0;
            schway__dat_hash_hex(parser->bytes + name.start, end - name.start,
                                 entry.entry_sha256);
            if (entry.content_sha256[0] == '\0') {
                schway__dat_hash_hex((const unsigned char *)"", 0u, entry.content_sha256);
            }
            parser->entries[parser->entry_count++] = entry;
        } else {
            size_t end = 0u;
            if (!schway__dat_parse_group(parser, 1u, NULL, &end)) return 0;
        }
    }
}

static int schway__dat_compare_serials(const void *left, const void *right)
{
    const schway__dat_entry *const *a = (const schway__dat_entry *const *)left;
    const schway__dat_entry *const *b = (const schway__dat_entry *const *)right;
    return strcmp((*a)->serial_sha256, (*b)->serial_sha256);
}

static uint64_t schway__dat_count_duplicates(const schway__dat_parser *parser,
                                             uint64_t *missing_out)
{
    schway__dat_entry **serials =
        (schway__dat_entry **)malloc(parser->entry_count * sizeof(*serials));
    if (serials == NULL) return UINT64_MAX;
    size_t serial_count = 0u;
    uint64_t missing = 0u;
    for (size_t i = 0u; i < parser->entry_count; i++) {
        if (parser->entries[i].has_serial) serials[serial_count++] = &parser->entries[i];
        else missing++;
    }
    qsort(serials, serial_count, sizeof(*serials), schway__dat_compare_serials);
    uint64_t duplicate_entries = 0u;
    size_t i = 0u;
    while (i < serial_count) {
        size_t end = i + 1u;
        while (end < serial_count &&
               strcmp(serials[i]->serial_sha256, serials[end]->serial_sha256) == 0) end++;
        if (end - i > 1u) duplicate_entries += (uint64_t)(end - i);
        i = end;
    }
    free(serials);
    *missing_out = missing;
    return duplicate_entries;
}

static int schway__dat_hex_equal(const char *text, const char expected[65])
{
    if (strlen(text) != 64u) return 0;
    for (size_t i = 0u; i < 64u; i++) {
        unsigned char a = (unsigned char)tolower((unsigned char)text[i]);
        unsigned char b = (unsigned char)tolower((unsigned char)expected[i]);
        if (a != b) return 0;
    }
    return 1;
}

static int schway__dat_valid_context(const char *text)
{
    size_t length = strlen(text);
    if (length == 0u || length > SCHWAY__DAT_CONTEXT_MAX) return 0;
    for (size_t i = 0u; i < length; i++) {
        unsigned char value = (unsigned char)text[i];
        if (value < 0x20u || value > 0x7eu || value == '"') return 0;
    }
    return 1;
}

static int schway__dat_valid_evidence(const char *status, const char *evidence,
                                      const char content_sha256[65])
{
    if (evidence[0] == '\0' || strlen(evidence) > SCHWAY__DAT_EVIDENCE_VALUE_MAX) return 0;
    if (strcmp(status, "unknown") == 0) return 1;
    const char *exit_marker = strstr(evidence, ";exit=");
    const char *sha_marker = strstr(evidence, ";sha256=");
    if (exit_marker == NULL || sha_marker == NULL || sha_marker < exit_marker) return 0;
    const char *number = exit_marker + 6;
    if (*number < '0' || *number > '9') return 0;
    unsigned long exit_code = 0ul;
    const char *cursor = number;
    while (*cursor >= '0' && *cursor <= '9') {
        unsigned digit = (unsigned)(*cursor - '0');
        if (exit_code > (ULONG_MAX - digit) / 10ul) return 0;
        exit_code = exit_code * 10ul + digit;
        cursor++;
    }
    if (*cursor != ';' || cursor != sha_marker) return 0;
    if (strncmp(sha_marker + 8, content_sha256, 64u) != 0 ||
        (sha_marker[72] != '\0' && sha_marker[72] != ';')) return 0;
    if (strcmp(status, "pass") == 0) return exit_code == 0ul;
    return exit_code != 0ul;
}

static int schway__dat_split_evidence(char *line, char *fields[8])
{
    size_t count = 0u;
    fields[count++] = line;
    for (char *cursor = line; *cursor != '\0'; cursor++) {
        if (*cursor == '"') return 0;
        if (*cursor == ',') {
            if (count == 8u) return 0;
            *cursor = '\0';
            fields[count++] = cursor + 1;
        }
    }
    return count == 8u;
}

static schway__dat_inventory_status schway__dat_apply_evidence(
    const char *path, const char source_sha256[65], schway__dat_parser *parser,
    unsigned char **buffer_out, char predicate[SCHWAY__DAT_CONTEXT_MAX + 1u],
    char profile[SCHWAY__DAT_CONTEXT_MAX + 1u])
{
    unsigned char *bytes = NULL;
    size_t length = 0u;
    int read_status = schway__dat_read_file(path, SCHWAY__DAT_EVIDENCE_MAX, &bytes, &length);
    if (read_status == -1) return SCHWAY__DAT_INVENTORY_IO;
    if (read_status == -2) return SCHWAY__DAT_INVENTORY_LIMIT;
    if (read_status == -4) return SCHWAY__DAT_INVENTORY_NO_MEMORY;
    if (read_status != 0) return SCHWAY__DAT_INVENTORY_EVIDENCE;

    const char header[] =
        "source_sha256,ordinal,entry_sha256,content_sha256,predicate,profile,status,evidence";
    size_t position = 0u;
    size_t line_number = 0u;
    size_t rows = 0u;
    int have_context = 0;
    schway__dat_inventory_status status = SCHWAY__DAT_INVENTORY_EVIDENCE;
    while (position < length) {
        size_t start = position;
        while (position < length && bytes[position] != '\n') position++;
        size_t line_length = position - start;
        if (position < length) position++;
        if (line_length != 0u && bytes[start + line_length - 1u] == '\r') line_length--;
        if (line_length == 0u || line_length > SCHWAY__DAT_EVIDENCE_LINE_MAX) goto done;
        bytes[start + line_length] = '\0';
        char *line = (char *)bytes + start;
        line_number++;
        if (line_number == 1u) {
            if (strcmp(line, header) != 0) goto done;
            continue;
        }
        char *fields[8];
        if (!schway__dat_split_evidence(line, fields)) goto done;
        for (size_t i = 0u; i < 8u; i++) {
            if (fields[i][0] == '\0') goto done;
        }
        if (!schway__dat_hex_equal(fields[0], source_sha256) ||
            !schway__dat_hex_equal(fields[2], fields[2]) ||
            !schway__dat_hex_equal(fields[3], fields[3])) goto done;
        char *end = NULL;
        errno = 0;
        unsigned long ordinal = strtoul(fields[1], &end, 10);
        if (errno != 0 || end == fields[1] || *end != '\0' ||
            ordinal == 0ul || ordinal > (unsigned long)parser->entry_count) goto done;
        schway__dat_entry *entry = &parser->entries[(size_t)ordinal - 1u];
        if (!schway__dat_hex_equal(fields[2], entry->entry_sha256) ||
            !schway__dat_hex_equal(fields[3], entry->content_sha256) ||
            entry->has_evidence || !schway__dat_valid_context(fields[4]) ||
            !schway__dat_valid_context(fields[5]) ||
            (strcmp(fields[6], "pass") != 0 && strcmp(fields[6], "fail") != 0 &&
             strcmp(fields[6], "unknown") != 0) ||
            !schway__dat_valid_evidence(fields[6], fields[7], entry->content_sha256)) goto done;
        if (!have_context) {
            (void)snprintf(predicate, SCHWAY__DAT_CONTEXT_MAX + 1u, "%s", fields[4]);
            (void)snprintf(profile, SCHWAY__DAT_CONTEXT_MAX + 1u, "%s", fields[5]);
            have_context = 1;
        } else if (strcmp(predicate, fields[4]) != 0 || strcmp(profile, fields[5]) != 0) {
            goto done;
        }
        entry->status = strcmp(fields[6], "pass") == 0 ? 'p' :
                        strcmp(fields[6], "fail") == 0 ? 'f' : 'u';
        entry->evidence_offset = (size_t)((unsigned char *)fields[7] - bytes);
        entry->evidence_length = strlen(fields[7]);
        entry->has_evidence = 1;
        rows++;
    }
    if (line_number < 2u || rows == 0u) goto done;
    *buffer_out = bytes;
    status = SCHWAY__DAT_INVENTORY_OK;
done:
    if (status != SCHWAY__DAT_INVENTORY_OK) free(bytes);
    return status;
}

static int schway__dat_safe_private_path(const char *path)
{
    size_t length = strlen(path);
    if (length == 0u || length > SCHWAY__DAT_PRIVATE_PATH_MAX ||
        path[length - 1u] == '/' || path[length - 1u] == '\\') return 0;
    const char *part = path;
    for (const char *cursor = path;; cursor++) {
        if (*cursor == '/' || *cursor == '\\' || *cursor == '\0') {
            size_t part_length = (size_t)(cursor - part);
            if (part_length == 2u && part[0] == '.' && part[1] == '.') return 0;
            if (*cursor == '\0') break;
            part = cursor + 1;
        }
    }
    return 1;
}

static int schway__dat_csv_field(FILE *file, const char *value, size_t length)
{
    if (fputc('"', file) == EOF) return 0;
    for (size_t i = 0u; i < length; i++) {
        if (value[i] == '\r' || value[i] == '\n') return 0;
        if (value[i] == '"' && fputc('"', file) == EOF) return 0;
        if (fputc((unsigned char)value[i], file) == EOF) return 0;
    }
    return fputc('"', file) != EOF;
}

static int schway__dat_replace_file(const char *temporary, const char *destination)
{
#ifdef _WIN32
    return MoveFileExA(temporary, destination,
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return rename(temporary, destination) == 0;
#endif
}

static schway__dat_inventory_status schway__dat_write_private(
    const char *path, const schway__dat_parser *parser, const unsigned char *evidence_bytes,
    const char *predicate, const char *profile)
{
    if (!schway__dat_safe_private_path(path)) return SCHWAY__DAT_INVENTORY_OUTPUT;
    char temporary[SCHWAY__DAT_PRIVATE_PATH_MAX + 48u];
    FILE *file = NULL;
    for (unsigned attempt = 0u; attempt < 16u; attempt++) {
        int written = snprintf(temporary, sizeof temporary, "%s.tmp.%u", path, attempt);
        if (written < 0 || (size_t)written >= sizeof temporary) return SCHWAY__DAT_INVENTORY_OUTPUT;
        file = schway__dat_open_file(temporary, "wbx");
        if (file != NULL) break;
        if (errno != EEXIST) return SCHWAY__DAT_INVENTORY_OUTPUT;
    }
    if (file == NULL) return SCHWAY__DAT_INVENTORY_OUTPUT;
    int failed = fprintf(file, "ordinal,entry_sha256,content_sha256,status,predicate,profile,evidence\n") < 0;
    for (size_t i = 0u; !failed && i < parser->entry_count; i++) {
        const schway__dat_entry *entry = &parser->entries[i];
        const char *status = entry->status == 'p' ? "pass" :
                             entry->status == 'f' ? "fail" : "unknown";
        if (fprintf(file, "%zu,%s,%s,%s,", i + 1u, entry->entry_sha256,
                    entry->content_sha256, status) < 0) {
            failed = 1;
            break;
        }
        const char *row_predicate = entry->has_evidence ? predicate : "";
        const char *row_profile = entry->has_evidence ? profile : "";
        const char *row_evidence = entry->has_evidence ?
            (const char *)evidence_bytes + entry->evidence_offset : "";
        size_t evidence_length = entry->has_evidence ? entry->evidence_length : 0u;
        if (!schway__dat_csv_field(file, row_predicate, strlen(row_predicate)) ||
            fputc(',', file) == EOF ||
            !schway__dat_csv_field(file, row_profile, strlen(row_profile)) ||
            fputc(',', file) == EOF ||
            !schway__dat_csv_field(file, row_evidence, evidence_length) ||
            fputc('\n', file) == EOF) {
            failed = 1;
        }
    }
    if (fflush(file) != 0 || ferror(file) != 0) failed = 1;
    if (fclose(file) != 0) failed = 1;
    if (failed || !schway__dat_replace_file(temporary, path)) {
        (void)remove(temporary);
        return SCHWAY__DAT_INVENTORY_OUTPUT;
    }
    return SCHWAY__DAT_INVENTORY_OK;
}

schway__dat_inventory_status schway__dat_inventory_run(
    const char *dat_path, const char *evidence_path, const char *private_output_path,
    schway__dat_inventory_result *result)
{
    unsigned char *dat_bytes = NULL;
    unsigned char *evidence_bytes = NULL;
    size_t dat_length = 0u;
    schway__dat_inventory_status status = SCHWAY__DAT_INVENTORY_INVALID;
    schway__dat_parser parser;
    (void)memset(&parser, 0, sizeof parser);
    schway__dat_inventory_result completed;
    (void)memset(&completed, 0, sizeof completed);
    char predicate[SCHWAY__DAT_CONTEXT_MAX + 1u] = "";
    char profile[SCHWAY__DAT_CONTEXT_MAX + 1u] = "";

    if (dat_path == NULL || dat_path[0] == '\0' || result == NULL ||
        (evidence_path != NULL && evidence_path[0] == '\0') ||
        (private_output_path != NULL && private_output_path[0] == '\0') ||
        (private_output_path != NULL &&
         (strcmp(dat_path, private_output_path) == 0 ||
          (evidence_path != NULL && strcmp(evidence_path, private_output_path) == 0)))) {
        return SCHWAY__DAT_INVENTORY_INVALID;
    }
    int read_status = schway__dat_read_file(dat_path, SCHWAY__DAT_MAX_BYTES,
                                            &dat_bytes, &dat_length);
    if (read_status == -1) return SCHWAY__DAT_INVENTORY_IO;
    if (read_status == -2) return SCHWAY__DAT_INVENTORY_LIMIT;
    if (read_status == -4) return SCHWAY__DAT_INVENTORY_NO_MEMORY;
    if (read_status != 0) return SCHWAY__DAT_INVENTORY_INVALID;
    parser.bytes = dat_bytes;
    parser.length = dat_length;
    int parsed = schway__dat_parse(&parser);
    if (parsed == -1) {
        status = SCHWAY__DAT_INVENTORY_NO_MEMORY;
        goto done;
    }
    if (parsed == -2) {
        status = SCHWAY__DAT_INVENTORY_LIMIT;
        goto done;
    }
    if (parsed == 0) {
        status = SCHWAY__DAT_INVENTORY_INVALID;
        goto done;
    }
    schway__dat_hash_hex(dat_bytes, dat_length, completed.source_sha256);
    completed.entries = (uint64_t)parser.entry_count;
    if (evidence_path != NULL) {
        status = schway__dat_apply_evidence(evidence_path, completed.source_sha256, &parser,
                                            &evidence_bytes, predicate, profile);
        if (status != SCHWAY__DAT_INVENTORY_OK) goto done;
    }
    completed.duplicate_serial_entries =
        schway__dat_count_duplicates(&parser, &completed.missing_serial);
    if (completed.duplicate_serial_entries == UINT64_MAX) {
        status = SCHWAY__DAT_INVENTORY_NO_MEMORY;
        goto done;
    }
    for (size_t i = 0u; i < parser.entry_count; i++) {
        if (parser.entries[i].status == 'p') completed.pass++;
        else if (parser.entries[i].status == 'f') completed.fail++;
        else completed.unknown++;
    }
    if (completed.pass + completed.fail + completed.unknown != completed.entries) {
        status = SCHWAY__DAT_INVENTORY_INVALID;
        goto done;
    }
    if (private_output_path != NULL) {
        status = schway__dat_write_private(private_output_path, &parser, evidence_bytes,
                                           predicate, profile);
        if (status != SCHWAY__DAT_INVENTORY_OK) goto done;
    }
    *result = completed;
    status = SCHWAY__DAT_INVENTORY_OK;
done:
    free(evidence_bytes);
    free(parser.entries);
    free(dat_bytes);
    return status;
}

const char *schway__dat_inventory_status_string(schway__dat_inventory_status status)
{
    switch (status) {
    case SCHWAY__DAT_INVENTORY_OK: return "ok";
    case SCHWAY__DAT_INVENTORY_IO: return "cannot read inventory input";
    case SCHWAY__DAT_INVENTORY_INVALID: return "malformed inventory input";
    case SCHWAY__DAT_INVENTORY_LIMIT: return "inventory input exceeds its bound";
    case SCHWAY__DAT_INVENTORY_NO_MEMORY: return "not enough memory for inventory";
    case SCHWAY__DAT_INVENTORY_EVIDENCE: return "invalid or ambiguous inventory evidence";
    case SCHWAY__DAT_INVENTORY_OUTPUT: return "unsafe or unavailable private output path";
    }
    return "invalid inventory state";
}
