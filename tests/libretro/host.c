/* Loads the built libretro module at run time and calls it in the order a
 * frontend such as RetroArch does.
 *
 *   schwaystation-test-libretro-host <module> ntsc-u|pal
 *
 * The host runs a reference instance of the library beside the module, frame
 * for frame, and converts its video with the same schway_host_xrgb8888_frame
 * the runner's --png uses, so equal bytes mean the delivered frame equals the
 * runner's conversion. Pixels are compared as the module wrote them, so a
 * swapped channel or byte order fails here. Each frame's audio must equal the
 * pairs the reference instance drains for that frame.
 *
 * ntsc-u: the host reports core options version 2, answers the region option
 * with ntsc-u and registers the batch audio callback. pal: the host has no
 * core options version, so the module must fall back to SET_VARIABLES; it
 * answers pal and registers only the single-sample callback. "Lnnn" is a
 * line of the vendored libretro.h. */
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
#define MAX_PAIRS 1024u
#define FRAMES 60u
#define SAVE_AT 30u
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

static int pal; /* the mode: 0 ntsc-u, 1 pal */
static int no_game_calls, no_game_value;
static int pixel_format_calls;
static enum retro_pixel_format pixel_format = RETRO_PIXEL_FORMAT_0RGB1555;
static unsigned options_v2_calls, variables_calls, get_variable_calls;
static const char *option_value; /* what GET_VARIABLE answers */
static bool option_updated;      /* what GET_VARIABLE_UPDATE answers, once */
static unsigned av_info_calls;
static struct retro_system_av_info av_info_sent;
static unsigned geometry_calls;
static struct retro_game_geometry geometry_sent;
static unsigned video_calls, video_bad;
static unsigned frame_w, frame_h;
static uint32_t frame[MAX_W * MAX_H];
static int16_t got_audio[MAX_PAIRS * 2u];
static size_t got_pairs;
static int content_test;
static unsigned content_polls;
static const char *bios_option_value = "none";
static const char *system_directory;
static const char *analog_mode_value = "analog";
static const char *analog_toggle_value = "enabled";
static int rumble_available;
struct rumble_event {
    unsigned port;
    enum retro_rumble_effect effect;
    uint16_t strength;
};
static struct rumble_event rumble_events[128];
static size_t rumble_event_count;

/* got must equal want; prints both. */
static void check_string(const char *what, const char *got, const char *want)
{
    if (got == NULL || strcmp(got, want) != 0) {
        fprintf(stderr, "%s: got \"%s\", expected \"%s\"\n", what, got != NULL ? got : "(null)",
                want);
        check_failures++;
    }
}

/* The version 2 definitions (L7747) must hold the region and no-content BIOS
   options, each with its default first and exact values. */
static void check_options_v2(const struct retro_core_options_v2 *o)
{
    const struct retro_core_option_v2_definition *d = o != NULL ? o->definitions : NULL;
    CHECK(d != NULL);
    if (d == NULL) {
        return;
    }
    check_string("option key", d[0].key, "schwaystation_region");
    check_string("option default", d[0].default_value, "ntsc-u");
    check_string("option value 0", d[0].values[0].value, "ntsc-u");
    check_string("option value 1", d[0].values[1].value, "ntsc-j");
    check_string("option value 2", d[0].values[2].value, "pal");
    CHECK(d[0].values[3].value == NULL);
    check_string("BIOS option key", d[1].key, "schwaystation_bios");
    check_string("BIOS option default", d[1].default_value, "none");
    check_string("BIOS option value 0", d[1].values[0].value, "none");
    check_string("BIOS option value 1", d[1].values[1].value, "scph5500.bin");
    check_string("BIOS option value 2", d[1].values[2].value, "scph5501.bin");
    check_string("BIOS option value 3", d[1].values[3].value, "scph5502.bin");
    CHECK(d[1].values[4].value == NULL);
    check_string("analog mode option key", d[2].key, "schwaystation_analog_mode");
    check_string("analog mode option default", d[2].default_value, "analog");
    check_string("analog mode value 0", d[2].values[0].value, "analog");
    check_string("analog mode value 1", d[2].values[1].value, "digital");
    CHECK(d[2].values[2].value == NULL);
    check_string("analog toggle option key", d[3].key, "schwaystation_analog_toggle");
    check_string("analog toggle option default", d[3].default_value, "enabled");
    check_string("analog toggle value 0", d[3].values[0].value, "enabled");
    check_string("analog toggle value 1", d[3].values[1].value, "disabled");
    CHECK(d[3].values[2].value == NULL);
    CHECK(d[4].key == NULL);
}

/* The version 0 variables (L7483) carry both options. */
static void check_variables(const struct retro_variable *v)
{
    CHECK(v != NULL);
    if (v == NULL) {
        return;
    }
    check_string("variable key", v[0].key, "schwaystation_region");
    check_string("variable value", v[0].value, "Region; ntsc-u|ntsc-j|pal");
    check_string("BIOS variable key", v[1].key, "schwaystation_bios");
    check_string("BIOS variable value",
                 v[1].value,
                 "BIOS for no-content runs; none|scph5500.bin|scph5501.bin|scph5502.bin");
    check_string("analog mode variable key", v[2].key, "schwaystation_analog_mode");
    check_string("analog mode variable value", v[2].value, "Analog mode; analog|digital");
    check_string("analog toggle variable key", v[3].key, "schwaystation_analog_toggle");
    check_string("analog toggle variable value", v[3].value, "Analog toggle; enabled|disabled");
    CHECK(v[4].key == NULL);
}

static bool RETRO_CALLCONV rumble(unsigned port, enum retro_rumble_effect effect, uint16_t strength)
{
    if (rumble_event_count < sizeof rumble_events / sizeof rumble_events[0]) {
        rumble_events[rumble_event_count].port = port;
        rumble_events[rumble_event_count].effect = effect;
        rumble_events[rumble_event_count].strength = strength;
    }
    rumble_event_count++;
    return true;
}

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
    case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
        if (pal) {
            return false;
        }
        *(unsigned *)data = 2u;
        return true;
    case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2:
        options_v2_calls++;
        check_options_v2((const struct retro_core_options_v2 *)data);
        return true;
    case RETRO_ENVIRONMENT_SET_VARIABLES:
        variables_calls++;
        check_variables((const struct retro_variable *)data);
        return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE: {
        struct retro_variable *var = (struct retro_variable *)data;
        get_variable_calls++;
        if (var == NULL || var->key == NULL) {
            return false;
        }
        if (strcmp(var->key, "schwaystation_region") == 0) {
            var->value = option_value;
            return true;
        }
        if (strcmp(var->key, "schwaystation_bios") == 0) {
            var->value = bios_option_value;
            return true;
        }
        if (strcmp(var->key, "schwaystation_analog_mode") == 0) {
            var->value = analog_mode_value;
            return true;
        }
        if (strcmp(var->key, "schwaystation_analog_toggle") == 0) {
            var->value = analog_toggle_value;
            return true;
        }
        return false;
    }
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
        if (system_directory == NULL) {
            return false;
        }
        *(const char **)data = system_directory;
        return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        *(bool *)data = option_updated;
        option_updated = false;
        return true;
    case RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE:
        if (!rumble_available || data == NULL) {
            return false;
        }
        ((struct retro_rumble_interface *)data)->set_rumble_state = rumble;
        return true;
    case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO:
        av_info_calls++;
        memcpy(&av_info_sent, data, sizeof av_info_sent);
        return true;
    case RETRO_ENVIRONMENT_SET_GEOMETRY:
        geometry_calls++;
        memcpy(&geometry_sent, data, sizeof geometry_sent);
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

