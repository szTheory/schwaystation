/* The libretro adapter: the 25 entry points of libretro.h, written as a thin
 * client of schwaystation.h. "Lnnn" is a line of the vendored libretro.h
 * (libretro-common, pinned in PROVENANCE.md).
 *
 * Fullpath PS-EXEs are copied through the bounded reader into the core; the
 * emulator core never opens a file. With no selected content,
 * retro_load_game(NULL) keeps the built-in test-card path unless the frontend
 * explicitly selects a BIOS relative to its system directory. Audio is the
 * core's own silent pairs, exactly as many per frame as its clock gives, so
 * a frontend that paces on audio runs at the core's rate. The av_info
 * divisions are the only floating point in the shipped code; the core itself
 * is integer only. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libretro.h"
#include "schwaystation.h"
#include "cards.h"
#include "xrgb8888.h"

/* The largest display the GPU scans out: 640 dots by 480 lines NTSC or 576
   PAL, interlaced (.planning/preparation/HOSTS-AND-INTEGRATION.md section 1,
   AV fields). */
#define LR_MAX_W 640u
#define LR_MAX_H_NTSC 480u
#define LR_MAX_H_PAL 576u
#define LR_EXE_HEADER 0x800u
#define LR_EXE_MAX_BYTES (LR_EXE_HEADER + 2u * 1024u * 1024u)
#define LR_CARD_PATH_CAP 4096u
#define LR_MEMORY_DESCRIPTOR_COUNT 6u
#define LR_BIOS_KEY "schwaystation_bios"
#define LR_ANALOG_MODE_KEY "schwaystation_analog_mode"
#define LR_ANALOG_TOGGLE_KEY "schwaystation_analog_toggle"
#define LR_ANALOG_TOGGLE_FRAMES 60u
#define LR_ANALOG_TOGGLE_BUTTONS                                                            \
    ((1u << RETRO_DEVICE_ID_JOYPAD_SELECT) | (1u << RETRO_DEVICE_ID_JOYPAD_START) |        \
     (1u << RETRO_DEVICE_ID_JOYPAD_L) | (1u << RETRO_DEVICE_ID_JOYPAD_L2) |               \
     (1u << RETRO_DEVICE_ID_JOYPAD_R) | (1u << RETRO_DEVICE_ID_JOYPAD_R2))

/* All adapter state. A frontend may keep the module loaded between sessions,
   so retro_deinit returns every one of these to its initial value (L8866). */
static retro_environment_t env_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static schway *inst;
static schway_info info;
static uint32_t *pixels;
static int16_t *samples;
static unsigned geometry_width, geometry_height;
static char lr_card2_path[LR_CARD_PATH_CAP];
static uint64_t lr_card2_saved_generation;
static struct retro_memory_descriptor lr_memory_descriptors[LR_MEMORY_DESCRIPTOR_COUNT];
static struct retro_memory_map lr_memory_map;
static int lr_memory_map_registered;
static struct retro_rumble_interface lr_rumble;
static unsigned lr_controller_devices[2] = {RETRO_DEVICE_ANALOG, RETRO_DEVICE_ANALOG};
static uint8_t lr_analog_mode = 1u;
static uint8_t lr_analog_toggle_enabled = 1u;
static uint8_t lr_mode_pulse_pending[2];
static uint8_t lr_toggle_latched[2];
static uint8_t lr_toggle_frames[2];

/* The region option (HOSTS-AND-INTEGRATION section 1, Core options): the
   machine profile's region, applied by recreating the instance, since a
   profile is fixed once the first frame has run. The default comes first. */
#define LR_REGION_KEY "schwaystation_region"
static struct retro_core_option_v2_definition lr_option_defs[] = {
    {LR_REGION_KEY,
     "Region",
     NULL,
     "The console's region. Changing it restarts the console.",
     NULL,
     NULL,
     {{"ntsc-u", "NTSC-U/C"}, {"ntsc-j", "NTSC-J"}, {"pal", "PAL"}, {NULL, NULL}},
     "ntsc-u"},
    {LR_BIOS_KEY,
     "BIOS for no-content runs",
     NULL,
     "Optional file relative to system/. Missing or invalid files refuse this load. PS-EXE content uses the stub.",
     NULL,
     NULL,
     {{"none", "Built-in test card"},
      {"scph5500.bin", "SCPH-5500"},
      {"scph5501.bin", "SCPH-5501"},
      {"scph5502.bin", "SCPH-5502"},
      {NULL, NULL}},
     "none"},
    {LR_ANALOG_MODE_KEY,
     "DualShock mode",
     NULL,
     "The mode used when a DualShock controller is connected.",
     NULL,
     NULL,
     {{"analog", "Analog"}, {"digital", "Digital"}, {NULL, NULL}},
     "analog"},
    {LR_ANALOG_TOGGLE_KEY,
     "DualShock mode toggle",
     NULL,
     "Allow holding Select, Start, L1, L2, R1 and R2 for one second to toggle mode.",
     NULL,
     NULL,
     {{"enabled", "Enabled"}, {"disabled", "Disabled"}, {NULL, NULL}},
     "enabled"},
    {NULL, NULL, NULL, NULL, NULL, NULL, {{NULL, NULL}}, NULL},
};
static struct retro_core_option_v2_category lr_option_cats[] = {{NULL, NULL, NULL}};
static struct retro_core_options_v2 lr_options = {lr_option_cats, lr_option_defs};
/* The same option for a frontend without core options version 2 (L1021). */
static struct retro_variable lr_variables[] = {
    {LR_REGION_KEY, "Region; ntsc-u|ntsc-j|pal"},
    {LR_BIOS_KEY, "BIOS for no-content runs; none|scph5500.bin|scph5501.bin|scph5502.bin"},
    {LR_ANALOG_MODE_KEY, "DualShock mode; analog|digital"},
    {LR_ANALOG_TOGGLE_KEY, "DualShock mode toggle; enabled|disabled"},
    {NULL, NULL},
};

