/* The libretro adapter: the 25 entry points of libretro.h, written as a thin
 * client of schwaystation.h. "Lnnn" is a line of the vendored libretro.h
 * (libretro-common, pinned in PROVENANCE.md).
 *
 * This release loads no content: retro_load_game(NULL) boots the built-in
 * test card and any content is refused. Audio is the core's own silent pairs,
 * exactly as many per frame as its clock gives, so a frontend that paces on
 * audio runs at the core's rate. The av_info divisions are the only floating
 * point in the shipped code; the core itself is integer only. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
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
    {NULL, NULL, NULL, NULL, NULL, NULL, {{NULL, NULL}}, NULL},
};
static struct retro_core_option_v2_category lr_option_cats[] = {{NULL, NULL, NULL}};
static struct retro_core_options_v2 lr_options = {lr_option_cats, lr_option_defs};
/* The same option for a frontend without core options version 2 (L1021). */
static struct retro_variable lr_variables[] = {
    {LR_REGION_KEY, "Region; ntsc-u|ntsc-j|pal"},
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
    return 1;
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

/* L8886: static strings. No content loads yet, so no extension is listed. A
   disc image is opened by path and never extracted from an archive
   (HOSTS-AND-INTEGRATION section 1, Content). */
void retro_get_system_info(struct retro_system_info *sys)
{
    if (sys == NULL) {
        return;
    }
    memset(sys, 0, sizeof *sys);
    sys->library_name = "schwaystation";
    sys->library_version = SCHWAYSTATION_LIBRETRO_VERSION;
    sys->valid_extensions = "";
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
    if (inst == NULL || !lr_check_region() || schway_run_frame(inst, NULL) != SCHWAY_STATUS_OK) {
        return;
    }
    memset(&video, 0, sizeof video);
    video.size = (uint32_t)sizeof video;
    if (schway_get_video(inst, &video) != SCHWAY_STATUS_OK || video.bpp != 16u ||
        video.width > LR_MAX_W || video.height > LR_MAX_H_PAL) {
        return;
    }
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

/* L9020. A NULL game boots the test card; content is refused in this
   release. XRGB8888 is requested here (L870); the default 0RGB1555 is
   deprecated. The region option is read here, before the first frame. */
bool retro_load_game(const struct retro_game_info *game)
{
    enum retro_pixel_format format = RETRO_PIXEL_FORMAT_XRGB8888;

    lr_unload();
    if (game != NULL) {
        return false;
    }
    if (env_cb == NULL || !env_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &format)) {
        return false;
    }
    return lr_create(lr_option_region()) != 0;
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
