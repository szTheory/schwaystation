#include "disc_chd.h"
#include "disc_chd_metadata.h"

#include <stdio.h>
#include <string.h>

#include "../test_open.h"

#define CHECK(condition, message) do { \
    if (!(condition)) { \
        fprintf(stderr, "ASSERT disc.image.chd_headers: %s\n", (message)); \
        result = 1; \
        goto cleanup; \
    } \
} while (0)

static int make_named_path(char path[1024], const char *directory, const char *name)
{
    int size = snprintf(path, 1024u, "%s/%s", directory, name);
    return size >= 0 && (size_t)size < 1024u;
}

static int make_path(char path[1024], const char *directory)
{
    return make_named_path(path, directory, "chd-header-test.bin");
}

#if SCHWAYSTATION_CHD
static int write_bytes(const char *path, const uint8_t *bytes, size_t size)
{
    FILE *file = schway__test_open_file(path, "wb");
    if (file == NULL) return 0;
    if (size != 0u && fwrite(bytes, 1u, size, file) != size) {
        (void)fclose(file);
        return 0;
    }
    return fclose(file) == 0;
}

static void put_be32(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)(value >> 24);
    bytes[1] = (uint8_t)(value >> 16);
    bytes[2] = (uint8_t)(value >> 8);
    bytes[3] = (uint8_t)value;
}

static void put_be64(uint8_t *bytes, uint64_t value)
{
    put_be32(bytes, (uint32_t)(value >> 32));
    put_be32(bytes + 4, (uint32_t)value);
}
#endif