static struct retro_controller_description lr_controller_types[] = {
    {"DualShock", RETRO_DEVICE_ANALOG},
    {"PlayStation Controller", RETRO_DEVICE_JOYPAD},
    {"None", RETRO_DEVICE_NONE},
};
static struct retro_controller_info lr_controller_info[] = {
    {lr_controller_types, (unsigned)(sizeof lr_controller_types / sizeof lr_controller_types[0])},
    {lr_controller_types, (unsigned)(sizeof lr_controller_types / sizeof lr_controller_types[0])},
    {NULL, 0u},
};

#define LR_PAD_DESCRIPTORS(port_)                                                             \
    {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_B, "Cross"},                     \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_A, "Circle"},                \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_Y, "Square"},                \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_X, "Triangle"},              \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_L, "L1"},                    \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_R, "R1"},                    \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_L2, "L2"},                   \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_R2, "R2"},                   \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_L3, "L3"},                   \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_R3, "R3"},                   \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select"},           \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_START, "Start"},             \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_UP, "Up"},                   \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right"},             \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down"},               \
        {port_, RETRO_DEVICE_JOYPAD, 0u, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left"},               \
        {port_, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, \
         "Left Stick X"},                                                                    \
        {port_, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, \
         "Left Stick Y"},                                                                    \
        {port_, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, \
         "Right Stick X"},                                                                   \
        {port_, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, \
         "Right Stick Y"}

static struct retro_input_descriptor lr_input_descriptors[] = {
    LR_PAD_DESCRIPTORS(0u),
    LR_PAD_DESCRIPTORS(1u),
    {0u, 0u, 0u, 0u, NULL},
};
#undef LR_PAD_DESCRIPTORS

/* The allocator the adapter hands the core: the C library's malloc and free. */
static void *lr_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void lr_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static void lr_set_rumble(unsigned port, enum retro_rumble_effect effect, uint16_t strength)
{
    if (lr_rumble.set_rumble_state != NULL) {
        (void)lr_rumble.set_rumble_state(port, effect, strength);
    }
}

static void lr_clear_rumble(void)
{
    for (unsigned port = 0u; port < 2u; port++) {
        lr_set_rumble(port, RETRO_RUMBLE_STRONG, 0u);
        lr_set_rumble(port, RETRO_RUMBLE_WEAK, 0u);
    }
}

static void lr_acquire_rumble(void)
{
    struct retro_rumble_interface interface;
    if (env_cb == NULL) {
        return;
    }
    memset(&interface, 0, sizeof interface);
    if (env_cb(RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &interface) &&
        interface.set_rumble_state != NULL) {
        lr_rumble = interface;
    }
}

static void lr_card2_flush(void)
{
    schway_span card;
    uint64_t generation;
    if (inst == NULL || lr_card2_path[0] == '\0' ||
        schway_memcard_generation(inst, SCHWAY_CARD_SLOT_2, &generation) != SCHWAY_STATUS_OK ||
        generation == lr_card2_saved_generation ||
        schway_get_memory(inst, SCHWAY_MEMORY_CARD2, &card) != SCHWAY_STATUS_OK ||
        card.size != SCHWAY_MEMCARD_SIZE) {
        return;
    }
    if (schway_host_card_save_atomic(lr_card2_path, card.data, card.size) ==
        SCHWAY_HOST_CARD_OK) {
        lr_card2_saved_generation = generation;
    }
}

static void lr_memory_map_clear(void)
{
    if (lr_memory_map_registered && env_cb != NULL) {
        struct retro_memory_map empty = {NULL, 0u};
        (void)env_cb(RETRO_ENVIRONMENT_SET_MEMORY_MAPS, &empty);
    }
    memset(lr_memory_descriptors, 0, sizeof lr_memory_descriptors);
    memset(&lr_memory_map, 0, sizeof lr_memory_map);
    lr_memory_map_registered = 0;
}

static void lr_memory_descriptor_set(unsigned index, uint64_t flags, void *ptr, size_t start,
                                     size_t select, size_t disconnect, size_t length)
{
    struct retro_memory_descriptor *descriptor = &lr_memory_descriptors[index];
    descriptor->flags = flags;
    descriptor->ptr = ptr;
    descriptor->offset = 0u;
    descriptor->start = start;
    descriptor->select = select;
    descriptor->disconnect = disconnect;
    descriptor->len = length;
    descriptor->addrspace = "PSX";
}

/* The instance owns mutable bytes; the public span is read-only but libretro
   exposes the same live storage through its writable memory API. */
