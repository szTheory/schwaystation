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
 * audio-equality modes run batch or single callbacks for NTSC-J, NTSC-U and
 * PAL. The regular ntsc-u and pal modes retain the options-version and legacy
 * SET_VARIABLES checks. "Lnnn" is a line of the vendored libretro.h. */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#define rmdir _rmdir
#else
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../test_open.h"

#include "../check.h"
#include "libretro.h"
#include "schwaystation.h"
#include "disc_image.h"
#include "disc_probe.h"
#include "region.h"
#include "xrgb8888.h"

#ifndef SCHWAYSTATION_CHD
#define SCHWAYSTATION_CHD 0
#endif

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
static const uint8_t *(RETRO_CALLCONV *p_test_card2_data)(size_t *);
static const char *(RETRO_CALLCONV *p_test_card2_path)(void);
static const uint8_t *(RETRO_CALLCONV *p_test_card_socket)(unsigned, unsigned, size_t *,
                                                            uint64_t *, uint64_t *,
                                                            const char **);
static int (RETRO_CALLCONV *p_test_write_card_socket)(unsigned, unsigned, uint8_t);
static void (RETRO_CALLCONV *p_test_flush_cards)(void);
static int (RETRO_CALLCONV *p_test_card_lifecycle)(unsigned, const char **, uint64_t *);
static int (RETRO_CALLCONV *p_test_card_read_begin)(unsigned);
static int (RETRO_CALLCONV *p_test_card_read_byte)(unsigned, uint8_t, int, uint8_t *);
static int(RETRO_CALLCONV *p_test_poll_endpoint)(unsigned, unsigned, uint16_t *);
static int(RETRO_CALLCONV *p_test_poll_bytes)(unsigned, unsigned, uint8_t *, size_t, size_t *);
static int(RETRO_CALLCONV *p_test_gun_input)(unsigned, schway_gun_input *);

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

static void resolve_all(int inspect_card2)
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
    if (inspect_card2) {
        RESOLVE(p_test_card2_data, "schway__libretro_test_card2_data");
        RESOLVE(p_test_card2_path, "schway__libretro_test_card2_path");
        RESOLVE(p_test_card_socket, "schway__libretro_test_card_socket");
        RESOLVE(p_test_write_card_socket, "schway__libretro_test_write_card_socket");
        RESOLVE(p_test_flush_cards, "schway__libretro_test_flush_cards");
        RESOLVE(p_test_card_lifecycle, "schway__libretro_test_card_lifecycle");
        RESOLVE(p_test_card_read_begin, "schway__libretro_test_card_read_begin");
        RESOLVE(p_test_card_read_byte, "schway__libretro_test_card_read_byte");
        RESOLVE(p_test_poll_endpoint, "schway__libretro_test_poll_endpoint");
        RESOLVE(p_test_poll_bytes, "schway__libretro_test_poll_bytes");
        RESOLVE(p_test_gun_input, "schway__libretro_test_gun_input");
    }
}

/* ---- frontend callbacks: record what the core does ---- */

static int pal; /* the mode: 0 ntsc-u, 1 pal */
static int single_audio; /* selects the frontend's audio_sample fallback */
static int no_game_calls, no_game_value;
static int pixel_format_calls;
static enum retro_pixel_format pixel_format = RETRO_PIXEL_FORMAT_0RGB1555;
static unsigned options_v2_calls, variables_calls, get_variable_calls;
static const char *option_value; /* what GET_VARIABLE answers */
static const char *machine_option_value = "pu18";
static const char *boot_option_value = "full";
static bool option_updated;      /* what GET_VARIABLE_UPDATE answers, once */
static char log_message[512];
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
static int input_movie_mode;
static unsigned input_movie_variant;
static int input_controls_active;
static unsigned input_controls_chord_until;
static int input_controls_chord_enabled;
static unsigned input_poll_calls, input_state_calls;
static unsigned controller_info_calls, input_descriptor_calls;
static const struct retro_input_descriptor *published_input_descriptors;
static struct retro_disk_control_ext_callback disk_control;
static unsigned disk_control_set_calls;
static unsigned disk_control_unregister_calls;
static const char *bios_option_value = "none";
static const char *system_directory;
static const char *save_directory;
static int save_directory_available = 1;
static unsigned save_directory_calls;
static int memory_maps_available = 1;
static unsigned memory_map_calls;
#define MEMORY_DESCRIPTOR_CAP 16u
static struct retro_memory_descriptor memory_descriptors[MEMORY_DESCRIPTOR_CAP];
static char memory_addrspaces[MEMORY_DESCRIPTOR_CAP][64];
static unsigned memory_descriptor_count;
static const char *analog_mode_value = "analog";
static const char *analog_toggle_value = "enabled";
static const char *multitap_port1_value = "disabled";
static const char *multitap_port2_value = "disabled";
static int multitap_test_mode;
static unsigned multitap_callback_visits[8];
static unsigned multitap_analog_visits[8];
static unsigned multitap_invalid_player_visits;
static int special_input_mode;
static int16_t special_mouse_x, special_mouse_y;
static int16_t special_lightgun_x, special_lightgun_y;
static int16_t special_axes[2][2];
static uint32_t special_buttons;
static unsigned special_queries[8][8][3][17];
static unsigned input_capabilities_calls;
static int input_capabilities_available = 1;
static uint64_t input_capabilities = UINT64_C(1) << RETRO_DEVICE_LIGHTGUN;
static int rumble_available;
static int rumble_axis_mode;
static unsigned audio_tap_checks;
static unsigned audio_tap_failures;
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

static const char *expected_valid_extensions(void)
{
#if SCHWAYSTATION_CHD
    return "exe|psexe|cue|iso|m3u|pbp|ecm|ccd|chd";
#else
    return "exe|psexe|cue|iso|m3u|pbp|ecm|ccd";
#endif
}

/* The version 2 definitions (L7747) must expose separate board and region
   choices plus the no-content BIOS option. */
static void check_options_v2(const struct retro_core_options_v2 *o)
{
    const struct retro_core_option_v2_definition *d = o != NULL ? o->definitions : NULL;
    CHECK(d != NULL);
    if (d == NULL) {
        return;
    }
    check_string("machine option key", d[0].key, "schwaystation_machine");
    check_string("machine option default", d[0].default_value, "pu18");
    static const char *const machines[] = {
        "pu18", "pu7", "pu8-early", "pu8-late", "pu20", "pu22", "pu23", "pm41", "pm41-2"};
    for (unsigned i = 0u; i < sizeof machines / sizeof machines[0]; i++) {
        check_string("machine option token", d[0].values[i].value, machines[i]);
    }
    CHECK(d[0].values[sizeof machines / sizeof machines[0]].value == NULL);
    check_string("region option key", d[1].key, "schwaystation_region");
    check_string("region option default", d[1].default_value, "auto");
    check_string("region option auto", d[1].values[0].value, "auto");
    check_string("region option NTSC-U", d[1].values[1].value, "ntsc-u");
    check_string("region option NTSC-J", d[1].values[2].value, "ntsc-j");
    check_string("region option PAL", d[1].values[3].value, "pal");
    CHECK(d[1].values[4].value == NULL);
    check_string("BIOS option key", d[2].key, "schwaystation_bios");
    check_string("BIOS option default", d[2].default_value, "none");
    check_string("BIOS option value 0", d[2].values[0].value, "none");
    check_string("BIOS option value 1", d[2].values[1].value, "scph5500.bin");
    check_string("BIOS option value 2", d[2].values[2].value, "scph5501.bin");
    check_string("BIOS option value 3", d[2].values[3].value, "scph5502.bin");
    CHECK(d[2].values[4].value == NULL);
    check_string("boot option key", d[3].key, "schwaystation_boot");
    check_string("boot option default", d[3].default_value, "full");
    check_string("boot option full", d[3].values[0].value, "full");
    check_string("boot option fast", d[3].values[1].value, "fast");
    CHECK(d[3].values[2].value == NULL);
    check_string("analog mode option key", d[4].key, "schwaystation_analog_mode");
    check_string("analog mode option default", d[4].default_value, "analog");
    check_string("analog mode value 0", d[4].values[0].value, "analog");
    check_string("analog mode value 1", d[4].values[1].value, "digital");
    CHECK(d[4].values[2].value == NULL);
    check_string("analog toggle option key", d[5].key, "schwaystation_analog_toggle");
    check_string("analog toggle option default", d[5].default_value, "enabled");
    check_string("analog toggle value 0", d[5].values[0].value, "enabled");
    check_string("analog toggle value 1", d[5].values[1].value, "disabled");
    CHECK(d[5].values[2].value == NULL);
    if (d[6].key == NULL) {
        CHECK(d[6].key != NULL);
        return;
    }
    check_string("port 1 multitap option key", d[6].key, "schwaystation_multitap_port1");
    check_string("port 1 multitap option default", d[6].default_value, "disabled");
    check_string("port 1 multitap option disabled", d[6].values[0].value, "disabled");
    check_string("port 1 multitap option enabled", d[6].values[1].value, "enabled");
    CHECK(d[6].values[2].value == NULL);
    check_string("port 2 multitap option key", d[7].key, "schwaystation_multitap_port2");
    check_string("port 2 multitap option default", d[7].default_value, "disabled");
    check_string("port 2 multitap option disabled", d[7].values[0].value, "disabled");
    check_string("port 2 multitap option enabled", d[7].values[1].value, "enabled");
    CHECK(d[7].values[2].value == NULL);
    CHECK(d[8].key == NULL);
}

/* The version 0 variables (L7483) carry both options. */
static void check_variables(const struct retro_variable *v)
{
    CHECK(v != NULL);
    if (v == NULL) {
        return;
    }
    check_string("machine variable key", v[0].key, "schwaystation_machine");
    check_string("machine variable value", v[0].value,
                 "Machine (reload content); pu18|pu7|pu8-early|pu8-late|pu20|pu22|pu23|pm41|pm41-2");
    check_string("region variable key", v[1].key, "schwaystation_region");
    check_string("region variable value", v[1].value, "Region (reload content); auto|ntsc-u|ntsc-j|pal");
    check_string("BIOS variable key", v[2].key, "schwaystation_bios");
    check_string("BIOS variable value",
                 v[2].value,
                 "BIOS for discs and no-content runs; none|scph5500.bin|scph5501.bin|scph5502.bin");
    check_string("boot variable key", v[3].key, "schwaystation_boot");
    check_string("boot variable value", v[3].value, "Boot (reload content); full|fast");
    check_string("analog mode variable key", v[4].key, "schwaystation_analog_mode");
    check_string("analog mode variable value", v[4].value, "DualShock mode; analog|digital");
    check_string("analog toggle variable key", v[5].key, "schwaystation_analog_toggle");
    check_string("analog toggle variable value", v[5].value,
                 "DualShock mode toggle; enabled|disabled");
    if (v[6].key == NULL) {
        CHECK(v[6].key != NULL);
        return;
    }
    check_string("port 1 multitap variable key", v[6].key, "schwaystation_multitap_port1");
    check_string("port 1 multitap variable value", v[6].value,
                 "Multitap on controller port 1 (reload content); disabled|enabled");
    check_string("port 2 multitap variable key", v[7].key, "schwaystation_multitap_port2");
    check_string("port 2 multitap variable value", v[7].value,
                 "Multitap on controller port 2 (reload content); disabled|enabled");
    CHECK(v[8].key == NULL);
}

static unsigned expected_active_players(void)
{
    return 2u + (strcmp(multitap_port1_value, "enabled") == 0 ? 3u : 0u) +
           (strcmp(multitap_port2_value, "enabled") == 0 ? 3u : 0u);
}

static int expected_player_has_gun(unsigned player)
{
    unsigned port1_players = strcmp(multitap_port1_value, "enabled") == 0 ? 4u : 1u;
    if (player < port1_players) {
        return player == 0u && strcmp(multitap_port1_value, "enabled") != 0;
    }
    return player == port1_players && strcmp(multitap_port2_value, "enabled") != 0;
}

static void check_controller_info(const struct retro_controller_info *info)
{
    static const unsigned base_ids[] = {
        RETRO_DEVICE_ANALOG,
        RETRO_DEVICE_JOYPAD,
        RETRO_DEVICE_NONE,
        RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG, 1),
        RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG, 2),
        RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG, 3),
        RETRO_DEVICE_MOUSE,
    };
    static const char *base_names[] = {
        "DualShock", "PlayStation Controller", "None", "Dual Analog Flight Stick",
        "neGcon", "Jogcon", "Mouse",
    };
    static const unsigned gun_ids[] = {
        RETRO_DEVICE_LIGHTGUN,
        RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_LIGHTGUN, 2),
    };
    static const char *gun_names[] = {"GunCon", "Justifier"};
    controller_info_calls++;
    CHECK(info != NULL);
    if (info == NULL) {
        return;
    }
    unsigned active_players = expected_active_players();
    for (unsigned port = 0u; port < active_players; port++) {
        unsigned gun_count = expected_player_has_gun(port) ? 2u : 0u;
        CHECK_EQ_U64(info[port].num_types,
                     sizeof base_ids / sizeof base_ids[0] + gun_count);
        CHECK(info[port].types != NULL);
        if (info[port].types == NULL) {
            continue;
        }
        for (unsigned i = 0u; i < sizeof base_ids / sizeof base_ids[0]; i++) {
            CHECK_EQ_U64(info[port].types[i].id, base_ids[i]);
            check_string("controller type name", info[port].types[i].desc, base_names[i]);
        }
        for (unsigned i = 0u; i < gun_count; i++) {
            CHECK_EQ_U64(info[port].types[7u + i].id, gun_ids[i]);
            check_string("controller type name", info[port].types[7u + i].desc, gun_names[i]);
        }
    }
    CHECK(info[active_players].types == NULL);
}

#define EXPECTED_PAD_DESCRIPTORS(port_)                                                              \
    {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_B, "Cross"},                           \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_A, "Circle"},                     \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_Y, "Square"},                     \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_X, "Triangle"},                   \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_L, "L1"},                         \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_R, "R1"},                         \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_L2, "L2"},                       \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_R2, "R2"},                       \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_L3, "L3"},                       \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_R3, "R3"},                       \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select"},               \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_START, "Start"},                 \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_UP, "Up"},                       \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right"},                 \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down"},                   \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left"},                   \
        {port_, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X,     \
         "Left Stick X"},                                                                        \
        {port_, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y,     \
         "Left Stick Y"},                                                                        \
        {port_, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X,    \
         "Right Stick X"},                                                                       \
        {port_, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y,    \
         "Right Stick Y"}

static void validate_input_descriptors(const struct retro_input_descriptor *descriptors)
{
    static const struct retro_input_descriptor expected_pad[] = {
        EXPECTED_PAD_DESCRIPTORS(0u),
    };
    static const struct retro_input_descriptor expected_gun[] = {
        {0u, RETRO_DEVICE_LIGHTGUN, 0u, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X, "Screen X"},
        {0u, RETRO_DEVICE_LIGHTGUN, 0u, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y, "Screen Y"},
        {0u, RETRO_DEVICE_LIGHTGUN, 0u, RETRO_DEVICE_ID_LIGHTGUN_TRIGGER, "Trigger"},
        {0u, RETRO_DEVICE_LIGHTGUN, 0u, RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN, "Offscreen"},
        {0u, RETRO_DEVICE_LIGHTGUN, 0u, RETRO_DEVICE_ID_LIGHTGUN_RELOAD, "Reload"},
        {0u, RETRO_DEVICE_LIGHTGUN, 0u, RETRO_DEVICE_ID_LIGHTGUN_AUX_A, "GunCon A"},
        {0u, RETRO_DEVICE_LIGHTGUN, 0u, RETRO_DEVICE_ID_LIGHTGUN_AUX_B, "GunCon B"},
        {0u, RETRO_DEVICE_LIGHTGUN, 0u, RETRO_DEVICE_ID_LIGHTGUN_START, "Justifier Start"},
        {0u, RETRO_DEVICE_LIGHTGUN, 0u, RETRO_DEVICE_ID_LIGHTGUN_SELECT, "Justifier Back"},
    };
    unsigned active_players = expected_active_players();
    unsigned per_player = (unsigned)(sizeof expected_pad / sizeof expected_pad[0]);
    unsigned per_gun = (unsigned)(sizeof expected_gun / sizeof expected_gun[0]);
    unsigned descriptor_count = 0u;
    CHECK(descriptors != NULL);
    if (descriptors == NULL) {
        return;
    }
    for (unsigned player = 0u; player < active_players; player++) {
        for (unsigned item = 0u; item < per_player; item++) {
            const struct retro_input_descriptor *actual = &descriptors[descriptor_count++];
            CHECK(actual->description != NULL);
            if (actual->description == NULL) {
                return;
            }
            CHECK_EQ_U64(actual->port, player);
            CHECK_EQ_U64(actual->device, expected_pad[item].device);
            CHECK_EQ_U64(actual->index, expected_pad[item].index);
            CHECK_EQ_U64(actual->id, expected_pad[item].id);
            check_string("input descriptor", actual->description, expected_pad[item].description);
        }
        if (expected_player_has_gun(player)) {
            for (unsigned item = 0u; item < per_gun; item++) {
                const struct retro_input_descriptor *actual = &descriptors[descriptor_count++];
                CHECK(actual->description != NULL);
                if (actual->description == NULL) {
                    return;
                }
                CHECK_EQ_U64(actual->port, player);
                CHECK_EQ_U64(actual->device, expected_gun[item].device);
                CHECK_EQ_U64(actual->index, expected_gun[item].index);
                CHECK_EQ_U64(actual->id, expected_gun[item].id);
                check_string("lightgun input descriptor", actual->description,
                             expected_gun[item].description);
            }
        }
    }
    CHECK(descriptors[descriptor_count].description == NULL);
    CHECK_EQ_U64(descriptors[descriptor_count].port, 0u);
    CHECK_EQ_U64(descriptors[descriptor_count].device, 0u);
    CHECK_EQ_U64(descriptors[descriptor_count].index, 0u);
    CHECK_EQ_U64(descriptors[descriptor_count].id, 0u);
}
#undef EXPECTED_PAD_DESCRIPTORS

