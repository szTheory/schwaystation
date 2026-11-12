/* schwaystation-run --probe: the identity of a PS-EXE or bounded disc image. The core's
   validator supplies the header fields; the runner adds the file's name and
   size and two digests. No instance is made and no frame runs. */
#include "probe.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "md5.h"
#include "schwaystation.h"
#include "sha1.h"
#include "disc_probe.h"

#define EXE_HEADER 0x800u
#define EXE_MAX (EXE_HEADER + 0x200000u) /* the validator's largest file */

static FILE *probe_open_readonly(const char *path)
{
#if defined(_MSC_VER)
    FILE *file = NULL;
    if (fopen_s(&file, path, "rb") != 0) return NULL;
    return file;
#else
    return fopen(path, "rb");
#endif
}

/* Writes text with every byte outside printable ASCII shown as '?', so a
   hostile name cannot add a line or a control sequence to the report. */
static void put_text(FILE *f, const char *text)
{
    for (const char *p = text; *p != '\0'; p++) {
        unsigned char c = (unsigned char)*p;
        fputc(c >= 0x20u && c < 0x7Fu ? (int)c : '?', f);
    }
}

static void refuse(const char *path, const char *why)
{
    fputs("schwaystation-run: ", stderr);
    put_text(stderr, path);
    fprintf(stderr, ": %s\n", why);
}

/* The licence text names the console's market (psx-spx, "PSX EXE Format");
   the guess is that market's video standard. */
static const char *licence_area_name(uint32_t area)
{
    switch (area) {
    case SCHWAY_LICENCE_JAPAN:
        return "japan";
    case SCHWAY_LICENCE_EUROPE:
        return "europe";
    case SCHWAY_LICENCE_NORTH_AMERICA:
        return "north_america";
    default:
        return "none";
    }
}

static const char *region_guess(uint32_t area)
{
    switch (area) {
    case SCHWAY_LICENCE_JAPAN:
        return "ntsc-j";
    case SCHWAY_LICENCE_EUROPE:
        return "pal";
    case SCHWAY_LICENCE_NORTH_AMERICA:
        return "ntsc-u";
    default:
        return "unknown";
    }
}

/* The achievement-hash rule prepends the boot name before executable bytes. */
static void rcheevos_disc_md5(const char *boot_name, const uint8_t *file,
                              uint32_t hash_len, char hex[33])
{
    schway__md5 m;
    uint8_t digest[16];
    schway__md5_init(&m);
    schway__md5_update(&m, boot_name, (uint32_t)strlen(boot_name));
    schway__md5_update(&m, file, hash_len);
    schway__md5_final(&m, digest);
    schway__md5_hex(digest, hex);
}

static int path_inside(const char *root, const char *path)
{
    size_t size = strlen(root);
    int absolute = root[0] == '/' ||
                   (size >= 3u &&
                    ((root[0] >= 'A' && root[0] <= 'Z') ||
                     (root[0] >= 'a' && root[0] <= 'z')) &&
                    root[1] == ':' && root[2] == '/');
    if (size == 0u || path == NULL || strncmp(root, path, size) != 0 || path[size] != '/') {
        return 0;
    }
    const char *part = path + size + 1u;
    while (*part != '\0') {
        const char *end = strchr(part, '/');
        size_t part_size = end == NULL ? strlen(part) : (size_t)(end - part);
        if ((part_size == 1u && part[0] == '.') ||
            (part_size == 2u && part[0] == '.' && part[1] == '.')) return 0;
        if (end == NULL) break;
        part = end + 1u;
    }
    return absolute;
}

#if defined(SCHWAY_RUN_TEST_HOOKS)
static int path_diagnostics_enabled(void)
{
#if defined(_MSC_VER)
    char *value = NULL;
    size_t size = 0u;
    int enabled = _dupenv_s(&value, &size, "SCHWAY_RUN_PATH_DIAGNOSTICS") == 0 &&
                  value != NULL && strcmp(value, "1") == 0;
    free(value);
    return enabled;
#else
    const char *value = getenv("SCHWAY_RUN_PATH_DIAGNOSTICS");
    return value != NULL && strcmp(value, "1") == 0;
#endif
}

static const char *test_path_class(const char *path)
{
    if (path == NULL) return "null";
    size_t size = strlen(path);
    int drive = size >= 2u &&
                ((path[0] >= 'A' && path[0] <= 'Z') ||
                 (path[0] >= 'a' && path[0] <= 'z')) && path[1] == ':';
    if (drive && size >= 3u && path[2] == '/') return "drive-forward-slash";
    if (drive && size >= 3u && path[2] == '\\') return "drive-backslash";
    if (drive) return "drive-other";
    if (path[0] == '/') return "posix-root";
    return "relative";
}

