#include "disc_image.h"
#include "disc_probe.h"
#include "save_game_identity.h"
#include "sha256.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../test_open.h"

#define LEGACY_SECTOR_BYTES 2352u
#define LEGACY_SUB_BYTES 96u
#define LEGACY_FILE_LIMIT (8u * 1024u * 1024u)

static int make_path(char path[1024], const char *directory, const char *name)
{
    int length = snprintf(path, 1024u, "%s/%s", directory, name);
    return length >= 0 && (size_t)length < 1024u;
}

static char *copy_text(const char *text)
{
    size_t size = strlen(text) + 1u;
    char *copy = (char *)malloc(size);
    if (copy != NULL) memcpy(copy, text, size);
    return copy;
}

static int contains_text(const uint8_t *bytes, size_t size, const char *needle)
{
    size_t needle_size = strlen(needle);
    if (needle_size > size) return 0;
    for (size_t i = 0u; i <= size - needle_size; i++) {
        if (memcmp(bytes + i, needle, needle_size) == 0) return 1;
    }
    return 0;
}

static int read_bytes(const char *path, uint8_t **bytes, size_t *size)
{
    FILE *file = schway__test_open_file(path, "rb");
    long end;
    uint8_t *data;
    if (file == NULL) return 0;
    if (fseek(file, 0L, SEEK_END) != 0 || (end = ftell(file)) < 0L ||
        (unsigned long)end > LEGACY_FILE_LIMIT || fseek(file, 0L, SEEK_SET) != 0) {
        (void)fclose(file);
        return 0;
    }
    data = (uint8_t *)malloc(end == 0L ? 1u : (size_t)end);
    if (data == NULL || (end != 0L && fread(data, 1u, (size_t)end, file) != (size_t)end)) {
        free(data);
        (void)fclose(file);
        return 0;
    }
    if (fclose(file) != 0) {
        free(data);
        return 0;
    }
    *bytes = data;
    *size = (size_t)end;
    return 1;
}

static int write_bytes(const char *path, const uint8_t *bytes, size_t size)
{
    FILE *file = schway__test_open_file(path, "wb");
    int ok;
    if (file == NULL) return 0;
    ok = size == 0u || fwrite(bytes, 1u, size, file) == size;
    return fclose(file) == 0 && ok;
}

static void hash_hex(const uint8_t *bytes, size_t size, char hex[65])
{
    schway__sha256 state;
    uint8_t digest[32];
    schway__sha256_init(&state);
    schway__sha256_update(&state, bytes, size);
    schway__sha256_final(&state, digest);
    schway__sha256_hex(digest, hex);
}

static int field_text(const char *manifest, const char *key,
                      char *value, size_t capacity)
{
    char prefix[96];
    int length = snprintf(prefix, sizeof prefix, "%s=", key);
    const char *line = manifest;
    if (length < 0 || (size_t)length >= sizeof prefix) return 0;
    while (line != NULL && *line != '\0') {
        const char *end = strchr(line, '\n');
        size_t line_size = end != NULL ? (size_t)(end - line) : strlen(line);
        if (line_size >= (size_t)length && memcmp(line, prefix, (size_t)length) == 0) {
            size_t value_size = line_size - (size_t)length;
            if (value_size >= capacity) return 0;
            memcpy(value, line + length, value_size);
            value[value_size] = '\0';
            return 1;
        }
        line = end != NULL ? end + 1 : NULL;
    }
    return 0;
}

static int field_number(const char *manifest, const char *key, uint64_t *value)
{
    char text[64];
    char *end = NULL;
    unsigned long long parsed;
    if (!field_text(manifest, key, text, sizeof text) || text[0] == '-') return 0;
    parsed = strtoull(text, &end, 10);
    if (end == text || *end != '\0') return 0;
    *value = (uint64_t)parsed;
    return 1;
}

static int field_equals(const char *manifest, const char *key, const char *expected)
{
    char value[256];
    return field_text(manifest, key, value, sizeof value) && strcmp(value, expected) == 0;
}

