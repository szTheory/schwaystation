#include "inputs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUTS_FILE_MAX_BYTES (64u * 1024u * 1024u)
#define INPUTS_RECORDS_MAX 1000000u

static void set_error(schway__inputs_error *error, uint32_t line, const char *reason)
{
    if (error != NULL) {
        error->line = line;
        error->reason = reason;
    }
}

static int decimal(const char *text, uint32_t maximum, uint32_t *out)
{
    uint32_t value = 0u;
    if (text == NULL || *text == '\0') {
        return 0;
    }
    for (const char *p = text; *p != '\0'; p++) {
        if (*p < '0' || *p > '9') {
            return 0;
        }
        uint32_t digit = (uint32_t)(*p - '0');
        if (digit > maximum || value > (maximum - digit) / 10u) {
            return 0;
        }
        value = value * 10u + digit;
    }
    *out = value;
    return 1;
}

static size_t split(char *text, char separator, char **fields, size_t maximum)
{
    size_t count = 1u;
    fields[0] = text;
    for (char *p = text; *p != '\0'; p++) {
        if (*p == separator) {
            if (count == maximum) {
                return maximum + 1u;
            }
            *p = '\0';
            fields[count++] = p + 1;
        }
    }
    return count;
}

static int buttons(char *text, uint16_t *mask, uint8_t *analog, const char **reason)
{
    *mask = 0u;
    *analog = 0u;
    if (strcmp(text, ".") == 0) {
        return 1;
    }
    /* Store active-high controller bits; the core inverts them for the serial reply.
       Source: .planning/preparation/PSX-PERIPHERALS-AND-SAVES.md, section two point one. */
    for (const char *p = text; *p != '\0'; p++) {
        uint16_t bit = 0u;
        switch (*p) {
        case 's': bit = 1u << 0; break;
        case '<': bit = 1u << 1; break;
        case '>': bit = 1u << 2; break;
        case 'S': bit = 1u << 3; break;
        case 'U': bit = 1u << 4; break;
        case 'R': bit = 1u << 5; break;
        case 'D': bit = 1u << 6; break;
        case 'L': bit = 1u << 7; break;
        case '3': bit = 1u << 8; break;
        case '4': bit = 1u << 9; break;
        case '1': bit = 1u << 10; break;
        case '2': bit = 1u << 11; break;
        case 'T': bit = 1u << 12; break;
        case 'O': bit = 1u << 13; break;
        case 'X': bit = 1u << 14; break;
        case 'Q': bit = 1u << 15; break;
        case 'M':
            if (*analog != 0u) {
                *reason = "repeated M analog-button marker";
                return 0;
            }
            *analog = 1u;
            continue;
        default:
            *reason = "unknown button symbol";
            return 0;
        }
        if ((*mask & bit) != 0u) {
            *reason = "repeated button symbol";
            return 0;
        }
        *mask = (uint16_t)(*mask | bit);
    }
    if (*text == '\0') {
        *reason = "button field is empty; use . for no buttons";
        return 0;
    }
    return 1;
}

static int axes(char *text, uint8_t out[4], const char **reason)
{
    char *fields[4];
    uint32_t values[4];
    if (split(text, ',', fields, 4u) != 4u) {
        *reason = "axes must be lx,ly,rx,ry";
        return 0;
    }
    for (size_t i = 0u; i < 4u; i++) {
        if (!decimal(fields[i], UINT8_MAX, &values[i])) {
            *reason = "axis values must be decimal numbers from 0 to 255";
            return 0;
        }
    }
    out[0] = (uint8_t)values[2];
    out[1] = (uint8_t)values[3];
    out[2] = (uint8_t)values[0];
    out[3] = (uint8_t)values[1];
    return 1;
}

static int port(char *text, const char *prefix, uint32_t index, schway_input *input,
                const char **reason)
{
    size_t prefix_size = strlen(prefix);
    if (strncmp(text, prefix, prefix_size) != 0) {
        *reason = "port fields must be p1 then p2";
        return 0;
    }
    char *value = text + prefix_size;
    if (strcmp(value, ".") == 0) {
        input->device[index] = SCHWAY_INPUT_DEVICE_NONE;
        input->buttons[index] = 0u;
        input->analog_button[index] = 0u;
        for (size_t i = 0u; i < 4u; i++) {
            input->axes[index][i] = 128u;
        }
        return 1;
    }
    char *parts[3];
    if (split(value, ':', parts, 3u) != 3u) {
        *reason = "a connected port must be device:buttons:lx,ly,rx,ry";
        return 0;
    }
    if (strcmp(parts[0], "digital") == 0) {
        input->device[index] = SCHWAY_INPUT_DEVICE_DIGITAL;
    } else if (strcmp(parts[0], "dualshock") == 0) {
        input->device[index] = SCHWAY_INPUT_DEVICE_DUALSHOCK;
    } else {
        *reason = "device must be digital, dualshock, or . for off";
        return 0;
    }
    if (!buttons(parts[1], &input->buttons[index], &input->analog_button[index], reason) ||
        !axes(parts[2], input->axes[index], reason)) {
        return 0;
    }
    if (input->analog_button[index] != 0u &&
        input->device[index] != SCHWAY_INPUT_DEVICE_DUALSHOCK) {
        *reason = "M analog-button marker requires a dualshock device";
        return 0;
    }
    return 1;
}

