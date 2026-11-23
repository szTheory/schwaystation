#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disc_image.h"
#include "disc_sbi.h"
#include "../check.h"

static int make_path(char path[1024], const char *directory, const char *name)
{
    int size = snprintf(path, 1024u, "%s/%s", directory, name);
    return size >= 0 && (size_t)size < 1024u;
}

static int write_bytes(const char *path, const uint8_t *bytes, size_t size)
{
    FILE *file = fopen(path, "wb");
    if (file == NULL) return 0;
    int failed = fwrite(bytes, 1u, size, file) != size;
    if (fclose(file) != 0) failed = 1;
    return failed == 0;
}

static int write_test_iso(const char *path, uint32_t sectors, uint8_t marker)
{
    FILE *file = fopen(path, "wb");
    uint8_t sector[2048];
    if (file == NULL) return 0;
    memset(sector, marker, sizeof sector);
    for (uint32_t i = 0u; i < sectors; i++) {
        if (fwrite(sector, 1u, sizeof sector, file) != sizeof sector) {
            (void)fclose(file);
            return 0;
        }
    }
    return fclose(file) == 0;
}

static void test_provider_binding(const char *directory)
{
    static const uint8_t overlay_a[] = {
        'S', 'B', 'I', 0,
        0x00u, 0x02u, 0x05u, 0x02u, 0xFAu, 0xFAu, 0xFAu,
        0x00u, 0x02u, 0x07u, 0x03u, 0xFAu, 0xFAu, 0xFAu
    };
    static const uint8_t overlay_b[] = {
        'S', 'B', 'I', 0,
        0x00u, 0x02u, 0x05u, 0x01u,
        0xFAu, 0xFAu, 0xFAu, 0xFAu, 0xFAu,
        0xFAu, 0xFAu, 0xFAu, 0xFAu, 0xFAu,
        0x00u, 0x02u, 0x06u, 0x02u, 0xFAu, 0xFAu, 0xFAu,
        0x00u, 0x02u, 0x16u, 0x03u, 0xFAu, 0xFAu, 0xFAu
    };
    static const uint8_t malformed[] = {'N', 'O', 'P', 'E'};
    static const uint8_t outside_map[] = {
        'S', 'B', 'I', 0, 0x00u, 0x02u, 0x16u, 0x02u, 0xFAu, 0xFAu, 0xFAu
    };
    static const uint8_t cue_overlay[] = {
        'S', 'B', 'I', 0, 0x00u, 0x02u, 0x02u, 0x02u, 0xFAu, 0xFAu, 0xFAu
    };
    static const char playlist[] = "schway-sbi-provider-a.iso\n"
                                   "schway-sbi-provider-b.iso\n";
    static const char cue[] = "FILE \"schway-sbi-provider.cue.bin\" BINARY\n"
                              "  TRACK 01 MODE1/2048\n"
                              "    INDEX 01 00:00:00\n";
    char a_path[1024] = "";
    char b_path[1024] = "";
    char a_sbi[1024] = "";
    char b_sbi[1024] = "";
    char m3u_path[1024] = "";
    char cue_path[1024] = "";
    char cue_data_path[1024] = "";
    char cue_sbi[1024] = "";
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    const schway_disc *disc_a = NULL;
    const schway_disc *disc_b = NULL;
    schway_toc toc_a;
    schway_toc toc_b;

    CHECK(make_path(a_path, directory, "schway-sbi-provider-a.iso"));
    CHECK(make_path(b_path, directory, "schway-sbi-provider-b.iso"));
    CHECK(make_path(a_sbi, directory, "schway-sbi-provider-a.sbi"));
    CHECK(make_path(b_sbi, directory, "schway-sbi-provider-b.sbi"));
    CHECK(make_path(m3u_path, directory, "schway-sbi-provider.m3u"));
    CHECK(make_path(cue_path, directory, "schway-sbi-provider.cue"));
    CHECK(make_path(cue_data_path, directory, "schway-sbi-provider.cue.bin"));
    CHECK(make_path(cue_sbi, directory, "schway-sbi-provider.sbi"));
    if (a_path[0] == '\0' || b_path[0] == '\0' || a_sbi[0] == '\0' ||
        b_sbi[0] == '\0' || m3u_path[0] == '\0' || cue_path[0] == '\0' ||
        cue_data_path[0] == '\0' || cue_sbi[0] == '\0') goto cleanup;
    CHECK(write_test_iso(a_path, 16u, 0x11u));
    CHECK(write_test_iso(b_path, 17u, 0x71u));
    CHECK(schway_disc_image_open(a_path, &image, &error) == SCHWAY_DISC_IMAGE_OK);
    if (image != NULL) {
        const schway_disc *plain = schway_disc_image_disc(image);
        CHECK(plain != NULL && plain->bad_q == NULL);
        schway_disc_image_close(image);
        image = NULL;
    }
    CHECK(write_test_iso(cue_data_path, 16u, 0x31u));
    CHECK(write_bytes(cue_path, (const uint8_t *)cue, sizeof cue - 1u));
    CHECK(write_bytes(cue_sbi, cue_overlay, sizeof cue_overlay));
    CHECK(schway_disc_image_open(cue_path, &image, &error) == SCHWAY_DISC_IMAGE_OK);
    if (image != NULL) {
        const schway_disc *cue_disc = schway_disc_image_disc(image);
        CHECK(cue_disc != NULL && cue_disc->bad_q != NULL);
        if (cue_disc != NULL && cue_disc->bad_q != NULL) {
            CHECK_EQ_U64(cue_disc->bad_q(cue_disc->user, 2u), 1u);
            CHECK_EQ_U64(cue_disc->bad_q(cue_disc->user, 3u), 0u);
        }
        schway_disc_image_close(image);
        image = NULL;
    }
    CHECK(write_bytes(a_sbi, overlay_a, sizeof overlay_a));
    CHECK(write_bytes(b_sbi, overlay_b, sizeof overlay_b));
    CHECK(write_bytes(m3u_path, (const uint8_t *)playlist, sizeof playlist - 1u));
    CHECK(schway_disc_image_open(m3u_path, &image, &error) == SCHWAY_DISC_IMAGE_OK);
    if (image != NULL) {
        CHECK(schway_disc_image_select(image, 0u, &disc_a) == SCHWAY_DISC_IMAGE_OK);
        CHECK(schway_disc_image_select(image, 1u, &disc_b) == SCHWAY_DISC_IMAGE_OK);
        CHECK(disc_a != NULL && disc_b != NULL && disc_a != disc_b);
        if (disc_a != NULL && disc_b != NULL) {
            CHECK(disc_a->bad_q != NULL && disc_b->bad_q != NULL);
            memset(&toc_a, 0, sizeof toc_a);
            memset(&toc_b, 0, sizeof toc_b);
            toc_a.size = (uint32_t)sizeof toc_a;
            toc_b.size = (uint32_t)sizeof toc_b;
            CHECK(disc_a->toc(disc_a->user, &toc_a) == 1u);
            CHECK(disc_b->toc(disc_b->user, &toc_b) == 1u);
            CHECK_EQ_U64(toc_a.tracks[0].leadout, 16u);
            CHECK_EQ_U64(toc_b.tracks[0].leadout, 17u);
            if (disc_a->bad_q != NULL && disc_b->bad_q != NULL) {
                CHECK_EQ_U64(disc_a->bad_q(disc_a->user, 5u), 1u);
                CHECK_EQ_U64(disc_a->bad_q(disc_a->user, 7u), 1u);
                CHECK_EQ_U64(disc_a->bad_q(disc_a->user, 6u), 0u);
                CHECK_EQ_U64(disc_b->bad_q(disc_b->user, 5u), 1u);
                CHECK_EQ_U64(disc_b->bad_q(disc_b->user, 6u), 1u);
                CHECK_EQ_U64(disc_b->bad_q(disc_b->user, 7u), 0u);
                CHECK_EQ_U64(disc_b->bad_q(disc_b->user, 16u), 1u);
                CHECK_EQ_U64(disc_a->bad_q(disc_a->user, 16u), 0u);
            }
        }
        schway_disc_image_close(image);
        image = NULL;
    }
    CHECK(write_bytes(b_sbi, malformed, sizeof malformed));
    CHECK(schway_disc_image_open(m3u_path, &image, &error) == SCHWAY_DISC_IMAGE_BAD_IMAGE &&
          image == NULL);
    CHECK(write_bytes(a_sbi, outside_map, sizeof outside_map));
    CHECK(schway_disc_image_open(a_path, &image, &error) == SCHWAY_DISC_IMAGE_BAD_IMAGE &&
          image == NULL);

cleanup:
    schway_disc_image_close(image);
    if (a_sbi[0] != '\0') (void)remove(a_sbi);
    if (b_sbi[0] != '\0') (void)remove(b_sbi);
    if (a_path[0] != '\0') (void)remove(a_path);
    if (b_path[0] != '\0') (void)remove(b_path);
    if (m3u_path[0] != '\0') (void)remove(m3u_path);
    if (cue_path[0] != '\0') (void)remove(cue_path);
    if (cue_data_path[0] != '\0') (void)remove(cue_data_path);
    if (cue_sbi[0] != '\0') (void)remove(cue_sbi);
}