static int field_matches_hash(const char *manifest, const char *key,
                              const uint8_t *bytes, size_t size)
{
    char expected[65];
    char actual[65];
    if (!field_text(manifest, key, expected, sizeof expected)) return 0;
    hash_hex(bytes, size, actual);
    return strcmp(expected, actual) == 0;
}

static int read_manifest(const char *path, char **manifest)
{
    uint8_t *bytes = NULL;
    uint8_t *terminated;
    size_t size = 0u;
    if (!read_bytes(path, &bytes, &size) || size == 0u ||
        memchr(bytes, '\0', size) != NULL) {
        free(bytes);
        return 0;
    }
    terminated = (uint8_t *)realloc(bytes, size + 1u);
    if (terminated == NULL) {
        free(bytes);
        return 0;
    }
    terminated[size] = '\0';
    *manifest = (char *)terminated;
    return 1;
}

static int validate_ecm(const char *directory, const char *generator,
                        uint64_t requested, const char *manifest_override)
{
    char path[1024];
    char source_path[1024];
    char source_cue_path[1024];
    char literal_path[1024];
    char modeled_path[1024];
    char literal_cue_path[1024];
    char modeled_cue_path[1024];
    uint8_t *source = NULL, *source_cue = NULL, *literal = NULL, *modeled = NULL;
    uint8_t *literal_cue = NULL, *modeled_cue = NULL, *generator_bytes = NULL;
    size_t source_size = 0u, cue_size = 0u, literal_size = 0u, modeled_size = 0u;
    size_t literal_cue_size = 0u, modeled_cue_size = 0u, generator_size = 0u;
    char *manifest = NULL;
    uint64_t sectors, bytes, reads, outputs;
    int ok = 0;
    static const char expected_source_cue[] =
        "FILE \"disc_boot_pbp_source.bin\" BINARY\n"
        "  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n";
    static const char expected_literal_cue[] =
        "FILE \"disc_boot_legacy_literal.ecm\" BINARY\n"
        "  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n";
    static const char expected_modeled_cue[] =
        "FILE \"disc_boot_legacy_modeled.ecm\" BINARY\n"
        "  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n";

    if (!make_path(source_path, directory, "disc_boot_pbp_source.bin") ||
        !make_path(source_cue_path, directory, "disc_boot_legacy_source.cue") ||
        !make_path(literal_path, directory, "disc_boot_legacy_literal.ecm") ||
        !make_path(modeled_path, directory, "disc_boot_legacy_modeled.ecm") ||
        !make_path(literal_cue_path, directory, "disc_boot_legacy_literal.cue") ||
        !make_path(modeled_cue_path, directory, "disc_boot_legacy_modeled.cue") ||
        (manifest_override == NULL &&
         !make_path(path, directory, "disc_boot_legacy_ecm.manifest")) ||
        !read_bytes(source_path, &source, &source_size) ||
        !read_bytes(source_cue_path, &source_cue, &cue_size) ||
        !read_bytes(literal_path, &literal, &literal_size) ||
        !read_bytes(modeled_path, &modeled, &modeled_size) ||
        !read_bytes(literal_cue_path, &literal_cue, &literal_cue_size) ||
        !read_bytes(modeled_cue_path, &modeled_cue, &modeled_cue_size) ||
        !read_bytes(generator, &generator_bytes, &generator_size) ||
        !(manifest_override != NULL ? (manifest = copy_text(manifest_override)) != NULL :
          read_manifest(path, &manifest))) goto done;

    sectors = (uint64_t)(source_size / LEGACY_SECTOR_BYTES);
    bytes = (uint64_t)source_size;
    if (source_size == 0u || source_size % LEGACY_SECTOR_BYTES != 0u ||
        cue_size != sizeof expected_source_cue - 1u ||
        memcmp(source_cue, expected_source_cue, cue_size) != 0 ||
        literal_cue_size != sizeof expected_literal_cue - 1u ||
        memcmp(literal_cue, expected_literal_cue, literal_cue_size) != 0 ||
        modeled_cue_size != sizeof expected_modeled_cue - 1u ||
        memcmp(modeled_cue, expected_modeled_cue, modeled_cue_size) != 0 ||
        requested != sectors || !field_equals(manifest, "format", "schwaystation-legacy-ecm-v1") ||
        !field_equals(manifest, "source", "disc_boot_pbp_source.bin") ||
        !field_number(manifest, "source_bytes", &bytes) || bytes != source_size ||
        !field_number(manifest, "source_sectors", &bytes) || bytes != sectors ||
        !field_number(manifest, "guest_reads", &reads) || reads != sectors ||
        !field_number(manifest, "guest_outputs", &outputs) || outputs != sectors ||
        !field_number(manifest, "literal_bytes", &bytes) || bytes != literal_size ||
        !field_number(manifest, "modeled_bytes", &bytes) || bytes != modeled_size ||
        !field_matches_hash(manifest, "source_sha256", source, source_size) ||
        !field_matches_hash(manifest, "source_cue_sha256", source_cue, cue_size) ||
        !field_matches_hash(manifest, "literal_sha256", literal, literal_size) ||
        !field_matches_hash(manifest, "modeled_sha256", modeled, modeled_size) ||
        !field_matches_hash(manifest, "generator_sha256", generator_bytes, generator_size)) goto done;
    ok = 1;
done:
    free(source); free(source_cue); free(literal); free(modeled);
    free(literal_cue); free(modeled_cue); free(generator_bytes); free(manifest);
    return ok;
}