static int line(char *text, uint32_t number, schway__input_event *event, int *present,
                const char **reason)
{
    char *comment = strchr(text, '#');
    if (comment != NULL) {
        *comment = '\0';
    }
    char *fields[4];
    size_t count = 0u;
    char *p = text;
    while (*p != '\0') {
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == '\0') {
            break;
        }
        if (count == 4u) {
            *reason = "each record needs a frame, p1, and p2";
            return 0;
        }
        fields[count++] = p;
        while (*p != '\0' && *p != ' ' && *p != '\t') {
            p++;
        }
        if (*p != '\0') {
            *p++ = '\0';
        }
    }
    if (count == 0u) {
        *present = 0;
        return 1;
    }
    if (count != 3u) {
        *reason = "each record needs a frame, p1, and p2";
        return 0;
    }
    uint32_t frame = 0u;
    if (!decimal(fields[0], SCHWAY__INPUTS_MAX_FRAME, &frame) || frame == 0u) {
        *reason = "frame must be a decimal number from 1 to 1000000";
        return 0;
    }
    memset(event, 0, sizeof *event);
    event->frame = frame;
    event->line = number;
    event->input.size = (uint32_t)sizeof event->input;
    for (uint32_t index = 0u; index < 2u; index++) {
        for (size_t axis = 0u; axis < 4u; axis++) {
            event->input.axes[index][axis] = 128u;
        }
    }
    if (!port(fields[1], "p1=", 0u, &event->input, reason) ||
        !port(fields[2], "p2=", 1u, &event->input, reason)) {
        return 0;
    }
    *present = 1;
    return 1;
}

static int same_input(const schway_input *a, const schway_input *b)
{
    if (a->size != b->size || a->reserved0 != b->reserved0) {
        return 0;
    }
    for (uint32_t i = 0u; i < 2u; i++) {
        if (a->device[i] != b->device[i] || a->buttons[i] != b->buttons[i] ||
            a->analog_button[i] != b->analog_button[i] ||
            memcmp(a->axes[i], b->axes[i], sizeof a->axes[i]) != 0) {
            return 0;
        }
    }
    return 1;
}

static int event_order(const void *left, const void *right)
{
    const schway__input_event *a = (const schway__input_event *)left;
    const schway__input_event *b = (const schway__input_event *)right;
    return a->frame < b->frame ? -1 : a->frame > b->frame ? 1 : 0;
}

static int ascii(const char *text, size_t size, uint32_t *line_number, const char **reason)
{
    *line_number = 1u;
    for (size_t i = 0u; i < size; i++) {
        unsigned char byte = (unsigned char)text[i];
        if (byte == (unsigned char)'\n') {
            (*line_number)++;
        } else if (byte == (unsigned char)'\r') {
            if (i + 1u >= size || text[i + 1u] != '\n') {
                *reason = "only LF and CRLF line endings are supported";
                return 0;
            }
        } else if (byte > 0x7Eu || (byte < 0x20u && byte != (unsigned char)'\t')) {
            *reason = "input must contain printable ASCII, spaces, tabs, and line endings";
            return 0;
        }
    }
    *line_number = 1u;
    return 1;
}