static void report_path_diagnostic(const char *operand, const char *category,
                                   const char *root, const char *path,
                                   int open_ok, int header_ok)
{
    size_t root_size = root == NULL ? 0u : strlen(root);
    size_t path_size = path == NULL ? 0u : strlen(path);
    int root_absolute = root != NULL &&
        (root[0] == '/' || (root_size >= 3u &&
         (((root[0] >= 'A' && root[0] <= 'Z') ||
           (root[0] >= 'a' && root[0] <= 'z')) && root[1] == ':' &&
          (root[2] == '/' || root[2] == '\\'))));
    int prefix = root != NULL && path != NULL && path_size >= root_size &&
                 strncmp(root, path, root_size) == 0;
    int separator = prefix && path_size > root_size && path[root_size] == '/';
    int component = prefix && (path_size == root_size || separator);
    int inside = root != NULL && path != NULL && path_inside(root, path);
    fprintf(stderr,
        "schwaystation-run: test-path operand=%s category=%s root_class=%s path_class=%s root_len=%zu path_len=%zu root_absolute=%u prefix=%u separator=%u component_boundary=%u inside=%u open=%u header=%u\n",
        operand, category, test_path_class(root), test_path_class(path),
        root_size, path_size, (unsigned)root_absolute, (unsigned)prefix,
        (unsigned)separator, (unsigned)component, (unsigned)inside,
        (unsigned)open_ok, (unsigned)header_ok);
}
#endif

static int metadata_sources_ready(const schway__probe_metadata_inputs *inputs,
                                  const char *image)
{
    FILE *file;
    char line[64];
    if (inputs == NULL || inputs->ledger == NULL || inputs->dat == NULL) return 0;
    if (inputs->root == NULL && inputs->config == NULL) return 1;
    if (inputs->root == NULL || inputs->config == NULL) {
#if defined(SCHWAY_RUN_TEST_HOOKS)
        if (path_diagnostics_enabled())
            report_path_diagnostic("config", "containment", inputs->root,
                                   inputs->config, 0, 0);
#endif
        return 0;
    }
#if defined(SCHWAY_RUN_TEST_HOOKS)
    const char *operands[] = { "ledger", "dat", "config", "image" };
#endif
    const char *paths[] = { inputs->ledger, inputs->dat, inputs->config, image };
    for (size_t i = 0u; i < sizeof paths / sizeof paths[0]; i++) {
        if (!path_inside(inputs->root, paths[i])) {
#if defined(SCHWAY_RUN_TEST_HOOKS)
            if (path_diagnostics_enabled())
                report_path_diagnostic(operands[i], "containment", inputs->root,
                                       paths[i], 0, 0);
#endif
            return 0;
        }
    }
    file = probe_open_readonly(inputs->config);
    if (file == NULL) {
#if defined(SCHWAY_RUN_TEST_HOOKS)
        if (path_diagnostics_enabled())
            report_path_diagnostic("config", "open", inputs->root,
                                   inputs->config, 0, 0);
#endif
        return 0;
    }
    int header_ok = fgets(line, sizeof line, file) != NULL &&
                    strcmp(line, "schwaystation-test-metadata-v1\n") == 0 &&
                    fgetc(file) == EOF && ferror(file) == 0;
    int close_ok = fclose(file) == 0;
#if defined(SCHWAY_RUN_TEST_HOOKS)
    if ((!header_ok || !close_ok) && path_diagnostics_enabled())
        report_path_diagnostic("config", close_ok ? "header" : "close",
                               inputs->root, inputs->config, 1, header_ok);
#endif
    return header_ok && close_ok;
}

static void trim_eol(char *line)
{
    size_t size = strlen(line);
    while (size != 0u && (line[size - 1u] == '\n' || line[size - 1u] == '\r')) {
        line[--size] = '\0';
    }
}

static int dat_value(char *line, const char *key, char *value, size_t capacity)
{
    char *start = line;
    size_t key_size = strlen(key);
    while (*start == ' ' || *start == '\t') start++;
    if (strncmp(start, key, key_size) != 0 || start[key_size] != ' ') return 0;
    start += key_size + 1u;
    while (*start == ' ') start++;
    if (*start++ != '"') return -1;
    char *end = strchr(start, '"');
    if (end == NULL || end[1] != '\0') return -1;
    size_t size = (size_t)(end - start);
    if (size == 0u || size >= capacity || memchr(start, '\\', size) != NULL ||
        memchr(start, ',', size) != NULL) return -1;
    for (size_t i = 0u; i < size; i++) {
        unsigned char c = (unsigned char)start[i];
        if (c < 0x20u || c > 0x7Eu) return -1;
    }
    memcpy(value, start, size);
    value[size] = '\0';
    return 1;
}