/* Both audio callbacks append to one buffer per frame; pairs past its end
   are counted and not kept. */
static void RETRO_CALLCONV sample(int16_t left, int16_t right)
{
    if (got_pairs < MAX_PAIRS) {
        got_audio[2u * got_pairs] = left;
        got_audio[2u * got_pairs + 1u] = right;
    }
    got_pairs++;
}

static size_t RETRO_CALLCONV batch(const int16_t *data, size_t frames)
{
    size_t i;
    for (i = 0; i < frames; i++) {
        sample(data[2u * i], data[2u * i + 1u]);
    }
    return frames;
}

static void RETRO_CALLCONV on_poll(void)
{
    if (content_test) {
        content_polls++;
    }
}

static int16_t RETRO_CALLCONV input(unsigned port, unsigned device, unsigned index, unsigned id)
{
    if (content_test && port == 0u && device == RETRO_DEVICE_JOYPAD && index == 0u &&
        id == RETRO_DEVICE_ID_JOYPAD_B && content_polls == 60u) {
        return 1;
    }
    return 0;
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
    got_pairs = 0u;
    p_run();
}

/* One frame of the module and of the reference instance: the frame must be
   the reference's conversion, and the audio exactly the pairs the reference
   drains for that frame, lo or lo + 1 of them. */
static void step(schway *ref, const char *what, unsigned lo)
{
    static int16_t ref_audio[MAX_PAIRS * 2u];
    schway_audio io;
    CHECK_EQ_U64(schway_run_frame(ref, NULL), SCHWAY_STATUS_OK);
    memset(&io, 0, sizeof io);
    io.size = (uint32_t)sizeof io;
    io.samples = ref_audio;
    io.capacity = MAX_PAIRS;
    CHECK_EQ_U64(schway_get_audio(ref, &io), SCHWAY_STATUS_OK);
    run_one();
    ref_convert(ref);
    check_frame(what);
    if (got_pairs != io.written || (got_pairs != lo && got_pairs != lo + 1u) ||
        memcmp(got_audio, ref_audio, (size_t)io.written * 2u * sizeof ref_audio[0]) != 0) {
        fprintf(stderr, "%s: %lu audio pairs, reference %lu, expected %u or %u\n", what,
                (unsigned long)got_pairs, (unsigned long)io.written, lo, lo + 1u);
        check_failures++;
    }
}

/* Sets the frontend callbacks, as a frontend does after retro_init. */
static void set_callbacks(void)
{
    p_set_video_refresh(video);
    p_set_audio_sample(sample);
    p_set_audio_sample_batch(pal ? NULL : batch);
    p_set_input_poll(on_poll);
    p_set_input_state(input);
}

/* The AV info a region must report. The rates are compared with exact
   equality: the adapter must divide the core's own rationals, not round a
   constant. */
static void check_av(const struct retro_system_av_info *av, uint32_t region)
{
    CHECK_EQ_U64(av->geometry.base_width, 320u);
    CHECK_EQ_U64(av->geometry.base_height, 240u);
    CHECK_EQ_U64(av->geometry.max_width, 640u);
    CHECK_EQ_DBL(av->geometry.aspect_ratio, 4.0f / 3.0f);
    CHECK_EQ_DBL(av->timing.sample_rate, 44100.0);
    if (region == SCHWAY_REGION_PAL) {
        CHECK_EQ_U64(av->geometry.max_height, 576u);
        CHECK_EQ_DBL(av->timing.fps, 53203425.0 / 1069484.0);
    } else {
        CHECK_EQ_U64(av->geometry.max_height, 480u);
        CHECK_EQ_DBL(av->timing.fps, 53693175.0 / 897619.0);
    }
}

static int test_path(char *path, size_t capacity, const char *directory, const char *name)
{
    int written = snprintf(path, capacity, "%s/%s", directory, name);
    return written > 0 && (size_t)written < capacity;
}

static int write_test_file(const char *path, const void *bytes, size_t size)
{
    FILE *file = fopen(path, "wb");
    int ok;
    if (file == NULL) {
        return 0;
    }
    ok = fwrite(bytes, 1u, size, file) == size;
    if (fclose(file) != 0) {
        ok = 0;
    }
    return ok;
}

static unsigned char *save_libretro_state(size_t *size)
{
    unsigned char *state;
    *size = p_serialize_size();
    if (*size == 0u) {
        return NULL;
    }
    state = (unsigned char *)malloc(*size);
    if (state == NULL || !p_serialize(state, *size)) {
        free(state);
        return NULL;
    }
    return state;
}

static int libretro_state_matches(const unsigned char *saved, size_t size)
{
    unsigned char *current;
    int matches;
    if (saved == NULL || size == 0u || p_serialize_size() != size) {
        return 0;
    }
    current = (unsigned char *)malloc(size);
    if (current == NULL) {
        return 0;
    }
    matches = p_serialize(current, size) && memcmp(saved, current, size) == 0;
    free(current);
    return matches;
}

/* Selected BIOS failures and rejected content leave the current serialized
   instance intact; a selected BIOS applies only to a no-content load. */
