/* Loads the built libretro module at run time and calls it in the order a
 * frontend such as RetroArch does.
 *
 *   schwaystation-test-libretro-host <module>
 *
 * The host runs a reference instance of the library beside the module, frame
 * for frame, and converts its video with the same schway_host_xrgb8888_frame
 * the runner's --png uses, so equal bytes mean the delivered frame equals the
 * runner's conversion. Pixels are compared as the module wrote them, so a
 * swapped channel or byte order fails here. "Lnnn" is a line of the vendored
 * libretro.h. */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../check.h"
#include "libretro.h"
#include "schwaystation.h"
#include "xrgb8888.h"

#define MAX_W 640u
#define MAX_H 576u
#define FRAMES 60u
#define POISON 0xA5u

/* a == b as doubles, printed with every digit. */
#define CHECK_EQ_DBL(a, b)                                                                         \
    do {                                                                                           \
        double check_a_ = (double)(a);                                                             \
        double check_b_ = (double)(b);                                                             \
        if (check_a_ != check_b_) {                                                                \
            fprintf(stderr, "%s:%d: %s == %s failed: %.17g != %.17g\n", __FILE__, __LINE__, #a,    \
                    #b, check_a_, check_b_);                                                       \
            check_failures++;                                                                      \
        }                                                                                          \
    } while (0)

/* ---- module loading ---- */

#ifdef _WIN32
static HMODULE module;
static int module_open(const char *path)
{
    module = LoadLibraryA(path);
    return module != NULL;
}
static void *module_sym(const char *name)
{
    FARPROC f = GetProcAddress(module, name);
    void *p = NULL;
    memcpy(&p, &f, sizeof p);
    return p;
}
static void module_close(void)
{
    FreeLibrary(module);
}
#else
static void *module;
static int module_open(const char *path)
{
    module = dlopen(path, RTLD_NOW);
    if (module == NULL) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
    }
    return module != NULL;
}
static void *module_sym(const char *name)
{
    return dlsym(module, name);
}
static void module_close(void)
{
    (void)dlclose(module);
}
#endif

/* The 25 entry points, typed as libretro.h declares them (L8770-9099). */
static void(RETRO_CALLCONV *p_set_environment)(retro_environment_t);
static void(RETRO_CALLCONV *p_set_video_refresh)(retro_video_refresh_t);
static void(RETRO_CALLCONV *p_set_audio_sample)(retro_audio_sample_t);
static void(RETRO_CALLCONV *p_set_audio_sample_batch)(retro_audio_sample_batch_t);
static void(RETRO_CALLCONV *p_set_input_poll)(retro_input_poll_t);
static void(RETRO_CALLCONV *p_set_input_state)(retro_input_state_t);
static void(RETRO_CALLCONV *p_init)(void);
static void(RETRO_CALLCONV *p_deinit)(void);
static unsigned(RETRO_CALLCONV *p_api_version)(void);
static void(RETRO_CALLCONV *p_get_system_info)(struct retro_system_info *);
static void(RETRO_CALLCONV *p_get_system_av_info)(struct retro_system_av_info *);
static void(RETRO_CALLCONV *p_set_controller_port_device)(unsigned, unsigned);
static void(RETRO_CALLCONV *p_reset)(void);
static void(RETRO_CALLCONV *p_run)(void);
static size_t(RETRO_CALLCONV *p_serialize_size)(void);
static bool(RETRO_CALLCONV *p_serialize)(void *, size_t);
static bool(RETRO_CALLCONV *p_unserialize)(const void *, size_t);
static void(RETRO_CALLCONV *p_cheat_reset)(void);
static void(RETRO_CALLCONV *p_cheat_set)(unsigned, bool, const char *);
static bool(RETRO_CALLCONV *p_load_game)(const struct retro_game_info *);
static bool(RETRO_CALLCONV *p_load_game_special)(unsigned, const struct retro_game_info *, size_t);
static void(RETRO_CALLCONV *p_unload_game)(void);
static unsigned(RETRO_CALLCONV *p_get_region)(void);
static void *(RETRO_CALLCONV *p_get_memory_data)(unsigned);
static size_t(RETRO_CALLCONV *p_get_memory_size)(unsigned);

