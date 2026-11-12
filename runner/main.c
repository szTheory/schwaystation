/* schwaystation-run: the headless runner.
 *
 *   schwaystation-run [--region ntsc-u|ntsc-j|pal] --frames N [--hash] [--png N:FILE]
 *   schwaystation-run [--region ntsc-u|ntsc-j|pal] --bench N [--hash] [--png N:FILE]
 *   schwaystation-run [--region ntsc-u|ntsc-j|pal] --bios FILE [--frames N] [--tty]
 *                     [--hash] [--hash-state]
 *   schwaystation-run [--region ntsc-u|ntsc-j|pal] --exe FILE [--frames N] [--tty]
 *                     [--stop-on mailbox] [--trace first-fault] [--no-stop-on-fault] [--hash]
 *                     [--hash-state]
 *   schwaystation-run --probe FILE
 *   schwaystation-run --help | --version
 *
 * A frame run without --bios or --exe shows the built-in test card. With
 * --hash the runner prints, after the last frame:
 *
 *   frame <N> sha256 <64 lowercase hex digits>
 *
 * The hash is SHA-256 over the video metadata as 13 little-endian u32 words
 * (field, interlaced, vres, hres, dot_div, region, x1, y1, start_x, start_y,
 * enabled, rgb24, bpp), then the display rows top to bottom, each pixel as a
 * little-endian u16 (RGB555) or packed RGB byte triples (RGB24), with no
 * padding (ARCHITECTURE section 7), so it does not depend on host byte order.
 *
 * --hash-state prints, after the frame line and every other line, the SHA-256 of the saved state
 * without its 20-byte header:
 *
 *   state sha256 <64 lowercase hex digits>
 *
 * With no picture beyond the test card a frame hash cannot show what the CPU, the GTE, DMA or the
 * timers did; the state hash can, and it is the same on every platform.
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
 * --bios FILE loads an exactly 524288-byte BIOS image into the instance before
 * reset. It prints the copied image's SHA-256 and digest-table flags, then runs
 * it for 600 frames by default. Its file name does not determine recognition.
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
 * kind=<kind>" and is status 7; with --no-stop-on-fault the run goes on, prints that line at the end
 * if the program made a fault, and keeps its own status. --trace first-fault also prints the last 4,096 trace records, oldest
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
#include <errno.h>

#include "bench.h"
#include "cards.h"
#include "clock.h"
#include "exit_status.h"
#include "frame_hash.h"
#include "inputs.h"
#include "mailbox.h"
#include "png.h"
#include "probe.h"
#include "revision.h"
#include "rss.h"
#include "schwaystation.h"
#include "sha256.h"
#include "stop.h"
#include "trace.h"
#include "tty.h"
#include "xrgb8888.h"

#ifdef SCHWAY_RUN_TEST_HOOKS
#include "internal.h"
#endif

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
    "usage: schwaystation-run [--region ntsc-u|ntsc-j|pal] --frames N [--inputs FILE] [--save-dir DIR] [--hash] [--png N:FILE]\n"
    "       schwaystation-run [--region ntsc-u|ntsc-j|pal] --bench N [--inputs FILE] [--save-dir DIR] [--hash] [--png N:FILE]\n"
    "       schwaystation-run [--region ntsc-u|ntsc-j|pal] --bios FILE [--frames N | --bench N] [--tty]\n"
    "                         [--hash] [--hash-state]\n"
    "       schwaystation-run [--region ntsc-u|ntsc-j|pal] --exe FILE [--frames N] [--tty]\n"
    "                         [--stop-on mailbox] [--trace first-fault] [--no-stop-on-fault]\n"
    "                         [--inputs FILE] [--save-dir DIR] [--memcard-names basename|serial]\n"
    "                         [--serial SERIAL] [--hash] [--hash-state]\n"
    "       schwaystation-run --probe FILE\n"
    "       schwaystation-run --help | --version\n"
    "  --region NAME      ntsc-u (default), ntsc-j or pal\n"
    "  --frames N         run N frames (1 <= N <= 1000000)\n"
    "  --bench N          run N frames and report the time per frame\n"
    "  --hash             print the SHA-256 of the last frame\n"
    "  --hash-state       print the SHA-256 of the saved machine state\n"
    "  --inputs FILE     apply a validated controller script before each frame latch\n"
    "  --save-dir DIR    import and save memory cards in this existing directory\n"
    "  --memcard-names MODE  basename (default) or serial\n"
    "  --serial SERIAL   use this validated serial as the card basename with serial mode\n"
    "  --png N:FILE       write frame N to FILE as a PNG (1 <= N <= --frames)\n"
    "  --bios FILE        load an exactly 524288-byte BIOS image and report its identity\n"
    "  --exe FILE         load a PS-EXE and run it (600 frames unless --frames or --bench)\n"
    "  --tty              print guest text as tty lines; needs --exe or --bios\n"
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

/* Read one exact-size BIOS image. The extra-byte probe bounds memory even for
   a hostile file and distinguishes a valid 512 KiB image from a longer one. */