static void content_lifecycle(const char *exe_path, const char *directory)
{
    char missing_bios[4096], short_bios[4096], valid_bios[4096];
    char wrong_extension[4096], short_exe[4096], missing_exe[4096];
    static const unsigned char short_image[] = {0x53u, 0x43u, 0x50u, 0x48u};
    struct retro_game_info game;
    unsigned char *bios_bytes = NULL, *state = NULL;
    size_t state_size = 0u;
    unsigned red_pixel_seen = 0u;
    int have_short_bios = 0, have_valid_bios = 0, have_short_exe = 0;

    CHECK(test_path(missing_bios, sizeof missing_bios, directory, "scph5500.bin"));
    CHECK(test_path(short_bios, sizeof short_bios, directory, "scph5501.bin"));
    CHECK(test_path(valid_bios, sizeof valid_bios, directory, "scph5502.bin"));
    CHECK(test_path(wrong_extension, sizeof wrong_extension, directory, "missing.bin"));
    CHECK(test_path(short_exe, sizeof short_exe, directory, "truncated.exe"));
    CHECK(test_path(missing_exe, sizeof missing_exe, directory, "missing.exe"));
    if (check_failures != 0) {
        return;
    }

    system_directory = directory;
    bios_option_value = "none";
    CHECK_EQ_U64(p_load_game(NULL), 1);
    state = save_libretro_state(&state_size);
    CHECK(state != NULL);
    if (state == NULL) {
        goto cleanup;
    }

    bios_option_value = "scph5500.bin";
    CHECK_EQ_U64(p_load_game(NULL), 0);
    CHECK(libretro_state_matches(state, state_size));
    bios_option_value = "scph5501.bin";
    have_short_bios = write_test_file(short_bios, short_image, sizeof short_image);
    CHECK(have_short_bios);
    CHECK_EQ_U64(p_load_game(NULL), 0);
    CHECK(libretro_state_matches(state, state_size));

    bios_bytes = (unsigned char *)calloc((size_t)SCHWAY_BIOS_SIZE, 1u);
    CHECK(bios_bytes != NULL);
    if (bios_bytes == NULL) {
        goto cleanup;
    }
    have_valid_bios = write_test_file(valid_bios, bios_bytes, (size_t)SCHWAY_BIOS_SIZE);
    CHECK(have_valid_bios);
    if (!have_valid_bios) {
        goto cleanup;
    }
    bios_option_value = "scph5502.bin";
    CHECK_EQ_U64(p_load_game(NULL), 1);
    CHECK(p_serialize_size() == state_size);
    if (p_serialize_size() == state_size) {
        unsigned char *bios_state = save_libretro_state(&state_size);
        CHECK(bios_state != NULL);
        if (bios_state != NULL) {
            CHECK(memcmp(state, bios_state, state_size) != 0);
        }
        free(state);
        state = bios_state;
    }

    /* Even with a valid BIOS option selected, an EXE must start from the
       stub path. The pad counter's guest-visible red pixel proves it ran. */
    memset(&game, 0, sizeof game);
    game.path = exe_path;
    content_polls = 0u;
    CHECK_EQ_U64(p_load_game(&game), 1);
    if (p_serialize_size() == 0u) {
        goto cleanup;
    }
    for (unsigned run = 1u; run <= 60u; run++) {
        run_one();
        if (frame_h > 100u && frame_w != 0u &&
            frame[(size_t)100u * frame_w] == 0x00FF0000u) {
            red_pixel_seen = 1u;
        }
    }
    CHECK_EQ_U64(red_pixel_seen, 1u);
    free(state);
    state = save_libretro_state(&state_size);
    CHECK(state != NULL);
    if (state == NULL) {
        goto cleanup;
    }

    game.path = wrong_extension;
    CHECK_EQ_U64(p_load_game(&game), 0);
    CHECK(libretro_state_matches(state, state_size));
    have_short_exe = write_test_file(short_exe, short_image, sizeof short_image);
    CHECK(have_short_exe);
    game.path = short_exe;
    CHECK_EQ_U64(p_load_game(&game), 0);
    CHECK(libretro_state_matches(state, state_size));
    game.path = missing_exe;
    CHECK_EQ_U64(p_load_game(&game), 0);
    CHECK(libretro_state_matches(state, state_size));

    /* A failed load after a frame ran also leaves the current instance intact. */
    run_one();
    free(state);
    state = save_libretro_state(&state_size);
    CHECK(state != NULL);
    game.path = wrong_extension;
    CHECK_EQ_U64(p_load_game(&game), 0);
    CHECK(libretro_state_matches(state, state_size));

    p_unload_game();
    CHECK_EQ_U64(p_serialize_size(), 0u);
    memset(&game, 0, sizeof game);
    game.path = exe_path;
    CHECK_EQ_U64(p_load_game(&game), 1);
    CHECK(p_serialize_size() > 0u);
    p_unload_game();
    bios_option_value = "none";
    CHECK_EQ_U64(p_load_game(NULL), 1);

cleanup:
    free(state);
    free(bios_bytes);
    if (have_short_bios) {
        (void)remove(short_bios);
    }
    if (have_valid_bios) {
        (void)remove(valid_bios);
    }
    if (have_short_exe) {
        (void)remove(short_exe);
    }
}

static void store_le32(unsigned char *bytes, size_t offset, uint32_t value)
{
    bytes[offset] = (unsigned char)value;
    bytes[offset + 1u] = (unsigned char)(value >> 8);
    bytes[offset + 2u] = (unsigned char)(value >> 16);
    bytes[offset + 3u] = (unsigned char)(value >> 24);
}

static void append_li(uint32_t *code, size_t *count, unsigned reg, uint32_t value)
{
    code[(*count)++] = 0x3C000000u | (reg << 16) | (value >> 16);
    if ((value & 0xFFFFu) != 0u) {
        code[(*count)++] = 0x34000000u | (reg << 21) | (reg << 16) | (value & 0xFFFFu);
    }
}

static void append_sw(uint32_t *code, size_t *count, unsigned reg, unsigned base, unsigned offset)
{
    code[(*count)++] = 0xAC000000u | (base << 21) | (reg << 16) | offset;
}

struct native_video_mode {
    unsigned width, height, start_x, start_y, frames;
    unsigned rgb24, interlaced, enabled;
};

static void append_gp1(uint32_t *code, size_t *count, uint32_t value)
{
    append_li(code, count, 9u, value);
    append_sw(code, count, 9u, 16u, 0x814u);
}