static int dat_title_for_serial(const char *path, const char *wanted, char title[256])
{
    FILE *file = probe_open_readonly(path);
    char line[4097];
    char name[256] = "";
    char serial[64] = "";
    long end;
    int inside_game = 0;
    int bad_game = 0;
    int found = 0;
    if (file == NULL) return 0;
    if (fseek(file, 0L, SEEK_END) != 0 || (end = ftell(file)) < 0L ||
        end > 16L * 1024L * 1024L || fseek(file, 0L, SEEK_SET) != 0) {
        (void)fclose(file);
        return 0;
    }
    while (fgets(line, sizeof line, file) != NULL) {
        size_t size = strlen(line);
        if (size == sizeof line - 1u && line[size - 1u] != '\n' && !feof(file)) {
            inside_game = 0;
            found = 0;
            break;
        }
        trim_eol(line);
        char *trim = line;
        while (*trim == ' ' || *trim == '\t') trim++;
        if (strcmp(trim, "game (") == 0) {
            inside_game = 1;
            bad_game = 0;
            name[0] = '\0';
            serial[0] = '\0';
        } else if (inside_game && strcmp(trim, ")") == 0) {
            if (!bad_game && strcmp(serial, wanted) == 0 && name[0] != '\0') {
                if (found && strcmp(title, name) != 0) {
                    found = 0;
                    break;
                }
                memcpy(title, name, strlen(name) + 1u);
                found = 1;
            }
            inside_game = 0;
        } else if (inside_game) {
            int result = dat_value(line, "name", name, sizeof name);
            if (result < 0) {
                bad_game = 1;
            }
            result = dat_value(line, "serial", serial, sizeof serial);
            if (result < 0) {
                bad_game = 1;
            }
        }
    }
    if (inside_game) found = 0;
    int io_error = ferror(file) != 0;
    if (fclose(file) != 0) io_error = 1;
    if (io_error) return 0;
    return found;
}

static int ledger_matches(const char *path, const char *serial, const char *md5,
                          const char *track_sha1, const char *title, char date[11])
{
    static const char header[] =
        "serial,md5,track1_sha1,title,region,media,predicate,status,evidence,date";
    FILE *file = probe_open_readonly(path);
    char line[2048];
    unsigned row = 0u;
    int matched = 0;
    int valid = 1;
    long end;
    if (file == NULL) return 0;
    if (fseek(file, 0L, SEEK_END) != 0 || (end = ftell(file)) < 0L ||
        end > 16L * 1024L * 1024L || fseek(file, 0L, SEEK_SET) != 0) {
        (void)fclose(file);
        return 0;
    }
    while (fgets(line, sizeof line, file) != NULL) {
        char *fields[10];
        size_t size = strlen(line);
        size_t field = 0u;
        if (size == sizeof line - 1u && line[size - 1u] != '\n' && !feof(file)) { valid = 0; break; }
        trim_eol(line);
        if (row++ == 0u) {
            if (strcmp(line, header) != 0) { valid = 0; break; }
            continue;
        }
        fields[field++] = line;
        for (char *p = line; *p != '\0'; p++) {
            if (*p == '"') { field = 0u; break; }
            if (*p == ',') {
                *p = '\0';
                if (field >= 10u) { field = 0u; break; }
                fields[field++] = p + 1;
            }
        }
        if (field != 10u) { valid = 0; break; }
        if (strcmp(fields[0], serial) == 0 && strcmp(fields[1], md5) == 0 &&
            strcmp(fields[2], track_sha1) == 0 && strcmp(fields[3], title) == 0 &&
            strcmp(fields[5], "cue/bin") == 0 && strcmp(fields[6], "metadata.identity") == 0 &&
            strcmp(fields[7], "pass") == 0 && strlen(fields[9]) == 10u &&
            fields[9][4] == '-' && fields[9][7] == '-') {
            int date_ok = 1;
            for (size_t i = 0u; i < 10u; i++) {
                if (i != 4u && i != 7u && (fields[9][i] < '0' || fields[9][i] > '9')) {
                    date_ok = 0;
                }
            }
            if (!date_ok) continue;
            memcpy(date, fields[9], 11u);
            matched = 1;
        }
    }
    if (row == 0u || ferror(file) != 0) valid = 0;
    if (fclose(file) != 0) valid = 0;
    return valid && matched;
}

static int metadata_identity(const schway__probe_metadata_inputs *inputs, const char *image,
                             const schway_host_disc_probe *disc, const char *md5,
                             char track_sha1[41], char title[256], char date[11])
{
    schway__sha1_hex(disc->track1_sha1, track_sha1);
    if (!metadata_sources_ready(inputs, image) || disc->serial[0] == '\0' ||
        !dat_title_for_serial(inputs->dat, disc->serial, title)) return 0;
    return ledger_matches(inputs->ledger, disc->serial, md5, track_sha1, title, date);
}

