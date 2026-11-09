/* schwaystation-run: the headless runner.
 *
 *   schwaystation-run [--region ntsc-u|ntsc-j|pal] --frames N [--hash] [--png N:FILE]
 *   schwaystation-run [--region ntsc-u|ntsc-j|pal] --bench N [--hash] [--png N:FILE]
 *   schwaystation-run [--region ntsc-u|ntsc-j|pal] --exe FILE [--frames N] [--tty]
 *                     [--stop-on mailbox] [--trace first-fault] [--no-stop-on-fault] [--hash]
 *   schwaystation-run --probe FILE
 *   schwaystation-run --help | --version
 *
 * Runs N frames (1 <= N <= 1,000,000) of one instance with no BIOS and no
 * disc, which shows the built-in test card. With --hash it prints, after the
 * last frame:
 *
 *   frame <N> sha256 <64 lowercase hex digits>
 *
 * The hash is SHA-256 over the video metadata as 13 little-endian u32 words
 * (field, interlaced, vres, hres, dot_div, region, x1, y1, start_x, start_y,
 * enabled, rgb24, bpp), then the display rows top to bottom, each pixel as a
 * little-endian u16, with no padding (ARCHITECTURE section 7), so it does not
 * depend on the host's byte order.
 *
 * --png N:FILE writes frame N (1 <= N <= --frames) to FILE as an 8-bit RGB
 * PNG, each pixel converted by the shared XRGB8888 converter with the X byte
 * dropped. A file that cannot be written in full is removed.
 *
 * --bench N runs N frames as --frames N does, times each schway_run_frame
 * with the runner's monotonic clock and prints the report described in
 * bench.h. The core reads no clock.
 *
 * --exe FILE loads a PS-EXE into the instance and runs it, 600 frames when
 * neither --frames nor --bench is given. --tty sets the profile's debug-port
 * flag and prints the guest's text, one "tty <line>" per line.
 *
 * --probe FILE prints the identity of a PS-EXE (probe.h) and runs nothing: no
 * instance is made and no frame runs. It is valid only alone.
 *
 * With --tty a guest can end the run through the exit port: byte 00h is
 * status 0, any other byte prints "exit code=0x<hh>" and is status 6.
 *
 * --stop-on mailbox reads the fixture mailbox after every frame (mailbox.h)
 * and ends the run when it reports done (status 0) or fault (status 6); a run
 * that reaches the frame cap with neither prints "hang frames=<N> pc=0x<pc>"
 * and is status 8.
 *
 * An --exe run stops at the program's first fault (--no-stop-on-fault turns that off): it prints
 * "fault frame=<N> line=<L> dot=<D> pc=0x<pc> op=0x<op> addr=0x<addr> width=<W> dev=<device>
 * kind=<kind>" and is status 7. --trace first-fault also prints the last 4,096 trace records, oldest
 * first, as twelve tab-separated fields (trace.h), after a fault line or a hang line. An --exe run that
 * ends with a non-zero count of accesses to devices that are not built prints one
 * "unbuilt dev=<device> n=<count> last_pc=0x<pc>" line.
 *
 * SIGINT or SIGTERM ends the run after the frame in progress: what was asked
 * is printed for the frames that ran, and the status is 0.
 *
 * --version prints the library's version and the Git revision the build was
 * configured from, or "unknown" when it was configured outside a Git
 * checkout. docs/runner.md describes all of this for users.
 *
 * Exit statuses are listed in exit_status.h and printed by --help.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bench.h"
#include "clock.h"
#include "exit_status.h"
#include "mailbox.h"
#include "png.h"
#include "probe.h"
#include "revision.h"
#include "rss.h"
#include "schwaystation.h"
#include "sha256.h"
#include "stop.h"
#include "trace.h"
#include "xrgb8888.h"

/* The tests build a second runner with SCHWAY_RUN_RAISE_AT=N, which raises
   SIGTERM from inside frame N, to show the stop is clean. The installed
   runner has no such hook. */
#ifdef SCHWAY_RUN_RAISE_AT
#include <signal.h>
#endif

#define FRAMES_MAX 1000000u
#define EXE_FRAMES_DEFAULT 600u /* an --exe run that gives no frame count */
#define TTY_LINE_MAX 4096u      /* a longer line is printed in pieces */

