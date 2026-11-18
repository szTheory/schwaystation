/* Bounded host-side sector provider; file access never enters the core. */
#if !defined(_WIN32) && !defined(_XOPEN_SOURCE)
#define _XOPEN_SOURCE 700
#endif
#include "disc_image.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#define SCHWAY__DISC_IMAGE_MAX_BYTES UINT64_C(1073741824)
#define SCHWAY__DISC_IMAGE_ISO_SECTOR_BYTES 2048u
#define SCHWAY__DISC_IMAGE_RAW_SECTOR_BYTES 2352u
#define SCHWAY__DISC_IMAGE_MIN_DATA_SECTORS 16u
#define SCHWAY__DISC_IMAGE_CUE_MAX_BYTES 65536u
#define SCHWAY__DISC_IMAGE_CUE_NAME_MAX 255u
#define SCHWAY__DISC_IMAGE_TRACK_LIMIT 99u
#define SCHWAY__DISC_IMAGE_INDEX_LIMIT 99u
#define SCHWAY__DISC_IMAGE_PATH_CAP 4096u
#define SCHWAY__DISC_IMAGE_M3U_MAX_BYTES 65536u
#define SCHWAY__DISC_IMAGE_M3U_ENTRY_MAX SCHWAY_DISC_IMAGE_PLAYLIST_LIMIT

typedef struct schway__disc_image_source {
    char name[SCHWAY__DISC_IMAGE_CUE_NAME_MAX + 1u];
    char *canonical_path;
    FILE *file;
    uint64_t size;
    uint32_t line;
} schway__disc_image_source;

typedef struct schway__disc_image_track {
    size_t source_index;
    uint32_t type;
    uint32_t sector_bytes;
    uint32_t index00_frame;
    uint32_t index01_frame;
    uint32_t last_index_frame;
    uint32_t index_count;
    uint32_t virtual_pregap;
    uint32_t physical_pregap;
    uint32_t file_line;
    uint32_t track_line;
    uint32_t index_line;
    uint8_t last_index_number;
    uint8_t have_index00;
    uint8_t have_index01;
    uint8_t have_index;
    uint8_t have_pregap;
    uint64_t index0_file_offset;
    uint64_t index1_file_offset;
    uint64_t index0_lba;
    uint64_t index1_lba;
    uint64_t leadout;
} schway__disc_image_track;

struct schway_disc_image {
    size_t source_count;
    size_t track_count;
    size_t playlist_count;
    char *path;
    char label[SCHWAY__DISC_IMAGE_CUE_NAME_MAX + 1u];
    schway_disc_image *playlist[SCHWAY__DISC_IMAGE_M3U_ENTRY_MAX];
    schway__disc_image_source sources[SCHWAY__DISC_IMAGE_TRACK_LIMIT];
    schway__disc_image_track tracks[SCHWAY__DISC_IMAGE_TRACK_LIMIT];
    schway_disc descriptor;
    schway_toc toc;
};

static uint32_t schway__disc_image_read_sector(void *user, uint32_t lba,
                                               uint8_t sector[2352], uint8_t subcode[96]);
static uint32_t schway__disc_image_read_toc(void *user, schway_toc *out);

static unsigned char schway__disc_image_ascii_lower(unsigned char value)
{
    if (value >= (unsigned char)'A' && value <= (unsigned char)'Z') {
        return (unsigned char)(value - (unsigned char)'A' + (unsigned char)'a');
    }
    return value;
}

static int schway__disc_image_word_is(const char *left, const char *right)
{
    size_t i = 0u;
    while (left[i] != '\0' && right[i] != '\0') {
        if (schway__disc_image_ascii_lower((unsigned char)left[i]) !=
            schway__disc_image_ascii_lower((unsigned char)right[i])) return 0;
        i++;
    }
    return left[i] == '\0' && right[i] == '\0';
}

static int schway__disc_image_extension_is(const char *path, const char *extension)
{
    const char *dot = strrchr(path, '.');
    const char *slash = strrchr(path, '/');
    const char *backslash = strrchr(path, '\\');
    const char *last = slash;
    if (backslash != NULL && (last == NULL || backslash > last)) last = backslash;
    if (dot == NULL || (last != NULL && dot < last) || strlen(dot) != strlen(extension)) {
        return 0;
    }
    return schway__disc_image_word_is(dot, extension);
}

static int schway__disc_image_path_is_unsupported(const char *path)
{
    return schway__disc_image_extension_is(path, ".chd") ||
           schway__disc_image_extension_is(path, ".pbp") ||
           schway__disc_image_extension_is(path, ".ecm") ||
           schway__disc_image_extension_is(path, ".ccd");
}

static FILE *schway__disc_image_open_readonly(const char *path)
{
#if defined(_MSC_VER)
    FILE *file = NULL;
    if (fopen_s(&file, path, "rb") != 0) return NULL;
    return file;
#else
    return fopen(path, "rb");
#endif
}

static void schway__disc_image_error_set(schway_disc_image_error *error,
                                          schway_disc_image_status status,
                                          uint32_t line, const char *message)
{
    if (error == NULL) return;
    memset(error, 0, sizeof *error);
    error->size = (uint32_t)sizeof *error;
    error->status = (uint32_t)status;
    error->line = line;
    (void)snprintf(error->message, sizeof error->message, "%s", message);
}

static schway_disc_image_status schway__disc_image_line_error(
    schway_disc_image_error *error, schway_disc_image_status status,
    uint32_t line, const char *reason)
{
    char message[96];
    const char *name = status == SCHWAY_DISC_IMAGE_BAD_IMAGE ? "BAD_IMAGE" :
                       status == SCHWAY_DISC_IMAGE_LIMIT ? "LIMIT" : "BAD_CUE";
    if (line == 0u) {
        (void)snprintf(message, sizeof message, "%s: %s", name, reason);
    } else {
        (void)snprintf(message, sizeof message, "%s line %u: %s", name, line, reason);
    }
    schway__disc_image_error_set(error, status, line, message);
    return status;
}

static uint8_t schway__disc_image_ecc_forward(uint8_t value)
{
    uint16_t doubled = (uint16_t)((uint16_t)value << 1);
    if ((value & 0x80u) != 0u) doubled = (uint16_t)(doubled ^ 0x11du);
    return (uint8_t)doubled;
}

static void schway__disc_image_ecc(const uint8_t *source, uint32_t major_count,
                                    uint32_t minor_count, uint32_t major_mult,
                                    uint32_t minor_inc, uint8_t *destination)
{
    uint32_t size = major_count * minor_count;
    uint8_t backward[256];
    for (uint32_t i = 0u; i < 256u; i++) {
        uint8_t value = (uint8_t)i;
        backward[value ^ schway__disc_image_ecc_forward(value)] = value;
    }
    for (uint32_t major = 0u; major < major_count; major++) {
        uint32_t index = (major >> 1) * major_mult + (major & 1u);
        uint8_t a = 0u;
        uint8_t b = 0u;
        for (uint32_t minor = 0u; minor < minor_count; minor++) {
            uint8_t value = source[index];
            index += minor_inc;
            if (index >= size) index -= size;
            a ^= value;
            b ^= value;
            a = schway__disc_image_ecc_forward(a);
        }
        a = backward[schway__disc_image_ecc_forward(a) ^ b];
        destination[major] = a;
        destination[major + major_count] = (uint8_t)(a ^ b);
    }
}