static const char *missing;

/* ISO C has no conversion from void * to a function pointer, so the address
   is copied byte for byte. The first missing name is kept. */
#define RESOLVE(var, name)                                                                         \
    do {                                                                                           \
        void *p_ = module_sym(name);                                                               \
        if (p_ == NULL) {                                                                          \
            if (missing == NULL) {                                                                 \
                missing = name;                                                                    \
            }                                                                                      \
        } else {                                                                                   \
            memcpy(&(var), &p_, sizeof(var));                                                      \
        }                                                                                          \
    } while (0)

static void resolve_all(void)
{
    RESOLVE(p_set_environment, "retro_set_environment");
    RESOLVE(p_set_video_refresh, "retro_set_video_refresh");
    RESOLVE(p_set_audio_sample, "retro_set_audio_sample");
    RESOLVE(p_set_audio_sample_batch, "retro_set_audio_sample_batch");
    RESOLVE(p_set_input_poll, "retro_set_input_poll");
    RESOLVE(p_set_input_state, "retro_set_input_state");
    RESOLVE(p_init, "retro_init");
    RESOLVE(p_deinit, "retro_deinit");
    RESOLVE(p_api_version, "retro_api_version");
    RESOLVE(p_get_system_info, "retro_get_system_info");
    RESOLVE(p_get_system_av_info, "retro_get_system_av_info");
    RESOLVE(p_set_controller_port_device, "retro_set_controller_port_device");
    RESOLVE(p_reset, "retro_reset");
    RESOLVE(p_run, "retro_run");
    RESOLVE(p_serialize_size, "retro_serialize_size");
    RESOLVE(p_serialize, "retro_serialize");
    RESOLVE(p_unserialize, "retro_unserialize");
    RESOLVE(p_cheat_reset, "retro_cheat_reset");
    RESOLVE(p_cheat_set, "retro_cheat_set");
    RESOLVE(p_load_game, "retro_load_game");
    RESOLVE(p_load_game_special, "retro_load_game_special");
    RESOLVE(p_unload_game, "retro_unload_game");
    RESOLVE(p_get_region, "retro_get_region");
    RESOLVE(p_get_memory_data, "retro_get_memory_data");
    RESOLVE(p_get_memory_size, "retro_get_memory_size");
}

/* ---- frontend callbacks: record what the core does ---- */

static int no_game_calls, no_game_value;
static int pixel_format_calls;
static enum retro_pixel_format pixel_format = RETRO_PIXEL_FORMAT_0RGB1555;
static unsigned video_calls, video_bad;
static unsigned frame_w, frame_h;
static uint32_t frame[MAX_W * MAX_H];

static bool RETRO_CALLCONV env(unsigned cmd, void *data)
{
    switch (cmd) {
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
        no_game_calls++;
        no_game_value = *(const bool *)data ? 1 : 0;
        return true;
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        pixel_format_calls++;
        pixel_format = *(const enum retro_pixel_format *)data;
        return true;
    default:
        return false;
    }
}

/* Keeps the rows of the last frame, packed. A frame with no data, too large,
   or whose pitch is not its width in XRGB8888 pixels is counted and not
   kept. */
static void RETRO_CALLCONV video(const void *data, unsigned width, unsigned height, size_t pitch)
{
    unsigned y;
    video_calls++;
    if (data == NULL || width == 0u || width > MAX_W || height == 0u || height > MAX_H ||
        pitch != (size_t)width * 4u) {
        fprintf(stderr, "video: %ux%u pitch %lu%s\n", width, height, (unsigned long)pitch,
                data == NULL ? " (NULL data)" : "");
        video_bad++;
        return;
    }
    frame_w = width;
    frame_h = height;
    for (y = 0; y < height; y++) {
        memcpy(&frame[(size_t)y * width], (const unsigned char *)data + y * pitch, pitch);
    }
}