static void *lr_memory_mutable_pointer(const void *data)
{
    return (void *)(uintptr_t)data;
}

static void lr_memory_map_register(void)
{
    static const size_t ram_starts[3] = {0u, (size_t)0x80000000u, (size_t)0xA0000000u};
    static const size_t scratch_starts[2] = {(size_t)0x1F800000u, (size_t)0x9F800000u};
    schway_span ram, scratchpad;
    if (inst == NULL ||
        schway_get_memory(inst, SCHWAY_MEMORY_RAM, &ram) != SCHWAY_STATUS_OK ||
        schway_get_memory(inst, SCHWAY_MEMORY_SCRATCHPAD, &scratchpad) != SCHWAY_STATUS_OK ||
        ram.size != 0x200000u || scratchpad.size != 0x400u) {
        return;
    }
    for (unsigned i = 0u; i < 3u; i++) {
        lr_memory_descriptor_set(i, RETRO_MEMDESC_SYSTEM_RAM,
                                 lr_memory_mutable_pointer(ram.data), ram_starts[i],
                                 ~(size_t)0x007FFFFFu, 0x00600000u, ram.size);
    }
    for (unsigned i = 0u; i < 2u; i++) {
        lr_memory_descriptor_set(3u + i, 0u, lr_memory_mutable_pointer(scratchpad.data),
                                 scratch_starts[i],
                                 ~(size_t)0x3FFu, 0u, scratchpad.size);
    }
    lr_memory_descriptor_set(5u, 0u, NULL, 0u, (size_t)0xFFFFFFFFu, 0u, 0u);
    lr_memory_map.descriptors = lr_memory_descriptors;
    lr_memory_map.num_descriptors = LR_MEMORY_DESCRIPTOR_COUNT;
    if (env_cb != NULL) {
        lr_memory_map_registered =
            env_cb(RETRO_ENVIRONMENT_SET_MEMORY_MAPS, &lr_memory_map) ? 1 : 0;
    }
}

static int lr_card2_path_for_game(const struct retro_game_info *game,
                                  char path[LR_CARD_PATH_CAP])
{
    const char *save_directory = NULL;
    const char *basename;
    const char *extension;
    char content_name[64];
    char card_one[LR_CARD_PATH_CAP];
    size_t name_size;
    if (game == NULL || game->path == NULL || env_cb == NULL ||
        !env_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_directory) ||
        save_directory == NULL || save_directory[0] == '\0') {
        return 0;
    }
    basename = game->path;
    for (const char *scan = game->path; *scan != '\0'; scan++) {
        if (*scan == '/' || *scan == '\\') {
            basename = scan + 1;
        }
    }
    extension = strrchr(basename, '.');
    if (extension == NULL || extension == basename) {
        return 0;
    }
    name_size = (size_t)(extension - basename);
    if (name_size == 0u || name_size >= sizeof content_name) {
        return 0;
    }
    memcpy(content_name, basename, name_size);
    content_name[name_size] = '\0';
    return schway_host_card_paths_for_basename(save_directory, content_name, card_one,
                                                sizeof card_one, path, LR_CARD_PATH_CAP) ==
           SCHWAY_HOST_CARD_OK;
}

static void lr_card2_load(const char *path)
{
    uint8_t *bytes;
    uint64_t generation = 0u;
    if (inst == NULL || path == NULL || path[0] == '\0') {
        return;
    }
    bytes = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    if (bytes == NULL) {
        return;
    }
    if (schway_host_card_import_file(path, bytes, SCHWAY_MEMCARD_SIZE, NULL) ==
            SCHWAY_HOST_CARD_OK &&
        schway_load_memcard(inst, SCHWAY_CARD_SLOT_2, bytes, SCHWAY_MEMCARD_SIZE) !=
            SCHWAY_STATUS_OK) {
        free(bytes);
        return;
    }
    free(bytes);
    memcpy(lr_card2_path, path, strlen(path) + 1u);
    if (schway_memcard_generation(inst, SCHWAY_CARD_SLOT_2, &generation) == SCHWAY_STATUS_OK) {
        lr_card2_saved_generation = generation;
    }
}

/* Destroys the instance and frees the frame buffer. */
static void lr_unload(void)
{
    lr_card2_flush();
    lr_memory_map_clear();
    if (inst != NULL) {
        lr_clear_rumble();
    }
    for (unsigned port = 0u; port < 2u; port++) {
        lr_mode_pulse_pending[port] =
            (lr_controller_devices[port] & RETRO_DEVICE_MASK) == RETRO_DEVICE_ANALOG &&
                    lr_analog_mode == 0u
                ? 1u
                : 0u;
        lr_toggle_frames[port] = 0u;
        lr_toggle_latched[port] = 0u;
    }
    schway_destroy(inst);
    inst = NULL;
    free(pixels);
    pixels = NULL;
    free(samples);
    samples = NULL;
    geometry_width = 0u;
    geometry_height = 0u;
    lr_card2_path[0] = '\0';
    lr_card2_saved_generation = 0u;
    memset(&info, 0, sizeof info);
}

/* Creates the one instance with the given region; 0 on failure, with
   nothing held. */
