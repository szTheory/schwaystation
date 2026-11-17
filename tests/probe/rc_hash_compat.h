/* Minimal declarations for the pinned rc_hash_disc.c test oracle. */
#ifndef SCHWAY_TEST_RC_HASH_COMPAT_H
#define SCHWAY_TEST_RC_HASH_COMPAT_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../../runner/md5.h"

struct rc_hash_iterator;
typedef void *(*rc_hash_cdreader_open_track_handler)(const char *, uint32_t);
typedef size_t (*rc_hash_cdreader_read_sector_handler)(void *, uint32_t, void *, size_t);
typedef void (*rc_hash_cdreader_close_track_handler)(void *);
typedef uint32_t (*rc_hash_cdreader_first_track_sector_handler)(void *);
typedef void *(*rc_hash_cdreader_open_track_iterator_handler)(
    const char *, uint32_t, const struct rc_hash_iterator *);
typedef void (*rc_hash_message_callback_func)(const char *, const struct rc_hash_iterator *);

typedef struct rc_hash_cdreader {
    rc_hash_cdreader_open_track_handler open_track;
    rc_hash_cdreader_read_sector_handler read_sector;
    rc_hash_cdreader_close_track_handler close_track;
    rc_hash_cdreader_first_track_sector_handler first_track_sector;
    rc_hash_cdreader_open_track_iterator_handler open_track_iterator;
} rc_hash_cdreader_t;

typedef struct rc_hash_callbacks {
    rc_hash_message_callback_func verbose_message;
    rc_hash_message_callback_func error_message;
    rc_hash_cdreader_t cdreader;
} rc_hash_callbacks_t;

typedef struct rc_hash_iterator {
    const char *path;
    rc_hash_callbacks_t callbacks;
} rc_hash_iterator_t;

#define RC_HASH_CDTRACK_FIRST_DATA ((uint32_t)-1)
#define RC_HASH_CDTRACK_LAST ((uint32_t)-2)
#define RC_HASH_CDTRACK_LARGEST ((uint32_t)-3)
#define RC_HASH_CDTRACK_FIRST_OF_SECOND_SESSION ((uint32_t)-4)
#define MAX_BUFFER_SIZE (64u * 1024u * 1024u)

typedef uint8_t md5_byte_t;
typedef schway__md5 md5_state_t;
static inline void md5_init(md5_state_t *state) { schway__md5_init(state); }
static inline void md5_append(md5_state_t *state, const md5_byte_t *data, int size)
{
    if (size > 0) schway__md5_update(state, data, (size_t)size);
}
static inline void md5_finish(md5_state_t *state, md5_byte_t digest[16])
{
    schway__md5_final(state, digest);
}

void rc_hash_get_default_cdreader(struct rc_hash_cdreader *reader);
void rc_hash_iterator_verbose(const rc_hash_iterator_t *iterator, const char *message);
void rc_hash_iterator_verbose_formatted(const rc_hash_iterator_t *iterator,
                                       const char *format, ...);
int rc_hash_iterator_error(const rc_hash_iterator_t *iterator, const char *message);
int rc_hash_iterator_error_formatted(const rc_hash_iterator_t *iterator,
                                     const char *format, ...);
int rc_hash_finalize(const rc_hash_iterator_t *iterator, md5_state_t *md5, char hash[33]);
int rc_hash_buffer(char hash[33], const uint8_t *buffer, size_t size,
                   const rc_hash_iterator_t *iterator);
void rc_hash_byteswap16(uint8_t *buffer, const uint8_t *stop);
void rc_hash_byteswap32(uint8_t *buffer, const uint8_t *stop);
void *rc_file_open(const rc_hash_iterator_t *iterator, const char *path);
void rc_file_seek(const rc_hash_iterator_t *iterator, void *file, int64_t offset, int origin);
int64_t rc_file_tell(const rc_hash_iterator_t *iterator, void *file);
size_t rc_file_read(const rc_hash_iterator_t *iterator, void *file, void *buffer, int size);
void rc_file_close(const rc_hash_iterator_t *iterator, void *file);
int64_t rc_file_size(const rc_hash_iterator_t *iterator, const char *path);
int rc_hash_whole_file(char hash[33], const rc_hash_iterator_t *iterator);
const char *rc_path_get_filename(const char *path);
int rc_path_compare_extension(const char *path, const char *extension);
int rc_hash_psx(char hash[33], const rc_hash_iterator_t *iterator);

#endif