static void check_input_descriptors(const struct retro_input_descriptor *descriptors)
{
    input_descriptor_calls++;
    published_input_descriptors = descriptors;
    validate_input_descriptors(descriptors);
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

static void RETRO_CALLCONV log_printf(enum retro_log_level level, const char *format, ...)
{
    va_list args;
    (void)level;
    va_start(args, format);
    (void)vsnprintf(log_message, sizeof log_message, format, args);
    va_end(args);
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
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        if (data == NULL) {
            return false;
        }
        ((struct retro_log_callback *)data)->log = log_printf;
        return true;
    case RETRO_ENVIRONMENT_GET_INPUT_DEVICE_CAPABILITIES:
        input_capabilities_calls++;
        if (!input_capabilities_available || data == NULL) {
            return false;
        }
        *(uint64_t *)data = input_capabilities;
        return true;
    case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
        check_controller_info((const struct retro_controller_info *)data);
        return true;
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
        check_input_descriptors((const struct retro_input_descriptor *)data);
        return true;
    case RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE:
        disk_control_set_calls++;
        if (data == NULL) {
            memset(&disk_control, 0, sizeof disk_control);
            disk_control_unregister_calls++;
        } else {
            disk_control = *(const struct retro_disk_control_ext_callback *)data;
        }
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
        if (strcmp(var->key, "schwaystation_machine") == 0) {
            var->value = machine_option_value;
            return true;
        }
        if (strcmp(var->key, "schwaystation_bios") == 0) {
            var->value = bios_option_value;
            return true;
        }
        if (strcmp(var->key, "schwaystation_boot") == 0) {
            var->value = boot_option_value;
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
        if (strcmp(var->key, "schwaystation_multitap_port1") == 0) {
            var->value = multitap_port1_value;
            return true;
        }
        if (strcmp(var->key, "schwaystation_multitap_port2") == 0) {
            var->value = multitap_port2_value;
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
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        if (!save_directory_available || save_directory == NULL || data == NULL) {
            return false;
        }
        save_directory_calls++;
        *(const char **)data = save_directory;
        return true;
    case RETRO_ENVIRONMENT_SET_MEMORY_MAPS: {
        const struct retro_memory_map *map = (const struct retro_memory_map *)data;
        memory_map_calls++;
        if (!memory_maps_available || map == NULL ||
            map->num_descriptors > MEMORY_DESCRIPTOR_CAP ||
            (map->num_descriptors != 0u && map->descriptors == NULL)) {
            return false;
        }
        memory_descriptor_count = map->num_descriptors;
        for (unsigned i = 0u; i < memory_descriptor_count; i++) {
            memory_descriptors[i] = map->descriptors[i];
            memory_addrspaces[i][0] = '\0';
            if (map->descriptors[i].addrspace != NULL) {
                size_t length = strlen(map->descriptors[i].addrspace);
                if (length >= sizeof memory_addrspaces[i]) {
                    return false;
                }
                memcpy(memory_addrspaces[i], map->descriptors[i].addrspace, length + 1u);
                memory_descriptors[i].addrspace = memory_addrspaces[i];
            } else {
                memory_descriptors[i].addrspace = NULL;
            }
        }
        return true;
    }
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
    input_poll_calls++;
    if (content_test) {
        content_polls++;
    }
}

static int16_t RETRO_CALLCONV input(unsigned port, unsigned device, unsigned index, unsigned id)
{
    static const unsigned player_buttons[8] = {
        RETRO_DEVICE_ID_JOYPAD_B, RETRO_DEVICE_ID_JOYPAD_A,
        RETRO_DEVICE_ID_JOYPAD_Y, RETRO_DEVICE_ID_JOYPAD_X,
        RETRO_DEVICE_ID_JOYPAD_L, RETRO_DEVICE_ID_JOYPAD_R,
        RETRO_DEVICE_ID_JOYPAD_L2, RETRO_DEVICE_ID_JOYPAD_R2,
    };
    if (multitap_test_mode) {
        if (port >= 8u) {
            multitap_invalid_player_visits++;
            return 0;
        }
        if (device == RETRO_DEVICE_ANALOG) {
            multitap_analog_visits[port]++;
        }
        if (device == RETRO_DEVICE_JOYPAD && index == 0u && id == player_buttons[port]) {
            multitap_callback_visits[port]++;
            return 1;
        }
    }
    if (special_input_mode) {
        if (port < 8u && device < 8u && index < 3u && id < 17u) {
            special_queries[port][device][index][id]++;
        }
        if (device == RETRO_DEVICE_JOYPAD && index == 0u && id < 32u) {
            return (special_buttons & (UINT32_C(1) << id)) != 0u ? 1 : 0;
        }
        if (device == RETRO_DEVICE_LIGHTGUN && index == 0u) {
            if (id == RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X) return special_lightgun_x;
            if (id == RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y) return special_lightgun_y;
            if (id < 32u) return (special_buttons & (UINT32_C(1) << id)) != 0u ? 1 : 0;
        }
        if (device == RETRO_DEVICE_ANALOG && index < 2u && id < 2u) {
            return special_axes[index][id];
        }
        if (device == RETRO_DEVICE_MOUSE) {
            if (id == RETRO_DEVICE_ID_MOUSE_X) return special_mouse_x;
            if (id == RETRO_DEVICE_ID_MOUSE_Y) return special_mouse_y;
            if (id == RETRO_DEVICE_ID_MOUSE_LEFT || id == RETRO_DEVICE_ID_MOUSE_RIGHT) {
                return (special_buttons & (UINT32_C(1) << id)) != 0u ? 1 : 0;
            }
        }
        return 0;
    }
    if (rumble_axis_mode && device == RETRO_DEVICE_ANALOG && port < 2u) {
        if (index == RETRO_DEVICE_INDEX_ANALOG_LEFT && id == RETRO_DEVICE_ID_ANALOG_X) {
            return INT16_MIN;
        }
        if (index == RETRO_DEVICE_INDEX_ANALOG_LEFT && id == RETRO_DEVICE_ID_ANALOG_Y) {
            return 0;
        }
        if (index == RETRO_DEVICE_INDEX_ANALOG_RIGHT && id == RETRO_DEVICE_ID_ANALOG_X) {
            return INT16_MAX;
        }
        if (index == RETRO_DEVICE_INDEX_ANALOG_RIGHT && id == RETRO_DEVICE_ID_ANALOG_Y) {
            return 0;
        }
    }
    if (input_controls_active) {
        input_state_calls++;
        if (input_controls_chord_enabled != 0 && port == 0u && device == RETRO_DEVICE_JOYPAD &&
            index == 0u && content_polls <= input_controls_chord_until &&
            (id == RETRO_DEVICE_ID_JOYPAD_SELECT || id == RETRO_DEVICE_ID_JOYPAD_START ||
             id == RETRO_DEVICE_ID_JOYPAD_L || id == RETRO_DEVICE_ID_JOYPAD_L2 ||
             id == RETRO_DEVICE_ID_JOYPAD_R || id == RETRO_DEVICE_ID_JOYPAD_R2)) {
            return 1;
        }
        return 0;
    }
    if (input_movie_mode) {
        unsigned frame_index = content_polls;
        input_state_calls++;
        if (device == RETRO_DEVICE_JOYPAD && index == 0u && id == RETRO_DEVICE_ID_JOYPAD_B &&
            port == 0u &&
            ((input_movie_variant == 1u && frame_index == 60u) ||
             (input_movie_variant == 2u &&
              (frame_index == 60u || frame_index == 100u || frame_index == 120u)) ||
             (input_movie_variant == 3u && frame_index >= 60u && frame_index <= 180u))) {
            return 1;
        }
        if (device == RETRO_DEVICE_ANALOG && port < 2u && id <= RETRO_DEVICE_ID_ANALOG_Y &&
            (index == RETRO_DEVICE_INDEX_ANALOG_LEFT ||
             index == RETRO_DEVICE_INDEX_ANALOG_RIGHT)) {
            if (frame_index == 1u) {
                if (index == RETRO_DEVICE_INDEX_ANALOG_LEFT && id == RETRO_DEVICE_ID_ANALOG_X) {
                    return INT16_MIN;
                }
                if (index == RETRO_DEVICE_INDEX_ANALOG_LEFT && id == RETRO_DEVICE_ID_ANALOG_Y) {
                    return 0;
                }
                if (index == RETRO_DEVICE_INDEX_ANALOG_RIGHT && id == RETRO_DEVICE_ID_ANALOG_X) {
                    return INT16_MAX;
                }
                return INT16_MIN;
            }
            if (frame_index == 2u) {
                if (index == RETRO_DEVICE_INDEX_ANALOG_LEFT && id == RETRO_DEVICE_ID_ANALOG_X) {
                    return 0;
                }
                if (index == RETRO_DEVICE_INDEX_ANALOG_LEFT && id == RETRO_DEVICE_ID_ANALOG_Y) {
                    return INT16_MAX;
                }
                if (index == RETRO_DEVICE_INDEX_ANALOG_RIGHT && id == RETRO_DEVICE_ID_ANALOG_X) {
                    return INT16_MIN;
                }
                return 0;
            }
            return 0;
        }
        return 0;
    }
    if (content_test && input_controls_active == 0 && port == 0u && device == RETRO_DEVICE_JOYPAD && index == 0u &&
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

/* A fresh instance with the given region and optional board, or NULL. */
static schway *ref_create_profile(uint32_t region, uint32_t board)
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
    profile.board = board;
    if (board == SCHWAY_BOARD_PU7 || board == SCHWAY_BOARD_PU8_EARLY) {
        profile.flags |= SCHWAY_PROFILE_GPU_V0;
    }
    if (schway_create(&cfg, &ref) != SCHWAY_STATUS_OK) {
        return NULL;
    }
    if (schway_set_profile(ref, &profile) != SCHWAY_STATUS_OK) {
        schway_destroy(ref);
        return NULL;
    }
    return ref;
}

static schway *ref_create(uint32_t region)
{
    return ref_create_profile(region, 0u);
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

/* Movie references bound poisoning to their expected packed active extent. */
static void run_one_movie(unsigned expected_width, unsigned expected_height)
{
    size_t poison_bytes = sizeof frame;
    if (expected_width != 0u && expected_width <= MAX_W && expected_height != 0u &&
        expected_height <= MAX_H) {
        size_t expected_pixels = (size_t)expected_width * (size_t)expected_height;
        poison_bytes = expected_pixels * sizeof frame[0];
    }
    memset(frame, POISON, poison_bytes);
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

static void step_with_devices(schway *ref, const char *what, unsigned lo, uint8_t device0,
                              uint8_t device1)
{
    schway_input input_snapshot;
    memset(&input_snapshot, 0, sizeof input_snapshot);
    input_snapshot.size = (uint32_t)sizeof input_snapshot;
    input_snapshot.device[0] = device0;
    input_snapshot.device[1] = device1;
    for (unsigned port = 0u; port < 2u; port++) {
        for (unsigned axis = 0u; axis < 4u; axis++) {
            input_snapshot.axes[port][axis] = 128u;
        }
    }
    CHECK_EQ_U64(schway_set_input(ref, &input_snapshot), SCHWAY_STATUS_OK);
    step(ref, what, lo);
}

/* Sets the frontend callbacks, as a frontend does after retro_init. */
static void set_callbacks(void)
{
    p_set_video_refresh(video);
    p_set_audio_sample(sample);
    p_set_audio_sample_batch(single_audio ? NULL : batch);
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

static uint32_t region_from_option(const char *name)
{
    if (strcmp(name, "ntsc-j") == 0) return SCHWAY_REGION_NTSC_J;
    if (strcmp(name, "pal") == 0) return SCHWAY_REGION_PAL;
    return SCHWAY_REGION_NTSC_U;
}

static void compare_loaded_profile(uint32_t expected_region, uint32_t expected_board)
{
    schway *reference = ref_create_profile(expected_region, expected_board);
    size_t module_size = p_serialize_size();
    size_t reference_size = reference != NULL ? schway_state_size(reference) : 0u;
    unsigned char *module_state = module_size != 0u ? (unsigned char *)malloc(module_size) : NULL;
    unsigned char *reference_state = reference_size != 0u
                                         ? (unsigned char *)malloc(reference_size) : NULL;
    CHECK(reference != NULL);
    CHECK(module_state != NULL);
    CHECK(reference_state != NULL);
    CHECK_EQ_U64(module_size, reference_size);
    if (reference != NULL && module_state != NULL && reference_state != NULL &&
        module_size == reference_size) {
        CHECK_EQ_U64(p_serialize(module_state, module_size), true);
        CHECK_EQ_U64(schway_save_state(reference, reference_state, reference_size),
                     SCHWAY_STATUS_OK);
        if (memcmp(module_state, reference_state, module_size) != 0) {
            size_t offset = 20u;
            while (offset < module_size && module_state[offset] == reference_state[offset]) {
                offset++;
            }
            fprintf(stderr, "profile state differs for region=%u board=%u at byte %lu: module=%02x reference=%02x\n",
                    expected_region, expected_board, (unsigned long)offset,
                    module_state[offset], reference_state[offset]);
            check_failures++;
        }
    }
    free(module_state);
    free(reference_state);
    schway_destroy(reference);
}

static void test_option_profiles(void)
{
    static const struct {
        const char *token;
        uint32_t board;
    } machines[] = {
        {"pu7", SCHWAY_BOARD_PU7},
        {"pu8-early", SCHWAY_BOARD_PU8_EARLY},
        {"pu8-late", SCHWAY_BOARD_PU8_LATE},
        {"pu18", SCHWAY_BOARD_PU18},
        {"pu20", SCHWAY_BOARD_PU20},
        {"pu22", SCHWAY_BOARD_PU22},
        {"pu23", SCHWAY_BOARD_PU23},
        {"pm41", SCHWAY_BOARD_PM41},
        {"pm41-2", SCHWAY_BOARD_PM41_2},
    };
    static const char *const regions[] = {"ntsc-u", "ntsc-j", "pal"};

    machine_option_value = "pu18";
    option_value = "auto";
    CHECK_EQ_U64(p_load_game(NULL), true);
    CHECK_EQ_U64(p_get_region(), RETRO_REGION_NTSC);
    compare_loaded_profile(SCHWAY_REGION_NTSC_U, SCHWAY_BOARD_PU18);
    p_unload_game();

    machine_option_value = NULL;
    CHECK_EQ_U64(p_load_game(NULL), true);
    compare_loaded_profile(SCHWAY_REGION_NTSC_U, SCHWAY_BOARD_PU18);
    p_unload_game();

    machine_option_value = "unsupported";
    CHECK_EQ_U64(p_load_game(NULL), true);
    compare_loaded_profile(SCHWAY_REGION_NTSC_U, SCHWAY_BOARD_PU18);
    p_unload_game();

    for (size_t machine = 0u; machine < sizeof machines / sizeof machines[0]; machine++) {
        machine_option_value = machines[machine].token;
        for (size_t region = 0u; region < sizeof regions / sizeof regions[0]; region++) {
            option_value = regions[region];
            CHECK_EQ_U64(p_load_game(NULL), true);
            uint32_t selected_region = region_from_option(regions[region]);
            CHECK_EQ_U64(p_get_region(), selected_region == SCHWAY_REGION_PAL
                                                ? RETRO_REGION_PAL : RETRO_REGION_NTSC);
            compare_loaded_profile(selected_region, machines[machine].board);
            p_unload_game();
        }
    }
}

static void test_content_profile(const char *path, const char *bios_directory)
{
    schway_disc_image *image = NULL;
    schway_disc_image_error image_error;
    schway_host_disc_probe probe;
    schway_host_disc_probe_status probe_status;
    uint32_t exe_licence = SCHWAY_LICENCE_NONE;
    schway__region_resolution resolution;
    struct retro_game_info game;
    struct retro_system_av_info av;

    memset(&image_error, 0, sizeof image_error);
    image_error.size = (uint32_t)sizeof image_error;
    schway_disc_image_status image_status = schway_disc_image_open(path, &image, &image_error);
    const char *probe_path = image_status == SCHWAY_DISC_IMAGE_OK && image != NULL
                                 ? schway_disc_image_path(image, 0u) : NULL;
    memset(&probe, 0, sizeof probe);
    probe_status = probe_path != NULL ? schway__disc_probe_region(probe_path, &probe)
                                      : SCHWAY_HOST_DISC_PROBE_UNSUPPORTED;
    CHECK_EQ_U64(probe_status, SCHWAY_HOST_DISC_PROBE_OK);
    if (probe_status == SCHWAY_HOST_DISC_PROBE_OK && probe.exe_bytes != NULL) {
        schway_exe_info exe;
        memset(&exe, 0, sizeof exe);
        exe.size = (uint32_t)sizeof exe;
        if (schway_probe_exe(probe.exe_bytes, probe.exe_size, &exe) == SCHWAY_STATUS_OK) {
            exe_licence = exe.licence_area;
        }
    }
    resolution = schway__region_resolve(
        probe_status == SCHWAY_HOST_DISC_PROBE_OK ? probe.serial : NULL,
        probe_status == SCHWAY_HOST_DISC_PROBE_OK ? probe.licence_area : SCHWAY_LICENCE_NONE,
        exe_licence);
    schway__disc_probe_dispose(&probe);
    schway_disc_image_close(image);

    memset(&game, 0, sizeof game);
    game.path = path;
    system_directory = bios_directory;
    machine_option_value = "pu8-early";
    option_value = "auto";
    log_message[0] = '\0';
    CHECK_EQ_U64(p_load_game(&game), true);
    CHECK_EQ_U64(p_get_region(), resolution.region == SCHWAY_REGION_PAL
                                    ? RETRO_REGION_PAL : RETRO_REGION_NTSC);
    memset(&av, 0, sizeof av);
    p_get_system_av_info(&av);
    check_av(&av, resolution.region);
    if (resolution.disagreements != 0u) {
        CHECK(strstr(log_message, "region hint disagreement") != NULL);
    }
    p_unload_game();

    machine_option_value = "pm41";
    option_value = "pal";
    log_message[0] = '\0';
    CHECK_EQ_U64(p_load_game(&game), true);
    CHECK_EQ_U64(p_get_region(), RETRO_REGION_PAL);
    memset(&av, 0, sizeof av);
    p_get_system_av_info(&av);
    check_av(&av, SCHWAY_REGION_PAL);
    if (resolution.disagreements != 0u) {
        CHECK(strstr(log_message, "selected=pal") != NULL);
    }
    p_unload_game();

    schway__region_resolution conflict = schway__region_resolve(
        "SLUS-00001", SCHWAY_LICENCE_JAPAN, SCHWAY_LICENCE_EUROPE);
    CHECK_EQ_U64(conflict.region, SCHWAY_REGION_NTSC_U);
    CHECK_EQ_U64(conflict.disagreements,
                 SCHWAY__REGION_DISAGREES_SERIAL_DISC |
                     SCHWAY__REGION_DISAGREES_SERIAL_EXE |
                     SCHWAY__REGION_DISAGREES_DISC_EXE);
}

static int test_path(char *path, size_t capacity, const char *directory, const char *name)
{
    int written = snprintf(path, capacity, "%s/%s", directory, name);
    return written > 0 && (size_t)written < capacity;
}

static int make_test_directory(const char *path)
{
#ifdef _WIN32
    int result = _mkdir(path);
#else
    int result = mkdir(path, 0700);
#endif
    return result == 0 || errno == EEXIST;
}

static int pcm_bytes_equal(const uint8_t *left, size_t left_size,
                           const uint8_t *right, size_t right_size)
{
    return left_size == right_size && memcmp(left, right, left_size) == 0;
}

static void audio_expect(int condition, const char *message)
{
    audio_tap_checks++;
    printf("%s %u - %s\n", condition ? "ok" : "not ok", audio_tap_checks, message);
    audio_tap_failures += condition ? 0u : 1u;
}

static uint8_t *read_pcm_file(const char *path, size_t *size)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL || fseek(file, 0, SEEK_END) != 0) {
        if (file != NULL) {
            (void)fclose(file);
        }
        return NULL;
    }
    long length = ftell(file);
    if (length <= 0 || fseek(file, 0, SEEK_SET) != 0) {
        (void)fclose(file);
        return NULL;
    }
    uint8_t *bytes = (uint8_t *)malloc((size_t)length);
    if (bytes == NULL || fread(bytes, 1u, (size_t)length, file) != (size_t)length) {
        free(bytes);
        (void)fclose(file);
        return NULL;
    }
    (void)fclose(file);
    *size = (size_t)length;
    return bytes;
}

static int test_audio_equality(const char *exe_path, const char *pcm_path, uint32_t region)
{
    size_t pcm_size = 0u;
    uint8_t *runner_pcm = read_pcm_file(pcm_path, &pcm_size);
    struct retro_system_av_info av;
    struct retro_game_info game;
    schway *audio_reference = ref_create(region);
    schway_info info;
    uint8_t callback_bytes[4u * MAX_PAIRS];
    uint8_t first_frame[4u * MAX_PAIRS];
    unsigned frames = region == SCHWAY_REGION_PAL ? 600u : 601u;
    unsigned pairs_floor = region == SCHWAY_REGION_PAL ? 886u : 737u;
    size_t offset = 0u;
    size_t first_frame_bytes = 0u;
    uint64_t total_pairs = 0u;
    unsigned nonzero = 0u;
    unsigned asymmetric = 0u;
    unsigned saw_floor = 0u;
    unsigned saw_ceil = 0u;
    unsigned video_start = video_calls;
    unsigned video_bad_start = video_bad;
    unsigned pairs_valid = 1u;
    unsigned stream_valid = 1u;

    printf("TAP version 13\n");
    audio_expect(runner_pcm != NULL, "runner PCM capture file is available");
    if (runner_pcm == NULL) {
        schway_destroy(audio_reference);
        p_deinit();
        module_close();
        printf("1..%u\n", audio_tap_checks);
        return 1;
    }
    audio_expect(pcm_size != 0u && (pcm_size % 4u) == 0u,
                 "runner PCM contains complete non-empty stereo pairs");
    memset(&game, 0, sizeof game);
    game.path = exe_path;
    audio_expect(p_load_game(&game), "fullpath tone executable loads through the libretro stub");
    memset(&av, 0, sizeof av);
    p_get_system_av_info(&av);
    check_av(&av, region);
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    audio_expect(audio_reference != NULL &&
                     schway_get_info(audio_reference, &info) == SCHWAY_STATUS_OK,
                 "selected region exposes its core audio and frame rates");
    if (audio_reference != NULL && info.fps_den != 0u && info.sample_rate_den != 0u) {
        audio_expect(av.timing.fps == (double)info.fps_num / (double)info.fps_den &&
                         av.timing.sample_rate == (double)info.sample_rate_num /
                                                      (double)info.sample_rate_den,
                     "libretro AV timing matches the core's native rates");
    } else {
        audio_expect(0, "core rates are available for AV cadence comparison");
    }
    p_set_controller_port_device(0u, RETRO_DEVICE_JOYPAD);

    for (unsigned frame_index = 0u; frame_index < frames; frame_index++) {
        run_one();
        if (got_pairs > MAX_PAIRS) {
            pairs_valid = 0u;
            continue;
        }
        pairs_valid &= got_pairs == pairs_floor || got_pairs == pairs_floor + 1u;
        saw_floor |= got_pairs == pairs_floor;
        saw_ceil |= got_pairs == pairs_floor + 1u;
        total_pairs += got_pairs;
        size_t frame_bytes = got_pairs * 4u;
        if (offset > pcm_size || frame_bytes > pcm_size - offset) {
            fprintf(stderr, "audio frame %u exceeds runner stream at byte %lu\n",
                    frame_index + 1u, (unsigned long)offset);
            stream_valid = 0u;
            continue;
        }
        for (size_t pair = 0u; pair < got_pairs; pair++) {
            uint16_t left = (uint16_t)got_audio[2u * pair];
            uint16_t right = (uint16_t)got_audio[2u * pair + 1u];
            callback_bytes[4u * pair] = (uint8_t)left;
            callback_bytes[4u * pair + 1u] = (uint8_t)(left >> 8);
            callback_bytes[4u * pair + 2u] = (uint8_t)right;
            callback_bytes[4u * pair + 3u] = (uint8_t)(right >> 8);
            nonzero |= left != 0u || right != 0u;
            asymmetric |= left != right;
        }
        if (frame_index == 0u && frame_bytes <= sizeof first_frame) {
            memcpy(first_frame, callback_bytes, frame_bytes);
            first_frame_bytes = frame_bytes;
        }
        if (!pcm_bytes_equal(callback_bytes, frame_bytes, runner_pcm + offset, frame_bytes)) {
            if (stream_valid != 0u) {
                size_t byte = 0u;
                while (byte < frame_bytes && callback_bytes[byte] == runner_pcm[offset + byte]) {
                    byte++;
                }
                printf("# audio frame %u differs at stream byte %lu\n", frame_index + 1u,
                       (unsigned long)(offset + byte));
            }
            stream_valid = 0u;
        }
        offset += frame_bytes;
    }
    audio_expect(pairs_valid != 0u, "every frame preserves the native region sample count");
    audio_expect(saw_floor != 0u && saw_ceil != 0u,
                 "remainder carry produces both native frame pair counts");
    if (audio_reference != NULL && info.fps_num != 0u && info.sample_rate_den != 0u) {
        uint64_t expected_numerator = (uint64_t)info.sample_rate_num * info.fps_den * frames;
        uint64_t expected_denominator = (uint64_t)info.sample_rate_den * info.fps_num;
        uint64_t expected_total = expected_numerator / expected_denominator;
        audio_expect(total_pairs >= expected_total && total_pairs <= expected_total + 1u,
                     "total callback pairs track the native AV cadence without drift");
    } else {
        audio_expect(0, "core rates are available for total cadence comparison");
    }
    audio_expect(video_calls - video_start == frames && video_bad == video_bad_start,
                 "every audio frame has one valid video callback");
    audio_expect(stream_valid && offset == pcm_size,
                 "batch callback bytes and total length equal the runner stream");
    audio_expect(nonzero != 0u, "tone callbacks contain nonzero PCM");
    audio_expect(asymmetric != 0u, "tone callbacks preserve asymmetric stereo PCM");

    if (pcm_size >= 4u) {
        uint8_t *changed = (uint8_t *)malloc(pcm_size);
        audio_expect(changed != NULL, "negative-control buffer allocation succeeds");
        if (changed != NULL) {
            memcpy(changed, runner_pcm, pcm_size);
            changed[0] ^= 1u;
            audio_expect(!pcm_bytes_equal(runner_pcm, pcm_size, changed, pcm_size),
                         "a changed sample is detected");
            audio_expect(!pcm_bytes_equal(runner_pcm, pcm_size, runner_pcm, pcm_size - 1u),
                         "a missing tail byte is detected");
            free(changed);
        }
    }

    p_reset();
    run_one();
    unsigned reset_audio_nonzero = 0u;
    for (size_t pair = 0u; pair < got_pairs && pair < MAX_PAIRS; pair++) {
        reset_audio_nonzero |= got_audio[2u * pair] != 0 || got_audio[2u * pair + 1u] != 0;
    }
    audio_expect(reset_audio_nonzero == 0u,
                 "power reset does not replay tone samples from the prior content frame");
    memset(&av, 0, sizeof av);
    p_get_system_av_info(&av);
    audio_expect(av.timing.fps == (double)info.fps_num / (double)info.fps_den &&
                     av.timing.sample_rate == (double)info.sample_rate_num /
                                                  (double)info.sample_rate_den,
                 "reset preserves the native AV rates");

    p_unload_game();
    audio_expect(p_load_game(&game), "tone content reloads after unload");
    run_one();
    size_t reload_audio_bytes = got_pairs * 4u;
    uint8_t reload_audio[4u * MAX_PAIRS];
    size_t reload_pairs = got_pairs < MAX_PAIRS ? got_pairs : MAX_PAIRS;
    for (size_t pair = 0u; pair < reload_pairs; pair++) {
        uint16_t left = (uint16_t)got_audio[2u * pair];
        uint16_t right = (uint16_t)got_audio[2u * pair + 1u];
        reload_audio[4u * pair] = (uint8_t)left;
        reload_audio[4u * pair + 1u] = (uint8_t)(left >> 8);
        reload_audio[4u * pair + 2u] = (uint8_t)right;
        reload_audio[4u * pair + 3u] = (uint8_t)(right >> 8);
    }
    int reload_audio_equal = got_pairs <= MAX_PAIRS && first_frame_bytes != 0u &&
                             reload_audio_bytes == first_frame_bytes &&
                             memcmp(reload_audio, first_frame, first_frame_bytes) == 0;
    if (!reload_audio_equal) {
        size_t compared = reload_audio_bytes < first_frame_bytes ? reload_audio_bytes
                                                                 : first_frame_bytes;
        size_t byte = 0u;
        while (byte < compared && reload_audio[byte] == first_frame[byte]) {
            byte++;
        }
        fprintf(stderr, "reload PCM: got %lu bytes, expected %lu; first difference at %lu",
                (unsigned long)reload_audio_bytes, (unsigned long)first_frame_bytes,
                (unsigned long)byte);
        if (byte < compared) {
            fprintf(stderr, " (got 0x%02x, expected 0x%02x)",
                    (unsigned)reload_audio[byte], (unsigned)first_frame[byte]);
        }
        fputc('\n', stderr);
    }
    audio_expect(reload_audio_equal,
                 "content reload clears queued samples and delivers its first frame intact");
    memset(&av, 0, sizeof av);
    p_get_system_av_info(&av);
    audio_expect(av.timing.fps == (double)info.fps_num / (double)info.fps_den &&
                     av.timing.sample_rate == (double)info.sample_rate_num /
                                                  (double)info.sample_rate_den,
                 "content reload preserves the native AV rates");

    p_unload_game();
    p_deinit();
    module_close();
    free(runner_pcm);
    schway_destroy(audio_reference);
    printf("1..%u\n", audio_tap_checks);
    return audio_tap_failures != 0u || check_failures != 0 ? 1 : 0;
}

static int valid_audio_region(const char *name)
{
    return strcmp(name, "ntsc-j") == 0 || strcmp(name, "ntsc-u") == 0 ||
           strcmp(name, "pal") == 0;
}

static uint32_t audio_region(const char *name)
{
    if (strcmp(name, "ntsc-j") == 0) {
        return SCHWAY_REGION_NTSC_J;
    }
    if (strcmp(name, "pal") == 0) {
        return SCHWAY_REGION_PAL;
    }
    return SCHWAY_REGION_NTSC_U;
}

static int write_test_file(const char *path, const void *bytes, size_t size)
{
    FILE *file = schway__test_open_file(path, "wb");
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

static int copy_test_file(const char *source_path, const char *destination_path)
{
    unsigned char buffer[4096];
    FILE *source = schway__test_open_file(source_path, "rb");
    FILE *destination;
    int ok = 1;
    if (source == NULL) {
        return 0;
    }
    destination = schway__test_open_file(destination_path, "wb");
    if (destination == NULL) {
        (void)fclose(source);
        return 0;
    }
    for (;;) {
        size_t count = fread(buffer, 1u, sizeof buffer, source);
        if (count == 0u) {
            break;
        }
        if (fwrite(buffer, 1u, count, destination) != count) {
            ok = 0;
            break;
        }
    }
    if (ferror(source) != 0) {
        ok = 0;
    }
    if (fclose(source) != 0) {
        ok = 0;
    }
    if (fclose(destination) != 0) {
        ok = 0;
    }
    if (!ok) {
        (void)remove(destination_path);
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

static int test_content_psexe(const char *exe_path, const char *directory)
{
    char psexe_path[4096];
    struct retro_system_info sys;
    struct retro_game_info game;
    unsigned red_pixel_seen = 0u;
    int copied = 0;
    const char *prior_boot = boot_option_value;
    const char *prior_bios = bios_option_value;

    CHECK(test_path(psexe_path, sizeof psexe_path, directory, "pad_counter.PsExE"));
    if (check_failures != 0) {
        goto cleanup;
    }
    copied = copy_test_file(exe_path, psexe_path);
    CHECK(copied);
    if (!copied) {
        goto cleanup;
    }
    memset(&sys, 0, sizeof sys);
    p_get_system_info(&sys);
    check_string("valid_extensions", sys.valid_extensions, expected_valid_extensions());
    memset(&game, 0, sizeof game);
    game.path = psexe_path;
    boot_option_value = "fast";
    bios_option_value = "missing.bin";
    CHECK_EQ_U64(p_load_game(&game), 0u);
    CHECK_EQ_U64(p_serialize_size(), 0u);
    boot_option_value = "full";
    int loaded = p_load_game(&game);
    boot_option_value = prior_boot;
    bios_option_value = prior_bios;
    CHECK_EQ_U64(loaded, 1u);
    if (p_serialize_size() != 0u) {
        p_set_controller_port_device(0u, RETRO_DEVICE_JOYPAD);
        for (unsigned run = 1u; run <= 60u; run++) {
            run_one();
            if (frame_h > 100u && frame_w != 0u &&
                frame[(size_t)100u * frame_w] == 0x00FF0000u) {
                red_pixel_seen = 1u;
            }
        }
        CHECK_EQ_U64(video_calls, 60u);
        CHECK_EQ_U64(video_bad, 0u);
        CHECK_EQ_U64(red_pixel_seen, 1u);
    }
    p_unload_game();
    CHECK_EQ_U64(p_serialize_size(), 0u);

cleanup:
    if (copied) {
        (void)remove(psexe_path);
    }
    p_deinit();
    module_close();
    return check_failures != 0 ? 1 : 0;
}

static int test_input_descriptors(void)
{
    CHECK_EQ_U64(input_descriptor_calls, 1u);
    validate_input_descriptors(published_input_descriptors);
    CHECK_EQ_U64(p_load_game(NULL), 1u);
    run_one();
    validate_input_descriptors(published_input_descriptors);
    p_unload_game();
    validate_input_descriptors(published_input_descriptors);
    p_deinit();
    validate_input_descriptors(published_input_descriptors);
    module_close();
    return check_failures != 0 ? 1 : 0;
}

static int test_input_multitap(void)
{
    static const uint16_t expected_buttons[8] = {
        UINT16_C(0xBFFF), UINT16_C(0xDFFF), UINT16_C(0x7FFF), UINT16_C(0xEFFF),
        UINT16_C(0xFBFF), UINT16_C(0xF7FF), UINT16_C(0xFEFF), UINT16_C(0xFDFF),
    };
    static const struct {
        const char *port1;
        const char *port2;
        unsigned players;
        unsigned physical_port[8];
        unsigned socket[8];
    } topologies[] = {
        {"disabled", "disabled", 2u, {0u, 1u}, {0u, 0u}},
        {"enabled", "disabled", 5u, {0u, 0u, 0u, 0u, 1u}, {0u, 1u, 2u, 3u, 0u}},
        {"disabled", "enabled", 5u, {0u, 1u, 1u, 1u, 1u}, {0u, 0u, 1u, 2u, 3u}},
        {"enabled", "enabled", 8u, {0u, 0u, 0u, 0u, 1u, 1u, 1u, 1u},
         {0u, 1u, 2u, 3u, 0u, 1u, 2u, 3u}},
    };

    CHECK(p_test_poll_endpoint != NULL);
    for (unsigned topology = 0u; topology < sizeof topologies / sizeof topologies[0]; topology++) {
        multitap_port1_value = topologies[topology].port1;
        multitap_port2_value = topologies[topology].port2;
        CHECK_EQ_U64(expected_active_players(), topologies[topology].players);
        unsigned descriptors_before = input_descriptor_calls;
        unsigned controllers_before = controller_info_calls;
        CHECK_EQ_U64(p_load_game(NULL), 1u);
        CHECK(input_descriptor_calls > descriptors_before);
        CHECK(controller_info_calls > controllers_before);
        if (input_descriptor_calls > descriptors_before) {
            validate_input_descriptors(published_input_descriptors);
        }
        for (unsigned player = 0u; player < 8u; player++) {
            p_set_controller_port_device(player, RETRO_DEVICE_JOYPAD);
        }
        p_set_controller_port_device(8u, RETRO_DEVICE_JOYPAD);

        unsigned polls_before = input_poll_calls;
        unsigned invalid_before = multitap_invalid_player_visits;
        unsigned callbacks_before[8];
        unsigned analog_before[8];
        memcpy(callbacks_before, multitap_callback_visits, sizeof callbacks_before);
        memcpy(analog_before, multitap_analog_visits, sizeof analog_before);
        p_run();
        CHECK_EQ_U64(input_poll_calls, polls_before + 1u);
        CHECK_EQ_U64(multitap_invalid_player_visits, invalid_before);
        for (unsigned player = 0u; player < topologies[topology].players; player++) {
            uint16_t buttons = 0u;
            CHECK_EQ_U64(multitap_callback_visits[player], callbacks_before[player] + 1u);
            CHECK_EQ_U64(p_test_poll_endpoint(topologies[topology].physical_port[player],
                                              topologies[topology].socket[player], &buttons), 1u);
            CHECK_EQ_U64(buttons, expected_buttons[player]);
        }
        if (topologies[topology].players < 8u) {
            unsigned inactive = topologies[topology].players;
            CHECK_EQ_U64(multitap_callback_visits[inactive], callbacks_before[inactive]);
        }

        if (topology == 1u) {
            const char *selected_port1 = multitap_port1_value;
            const char *selected_port2 = multitap_port2_value;
            unsigned reload_descriptors_before = input_descriptor_calls;
            unsigned reload_controllers_before = controller_info_calls;
            struct retro_game_info invalid_game = {
                "missing-controller-topology.exe", NULL, 0u, NULL,
            };
            multitap_port1_value = "disabled";
            multitap_port2_value = "disabled";
            CHECK_EQ_U64(p_load_game(&invalid_game), 0u);
            CHECK_EQ_U64(input_descriptor_calls, reload_descriptors_before);
            CHECK_EQ_U64(controller_info_calls, reload_controllers_before);
            multitap_port1_value = selected_port1;
            multitap_port2_value = selected_port2;
            if (input_descriptor_calls == reload_descriptors_before &&
                controller_info_calls == reload_controllers_before) {
                validate_input_descriptors(published_input_descriptors);
            }
        }

        if (topology == 0u) {
            unsigned analog_visits = multitap_analog_visits[0];
            p_set_controller_port_device(0u, RETRO_DEVICE_ANALOG | (1u << 8));
            polls_before = input_poll_calls;
            p_run();
            CHECK_EQ_U64(input_poll_calls, polls_before + 1u);
            CHECK_EQ_U64(multitap_analog_visits[0], analog_visits);
            uint16_t buttons = 0u;
            CHECK_EQ_U64(p_test_poll_endpoint(0u, 0u, &buttons), 1u);
            CHECK_EQ_U64(buttons, expected_buttons[0]);
        }
        p_unload_game();
    }

    multitap_port1_value = "disabled";
    multitap_port2_value = "disabled";
    p_deinit();
    p_set_environment(env);
    p_init();
    validate_input_descriptors(published_input_descriptors);
    p_deinit();
    p_unload_game();
    module_close();
    return check_failures != 0 ? 1 : 0;
}

static void clear_special_queries(void)
{
    memset(special_queries, 0, sizeof special_queries);
}

static void expect_special_packet(unsigned port, unsigned socket, uint8_t id,
                                  const uint8_t *payload, size_t payload_size)
{
    uint8_t actual[9];
    size_t written = 0u;
    CHECK(p_test_poll_bytes != NULL);
    if (p_test_poll_bytes == NULL) {
        return;
    }
    CHECK_EQ_U64(p_test_poll_bytes(port, socket, actual, sizeof actual, &written), 1u);
    CHECK_EQ_U64(written, 3u + payload_size);
    if (written == 3u + payload_size) {
        CHECK_EQ_HEX(actual[0], 0xFFu);
        CHECK_EQ_HEX(actual[1], id);
        CHECK_EQ_HEX(actual[2], 0x5Au);
        CHECK(memcmp(actual + 3u, payload, payload_size) == 0);
    }
}

static void expect_special_query(unsigned player, unsigned device, unsigned index, unsigned id,
                                 unsigned count)
{
    CHECK_EQ_U64(special_queries[player][device][index][id], count);
}

static int test_input_special(void)
{
    static const uint8_t mouse_moved[] = {0xFFu, 0xF0u, 0x80u, 0x7Fu};
    static const uint8_t mouse_neutral[] = {0xFFu, 0xFCu, 0u, 0u};
    static const uint8_t flight_extremes[] = {0xFFu, 0xBFu, 0xFFu, 0u, 0u, 0xFFu};
    static const uint8_t negcon_pressure[] = {0xE7u, 0x97u, 0xFFu, 0xFFu, 0xFFu, 0xFFu};
    static const uint8_t jogcon_min[] = {0xD7u, 0xFFu, 0u, 0x80u, 1u, 0u};
    static const uint8_t jogcon_zero[] = {0xFFu, 0xFFu, 0u, 0u, 0u, 0u};
    static const uint8_t jogcon_max[] = {0xFFu, 0xFFu, 0xFEu, 0x7Fu, 2u, 0u};
    const unsigned flight = RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG, 1);
    const unsigned negcon = RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG, 2);
    const unsigned jogcon = RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG, 3);
    uint16_t buttons = 0u;
    unsigned polls;

    special_input_mode = 1;
    special_buttons = UINT32_C(1) << RETRO_DEVICE_ID_JOYPAD_B;
    special_mouse_x = 0;
    special_mouse_y = 0;
    memset(special_axes, 0, sizeof special_axes);
    CHECK_EQ_U64(p_load_game(NULL), 1u);

    /* Existing digital and DualShock caller mappings remain intact. */
    p_set_controller_port_device(0u, RETRO_DEVICE_JOYPAD);
    run_one();
    CHECK_EQ_U64(p_test_poll_endpoint(0u, 0u, &buttons), 1u);
    CHECK_EQ_HEX(buttons, 0xBFFFu);
    p_set_controller_port_device(0u, RETRO_DEVICE_ANALOG);
    run_one();
    CHECK_EQ_U64(p_test_poll_endpoint(0u, 0u, &buttons), 1u);
    CHECK_EQ_HEX(buttons, 0xBFFFu);

    p_set_controller_port_device(0u, RETRO_DEVICE_MOUSE);
    special_mouse_x = INT16_MIN;
    special_mouse_y = INT16_MAX;
    special_buttons = (UINT32_C(1) << RETRO_DEVICE_ID_MOUSE_LEFT) |
                      (UINT32_C(1) << RETRO_DEVICE_ID_MOUSE_RIGHT);
    clear_special_queries();
    polls = input_poll_calls;
    run_one();
    CHECK_EQ_U64(input_poll_calls, polls + 1u);
    expect_special_query(0u, RETRO_DEVICE_MOUSE, 0u, RETRO_DEVICE_ID_MOUSE_X, 1u);
    expect_special_query(0u, RETRO_DEVICE_MOUSE, 0u, RETRO_DEVICE_ID_MOUSE_Y, 1u);
    expect_special_query(0u, RETRO_DEVICE_MOUSE, 0u, RETRO_DEVICE_ID_MOUSE_LEFT, 1u);
    expect_special_query(0u, RETRO_DEVICE_MOUSE, 0u, RETRO_DEVICE_ID_MOUSE_RIGHT, 1u);
    /* Mutating callback values after retro_run cannot change this frame's snapshot. */
    special_mouse_x = 0;
    special_mouse_y = 0;
    special_buttons = 0u;
    expect_special_packet(0u, 0u, 0x12u, mouse_moved, sizeof mouse_moved);
    run_one();
    expect_special_packet(0u, 0u, 0x12u, mouse_neutral, sizeof mouse_neutral);

    p_set_controller_port_device(0u, flight);
    special_buttons = UINT32_C(1) << RETRO_DEVICE_ID_JOYPAD_B;
    special_axes[RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = INT16_MIN;
    special_axes[RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_Y] = INT16_MAX;
    special_axes[RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_X] = INT16_MAX;
    special_axes[RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_Y] = INT16_MIN;
    clear_special_queries();
    run_one();
    expect_special_query(0u, RETRO_DEVICE_ANALOG, 0u, RETRO_DEVICE_ID_ANALOG_X, 1u);
    expect_special_query(0u, RETRO_DEVICE_ANALOG, 0u, RETRO_DEVICE_ID_ANALOG_Y, 1u);
    expect_special_query(0u, RETRO_DEVICE_ANALOG, 1u, RETRO_DEVICE_ID_ANALOG_X, 1u);
    expect_special_query(0u, RETRO_DEVICE_ANALOG, 1u, RETRO_DEVICE_ID_ANALOG_Y, 1u);
    expect_special_packet(0u, 0u, 0x53u, flight_extremes, sizeof flight_extremes);

    p_set_controller_port_device(0u, negcon);
    special_buttons = (UINT32_C(1) << RETRO_DEVICE_ID_JOYPAD_START) |
                      (UINT32_C(1) << RETRO_DEVICE_ID_JOYPAD_UP) |
                      (UINT32_C(1) << RETRO_DEVICE_ID_JOYPAD_R) |
                      (UINT32_C(1) << RETRO_DEVICE_ID_JOYPAD_B) |
                      (UINT32_C(1) << RETRO_DEVICE_ID_JOYPAD_A) |
                      (UINT32_C(1) << RETRO_DEVICE_ID_JOYPAD_L2);
    special_axes[RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = INT16_MAX;
    special_axes[RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_X] = INT16_MAX;
    special_axes[RETRO_DEVICE_INDEX_ANALOG_RIGHT][RETRO_DEVICE_ID_ANALOG_Y] = INT16_MIN;
    clear_special_queries();
    run_one();
    expect_special_packet(0u, 0u, 0x23u, negcon_pressure, sizeof negcon_pressure);

    p_unload_game();
    multitap_port1_value = "enabled";
    CHECK_EQ_U64(p_load_game(NULL), 1u);
    p_set_controller_port_device(1u, jogcon);
    special_buttons = (UINT32_C(1) << RETRO_DEVICE_ID_JOYPAD_START) |
                      (UINT32_C(1) << RETRO_DEVICE_ID_JOYPAD_RIGHT);
    special_axes[RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = INT16_MIN;
    clear_special_queries();
    polls = input_poll_calls;
    run_one();
    CHECK_EQ_U64(input_poll_calls, polls + 1u);
    expect_special_query(1u, RETRO_DEVICE_ANALOG, 0u, RETRO_DEVICE_ID_ANALOG_X, 1u);
    expect_special_packet(0u, 1u, 0xE3u, jogcon_min, sizeof jogcon_min);

    special_buttons = 0u;
    special_axes[RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = 0;
    run_one();
    expect_special_packet(0u, 1u, 0xE3u, jogcon_zero, sizeof jogcon_zero);
    special_axes[RETRO_DEVICE_INDEX_ANALOG_LEFT][RETRO_DEVICE_ID_ANALOG_X] = INT16_MAX;
    run_one();
    expect_special_packet(0u, 1u, 0xE3u, jogcon_max, sizeof jogcon_max);

    /* Unknown subclasses are refused without replacing the selected device. */
    p_set_controller_port_device(1u, RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG, 4));
    run_one();
    expect_special_packet(0u, 1u, 0xE3u, jogcon_max, sizeof jogcon_max);

    p_unload_game();
    p_deinit();
    module_close();
    special_input_mode = 0;
    multitap_port1_value = "disabled";
    return check_failures != 0 ? 1 : 0;
}

/* Selected BIOS failures and rejected content leave the current serialized
   instance intact; a selected BIOS applies only to a no-content load. */
static void expect_gun_reply(unsigned port, uint8_t id, uint16_t buttons,
                             unsigned payload_size)
{
    uint8_t actual[9];
    size_t written = 0u;
    CHECK(p_test_poll_bytes != NULL);
    if (p_test_poll_bytes == NULL) return;
    CHECK_EQ_U64(p_test_poll_bytes(port, 0u, actual, sizeof actual, &written), 1u);
    CHECK_EQ_U64(written, 3u + payload_size);
    if (written == 3u + payload_size) {
        CHECK_EQ_HEX(actual[0], 0xFFu);
        CHECK_EQ_HEX(actual[1], id);
        CHECK_EQ_HEX(actual[2], 0x5Au);
        CHECK_EQ_HEX(actual[3], buttons & 0xFFu);
        CHECK_EQ_HEX(actual[4], buttons >> 8);
    }
}

static int test_input_lightgun(void)
{
    const unsigned justifier = RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_LIGHTGUN, 2);
    const uint32_t flags = SCHWAY_GUN_INPUT_VALID | SCHWAY_GUN_INPUT_TRIGGER;
    const uint32_t pressed = (UINT32_C(1) << RETRO_DEVICE_ID_LIGHTGUN_TRIGGER) |
                             (UINT32_C(1) << RETRO_DEVICE_ID_LIGHTGUN_AUX_A) |
                             (UINT32_C(1) << RETRO_DEVICE_ID_LIGHTGUN_AUX_B) |
                             (UINT32_C(1) << RETRO_DEVICE_ID_LIGHTGUN_START) |
                             (UINT32_C(1) << RETRO_DEVICE_ID_LIGHTGUN_SELECT);
    schway_gun_input snapshot;
    unsigned polls, capabilities;

    special_input_mode = 1;
    special_lightgun_x = 0;
    special_lightgun_y = 0;
    special_buttons = pressed;
    input_capabilities_available = 1;
    input_capabilities = UINT64_C(1) << RETRO_DEVICE_LIGHTGUN;
    input_capabilities_calls = 0u;
    CHECK(p_test_gun_input != NULL);
    CHECK_EQ_U64(p_load_game(NULL), 1u);
    p_set_controller_port_device(0u, RETRO_DEVICE_LIGHTGUN);
    p_set_controller_port_device(1u, justifier);

    memset(special_queries, 0, sizeof special_queries);
    polls = input_poll_calls;
    capabilities = input_capabilities_calls;
    run_one();
    CHECK_EQ_U64(input_poll_calls, polls + 1u);
    CHECK_EQ_U64(input_capabilities_calls, capabilities + 1u);
    expect_special_query(0u, RETRO_DEVICE_LIGHTGUN, 0u,
                         RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X, 1u);
    expect_special_query(0u, RETRO_DEVICE_LIGHTGUN, 0u,
                         RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y, 1u);
    expect_special_query(0u, RETRO_DEVICE_LIGHTGUN, 0u,
                         RETRO_DEVICE_ID_LIGHTGUN_TRIGGER, 1u);
    expect_special_query(0u, RETRO_DEVICE_LIGHTGUN, 0u,
                         RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN, 1u);
    expect_special_query(0u, RETRO_DEVICE_LIGHTGUN, 0u,
                         RETRO_DEVICE_ID_LIGHTGUN_RELOAD, 1u);
    expect_special_query(0u, RETRO_DEVICE_LIGHTGUN, 0u,
                         RETRO_DEVICE_ID_LIGHTGUN_AUX_A, 1u);
    expect_special_query(0u, RETRO_DEVICE_LIGHTGUN, 0u,
                         RETRO_DEVICE_ID_LIGHTGUN_AUX_B, 1u);
    expect_special_query(1u, RETRO_DEVICE_LIGHTGUN, 0u,
                         RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X, 1u);
    expect_special_query(1u, RETRO_DEVICE_LIGHTGUN, 0u,
                         RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y, 1u);
    expect_special_query(1u, RETRO_DEVICE_LIGHTGUN, 0u,
                         RETRO_DEVICE_ID_LIGHTGUN_TRIGGER, 1u);
    expect_special_query(1u, RETRO_DEVICE_LIGHTGUN, 0u,
                         RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN, 1u);
    expect_special_query(1u, RETRO_DEVICE_LIGHTGUN, 0u,
                         RETRO_DEVICE_ID_LIGHTGUN_RELOAD, 1u);
    expect_special_query(1u, RETRO_DEVICE_LIGHTGUN, 0u,
                         RETRO_DEVICE_ID_LIGHTGUN_START, 1u);
    expect_special_query(1u, RETRO_DEVICE_LIGHTGUN, 0u,
                         RETRO_DEVICE_ID_LIGHTGUN_SELECT, 1u);

    memset(&snapshot, 0, sizeof snapshot);
    CHECK_EQ_U64(p_test_gun_input(0u, &snapshot), 1u);
    CHECK_EQ_U64(snapshot.x, 160u);
    CHECK_EQ_U64(snapshot.y, 120u);
    CHECK_EQ_HEX(snapshot.flags, flags);
    CHECK_EQ_U64(p_test_gun_input(1u, &snapshot), 1u);
    CHECK_EQ_U64(snapshot.x, 160u);
    CHECK_EQ_U64(snapshot.y, 120u);
    CHECK_EQ_HEX(snapshot.flags, flags);
    expect_gun_reply(0u, 0x63u, 0x9FF7u, 6u);
    expect_gun_reply(1u, 0x31u, 0x3FF7u, 2u);

    /* Changes after retro_run belong to the next frame, not this snapshot. */
    special_lightgun_x = INT16_MAX;
    special_lightgun_y = INT16_MIN;
    special_buttons = 0u;
    CHECK_EQ_U64(p_test_gun_input(0u, &snapshot), 1u);
    CHECK_EQ_U64(snapshot.x, 160u);
    CHECK_EQ_U64(snapshot.y, 120u);
    CHECK_EQ_HEX(snapshot.flags, flags);
    run_one();
    CHECK_EQ_U64(p_test_gun_input(0u, &snapshot), 1u);
    CHECK_EQ_U64(snapshot.x, 319u);
    CHECK_EQ_U64(snapshot.y, 0u);
    CHECK_EQ_HEX(snapshot.flags, SCHWAY_GUN_INPUT_VALID);

    p_unload_game();
    p_deinit();
    module_close();
    special_input_mode = 0;
    special_buttons = 0u;
    return check_failures != 0 ? 1 : 0;
}

static void content_region_reload(const char *exe_path)
{
    unsigned char prefix[0x810];
    FILE *file = schway__test_open_file(exe_path, "rb");
    CHECK(file != NULL);
    if (file == NULL) {
        return;
    }
    size_t count = fread(prefix, 1u, sizeof prefix, file);
    CHECK_EQ_U64(fclose(file), 0u);
    CHECK_EQ_U64(count, sizeof prefix);
    if (count != sizeof prefix) {
        return;
    }
    uint32_t address = ((uint32_t)prefix[0x18] | (uint32_t)prefix[0x19] << 8 |
                        (uint32_t)prefix[0x1A] << 16 | (uint32_t)prefix[0x1B] << 24) &
                       0x1FFFFFu;
    CHECK(address <= 0x200000u - 16u);
    if (address > 0x200000u - 16u) {
        return;
    }
    const char *prior_option = option_value;
    option_value = "ntsc-u";
    option_updated = false;
    struct retro_game_info game = {exe_path, NULL, 0u, NULL};
    CHECK_EQ_U64(p_load_game(&game), 1u);
    unsigned char *card = (unsigned char *)p_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    unsigned char *ram = (unsigned char *)p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    CHECK(card != NULL && ram != NULL);
    if (card != NULL && ram != NULL) {
        uintptr_t card_address = (uintptr_t)card;
        uintptr_t ram_address = (uintptr_t)ram;
        unsigned char marker = (unsigned char)(card[12345] ^ 0xA5u);
        CHECK(memcmp(ram + address, prefix + 0x800u, 16u) == 0);
        card[12345] = marker;
        run_one();
        unsigned old_av_calls = av_info_calls;
        option_value = "pal";
        option_updated = true;
        run_one();
        card = (unsigned char *)p_get_memory_data(RETRO_MEMORY_SAVE_RAM);
        ram = (unsigned char *)p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
        CHECK(card != NULL && ram != NULL);
        CHECK_EQ_U64((uintptr_t)card, card_address);
        CHECK_EQ_U64((uintptr_t)ram, ram_address);
        if (card != NULL && ram != NULL) {
            CHECK_EQ_HEX(card[12345], marker);
            CHECK(memcmp(ram + address, prefix + 0x800u, 16u) == 0);
        }
        CHECK_EQ_U64(p_get_region(), RETRO_REGION_NTSC);
        CHECK_EQ_U64(av_info_calls, old_av_calls);
        struct retro_system_av_info av;
        p_get_system_av_info(&av);
        check_av(&av, SCHWAY_REGION_NTSC_U);
    }
    p_unload_game();
    CHECK_EQ_U64(p_load_game(&game), 1u);
    CHECK_EQ_U64(p_get_region(), RETRO_REGION_PAL);
    p_unload_game();
    option_value = prior_option;
    option_updated = false;
}

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
    p_set_controller_port_device(0u, RETRO_DEVICE_JOYPAD);
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
    content_region_reload(exe_path);

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

static void check_card_file_value(const char *path, int expected)
{
    FILE *file = schway__test_open_file(path, "rb");
    CHECK(file != NULL);
    if (file == NULL) {
        return;
    }
    CHECK_EQ_U64(fseek(file, 128L, SEEK_SET), 0);
    CHECK_EQ_U64(fgetc(file), (unsigned)expected);
    CHECK_EQ_U64(fclose(file), 0u);
}

static int read_card_file(const char *path, unsigned char *bytes)
{
    FILE *file = schway__test_open_file(path, "rb");
    size_t count;
    int extra, failed;
    if (file == NULL) {
        return 0;
    }
    count = fread(bytes, 1u, SCHWAY_MEMCARD_SIZE, file);
    failed = ferror(file);
    extra = failed ? EOF : fgetc(file);
    if (fclose(file) != 0) {
        failed = 1;
    }
    return !failed && count == SCHWAY_MEMCARD_SIZE && extra == EOF;
}

static void check_card_file_equal(const char *path, const unsigned char *expected)
{
    unsigned char *actual = (unsigned char *)malloc(SCHWAY_MEMCARD_SIZE);
    CHECK(actual != NULL);
    if (actual != NULL) {
        int read_ok = read_card_file(path, actual);
        CHECK(read_ok);
        if (read_ok) {
            CHECK(memcmp(actual, expected, SCHWAY_MEMCARD_SIZE) == 0);
        }
        free(actual);
    }
}

static int test_cards_multitap(const char *directory)
{
    static const char *const card_names[8] = {
        "memcard_roundtrip.srm", "memcard_roundtrip.p1b.mcr",
        "memcard_roundtrip.p1c.mcr", "memcard_roundtrip.p1d.mcr",
        "memcard_roundtrip.1.mcr", "memcard_roundtrip.p2b.mcr",
        "memcard_roundtrip.p2c.mcr", "memcard_roundtrip.p2d.mcr",
    };
    char exe_path[4096] = {0};
    char failed_exe_path[4096] = {0};
    char playlist_path[4096] = {0};
    char bios_directory[4096] = {0};
    char swapped_card_path[4096] = {0};
    char paths[8][4096] = {{0}};
    char backup_path[4096] = {0};
    struct retro_game_info game;
    void *save_ram;
    const uint8_t *card;
    size_t size;
    uint64_t generation, saved_generation, prior_saved_generation;
    uint64_t receiving_generations[8] = {0};
    uint8_t receiving_bytes[8] = {0};
    uint8_t first_replies[6] = {0}, restored_replies[6] = {0};
    void *saved_state = NULL;
    size_t saved_state_size = 0u;
    const char *path;
    const char *cleared_path;
    int loaded = 0;
    int path_length;

    path_length = snprintf(exe_path, sizeof exe_path,
                           "%s/../../fixtures/memcard_roundtrip.exe", directory);
    CHECK(path_length > 0 && (size_t)path_length < sizeof exe_path);
    CHECK(test_path(backup_path, sizeof backup_path, directory,
                    "memcard_roundtrip.p1b.previous"));
    for (unsigned index = 0u; index < 8u; index++) {
        CHECK(test_path(paths[index], sizeof paths[index], directory, card_names[index]));
        (void)remove(paths[index]);
    }
    path_length = snprintf(failed_exe_path, sizeof failed_exe_path,
                           "%s/missing-save-content.exe", directory);
    CHECK(path_length > 0 && (size_t)path_length < sizeof failed_exe_path);
    path_length = snprintf(playlist_path, sizeof playlist_path,
                           "%s/../../fixtures/disc_boot_runner.m3u", directory);
    CHECK(path_length > 0 && (size_t)path_length < sizeof playlist_path);
    path_length = snprintf(bios_directory, sizeof bios_directory,
                           "%s/../../openbios", directory);
    CHECK(path_length > 0 && (size_t)path_length < sizeof bios_directory);
    CHECK(test_path(swapped_card_path, sizeof swapped_card_path, directory,
                    "disc_boot_runner.1.mcr"));
    (void)remove(swapped_card_path);
    (void)remove(backup_path);
    if (check_failures != 0) goto cleanup;

    save_directory = directory;
    save_directory_available = 1;
    memset(&game, 0, sizeof game);
    game.path = exe_path;
    loaded = p_load_game(&game) ? 1 : 0;
    CHECK(loaded);
    if (!loaded) goto cleanup;
    save_ram = p_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    CHECK(save_ram != NULL);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SAVE_RAM), SCHWAY_MEMCARD_SIZE);
    card = p_test_card_socket(1u, 1u, &size, &generation, &saved_generation, &path);
    CHECK(card != NULL && card == save_ram);
    CHECK_EQ_U64(size, SCHWAY_MEMCARD_SIZE);
    CHECK(path == NULL);

    /* A clean adapter card must not create a sidecar; card 1 remains frontend-owned. */
    ((uint8_t *)save_ram)[128u] = 0xA0u;
    p_test_flush_cards();
    for (unsigned index = 0u; index < 8u; index++) {
        FILE *file = fopen(paths[index], "rb");
        CHECK(file == NULL);
        if (file != NULL) (void)fclose(file);
    }

    /* Simulate one completed write per adapter-owned endpoint without booting a BIOS. */
    for (unsigned index = 1u; index < 8u; index++) {
        unsigned port_number = index / 4u + 1u;
        unsigned socket_number = index % 4u + 1u;
        uint8_t value = (uint8_t)(0x40u + index);
        card = p_test_card_socket(port_number, socket_number, &size,
                                  &generation, &saved_generation, &path);
        CHECK(card != NULL);
        CHECK_EQ_U64(size, SCHWAY_MEMCARD_SIZE);
        CHECK(path != NULL && strcmp(path, paths[index]) == 0);
        CHECK(p_test_write_card_socket(port_number, socket_number, value));
        card = p_test_card_socket(port_number, socket_number, &size,
                                  &generation, &saved_generation, &path);
        CHECK(card != NULL);
        CHECK_EQ_U64(card[128u], value);
        CHECK_EQ_U64(generation, saved_generation + 1u);
    }
    p_test_flush_cards();
    for (unsigned index = 1u; index < 8u; index++) {
        check_card_file_value(paths[index], (int)(0x40u + index));
    }
    {
        FILE *file = fopen(paths[0], "rb");
        CHECK(file == NULL);
        if (file != NULL) (void)fclose(file);
    }

    /* Failed replacement leaves the old image and generation retryable. */
    CHECK(copy_test_file(paths[1], backup_path));
    card = p_test_card_socket(1u, 2u, &size, &generation,
                              &saved_generation, &path);
    CHECK(card != NULL);
    prior_saved_generation = saved_generation;
    CHECK(p_test_write_card_socket(1u, 2u, 0x7Eu));
    CHECK(remove(paths[1]) == 0);
    CHECK(make_test_directory(paths[1]));
    p_test_flush_cards();
    card = p_test_card_socket(1u, 2u, &size, &generation,
                              &saved_generation, &path);
    CHECK(card != NULL);
    CHECK_EQ_U64(generation, prior_saved_generation + 1u);
    CHECK_EQ_U64(saved_generation, prior_saved_generation);
    check_card_file_value(backup_path, (int)(0x40u + 1u));
    CHECK(rmdir(paths[1]) == 0);
    CHECK(copy_test_file(backup_path, paths[1]));
    p_test_flush_cards();
    card = p_test_card_socket(1u, 2u, &size, &generation,
                              &saved_generation, &path);
    CHECK(card != NULL);
    CHECK_EQ_U64(generation, saved_generation);
    check_card_file_value(paths[1], 0x7E);

    p_unload_game();
    loaded = 0;
    loaded = p_load_game(&game) ? 1 : 0;
    CHECK(loaded);
    if (loaded) {
        for (unsigned index = 1u; index < 8u; index++) {
            unsigned port_number = index / 4u + 1u;
            unsigned socket_number = index % 4u + 1u;
            unsigned expected = index == 1u ? 0x7Eu : 0x40u + index;
            card = p_test_card_socket(port_number, socket_number, &size,
                                      &generation, &saved_generation, &path);
            CHECK(card != NULL);
            CHECK_EQ_U64(size, SCHWAY_MEMCARD_SIZE);
            CHECK(path != NULL && strcmp(path, paths[index]) == 0);
            CHECK_EQ_U64(card[128u], expected);
            CHECK_EQ_U64(generation, saved_generation);
        }
        save_ram = p_get_memory_data(RETRO_MEMORY_SAVE_RAM);
        CHECK(save_ram != NULL);
        CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SAVE_RAM), SCHWAY_MEMCARD_SIZE);
        card = p_test_card_socket(1u, 1u, &size, &generation, &saved_generation, &path);
        CHECK(card != NULL && card == save_ram);
        CHECK(path == NULL);

        /* A state restores the selected card's protocol, not any receiving image or generation. */
        CHECK(p_test_card_read_begin(1u));
        saved_state_size = p_serialize_size();
        saved_state = saved_state_size != 0u ? malloc(saved_state_size) : NULL;
        CHECK(saved_state != NULL);
        CHECK(saved_state != NULL && p_serialize(saved_state, saved_state_size));
        for (unsigned index = 0u; index < 8u; index++) {
            unsigned port_number = index / 4u + 1u;
            unsigned socket_number = index % 4u + 1u;
            uint8_t value = (uint8_t)(0x60u + index);
            CHECK(p_test_write_card_socket(port_number, socket_number, value));
            card = p_test_card_socket(port_number, socket_number, &size,
                                      &generation, &saved_generation, &path);
            CHECK(card != NULL);
            if (card != NULL) {
                receiving_bytes[index] = card[128u];
                receiving_generations[index] = generation;
                CHECK_EQ_U64(receiving_bytes[index], value);
            }
        }
        for (unsigned byte = 0u; byte < 6u; byte++) {
            CHECK(p_test_card_read_byte(1u, 0u, 1, &first_replies[byte]));
        }
        CHECK(p_unserialize(saved_state, saved_state_size));
        for (unsigned index = 0u; index < 8u; index++) {
            unsigned port_number = index / 4u + 1u;
            unsigned socket_number = index % 4u + 1u;
            card = p_test_card_socket(port_number, socket_number, &size,
                                      &generation, &saved_generation, &path);
            CHECK(card != NULL);
            if (card != NULL) {
                CHECK_EQ_U64(card[128u], receiving_bytes[index]);
                CHECK_EQ_U64(generation, receiving_generations[index]);
            }
        }
        for (unsigned byte = 0u; byte < 6u; byte++) {
            CHECK(p_test_card_read_byte(1u, 0u, 1, &restored_replies[byte]));
        }
        CHECK(memcmp(first_replies, restored_replies, sizeof first_replies) == 0);
        free(saved_state);
        saved_state = NULL;

        p_reset();
        for (unsigned index = 0u; index < 8u; index++) {
            unsigned port_number = index / 4u + 1u;
            unsigned socket_number = index % 4u + 1u;
            card = p_test_card_socket(port_number, socket_number, &size,
                                      &generation, &saved_generation, &path);
            CHECK(card != NULL);
            if (card != NULL) {
                CHECK_EQ_U64(card[128u], receiving_bytes[index]);
                CHECK_EQ_U64(generation, receiving_generations[index]);
            }
        }

        struct retro_game_info failed_game;
        memset(&failed_game, 0, sizeof failed_game);
        failed_game.path = failed_exe_path;
        CHECK(!p_load_game(&failed_game));
        CHECK(p_serialize_size() != 0u);
        for (unsigned index = 0u; index < 8u; index++) {
            unsigned port_number = index / 4u + 1u;
            unsigned socket_number = index % 4u + 1u;
            card = p_test_card_socket(port_number, socket_number, &size,
                                      &generation, &saved_generation, &path);
            CHECK(card != NULL);
            if (card != NULL) {
                CHECK_EQ_U64(card[128u], receiving_bytes[index]);
                CHECK_EQ_U64(generation, receiving_generations[index]);
                CHECK(index == 0u ? path == NULL :
                      path != NULL && strcmp(path, paths[index]) == 0);
            }
        }
    }

cleanup:
    free(saved_state);
    if (loaded) p_unload_game();
    for (unsigned index = 0u; index < 8u; index++) {
        (void)remove(paths[index]);
    }
    (void)rmdir(paths[1]);
    (void)remove(backup_path);
    for (unsigned index = 0u; index < 8u; index++) {
        uint64_t saved_after_unload = UINT64_MAX;
        cleared_path = (const char *)1;
        CHECK(p_test_card_lifecycle(index, &cleared_path, &saved_after_unload));
        CHECK(cleared_path == NULL);
        CHECK_EQ_U64(saved_after_unload, 0u);
    }

    /* M3U swaps keep sidecars tied to the initial playlist basename. */
    system_directory = bios_directory;
    bios_option_value = "none";
    save_directory = directory;
    save_directory_available = 1;
    memset(&game, 0, sizeof game);
    game.path = playlist_path;
    loaded = p_load_game(&game) ? 1 : 0;
    CHECK(loaded);
    if (loaded) {
        CHECK_EQ_U64(disk_control.get_num_images(), 2u);
        card = p_test_card_socket(2u, 1u, &size, &generation,
                                  &saved_generation, &path);
        CHECK(card != NULL);
        CHECK(path != NULL && strcmp(path, swapped_card_path) == 0);
        CHECK(disk_control.set_eject_state(true));
        CHECK(disk_control.set_image_index(1u));
        CHECK(disk_control.set_eject_state(false));
        card = p_test_card_socket(2u, 1u, &size, &generation,
                                  &saved_generation, &path);
        CHECK(card != NULL);
        CHECK(path != NULL && strcmp(path, swapped_card_path) == 0);
        for (unsigned index = 1u; index < 8u; index++) {
            static const char *const names[8] = {
                "disc_boot_runner.srm", "disc_boot_runner.p1b.mcr",
                "disc_boot_runner.p1c.mcr", "disc_boot_runner.p1d.mcr",
                "disc_boot_runner.1.mcr", "disc_boot_runner.p2b.mcr",
                "disc_boot_runner.p2c.mcr", "disc_boot_runner.p2d.mcr",
            };
            unsigned port_number = index / 4u + 1u;
            unsigned socket_number = index % 4u + 1u;
            char expected_path[4096];
            CHECK(test_path(expected_path, sizeof expected_path, directory, names[index]));
            card = p_test_card_socket(port_number, socket_number, &size,
                                      &generation, &saved_generation, &path);
            CHECK(card != NULL);
            CHECK(path != NULL && strcmp(path, expected_path) == 0);
        }
        p_unload_game();
        loaded = 0;
    }
    p_deinit();
    for (unsigned index = 0u; index < 8u; index++) {
        uint64_t saved_after_deinit = UINT64_MAX;
        cleared_path = (const char *)1;
        CHECK(p_test_card_lifecycle(index, &cleared_path, &saved_after_deinit));
        CHECK(cleared_path == NULL);
        CHECK_EQ_U64(saved_after_deinit, 0u);
    }
    (void)remove(swapped_card_path);
    int result = check_failures != 0 ? 1 : 0;
    module_close();
    return result;
}

static int test_card_persistence(const char *exe_path, const char *directory)
{
    char card_one[4096], card_two[4096];
    char no_injection_directory[4096], no_injection_card_one[4096];
    char no_injection_card_two[4096];
    struct retro_game_info game;
    unsigned char *saved_card_one = (unsigned char *)malloc(SCHWAY_MEMCARD_SIZE);
    unsigned char *saved_card_two = (unsigned char *)malloc(SCHWAY_MEMCARD_SIZE);
    unsigned char *saved_control_card_one = (unsigned char *)malloc(SCHWAY_MEMCARD_SIZE);
    void *save_ram;
    const uint8_t *card_two_data;
    const char *card_two_path;
    size_t card_two_size;
    int initialized = 1;
    int loaded = 0;

    CHECK(saved_card_one != NULL);
    CHECK(saved_card_two != NULL);
    CHECK(saved_control_card_one != NULL);
    CHECK(test_path(card_one, sizeof card_one, directory, "memcard_roundtrip.srm"));
    CHECK(test_path(card_two, sizeof card_two, directory, "memcard_roundtrip.1.mcr"));
    CHECK(test_path(no_injection_directory, sizeof no_injection_directory, directory,
                    "libretro-card-no-injection"));
    CHECK(test_path(no_injection_card_one, sizeof no_injection_card_one, no_injection_directory,
                    "memcard_roundtrip.srm"));
    CHECK(test_path(no_injection_card_two, sizeof no_injection_card_two, no_injection_directory,
                    "memcard_roundtrip.1.mcr"));
    if (check_failures != 0) {
        goto cleanup;
    }
    CHECK(make_test_directory(no_injection_directory));
    (void)remove(card_one);
    (void)remove(card_two);
    (void)remove(no_injection_card_one);
    (void)remove(no_injection_card_two);
    save_directory = directory;
    save_directory_available = 1;
    memset(&game, 0, sizeof game);
    game.path = exe_path;

    loaded = p_load_game(&game) ? 1 : 0;
    CHECK(loaded);
    if (!loaded) {
        goto cleanup;
    }
    save_ram = p_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    CHECK(save_ram != NULL);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SAVE_RAM), SCHWAY_MEMCARD_SIZE);
    CHECK(memory_map_calls > 0u);
    CHECK(memory_descriptor_count > 0u);
    if (save_ram == NULL || p_get_memory_size(RETRO_MEMORY_SAVE_RAM) != SCHWAY_MEMCARD_SIZE) {
        goto cleanup;
    }
    for (unsigned frame_index = 0u; frame_index < 120u; frame_index++) {
        run_one();
    }
    CHECK(p_get_memory_data(RETRO_MEMORY_SAVE_RAM) == save_ram);
    CHECK_EQ_U64(((const uint8_t *)save_ram)[128u], 1u);
    card_two_data = p_test_card2_data(&card_two_size);
    CHECK(card_two_data != NULL);
    CHECK_EQ_U64(card_two_size, SCHWAY_MEMCARD_SIZE);
    if (card_two_data != NULL && card_two_size == SCHWAY_MEMCARD_SIZE) {
        CHECK_EQ_U64(card_two_data[128u], 2u);
    }
    CHECK(write_test_file(card_one, save_ram, SCHWAY_MEMCARD_SIZE));
    CHECK(read_card_file(card_one, saved_card_one));
    CHECK_EQ_U64(saved_card_one[128u], 1u);
    p_unload_game();
    loaded = 0;
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SAVE_RAM), 0u);
    CHECK(read_card_file(card_two, saved_card_two));
    CHECK_EQ_U64(saved_card_two[128u], 2u);
    check_card_file_value(card_two, 2);

    /* The next load is a fresh emulator instance; the frontend restores all
       bytes from its own card-1 file before the guest gets its first frame. */
    p_deinit();
    initialized = 0;
    p_set_environment(env);
    p_init();
    initialized = 1;
    set_callbacks();
    CHECK(read_card_file(card_one, saved_card_one));
    loaded = p_load_game(&game) ? 1 : 0;
    CHECK(loaded);
    if (!loaded) {
        goto cleanup;
    }
    save_ram = p_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    CHECK(save_ram != NULL);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SAVE_RAM), SCHWAY_MEMCARD_SIZE);
    if (save_ram == NULL || p_get_memory_size(RETRO_MEMORY_SAVE_RAM) != SCHWAY_MEMCARD_SIZE) {
        goto cleanup;
    }
    memcpy(save_ram, saved_card_one, SCHWAY_MEMCARD_SIZE);
    CHECK_EQ_U64(((const uint8_t *)save_ram)[128u], 1u);
    card_two_data = p_test_card2_data(&card_two_size);
    card_two_path = p_test_card2_path();
    CHECK(card_two_data != NULL);
    CHECK_EQ_U64(card_two_size, SCHWAY_MEMCARD_SIZE);
    CHECK(card_two_path != NULL);
    if (card_two_data != NULL && card_two_size == SCHWAY_MEMCARD_SIZE) {
        CHECK_EQ_U64(card_two_data[128u], 2u);
    }
    CHECK(card_two_path != NULL && strcmp(card_two_path, card_two) == 0);
    for (unsigned frame_index = 0u; frame_index < 120u; frame_index++) {
        run_one();
    }
    CHECK_EQ_U64(((const uint8_t *)save_ram)[128u], 2u);
    card_two_data = p_test_card2_data(&card_two_size);
    CHECK(card_two_data != NULL);
    CHECK_EQ_U64(card_two_size, SCHWAY_MEMCARD_SIZE);
    if (card_two_data != NULL && card_two_size == SCHWAY_MEMCARD_SIZE) {
        CHECK_EQ_U64(card_two_data[128u], 3u);
    }
    CHECK(write_test_file(card_one, save_ram, SCHWAY_MEMCARD_SIZE));
    p_unload_game();
    loaded = 0;
    CHECK(read_card_file(card_one, saved_card_one));
    CHECK_EQ_U64(saved_card_one[128u], 2u);
    CHECK(read_card_file(card_two, saved_card_two));
    CHECK_EQ_U64(saved_card_two[128u], 3u);

    /* Copy both frontend saves to an isolated directory, then omit card-1
       reinjection. Card 1 must start fresh while card 2 still resumes. */
    CHECK(copy_test_file(card_one, no_injection_card_one));
    CHECK(copy_test_file(card_two, no_injection_card_two));
    CHECK(read_card_file(no_injection_card_one, saved_control_card_one));
    CHECK_EQ_U64(saved_control_card_one[128u], 2u);
    CHECK(read_card_file(no_injection_card_two, saved_card_two));
    CHECK_EQ_U64(saved_card_two[128u], 3u);

    p_deinit();
    initialized = 0;
    p_set_environment(env);
    p_init();
    initialized = 1;
    set_callbacks();
    save_directory = no_injection_directory;
    loaded = p_load_game(&game) ? 1 : 0;
    CHECK(loaded);
    if (!loaded) {
        goto cleanup;
    }
    save_ram = p_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    CHECK(save_ram != NULL);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SAVE_RAM), SCHWAY_MEMCARD_SIZE);
    if (save_ram == NULL || p_get_memory_size(RETRO_MEMORY_SAVE_RAM) != SCHWAY_MEMCARD_SIZE) {
        goto cleanup;
    }
    card_two_data = p_test_card2_data(&card_two_size);
    card_two_path = p_test_card2_path();
    CHECK(card_two_data != NULL);
    CHECK_EQ_U64(card_two_size, SCHWAY_MEMCARD_SIZE);
    CHECK(card_two_path != NULL && strcmp(card_two_path, no_injection_card_two) == 0);
    if (card_two_data != NULL && card_two_size == SCHWAY_MEMCARD_SIZE) {
        CHECK_EQ_U64(card_two_data[128u], 3u);
    }
    for (unsigned frame_index = 0u; frame_index < 120u; frame_index++) {
        run_one();
    }
    CHECK_EQ_U64(((const uint8_t *)save_ram)[128u], 1u);
    card_two_data = p_test_card2_data(&card_two_size);
    CHECK(card_two_data != NULL);
    CHECK_EQ_U64(card_two_size, SCHWAY_MEMCARD_SIZE);
    if (card_two_data != NULL && card_two_size == SCHWAY_MEMCARD_SIZE) {
        CHECK_EQ_U64(card_two_data[128u], 4u);
    }
    p_unload_game();
    loaded = 0;
    CHECK(read_card_file(no_injection_card_one, saved_control_card_one));
    CHECK_EQ_U64(saved_control_card_one[128u], 2u);
    CHECK(read_card_file(no_injection_card_two, saved_card_two));
    CHECK_EQ_U64(saved_card_two[128u], 4u);
    p_deinit();
    initialized = 0;