static int lr_create(uint32_t region)
{
    schway_config cfg;
    schway_profile profile;

    lr_unload();
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = lr_alloc;
    cfg.allocator.free = lr_free;
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = region;
    info.size = (uint32_t)sizeof info;
    if (schway_create(&cfg, &inst) != SCHWAY_STATUS_OK) {
        inst = NULL;
        return 0;
    }
    if (schway_set_profile(inst, &profile) != SCHWAY_STATUS_OK ||
        schway_get_info(inst, &info) != SCHWAY_STATUS_OK || info.fps_den == 0u ||
        info.sample_rate_den == 0u) {
        lr_unload();
        return 0;
    }
    pixels = (uint32_t *)malloc((size_t)LR_MAX_W * LR_MAX_H_PAL * sizeof *pixels);
    samples = (int16_t *)malloc((size_t)info.max_audio_frames * 2u * sizeof *samples);
    if (pixels == NULL || samples == NULL) {
        lr_unload();
        return 0;
    }
    geometry_width = info.width;
    geometry_height = info.height;
    return 1;
}

/* Content selection is a host file operation. Read at most the largest image
   schway_probe_exe can accept, plus one byte to distinguish oversized input. */
static FILE *lr_open_readonly(const char *path)
{
#if defined(_MSC_VER)
    FILE *file = NULL;
    if (fopen_s(&file, path, "rb") != 0) {
        return NULL;
    }
    return file;
#else
    return fopen(path, "rb");
#endif
}

static uint8_t *lr_read_exe(const char *path, size_t *size)
{
    FILE *file;
    uint8_t *bytes;
    size_t length;
    int failed;

    if (path == NULL || size == NULL) {
        return NULL;
    }
    *size = 0u;
    file = lr_open_readonly(path);
    if (file == NULL) {
        return NULL;
    }
    bytes = (uint8_t *)malloc((size_t)LR_EXE_MAX_BYTES + 1u);
    if (bytes == NULL) {
        (void)fclose(file);
        return NULL;
    }
    length = fread(bytes, 1u, (size_t)LR_EXE_MAX_BYTES + 1u, file);
    failed = ferror(file);
    if (fclose(file) != 0) {
        failed = 1;
    }
    if (failed != 0 || length > (size_t)LR_EXE_MAX_BYTES) {
        free(bytes);
        return NULL;
    }
    *size = length;
    return bytes;
}

static int lr_is_exe_path(const char *path)
{
    const char *extension;
    size_t length;
    if (path == NULL) {
        return 0;
    }
    extension = strrchr(path, '.');
    if (extension == NULL) {
        return 0;
    }
    length = strlen(extension);
    if (length == 4u && (extension[1] == 'e' || extension[1] == 'E') &&
        (extension[2] == 'x' || extension[2] == 'X') &&
        (extension[3] == 'e' || extension[3] == 'E')) {
        return 1;
    }
    return length == 6u && (extension[1] == 'p' || extension[1] == 'P') &&
           (extension[2] == 's' || extension[2] == 'S') &&
           (extension[3] == 'e' || extension[3] == 'E') &&
           (extension[4] == 'x' || extension[4] == 'X') &&
           (extension[5] == 'e' || extension[5] == 'E');
}

