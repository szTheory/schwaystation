/* Runner-side save-game identity selection. */
#include "save_game_identity.h"

#include <string.h>

#include "cards.h"
#include "disc_image.h"
#include "disc_probe.h"
#include "../runner/sha256.h"

#ifdef SCHWAY_RUN_TEST_HOOKS
static uint64_t schway__save_game_identity_fail_after = UINT64_MAX;

void schway__save_game_identity_test_fail_after(uint32_t sectors)
{
    schway__save_game_identity_fail_after = sectors == UINT32_MAX ? UINT64_MAX : sectors;
}
#endif

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

static int serial_text_component(const char *name, size_t stem_size,
                                 char out[SCHWAY__SAVE_GAME_IDENTITY_CAPACITY])
{
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

static int serial_component(const char *path, char out[SCHWAY__SAVE_GAME_IDENTITY_CAPACITY])
{
    if (path == NULL || path[0] == '\0') return 0;
    const char *name = path;
    for (const char *at = path; *at != '\0'; at++) {
        if (*at == '/' || *at == '\\') name = at + 1;
    }
    size_t length = strlen(name);
    size_t stem_size = length;
    if (length >= 4u && name[length - 4u] == '.' &&
        ascii_equal(name + length - 4u, ".exe", 4u)) stem_size -= 4u;
    return serial_text_component(name, stem_size, out);
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

static int probe_serial_component(const char *serial,
                                  char out[SCHWAY__SAVE_GAME_IDENTITY_CAPACITY])
{
    char raw[12];
    if (serial == NULL) return 0;
    if (strlen(serial) != 10u || serial[4] != '-') return 0;
    memcpy(raw, serial, 4u);
    raw[4] = '_';
    memcpy(raw + 5u, serial + 5u, 3u);
    raw[8] = '.';
    memcpy(raw + 9u, serial + 8u, 2u);
    raw[11] = '\0';
    return serial_text_component(raw, 11u, out);
}

static void hash_u32_le(schway__sha256 *hash, uint32_t value)
{
    uint8_t bytes[4];
    for (size_t i = 0u; i < sizeof bytes; i++) {
        bytes[i] = (uint8_t)(value >> (i * 8u));
    }
    schway__sha256_update(hash, bytes, sizeof bytes);
}

static int path_has_m3u_extension(const char *path)
{
    size_t length;
    if (path == NULL) return 0;
    length = strlen(path);
    if (length < 4u || path[length - 4u] != '.') return 0;
    return ascii_upper((unsigned char)path[length - 3u]) == 'M' &&
           ascii_upper((unsigned char)path[length - 2u]) == '3' &&
           ascii_upper((unsigned char)path[length - 1u]) == 'U';
}

static schway__save_game_identity_status disc_digest(
    const schway_disc_image *image, int playlist,
    char out[SCHWAY__SAVE_GAME_IDENTITY_CAPACITY])
{
    static const uint8_t domain[] = "schwaystation-save-game-identity";
    const uint8_t version = 2u;
    uint32_t member_count = schway_disc_image_count(image);
    schway__sha256 hash;
    uint8_t digest[32];
    char hex[65];
    if (image == NULL || member_count == 0u ||
        member_count > SCHWAY_DISC_IMAGE_PLAYLIST_LIMIT) {
        return SCHWAY__SAVE_GAME_IDENTITY_ERR_ARGUMENT;
    }

    schway__sha256_init(&hash);
    schway__sha256_update(&hash, domain, sizeof domain - 1u);
    schway__sha256_update(&hash, &version, sizeof version);
    uint8_t media_type = playlist || member_count > 1u ? 3u : 2u;
    schway__sha256_update(&hash, &media_type, sizeof media_type);
    hash_u32_le(&hash, member_count);
    for (uint32_t member = 0u; member < member_count; member++) {
        const schway_disc *disc = NULL;
        schway_toc toc;
        if (schway_disc_image_select(image, member, &disc) != SCHWAY_DISC_IMAGE_OK ||
            disc == NULL || disc->toc == NULL || disc->read_sector == NULL) {
            return SCHWAY__SAVE_GAME_IDENTITY_ERR_IO;
        }
        memset(&toc, 0, sizeof toc);
        toc.size = (uint32_t)sizeof toc;
        if (disc->toc(disc->user, &toc) == 0u || toc.size != sizeof toc ||
            toc.track_count == 0u || toc.track_count > SCHWAY_DISC_MAX_TRACKS) {
            return SCHWAY__SAVE_GAME_IDENTITY_ERR_IO;
        }
        hash_u32_le(&hash, toc.track_count);
        for (uint32_t i = 0u; i < toc.track_count; i++) {
            const schway_disc_track *track = &toc.tracks[i];
            uint8_t sector[2352];
            uint8_t subcode[96];
            if (track->size != sizeof *track ||
                (track->type != SCHWAY_DISC_TRACK_AUDIO &&
                 track->type != SCHWAY_DISC_TRACK_MODE1 &&
                 track->type != SCHWAY_DISC_TRACK_MODE2) ||
                track->index0 > track->index1 || track->index1 > track->leadout) {
                return SCHWAY__SAVE_GAME_IDENTITY_ERR_IO;
            }
            hash_u32_le(&hash, track->type);
            hash_u32_le(&hash, track->index0);
            hash_u32_le(&hash, track->index1);
            hash_u32_le(&hash, track->pregap);
            hash_u32_le(&hash, track->leadout);
            for (uint32_t lba = track->index0; lba < track->leadout; lba++) {
#ifdef SCHWAY_RUN_TEST_HOOKS
                if (schway__save_game_identity_fail_after != UINT64_MAX &&
                    schway__save_game_identity_fail_after == 0u) {
                    return SCHWAY__SAVE_GAME_IDENTITY_ERR_IO;
                }
                if (schway__save_game_identity_fail_after != UINT64_MAX) {
                    schway__save_game_identity_fail_after--;
                }
#endif
                if (disc->read_sector(disc->user, lba, sector, subcode) == 0u) {
                    return SCHWAY__SAVE_GAME_IDENTITY_ERR_IO;
                }
                schway__sha256_update(&hash, sector, sizeof sector);
                schway__sha256_update(&hash, subcode, sizeof subcode);
            }
        }
    }
    schway__sha256_final(&hash, digest);
    schway__sha256_hex(digest, hex);
    memcpy(out, "sgi2-", 5u);
    memcpy(out + 5u, hex, sizeof hex);
    return SCHWAY__SAVE_GAME_IDENTITY_OK;
}

static schway__save_game_identity_status disc_identity(
    const char *source_path, const schway_disc_image *image,
    schway__save_game_identity *resolved)
{
    const char *first_path = schway_disc_image_path(image, 0u);
    schway_host_disc_probe probe;
    if (first_path == NULL) return SCHWAY__SAVE_GAME_IDENTITY_ERR_IO;
    memset(&probe, 0, sizeof probe);
    schway_host_disc_probe_status probe_status = schway__disc_probe_image(first_path, &probe);
    if (probe_status == SCHWAY_HOST_DISC_PROBE_OK) {
        if (probe_serial_component(probe.serial, resolved->value)) {
            resolved->kind = SCHWAY__SAVE_GAME_IDENTITY_DISC_SERIAL;
            schway__disc_probe_dispose(&probe);
            return SCHWAY__SAVE_GAME_IDENTITY_OK;
        }
        schway__disc_probe_dispose(&probe);
    } else if (probe_status == SCHWAY_HOST_DISC_PROBE_IO ||
               probe_status == SCHWAY_HOST_DISC_PROBE_CUE_IO) {
        return SCHWAY__SAVE_GAME_IDENTITY_ERR_IO;
    } else if (probe_status == SCHWAY_HOST_DISC_PROBE_OUT_OF_MEMORY) {
        return SCHWAY__SAVE_GAME_IDENTITY_ERR_MEMORY;
    }

    schway__save_game_identity_status status =
        disc_digest(image, path_has_m3u_extension(source_path), resolved->value);
    if (status == SCHWAY__SAVE_GAME_IDENTITY_OK) {
        resolved->kind = SCHWAY__SAVE_GAME_IDENTITY_DISC_DIGEST;
    }
    return status;
}

schway__save_game_identity_status schway__save_game_identity_resolve(
    const char *literal_override, const uint8_t *exe_bytes, size_t exe_size,
    const char *source_path, const schway_disc_image *disc_image,
    schway__save_game_identity *out)
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

    if (disc_image != NULL) {
        schway__save_game_identity_status status =
            disc_identity(source_path, disc_image, &resolved);
        if (status == SCHWAY__SAVE_GAME_IDENTITY_OK) *out = resolved;
        return status;
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