cleanup:
    if (loaded) {
        p_unload_game();
    }
    if (initialized) {
        p_deinit();
    }
    module_close();
    free(saved_card_one);
    free(saved_card_two);
    free(saved_control_card_one);
    return check_failures != 0 ? 1 : 0;
}

static int test_card_controls(const char *exe_path, const char *directory)
{
    enum { BLOCKER_COUNT = 64 };
    char card_one[4096], card_two[4096], backup_card[4096];
    char missing_directory[4096], missing_card_two[4096];
    char blocker[4096];
    static const unsigned char rejected_image[] = {'V', 'g', 's', 'M'};
    struct retro_game_info game;
    unsigned char *baseline = (unsigned char *)malloc(SCHWAY_MEMCARD_SIZE);
    unsigned char wrong_image = 0x5Au;
    int loaded;

    CHECK(baseline != NULL);
    CHECK(test_path(card_one, sizeof card_one, directory, "memcard_roundtrip.srm"));
    CHECK(test_path(card_two, sizeof card_two, directory, "memcard_roundtrip.1.mcr"));
    CHECK(test_path(backup_card, sizeof backup_card, directory, "previous-good-card.mcr"));
    CHECK(test_path(missing_directory, sizeof missing_directory, directory, "missing-save-dir"));
    CHECK(test_path(missing_card_two, sizeof missing_card_two, missing_directory,
                    "memcard_roundtrip.1.mcr"));
    if (baseline == NULL || check_failures != 0) {
        free(baseline);
        return 1;
    }
    (void)remove(card_one);
    (void)remove(card_two);
    (void)remove(backup_card);
    save_directory = directory;
    save_directory_available = 1;
    memset(&game, 0, sizeof game);
    game.path = exe_path;

    /* Seed a real, guest-written previous save for the refusal controls. */
    loaded = p_load_game(&game) ? 1 : 0;
    CHECK(loaded);
    for (unsigned frame_index = 0u; loaded && frame_index < 120u; frame_index++) {
        run_one();
    }
    p_unload_game();
    check_card_file_value(card_two, 2);
    CHECK(read_card_file(card_two, baseline));

    /* No card write means the same generation and no persistence attempt. */
    loaded = p_load_game(&game) ? 1 : 0;
    CHECK(loaded);
    if (loaded) {
        CHECK_EQ_U64(rename(card_two, backup_card), 0u);
        p_unload_game();
        {
            FILE *unexpected_write = schway__test_open_file(card_two, "rb");
            CHECK(unexpected_write == NULL);
            if (unexpected_write != NULL) {
                (void)fclose(unexpected_write);
            }
        }
        CHECK_EQ_U64(rename(backup_card, card_two), 0u);
    } else {
        p_unload_game();
    }
    check_card_file_equal(card_two, baseline);

    /* A truncated recognized wrapper is refused and remains untouched even
       when the guest writes to the live card and unloads. */
    CHECK(write_test_file(card_two, rejected_image, sizeof rejected_image));
    loaded = p_load_game(&game) ? 1 : 0;
    CHECK(loaded);
    for (unsigned frame_index = 0u; loaded && frame_index < 120u; frame_index++) {
        run_one();
    }
    p_unload_game();
    {
        unsigned char actual[sizeof rejected_image + 1u];
        FILE *rejected = schway__test_open_file(card_two, "rb");
        CHECK(rejected != NULL);
        if (rejected != NULL) {
            size_t count = fread(actual, 1u, sizeof actual, rejected);
            CHECK_EQ_U64(count, sizeof rejected_image);
            if (count >= sizeof rejected_image) {
                CHECK(memcmp(actual, rejected_image, sizeof rejected_image) == 0);
            }
            CHECK_EQ_U64(fclose(rejected), 0u);
        }
    }
    CHECK(write_test_file(card_two, baseline, SCHWAY_MEMCARD_SIZE));

    /* Exhaust the helper's exclusive temporary names. Atomic failure must
       leave the previous complete card untouched. */
    loaded = p_load_game(&game) ? 1 : 0;
    CHECK(loaded);
    for (unsigned frame_index = 0u; loaded && frame_index < 120u; frame_index++) {
        run_one();
    }
    for (unsigned i = 0u; i < BLOCKER_COUNT; i++) {
        int written = snprintf(blocker, sizeof blocker, "%s.schwaystation.tmp.%u", card_two, i);
        CHECK(written > 0 && (size_t)written < sizeof blocker);
        if (written > 0 && (size_t)written < sizeof blocker) {
            (void)remove(blocker);
            CHECK(write_test_file(blocker, &wrong_image, sizeof wrong_image));
        }
    }
    p_unload_game();
    check_card_file_equal(card_two, baseline);
    for (unsigned i = 0u; i < BLOCKER_COUNT; i++) {
        int written = snprintf(blocker, sizeof blocker, "%s.schwaystation.tmp.%u", card_two, i);
        if (written > 0 && (size_t)written < sizeof blocker) {
            (void)remove(blocker);
        }
    }

    /* An absent callback disables card2 I/O without changing the existing file. */
    save_directory_available = 0;
    loaded = p_load_game(&game) ? 1 : 0;
    CHECK(loaded);
    for (unsigned frame_index = 0u; loaded && frame_index < 120u; frame_index++) {
        run_one();
    }
    p_unload_game();
    check_card_file_equal(card_two, baseline);

    /* A callback can succeed while its directory is unavailable. The core
       still runs and the failed atomic create leaves that directory absent. */
    save_directory_available = 1;
    save_directory = missing_directory;
    loaded = p_load_game(&game) ? 1 : 0;
    CHECK(loaded);
    for (unsigned frame_index = 0u; loaded && frame_index < 120u; frame_index++) {
        run_one();
    }
    p_unload_game();
    {
        FILE *missing_file = schway__test_open_file(missing_card_two, "rb");
        CHECK(missing_file == NULL);
        if (missing_file != NULL) {
            (void)fclose(missing_file);
        }
    }
    save_directory = directory;
    save_directory_available = 1;
    check_card_file_equal(card_two, baseline);
    {
        FILE *card_one_file = schway__test_open_file(card_one, "rb");
        CHECK(card_one_file == NULL);
        if (card_one_file != NULL) {
            (void)fclose(card_one_file);
        }
    }
    (void)remove(card_two);
    (void)remove(backup_card);
    free(baseline);
    p_deinit();
    module_close();
    return check_failures != 0 ? 1 : 0;
}