static void schway__disc_image_mode1_header(
    uint8_t sector[SCHWAY__DISC_IMAGE_RAW_SECTOR_BYTES], uint32_t lba)
{
    uint32_t absolute = lba + 150u;
    uint32_t minute = (absolute / (60u * 75u)) % 100u;
    uint32_t second = (absolute / 75u) % 60u;
    uint32_t frame = absolute % 75u;
#define SCHWAY__BCD(value) ((uint8_t)((((value) / 10u) << 4) | ((value) % 10u)))
    sector[0] = 0u;
    memset(sector + 1u, 0xff, 10u);
    sector[11] = 0u;
    sector[12] = SCHWAY__BCD(minute);
    sector[13] = SCHWAY__BCD(second);
    sector[14] = SCHWAY__BCD(frame);
    sector[15] = 1u;
#undef SCHWAY__BCD
}

static void schway__disc_image_mode1_trailer(
    uint8_t sector[SCHWAY__DISC_IMAGE_RAW_SECTOR_BYTES])
{
    uint32_t edc = 0u;
    for (uint32_t i = 0u; i < 2064u; i++) {
        edc ^= sector[i];
        for (uint32_t bit = 0u; bit < 8u; bit++) {
            edc = (edc >> 1) ^ ((edc & 1u) != 0u ? UINT32_C(0xd8018001) : 0u);
        }
    }
    sector[2064] = (uint8_t)edc;
    sector[2065] = (uint8_t)(edc >> 8);
    sector[2066] = (uint8_t)(edc >> 16);
    sector[2067] = (uint8_t)(edc >> 24);
    memset(sector + 2068u, 0, 8u);
    schway__disc_image_ecc(sector + 12u, 86u, 24u, 2u, 86u, sector + 2076u);
    schway__disc_image_ecc(sector + 12u, 52u, 43u, 86u, 88u, sector + 2248u);
}

static void schway__disc_image_mode2_blank(
    uint8_t sector[SCHWAY__DISC_IMAGE_RAW_SECTOR_BYTES], uint32_t lba)
{
    uint32_t absolute = lba + 150u;
    uint32_t minute = (absolute / (60u * 75u)) % 100u;
    uint32_t second = (absolute / 75u) % 60u;
    uint32_t frame = absolute % 75u;
#define SCHWAY__BCD(value) ((uint8_t)((((value) / 10u) << 4) | ((value) % 10u)))
    memset(sector, 0, SCHWAY__DISC_IMAGE_RAW_SECTOR_BYTES);
    memset(sector + 1u, 0xff, 10u);
    sector[12] = SCHWAY__BCD(minute);
    sector[13] = SCHWAY__BCD(second);
    sector[14] = SCHWAY__BCD(frame);
    sector[15] = 2u;
#undef SCHWAY__BCD
}

static uint32_t schway__disc_image_digit_pair(const char *text, uint32_t *out)
{
    if (text[0] < '0' || text[0] > '9' || text[1] < '0' || text[1] > '9') return 0u;
    *out = (uint32_t)(text[0] - '0') * 10u + (uint32_t)(text[1] - '0');
    return 1u;
}

static uint32_t schway__disc_image_msf(const char *text, uint32_t *frames)
{
    uint32_t minute;
    uint32_t second;
    uint32_t frame;
    if (strlen(text) != 8u || text[2] != ':' || text[5] != ':' ||
        !schway__disc_image_digit_pair(text, &minute) ||
        !schway__disc_image_digit_pair(text + 3u, &second) ||
        !schway__disc_image_digit_pair(text + 6u, &frame) ||
        minute > 99u || second > 59u || frame > 74u) return 0u;
    *frames = (minute * 60u + second) * 75u + frame;
    return 1u;
}

static void schway__disc_image_skip_space(char **cursor)
{
    while (**cursor == ' ' || **cursor == '\t') (*cursor)++;
}

static uint32_t schway__disc_image_token(char **cursor, char *out, size_t capacity)
{
    size_t size = 0u;
    char quote = '\0';
    schway__disc_image_skip_space(cursor);
    if (**cursor == '\0') return 0u;
    if (**cursor == '"') {
        quote = '"';
        (*cursor)++;
    }
    while (**cursor != '\0' && (quote != '\0' ? **cursor != quote :
                                  **cursor != ' ' && **cursor != '\t')) {
        if (size + 1u >= capacity) return 0u;
        out[size++] = **cursor;
        (*cursor)++;
    }
    if (quote != '\0') {
        if (**cursor != quote) return 0u;
        (*cursor)++;
        if (**cursor != '\0' && **cursor != ' ' && **cursor != '\t') return 0u;
    }
    if (size == 0u) return 0u;
    out[size] = '\0';
    return 1u;
}

static uint32_t schway__disc_image_end_of_line(char *cursor)
{
    schway__disc_image_skip_space(&cursor);
    return *cursor == '\0';
}

static uint32_t schway__disc_image_safe_name(char *name)
{
    size_t size = strlen(name);
    size_t component_start = 0u;
    if (size == 0u || size > SCHWAY__DISC_IMAGE_CUE_NAME_MAX ||
        name[0] == '/' || name[0] == '\\') return 0u;
    for (size_t i = 0u; i <= size; i++) {
        unsigned char value = (unsigned char)name[i];
        if (i < size) {
            if (value == (unsigned char)'\\') name[i] = '/';
            if (value == 0u || value < 0x20u || value == 0x7fu ||
                value == (unsigned char)':') return 0u;
        }
        if (name[i] == '/' || name[i] == '\0') {
            size_t component_size = i - component_start;
            if (component_size == 2u && name[component_start] == '.' &&
                name[component_start + 1u] == '.') return 0u;
            component_start = i + 1u;
        }
    }
    return 1u;
}