static int run_metadata_controls(void)
{
    static const char track1[] =
        "TRACK:1 TYPE:MODE2_RAW SUBTYPE:NONE FRAMES:64 PREGAP:0 PGTYPE:MODE1 PGSUB:NONE POSTGAP:0";
    static const char track2[] =
        "TRACK:2 TYPE:AUDIO SUBTYPE:NONE FRAMES:158 PREGAP:150 PGTYPE:VAUDIO PGSUB:NONE POSTGAP:0";
    static const char virtual_gap[] =
        "TRACK:1 TYPE:AUDIO SUBTYPE:NONE FRAMES:8 PREGAP:150 PGTYPE:AUDIO PGSUB:NONE POSTGAP:0";
    static const char legacy[] = "TRACK:1 TYPE:MODE2_RAW SUBTYPE:NONE FRAMES:64";
    static const char bad_mode[] =
        "TRACK:1 TYPE:MODE2_MIX SUBTYPE:NONE FRAMES:64 PREGAP:0 PGTYPE:MODE1 PGSUB:NONE POSTGAP:0";
    static const char bad_subcode[] =
        "TRACK:1 TYPE:MODE2_RAW SUBTYPE:RW FRAMES:64 PREGAP:0 PGTYPE:MODE1 PGSUB:NONE POSTGAP:0";
    static const char bad_postgap[] =
        "TRACK:1 TYPE:MODE2_RAW SUBTYPE:NONE FRAMES:64 PREGAP:0 PGTYPE:MODE1 PGSUB:NONE POSTGAP:1";
    static const char bad_frames[] =
        "TRACK:1 TYPE:MODE2_RAW SUBTYPE:NONE FRAMES:4294967296 PREGAP:0 PGTYPE:MODE1 PGSUB:NONE POSTGAP:0";
    static const char track2_duplicate[] =
        "TRACK:1 TYPE:AUDIO SUBTYPE:NONE FRAMES:158 PREGAP:150 PGTYPE:VAUDIO PGSUB:NONE POSTGAP:0";
    static const char track1_out_of_order[] =
        "TRACK:2 TYPE:MODE2_RAW SUBTYPE:NONE FRAMES:64 PREGAP:0 PGTYPE:MODE1 PGSUB:NONE POSTGAP:0";
    char max_texts[SCHWAY_DISC_MAX_TRACKS][96];
    char unterminated[sizeof track1];
    schway__disc_chd_metadata_record records[2] = {
        { SCHWAY__DISC_CHD_TAG_CHT2, track1, (uint32_t)sizeof track1 },
        { SCHWAY__DISC_CHD_TAG_CHT2, track2, (uint32_t)sizeof track2 }
    };
    schway__disc_chd_metadata_map map;
    schway__disc_chd_metadata_record one;
    schway__disc_chd_metadata_record max_records[SCHWAY_DISC_MAX_TRACKS];
    schway__disc_chd_metadata_record too_many[SCHWAY_DISC_MAX_TRACKS + 1u];
    uint32_t frame;
    uint32_t track_type;
    uint32_t virtual_pregap;

#define META_CHECK(condition, message) do { \
    if (!(condition)) { \
        fprintf(stderr, "ASSERT disc.image.chd_metadata: %s\n", (message)); \
        return 1; \
    } \
} while (0)

    META_CHECK(schway__disc_chd_metadata_parse(records, 2u, 5u, 19584u,
              UINT64_C(548352), &map) == SCHWAY_DISC_IMAGE_OK,
              "owned CHT2 metadata did not parse");
    META_CHECK(map.track_count == 2u && map.chd_frame_count == 224u &&
              map.tracks[0].toc.type == SCHWAY_DISC_TRACK_MODE2 &&
              map.tracks[0].toc.index0 == 0u && map.tracks[0].toc.index1 == 0u &&
              map.tracks[0].toc.leadout == 64u && map.tracks[0].chd_frame_start == 0u &&
              map.tracks[0].chd_frame_count == 64u && map.tracks[0].stored_pregap == 0u &&
              map.tracks[1].toc.type == SCHWAY_DISC_TRACK_AUDIO &&
              map.tracks[1].toc.index0 == 64u && map.tracks[1].toc.index1 == 214u &&
              map.tracks[1].toc.pregap == 150u && map.tracks[1].toc.leadout == 222u &&
              map.tracks[1].chd_frame_start == 64u &&
              map.tracks[1].chd_frame_count == 158u &&
              map.tracks[1].stored_pregap == 1u,
              "CHT2 padding or stored pregap changed the logical TOC");
    META_CHECK(schway__disc_chd_map_lba(&map, 64u, &frame, &track_type,
              &virtual_pregap) != 0u && frame == 64u &&
              track_type == SCHWAY_DISC_TRACK_AUDIO && virtual_pregap == 0u &&
              schway__disc_chd_map_lba(&map, 213u, &frame, &track_type,
              &virtual_pregap) != 0u && frame == 213u && virtual_pregap == 0u &&
              schway__disc_chd_map_lba(&map, 214u, &frame, &track_type,
              &virtual_pregap) != 0u && frame == 214u && virtual_pregap == 0u &&
              schway__disc_chd_map_lba(&map, 221u, &frame, &track_type,
              &virtual_pregap) != 0u && frame == 221u && virtual_pregap == 0u &&
              schway__disc_chd_map_lba(&map, 222u, &frame, &track_type,
              &virtual_pregap) == 0u,
              "stored pregap and leadout did not map to exact CHD frames");

    one = (schway__disc_chd_metadata_record){
        SCHWAY__DISC_CHD_TAG_CHT2, virtual_gap, (uint32_t)sizeof virtual_gap
    };
    META_CHECK(schway__disc_chd_metadata_parse(&one, 1u, 4u, 19584u,
              UINT64_C(19584), &map) == SCHWAY_DISC_IMAGE_OK &&
              map.tracks[0].toc.index0 == 0u && map.tracks[0].toc.index1 == 150u &&
              map.tracks[0].toc.leadout == 158u && map.tracks[0].stored_pregap == 0u,
              "virtual pregap was counted as stored frames");
    META_CHECK(schway__disc_chd_map_lba(&map, 0u, &frame, &track_type,
              &virtual_pregap) != 0u && track_type == SCHWAY_DISC_TRACK_AUDIO &&
              virtual_pregap == 1u && schway__disc_chd_map_lba(&map, 149u,
              &frame, &track_type, &virtual_pregap) != 0u && virtual_pregap == 1u &&
              schway__disc_chd_map_lba(&map, 150u, &frame, &track_type,
              &virtual_pregap) != 0u && frame == 0u && virtual_pregap == 0u &&
              schway__disc_chd_map_lba(&map, 157u, &frame, &track_type,
              &virtual_pregap) != 0u && frame == 7u && virtual_pregap == 0u &&
              schway__disc_chd_map_lba(&map, 158u, &frame, &track_type,
              &virtual_pregap) == 0u,
              "virtual pregap frames or data extent mapped incorrectly");

    one = (schway__disc_chd_metadata_record){
        SCHWAY__DISC_CHD_TAG_CHTR, legacy, (uint32_t)sizeof legacy
    };
    META_CHECK(schway__disc_chd_metadata_parse(&one, 1u, 3u, 19584u,
              UINT64_C(156672), &map) == SCHWAY_DISC_IMAGE_OK &&
              map.tracks[0].toc.index0 == map.tracks[0].toc.index1 &&
              map.tracks[0].toc.pregap == 0u && map.tracks[0].toc.leadout == 64u,
              "bounded CHTR metadata did not use its no-gap layout");

    for (uint32_t i = 0u; i < SCHWAY_DISC_MAX_TRACKS; i++) {
        int length = snprintf(max_texts[i], sizeof max_texts[i],
            "TRACK:%u TYPE:AUDIO SUBTYPE:NONE FRAMES:4", i + 1u);
        if (length < 0 || (size_t)length >= sizeof max_texts[i]) {
            fprintf(stderr, "ASSERT disc.image.chd_metadata: could not build max-track controls\n");
            return 1;
        }
        max_records[i].tag = SCHWAY__DISC_CHD_TAG_CHTR;
        max_records[i].text = max_texts[i];
        max_records[i].length = (uint32_t)length + 1u;
    }
    META_CHECK(schway__disc_chd_metadata_parse(max_records, SCHWAY_DISC_MAX_TRACKS,
              4u, 19584u, UINT64_C(969408), &map) == SCHWAY_DISC_IMAGE_OK &&
              map.track_count == SCHWAY_DISC_MAX_TRACKS &&
              map.tracks[SCHWAY_DISC_MAX_TRACKS - 1u].toc.leadout == 396u,
              "the exact 99-track limit was rejected");

    META_CHECK(schway__disc_chd_metadata_parse(NULL, 0u, 5u, 19584u,
              UINT64_C(19584), &map) == SCHWAY_DISC_IMAGE_UNSUPPORTED,
              "a CHD with no CD track records was accepted");
    META_CHECK(schway__disc_chd_metadata_parse(records, 2u, 2u, 19584u,
              UINT64_C(548352), &map) == SCHWAY_DISC_IMAGE_UNSUPPORTED,
              "legacy header without supported CD metadata was accepted");
    for (size_t i = 0u; i < SCHWAY_DISC_MAX_TRACKS + 1u; i++) too_many[i] = records[0];
    META_CHECK(schway__disc_chd_metadata_parse(too_many, SCHWAY_DISC_MAX_TRACKS + 1u,
              5u, 19584u, UINT64_C(548352), &map) == SCHWAY_DISC_IMAGE_LIMIT,
              "over-limit track count was accepted");

    records[0].text = track1_out_of_order;
    records[0].length = (uint32_t)sizeof track1_out_of_order;
    META_CHECK(schway__disc_chd_metadata_parse(records, 2u, 5u, 19584u,
              UINT64_C(548352), &map) == SCHWAY_DISC_IMAGE_BAD_IMAGE,
              "out-of-order track number was accepted");
    records[0].text = track1;
    records[0].length = (uint32_t)sizeof track1;
    records[1].text = track2_duplicate;
    records[1].length = (uint32_t)sizeof track2_duplicate;
    META_CHECK(schway__disc_chd_metadata_parse(records, 2u, 5u, 19584u,
              UINT64_C(548352), &map) == SCHWAY_DISC_IMAGE_BAD_IMAGE,
              "duplicate track number was accepted");
    records[1].text = track2;
    records[1].length = (uint32_t)sizeof track2;
    records[1].tag = SCHWAY__DISC_CHD_TAG_CHTR;
    META_CHECK(schway__disc_chd_metadata_parse(records, 2u, 5u, 19584u,
              UINT64_C(548352), &map) == SCHWAY_DISC_IMAGE_UNSUPPORTED,
              "mixed metadata layouts were accepted");
    records[1].tag = SCHWAY__DISC_CHD_TAG_CHT2;

    one = (schway__disc_chd_metadata_record){
        SCHWAY__DISC_CHD_TAG_CHT2, track1, (uint32_t)sizeof track1
    };
    memcpy(unterminated, track1, sizeof track1);
    unterminated[sizeof unterminated - 1u] = 'x';
    one.text = unterminated;
    META_CHECK(schway__disc_chd_metadata_parse(&one, 1u, 5u, 19584u,
              UINT64_C(156672), &map) == SCHWAY_DISC_IMAGE_BAD_IMAGE,
              "unterminated metadata was accepted");
    one.tag = UINT32_C(0x43484744);
    META_CHECK(schway__disc_chd_metadata_parse(&one, 1u, 5u, 19584u,
              UINT64_C(156672), &map) == SCHWAY_DISC_IMAGE_UNSUPPORTED,
              "unsupported non-CD metadata was accepted");

    one = (schway__disc_chd_metadata_record){
        SCHWAY__DISC_CHD_TAG_CHT2, bad_mode, (uint32_t)sizeof bad_mode
    };
    META_CHECK(schway__disc_chd_metadata_parse(&one, 1u, 5u, 19584u,
              UINT64_C(156672), &map) == SCHWAY_DISC_IMAGE_UNSUPPORTED,
              "unsupported track mode was accepted");
    one.text = bad_subcode;
    one.length = (uint32_t)sizeof bad_subcode;
    META_CHECK(schway__disc_chd_metadata_parse(&one, 1u, 5u, 19584u,
              UINT64_C(156672), &map) == SCHWAY_DISC_IMAGE_UNSUPPORTED,
              "unrepresented subcode was accepted");
    one.text = bad_postgap;
    one.length = (uint32_t)sizeof bad_postgap;
    META_CHECK(schway__disc_chd_metadata_parse(&one, 1u, 5u, 19584u,
              UINT64_C(156672), &map) == SCHWAY_DISC_IMAGE_UNSUPPORTED,
              "postgap semantics were reconstructed");
    one.text = bad_frames;
    one.length = (uint32_t)sizeof bad_frames;
    META_CHECK(schway__disc_chd_metadata_parse(&one, 1u, 5u, 19584u,
              UINT64_C(156672), &map) == SCHWAY_DISC_IMAGE_BAD_IMAGE,
              "overflowing frame count was accepted");
    one.text = "TRACK:1 TYPE:MODE2_RAW SUBTYPE:NONE FRAMES:4294967295 PREGAP:0 PGTYPE:MODE1 PGSUB:NONE POSTGAP:0";
    one.length = (uint32_t)strlen(one.text) + 1u;
    META_CHECK(schway__disc_chd_metadata_parse(&one, 1u, 5u, 19584u,
              UINT64_C(156672), &map) == SCHWAY_DISC_IMAGE_LIMIT,
              "maximum frame-count arithmetic overflow was accepted");
    one.text = track1;
    one.length = (uint32_t)sizeof track1;
    META_CHECK(schway__disc_chd_metadata_parse(&one, 1u, 5u, 19584u,
              UINT64_C(159120), &map) == SCHWAY_DISC_IMAGE_BAD_IMAGE,
              "inconsistent padded frame count was accepted");
    META_CHECK(schway__disc_chd_metadata_parse(&one, 1u, 5u, 1u,
              UINT64_C(156672), &map) == SCHWAY_DISC_IMAGE_BAD_IMAGE,
              "non-frame-aligned hunk size was accepted");
    META_CHECK(schway__disc_chd_metadata_parse(&one, 1u, 5u, 19584u,
              UINT64_MAX, &map) == SCHWAY_DISC_IMAGE_LIMIT,
              "over-limit logical size was accepted");
    puts("PASS disc.image.chd_metadata");
    return 0;