static const struct retro_memory_descriptor *resolve_memory_address(size_t address,
                                                                      size_t *byte_offset)
{
    for (unsigned i = 0u; i < memory_descriptor_count; i++) {
        const struct retro_memory_descriptor *descriptor = &memory_descriptors[i];
        size_t mapped = address & ~descriptor->disconnect;
        size_t offset;
        if ((address & descriptor->select) != (descriptor->start & descriptor->select) ||
            mapped < descriptor->start) {
            continue;
        }
        offset = mapped - descriptor->start;
        if (descriptor->len != 0u && offset >= descriptor->len) {
            continue;
        }
        if (descriptor->ptr != NULL) {
            *byte_offset = descriptor->offset + offset;
            return descriptor;
        }
    }
    return NULL;
}

static const struct retro_memory_descriptor *check_memory_address(size_t address,
                                                                   size_t expected_offset)
{
    size_t actual_offset = 0u;
    const struct retro_memory_descriptor *descriptor = resolve_memory_address(address,
                                                                               &actual_offset);
    CHECK(descriptor != NULL);
    if (descriptor != NULL) {
        CHECK_EQ_U64(actual_offset, expected_offset);
    }
    return descriptor;
}

static int test_memory_aliases(void)
{
    static const size_t ram_bases[] = {0u, (size_t)0x80000000u, (size_t)0xA0000000u};
    void *system_ram;

    CHECK_EQ_U64(p_load_game(NULL), 1u);
    CHECK(memory_map_calls > 0u);
    CHECK_EQ_U64(memory_descriptor_count, 6u);
    CHECK(memory_descriptors[5].ptr == NULL);
    CHECK_EQ_U64(memory_descriptors[5].select, (size_t)0xFFFFFFFFu);
    system_ram = p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    CHECK(system_ram != NULL);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SYSTEM_RAM), 2u * 1024u * 1024u);

    for (size_t region = 0u; region < sizeof ram_bases / sizeof ram_bases[0]; region++) {
        const struct retro_memory_descriptor *first = check_memory_address(ram_bases[region], 0u);
        const struct retro_memory_descriptor *last =
            check_memory_address(ram_bases[region] + 0x001FFFFFu, 0x001FFFFFu);
        const struct retro_memory_descriptor *mirror_first =
            check_memory_address(ram_bases[region] + 0x00200000u, 0u);
        const struct retro_memory_descriptor *mirror_last =
            check_memory_address(ram_bases[region] + 0x007FFFFFu, 0x001FFFFFu);
        CHECK(first != NULL && first->ptr == system_ram);
        CHECK(last != NULL && last->ptr == system_ram);
        CHECK(mirror_first != NULL && mirror_first->ptr == system_ram);
        CHECK(mirror_last != NULL && mirror_last->ptr == system_ram);
        CHECK_EQ_U64(first != NULL ? first->disconnect : 0u, 0x00600000u);
        CHECK_EQ_U64(first != NULL ? first->select : 0u, ~(size_t)0x007FFFFFu);
    }

    CHECK(resolve_memory_address((size_t)0x00800000u, &(size_t){0u}) == NULL);
    CHECK(resolve_memory_address((size_t)0x80800000u, &(size_t){0u}) == NULL);
    CHECK(resolve_memory_address((size_t)0xA0800000u, &(size_t){0u}) == NULL);
    {
        const struct retro_memory_descriptor *scratch_first =
            check_memory_address((size_t)0x1F800000u, 0u);
        const struct retro_memory_descriptor *scratch_last =
            check_memory_address((size_t)0x1F8003FFu, 0x3FFu);
        const struct retro_memory_descriptor *scratch_alias =
            check_memory_address((size_t)0x9F800000u, 0u);
        CHECK(scratch_first != NULL && scratch_first->len == 0x400u);
        CHECK(scratch_last != NULL && scratch_last->ptr == scratch_first->ptr);
        CHECK(scratch_alias != NULL && scratch_alias->ptr == scratch_first->ptr);
    }
    CHECK(resolve_memory_address((size_t)0x1F800400u, &(size_t){0u}) == NULL);
    CHECK(resolve_memory_address((size_t)0xBF800000u, &(size_t){0u}) == NULL);

    p_unload_game();
    p_deinit();
    module_close();
    return check_failures != 0 ? 1 : 0;
}

static int test_memory_boundaries(void)
{
    unsigned map_calls;
    void *ram;
    size_t byte_offset = 0u;

    CHECK_EQ_U64(p_load_game(NULL), 1u);
    CHECK_EQ_U64(memory_descriptor_count, 6u);
    ram = p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    CHECK(ram != NULL);
    map_calls = memory_map_calls;
    CHECK(resolve_memory_address((size_t)0x80000000u, &byte_offset) != NULL);
    CHECK_EQ_U64(byte_offset, 0u);

    p_reset();
    CHECK(p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM) == ram);
    CHECK_EQ_U64(memory_map_calls, map_calls);
    CHECK_EQ_U64(memory_descriptor_count, 6u);
    byte_offset = 0u;
    CHECK(resolve_memory_address((size_t)0x807FFFFFu, &byte_offset) != NULL);
    CHECK_EQ_U64(byte_offset, 0x001FFFFFu);

    p_unload_game();
    CHECK(memory_map_calls > map_calls);
    CHECK_EQ_U64(memory_descriptor_count, 0u);
    CHECK(p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM) == NULL);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SYSTEM_RAM), 0u);

    CHECK_EQ_U64(p_load_game(NULL), 1u);
    CHECK_EQ_U64(memory_descriptor_count, 6u);
    CHECK(p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM) != NULL);
    p_deinit();
    CHECK_EQ_U64(memory_descriptor_count, 0u);
    CHECK(p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM) == NULL);

    /* A frontend that declines the optional map still gets a working core
       and direct SYSTEM_RAM access. */
    memory_maps_available = 0;
    map_calls = memory_map_calls;
    p_set_environment(env);
    p_init();
    set_callbacks();
    CHECK_EQ_U64(p_load_game(NULL), 1u);
    CHECK_EQ_U64(memory_map_calls, map_calls + 1u);
    CHECK_EQ_U64(memory_descriptor_count, 0u);
    CHECK(p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM) != NULL);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SYSTEM_RAM), 2u * 1024u * 1024u);
    p_unload_game();
    CHECK_EQ_U64(memory_descriptor_count, 0u);
    CHECK(p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM) == NULL);
    p_deinit();
    memory_maps_available = 1;
    module_close();
    return check_failures != 0 ? 1 : 0;
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
    FILE *file = schway__test_open_file(path, "rb");
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
    CHECK_EQ_U64(av_info_calls, 0u);
    CHECK_EQ_U64(geometry_calls, 1u);
    CHECK_EQ_U64(p_get_region(), RETRO_REGION_NTSC);
    struct retro_system_av_info av;
    memset(&av, 0, sizeof av);
    p_get_system_av_info(&av);
    check_av(&av, SCHWAY_REGION_NTSC_U);
    option_updated = true;
    run_one();
    CHECK_EQ_U64(av_info_calls, 0u);
    CHECK_EQ_U64(geometry_calls, 1u);

    p_unload_game();
    CHECK_EQ_U64(p_load_game(&game), 1u);
    CHECK_EQ_U64(p_get_region(), RETRO_REGION_PAL);
    memset(&av, 0, sizeof av);
    p_get_system_av_info(&av);
    check_av(&av, SCHWAY_REGION_PAL);
    CHECK_EQ_U64(av_info_calls, 0u);
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
            if (modes[i].interlaced != 0u) {
                struct retro_system_av_info av;
                memset(&av, 0, sizeof av);
                p_get_system_av_info(&av);
                check_av(&av, SCHWAY_REGION_NTSC_U);
            }
        }
        if (changed_dimensions) {
            CHECK_EQ_U64(geometry_sent.base_width, modes[i].width);
            CHECK_EQ_U64(geometry_sent.base_height, modes[i].height);
        }
    }

    option_value = "pal";
    option_updated = true;
    run_one();
    CHECK_EQ_U64(av_info_calls, 0u);
    CHECK_EQ_U64(geometry_calls, 2u);
    CHECK_EQ_U64(p_get_region(), RETRO_REGION_NTSC);
    struct retro_system_av_info av;
    memset(&av, 0, sizeof av);
    p_get_system_av_info(&av);
    check_av(&av, SCHWAY_REGION_NTSC_U);
    option_updated = true;
    run_one();
    CHECK_EQ_U64(av_info_calls, 0u);
    CHECK_EQ_U64(geometry_calls, 2u);

    p_unload_game();
    CHECK_EQ_U64(p_load_game(&game), 1u);
    CHECK_EQ_U64(p_get_region(), RETRO_REGION_PAL);
    memset(&av, 0, sizeof av);
    p_get_system_av_info(&av);
    check_av(&av, SCHWAY_REGION_PAL);
    CHECK_EQ_U64(av_info_calls, 0u);
    p_deinit();
    module_close();
    (void)remove(path);
    return check_failures != 0 ? 1 : 0;
}

struct input_state_snapshot {
    uint8_t device[2];
    uint8_t present[2];
    uint8_t analog_mode[2];
    uint16_t buttons[2];
    uint8_t axes[8];
};

#define INPUT_MOVIE_FRAMES 300u
#define INPUT_MOVIE_CHECKPOINTS 6u
#define INPUT_MOVIE_VARIANTS 4u

static const unsigned input_movie_checkpoints[INPUT_MOVIE_CHECKPOINTS] = {1u, 2u, 60u,
                                                                           100u, 120u, 300u};

struct movie_pixel_run {
    uint32_t count;
    uint32_t pixel;
};

struct movie_frame_reference {
    unsigned width, height;
    size_t run_count;
    struct movie_pixel_run *runs;
    size_t audio_pairs;
    int16_t *audio;
};

struct movie_state_reference {
    size_t size;
    unsigned char *bytes[INPUT_MOVIE_CHECKPOINTS];
};

struct movie_mailbox_reference {
    unsigned char bytes[0x100];
};

static uint32_t load_le32(const unsigned char *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) | ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static uint8_t disc_to_bcd(uint32_t value)
{
    value %= 100u;
    return (uint8_t)(((value / 10u) << 4) | (value % 10u));
}

static uint32_t disc_gettd_for_path(const char *path)
{
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    schway_toc toc;
    uint32_t result = 0u;
    memset(&error, 0, sizeof error);
    error.size = (uint32_t)sizeof error;
    if (schway_disc_image_open(path, &image, &error) == SCHWAY_DISC_IMAGE_OK && image != NULL) {
        const schway_disc *disc = schway_disc_image_disc(image);
        memset(&toc, 0, sizeof toc);
        toc.size = (uint32_t)sizeof toc;
        if (disc != NULL && disc->toc != NULL && disc->toc(disc->user, &toc) != 0u &&
            toc.track_count >= 2u) {
            uint64_t absolute = (uint64_t)toc.tracks[1].index1 + 150u;
            uint32_t minute = (uint32_t)(absolute / (60u * 75u));
            uint32_t second = (uint32_t)((absolute / 75u) % 60u);
            result = UINT32_C(0x02) | ((uint32_t)disc_to_bcd(minute) << 8) |
                     ((uint32_t)disc_to_bcd(second) << 16);
        }
    }
    schway_disc_image_close(image);
    return result;
}

static int disc_copy_canonical_path(const char *path, char *canonical, size_t capacity)
{
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    const char *image_path;
    size_t length;
    int valid = 0;

    if (path == NULL || canonical == NULL || capacity == 0u) return 0;
    memset(&error, 0, sizeof error);
    error.size = (uint32_t)sizeof error;
    if (schway_disc_image_open(path, &image, &error) != SCHWAY_DISC_IMAGE_OK || image == NULL) {
        goto cleanup;
    }
    image_path = schway_disc_image_path(image, 0u);
    if (image_path == NULL) goto cleanup;
    length = strlen(image_path);
    if (length >= capacity) goto cleanup;
    memcpy(canonical, image_path, length + 1u);
    valid = 1;

cleanup:
    schway_disc_image_close(image);
    return valid;
}

static uint32_t guest_mailbox_record(const unsigned char *ram, unsigned id)
{
    const unsigned char *mailbox = ram + 0x10000u;
    unsigned count = mailbox[0x0Au];
    for (unsigned i = 0u; i < count && i < 30u; i++) {
        const unsigned char *record = mailbox + 0x10u + (size_t)i * 8u;
        if (load_le32(record) == (UINT32_C(0x00010000) | id)) {
            return load_le32(record + 4u);
        }
    }
    return 0u;
}

static unsigned run_disc_to_mailbox(uint32_t expected_toc)
{
    unsigned char *ram = NULL;
    unsigned frames = 0u;
    for (; frames < 1200u; frames++) {
        run_one();
        ram = (unsigned char *)p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
        if (ram != NULL && memcmp(ram + 0x10000u, "SWMB", 4u) == 0 &&
            ram[0x10008u] == 0u) break;
    }
    CHECK(frames < 1200u);
    CHECK(ram != NULL);
    if (frames < 1200u && ram != NULL) {
        CHECK_EQ_HEX(guest_mailbox_record(ram, 2u), UINT32_C(0x00020102));
        CHECK_EQ_HEX(guest_mailbox_record(ram, 4u), expected_toc);
        return frames + 1u;
    }
    return 0u;
}

static void host_write_iso_u16_both(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 8);
    bytes[3] = (uint8_t)value;
}

static void host_write_iso_u32_both(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
    bytes[4] = (uint8_t)(value >> 24);
    bytes[5] = (uint8_t)(value >> 16);
    bytes[6] = (uint8_t)(value >> 8);
    bytes[7] = (uint8_t)value;
}

static size_t host_write_iso_record(uint8_t *bytes, uint32_t extent, uint32_t size,
                                    uint8_t flags, const uint8_t *name,
                                    size_t name_size)
{
    size_t record_size = 33u + name_size + ((name_size & 1u) == 0u ? 1u : 0u);
    memset(bytes, 0, record_size);
    bytes[0] = (uint8_t)record_size;
    host_write_iso_u32_both(bytes + 2u, extent);
    host_write_iso_u32_both(bytes + 10u, size);
    bytes[25] = flags;
    host_write_iso_u16_both(bytes + 28u, 1u);
    bytes[32] = (uint8_t)name_size;
    memcpy(bytes + 33u, name, name_size);
    return record_size;
}

static int write_nonboot_iso(const char *path)
{
    static const uint8_t dot[] = {0u};
    static const uint8_t dotdot[] = {1u};
    const size_t sector_size = 2048u;
    uint8_t *image = (uint8_t *)calloc(24u, sector_size);
    uint8_t *pvd;
    uint8_t *root;
    size_t root_size = 0u;
    int ok;
    if (image == NULL) {
        return 0;
    }
    pvd = image + 16u * sector_size;
    pvd[0] = 1u;
    memcpy(pvd + 1u, "CD001", 5u);
    pvd[6] = 1u;
    host_write_iso_u32_both(pvd + 80u, 24u);
    host_write_iso_u16_both(pvd + 128u, (uint16_t)sector_size);
    (void)host_write_iso_record(pvd + 156u, 18u, (uint32_t)sector_size,
                                2u, dot, sizeof dot);
    uint8_t *terminator = image + 17u * sector_size;
    terminator[0] = 255u;
    memcpy(terminator + 1u, "CD001", 5u);
    terminator[6] = 1u;
    root = image + 18u * sector_size;
    root_size += host_write_iso_record(root + root_size, 18u,
                                       (uint32_t)sector_size, 2u,
                                       dot, sizeof dot);
    root_size += host_write_iso_record(root + root_size, 18u,
                                       (uint32_t)sector_size, 2u,
                                       dotdot, sizeof dotdot);
    ok = root_size <= sector_size &&
         write_test_file(path, image, 24u * sector_size);
    free(image);
    return ok;
}

