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
#include "xrgb8888.h"

/* The largest display the GPU scans out: 640 dots by 480 lines NTSC or 576
   PAL, interlaced (.planning/preparation/HOSTS-AND-INTEGRATION.md section 1,
   AV fields). */
#define LR_MAX_W 640u
#define LR_MAX_H_NTSC 480u
#define LR_MAX_H_PAL 576u
#define LR_EXE_HEADER 0x800u
#define LR_EXE_MAX_BYTES (LR_EXE_HEADER + 2u * 1024u * 1024u)
#define LR_BIOS_KEY "schwaystation_bios"

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
    {NULL, NULL, NULL, NULL, NULL, NULL, {{NULL, NULL}}, NULL},
};
static struct retro_core_option_v2_category lr_option_cats[] = {{NULL, NULL, NULL}};
static struct retro_core_options_v2 lr_options = {lr_option_cats, lr_option_defs};
/* The same option for a frontend without core options version 2 (L1021). */
static struct retro_variable lr_variables[] = {
    {LR_REGION_KEY, "Region; ntsc-u|ntsc-j|pal"},
    {LR_BIOS_KEY, "BIOS for no-content runs; none|scph5500.bin|scph5501.bin|scph5502.bin"},
    {NULL, NULL},
};

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

/* Destroys the instance and frees the frame buffer. */
static void lr_unload(void)
{
    schway_destroy(inst);
    inst = NULL;
    free(pixels);
    pixels = NULL;
    free(samples);
    samples = NULL;
    geometry_width = 0u;
    geometry_height = 0u;
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
    file = fopen(path, "rb");
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
    if (path == NULL) {
        return 0;
    }
    extension = strrchr(path, '.');
    if (extension == NULL || extension[1] == '\0' || extension[2] == '\0' ||
        extension[3] == '\0' || extension[4] != '\0') {
        return 0;
    }
    return (extension[1] == 'e' || extension[1] == 'E') &&
           (extension[2] == 'x' || extension[2] == 'X') &&
           (extension[3] == 'e' || extension[3] == 'E');
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
        snapshot.device[port] = SCHWAY_INPUT_DEVICE_DIGITAL;
        for (size_t i = 0u; i < sizeof mapping / sizeof mapping[0]; i++) {
            if (input_state_cb != NULL &&
                input_state_cb(port, RETRO_DEVICE_JOYPAD, 0u, mapping[i].id) != 0) {
                snapshot.buttons[port] |= mapping[i].mask;
            }
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
    file = fopen(path, "rb");
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
}

/* L8866: release everything and reset every static; the module may be
   initialised again. */
void retro_deinit(void)
{
    lr_unload();
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
    sys->valid_extensions = "exe";
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
    (void)port;
    (void)device;
}

/* L8939: a power cycle, the only reset this release has. */
void retro_reset(void)
{
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

    if (input_poll_cb != NULL) {
        input_poll_cb();
    }
    if (inst == NULL || !lr_check_region()) {
        return;
    }
    lr_queue_input();
    if (schway_run_frame(inst, NULL) != SCHWAY_STATUS_OK) {
        return;
    }
    memset(&video, 0, sizeof video);
    video.size = (uint32_t)sizeof video;
    if (schway_get_video(inst, &video) != SCHWAY_STATUS_OK || video.bpp != 16u ||
        video.width > LR_MAX_W || video.height > LR_MAX_H_PAL) {
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
    schway_host_xrgb8888_frame(&video, pixels, video.width);
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

/* L9085: no memory regions are exposed in this release. */
void *retro_get_memory_data(unsigned id)
{
    (void)id;
    return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
    (void)id;
    return 0u;
}
