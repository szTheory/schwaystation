/* Raw card import and atomic persistence through the host helper. */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../test_open.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "cards.h"
#include "../check.h"

#define TEMP_PATH_CAP 256u
#define TEMP_CANDIDATES 64u

static int make_directory(const char *path)
{
#ifdef _WIN32
    return _mkdir(path);
#else
    return mkdir(path, 0700);
#endif
}

static int remove_directory(const char *path)
{
#ifdef _WIN32
    return _rmdir(path);
#else
    return rmdir(path);
#endif
}

static int create_work_directory(char *path, size_t capacity)
{
    for (unsigned attempt = 0u; attempt < 100u; attempt++) {
        int written = snprintf(path, capacity, "card-files-%u", attempt);
        if (written < 0 || (size_t)written >= capacity) {
            return 0;
        }
        if (make_directory(path) == 0) {
            return 1;
        }
        if (errno != EEXIST) {
            return 0;
        }
    }
    return 0;
}

static int set_path(char *out, size_t capacity, const char *directory, const char *name)
{
    int written = snprintf(out, capacity, "%s/%s", directory, name);
    return written >= 0 && (size_t)written < capacity;
}

static int write_file(const char *path, const void *bytes, size_t size)
{
    FILE *file = schway__test_open_file(path, "wb");
    if (file == NULL) {
        return 0;
    }
    int ok = fwrite(bytes, 1u, size, file) == size;
    if (fclose(file) != 0) {
        ok = 0;
    }
    return ok;
}

static int read_file(const char *path, void *out, size_t capacity, size_t *size)
{
    FILE *file = schway__test_open_file(path, "rb");
    if (file == NULL) {
        return 0;
    }
    size_t read = fread(out, 1u, capacity, file);
    int extra = fgetc(file);
    int read_error = ferror(file);
    int close_error = fclose(file);
    if (read_error || extra != EOF || close_error != 0) {
        return 0;
    }
    *size = read;
    return 1;
}

static void check_file_unchanged(const char *path, const void *expected, size_t expected_size,
                                 void *snapshot, size_t snapshot_capacity)
{
    size_t size = 0u;
    CHECK(read_file(path, snapshot, snapshot_capacity, &size));
    CHECK_EQ_U64(size, expected_size);
    if (size == expected_size) {
        CHECK(memcmp(snapshot, expected, expected_size) == 0);
    }
}

static int set_temp_path(char *out, size_t capacity, const char *destination, unsigned index)
{
    int written = snprintf(out, capacity, "%s.schwaystation.tmp.%u", destination, index);
    return written >= 0 && (size_t)written < capacity;
}

static int file_exists(const char *path)
{
    FILE *file = schway__test_open_file(path, "rb");
    if (file == NULL) {
        return 0;
    }
    return fclose(file) == 0;
}

static int all_bytes_are(const uint8_t *bytes, size_t size, uint8_t expected)
{
    for (size_t i = 0u; i < size; i++) {
        if (bytes[i] != expected) {
            return 0;
        }
    }
    return 1;
}

static uint8_t frame_checksum(const uint8_t *frame)
{
    uint8_t checksum = 0u;
    for (size_t i = 0u; i < 127u; i++) {
        checksum ^= frame[i];
    }
    return checksum;
}

static void make_valid_card(uint8_t *bytes)
{
    memset(bytes, 0xFF, SCHWAY_MEMCARD_SIZE);
    const size_t header_offset = 63u * 128u;
    bytes[header_offset] = (uint8_t)'M';
    bytes[header_offset + 1u] = (uint8_t)'C';
    for (size_t frame = 0u; frame < SCHWAY_MEMCARD_SIZE / 128u; frame++) {
        uint8_t *contents = bytes + frame * 128u;
        contents[127] = frame_checksum(contents);
    }
}

static void make_asymmetric_card(uint8_t *bytes)
{
    for (size_t i = 0u; i < SCHWAY_MEMCARD_SIZE; i++) {
        bytes[i] = (uint8_t)((i * 37u) ^ (i >> 3u) ^ (i >> 7u));
    }
}

static void expect_import_refusal(const char *path, schway_host_card_status expected_status,
                                  const void *source, size_t source_size,
                                  uint8_t *out, schway_host_card_lint *lint,
                                  uint8_t *snapshot, size_t snapshot_capacity)
{
    memset(out, 0x5A, SCHWAY_MEMCARD_SIZE);
    lint->flags = 0x12345678u;
    lint->bad_frame_checksums = 0x87654321u;
    schway_host_card_status status =
        schway_host_card_import_file(path, out, SCHWAY_MEMCARD_SIZE, lint);
    CHECK_EQ_U64(status, expected_status);
    CHECK(all_bytes_are(out, SCHWAY_MEMCARD_SIZE, 0x5Au));
    CHECK_EQ_U64(lint->flags, 0x12345678u);
    CHECK_EQ_U64(lint->bad_frame_checksums, 0x87654321u);
    if (source != NULL) {
        check_file_unchanged(path, source, source_size, snapshot, snapshot_capacity);
    }
}