static schway_disc_image_status schway__disc_image_parse_cue(
    const char *path, schway_disc_image *image, schway_disc_image_error *error)
{
    FILE *file = schway__disc_image_open_readonly(path);
    long end;
    char *bytes;
    size_t size;
    size_t offset = 0u;
    size_t active_file = SIZE_MAX;
    schway__disc_image_track *active_track = NULL;
    uint32_t line_number = 0u;
    if (file == NULL) {
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_IO, 0u,
                                     "cannot open cue sheet");
        return SCHWAY_DISC_IMAGE_IO;
    }
    if (fseek(file, 0L, SEEK_END) != 0 || (end = ftell(file)) < 0L ||
        fseek(file, 0L, SEEK_SET) != 0) {
        (void)fclose(file);
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_IO, 0u,
                                     "cannot size cue sheet");
        return SCHWAY_DISC_IMAGE_IO;
    }
    if ((uint64_t)(unsigned long)end > SCHWAY__DISC_IMAGE_CUE_MAX_BYTES) {
        (void)fclose(file);
        return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE, 0u,
                                             "cue sheet exceeds 64 KiB");
    }
    size = (size_t)end;
    if (size == 0u) {
        (void)fclose(file);
        return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE, 0u,
                                             "empty cue sheet");
    }
    bytes = (char *)malloc(size + 1u);
    if (bytes == NULL) {
        (void)fclose(file);
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_OUT_OF_MEMORY, 0u,
                                     "out of memory reading cue sheet");
        return SCHWAY_DISC_IMAGE_OUT_OF_MEMORY;
    }
    int failed = fread(bytes, 1u, size, file) != size;
    if (fclose(file) != 0) failed = 1;
    if (failed) {
        free(bytes);
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_IO, 0u,
                                     "cannot read cue sheet");
        return SCHWAY_DISC_IMAGE_IO;
    }
    bytes[size] = '\0';
    for (size_t i = 0u; i < size; i++) {
        unsigned char value = (unsigned char)bytes[i];
        if (value == 0u || (value < 0x20u && value != '\r' && value != '\n' && value != '\t') ||
            value == 0x7fu) {
            free(bytes);
            return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE, 1u,
                                                 "invalid control byte");
        }
    }
    while (offset < size) {
        size_t end_line = offset;
        char *cursor;
        char keyword[32];
        while (end_line < size && bytes[end_line] != '\n') end_line++;
        bytes[end_line] = '\0';
        if (end_line > offset && bytes[end_line - 1u] == '\r') bytes[end_line - 1u] = '\0';
        cursor = bytes + offset;
        offset = end_line + 1u;
        line_number++;
        schway__disc_image_skip_space(&cursor);
        if (*cursor == '\0') continue;
        if (!schway__disc_image_token(&cursor, keyword, sizeof keyword)) {
            free(bytes);
            return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE,
                                                 line_number, "invalid cue command");
        }
        if (schway__disc_image_word_is(keyword, "FILE")) {
            char name[SCHWAY__DISC_IMAGE_CUE_NAME_MAX + 1u];
            char file_type[16];
            if (image->source_count >= SCHWAY__DISC_IMAGE_TRACK_LIMIT ||
                (active_track != NULL && active_track->have_index01 == 0u) ||
                !schway__disc_image_token(&cursor, name, sizeof name) ||
                !schway__disc_image_token(&cursor, file_type, sizeof file_type) ||
                !schway__disc_image_word_is(file_type, "BINARY") ||
                !schway__disc_image_end_of_line(cursor)) {
                free(bytes);
                return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE,
                                                     line_number, "invalid FILE declaration");
            }
            if (!schway__disc_image_safe_name(name)) {
                free(bytes);
                return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE,
                                                     line_number, "unsafe FILE child path");
            }
            schway__disc_image_source *source = &image->sources[image->source_count];
            memcpy(source->name, name, strlen(name) + 1u);
            source->line = line_number;
            active_file = image->source_count++;
            active_track = NULL;
        } else if (schway__disc_image_word_is(keyword, "TRACK")) {
            char number_text[8];
            char mode[24];
            uint32_t number;
            if (active_file == SIZE_MAX || image->track_count >= SCHWAY__DISC_IMAGE_TRACK_LIMIT ||
                (active_track != NULL && active_track->have_index01 == 0u) ||
                !schway__disc_image_token(&cursor, number_text, sizeof number_text) ||
                strlen(number_text) != 2u ||
                !schway__disc_image_digit_pair(number_text, &number) ||
                number != (uint32_t)image->track_count + 1u ||
                !schway__disc_image_token(&cursor, mode, sizeof mode) ||
                !schway__disc_image_end_of_line(cursor)) {
                free(bytes);
                return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE,
                                                     line_number, "invalid or excessive TRACK declaration");
            }
            active_track = &image->tracks[image->track_count++];
            active_track->source_index = active_file;
            active_track->track_line = line_number;
            active_track->file_line = image->sources[active_file].line;
            if (schway__disc_image_word_is(mode, "AUDIO")) {
                active_track->type = SCHWAY_DISC_TRACK_AUDIO;
                active_track->sector_bytes = SCHWAY__DISC_IMAGE_RAW_SECTOR_BYTES;
            } else if (schway__disc_image_word_is(mode, "MODE1/2048")) {
                active_track->type = SCHWAY_DISC_TRACK_MODE1;
                active_track->sector_bytes = SCHWAY__DISC_IMAGE_ISO_SECTOR_BYTES;
            } else if (schway__disc_image_word_is(mode, "MODE2/2352")) {
                active_track->type = SCHWAY_DISC_TRACK_MODE2;
                active_track->sector_bytes = SCHWAY__DISC_IMAGE_RAW_SECTOR_BYTES;
            } else {
                free(bytes);
                return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE,
                                                     line_number, "unsupported cue track mode");
            }
        } else if (schway__disc_image_word_is(keyword, "INDEX")) {
            char number_text[8];
            char time_text[16];
            uint32_t number;
            uint32_t frame;
            if (active_track == NULL || active_track->index_count >= SCHWAY__DISC_IMAGE_INDEX_LIMIT ||
                !schway__disc_image_token(&cursor, number_text, sizeof number_text) ||
                strlen(number_text) != 2u ||
                !schway__disc_image_digit_pair(number_text, &number) || number > 99u ||
                !schway__disc_image_token(&cursor, time_text, sizeof time_text) ||
                !schway__disc_image_msf(time_text, &frame) ||
                !schway__disc_image_end_of_line(cursor) ||
                (active_track->have_index != 0u &&
                 (number <= active_track->last_index_number || frame < active_track->last_index_frame)) ||
                (active_track->have_index == 0u && number > 1u)) {
                free(bytes);
                return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE,
                                                     line_number, "invalid, backward or excessive INDEX");
            }
            if (number == 0u) {
                active_track->index00_frame = frame;
                active_track->have_index00 = 1u;
            } else if (number == 1u) {
                active_track->index01_frame = frame;
                active_track->have_index01 = 1u;
                active_track->index_line = line_number;
            }
            active_track->last_index_number = (uint8_t)number;
            active_track->last_index_frame = frame;
            active_track->have_index = 1u;
            active_track->index_count++;
        } else if (schway__disc_image_word_is(keyword, "PREGAP")) {
            char time_text[16];
            uint32_t frame;
            if (active_track == NULL || active_track->have_pregap != 0u ||
                active_track->have_index01 != 0u ||
                !schway__disc_image_token(&cursor, time_text, sizeof time_text) ||
                !schway__disc_image_msf(time_text, &frame) ||
                !schway__disc_image_end_of_line(cursor)) {
                free(bytes);
                return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE,
                                                     line_number, "invalid PREGAP");
            }
            active_track->virtual_pregap = frame;
            active_track->have_pregap = 1u;
        } else if (schway__disc_image_word_is(keyword, "REM") ||
                   schway__disc_image_word_is(keyword, "TITLE") ||
                   schway__disc_image_word_is(keyword, "PERFORMER") ||
                   schway__disc_image_word_is(keyword, "CATALOG") ||
                   schway__disc_image_word_is(keyword, "ISRC") ||
                   schway__disc_image_word_is(keyword, "FLAGS") ||
                   schway__disc_image_word_is(keyword, "CDTEXTFILE")) {
            continue;
        } else {
            free(bytes);
            return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE,
                                                 line_number, "unsupported cue command");
        }
    }
    free(bytes);
    if (image->source_count == 0u || image->track_count == 0u) {
        return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE, line_number,
                                             "cue sheet has no FILE or TRACK");
    }
    for (size_t i = 0u; i < image->track_count; i++) {
        schway__disc_image_track *track = &image->tracks[i];
        if (track->have_index01 == 0u ||
            (track->have_index00 != 0u && track->index00_frame > track->index01_frame)) {
            return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE,
                                                 track->track_line, "TRACK is missing a valid INDEX 01");
        }
    }
    return SCHWAY_DISC_IMAGE_OK;
}