static void test_format_one(void)
{
    static const uint8_t bytes[] = {
        'S', 'B', 'I', 0,
        0x00u, 0x03u, 0x00u, 0x01u,
        0x41u, 0x01u, 0x01u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x03u, 0x00u
    };
    schway_disc_sbi overlay = {0};
    CHECK_EQ_U64(schway_disc_sbi_parse(bytes, sizeof bytes, 100u, &overlay),
                 SCHWAY_DISC_SBI_OK);
    CHECK_EQ_U64(overlay.count, 1u);
    if (overlay.count == 1u) CHECK_EQ_U64(overlay.lbas[0], 75u);
    schway_disc_sbi_clear(&overlay);
}

static void test_bad_header_is_atomic(void)
{
    static const uint8_t bytes[] = {
        'X', 'B', 'I', 0,
        0x00u, 0x03u, 0x00u, 0x01u,
        0x41u, 0x01u, 0x01u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x03u, 0x00u
    };
    schway_disc_sbi overlay = {0};
    CHECK_EQ_U64(schway_disc_sbi_parse(bytes, sizeof bytes, 100u, &overlay),
                 SCHWAY_DISC_SBI_BAD_HEADER);
    CHECK(overlay.lbas == NULL);
    CHECK_EQ_U64(overlay.count, 0u);
}

