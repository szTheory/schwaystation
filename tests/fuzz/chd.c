/* Bounded replay of hostile CHD bytes through the production disc provider. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disc_image.h"
#include "media_limits.h"
#include "../test_open.h"

#ifndef SCHWAY_FUZZ_WORK_DIR
#error "CMake must provide a build-owned CHD fuzz directory"
#endif

#ifndef SCHWAYSTATION_CHD
#define SCHWAYSTATION_CHD 0
#endif

#if SCHWAYSTATION_CHD
#ifndef SCHWAY_FUZZ_CHD_FIXTURE
#error "CMake must provide the generated CHD fixture path"
#endif
#endif

#define INPUT_MAX (16u * 1024u)
#define HEADER_SIZE 124u
#if SCHWAYSTATION_CHD
#define SEED_COUNT 12u
#else
#define SEED_COUNT 11u
#endif

static void broken(const char *what)
{
    fprintf(stderr, "fuzz.regress.chd: %s\n", what);
    abort();
}

#ifdef SCHWAY_FUZZ_REPLAY
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
    put_be32(bytes + 4u, (uint32_t)value);
}

static void make_v5(uint8_t *bytes, size_t size, uint64_t logical_bytes,
                    uint64_t map_offset, uint64_t metadata_offset,
                    uint32_t hunk_bytes, uint32_t compression)
{
    memset(bytes, 0, size);
    memcpy(bytes, "MComprHD", 8u);
    put_be32(bytes + 8u, HEADER_SIZE);
    put_be32(bytes + 12u, 5u);
    put_be32(bytes + 16u, compression);
    put_be64(bytes + 32u, logical_bytes);
    put_be64(bytes + 40u, map_offset);
    put_be64(bytes + 48u, metadata_offset);
    put_be32(bytes + 56u, hunk_bytes);
}
#endif

static int write_input(const char *path, const uint8_t *bytes, size_t size)
{
    FILE *file = schway__test_open_file(path, "wb");
    if (file == NULL) return 0;
    int ok = size == 0u || fwrite(bytes, 1u, size, file) == size;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

#if SCHWAYSTATION_CHD
static int read_toc(const schway_disc *disc, schway_toc *toc)
{
    if (disc == NULL || disc->toc == NULL || toc == NULL) return 0;
    memset(toc, 0, sizeof(*toc));
    toc->size = (uint32_t)sizeof(*toc);
    if (disc->toc(disc->user, toc) == 0u || toc->track_count == 0u ||
        toc->track_count > SCHWAY_DISC_MAX_TRACKS) return 0;
    for (uint32_t i = 0u; i < toc->track_count; i++) {
        const schway_disc_track *track = &toc->tracks[i];
        if (track->size != sizeof(*track) ||
            (track->type != SCHWAY_DISC_TRACK_AUDIO &&
             track->type != SCHWAY_DISC_TRACK_MODE1 &&
             track->type != SCHWAY_DISC_TRACK_MODE2) ||
            track->index0 > track->index1 || track->index1 > track->leadout) return 0;
    }
    return 1;
}

static int read_sample(const schway_disc *disc, uint8_t sector[2352],
                       uint8_t subcode[96], schway_toc *toc)
{
    if (disc == NULL || disc->read_sector == NULL || toc == NULL || !read_toc(disc, toc)) return 0;
    memset(sector, 0xA5, 2352u);
    memset(subcode, 0x5A, 96u);
    return disc->read_sector(disc->user, toc->tracks[0].index0, sector, subcode) != 0u;
}
#endif

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char input_path[1024];
    int path_length = snprintf(input_path, sizeof input_path, "%s/input.chd",
                               SCHWAY_FUZZ_WORK_DIR);
    if (path_length < 0 || (size_t)path_length >= sizeof input_path) {
        broken("build-owned path did not fit");
    }
    if (size > INPUT_MAX || (size != 0u && data == NULL)) return 0;
    if (schway__media_add_u64(UINT64_MAX, 1u, &(uint64_t){0}) ||
        schway__media_mul_u64(UINT64_MAX, 2u, &(uint64_t){0}) ||
        schway__media_add_u64(UINT64_MAX, 1u, NULL) ||
        schway__media_mul_u64(1u, 1u, NULL)) {
        broken("shared media arithmetic overflow contract changed");
    }
    if (!write_input(input_path, data, size)) broken("could not write bounded CHD input");

#if SCHWAYSTATION_CHD
    schway_disc_image *baseline = NULL;
    schway_disc_image *candidate = NULL;
    schway_disc_image_error error;
    uint8_t baseline_sector[2352];
    uint8_t baseline_subcode[96];
    uint8_t after_sector[2352];
    uint8_t after_subcode[96];
    schway_toc baseline_toc;
    schway_toc after_toc;
    const schway_disc *baseline_disc;
    schway_disc_image_status status;

    memset(&error, 0, sizeof(error));
    if (schway_disc_image_open(SCHWAY_FUZZ_CHD_FIXTURE, &baseline, &error) !=
            SCHWAY_DISC_IMAGE_OK || baseline == NULL) {
        broken("owned baseline CHD fixture did not open");
    }
    baseline_disc = schway_disc_image_disc(baseline);
    if (!read_sample(baseline_disc, baseline_sector, baseline_subcode, &baseline_toc)) {
        schway_disc_image_close(baseline);
        broken("owned baseline CHD did not provide a bounded sector");
    }
    status = schway_disc_image_open(input_path, &candidate, &error);
    if (status == SCHWAY_DISC_IMAGE_OK) {
        const schway_disc *disc = schway_disc_image_disc(candidate);
        uint8_t first_sector[2352];
        uint8_t second_sector[2352];
        uint8_t first_subcode[96];
        uint8_t second_subcode[96];
        schway_toc toc;
        schway_toc repeated_toc;
        uint32_t first_read;
        uint32_t second_read;
        if (disc == NULL || disc->read_sector == NULL || !read_toc(disc, &toc) ||
            !read_toc(disc, &repeated_toc) ||
            memcmp(&toc, &repeated_toc, sizeof(toc)) != 0) {
            schway_disc_image_close(candidate);
            schway_disc_image_close(baseline);
            broken("accepted CHD returned an invalid or unstable TOC");
        }
        memset(first_sector, 0xA5, sizeof(first_sector));
        memset(second_sector, 0xA5, sizeof(second_sector));
        memset(first_subcode, 0x5A, sizeof(first_subcode));
        memset(second_subcode, 0x5A, sizeof(second_subcode));
        first_read = disc->read_sector(disc->user, toc.tracks[0].index0,
                                       first_sector, first_subcode);
        second_read = disc->read_sector(disc->user, toc.tracks[0].index0,
                                        second_sector, second_subcode);
        if ((first_read != 0u) != (second_read != 0u) ||
            (first_read != 0u &&
             (memcmp(first_sector, second_sector, sizeof(first_sector)) != 0 ||
              memcmp(first_subcode, second_subcode, sizeof(first_subcode)) != 0))) {
            schway_disc_image_close(candidate);
            schway_disc_image_close(baseline);
            broken("accepted CHD changed a repeated sector read result");
        }
        uint8_t beyond_sector[2352];
        uint8_t beyond_subcode[96];
        if (disc->read_sector(disc->user,
                              toc.tracks[toc.track_count - 1u].leadout,
                              beyond_sector, beyond_subcode) != 0u) {
            schway_disc_image_close(candidate);
            schway_disc_image_close(baseline);
            broken("accepted CHD read beyond its TOC leadout");
        }
    } else if (candidate != NULL ||
               (status != SCHWAY_DISC_IMAGE_BAD_IMAGE &&
                status != SCHWAY_DISC_IMAGE_UNSUPPORTED &&
                status != SCHWAY_DISC_IMAGE_LIMIT)) {
        schway_disc_image_close(candidate);
        schway_disc_image_close(baseline);
        broken("rejected CHD returned an unexpected status or retained an image");
    }
    if (!read_sample(baseline_disc, after_sector, after_subcode, &after_toc) ||
        memcmp(baseline_sector, after_sector, sizeof(baseline_sector)) != 0 ||
        memcmp(baseline_subcode, after_subcode, sizeof(baseline_subcode)) != 0 ||
        memcmp(&baseline_toc, &after_toc, sizeof(baseline_toc)) != 0) {
        schway_disc_image_close(candidate);
        schway_disc_image_close(baseline);
        broken("candidate open changed the retained baseline media");
    }
    schway_disc_image_close(candidate);
    schway_disc_image_close(baseline);
#else
    schway_disc_image *candidate = NULL;
    schway_disc_image_error error;
    if (schway_disc_image_open(input_path, &candidate, &error) !=
            SCHWAY_DISC_IMAGE_UNSUPPORTED || candidate != NULL) {
        schway_disc_image_close(candidate);
        broken("CHD-off build accepted input or retained an image");
    }
#endif
    return 0;
}

#ifdef SCHWAY_FUZZ_REPLAY
#include "seeds.h"

uint32_t seeds_count(void)
{
    return SEED_COUNT;
}

const char *seeds_name(uint32_t index)
{
    static const char *const names[SEED_COUNT] = {
        "empty", "truncated", "v2", "parent", "exact_hunks", "over_hunks",
        "exact_map", "over_map", "metadata_offset", "map_overflow", "exact_image",
#if SCHWAYSTATION_CHD
        "unreadable_hunk",
#endif
    };
    return index < SEED_COUNT ? names[index] : "invalid";
}

size_t seeds_build(uint32_t index, uint8_t *bytes)
{
    uint64_t logical = UINT64_C(2448);
    uint32_t hunk = 2448u;
    size_t size = 0u;
    memset(bytes, 0, INPUT_MAX);
    switch (index) {
    case 0u: return 0u;
    case 1u:
        memcpy(bytes, "MComprHD", 8u);
        return 8u;
    case 2u:
        memcpy(bytes, "MComprHD", 8u);
        put_be32(bytes + 8u, 80u);
        put_be32(bytes + 12u, 2u);
        return 80u;
    case 3u:
        size = HEADER_SIZE;
        make_v5(bytes, size, logical, HEADER_SIZE, 0u, hunk, 0u);
        bytes[104] = 1u;
        return size;
    case 4u:
    case 5u:
        logical = (uint64_t)hunk * (index == 4u ? 65536u : 65537u);
        size = HEADER_SIZE + 18u;
        make_v5(bytes, size, logical, HEADER_SIZE, 0u, hunk, 1u);
        put_be32(bytes + HEADER_SIZE, 1u);
        return size;
    case 6u:
    case 7u:
        size = HEADER_SIZE + 18u;
        make_v5(bytes, size, logical, HEADER_SIZE, 0u, hunk, 1u);
        put_be32(bytes + HEADER_SIZE, index == 6u ? 16777216u : 16777217u);
        return size;
    case 8u:
        size = HEADER_SIZE;
        make_v5(bytes, size, logical, HEADER_SIZE, HEADER_SIZE, hunk, 0u);
        return size;
    case 9u:
        size = HEADER_SIZE;
        make_v5(bytes, size, logical, UINT64_MAX, 0u, hunk, 0u);
        return size;
    case 10u:
        size = HEADER_SIZE + 18u;
        make_v5(bytes, size, UINT64_C(1073741824), HEADER_SIZE, 0u,
                1048576u, 1u);
        put_be32(bytes + HEADER_SIZE, 1u);
        return size;
#if SCHWAYSTATION_CHD
    case 11u: {
        FILE *fixture = schway__test_open_file(SCHWAY_FUZZ_CHD_FIXTURE, "rb");
        long fixture_size = -1L;
        if (fixture == NULL) broken("could not open owned CHD seed fixture");
        if (fseek(fixture, 0L, SEEK_END) != 0 ||
            (fixture_size = ftell(fixture)) < 396L ||
            (unsigned long)fixture_size > INPUT_MAX ||
            fseek(fixture, 0L, SEEK_SET) != 0) {
            (void)fclose(fixture);
            broken("owned CHD seed fixture has an unexpected size");
        }
        size_t fixture_read = fread(bytes, 1u, (size_t)fixture_size, fixture);
        int fixture_close = fclose(fixture);
        if (fixture_read != (size_t)fixture_size || fixture_close != 0) {
            broken("could not read owned CHD seed fixture");
        }
        /* This one-byte change leaves the map and TOC valid but breaks lazy decode. */
        bytes[395u] ^= UINT8_C(0x29);
        return (size_t)fixture_size;
    }
#endif
    default:
        return 0u;
    }
}

#define SCHWAY_REPLAY_EMPTY_SEED "empty"
#define SCHWAY_REPLAY_NAME "fuzz.regress.chd"
#include "replay_main.h"
#endif