static void identity_paths(const char *directory)
{
    char slot1[TEMP_PATH_CAP];
    char slot2[TEMP_PATH_CAP];
    char serial_63[64];
    char long_serial[65];
    char digest_one[70];
    char digest_two[70];
    char digest_v2[70];
    char invalid_digest[70];
    char digest_slot1[TEMP_PATH_CAP];
    char digest_slot2[TEMP_PATH_CAP];
    char expected_slot1[TEMP_PATH_CAP];
    char expected_slot2[TEMP_PATH_CAP];
    char identity_name[80];
    schway_host_card_status status;

    memset(serial_63, 'A', sizeof serial_63 - 1u);
    serial_63[sizeof serial_63 - 1u] = '\0';
    status = schway_host_card_paths_for_basename(directory, serial_63,
                                                 slot1, sizeof slot1,
                                                 slot2, sizeof slot2);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    CHECK(snprintf(identity_name, sizeof identity_name, "%s.srm", serial_63) > 0);
    CHECK(set_path(expected_slot1, sizeof expected_slot1, directory, identity_name));
    CHECK(strcmp(slot1, expected_slot1) == 0);
    CHECK(snprintf(identity_name, sizeof identity_name, "%s.1.mcr", serial_63) > 0);
    CHECK(set_path(expected_slot2, sizeof expected_slot2, directory, identity_name));
    CHECK(strcmp(slot2, expected_slot2) == 0);

    memcpy(digest_one, "sgi1-", 5u);
    memcpy(digest_two, "sgi1-", 5u);
    memcpy(digest_v2, "sgi2-", 5u);
    memcpy(invalid_digest, "sgi1-", 5u);
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0u; i < 64u; i++) {
        digest_one[5u + i] = hex[i % (sizeof hex - 1u)];
        digest_two[5u + i] = hex[(i + 7u) % (sizeof hex - 1u)];
        digest_v2[5u + i] = hex[(i + 11u) % (sizeof hex - 1u)];
        invalid_digest[5u + i] = (char)'g';
    }
    digest_one[69] = '\0';
    digest_two[69] = '\0';
    digest_v2[69] = '\0';
    invalid_digest[69] = '\0';

    status = schway__host_card_paths_for_identity(directory, digest_one,
                                                  slot1, sizeof slot1,
                                                  slot2, sizeof slot2);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    CHECK(snprintf(identity_name, sizeof identity_name, "%s.srm", digest_one) > 0);
    CHECK(set_path(expected_slot1, sizeof expected_slot1, directory, identity_name));
    CHECK(strcmp(slot1, expected_slot1) == 0);
    CHECK(snprintf(identity_name, sizeof identity_name, "%s.1.mcr", digest_one) > 0);
    CHECK(set_path(expected_slot2, sizeof expected_slot2, directory, identity_name));
    CHECK(strcmp(slot2, expected_slot2) == 0);

    status = schway__host_card_paths_for_identity(directory, digest_v2,
                                                  slot1, sizeof slot1,
                                                  slot2, sizeof slot2);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    CHECK(snprintf(identity_name, sizeof identity_name, "%s.srm", digest_v2) > 0);
    CHECK(set_path(expected_slot1, sizeof expected_slot1, directory, identity_name));
    CHECK(strcmp(slot1, expected_slot1) == 0);
    CHECK(snprintf(identity_name, sizeof identity_name, "%s.1.mcr", digest_v2) > 0);
    CHECK(set_path(expected_slot2, sizeof expected_slot2, directory, identity_name));
    CHECK(strcmp(slot2, expected_slot2) == 0);

    status = schway__host_card_paths_for_identity(directory, digest_two,
                                                  digest_slot1, sizeof digest_slot1,
                                                  digest_slot2, sizeof digest_slot2);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    CHECK(strcmp(slot1, digest_slot1) != 0 && strcmp(slot2, digest_slot2) != 0);

    memset(slot1, 'Q', sizeof slot1);
    memset(slot2, 'R', sizeof slot2);
    status = schway__host_card_paths_for_identity(directory, invalid_digest,
                                                  slot1, sizeof slot1,
                                                  slot2, sizeof slot2);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_ERR_ARGUMENT);
    CHECK(all_bytes_are((const uint8_t *)slot1, sizeof slot1, (uint8_t)'Q'));
    CHECK(all_bytes_are((const uint8_t *)slot2, sizeof slot2, (uint8_t)'R'));

    status = schway__host_card_paths_for_identity(directory, digest_one,
                                                  slot1, 1u, slot2, sizeof slot2);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_ERR_SIZE);
    CHECK(all_bytes_are((const uint8_t *)slot1, sizeof slot1, (uint8_t)'Q'));
    CHECK(all_bytes_are((const uint8_t *)slot2, sizeof slot2, (uint8_t)'R'));
    status = schway__host_card_paths_for_identity(directory, digest_one,
                                                  slot1, sizeof slot1, slot2, 1u);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_ERR_SIZE);
    CHECK(all_bytes_are((const uint8_t *)slot1, sizeof slot1, (uint8_t)'Q'));
    CHECK(all_bytes_are((const uint8_t *)slot2, sizeof slot2, (uint8_t)'R'));

    memset(long_serial, 'A', sizeof long_serial - 1u);
    long_serial[sizeof long_serial - 1u] = '\0';
    memset(slot1, 'Q', sizeof slot1);
    memset(slot2, 'R', sizeof slot2);
    status = schway__host_card_paths_for_identity(directory, long_serial,
                                                  slot1, sizeof slot1,
                                                  slot2, sizeof slot2);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_ERR_ARGUMENT);
    CHECK(all_bytes_are((const uint8_t *)slot1, sizeof slot1, (uint8_t)'Q'));
    CHECK(all_bytes_are((const uint8_t *)slot2, sizeof slot2, (uint8_t)'R'));
}

