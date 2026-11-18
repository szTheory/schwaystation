#include <SDL.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disc_image.h"
#include "xrgb8888.h"

#define EXAMPLE_BIOS_BYTES 524288u
#define EXAMPLE_WIDTH SCHWAY_HOST_VIDEO_MAX_WIDTH
#define EXAMPLE_HEIGHT SCHWAY_HOST_VIDEO_MAX_HEIGHT

enum {
    EXAMPLE_BUTTON_SELECT = 0x0001u,
    EXAMPLE_BUTTON_START = 0x0008u,
    EXAMPLE_BUTTON_UP = 0x0010u,
    EXAMPLE_BUTTON_RIGHT = 0x0020u,
    EXAMPLE_BUTTON_DOWN = 0x0040u,
    EXAMPLE_BUTTON_LEFT = 0x0080u,
    EXAMPLE_BUTTON_CROSS = 0x4000u,
    EXAMPLE_BUTTON_CIRCLE = 0x2000u,
    EXAMPLE_BUTTON_TRIANGLE = 0x8000u,
    EXAMPLE_BUTTON_SQUARE = 0x1000u
};

static void *example_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void example_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static int read_bios(const char *path, uint8_t **out)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) return 0;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return 0;
    }
    long length = ftell(file);
    if (length != (long)EXAMPLE_BIOS_BYTES || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return 0;
    }
    uint8_t *bytes = (uint8_t *)malloc(EXAMPLE_BIOS_BYTES);
    if (bytes == NULL) {
        fclose(file);
        return 0;
    }
    size_t read = fread(bytes, 1u, EXAMPLE_BIOS_BYTES, file);
    int ok = read == EXAMPLE_BIOS_BYTES && ferror(file) == 0;
    fclose(file);
    if (!ok) {
        free(bytes);
        return 0;
    }
    *out = bytes;
    return 1;
}

static void set_button(uint16_t *buttons, const Uint8 *keys, SDL_Scancode key,
                       uint16_t mask)
{
    if (keys[key] != 0u) *buttons = (uint16_t)(*buttons | mask);
}

static schway_status set_lid(schway *instance, uint32_t *open)
{
    uint32_t next = *open == 0u ? 1u : 0u;
    schway_status status = schway_set_lid(instance, next);
    if (status == SCHWAY_STATUS_OK) {
        *open = next;
        fprintf(stdout, "lid %s\n", next != 0u ? "open" : "closed");
    } else {
        fprintf(stderr, "cannot change lid: %s\n", schway_status_string(status));
    }
    return status;
}