static int schway__disc_image_canonical_path(const char *path, char out[SCHWAY__DISC_IMAGE_PATH_CAP])
{
#ifdef _WIN32
    HANDLE handle = CreateFileA(path, FILE_READ_ATTRIBUTES,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    DWORD length;
    if (handle == INVALID_HANDLE_VALUE) return 0;
    length = GetFinalPathNameByHandleA(handle, out, SCHWAY__DISC_IMAGE_PATH_CAP,
                                       FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    (void)CloseHandle(handle);
    return length != 0u && length < SCHWAY__DISC_IMAGE_PATH_CAP;
#elif defined(__wasi__)
    /* WASI filesystem calls resolve paths relative to preopened directories.
       realpath's readlink calls therefore need a path relative to the guest CWD. */
    char cwd[SCHWAY__DISC_IMAGE_PATH_CAP];
    if (getcwd(cwd, sizeof cwd) == NULL) return 0;
    size_t cwd_size = strlen(cwd);
    if (path[0] == '/') {
        if (cwd_size == 1u && cwd[0] == '/') {
            path++;
        } else {
            if (strncmp(path, cwd, cwd_size) != 0 ||
                (path[cwd_size] != '/' && path[cwd_size] != '\0')) return 0;
            path += cwd_size;
        }
        while (*path == '/') path++;
        if (*path == '\0') path = ".";
    }
    char *resolved = realpath(path, NULL);
    if (resolved == NULL) return 0;
    /* realpath succeeds only through the filesystem capabilities Wasmtime preopened. */
    if ((cwd_size == 1u && cwd[0] == '/') ? resolved[0] != '/' :
        (strncmp(resolved, cwd, cwd_size) != 0 ||
         (resolved[cwd_size] != '/' && resolved[cwd_size] != '\0'))) {
        free(resolved);
        return 0;
    }
    size_t length = strlen(resolved);
    int fits = length < SCHWAY__DISC_IMAGE_PATH_CAP;
    if (fits) memcpy(out, resolved, length + 1u);
    free(resolved);
    return fits;
#else
    char *resolved = realpath(path, NULL);
    if (resolved == NULL) return 0;
    size_t length = strlen(resolved);
    int fits = length < SCHWAY__DISC_IMAGE_PATH_CAP;
    if (fits) memcpy(out, resolved, length + 1u);
    free(resolved);
    return fits;
#endif
}

static int schway__disc_image_store_path(schway_disc_image *image, const char *path)
{
    size_t size = strlen(path);
    size_t label_start = 0u;
    size_t label_size;
    if (size == 0u || size >= SCHWAY__DISC_IMAGE_PATH_CAP) return 0;
    image->path = (char *)malloc(size + 1u);
    if (image->path == NULL) return 0;
    memcpy(image->path, path, size + 1u);
    for (size_t i = 0u; i < size; i++) {
        if (path[i] == '/' || path[i] == '\\') label_start = i + 1u;
    }
    label_size = size - label_start;
    if (label_size > SCHWAY__DISC_IMAGE_CUE_NAME_MAX) {
        label_size = SCHWAY__DISC_IMAGE_CUE_NAME_MAX;
    }
    memcpy(image->label, path + label_start, label_size);
    image->label[label_size] = '\0';
    return 1;
}

static const char *schway__disc_image_unsupported_reason(const char *path)
{
    if (schway__disc_image_extension_is(path, ".chd")) return "unsupported: CHD (planned v2.0)";
    if (schway__disc_image_extension_is(path, ".pbp")) return "unsupported: PBP (planned v2.0)";
    if (schway__disc_image_extension_is(path, ".ecm")) return "unsupported: ECM (planned v2.0)";
    if (schway__disc_image_extension_is(path, ".ccd")) return "unsupported: CCD (planned v2.0)";
    return "unsupported disc image";
}

static int schway__disc_image_cue_directory(const char *cue_path,
                                             char out[SCHWAY__DISC_IMAGE_PATH_CAP])
{
    char directory[SCHWAY__DISC_IMAGE_PATH_CAP];
    const char *slash = strrchr(cue_path, '/');
#ifdef _WIN32
    const char *backslash = strrchr(cue_path, '\\');
    if (backslash != NULL && (slash == NULL || backslash > slash)) slash = backslash;
    const char separator = '\\';
#else
    const char separator = '/';
#endif
    size_t size;
    if (slash == NULL) {
        memcpy(directory, ".", 2u);
    } else {
        size = (size_t)(slash - cue_path + 1);
        if (size >= sizeof directory) return 0;
        memcpy(directory, cue_path, size);
        directory[size] = '\0';
    }
    if (!schway__disc_image_canonical_path(directory, out)) return 0;
    size = strlen(out);
    if (size == 0u || (out[size - 1u] != '/' && out[size - 1u] != '\\')) {
        if (size + 1u >= SCHWAY__DISC_IMAGE_PATH_CAP) return 0;
        out[size++] = separator;
        out[size] = '\0';
    }
    return 1;
}

static int schway__disc_image_path_contained(const char *root, const char *candidate)
{
    size_t root_size = strlen(root);
    if (strlen(candidate) < root_size) return 0;
    if (root_size == 1u && root[0] == '/') {
        if (candidate[0] != '/' || candidate[1] == '\0') return 0;
#ifdef __wasi__
        /* The build tree is guest /; /source is a separate preopen. */
        size_t source_root_size = sizeof "/source" - 1u;
        if (strncmp(candidate, "/source", source_root_size) == 0 &&
            (candidate[source_root_size] == '/' || candidate[source_root_size] == '\0')) {
            return 0;
        }
#endif
        return 1;
    }
    for (size_t i = 0u; i < root_size; i++) {
        unsigned char left = (unsigned char)root[i];
        unsigned char right = (unsigned char)candidate[i];
#ifdef _WIN32
        if (left == (unsigned char)'/') left = (unsigned char)'\\';
        if (right == (unsigned char)'/') right = (unsigned char)'\\';
        left = schway__disc_image_ascii_lower(left);
        right = schway__disc_image_ascii_lower(right);
#endif
        if (left != right) return 0;
    }
    return 1;
}

static int schway__disc_image_child_path(const char *root, const char *name,
                                          char out[SCHWAY__DISC_IMAGE_PATH_CAP])
{
    size_t root_size = strlen(root);
    size_t name_size = strlen(name);
    if (root_size > SCHWAY__DISC_IMAGE_PATH_CAP - name_size - 1u) return 0;
    memcpy(out, root, root_size);
    memcpy(out + root_size, name, name_size + 1u);
    return 1;
}

static int schway__disc_image_file_size(FILE *file, uint64_t *size)
{
#ifdef _WIN32
    __int64 end;
    if (_fseeki64(file, 0, SEEK_END) != 0 || (end = _ftelli64(file)) < 0 ||
        _fseeki64(file, 0, SEEK_SET) != 0) return 0;
    *size = (uint64_t)end;
#else
    off_t end;
    if (fseeko(file, 0, SEEK_END) != 0 || (end = ftello(file)) < 0 ||
        fseeko(file, 0, SEEK_SET) != 0) return 0;
    *size = (uint64_t)end;
#endif
    return 1;
}

static int schway__disc_image_same_path(const char *left, const char *right)
{
    size_t i = 0u;
    while (left[i] != '\0' && right[i] != '\0') {
        unsigned char a = (unsigned char)left[i];
        unsigned char b = (unsigned char)right[i];
#ifdef _WIN32
        if (a == (unsigned char)'/') a = (unsigned char)'\\';
        if (b == (unsigned char)'/') b = (unsigned char)'\\';
        a = schway__disc_image_ascii_lower(a);
        b = schway__disc_image_ascii_lower(b);
#endif
        if (a != b) return 0;
        i++;
    }
    return left[i] == '\0' && right[i] == '\0';
}

static schway_disc_image_status schway__disc_image_open_cue_children(
    const char *cue_path, schway_disc_image *image, schway_disc_image_error *error)
{
    char root[SCHWAY__DISC_IMAGE_PATH_CAP];
    uint64_t total = 0u;
    if (!schway__disc_image_cue_directory(cue_path, root)) {
        return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE, 1u,
                                             "cannot resolve cue directory");
    }
    for (size_t i = 0u; i < image->source_count; i++) {
        schway__disc_image_source *source = &image->sources[i];
        char candidate[SCHWAY__DISC_IMAGE_PATH_CAP];
        char canonical[SCHWAY__DISC_IMAGE_PATH_CAP];
        if (schway__disc_image_path_is_unsupported(source->name)) {
            schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_UNSUPPORTED,
                                         source->line,
                                         schway__disc_image_unsupported_reason(source->name));
            return SCHWAY_DISC_IMAGE_UNSUPPORTED;
        }
        if (!schway__disc_image_child_path(root, source->name, candidate) ||
            !schway__disc_image_canonical_path(candidate, canonical) ||
            !schway__disc_image_path_contained(root, canonical)) {
            return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE,
                                                 source->line, "missing or escaping FILE child");
        }
        source->canonical_path = (char *)malloc(strlen(canonical) + 1u);
        if (source->canonical_path == NULL) {
            schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_OUT_OF_MEMORY, source->line,
                                         "out of memory resolving cue child");
            return SCHWAY_DISC_IMAGE_OUT_OF_MEMORY;
        }
        memcpy(source->canonical_path, canonical, strlen(canonical) + 1u);
        source->file = schway__disc_image_open_readonly(source->canonical_path);
        if (source->file == NULL || !schway__disc_image_file_size(source->file, &source->size)) {
            return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE,
                                                 source->line, "cannot size FILE child");
        }
        int duplicate = 0;
        for (size_t prior = 0u; prior < i; prior++) {
            if (schway__disc_image_same_path(image->sources[prior].canonical_path,
                                              source->canonical_path)) {
                duplicate = 1;
                break;
            }
        }
        if (!duplicate) {
            if (source->size > SCHWAY__DISC_IMAGE_MAX_BYTES - total) {
                return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                                     source->line, "total cue image exceeds 1 GiB");
            }
            total += source->size;
        }
    }
    return SCHWAY_DISC_IMAGE_OK;
}