static void test_truncated_record_is_atomic(void)
{
    static const uint8_t bytes[] = {
        'S', 'B', 'I', 0,
        0x00u, 0x03u, 0x00u, 0x01u,
        0x41u, 0x01u, 0x01u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x03u
    };
    schway_disc_sbi overlay = {0};
    CHECK_EQ_U64(schway_disc_sbi_parse(bytes, sizeof bytes, 100u, &overlay),
                 SCHWAY_DISC_SBI_TRUNCATED);
    CHECK(overlay.lbas == NULL);
    CHECK_EQ_U64(overlay.count, 0u);
    for (size_t size = 5u; size < sizeof bytes; size++) {
        CHECK_EQ_U64(schway_disc_sbi_parse(bytes, size, 100u, &overlay),
                     SCHWAY_DISC_SBI_TRUNCATED);
    }
}

static void test_formats_two_and_three(void)
{
    static const uint8_t bytes[] = {
        'S', 'B', 'I', 0,
        0x00u, 0x03u, 0x00u, 0x02u, 0xFAu, 0xFFu, 0x7Fu,
        0x00u, 0x04u, 0x00u, 0x03u, 0xFFu, 0xFAu, 0x7Fu
    };
    schway_disc_sbi overlay = {0};
    CHECK_EQ_U64(schway_disc_sbi_parse(bytes, sizeof bytes, 200u, &overlay),
                 SCHWAY_DISC_SBI_OK);
    CHECK_EQ_U64(overlay.count, 2u);
    if (overlay.count == 2u) {
        CHECK_EQ_U64(overlay.lbas[0], 75u);
        CHECK_EQ_U64(overlay.lbas[1], 150u);
    }
    schway_disc_sbi_clear(&overlay);
}