static int validate_ccd(const char *directory, const char *generator,
                        uint64_t requested, const char *manifest_override,
                        const char *image_override)
{
    char path[1024], cue_path[1024], image_cue_path[1024];
    char track1_path[1024], track2_path[1024];
    char image_path[1024], ccd_path[1024], sub_path[1024];
    uint8_t *cue = NULL, *image_cue = NULL, *track1 = NULL, *track2 = NULL, *image = NULL;
    uint8_t *ccd = NULL, *sub = NULL, *generator_bytes = NULL;
    size_t cue_size = 0u, image_cue_size = 0u;
    size_t track1_size = 0u, track2_size = 0u, image_size = 0u;
    size_t ccd_size = 0u, sub_size = 0u, generator_size = 0u;
    char *manifest = NULL;
    uint64_t track1_sectors, track2_sectors, sectors, value;
    int ok = 0;
    static const char expected_cue[] =
        "FILE \"disc_boot.bin\" BINARY\n"
        "  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n"
        "FILE \"disc_boot_track02.bin\" BINARY\n"
        "  TRACK 02 AUDIO\n    INDEX 00 00:00:00\n    INDEX 01 00:02:00\n";
    static const char expected_image_cue[] =
        "FILE \"disc_boot_legacy.img\" BINARY\n"
        "  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n"
        "  TRACK 02 AUDIO\n    INDEX 00 00:00:64\n    INDEX 01 00:02:64\n";

    if (!make_path(image_cue_path, directory, "disc_boot_legacy_image.cue") ||
        !read_bytes(image_cue_path, &image_cue, &image_cue_size)) {
        fprintf(stderr, "ASSERT disc.image.legacy_preflight: generated IMG CUE is missing\n");
        goto done;
    }
    if (!make_path(cue_path, directory, "disc_boot.cue") ||
        !make_path(track1_path, directory, "disc_boot.bin") ||
        !make_path(track2_path, directory, "disc_boot_track02.bin") ||
        !make_path(image_path, directory, "disc_boot_legacy.img") ||
        !make_path(ccd_path, directory, "disc_boot_legacy.ccd") ||
        !make_path(sub_path, directory, "disc_boot_legacy.sub") ||
        (manifest_override == NULL &&
         !make_path(path, directory, "disc_boot_legacy_ccd.manifest")) ||
        !read_bytes(cue_path, &cue, &cue_size) ||
        !read_bytes(track1_path, &track1, &track1_size) ||
        !read_bytes(track2_path, &track2, &track2_size) ||
        !read_bytes(image_override != NULL ? image_override : image_path, &image, &image_size) ||
        !read_bytes(ccd_path, &ccd, &ccd_size) || !read_bytes(sub_path, &sub, &sub_size) ||
        !read_bytes(generator, &generator_bytes, &generator_size) ||
        !(manifest_override != NULL ? (manifest = copy_text(manifest_override)) != NULL :
          read_manifest(path, &manifest))) goto done;

    track1_sectors = (uint64_t)(track1_size / LEGACY_SECTOR_BYTES);
    track2_sectors = (uint64_t)(track2_size / LEGACY_SECTOR_BYTES);
    sectors = track1_sectors + track2_sectors;
    if (track1_size == 0u || track2_size <= 150u * LEGACY_SECTOR_BYTES ||
        track1_size % LEGACY_SECTOR_BYTES != 0u || track2_size % LEGACY_SECTOR_BYTES != 0u ||
        image_size != track1_size + track2_size || cue_size != sizeof expected_cue - 1u ||
        memcmp(cue, expected_cue, cue_size) != 0 ||
        image_cue_size != sizeof expected_image_cue - 1u ||
        memcmp(image_cue, expected_image_cue, image_cue_size) != 0 ||
        memcmp(image, track1, track1_size) != 0 ||
        memcmp(image + track1_size, track2, track2_size) != 0 ||
        sub_size != sectors * LEGACY_SUB_BYTES || requested != sectors ||
        !field_equals(manifest, "format", "schwaystation-legacy-ccd-v1") ||
        !field_equals(manifest, "source_track1", "disc_boot.bin") ||
        !field_equals(manifest, "source_track2", "disc_boot_track02.bin") ||
        !field_number(manifest, "source_track1_bytes", &value) || value != track1_size ||
        !field_number(manifest, "source_track2_bytes", &value) || value != track2_size ||
        !field_number(manifest, "source_track1_sectors", &value) || value != track1_sectors ||
        !field_number(manifest, "source_track2_sectors", &value) || value != track2_sectors ||
        !field_number(manifest, "source_sectors", &value) || value != sectors ||
        !field_number(manifest, "source_bytes", &value) || value != image_size ||
        !field_number(manifest, "guest_reads", &value) || value != sectors ||
        !field_number(manifest, "guest_outputs", &value) || value != sectors ||
        !field_number(manifest, "img_bytes", &value) || value != image_size ||
        !field_number(manifest, "ccd_bytes", &value) || value != ccd_size ||
        !field_number(manifest, "sub_bytes", &value) || value != sub_size ||
        !field_number(manifest, "image_cue_bytes", &value) || value != image_cue_size ||
        !field_matches_hash(manifest, "source_cue_sha256", cue, cue_size) ||
        !field_matches_hash(manifest, "source_track1_sha256", track1, track1_size) ||
        !field_matches_hash(manifest, "source_track2_sha256", track2, track2_size) ||
        !field_matches_hash(manifest, "img_sha256", image, image_size) ||
        !field_matches_hash(manifest, "ccd_sha256", ccd, ccd_size) ||
        !field_matches_hash(manifest, "sub_sha256", sub, sub_size) ||
        !field_matches_hash(manifest, "image_cue_sha256", image_cue, image_cue_size) ||
        !field_matches_hash(manifest, "generator_sha256", generator_bytes, generator_size)) goto done;
    if (!contains_text(ccd, ccd_size, "[CloneCD]\nVersion=3\n") ||
        !contains_text(ccd, ccd_size, "[TRACK 1]\nMODE=2\nINDEX 1=0\n") ||
        !contains_text(ccd, ccd_size, "[TRACK 2]\nMODE=0\nINDEX 0=64\nINDEX 1=214\n")) goto done;
    for (uint64_t lba = 0u; lba < sectors; lba++) {
        for (uint32_t channel = 0u; channel < 8u; channel++) {
            for (uint32_t offset = 0u; offset < 12u; offset++) {
                size_t index = (size_t)lba * LEGACY_SUB_BYTES +
                               (size_t)channel * 12u + offset;
                uint8_t expected = (uint8_t)(0x35u + (uint32_t)lba * 13u +
                                              channel * 29u + offset * 7u);
                if (sub[index] != expected) goto done;
            }
        }
    }
    ok = 1;
done:
    free(cue); free(image_cue); free(track1); free(track2); free(image); free(ccd); free(sub);
    free(generator_bytes); free(manifest);
    return ok;
}