static void wrappers(void)
{
    enum { GME_HEADER_SIZE = 0x0F40, VGS_HEADER_SIZE = 64, PSX_HEADER_SIZE = 256 };
    char first_directory[TEMP_PATH_CAP] = {0};
    char second_directory[TEMP_PATH_CAP] = {0};
    char gme_path[TEMP_PATH_CAP] = {0};
    char vgs_path[TEMP_PATH_CAP] = {0};
    char psx_path[TEMP_PATH_CAP] = {0};
    char first_slot1[TEMP_PATH_CAP] = {0};
    char first_slot2[TEMP_PATH_CAP] = {0};
    char second_slot1[TEMP_PATH_CAP] = {0};
    char second_slot2[TEMP_PATH_CAP] = {0};
    char content_slot1[TEMP_PATH_CAP] = {0};
    char content_slot2[TEMP_PATH_CAP] = {0};
    char expected_slot1[TEMP_PATH_CAP] = {0};
    char expected_slot2[TEMP_PATH_CAP] = {0};
    uint8_t *card = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    uint8_t *wrapped = (uint8_t *)malloc((size_t)GME_HEADER_SIZE + SCHWAY_MEMCARD_SIZE);
    uint8_t *snapshot = (uint8_t *)malloc((size_t)GME_HEADER_SIZE + SCHWAY_MEMCARD_SIZE);
    uint8_t *out = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    int made_first_directory = 0;
    int made_second_directory = 0;
    int have_gme = 0;
    int have_vgs = 0;
    int have_psx = 0;
    int have_slot1 = 0;
    int have_slot2 = 0;
    int have_second_slot1 = 0;
    int have_second_slot2 = 0;

    CHECK(card != NULL && wrapped != NULL && snapshot != NULL && out != NULL);
    if (card == NULL || wrapped == NULL || snapshot == NULL || out == NULL) {
        goto cleanup;
    }
    made_first_directory = create_work_directory(first_directory, sizeof first_directory);
    made_second_directory = create_work_directory(second_directory, sizeof second_directory);
    CHECK(made_first_directory != 0 && made_second_directory != 0);
    if (!made_first_directory || !made_second_directory ||
        !set_path(gme_path, sizeof gme_path, first_directory, "legacy.gme") ||
        !set_path(vgs_path, sizeof vgs_path, first_directory, "legacy.mem") ||
        !set_path(psx_path, sizeof psx_path, first_directory, "legacy.psx")) {
        CHECK(0);
        goto cleanup;
    }

    make_valid_card(card);
    memset(wrapped, 0, GME_HEADER_SIZE);
    /* PSX-SPX Memory Card Images, verified at commit 6d7d1bc106a7e0b616b0330fe58401ab1ba57f0f. */
    memcpy(wrapped, "123-456-STD", 11u);
    wrapped[11] = 0u;
    memcpy(wrapped + 0x10, "\0\0\1\0\1", 5u);
    for (size_t sector = 0u; sector < 16u; sector++) {
        wrapped[0x15u + sector] = card[sector * 128u];
        wrapped[0x25u + sector] = card[sector * 128u + 8u];
    }
    memcpy(wrapped + GME_HEADER_SIZE, card, SCHWAY_MEMCARD_SIZE);
    CHECK(write_file(gme_path, wrapped, (size_t)GME_HEADER_SIZE + SCHWAY_MEMCARD_SIZE));
    have_gme = 1;
    schway_host_card_lint lint = {UINT32_MAX, UINT32_MAX};
    memset(out, 0x5A, SCHWAY_MEMCARD_SIZE);
    schway_host_card_status status =
        schway_host_card_import_file(gme_path, out, SCHWAY_MEMCARD_SIZE, &lint);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    check_file_unchanged(gme_path, wrapped, (size_t)GME_HEADER_SIZE + SCHWAY_MEMCARD_SIZE,
                         snapshot, (size_t)GME_HEADER_SIZE + SCHWAY_MEMCARD_SIZE);
    if (status == SCHWAY_HOST_CARD_OK) {
        CHECK(memcmp(out, card, SCHWAY_MEMCARD_SIZE) == 0);
        CHECK_EQ_U64(lint.flags, 0u);
        CHECK_EQ_U64(lint.bad_frame_checksums, 0u);
    }

    memset(wrapped, 0, VGS_HEADER_SIZE);
    memcpy(wrapped, "VgsM", 4u);
    memcpy(wrapped + VGS_HEADER_SIZE, card, SCHWAY_MEMCARD_SIZE);
    CHECK(write_file(vgs_path, wrapped, (size_t)VGS_HEADER_SIZE + SCHWAY_MEMCARD_SIZE));
    have_vgs = 1;
    status = schway_host_card_import_file(vgs_path, out, SCHWAY_MEMCARD_SIZE, &lint);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    check_file_unchanged(vgs_path, wrapped, (size_t)VGS_HEADER_SIZE + SCHWAY_MEMCARD_SIZE,
                         snapshot, (size_t)GME_HEADER_SIZE + SCHWAY_MEMCARD_SIZE);
    if (status == SCHWAY_HOST_CARD_OK) {
        CHECK(memcmp(out, card, SCHWAY_MEMCARD_SIZE) == 0);
        CHECK_EQ_U64(lint.flags, 0u);
    }

    memset(wrapped, 0, PSX_HEADER_SIZE);
    memcpy(wrapped, "PSV", 3u);
    memcpy(wrapped + PSX_HEADER_SIZE, card, SCHWAY_MEMCARD_SIZE);
    CHECK(write_file(psx_path, wrapped, (size_t)PSX_HEADER_SIZE + SCHWAY_MEMCARD_SIZE));
    have_psx = 1;
    status = schway_host_card_import_file(psx_path, out, SCHWAY_MEMCARD_SIZE, &lint);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    check_file_unchanged(psx_path, wrapped, (size_t)PSX_HEADER_SIZE + SCHWAY_MEMCARD_SIZE,
                         snapshot, (size_t)GME_HEADER_SIZE + SCHWAY_MEMCARD_SIZE);
    if (status == SCHWAY_HOST_CARD_OK) {
        CHECK(memcmp(out, card, SCHWAY_MEMCARD_SIZE) == 0);
        CHECK_EQ_U64(lint.flags, 0u);
    }

    /* Header fields and directory copies are sourced from PSX-SPX's pinned Memory Card Images table. */
    make_asymmetric_card(card);
    status = schway_host_card_export_atomic(gme_path, card, SCHWAY_MEMCARD_SIZE,
                                            SCHWAY_HOST_CARD_FORMAT_GME);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    size_t exported_size = 0u;
    CHECK(read_file(gme_path, wrapped, (size_t)GME_HEADER_SIZE + SCHWAY_MEMCARD_SIZE,
                    &exported_size));
    CHECK_EQ_U64(exported_size, (size_t)GME_HEADER_SIZE + SCHWAY_MEMCARD_SIZE);
    CHECK(memcmp(wrapped, "123-456-STD\0", 12u) == 0);
    CHECK(memcmp(wrapped + 0x10u, "\0\0\1\0\1", 5u) == 0);
    for (size_t sector = 0u; sector < 16u; sector++) {
        CHECK_EQ_HEX(wrapped[0x15u + sector], card[sector * 128u]);
        CHECK_EQ_HEX(wrapped[0x25u + sector], card[sector * 128u + 8u]);
    }
    CHECK(all_bytes_are(wrapped + 0x40u, 0xF00u, 0u));
    CHECK(memcmp(wrapped + GME_HEADER_SIZE, card, SCHWAY_MEMCARD_SIZE) == 0);
    status = schway_host_card_import_file(gme_path, out, SCHWAY_MEMCARD_SIZE, &lint);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    CHECK(memcmp(out, card, SCHWAY_MEMCARD_SIZE) == 0);

    status = schway_host_card_export_atomic(vgs_path, card, SCHWAY_MEMCARD_SIZE,
                                            SCHWAY_HOST_CARD_FORMAT_VGS);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    CHECK(read_file(vgs_path, wrapped, (size_t)GME_HEADER_SIZE + SCHWAY_MEMCARD_SIZE,
                    &exported_size));
    CHECK_EQ_U64(exported_size, (size_t)VGS_HEADER_SIZE + SCHWAY_MEMCARD_SIZE);
    CHECK(memcmp(wrapped, "VgsM", 4u) == 0);
    CHECK(memcmp(wrapped + VGS_HEADER_SIZE, card, SCHWAY_MEMCARD_SIZE) == 0);
    status = schway_host_card_import_file(vgs_path, out, SCHWAY_MEMCARD_SIZE, &lint);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    CHECK(memcmp(out, card, SCHWAY_MEMCARD_SIZE) == 0);

    status = schway_host_card_export_atomic(psx_path, card, SCHWAY_MEMCARD_SIZE,
                                            SCHWAY_HOST_CARD_FORMAT_PSV);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    CHECK(read_file(psx_path, wrapped, (size_t)GME_HEADER_SIZE + SCHWAY_MEMCARD_SIZE,
                    &exported_size));
    CHECK_EQ_U64(exported_size, (size_t)PSX_HEADER_SIZE + SCHWAY_MEMCARD_SIZE);
    CHECK(memcmp(wrapped, "PSV", 3u) == 0);
    CHECK(memcmp(wrapped + PSX_HEADER_SIZE, card, SCHWAY_MEMCARD_SIZE) == 0);
    status = schway_host_card_import_file(psx_path, out, SCHWAY_MEMCARD_SIZE, &lint);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    CHECK(memcmp(out, card, SCHWAY_MEMCARD_SIZE) == 0);

    status = schway_host_card_export_atomic(gme_path, card, SCHWAY_MEMCARD_SIZE,
                                            SCHWAY_HOST_CARD_FORMAT_RAW);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    CHECK(read_file(gme_path, wrapped, (size_t)GME_HEADER_SIZE + SCHWAY_MEMCARD_SIZE,
                    &exported_size));
    CHECK_EQ_U64(exported_size, SCHWAY_MEMCARD_SIZE);
    CHECK(memcmp(wrapped, card, SCHWAY_MEMCARD_SIZE) == 0);
    status = schway_host_card_import_file(gme_path, out, SCHWAY_MEMCARD_SIZE, &lint);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    CHECK(memcmp(out, card, SCHWAY_MEMCARD_SIZE) == 0);

    CHECK(schway_host_card_paths_for_basename(first_directory, "game",
                                              content_slot1, sizeof content_slot1,
                                              content_slot2, sizeof content_slot2) == SCHWAY_HOST_CARD_OK);
    CHECK(set_path(expected_slot1, sizeof expected_slot1, first_directory, "game.srm"));
    CHECK(set_path(expected_slot2, sizeof expected_slot2, first_directory, "game.1.mcr"));
    CHECK(strcmp(content_slot1, expected_slot1) == 0);
    CHECK(strcmp(content_slot2, expected_slot2) == 0);
    identity_paths(first_directory);

    CHECK(schway_host_card_paths_for_basename(first_directory, "SLUS_123.45",
                                              first_slot1, sizeof first_slot1,
                                              first_slot2, sizeof first_slot2) == SCHWAY_HOST_CARD_OK);
    CHECK(schway_host_card_paths_for_basename(second_directory, "SLUS_123.45",
                                              second_slot1, sizeof second_slot1,
                                              second_slot2, sizeof second_slot2) == SCHWAY_HOST_CARD_OK);
    CHECK(set_path(expected_slot1, sizeof expected_slot1, first_directory, "SLUS_123.45.srm"));
    CHECK(set_path(expected_slot2, sizeof expected_slot2, first_directory, "SLUS_123.45.1.mcr"));
    CHECK(strcmp(first_slot1, expected_slot1) == 0);
    CHECK(strcmp(first_slot2, expected_slot2) == 0);
    CHECK(strcmp(first_slot1, second_slot1) != 0 && strcmp(first_slot2, second_slot2) != 0);
    if (first_slot1[0] != '\0' && first_slot2[0] != '\0') {
        status = schway_host_card_save_atomic(first_slot1, card, SCHWAY_MEMCARD_SIZE);
        CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
        have_slot1 = status == SCHWAY_HOST_CARD_OK;
        status = schway_host_card_save_atomic(first_slot2, card, SCHWAY_MEMCARD_SIZE);
        CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
        have_slot2 = status == SCHWAY_HOST_CARD_OK;
        status = schway_host_card_save_atomic(second_slot1, card, SCHWAY_MEMCARD_SIZE);
        CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
        have_second_slot1 = status == SCHWAY_HOST_CARD_OK;
        status = schway_host_card_save_atomic(second_slot2, card, SCHWAY_MEMCARD_SIZE);
        CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
        have_second_slot2 = status == SCHWAY_HOST_CARD_OK;
    }

cleanup:
    if (have_second_slot2) {
        (void)remove(second_slot2);
    }
    if (have_second_slot1) {
        (void)remove(second_slot1);
    }
    if (have_slot2) {
        (void)remove(first_slot2);
    }
    if (have_slot1) {
        (void)remove(first_slot1);
    }
    if (have_psx) {
        (void)remove(psx_path);
    }
    if (have_vgs) {
        (void)remove(vgs_path);
    }
    if (have_gme) {
        (void)remove(gme_path);
    }
    if (made_second_directory) {
        (void)remove_directory(second_directory);
    }
    if (made_first_directory) {
        (void)remove_directory(first_directory);
    }
    free(out);
    free(snapshot);
    free(wrapped);
    free(card);
}