#undef META_CHECK
}

static int expect_rejection(const char *path, schway_disc_image_status expected)
{
    schway__disc_chd *reader = (schway__disc_chd *)(uintptr_t)1u;
    schway_disc_image_error error;
    schway_disc_image_status status = schway__disc_chd_open(path, &reader, &error);
    if (status != expected || reader != NULL || error.status != (uint32_t)expected) {
        schway__disc_chd_close(reader);
        return 0;
    }
    schway__disc_chd_close(NULL);
    return 1;
}

static int run_chd_toc(const char *path)
{
#if SCHWAYSTATION_CHD
    schway__disc_chd *reader = NULL;
    schway_disc_image_error error;
    schway_toc toc;
    schway_disc_image_status status;

    memset(&error, 0, sizeof error);
    status = schway__disc_chd_open(path, &reader, &error);
    if (status != SCHWAY_DISC_IMAGE_OK || reader == NULL) {
        fprintf(stderr, "ASSERT disc.image.chd_toc: open failed: %s\n", error.message);
        return 1;
    }
    memset(&toc, 0, sizeof toc);
    toc.size = (uint32_t)sizeof toc;
    status = schway__disc_chd_toc(reader, &toc);
    schway__disc_chd_close(reader);
    if (status != SCHWAY_DISC_IMAGE_OK || toc.size != sizeof toc || toc.track_count != 2u ||
        toc.tracks[0].size != sizeof toc.tracks[0] ||
        toc.tracks[1].size != sizeof toc.tracks[1] ||
        toc.tracks[0].type != SCHWAY_DISC_TRACK_MODE2 ||
        toc.tracks[0].index0 != 0u || toc.tracks[0].index1 != 0u ||
        toc.tracks[0].pregap != 0u || toc.tracks[0].leadout != 64u ||
        toc.tracks[1].type != SCHWAY_DISC_TRACK_AUDIO ||
        toc.tracks[1].index0 != 64u || toc.tracks[1].index1 != 214u ||
        toc.tracks[1].pregap != 150u || toc.tracks[1].leadout != 222u) {
        fprintf(stderr, "ASSERT disc.image.chd_toc: normalized TOC differs from the CUE oracle\n");
        return 1;
    }
#else
    if (!expect_rejection(path, SCHWAY_DISC_IMAGE_UNSUPPORTED)) {
        fprintf(stderr, "ASSERT disc.image.chd_toc: disabled build accepted CHD\n");
        return 1;
    }
#endif
    puts("PASS disc.image.chd_toc");
    return 0;
}