static uint8_t *runner_read_bios_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        schway__refuse_file(path, "cannot open");
        return NULL;
    }
    uint8_t *bytes = (uint8_t *)malloc(SCHWAY_BIOS_SIZE);
    if (bytes == NULL) {
        (void)fclose(file);
        fprintf(stderr, "schwaystation-run: out of memory\n");
        return NULL;
    }
    size_t count = fread(bytes, 1u, SCHWAY_BIOS_SIZE, file);
    int read_error = ferror(file);
    int extra = EOF;
    if (count == SCHWAY_BIOS_SIZE && !read_error) {
        extra = fgetc(file);
        read_error = ferror(file);
    }
    if (fclose(file) != 0) {
        read_error = 1;
    }
    if (read_error) {
        schway__refuse_file(path, "cannot read");
        free(bytes);
        return NULL;
    }
    if (count != SCHWAY_BIOS_SIZE || extra != EOF) {
        schway__refuse_file(path, "BIOS must be exactly 524288 bytes");
        free(bytes);
        return NULL;
    }
    return bytes;
}

static const char *runner_bios_status_name(uint32_t status)
{
    return status == SCHWAY_BIOS_STATUS_RECOGNIZED ? "recognized" : "unknown";
}

static const char *runner_bios_region_name(uint32_t region)
{
    switch (region) {
    case SCHWAY_REGION_NTSC_U:
        return "ntsc-u";
    case SCHWAY_REGION_NTSC_J:
        return "ntsc-j";
    case SCHWAY_REGION_PAL:
        return "pal";
    case SCHWAY_BIOS_REGION_ANY:
        return "any";
    default:
        return "unknown";
    }
}

static const char *runner_bios_flags_name(uint32_t flags)
{
    if (flags == (SCHWAY_BIOS_INFO_LOADED | SCHWAY_BIOS_INFO_RECOGNIZED)) {
        return "loaded,recognized";
    }
    if (flags == SCHWAY_BIOS_INFO_LOADED) {
        return "loaded";
    }
    return "none";
}

static int runner_print_bios_info(const schway *inst)
{
    schway_bios_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    if (schway_get_bios_info(inst, &info) != SCHWAY_STATUS_OK) {
        fprintf(stderr, "schwaystation-run: cannot read BIOS identity\n");
        return SCHWAY__EXIT_FAILURE;
    }
    char sha256[65];
    schway__sha256_hex(info.sha256, sha256);
    printf("bios flags=%s status=%s region=%s sha256=%s\n",
           runner_bios_flags_name(info.flags), runner_bios_status_name(info.status),
           runner_bios_region_name(info.region), sha256);
    return SCHWAY__EXIT_OK;
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
    int hash_state; /* --hash-state: the SHA-256 of the saved state */
    uint32_t png_frame; /* 0 when no --png was given */
    const char *png_path;
    const char *inputs; /* --inputs FILE, else NULL */
    const char *save_dir; /* --save-dir DIR, else NULL */
    const char *serial; /* --serial SERIAL when serial card naming is selected */
    int serial_card_names; /* --memcard-names serial */
    int memcard_names_set;
    int region_set; /* --region was supplied */
    const char *probe; /* --probe FILE, else NULL */
#ifdef SCHWAY_RUN_TEST_HOOKS
    const char *test_ledger;
    const char *test_dat;
    const char *test_root;
    const char *test_config;
#endif
    const char *bios;  /* --bios FILE, else NULL */
    const char *exe;   /* --exe FILE, else NULL */
    int tty;           /* --tty: debug-port flag and the tty lines */
    int stop_mailbox;  /* --stop-on mailbox */
    int trace;         /* --trace first-fault */
    int keep_going;    /* --no-stop-on-fault */
    uint32_t region;
    int printed; /* --help or --version ran; nothing else to do */
#ifdef SCHWAY_RUN_TEST_HOOKS
    int test_card_write;
    int test_save_failure;
#endif
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
        } else if (strcmp(arg, "--hash-state") == 0) {
            o->hash_state = 1;
        } else if (strcmp(arg, "--inputs") == 0) {
            if (o->inputs != NULL) {
                return usage("give --inputs once");
            }
            if (value == NULL || value[0] == '-') {
                return usage("--inputs needs a file");
            }
            o->inputs = value;
            i++;
        } else if (strcmp(arg, "--save-dir") == 0) {
            if (o->save_dir != NULL) {
                return usage("give --save-dir once");
            }
            if (value == NULL || value[0] == '\0' || value[0] == '-') {
                return usage("--save-dir needs a directory");
            }
            o->save_dir = value;
            i++;
        } else if (strcmp(arg, "--memcard-names") == 0) {
            if (o->memcard_names_set || value == NULL) {
                return usage("give --memcard-names basename or serial once");
            }
            o->memcard_names_set = 1;
            if (strcmp(value, "serial") == 0) {
                o->serial_card_names = 1;
            } else if (strcmp(value, "basename") != 0) {
                return usage("--memcard-names needs basename or serial");
            }
            i++;
        } else if (strcmp(arg, "--serial") == 0) {
            if (o->serial != NULL) {
                return usage("give --serial once");
            }
            if (value == NULL || value[0] == '\0' || value[0] == '-') {
                return usage("--serial needs a value");
            }
            o->serial = value;
            i++;
#ifdef SCHWAY_RUN_TEST_HOOKS
        } else if (strcmp(arg, "--test-card-write") == 0) {
            if (o->test_card_write != 0 || value == NULL) {
                return usage("give --test-card-write once");
            }
            if (strcmp(value, "slot1") == 0) {
                o->test_card_write = 1;
            } else if (strcmp(value, "slot2") == 0) {
                o->test_card_write = 2;
            } else if (strcmp(value, "partial1") == 0) {
                o->test_card_write = 3;
            } else if (strcmp(value, "bad1") == 0) {
                o->test_card_write = 4;
            } else if (strcmp(value, "slot1_then_bad") == 0) {
                o->test_card_write = 5;
            } else {
                return usage("--test-card-write needs a known card test mode");
            }
            i++;
        } else if (strcmp(arg, "--test-save-failure") == 0) {
            if (o->test_save_failure != 0 || value == NULL) {
                return usage("give --test-save-failure once");
            }
            if (strcmp(value, "flush") == 0) {
                o->test_save_failure = SCHWAY_HOST_CARD_TEST_FAILURE_FLUSH;
            } else if (strcmp(value, "rename") == 0) {
                o->test_save_failure = SCHWAY_HOST_CARD_TEST_FAILURE_RENAME;
            } else {
                return usage("--test-save-failure needs flush or rename");
            }
            i++;
#endif
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
            o->region_set = 1;
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
        } else if (strcmp(arg, "--bios") == 0) {
            if (o->bios != NULL) {
                return usage("give --bios once");
            }
            if (value == NULL || value[0] == '\0' || value[0] == '-') {
                return usage("--bios needs a file");
            }
            o->bios = value;
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
            if (o->probe != NULL || value == NULL || value[0] == '-') {
                return usage("--probe needs one file");
            }
            o->probe = value;
            i++;
#ifdef SCHWAY_RUN_TEST_HOOKS
        } else if (strcmp(arg, "--test-ledger") == 0 ||
                   strcmp(arg, "--test-dat") == 0 ||
                   strcmp(arg, "--test-root") == 0 ||
                   strcmp(arg, "--test-config") == 0) {
            const char **slot = NULL;
            if (strcmp(arg, "--test-ledger") == 0) slot = &o->test_ledger;
            else if (strcmp(arg, "--test-dat") == 0) slot = &o->test_dat;
            else if (strcmp(arg, "--test-root") == 0) slot = &o->test_root;
            else slot = &o->test_config;
            if (*slot != NULL || value == NULL || value[0] == '-') {
                return usage("--test-* takes one path once");
            }
            *slot = value;
            i++;
#endif
        } else if (strcmp(arg, "--help") == 0 || strcmp(arg, "--version") == 0) {
            return usage("--help and --version take no other arguments");
        } else {
            return usage(arg[0] == '-' ? "unknown option" : "unexpected argument");
        }
    }
    if (o->bios != NULL && o->exe != NULL) {
        return usage("--bios and --exe cannot be used together");
    }