static void wrapper_edges(void)
{
    enum { GME_HEADER_SIZE = 0x0F40, VGS_HEADER_SIZE = 64, PSX_HEADER_SIZE = 256 };
    const size_t input_capacity = (size_t)GME_HEADER_SIZE + SCHWAY_MEMCARD_SIZE;
    char directory[TEMP_PATH_CAP] = {0};
    char source_path[TEMP_PATH_CAP] = {0};
    char missing_path[TEMP_PATH_CAP] = {0};
    char partial_gme_path[TEMP_PATH_CAP] = {0};
    char partial_vgs_path[TEMP_PATH_CAP] = {0};
    char partial_psx_path[TEMP_PATH_CAP] = {0};
    char single_psx_path[TEMP_PATH_CAP] = {0};
    char single_mcs_path[TEMP_PATH_CAP] = {0};
    char ps3_psv_path[TEMP_PATH_CAP] = {0};
    char slot1[TEMP_PATH_CAP];
    char slot2[TEMP_PATH_CAP];
    char long_serial[65];
    uint8_t *card = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    uint8_t *input = (uint8_t *)malloc(input_capacity + 1u);
    uint8_t *out = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    uint8_t *snapshot = (uint8_t *)malloc(input_capacity + 1u);
    int made_directory = 0;
    int have_source = 0;
    int have_partial_gme = 0;
    int have_partial_vgs = 0;
    int have_partial_psx = 0;
    int have_single_psx = 0;
    int have_single_mcs = 0;
    int have_ps3_psv = 0;

    CHECK(card != NULL && input != NULL && out != NULL && snapshot != NULL);
    if (card == NULL || input == NULL || out == NULL || snapshot == NULL) {
        goto cleanup;
    }
    made_directory = create_work_directory(directory, sizeof directory);
    CHECK(made_directory != 0);
    if (!made_directory || !set_path(source_path, sizeof source_path, directory, "malformed.card") ||
        !set_path(missing_path, sizeof missing_path, directory, "missing.card") ||
        !set_path(partial_gme_path, sizeof partial_gme_path, directory, "partial.gme") ||
        !set_path(partial_vgs_path, sizeof partial_vgs_path, directory, "partial.mem") ||
        !set_path(partial_psx_path, sizeof partial_psx_path, directory, "partial.psx") ||
        !set_path(single_psx_path, sizeof single_psx_path, directory, "single.psx") ||
        !set_path(single_mcs_path, sizeof single_mcs_path, directory, "single.mcs") ||
        !set_path(ps3_psv_path, sizeof ps3_psv_path, directory, "single.psv")) {
        CHECK(0);
        goto cleanup;
    }

    schway_host_card_lint lint = {0u, 0u};
    expect_import_refusal(missing_path, SCHWAY_HOST_CARD_ERR_NOT_FOUND, NULL, 0u,
                          out, &lint, snapshot, input_capacity + 1u);

    /* A known wrapper extension plus a partial pinned signature is a malformed wrapper. */
    memcpy(input, "123-456", 7u);
    CHECK(write_file(partial_gme_path, input, 7u));
    have_partial_gme = 1;
    expect_import_refusal(partial_gme_path, SCHWAY_HOST_CARD_ERR_SIZE, input, 7u,
                          out, &lint, snapshot, input_capacity + 1u);

    memcpy(input, "Vgs", 3u);
    CHECK(write_file(partial_vgs_path, input, 3u));
    have_partial_vgs = 1;
    expect_import_refusal(partial_vgs_path, SCHWAY_HOST_CARD_ERR_SIZE, input, 3u,
                          out, &lint, snapshot, input_capacity + 1u);

    memcpy(input, "PS", 2u);
    CHECK(write_file(partial_psx_path, input, 2u));
    have_partial_psx = 1;
    expect_import_refusal(partial_psx_path, SCHWAY_HOST_CARD_ERR_SIZE, input, 2u,
                          out, &lint, snapshot, input_capacity + 1u);

    /* PSX-SPX identifies these as single-save layouts rather than card images. */
    memset(input, 0, 54u + 8192u);
    memcpy(input, "SLUS_123.45", sizeof("SLUS_123.45") - 1u);
    memcpy(input + 21u, "Saved title", sizeof("Saved title") - 1u);
    input[54] = (uint8_t)'S';
    input[55] = (uint8_t)'C';
    CHECK(write_file(single_psx_path, input, 54u + 8192u));
    have_single_psx = 1;
    expect_import_refusal(single_psx_path, SCHWAY_HOST_CARD_ERR_SIZE, input, 54u + 8192u,
                          out, &lint, snapshot, input_capacity + 1u);

    memset(input, 0, 128u + 8192u);
    input[128] = (uint8_t)'S';
    input[129] = (uint8_t)'C';
    CHECK(write_file(single_mcs_path, input, 128u + 8192u));
    have_single_mcs = 1;
    expect_import_refusal(single_mcs_path, SCHWAY_HOST_CARD_ERR_SIZE, input, 128u + 8192u,
                          out, &lint, snapshot, input_capacity + 1u);

    memset(input, 0, PSX_HEADER_SIZE + SCHWAY_MEMCARD_SIZE);
    memcpy(input, "PSV", 3u);
    CHECK(write_file(ps3_psv_path, input, (size_t)PSX_HEADER_SIZE + SCHWAY_MEMCARD_SIZE));
    have_ps3_psv = 1;
    expect_import_refusal(ps3_psv_path, SCHWAY_HOST_CARD_ERR_SIZE, input,
                          (size_t)PSX_HEADER_SIZE + SCHWAY_MEMCARD_SIZE,
                          out, &lint, snapshot, input_capacity + 1u);

    make_valid_card(card);
    CHECK(write_file(source_path, card, 0u));
    have_source = 1;
    expect_import_refusal(source_path, SCHWAY_HOST_CARD_ERR_SIZE, card, 0u,
                          out, &lint, snapshot, input_capacity + 1u);

    memset(input, 0x47, (size_t)SCHWAY_MEMCARD_SIZE + 1u);
    CHECK(write_file(source_path, input, (size_t)SCHWAY_MEMCARD_SIZE + 1u));
    expect_import_refusal(source_path, SCHWAY_HOST_CARD_ERR_SIZE, input,
                          (size_t)SCHWAY_MEMCARD_SIZE + 1u,
                          out, &lint, snapshot, input_capacity + 1u);

    memset(input, 0, GME_HEADER_SIZE);
    memcpy(input, "123-456-STD", 11u);
    memcpy(input + GME_HEADER_SIZE, card, SCHWAY_MEMCARD_SIZE);
    size_t gme_size = (size_t)GME_HEADER_SIZE + SCHWAY_MEMCARD_SIZE;
    CHECK(write_file(source_path, input, gme_size - 1u));
    expect_import_refusal(source_path, SCHWAY_HOST_CARD_ERR_SIZE, input, gme_size - 1u,
                          out, &lint, snapshot, input_capacity + 1u);

    input[11] = 1u;
    CHECK(write_file(source_path, input, gme_size));
    expect_import_refusal(source_path, SCHWAY_HOST_CARD_ERR_SIZE, input, gme_size,
                          out, &lint, snapshot, input_capacity + 1u);

    memset(input, 0, GME_HEADER_SIZE);
    memcpy(input, "123-456-STD", 11u);
    memcpy(input + 0x10u, "\0\0\1\0\1", 5u);
    input[0x12u] = 0u;
    memcpy(input + GME_HEADER_SIZE, card, SCHWAY_MEMCARD_SIZE);
    CHECK(write_file(source_path, input, gme_size));
    expect_import_refusal(source_path, SCHWAY_HOST_CARD_ERR_SIZE, input, gme_size,
                          out, &lint, snapshot, input_capacity + 1u);

    memset(input, 0, VGS_HEADER_SIZE);
    memcpy(input, "VgsM", 4u);
    memcpy(input + VGS_HEADER_SIZE, card, SCHWAY_MEMCARD_SIZE);
    size_t vgs_size = (size_t)VGS_HEADER_SIZE + SCHWAY_MEMCARD_SIZE;
    CHECK(write_file(source_path, input, vgs_size - 1u));
    expect_import_refusal(source_path, SCHWAY_HOST_CARD_ERR_SIZE, input, vgs_size - 1u,
                          out, &lint, snapshot, input_capacity + 1u);

    memset(input, 0, PSX_HEADER_SIZE);
    memcpy(input, "PSV", 3u);
    memcpy(input + PSX_HEADER_SIZE, card, SCHWAY_MEMCARD_SIZE);
    size_t psx_size = (size_t)PSX_HEADER_SIZE + SCHWAY_MEMCARD_SIZE;
    CHECK(write_file(source_path, input, psx_size - 1u));
    expect_import_refusal(source_path, SCHWAY_HOST_CARD_ERR_SIZE, input, psx_size - 1u,
                          out, &lint, snapshot, input_capacity + 1u);

    memset(input, 0, VGS_HEADER_SIZE);
    input[0] = (uint8_t)'X';
    memcpy(input + VGS_HEADER_SIZE, card, SCHWAY_MEMCARD_SIZE);
    CHECK(write_file(source_path, input, vgs_size));
    expect_import_refusal(source_path, SCHWAY_HOST_CARD_ERR_SIZE, input, vgs_size,
                          out, &lint, snapshot, input_capacity + 1u);

    memset(input, 0x31, SCHWAY_MEMCARD_SIZE + 1u);
    memcpy(input, "MCX", 3u);
    CHECK(write_file(source_path, input, SCHWAY_MEMCARD_SIZE + 1u));
    expect_import_refusal(source_path, SCHWAY_HOST_CARD_ERR_SIZE, input,
                          SCHWAY_MEMCARD_SIZE + 1u,
                          out, &lint, snapshot, input_capacity + 1u);

    memset(input, 0x29, 23u);
    CHECK(write_file(source_path, input, 23u));
    memset(out, 0x5A, SCHWAY_MEMCARD_SIZE);
    lint.flags = UINT32_MAX;
    lint.bad_frame_checksums = UINT32_MAX;
    schway_host_card_status status =
        schway_host_card_import_file(source_path, out, SCHWAY_MEMCARD_SIZE, &lint);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    CHECK(memcmp(out, input, 23u) == 0);
    CHECK(all_bytes_are(out + 23u, SCHWAY_MEMCARD_SIZE - 23u, 0xFFu));
    CHECK((lint.flags & SCHWAY_HOST_CARD_LINT_BAD_HEADER) != 0u);
    check_file_unchanged(source_path, input, 23u, snapshot, input_capacity + 1u);

    make_valid_card(card);
    const size_t header_offset = 63u * 128u;
    card[header_offset] = (uint8_t)'X';
    card[header_offset + 127u] = frame_checksum(card + header_offset);
    memset(input, 0, VGS_HEADER_SIZE);
    memcpy(input, "VgsM", 4u);
    memcpy(input + VGS_HEADER_SIZE, card, SCHWAY_MEMCARD_SIZE);
    CHECK(write_file(source_path, input, vgs_size));
    status = schway_host_card_import_file(source_path, out, SCHWAY_MEMCARD_SIZE, &lint);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    CHECK(memcmp(out, card, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK_EQ_U64(lint.flags, SCHWAY_HOST_CARD_LINT_BAD_HEADER);
    CHECK_EQ_U64(lint.bad_frame_checksums, 0u);
    check_file_unchanged(source_path, input, vgs_size, snapshot, input_capacity + 1u);

    make_valid_card(card);
    card[0] ^= 1u;
    memset(input, 0, PSX_HEADER_SIZE);
    memcpy(input, "PSV", 3u);
    memcpy(input + PSX_HEADER_SIZE, card, SCHWAY_MEMCARD_SIZE);
    CHECK(write_file(source_path, input, psx_size));
    status = schway_host_card_import_file(source_path, out, SCHWAY_MEMCARD_SIZE, &lint);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    CHECK(memcmp(out, card, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK_EQ_U64(lint.flags, SCHWAY_HOST_CARD_LINT_BAD_CHECKSUM);
    CHECK_EQ_U64(lint.bad_frame_checksums, 1u);
    check_file_unchanged(source_path, input, psx_size, snapshot, input_capacity + 1u);

    const char *invalid_serials[] = {
        "", ".", "..", "SLUS/../escape", "SLUS\\..\\escape", "BAD:SERIAL", "SERIAL NAME"
    };
    for (size_t i = 0u; i < sizeof invalid_serials / sizeof invalid_serials[0]; i++) {
        memset(slot1, 'Q', sizeof slot1);
        memset(slot2, 'R', sizeof slot2);
        status = schway_host_card_paths_for_basename(directory, invalid_serials[i],
                                                     slot1, sizeof slot1,
                                                     slot2, sizeof slot2);
        CHECK_EQ_U64(status, SCHWAY_HOST_CARD_ERR_ARGUMENT);
        CHECK(all_bytes_are((const uint8_t *)slot1, sizeof slot1, (uint8_t)'Q'));
        CHECK(all_bytes_are((const uint8_t *)slot2, sizeof slot2, (uint8_t)'R'));
    }
    memset(long_serial, 'A', sizeof long_serial - 1u);
    long_serial[sizeof long_serial - 1u] = '\0';
    memset(slot1, 'Q', sizeof slot1);
    memset(slot2, 'R', sizeof slot2);
    status = schway_host_card_paths_for_basename(directory, long_serial,
                                                 slot1, sizeof slot1,
                                                 slot2, sizeof slot2);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_ERR_ARGUMENT);
    CHECK(all_bytes_are((const uint8_t *)slot1, sizeof slot1, (uint8_t)'Q'));
    CHECK(all_bytes_are((const uint8_t *)slot2, sizeof slot2, (uint8_t)'R'));

    memset(slot1, 'Q', sizeof slot1);
    memset(slot2, 'R', sizeof slot2);
    status = schway_host_card_paths_for_basename(directory, "SLUS_123.45",
                                                 slot1, 1u, slot2, sizeof slot2);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_ERR_SIZE);
    CHECK(all_bytes_are((const uint8_t *)slot1, sizeof slot1, (uint8_t)'Q'));
    CHECK(all_bytes_are((const uint8_t *)slot2, sizeof slot2, (uint8_t)'R'));
    status = schway_host_card_paths_for_basename(directory, "SLUS_123.45",
                                                 slot1, sizeof slot1, slot2, 1u);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_ERR_SIZE);
    CHECK(all_bytes_are((const uint8_t *)slot1, sizeof slot1, (uint8_t)'Q'));
    CHECK(all_bytes_are((const uint8_t *)slot2, sizeof slot2, (uint8_t)'R'));

cleanup:
    if (have_source) (void)remove(source_path);
    if (have_ps3_psv) (void)remove(ps3_psv_path);
    if (have_single_mcs) (void)remove(single_mcs_path);
    if (have_single_psx) (void)remove(single_psx_path);
    if (have_partial_psx) (void)remove(partial_psx_path);
    if (have_partial_vgs) (void)remove(partial_vgs_path);
    if (have_partial_gme) (void)remove(partial_gme_path);
    if (made_directory) (void)remove_directory(directory);
    free(snapshot);
    free(out);
    free(input);
    free(card);
}

static void atomic_roundtrip(void)
{
    char directory[TEMP_PATH_CAP] = {0};
    char source[TEMP_PATH_CAP] = {0};
    char destination[TEMP_PATH_CAP] = {0};
    char temporary[TEMP_PATH_CAP] = {0};
    uint8_t raw[37];
    uint8_t source_readback[sizeof raw];
    uint8_t marker = 0xA7u;
    uint8_t *initial = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    uint8_t *imported = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    uint8_t *dirty = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    uint8_t *readback = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    int made_directory = 0;
    int have_source = 0;
    int have_destination = 0;
    int have_temp[TEMP_CANDIDATES] = {0};

    CHECK(initial != NULL && imported != NULL && dirty != NULL && readback != NULL);
    if (initial == NULL || imported == NULL || dirty == NULL || readback == NULL) {
        goto cleanup;
    }
    made_directory = create_work_directory(directory, sizeof directory);
    CHECK(made_directory != 0);
    if (!made_directory || !set_path(source, sizeof source, directory, "source.raw") ||
        !set_path(destination, sizeof destination, directory, "save.mcr")) {
        CHECK(0);
        goto cleanup;
    }

    for (size_t i = 0u; i < sizeof raw; i++) {
        raw[i] = (uint8_t)(i * 17u + 3u);
    }
    memset(initial, 0x5Au, SCHWAY_MEMCARD_SIZE);
    memset(imported, 0x11, SCHWAY_MEMCARD_SIZE);
    CHECK(write_file(source, raw, sizeof raw));
    have_source = 1;
    CHECK(write_file(destination, initial, SCHWAY_MEMCARD_SIZE));
    have_destination = 1;

    schway_host_card_lint lint = {0u, 0u};
    schway_host_card_status status =
        schway_host_card_import_raw(source, imported, SCHWAY_MEMCARD_SIZE, &lint);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    if (status != SCHWAY_HOST_CARD_OK) {
        goto cleanup;
    }
    CHECK(memcmp(imported, raw, sizeof raw) == 0);
    CHECK_EQ_U64(lint.flags, SCHWAY_HOST_CARD_LINT_BAD_HEADER | SCHWAY_HOST_CARD_LINT_BAD_CHECKSUM);
    CHECK_EQ_U64(lint.bad_frame_checksums, 1u);
    size_t source_size = 0u;
    CHECK(read_file(source, source_readback, sizeof source_readback, &source_size));
    CHECK_EQ_U64(source_size, sizeof raw);
    CHECK(memcmp(source_readback, raw, sizeof raw) == 0);
    for (size_t i = sizeof raw; i < SCHWAY_MEMCARD_SIZE; i++) {
        if (imported[i] != 0xFFu) {
            CHECK(imported[i] == 0xFFu);
            break;
        }
    }
    for (size_t i = 0u; i < SCHWAY_MEMCARD_SIZE; i++) {
        dirty[i] = (uint8_t)(imported[i] ^ (uint8_t)(i * 29u));
    }

    CHECK(set_temp_path(temporary, sizeof temporary, destination, 0u));
    if (temporary[0] == '\0' || !write_file(temporary, &marker, sizeof marker)) {
        CHECK(0);
        goto cleanup;
    }
    have_temp[0] = 1;
    status = schway_host_card_save_atomic(destination, dirty, SCHWAY_MEMCARD_SIZE);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    if (status != SCHWAY_HOST_CARD_OK) {
        goto cleanup;
    }
    size_t readback_size = 0u;
    CHECK(read_file(destination, readback, SCHWAY_MEMCARD_SIZE, &readback_size));
    CHECK_EQ_U64(readback_size, SCHWAY_MEMCARD_SIZE);
    CHECK(memcmp(readback, dirty, SCHWAY_MEMCARD_SIZE) == 0);
    uint8_t marker_read = 0u;
    size_t marker_size = 0u;
    CHECK(read_file(temporary, &marker_read, sizeof marker_read, &marker_size));
    CHECK_EQ_U64(marker_size, sizeof marker);
    CHECK_EQ_HEX(marker_read, marker);

    for (unsigned i = 0u; i < TEMP_CANDIDATES; i++) {
        if (i == 0u) {
            continue;
        }
        CHECK(set_temp_path(temporary, sizeof temporary, destination, i));
        uint8_t candidate_marker = (uint8_t)(0x40u + i);
        CHECK(write_file(temporary, &candidate_marker, sizeof candidate_marker));
        have_temp[i] = 1;
    }
    memset(dirty, 0xC3, SCHWAY_MEMCARD_SIZE);
    status = schway_host_card_save_atomic(destination, dirty, SCHWAY_MEMCARD_SIZE);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_ERR_IO);
    CHECK(read_file(destination, readback, SCHWAY_MEMCARD_SIZE, &readback_size));
    CHECK_EQ_U64(readback_size, SCHWAY_MEMCARD_SIZE);
    for (size_t i = 0u; i < SCHWAY_MEMCARD_SIZE; i++) {
        if (readback[i] != (uint8_t)(imported[i] ^ (uint8_t)(i * 29u))) {
            CHECK(0);
            break;
        }
    }

cleanup:
    for (unsigned i = 0u; i < TEMP_CANDIDATES; i++) {
        if (have_temp[i]) {
            if (set_temp_path(temporary, sizeof temporary, destination, i)) {
                (void)remove(temporary);
            }
        }
    }
    if (have_source) {
        (void)remove(source);
    }
    if (have_destination) {
        (void)remove(destination);
    }
    if (made_directory) {
        (void)remove_directory(directory);
    }
    free(readback);
    free(dirty);
    free(imported);
    free(initial);
}