static int schway__disc_image_add_u64(uint64_t left, uint64_t right, uint64_t *out)
{
    if (right > UINT64_MAX - left) return 0;
    *out = left + right;
    return 1;
}

static int schway__disc_image_mul_u64(uint64_t left, uint64_t right, uint64_t *out)
{
    if (left != 0u && right > UINT64_MAX / left) return 0;
    *out = left * right;
    return 1;
}

static schway_disc_image_status schway__disc_image_build_cue_toc(
    schway_disc_image *image, schway_disc_image_error *error)
{
    uint64_t data_sectors = 0u;
    for (size_t i = 0u; i < image->track_count; i++) {
        schway__disc_image_track *track = &image->tracks[i];
        schway__disc_image_source *source = &image->sources[track->source_index];
        uint32_t start_frame = track->have_index00 != 0u ?
                                   track->index00_frame : track->index01_frame;
        uint64_t physical_bytes;
        if (track->have_index00 != 0u) {
            track->physical_pregap = track->index01_frame - track->index00_frame;
        }
        if (i == 0u) {
            track->index0_lba = 0u;
            if (track->have_index00 != 0u) {
                if (!schway__disc_image_mul_u64(start_frame, track->sector_bytes,
                                                 &track->index0_file_offset)) {
                    return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE,
                                                         track->index_line, "INDEX offset overflow");
                }
            } else if (!schway__disc_image_mul_u64(track->index01_frame, track->sector_bytes,
                                                   &track->index0_file_offset)) {
                return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE,
                                                     track->index_line, "INDEX offset overflow");
            }
        } else {
            schway__disc_image_track *previous = &image->tracks[i - 1u];
            if (track->source_index == previous->source_index) {
                uint64_t delta_bytes;
                uint32_t delta_frames;
                if (start_frame < previous->index01_frame) {
                    return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE,
                                                         track->index_line,
                                                         "overlapping or backward track indexes");
                }
                delta_frames = start_frame - previous->index01_frame;
                if (!schway__disc_image_mul_u64(delta_frames, previous->sector_bytes,
                                                 &delta_bytes) ||
                    !schway__disc_image_add_u64(previous->index1_file_offset, delta_bytes,
                                                 &track->index0_file_offset) ||
                    !schway__disc_image_add_u64(previous->index1_lba, delta_frames,
                                                 &track->index0_lba)) {
                    return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE,
                                                         track->index_line, "track offset overflow");
                }
            } else {
                track->index0_lba = previous->leadout;
                if (!schway__disc_image_mul_u64(start_frame, track->sector_bytes,
                                                 &track->index0_file_offset)) {
                    return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE,
                                                         track->index_line, "INDEX offset overflow");
                }
            }
        }
        if (!schway__disc_image_mul_u64(track->physical_pregap, track->sector_bytes,
                                         &physical_bytes) ||
            !schway__disc_image_add_u64(track->index0_file_offset, physical_bytes,
                                         &track->index1_file_offset) ||
            track->index0_file_offset > source->size || track->index1_file_offset > source->size) {
            return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                                 track->file_line, "track data exceeds FILE child");
        }
        uint64_t index1_lba;
        if (!schway__disc_image_add_u64(track->index0_lba, track->physical_pregap,
                                         &index1_lba) ||
            !schway__disc_image_add_u64(index1_lba, track->virtual_pregap,
                                         &track->index1_lba) ||
            track->index1_lba > UINT32_MAX) {
            return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE,
                                                 track->index_line, "TOC index arithmetic overflow");
        }
        uint64_t data_bytes;
        uint64_t sectors;
        if (i + 1u < image->track_count &&
            image->tracks[i + 1u].source_index == track->source_index) {
            schway__disc_image_track *next = &image->tracks[i + 1u];
            uint32_t next_start = next->have_index00 != 0u ?
                                      next->index00_frame : next->index01_frame;
            if (next_start < track->index01_frame ||
                !schway__disc_image_mul_u64(next_start - track->index01_frame,
                                             track->sector_bytes, &data_bytes) ||
                !schway__disc_image_add_u64(track->index1_file_offset, data_bytes, &sectors) ||
                sectors > source->size || data_bytes % track->sector_bytes != 0u) {
                return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE,
                                                     next->index_line,
                                                     "overlapping or backward track file offsets");
            }
            sectors = data_bytes / track->sector_bytes;
        } else {
            if (track->index1_file_offset > source->size) {
                return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                                     track->file_line, "track offset exceeds FILE child");
            }
            data_bytes = source->size - track->index1_file_offset;
            if (data_bytes % track->sector_bytes != 0u) {
                return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                                     track->file_line,
                                                     "FILE child has a partial declared sector");
            }
            sectors = data_bytes / track->sector_bytes;
        }
        if (sectors == 0u || !schway__disc_image_add_u64(track->index1_lba, sectors,
                                                         &track->leadout) ||
            track->leadout > UINT32_MAX) {
            return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                                 track->file_line, "track has no bounded data sectors");
        }
        if (track->type != SCHWAY_DISC_TRACK_AUDIO) {
            if (!schway__disc_image_add_u64(data_sectors, sectors, &data_sectors)) {
                return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE,
                                                     track->file_line, "data sector count overflow");
            }
        }
        if (track->last_index_frame > track->index01_frame) {
            uint64_t last_index_offset;
            if (!schway__disc_image_mul_u64(track->last_index_frame, track->sector_bytes,
                                             &last_index_offset) ||
                last_index_offset > source->size) {
                return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_CUE,
                                                     track->index_line,
                                                     "INDEX points outside the FILE child");
            }
        }
    }
    if (data_sectors < SCHWAY__DISC_IMAGE_MIN_DATA_SECTORS) {
        return schway__disc_image_line_error(error, SCHWAY_DISC_IMAGE_BAD_IMAGE, 0u,
                                             "disc has fewer than 16 data sectors");
    }
    memset(&image->toc, 0, sizeof image->toc);
    image->toc.size = (uint32_t)sizeof image->toc;
    image->toc.track_count = (uint32_t)image->track_count;
    for (size_t i = 0u; i < image->track_count; i++) {
        const schway__disc_image_track *source = &image->tracks[i];
        schway_disc_track *target = &image->toc.tracks[i];
        target->size = (uint32_t)sizeof *target;
        target->type = source->type;
        target->index0 = (uint32_t)source->index0_lba;
        target->index1 = (uint32_t)source->index1_lba;
        target->pregap = (uint32_t)(source->index1_lba - source->index0_lba);
        target->leadout = (uint32_t)source->leadout;
    }
    return SCHWAY_DISC_IMAGE_OK;
}