/* The allocator the runner hands the core: the C library's malloc and free. */
static void *run_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void run_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static const char synopsis[] =
    "usage: schwaystation-run [--region ntsc-u|ntsc-j|pal] --frames N [--hash] [--png N:FILE]\n"
    "       schwaystation-run [--region ntsc-u|ntsc-j|pal] --bench N [--hash] [--png N:FILE]\n"
    "       schwaystation-run [--region ntsc-u|ntsc-j|pal] --exe FILE [--frames N] [--tty]\n"
    "                         [--stop-on mailbox] [--trace first-fault] [--no-stop-on-fault]\n"
    "                         [--hash]\n"
    "       schwaystation-run --probe FILE\n"
    "       schwaystation-run --help | --version\n"
    "  --region NAME      ntsc-u (default), ntsc-j or pal\n"
    "  --frames N         run N frames (1 <= N <= 1000000)\n"
    "  --bench N          run N frames and report the time per frame\n"
    "  --hash             print the SHA-256 of the last frame\n"
    "  --png N:FILE       write frame N to FILE as a PNG (1 <= N <= --frames)\n"
    "  --exe FILE         load a PS-EXE and run it (600 frames unless --frames or --bench)\n"
    "  --tty              print the program's text as tty lines; needs --exe\n"
    "  --stop-on mailbox  end the run when the program's mailbox says done or fault; needs --exe\n"
    "  --trace first-fault  print the last 4096 trace records after a fault or a hang; needs --exe\n"
    "  --no-stop-on-fault   keep running after the program's first fault; needs --exe\n"
    "  --probe FILE       print what a PS-EXE file is; runs nothing\n"
    "  --help             print this text and the exit statuses\n"
    "  --version          print the version\n";

static int usage(const char *why)
{
    fprintf(stderr, "schwaystation-run: %s\n%s", why, synopsis);
    return SCHWAY__EXIT_USAGE;
}

static void help(void)
{
    fputs(synopsis, stdout);
    fputs("exit status:\n", stdout);
    for (size_t i = 0; i < sizeof schway__exit_statuses / sizeof schway__exit_statuses[0]; i++) {
        printf("  %d  %-7s  %s\n", schway__exit_statuses[i].code, schway__exit_statuses[i].name,
               schway__exit_statuses[i].meaning);
    }
}

/* The library's version, which is the release's version, and the revision
   the build was configured from. */
static void version(void)
{
    schway_versions v;
    memset(&v, 0, sizeof v);
    v.size = (uint32_t)sizeof v;
    (void)schway_get_versions(&v);
    printf("schwaystation-run %lu.%lu.%lu %s\n", (unsigned long)v.library_major,
           (unsigned long)v.library_minor, (unsigned long)v.library_patch,
           SCHWAY__RUNNER_REVISION);
}

/* Parses text as a decimal number from 1 to FRAMES_MAX. Returns 0 on any
   other text. */
static int parse_frames(const char *text, uint32_t *out)
{
    uint32_t v = 0;
    if (text == NULL || *text == '\0') {
        return 0;
    }
    for (const char *p = text; *p != '\0'; p++) {
        if (*p < '0' || *p > '9') {
            return 0;
        }
        v = v * 10u + (uint32_t)(*p - '0');
        if (v > FRAMES_MAX) {
            return 0;
        }
    }
    if (v == 0u) {
        return 0;
    }
    *out = v;
    return 1;
}

/* Parses "N:FILE" with N a frame count as parse_frames reads it and FILE
   not empty. Returns 0 on any other text. */
static int parse_png(const char *text, uint32_t *frame, const char **path)
{
    const char *colon = text != NULL ? strchr(text, ':') : NULL;
    char number[8];
    size_t len = colon != NULL ? (size_t)(colon - text) : 0u;
    if (colon == NULL || len == 0u || len >= sizeof number || colon[1] == '\0') {
        return 0;
    }
    memcpy(number, text, len);
    number[len] = '\0';
    if (!parse_frames(number, frame)) {
        return 0;
    }
    *path = colon + 1;
    return 1;
}

typedef struct options {
    uint32_t frames;
    int bench; /* --bench N rather than --frames N */
    int hash;
    uint32_t png_frame; /* 0 when no --png was given */
    const char *png_path;
    const char *probe; /* --probe FILE, else NULL */
    const char *exe;   /* --exe FILE, else NULL */
    int tty;           /* --tty: debug-port flag and the tty lines */
    int stop_mailbox;  /* --stop-on mailbox */
    int trace;         /* --trace first-fault */
    int keep_going;    /* --no-stop-on-fault */
    uint32_t region;
    int printed; /* --help or --version ran; nothing else to do */
} options;