static void test_content_boot(const char *playlist_path,
                              const char *selected_playlist_path,
                              const char *selected_no_boot_path,
                              const char *bios_directory,
                              const char *test_directory)
{
    unsigned char ineligible_image[32768];
    unsigned char full_mailbox[0x100];
    unsigned char *ram;
    char ineligible_path[4096];
    char selected_no_boot_canonical[4096];
    struct retro_game_info game;
    unsigned full_frames = 0u;
    unsigned fast_frames = 0u;
    uint32_t expected_toc;
    int loaded;
    unsigned video_before;

    memset(full_mailbox, 0, sizeof full_mailbox);
    memset(ineligible_image, 'S', sizeof ineligible_image);
    ineligible_path[0] = '\0';
    int path_ok = test_path(ineligible_path, sizeof ineligible_path, test_directory,
                            "ineligible.iso");
    selected_no_boot_canonical[0] = '\0';
    CHECK(path_ok);
    if (!path_ok) {
        return;
    }
    int image_written = write_test_file(ineligible_path, ineligible_image,
                                       sizeof ineligible_image);
    CHECK(image_written);
    if (!image_written) {
        (void)remove(ineligible_path);
        return;
    }
    int selected_image_written = write_nonboot_iso(selected_no_boot_path);
    CHECK(selected_image_written);
    int selected_path_valid = disc_copy_canonical_path(
        selected_no_boot_path, selected_no_boot_canonical,
        sizeof selected_no_boot_canonical);
    CHECK(selected_path_valid);
    if (!selected_image_written || !selected_path_valid) {
        (void)remove(ineligible_path);
        return;
    }

    system_directory = bios_directory;
    save_directory = test_directory;
    save_directory_available = 1;
    bios_option_value = "none";
    machine_option_value = "pu18";
    option_value = "ntsc-u";
    expected_toc = disc_gettd_for_path(playlist_path);
    CHECK(expected_toc != 0u);
    memset(&game, 0, sizeof game);
    game.path = playlist_path;

    /* A missing frontend value follows the declared full-boot default. */
    boot_option_value = NULL;
    loaded = p_load_game(&game);
    CHECK_EQ_U64(loaded, 1u);
    if (loaded) {
        full_frames = run_disc_to_mailbox(expected_toc);
        ram = (unsigned char *)p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
        CHECK(ram != NULL);
        if (ram != NULL) {
            memcpy(full_mailbox, ram + 0x10000u, sizeof full_mailbox);
        }
        p_unload_game();
    }

    boot_option_value = "fast";
    loaded = p_load_game(&game);
    CHECK_EQ_U64(loaded, 1u);
    if (loaded) {
        fast_frames = run_disc_to_mailbox(expected_toc);
        ram = (unsigned char *)p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
        CHECK(ram != NULL);
        if (ram != NULL) {
            CHECK(memcmp(full_mailbox, ram + 0x10000u, sizeof full_mailbox) == 0);
        }
        p_unload_game();
    }
    CHECK(full_frames != 0u && fast_frames != 0u);

    /* The initial member, rather than playlist member zero, controls identity
       and fast-boot eligibility. The selected member has a valid ISO volume
       but no SYSTEM.CNF boot executable. */
    game.path = selected_playlist_path;
    option_value = "auto";
    boot_option_value = "full";
    CHECK(disk_control.set_initial_image(1u, selected_no_boot_canonical));
    loaded = p_load_game(&game);
    CHECK_EQ_U64(loaded, 1u);
    if (loaded) {
        CHECK_EQ_U64(p_get_region(), RETRO_REGION_NTSC);
        p_unload_game();
    }
    boot_option_value = "fast";
    CHECK(disk_control.set_initial_image(1u, selected_no_boot_canonical));
    video_before = video_calls;
    loaded = p_load_game(&game);
    CHECK_EQ_U64(loaded, 0u);
    CHECK_EQ_U64(p_serialize_size(), 0u);
    CHECK_EQ_U64(video_calls, video_before);
    p_unload_game();
    option_value = "ntsc-u";

    /* A selected but missing BIOS rejects fast mode before guest execution. */
    bios_option_value = "missing.bin";
    game.path = playlist_path;
    video_before = video_calls;
    CHECK_EQ_U64(p_load_game(&game), 0u);
    CHECK_EQ_U64(p_serialize_size(), 0u);
    CHECK_EQ_U64(video_calls, video_before);

    /* A readable image without a valid boot executable also rejects fast. */
    bios_option_value = "none";
    game.path = ineligible_path;
    video_before = video_calls;
    CHECK_EQ_U64(p_load_game(&game), 0u);
    CHECK_EQ_U64(p_serialize_size(), 0u);
    CHECK_EQ_U64(video_calls, video_before);
    p_unload_game();
    (void)remove(ineligible_path);
}

static int test_content_chd(const char *chd_path, const char *cue_path,
                            const char *bad_chd_path, const char *bios_directory,
                            const char *card_directory, int chd_enabled)
{
    struct retro_game_info game;
    struct retro_system_info sys;
    unsigned char *saved_ram = NULL;
    unsigned char *cue_ram = NULL;
    unsigned char *saved_card = NULL;
    unsigned char *system_ram;
    unsigned char *active_system_ram = NULL;
    const void *active_card = NULL;
    char active_path[4096];
    char expected_content_path[4096];
    char saved_card2_path[4096];
    unsigned save_calls_before = save_directory_calls;
    unsigned frames;
    uint32_t expected_toc;
    const char *content_path = chd_enabled ? chd_path : cue_path;

    expected_content_path[0] = '\0';
    CHECK(disc_copy_canonical_path(content_path, expected_content_path,
                                   sizeof expected_content_path));

    memset(&sys, 0, sizeof sys);
    p_get_system_info(&sys);
    CHECK_EQ_U64(chd_enabled, SCHWAYSTATION_CHD);
    check_string("CHD-enabled extensions", sys.valid_extensions, expected_valid_extensions());
    CHECK(disk_control_set_calls != 0u);
    CHECK(disk_control.get_image_path != NULL);
    CHECK(disk_control.get_num_images != NULL);
    CHECK(disk_control.get_image_index != NULL);
    system_directory = bios_directory;
    save_directory = card_directory;
    save_directory_available = 1;
    bios_option_value = "none";
    expected_toc = disc_gettd_for_path(content_path);
    CHECK(expected_toc != 0u);

    memset(&game, 0, sizeof game);
    game.path = content_path;
    int loaded = p_load_game(&game);
    CHECK_EQ_U64(loaded, 1u);
    if (!loaded) goto cleanup;
    CHECK_EQ_U64(disk_control.get_num_images(), 1u);
    CHECK_EQ_U64(disk_control.get_image_index(), 0u);
    memset(active_path, 0, sizeof active_path);
    CHECK(disk_control.get_image_path(0u, active_path, sizeof active_path));
    CHECK(strcmp(active_path, expected_content_path) == 0);
    frames = run_disc_to_mailbox(expected_toc);
    CHECK(frames != 0u);
    system_ram = (unsigned char *)p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    CHECK(system_ram != NULL);
    active_system_ram = system_ram;
    saved_ram = system_ram != NULL ? (unsigned char *)malloc(2u * 1024u * 1024u) : NULL;
    CHECK(saved_ram != NULL);
    if (system_ram != NULL && saved_ram != NULL) {
        memcpy(saved_ram, system_ram, 2u * 1024u * 1024u);
    }
    active_card = p_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    CHECK(active_card != NULL);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SAVE_RAM), SCHWAY_MEMCARD_SIZE);
    if (active_card != NULL) {
        saved_card = (unsigned char *)malloc(SCHWAY_MEMCARD_SIZE);
        CHECK(saved_card != NULL);
        if (saved_card != NULL) memcpy(saved_card, active_card, SCHWAY_MEMCARD_SIZE);
    }
    const char *card2_path = p_test_card2_path();
    CHECK(card2_path != NULL);
    if (card2_path != NULL && strlen(card2_path) < sizeof saved_card2_path) {
        memcpy(saved_card2_path, card2_path, strlen(card2_path) + 1u);
    } else {
        saved_card2_path[0] = '\0';
    }

    memset(&game, 0, sizeof game);
    game.path = chd_enabled ? bad_chd_path : chd_path;
    CHECK_EQ_U64(p_load_game(&game), 0u);
    system_ram = (unsigned char *)p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    CHECK(system_ram != NULL);
    CHECK(system_ram == active_system_ram);
    if (system_ram != NULL && saved_ram != NULL) {
        CHECK(memcmp(saved_ram, system_ram, 2u * 1024u * 1024u) == 0);
    }
    CHECK(p_get_memory_data(RETRO_MEMORY_SAVE_RAM) == active_card);
    CHECK_EQ_U64(disk_control.get_num_images(), 1u);
    CHECK_EQ_U64(disk_control.get_image_index(), 0u);
    memset(active_path, 0, sizeof active_path);
    CHECK(disk_control.get_image_path(0u, active_path, sizeof active_path));
    CHECK(strcmp(active_path, expected_content_path) == 0);
    card2_path = p_test_card2_path();
    CHECK(card2_path != NULL);
    if (card2_path != NULL && saved_card2_path[0] != '\0') {
        CHECK(strcmp(card2_path, saved_card2_path) == 0);
    }
    CHECK_EQ_U64(save_directory_calls, save_calls_before + 1u);

    if (chd_enabled) {
        p_unload_game();
        CHECK_EQ_U64(p_serialize_size(), 0u);
        memset(&game, 0, sizeof game);
        game.path = cue_path;
        loaded = p_load_game(&game);
        CHECK_EQ_U64(loaded, 1u);
        if (!loaded) goto cleanup;
        CHECK_EQ_U64(disk_control.get_num_images(), 1u);
        CHECK_EQ_U64(disk_control.get_image_index(), 0u);
        uint32_t cue_toc = disc_gettd_for_path(cue_path);
        CHECK_EQ_U64(cue_toc, expected_toc);
        unsigned cue_frames = run_disc_to_mailbox(cue_toc);
        CHECK_EQ_U64(cue_frames, frames);
        system_ram = (unsigned char *)p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
        CHECK(system_ram != NULL);
        cue_ram = system_ram != NULL ? (unsigned char *)malloc(2u * 1024u * 1024u) : NULL;
        CHECK(cue_ram != NULL);
        if (system_ram != NULL && cue_ram != NULL) {
            memcpy(cue_ram, system_ram, 2u * 1024u * 1024u);
        }
        if (saved_ram != NULL && cue_ram != NULL) {
            CHECK(memcmp(saved_ram, cue_ram, 2u * 1024u * 1024u) == 0);
        }
        const unsigned char *cue_card =
            (const unsigned char *)p_get_memory_data(RETRO_MEMORY_SAVE_RAM);
        CHECK(cue_card != NULL);
        if (cue_card != NULL && saved_card != NULL) {
            CHECK(memcmp(saved_card, cue_card, SCHWAY_MEMCARD_SIZE) == 0);
        }
        card2_path = p_test_card2_path();
        CHECK(card2_path != NULL);
        if (card2_path != NULL && saved_card2_path[0] != '\0') {
            CHECK(strcmp(card2_path, saved_card2_path) == 0);
        }
        CHECK_EQ_U64(save_directory_calls, save_calls_before + 2u);
    }

cleanup:
    p_unload_game();
    CHECK_EQ_U64(p_serialize_size(), 0u);
    free(cue_ram);
    free(saved_card);
    free(saved_ram);
    return check_failures == 0 ? 0 : 1;
}

struct pbp_frame_trace {
    uint64_t video_hash;
    uint64_t audio_hash;
    size_t audio_pairs;
    unsigned width;
    unsigned height;
};

static uint64_t pbp_hash_bytes(uint64_t hash, const void *data, size_t size)
{
    const unsigned char *bytes = (const unsigned char *)data;
    for (size_t i = 0u; i < size; i++) {
        hash = (hash ^ bytes[i]) * UINT64_C(1099511628211);
    }
    return hash;
}

static int legacy_images_match(const char *image_path, const char *cue_path,
                               int compare_subcode)
{
    schway_disc_image *image = NULL;
    schway_disc_image *cue = NULL;
    schway_disc_image_error error;
    const schway_disc *image_disc;
    const schway_disc *cue_disc;
    schway_toc image_toc, cue_toc;
    uint8_t image_sector[2352], cue_sector[2352];
    uint8_t image_subcode[96], cue_subcode[96];
    int matched = 0;

    memset(&error, 0, sizeof error);
    error.size = (uint32_t)sizeof error;
    CHECK_EQ_U64(schway_disc_image_open(image_path, &image, &error), SCHWAY_DISC_IMAGE_OK);
    memset(&error, 0, sizeof error);
    error.size = (uint32_t)sizeof error;
    CHECK_EQ_U64(schway_disc_image_open(cue_path, &cue, &error), SCHWAY_DISC_IMAGE_OK);
    if (image == NULL || cue == NULL) goto cleanup;
    image_disc = schway_disc_image_disc(image);
    cue_disc = schway_disc_image_disc(cue);
    CHECK(image_disc != NULL && cue_disc != NULL);
    if (image_disc == NULL || cue_disc == NULL || image_disc->toc == NULL ||
        cue_disc->toc == NULL || image_disc->read_sector == NULL ||
        cue_disc->read_sector == NULL) goto cleanup;
    memset(&image_toc, 0, sizeof image_toc);
    memset(&cue_toc, 0, sizeof cue_toc);
    image_toc.size = (uint32_t)sizeof image_toc;
    cue_toc.size = (uint32_t)sizeof cue_toc;
    CHECK(image_disc->toc(image_disc->user, &image_toc) != 0u);
    CHECK(cue_disc->toc(cue_disc->user, &cue_toc) != 0u);
    CHECK(memcmp(&image_toc, &cue_toc, sizeof image_toc) == 0);
    CHECK_EQ_U64(image_disc->identity_size, cue_disc->identity_size);
    if (image_disc->identity_size == cue_disc->identity_size &&
        image_disc->identity_size == sizeof image_disc->identity_sha256) {
        CHECK(memcmp(image_disc->identity_sha256, cue_disc->identity_sha256,
                     sizeof image_disc->identity_sha256) == 0);
    }
    if (image_toc.track_count == 0u ||
        image_toc.track_count > sizeof image_toc.tracks / sizeof image_toc.tracks[0]) {
        CHECK(0);
        goto cleanup;
    }
    uint32_t leadout = image_toc.tracks[image_toc.track_count - 1u].leadout;
    CHECK(leadout != 0u);
    for (uint32_t lba = 0u; lba < leadout; lba++) {
        uint32_t image_read = image_disc->read_sector(image_disc->user, lba,
                                                       image_sector, image_subcode);
        uint32_t cue_read = cue_disc->read_sector(cue_disc->user, lba,
                                                  cue_sector, cue_subcode);
        CHECK(image_read != 0u);
        CHECK(cue_read != 0u);
        if (image_read == 0u || cue_read == 0u) break;
        CHECK(memcmp(image_sector, cue_sector, sizeof image_sector) == 0);
        if (compare_subcode) {
            CHECK(memcmp(image_subcode, cue_subcode, sizeof image_subcode) == 0);
        }
    }
    matched = check_failures == 0;

cleanup:
    schway_disc_image_close(cue);
    schway_disc_image_close(image);
    return matched;
}

static void capture_content_trace(struct pbp_frame_trace *trace, unsigned frames)
{
    for (unsigned i = 0u; i < frames; i++) {
        run_one();
        int valid_frame = frame_w != 0u && frame_w <= MAX_W &&
                          frame_h != 0u && frame_h <= MAX_H;
        CHECK(valid_frame);
        CHECK(got_pairs <= MAX_PAIRS);
        CHECK_EQ_U64(video_bad, 0u);
        trace[i].width = frame_w;
        trace[i].height = frame_h;
        trace[i].audio_pairs = got_pairs;
        trace[i].video_hash = valid_frame
            ? pbp_hash_bytes(UINT64_C(14695981039346656037), frame,
                (size_t)frame_w * frame_h * sizeof frame[0]) : 0u;
        trace[i].audio_hash = pbp_hash_bytes(UINT64_C(14695981039346656037), got_audio,
            got_pairs <= MAX_PAIRS ? got_pairs * 2u * sizeof got_audio[0] : 0u);
    }
}

static void check_content_trace_equal(const struct pbp_frame_trace *left,
                                      const struct pbp_frame_trace *right,
                                      unsigned frames)
{
    for (unsigned i = 0u; i < frames; i++) {
        CHECK_EQ_U64(left[i].width, right[i].width);
        CHECK_EQ_U64(left[i].height, right[i].height);
        CHECK_EQ_U64(left[i].audio_pairs, right[i].audio_pairs);
        CHECK_EQ_U64(left[i].video_hash, right[i].video_hash);
        CHECK_EQ_U64(left[i].audio_hash, right[i].audio_hash);
    }
}

static int make_pbp_pgd_variant(const char *source_path, const char *variant_path)
{
    FILE *file = schway__test_open_file(source_path, "rb");
    unsigned char *bytes = NULL;
    long length;
    uint32_t psar_offset;
    int ok = 0;
    if (file == NULL) return 0;
    if (fseek(file, 0L, SEEK_END) != 0 || (length = ftell(file)) < 40L ||
        length > 1572864L || fseek(file, 0L, SEEK_SET) != 0) goto cleanup;
    bytes = (unsigned char *)malloc((size_t)length);
    if (bytes == NULL || fread(bytes, 1u, (size_t)length, file) != (size_t)length) goto cleanup;
    psar_offset = load_le32(bytes + 36u);
    if ((uint64_t)psar_offset + 3u > (uint64_t)length) goto cleanup;
    memcpy(bytes + psar_offset, "PGD", 3u);
    ok = write_test_file(variant_path, bytes, (size_t)length);

cleanup:
    free(bytes);
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static int pbp_images_match(const char *pbp_path, const char *cue_path)
{
    schway_disc_image *pbp = NULL;
    schway_disc_image *cue = NULL;
    schway_disc_image_error error;
    schway_toc pbp_toc, cue_toc;
    const schway_disc *pbp_disc;
    const schway_disc *cue_disc;
    uint8_t pbp_sector[2352], cue_sector[2352];
    uint8_t pbp_subcode[96], cue_subcode[96];
    int ok = 0;

    memset(&error, 0, sizeof error);
    error.size = (uint32_t)sizeof error;
    CHECK_EQ_U64(schway_disc_image_open(pbp_path, &pbp, &error), SCHWAY_DISC_IMAGE_OK);
    memset(&error, 0, sizeof error);
    error.size = (uint32_t)sizeof error;
    CHECK_EQ_U64(schway_disc_image_open(cue_path, &cue, &error), SCHWAY_DISC_IMAGE_OK);
    if (pbp == NULL || cue == NULL) goto cleanup;
    pbp_disc = schway_disc_image_disc(pbp);
    cue_disc = schway_disc_image_disc(cue);
    CHECK(pbp_disc != NULL && cue_disc != NULL);
    if (pbp_disc == NULL || cue_disc == NULL || pbp_disc->toc == NULL || cue_disc->toc == NULL ||
        pbp_disc->read_sector == NULL || cue_disc->read_sector == NULL) goto cleanup;
    memset(&pbp_toc, 0, sizeof pbp_toc);
    memset(&cue_toc, 0, sizeof cue_toc);
    pbp_toc.size = (uint32_t)sizeof pbp_toc;
    cue_toc.size = (uint32_t)sizeof cue_toc;
    CHECK(pbp_disc->toc(pbp_disc->user, &pbp_toc) != 0u);
    CHECK(cue_disc->toc(cue_disc->user, &cue_toc) != 0u);
    CHECK(memcmp(&pbp_toc, &cue_toc, sizeof pbp_toc) == 0);
    CHECK_EQ_U64(pbp_disc->identity_size, cue_disc->identity_size);
    if (pbp_disc->identity_size == cue_disc->identity_size &&
        pbp_disc->identity_size == sizeof pbp_disc->identity_sha256) {
        CHECK(memcmp(pbp_disc->identity_sha256, cue_disc->identity_sha256,
                     sizeof pbp_disc->identity_sha256) == 0);
    }
    if (pbp_toc.track_count != 1u || cue_toc.track_count != 1u) goto cleanup;
    for (uint32_t lba = 0u; lba < pbp_toc.tracks[0].leadout; lba++) {
        uint32_t pbp_read = pbp_disc->read_sector(pbp_disc->user, lba,
                                                  pbp_sector, pbp_subcode);
        uint32_t cue_read = cue_disc->read_sector(cue_disc->user, lba,
                                                  cue_sector, cue_subcode);
        CHECK(pbp_read != 0u);
        CHECK(cue_read != 0u);
        if (pbp_read == 0u || cue_read == 0u) break;
        CHECK(memcmp(pbp_sector, cue_sector, sizeof pbp_sector) == 0);
        CHECK(memcmp(pbp_subcode, cue_subcode, sizeof pbp_subcode) == 0);
    }
    ok = check_failures == 0;

cleanup:
    schway_disc_image_close(cue);
    schway_disc_image_close(pbp);
    return ok;
}

static int test_pbp_rejection_unchanged(const char *path, uint32_t want_status,
                                        const unsigned char *saved_ram,
                                        const unsigned char *saved_card,
                                        const unsigned char *saved_card2,
                                        const char *saved_card2_path,
                                        const char *active_path)
{
    struct retro_game_info game;
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    unsigned char *ram;
    const unsigned char *card2;
    size_t card2_size = 0u;
    char current_path[4096];
    memset(&game, 0, sizeof game);
    game.path = path;
    CHECK_EQ_U64(p_load_game(&game), 0u);
    memset(&error, 0, sizeof error);
    error.size = (uint32_t)sizeof error;
    CHECK_EQ_U64(schway_disc_image_open(path, &image, &error), want_status);
    schway_disc_image_close(image);
    ram = (unsigned char *)p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    CHECK(ram != NULL);
    if (ram != NULL && saved_ram != NULL) {
        CHECK(memcmp(ram, saved_ram, 2u * 1024u * 1024u) == 0);
    }
    const unsigned char *card =
        (const unsigned char *)p_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    CHECK(card != NULL);
    if (card != NULL && saved_card != NULL) {
        CHECK(memcmp(card, saved_card, SCHWAY_MEMCARD_SIZE) == 0);
    }
    card2 = p_test_card2_data(&card2_size);
    CHECK(card2 != NULL);
    CHECK_EQ_U64(card2_size, SCHWAY_MEMCARD_SIZE);
    if (card2 != NULL && saved_card2 != NULL && card2_size == SCHWAY_MEMCARD_SIZE) {
        CHECK(memcmp(card2, saved_card2, SCHWAY_MEMCARD_SIZE) == 0);
    }
    const char *card2_path = p_test_card2_path();
    CHECK(card2_path != NULL && strcmp(card2_path, saved_card2_path) == 0);
    CHECK_EQ_U64(disk_control.get_num_images(), 1u);
    CHECK_EQ_U64(disk_control.get_image_index(), 0u);
    memset(current_path, 0, sizeof current_path);
    CHECK(disk_control.get_image_path(0u, current_path, sizeof current_path));
    CHECK(strcmp(current_path, active_path) == 0);
    return check_failures == 0;
}

static int test_content_pbp(const char *pbp_path, const char *cue_path,
                            const char *malformed_path, const char *bios_directory,
                            const char *card_directory, int chd_enabled)
{
    enum { PBP_TRACE_FRAMES = 1200 };
    struct retro_game_info game;
    struct retro_system_info sys;
    struct pbp_frame_trace pbp_trace[PBP_TRACE_FRAMES];
    struct pbp_frame_trace cue_trace[PBP_TRACE_FRAMES];
    unsigned char *pbp_ram = NULL;
    unsigned char *cue_ram = NULL;
    unsigned char *saved_ram = NULL;
    unsigned char *saved_card = NULL;
    unsigned char *saved_card2 = NULL;
    size_t card2_size = 0u;
    char active_path[4096];
    char expected_path[4096];
    char unsupported_path[4096];
    char saved_card2_path[4096];
    unsigned char *system_ram;
    const unsigned char *card2;
    unsigned save_calls_before = save_directory_calls;
    int result = 1;

    CHECK_EQ_U64(chd_enabled, SCHWAYSTATION_CHD);
    memset(&sys, 0, sizeof sys);
    p_get_system_info(&sys);
    check_string("PBP-enabled extensions", sys.valid_extensions, expected_valid_extensions());
    CHECK(disk_control_set_calls != 0u);
    CHECK(disk_control.get_image_path != NULL && disk_control.get_num_images != NULL &&
          disk_control.get_image_index != NULL);
    CHECK(pbp_images_match(pbp_path, cue_path));
    CHECK(test_path(unsupported_path, sizeof unsupported_path, card_directory,
                    "unsupported.pbp"));
    CHECK(make_pbp_pgd_variant(pbp_path, unsupported_path));
    CHECK_EQ_U64(save_directory_calls, save_calls_before);
    if (check_failures != 0) goto cleanup;

    system_directory = bios_directory;
    save_directory = card_directory;
    save_directory_available = 1;
    bios_option_value = "none";
    CHECK(disc_copy_canonical_path(pbp_path, expected_path, sizeof expected_path));
    memset(&game, 0, sizeof game);
    game.path = pbp_path;
    CHECK_EQ_U64(p_load_game(&game), 1u);
    CHECK_EQ_U64(disk_control.get_num_images(), 1u);
    CHECK_EQ_U64(disk_control.get_image_index(), 0u);
    memset(active_path, 0, sizeof active_path);
    CHECK(disk_control.get_image_path(0u, active_path, sizeof active_path));
    CHECK(strcmp(active_path, expected_path) == 0);
    if (check_failures != 0) goto cleanup;

    for (unsigned i = 0u; i < PBP_TRACE_FRAMES; i++) {
        run_one();
        CHECK(frame_w != 0u && frame_w <= MAX_W && frame_h != 0u && frame_h <= MAX_H);
        CHECK(got_pairs <= MAX_PAIRS);
        CHECK_EQ_U64(video_bad, 0u);
        pbp_trace[i].width = frame_w;
        pbp_trace[i].height = frame_h;
        pbp_trace[i].audio_pairs = got_pairs;
        pbp_trace[i].video_hash = pbp_hash_bytes(UINT64_C(14695981039346656037),
            frame, (size_t)frame_w * frame_h * sizeof frame[0]);
        pbp_trace[i].audio_hash = pbp_hash_bytes(UINT64_C(14695981039346656037), got_audio,
            got_pairs <= MAX_PAIRS ? got_pairs * 2u * sizeof got_audio[0] : 0u);
    }
    CHECK_EQ_U64(video_calls, PBP_TRACE_FRAMES);
    CHECK_EQ_U64(video_bad, 0u);
    system_ram = (unsigned char *)p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    CHECK(system_ram != NULL);
    pbp_ram = system_ram;
    if (system_ram != NULL) {
        saved_ram = (unsigned char *)malloc(2u * 1024u * 1024u);
        CHECK(saved_ram != NULL);
        if (saved_ram != NULL) memcpy(saved_ram, system_ram, 2u * 1024u * 1024u);
    }
    const unsigned char *card =
        (const unsigned char *)p_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    CHECK(card != NULL);
    if (card != NULL) {
        saved_card = (unsigned char *)malloc(SCHWAY_MEMCARD_SIZE);
        CHECK(saved_card != NULL);
        if (saved_card != NULL) memcpy(saved_card, card, SCHWAY_MEMCARD_SIZE);
    }
    card2 = p_test_card2_data(&card2_size);
    CHECK(card2 != NULL);
    CHECK_EQ_U64(card2_size, SCHWAY_MEMCARD_SIZE);
    if (card2 != NULL && card2_size == SCHWAY_MEMCARD_SIZE) {
        saved_card2 = (unsigned char *)malloc(SCHWAY_MEMCARD_SIZE);
        CHECK(saved_card2 != NULL);
        if (saved_card2 != NULL) memcpy(saved_card2, card2, SCHWAY_MEMCARD_SIZE);
    }
    const char *card2_path = p_test_card2_path();
    CHECK(card2_path != NULL && strlen(card2_path) < sizeof saved_card2_path);
    if (card2_path != NULL && strlen(card2_path) < sizeof saved_card2_path) {
        memcpy(saved_card2_path, card2_path, strlen(card2_path) + 1u);
    } else {
        saved_card2_path[0] = '\0';
    }
    CHECK(saved_ram != NULL && saved_card != NULL && saved_card2 != NULL &&
          saved_card2_path[0] != '\0');
    if (check_failures != 0) goto cleanup;

    CHECK(test_pbp_rejection_unchanged(malformed_path, SCHWAY_DISC_IMAGE_BAD_IMAGE,
        saved_ram, saved_card, saved_card2, saved_card2_path,
        expected_path));
    CHECK(test_pbp_rejection_unchanged(unsupported_path, SCHWAY_DISC_IMAGE_UNSUPPORTED,
        saved_ram, saved_card, saved_card2, saved_card2_path,
        expected_path));
    CHECK(p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM) == pbp_ram);
    CHECK_EQ_U64(save_directory_calls, save_calls_before + 1u);
    if (check_failures != 0) goto cleanup;

    p_unload_game();
    CHECK_EQ_U64(p_serialize_size(), 0u);
    memset(&game, 0, sizeof game);
    game.path = cue_path;
    CHECK_EQ_U64(p_load_game(&game), 1u);
    CHECK_EQ_U64(disk_control.get_num_images(), 1u);
    CHECK_EQ_U64(disk_control.get_image_index(), 0u);
    for (unsigned i = 0u; i < PBP_TRACE_FRAMES; i++) {
        run_one();
        cue_trace[i].width = frame_w;
        cue_trace[i].height = frame_h;
        cue_trace[i].audio_pairs = got_pairs;
        cue_trace[i].video_hash = pbp_hash_bytes(UINT64_C(14695981039346656037),
            frame, (size_t)frame_w * frame_h * sizeof frame[0]);
        cue_trace[i].audio_hash = pbp_hash_bytes(UINT64_C(14695981039346656037), got_audio,
            got_pairs <= MAX_PAIRS ? got_pairs * 2u * sizeof got_audio[0] : 0u);
    }
    CHECK_EQ_U64(video_calls, PBP_TRACE_FRAMES * 2u);
    CHECK_EQ_U64(video_bad, 0u);
    for (unsigned i = 0u; i < PBP_TRACE_FRAMES; i++) {
        CHECK_EQ_U64(cue_trace[i].width, pbp_trace[i].width);
        CHECK_EQ_U64(cue_trace[i].height, pbp_trace[i].height);
        CHECK_EQ_U64(cue_trace[i].audio_pairs, pbp_trace[i].audio_pairs);
        CHECK_EQ_U64(cue_trace[i].video_hash, pbp_trace[i].video_hash);
        CHECK_EQ_U64(cue_trace[i].audio_hash, pbp_trace[i].audio_hash);
    }
    system_ram = (unsigned char *)p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    CHECK(system_ram != NULL);
    cue_ram = system_ram != NULL ? (unsigned char *)malloc(2u * 1024u * 1024u) : NULL;
    CHECK(cue_ram != NULL);
    if (system_ram != NULL && cue_ram != NULL) {
        memcpy(cue_ram, system_ram, 2u * 1024u * 1024u);
        CHECK(memcmp(saved_ram, cue_ram, 2u * 1024u * 1024u) == 0);
    }
    card = (const unsigned char *)p_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    CHECK(card != NULL);
    if (card != NULL && saved_card != NULL) {
        CHECK(memcmp(card, saved_card, SCHWAY_MEMCARD_SIZE) == 0);
    }
    card2 = p_test_card2_data(&card2_size);
    CHECK(card2 != NULL);
    CHECK_EQ_U64(card2_size, SCHWAY_MEMCARD_SIZE);
    if (card2 != NULL && saved_card2 != NULL && card2_size == SCHWAY_MEMCARD_SIZE) {
        CHECK(memcmp(card2, saved_card2, SCHWAY_MEMCARD_SIZE) == 0);
    }
    CHECK_EQ_U64(save_directory_calls, save_calls_before + 2u);
    result = check_failures == 0 ? 0 : 1;

cleanup:
    p_unload_game();
    CHECK_EQ_U64(p_serialize_size(), 0u);
    free(saved_card2);
    free(saved_card);
    free(cue_ram);
    free(saved_ram);
    return result != 0 || check_failures != 0 ? 1 : 0;
}