#ifdef SCHWAY_RUN_TEST_HOOKS
    if (o->probe != NULL) {
        int any_test_input = o->test_ledger != NULL || o->test_dat != NULL ||
                             o->test_root != NULL || o->test_config != NULL;
        int all_test_inputs = o->test_ledger != NULL && o->test_dat != NULL &&
                              o->test_root != NULL && o->test_config != NULL;
        if ((any_test_input && !all_test_inputs) || o->frames != 0u || o->bios != NULL ||
            o->exe != NULL || o->inputs != NULL || o->save_dir != NULL || o->hash ||
            o->hash_state || o->bench || o->tty || o->stop_mailbox || o->trace ||
            o->keep_going || o->png_path != NULL || o->memcard_names_set || o->serial != NULL ||
            o->region_set) {
            return usage("--probe cannot be combined with run options or partial test metadata");
        }
        return SCHWAY__EXIT_OK;
    }
    if (o->test_ledger != NULL || o->test_dat != NULL || o->test_root != NULL || o->test_config != NULL) {
        return usage("--test-* needs --probe");
    }
#else
    if (o->probe != NULL) {
        if (o->frames != 0u || o->bios != NULL || o->exe != NULL || o->inputs != NULL ||
            o->save_dir != NULL || o->hash || o->hash_state || o->bench || o->tty ||
            o->stop_mailbox || o->trace || o->keep_going || o->png_path != NULL ||
            o->memcard_names_set || o->serial != NULL || o->region_set) {
            return usage("--probe takes no other option");
        }
        return SCHWAY__EXIT_OK;
    }