#if SCHWAYSTATION_CHD
static int same_toc(const schway_disc *left, const schway_disc *right)
{
    schway_toc a;
    schway_toc b;
    if (left == NULL || right == NULL || left->toc == NULL || right->toc == NULL) return 0;
    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    a.size = (uint32_t)sizeof a;
    b.size = (uint32_t)sizeof b;
    if (left->toc(left->user, &a) == 0u || right->toc(right->user, &b) == 0u ||
        a.size != sizeof a || b.size != sizeof b || a.track_count != b.track_count ||
        a.track_count == 0u || a.track_count > SCHWAY_DISC_MAX_TRACKS) return 0;
    for (uint32_t i = 0u; i < a.track_count; i++) {
        if (a.tracks[i].size != sizeof a.tracks[i] ||
            b.tracks[i].size != sizeof b.tracks[i] ||
            a.tracks[i].type != b.tracks[i].type ||
            a.tracks[i].index0 != b.tracks[i].index0 ||
            a.tracks[i].index1 != b.tracks[i].index1 ||
            a.tracks[i].pregap != b.tracks[i].pregap ||
            a.tracks[i].leadout != b.tracks[i].leadout) return 0;
    }
    return 1;
}

static int same_sector_at_lba(const schway_disc *left, const schway_disc *right,
                              uint32_t lba)
{
    uint8_t left_sector[2354];
    uint8_t right_sector[2354];
    uint8_t left_subcode[98];
    uint8_t right_subcode[98];
    memset(left_sector, 0xa5, sizeof left_sector);
    memset(right_sector, 0xa5, sizeof right_sector);
    memset(left_subcode, 0xa5, sizeof left_subcode);
    memset(right_subcode, 0xa5, sizeof right_subcode);
    if (left == NULL || right == NULL || left->read_sector == NULL ||
        right->read_sector == NULL ||
        left->read_sector(left->user, lba, left_sector + 1u, left_subcode + 1u) != 1u ||
        right->read_sector(right->user, lba, right_sector + 1u, right_subcode + 1u) != 1u ||
        left_sector[0] != 0xa5u || left_sector[2353] != 0xa5u ||
        right_sector[0] != 0xa5u || right_sector[2353] != 0xa5u ||
        left_subcode[0] != 0xa5u || left_subcode[97] != 0xa5u ||
        right_subcode[0] != 0xa5u || right_subcode[97] != 0xa5u) return 0;
    for (size_t i = 0u; i < 2352u; i++) {
        if (left_sector[i + 1u] != right_sector[i + 1u]) {
            fprintf(stderr, "sector mismatch LBA %u offset %zu CUE=%02x CHD=%02x\n",
                    lba, i, left_sector[i + 1u], right_sector[i + 1u]);
            return 0;
        }
    }
    for (size_t i = 0u; i < 96u; i++) {
        if (left_subcode[i + 1u] != right_subcode[i + 1u]) {
            fprintf(stderr, "subcode mismatch LBA %u offset %zu CUE=%02x CHD=%02x\n",
                    lba, i, left_subcode[i + 1u], right_subcode[i + 1u]);
            return 0;
        }
    }
    return 1;
}
#endif