schway__inputs_status schway__inputs_load(const char *path, schway__input_movie *movie,
                                          schway__inputs_error *error)
{
    if (path == NULL || movie == NULL) {
        set_error(error, 0u, "invalid parser argument");
        return SCHWAY__INPUTS_INVALID;
    }
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        set_error(error, 0u, "cannot open input file");
        return SCHWAY__INPUTS_IO;
    }
    schway__inputs_status status = SCHWAY__INPUTS_OK;
    if (fseek(file, 0L, SEEK_END) != 0) {
        status = SCHWAY__INPUTS_IO;
        set_error(error, 0u, "cannot size input file");
    }
    long length = status == SCHWAY__INPUTS_OK ? ftell(file) : -1L;
    if (status == SCHWAY__INPUTS_OK && length < 0L) {
        status = SCHWAY__INPUTS_IO;
        set_error(error, 0u, "cannot size input file");
    }
    if (status == SCHWAY__INPUTS_OK && (uint64_t)length > INPUTS_FILE_MAX_BYTES) {
        status = SCHWAY__INPUTS_INVALID;
        set_error(error, 0u, "input file exceeds 64 MiB");
    }
    if (status == SCHWAY__INPUTS_OK && fseek(file, 0L, SEEK_SET) != 0) {
        status = SCHWAY__INPUTS_IO;
        set_error(error, 0u, "cannot read input file");
    }
    size_t size = status == SCHWAY__INPUTS_OK ? (size_t)length : 0u;
    char *text = NULL;
    if (status == SCHWAY__INPUTS_OK) {
        text = (char *)malloc(size + 1u);
        if (text == NULL) {
            status = SCHWAY__INPUTS_MEMORY;
            set_error(error, 0u, "out of memory");
        }
    }
    if (status == SCHWAY__INPUTS_OK) {
        size_t read_size = fread(text, 1u, size, file);
        int extra = fgetc(file);
        if (read_size != size || ferror(file)) {
            status = SCHWAY__INPUTS_IO;
            set_error(error, 0u, "cannot read input file");
        } else if (extra != EOF) {
            status = SCHWAY__INPUTS_INVALID;
            set_error(error, 0u, "input file changed while being read");
        }
    }
    if (fclose(file) != 0 && status == SCHWAY__INPUTS_OK) {
        status = SCHWAY__INPUTS_IO;
        set_error(error, 0u, "cannot finish reading input file");
    }
    if (status != SCHWAY__INPUTS_OK) {
        free(text);
        return status;
    }
    text[size] = '\0';
    uint32_t bad_line = 1u;
    const char *reason = NULL;
    if (!ascii(text, size, &bad_line, &reason)) {
        set_error(error, bad_line, reason);
        free(text);
        return SCHWAY__INPUTS_INVALID;
    }

    schway__input_event *events = NULL;
    size_t count = 0u;
    size_t capacity = 0u;
    size_t offset = 0u;
    uint32_t line_number = 1u;
    while (offset < size) {
        size_t end = offset;
        while (end < size && text[end] != '\n') {
            end++;
        }
        size_t content_end = end;
        if (content_end > offset && text[content_end - 1u] == '\r') {
            content_end--;
        }
        text[content_end] = '\0';
        schway__input_event event;
        int present = 0;
        if (!line(text + offset, line_number, &event, &present, &reason)) {
            set_error(error, line_number, reason);
            free(events);
            free(text);
            return SCHWAY__INPUTS_INVALID;
        }
        if (present != 0) {
            if (count == INPUTS_RECORDS_MAX) {
                set_error(error, line_number, "input movie exceeds 1000000 records");
                free(events);
                free(text);
                return SCHWAY__INPUTS_INVALID;
            }
            if (count == capacity) {
                size_t next = capacity == 0u ? 16u : capacity * 2u;
                if (next > INPUTS_RECORDS_MAX) {
                    next = INPUTS_RECORDS_MAX;
                }
                if (next > SIZE_MAX / sizeof *events) {
                    set_error(error, line_number, "input movie is too large");
                    free(events);
                    free(text);
                    return SCHWAY__INPUTS_INVALID;
                }
                schway__input_event *grown =
                    (schway__input_event *)realloc(events, next * sizeof *events);
                if (grown == NULL) {
                    set_error(error, line_number, "out of memory");
                    free(events);
                    free(text);
                    return SCHWAY__INPUTS_MEMORY;
                }
                events = grown;
                capacity = next;
            }
            events[count++] = event;
        }
        if (end == size) {
            break;
        }
        offset = end + 1u;
        line_number++;
    }
    free(text);
    if (count > 1u) {
        qsort(events, count, sizeof *events, event_order);
    }
    size_t unique = 0u;
    for (size_t i = 0u; i < count; i++) {
        if (unique != 0u && events[unique - 1u].frame == events[i].frame) {
            if (!same_input(&events[unique - 1u].input, &events[i].input)) {
                set_error(error, events[i].line, "conflicting records for the same frame");
                free(events);
                return SCHWAY__INPUTS_INVALID;
            }
            continue;
        }
        events[unique++] = events[i];
    }
    movie->events = events;
    movie->count = unique;
    set_error(error, 0u, NULL);
    return SCHWAY__INPUTS_OK;
}

schway_status schway__inputs_apply_frame(schway *inst, const schway__input_movie *movie,
                                         size_t *cursor, schway_input *snapshot, uint32_t frame)
{
    int changed = 0;
    while (*cursor < movie->count && movie->events[*cursor].frame <= frame) {
        *snapshot = movie->events[*cursor].input;
        (*cursor)++;
        changed = 1;
    }
    return changed != 0 ? schway_set_input(inst, snapshot) : SCHWAY_STATUS_OK;
}

void schway__inputs_release(schway__input_movie *movie)
{
    if (movie != NULL) {
        free(movie->events);
        movie->events = NULL;
        movie->count = 0u;
    }
}