static int legacy_rejection_unchanged(const char *path, const unsigned char *saved_ram,
                                     const void *active_ram, const void *active_card,
                                     const char *active_path)
{
    struct retro_game_info game;
    unsigned char *ram;
    char current_path[4096];
    memset(&game, 0, sizeof game);
    game.path = path;
    CHECK_EQ_U64(p_load_game(&game), 0u);
    ram = (unsigned char *)p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    CHECK(ram != NULL);
    CHECK(ram == active_ram);
    if (ram != NULL && saved_ram != NULL) {
        CHECK(memcmp(ram, saved_ram, 2u * 1024u * 1024u) == 0);
    }
    CHECK(p_get_memory_data(RETRO_MEMORY_SAVE_RAM) == active_card);
    CHECK_EQ_U64(disk_control.get_num_images(), 1u);
    CHECK_EQ_U64(disk_control.get_image_index(), 0u);
    memset(current_path, 0, sizeof current_path);
    CHECK(disk_control.get_image_path(0u, current_path, sizeof current_path));
    CHECK(strcmp(current_path, active_path) == 0);
    return check_failures == 0;
}

static int test_content_legacy(const char *ecm_path, const char *ecm_cue_path,
                               const char *ccd_path, const char *ccd_cue_path,
                               const char *bad_ecm_path, const char *bad_ccd_path,
                               const char *bios_directory, const char *card_directory,
                               int chd_enabled)
{
    enum { LEGACY_TRACE_FRAMES = 1200 };
    struct retro_game_info game;
    struct retro_system_info sys;
    struct pbp_frame_trace provider_trace[LEGACY_TRACE_FRAMES];
    struct pbp_frame_trace cue_trace[LEGACY_TRACE_FRAMES];
    unsigned char *saved_ram = NULL;
    unsigned char *system_ram;
    const void *active_card;
    char ecm_active_path[4096], ccd_active_path[4096], current_path[4096];
    uint32_t ccd_toc, cue_toc;
    unsigned ccd_mailbox_frames, cue_mailbox_frames, control_mailbox_frames;
    int loaded = 0;
    int result = 1;

    CHECK_EQ_U64(chd_enabled, SCHWAYSTATION_CHD);
    CHECK(legacy_images_match(ecm_path, ecm_cue_path, 1));
    CHECK(legacy_images_match(ccd_path, ccd_cue_path, 0));
    ccd_toc = disc_gettd_for_path(ccd_path);
    cue_toc = disc_gettd_for_path(ccd_cue_path);
    CHECK(ccd_toc != 0u);
    CHECK_EQ_U64(ccd_toc, cue_toc);
    CHECK(disc_copy_canonical_path(ecm_path, ecm_active_path, sizeof ecm_active_path));
    CHECK(disc_copy_canonical_path(ccd_path, ccd_active_path, sizeof ccd_active_path));
    CHECK(disk_control_set_calls != 0u);
    CHECK(disk_control.set_eject_state != NULL);
    CHECK(disk_control.get_eject_state != NULL);
    CHECK(disk_control.get_image_index != NULL);
    CHECK(disk_control.set_image_index != NULL);
    CHECK(disk_control.get_num_images != NULL);
    CHECK(disk_control.replace_image_index != NULL);
    CHECK(disk_control.get_image_path != NULL);
    if (check_failures != 0) goto cleanup;

    system_directory = bios_directory;
    save_directory = card_directory;
    save_directory_available = 1;
    bios_option_value = "none";
    saved_ram = (unsigned char *)malloc(2u * 1024u * 1024u);
    CHECK(saved_ram != NULL);
    if (saved_ram == NULL) goto cleanup;

    memset(&game, 0, sizeof game);
    game.path = ecm_path;
    loaded = p_load_game(&game);
    CHECK_EQ_U64(loaded, 1u);
    if (!loaded) goto cleanup;
    CHECK_EQ_U64(disk_control.get_num_images(), 1u);
    CHECK_EQ_U64(disk_control.get_image_index(), 0u);
    memset(current_path, 0, sizeof current_path);
    CHECK(disk_control.get_image_path(0u, current_path, sizeof current_path));
    CHECK(strcmp(current_path, ecm_active_path) == 0);
    p_reset();
    capture_content_trace(provider_trace, LEGACY_TRACE_FRAMES);
    system_ram = (unsigned char *)p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    CHECK(system_ram != NULL);
    if (system_ram != NULL) memcpy(saved_ram, system_ram, 2u * 1024u * 1024u);
    active_card = p_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    CHECK(active_card != NULL);
    CHECK(legacy_rejection_unchanged(bad_ecm_path, saved_ram, system_ram, active_card,
                                     ecm_active_path));
    CHECK(legacy_rejection_unchanged(bad_ccd_path, saved_ram, system_ram, active_card,
                                     ecm_active_path));

    p_unload_game();
    loaded = 0;
    CHECK_EQ_U64(p_serialize_size(), 0u);
    memset(&game, 0, sizeof game);
    game.path = ecm_cue_path;
    loaded = p_load_game(&game);
    CHECK_EQ_U64(loaded, 1u);
    if (!loaded) goto cleanup;
    p_reset();
    capture_content_trace(cue_trace, LEGACY_TRACE_FRAMES);
    check_content_trace_equal(provider_trace, cue_trace, LEGACY_TRACE_FRAMES);

    p_unload_game();
    loaded = 0;
    CHECK_EQ_U64(p_serialize_size(), 0u);
    memset(&game, 0, sizeof game);
    game.path = ccd_path;
    loaded = p_load_game(&game);
    CHECK_EQ_U64(loaded, 1u);
    if (!loaded) goto cleanup;
    CHECK_EQ_U64(disk_control.get_num_images(), 1u);
    CHECK_EQ_U64(disk_control.get_image_index(), 0u);
    memset(current_path, 0, sizeof current_path);
    CHECK(disk_control.get_image_path(0u, current_path, sizeof current_path));
    CHECK(strcmp(current_path, ccd_active_path) == 0);
    p_reset();
    capture_content_trace(provider_trace, LEGACY_TRACE_FRAMES);
    p_reset();
    ccd_mailbox_frames = run_disc_to_mailbox(ccd_toc);
    CHECK(ccd_mailbox_frames != 0u);
    system_ram = (unsigned char *)p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    CHECK(system_ram != NULL);
    if (system_ram != NULL) memcpy(saved_ram, system_ram, 2u * 1024u * 1024u);
    active_card = p_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    CHECK(active_card != NULL);
    CHECK(legacy_rejection_unchanged(bad_ecm_path, saved_ram, system_ram, active_card,
                                     ccd_active_path));
    CHECK(legacy_rejection_unchanged(bad_ccd_path, saved_ram, system_ram, active_card,
                                     ccd_active_path));

    p_unload_game();
    loaded = 0;
    CHECK_EQ_U64(p_serialize_size(), 0u);
    memset(&game, 0, sizeof game);
    game.path = ccd_cue_path;
    loaded = p_load_game(&game);
    CHECK_EQ_U64(loaded, 1u);
    if (!loaded) goto cleanup;
    p_reset();
    capture_content_trace(cue_trace, LEGACY_TRACE_FRAMES);
    check_content_trace_equal(provider_trace, cue_trace, LEGACY_TRACE_FRAMES);
    p_reset();
    cue_mailbox_frames = run_disc_to_mailbox(cue_toc);
    CHECK_EQ_U64(ccd_mailbox_frames, cue_mailbox_frames);

    CHECK(disk_control.set_eject_state(true));
    CHECK(disk_control.get_eject_state());
    game.path = ccd_path;
    CHECK(disk_control.replace_image_index(0u, &game));
    CHECK(disk_control.get_image_path(0u, current_path, sizeof current_path));
    CHECK(strcmp(current_path, ccd_active_path) == 0);
    CHECK(disk_control.get_image_index() == 0u);
    CHECK(disk_control.set_eject_state(false));
    CHECK(!disk_control.get_eject_state());
    p_reset();
    control_mailbox_frames = run_disc_to_mailbox(ccd_toc);
    CHECK_EQ_U64(ccd_mailbox_frames, control_mailbox_frames);
    memset(&sys, 0, sizeof sys);
    p_get_system_info(&sys);
    check_string("ECM/CCD runtime extensions", sys.valid_extensions,
                 expected_valid_extensions());
    result = check_failures == 0 ? 0 : 1;

cleanup:
    if (loaded) p_unload_game();
    CHECK_EQ_U64(p_serialize_size(), 0u);
    free(saved_ram);
    return result != 0 || check_failures != 0 ? 1 : 0;
}

static int test_disc_control(const char *playlist_path, const char *second_path,
                             const char *bios_directory, const char *card_directory)
{
    struct retro_game_info game;
    char expected_first[4096];
    char expected_second[4096];
    char playlist_card1[4096], playlist_card2[4096], member_card2[4096];
    char path[4096];
    char label[256];
    unsigned char card_seed[SCHWAY_MEMCARD_SIZE];
    unsigned char *ram;
    uint32_t expected_first_toc, expected_second_toc;
    unsigned save_calls_before = save_directory_calls;

    CHECK(disk_control_set_calls != 0u);
    CHECK(disk_control.set_eject_state != NULL);
    CHECK(disk_control.get_eject_state != NULL);
    CHECK(disk_control.get_image_index != NULL);
    CHECK(disk_control.set_image_index != NULL);
    CHECK(disk_control.get_num_images != NULL);
    CHECK(disk_control.replace_image_index != NULL);
    CHECK(disk_control.add_image_index != NULL);
    CHECK(disk_control.set_initial_image != NULL);
    CHECK(disk_control.get_image_path != NULL);
    CHECK(disk_control.get_image_label != NULL);
    if (check_failures != 0) {
        return 1;
    }

    CHECK(disc_copy_canonical_path(playlist_path, expected_first, sizeof expected_first));
    CHECK(disc_copy_canonical_path(second_path, expected_second, sizeof expected_second));
    if (check_failures != 0) return 1;
    expected_first_toc = disc_gettd_for_path(playlist_path);
    expected_second_toc = disc_gettd_for_path(second_path);
    CHECK(expected_first_toc != 0u);
    CHECK(expected_second_toc != 0u);
    CHECK(expected_first_toc != expected_second_toc);
    CHECK(test_path(playlist_card1, sizeof playlist_card1, card_directory,
                    "disc_boot_runner.srm"));
    CHECK(test_path(playlist_card2, sizeof playlist_card2, card_directory,
                    "disc_boot_runner.1.mcr"));
    CHECK(test_path(member_card2, sizeof member_card2, card_directory,
                    "disc_boot_playlist.1.mcr"));
    if (check_failures != 0) {
        return 1;
    }
    (void)remove(playlist_card1);
    (void)remove(playlist_card2);
    (void)remove(member_card2);
    memset(card_seed, 0xFF, sizeof card_seed);
    card_seed[128u] = 0x42u;
    CHECK(write_test_file(playlist_card2, card_seed, sizeof card_seed));
    save_directory = card_directory;
    save_directory_available = 1;

    system_directory = bios_directory;
    bios_option_value = "none";
    option_value = "auto";
    CHECK(disk_control.set_initial_image(1u, expected_second));
    CHECK(!disk_control.set_initial_image(SCHWAY_DISC_IMAGE_PLAYLIST_LIMIT, expected_second));
    CHECK(!disk_control.set_initial_image(0u, NULL));
    CHECK_EQ_U64(disk_control.get_num_images(), 0u);
    CHECK_EQ_U64(disk_control.get_image_index(), 0u);
    memset(&game, 0, sizeof game);
    game.path = playlist_path;
    CHECK(p_load_game(&game));
    CHECK_EQ_U64(disk_control.get_num_images(), 2u);
    CHECK_EQ_U64(disk_control.get_image_index(), 1u);
    CHECK_EQ_U64(p_get_region(), RETRO_REGION_PAL);
    CHECK(!disk_control.get_eject_state());
    CHECK_EQ_U64(save_directory_calls, save_calls_before + 1u);
    {
        size_t card2_size = 0u;
        const uint8_t *card2 = p_test_card2_data(&card2_size);
        const char *card2_path = p_test_card2_path();
        CHECK(card2 != NULL);
        CHECK_EQ_U64(card2_size, SCHWAY_MEMCARD_SIZE);
        if (card2 != NULL && card2_size == SCHWAY_MEMCARD_SIZE) {
            CHECK_EQ_U64(card2[128u], 0x42u);
        }
        CHECK(card2_path != NULL);
        if (card2_path != NULL) {
            CHECK(strcmp(card2_path, playlist_card2) == 0);
        }
    }

    memset(path, 0xA5, sizeof path);
    CHECK(disk_control.get_image_path(1u, path, sizeof path));
    CHECK(strcmp(path, expected_second) == 0);
    CHECK(disk_control.get_image_label(1u, label, sizeof label));
    CHECK(label[0] != '\0');
    memset(path, 0xA5, sizeof path);
    CHECK(disk_control.get_image_path(1u, path, 5u));
    CHECK(path[4] == '\0');
    CHECK(strncmp(path, expected_second, 4u) == 0);
    CHECK(!disk_control.get_image_path(2u, path, sizeof path));
    CHECK(!disk_control.get_image_path(1u, NULL, sizeof path));
    CHECK(!disk_control.get_image_path(1u, path, 0u));
    CHECK(!disk_control.get_image_label(2u, label, sizeof label));
    CHECK(!disk_control.get_image_label(1u, NULL, sizeof label));
    CHECK(!disk_control.get_image_label(1u, label, 0u));
    CHECK(!disk_control.set_image_index(0u));
    CHECK_EQ_U64(disk_control.get_image_index(), 1u);
    CHECK(disk_control.set_eject_state(false));
    CHECK(!disk_control.get_eject_state());

    ram = (unsigned char *)p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    CHECK(ram != NULL);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SYSTEM_RAM), 2u * 1024u * 1024u);
    if (ram == NULL) {
        p_unload_game();
        return 1;
    }
    unsigned frame_index = 0u;
    for (; frame_index < 1200u; frame_index++) {
        run_one();
        ram = (unsigned char *)p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
        if (ram != NULL && memcmp(ram + 0x10000u, "SWMB", 4u) == 0 &&
            ram[0x10008u] == 0u) {
            break;
        }
    }
    CHECK(frame_index < 1200u);
    if (frame_index >= 1200u || ram == NULL) {
        p_unload_game();
        return 1;
    }
    CHECK_EQ_HEX(guest_mailbox_record(ram, 2u), UINT32_C(0x00020102));
    CHECK_EQ_HEX(guest_mailbox_record(ram, 4u), expected_second_toc);

    CHECK(disk_control.set_eject_state(true));
    CHECK(disk_control.get_eject_state());
    CHECK(disk_control.set_eject_state(true));
    CHECK_EQ_U64(disk_control.get_image_index(), 1u);
    CHECK(disk_control.add_image_index());
    CHECK_EQ_U64(disk_control.get_num_images(), 3u);
    CHECK(!disk_control.set_image_index(2u));
    CHECK(disk_control.replace_image_index(2u, &(struct retro_game_info){.path = expected_first}));
    CHECK(disk_control.get_image_path(2u, path, sizeof path));
    CHECK(strcmp(path, expected_second) != 0);
    CHECK(!disk_control.replace_image_index(2u,
                                             &(struct retro_game_info){.path = "missing.cue"}));
    CHECK(disk_control.replace_image_index(2u, NULL));
    CHECK_EQ_U64(disk_control.get_num_images(), 2u);
    CHECK(!disk_control.replace_image_index(9u, &(struct retro_game_info){.path = second_path}));
    CHECK(!disk_control.replace_image_index(1u,
                                             &(struct retro_game_info){.path = "missing.cue"}));
    CHECK(disk_control.get_image_path(1u, path, sizeof path));
    CHECK(strcmp(path, expected_second) == 0);
    CHECK(disk_control.replace_image_index(1u, &(struct retro_game_info){.path = expected_first}));
    CHECK(disk_control.get_image_path(1u, path, sizeof path));
    CHECK(strcmp(path, expected_first) == 0);
    CHECK(disk_control.set_image_index(0u));
    CHECK_EQ_U64(disk_control.get_image_index(), 0u);
    CHECK_EQ_U64(save_directory_calls, save_calls_before + 1u);
    {
        size_t card2_size = 0u;
        const uint8_t *card2 = p_test_card2_data(&card2_size);
        const char *card2_path = p_test_card2_path();
        CHECK(card2 != NULL);
        CHECK_EQ_U64(card2_size, SCHWAY_MEMCARD_SIZE);
        if (card2 != NULL && card2_size == SCHWAY_MEMCARD_SIZE) {
            CHECK_EQ_U64(card2[128u], 0x42u);
        }
        CHECK(card2_path != NULL);
        if (card2_path != NULL) {
            CHECK(strcmp(card2_path, playlist_card2) == 0);
        }
    }
    CHECK(disk_control.set_eject_state(false));
    CHECK(!disk_control.get_eject_state());
    CHECK(!disk_control.set_image_index(1u));
    CHECK_EQ_U64(disk_control.get_image_index(), 0u);
    CHECK(!disk_control.replace_image_index(0u, &(struct retro_game_info){.path = second_path}));
    CHECK(disk_control.get_image_path(0u, path, sizeof path));
    CHECK(strcmp(path, expected_second) != 0);

    for (frame_index = 0u; frame_index < 180u; frame_index++) {
        run_one();
        ram = (unsigned char *)p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
        if (ram != NULL && load_le32(ram + 0x70000u) == 5u &&
            (load_le32(ram + 0x70008u) & UINT32_C(0xFFFFFF00)) ==
                (expected_first_toc & UINT32_C(0xFFFFFF00))) {
            break;
        }
    }
    CHECK(frame_index < 180u);
    CHECK(ram != NULL);
    if (ram != NULL) {
        CHECK_EQ_U64(load_le32(ram + 0x70000u), 5u);
        CHECK_EQ_HEX(load_le32(ram + 0x70004u) & UINT32_C(0xFFFFFF00),
                     UINT32_C(0x00020100));
        CHECK_EQ_HEX(load_le32(ram + 0x70008u) & UINT32_C(0xFFFFFF00),
                     expected_first_toc & UINT32_C(0xFFFFFF00));
    }

    unsigned no_media_index = disk_control.get_num_images();
    CHECK(disk_control.set_eject_state(true));
    CHECK(disk_control.set_image_index(no_media_index));
    CHECK_EQ_U64(disk_control.get_image_index(), disk_control.get_num_images());
    CHECK(disk_control.get_eject_state());
    CHECK(disk_control.set_eject_state(false));
    CHECK(!disk_control.get_eject_state());
    for (frame_index = 0u; frame_index < 180u; frame_index++) {
        run_one();
        ram = (unsigned char *)p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
        if (ram != NULL && load_le32(ram + 0x7000Cu) == 5u &&
            ((load_le32(ram + 0x70010u) >> 8) & 0xFFu) == 0x80u) {
            break;
        }
    }
    CHECK(frame_index < 180u);
    CHECK(ram != NULL);
    if (ram != NULL) {
        CHECK_EQ_U64(load_le32(ram + 0x7000Cu), 5u);
        CHECK_EQ_HEX((load_le32(ram + 0x70010u) >> 8) & 0xFFu, 0x80u);
    }

    CHECK(disk_control.set_initial_image(1u, expected_first));
    p_unload_game();
    CHECK_EQ_U64(disk_control.get_num_images(), 0u);
    CHECK(p_load_game(&game));
    CHECK_EQ_U64(disk_control.get_image_index(), 0u);
    CHECK(disk_control.get_eject_state() == false);
    p_unload_game();
    check_card_file_equal(playlist_card2, card_seed);
    {
        FILE *card_one_file = schway__test_open_file(playlist_card1, "rb");
        CHECK(card_one_file == NULL);
        if (card_one_file != NULL) {
            (void)fclose(card_one_file);
        }
    }

    memset(card_seed, 0xFF, sizeof card_seed);
    card_seed[128u] = 0x63u;
    CHECK(write_test_file(member_card2, card_seed, sizeof card_seed));
    game.path = second_path;
    CHECK(p_load_game(&game));
    CHECK_EQ_U64(save_directory_calls, save_calls_before + 3u);
    {
        size_t card2_size = 0u;
        const uint8_t *card2 = p_test_card2_data(&card2_size);
        const char *card2_path = p_test_card2_path();
        CHECK(card2 != NULL);
        CHECK_EQ_U64(card2_size, SCHWAY_MEMCARD_SIZE);
        if (card2 != NULL && card2_size == SCHWAY_MEMCARD_SIZE) {
            CHECK_EQ_U64(card2[128u], 0x63u);
        }
        CHECK(card2_path != NULL);
        if (card2_path != NULL) {
            CHECK(strcmp(card2_path, member_card2) == 0);
        }
    }
    p_unload_game();
    check_card_file_equal(member_card2, card_seed);
    p_deinit();
    CHECK(disk_control_unregister_calls != 0u);
    CHECK(disk_control.set_eject_state == NULL);
    module_close();
    return check_failures != 0 ? 1 : 0;
}

static int test_disc_audio_equality(const char *playlist_path, const char *pcm_path,
                                   const char *bios_directory, uint32_t region)
{
    size_t pcm_size = 0u;
    uint8_t *runner_pcm = read_pcm_file(pcm_path, &pcm_size);
    struct retro_system_av_info av;
    struct retro_game_info game;
    schway *audio_reference = ref_create(region);
    schway_info info;
    uint8_t callback_bytes[4u * MAX_PAIRS];
    uint8_t first_frame[4u * MAX_PAIRS];
    size_t offset = 0u;
    size_t first_frame_bytes = 0u;
    uint64_t steady_pairs = 0u;
    unsigned frames = 0u;
    unsigned steady_frames = 0u;
    unsigned nonzero = 0u;
    unsigned saw_floor = 0u;
    unsigned saw_ceil = 0u;
    unsigned pairs_valid = 1u;
    unsigned stream_valid = 1u;
    const unsigned steady_start_frame = 16u;
    unsigned floor_pairs = region == SCHWAY_REGION_PAL ? 886u : 737u;
    unsigned video_start = video_calls;
    unsigned video_bad_start = video_bad;

    printf("TAP version 13\n");
    audio_expect(runner_pcm != NULL, "runner disc PCM capture is available");
    if (runner_pcm == NULL) {
        schway_destroy(audio_reference);
        p_deinit();
        module_close();
        printf("1..%u\n", audio_tap_checks);
        return 1;
    }
    audio_expect(pcm_size != 0u && (pcm_size % 4u) == 0u,
                 "runner disc PCM contains complete stereo pairs");
    system_directory = bios_directory;
    memset(&game, 0, sizeof game);
    game.path = playlist_path;
    audio_expect(p_load_game(&game), "M3U content loads with the checked system BIOS");
    memset(&av, 0, sizeof av);
    p_get_system_av_info(&av);
    check_av(&av, region);
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    audio_expect(audio_reference != NULL &&
                     schway_get_info(audio_reference, &info) == SCHWAY_STATUS_OK,
                 "selected region exposes native disc audio rates");
    if (audio_reference != NULL && info.fps_den != 0u && info.sample_rate_den != 0u) {
        audio_expect(av.timing.fps == (double)info.fps_num / (double)info.fps_den &&
                         av.timing.sample_rate == (double)info.sample_rate_num /
                                                      (double)info.sample_rate_den,
                     "libretro disc AV timing matches the core's native rates");
    } else {
        audio_expect(0, "core rates are available for disc AV cadence comparison");
    }
    p_set_controller_port_device(0u, RETRO_DEVICE_JOYPAD);

    while (frames < 1200u) {
        run_one();
        frames++;
        if (got_pairs > MAX_PAIRS) {
            pairs_valid = 0u;
            continue;
        }
        /* BIOS video-mode setup can split a sample frame across early public
           calls; exact PCM parity above still covers those startup callbacks. */
        if (frames > steady_start_frame) {
            if (got_pairs != floor_pairs && got_pairs != floor_pairs + 1u) {
                fprintf(stderr, "disc steady audio frame %u delivered %lu pairs\n", frames,
                        (unsigned long)got_pairs);
                pairs_valid = 0u;
            }
            saw_floor |= got_pairs == floor_pairs;
            saw_ceil |= got_pairs == floor_pairs + 1u;
            steady_pairs += got_pairs;
            steady_frames++;
        }
        size_t frame_bytes = got_pairs * 4u;
        if (offset > pcm_size || frame_bytes > pcm_size - offset) {
            fprintf(stderr, "disc audio frame %u exceeds runner stream at byte %lu\n", frames,
                    (unsigned long)offset);
            stream_valid = 0u;
        } else {
            for (size_t pair = 0u; pair < got_pairs; pair++) {
                uint16_t left = (uint16_t)got_audio[2u * pair];
                uint16_t right = (uint16_t)got_audio[2u * pair + 1u];
                callback_bytes[4u * pair] = (uint8_t)left;
                callback_bytes[4u * pair + 1u] = (uint8_t)(left >> 8);
                callback_bytes[4u * pair + 2u] = (uint8_t)right;
                callback_bytes[4u * pair + 3u] = (uint8_t)(right >> 8);
                nonzero |= left != 0u || right != 0u;
            }
            if (frames == 1u && frame_bytes <= sizeof first_frame) {
                memcpy(first_frame, callback_bytes, frame_bytes);
                first_frame_bytes = frame_bytes;
            }
            if (!pcm_bytes_equal(callback_bytes, frame_bytes, runner_pcm + offset,
                                 frame_bytes)) {
                if (stream_valid != 0u) {
                    size_t byte = 0u;
                    while (byte < frame_bytes && callback_bytes[byte] == runner_pcm[offset + byte]) {
                        byte++;
                    }
                    printf("# disc audio frame %u differs at stream byte %lu\n", frames,
                           (unsigned long)(offset + byte));
                }
                stream_valid = 0u;
            }
            offset += frame_bytes;
        }
        unsigned char *ram = (unsigned char *)p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
        if (ram != NULL && memcmp(ram + 0x10000u, "SWMB", 4u) == 0 &&
            ram[0x10008u] == 0u) {
            break;
        }
    }
    audio_expect(frames < 1200u, "disc guest seals its mailbox within the frame bound");
    audio_expect(pairs_valid != 0u && steady_frames != 0u,
                 "steady disc callbacks keep the native region pair count per frame");
    audio_expect(saw_floor != 0u && saw_ceil != 0u,
                 "steady disc audio cadence exercises both sides of the region sample remainder");
    if (audio_reference != NULL && info.fps_num != 0u && info.sample_rate_den != 0u) {
        uint64_t numerator = (uint64_t)info.sample_rate_num * info.fps_den * steady_frames;
        uint64_t denominator = (uint64_t)info.sample_rate_den * info.fps_num;
        uint64_t expected = numerator / denominator;
        audio_expect(steady_pairs + 1u >= expected && steady_pairs <= expected + 1u,
                     "steady disc pair total follows native AV cadence without drift");
    } else {
        audio_expect(0, "core rates are available for total disc cadence comparison");
    }
    audio_expect(video_calls - video_start == frames && video_bad == video_bad_start,
                 "every disc audio frame has one valid video callback");
    audio_expect(stream_valid && offset == pcm_size,
                 "batch or single callback PCM exactly equals the runner stream");
    audio_expect(nonzero != 0u, "the XA and CD-DA fixture produces audible PCM");

    p_reset();
    run_one();
    unsigned reset_nonzero = 0u;
    for (size_t pair = 0u; pair < got_pairs && pair < MAX_PAIRS; pair++) {
        reset_nonzero |= got_audio[2u * pair] != 0 || got_audio[2u * pair + 1u] != 0;
    }
    audio_expect(reset_nonzero == 0u, "power reset clears queued disc PCM");

    p_unload_game();
    audio_expect(p_load_game(&game), "disc content reloads after unload");
    run_one();
    size_t reload_bytes = got_pairs <= MAX_PAIRS ? got_pairs * 4u : sizeof callback_bytes + 1u;
    for (size_t pair = 0u; pair < got_pairs && pair < MAX_PAIRS; pair++) {
        uint16_t left = (uint16_t)got_audio[2u * pair];
        uint16_t right = (uint16_t)got_audio[2u * pair + 1u];
        callback_bytes[4u * pair] = (uint8_t)left;
        callback_bytes[4u * pair + 1u] = (uint8_t)(left >> 8);
        callback_bytes[4u * pair + 2u] = (uint8_t)right;
        callback_bytes[4u * pair + 3u] = (uint8_t)(right >> 8);
    }
    audio_expect(first_frame_bytes != 0u && reload_bytes == first_frame_bytes &&
                     memcmp(callback_bytes, first_frame, first_frame_bytes) == 0,
                 "disc reload delivers its first PCM frame intact");

    p_set_audio_sample(NULL);
    p_set_audio_sample_batch(NULL);
    run_one();
    audio_expect(got_pairs == 0u, "muted frontend receives no disc sample callbacks");
    p_set_audio_sample(sample);
    p_set_audio_sample_batch(single_audio ? NULL : batch);
    run_one();
    audio_expect(got_pairs != 0u && got_pairs <= floor_pairs + 1u,
                 "disc sample callbacks resume after mute");

    p_unload_game();
    p_deinit();
    module_close();
    free(runner_pcm);
    schway_destroy(audio_reference);
    printf("1..%u\n", audio_tap_checks);
    return audio_tap_failures != 0u || check_failures != 0 ? 1 : 0;
}