static int run_chd_open(int argc, char **argv)
{
#if SCHWAYSTATION_CHD
    schway_disc_image *cue_image = NULL;
    schway_disc_image *chd_image = NULL;
    schway_disc_image *failed_image = NULL;
    schway_disc_image_error error;
    const schway_disc *cue_disc;
    const schway_disc *chd_disc;
    char bad_path[1024];
    uint8_t bad_header[16] = { 'M', 'C', 'o', 'm', 'p', 'r', 'H', 'D' };
    int result = 1;
    if (argc != 5) return 2;
    put_be32(bad_header + 8u, 124u);
    put_be32(bad_header + 12u, 6u);
    if (!make_named_path(bad_path, argv[4], "chd-open-invalid.chd")) return 2;
    memset(&error, 0, sizeof error);
    if (schway_disc_image_open(argv[2], &cue_image, &error) != SCHWAY_DISC_IMAGE_OK ||
        schway_disc_image_open(argv[3], &chd_image, &error) != SCHWAY_DISC_IMAGE_OK) {
        fprintf(stderr, "ASSERT disc.image.chd_open: a shared CUE or CHD open failed: %s\n",
                error.message);
        goto cleanup_open;
    }
    cue_disc = schway_disc_image_disc(cue_image);
    chd_disc = schway_disc_image_disc(chd_image);
    if (!same_toc(cue_disc, chd_disc) || !same_sector_at_lba(cue_disc, chd_disc, 0u)) {
        fprintf(stderr, "ASSERT disc.image.chd_open: public callback differs from the CUE\n");
        goto cleanup_open;
    }
    if (!write_bytes(bad_path, bad_header, sizeof bad_header) ||
        schway_disc_image_open(bad_path, &failed_image, &error) !=
            SCHWAY_DISC_IMAGE_UNSUPPORTED || failed_image != NULL ||
        !same_sector_at_lba(cue_disc, chd_disc, 0u)) {
        fprintf(stderr, "ASSERT disc.image.chd_open: failed replacement damaged active media\n");
        goto cleanup_open;
    }
    result = 0;
cleanup_open:
    schway_disc_image_close(failed_image);
    schway_disc_image_close(chd_image);
    schway_disc_image_close(cue_image);
    (void)remove(bad_path);
    if (result == 0) puts("PASS disc.image.chd_open");
    return result;
#else
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    if (argc != 3) return 2;
    if (schway_disc_image_open(argv[2], &image, &error) != SCHWAY_DISC_IMAGE_UNSUPPORTED ||
        image != NULL) {
        schway_disc_image_close(image);
        fprintf(stderr, "ASSERT disc.image.chd_open: CHD-off build accepted CHD\n");
        return 1;
    }
    schway_disc_image_close(image);
    puts("PASS disc.image.chd_open");
    return 0;
#endif
}