static int replace_manifest_field(const char *source, const char *key,
                                  const char *replacement, char *out, size_t capacity)
{
    char prefix[96];
    int length = snprintf(prefix, sizeof prefix, "%s=", key);
    const char *start;
    const char *end;
    size_t head, tail, replacement_size = strlen(replacement);
    if (length < 0 || (size_t)length >= sizeof prefix) return 0;
    start = strstr(source, prefix);
    if (start == NULL || (start != source && start[-1] != '\n')) return 0;
    end = strchr(start, '\n');
    if (end == NULL) return 0;
    head = (size_t)(start - source) + (size_t)length;
    tail = strlen(end);
    if (head + replacement_size + tail + 1u > capacity) return 0;
    memcpy(out, source, head);
    memcpy(out + head, replacement, replacement_size);
    memcpy(out + head + replacement_size, end, tail + 1u);
    return 1;
}

static int copy_with_extra_byte(const char *source, const char *destination)
{
    uint8_t *bytes = NULL;
    uint8_t *grown;
    size_t size = 0u;
    int ok;
    if (!read_bytes(source, &bytes, &size)) return 0;
    grown = (uint8_t *)realloc(bytes, size + 1u);
    if (grown == NULL) {
        free(bytes);
        return 0;
    }
    bytes = grown;
    bytes[size] = 0u;
    ok = write_bytes(destination, bytes, size + 1u);
    free(bytes);
    return ok;
}