/* Returns 0 when the options are valid, else 2 after a usage message. Any
   option not listed in the synopsis, and any other argument, is a usage
   error. */
static int parse_options(int argc, char **argv, options *o)
{
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        help();
        o->printed = 1;
        return SCHWAY__EXIT_OK;
    }
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        version();
        o->printed = 1;
        return SCHWAY__EXIT_OK;
    }
    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        const char *value = i + 1 < argc ? argv[i + 1] : NULL;
        if (strcmp(arg, "--frames") == 0 || strcmp(arg, "--bench") == 0) {
            if (o->frames != 0u) {
                return usage("give --frames or --bench once");
            }
            o->bench = strcmp(arg, "--bench") == 0;
            if (!parse_frames(value, &o->frames) || schway__bench_bytes(o->frames) == 0u) {
                return usage("--frames and --bench need a whole number from 1 to 1000000");
            }
            i++;
        } else if (strcmp(arg, "--hash") == 0) {
            o->hash = 1;
        } else if (strcmp(arg, "--png") == 0) {
            if (o->png_frame != 0u) {
                return usage("give --png once");
            }
            if (!parse_png(value, &o->png_frame, &o->png_path)) {
                return usage("--png needs N:FILE with N from 1 to 1000000");
            }
            i++;
        } else if (strcmp(arg, "--region") == 0) {
            if (value != NULL && strcmp(value, "ntsc-u") == 0) {
                o->region = SCHWAY_REGION_NTSC_U;
            } else if (value != NULL && strcmp(value, "ntsc-j") == 0) {
                o->region = SCHWAY_REGION_NTSC_J;
            } else if (value != NULL && strcmp(value, "pal") == 0) {
                o->region = SCHWAY_REGION_PAL;
            } else {
                return usage("--region needs ntsc-u, ntsc-j or pal");
            }
            i++;
        } else if (strcmp(arg, "--exe") == 0) {
            if (o->exe != NULL) {
                return usage("give --exe once");
            }
            if (value == NULL || value[0] == '-') {
                return usage("--exe needs a file");
            }
            o->exe = value;
            i++;
        } else if (strcmp(arg, "--tty") == 0) {
            o->tty = 1;
        } else if (strcmp(arg, "--stop-on") == 0) {
            if (o->stop_mailbox) {
                return usage("give --stop-on once");
            }
            if (value == NULL || strcmp(value, "mailbox") != 0) {
                return usage("--stop-on needs mailbox");
            }
            o->stop_mailbox = 1;
            i++;
        } else if (strcmp(arg, "--trace") == 0) {
            if (o->trace) {
                return usage("give --trace once");
            }
            if (value == NULL || strcmp(value, "first-fault") != 0) {
                return usage("--trace needs first-fault");
            }
            o->trace = 1;
            i++;
        } else if (strcmp(arg, "--no-stop-on-fault") == 0) {
            o->keep_going = 1;
        } else if (strcmp(arg, "--probe") == 0) {
            /* Alone, with one file name that is not an option. */
            if (argc != 3 || value == NULL || value[0] == '-') {
                return usage("--probe takes one file and no other argument");
            }
            o->probe = value;
            return SCHWAY__EXIT_OK;
        } else if (strcmp(arg, "--help") == 0 || strcmp(arg, "--version") == 0) {
            return usage("--help and --version take no other arguments");
        } else {
            return usage(arg[0] == '-' ? "unknown option" : "unexpected argument");
        }
    }
    if (o->tty && o->exe == NULL) {
        return usage("--tty needs --exe");
    }
    if (o->stop_mailbox && o->exe == NULL) {
        return usage("--stop-on needs --exe");
    }
    if (o->trace && o->exe == NULL) {
        return usage("--trace needs --exe");
    }
    if (o->keep_going && o->exe == NULL) {
        return usage("--no-stop-on-fault needs --exe");
    }
    if (o->frames == 0u && o->exe != NULL) {
        o->frames = EXE_FRAMES_DEFAULT;
    }
    if (o->frames == 0u) {
        return usage("--frames N or --bench N is required");
    }
    if (o->png_frame > o->frames) {
        return usage("--png N must not exceed --frames");
    }
    return SCHWAY__EXIT_OK;
}