static void schway__disc_image_prepare_descriptor(schway_disc_image *image)
{
    image->descriptor.size = (uint32_t)sizeof image->descriptor;
    image->descriptor.user = image;
    image->descriptor.read_sector = schway__disc_image_read_sector;
    image->descriptor.toc = schway__disc_image_read_toc;
}

static schway_disc_image_status schway__disc_image_open_iso(
    const char *path, schway_disc_image *image, schway_disc_image_error *error)
{
    uint64_t file_size;
    image->sources[0].file = schway__disc_image_open_readonly(path);
    if (image->sources[0].file == NULL) {
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_IO, 0u,
                                     "cannot open disc image");
        return SCHWAY_DISC_IMAGE_IO;
    }
    image->source_count = 1u;
    if (!schway__disc_image_file_size(image->sources[0].file, &file_size)) {
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_IO, 0u,
                                     "cannot size disc image");
        return SCHWAY_DISC_IMAGE_IO;
    }
    if (file_size > SCHWAY__DISC_IMAGE_MAX_BYTES) {
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_LIMIT, 0u,
                                     "disc image exceeds the 1 GiB limit");
        return SCHWAY_DISC_IMAGE_LIMIT;
    }
    if (file_size == 0u || file_size % SCHWAY__DISC_IMAGE_ISO_SECTOR_BYTES != 0u ||
        file_size / SCHWAY__DISC_IMAGE_ISO_SECTOR_BYTES < SCHWAY__DISC_IMAGE_MIN_DATA_SECTORS) {
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_BAD_IMAGE, 0u,
                                     "ISO must contain at least 16 complete 2048-byte sectors");
        return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    }
    image->sources[0].size = file_size;
    image->track_count = 1u;
    image->tracks[0].source_index = 0u;
    image->tracks[0].type = SCHWAY_DISC_TRACK_MODE1;
    image->tracks[0].sector_bytes = SCHWAY__DISC_IMAGE_ISO_SECTOR_BYTES;
    image->tracks[0].leadout = file_size / SCHWAY__DISC_IMAGE_ISO_SECTOR_BYTES;
    memset(&image->toc, 0, sizeof image->toc);
    image->toc.size = (uint32_t)sizeof image->toc;
    image->toc.track_count = 1u;
    image->toc.tracks[0].size = (uint32_t)sizeof image->toc.tracks[0];
    image->toc.tracks[0].type = SCHWAY_DISC_TRACK_MODE1;
    image->toc.tracks[0].leadout = (uint32_t)image->tracks[0].leadout;
    schway__disc_image_prepare_descriptor(image);
    return SCHWAY_DISC_IMAGE_OK;
}

static schway_disc_image_status schway__disc_image_open_cue(
    const char *path, schway_disc_image *image, schway_disc_image_error *error)
{
    schway_disc_image_status status = schway__disc_image_parse_cue(path, image, error);
    if (status != SCHWAY_DISC_IMAGE_OK) return status;
    status = schway__disc_image_open_cue_children(path, image, error);
    if (status != SCHWAY_DISC_IMAGE_OK) return status;
    status = schway__disc_image_build_cue_toc(image, error);
    if (status != SCHWAY_DISC_IMAGE_OK) return status;
    schway__disc_image_prepare_descriptor(image);
    return SCHWAY_DISC_IMAGE_OK;
}

static schway_disc_image_status schway__disc_image_m3u_error(
    schway_disc_image_error *error, schway_disc_image_status status,
    uint32_t line, const char *reason)
{
    char message[96];
    const char *name = status == SCHWAY_DISC_IMAGE_LIMIT ? "BAD_M3U LIMIT" : "BAD_M3U";
    if (line == 0u) (void)snprintf(message, sizeof message, "%s: %s", name, reason);
    else (void)snprintf(message, sizeof message, "%s line %u: %s", name, line, reason);
    schway__disc_image_error_set(error, status, line, message);
    return status;
}