static schway_status next_disc(schway *instance, schway_disc_image *image,
                               uint32_t *selected, uint32_t lid_open)
{
    uint32_t count = schway_disc_image_count(image);
    if (lid_open == 0u || count < 2u) {
        fprintf(stderr, "open the lid and load an M3U to swap discs\n");
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    uint32_t next = (*selected + 1u) % count;
    const schway_disc *disc = NULL;
    schway_disc_image_status image_status = schway_disc_image_select(image, next, &disc);
    if (image_status != SCHWAY_DISC_IMAGE_OK || disc == NULL) {
        fprintf(stderr, "cannot select playlist entry: %s\n",
                schway_disc_image_status_string(image_status));
        return SCHWAY_STATUS_ERR_ARGUMENT;
    }
    schway_status status = schway_swap_disc(instance, disc);
    if (status == SCHWAY_STATUS_OK) {
        *selected = next;
        fprintf(stdout, "disc %u: %s\n", next + 1u,
                schway_disc_image_label(image, next));
    } else {
        fprintf(stderr, "cannot swap disc: %s\n", schway_status_string(status));
    }
    return status;
}

static schway_input keyboard_input(const Uint8 *keys)
{
    schway_input input;
    memset(&input, 0, sizeof input);
    input.size = (uint32_t)sizeof input;
    input.device[0] = SCHWAY_INPUT_DEVICE_DIGITAL;
    input.axes[0][0] = 128u;
    input.axes[0][1] = 128u;
    input.axes[0][2] = 128u;
    input.axes[0][3] = 128u;
    set_button(&input.buttons[0], keys, SDL_SCANCODE_BACKSPACE, EXAMPLE_BUTTON_SELECT);
    set_button(&input.buttons[0], keys, SDL_SCANCODE_RETURN, EXAMPLE_BUTTON_START);
    set_button(&input.buttons[0], keys, SDL_SCANCODE_UP, EXAMPLE_BUTTON_UP);
    set_button(&input.buttons[0], keys, SDL_SCANCODE_RIGHT, EXAMPLE_BUTTON_RIGHT);
    set_button(&input.buttons[0], keys, SDL_SCANCODE_DOWN, EXAMPLE_BUTTON_DOWN);
    set_button(&input.buttons[0], keys, SDL_SCANCODE_LEFT, EXAMPLE_BUTTON_LEFT);
    set_button(&input.buttons[0], keys, SDL_SCANCODE_Z, EXAMPLE_BUTTON_CROSS);
    set_button(&input.buttons[0], keys, SDL_SCANCODE_X, EXAMPLE_BUTTON_CIRCLE);
    set_button(&input.buttons[0], keys, SDL_SCANCODE_S, EXAMPLE_BUTTON_TRIANGLE);
    set_button(&input.buttons[0], keys, SDL_SCANCODE_A, EXAMPLE_BUTTON_SQUARE);
    return input;
}

int main(int argc, char **argv)
{
    int result = 1;
    int sdl_started = 0;
    int running = 1;
    uint32_t lid_open = 0u;
    uint32_t selected = 0u;
    uint8_t *bios = NULL;
    uint32_t *pixels = NULL;
    schway *instance = NULL;
    schway_disc_image *image = NULL;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;
    SDL_AudioDeviceID audio_device = 0u;
    uint32_t texture_width = 0u;
    uint32_t texture_height = 0u;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <disc.cue|disc.iso|disc.m3u> <bios.bin>\n", argv[0]);
        goto cleanup;
    }
    if (!read_bios(argv[2], &bios)) {
        fprintf(stderr, "cannot read a %u-byte BIOS from %s\n", EXAMPLE_BIOS_BYTES, argv[2]);
        goto cleanup;
    }
    schway_disc_image_error image_error;
    memset(&image_error, 0, sizeof image_error);
    image_error.size = (uint32_t)sizeof image_error;
    schway_disc_image_status image_status = schway_disc_image_open(argv[1], &image, &image_error);
    if (image_status != SCHWAY_DISC_IMAGE_OK || image == NULL) {
        fprintf(stderr, "cannot open disc: %s\n", image_error.message);
        goto cleanup;
    }
    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = example_alloc;
    config.allocator.free = example_free;
    schway_status status = schway_create(&config, &instance);
    if (status != SCHWAY_STATUS_OK) {
        fprintf(stderr, "cannot create instance: %s\n", schway_status_string(status));
        goto cleanup;
    }
    schway_profile profile;
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = SCHWAY_REGION_NTSC_U;
    profile.disc_flags = SCHWAY_PROFILE_CD_LICENSED;
    status = schway_set_profile(instance, &profile);
    if (status == SCHWAY_STATUS_OK) status = schway_load_bios(instance, bios, EXAMPLE_BIOS_BYTES);
    free(bios);
    bios = NULL;
    if (status == SCHWAY_STATUS_OK) status = schway_open_disc(instance, schway_disc_image_disc(image));
    if (status != SCHWAY_STATUS_OK) {
        fprintf(stderr, "cannot initialize disc session: %s\n", schway_status_string(status));
        goto cleanup;
    }
    schway_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    if (schway_get_info(instance, &info) != SCHWAY_STATUS_OK || info.max_audio_frames == 0u ||
        info.fps_num == 0u ||
        info.sample_rate_den == 0u || info.sample_rate_num % info.sample_rate_den != 0u) {
        fprintf(stderr, "library returned unsupported audio timing\n");
        goto cleanup;
    }
    pixels = (uint32_t *)malloc((size_t)EXAMPLE_WIDTH * EXAMPLE_HEIGHT * sizeof *pixels);
    int16_t *samples = (int16_t *)malloc((size_t)info.max_audio_frames * 2u * sizeof *samples);
    if (pixels == NULL || samples == NULL) {
        fprintf(stderr, "not enough memory for one frame\n");
        free(samples);
        goto cleanup;
    }
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        free(samples);
        goto cleanup;
    }
    sdl_started = 1;
    window = SDL_CreateWindow("schwaystation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              960, 720, SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        fprintf(stderr, "SDL window failed: %s\n", SDL_GetError());
        free(samples);
        goto cleanup;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) renderer = SDL_CreateRenderer(window, -1, 0u);
    if (renderer == NULL) {
        fprintf(stderr, "SDL renderer failed: %s\n", SDL_GetError());
        free(samples);
        goto cleanup;
    }
    SDL_AudioSpec desired;
    SDL_zero(desired);
    desired.freq = (int)(info.sample_rate_num / info.sample_rate_den);
    desired.format = AUDIO_S16SYS;
    desired.channels = 2u;
    desired.samples = 1024u;
    audio_device = SDL_OpenAudioDevice(NULL, 0, &desired, NULL, 0u);
    if (audio_device == 0u) {
        fprintf(stderr, "SDL audio failed: %s\n", SDL_GetError());
        free(samples);
        goto cleanup;
    }
    SDL_PauseAudioDevice(audio_device, 0);

    fprintf(stdout, "F1 opens/closes the lid; F2 swaps to the next M3U entry while open.\n");
    while (running != 0) {
        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) running = 0;
            if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) running = 0;
                if (event.key.keysym.scancode == SDL_SCANCODE_F1) (void)set_lid(instance, &lid_open);
                if (event.key.keysym.scancode == SDL_SCANCODE_F2) {
                    (void)next_disc(instance, image, &selected, lid_open);
                }
            }
        }
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        schway_input input = keyboard_input(keys);
        status = schway_set_input(instance, &input);
        if (status == SCHWAY_STATUS_OK) status = schway_run_frame(instance, NULL);
        schway_video video;
        memset(&video, 0, sizeof video);
        video.size = (uint32_t)sizeof video;
        if (status == SCHWAY_STATUS_OK) status = schway_get_video(instance, &video);
        if (status != SCHWAY_STATUS_OK) {
            fprintf(stderr, "emulation failed: %s\n", schway_status_string(status));
            free(samples);
            goto cleanup;
        }
        if (video.enabled != 0u && video.width != 0u && video.height != 0u) {
            if (schway_host_xrgb8888_frame(&video, pixels, EXAMPLE_WIDTH) == 0) {
                fprintf(stderr, "library returned invalid video dimensions\n");
                free(samples);
                goto cleanup;
            }
            if (texture_width != video.width || texture_height != video.height) {
                SDL_DestroyTexture(texture);
                texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888,
                                            SDL_TEXTUREACCESS_STREAMING,
                                            (int)video.width, (int)video.height);
                if (texture == NULL) {
                    fprintf(stderr, "SDL texture failed: %s\n", SDL_GetError());
                    free(samples);
                    goto cleanup;
                }
                texture_width = video.width;
                texture_height = video.height;
                (void)SDL_RenderSetLogicalSize(renderer, (int)video.width, (int)video.height);
            }
            if (SDL_UpdateTexture(texture, NULL, pixels,
                                 (int)(EXAMPLE_WIDTH * sizeof *pixels)) != 0) {
                fprintf(stderr, "SDL texture update failed: %s\n", SDL_GetError());
                free(samples);
                goto cleanup;
            }
            (void)SDL_RenderClear(renderer);
            (void)SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }
        schway_audio audio;
        memset(&audio, 0, sizeof audio);
        audio.size = (uint32_t)sizeof audio;
        audio.samples = samples;
        audio.capacity = info.max_audio_frames;
        status = schway_get_audio(instance, &audio);
        if (status != SCHWAY_STATUS_OK || audio.written > audio.capacity ||
            (audio.written != 0u && SDL_QueueAudio(audio_device, samples,
                audio.written * 2u * (uint32_t)sizeof *samples) != 0)) {
            fprintf(stderr, "SDL audio queue failed: %s\n", SDL_GetError());
            free(samples);
            goto cleanup;
        }
        uint32_t frame_ms = (uint32_t)(((uint64_t)1000u * info.fps_den) / info.fps_num);
        SDL_Delay(frame_ms);
    }
    free(samples);
    result = 0;

cleanup:
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    if (audio_device != 0u) SDL_CloseAudioDevice(audio_device);
    if (sdl_started != 0) SDL_Quit();
    free(bios);
    free(pixels);
    schway_destroy(instance);
    schway_disc_image_close(image);
    return result;
}