static void atomic_failures(void)
{
    enum { GME_HEADER_SIZE = 0x0F40 };
    const size_t gme_size = (size_t)GME_HEADER_SIZE + SCHWAY_MEMCARD_SIZE;
    char first_directory[TEMP_PATH_CAP] = {0};
    char second_directory[TEMP_PATH_CAP] = {0};
    char exact_source[TEMP_PATH_CAP] = {0};
    char empty_source[TEMP_PATH_CAP] = {0};
    char oversized_source[TEMP_PATH_CAP] = {0};
    char missing_source[TEMP_PATH_CAP] = {0};
    char first_destination[TEMP_PATH_CAP] = {0};
    char second_destination[TEMP_PATH_CAP] = {0};
    char wrapped_destination[TEMP_PATH_CAP] = {0};
    char temporary[TEMP_PATH_CAP] = {0};
    uint8_t marker = 0xD2u;
    uint8_t *first = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    uint8_t *second = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    uint8_t *oversized = (uint8_t *)malloc((size_t)SCHWAY_MEMCARD_SIZE + 1u);
    uint8_t *out = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    uint8_t *readback = (uint8_t *)malloc(SCHWAY_MEMCARD_SIZE);
    uint8_t *wrapped_previous = (uint8_t *)malloc(gme_size);
    uint8_t *wrapped_readback = (uint8_t *)malloc(gme_size);
    int made_first_directory = 0;
    int made_second_directory = 0;
    int have_exact_source = 0;
    int have_empty_source = 0;
    int have_oversized_source = 0;
    int have_first_destination = 0;
    int have_second_destination = 0;
    int have_temp_marker = 0;
    int have_wrapped_destination = 0;

    CHECK(first != NULL && second != NULL && oversized != NULL && out != NULL && readback != NULL &&
          wrapped_previous != NULL && wrapped_readback != NULL);
    if (first == NULL || second == NULL || oversized == NULL || out == NULL || readback == NULL ||
        wrapped_previous == NULL || wrapped_readback == NULL) {
        goto cleanup;
    }
    made_first_directory = create_work_directory(first_directory, sizeof first_directory);
    CHECK(made_first_directory != 0);
    made_second_directory = create_work_directory(second_directory, sizeof second_directory);
    CHECK(made_second_directory != 0);
    if (!made_first_directory || !made_second_directory ||
        !set_path(exact_source, sizeof exact_source, first_directory, "exact.raw") ||
        !set_path(empty_source, sizeof empty_source, first_directory, "empty.raw") ||
        !set_path(oversized_source, sizeof oversized_source, first_directory, "oversized.raw") ||
        !set_path(missing_source, sizeof missing_source, first_directory, "missing.raw") ||
        !set_path(first_destination, sizeof first_destination, first_directory, "save.mcr") ||
        !set_path(second_destination, sizeof second_destination, second_directory, "save.mcr") ||
        !set_path(wrapped_destination, sizeof wrapped_destination, first_directory, "wrapped.gme")) {
        CHECK(0);
        goto cleanup;
    }

    make_valid_card(first);
    CHECK(write_file(exact_source, first, SCHWAY_MEMCARD_SIZE));
    have_exact_source = 1;
    memset(out, 0x5A, SCHWAY_MEMCARD_SIZE);
    schway_host_card_lint lint = {UINT32_MAX, UINT32_MAX};
    schway_host_card_status status =
        schway_host_card_import_raw(exact_source, out, SCHWAY_MEMCARD_SIZE, &lint);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    CHECK(lint.flags == 0u);
    CHECK_EQ_U64(lint.bad_frame_checksums, 0u);
    CHECK(memcmp(out, first, SCHWAY_MEMCARD_SIZE) == 0);

    CHECK(write_file(empty_source, &marker, 0u));
    have_empty_source = 1;
    memset(out, 0x5A, SCHWAY_MEMCARD_SIZE);
    status = schway_host_card_import_raw(empty_source, out, SCHWAY_MEMCARD_SIZE, NULL);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_ERR_SIZE);
    CHECK(all_bytes_are(out, SCHWAY_MEMCARD_SIZE, 0x5Au));

    memset(oversized, 0x39, (size_t)SCHWAY_MEMCARD_SIZE + 1u);
    CHECK(write_file(oversized_source, oversized, (size_t)SCHWAY_MEMCARD_SIZE + 1u));
    have_oversized_source = 1;
    status = schway_host_card_import_raw(oversized_source, out, SCHWAY_MEMCARD_SIZE, NULL);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_ERR_SIZE);
    CHECK(all_bytes_are(out, SCHWAY_MEMCARD_SIZE, 0x5Au));
    status = schway_host_card_import_raw(missing_source, out, SCHWAY_MEMCARD_SIZE, NULL);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_ERR_NOT_FOUND);
    CHECK(all_bytes_are(out, SCHWAY_MEMCARD_SIZE, 0x5Au));
    status = schway_host_card_import_raw(exact_source, out, SCHWAY_MEMCARD_SIZE - 1u, NULL);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_ERR_ARGUMENT);
    CHECK(all_bytes_are(out, SCHWAY_MEMCARD_SIZE, 0x5Au));

    for (size_t i = 0u; i < SCHWAY_MEMCARD_SIZE; i++) {
        first[i] = (uint8_t)(i * 7u + 1u);
        second[i] = (uint8_t)(i * 11u + 2u);
    }
    CHECK(set_temp_path(temporary, sizeof temporary, first_destination, 0u));
    CHECK(write_file(temporary, &marker, sizeof marker));
    have_temp_marker = 1;
    status = schway_host_card_save_atomic(first_destination, first, SCHWAY_MEMCARD_SIZE);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    have_first_destination = 1;
    status = schway_host_card_save_atomic(second_destination, second, SCHWAY_MEMCARD_SIZE);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    have_second_destination = 1;
    size_t readback_size = 0u;
    CHECK(read_file(first_destination, readback, SCHWAY_MEMCARD_SIZE, &readback_size));
    CHECK_EQ_U64(readback_size, SCHWAY_MEMCARD_SIZE);
    CHECK(memcmp(readback, first, SCHWAY_MEMCARD_SIZE) == 0);
    CHECK(read_file(second_destination, readback, SCHWAY_MEMCARD_SIZE, &readback_size));
    CHECK_EQ_U64(readback_size, SCHWAY_MEMCARD_SIZE);
    CHECK(memcmp(readback, second, SCHWAY_MEMCARD_SIZE) == 0);
    uint8_t marker_read = 0u;
    size_t marker_size = 0u;
    CHECK(read_file(temporary, &marker_read, sizeof marker_read, &marker_size));
    CHECK_EQ_U64(marker_size, sizeof marker);
    CHECK_EQ_HEX(marker_read, marker);

    status = schway_host_card_save_atomic(first_destination, second, SCHWAY_MEMCARD_SIZE - 1u);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_ERR_SIZE);
    CHECK(read_file(first_destination, readback, SCHWAY_MEMCARD_SIZE, &readback_size));
    CHECK(memcmp(readback, first, SCHWAY_MEMCARD_SIZE) == 0);

    CHECK(remove(temporary) == 0);
    have_temp_marker = 0;
    status = schway_host_card_test_save_atomic(first_destination, second, SCHWAY_MEMCARD_SIZE,
                                               SCHWAY_HOST_CARD_TEST_FAILURE_FLUSH);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_ERR_IO);
    CHECK(read_file(first_destination, readback, SCHWAY_MEMCARD_SIZE, &readback_size));
    CHECK(memcmp(readback, first, SCHWAY_MEMCARD_SIZE) == 0);
    for (unsigned i = 0u; i < TEMP_CANDIDATES; i++) {
        CHECK(set_temp_path(temporary, sizeof temporary, first_destination, i));
        CHECK(file_exists(temporary) == 0);
    }

    status = schway_host_card_test_save_atomic(first_destination, second, SCHWAY_MEMCARD_SIZE,
                                               SCHWAY_HOST_CARD_TEST_FAILURE_RENAME);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_ERR_IO);
    CHECK(read_file(first_destination, readback, SCHWAY_MEMCARD_SIZE, &readback_size));
    CHECK(memcmp(readback, first, SCHWAY_MEMCARD_SIZE) == 0);
    for (unsigned i = 0u; i < TEMP_CANDIDATES; i++) {
        CHECK(set_temp_path(temporary, sizeof temporary, first_destination, i));
        CHECK(file_exists(temporary) == 0);
    }

    status = schway_host_card_export_atomic(wrapped_destination, first, SCHWAY_MEMCARD_SIZE,
                                            SCHWAY_HOST_CARD_FORMAT_GME);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_OK);
    have_wrapped_destination = status == SCHWAY_HOST_CARD_OK;
    size_t wrapped_size = 0u;
    CHECK(read_file(wrapped_destination, wrapped_previous, gme_size, &wrapped_size));
    CHECK_EQ_U64(wrapped_size, gme_size);

    status = schway_host_card_test_export_atomic(
        wrapped_destination, second, SCHWAY_MEMCARD_SIZE, SCHWAY_HOST_CARD_FORMAT_GME,
        SCHWAY_HOST_CARD_TEST_FAILURE_FLUSH);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_ERR_IO);
    check_file_unchanged(wrapped_destination, wrapped_previous, gme_size,
                         wrapped_readback, gme_size);
    for (unsigned i = 0u; i < TEMP_CANDIDATES; i++) {
        CHECK(set_temp_path(temporary, sizeof temporary, wrapped_destination, i));
        CHECK(file_exists(temporary) == 0);
    }

    status = schway_host_card_test_export_atomic(
        wrapped_destination, second, SCHWAY_MEMCARD_SIZE, SCHWAY_HOST_CARD_FORMAT_GME,
        SCHWAY_HOST_CARD_TEST_FAILURE_RENAME);
    CHECK_EQ_U64(status, SCHWAY_HOST_CARD_ERR_IO);
    check_file_unchanged(wrapped_destination, wrapped_previous, gme_size,
                         wrapped_readback, gme_size);
    for (unsigned i = 0u; i < TEMP_CANDIDATES; i++) {
        CHECK(set_temp_path(temporary, sizeof temporary, wrapped_destination, i));
        CHECK(file_exists(temporary) == 0);
    }