/* RetroPad positions map to the active-low serial word used by the PSX pad. */
static void lr_queue_input(void)
{
    static const struct {
        unsigned id;
        uint16_t mask;
    } mapping[] = {
        {RETRO_DEVICE_ID_JOYPAD_SELECT, 0x0001u}, {RETRO_DEVICE_ID_JOYPAD_L3, 0x0002u},
        {RETRO_DEVICE_ID_JOYPAD_R3, 0x0004u},     {RETRO_DEVICE_ID_JOYPAD_START, 0x0008u},
        {RETRO_DEVICE_ID_JOYPAD_UP, 0x0010u},     {RETRO_DEVICE_ID_JOYPAD_RIGHT, 0x0020u},
        {RETRO_DEVICE_ID_JOYPAD_DOWN, 0x0040u},   {RETRO_DEVICE_ID_JOYPAD_LEFT, 0x0080u},
        {RETRO_DEVICE_ID_JOYPAD_L2, 0x0100u},     {RETRO_DEVICE_ID_JOYPAD_R2, 0x0200u},
        {RETRO_DEVICE_ID_JOYPAD_L, 0x0400u},      {RETRO_DEVICE_ID_JOYPAD_R, 0x0800u},
        {RETRO_DEVICE_ID_JOYPAD_X, 0x1000u},      {RETRO_DEVICE_ID_JOYPAD_A, 0x2000u},
        {RETRO_DEVICE_ID_JOYPAD_B, 0x4000u},      {RETRO_DEVICE_ID_JOYPAD_Y, 0x8000u},
    };
    schway_input snapshot;

    memset(&snapshot, 0, sizeof snapshot);
    snapshot.size = (uint32_t)sizeof snapshot;
    for (unsigned port = 0u; port < 2u; port++) {
        for (unsigned axis = 0u; axis < 4u; axis++) {
            snapshot.axes[port][axis] = 128u;
        }
    }
    for (unsigned port = 0u; port < 2u; port++) {
        unsigned device = lr_controller_devices[port] & RETRO_DEVICE_MASK;
        uint32_t toggle_buttons = 0u;
        uint8_t toggle_pulse = 0u;

        if (device == RETRO_DEVICE_ANALOG) {
            snapshot.device[port] = SCHWAY_INPUT_DEVICE_DUALSHOCK;
        } else if (device == RETRO_DEVICE_JOYPAD) {
            snapshot.device[port] = SCHWAY_INPUT_DEVICE_DIGITAL;
        } else {
            snapshot.device[port] = SCHWAY_INPUT_DEVICE_NONE;
        }
        if (snapshot.device[port] != SCHWAY_INPUT_DEVICE_NONE) {
            for (size_t i = 0u; i < sizeof mapping / sizeof mapping[0]; i++) {
                if (input_state_cb != NULL &&
                    input_state_cb(port, RETRO_DEVICE_JOYPAD, 0u, mapping[i].id) != 0) {
                    snapshot.buttons[port] |= mapping[i].mask;
                    if (mapping[i].id < 32u) {
                        toggle_buttons |= 1u << mapping[i].id;
                    }
                }
            }
        }
        if (snapshot.device[port] == SCHWAY_INPUT_DEVICE_DUALSHOCK && input_state_cb != NULL) {
            int16_t left_x = input_state_cb(port, RETRO_DEVICE_ANALOG,
                                            RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                            RETRO_DEVICE_ID_ANALOG_X);
            int16_t left_y = input_state_cb(port, RETRO_DEVICE_ANALOG,
                                            RETRO_DEVICE_INDEX_ANALOG_LEFT,
                                            RETRO_DEVICE_ID_ANALOG_Y);
            int16_t right_x = input_state_cb(port, RETRO_DEVICE_ANALOG,
                                             RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                             RETRO_DEVICE_ID_ANALOG_X);
            int16_t right_y = input_state_cb(port, RETRO_DEVICE_ANALOG,
                                             RETRO_DEVICE_INDEX_ANALOG_RIGHT,
                                             RETRO_DEVICE_ID_ANALOG_Y);
            int32_t axes[4] = {right_x, right_y, left_x, left_y};
            for (unsigned axis = 0u; axis < 4u; axis++) {
                int32_t value = axes[axis];
                snapshot.axes[port][axis] = value < 0
                                                ? (uint8_t)(((value + 32768) * 128) / 32768)
                                                : (uint8_t)(128 + (value * 127) / 32767);
            }
        }
        if (snapshot.device[port] == SCHWAY_INPUT_DEVICE_DUALSHOCK) {
            if (lr_mode_pulse_pending[port] != 0u) {
                toggle_pulse = 1u;
                lr_mode_pulse_pending[port] = 0u;
            }
            if (lr_analog_toggle_enabled != 0u &&
                (toggle_buttons & LR_ANALOG_TOGGLE_BUTTONS) == LR_ANALOG_TOGGLE_BUTTONS) {
                if (lr_toggle_frames[port] < LR_ANALOG_TOGGLE_FRAMES) {
                    lr_toggle_frames[port]++;
                }
                if (lr_toggle_frames[port] == LR_ANALOG_TOGGLE_FRAMES &&
                    lr_toggle_latched[port] == 0u) {
                    toggle_pulse = 1u;
                    lr_toggle_latched[port] = 1u;
                }
            } else {
                lr_toggle_frames[port] = 0u;
                lr_toggle_latched[port] = 0u;
            }
            snapshot.analog_button[port] = toggle_pulse;
        } else {
            lr_toggle_frames[port] = 0u;
            lr_toggle_latched[port] = 0u;
        }
    }
    (void)schway_set_input(inst, &snapshot);
}

/* The region the frontend's option names; ntsc-u when it names none. */
static uint32_t lr_option_region(void)
{
    struct retro_variable var = {LR_REGION_KEY, NULL};
    if (env_cb == NULL || !env_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || var.value == NULL) {
        return SCHWAY_REGION_NTSC_U;
    }
    if (strcmp(var.value, "ntsc-j") == 0) {
        return SCHWAY_REGION_NTSC_J;
    }
    if (strcmp(var.value, "pal") == 0) {
        return SCHWAY_REGION_PAL;
    }
    return SCHWAY_REGION_NTSC_U;
}

static const char *lr_option_bios(void)
{
    struct retro_variable var = {LR_BIOS_KEY, NULL};
    if (env_cb == NULL || !env_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || var.value == NULL) {
        return "none";
    }
    return var.value;
}

static void lr_refresh_input_options(void)
{
    struct retro_variable var = {LR_ANALOG_MODE_KEY, NULL};
    if (env_cb != NULL && env_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value != NULL) {
        uint8_t mode;
        if (strcmp(var.value, "digital") == 0) {
            mode = 0u;
        } else if (strcmp(var.value, "analog") == 0) {
            mode = 1u;
        } else {
            mode = lr_analog_mode;
        }
        if (mode != lr_analog_mode) {
            lr_analog_mode = mode;
            for (unsigned port = 0u; port < 2u; port++) {
                lr_mode_pulse_pending[port] = 1u;
            }
        }
    }
    var.key = LR_ANALOG_TOGGLE_KEY;
    var.value = NULL;
    if (env_cb != NULL && env_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value != NULL) {
        if (strcmp(var.value, "enabled") == 0) {
            lr_analog_toggle_enabled = 1u;
        } else if (strcmp(var.value, "disabled") == 0) {
            lr_analog_toggle_enabled = 0u;
            memset(lr_toggle_frames, 0, sizeof lr_toggle_frames);
            memset(lr_toggle_latched, 0, sizeof lr_toggle_latched);
        }
    }
}