static void visit_input_state(void *user, const char *name, const void *data, uint32_t count,
                              uint32_t width)
{
    struct input_state_snapshot *snapshot = (struct input_state_snapshot *)user;
    if (strcmp(name, "pad.device") == 0 && count == 8u && width == 1u) {
        memcpy(snapshot->device, data, sizeof snapshot->device);
    } else if (strcmp(name, "pad.present") == 0 && count == 8u && width == 1u) {
        memcpy(snapshot->present, data, sizeof snapshot->present);
    } else if (strcmp(name, "pad.analog_mode") == 0 && count == 8u && width == 1u) {
        memcpy(snapshot->analog_mode, data, sizeof snapshot->analog_mode);
    } else if (strcmp(name, "pad.buttons") == 0 && count == 8u && width == 2u) {
        memcpy(snapshot->buttons, data, sizeof snapshot->buttons);
    } else if (strcmp(name, "pad.axes") == 0 && count == 32u && width == 1u) {
        memcpy(snapshot->axes, data, sizeof snapshot->axes);
    }
}

static void capture_input_state(schway *ref, struct movie_state_reference *state,
                               unsigned frame_index)
{
    size_t reference_size = schway_state_size(ref);
    unsigned checkpoint = INPUT_MOVIE_CHECKPOINTS;
    for (unsigned i = 0u; i < INPUT_MOVIE_CHECKPOINTS; i++) {
        if (input_movie_checkpoints[i] == frame_index) {
            checkpoint = i;
            break;
        }
    }
    CHECK(checkpoint < INPUT_MOVIE_CHECKPOINTS);
    CHECK(reference_size != 0u);
    if (checkpoint >= INPUT_MOVIE_CHECKPOINTS || reference_size == 0u) {
        return;
    }
    state->size = reference_size;
    state->bytes[checkpoint] = (unsigned char *)malloc(reference_size);
    CHECK(state->bytes[checkpoint] != NULL);
    if (state->bytes[checkpoint] == NULL) {
        return;
    }
    CHECK_EQ_U64(schway_save_state(ref, state->bytes[checkpoint], reference_size),
                 SCHWAY_STATUS_OK);
}

static void check_module_input_state(const struct movie_state_reference *state, unsigned layout,
                                     unsigned frame_index)
{
    size_t module_size = p_serialize_size();
    unsigned checkpoint = INPUT_MOVIE_CHECKPOINTS;
    unsigned char *module_state;
    const unsigned char *reference_state;

    for (unsigned i = 0u; i < INPUT_MOVIE_CHECKPOINTS; i++) {
        if (input_movie_checkpoints[i] == frame_index) {
            checkpoint = i;
            break;
        }
    }
    CHECK(checkpoint < INPUT_MOVIE_CHECKPOINTS);
    if (checkpoint >= INPUT_MOVIE_CHECKPOINTS) {
        return;
    }
    reference_state = state->bytes[checkpoint];
    CHECK(reference_state != NULL);
    CHECK_EQ_U64(module_size, state->size);
    if (reference_state == NULL || module_size == 0u || module_size != state->size) {
        return;
    }
    module_state = (unsigned char *)malloc(module_size);
    CHECK(module_state != NULL);
    if (module_state == NULL) {
        return;
    }
    CHECK_EQ_U64(p_serialize(module_state, module_size), true);
    if (memcmp(module_state, reference_state, module_size) != 0) {
        size_t offset = 0u;
        while (offset < module_size && module_state[offset] == reference_state[offset]) {
            offset++;
        }
        fprintf(stderr, "input state layout %u frame %u differs at byte %lu: %02X != %02X\n",
                layout, frame_index, (unsigned long)offset, module_state[offset],
                reference_state[offset]);
        check_failures++;
    }
    free(module_state);
}

static void expected_movie_input(schway_input *input_snapshot, unsigned layout, unsigned variant,
                                 unsigned frame_index)
{
    memset(input_snapshot, 0, sizeof *input_snapshot);
    input_snapshot->size = (uint32_t)sizeof *input_snapshot;
    input_snapshot->device[0] = layout == 0u ? SCHWAY_INPUT_DEVICE_DUALSHOCK
                                             : SCHWAY_INPUT_DEVICE_DIGITAL;
    input_snapshot->device[1] = SCHWAY_INPUT_DEVICE_DUALSHOCK;
    for (unsigned port = 0u; port < 2u; port++) {
        for (unsigned axis = 0u; axis < 4u; axis++) {
            input_snapshot->axes[port][axis] = 128u;
        }
    }
    if ((variant == 1u && frame_index == 60u) ||
        (variant == 2u &&
         (frame_index == 60u || frame_index == 100u || frame_index == 120u)) ||
        (variant == 3u && frame_index >= 60u && frame_index <= 180u)) {
        input_snapshot->buttons[0] = 0x4000u;
    }
    if (frame_index == 1u) {
        uint8_t *axes = input_snapshot->axes[layout == 0u ? 0u : 1u];
        axes[0] = 255u;
        axes[1] = 0u;
        axes[2] = 0u;
        axes[3] = 128u;
        if (layout == 0u) {
            memcpy(input_snapshot->axes[1], axes, 4u);
        }
        if (layout == 0u) {
            input_snapshot->analog_button[0] = 1u;
            input_snapshot->analog_button[1] = 1u;
        }
    } else if (frame_index == 2u) {
        uint8_t *axes = input_snapshot->axes[layout == 0u ? 0u : 1u];
        axes[0] = 0u;
        axes[1] = 128u;
        axes[2] = 128u;
        axes[3] = 255u;
        if (layout == 0u) {
            memcpy(input_snapshot->axes[1], axes, 4u);
        }
    }
}

static void check_movie_pixels(unsigned variant, unsigned frame_index)
{
    unsigned pressed = 0u;
    if (variant == 1u && frame_index >= 60u) {
        pressed = 1u;
    } else if (variant == 2u) {
        pressed = (frame_index >= 60u ? 1u : 0u) +
                  (frame_index >= 100u ? 1u : 0u) +
                  (frame_index >= 120u ? 1u : 0u);
    } else if (variant == 3u && frame_index >= 60u) {
        pressed = 1u;
    }
    CHECK(frame_h > 100u);
    CHECK(frame_w >= 4u);
    if (frame_h > 100u && frame_w >= 4u) {
        for (unsigned x = 0u; x < 4u; x++) {
            uint32_t expected = x < pressed ? 0x00FF0000u : 0u;
            CHECK_EQ_HEX(frame[(size_t)100u * frame_w + x], expected);
        }
    }
}

static void capture_movie_pixels(struct movie_frame_reference *expected)
{
    size_t pixel_count = (size_t)want_w * want_h;
    size_t run_count = pixel_count == 0u ? 0u : 1u;

    expected->width = want_w;
    expected->height = want_h;
    for (size_t i = 1u; i < pixel_count; i++) {
        if (want[i] != want[i - 1u]) {
            run_count++;
        }
    }
    expected->runs = run_count != 0u ?
                         (struct movie_pixel_run *)malloc(run_count * sizeof *expected->runs) :
                         NULL;
    CHECK(run_count == 0u || expected->runs != NULL);
    if (run_count == 0u || expected->runs == NULL) {
        expected->run_count = 0u;
        return;
    }
    expected->run_count = run_count;
    size_t run = 0u;
    expected->runs[run].count = 1u;
    expected->runs[run].pixel = want[0];
    for (size_t i = 1u; i < pixel_count; i++) {
        if (want[i] == expected->runs[run].pixel) {
            expected->runs[run].count++;
        } else {
            run++;
            expected->runs[run].count = 1u;
            expected->runs[run].pixel = want[i];
        }
    }
}

static void check_movie_pixels_equal(const uint32_t *pixels, unsigned width, unsigned height,
                                     const struct movie_frame_reference *expected, unsigned layout,
                                     unsigned frame_index)
{
    size_t pixel_index = 0u;
    size_t pixel_count = (size_t)expected->width * expected->height;
    if (width != expected->width || height != expected->height) {
        fprintf(stderr, "input movie layout %u frame %u: delivered %ux%u, expected %ux%u\n",
                layout, frame_index, width, height, expected->width, expected->height);
        check_failures++;
        return;
    }
    for (size_t run = 0u; run < expected->run_count; run++) {
        for (uint32_t item = 0u; item < expected->runs[run].count; item++, pixel_index++) {
            if (pixel_index >= pixel_count || pixels[pixel_index] != expected->runs[run].pixel) {
                fprintf(stderr,
                        "input movie layout %u frame %u differs at (%lu,%lu): 0x%08lx, "
                        "reference 0x%08lx\n",
                        layout, frame_index, (unsigned long)(pixel_index % expected->width),
                        (unsigned long)(pixel_index / expected->width),
                        (unsigned long)pixels[pixel_index],
                        (unsigned long)expected->runs[run].pixel);
                check_failures++;
                return;
            }
        }
    }
    CHECK_EQ_U64(pixel_index, pixel_count);
}

static void visit_movie_mailbox(void *user, const char *name, const void *data, uint32_t count,
                                uint32_t width)
{
    struct movie_mailbox_reference *mailbox = (struct movie_mailbox_reference *)user;
    if (strcmp(name, "bus.ram") == 0 && width == 1u && count >= 0x10100u) {
        memcpy(mailbox->bytes, (const unsigned char *)data + 0x10000u,
               sizeof mailbox->bytes);
    }
}

static void check_movie_mailbox(const struct movie_mailbox_reference *mailbox, unsigned variant)
{
    static const unsigned expected_presses[INPUT_MOVIE_VARIANTS] = {0u, 1u, 3u, 1u};
    uint32_t expected_pixel = expected_presses[variant] != 0u ? 0x1Fu : 0u;
    CHECK(memcmp(mailbox->bytes, "SWMB", 4u) == 0);
    CHECK_EQ_U64(mailbox->bytes[0x08u], 0u);
    CHECK_EQ_U64(mailbox->bytes[0x09u], 1u);
    CHECK_EQ_U64(mailbox->bytes[0x0Au], 3u);
    CHECK_EQ_U64(mailbox->bytes[0x0Bu], 11u);
    CHECK_EQ_HEX(load_le32(&mailbox->bytes[0x10u]), 0x00010001u);
    CHECK_EQ_U64(load_le32(&mailbox->bytes[0x14u]), expected_presses[variant]);
    CHECK_EQ_HEX(load_le32(&mailbox->bytes[0x18u]), 0x00010002u);
    CHECK_EQ_U64(load_le32(&mailbox->bytes[0x1Cu]), expected_pixel);
    CHECK_EQ_HEX(load_le32(&mailbox->bytes[0x20u]), 0x00010003u);
    CHECK_EQ_U64(load_le32(&mailbox->bytes[0x24u]), expected_pixel);
}

static void free_movie_reference(struct movie_frame_reference *frames)
{
    for (unsigned frame_index = 0u; frame_index < INPUT_MOVIE_FRAMES; frame_index++) {
        free(frames[frame_index].runs);
        free(frames[frame_index].audio);
    }
}

static void free_movie_states(struct movie_state_reference *state)
{
    for (unsigned checkpoint = 0u; checkpoint < INPUT_MOVIE_CHECKPOINTS; checkpoint++) {
        free(state->bytes[checkpoint]);
    }
}

static void capture_movie_reference_frame(schway *reference, unsigned layout, unsigned variant,
                                          unsigned frame_index, unsigned audio_floor,
                                          struct movie_frame_reference *expected)
{
    int16_t reference_audio[MAX_PAIRS * 2u];
    schway_audio audio;
    const char *label = layout == 0u ? "DualShock digital-wire movie reference"
                                     : "digital controller movie reference";
    (void)variant;

    CHECK_EQ_U64(schway_run_frame(reference, NULL), SCHWAY_STATUS_OK);
    memset(&audio, 0, sizeof audio);
    audio.size = (uint32_t)sizeof audio;
    audio.samples = reference_audio;
    audio.capacity = MAX_PAIRS;
    CHECK_EQ_U64(schway_get_audio(reference, &audio), SCHWAY_STATUS_OK);
    if (audio.written != audio_floor && audio.written != (size_t)audio_floor + 1u) {
        fprintf(stderr, "%s frame %u: %lu audio pairs, expected %u or %u\n", label, frame_index,
                (unsigned long)audio.written, audio_floor, audio_floor + 1u);
        check_failures++;
    }
    ref_convert(reference);
    if (layout == 0u) {
        expected->audio_pairs = audio.written;
        if (audio.written > 0u && audio.written <= MAX_PAIRS) {
            expected->audio = (int16_t *)malloc(audio.written * 2u * sizeof reference_audio[0]);
            CHECK(expected->audio != NULL);
            if (expected->audio != NULL) {
                memcpy(expected->audio, reference_audio,
                       audio.written * 2u * sizeof reference_audio[0]);
            }
        } else {
            CHECK(audio.written <= MAX_PAIRS);
        }
        capture_movie_pixels(expected);
    } else {
        check_movie_pixels_equal(want, want_w, want_h, expected, layout, frame_index);
        CHECK_EQ_U64(audio.written, expected->audio_pairs);
        if (audio.written == expected->audio_pairs && audio.written <= MAX_PAIRS &&
            expected->audio != NULL) {
            CHECK(memcmp(reference_audio, expected->audio,
                         audio.written * 2u * sizeof reference_audio[0]) == 0);
        }
    }
}