uint8_t *schway__read_exe_file(const char *path, size_t *n_out)
{
    FILE *f = probe_open_readonly(path);
    if (f == NULL) {
        refuse("input", "cannot open the file");
        return NULL;
    }
    uint8_t *file = (uint8_t *)malloc((size_t)EXE_MAX + 1u);
    if (file == NULL) {
        (void)fclose(f);
        refuse("input", "out of memory");
        return NULL;
    }
    /* One byte more than the largest valid file, so a longer file is seen as
       too long rather than cut to a valid size. */
    size_t n = fread(file, 1u, (size_t)EXE_MAX + 1u, f);
    int read_ok = ferror(f) == 0;
    (void)fclose(f);
    if (!read_ok) {
        free(file);
        refuse("input", "cannot read the file");
        return NULL;
    }
    if (n == 0u) {
        free(file);
        refuse("input", "the file is empty");
        return NULL;
    }
    *n_out = n;
    return file;
}

void schway__refuse_file(const char *path, const char *why)
{
    (void)path;
    refuse("input", why);
}

int schway__probe_print(FILE *out, const char *path,
                        const schway__probe_metadata_inputs *metadata)
{
    schway_host_disc_probe disc;
    schway_host_disc_probe_status disc_status = schway__disc_probe_image(path, &disc);
    if (disc_status != SCHWAY_HOST_DISC_PROBE_NOT_ISO) {
        if (disc_status != SCHWAY_HOST_DISC_PROBE_OK) {
            refuse("input", schway__disc_probe_status_string(disc_status));
            return 1;
        }
        schway_exe_info disc_info;
        memset(&disc_info, 0, sizeof disc_info);
        disc_info.size = (uint32_t)sizeof disc_info;
        schway_status validation = schway_probe_exe(disc.exe_bytes, disc.exe_size, &disc_info);
        if (validation != SCHWAY_STATUS_OK) {
            schway__disc_probe_dispose(&disc);
            refuse("input", schway_status_string(validation));
            return 1;
        }
        char md5_hex[33];
        char track_sha1_hex[41];
        char title[256] = "";
        char date[11] = "";
        rcheevos_disc_md5(disc.boot_name, disc.exe_bytes, disc_info.hash_len, md5_hex);
        int identity_match = metadata_identity(metadata, path, &disc, md5_hex,
                                               track_sha1_hex, title, date);
        fputs("format=disc\n", out);
        if (identity_match) {
            fprintf(out, "serial=%s\nrcheevos_md5=%s\ntrack1_sha1=%s\ntitle=",
                    disc.serial, md5_hex, track_sha1_hex);
            put_text(out, title);
            fprintf(out, "\nmetadata.identity=pass\nmetadata.identity_date=%s\n", date);
        } else {
            fputs("serial=redacted\nrcheevos_md5=redacted\ntrack1_sha1=redacted\n"
                  "boot_name=redacted\nsha256=redacted\ntitle=unavailable\n"
                  "metadata.identity=unknown\n", out);
        }
        fputs("boot_status=unknown\nplay_status=unknown\n", out);
        schway__disc_probe_dispose(&disc);
        return ferror(out) == 0 ? 0 : 1;
    }
    size_t n = 0;
    uint8_t *file = schway__read_exe_file(path, &n);
    if (file == NULL) {
        return 1;
    }
    schway_exe_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    schway_status st = schway_probe_exe(file, n, &info);
    if (st != SCHWAY_STATUS_OK) {
        free(file);
        refuse("input", schway_status_string(st));
        return 1;
    }

    free(file);

    fputs("format=exe\nserial=n/a\n", out);
    fprintf(out, "licence_area=%s\nregion_guess=%s\nexe_name=redacted\n"
                 "exe_size=%lu\nsha256=redacted\nrcheevos_md5=redacted\n",
            licence_area_name(info.licence_area), region_guess(info.licence_area),
            (unsigned long)n);
    fputs("metadata.identity=unknown\n", out);
    fprintf(out, "pc0=0x%08lx\ngp0=0x%08lx\n", (unsigned long)info.pc0, (unsigned long)info.gp0);
    fprintf(out, "t_addr=0x%08lx\nt_size=0x%08lx\n", (unsigned long)info.t_addr,
            (unsigned long)info.t_size);
    fprintf(out, "d_addr=0x%08lx\nd_size=0x%08lx\n", (unsigned long)info.d_addr,
            (unsigned long)info.d_size);
    fprintf(out, "b_addr=0x%08lx\nb_size=0x%08lx\n", (unsigned long)info.b_addr,
            (unsigned long)info.b_size);
    fprintf(out, "s_addr=0x%08lx\ns_size=0x%08lx\n", (unsigned long)info.s_addr,
            (unsigned long)info.s_size);
    fprintf(out, "lint=0x%08lx\n", (unsigned long)info.lint);
    return 0;
}