static void RETRO_CALLCONV sample(int16_t left, int16_t right)
{
    (void)left;
    (void)right;
}

static size_t RETRO_CALLCONV batch(const int16_t *data, size_t frames)
{
    (void)data;
    return frames;
}

static void RETRO_CALLCONV on_poll(void)
{
}

static int16_t RETRO_CALLCONV input(unsigned port, unsigned device, unsigned index, unsigned id)
{
    (void)port;
    (void)device;
    (void)index;
    (void)id;
    return 0;
}

/* got must equal want; prints both. */
static void check_string(const char *what, const char *got, const char *want)
{
    if (got == NULL || strcmp(got, want) != 0) {
        fprintf(stderr, "%s: got \"%s\", expected \"%s\"\n", what, got != NULL ? got : "(null)",
                want);
        check_failures++;
    }
}

/* ---- the reference instance ---- */

static void *ref_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void ref_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

/* A fresh instance with the given region, or NULL. */
static schway *ref_create(uint32_t region)
{
    schway_config cfg;
    schway_profile profile;
    schway *ref = NULL;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = ref_alloc;
    cfg.allocator.free = ref_free;
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = region;
    if (schway_create(&cfg, &ref) != SCHWAY_STATUS_OK) {
        return NULL;
    }
    if (schway_set_profile(ref, &profile) != SCHWAY_STATUS_OK) {
        schway_destroy(ref);
        return NULL;
    }
    return ref;
}

static uint32_t want[MAX_W * MAX_H];
static unsigned want_w, want_h;

/* Converts the reference instance's current frame as the runner does. */
static void ref_convert(const schway *ref)
{
    schway_video v;
    memset(&v, 0, sizeof v);
    v.size = (uint32_t)sizeof v;
    CHECK_EQ_U64(schway_get_video(ref, &v), SCHWAY_STATUS_OK);
    if (v.width == 0u || v.width > MAX_W || v.height == 0u || v.height > MAX_H) {
        fprintf(stderr, "reference frame %ux%u out of range\n", (unsigned)v.width,
                (unsigned)v.height);
        check_failures++;
        want_w = want_h = 0u;
        return;
    }
    want_w = v.width;
    want_h = v.height;
    schway_host_xrgb8888_frame(&v, want, want_w);
}

/* The captured frame must equal the reference conversion byte for byte;
   prints the first differing pixel. */
static void check_frame(const char *what)
{
    size_t i, n;
    if (frame_w != want_w || frame_h != want_h) {
        fprintf(stderr, "%s: delivered %ux%u, reference %ux%u\n", what, frame_w, frame_h, want_w,
                want_h);
        check_failures++;
        return;
    }
    n = (size_t)want_w * want_h;
    if (memcmp(frame, want, n * sizeof frame[0]) == 0) {
        return;
    }
    for (i = 0; i < n; i++) {
        if (frame[i] != want[i]) {
            fprintf(stderr, "%s: differs at (%lu,%lu): 0x%08lx, reference 0x%08lx\n", what,
                    (unsigned long)(i % want_w), (unsigned long)(i / want_w),
                    (unsigned long)frame[i], (unsigned long)want[i]);
            break;
        }
    }
    check_failures++;
}

/* Runs one module frame into a poisoned copy. */
static void run_one(void)
{
    memset(frame, POISON, sizeof frame);
    frame_w = frame_h = 0u;
    p_run();
}

/* Sets the frontend callbacks, as a frontend does after retro_init. */
static void set_callbacks(void)
{
    p_set_video_refresh(video);
    p_set_audio_sample(sample);
    p_set_audio_sample_batch(batch);
    p_set_input_poll(on_poll);
    p_set_input_state(input);
}