static void put_u32(schway__sha256 *s, uint32_t v)
{
    uint8_t b[4];
    for (uint32_t i = 0; i < 4u; i++) {
        b[i] = (uint8_t)(v >> (8u * i));
    }
    schway__sha256_update(s, b, sizeof b);
}

/* Prints the hash line for the frame just run. Returns the exit status. */
static int print_hash(const schway *inst, uint32_t frames)
{
    schway_video v;
    memset(&v, 0, sizeof v);
    v.size = (uint32_t)sizeof v;
    schway_status st = schway_get_video(inst, &v);
    if (st != SCHWAY_STATUS_OK) {
        fprintf(stderr, "schwaystation-run: schway_get_video failed: %s\n",
                schway_status_string(st));
        return SCHWAY__EXIT_FAILURE;
    }
    if (v.bpp != 16u || v.width > 1024u) {
        fprintf(stderr, "schwaystation-run: unexpected video format\n");
        return SCHWAY__EXIT_FAILURE;
    }
    const uint32_t tuple[13] = {v.field,  v.interlaced, v.vres,    v.hres,    v.dot_div,
                                v.region, v.x1,         v.y1,      v.start_x, v.start_y,
                                v.enabled, v.rgb24,     v.bpp};
    schway__sha256 s;
    uint8_t digest[32];
    char hex[65];
    uint8_t row[2u * 1024u];
    schway__sha256_init(&s);
    for (size_t i = 0; i < sizeof tuple / sizeof tuple[0]; i++) {
        put_u32(&s, tuple[i]);
    }
    /* Each row is serialised low byte first before hashing, so the result is
       the same on a big-endian host. */
    const uint8_t *base = (const uint8_t *)v.pixels;
    for (uint32_t y = 0; y < v.height; y++) {
        const uint16_t *src = (const uint16_t *)(const void *)(base + (size_t)y * v.pitch_bytes);
        for (uint32_t x = 0; x < v.width; x++) {
            row[2u * x] = (uint8_t)(src[x] & 0xFFu);
            row[2u * x + 1u] = (uint8_t)(src[x] >> 8);
        }
        schway__sha256_update(&s, row, (size_t)v.width * 2u);
    }
    schway__sha256_final(&s, digest);
    schway__sha256_hex(digest, hex);
    printf("frame %lu sha256 %s\n", (unsigned long)frames, hex);
    return SCHWAY__EXIT_OK;
}

/* Writes the frame just run to path as a PNG. The file is built in memory
   and written with one fwrite; a file that cannot be written in full is
   removed. Returns the exit status. */
static int write_png(const schway *inst, const char *path)
{
    schway_video v;
    memset(&v, 0, sizeof v);
    v.size = (uint32_t)sizeof v;
    schway_status st = schway_get_video(inst, &v);
    if (st != SCHWAY_STATUS_OK) {
        fprintf(stderr, "schwaystation-run: schway_get_video failed: %s\n",
                schway_status_string(st));
        return SCHWAY__EXIT_FAILURE;
    }
    size_t size = schway__png_size(v.width, v.height);
    if (v.bpp != 16u || v.width > 1024u || v.height > 512u || size == 0u) {
        fprintf(stderr, "schwaystation-run: unexpected video format\n");
        return SCHWAY__EXIT_FAILURE;
    }
    size_t count = (size_t)v.width * v.height;
    uint32_t *xrgb = (uint32_t *)malloc(count * sizeof *xrgb);
    uint8_t *rgb = (uint8_t *)malloc(count * 3u);
    uint8_t *file = (uint8_t *)malloc(size);
    int status = SCHWAY__EXIT_OK;
    if (xrgb == NULL || rgb == NULL || file == NULL) {
        fprintf(stderr, "schwaystation-run: out of memory\n");
        status = SCHWAY__EXIT_FAILURE;
    } else {
        schway_host_xrgb8888_frame(&v, xrgb, v.width);
        for (size_t i = 0; i < count; i++) {
            rgb[3u * i] = (uint8_t)(xrgb[i] >> 16);
            rgb[3u * i + 1u] = (uint8_t)(xrgb[i] >> 8);
            rgb[3u * i + 2u] = (uint8_t)xrgb[i];
        }
        size_t n = schway__png_write(file, size, rgb, v.width, v.height);
        FILE *f = n != 0u ? fopen(path, "wb") : NULL;
        int ok = f != NULL && fwrite(file, 1, n, f) == n;
        if (f != NULL && fclose(f) != 0) {
            ok = 0;
        }
        if (!ok) {
            fprintf(stderr, "schwaystation-run: cannot write %s\n", path);
            if (f != NULL) {
                (void)remove(path);
            }
            status = SCHWAY__EXIT_FAILURE;
        }
    }
    free(xrgb);
    free(rgb);
    free(file);
    return status;
}