cleanup:
    if (have_temp_marker) {
        (void)remove(temporary);
    }
    if (have_exact_source) {
        (void)remove(exact_source);
    }
    if (have_empty_source) {
        (void)remove(empty_source);
    }
    if (have_oversized_source) {
        (void)remove(oversized_source);
    }
    if (have_first_destination) {
        (void)remove(first_destination);
    }
    if (have_second_destination) {
        (void)remove(second_destination);
    }
    if (have_wrapped_destination) {
        (void)remove(wrapped_destination);
    }
    if (made_second_directory) {
        (void)remove_directory(second_directory);
    }
    if (made_first_directory) {
        (void)remove_directory(first_directory);
    }
    free(readback);
    free(wrapped_readback);
    free(wrapped_previous);
    free(out);
    free(oversized);
    free(second);
    free(first);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "expected one known card file test name\n");
        return 2;
    }
    if (strcmp(argv[1], "atomic_roundtrip") == 0) {
        atomic_roundtrip();
    } else if (strcmp(argv[1], "atomic_failures") == 0) {
        atomic_failures();
    } else if (strcmp(argv[1], "wrappers") == 0) {
        wrappers();
    } else if (strcmp(argv[1], "wrapper_edges") == 0) {
        wrapper_edges();
    } else {
        fprintf(stderr, "unknown card file test name\n");
        return 2;
    }
    CHECK_DONE();
}