static void append_mode(uint32_t *code, size_t *count, const struct native_video_mode *mode)
{
    unsigned divider, resolution, mode_bits;
    unsigned horizontal_end, vertical_end;

    switch (mode->width) {
    case 256u:
        divider = 10u;
        resolution = 0u;
        break;
    case 320u:
        divider = 8u;
        resolution = 1u;
        break;
    case 512u:
        divider = 5u;
        resolution = 2u;
        break;
    default:
        divider = 4u;
        resolution = 3u;
        break;
    }
    horizontal_end = 0x260u + mode->width * divider;
    vertical_end = mode->height;
    mode_bits = resolution | (mode->height > 240u ? 4u : 0u) |
                (mode->rgb24 != 0u ? 0x10u : 0u) |
                (mode->interlaced != 0u ? 0x20u : 0u);

    append_gp1(code, count, 0x05000000u | (mode->start_y << 10) | mode->start_x);
    append_gp1(code, count, 0x06000000u | (horizontal_end << 12) | 0x260u);
    append_gp1(code, count, 0x07000000u | (vertical_end << 10));
    append_gp1(code, count, 0x08000000u | mode_bits);
    append_gp1(code, count, 0x03000000u | (mode->enabled != 0u ? 0u : 1u));

    append_li(code, count, 9u, 0xE3000000u);
    append_sw(code, count, 9u, 16u, 0x810u);
    append_li(code, count, 9u, 0xE407FFFFu);
    append_sw(code, count, 9u, 16u, 0x810u);
    append_li(code, count, 9u, 0x600000FFu);
    append_sw(code, count, 9u, 16u, 0x810u);
    append_li(code, count, 9u, (mode->start_y << 16) | mode->start_x);
    append_sw(code, count, 9u, 16u, 0x810u);
    append_li(code, count, 9u, 0x00010001u);
    append_sw(code, count, 9u, 16u, 0x810u);
}

static int write_native_video_exe_for_mode(const char *path, const struct native_video_mode *mode)
{
    unsigned char image[0x1000] = {0};
    uint32_t code[64];
    size_t count = 0u;

    code[count++] = 0x3C10BF80u; /* lui s0, 0xbf80 */
    code[count++] = 0x36101000u; /* ori s0, s0, 0x1000 */
    append_mode(code, &count, mode);
    code[count++] = 0x1000FFFFu; /* beq zero, zero, self */
    code[count++] = 0u;

    memcpy(image, "PS-X EXE", 8u);
    store_le32(image, 0x10u, 0x80010100u);
    store_le32(image, 0x18u, 0x80010000u);
    store_le32(image, 0x1Cu, 0x800u);
    store_le32(image, 0x30u, 0x801FF000u);
    store_le32(image, 0x34u, 0x1000u);
    for (size_t i = 0u; i < count; i++) {
        store_le32(image, 0x900u + 4u * i, code[i]);
    }
    return write_test_file(path, image, sizeof image);
}

/* Build a first-party guest that selects a native display mode, draws one
   red RGB555 pixel, and then stays in a guest loop. */
static int write_native_video_exe(const char *path)
{
    unsigned char image[0x1000] = {0};
    uint32_t code[64];
    size_t count = 0u;
    const unsigned io = 16u;
    const unsigned value = 9u;

    code[count++] = 0x3C10BF80u; /* lui s0, 0xbf80 */
    code[count++] = 0x36101000u; /* ori s0, s0, 0x1000 */
    append_li(code, &count, value, 0x03000001u); /* Display off while mode changes. */
    append_sw(code, &count, value, io, 0x814u);
    append_li(code, &count, value, 0x05000000u); /* Display start at VRAM (0, 0). */
    append_sw(code, &count, value, io, 0x814u);
    append_li(code, &count, value, 0x06C60260u); /* 640 dots at divider 4. */
    append_sw(code, &count, value, io, 0x814u);
    append_li(code, &count, value, 0x0703C000u); /* 240 native rows. */
    append_sw(code, &count, value, io, 0x814u);
    append_li(code, &count, value, 0x08000013u); /* 640x240, packed RGB24. */
    append_sw(code, &count, value, io, 0x814u);
    append_li(code, &count, value, 0x03000000u); /* Display on. */
    append_sw(code, &count, value, io, 0x814u);
    append_li(code, &count, value, 0xE3000000u); /* Draw area top-left. */
    append_sw(code, &count, value, io, 0x810u);
    append_li(code, &count, value, 0xE407FFFFu); /* Draw area covers VRAM. */
    append_sw(code, &count, value, io, 0x810u);
    append_li(code, &count, value, 0x600000FFu); /* Flat red 1x1 rectangle. */
    append_sw(code, &count, value, io, 0x810u);
    append_li(code, &count, value, 0u);
    append_sw(code, &count, value, io, 0x810u);
    append_li(code, &count, value, 0x00010001u);
    append_sw(code, &count, value, io, 0x810u);
    code[count++] = 0x1000FFFFu; /* beq zero, zero, self */
    code[count++] = 0u;

    memcpy(image, "PS-X EXE", 8u);
    store_le32(image, 0x10u, 0x80010100u);
    store_le32(image, 0x18u, 0x80010000u);
    store_le32(image, 0x1Cu, 0x800u);
    store_le32(image, 0x30u, 0x801FF000u);
    store_le32(image, 0x34u, 0x1000u);
    for (size_t i = 0u; i < count; i++) {
        store_le32(image, 0x900u + 4u * i, code[i]);
    }
    return write_test_file(path, image, sizeof image);
}