static schway_disc_image_status schway__disc_image_open_m3u(
    const char *path, schway_disc_image *image, schway_disc_image_error *error)
{
    FILE *file = schway__disc_image_open_readonly(path);
    char root[SCHWAY__DISC_IMAGE_PATH_CAP];
    uint64_t file_size;
    char *text = NULL;
    schway_disc_image_status status = SCHWAY_DISC_IMAGE_OK;
    if (file == NULL) {
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_IO, 0u,
                                     "cannot open m3u playlist");
        return SCHWAY_DISC_IMAGE_IO;
    }
    if (!schway__disc_image_file_size(file, &file_size)) {
        (void)fclose(file);
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_IO, 0u,
                                     "cannot size m3u playlist");
        return SCHWAY_DISC_IMAGE_IO;
    }
    if (file_size > SCHWAY__DISC_IMAGE_M3U_MAX_BYTES) {
        (void)fclose(file);
        return schway__disc_image_m3u_error(error, SCHWAY_DISC_IMAGE_LIMIT, 0u,
                                            "playlist exceeds 64 KiB");
    }
    if (file_size == 0u) {
        (void)fclose(file);
        return schway__disc_image_m3u_error(error, SCHWAY_DISC_IMAGE_BAD_M3U, 0u,
                                            "playlist has no entries");
    }
    text = (char *)malloc((size_t)file_size + 1u);
    if (text == NULL) {
        (void)fclose(file);
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_OUT_OF_MEMORY, 0u,
                                     "out of memory reading m3u playlist");
        return SCHWAY_DISC_IMAGE_OUT_OF_MEMORY;
    }
    int read_failed = fread(text, 1u, (size_t)file_size, file) != (size_t)file_size;
    if (fclose(file) != 0) read_failed = 1;
    if (read_failed) {
        free(text);
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_IO, 0u,
                                     "cannot read m3u playlist");
        return SCHWAY_DISC_IMAGE_IO;
    }
    text[file_size] = '\0';
    if (!schway__disc_image_cue_directory(path, root)) {
        free(text);
        return schway__disc_image_m3u_error(error, SCHWAY_DISC_IMAGE_BAD_M3U, 1u,
                                            "cannot resolve playlist directory");
    }
    size_t offset = 0u;
    uint32_t line = 0u;
    while (offset < (size_t)file_size) {
        size_t start = offset;
        size_t end;
        size_t first;
        size_t last;
        line++;
        while (offset < (size_t)file_size && text[offset] != '\n') offset++;
        end = offset;
        if (offset < (size_t)file_size) offset++;
        if (end > start && text[end - 1u] == '\r') end--;
        first = start;
        last = end;
        for (size_t i = start; i < end; i++) {
            unsigned char c = (unsigned char)text[i];
            if (c < 0x20u && c != (unsigned char)'\t') {
                status = schway__disc_image_m3u_error(error, SCHWAY_DISC_IMAGE_BAD_M3U,
                                                      line, "non-text byte in playlist");
                goto done;
            }
            if (c > 0x7eu) {
                status = schway__disc_image_m3u_error(error, SCHWAY_DISC_IMAGE_BAD_M3U,
                                                      line, "playlist must be ASCII");
                goto done;
            }
        }
        while (first < last && (text[first] == ' ' || text[first] == '\t')) first++;
        while (last > first && (text[last - 1u] == ' ' || text[last - 1u] == '\t')) last--;
        if (first == last || text[first] == '#') continue;
        if (last - first > SCHWAY__DISC_IMAGE_CUE_NAME_MAX) {
            status = schway__disc_image_m3u_error(error, SCHWAY_DISC_IMAGE_BAD_M3U,
                                                  line, "image path exceeds 255 bytes");
            goto done;
        }
        if (image->playlist_count >= SCHWAY__DISC_IMAGE_M3U_ENTRY_MAX) {
            status = schway__disc_image_m3u_error(error, SCHWAY_DISC_IMAGE_LIMIT,
                                                  line, "playlist exceeds 32 images");
            goto done;
        }
        char entry[SCHWAY__DISC_IMAGE_CUE_NAME_MAX + 1u];
        size_t entry_size = last - first;
        memcpy(entry, text + first, entry_size);
        entry[entry_size] = '\0';
        if (!schway__disc_image_safe_name(entry)) {
            status = schway__disc_image_m3u_error(error, SCHWAY_DISC_IMAGE_BAD_M3U,
                                                  line, "unsafe or invalid image path");
            goto done;
        }
        if (schway__disc_image_extension_is(entry, ".m3u")) {
            status = schway__disc_image_m3u_error(error, SCHWAY_DISC_IMAGE_BAD_M3U,
                                                  line, "recursive playlists are not supported");
            goto done;
        }
        if (schway__disc_image_path_is_unsupported(entry)) {
            schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_UNSUPPORTED, line,
                                         schway__disc_image_unsupported_reason(entry));
            status = SCHWAY_DISC_IMAGE_UNSUPPORTED;
            goto done;
        }
        char candidate[SCHWAY__DISC_IMAGE_PATH_CAP];
        char canonical[SCHWAY__DISC_IMAGE_PATH_CAP];
        if (!schway__disc_image_child_path(root, entry, candidate) ||
            !schway__disc_image_canonical_path(candidate, canonical) ||
            !schway__disc_image_path_contained(root, canonical)) {
            status = schway__disc_image_m3u_error(error, SCHWAY_DISC_IMAGE_BAD_M3U,
                                                  line, "missing or escaping image entry");
            goto done;
        }
        schway_disc_image *child = NULL;
        schway_disc_image_error child_error;
        status = schway_disc_image_open(canonical, &child, &child_error);
        if (status != SCHWAY_DISC_IMAGE_OK) {
            if (error != NULL) {
                *error = child_error;
                error->line = line;
            }
            goto done;
        }
        image->playlist[image->playlist_count++] = child;
    }
    if (image->playlist_count == 0u) {
        status = schway__disc_image_m3u_error(error, SCHWAY_DISC_IMAGE_BAD_M3U, 0u,
                                              "playlist has no image entries");
    }

done:
    free(text);
    return status;
}

static uint32_t schway__disc_image_read_sector(void *user, uint32_t lba,
                                               uint8_t sector[2352], uint8_t subcode[96])
{
    schway_disc_image *image = (schway_disc_image *)user;
    schway__disc_image_track *track = NULL;
    uint64_t file_offset;
    uint64_t local_lba;
    uint64_t virtual_end;
    uint8_t raw[SCHWAY__DISC_IMAGE_RAW_SECTOR_BYTES];
    if (image == NULL || sector == NULL) return 0u;
    for (size_t i = 0u; i < image->track_count; i++) {
        if (lba >= image->tracks[i].index0_lba && lba < image->tracks[i].leadout) {
            track = &image->tracks[i];
            break;
        }
    }
    if (track == NULL) return 0u;
    virtual_end = track->index0_lba + track->virtual_pregap;
    if ((uint64_t)lba < virtual_end) {
        memset(raw, 0, sizeof raw);
        if (track->type == SCHWAY_DISC_TRACK_MODE1) {
            schway__disc_image_mode1_header(raw, lba);
            schway__disc_image_mode1_trailer(raw);
        } else if (track->type == SCHWAY_DISC_TRACK_MODE2) {
            schway__disc_image_mode2_blank(raw, lba);
        }
    } else {
        if ((uint64_t)lba < track->index1_lba) {
            local_lba = (uint64_t)lba - virtual_end;
            if (!schway__disc_image_mul_u64(local_lba, track->sector_bytes, &file_offset) ||
                !schway__disc_image_add_u64(track->index0_file_offset, file_offset,
                                             &file_offset)) return 0u;
        } else {
            local_lba = (uint64_t)lba - track->index1_lba;
            if (!schway__disc_image_mul_u64(local_lba, track->sector_bytes, &file_offset) ||
                !schway__disc_image_add_u64(track->index1_file_offset, file_offset,
                                             &file_offset)) return 0u;
        }
        schway__disc_image_source *source = &image->sources[track->source_index];
        if (file_offset > source->size || track->sector_bytes > source->size - file_offset ||
            file_offset > (uint64_t)LONG_MAX ||
            fseek(source->file, (long)file_offset, SEEK_SET) != 0 ||
            fread(raw, 1u, track->sector_bytes, source->file) != track->sector_bytes) return 0u;
        if (track->type == SCHWAY_DISC_TRACK_MODE1) {
            uint8_t cooked[SCHWAY__DISC_IMAGE_ISO_SECTOR_BYTES];
            memcpy(cooked, raw, sizeof cooked);
            memset(raw, 0, sizeof raw);
            schway__disc_image_mode1_header(raw, lba);
            memcpy(raw + 16u, cooked, sizeof cooked);
            schway__disc_image_mode1_trailer(raw);
        }
    }
    memcpy(sector, raw, sizeof raw);
    if (subcode != NULL) memset(subcode, 0, 96u);
    return 1u;
}

