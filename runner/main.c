/* schwaystation-run: the headless runner.
 *
 *   schwaystation-run [--region ntsc-u|ntsc-j|pal] --frames N [--hash] [--png N:FILE]
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
 * Exit statuses are listed in exit_status.h and printed by --help.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exit_status.h"
#include "png.h"
#include "schwaystation.h"
#include "sha256.h"
#include "xrgb8888.h"

#define FRAMES_MAX 1000000u

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
    "       schwaystation-run --help | --version\n"
    "  --region NAME      ntsc-u (default), ntsc-j or pal\n"
    "  --frames N         run N frames (1 <= N <= 1000000)\n"
    "  --hash             print the SHA-256 of the last frame\n"
    "  --png N:FILE       write frame N to FILE as a PNG (1 <= N <= --frames)\n"
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

/* The library's version, which is the release's version. */
static void version(void)
{
    schway_versions v;
    memset(&v, 0, sizeof v);
    v.size = (uint32_t)sizeof v;
    (void)schway_get_versions(&v);
    printf("schwaystation-run %lu.%lu.%lu\n", (unsigned long)v.library_major,
           (unsigned long)v.library_minor, (unsigned long)v.library_patch);
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
    int hash;
    uint32_t png_frame; /* 0 when no --png was given */
    const char *png_path;
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
        if (strcmp(arg, "--frames") == 0) {
            if (o->frames != 0u) {
                return usage("give --frames once");
            }
            if (!parse_frames(value, &o->frames)) {
                return usage("--frames needs a whole number from 1 to 1000000");
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
        } else if (strcmp(arg, "--help") == 0 || strcmp(arg, "--version") == 0) {
            return usage("--help and --version take no other arguments");
        } else {
            return usage(arg[0] == '-' ? "unknown option" : "unexpected argument");
        }
    }
    if (o->frames == 0u) {
        return usage("--frames N is required");
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

/* Runs the frames. Returns the exit status. */
static int run(const options *opt)
{
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
        return SCHWAY__EXIT_FAILURE;
    }
    int status = SCHWAY__EXIT_OK;
    if (opt->region != 0u) {
        schway_profile p;
        memset(&p, 0, sizeof p);
        p.size = (uint32_t)sizeof p;
        p.region = opt->region;
        st = schway_set_profile(inst, &p);
        if (st != SCHWAY_STATUS_OK) {
            fprintf(stderr, "schwaystation-run: schway_set_profile failed: %s\n",
                    schway_status_string(st));
            status = SCHWAY__EXIT_FAILURE;
        }
    }
    for (uint32_t n = 1; n <= opt->frames && status == SCHWAY__EXIT_OK; n++) {
        st = schway_run_frame(inst, NULL);
        if (st != SCHWAY_STATUS_OK) {
            fprintf(stderr, "schwaystation-run: schway_run_frame failed: %s\n",
                    schway_status_string(st));
            status = SCHWAY__EXIT_FAILURE;
        }
        if (status == SCHWAY__EXIT_OK && n == opt->png_frame) {
            status = write_png(inst, opt->png_path);
        }
    }
    if (status == SCHWAY__EXIT_OK && opt->hash) {
        status = print_hash(inst, opt->frames);
    }
    schway_destroy(inst);
    return status;
}

int main(int argc, char **argv)
{
    options opt;
    memset(&opt, 0, sizeof opt);
    int status = parse_options(argc, argv, &opt);
    if (status == SCHWAY__EXIT_OK && !opt.printed) {
        status = run(&opt);
    }
    if (fflush(stdout) != 0) {
        fprintf(stderr, "schwaystation-run: cannot write output\n");
        return SCHWAY__EXIT_FAILURE;
    }
    return status;
}