/* BIOS values are relative to the frontend's system directory. Reject
   absolute paths and dot segments so the selection cannot escape that directory. */
static int lr_bios_path_is_relative(const char *name)
{
    const char *part;
    if (name == NULL || name[0] == '\0' || name[0] == '/' || name[0] == '\\' ||
        (name[0] != '\0' && name[1] == ':')) {
        return 0;
    }
    part = name;
    for (const char *p = name;; p++) {
        if (*p == '\\' || *p == ':') {
            return 0;
        }
        if (*p == '/' || *p == '\0') {
            size_t length = (size_t)(p - part);
            if (length == 0u || (length == 1u && part[0] == '.') ||
                (length == 2u && part[0] == '.' && part[1] == '.')) {
                return 0;
            }
            if (*p == '\0') {
                return 1;
            }
            part = p + 1;
        }
    }
}

/* Reads only an exact-size user BIOS from the frontend's system directory. */
static uint8_t *lr_read_bios(const char *name, size_t *size)
{
    const char *system_dir = NULL;
    char *path;
    FILE *file;
    uint8_t *bytes;
    size_t dir_len, name_len, separator, path_len, length;
    int failed;

    if (size == NULL || !lr_bios_path_is_relative(name) || env_cb == NULL ||
        !env_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) || system_dir == NULL ||
        system_dir[0] == '\0') {
        return NULL;
    }
    *size = 0u;
    dir_len = strlen(system_dir);
    name_len = strlen(name);
    if (dir_len > 4096u || name_len > 1024u) {
        return NULL;
    }
    separator = dir_len != 0u && system_dir[dir_len - 1u] != '/' &&
                        system_dir[dir_len - 1u] != '\\'
                    ? 1u
                    : 0u;
    if (dir_len > SIZE_MAX - name_len || dir_len + name_len > SIZE_MAX - separator - 1u) {
        return NULL;
    }
    path_len = dir_len + name_len + separator;
    path = (char *)malloc(path_len + 1u);
    if (path == NULL) {
        return NULL;
    }
    memcpy(path, system_dir, dir_len);
    if (separator != 0u) {
        path[dir_len] = '/';
    }
    memcpy(path + dir_len + separator, name, name_len);
    path[path_len] = '\0';
    file = lr_open_readonly(path);
    free(path);
    if (file == NULL) {
        return NULL;
    }
    bytes = (uint8_t *)malloc((size_t)SCHWAY_BIOS_SIZE + 1u);
    if (bytes == NULL) {
        (void)fclose(file);
        return NULL;
    }
    length = fread(bytes, 1u, (size_t)SCHWAY_BIOS_SIZE + 1u, file);
    failed = ferror(file);
    if (fclose(file) != 0) {
        failed = 1;
    }
    if (failed != 0 || length != (size_t)SCHWAY_BIOS_SIZE) {
        free(bytes);
        return NULL;
    }
    *size = length;
    return bytes;
}

/* L8770, called before retro_init (L8766). Declaring no-game support (L1056)
   makes the frontend call retro_load_game(NULL). The options go through
   SET_CORE_OPTIONS_V2 (L2362) when the frontend reports version 2 or more
   (L1871), else through SET_VARIABLES (L1021). */
void retro_set_environment(retro_environment_t cb)
{
    bool no_game = true;
    unsigned version = 0u;
    env_cb = cb;
    if (cb == NULL) {
        return;
    }
    (void)cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_game);
    if (cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version) && version >= 2u) {
        (void)cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &lr_options);
    } else {
        (void)cb(RETRO_ENVIRONMENT_SET_VARIABLES, lr_variables);
    }
    (void)cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, lr_controller_info);
    (void)cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, lr_input_descriptors);
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
    video_cb = cb;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
    audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
    audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
    input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
    input_state_cb = cb;
}

/* L8841: the instance is created in retro_load_game, not here. */
void retro_init(void)
{
    lr_acquire_rumble();
}

/* L8866: release everything and reset every static; the module may be
   initialised again. */
void retro_deinit(void)
{
    lr_unload();
    memset(&lr_rumble, 0, sizeof lr_rumble);
    lr_controller_devices[0] = RETRO_DEVICE_ANALOG;
    lr_controller_devices[1] = RETRO_DEVICE_ANALOG;
    lr_analog_mode = 1u;
    lr_analog_toggle_enabled = 1u;
    memset(lr_mode_pulse_pending, 0, sizeof lr_mode_pulse_pending);
    memset(lr_toggle_latched, 0, sizeof lr_toggle_latched);
    memset(lr_toggle_frames, 0, sizeof lr_toggle_frames);
    env_cb = NULL;
    video_cb = NULL;
    audio_cb = NULL;
    audio_batch_cb = NULL;
    input_poll_cb = NULL;
    input_state_cb = NULL;
}

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

/* L8886: PS-EXEs use a fullpath host read. Disc images remain unsupported and
   archives are not extracted. */