static int run_preflight_controls(const char *directory, const char *test_directory,
                                  const char *ecm_generator, const char *ccd_generator,
                                  uint64_t ecm_sectors, uint64_t ccd_sectors)
{
    char path[1024], image_path[1024], ccd_manifest_path[1024], ecm_manifest_path[1024];
    char missing_ccd[1024], extra_img[1024];
    char *ecm_manifest = NULL, *ccd_manifest = NULL;
    char bad_count[4096], bad_digest[4096];
    uint8_t *ccd_bytes = NULL;
    size_t ccd_size = 0u;
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    int ok = 0;

    if (!make_path(ecm_manifest_path, directory, "disc_boot_legacy_ecm.manifest") ||
        !make_path(ccd_manifest_path, directory, "disc_boot_legacy_ccd.manifest") ||
        !make_path(path, directory, "disc_boot_legacy.ccd") ||
        !make_path(image_path, directory, "disc_boot_legacy.img") ||
        !make_path(missing_ccd, test_directory, "legacy-missing.ccd") ||
        !make_path(extra_img, test_directory, "legacy-extent.img") ||
        !read_manifest(ecm_manifest_path, &ecm_manifest) ||
        !read_manifest(ccd_manifest_path, &ccd_manifest) ||
        !read_bytes(path, &ccd_bytes, &ccd_size)) goto done;

    if (!replace_manifest_field(ccd_manifest, "source_sectors", "223", bad_count, sizeof bad_count) ||
        validate_ccd(directory, ccd_generator, ccd_sectors, bad_count, NULL)) goto done;
    if (!replace_manifest_field(ecm_manifest, "generator_sha256",
            "0000000000000000000000000000000000000000000000000000000000000000",
            bad_digest, sizeof bad_digest) ||
        validate_ecm(directory, ecm_generator, ecm_sectors, bad_digest)) goto done;
    if (validate_ecm(directory, ecm_generator, ecm_sectors - 1u, NULL)) goto done;
    if (!copy_with_extra_byte(image_path, extra_img) ||
        validate_ccd(directory, ccd_generator, ccd_sectors, NULL, extra_img)) goto done;
    if (!write_bytes(missing_ccd, ccd_bytes, ccd_size) ||
        schway_disc_image_open(missing_ccd, &image, &error) == SCHWAY_DISC_IMAGE_OK ||
        image != NULL) goto done;
    ok = 1;
done:
    free(ecm_manifest); free(ccd_manifest); free(ccd_bytes);
    schway_disc_image_close(image);
    return ok;
}