/* The guest's text, collected a line at a time. A line is the bytes up to a
   0Ah, which is not part of it; bytes are printed as they came, with no
   character-set conversion. */
typedef struct tty_lines {
    uint8_t line[TTY_LINE_MAX];
    uint32_t len;
} tty_lines;

static void tty_emit(tty_lines *t)
{
    fputs("tty ", stdout);
    (void)fwrite(t->line, 1u, t->len, stdout);
    fputc('\n', stdout);
    t->len = 0u;
}

/* Drains the instance's TTY ring (4 KiB, so one call takes all of it) and
   prints each line that is now complete. A line over TTY_LINE_MAX bytes is
   printed in pieces, so a guest that never ends a line cannot grow memory. */
static void tty_drain(schway *inst, tty_lines *t)
{
    uint8_t bytes[4096];
    schway_tty io;
    memset(&io, 0, sizeof io);
    io.size = (uint32_t)sizeof io;
    io.bytes = bytes;
    io.capacity = (uint32_t)sizeof bytes;
    if (schway_get_tty(inst, &io) != SCHWAY_STATUS_OK) {
        return;
    }
    for (uint32_t i = 0; i < io.written; i++) {
        if (bytes[i] == 0x0Au) {
            tty_emit(t);
        } else {
            if (t->len == TTY_LINE_MAX) {
                tty_emit(t);
            }
            t->line[t->len++] = bytes[i];
        }
    }
}

/* The program counter, read through the state visitor's cpu.pc field. */
static void pc_field(void *user, const char *name, const void *data, uint32_t count,
                     uint32_t width)
{
    if (strcmp(name, "cpu.pc") == 0 && count == 1u && width == 4u) {
        memcpy(user, data, 4u);
    }
}

static uint32_t guest_pc(const schway *inst)
{
    uint32_t pc = 0u;
    (void)schway_state_visit(inst, pc_field, &pc);
    return pc;
}

/* The bus's unbuilt-device counters and the last unbuilt access, read through the state visitor. */
typedef struct unbuilt_view {
    uint32_t n[SCHWAY_DEVICE_UNMAPPED + 1u];
    uint32_t last_dev;
    uint32_t last_pc;
} unbuilt_view;

static void unbuilt_field(void *user, const char *name, const void *data, uint32_t count,
                          uint32_t width)
{
    unbuilt_view *v = (unbuilt_view *)user;
    if (width != 4u) {
        return;
    }
    if (strcmp(name, "bus.unbuilt_n") == 0 && count == SCHWAY_DEVICE_UNMAPPED + 1u) {
        memcpy(v->n, data, sizeof v->n);
    } else if (strcmp(name, "bus.last_unbuilt.dev") == 0 && count == 1u) {
        memcpy(&v->last_dev, data, 4u);
    } else if (strcmp(name, "bus.last_unbuilt.pc") == 0 && count == 1u) {
        memcpy(&v->last_pc, data, 4u);
    }
}

/* Prints the unbuilt line when any device that is not built has been accessed: the last such device,
   how many times it was, and the program counter of the last access. */
static void print_unbuilt(const schway *inst)
{
    unbuilt_view v;
    memset(&v, 0, sizeof v);
    if (schway_state_visit(inst, unbuilt_field, &v) != SCHWAY_STATUS_OK) {
        return;
    }
    uint32_t any = 0u;
    for (size_t i = 0; i < sizeof v.n / sizeof v.n[0]; i++) {
        any |= v.n[i];
    }
    if (any == 0u || v.last_dev >= sizeof v.n / sizeof v.n[0]) {
        return;
    }
    printf("unbuilt dev=%s n=%lu last_pc=0x%08lx\n", schway__trace_device_name(v.last_dev),
           (unsigned long)v.n[v.last_dev], (unsigned long)v.last_pc);
}