void retro_get_system_info(struct retro_system_info *sys)
{
    if (sys == NULL) {
        return;
    }
    memset(sys, 0, sizeof *sys);
    sys->library_name = "schwaystation";
    sys->library_version = SCHWAYSTATION_LIBRETRO_VERSION;
    sys->valid_extensions = "exe|psexe";
    sys->need_fullpath = true;
    sys->block_extract = true;
}

/* L8901: geometry and timing. The rates are the core's exact rationals from
   schway_get_info, divided once here; the display area fills a 4:3 raster at
   every width mode (HOSTS-AND-INTEGRATION section 1, AV fields). */
void retro_get_system_av_info(struct retro_system_av_info *av)
{
    if (av == NULL) {
        return;
    }
    memset(av, 0, sizeof *av);
    if (inst == NULL) {
        return;
    }
    av->geometry.base_width = info.width;
    av->geometry.base_height = info.height;
    av->geometry.max_width = LR_MAX_W;
    av->geometry.max_height = info.region == SCHWAY_REGION_PAL ? LR_MAX_H_PAL : LR_MAX_H_NTSC;
    av->geometry.aspect_ratio = 4.0f / 3.0f;
    av->timing.fps = (double)info.fps_num / (double)info.fps_den;
    av->timing.sample_rate = (double)info.sample_rate_num / (double)info.sample_rate_den;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
    unsigned base = device & RETRO_DEVICE_MASK;
    if (port >= 2u || (base != RETRO_DEVICE_NONE && base != RETRO_DEVICE_JOYPAD &&
                       base != RETRO_DEVICE_ANALOG)) {
        return;
    }
    lr_controller_devices[port] = device;
    lr_toggle_frames[port] = 0u;
    lr_toggle_latched[port] = 0u;
    lr_mode_pulse_pending[port] =
        base == RETRO_DEVICE_ANALOG && lr_analog_mode == 0u ? 1u : 0u;
}

/* L8939: a power cycle, the only reset this release has. */
void retro_reset(void)
{
    lr_clear_rumble();
    for (unsigned port = 0u; port < 2u; port++) {
        lr_mode_pulse_pending[port] =
            (lr_controller_devices[port] & RETRO_DEVICE_MASK) == RETRO_DEVICE_ANALOG &&
                    lr_analog_mode == 0u
                ? 1u
                : 0u;
        lr_toggle_frames[port] = 0u;
        lr_toggle_latched[port] = 0u;
    }
    if (inst != NULL) {
        (void)schway_reset(inst, SCHWAY_RESET_POWER);
    }
}

/* Sends this frame's pairs, drained from the core, through the batch
   callback, or pair by pair when the frontend registered only the single
   sample callback (HOSTS-AND-INTEGRATION section 1, Frame delivery). */
static void lr_audio(void)
{
    schway_audio io;
    uint32_t i;
    memset(&io, 0, sizeof io);
    io.size = (uint32_t)sizeof io;
    io.samples = samples;
    io.capacity = info.max_audio_frames;
    if (schway_get_audio(inst, &io) != SCHWAY_STATUS_OK || io.written == 0u) {
        return;
    }
    if (audio_batch_cb != NULL) {
        (void)audio_batch_cb(samples, io.written);
    } else if (audio_cb != NULL) {
        for (i = 0; i < io.written; i++) {
            audio_cb(samples[2u * i], samples[2u * i + 1u]);
        }
    }
}

/* A changed region option (L1039) recreates the instance and tells the
   frontend the new rate and geometry (L1370). Returns 0 when no instance
   could be created. */
static int lr_check_region(void)
{
    bool updated = false;
    struct retro_system_av_info av;
    uint32_t region;
    if (env_cb == NULL || !env_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) || !updated) {
        return 1;
    }
    lr_refresh_input_options();
    region = lr_option_region();
    if (region == info.region) {
        return 1;
    }
    if (!lr_create(region)) {
        return 0;
    }
    retro_get_system_av_info(&av);
    (void)env_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &av);
    return 1;
}

/* L8953: one frame. Input is polled once and not yet read. The display area
   is converted into the adapter's buffer and sent with one video call; a
   failed frame sends no video or audio. */
void retro_run(void)
{
    schway_video video;
    schway_run_result result;

    if (input_poll_cb != NULL) {
        input_poll_cb();
    }
    if (inst == NULL || !lr_check_region()) {
        return;
    }
    lr_queue_input();
    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    if (schway_run_frame(inst, &result) != SCHWAY_STATUS_OK) {
        return;
    }
    for (unsigned port = 0u; port < 2u; port++) {
        uint8_t small = (uint8_t)(result.rumble[port] & 0xFFu);
        uint8_t large = (uint8_t)(result.rumble[port] >> 8);
        lr_set_rumble(port, RETRO_RUMBLE_STRONG, (uint16_t)((uint32_t)large * 257u));
        lr_set_rumble(port, RETRO_RUMBLE_WEAK, small != 0u ? UINT16_MAX : 0u);
    }
    memset(&video, 0, sizeof video);
    video.size = (uint32_t)sizeof video;
    if (schway_get_video(inst, &video) != SCHWAY_STATUS_OK ||
        !schway_host_video_valid(&video) || video.width > LR_MAX_W ||
        video.height > LR_MAX_H_PAL ||
        !schway_host_xrgb8888_frame(&video, pixels, video.width)) {
        return;
    }
    if ((video.width != geometry_width || video.height != geometry_height) && env_cb != NULL) {
        struct retro_game_geometry geometry;
        memset(&geometry, 0, sizeof geometry);
        geometry.base_width = video.width;
        geometry.base_height = video.height;
        geometry.max_width = LR_MAX_W;
        geometry.max_height = info.region == SCHWAY_REGION_PAL ? LR_MAX_H_PAL : LR_MAX_H_NTSC;
        geometry.aspect_ratio = 4.0f / 3.0f;
        (void)env_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &geometry);
    }
    geometry_width = video.width;
    geometry_height = video.height;
    if (video_cb != NULL) {
        video_cb(pixels, video.width, video.height, (size_t)video.width * sizeof pixels[0]);
    }
    lr_audio();
}