static int toc_matches(const schway_disc *disc, uint32_t track_count,
                       uint32_t total_sectors, int mixed)
{
    schway_toc toc;
    memset(&toc, 0, sizeof toc);
    toc.size = (uint32_t)sizeof toc;
    if (disc == NULL || disc->toc == NULL || disc->toc(disc->user, &toc) != 1u ||
        toc.track_count != track_count) return 0;
    if (!mixed) {
        return toc.tracks[0].type == SCHWAY_DISC_TRACK_MODE2 &&
               toc.tracks[0].index0 == 0u && toc.tracks[0].index1 == 0u &&
               toc.tracks[0].pregap == 0u && toc.tracks[0].leadout == total_sectors;
    }
    return toc.tracks[0].type == SCHWAY_DISC_TRACK_MODE2 &&
           toc.tracks[0].index0 == 0u && toc.tracks[0].index1 == 0u &&
           toc.tracks[0].leadout == 64u &&
           toc.tracks[1].type == SCHWAY_DISC_TRACK_AUDIO &&
           toc.tracks[1].index0 == 64u && toc.tracks[1].index1 == 214u &&
           toc.tracks[1].pregap == 150u && toc.tracks[1].leadout == total_sectors;
}

static void expected_subcode(const uint8_t raw[LEGACY_SUB_BYTES],
                             uint8_t decoded[LEGACY_SUB_BYTES])
{
    for (size_t i = 0u; i < LEGACY_SUB_BYTES; i++) {
        uint8_t value = 0u;
        for (uint32_t channel = 0u; channel < 8u; channel++) {
            size_t index = (size_t)channel * 12u + i / 8u;
            uint32_t bit = 7u - (uint32_t)(i % 8u);
            value |= (uint8_t)(((raw[index] >> bit) & 1u) << (7u - channel));
        }
        decoded[i] = value;
    }
}

static int compare_provider(const char *label, const char *path, const uint8_t *source,
                            size_t sectors, const uint8_t *raw_sub,
                            int mixed)
{
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    const schway_disc *disc;
    uint32_t track_count = mixed ? 2u : 1u;
    int ok = 0;
    schway_disc_image_status status = schway_disc_image_open(path, &image, &error);
    if (status != SCHWAY_DISC_IMAGE_OK || image == NULL) {
        fprintf(stderr, "legacy parity: %s open failed (%u): %s\n",
                label, (unsigned int)status, error.message);
        goto done;
    }
    disc = schway_disc_image_disc(image);
    if (!toc_matches(disc, track_count, (uint32_t)sectors, mixed)) {
        fprintf(stderr, "legacy parity: %s TOC differs from %zu source sectors\n", label, sectors);
        goto done;
    }
    if (disc == NULL || disc->read_sector == NULL) {
        fprintf(stderr, "legacy parity: %s has no sector reader\n", label);
        goto done;
    }
    for (uint32_t pass = 0u; pass < 4u; pass++) {
        for (uint32_t i = 0u; i < (uint32_t)sectors; i++) {
            uint32_t lba = pass == 0u ? i : pass == 1u ? (uint32_t)sectors - i - 1u :
                           pass == 2u ? i % 7u : (i * 73u) % (uint32_t)sectors;
            uint8_t sector[LEGACY_SECTOR_BYTES];
            uint8_t subcode[LEGACY_SUB_BYTES];
            if (disc->read_sector(disc->user, lba, sector, subcode) != 1u ||
                memcmp(sector, source + (size_t)lba * LEGACY_SECTOR_BYTES,
                       sizeof sector) != 0) {
                const uint8_t *expected = source + (size_t)lba * LEGACY_SECTOR_BYTES;
                size_t byte = 0u;
                while (byte < sizeof sector && sector[byte] == expected[byte]) byte++;
                fprintf(stderr, "legacy parity: %s sector %u differs at byte %zu\n",
                        label, lba, byte);
                goto done;
            }
            if (mixed) {
                uint8_t expected[LEGACY_SUB_BYTES];
                expected_subcode(raw_sub + (size_t)lba * LEGACY_SUB_BYTES, expected);
                if (memcmp(subcode, expected, sizeof expected) != 0) {
                    size_t byte = 0u;
                    while (byte < sizeof expected && subcode[byte] == expected[byte]) byte++;
                    fprintf(stderr, "legacy parity: %s SUB sector %u differs at byte %zu\n",
                            label, lba, byte);
                    goto done;
                }
            } else {
                for (size_t b = 0u; b < sizeof subcode; b++) {
                    if (subcode[b] != 0u) {
                        fprintf(stderr, "legacy parity: %s unexpected SUB byte at sector %u offset %zu\n",
                                label, lba, b);
                        goto done;
                    }
                }
            }
        }
    }
    ok = 1;
done:
    schway_disc_image_close(image);
    return ok;
}