/* The first-fault line, from the core's fault record. */
static void print_fault(const schway *inst)
{
    schway_fault f;
    memset(&f, 0, sizeof f);
    f.size = (uint32_t)sizeof f;
    if (schway_get_fault(inst, &f) != SCHWAY_STATUS_OK) {
        return;
    }
    printf("fault frame=%llu line=%lu dot=%lu pc=0x%08lx op=0x%08lx addr=0x%08lx width=%lu dev=%s "
           "kind=%s\n",
           (unsigned long long)f.frame, (unsigned long)f.line, (unsigned long)f.dot,
           (unsigned long)f.pc, (unsigned long)f.opcode, (unsigned long)f.addr,
           (unsigned long)f.width, schway__trace_device_name(f.device),
           schway__trace_kind_name(f.kind));
}

/* The trace ring, oldest record first, one twelve-field line each. */
static void print_tail(const schway *inst)
{
    schway_trace r;
    char line[SCHWAY__TRACE_LINE_MAX];
    memset(&r, 0, sizeof r);
    r.size = (uint32_t)sizeof r;
    for (uint32_t i = 0; schway_get_trace(inst, i, &r) == SCHWAY_STATUS_OK; i++) {
        if (schway__trace_write_record(line, sizeof line, &r) < 0) {
            return;
        }
        puts(line);
    }
}

/* Runs the frames. The bench samples take 4 bytes per frame, so they are
   on the heap. Returns the exit status. */