static int run_chd_boundaries(int argc, char **argv)
{
#if SCHWAYSTATION_CHD
    static const uint32_t fixed_lbas[] = { 0u, 63u, 64u, 213u, 214u, 215u, 221u };
    schway_disc_image *cue_image = NULL;
    schway_disc_image *chd_image = NULL;
    schway_disc_image_error error;
    const schway_disc *cue_disc;
    const schway_disc *chd_disc;
    uint8_t sector[2352];
    uint8_t subcode[96];
    int result = 1;
    if (argc != 4) return 2;
    memset(&error, 0, sizeof error);
    if (schway_disc_image_open(argv[2], &cue_image, &error) != SCHWAY_DISC_IMAGE_OK ||
        schway_disc_image_open(argv[3], &chd_image, &error) != SCHWAY_DISC_IMAGE_OK) {
        fprintf(stderr, "ASSERT disc.image.chd_boundaries: image open failed: %s\n", error.message);
        goto cleanup_boundaries;
    }
    cue_disc = schway_disc_image_disc(cue_image);
    chd_disc = schway_disc_image_disc(chd_image);
    if (!same_toc(cue_disc, chd_disc)) {
        fprintf(stderr, "ASSERT disc.image.chd_boundaries: TOCs differ\n");
        goto cleanup_boundaries;
    }
    for (size_t i = 0u; i < sizeof fixed_lbas / sizeof fixed_lbas[0]; i++) {
        if (!same_sector_at_lba(cue_disc, chd_disc, fixed_lbas[i])) {
            fprintf(stderr, "ASSERT disc.image.chd_boundaries: mismatch at LBA %u\n",
                    fixed_lbas[i]);
            goto cleanup_boundaries;
        }
    }
    for (uint32_t boundary = 8u; boundary < 222u; boundary += 8u) {
        if (!same_sector_at_lba(cue_disc, chd_disc, boundary - 1u) ||
            !same_sector_at_lba(cue_disc, chd_disc, boundary)) {
            fprintf(stderr, "ASSERT disc.image.chd_boundaries: hunk mismatch near LBA %u\n",
                    boundary);
            goto cleanup_boundaries;
        }
    }
    if (cue_disc->read_sector(cue_disc->user, 222u, sector, subcode) != 0u ||
        chd_disc->read_sector(chd_disc->user, 222u, sector, subcode) != 0u ||
        cue_disc->read_sector(cue_disc->user, UINT32_MAX, sector, subcode) != 0u ||
        chd_disc->read_sector(chd_disc->user, UINT32_MAX, sector, subcode) != 0u) {
        fprintf(stderr, "ASSERT disc.image.chd_boundaries: leadout/out-of-range read succeeded\n");
        goto cleanup_boundaries;
    }
    result = 0;
cleanup_boundaries:
    schway_disc_image_close(chd_image);
    schway_disc_image_close(cue_image);
    if (result == 0) puts("PASS disc.image.chd_boundaries");
    return result;
#else
    schway_disc_image *image = NULL;
    schway_disc_image_error error;
    if (argc != 3) return 2;
    if (schway_disc_image_open(argv[2], &image, &error) != SCHWAY_DISC_IMAGE_UNSUPPORTED ||
        image != NULL) {
        schway_disc_image_close(image);
        fprintf(stderr, "ASSERT disc.image.chd_boundaries: CHD-off build accepted CHD\n");
        return 1;
    }
    schway_disc_image_close(image);
    puts("PASS disc.image.chd_boundaries");
    return 0;
#endif
}