static int same_game_identity(const char *source_path, const char *encoded_path)
{
    schway_disc_image *source_image = NULL, *encoded_image = NULL;
    schway_disc_image_error error;
    schway__save_game_identity source_identity, encoded_identity;
    int ok = 0;
    schway_disc_image_status source_status = schway_disc_image_open(source_path, &source_image, &error);
    schway_disc_image_status encoded_status = schway_disc_image_open(encoded_path, &encoded_image, &error);
    if (source_status != SCHWAY_DISC_IMAGE_OK || encoded_status != SCHWAY_DISC_IMAGE_OK) {
        fprintf(stderr, "legacy parity: identity image open failed (%u, %u)\n",
                (unsigned int)source_status, (unsigned int)encoded_status);
        goto done;
    }
    schway__save_game_identity_status source_identity_status =
        schway__save_game_identity_resolve(NULL, NULL, 0u, source_path, source_image,
                                           &source_identity);
    schway__save_game_identity_status encoded_identity_status =
        schway__save_game_identity_resolve(NULL, NULL, 0u, encoded_path, encoded_image,
                                           &encoded_identity);
    if (source_identity_status != SCHWAY__SAVE_GAME_IDENTITY_OK ||
        encoded_identity_status != SCHWAY__SAVE_GAME_IDENTITY_OK) {
        fprintf(stderr, "legacy parity: save identity lookup failed (%u, %u)\n",
                (unsigned int)source_identity_status,
                (unsigned int)encoded_identity_status);
        goto done;
    }
    if (source_identity.kind != encoded_identity.kind ||
        strcmp(source_identity.value, encoded_identity.value) != 0) {
        fprintf(stderr, "legacy parity: save identity differs for %s (kind %u vs %u)\n",
                encoded_path, (unsigned int)source_identity.kind,
                (unsigned int)encoded_identity.kind);
        goto done;
    }
    ok = 1;
done:
    schway_disc_image_close(source_image);
    schway_disc_image_close(encoded_image);
    return ok;
}

static int same_content_identity(const char *source_path, const char *image_path)
{
    schway_host_disc_probe source_probe;
    schway_host_disc_probe image_probe;
    schway_host_disc_probe_status source_status;
    schway_host_disc_probe_status image_status;
    int ok = 0;
    memset(&source_probe, 0, sizeof source_probe);
    memset(&image_probe, 0, sizeof image_probe);
    source_status = schway__disc_probe_image(source_path, &source_probe);
    image_status = schway__disc_probe_image(image_path, &image_probe);
    if (source_status != SCHWAY_HOST_DISC_PROBE_OK ||
        image_status != SCHWAY_HOST_DISC_PROBE_OK) {
        fprintf(stderr, "legacy parity: content identity probe failed (%u, %u)\n",
                (unsigned int)source_status, (unsigned int)image_status);
        goto done;
    }
    if (strcmp(source_probe.serial, image_probe.serial) != 0 ||
        strcmp(source_probe.boot_name, image_probe.boot_name) != 0 ||
        memcmp(source_probe.track1_sha1, image_probe.track1_sha1,
               sizeof source_probe.track1_sha1) != 0 ||
        source_probe.exe_size != image_probe.exe_size ||
        memcmp(source_probe.exe_bytes, image_probe.exe_bytes, source_probe.exe_size) != 0) {
        fprintf(stderr, "legacy parity: source boot content identity differs\n");
        goto done;
    }
    ok = 1;
done:
    schway__disc_probe_dispose(&source_probe);
    schway__disc_probe_dispose(&image_probe);
    return ok;
}

