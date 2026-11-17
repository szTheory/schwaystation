#include "disc_image.h"
#include "disc_probe.h"
#include "rc_hash_compat.h"
#include "schwaystation.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct disc_hash_context {
    const char *path;
    const schway_disc *disc;
    schway_toc toc;
    const uint8_t *exe_bytes;
    size_t exe_size;
    int mutate;
    int did_mutate;
} disc_hash_context;

typedef struct disc_hash_track {
    disc_hash_context *context;
    schway_disc_track track;
} disc_hash_track;

static disc_hash_context *active_context;

static void *disc_hash_open_track(const char *path, uint32_t track_number)
{
    disc_hash_track *track;
    if (active_context == NULL || strcmp(path, active_context->path) != 0 ||
        track_number != 1u || active_context->toc.track_count == 0u) return NULL;
    track = (disc_hash_track *)malloc(sizeof *track);
    if (track == NULL) return NULL;
    track->context = active_context;
    track->track = active_context->toc.tracks[0];
    return track;
}

static size_t disc_hash_read_sector(void *handle, uint32_t lba, void *buffer,
                                    size_t requested_bytes)
{
    disc_hash_track *track = (disc_hash_track *)handle;
    uint8_t raw[2352];
    size_t user_offset;
    if (track == NULL || buffer == NULL || requested_bytes > 2048u ||
        lba < track->track.index1 || lba >= track->track.leadout ||
        (track->track.type != SCHWAY_DISC_TRACK_MODE1 &&
         track->track.type != SCHWAY_DISC_TRACK_MODE2) ||
        track->context->disc->read_sector(track->context->disc->user, lba, raw, NULL) == 0u) {
        return 0u;
    }
    user_offset = track->track.type == SCHWAY_DISC_TRACK_MODE1 ? 16u : 24u;
    memcpy(buffer, raw + user_offset, requested_bytes);
    if (track->context->mutate && !track->context->did_mutate &&
        requested_bytes > 100u && track->context->exe_size > 100u &&
        memcmp(buffer, track->context->exe_bytes, 16u) == 0) {
        ((uint8_t *)buffer)[100] ^= 0x80u;
        track->context->did_mutate = 1;
    }
    return requested_bytes;
}

static void disc_hash_close_track(void *handle)
{
    free(handle);
}

static uint32_t disc_hash_first_track_sector(void *handle)
{
    const disc_hash_track *track = (const disc_hash_track *)handle;
    return track->track.index1;
}

void rc_hash_get_default_cdreader(struct rc_hash_cdreader *reader)
{
    memset(reader, 0, sizeof *reader);
}

void rc_hash_iterator_verbose(const rc_hash_iterator_t *iterator, const char *message)
{
    (void)iterator;
    (void)message;
}

void rc_hash_iterator_verbose_formatted(const rc_hash_iterator_t *iterator,
                                        const char *format, ...)
{
    (void)iterator;
    (void)format;
}

int rc_hash_iterator_error(const rc_hash_iterator_t *iterator, const char *message)
{
    (void)iterator;
    fprintf(stderr, "rcheevos adapter: %s\n", message);
    return 0;
}

int rc_hash_iterator_error_formatted(const rc_hash_iterator_t *iterator,
                                     const char *format, ...)
{
    va_list args;
    (void)iterator;
    fputs("rcheevos adapter: ", stderr);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);
    return 0;
}

int rc_hash_finalize(const rc_hash_iterator_t *iterator, md5_state_t *md5, char hash[33])
{
    uint8_t digest[16];
    (void)iterator;
    md5_finish(md5, digest);
    schway__md5_hex(digest, hash);
    return 1;
}

int rc_hash_buffer(char hash[33], const uint8_t *buffer, size_t size,
                   const rc_hash_iterator_t *iterator)
{
    md5_state_t md5;
    md5_init(&md5);
    if (size > 0u) md5_append(&md5, buffer, (int)size);
    return rc_hash_finalize(iterator, &md5, hash);
}

void rc_hash_byteswap16(uint8_t *buffer, const uint8_t *stop)
{
    while (buffer + 1 < stop) {
        uint8_t byte = buffer[0];
        buffer[0] = buffer[1];
        buffer[1] = byte;
        buffer += 2;
    }
}

void rc_hash_byteswap32(uint8_t *buffer, const uint8_t *stop)
{
    while (buffer + 3 < stop) {
        uint8_t byte = buffer[0];
        buffer[0] = buffer[3];
        buffer[3] = byte;
        byte = buffer[1];
        buffer[1] = buffer[2];
        buffer[2] = byte;
        buffer += 4;
    }
}

void *rc_file_open(const rc_hash_iterator_t *iterator, const char *path)
{
    (void)iterator;
    (void)path;
    return NULL;
}

void rc_file_seek(const rc_hash_iterator_t *iterator, void *file, int64_t offset, int origin)
{
    (void)iterator;
    (void)file;
    (void)offset;
    (void)origin;
}

int64_t rc_file_tell(const rc_hash_iterator_t *iterator, void *file)
{
    (void)iterator;
    (void)file;
    return -1;
}

size_t rc_file_read(const rc_hash_iterator_t *iterator, void *file, void *buffer, int size)
{
    (void)iterator;
    (void)file;
    (void)buffer;
    (void)size;
    return 0u;
}

void rc_file_close(const rc_hash_iterator_t *iterator, void *file)
{
    (void)iterator;
    (void)file;
}

int64_t rc_file_size(const rc_hash_iterator_t *iterator, const char *path)
{
    (void)iterator;
    (void)path;
    return -1;
}