static void test_duplicate_records_are_sorted_and_idempotent(void)
{
    static const uint8_t bytes[] = {
        'S', 'B', 'I', 0,
        0x00u, 0x04u, 0x00u, 0x01u,
        0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu,
        0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu,
        0x00u, 0x03u, 0x00u, 0x02u, 0xFAu, 0xFAu, 0xFAu,
        0x00u, 0x04u, 0x00u, 0x03u, 0xFFu, 0xFFu, 0xFFu
    };
    schway_disc_sbi overlay = {0};
    CHECK_EQ_U64(schway_disc_sbi_parse(bytes, sizeof bytes, 200u, &overlay),
                 SCHWAY_DISC_SBI_OK);
    CHECK_EQ_U64(overlay.count, 2u);
    if (overlay.count == 2u) {
        CHECK_EQ_U64(overlay.lbas[0], 75u);
        CHECK_EQ_U64(overlay.lbas[1], 150u);
    }
    schway_disc_sbi_clear(&overlay);
}

static void test_invalid_addresses_and_bounds(void)
{
    static const uint8_t header[] = {'S', 'B', 'I', 0};
    static const uint8_t bad_bcd[] = {
        'S', 'B', 'I', 0, 0x0Au, 0x03u, 0x00u, 0x02u, 0x01u, 0x02u, 0x03u
    };
    static const uint8_t bad_seconds[] = {
        'S', 'B', 'I', 0, 0x00u, 0x60u, 0x00u, 0x02u, 0x01u, 0x02u, 0x03u
    };
    static const uint8_t bad_frames[] = {
        'S', 'B', 'I', 0, 0x00u, 0x03u, 0x75u, 0x02u, 0x01u, 0x02u, 0x03u
    };
    static const uint8_t before_leadin[] = {
        'S', 'B', 'I', 0, 0x00u, 0x01u, 0x74u, 0x02u, 0x01u, 0x02u, 0x03u
    };
    static const uint8_t outside_map[] = {
        'S', 'B', 'I', 0, 0x00u, 0x04u, 0x00u, 0x02u, 0x01u, 0x02u, 0x03u
    };
    static const uint8_t unknown_format[] = {
        'S', 'B', 'I', 0, 0x00u, 0x03u, 0x00u, 0x04u, 0x01u, 0x02u, 0x03u
    };
    static const uint8_t short_format_two[] = {
        'S', 'B', 'I', 0, 0x00u, 0x03u, 0x00u, 0x02u, 0x01u, 0x02u
    };
    static const uint8_t empty = 0u;
    schway_disc_sbi overlay = {0};
    CHECK_EQ_U64(schway_disc_sbi_parse(&empty, 0u, 100u, &overlay),
                 SCHWAY_DISC_SBI_BAD_HEADER);
    CHECK_EQ_U64(schway_disc_sbi_parse(header, sizeof header, 100u, &overlay),
                 SCHWAY_DISC_SBI_OK);
    CHECK(overlay.lbas == NULL);
    CHECK_EQ_U64(overlay.count, 0u);
    CHECK_EQ_U64(schway_disc_sbi_parse(bad_bcd, sizeof bad_bcd, 200u, &overlay),
                 SCHWAY_DISC_SBI_BAD_RECORD);
    CHECK_EQ_U64(schway_disc_sbi_parse(bad_seconds, sizeof bad_seconds, 200u, &overlay),
                 SCHWAY_DISC_SBI_BAD_RECORD);
    CHECK_EQ_U64(schway_disc_sbi_parse(bad_frames, sizeof bad_frames, 200u, &overlay),
                 SCHWAY_DISC_SBI_BAD_RECORD);
    CHECK_EQ_U64(schway_disc_sbi_parse(before_leadin, sizeof before_leadin, 200u, &overlay),
                 SCHWAY_DISC_SBI_BAD_RECORD);
    CHECK_EQ_U64(schway_disc_sbi_parse(outside_map, sizeof outside_map, 100u, &overlay),
                 SCHWAY_DISC_SBI_OUT_OF_RANGE);
    CHECK_EQ_U64(schway_disc_sbi_parse(unknown_format, sizeof unknown_format, 200u, &overlay),
                 SCHWAY_DISC_SBI_BAD_RECORD);
    CHECK_EQ_U64(schway_disc_sbi_parse(short_format_two, sizeof short_format_two, 200u,
                                       &overlay), SCHWAY_DISC_SBI_TRUNCATED);
    for (size_t size = 5u; size < sizeof short_format_two; size++) {
        CHECK_EQ_U64(schway_disc_sbi_parse(short_format_two, size, 200u, &overlay),
                     SCHWAY_DISC_SBI_TRUNCATED);
    }
    CHECK(overlay.lbas == NULL);
    CHECK_EQ_U64(overlay.count, 0u);
}