static void check_native_video_fixture(const char *path)
{
    unsigned char image[0x1000];
    FILE *file = fopen(path, "rb");
    schway_video native;
    schway *ref = ref_create(SCHWAY_REGION_NTSC_U);

    CHECK(file != NULL);
    CHECK(ref != NULL);
    if (file == NULL || ref == NULL) {
        if (file != NULL) {
            (void)fclose(file);
        }
        schway_destroy(ref);
        return;
    }
    CHECK(fread(image, 1u, sizeof image, file) == sizeof image);
    CHECK_EQ_U64(fclose(file), 0u);
    CHECK_EQ_U64(schway_load_exe(ref, image, sizeof image), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_run_frame(ref, NULL), SCHWAY_STATUS_OK);
    memset(&native, 0, sizeof native);
    native.size = (uint32_t)sizeof native;
    CHECK_EQ_U64(schway_get_video(ref, &native), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(native.bpp, 24u);
    CHECK_EQ_U64(native.width, 640u);
    CHECK_EQ_U64(native.height, 240u);
    if (native.pixels != NULL) {
        const unsigned char *pixels = (const unsigned char *)native.pixels;
        CHECK_EQ_U64(pixels[0], 0x1Fu);
        CHECK_EQ_U64(pixels[1], 0u);
        CHECK_EQ_U64(pixels[2], 0u);
    } else {
        CHECK(native.pixels != NULL);
    }
    schway_destroy(ref);
}

static int test_native_rgb24_geometry(const char *path)
{
    struct retro_game_info game;
    const unsigned video_before = video_calls;

    CHECK(write_native_video_exe(path));
    check_native_video_fixture(path);
    memset(&game, 0, sizeof game);
    game.path = path;
    CHECK_EQ_U64(p_load_game(&game), 1u);
    run_one();
    CHECK_EQ_U64(video_calls, video_before + 1u);
    CHECK_EQ_U64(video_bad, 0u);
    CHECK_EQ_U64(frame_w, 640u);
    CHECK_EQ_U64(frame_h, 240u);
    CHECK_EQ_U64(frame[0], 0x001F0000u);
    CHECK_EQ_U64(frame[1], 0u);
    CHECK_EQ_U64(geometry_calls, 1u);
    CHECK_EQ_U64(geometry_sent.base_width, 640u);
    CHECK_EQ_U64(geometry_sent.base_height, 240u);
    CHECK_EQ_U64(geometry_sent.max_width, 640u);
    CHECK_EQ_U64(av_info_calls, 0u);

    for (unsigned i = 0u; i < 2u; i++) {
        run_one();
        CHECK_EQ_U64(video_calls, video_before + 2u + i);
        CHECK_EQ_U64(frame[0], 0x001F0000u);
        CHECK_EQ_U64(geometry_calls, 1u);
        CHECK_EQ_U64(av_info_calls, 0u);
    }

    option_value = "pal";
    option_updated = true;
    run_one();
    CHECK_EQ_U64(av_info_calls, 1u);
    check_av(&av_info_sent, SCHWAY_REGION_PAL);
    CHECK_EQ_U64(geometry_calls, 1u);
    option_updated = true;
    run_one();
    CHECK_EQ_U64(av_info_calls, 1u);
    CHECK_EQ_U64(geometry_calls, 1u);

    p_unload_game();
    p_deinit();
    module_close();
    (void)remove(path);
    return check_failures != 0 ? 1 : 0;
}

static int test_native_video_geometry_edges(const char *path)
{
    static const struct native_video_mode modes[] = {
        {320u, 240u, 0u, 0u, 1u, 0u, 0u, 1u},
        {320u, 240u, 0u, 0u, 1u, 1u, 0u, 1u},
        {640u, 240u, 0u, 0u, 1u, 1u, 0u, 1u},
        {320u, 240u, 4u, 3u, 1u, 1u, 0u, 1u},
        {320u, 480u, 0u, 0u, 2u, 0u, 1u, 1u},
        {320u, 240u, 0u, 0u, 1u, 0u, 0u, 0u},
        {320u, 240u, 0u, 0u, 1u, 0u, 0u, 1u},
    };
    static const uint32_t expected_pixels[] = {
        0x00FF0000u, 0x001F0000u, 0x001F0000u, 0x001F0000u,
        0x00FF0000u, 0u,          0x00FF0000u,
    };
    struct retro_game_info game;
    for (size_t i = 0u; i < sizeof modes / sizeof modes[0]; i++) {
        unsigned video_before = video_calls;
        unsigned geometry_before = geometry_calls;
        unsigned changed_dimensions = modes[i].width != 320u || modes[i].height != 240u;

        CHECK(write_native_video_exe_for_mode(path, &modes[i]));
        memset(&game, 0, sizeof game);
        game.path = path;
        CHECK_EQ_U64(p_load_game(&game), 1u);
        for (unsigned frame_index = 0u; frame_index < modes[i].frames; frame_index++) {
            run_one();
            CHECK_EQ_U64(video_calls, video_before + frame_index + 1u);
            CHECK_EQ_U64(video_bad, 0u);
            CHECK_EQ_U64(frame_w, modes[i].width);
            CHECK_EQ_U64(frame_h, modes[i].height);
            CHECK_EQ_U64(frame[0], expected_pixels[i]);
            CHECK_EQ_U64(geometry_calls, geometry_before + changed_dimensions);
            CHECK_EQ_U64(av_info_calls, 0u);
        }
        if (changed_dimensions) {
            CHECK_EQ_U64(geometry_sent.base_width, modes[i].width);
            CHECK_EQ_U64(geometry_sent.base_height, modes[i].height);
        }
    }

    option_value = "pal";
    option_updated = true;
    run_one();
    CHECK_EQ_U64(av_info_calls, 1u);
    CHECK_EQ_U64(geometry_calls, 2u);
    check_av(&av_info_sent, SCHWAY_REGION_PAL);
    option_updated = true;
    run_one();
    CHECK_EQ_U64(av_info_calls, 1u);
    CHECK_EQ_U64(geometry_calls, 2u);

    p_unload_game();
    p_deinit();
    module_close();
    (void)remove(path);
    return check_failures != 0 ? 1 : 0;
}

static void append_sh(uint32_t *code, size_t *count, unsigned reg, unsigned base, unsigned offset)
{
    code[(*count)++] = 0xA4000000u | (base << 21) | (reg << 16) | offset;
}

static size_t append_branch(uint32_t *code, size_t *count, unsigned opcode, unsigned left,
                            unsigned right)
{
    size_t at = *count;
    code[(*count)++] = opcode | (left << 21) | (right << 16);
    return at;
}

static void patch_branch(uint32_t *code, size_t at, size_t target)
{
    int32_t displacement = (int32_t)target - (int32_t)(at + 1u);
    code[at] |= (uint16_t)displacement;
}

static void append_sio_tx(uint32_t *code, size_t *count, size_t *jal_sites, size_t *site_count,
                          unsigned value, unsigned ack)
{
    append_li(code, count, 4u, value); /* a0: byte to send */
    append_li(code, count, 5u, ack);   /* a1: wait for ACK */
    jal_sites[(*site_count)++] = *count;
    code[(*count)++] = 0x0C000000u;
    code[(*count)++] = 0u;
}

static void append_sio_command(uint32_t *code, size_t *count, size_t *jal_sites,
                               size_t *site_count, unsigned port, unsigned command,
                               const uint8_t payload[6])
{
    append_li(code, count, 8u, 0x0003u | (port != 0u ? 0x2000u : 0u));
    append_sh(code, count, 8u, 16u, 0x04Au);
    append_sio_tx(code, count, jal_sites, site_count, 0x01u, 1u);
    append_sio_tx(code, count, jal_sites, site_count, command, 1u);
    append_sio_tx(code, count, jal_sites, site_count, 0u, 1u);
    for (size_t i = 0u; i < 6u; i++) {
        append_sio_tx(code, count, jal_sites, site_count, payload[i], i != 5u ? 1u : 0u);
    }
}

/* A first-party PS-X EXE enters DualShock config, maps both motors, and polls
   both ports once per frame. It sends a fixed real 42h motor transaction. */
static int write_rumble_guest(const char *path, uint8_t small, uint8_t large)
{
    unsigned char image[0x1000] = {0};
    uint32_t code[512];
    size_t count = 0u, site_count = 0u, jal_sites[128];
    const uint8_t enter_config[6] = {1u, 0u, 0u, 0u, 0u, 0u};
    const uint8_t motor_map[6] = {0u, 1u, 0xFFu, 0xFFu, 0xFFu, 0xFFu};
    size_t frame_wait, rx_wait, ack_wait, release_wait, done, sio_entry;

    append_li(code, &count, 16u, 0xBF801000u); /* s0: hardware I/O */
    append_li(code, &count, 8u, 0x000Cu);
    append_sh(code, &count, 8u, 16u, 0x048u);
    append_li(code, &count, 8u, 0x0088u);
    append_sh(code, &count, 8u, 16u, 0x04Eu);
    append_li(code, &count, 8u, 0x0003u);
    append_sh(code, &count, 8u, 16u, 0x04Au);

    append_sio_command(code, &count, jal_sites, &site_count, 0u, 0x43u, enter_config);
    append_sio_command(code, &count, jal_sites, &site_count, 0u, 0x4Du, motor_map);
    append_sio_command(code, &count, jal_sites, &site_count, 1u, 0x43u, enter_config);
    append_sio_command(code, &count, jal_sites, &site_count, 1u, 0x4Du, motor_map);

    frame_wait = count;
    for (unsigned port = 0u; port < 2u; port++) {
        append_li(code, &count, 8u, 0x0003u | (port != 0u ? 0x2000u : 0u));
        append_sh(code, &count, 8u, 16u, 0x04Au);
        append_sio_tx(code, &count, jal_sites, &site_count, 0x01u, 1u);
        append_sio_tx(code, &count, jal_sites, &site_count, 0x42u, 1u);
        append_sio_tx(code, &count, jal_sites, &site_count, 0u, 1u);
        append_sio_tx(code, &count, jal_sites, &site_count, small, 1u);
        append_sio_tx(code, &count, jal_sites, &site_count, large, 1u);
        append_sio_tx(code, &count, jal_sites, &site_count, 0u, 1u);
        append_sio_tx(code, &count, jal_sites, &site_count, 0u, 1u);
        append_sio_tx(code, &count, jal_sites, &site_count, 0u, 1u);
        append_sio_tx(code, &count, jal_sites, &site_count, 0u, 0u);
    }
    size_t loop_branch = append_branch(code, &count, 0x10000000u, 0u, 0u);
    code[count++] = 0u;
    patch_branch(code, loop_branch, frame_wait);

    sio_entry = count;
    code[count++] = 0xA2040040u; /* sb a0, 40h(s0) */
    rx_wait = count;
    code[count++] = 0x96080044u; /* lhu t0, 44h(s0) */
    code[count++] = 0u;
    code[count++] = 0x31080002u; /* andi t0, t0, 2 */
    size_t rx_branch = append_branch(code, &count, 0x11000000u, 8u, 0u);
    code[count++] = 0u;
    code[count++] = 0x92020040u; /* lbu v0, 40h(s0) */
    size_t no_ack_branch = append_branch(code, &count, 0x10A00000u, 5u, 0u);
    code[count++] = 0u;
    ack_wait = count;
    code[count++] = 0x96080044u;
    code[count++] = 0u;
    code[count++] = 0x31080080u;
    size_t ack_branch = append_branch(code, &count, 0x11000000u, 8u, 0u);
    code[count++] = 0u;
    release_wait = count;
    code[count++] = 0x96080044u;
    code[count++] = 0u;
    code[count++] = 0x31080080u;
    size_t release_branch = append_branch(code, &count, 0x15000000u, 8u, 0u);
    code[count++] = 0u;
    done = count;
    code[count++] = 0x03E00008u; /* jr ra */
    code[count++] = 0u;
    patch_branch(code, rx_branch, rx_wait);
    patch_branch(code, no_ack_branch, done);
    patch_branch(code, ack_branch, ack_wait);
    patch_branch(code, release_branch, release_wait);
    for (size_t i = 0u; i < site_count; i++) {
        uint32_t target = 0x80010100u + (uint32_t)(sio_entry * sizeof code[0]);
        code[jal_sites[i]] |= (target >> 2) & 0x03FFFFFFu;
    }

    if (count * sizeof code[0] > 0x800u) {
        return 0;
    }
    memcpy(image, "PS-X EXE", 8u);
    store_le32(image, 0x10u, 0x80010100u);
    store_le32(image, 0x18u, 0x80010000u);
    store_le32(image, 0x1Cu, 0x800u);
    store_le32(image, 0x30u, 0x801FF000u);
    store_le32(image, 0x34u, 0x1000u);
    for (size_t i = 0u; i < count; i++) {
        store_le32(image, 0x900u + 4u * i, code[i]);
    }
    return write_test_file(path, image, sizeof image);
}

static void visit_rumble_reference(void *user, const char *name, const void *data, uint32_t count,
                                   uint32_t width)
{
    uint32_t *found = (uint32_t *)user;
    if (strcmp(name, "pad.device") == 0 && count == 2u && width == 1u) {
        found[0] = ((const uint8_t *)data)[0];
        found[1] = ((const uint8_t *)data)[1];
    }
}

static void check_rumble_events(size_t offset, uint16_t strong, uint16_t weak)
{
    CHECK(rumble_event_count >= offset + 4u);
    if (rumble_event_count < offset + 4u) {
        return;
    }
    CHECK_EQ_U64(rumble_events[offset].port, 0u);
    CHECK_EQ_U64(rumble_events[offset].effect, RETRO_RUMBLE_STRONG);
    CHECK_EQ_U64(rumble_events[offset].strength, strong);
    CHECK_EQ_U64(rumble_events[offset + 1u].port, 0u);
    CHECK_EQ_U64(rumble_events[offset + 1u].effect, RETRO_RUMBLE_WEAK);
    CHECK_EQ_U64(rumble_events[offset + 1u].strength, weak);
    CHECK_EQ_U64(rumble_events[offset + 2u].port, 1u);
    CHECK_EQ_U64(rumble_events[offset + 2u].effect, RETRO_RUMBLE_STRONG);
    CHECK_EQ_U64(rumble_events[offset + 2u].strength, strong);
    CHECK_EQ_U64(rumble_events[offset + 3u].port, 1u);
    CHECK_EQ_U64(rumble_events[offset + 3u].effect, RETRO_RUMBLE_WEAK);
    CHECK_EQ_U64(rumble_events[offset + 3u].strength, weak);
}

static int test_rumble_guest(const char *path)
{
    struct retro_game_info game;
    schway_input input_snapshot;
    schway_run_result result;
    schway *reference = ref_create(SCHWAY_REGION_NTSC_U);
    unsigned char exe[0x1000];
    FILE *file;
    uint32_t devices[2] = {0u, 0u};

    CHECK(reference != NULL);
    CHECK(write_rumble_guest(path, 1u, 255u));
    file = fopen(path, "rb");
    CHECK(file != NULL);
    if (reference == NULL || file == NULL) {
        schway_destroy(reference);
        if (file != NULL) {
            (void)fclose(file);
        }
        (void)remove(path);
        return 1;
    }
    CHECK_EQ_U64(fread(exe, 1u, sizeof exe, file), sizeof exe);
    CHECK_EQ_U64(fclose(file), 0u);
    memset(&input_snapshot, 0, sizeof input_snapshot);
    input_snapshot.size = (uint32_t)sizeof input_snapshot;
    input_snapshot.device[0] = SCHWAY_INPUT_DEVICE_DUALSHOCK;
    input_snapshot.device[1] = SCHWAY_INPUT_DEVICE_DUALSHOCK;
    for (unsigned port = 0u; port < 2u; port++) {
        for (unsigned axis = 0u; axis < 4u; axis++) {
            input_snapshot.axes[port][axis] = 128u;
        }
    }
    CHECK_EQ_U64(schway_load_exe(reference, exe, sizeof exe), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_set_input(reference, &input_snapshot), SCHWAY_STATUS_OK);
    for (unsigned frame_index = 0u; frame_index < 4u; frame_index++) {
        memset(&result, 0, sizeof result);
        result.size = (uint32_t)sizeof result;
        CHECK_EQ_U64(schway_run_frame(reference, &result), SCHWAY_STATUS_OK);
        if (frame_index >= 1u) {
            CHECK_EQ_HEX(result.rumble[0], 0xFF01u);
            CHECK_EQ_HEX(result.rumble[1], 0xFF01u);
        }
    }
    CHECK_EQ_U64(schway_state_visit(reference, visit_rumble_reference, devices), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(devices[0], SCHWAY_INPUT_DEVICE_DUALSHOCK);
    CHECK_EQ_U64(devices[1], SCHWAY_INPUT_DEVICE_DUALSHOCK);
    schway_destroy(reference);

    memset(&game, 0, sizeof game);
    game.path = path;
    rumble_event_count = 0u;
    CHECK_EQ_U64(p_load_game(&game), 1u);
    p_set_controller_port_device(0u, RETRO_DEVICE_ANALOG);
    p_set_controller_port_device(1u, RETRO_DEVICE_ANALOG);
    for (unsigned frame_index = 0u; frame_index < 4u; frame_index++) {
        size_t before = rumble_event_count;
        run_one();
        check_rumble_events(before, (uint16_t)(255u * 257u), UINT16_MAX);
    }
    CHECK_EQ_U64(rumble_event_count, 16u);
    p_unload_game();
    p_deinit();
    module_close();
    (void)remove(path);
    return check_failures != 0 ? 1 : 0;
}

int main(int argc, char **argv)
{
    struct retro_system_info sys;
    struct retro_system_av_info av;
    struct retro_game_info game;
    static const unsigned char one_byte[1] = {0};
    static uint32_t frame60[MAX_W * MAX_H];
    char version[32];
    schway_versions v;
    schway *ref;
    unsigned char *state, *ref_state;
    size_t size;
    unsigned run, lo, other_lo;
    uint32_t region, other;
    int content_mode = argc == 4 && strcmp(argv[2], "content-no-bios") == 0;
    int video_geometry_mode = argc == 4 && strcmp(argv[2], "video-geometry") == 0;
    int video_edges_mode = argc == 4 && strcmp(argv[2], "video-geometry-edges") == 0;
    int lifecycle_mode = argc == 5 && strcmp(argv[2], "content-lifecycle") == 0;
    int rumble_guest_mode = argc == 4 && strcmp(argv[2], "rumble-guest") == 0;

    if ((!content_mode && !video_geometry_mode && !video_edges_mode && !lifecycle_mode &&
         !rumble_guest_mode &&
         (argc != 3 || (strcmp(argv[2], "ntsc-u") != 0 && strcmp(argv[2], "pal") != 0))) ||
        ((content_mode || video_geometry_mode || video_edges_mode || lifecycle_mode ||
          rumble_guest_mode) && argv[3][0] == '\0') ||
        (lifecycle_mode && argv[4][0] == '\0')) {
        fprintf(stderr,
                "usage: schwaystation-test-libretro-host <module> ntsc-u|pal | content-no-bios <exe> | content-lifecycle <exe> <system-dir> | video-geometry <exe> | video-geometry-edges <exe> | rumble-guest <exe>\n");
        return 2;
    }
    content_test = content_mode || video_geometry_mode || video_edges_mode || lifecycle_mode;
    rumble_available = rumble_guest_mode;
    pal = !content_test && strcmp(argv[2], "pal") == 0;
    option_value = pal ? "pal" : "ntsc-u";
    region = pal ? SCHWAY_REGION_PAL : SCHWAY_REGION_NTSC_U;
    /* The region a changed option switches to, and both regions' pairs per
       frame: 33,868,800 x 44,100 / (768 x 44,100) per second over the frame
       rate (HOSTS-AND-INTEGRATION section 1, AV fields). */
    other = pal ? SCHWAY_REGION_NTSC_J : SCHWAY_REGION_PAL;
    lo = pal ? 886u : 737u;
    other_lo = pal ? 737u : 886u;
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

    /* No-game support and the options are declared before retro_init. */
    CHECK_EQ_U64(p_api_version(), RETRO_API_VERSION);
    p_set_environment(env);
    CHECK_EQ_U64(no_game_calls, 1);
    CHECK_EQ_U64(no_game_value, 1);
    CHECK_EQ_U64(options_v2_calls, pal ? 0u : 1u);
    CHECK_EQ_U64(variables_calls, pal ? 1u : 0u);
    p_init();

    memset(&sys, 0, sizeof sys);
    p_get_system_info(&sys);
    check_string("library_name", sys.library_name, "schwaystation");
    check_string("library_version", sys.library_version, version);
    if (!content_test) {
        check_string("valid_extensions", sys.valid_extensions, "exe");
    }
    CHECK_EQ_U64(sys.need_fullpath, 1);
    CHECK_EQ_U64(sys.block_extract, 1);
    set_callbacks();

    if (rumble_guest_mode) {
        return test_rumble_guest(argv[3]);
    }

    if (lifecycle_mode) {
        content_lifecycle(argv[3], argv[4]);
        p_unload_game();
        p_deinit();
        module_close();
        CHECK_DONE();
    }

    if (video_geometry_mode) {
        return test_native_rgb24_geometry(argv[3]);
    }

    if (video_edges_mode) {
        return test_native_video_geometry_edges(argv[3]);
    }

    if (content_mode) {
        unsigned red_pixel_seen = 0u;
        memset(&game, 0, sizeof game);
        game.path = argv[3];
        CHECK_EQ_U64(p_load_game(&game), 1);
        if (p_serialize_size() == 0u) {
            module_close();
            CHECK_DONE();
        }
        check_string("valid_extensions", sys.valid_extensions, "exe");
        for (run = 1u; run <= 60u; run++) {
            run_one();
            if (frame_h > 100u && frame_w != 0u && frame[(size_t)100u * frame_w] == 0x00FF0000u) {
                red_pixel_seen = 1u;
            }
        }
        CHECK_EQ_U64(video_calls, 60u);
        CHECK_EQ_U64(video_bad, 0u);
        CHECK_EQ_U64(red_pixel_seen, 1u);
        CHECK_EQ_U64(geometry_calls, 1u);
        CHECK_EQ_U64(geometry_sent.base_width, frame_w);
        CHECK_EQ_U64(geometry_sent.base_height, frame_h);
        p_unload_game();
        CHECK_EQ_U64(p_serialize_size(), 0u);
        p_deinit();
        module_close();
        CHECK_DONE();
    }

    /* The selected content test is kept separate; normal modes exercise the
       no-content test-card contract below. */
    memset(&game, 0, sizeof game);
    game.path = "game.cue";
    game.data = one_byte;
    game.size = sizeof one_byte;
    CHECK_EQ_U64(p_load_game(&game), 0);
    CHECK_EQ_U64(p_load_game_special(0, &game, 1), 0);
    CHECK_EQ_U64(pixel_format_calls, 0);
    CHECK_EQ_U64(p_serialize_size(), 0);

    /* No content: the pixel format is set before load returns (L870), and
       the region option is read. */
    CHECK_EQ_U64(p_load_game(NULL), 1);
    CHECK_EQ_U64(pixel_format_calls, 1);
    CHECK_EQ_U64(pixel_format, RETRO_PIXEL_FORMAT_XRGB8888);
    CHECK(get_variable_calls >= 1u);
    CHECK_EQ_U64(p_get_region(), pal ? RETRO_REGION_PAL : RETRO_REGION_NTSC);
    memset(&av, 0, sizeof av);
    p_get_system_av_info(&av);
    check_av(&av, region);

    /* No memory regions in this release. */
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SAVE_RAM), 0);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SYSTEM_RAM), 0);
    CHECK(p_get_memory_data(RETRO_MEMORY_SAVE_RAM) == NULL);
    p_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
    p_cheat_reset();
    p_cheat_set(0, true, "");

    ref = ref_create(region);
    CHECK(ref != NULL);
    if (ref == NULL) {
        module_close();
        return 1;
    }

    /* 60 frames, one video call each, each poisoned first and each equal to
       the reference's frame and audio. A state is saved at frame 30. */
    size = 0u;
    state = ref_state = NULL;
    for (run = 1; run <= FRAMES; run++) {
        step(ref, "frame", lo);
        if (run == SAVE_AT) {
            size = p_serialize_size();
            CHECK(size > 0u);
            CHECK_EQ_U64(size, schway_state_size(ref));
            state = (unsigned char *)malloc(size);
            ref_state = (unsigned char *)malloc(size);
            CHECK(state != NULL && ref_state != NULL);
            if (state == NULL || ref_state == NULL) {
                free(state);
                free(ref_state);
                schway_destroy(ref);
                module_close();
                return 1;
            }
            CHECK_EQ_U64(p_serialize(state, size - 1u), 0);
            CHECK_EQ_U64(p_serialize(state, size), 1);
            CHECK_EQ_U64(schway_save_state(ref, ref_state, size), SCHWAY_STATUS_OK);
            CHECK(memcmp(state, ref_state, size) == 0);
        }
    }
    CHECK_EQ_U64(video_calls, FRAMES);
    CHECK_EQ_U64(video_bad, 0);
    memcpy(frame60, frame, sizeof frame60);

    /* Unserialize to frame 30 and run to 60 again: the same frames and audio,
       and the same 60th frame. A damaged state is refused. */
    if (state != NULL && ref_state != NULL) {
        state[size - 1u] ^= 0xFFu;
        CHECK_EQ_U64(p_unserialize(state, size), 0);
        state[size - 1u] ^= 0xFFu;
        CHECK_EQ_U64(p_unserialize(state, size), 1);
        CHECK_EQ_U64(schway_load_state(ref, ref_state, size), SCHWAY_STATUS_OK);
        for (run = SAVE_AT + 1u; run <= FRAMES; run++) {
            step(ref, "replayed frame", lo);
        }
        CHECK(memcmp(frame, frame60, sizeof frame60) == 0);
    }
    free(state);
    free(ref_state);

    /* A power cycle, on both. */
    p_reset();
    CHECK_EQ_U64(schway_reset(ref, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    step(ref, "frame after reset", lo);

    /* An update that leaves the region alone keeps the instance. */
    option_updated = true;
    step(ref, "frame after an unchanged option", lo);
    CHECK_EQ_U64(av_info_calls, 0);

    /* A new region recreates the instance from power-on and sends the new
       AV info once. */
    schway_destroy(ref);
    ref = ref_create(other);
    CHECK(ref != NULL);
    if (ref == NULL) {
        module_close();
        return 1;
    }
    option_value = other == SCHWAY_REGION_PAL ? "pal" : "ntsc-j";
    option_updated = true;
    step(ref, "frame after a region change", other_lo);
    CHECK_EQ_U64(av_info_calls, 1);
    check_av(&av_info_sent, other);
    CHECK_EQ_U64(p_get_region(), other == SCHWAY_REGION_PAL ? RETRO_REGION_PAL : RETRO_REGION_NTSC);
    memset(&av, 0, sizeof av);
    p_get_system_av_info(&av);
    check_av(&av, other);
    schway_destroy(ref);

    /* Unload, deinit and start again with the mode's region: no state
       survives, so the first frame is the reference's first. */
    p_unload_game();
    CHECK_EQ_U64(p_serialize_size(), 0);
    p_deinit();
    no_game_calls = 0;
    pixel_format_calls = 0;
    option_value = argv[2];
    p_set_environment(env);
    p_init();
    set_callbacks();
    CHECK_EQ_U64(no_game_calls, 1);
    CHECK_EQ_U64(p_load_game(NULL), 1);
    CHECK_EQ_U64(pixel_format_calls, 1);
    ref = ref_create(region);
    CHECK(ref != NULL);
    if (ref != NULL) {
        step(ref, "first frame after deinit", lo);
        schway_destroy(ref);
    }

    p_unload_game();
    p_deinit();
    module_close();
    CHECK_DONE();
}