static uint32_t schway__disc_image_read_toc(void *user, schway_toc *out)
{
    const schway_disc_image *image = (const schway_disc_image *)user;
    if (image == NULL || out == NULL || out->size != sizeof *out) return 0u;
    *out = image->toc;
    return 1u;
}

schway_disc_image_status schway_disc_image_open(const char *path,
                                                schway_disc_image **out,
                                                schway_disc_image_error *error)
{
    schway_disc_image *image;
    schway_disc_image_status status;
    char canonical[SCHWAY__DISC_IMAGE_PATH_CAP];
    if (out == NULL || path == NULL || path[0] == '\0') {
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_BAD_ARGUMENT, 0u,
                                     "invalid disc image path or output");
        return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    }
    *out = NULL;
    if (error != NULL) {
        memset(error, 0, sizeof *error);
        error->size = (uint32_t)sizeof *error;
    }
    if (schway__disc_image_path_is_unsupported(path)) {
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_UNSUPPORTED, 0u,
                                     schway__disc_image_unsupported_reason(path));
        return SCHWAY_DISC_IMAGE_UNSUPPORTED;
    }
    if (!schway__disc_image_extension_is(path, ".iso") &&
        !schway__disc_image_extension_is(path, ".cue") &&
        !schway__disc_image_extension_is(path, ".m3u")) {
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_BAD_IMAGE, 0u,
                                     "expected a cue sheet, ISO image, or m3u playlist");
        return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    }
    if (!schway__disc_image_canonical_path(path, canonical)) {
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_IO, 0u,
                                     "cannot resolve disc image path");
        return SCHWAY_DISC_IMAGE_IO;
    }
    image = (schway_disc_image *)calloc(1u, sizeof *image);
    if (image == NULL) {
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_OUT_OF_MEMORY, 0u,
                                     "out of memory opening disc image");
        return SCHWAY_DISC_IMAGE_OUT_OF_MEMORY;
    }
    if (!schway__disc_image_store_path(image, canonical)) {
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_OUT_OF_MEMORY, 0u,
                                     "out of memory storing disc image path");
        schway_disc_image_close(image);
        return SCHWAY_DISC_IMAGE_OUT_OF_MEMORY;
    }
    if (schway__disc_image_extension_is(canonical, ".iso")) {
        status = schway__disc_image_open_iso(canonical, image, error);
    } else if (schway__disc_image_extension_is(canonical, ".m3u")) {
        status = schway__disc_image_open_m3u(canonical, image, error);
    } else {
        status = schway__disc_image_open_cue(canonical, image, error);
    }
    if (status != SCHWAY_DISC_IMAGE_OK) {
        schway_disc_image_close(image);
        return status;
    }
    *out = image;
    schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_OK, 0u, "ok");
    return SCHWAY_DISC_IMAGE_OK;
}

void schway_disc_image_close(schway_disc_image *image)
{
    if (image == NULL) return;
    for (size_t i = 0u; i < image->playlist_count; i++) {
        schway_disc_image_close(image->playlist[i]);
    }
    for (size_t i = 0u; i < image->source_count; i++) {
        if (image->sources[i].file != NULL) (void)fclose(image->sources[i].file);
        free(image->sources[i].canonical_path);
    }
    free(image->path);
    free(image);
}

const schway_disc *schway_disc_image_disc(const schway_disc_image *image)
{
    if (image == NULL) return NULL;
    if (image->playlist_count != 0u) return schway_disc_image_disc(image->playlist[0]);
    return &image->descriptor;
}

uint32_t schway_disc_image_count(const schway_disc_image *image)
{
    if (image == NULL) return 0u;
    return image->playlist_count != 0u ? (uint32_t)image->playlist_count : 1u;
}

schway_disc_image_status schway_disc_image_select(const schway_disc_image *image,
                                                  uint32_t index,
                                                  const schway_disc **out)
{
    if (out == NULL) return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    *out = NULL;
    if (image == NULL) return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    if (image->playlist_count == 0u) {
        if (index != 0u) return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
        *out = &image->descriptor;
        return SCHWAY_DISC_IMAGE_OK;
    }
    if (index >= image->playlist_count) return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    *out = schway_disc_image_disc(image->playlist[index]);
    return *out == NULL ? SCHWAY_DISC_IMAGE_BAD_M3U : SCHWAY_DISC_IMAGE_OK;
}

const char *schway_disc_image_path(const schway_disc_image *image, uint32_t index)
{
    if (image == NULL) return NULL;
    if (image->playlist_count == 0u) return index == 0u ? image->path : NULL;
    if (index >= image->playlist_count) return NULL;
    return schway_disc_image_path(image->playlist[index], 0u);
}

const char *schway_disc_image_label(const schway_disc_image *image, uint32_t index)
{
    if (image == NULL) return NULL;
    if (image->playlist_count == 0u) return index == 0u ? image->label : NULL;
    if (index >= image->playlist_count) return NULL;
    return schway_disc_image_label(image->playlist[index], 0u);
}

const char *schway_disc_image_status_string(schway_disc_image_status status)
{
    switch (status) {
    case SCHWAY_DISC_IMAGE_OK: return "ok";
    case SCHWAY_DISC_IMAGE_BAD_ARGUMENT: return "invalid argument";
    case SCHWAY_DISC_IMAGE_IO: return "disc image I/O error";
    case SCHWAY_DISC_IMAGE_BAD_CUE: return "BAD_CUE";
    case SCHWAY_DISC_IMAGE_BAD_IMAGE: return "BAD_IMAGE";
    case SCHWAY_DISC_IMAGE_LIMIT: return "disc image exceeds limits";
    case SCHWAY_DISC_IMAGE_OUT_OF_MEMORY: return "out of memory";
    case SCHWAY_DISC_IMAGE_UNSUPPORTED: return "unsupported disc image";
    case SCHWAY_DISC_IMAGE_BAD_M3U: return "BAD_M3U";
    case SCHWAY_DISC_IMAGE_BAD_SYSCNF: return "BAD_SYSCNF";
    }
    return "unknown disc image status";
}