static void check_movie_reference_input(schway *reference, const schway_input *expected,
                                        unsigned layout, unsigned frame_index)
{
    struct input_state_snapshot snapshot;
    memset(&snapshot, 0, sizeof snapshot);
    CHECK_EQ_U64(schway_state_visit(reference, visit_input_state, &snapshot), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(snapshot.device[0], expected->device[0]);
    CHECK_EQ_U64(snapshot.device[1], expected->device[1]);
    CHECK(memcmp(snapshot.axes, expected->axes, sizeof snapshot.axes) == 0);
    if (layout == 0u && frame_index == 1u) {
        CHECK_EQ_U64(expected->analog_button[0], 1u);
        CHECK_EQ_U64(expected->analog_button[1], 1u);
    }
}

static void run_movie_reference_layout(const unsigned char *image, size_t image_size, unsigned layout,
                                       unsigned variant, unsigned audio_floor,
                                       struct movie_frame_reference *expected_frames,
                                       struct movie_state_reference *states,
                                       struct movie_mailbox_reference *mailbox)
{
    schway *reference = ref_create(SCHWAY_REGION_NTSC_U);
    CHECK(reference != NULL);
    if (reference == NULL) {
        return;
    }
    CHECK_EQ_U64(schway_load_exe(reference, image, image_size), SCHWAY_STATUS_OK);
    for (unsigned frame_index = 1u; frame_index <= INPUT_MOVIE_FRAMES; frame_index++) {
        schway_input input_snapshot;
        expected_movie_input(&input_snapshot, layout, variant, frame_index);
        CHECK_EQ_U64(schway_set_input(reference, &input_snapshot), SCHWAY_STATUS_OK);
        capture_movie_reference_frame(reference, layout, variant, frame_index, audio_floor,
                                      &expected_frames[frame_index - 1u]);
        if (frame_index <= 2u) {
            check_movie_reference_input(reference, &input_snapshot, layout, frame_index);
        }
        for (unsigned checkpoint = 0u; checkpoint < INPUT_MOVIE_CHECKPOINTS; checkpoint++) {
            if (input_movie_checkpoints[checkpoint] == frame_index) {
                capture_input_state(reference, states, frame_index);
                break;
            }
        }
    }
    memset(mailbox, 0, sizeof *mailbox);
    CHECK_EQ_U64(schway_state_visit(reference, visit_movie_mailbox, mailbox), SCHWAY_STATUS_OK);
    check_movie_mailbox(mailbox, variant);
    schway_destroy(reference);
}

static int test_input_movies(const char *path)
{
    struct retro_game_info game;
    schway_info info;
    struct movie_frame_reference expected_frames[INPUT_MOVIE_FRAMES];
    struct movie_state_reference states[2];
    struct movie_mailbox_reference mailboxes[2];
    unsigned char *image = NULL;
    FILE *file = NULL;
    long file_length;
    unsigned audio_floor = 0u;

    file = schway__test_open_file(path, "rb");
    CHECK(file != NULL);
    if (file == NULL) {
        if (file != NULL) {
            (void)fclose(file);
        }
        p_deinit();
        module_close();
        return 1;
    }
    CHECK_EQ_U64(fseek(file, 0L, SEEK_END), 0);
    file_length = ftell(file);
    CHECK(file_length > 0L);
    CHECK_EQ_U64(fseek(file, 0L, SEEK_SET), 0);
    if (file_length <= 0L) {
        (void)fclose(file);
        p_deinit();
        module_close();
        return 1;
    }
    image = (unsigned char *)malloc((size_t)file_length);
    CHECK(image != NULL);
    if (image == NULL) {
        (void)fclose(file);
        p_deinit();
        module_close();
        return 1;
    }
    CHECK_EQ_U64(fread(image, 1u, (size_t)file_length, file), (size_t)file_length);
    CHECK_EQ_U64(fclose(file), 0);
    file = NULL;
    memset(&game, 0, sizeof game);
    game.path = path;
    memset(states, 0, sizeof states);
    input_movie_mode = 1;
    schway *audio_reference = ref_create(SCHWAY_REGION_NTSC_U);
    CHECK(audio_reference != NULL);
    if (audio_reference != NULL) {
        memset(&info, 0, sizeof info);
        info.size = (uint32_t)sizeof info;
        CHECK_EQ_U64(schway_get_info(audio_reference, &info), SCHWAY_STATUS_OK);
        if (info.sample_rate_den != 0u && info.fps_num != 0u) {
            uint64_t numerator = (uint64_t)info.sample_rate_num * info.fps_den;
            uint64_t denominator = (uint64_t)info.sample_rate_den * info.fps_num;
            audio_floor = (unsigned)(numerator / denominator);
        }
        schway_destroy(audio_reference);
    }
    for (unsigned variant = 0u; variant < INPUT_MOVIE_VARIANTS; variant++) {
        memset(expected_frames, 0, sizeof expected_frames);
        memset(states, 0, sizeof states);
        for (unsigned layout = 0u; layout < 2u; layout++) {
            run_movie_reference_layout(image, (size_t)file_length, layout, variant, audio_floor,
                                       expected_frames, &states[layout], &mailboxes[layout]);
        }
        CHECK(memcmp(mailboxes[0].bytes, mailboxes[1].bytes,
                     sizeof mailboxes[0].bytes) == 0);
        for (unsigned layout = 0u; layout < 2u; layout++) {
            analog_mode_value = layout == 0u ? "digital" : "analog";
            input_movie_variant = variant;
            p_set_controller_port_device(0u,
                                         layout == 0u ? RETRO_DEVICE_ANALOG : RETRO_DEVICE_JOYPAD);
            CHECK_EQ_U64(p_load_game(&game), 1u);
            content_polls = 0u;
            input_poll_calls = 0u;
            input_state_calls = 0u;
            for (unsigned frame_index = 1u; frame_index <= INPUT_MOVIE_FRAMES; frame_index++) {
                const struct movie_frame_reference *expected = &expected_frames[frame_index - 1u];
                run_one_movie(expected->width, expected->height);
                check_movie_pixels_equal(frame, frame_w, frame_h,
                                         expected, layout, frame_index);
                CHECK_EQ_U64(got_pairs, expected_frames[frame_index - 1u].audio_pairs);
                if (got_pairs == expected_frames[frame_index - 1u].audio_pairs &&
                    expected_frames[frame_index - 1u].audio != NULL) {
                    CHECK(memcmp(got_audio, expected_frames[frame_index - 1u].audio,
                                 got_pairs * 2u * sizeof got_audio[0]) == 0);
                }
                if (frame_index == 59u || frame_index == 60u || frame_index == 80u ||
                    frame_index == 100u || frame_index == 120u || frame_index == 180u ||
                    frame_index == 300u) {
                    check_movie_pixels(variant, frame_index);
                }
                for (unsigned checkpoint = 0u; checkpoint < INPUT_MOVIE_CHECKPOINTS; checkpoint++) {
                    if (input_movie_checkpoints[checkpoint] == frame_index) {
                        check_module_input_state(&states[layout], layout, frame_index);
                        break;
                    }
                }
            }
            CHECK_EQ_U64(input_poll_calls, INPUT_MOVIE_FRAMES);
            CHECK_EQ_U64(content_polls, INPUT_MOVIE_FRAMES);
            CHECK_EQ_U64(input_state_calls, layout == 0u ? 12000u : 10800u);
            p_unload_game();
        }
        free_movie_reference(expected_frames);
        free_movie_states(&states[0]);
        free_movie_states(&states[1]);
    }
    input_movie_mode = 0;
    input_movie_variant = 0u;
    p_deinit();
    module_close();
    free(image);
    return check_failures != 0 ? 1 : 0;
}

static void check_controls_state(schway *reference, const schway_input *input_snapshot,
                                 const uint8_t analog_mode[2], const char *label,
                                 int compare_serialized)
{
    struct input_state_snapshot snapshot;
    size_t module_size = p_serialize_size();
    size_t reference_size = schway_state_size(reference);
    unsigned char *module_state = NULL;
    unsigned char *reference_state = NULL;

    memset(&snapshot, 0, sizeof snapshot);
    CHECK_EQ_U64(schway_state_visit(reference, visit_input_state, &snapshot), SCHWAY_STATUS_OK);
    for (unsigned port = 0u; port < 2u; port++) {
        CHECK_EQ_U64(snapshot.device[port], input_snapshot->device[port]);
        CHECK_EQ_U64(snapshot.present[port],
                     input_snapshot->device[port] == SCHWAY_INPUT_DEVICE_NONE ? 0u : 1u);
        CHECK_EQ_U64(snapshot.analog_mode[port], analog_mode[port]);
        CHECK_EQ_U64(snapshot.buttons[port], (uint16_t)~input_snapshot->buttons[port]);
        CHECK(memcmp(&snapshot.axes[port * 4u], input_snapshot->axes[port], 4u) == 0);
    }
    if (compare_serialized == 0) {
        return;
    }
    CHECK_EQ_U64(module_size, reference_size);
    if (module_size == 0u || module_size != reference_size) {
        return;
    }
    module_state = (unsigned char *)malloc(module_size);
    reference_state = (unsigned char *)malloc(reference_size);
    CHECK(module_state != NULL);
    CHECK(reference_state != NULL);
    if (module_state == NULL || reference_state == NULL) {
        free(module_state);
        free(reference_state);
        return;
    }
    CHECK_EQ_U64(p_serialize(module_state, module_size), true);
    CHECK_EQ_U64(schway_save_state(reference, reference_state, reference_size), SCHWAY_STATUS_OK);
    if (memcmp(module_state, reference_state, module_size) != 0) {
        size_t offset = 0u;
        while (offset < module_size && module_state[offset] == reference_state[offset]) {
            offset++;
        }
        fprintf(stderr, "%s: serialized input state differs at byte %lu\n", label,
                (unsigned long)offset);
        check_failures++;
    }
    free(module_state);
    free(reference_state);
}

static void controls_input(schway_input *input_snapshot, uint8_t device0, uint8_t device1)
{
    memset(input_snapshot, 0, sizeof *input_snapshot);
    input_snapshot->size = (uint32_t)sizeof *input_snapshot;
    input_snapshot->device[0] = device0;
    input_snapshot->device[1] = device1;
    for (unsigned port = 0u; port < 2u; port++) {
        for (unsigned axis = 0u; axis < 4u; axis++) {
            input_snapshot->axes[port][axis] = 128u;
        }
    }
}

static void controls_step(schway *reference, schway_input *input_snapshot,
                          const uint8_t analog_mode[2], const char *label, unsigned audio_floor,
                          int compare_serialized)
{
    CHECK_EQ_U64(schway_set_input(reference, input_snapshot), SCHWAY_STATUS_OK);
    step(reference, label, audio_floor);
    check_controls_state(reference, input_snapshot, analog_mode, label, compare_serialized);
}

static schway *controls_reference(const unsigned char *image, size_t image_size)
{
    schway *reference = ref_create(SCHWAY_REGION_NTSC_U);
    CHECK(reference != NULL);
    if (reference != NULL) {
        CHECK_EQ_U64(schway_load_exe(reference, image, image_size), SCHWAY_STATUS_OK);
    }
    return reference;
}

static int test_input_controls(const char *path)
{
    static const uint8_t analog[2] = {1u, 1u};
    static const uint8_t digital[2] = {0u, 0u};
    static const uint8_t digital_analog[2] = {0u, 1u};
    static const uint8_t digital_none[2] = {0u, 0u};
    static const uint8_t none_analog[2] = {0u, 1u};
    static const uint8_t analog_digital[2] = {1u, 0u};
    const uint16_t chord = 0x0F09u;
    struct retro_game_info game;
    schway_input input_snapshot;
    schway_info info;
    schway *reference;
    unsigned char *image;
    FILE *file = schway__test_open_file(path, "rb");
    long file_length;
    unsigned audio_floor = 0u;

    CHECK(file != NULL);
    if (file == NULL) {
        p_deinit();
        module_close();
        return 1;
    }
    CHECK_EQ_U64(fseek(file, 0L, SEEK_END), 0);
    file_length = ftell(file);
    CHECK(file_length > 0L);
    CHECK_EQ_U64(fseek(file, 0L, SEEK_SET), 0);
    if (file_length <= 0L) {
        (void)fclose(file);
        p_deinit();
        module_close();
        return 1;
    }
    image = (unsigned char *)malloc((size_t)file_length);
    CHECK(image != NULL);
    if (image == NULL) {
        (void)fclose(file);
        p_deinit();
        module_close();
        return 1;
    }
    CHECK_EQ_U64(fread(image, 1u, (size_t)file_length, file), (size_t)file_length);
    CHECK_EQ_U64(fclose(file), 0);
    memset(&game, 0, sizeof game);
    game.path = path;
    input_controls_active = 1;
    input_controls_chord_enabled = 0;
    input_controls_chord_until = 0u;
    analog_mode_value = "analog";
    analog_toggle_value = "enabled";
    p_set_input_state(input);

    reference = controls_reference(image, (size_t)file_length);
    CHECK(reference != NULL);
    if (reference != NULL) {
        memset(&info, 0, sizeof info);
        info.size = (uint32_t)sizeof info;
        CHECK_EQ_U64(schway_get_info(reference, &info), SCHWAY_STATUS_OK);
        if (info.sample_rate_den != 0u && info.fps_num != 0u) {
            audio_floor = (unsigned)(((uint64_t)info.sample_rate_num * info.fps_den) /
                                     ((uint64_t)info.sample_rate_den * info.fps_num));
        }
        schway_destroy(reference);
    }

    /* No controller selection means the documented DualShock/analog defaults on both ports. */
    CHECK_EQ_U64(p_load_game(&game), 1u);
    reference = controls_reference(image, (size_t)file_length);
    CHECK(reference != NULL);
    if (reference != NULL) {
        controls_input(&input_snapshot, SCHWAY_INPUT_DEVICE_DUALSHOCK,
                       SCHWAY_INPUT_DEVICE_DUALSHOCK);
        controls_step(reference, &input_snapshot, analog, "default DualShock analog mode",
                      audio_floor, 1);
        schway_destroy(reference);
    }
    p_unload_game();

    /* Without the optional callback, buttons release and both sticks stay centered. */
    CHECK_EQ_U64(p_load_game(&game), 1u);
    reference = controls_reference(image, (size_t)file_length);
    CHECK(reference != NULL);
    p_set_input_state(NULL);
    if (reference != NULL) {
        controls_input(&input_snapshot, SCHWAY_INPUT_DEVICE_DUALSHOCK,
                       SCHWAY_INPUT_DEVICE_DUALSHOCK);
        controls_step(reference, &input_snapshot, analog, "absent input-state callback",
                      audio_floor, 1);
        schway_destroy(reference);
    }
    p_set_input_state(input);
    p_unload_game();

    /* The configured digital option is sent as a one-frame mode pulse to each DualShock. */
    analog_mode_value = "digital";
    CHECK_EQ_U64(p_load_game(&game), 1u);
    reference = controls_reference(image, (size_t)file_length);
    CHECK(reference != NULL);
    if (reference != NULL) {
        controls_input(&input_snapshot, SCHWAY_INPUT_DEVICE_DUALSHOCK,
                       SCHWAY_INPUT_DEVICE_DUALSHOCK);
        input_snapshot.analog_button[0] = 1u;
        input_snapshot.analog_button[1] = 1u;
        controls_step(reference, &input_snapshot, digital, "configured digital mode",
                      audio_floor, 1);
        input_snapshot.analog_button[0] = 0u;
        input_snapshot.analog_button[1] = 0u;
        controls_step(reference, &input_snapshot, digital, "digital mode stays selected",
                      audio_floor, 1);
        schway_destroy(reference);
    }
    p_unload_game();

    /* Per-port switches work; invalid device and port requests keep the prior selection. */
    analog_mode_value = "analog";
    p_set_controller_port_device(0u, RETRO_DEVICE_ANALOG);
    p_set_controller_port_device(1u, RETRO_DEVICE_ANALOG);
    CHECK_EQ_U64(p_load_game(&game), 1u);
    reference = controls_reference(image, (size_t)file_length);
    CHECK(reference != NULL);
    if (reference != NULL) {
        p_set_controller_port_device(0u, RETRO_DEVICE_JOYPAD);
        p_set_controller_port_device(1u, RETRO_DEVICE_NONE);
        controls_input(&input_snapshot, SCHWAY_INPUT_DEVICE_DIGITAL, SCHWAY_INPUT_DEVICE_NONE);
        controls_step(reference, &input_snapshot, digital_none,
                      "independent digital and none ports", audio_floor, 1);
        p_set_controller_port_device(0u, RETRO_DEVICE_LIGHTGUN);
        p_set_controller_port_device(2u, RETRO_DEVICE_ANALOG);
        controls_step(reference, &input_snapshot, digital_none,
                      "invalid requests preserve selected devices", audio_floor, 1);
        p_set_controller_port_device(0u, RETRO_DEVICE_NONE);
        p_set_controller_port_device(1u, RETRO_DEVICE_ANALOG);
        controls_input(&input_snapshot, SCHWAY_INPUT_DEVICE_NONE,
                       SCHWAY_INPUT_DEVICE_DUALSHOCK);
        controls_step(reference, &input_snapshot, none_analog, "switch none and DualShock",
                      audio_floor, 1);
        p_set_controller_port_device(0u, RETRO_DEVICE_ANALOG);
        p_set_controller_port_device(1u, RETRO_DEVICE_JOYPAD);
        controls_input(&input_snapshot, SCHWAY_INPUT_DEVICE_DUALSHOCK,
                       SCHWAY_INPUT_DEVICE_DIGITAL);
        controls_step(reference, &input_snapshot, analog_digital, "switch DualShock and digital",
                      audio_floor, 1);
        schway_destroy(reference);
    }
    p_unload_game();

    /* The six-button chord changes mode at frame 60 exactly once while held. */
    p_set_controller_port_device(0u, RETRO_DEVICE_ANALOG);
    p_set_controller_port_device(1u, RETRO_DEVICE_ANALOG);
    CHECK_EQ_U64(p_load_game(&game), 1u);
    reference = controls_reference(image, (size_t)file_length);
    CHECK(reference != NULL);
    input_controls_chord_enabled = 1;
    input_controls_chord_until = 60u;
    content_polls = 0u;
    if (reference != NULL) {
        for (unsigned frame_index = 1u; frame_index <= 61u; frame_index++) {
            controls_input(&input_snapshot, SCHWAY_INPUT_DEVICE_DUALSHOCK,
                           SCHWAY_INPUT_DEVICE_DUALSHOCK);
            if (frame_index <= 60u) {
                input_snapshot.buttons[0] = chord;
            }
            input_snapshot.analog_button[0] = frame_index == 60u ? 1u : 0u;
            controls_step(reference, &input_snapshot,
                          frame_index < 60u ? analog : digital_analog,
                          "enabled 60-frame mode toggle", audio_floor,
                          frame_index == 59u || frame_index == 60u || frame_index == 61u);
        }
        schway_destroy(reference);
    }
    p_unload_game();

    /* Disabling the option prevents that chord from changing the analog mode. */
    analog_toggle_value = "disabled";
    p_set_controller_port_device(0u, RETRO_DEVICE_ANALOG);
    p_set_controller_port_device(1u, RETRO_DEVICE_ANALOG);
    CHECK_EQ_U64(p_load_game(&game), 1u);
    reference = controls_reference(image, (size_t)file_length);
    CHECK(reference != NULL);
    content_polls = 0u;
    if (reference != NULL) {
        for (unsigned frame_index = 1u; frame_index <= 61u; frame_index++) {
            controls_input(&input_snapshot, SCHWAY_INPUT_DEVICE_DUALSHOCK,
                           SCHWAY_INPUT_DEVICE_DUALSHOCK);
            if (frame_index <= 60u) {
                input_snapshot.buttons[0] = chord;
            }
            controls_step(reference, &input_snapshot, analog, "disabled mode toggle",
                          audio_floor,
                          frame_index == 59u || frame_index == 60u || frame_index == 61u);
        }
        schway_destroy(reference);
    }
    input_controls_chord_enabled = 0;
    input_controls_active = 0;
    p_unload_game();
    p_deinit();

    /* Reinitializing the module restores the default controller and mode state. */
    analog_mode_value = "analog";
    analog_toggle_value = "enabled";
    p_set_environment(env);
    p_init();
    set_callbacks();
    CHECK_EQ_U64(p_load_game(&game), 1u);
    reference = controls_reference(image, (size_t)file_length);
    CHECK(reference != NULL);
    if (reference != NULL) {
        controls_input(&input_snapshot, SCHWAY_INPUT_DEVICE_DUALSHOCK,
                       SCHWAY_INPUT_DEVICE_DUALSHOCK);
        controls_step(reference, &input_snapshot, analog, "deinit restores input defaults",
                      audio_floor, 1);
        schway_destroy(reference);
    }
    p_unload_game();
    p_deinit();
    module_close();
    free(image);
    return check_failures != 0 ? 1 : 0;
}

static void append_sh(uint32_t *code, size_t *count, unsigned reg, unsigned base, unsigned offset)
{
    code[(*count)++] = 0xA4000000u | (base << 21) | (reg << 16) | offset;
}

static void append_sb(uint32_t *code, size_t *count, unsigned reg, unsigned base, unsigned offset)
{
    code[(*count)++] = 0xA0000000u | (base << 21) | (reg << 16) | offset;
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
    append_li(code, &count, 17u, 0x80020000u); /* s1: guest-visible SIO response scratch */
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
        const uint8_t motor_bytes[6] = {small, large, 0u, 0u, 0u, 0u};
        append_li(code, &count, 8u, 0x0003u | (port != 0u ? 0x2000u : 0u));
        append_sh(code, &count, 8u, 16u, 0x04Au);
        append_sio_tx(code, &count, jal_sites, &site_count, 0x01u, 1u);
        append_sio_tx(code, &count, jal_sites, &site_count, 0x42u, 1u);
        append_sio_tx(code, &count, jal_sites, &site_count, 0u, 1u);
        for (unsigned index = 0u; index < 6u; index++) {
            append_sio_tx(code, &count, jal_sites, &site_count, motor_bytes[index],
                          index != 5u ? 1u : 0u);
            append_sb(code, &count, 2u, 17u, port * 6u + index);
        }
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

struct rumble_reference_snapshot {
    uint32_t device[2];
    uint8_t poll_response[12];
};

static void visit_rumble_reference(void *user, const char *name, const void *data, uint32_t count,
                                   uint32_t width)
{
    struct rumble_reference_snapshot *found = (struct rumble_reference_snapshot *)user;
    if (strcmp(name, "pad.device") == 0 && count == 8u && width == 1u) {
        found->device[0] = ((const uint8_t *)data)[0];
        found->device[1] = ((const uint8_t *)data)[1];
    } else if (strcmp(name, "bus.ram") == 0 && width == 1u && count >= 0x2000Cu) {
        memcpy(found->poll_response, (const uint8_t *)data + 0x20000u,
               sizeof found->poll_response);
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
    struct rumble_reference_snapshot reference_snapshot;

    CHECK(reference != NULL);
    CHECK(write_rumble_guest(path, 1u, 255u));
    file = schway__test_open_file(path, "rb");
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
        input_snapshot.axes[port][0] = 255u;
        input_snapshot.axes[port][1] = 128u;
        input_snapshot.axes[port][2] = 0u;
        input_snapshot.axes[port][3] = 128u;
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
    memset(&reference_snapshot, 0, sizeof reference_snapshot);
    CHECK_EQ_U64(schway_state_visit(reference, visit_rumble_reference, &reference_snapshot),
                 SCHWAY_STATUS_OK);
    CHECK_EQ_U64(reference_snapshot.device[0], SCHWAY_INPUT_DEVICE_DUALSHOCK);
    CHECK_EQ_U64(reference_snapshot.device[1], SCHWAY_INPUT_DEVICE_DUALSHOCK);
    for (unsigned port = 0u; port < 2u; port++) {
        size_t offset = port * 6u;
        CHECK_EQ_HEX(reference_snapshot.poll_response[offset], 0xFFu);
        CHECK_EQ_HEX(reference_snapshot.poll_response[offset + 1u], 0xFFu);
        CHECK_EQ_HEX(reference_snapshot.poll_response[offset + 2u], 0xFFu);
        CHECK_EQ_HEX(reference_snapshot.poll_response[offset + 3u], 128u);
        CHECK_EQ_HEX(reference_snapshot.poll_response[offset + 4u], 0u);
        CHECK_EQ_HEX(reference_snapshot.poll_response[offset + 5u], 128u);
    }
    schway_destroy(reference);

    memset(&game, 0, sizeof game);
    game.path = path;
    rumble_axis_mode = 1;
    CHECK_EQ_U64(p_load_game(&game), 1u);
    p_set_controller_port_device(0u, RETRO_DEVICE_ANALOG);
    p_set_controller_port_device(1u, RETRO_DEVICE_ANALOG);
    rumble_event_count = 0u;
    for (unsigned frame_index = 0u; frame_index < 4u; frame_index++) {
        size_t before = rumble_event_count;
        run_one();
        check_rumble_events(before, (uint16_t)(255u * 257u), UINT16_MAX);
    }
    CHECK_EQ_U64(rumble_event_count, 16u);
    p_unload_game();
    rumble_axis_mode = 0;
    p_deinit();
    module_close();
    (void)remove(path);
    return check_failures != 0 ? 1 : 0;
}

static int test_rumble_controls(const char *path)
{
    struct retro_game_info game;
    unsigned video_before;
    size_t offset;

    CHECK(write_rumble_guest(path, 1u, 255u));
    memset(&game, 0, sizeof game);
    game.path = path;
    rumble_axis_mode = 1;
    CHECK_EQ_U64(p_load_game(&game), 1u);
    p_set_controller_port_device(0u, RETRO_DEVICE_ANALOG);
    p_set_controller_port_device(1u, RETRO_DEVICE_ANALOG);
    rumble_event_count = 0u;
    for (unsigned frame_index = 0u; frame_index < 4u; frame_index++) {
        run_one();
        check_rumble_events((size_t)frame_index * 4u, (uint16_t)(255u * 257u), UINT16_MAX);
    }
    CHECK_EQ_U64(rumble_event_count, 16u);

    /* Reset clears both reported motors on both ports before the guest restarts. */
    offset = rumble_event_count;
    p_reset();
    CHECK_EQ_U64(rumble_event_count, offset + 4u);
    check_rumble_events(offset, 0u, 0u);
    offset = rumble_event_count;
    p_unload_game();
    CHECK_EQ_U64(rumble_event_count, offset + 4u);
    check_rumble_events(offset, 0u, 0u);
    CHECK_EQ_U64(p_load_game(&game), 1u);
    p_set_controller_port_device(0u, RETRO_DEVICE_ANALOG);
    p_set_controller_port_device(1u, RETRO_DEVICE_ANALOG);
    offset = rumble_event_count;
    for (unsigned frame_index = 0u; frame_index < 4u; frame_index++) {
        run_one();
        check_rumble_events(offset + (size_t)frame_index * 4u, (uint16_t)(255u * 257u),
                            UINT16_MAX);
    }
    offset = rumble_event_count;
    p_unload_game();
    CHECK_EQ_U64(rumble_event_count, offset + 4u);
    check_rumble_events(offset, 0u, 0u);

    /* A real mapped transaction with both motors off reports zero repeatedly. */
    CHECK(write_rumble_guest(path, 0u, 0u));
    CHECK_EQ_U64(p_load_game(&game), 1u);
    p_set_controller_port_device(0u, RETRO_DEVICE_ANALOG);
    p_set_controller_port_device(1u, RETRO_DEVICE_ANALOG);
    offset = rumble_event_count;
    for (unsigned frame_index = 0u; frame_index < 4u; frame_index++) {
        run_one();
        check_rumble_events(offset + (size_t)frame_index * 4u, 0u, 0u);
    }
    CHECK_EQ_U64(rumble_event_count, offset + 16u);
    offset = rumble_event_count;
    p_unload_game();
    CHECK_EQ_U64(rumble_event_count, offset + 4u);
    check_rumble_events(offset, 0u, 0u);

    /* With no frontend interface, guest input, video and frames continue safely. */
    p_deinit();
    rumble_available = 0;
    p_set_environment(env);
    p_init();
    set_callbacks();
    CHECK_EQ_U64(p_load_game(&game), 1u);
    p_set_controller_port_device(0u, RETRO_DEVICE_ANALOG);
    p_set_controller_port_device(1u, RETRO_DEVICE_ANALOG);
    offset = rumble_event_count;
    video_before = video_calls;
    for (unsigned frame_index = 0u; frame_index < 4u; frame_index++) {
        run_one();
    }
    CHECK_EQ_U64(video_calls, video_before + 4u);
    CHECK_EQ_U64(rumble_event_count, offset);
    p_unload_game();
    CHECK_EQ_U64(rumble_event_count, offset);
    rumble_axis_mode = 0;
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
    int audio_batch_mode = argc == 6 && strcmp(argv[2], "audio-equality-batch") == 0;
    int audio_single_mode = argc == 6 && strcmp(argv[2], "audio-equality-single") == 0;
    int audio_mode = audio_batch_mode || audio_single_mode;
    int disc_audio_batch_mode = argc == 7 && strcmp(argv[2], "disc-audio-equality-batch") == 0;
    int disc_audio_single_mode = argc == 7 && strcmp(argv[2], "disc-audio-equality-single") == 0;
    int disc_audio_mode = disc_audio_batch_mode || disc_audio_single_mode;
    uint32_t selected_audio_region = audio_mode || disc_audio_mode
                                         ? audio_region(argv[3])
                                         : SCHWAY_REGION_NTSC_U;
    int content_mode = argc == 4 && strcmp(argv[2], "content-no-bios") == 0;
    int disc_control_mode = argc == 7 && strcmp(argv[2], "disc-control") == 0;
    int content_chd_mode = argc == 9 && strcmp(argv[2], "content-chd") == 0;
    int content_pbp_mode = argc == 9 && strcmp(argv[2], "content-pbp") == 0;
    int content_legacy_mode = argc == 12 && strcmp(argv[2], "content-legacy") == 0;
    int content_profile_mode = argc == 6 && strcmp(argv[2], "content-profile") == 0;
    int content_boot_mode = argc == 8 && strcmp(argv[2], "content-boot") == 0;
    int options_profile_mode = argc == 3 && strcmp(argv[2], "options-profile") == 0;
    int psexe_mode = argc == 5 && strcmp(argv[2], "content-psexe") == 0;
    int input_descriptors_mode = argc == 3 && strcmp(argv[2], "input-descriptors") == 0;
    int input_multitap_mode = argc == 3 && strcmp(argv[2], "input-multitap") == 0;
    int input_special_mode = argc == 3 && strcmp(argv[2], "input-special") == 0;
    int input_lightgun_mode = argc == 3 && strcmp(argv[2], "input-lightgun") == 0;
    int video_geometry_mode = argc == 4 && strcmp(argv[2], "video-geometry") == 0;
    int video_edges_mode = argc == 4 && strcmp(argv[2], "video-geometry-edges") == 0;
    int input_movies_mode = argc == 4 && strcmp(argv[2], "input-movies") == 0;
    int input_controls_mode = argc == 4 && strcmp(argv[2], "input-controls") == 0;
    int lifecycle_mode = argc == 5 && strcmp(argv[2], "content-lifecycle") == 0;
    int card_persistence_mode = argc == 5 && strcmp(argv[2], "card-persistence") == 0;
    int card_controls_mode = argc == 5 && strcmp(argv[2], "card-controls") == 0;
    int cards_multitap_mode = argc == 4 && strcmp(argv[2], "cards-multitap") == 0;
    int memory_aliases_mode = argc == 3 && strcmp(argv[2], "memory-aliases") == 0;
    int memory_boundaries_mode = argc == 3 && strcmp(argv[2], "memory-boundaries") == 0;
    int rumble_guest_mode = argc == 4 && strcmp(argv[2], "rumble-guest") == 0;
    int rumble_controls_mode = argc == 4 && strcmp(argv[2], "rumble-controls") == 0;

    if ((!audio_mode && !disc_audio_mode && !content_mode && !disc_control_mode && !content_chd_mode && !content_pbp_mode && !content_legacy_mode && !psexe_mode && !content_profile_mode && !content_boot_mode && !options_profile_mode && !input_descriptors_mode && !input_multitap_mode &&
         !input_special_mode && !input_lightgun_mode && !video_geometry_mode &&
         !video_edges_mode && !input_movies_mode &&
         !input_controls_mode && !lifecycle_mode && !card_persistence_mode &&
         !card_controls_mode && !cards_multitap_mode && !memory_aliases_mode && !memory_boundaries_mode &&
         !rumble_guest_mode && !rumble_controls_mode &&
         (argc != 3 || (strcmp(argv[2], "ntsc-u") != 0 && strcmp(argv[2], "pal") != 0))) ||
        (audio_mode && (!valid_audio_region(argv[3]) || argv[4][0] == '\0' ||
                        argv[5][0] == '\0')) ||
        (disc_audio_mode && (!valid_audio_region(argv[3]) || argv[4][0] == '\0' ||
                             argv[5][0] == '\0' || argv[6][0] == '\0')) ||
        ((content_mode || disc_control_mode || content_chd_mode || content_pbp_mode || content_legacy_mode || psexe_mode || content_profile_mode || content_boot_mode || video_geometry_mode || video_edges_mode || lifecycle_mode ||
          input_movies_mode || input_controls_mode || rumble_guest_mode || rumble_controls_mode ||
          card_persistence_mode || card_controls_mode || cards_multitap_mode) &&
         argv[3][0] == '\0') ||
        (content_chd_mode && (argv[4][0] == '\0' || argv[5][0] == '\0' ||
                              argv[6][0] == '\0' || argv[7][0] == '\0' ||
                              argv[8][0] == '\0')) ||
        (content_pbp_mode && (argv[3][0] == '\0' || argv[4][0] == '\0' ||
                              argv[5][0] == '\0' || argv[6][0] == '\0' ||
                              argv[7][0] == '\0' || argv[8][0] == '\0')) ||
        (content_legacy_mode &&
         (argv[3][0] == '\0' || argv[4][0] == '\0' || argv[5][0] == '\0' ||
          argv[6][0] == '\0' || argv[7][0] == '\0' || argv[8][0] == '\0' ||
          argv[9][0] == '\0' || argv[10][0] == '\0' || argv[11][0] == '\0')) ||
        (disc_control_mode &&
         (argv[4][0] == '\0' || argv[5][0] == '\0' || argv[6][0] == '\0')) ||
        (content_profile_mode &&
         (argv[4][0] == '\0' || argv[5][0] == '\0')) ||
        (content_boot_mode &&
         (argv[3][0] == '\0' || argv[4][0] == '\0' || argv[5][0] == '\0' ||
          argv[6][0] == '\0' || argv[7][0] == '\0')) ||
        ((psexe_mode || lifecycle_mode || card_persistence_mode || card_controls_mode) &&
         argv[4][0] == '\0')) {
        fprintf(stderr,
                "usage: schwaystation-test-libretro-host <module> ntsc-u|pal | content-no-bios <exe> | "
                "disc-control <m3u> <second-cue> <bios-directory> <save-directory> | "
                "content-chd <enabled:0|1> <chd> <cue> <invalid-chd> <bios-directory> <save-directory> | "
                "content-pbp <chd-enabled:0|1> <pbp> <cue> <invalid-pbp> <bios-directory> <save-directory> | "
                "content-legacy <chd-enabled:0|1> <ecm> <ecm-cue> <ccd> <ccd-cue> "
                "<invalid-ecm> <invalid-ccd> <bios-directory> <save-directory> | "
                "content-profile <m3u> <bios-directory> <save-directory> | "
                "content-boot <m3u> <selected-m3u> <selected-iso> "
                "<bios-directory> <test-directory> | options-profile | "
                "disc-audio-equality-batch|disc-audio-equality-single <region> <m3u> <pcm> <bios-directory> | "
                "content-psexe <exe> <directory> | audio-equality-batch|audio-equality-single <region> <exe> <pcm> | input-descriptors | "
                "content-lifecycle <exe> <system-dir> | card-persistence <exe> <save-dir> | "
                "card-controls <exe> <save-dir> | cards-multitap <save-dir> | memory-aliases | memory-boundaries | "
                "video-geometry <exe> | video-geometry-edges <exe> | input-movies <exe> | "
                "input-controls <exe> | rumble-guest <exe> | rumble-controls <exe>\n");
        return 2;
    }
    content_test = audio_mode || disc_audio_mode || content_mode || disc_control_mode || content_chd_mode || content_pbp_mode || content_legacy_mode || psexe_mode || content_profile_mode || content_boot_mode || video_geometry_mode || video_edges_mode || input_movies_mode ||
                   input_controls_mode || lifecycle_mode || card_persistence_mode ||
                   card_controls_mode;
    rumble_available = rumble_guest_mode || rumble_controls_mode;
    pal = audio_mode || disc_audio_mode
              ? selected_audio_region == SCHWAY_REGION_PAL
              : (!content_test && strcmp(argv[2], "pal") == 0);
    single_audio = audio_mode || disc_audio_mode
                       ? audio_single_mode || disc_audio_single_mode
                       : pal;
    option_value = audio_mode || disc_audio_mode ? argv[3] : (pal ? "pal" : "ntsc-u");
    region = audio_mode || disc_audio_mode ? selected_audio_region
                                            : (pal ? SCHWAY_REGION_PAL : SCHWAY_REGION_NTSC_U);
    /* The region selected for the next load, and both regions' pairs per
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
    int inspect_card2 = disc_control_mode || content_chd_mode || content_pbp_mode ||
                        card_persistence_mode || input_multitap_mode || input_special_mode ||
                        input_lightgun_mode || cards_multitap_mode;
    resolve_all(inspect_card2);
    if (missing != NULL) {
        fprintf(stderr, "missing entry point %s\n", missing);
        module_close();
        return 1;
    }
    if (!inspect_card2 &&
        (module_sym("schway__libretro_test_card2_data") != NULL ||
         module_sym("schway__libretro_test_card2_path") != NULL ||
         module_sym("schway__libretro_test_card_socket") != NULL ||
         module_sym("schway__libretro_test_write_card_socket") != NULL ||
         module_sym("schway__libretro_test_flush_cards") != NULL ||
         module_sym("schway__libretro_test_poll_endpoint") != NULL ||
         module_sym("schway__libretro_test_poll_bytes") != NULL ||
         module_sym("schway__libretro_test_gun_input") != NULL)) {
        fprintf(stderr, "production libretro module exports card test accessors\n");
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
        check_string("valid_extensions", sys.valid_extensions, expected_valid_extensions());
    }
    CHECK_EQ_U64(sys.need_fullpath, 1);
    CHECK_EQ_U64(sys.block_extract, 1);
    set_callbacks();

    if (audio_mode) {
        return test_audio_equality(argv[4], argv[5], region);
    }

    if (options_profile_mode) {
        test_option_profiles();
        p_deinit();
        module_close();
        CHECK_DONE();
    }

    if (content_profile_mode) {
        test_content_profile(argv[3], argv[4]);
        p_deinit();
        module_close();
        CHECK_DONE();
    }

    if (content_boot_mode) {
        test_content_boot(argv[3], argv[4], argv[5], argv[6], argv[7]);
        p_deinit();
        module_close();
        CHECK_DONE();
    }

    if (disc_audio_mode) {
        return test_disc_audio_equality(argv[4], argv[5], argv[6], region);
    }

    if (disc_control_mode) {
        return test_disc_control(argv[3], argv[4], argv[5], argv[6]);
    }

    if (content_chd_mode) {
        int result = test_content_chd(argv[4], argv[5], argv[6], argv[7],
                                      argv[8], argv[3][0] == '1');
        p_deinit();
        module_close();
        if (result != 0) return result;
        CHECK_DONE();
    }

    if (content_pbp_mode) {
        int result = test_content_pbp(argv[4], argv[5], argv[6], argv[7],
                                      argv[8], argv[3][0] == '1');
        p_deinit();
        module_close();
        if (result != 0) return result;
        CHECK_DONE();
    }

    if (content_legacy_mode) {
        int result = test_content_legacy(argv[4], argv[5], argv[6], argv[7],
                                         argv[8], argv[9], argv[10], argv[11],
                                         argv[3][0] == '1');
        p_deinit();
        module_close();
        if (result != 0) return result;
        CHECK_DONE();
    }

    if (rumble_guest_mode) {
        return test_rumble_guest(argv[3]);
    }

    if (rumble_controls_mode) {
        return test_rumble_controls(argv[3]);
    }

    if (input_descriptors_mode) {
        return test_input_descriptors();
    }

    if (input_multitap_mode) {
        multitap_test_mode = 1;
        return test_input_multitap();
    }

    if (input_special_mode) {
        return test_input_special();
    }

    if (input_lightgun_mode) {
        return test_input_lightgun();
    }

    if (psexe_mode) {
        return test_content_psexe(argv[3], argv[4]);
    }

    if (lifecycle_mode) {
        content_lifecycle(argv[3], argv[4]);
        p_unload_game();
        p_deinit();
        module_close();
        CHECK_DONE();
    }

    if (card_persistence_mode) {
        return test_card_persistence(argv[3], argv[4]);
    }

    if (card_controls_mode) {
        return test_card_controls(argv[3], argv[4]);
    }

    if (cards_multitap_mode) {
        int result = test_cards_multitap(argv[3]);
        printf("TAP version 13\n1..1\n%s 1 - libretro.cards.multitap\n",
               result == 0 ? "ok" : "not ok");
        return result;
    }

    if (memory_aliases_mode) {
        return test_memory_aliases();
    }

    if (memory_boundaries_mode) {
        return test_memory_boundaries();
    }

    if (video_geometry_mode) {
        return test_native_rgb24_geometry(argv[3]);
    }

    if (video_edges_mode) {
        return test_native_video_geometry_edges(argv[3]);
    }

    if (input_movies_mode) {
        return test_input_movies(argv[3]);
    }

    if (input_controls_mode) {
        return test_input_controls(argv[3]);
    }

    if (content_mode) {
        unsigned red_pixel_seen = 0u;
        memset(&game, 0, sizeof game);
        game.path = argv[3];
        CHECK_EQ_U64(p_load_game(&game), 1);
        p_set_controller_port_device(0u, RETRO_DEVICE_JOYPAD);
        if (p_serialize_size() == 0u) {
            module_close();
            CHECK_DONE();
        }
        check_string("valid_extensions", sys.valid_extensions, expected_valid_extensions());
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

    /* Loaded games expose the first card and main RAM through libretro. */
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SAVE_RAM), 131072u);
    CHECK(p_get_memory_data(RETRO_MEMORY_SAVE_RAM) != NULL);
    CHECK_EQ_U64(p_get_memory_size(RETRO_MEMORY_SYSTEM_RAM), 2u * 1024u * 1024u);
    CHECK(p_get_memory_data(RETRO_MEMORY_SYSTEM_RAM) != NULL);
    p_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
    p_set_controller_port_device(1, RETRO_DEVICE_JOYPAD);
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
        step_with_devices(ref, "frame", lo, SCHWAY_INPUT_DEVICE_DIGITAL,
                          SCHWAY_INPUT_DEVICE_DIGITAL);
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
            step_with_devices(ref, "replayed frame", lo, SCHWAY_INPUT_DEVICE_DIGITAL,
                              SCHWAY_INPUT_DEVICE_DIGITAL);
        }
        CHECK(memcmp(frame, frame60, sizeof frame60) == 0);
    }
    free(state);
    free(ref_state);

    /* A power cycle, on both. */
    p_reset();
    CHECK_EQ_U64(schway_reset(ref, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    step_with_devices(ref, "frame after reset", lo, SCHWAY_INPUT_DEVICE_DIGITAL,
                      SCHWAY_INPUT_DEVICE_DIGITAL);

    /* An update that leaves the region alone keeps the instance. */
    option_updated = true;
    step_with_devices(ref, "frame after an unchanged option", lo, SCHWAY_INPUT_DEVICE_DIGITAL,
                      SCHWAY_INPUT_DEVICE_DIGITAL);
    CHECK_EQ_U64(av_info_calls, 0);

    /* Region changes wait for the next load and leave this session intact. */
    option_value = other == SCHWAY_REGION_PAL ? "pal" : "ntsc-j";
    option_updated = true;
    step_with_devices(ref, "frame with pending region selection", lo, SCHWAY_INPUT_DEVICE_DIGITAL,
                      SCHWAY_INPUT_DEVICE_DIGITAL);
    CHECK_EQ_U64(av_info_calls, 0);
    CHECK_EQ_U64(p_get_region(), pal ? RETRO_REGION_PAL : RETRO_REGION_NTSC);
    memset(&av, 0, sizeof av);
    p_get_system_av_info(&av);
    check_av(&av, region);
    p_unload_game();
    schway_destroy(ref);
    CHECK_EQ_U64(p_load_game(NULL), 1);
    ref = ref_create(other);
    CHECK(ref != NULL);
    if (ref == NULL) {
        module_close();
        return 1;
    }
    step_with_devices(ref, "frame after reloading changed region", other_lo,
                      SCHWAY_INPUT_DEVICE_DIGITAL, SCHWAY_INPUT_DEVICE_DIGITAL);
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
        step_with_devices(ref, "first frame after deinit", lo, SCHWAY_INPUT_DEVICE_DUALSHOCK,
                          SCHWAY_INPUT_DEVICE_DUALSHOCK);
        schway_destroy(ref);
    }

    p_unload_game();
    p_deinit();
    module_close();
    CHECK_DONE();
}
