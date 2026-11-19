/* Runner-side save identity resolution; no filesystem access is performed here. */
#ifndef SCHWAYSTATION_HOST_SAVE_GAME_IDENTITY_H
#define SCHWAYSTATION_HOST_SAVE_GAME_IDENTITY_H

#include <stddef.h>
#include <stdint.h>

enum { SCHWAY__SAVE_GAME_IDENTITY_CAPACITY = 70u };

typedef enum schway__save_game_identity_kind {
    SCHWAY__SAVE_GAME_IDENTITY_LITERAL = 1,
    SCHWAY__SAVE_GAME_IDENTITY_SERIAL = 2,
    SCHWAY__SAVE_GAME_IDENTITY_EXECUTABLE_DIGEST = 3
} schway__save_game_identity_kind;

typedef enum schway__save_game_identity_status {
    SCHWAY__SAVE_GAME_IDENTITY_OK = 0,
    SCHWAY__SAVE_GAME_IDENTITY_ERR_ARGUMENT = 1,
    SCHWAY__SAVE_GAME_IDENTITY_NO_SOURCE = 2
} schway__save_game_identity_status;

typedef struct schway__save_game_identity {
    schway__save_game_identity_kind kind;
    char value[SCHWAY__SAVE_GAME_IDENTITY_CAPACITY];
} schway__save_game_identity;

schway__save_game_identity_status schway__save_game_identity_resolve(
    const char *literal_override, const uint8_t *exe_bytes, size_t exe_size,
    const char *source_path, schway__save_game_identity *out);

#endif /* SCHWAYSTATION_HOST_SAVE_GAME_IDENTITY_H */