int main(int argc, char **argv)
{
    struct retro_system_info sys;
    struct retro_system_av_info av;
    char version[32];
    schway_versions v;
    schway *ref;
    unsigned run;

    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-libretro-host <module>\n");
        return 2;
    }
    if (!module_open(argv[1])) {
        fprintf(stderr, "cannot load %s\n", argv[1]);
        return 1;
    }
    /* Every entry point, before anything else is called. */
    resolve_all();
    if (missing != NULL) {
        fprintf(stderr, "missing entry point %s\n", missing);
        module_close();
        return 1;
    }

    memset(&v, 0, sizeof v);
    v.size = (uint32_t)sizeof v;
    CHECK_EQ_U64(schway_get_versions(&v), SCHWAY_STATUS_OK);
    snprintf(version, sizeof version, "%u.%u.%u", (unsigned)v.library_major,
             (unsigned)v.library_minor, (unsigned)v.library_patch);

    CHECK_EQ_U64(p_api_version(), RETRO_API_VERSION);
    p_set_environment(env);
    CHECK_EQ_U64(no_game_calls, 1);
    CHECK_EQ_U64(no_game_value, 1);
    p_init();

    memset(&sys, 0, sizeof sys);
    p_get_system_info(&sys);
    check_string("library_name", sys.library_name, "schwaystation");
    check_string("library_version", sys.library_version, version);
    check_string("valid_extensions", sys.valid_extensions, "");
    CHECK_EQ_U64(sys.need_fullpath, 1);
    CHECK_EQ_U64(sys.block_extract, 1);
    set_callbacks();

    /* No content: the pixel format is set before load returns (L870). */
    CHECK_EQ_U64(p_load_game(NULL), 1);
    CHECK_EQ_U64(pixel_format_calls, 1);
    CHECK_EQ_U64(pixel_format, RETRO_PIXEL_FORMAT_XRGB8888);
    CHECK_EQ_U64(p_get_region(), RETRO_REGION_NTSC);

    /* The rates are compared with exact equality: the adapter must divide
       the core's own rationals, not round a constant. */
    memset(&av, 0, sizeof av);
    p_get_system_av_info(&av);
    CHECK_EQ_U64(av.geometry.base_width, 320u);
    CHECK_EQ_U64(av.geometry.base_height, 240u);
    CHECK_EQ_U64(av.geometry.max_width, 640u);
    CHECK_EQ_U64(av.geometry.max_height, 480u);
    CHECK_EQ_DBL(av.geometry.aspect_ratio, 4.0f / 3.0f);
    CHECK_EQ_DBL(av.timing.fps, 53693175.0 / 897619.0);
    CHECK_EQ_DBL(av.timing.sample_rate, 44100.0);

    /* No memory regions in this release. */
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SAVE_RAM), 0);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SYSTEM_RAM), 0);
    CHECK(p_get_memory_data(RETRO_MEMORY_SAVE_RAM) == NULL);
    p_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
    p_cheat_reset();
    p_cheat_set(0, true, "");

    /* 60 frames, one video call each, each poisoned first; the 60th equals
       the reference instance's 60th frame. */
    ref = ref_create(SCHWAY_REGION_NTSC_U);
    CHECK(ref != NULL);
    if (ref == NULL) {
        module_close();
        return 1;
    }
    for (run = 1; run <= FRAMES; run++) {
        CHECK_EQ_U64(schway_run_frame(ref, NULL), SCHWAY_STATUS_OK);
        run_one();
    }
    CHECK_EQ_U64(video_calls, FRAMES);
    CHECK_EQ_U64(video_bad, 0);
    ref_convert(ref);
    check_frame("frame 60");

    schway_destroy(ref);
    p_unload_game();
    p_deinit();
    module_close();
    CHECK_DONE();
}