/* L8966-8989: the library's state, little-endian and checked on load. The
   .info still claims no save states until a release proves them in
   RetroArch. */
size_t retro_serialize_size(void)
{
    return inst != NULL ? schway_state_size(inst) : 0u;
}

bool retro_serialize(void *data, size_t len)
{
    return inst != NULL && data != NULL && schway_save_state(inst, data, len) == SCHWAY_STATUS_OK;
}

bool retro_unserialize(const void *data, size_t len)
{
    return inst != NULL && data != NULL && schway_load_state(inst, data, len) == SCHWAY_STATUS_OK;
}

void retro_cheat_reset(void)
{
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
    (void)index;
    (void)enabled;
    (void)code;
}

/* L9020. NULL boots the test card or an explicitly selected BIOS. A fullpath
   PS-EXE always uses the core's real-MIPS stub. XRGB8888 is requested here
   (L870); the default 0RGB1555 is deprecated. */
bool retro_load_game(const struct retro_game_info *game)
{
    enum retro_pixel_format format = RETRO_PIXEL_FORMAT_XRGB8888;
    uint8_t *exe = NULL;
    size_t exe_size = 0u;
    schway_exe_info exe_info;
    uint8_t *bios = NULL;
    size_t bios_size = 0u;
    char card2_path[LR_CARD_PATH_CAP];
    int have_card2_path = 0;

    card2_path[0] = '\0';

    lr_acquire_rumble();
    lr_refresh_input_options();

    if (game != NULL) {
        if (!lr_is_exe_path(game->path)) {
            return false;
        }
        exe = lr_read_exe(game->path, &exe_size);
        if (exe == NULL) {
            return false;
        }
        memset(&exe_info, 0, sizeof exe_info);
        exe_info.size = (uint32_t)sizeof exe_info;
        if (schway_probe_exe(exe, exe_size, &exe_info) != SCHWAY_STATUS_OK) {
            free(exe);
            return false;
        }
        have_card2_path = lr_card2_path_for_game(game, card2_path);
    } else {
        const char *bios_name = lr_option_bios();
        if (strcmp(bios_name, "none") != 0) {
            bios = lr_read_bios(bios_name, &bios_size);
            if (bios == NULL) {
                return false;
            }
        }
    }
    if (env_cb == NULL || !env_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &format)) {
        free(exe);
        free(bios);
        return false;
    }
    if (!lr_create(lr_option_region())) {
        free(exe);
        free(bios);
        return false;
    }
    if (have_card2_path) {
        lr_card2_load(card2_path);
    }
    if (game != NULL) {
        schway_status status = schway_load_exe(inst, exe, exe_size);
        free(exe);
        if (status != SCHWAY_STATUS_OK) {
            lr_unload();
            return false;
        }
    } else if (bios != NULL) {
        schway_status status = schway_load_bios(inst, bios, bios_size);
        free(bios);
        if (status != SCHWAY_STATUS_OK) {
            lr_unload();
            return false;
        }
    }
    lr_memory_map_register();
    return true;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *game,
                             size_t num_info)
{
    (void)game_type;
    (void)game;
    (void)num_info;
    return false;
}

/* L9057: the instance goes with the game. */
void retro_unload_game(void)
{
    lr_unload();
}

/* L9071, values L443-444: from the loaded profile. */
unsigned retro_get_region(void)
{
    return info.region == SCHWAY_REGION_PAL ? RETRO_REGION_PAL : RETRO_REGION_NTSC;
}

static int lr_memory_span(unsigned id, schway_span *span)
{
    uint32_t memory;
    if (inst == NULL || span == NULL) {
        return 0;
    }
    if (id == RETRO_MEMORY_SAVE_RAM) {
        memory = SCHWAY_MEMORY_CARD1;
    } else if (id == RETRO_MEMORY_SYSTEM_RAM) {
        memory = SCHWAY_MEMORY_RAM;
    } else {
        return 0;
    }
    return schway_get_memory(inst, memory, span) == SCHWAY_STATUS_OK;
}

/* L9085: frontend SAVE_RAM is card 1; the system memory API reports main RAM. */
void *retro_get_memory_data(unsigned id)
{
    schway_span span;
    return lr_memory_span(id, &span) ? lr_memory_mutable_pointer(span.data) : NULL;
}

size_t retro_get_memory_size(unsigned id)
{
    schway_span span;
    return lr_memory_span(id, &span) ? span.size : 0u;
}