#endif
    if (o->tty && o->exe == NULL && o->bios == NULL) {
        return usage("--tty needs --exe or --bios");
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
    if (o->serial_card_names && (o->save_dir == NULL || o->serial == NULL)) {
        return usage("--memcard-names serial needs --save-dir and --serial");
    }
    if (o->serial != NULL && !o->serial_card_names) {
        return usage("--serial needs --memcard-names serial");
    }
#ifdef SCHWAY_RUN_TEST_HOOKS
    if ((o->test_card_write != 0 || o->test_save_failure != 0) && o->save_dir == NULL) {
        return usage("card test options need --save-dir");
    }
#endif
    if (o->frames == 0u && (o->exe != NULL || o->bios != NULL)) {
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

typedef struct runner_card_files {
    char *path[2];
    uint64_t saved_generation[2];
    int ready;
} runner_card_files;

static void runner_cards_release(runner_card_files *cards)
{
    free(cards->path[0]);
    free(cards->path[1]);
    memset(cards, 0, sizeof *cards);
}

static int runner_card_basename(const options *opt, char out[64])
{
    const char *name = "schwaystation";
    if (opt->serial_card_names) {
        name = opt->serial;
    } else if (opt->exe != NULL) {
        name = opt->exe;
        for (const char *p = opt->exe; *p != '\0'; p++) {
            if (*p == '/' || *p == '\\') {
                name = p + 1;
            }
        }
        const char *dot = strrchr(name, '.');
        if (dot != NULL && dot != name) {
            size_t length = (size_t)(dot - name);
            if (length == 0u || length >= 64u) {
                return 0;
            }
            memcpy(out, name, length);
            out[length] = '\0';
            return 1;
        }
    }
    size_t length = strlen(name);
    if (length == 0u || length >= 64u) {
        return 0;
    }
    memcpy(out, name, length + 1u);
    return 1;
}

static void runner_card_lint(const char *path, const schway_host_card_lint *lint)
{
    if ((lint->flags & SCHWAY_HOST_CARD_LINT_BAD_HEADER) != 0u) {
        fprintf(stderr, "schwaystation-run: card %s: missing MC header\n", path);
    }
    if ((lint->flags & SCHWAY_HOST_CARD_LINT_BAD_CHECKSUM) != 0u) {
        fprintf(stderr, "schwaystation-run: card %s: %lu bad frame checksums\n", path,
                (unsigned long)lint->bad_frame_checksums);
    }
}

static int runner_cards_load(const options *opt, schway *inst, runner_card_files *cards)
{
    if (opt->save_dir == NULL) {
        return SCHWAY__EXIT_OK;
    }

    char basename[64];
    if (!runner_card_basename(opt, basename)) {
        fprintf(stderr, "schwaystation-run: invalid card save basename\n");
        return SCHWAY__EXIT_USAGE;
    }
    size_t directory_size = strlen(opt->save_dir);
    size_t suffix_capacity = 1u + 63u + sizeof(".1.mcr");
    if (directory_size > SIZE_MAX - suffix_capacity) {
        fprintf(stderr, "schwaystation-run: card save path is too long\n");
        return SCHWAY__EXIT_FAILURE;
    }
    size_t path_capacity = directory_size + suffix_capacity;
    cards->path[0] = (char *)malloc(path_capacity);
    cards->path[1] = (char *)malloc(path_capacity);
    if (cards->path[0] == NULL || cards->path[1] == NULL) {
        fprintf(stderr, "schwaystation-run: out of memory\n");
        runner_cards_release(cards);
        return SCHWAY__EXIT_FAILURE;
    }
    schway_host_card_status path_status = schway_host_card_paths_for_basename(
        opt->save_dir, basename, cards->path[0], path_capacity, cards->path[1], path_capacity);
    if (path_status != SCHWAY_HOST_CARD_OK) {
        fprintf(stderr, "schwaystation-run: invalid card save path\n");
        runner_cards_release(cards);
        return path_status == SCHWAY_HOST_CARD_ERR_ARGUMENT ? SCHWAY__EXIT_USAGE
                                                            : SCHWAY__EXIT_FAILURE;
    }

    for (uint32_t i = 0u; i < 2u; i++) {
        uint32_t slot = i + 1u;
        uint8_t *image = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
        if (image == NULL) {
            fprintf(stderr, "schwaystation-run: out of memory\n");
            runner_cards_release(cards);
            return SCHWAY__EXIT_FAILURE;
        }

        errno = 0;
        FILE *file = fopen(cards->path[i], "rb");
        if (file == NULL && errno == ENOENT) {
            free(image);
            continue;
        }
        if (file == NULL) {
            fprintf(stderr, "schwaystation-run: cannot read card file %s\n", cards->path[i]);
            free(image);
            runner_cards_release(cards);
            return SCHWAY__EXIT_FAILURE;
        }
        if (fclose(file) != 0) {
            fprintf(stderr, "schwaystation-run: cannot read card file %s\n", cards->path[i]);
            free(image);
            runner_cards_release(cards);
            return SCHWAY__EXIT_FAILURE;
        }

        schway_host_card_lint lint;
        memset(&lint, 0, sizeof lint);
        schway_host_card_status import_status = schway_host_card_import_file(
            cards->path[i], image, SCHWAY_MEMCARD_SIZE, &lint);
        if (import_status != SCHWAY_HOST_CARD_OK) {
            fprintf(stderr, "schwaystation-run: cannot import card file %s\n", cards->path[i]);
            free(image);
            runner_cards_release(cards);
            return SCHWAY__EXIT_FAILURE;
        }
        runner_card_lint(cards->path[i], &lint);
        schway_status status = schway_load_memcard(inst, slot, image, SCHWAY_MEMCARD_SIZE);
        free(image);
        if (status != SCHWAY_STATUS_OK ||
            schway_memcard_generation(inst, slot, &cards->saved_generation[i]) != SCHWAY_STATUS_OK) {
            fprintf(stderr, "schwaystation-run: cannot load card file %s\n", cards->path[i]);
            runner_cards_release(cards);
            return SCHWAY__EXIT_FAILURE;
        }
    }
    cards->ready = 1;
    return SCHWAY__EXIT_OK;
}

static int runner_cards_flush(const options *opt, schway *inst, runner_card_files *cards, int status)
{
    if (opt->save_dir == NULL || cards->ready == 0) {
        return status;
    }
    for (uint32_t i = 0u; i < 2u; i++) {
        uint32_t slot = i + 1u;
        uint64_t generation = 0u;
        if (schway_memcard_generation(inst, slot, &generation) != SCHWAY_STATUS_OK) {
            fprintf(stderr, "schwaystation-run: cannot read card generation for slot %lu\n",
                    (unsigned long)slot);
            if (status == SCHWAY__EXIT_OK) {
                status = SCHWAY__EXIT_FAILURE;
            }
            continue;
        }
        if (generation == cards->saved_generation[i]) {
            continue;
        }
        schway_span span;
        uint32_t memory = i == 0u ? SCHWAY_MEMORY_CARD1 : SCHWAY_MEMORY_CARD2;
        if (schway_get_memory(inst, memory, &span) != SCHWAY_STATUS_OK ||
            span.data == NULL || span.size != SCHWAY_MEMCARD_SIZE) {
            fprintf(stderr, "schwaystation-run: cannot read card slot %lu\n", (unsigned long)slot);
            if (status == SCHWAY__EXIT_OK) {
                status = SCHWAY__EXIT_FAILURE;
            }
            continue;
        }
        schway_host_card_status save_status;
#ifdef SCHWAY_RUN_TEST_HOOKS
        if (opt->test_save_failure != 0) {
            save_status = schway_host_card_test_save_atomic(
                cards->path[i], span.data, span.size,
                (schway_host_card_test_failure)opt->test_save_failure);
        } else
#endif
        {
            save_status = schway_host_card_save_atomic(cards->path[i], span.data, span.size);
        }
        if (save_status != SCHWAY_HOST_CARD_OK) {
            fprintf(stderr, "schwaystation-run: cannot save card file %s\n", cards->path[i]);
            if (status == SCHWAY__EXIT_OK) {
                status = SCHWAY__EXIT_FAILURE;
            }
            continue;
        }
        cards->saved_generation[i] = generation;
    }
    return status;
}

#ifdef SCHWAY_RUN_TEST_HOOKS
enum {
    SCHWAY__TEST_JOY_DATA = 0x1F801040u,
    SCHWAY__TEST_JOY_MODE = 0x1F801048u,
    SCHWAY__TEST_JOY_CTRL = 0x1F80104Au,
    SCHWAY__TEST_JOY_BAUD = 0x1F80104Eu,
    SCHWAY__TEST_I_STAT = 0x1F801070u,
    SCHWAY__TEST_CARD_ACK = 1500u
};

static int runner_test_mmio_write(schway *inst, uint32_t address, uint32_t width, uint32_t value)
{
    uint32_t cycles = 0u;
    return schway__bus_write(inst, address, width, value, &cycles) == 0u;
}

static int runner_test_mmio_read(schway *inst, uint32_t address, uint32_t width, uint32_t *value)
{
    uint32_t cycles = 0u;
    return schway__bus_read(inst, address, width, value, &cycles) == 0u;
}

static int runner_test_advance(schway *inst, uint64_t tick)
{
    if (tick < inst->ticks) {
        return 0;
    }
    inst->ticks = tick;
    schway__sched_sync(inst);
    schway__sched_run(inst);
    return 1;
}

static int runner_test_transfer(schway *inst, uint8_t sent, uint32_t ack_ticks, uint8_t *received)
{
    uint64_t start = inst->ticks;
    uint32_t port_bit = (inst->sio.ctrl & 0x2000u) != 0u ? 0x2000u : 0u;
    uint32_t value = 0u;
    if (!runner_test_mmio_write(inst, SCHWAY__TEST_JOY_DATA, 1u, sent) ||
        !runner_test_advance(inst, start + 1088u) ||
        !runner_test_mmio_read(inst, SCHWAY__TEST_JOY_DATA, 1u, &value)) {
        return 0;
    }
    *received = (uint8_t)value;
    if (ack_ticks == 0u) {
        return 1;
    }
    if (inst->sio.ack_pending == 0u || inst->sio.ack_deadline != start + ack_ticks ||
        !runner_test_advance(inst, start + ack_ticks) || inst->sio.ack_low == 0u ||
        !runner_test_mmio_write(inst, SCHWAY__TEST_I_STAT, 4u, 0u) ||
        !runner_test_mmio_write(inst, SCHWAY__TEST_JOY_CTRL, 2u, 0x1013u | port_bit) ||
        !runner_test_advance(inst, start + ack_ticks + 68u) || inst->sio.ack_low != 0u ||
        !runner_test_mmio_write(inst, SCHWAY__TEST_JOY_CTRL, 2u, 0x1003u | port_bit)) {
        return 0;
    }
    return 1;
}

static int runner_test_card_write(schway *inst, uint32_t port, uint8_t seed, uint32_t mode,
                                  uint8_t expected_flag)
{
    uint32_t port_bit = port == 0u ? 0u : 0x2000u;
    uint8_t reply = 0u;
    uint8_t data[128];
    if (!runner_test_mmio_write(inst, SCHWAY__TEST_JOY_MODE, 2u, 0x000Cu) ||
        !runner_test_mmio_write(inst, SCHWAY__TEST_JOY_BAUD, 2u, 0x0088u) ||
        !runner_test_mmio_write(inst, SCHWAY__TEST_JOY_CTRL, 2u, 0x1003u | port_bit)) {
        return 0;
    }
    if (!runner_test_transfer(inst, 0x81u, SCHWAY__TEST_CARD_ACK, &reply) || reply != 0xFFu ||
        !runner_test_transfer(inst, 0x57u, SCHWAY__TEST_CARD_ACK, &reply) ||
        reply != expected_flag ||
        !runner_test_transfer(inst, 0u, SCHWAY__TEST_CARD_ACK, &reply) || reply != 0x5Au ||
        !runner_test_transfer(inst, 0u, SCHWAY__TEST_CARD_ACK, &reply) || reply != 0x5Du ||
        !runner_test_transfer(inst, 0u, SCHWAY__TEST_CARD_ACK, &reply) || reply != 0u ||
        !runner_test_transfer(inst, 0u, SCHWAY__TEST_CARD_ACK, &reply) || reply != 0u ||
        !runner_test_transfer(inst, 7u, SCHWAY__TEST_CARD_ACK, &reply) || reply != 0u) {
        return 0;
    }
    for (uint32_t i = 0u; i < 128u; i++) {
        data[i] = (uint8_t)(i * 29u + seed);
    }
    uint8_t checksum = 7u;
    checksum ^= 0u;
    for (uint32_t i = 0u; i < 128u; i++) {
        if (mode == 3u && i == 32u) {
            return 1;
        }
        if (!runner_test_transfer(inst, data[i], SCHWAY__TEST_CARD_ACK, &reply) || reply != 0u) {
            return 0;
        }
        checksum ^= data[i];
    }
    uint8_t sent_checksum = mode == 4u ? (uint8_t)(checksum ^ 1u) : checksum;
    if (!runner_test_transfer(inst, sent_checksum, SCHWAY__TEST_CARD_ACK, &reply) || reply != 0u ||
        !runner_test_transfer(inst, 0u, SCHWAY__TEST_CARD_ACK, &reply) || reply != 0x5Cu ||
        !runner_test_transfer(inst, 0u, SCHWAY__TEST_CARD_ACK, &reply) || reply != 0x5Du ||
        !runner_test_transfer(inst, 0u, 0u, &reply)) {
        return 0;
    }
    return reply == (mode == 4u ? 0x4Eu : 0x47u);
}

static int runner_test_card_mode(schway *inst, int mode)
{
    if (mode == 1) {
        return runner_test_card_write(inst, 0u, 0x31u, 1u, 0x08u);
    }
    if (mode == 2) {
        return runner_test_card_write(inst, 1u, 0x62u, 1u, 0x08u);
    }
    if (mode == 3) {
        return runner_test_card_write(inst, 0u, 0x31u, 3u, 0x08u);
    }
    if (mode == 4) {
        return runner_test_card_write(inst, 0u, 0x62u, 4u, 0x08u);
    }
    if (mode == 5) {
        return runner_test_card_write(inst, 0u, 0x31u, 1u, 0x08u) &&
               runner_test_card_write(inst, 0u, 0x62u, 4u, 0u);
    }
    return 0;
}
#endif

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
    char hex[65];
    if (!schway__runner_frame_hash(&v, hex)) {
        fprintf(stderr, "schwaystation-run: unexpected video format\n");
        return SCHWAY__EXIT_FAILURE;
    }
    printf("frame %lu sha256 %s\n", (unsigned long)frames, hex);
    return SCHWAY__EXIT_OK;
}

/* The state's header: magic, format, total size and body CRC-32, 20 bytes (docs/embedding.md). The
   hash covers the body only, so it does not depend on the format number or the checksum. */
#define STATE_HEADER_SIZE 20u

/* Prints the state hash line: the SHA-256 of the saved state after its header. The runner may
   allocate; the core does not. Returns the exit status. */
static int print_state_hash(const schway *inst)
{
    size_t size = schway_state_size(inst);
    uint8_t *buf = size > STATE_HEADER_SIZE ? (uint8_t *)malloc(size) : NULL;
    if (buf == NULL) {
        fprintf(stderr, "schwaystation-run: cannot size the state\n");
        return SCHWAY__EXIT_FAILURE;
    }
    schway_status st = schway_save_state(inst, buf, size);
    if (st != SCHWAY_STATUS_OK) {
        fprintf(stderr, "schwaystation-run: schway_save_state failed: %s\n",
                schway_status_string(st));
        free(buf);
        return SCHWAY__EXIT_FAILURE;
    }
    schway__sha256 s;
    uint8_t digest[32];
    char hex[65];
    schway__sha256_init(&s);
    schway__sha256_update(&s, buf + STATE_HEADER_SIZE, size - STATE_HEADER_SIZE);
    schway__sha256_final(&s, digest);
    schway__sha256_hex(digest, hex);
    printf("state sha256 %s\n", hex);
    free(buf);
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
    if (!schway_host_video_valid(&v)) {
        fprintf(stderr, "schwaystation-run: unexpected video format\n");
        return SCHWAY__EXIT_FAILURE;
    }
    size_t size = schway__png_size(v.width, v.height);
    if (size == 0u) {
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
        size_t n = 0u;
        if (schway_host_xrgb8888_frame(&v, xrgb, v.width)) {
            for (size_t i = 0; i < count; i++) {
                rgb[3u * i] = (uint8_t)(xrgb[i] >> 16);
                rgb[3u * i + 1u] = (uint8_t)(xrgb[i] >> 8);
                rgb[3u * i + 2u] = (uint8_t)xrgb[i];
            }
            n = schway__png_write(file, size, rgb, v.width, v.height);
        }
        if (n == 0u) {
            fprintf(stderr, "schwaystation-run: unexpected video format\n");
            status = SCHWAY__EXIT_FAILURE;
        }
        FILE *f = n != 0u ? fopen(path, "wb") : NULL;
        int ok = f != NULL && fwrite(file, 1, n, f) == n;
        if (f != NULL && fclose(f) != 0) {
            ok = 0;
        }
        if (n != 0u && !ok) {
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
   0Ah, which is not part of it; other non-text bytes are replaced with '?'. */
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
            t->line[t->len++] = schway__runner_tty_byte(bytes[i]);
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

/* The first-fault line, from the core's fault record: 1 when there was a fault to print. */
static int print_fault(const schway *inst)
{
    schway_fault f;
    memset(&f, 0, sizeof f);
    f.size = (uint32_t)sizeof f;
    if (schway_get_fault(inst, &f) != SCHWAY_STATUS_OK || f.kind == SCHWAY_FAULT_NONE) {
        return 0;
    }
    printf("fault frame=%llu line=%lu dot=%lu pc=0x%08lx op=0x%08lx addr=0x%08lx width=%lu dev=%s "
           "kind=%s\n",
           (unsigned long long)f.frame, (unsigned long)f.line, (unsigned long)f.dot,
           (unsigned long)f.pc, (unsigned long)f.opcode, (unsigned long)f.addr,
           (unsigned long)f.width, schway__trace_device_name(f.device),
           schway__trace_kind_name(f.kind));
    return 1;
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
    schway__input_movie movie;
    memset(&movie, 0, sizeof movie);
    if (opt->inputs != NULL) {
        schway__inputs_error error;
        memset(&error, 0, sizeof error);
        schway__inputs_status input_status = schway__inputs_load(opt->inputs, &movie, &error);
        if (input_status != SCHWAY__INPUTS_OK) {
            if (input_status == SCHWAY__INPUTS_INVALID) {
                fprintf(stderr, "schwaystation-run: invalid input movie");
                if (error.line != 0u) {
                    fprintf(stderr, " at line %lu", (unsigned long)error.line);
                }
                if (error.reason != NULL) {
                    fprintf(stderr, ": %s", error.reason);
                }
                fputc('\n', stderr);
                fputs(synopsis, stderr);
                return SCHWAY__EXIT_USAGE;
            }
            fprintf(stderr, "schwaystation-run: %s\n",
                    input_status == SCHWAY__INPUTS_MEMORY ? "out of memory" :
                                                           "cannot read input movie");
            return SCHWAY__EXIT_FAILURE;
        }
    }
    uint8_t *bios = NULL;
    if (opt->bios != NULL) {
        bios = runner_read_bios_file(opt->bios);
        if (bios == NULL) {
            schway__inputs_release(&movie);
            return SCHWAY__EXIT_FAILURE;
        }
    }
    uint8_t *exe = NULL;
    size_t exe_size = 0;
    if (opt->exe != NULL) {
        exe = schway__read_exe_file(opt->exe, &exe_size);
        if (exe == NULL) {
            free(bios);
            schway__inputs_release(&movie);
            return SCHWAY__EXIT_FAILURE;
        }
    }
    uint32_t *samples = NULL;
    if (opt->bench) {
        samples = (uint32_t *)malloc(schway__bench_bytes(opt->frames));
        if (samples == NULL) {
            fprintf(stderr, "schwaystation-run: out of memory\n");
            free(bios);
            free(exe);
            schway__inputs_release(&movie);
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
        free(bios);
        free(exe);
        schway__inputs_release(&movie);
        return SCHWAY__EXIT_FAILURE;
    }
    int status = SCHWAY__EXIT_OK;
    runner_card_files card_files;
    memset(&card_files, 0, sizeof card_files);
    status = runner_cards_load(opt, inst, &card_files);
    /* The debug-port flag is set for --tty and for nothing else. */
    if (status == SCHWAY__EXIT_OK && (opt->region != 0u || opt->tty)) {
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
    if (status == SCHWAY__EXIT_OK && bios != NULL) {
        st = schway_load_bios(inst, bios, SCHWAY_BIOS_SIZE);
        if (st != SCHWAY_STATUS_OK) {
            schway__refuse_file(opt->bios, schway_status_string(st));
            status = SCHWAY__EXIT_FAILURE;
        } else {
            status = runner_print_bios_info(inst);
        }
    }
    if (status == SCHWAY__EXIT_OK && exe != NULL) {
        st = schway_load_exe(inst, exe, exe_size);
        if (st != SCHWAY_STATUS_OK) {
            schway__refuse_file(opt->exe, schway_status_string(st));
            status = SCHWAY__EXIT_FAILURE;
        }
    }
#ifdef SCHWAY_RUN_TEST_HOOKS
    if (status == SCHWAY__EXIT_OK && opt->test_card_write != 0 &&
        !runner_test_card_mode(inst, opt->test_card_write)) {
        fprintf(stderr, "schwaystation-run: card test transaction failed\n");
        status = SCHWAY__EXIT_FAILURE;
    }
#endif
    free(bios);
    free(exe);
    tty_lines tty;
    tty.len = 0u;
    /* Only schway_run_frame is timed, not hashing or writing files. */
    uint32_t ran = 0;
    uint32_t lag_frames = 0u;
    uint32_t exit_code = 0u; /* the byte the guest wrote to the exit port */
    int ended = 0;           /* the loop ended before the frame cap, by a stop of any kind */
    int faulted = 0;         /* the core stopped the run on the program's first fault */
    schway_budget budget;    /* an --exe run goes through schway_run, for its fault and trace flags */
    memset(&budget, 0, sizeof budget);
    budget.size = (uint32_t)sizeof budget;
    budget.max_cycles = UINT64_MAX;
    budget.flags = (opt->keep_going ? 0u : SCHWAY_RUN_STOP_ON_FAULT) |
                   (opt->trace ? SCHWAY_RUN_TRACE : 0u);
    size_t input_cursor = 0u;
    schway_input input_snapshot;
    memset(&input_snapshot, 0, sizeof input_snapshot);
    input_snapshot.size = (uint32_t)sizeof input_snapshot;
    for (uint32_t n = 1; n <= opt->frames && status == SCHWAY__EXIT_OK; n++) {
        if (opt->inputs != NULL) {
            st = schway__inputs_apply_frame(inst, &movie, &input_cursor, &input_snapshot, n);
            if (st != SCHWAY_STATUS_OK) {
                fprintf(stderr, "schwaystation-run: schway_set_input failed: %s\n",
                        schway_status_string(st));
                status = SCHWAY__EXIT_FAILURE;
                break;
            }
        }
        uint64_t start_ns = opt->bench ? schway__clock_ns() : 0u;
        schway_run_result res;
        memset(&res, 0, sizeof res);
        res.size = (uint32_t)sizeof res;
        st = opt->exe != NULL ? schway_run(inst, &budget, &res) : schway_run_frame(inst, &res);
        if (opt->bench) {
            samples[ran] = schway__bench_us(schway__clock_ns() - start_ns);
        }
        if (st == SCHWAY_STATUS_OK && opt->inputs != NULL && res.lag != 0u) {
            lag_frames++;
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
    /* A run that went on past its first fault (--no-stop-on-fault) still names it. */
    if ((faulted || opt->keep_going) && print_fault(inst) && opt->trace) {
        print_tail(inst);
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
    if (status == SCHWAY__EXIT_OK && opt->hash_state) {
        status = print_state_hash(inst);
    }
    if (status == SCHWAY__EXIT_OK && opt->inputs != NULL) {
        printf("lag frames=%lu\n", (unsigned long)lag_frames);
    }
    if (status == SCHWAY__EXIT_OK && opt->bench) {
        char report[512];
        size_t len =
            schway__bench_format(report, sizeof report, samples, ran, schway__rss_max_kib());
        (void)fwrite(report, 1u, len, stdout);
    }
    status = runner_cards_flush(opt, inst, &card_files, status);
    runner_cards_release(&card_files);
    schway_destroy(inst);
    free(samples);
    schway__inputs_release(&movie);
    return status;
}

int main(int argc, char **argv)
{
    options opt;
    memset(&opt, 0, sizeof opt);
    int status = parse_options(argc, argv, &opt);
    if (status == SCHWAY__EXIT_OK && opt.probe != NULL) {
#ifdef SCHWAY_RUN_TEST_HOOKS
        schway__probe_test_inputs test_inputs = {
            opt.test_ledger, opt.test_dat, opt.test_root, opt.test_config
        };
        status = schway__probe_print(stdout, opt.probe,
                                     opt.test_ledger != NULL ? &test_inputs : NULL) == 0 ? SCHWAY__EXIT_OK
                                                                                       : SCHWAY__EXIT_FAILURE;
#else
        status = schway__probe_print(stdout, opt.probe, NULL) == 0 ? SCHWAY__EXIT_OK
                                                             : SCHWAY__EXIT_FAILURE;
#endif
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