static int run(const options *opt)
{
    uint8_t *exe = NULL;
    size_t exe_size = 0;
    if (opt->exe != NULL) {
        exe = schway__read_exe_file(opt->exe, &exe_size);
        if (exe == NULL) {
            return SCHWAY__EXIT_FAILURE;
        }
    }
    uint32_t *samples = NULL;
    if (opt->bench) {
        samples = (uint32_t *)malloc(schway__bench_bytes(opt->frames));
        if (samples == NULL) {
            fprintf(stderr, "schwaystation-run: out of memory\n");
            free(exe);
            return SCHWAY__EXIT_FAILURE;
        }
    }
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = run_alloc;
    cfg.allocator.free = run_free;
    schway *inst = NULL;
    schway_status st = schway_create(&cfg, &inst);
    if (st != SCHWAY_STATUS_OK) {
        fprintf(stderr, "schwaystation-run: schway_create failed: %s\n",
                schway_status_string(st));
        free(samples);
        free(exe);
        return SCHWAY__EXIT_FAILURE;
    }
    int status = SCHWAY__EXIT_OK;
    /* The debug-port flag is set for --tty and for nothing else. */
    if (opt->region != 0u || opt->tty) {
        schway_profile p;
        memset(&p, 0, sizeof p);
        p.size = (uint32_t)sizeof p;
        p.region = opt->region != 0u ? opt->region : SCHWAY_REGION_NTSC_U;
        p.flags = opt->tty ? SCHWAY_PROFILE_DEBUG_PORT : 0u;
        st = schway_set_profile(inst, &p);
        if (st != SCHWAY_STATUS_OK) {
            fprintf(stderr, "schwaystation-run: schway_set_profile failed: %s\n",
                    schway_status_string(st));
            status = SCHWAY__EXIT_FAILURE;
        }
    }
    if (status == SCHWAY__EXIT_OK && exe != NULL) {
        st = schway_load_exe(inst, exe, exe_size);
        if (st != SCHWAY_STATUS_OK) {
            schway__refuse_file(opt->exe, schway_status_string(st));
            status = SCHWAY__EXIT_FAILURE;
        }
    }
    free(exe);
    tty_lines tty;
    tty.len = 0u;
    /* Only schway_run_frame is timed, not hashing or writing files. */
    uint32_t ran = 0;
    uint32_t exit_code = 0u; /* the byte the guest wrote to the exit port */
    int ended = 0;           /* the loop ended before the frame cap, by a stop of any kind */
    int faulted = 0;         /* the core stopped the run on the program's first fault */
    schway_budget budget;    /* an --exe run goes through schway_run, for its fault and trace flags */
    memset(&budget, 0, sizeof budget);
    budget.size = (uint32_t)sizeof budget;
    budget.max_cycles = UINT64_MAX;
    budget.flags = (opt->keep_going ? 0u : SCHWAY_RUN_STOP_ON_FAULT) |
                   (opt->trace ? SCHWAY_RUN_TRACE : 0u);
    for (uint32_t n = 1; n <= opt->frames && status == SCHWAY__EXIT_OK; n++) {
        uint64_t start_ns = opt->bench ? schway__clock_ns() : 0u;
        schway_run_result res;
        memset(&res, 0, sizeof res);
        res.size = (uint32_t)sizeof res;
        st = opt->exe != NULL ? schway_run(inst, &budget, &res) : schway_run_frame(inst, &res);
        if (opt->bench) {
            samples[ran] = schway__bench_us(schway__clock_ns() - start_ns);
        }
#ifdef SCHWAY_RUN_RAISE_AT
        if (n == SCHWAY_RUN_RAISE_AT) {
            (void)raise(SIGTERM);
        }
#endif
        ran = n;
        if (opt->tty) {
            tty_drain(inst, &tty);
        }
        if (st != SCHWAY_STATUS_OK) {
            fprintf(stderr, "schwaystation-run: schway_run_frame failed: %s\n",
                    schway_status_string(st));
            status = SCHWAY__EXIT_FAILURE;
        }
        /* A fault that stopped the run ends it here, after the frame's text. */
        if (status == SCHWAY__EXIT_OK && res.stop == SCHWAY_STOP_FAULT) {
            faulted = 1;
            status = SCHWAY__EXIT_FAULT;
            break;
        }
        if (status == SCHWAY__EXIT_OK && n == opt->png_frame) {
            status = write_png(inst, opt->png_path);
        }
        /* The guest reported through the exit port: the run ends here. The
           text it printed first is flushed below, then the code is reported. */
        int hooked = status == SCHWAY__EXIT_OK && res.stop == SCHWAY_STOP_HOOK;
        if (hooked) {
            exit_code = (uint32_t)(res.exit_code & 0xFFu);
        }
        /* The mailbox is read once per frame, after the frame's text is out, so a HOOK stop in
           the same frame is read too. Done ends the run; fault and a CRC mismatch are status 6. */
        if (status == SCHWAY__EXIT_OK && opt->stop_mailbox) {
            int mb = schway__mailbox_check(inst, stdout);
            if (mb == SCHWAY__MAILBOX_DONE) {
                ended = 1;
                break;
            }
            if (mb != SCHWAY__MAILBOX_PENDING) {
                status = SCHWAY__EXIT_FIXTURE;
                break;
            }
        }
        if (hooked) {
            ended = 1;
            break;
        }
        /* A stop request ends the run only between whole frames, after the
           frame's outputs are written. */
        if (schway__stop_requested()) {
            ended = 1;
            break;
        }
    }
    /* No done or fault status by the frame cap is a hang. */
    if (status == SCHWAY__EXIT_OK && opt->stop_mailbox && !ended) {
        printf("hang frames=%lu pc=0x%08lx\n", (unsigned long)ran, (unsigned long)guest_pc(inst));
        if (opt->trace) {
            print_tail(inst);
        }
        status = SCHWAY__EXIT_HANG;
    }
    if (tty.len != 0u) {
        tty_emit(&tty);
    }
    if (faulted) {
        print_fault(inst);
        if (opt->trace) {
            print_tail(inst);
        }
    }
    if (status == SCHWAY__EXIT_OK && exit_code != 0u) {
        printf("exit code=0x%02lx\n", (unsigned long)exit_code);
        status = SCHWAY__EXIT_FIXTURE;
    }
    if (opt->exe != NULL) {
        print_unbuilt(inst);
    }
    if (status == SCHWAY__EXIT_OK && opt->hash) {
        status = print_hash(inst, ran);
    }
    if (status == SCHWAY__EXIT_OK && opt->bench) {
        char report[512];
        size_t len =
            schway__bench_format(report, sizeof report, samples, ran, schway__rss_max_kib());
        (void)fwrite(report, 1u, len, stdout);
    }
    schway_destroy(inst);
    free(samples);
    return status;
}

int main(int argc, char **argv)
{
    options opt;
    memset(&opt, 0, sizeof opt);
    int status = parse_options(argc, argv, &opt);
    if (status == SCHWAY__EXIT_OK && opt.probe != NULL) {
        status = schway__probe_print(stdout, opt.probe) == 0 ? SCHWAY__EXIT_OK
                                                             : SCHWAY__EXIT_FAILURE;
    } else if (status == SCHWAY__EXIT_OK && !opt.printed) {
        schway__stop_install();
        status = run(&opt);
    }
    if (fflush(stdout) != 0) {
        fprintf(stderr, "schwaystation-run: cannot write output\n");
        return SCHWAY__EXIT_FAILURE;
    }
    return status;
}