int main(int argc, char **argv)
{
    int result = 0;
    char path[1024];
    if (argc == 2 && strcmp(argv[1], "--metadata-controls") == 0) {
        return run_metadata_controls();
    }
    if (argc == 3 && strcmp(argv[1], "--toc") == 0) {
        return run_chd_toc(argv[2]);
    }
    if (argc >= 2 && strcmp(argv[1], "--open") == 0) return run_chd_open(argc, argv);
    if (argc >= 2 && strcmp(argv[1], "--boundaries") == 0) {
        return run_chd_boundaries(argc, argv);
    }
#if SCHWAYSTATION_CHD
    uint8_t header[124];
    static const uint8_t truncated[] = "MComprHD";
    static const uint32_t header_lengths[5] = { 76u, 80u, 120u, 108u, 124u };
#endif
    if (argc != 2 || !make_path(path, argv[1])) return 2;
#if SCHWAYSTATION_CHD
    CHECK(write_bytes(path, NULL, 0u) &&
          expect_rejection(path, SCHWAY_DISC_IMAGE_BAD_IMAGE), "empty header accepted");
    CHECK(write_bytes(path, truncated, sizeof(truncated) - 1u) &&
          expect_rejection(path, SCHWAY_DISC_IMAGE_BAD_IMAGE), "truncated header accepted");
    memset(header, 0, sizeof(header));
    put_be32(header + 8u, 124u);
    put_be32(header + 12u, 5u);
    CHECK(write_bytes(path, header, sizeof(header)) &&
          expect_rejection(path, SCHWAY_DISC_IMAGE_BAD_IMAGE), "wrong magic accepted");
    for (uint32_t version = 1u; version <= 5u; version++) {
        memset(header, 0, sizeof(header));
        memcpy(header, "MComprHD", 8u);
        put_be32(header + 8u, header_lengths[version - 1u]);
        put_be32(header + 12u, version);
        CHECK(write_bytes(path, header, header_lengths[version - 1u]) &&
              expect_rejection(path, SCHWAY_DISC_IMAGE_BAD_IMAGE),
              "a recognized v1-v5 header length was misclassified");
    }
    memset(header, 0, sizeof(header));
    memcpy(header, "MComprHD", 8u);
    put_be32(header + 8u, 124u);
    put_be32(header + 12u, 5u);
    put_be64(header + 32u, UINT64_C(2448));
    put_be32(header + 56u, UINT32_C(16777217));
    put_be32(header + 60u, 2448u);
    CHECK(write_bytes(path, header, sizeof(header)) &&
          expect_rejection(path, SCHWAY_DISC_IMAGE_LIMIT), "oversized hunk accepted");
    memset(header, 0, sizeof(header));
    memcpy(header, "MComprHD", 8u);
    put_be32(header + 8u, 124u);
    put_be32(header + 12u, 6u);
    CHECK(write_bytes(path, header, sizeof(header)) &&
          expect_rejection(path, SCHWAY_DISC_IMAGE_UNSUPPORTED), "unsupported version accepted");
    put_be32(header + 12u, 5u);
    put_be64(header + 32u, UINT64_C(1073741825));
    put_be32(header + 56u, 19584u);
    put_be32(header + 60u, 2448u);
    CHECK(write_bytes(path, header, sizeof(header)) &&
          expect_rejection(path, SCHWAY_DISC_IMAGE_LIMIT), "oversized logical geometry accepted");
    memset(header, 0, sizeof(header));
    memcpy(header, "MComprHD", 8u);
    put_be32(header + 8u, 80u);
    put_be32(header + 12u, 2u);
    put_be32(header + 24u, 1u);
    put_be32(header + 28u, UINT32_MAX);
    put_be32(header + 32u, UINT32_MAX);
    put_be32(header + 36u, UINT32_MAX);
    put_be32(header + 40u, UINT32_MAX);
    put_be32(header + 76u, 2352u);
    CHECK(write_bytes(path, header, 80u) &&
          expect_rejection(path, SCHWAY_DISC_IMAGE_LIMIT), "overflowing legacy geometry accepted");
    memset(header, 0, sizeof(header));
    memcpy(header, "MComprHD", 8u);
    put_be32(header + 8u, 124u);
    put_be32(header + 12u, 5u);
    put_be64(header + 32u, UINT64_C(1073741824));
    put_be32(header + 56u, 1u);
    put_be32(header + 60u, 1u);
    CHECK(write_bytes(path, header, sizeof(header)) &&
          expect_rejection(path, SCHWAY_DISC_IMAGE_LIMIT), "excessive hunk count accepted");
#else
    CHECK(expect_rejection("disabled-feature.chd", SCHWAY_DISC_IMAGE_UNSUPPORTED),
          "disabled CHD provider did not report unsupported");
#endif

cleanup:
    (void)remove(path);
    if (result == 0) puts("PASS disc.image.chd_headers");
    return result;
}