static int run_parity(const char *directory)
{
    char source_cue[1024], literal[1024], modeled[1024];
    char source_ccd_cue[1024], ccd[1024], ccd_image_cue[1024];
    char path[1024];
    uint8_t *ecm_source = NULL, *ccd_source = NULL, *sub = NULL;
    size_t ecm_size = 0u, ccd_size = 0u, sub_size = 0u;
    int ok = 0;
    if (!make_path(source_cue, directory, "disc_boot_legacy_source.cue") ||
        !make_path(literal, directory, "disc_boot_legacy_literal.ecm") ||
        !make_path(modeled, directory, "disc_boot_legacy_modeled.ecm") ||
        !make_path(source_ccd_cue, directory, "disc_boot.cue") ||
        !make_path(ccd_image_cue, directory, "disc_boot_legacy_image.cue") ||
        !make_path(ccd, directory, "disc_boot_legacy.ccd") ||
        !make_path(path, directory, "disc_boot_pbp_source.bin") ||
        !read_bytes(path, &ecm_source, &ecm_size) ||
        !make_path(path, directory, "disc_boot_legacy.img") ||
        !read_bytes(path, &ccd_source, &ccd_size) ||
        !make_path(path, directory, "disc_boot_legacy.sub") ||
        !read_bytes(path, &sub, &sub_size)) goto done;
    if (ecm_size % LEGACY_SECTOR_BYTES != 0u ||
        ccd_size % LEGACY_SECTOR_BYTES != 0u ||
        sub_size != (ccd_size / LEGACY_SECTOR_BYTES) * LEGACY_SUB_BYTES ||
        !compare_provider("ECM literal", literal, ecm_source,
                          ecm_size / LEGACY_SECTOR_BYTES, NULL, 0) ||
        !compare_provider("ECM modeled", modeled, ecm_source,
                          ecm_size / LEGACY_SECTOR_BYTES, NULL, 0) ||
        !compare_provider("CloneCD", ccd, ccd_source,
                          ccd_size / LEGACY_SECTOR_BYTES, sub, 1) ||
        !same_game_identity(source_cue, literal) ||
        !same_game_identity(source_cue, modeled) ||
        !same_game_identity(source_ccd_cue, ccd_image_cue) ||
        !same_content_identity(source_ccd_cue, ccd_image_cue)) goto done;
    ok = 1;
done:
    free(ecm_source); free(ccd_source); free(sub);
    return ok;
}

int main(int argc, char **argv)
{
    if (argc == 8 && strcmp(argv[1], "--preflight") == 0) {
        uint64_t ecm_requested = (uint64_t)strtoull(argv[6], NULL, 10);
        uint64_t ccd_requested = (uint64_t)strtoull(argv[7], NULL, 10);
        if (!validate_ecm(argv[2], argv[4], ecm_requested, NULL) ||
            !validate_ccd(argv[2], argv[5], ccd_requested, NULL, NULL) ||
            !run_preflight_controls(argv[2], argv[3], argv[4], argv[5],
                                    ecm_requested, ccd_requested)) {
            fprintf(stderr, "ASSERT disc.image.legacy_preflight: source or encoded counts, manifest, companion, or requested extents failed\n");
            return 1;
        }
        return 0;
    }
    if (argc == 3 && strcmp(argv[1], "--parity") == 0) {
        if (!run_parity(argv[2])) {
            fprintf(stderr, "ASSERT disc.image.legacy_parity: source sectors, TOC, SUB bytes, or game identity differ\n");
            return 1;
        }
        return 0;
    }
    fprintf(stderr, "usage: schwaystation-test-disc-legacy --preflight <fixtures> <test-dir> <gen_ecm.py> <gen_ccd.py> <ecm-sectors> <ccd-sectors> | --parity <fixtures>\n");
    return 2;
}
