/* Runner-side PS-EXE save identity selection. */
#include "save_game_identity.h"

#include <string.h>

#include "cards.h"
#include "../runner/sha256.h"

static unsigned char ascii_upper(unsigned char value)
{
    if (value >= 'a' && value <= 'z') {
        return (unsigned char)(value - ('a' - 'A'));
    }
    return value;
}

static int ascii_equal(const char *left, const char *right, size_t length)
{
    for (size_t i = 0u; i < length; i++) {
        if (ascii_upper((unsigned char)left[i]) != ascii_upper((unsigned char)right[i])) {
            return 0;
        }
    }
    return 1;
}

static int serial_component(const char *path, char out[SCHWAY__SAVE_GAME_IDENTITY_CAPACITY])
{
    if (path == NULL || path[0] == '\0') {
        return 0;
    }
    const char *name = path;
    for (const char *at = path; *at != '\0'; at++) {
        if (*at == '/' || *at == '\\') {
            name = at + 1;
        }
    }
    size_t length = strlen(name);
    size_t stem_size = length;
    if (length >= 4u && name[length - 4u] == '.' &&
        ascii_equal(name + length - 4u, ".exe", 4u)) {
        stem_size -= 4u;
    }

    size_t prefix_size;
    if (stem_size == 11u) {
        prefix_size = 4u;
    } else if (stem_size == 10u && ascii_equal(name, "LSP", 3u)) {
        prefix_size = 3u;
    } else {
        return 0;
    }
    size_t separator = prefix_size;
    size_t digits = separator + 1u;
    size_t decimal = digits + 3u;
    if (name[separator] != '_' && name[separator] != '-') {
        return 0;
    }
    if (name[decimal] != '.') {
        return 0;
    }
    for (size_t i = 0u; i < prefix_size; i++) {
        unsigned char value = (unsigned char)name[i];
        if (!((value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z'))) {
            return 0;
        }
        out[i] = (char)ascii_upper(value);
    }
    for (size_t i = 0u; i < 3u; i++) {
        unsigned char value = (unsigned char)name[digits + i];
        if (value < '0' || value > '9') {
            return 0;
        }
        out[prefix_size + 1u + i] = (char)value;
    }
    for (size_t i = 0u; i < 2u; i++) {
        unsigned char value = (unsigned char)name[decimal + 1u + i];
        if (value < '0' || value > '9') {
            return 0;
        }
        out[prefix_size + 4u + i] = (char)value;
    }
    out[prefix_size] = '-';
    out[prefix_size + 6u] = '\0';
    return 1;
}

static int literal_component_is_valid(const char *value)
{
    char slot1[80];
    char slot2[80];
    size_t length = strlen(value);
    if (length == 0u || length > 63u) {
        return 0;
    }
    return schway_host_card_paths_for_basename(".", value, slot1, sizeof slot1,
                                               slot2, sizeof slot2) == SCHWAY_HOST_CARD_OK;
}

static void executable_digest(const uint8_t *bytes, size_t size, char out[70])
{
    static const uint8_t domain[] = "schwaystation-save-game-identity";
    const uint8_t version = 1u;
    const uint8_t media_ps_exe = 1u;
    uint64_t length = (uint64_t)size;
    uint8_t framed_length[8];
    for (size_t i = 0u; i < sizeof framed_length; i++) {
        framed_length[i] = (uint8_t)(length >> (i * 8u));
    }

    schway__sha256 hash;
    uint8_t digest[32];
    char hex[65];
    schway__sha256_init(&hash);
    schway__sha256_update(&hash, domain, sizeof domain - 1u);
    schway__sha256_update(&hash, &version, sizeof version);
    schway__sha256_update(&hash, &media_ps_exe, sizeof media_ps_exe);
    schway__sha256_update(&hash, framed_length, sizeof framed_length);
    schway__sha256_update(&hash, bytes, size);
    schway__sha256_final(&hash, digest);
    schway__sha256_hex(digest, hex);
    memcpy(out, "sgi1-", 5u);
    memcpy(out + 5u, hex, sizeof hex);
}

schway__save_game_identity_status schway__save_game_identity_resolve(
    const char *literal_override, const uint8_t *exe_bytes, size_t exe_size,
    const char *source_path, schway__save_game_identity *out)
{
    if (out == NULL) {
        return SCHWAY__SAVE_GAME_IDENTITY_ERR_ARGUMENT;
    }

    schway__save_game_identity resolved;
    memset(&resolved, 0, sizeof resolved);
    if (literal_override != NULL) {
        if (!literal_component_is_valid(literal_override)) {
            return SCHWAY__SAVE_GAME_IDENTITY_ERR_ARGUMENT;
        }
        resolved.kind = SCHWAY__SAVE_GAME_IDENTITY_LITERAL;
        size_t length = strlen(literal_override);
        memcpy(resolved.value, literal_override, length + 1u);
        *out = resolved;
        return SCHWAY__SAVE_GAME_IDENTITY_OK;
    }

    if (exe_bytes == NULL || exe_size == 0u || source_path == NULL || source_path[0] == '\0') {
        return SCHWAY__SAVE_GAME_IDENTITY_NO_SOURCE;
    }
    if (serial_component(source_path, resolved.value)) {
        resolved.kind = SCHWAY__SAVE_GAME_IDENTITY_SERIAL;
        *out = resolved;
        return SCHWAY__SAVE_GAME_IDENTITY_OK;
    }

    uint64_t length = (uint64_t)exe_size;
    if ((size_t)length != exe_size) {
        return SCHWAY__SAVE_GAME_IDENTITY_ERR_ARGUMENT;
    }
    resolved.kind = SCHWAY__SAVE_GAME_IDENTITY_EXECUTABLE_DIGEST;
    executable_digest(exe_bytes, exe_size, resolved.value);
    *out = resolved;
    return SCHWAY__SAVE_GAME_IDENTITY_OK;
}