int rc_hash_whole_file(char hash[33], const rc_hash_iterator_t *iterator)
{
    (void)hash;
    (void)iterator;
    return 0;
}

const char *rc_path_get_filename(const char *path)
{
    const char *filename = path;
    for (const char *cursor = path; *cursor != '\0'; cursor++) {
        if (*cursor == '/' || *cursor == '\\') filename = cursor + 1;
    }
    return filename;
}

int rc_path_compare_extension(const char *path, const char *extension)
{
    const char *dot = strrchr(path, '.');
    if (dot == NULL) return 0;
    dot++;
    while (*dot != '\0' && *extension != '\0') {
        unsigned char left = (unsigned char)*dot++;
        unsigned char right = (unsigned char)*extension++;
        if (left >= (unsigned char)'A' && left <= (unsigned char)'Z') {
            left = (unsigned char)(left + ((unsigned char)'a' - (unsigned char)'A'));
        }
        if (right >= (unsigned char)'A' && right <= (unsigned char)'Z') {
            right = (unsigned char)(right + ((unsigned char)'a' - (unsigned char)'A'));
        }
        if (left != right) return 0;
    }
    return *dot == '\0' && *extension == '\0';
}

static int hash_disc_image(schway_disc_image *image, const char *path,
                           const uint8_t *exe_bytes, size_t exe_size,
                           int mutate, char hash[33], int *did_mutate)
{
    disc_hash_context context;
    rc_hash_iterator_t iterator;
    memset(&context, 0, sizeof context);
    memset(&iterator, 0, sizeof iterator);
    context.path = path;
    context.disc = schway_disc_image_disc(image);
    context.toc.size = (uint32_t)sizeof context.toc;
    context.exe_bytes = exe_bytes;
    context.exe_size = exe_size;
    context.mutate = mutate;
    if (context.disc == NULL || context.disc->toc(context.disc->user, &context.toc) == 0u) return 0;
    iterator.path = path;
    iterator.callbacks.cdreader.open_track = disc_hash_open_track;
    iterator.callbacks.cdreader.read_sector = disc_hash_read_sector;
    iterator.callbacks.cdreader.close_track = disc_hash_close_track;
    iterator.callbacks.cdreader.first_track_sector = disc_hash_first_track_sector;
    active_context = &context;
    int result = rc_hash_psx(hash, &iterator);
    active_context = NULL;
    if (did_mutate != NULL) *did_mutate = context.did_mutate;
    return result;
}

int main(int argc, char **argv)
{
    schway_disc_image *image = NULL;
    schway_disc_image_error image_error;
    schway_host_disc_probe probe;
    schway_exe_info exe_info;
    uint8_t *changed_exe = NULL;
    char reference[33];
    char changed_reference[33];
    char production[33];
    char changed_production[33];
    const char *failure = "open image and probe";
    int did_mutate = 0;
    int status = 1;
    if (argc != 2) return 2;
    memset(&probe, 0, sizeof probe);
    memset(&image_error, 0, sizeof image_error);
    if (schway_disc_image_open(argv[1], &image, &image_error) != SCHWAY_DISC_IMAGE_OK ||
        schway__disc_probe_image(argv[1], &probe) != SCHWAY_HOST_DISC_PROBE_OK) goto done;
    failure = "validate boot identity";
    memset(&exe_info, 0, sizeof exe_info);
    exe_info.size = (uint32_t)sizeof exe_info;
    schway_status exe_status = schway_probe_exe(probe.exe_bytes, probe.exe_size, &exe_info);
    /* disc_boot's SYSTEM.CNF names a homebrew executable, not a retail serial. */
    if (strcmp(probe.boot_name, "SWAY.EXE") != 0 || probe.serial[0] != '\0' ||
        probe.exe_bytes == NULL ||
        exe_status != SCHWAY_STATUS_OK || exe_info.hash_len <= 100u ||
        exe_info.hash_len > probe.exe_size) {
        fprintf(stderr, "boot=%s serial=%s exe=%p size=%zu validation=%u hash_len=%u\n",
                probe.boot_name, probe.serial, (void *)probe.exe_bytes, probe.exe_size,
                (unsigned)exe_status, exe_info.hash_len);
        goto done;
    }
    failure = "hash original disc image";
    if (!hash_disc_image(image, argv[1], probe.exe_bytes, probe.exe_size, 0, reference, NULL)) goto done;
    failure = "compare original identities";
    schway__md5_disc_identity(probe.boot_name, probe.exe_bytes, exe_info.hash_len, production);
    if (strcmp(reference, production) != 0) goto done;
    failure = "prepare changed executable";
    changed_exe = (uint8_t *)malloc(probe.exe_size);
    if (changed_exe == NULL) goto done;
    memcpy(changed_exe, probe.exe_bytes, probe.exe_size);
    changed_exe[100] ^= 0x80u;
    schway__md5_disc_identity(probe.boot_name, changed_exe, exe_info.hash_len, changed_production);
    failure = "hash changed disc image";
    if (strcmp(changed_production, production) == 0 ||
        !hash_disc_image(image, argv[1], probe.exe_bytes, probe.exe_size, 1,
                         changed_reference, &did_mutate) || !did_mutate ||
        strcmp(changed_reference, reference) == 0 ||
        strcmp(changed_reference, changed_production) != 0) goto done;
    status = 0;
done:
    free(changed_exe);
    schway__disc_probe_dispose(&probe);
    schway_disc_image_close(image);
    if (status != 0) fprintf(stderr, "disc hash oracle mismatch at %s\n", failure);
    return status;
}