static void test_file_and_record_limits(void)
{
    size_t size = (size_t)SCHWAY_DISC_SBI_MAX_BYTES + 1u;
    uint8_t *bytes = (uint8_t *)calloc(size, 1u);
    schway_disc_sbi overlay = {0};
    CHECK(bytes != NULL);
    if (bytes == NULL) return;
    memcpy(bytes, "SBI\0", 4u);
    CHECK_EQ_U64(schway_disc_sbi_parse(bytes, size, 200u, &overlay),
                 SCHWAY_DISC_SBI_LIMIT);
    free(bytes);

    size = 4u + (size_t)(SCHWAY_DISC_SBI_MAX_RECORDS + 1u) * 7u;
    bytes = (uint8_t *)malloc(size);
    CHECK(bytes != NULL);
    if (bytes == NULL) return;
    memcpy(bytes, "SBI\0", 4u);
    for (size_t i = 0u; i < SCHWAY_DISC_SBI_MAX_RECORDS + 1u; i++) {
        size_t at = 4u + i * 7u;
        bytes[at] = 0x00u;
        bytes[at + 1u] = 0x03u;
        bytes[at + 2u] = 0x00u;
        bytes[at + 3u] = 0x02u;
        bytes[at + 4u] = 0xFFu;
        bytes[at + 5u] = 0xFFu;
        bytes[at + 6u] = 0xFFu;
    }
    CHECK_EQ_U64(schway_disc_sbi_parse(bytes, size, 200u, &overlay),
                 SCHWAY_DISC_SBI_LIMIT);
    free(bytes);
    CHECK(overlay.lbas == NULL);
    CHECK_EQ_U64(overlay.count, 0u);
}

int main(int argc, char **argv)
{
    if (argc == 2) test_provider_binding(argv[1]);
    else if (argc != 1) return 2;
    test_format_one();
    test_bad_header_is_atomic();
    test_truncated_record_is_atomic();
    test_formats_two_and_three();
    test_duplicate_records_are_sorted_and_idempotent();
    test_invalid_addresses_and_bounds();
    test_file_and_record_limits();
    CHECK_DONE();
}
