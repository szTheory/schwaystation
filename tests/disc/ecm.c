#include "disc_ecm.h"
#include "disc_image.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../test_open.h"

static int make_path(char path[1024], const char *directory, const char *name)
{
    int length = snprintf(path, 1024u, "%s/%s", directory, name);
    return length >= 0 && (size_t)length < 1024u;
}

static int write_file(const char *path, const uint8_t *bytes, size_t size)
{
    FILE *file = schway__test_open_file(path, "wb");
    int ok;
    if (file == NULL) return 0;
    ok = size == 0u || fwrite(bytes, 1u, size, file) == size;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static int append_byte(const char *path, uint8_t byte)
{
    FILE *file = schway__test_open_file(path, "ab");
    int ok;
    if (file == NULL) return 0;
    ok = fwrite(&byte, 1u, 1u, file) == 1u;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static uint32_t ecm_edc_update(uint32_t value, const uint8_t *bytes, size_t size)
{
    for (size_t i = 0u; i < size; i++) {
        value ^= bytes[i];
        for (uint32_t bit = 0u; bit < 8u; bit++) {
            value = (value >> 1) ^ ((value & 1u) != 0u ? UINT32_C(0xd8018001) : 0u);
        }
    }
    return value;
}

static uint32_t ecm_edc(const uint8_t *bytes, size_t size)
{
    return ecm_edc_update(0u, bytes, size);
}

static int write_record_count(FILE *file, uint32_t type, uint32_t count);
static int write_literal_ecm(const char *path, const uint8_t *bytes, size_t size,
                             int bad_checksum);

static int write_count(FILE *file, uint32_t count)
{
    return write_record_count(file, 0u, count);
}

static int write_record_count(FILE *file, uint32_t type, uint32_t count)
{
    uint32_t encoded = count - 1u;
    uint8_t byte = (uint8_t)(((encoded & 0x1fu) << 2) | type);
    encoded >>= 5;
    if (encoded != 0u) byte |= 0x80u;
    if (fwrite(&byte, 1u, 1u, file) != 1u) return 0;
    while (encoded != 0u) {
        byte = (uint8_t)(encoded & 0x7fu);
        encoded >>= 7;
        if (encoded != 0u) byte |= 0x80u;
        if (fwrite(&byte, 1u, 1u, file) != 1u) return 0;
    }
    return 1;
}

static int hex_digit(char value)
{
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    return -1;
}

static int decode_hex(const char *hex, uint8_t *bytes, size_t size)
{
    if (hex == NULL || bytes == NULL || strlen(hex) != size * 2u) return 0;
    for (size_t i = 0u; i < size; i++) {
        int high = hex_digit(hex[i * 2u]);
        int low = hex_digit(hex[i * 2u + 1u]);
        if (high < 0 || low < 0) return 0;
        bytes[i] = (uint8_t)((high << 4) | low);
    }
    return 1;
}

/* Independent ECM v1.0 known answers; trailers were calculated from the
   source fields and format equations, not by the production reader. */
static const char ecm_type1_trailer_hex[] =
    "12e390d9000000000000000028e417f85697ec964681e3dfacca093861d2bafcff9ff5b8055690428e96089fd89517b9"
    "28b7a6b1a2ea48eb16d0adcfd2c8b10b674ea89ced8ca44f361237cb208f7ece6b7599f2732a5e54f00b68e5b0d93e46"
    "dc5fef289255a607ccf6d6b1035fdcbaa918b142ba9c2f6f5518750670421ec6283f286597d918274691329ae86b26e0"
    "cdaf0258b1ab377e487cdddc044fa6c257ab10bffe6e1be5b912e31a3f2db10ae7f3dd0d75f4d54fa04b0767df33bebe"
    "f558f1854586637f6231d2aad6651a67133b0f37b58d89212bd31f05a9de92fc3a1abfa46d0975dd66164c34577ee998"
    "23821579f44b4cbe64d6d8353a309d9de1f58dbe0f28211f70872336a5348bc4391fe739516b16d6edb1245ee44ba0a4";
static const char ecm_type2_trailer_hex[] =
    "e2c1f023fdf7e088418de65adde415fe5697ec964681e3dfacca093861d2bafcff9ff5b8055690428e96089fd89517b9"
    "28b7a6b1a2ea48eb16d0adcfd2c8b10b674ea89ced8ca44f361237cb208f7ece6b7599f2732a5332501872e18d5c0b3d"
    "ec4e1b289356a607ccf6d6b1035fdcbaa918b142ba9c2f6f5518750670421ec6283f286597d918274691329ae86b26e0"
    "cdaf0258b1ab377e487cdddc044fa6c257ab10bffe6e1be5b912e31ac26971e3409a20d976382ae759e2aa64f8f8f03d"
    "8d61703c402ac894b5d512f74fe8a7df43ecbcfb7bc81e16f55d7ccf981ce49b62ebf01edeef9da29768a8ad45f45b66"
    "690fda8e993693dbb86422c37b7246f6dfe9921ae85dd549bb304dd3430398607b42b7e98cb96120";
static const char ecm_type3_trailer_hex[] = "90adca2b";

static int write_modeled_ecm(const char *path, const uint8_t *decoded,
                             const uint8_t type1[2051], const uint8_t type2[2052],
                             const uint8_t type3[2328], int bad_checksum)
{
    static const uint8_t terminator[] = { 0xfcu, 0xffu, 0xffu, 0xffu, 0x3fu };
    static const uint8_t header2[16] = {
        0u, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu,
        0xffu, 0xffu, 0xffu, 0u, 0u, 2u, 1u, 2u
    };
    static const uint8_t header3[16] = {
        0u, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu,
        0xffu, 0xffu, 0xffu, 0u, 0u, 2u, 2u, 2u
    };
    uint32_t edc = ecm_edc(decoded, 3u * 2352u);
    uint8_t checksum[4] = { (uint8_t)edc, (uint8_t)(edc >> 8),
                            (uint8_t)(edc >> 16), (uint8_t)(edc >> 24) };
    FILE *file = schway__test_open_file(path, "wb");
    int ok;
    if (file == NULL) return 0;
    if (bad_checksum) checksum[0] ^= 1u;
    ok = fwrite("ECM\0", 1u, 4u, file) == 4u &&
         write_record_count(file, 1u, 1u) && fwrite(type1, 1u, 2051u, file) == 2051u &&
         write_count(file, 16u) && fwrite(header2, 1u, sizeof header2, file) == sizeof header2 &&
         write_record_count(file, 2u, 1u) && fwrite(type2, 1u, 2052u, file) == 2052u &&
         write_count(file, 16u) && fwrite(header3, 1u, sizeof header3, file) == sizeof header3 &&
         write_record_count(file, 3u, 1u) && fwrite(type3, 1u, 2328u, file) == 2328u &&
         fwrite(terminator, 1u, sizeof terminator, file) == sizeof terminator &&
         fwrite(checksum, 1u, sizeof checksum, file) == sizeof checksum;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static int write_type1_repeat_ecm(const char *path, const uint8_t stored[2051],
                                  const uint8_t decoded[2352], uint32_t count)
{
    static const uint8_t terminator[] = { 0xfcu, 0xffu, 0xffu, 0xffu, 0x3fu };
    FILE *file = schway__test_open_file(path, "wb");
    uint32_t edc = 0u;
    uint8_t checksum[4];
    int ok;
    if (file == NULL) return 0;
    ok = fwrite("ECM\0", 1u, 4u, file) == 4u &&
         write_record_count(file, 1u, count);
    for (uint32_t i = 0u; ok && i < count; i++) {
        ok = fwrite(stored, 1u, 2051u, file) == 2051u;
        edc = ecm_edc_update(edc, decoded, 2352u);
    }
    checksum[0] = (uint8_t)edc;
    checksum[1] = (uint8_t)(edc >> 8);
    checksum[2] = (uint8_t)(edc >> 16);
    checksum[3] = (uint8_t)(edc >> 24);
    ok = ok && fwrite(terminator, 1u, sizeof terminator, file) == sizeof terminator &&
         fwrite(checksum, 1u, sizeof checksum, file) == sizeof checksum;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static int write_one_model_ecm(const char *path, uint8_t type, const uint8_t *stored,
                               size_t stored_size, const uint8_t *decoded,
                               size_t decoded_size)
{
    static const uint8_t terminator[] = { 0xfcu, 0xffu, 0xffu, 0xffu, 0x3fu };
    uint32_t edc = ecm_edc(decoded, decoded_size);
    uint8_t checksum[4] = { (uint8_t)edc, (uint8_t)(edc >> 8),
                            (uint8_t)(edc >> 16), (uint8_t)(edc >> 24) };
    FILE *file = schway__test_open_file(path, "wb");
    int ok;
    if (file == NULL) return 0;
    ok = fwrite("ECM\0", 1u, 4u, file) == 4u &&
         write_record_count(file, type, 1u) && fwrite(stored, 1u, stored_size, file) == stored_size &&
         fwrite(terminator, 1u, sizeof terminator, file) == sizeof terminator &&
         fwrite(checksum, 1u, sizeof checksum, file) == sizeof checksum;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static int test_modeled_known_answers(const char *path)
{
    static const uint8_t header2[16] = {
        0u, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu,
        0xffu, 0xffu, 0xffu, 0u, 0u, 2u, 1u, 2u
    };
    static const uint8_t header3[16] = {
        0u, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu,
        0xffu, 0xffu, 0xffu, 0u, 0u, 2u, 2u, 2u
    };
    uint8_t decoded[3u * 2352u] = {0};
    uint8_t type1[2051];
    uint8_t type2[2052];
    uint8_t type3[2328];
    uint8_t actual[sizeof decoded];
    schway__disc_ecm *reader = NULL;
    schway_disc_image_error error;
    schway_disc_image *image = NULL;
    int ok = 0;
    decoded[0] = 0u;
    memset(decoded + 1u, 0xff, 10u);
    decoded[11] = 0u;
    decoded[12] = 0u; decoded[13] = 2u; decoded[14] = 0u; decoded[15] = 1u;
    type1[0] = 0u; type1[1] = 2u; type1[2] = 0u;
    for (size_t i = 0u; i < 2048u; i++) {
        type1[3u + i] = (uint8_t)(i * 7u + 0x13u);
        decoded[16u + i] = type1[3u + i];
    }
    if (!decode_hex(ecm_type1_trailer_hex, decoded + 2064u, 288u)) return 0;
    memcpy(decoded + 2352u, header2, sizeof header2);
    memcpy(type2, (uint8_t[]){1u, 2u, 3u, 4u}, 4u);
    memcpy(decoded + 2368u, type2, 4u);
    memcpy(decoded + 2372u, type2, 4u);
    for (size_t i = 0u; i < 2048u; i++) {
        type2[4u + i] = (uint8_t)(i * 7u + 0x13u);
        decoded[2376u + i] = type2[4u + i];
    }
    if (!decode_hex(ecm_type2_trailer_hex, decoded + 4424u, 280u)) return 0;
    memcpy(decoded + 4704u, header3, sizeof header3);
    memcpy(type3, (uint8_t[]){0x81u, 1u, 1u, 8u}, 4u);
    memcpy(decoded + 4720u, type3, 4u);
    memcpy(decoded + 4724u, type3, 4u);
    for (size_t i = 0u; i < 2324u; i++) {
        type3[4u + i] = (uint8_t)(i * 11u + 0x31u);
        decoded[4728u + i] = type3[4u + i];
    }
    if (!decode_hex(ecm_type3_trailer_hex, decoded + 7052u, 4u) ||
        !write_modeled_ecm(path, decoded, type1, type2, type3, 0) ||
        schway__disc_ecm_open(path, &reader, &error) != SCHWAY_DISC_IMAGE_OK) {
        fprintf(stderr, "ASSERT disc.image.ecm: independent modeled known-answer stream did not open: %s\n",
                error.message);
        goto done;
    }
    if (schway__disc_ecm_read_bytes(reader, 0u, actual, sizeof actual) != SCHWAY_DISC_IMAGE_OK ||
        memcmp(actual, decoded, sizeof decoded) != 0) {
        fprintf(stderr, "ASSERT disc.image.ecm: type 1/2/3 complete decoded bytes differ\n");
        goto done;
    }
    if (schway_disc_image_open(path, &image, &error) != SCHWAY_DISC_IMAGE_OK) {
        fprintf(stderr, "ASSERT disc.image.ecm: modeled fallback open failed: %s\n", error.message);
        goto done;
    }
    const schway_disc *disc = schway_disc_image_disc(image);
    for (uint32_t lba = 0u; lba < 3u; lba++) {
        uint8_t sector[2352];
        uint8_t subcode[96];
        if (disc == NULL || disc->read_sector == NULL ||
            disc->read_sector(disc->user, lba, sector, subcode) != 1u ||
            memcmp(sector, decoded + (size_t)lba * 2352u, sizeof sector) != 0) {
            fprintf(stderr, "ASSERT disc.image.ecm: modeled callback sector %u differs\n", lba);
            goto done;
        }
    }
    schway_disc_image_close(image);
    image = NULL;
    schway__disc_ecm_close(reader);
    reader = NULL;
    if (!write_modeled_ecm(path, decoded, type1, type2, type3, 1) ||
        schway__disc_ecm_open(path, &reader, &error) != SCHWAY_DISC_IMAGE_BAD_IMAGE ||
        reader != NULL) {
        fprintf(stderr, "ASSERT disc.image.ecm: modeled output checksum mutation was accepted\n");
        goto done;
    }
    if (!write_one_model_ecm(path, 2u, type2, sizeof type2, decoded + 2368u, 2336u) ||
        schway__disc_ecm_open(path, &reader, &error) != SCHWAY_DISC_IMAGE_BAD_IMAGE ||
        reader != NULL) {
        fprintf(stderr, "ASSERT disc.image.ecm: bare 2336-byte modeled image was accepted\n");
        goto done;
    }
    if (!write_type1_repeat_ecm(path, type1, decoded, 40u) ||
        schway__disc_ecm_open(path, &reader, &error) != SCHWAY_DISC_IMAGE_OK) {
        fprintf(stderr, "ASSERT disc.image.ecm: repeated modeled checkpoint image did not open\n");
        goto done;
    }
    uint8_t around_checkpoint[128];
    if (schway__disc_ecm_read_bytes(reader, 65536u, around_checkpoint,
                                    sizeof around_checkpoint) != SCHWAY_DISC_IMAGE_OK) {
        fprintf(stderr, "ASSERT disc.image.ecm: modeled checkpoint restart failed\n");
        goto done;
    }
    for (size_t i = 0u; i < sizeof around_checkpoint; i++) {
        if (around_checkpoint[i] != decoded[(65536u + i) % 2352u]) {
            fprintf(stderr, "ASSERT disc.image.ecm: modeled checkpoint byte %zu differs\n", i);
            goto done;
        }
    }
    ok = 1;
done:
    schway_disc_image_close(image);
    schway__disc_ecm_close(reader);
    return ok;
}

static int test_cue_ecm_equivalence(const char *directory)
{
    static const char cue_bin[] =
        "FILE \"disc-ecm-cue.bin\" BINARY\n"
        "  TRACK 01 MODE2/2352\n"
        "    INDEX 01 00:00:00\n"
        "  TRACK 02 AUDIO\n"
        "    PREGAP 00:00:01\n"
        "    INDEX 00 00:00:16\n"
        "    INDEX 01 00:00:18\n";
    static const char cue_ecm[] =
        "FILE \"disc-ecm-cue.ecm\" BINARY\n"
        "  TRACK 01 MODE2/2352\n"
        "    INDEX 01 00:00:00\n"
        "  TRACK 02 AUDIO\n"
        "    PREGAP 00:00:01\n"
        "    INDEX 00 00:00:16\n"
        "    INDEX 01 00:00:18\n";
    static const uint32_t lbas[] = {26u, 0u, 19u, 17u, 15u, 16u, 18u, 25u, 10u, 1u};
    const size_t byte_count = 26u * 2352u;
    uint8_t *raw = (uint8_t *)malloc(byte_count);
    char bin_path[1024] = {0}, ecm_path[1024] = {0};
    char bin_cue_path[1024] = {0}, ecm_cue_path[1024] = {0};
    char escape_path[1024] = {0};
    schway_disc_image *bin_image = NULL;
    schway_disc_image *ecm_image = NULL;
    schway_disc_image_error error;
    int ok = 0;
    if (raw == NULL || !make_path(bin_path, directory, "disc-ecm-cue.bin") ||
        !make_path(ecm_path, directory, "disc-ecm-cue.ecm") ||
        !make_path(bin_cue_path, directory, "disc-ecm-cue-bin.cue") ||
        !make_path(ecm_cue_path, directory, "disc-ecm-cue-ecm.cue")) goto done;
    for (size_t i = 0u; i < byte_count; i++) raw[i] = (uint8_t)(i * 13u + 0x29u);
    if (!write_file(bin_path, raw, byte_count) ||
        !write_literal_ecm(ecm_path, raw, byte_count, 0) ||
        !write_file(bin_cue_path, (const uint8_t *)cue_bin, sizeof cue_bin - 1u) ||
        !write_file(ecm_cue_path, (const uint8_t *)cue_ecm, sizeof cue_ecm - 1u) ||
        schway_disc_image_open(bin_cue_path, &bin_image, &error) != SCHWAY_DISC_IMAGE_OK) {
        fprintf(stderr, "ASSERT disc.image.ecm: uncompressed CUE oracle setup failed: %s\n", error.message);
        goto done;
    }
    if (schway_disc_image_open(ecm_cue_path, &ecm_image, &error) != SCHWAY_DISC_IMAGE_OK) {
        fprintf(stderr, "ASSERT disc.image.ecm: ECM-backed CUE was rejected: %s\n", error.message);
        goto done;
    }
    const schway_disc *bin_disc = schway_disc_image_disc(bin_image);
    const schway_disc *ecm_disc = schway_disc_image_disc(ecm_image);
    schway_toc bin_toc, ecm_toc;
    memset(&bin_toc, 0, sizeof bin_toc);
    memset(&ecm_toc, 0, sizeof ecm_toc);
    bin_toc.size = (uint32_t)sizeof bin_toc;
    ecm_toc.size = (uint32_t)sizeof ecm_toc;
    if (bin_disc == NULL || ecm_disc == NULL || bin_disc->toc == NULL || ecm_disc->toc == NULL ||
        bin_disc->toc(bin_disc->user, &bin_toc) != 1u ||
        ecm_disc->toc(ecm_disc->user, &ecm_toc) != 1u ||
        bin_toc.track_count != 2u || ecm_toc.track_count != 2u ||
        bin_toc.tracks[0].leadout != 16u || bin_toc.tracks[1].index0 != 16u ||
        bin_toc.tracks[1].index1 != 19u || bin_toc.tracks[1].pregap != 3u ||
        bin_toc.tracks[1].leadout != 27u ||
        memcmp(&bin_toc, &ecm_toc, sizeof bin_toc) != 0) {
        fprintf(stderr, "ASSERT disc.image.ecm: ECM-backed CUE geometry differs from raw CUE\n");
        goto done;
    }
    for (size_t i = 0u; i < sizeof lbas / sizeof lbas[0]; i++) {
        uint8_t bin_sector[2352], ecm_sector[2352], bin_subcode[96], ecm_subcode[96];
        if (bin_disc->read_sector(bin_disc->user, lbas[i], bin_sector, bin_subcode) != 1u ||
            ecm_disc->read_sector(ecm_disc->user, lbas[i], ecm_sector, ecm_subcode) != 1u ||
            memcmp(bin_sector, ecm_sector, sizeof bin_sector) != 0 ||
            memcmp(bin_subcode, ecm_subcode, sizeof bin_subcode) != 0) {
            fprintf(stderr, "ASSERT disc.image.ecm: ECM CUE LBA %u differs from raw CUE\n", lbas[i]);
            goto done;
        }
    }
    schway_disc_image_close(ecm_image);
    ecm_image = NULL;
    schway_disc_image_close(bin_image);
    bin_image = NULL;
    int escape_size = snprintf(escape_path, sizeof escape_path,
                               "%s/../disc-ecm-cue-escape.ecm", directory);
    static const char escaped_cue[] =
        "FILE \"../disc-ecm-cue-escape.ecm\" BINARY\n"
        "TRACK 01 MODE2/2352\nINDEX 01 00:00:00\n";
    if (escape_size < 0 || (size_t)escape_size >= sizeof escape_path ||
        !write_literal_ecm(escape_path, raw, byte_count, 0) ||
        !write_file(ecm_cue_path, (const uint8_t *)escaped_cue, sizeof escaped_cue - 1u) ||
        schway_disc_image_open(ecm_cue_path, &ecm_image, &error) != SCHWAY_DISC_IMAGE_BAD_CUE ||
        ecm_image != NULL) {
        fprintf(stderr, "ASSERT disc.image.ecm: ECM CUE source escaped its canonical directory\n");
        goto done;
    }
    static const char cooked_cue[] =
        "FILE \"disc-ecm-cue.ecm\" BINARY\n"
        "TRACK 01 MODE1/2048\nINDEX 01 00:00:00\n";
    if (!write_file(ecm_cue_path, (const uint8_t *)cooked_cue, sizeof cooked_cue - 1u) ||
        schway_disc_image_open(ecm_cue_path, &ecm_image, &error) !=
            SCHWAY_DISC_IMAGE_UNSUPPORTED || ecm_image != NULL) {
        fprintf(stderr, "ASSERT disc.image.ecm: unsupported cooked ECM CUE mode was accepted\n");
        goto done;
    }
    static const char duplicate_cue[] =
        "FILE \"disc-ecm-cue.ecm\" BINARY\n"
        "TRACK 01 MODE2/2352\nINDEX 01 00:00:00\n"
        "FILE \"disc-ecm-cue.ecm\" BINARY\n"
        "TRACK 02 AUDIO\nINDEX 01 00:00:00\n";
    if (!write_file(ecm_cue_path, (const uint8_t *)duplicate_cue,
                    sizeof duplicate_cue - 1u) ||
        schway_disc_image_open(ecm_cue_path, &ecm_image, &error) != SCHWAY_DISC_IMAGE_OK) {
        fprintf(stderr, "ASSERT disc.image.ecm: repeated ECM CUE source failed: %s\n", error.message);
        goto done;
    }
    const schway_disc *duplicate_disc = schway_disc_image_disc(ecm_image);
    schway_toc duplicate_toc;
    memset(&duplicate_toc, 0, sizeof duplicate_toc);
    duplicate_toc.size = (uint32_t)sizeof duplicate_toc;
    uint8_t duplicate_sector[2352], duplicate_subcode[96];
    if (duplicate_disc == NULL || duplicate_disc->toc == NULL ||
        duplicate_disc->toc(duplicate_disc->user, &duplicate_toc) != 1u ||
        duplicate_toc.track_count != 2u || duplicate_toc.tracks[0].leadout != 26u ||
        duplicate_toc.tracks[1].leadout != 52u || duplicate_disc->read_sector == NULL ||
        duplicate_disc->read_sector(duplicate_disc->user, 26u, duplicate_sector,
                                    duplicate_subcode) != 1u ||
        memcmp(duplicate_sector, raw, sizeof duplicate_sector) != 0) {
        fprintf(stderr, "ASSERT disc.image.ecm: repeated ECM source extent or track offset differs\n");
        goto done;
    }
    ok = 1;
done:
    free(raw);
    schway_disc_image_close(ecm_image);
    schway_disc_image_close(bin_image);
    if (bin_path[0] != '\0') (void)remove(bin_path);
    if (ecm_path[0] != '\0') (void)remove(ecm_path);
    if (bin_cue_path[0] != '\0') (void)remove(bin_cue_path);
    if (ecm_cue_path[0] != '\0') (void)remove(ecm_cue_path);
    if (escape_path[0] != '\0') (void)remove(escape_path);
    return ok;
}

static int write_literal_ecm(const char *path, const uint8_t *bytes, size_t size,
                             int bad_checksum)
{
    static const uint8_t terminator[] = { 0xfcu, 0xffu, 0xffu, 0xffu, 0x3fu };
    const size_t split = 73u;
    uint32_t edc = ecm_edc(bytes, size);
    uint8_t checksum[4] = {(uint8_t)edc, (uint8_t)(edc >> 8),
                           (uint8_t)(edc >> 16), (uint8_t)(edc >> 24)};
    FILE *file = schway__test_open_file(path, "wb");
    int ok;
    if (file == NULL) return 0;
    if (bad_checksum) checksum[0] ^= 1u;
    ok = fwrite("ECM\0", 1u, 4u, file) == 4u &&
         write_count(file, (uint32_t)split) &&
         fwrite(bytes, 1u, split, file) == split &&
         write_count(file, (uint32_t)(size - split)) &&
         fwrite(bytes + split, 1u, size - split, file) == size - split &&
         fwrite(terminator, 1u, sizeof terminator, file) == sizeof terminator &&
         fwrite(checksum, 1u, sizeof checksum, file) == sizeof checksum;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static int write_tiny_record_ecm(const char *path, uint32_t records)
{
    static const uint8_t terminator[] = { 0xfcu, 0xffu, 0xffu, 0xffu, 0x3fu };
    const uint8_t value = 0x5au;
    FILE *file = schway__test_open_file(path, "wb");
    uint32_t edc = 0u;
    uint8_t checksum[4];
    int ok;
    if (file == NULL) return 0;
    ok = fwrite("ECM\0", 1u, 4u, file) == 4u;
    for (uint32_t i = 0u; ok && i < records; i++) {
        ok = write_count(file, 1u) && fwrite(&value, 1u, 1u, file) == 1u;
        edc = ecm_edc_update(edc, &value, 1u);
    }
    checksum[0] = (uint8_t)edc;
    checksum[1] = (uint8_t)(edc >> 8);
    checksum[2] = (uint8_t)(edc >> 16);
    checksum[3] = (uint8_t)(edc >> 24);
    ok = ok && fwrite(terminator, 1u, sizeof terminator, file) == sizeof terminator &&
         fwrite(checksum, 1u, sizeof checksum, file) == sizeof checksum;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static int public_image_matches(const char *path, const uint8_t expected[2352])
{
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    const schway_disc *disc;
    schway_toc toc;
    uint8_t sector[2352];
    uint8_t subcode[96];
    int ok = schway_disc_image_open(path, &image, &error) == SCHWAY_DISC_IMAGE_OK;
    if (!ok) {
        fprintf(stderr, "ASSERT disc.image.ecm: valid literal ECM open failed: %s\n", error.message);
        return 0;
    }
    disc = schway_disc_image_disc(image);
    memset(&toc, 0, sizeof toc);
    toc.size = (uint32_t)sizeof toc;
    ok = disc != NULL && disc->toc != NULL && disc->read_sector != NULL &&
         disc->toc(disc->user, &toc) == 1u && toc.track_count == 1u &&
         toc.tracks[0].type == SCHWAY_DISC_TRACK_MODE2 && toc.tracks[0].leadout == 1u &&
         disc->read_sector(disc->user, 0u, sector, subcode) == 1u &&
         memcmp(sector, expected, sizeof sector) == 0;
    if (!ok) fprintf(stderr, "ASSERT disc.image.ecm: public sector or fallback TOC differs\n");
    schway_disc_image_close(image);
    return ok;
}

static int test_read_work_limit(const char *path)
{
    const size_t size = 60u * 2352u;
    const size_t limit = 70240u;
    uint8_t *expected = (uint8_t *)malloc(size);
    uint8_t *buffer = (uint8_t *)malloc(limit + 1u);
    schway_disc_image_error error;
    schway__disc_ecm *reader = NULL;
    schway__disc_ecm_read_metrics metrics;
    int ok = 0;
    if (expected == NULL || buffer == NULL) goto done;
    for (size_t i = 0u; i < size; i++) expected[i] = (uint8_t)(i * 17u + 3u);
    if (!write_literal_ecm(path, expected, size, 0) ||
        schway__disc_ecm_open(path, &reader, &error) != SCHWAY_DISC_IMAGE_OK) goto done;
    memset(&metrics, 0, sizeof metrics);
    metrics.size = (uint32_t)sizeof metrics;
    if (schway__disc_ecm_read_bytes(reader, 65536u, buffer, limit) != SCHWAY_DISC_IMAGE_OK ||
        memcmp(buffer, expected + 65536u, limit) != 0 ||
        schway__disc_ecm_read_metrics_get(reader, &metrics) != SCHWAY_DISC_IMAGE_OK ||
        metrics.checkpoint_count != 3u || metrics.decoded_forward_bytes != limit ||
        metrics.record_headers != 0u ||
        schway__disc_ecm_read_bytes(reader, 65536u, buffer, limit + 1u) != SCHWAY_DISC_IMAGE_LIMIT ||
        schway__disc_ecm_read_bytes(reader, UINT64_MAX, buffer, 1u) != SCHWAY_DISC_IMAGE_BAD_ARGUMENT) {
        fprintf(stderr, "ASSERT disc.image.ecm: per-read work limit did not accept 70240 and reject 70241 bytes\n");
        goto done;
    }
    for (uint32_t lba = 60u; lba-- > 0u;) {
        uint64_t offset = (uint64_t)lba * 2352u;
        if (schway__disc_ecm_read_bytes(reader, offset, buffer, 2352u) != SCHWAY_DISC_IMAGE_OK ||
            memcmp(buffer, expected + (size_t)offset, 2352u) != 0) {
            fprintf(stderr, "ASSERT disc.image.ecm: reverse sector read %u differed\n", lba);
            goto done;
        }
    }
    static const uint32_t interleaved[] = {0u, 59u, 30u, 0u, 58u, 1u, 30u};
    for (size_t i = 0u; i < sizeof interleaved / sizeof interleaved[0]; i++) {
        uint64_t offset = (uint64_t)interleaved[i] * 2352u;
        if (schway__disc_ecm_read_bytes(reader, offset, buffer, 2352u) != SCHWAY_DISC_IMAGE_OK ||
            memcmp(buffer, expected + (size_t)offset, 2352u) != 0) {
            fprintf(stderr, "ASSERT disc.image.ecm: repeated/interleaved read %u differed\n",
                    interleaved[i]);
            goto done;
        }
    }
    ok = 1;
done:
    schway__disc_ecm_close(reader);
    free(buffer);
    free(expected);
    return ok;
}

static int test_tiny_record_storm(const char *path)
{
    const uint32_t records = 70560u;
    const size_t limit = 70240u;
    uint8_t *buffer = (uint8_t *)malloc(limit + 1u);
    schway_disc_image_error error;
    schway__disc_ecm *reader = NULL;
    schway__disc_ecm_read_metrics metrics;
    int ok = 0;
    if (buffer == NULL || !write_tiny_record_ecm(path, records) ||
        schway__disc_ecm_open(path, &reader, &error) != SCHWAY_DISC_IMAGE_OK) goto done;
    if (schway__disc_ecm_read_bytes(reader, 0u, buffer, limit) != SCHWAY_DISC_IMAGE_OK) goto done;
    for (size_t i = 0u; i < limit; i++) {
        if (buffer[i] != 0x5au) goto done;
    }
    memset(&metrics, 0, sizeof metrics);
    metrics.size = (uint32_t)sizeof metrics;
    if (schway__disc_ecm_read_metrics_get(reader, &metrics) != SCHWAY_DISC_IMAGE_OK ||
        metrics.checkpoint_count != 2u || metrics.decoded_forward_bytes != limit ||
        metrics.record_headers != (uint32_t)(limit - 1u) ||
        schway__disc_ecm_read_bytes(reader, 0u, buffer, limit + 1u) != SCHWAY_DISC_IMAGE_LIMIT) {
        fprintf(stderr, "ASSERT disc.image.ecm: tiny-record read work exceeded its independent bound\n");
        goto done;
    }
    ok = 1;
done:
    schway__disc_ecm_close(reader);
    free(buffer);
    return ok;
}

static int test_metadata_limits(void)
{
    const uint64_t maximum = UINT64_C(1073741824);
    if (!schway__disc_ecm_test_limits(maximum, maximum, 1048576u, 16386u) ||
        schway__disc_ecm_test_limits(maximum + 1u, maximum, 1048576u, 16386u) ||
        schway__disc_ecm_test_limits(maximum, maximum + 1u, 1048576u, 16386u) ||
        schway__disc_ecm_test_limits(maximum, maximum, 1048577u, 16386u) ||
        schway__disc_ecm_test_limits(maximum, maximum, 1048576u, 16387u)) {
        fprintf(stderr, "ASSERT disc.image.ecm: media, record or index boundary changed\n");
        return 0;
    }
    return 1;
}

static int test_allocation_failure(const char *path, const uint8_t bytes[2352])
{
    schway_disc_image_error error;
    schway__disc_ecm *reader = NULL;
    if (!write_literal_ecm(path, bytes, 2352u, 0) ||
        schway__disc_ecm_test_open_fail_index_alloc(path, &reader, &error) !=
            SCHWAY_DISC_IMAGE_OUT_OF_MEMORY || reader != NULL ||
        error.status != SCHWAY_DISC_IMAGE_OUT_OF_MEMORY ||
        !write_literal_ecm(path, bytes, 2352u, 0) ||
        schway__disc_ecm_open(path, &reader, &error) != SCHWAY_DISC_IMAGE_OK) {
        schway__disc_ecm_close(reader);
        fprintf(stderr, "ASSERT disc.image.ecm: allocation failure did not clean up the source\n");
        return 0;
    }
    schway__disc_ecm_close(reader);
    return 1;
}

static int expect_open(const char *path, schway_disc_image_status expected,
                       const char *name)
{
    schway_disc_image_error error;
    schway__disc_ecm *reader;
    int sentinel;
    schway_disc_image_status status;
    reader = (schway__disc_ecm *)(void *)&sentinel;
    memset(&error, 0, sizeof error);
    status = schway__disc_ecm_open(path, &reader, &error);
    if (reader != NULL && reader != (schway__disc_ecm *)(void *)&sentinel) {
        schway__disc_ecm_close(reader);
    }
    if (status == expected && reader == NULL &&
        error.size == (uint32_t)sizeof error && error.status == (uint32_t)expected) return 1;
    fprintf(stderr, "ASSERT disc.image.ecm: %s expected status %u and null reader, got status %u, reader %s\n",
            name, (unsigned int)expected, (unsigned int)status,
            reader == NULL ? "null" : "non-null");
    return 0;
}

static int write_active_iso(const char *path)
{
    uint8_t sector[2048];
    FILE *file = schway__test_open_file(path, "wb");
    if (file == NULL) return 0;
    for (uint32_t lba = 0u; lba < 16u; lba++) {
        for (uint32_t i = 0u; i < (uint32_t)sizeof sector; i++) {
            sector[i] = (uint8_t)(lba * 7u + i);
        }
        if (fwrite(sector, 1u, sizeof sector, file) != sizeof sector) {
            (void)fclose(file);
            return 0;
        }
    }
    return fclose(file) == 0;
}

static int active_iso_is_readable(schway_disc_image *image,
                                  const schway_disc *original_disc)
{
    const schway_disc *disc = schway_disc_image_disc(image);
    uint8_t sector[2352];
    uint8_t subcode[96];
    return disc == original_disc && disc != NULL && disc->read_sector != NULL &&
           disc->read_sector(disc->user, 3u, sector, subcode) == 1u &&
           sector[16] == 21u;
}

int main(int argc, char **argv)
{
    static const uint8_t bad_magic[] = { 'B', 'A', 'D', '!' };
    static const uint8_t short_signature[] = { 'E', 'C', 'M', 0 };
    static const uint8_t short_record[] = { 'E', 'C', 'M', 0, 0x80u };
    static const uint8_t short_payload[] = { 'E', 'C', 'M', 0, 0u };
    static const uint8_t overflow_count[] = { 'E', 'C', 'M', 0, 0xfcu, 0xffu, 0xffu, 0xffu, 0x7fu };
    static const uint8_t modeled_record[] = { 'E', 'C', 'M', 0, 0x01u };
    char input_path[1024];
    char iso_path[1024];
    char playlist_path[1024];
    uint8_t expected[2352];
    schway_disc_image *active = NULL;
    schway_disc_image_error error;
    const schway_disc *active_disc = NULL;
    int result = 0;

    if (argc != 2 || !make_path(input_path, argv[1], "disc-ecm-input.EcM") ||
        !make_path(iso_path, argv[1], "disc-ecm-active.iso") ||
        !make_path(playlist_path, argv[1], "disc-ecm-input.m3u")) {
        fprintf(stderr, "ASSERT disc.image.ecm: expected a writable test directory\n");
        return 1;
    }
    if (!write_active_iso(iso_path) ||
        schway_disc_image_open(iso_path, &active, &error) != SCHWAY_DISC_IMAGE_OK) {
        fprintf(stderr, "ASSERT disc.image.ecm: could not open the existing ISO source\n");
        result = 1;
        goto done;
    }
    if (!test_metadata_limits()) result = 1;
    active_disc = schway_disc_image_disc(active);
    for (size_t i = 0u; i < sizeof expected; i++) expected[i] = (uint8_t)(i * 29u + 7u);
    if (!write_literal_ecm(input_path, expected, sizeof expected, 0) ||
        !public_image_matches(input_path, expected)) result = 1;
    static const uint8_t playlist[] = "disc-ecm-input.EcM\n";
    if (!write_file(playlist_path, playlist, sizeof playlist - 1u) ||
        !public_image_matches(playlist_path, expected)) result = 1;
    schway__disc_ecm *reader = NULL;
    if (schway__disc_ecm_open(input_path, &reader, &error) != SCHWAY_DISC_IMAGE_OK) {
        fprintf(stderr, "ASSERT disc.image.ecm: valid reader open failed for byte spans\n");
        result = 1;
    } else {
        uint8_t span[41];
        uint8_t edge;
        if (schway__disc_ecm_read_bytes(reader, 60u, span, sizeof span) != SCHWAY_DISC_IMAGE_OK ||
            memcmp(span, expected + 60u, sizeof span) != 0 ||
            schway__disc_ecm_read_bytes(reader, 0u, &edge, 1u) != SCHWAY_DISC_IMAGE_OK ||
            edge != expected[0] ||
            schway__disc_ecm_read_bytes(reader, sizeof expected - 1u, &edge, 1u) != SCHWAY_DISC_IMAGE_OK ||
            edge != expected[sizeof expected - 1u] ||
            schway__disc_ecm_read_bytes(reader, sizeof expected, &edge, 1u) != SCHWAY_DISC_IMAGE_BAD_ARGUMENT) {
            fprintf(stderr, "ASSERT disc.image.ecm: split-record or boundary byte read differed\n");
            result = 1;
        }
        schway__disc_ecm_close(reader);
    }
    if (!test_read_work_limit(input_path) || !test_tiny_record_storm(input_path) ||
        !test_allocation_failure(input_path, expected) ||
        !test_modeled_known_answers(input_path) ||
        !test_cue_ecm_equivalence(argv[1])) result = 1;
    uint8_t unaligned_image[2351];
    memcpy(unaligned_image, expected, sizeof unaligned_image);
    if (!write_literal_ecm(input_path, unaligned_image, sizeof unaligned_image, 0) ||
        !expect_open(input_path, SCHWAY_DISC_IMAGE_BAD_IMAGE, "unaligned decoded image")) result = 1;
    if (!write_literal_ecm(input_path, expected, sizeof expected, 1) ||
        !expect_open(input_path, SCHWAY_DISC_IMAGE_BAD_IMAGE, "mutated EDC") ||
        !active_iso_is_readable(active, active_disc)) {
        fprintf(stderr, "ASSERT disc.image.ecm: bad EDC published or disturbed a source\n");
        result = 1;
    }
    if (!write_file(input_path, bad_magic, sizeof bad_magic) ||
        !expect_open(input_path, SCHWAY_DISC_IMAGE_BAD_IMAGE, "bad signature")) result = 1;
    if (!write_file(input_path, short_signature, sizeof short_signature) ||
        !expect_open(input_path, SCHWAY_DISC_IMAGE_BAD_IMAGE, "truncated signature")) result = 1;
    if (!write_file(input_path, short_record, sizeof short_record) ||
        !expect_open(input_path, SCHWAY_DISC_IMAGE_BAD_IMAGE, "truncated record count")) result = 1;
    if (!write_file(input_path, short_payload, sizeof short_payload) ||
        !expect_open(input_path, SCHWAY_DISC_IMAGE_BAD_IMAGE, "truncated literal payload")) result = 1;
    if (!write_file(input_path, overflow_count, sizeof overflow_count) ||
        !expect_open(input_path, SCHWAY_DISC_IMAGE_BAD_IMAGE, "varint overflow")) result = 1;
    if (!write_file(input_path, modeled_record, sizeof modeled_record) ||
        !expect_open(input_path, SCHWAY_DISC_IMAGE_BAD_IMAGE, "truncated modeled record")) result = 1;
    if (!write_literal_ecm(input_path, expected, sizeof expected, 0) ||
        !append_byte(input_path, 0u) ||
        !expect_open(input_path, SCHWAY_DISC_IMAGE_BAD_IMAGE, "trailing byte")) result = 1;
    if (!active_iso_is_readable(active, active_disc)) {
        fprintf(stderr, "ASSERT disc.image.ecm: failed open changed the active ISO source\n");
        result = 1;
    }

done:
    schway_disc_image_close(active);
    (void)remove(input_path);
    (void)remove(iso_path);
    (void)remove(playlist_path);
    return result;
}
