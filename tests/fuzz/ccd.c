/* Bounded replay of hostile CloneCD descriptors through the production image reader. */
#if !defined(_WIN32) && !defined(_XOPEN_SOURCE)
#define _XOPEN_SOURCE 700
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disc_ccd.h"
#include "disc_image.h"
#include "../test_open.h"
#ifdef SCHWAY_FUZZ_REPLAY
#include "seeds.h"
#endif
#if !defined(_WIN32) && !defined(__wasi__)
#include <unistd.h>
#define SCHWAY_CCD_CAN_SYMLINK 1
#else
#define SCHWAY_CCD_CAN_SYMLINK 0
#endif

#ifndef SCHWAY_FUZZ_WORK_DIR
#error "CMake must provide a build-owned CCD fuzz directory"
#endif
#ifndef SCHWAY_FUZZ_CCD_FIXTURE
#error "CMake must provide the owned CCD descriptor fixture"
#endif
#ifndef SCHWAY_FUZZ_CCD_IMAGE
#error "CMake must provide the owned CCD image fixture"
#endif
#ifndef SCHWAY_FUZZ_CCD_SUB
#error "CMake must provide the owned CCD SUB fixture"
#endif

#define CCD_INPUT_MAX 65536u
#define CCD_MEDIA_MAX 1048576u
#define CCD_SEED_COUNT 10u
#define CCD_TAG "SCCD"
#define CCD_TAG_BYTES 5u

enum ccd_case {
    CCD_NORMAL = 0u,
    CCD_MISSING_IMAGE = 1u,
    CCD_SHORT_IMAGE = 2u,
    CCD_SHORT_SUB = 3u,
    CCD_ESCAPE_SUB = 4u
};

static void broken(const char *what)
{
    fprintf(stderr, "fuzz.regress.ccd: %s\n", what);
    abort();
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

static uint8_t *read_fixture(const char *path, size_t cap, size_t *size)
{
    FILE *file = schway__test_open_file(path, "rb");
    long length;
    uint8_t *bytes;
    if (file == NULL) return NULL;
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0 ||
        (size_t)length > cap || fseek(file, 0, SEEK_SET) != 0) {
        (void)fclose(file);
        return NULL;
    }
    bytes = (uint8_t *)malloc(length == 0 ? 1u : (size_t)length);
    if (bytes == NULL || ((size_t)length != 0u &&
        fread(bytes, 1u, (size_t)length, file) != (size_t)length)) {
        free(bytes);
        (void)fclose(file);
        return NULL;
    }
    if (fclose(file) != 0) {
        free(bytes);
        return NULL;
    }
    *size = (size_t)length;
    return bytes;
}

#ifdef SCHWAY_FUZZ_REPLAY
static size_t replace_text(uint8_t *bytes, size_t size, const char *from,
                           const char *to)
{
    size_t from_size = strlen(from), to_size = strlen(to);
    uint8_t *match = NULL;
    if (from_size != to_size) return 0u;
    for (size_t i = 0u; i + from_size <= size; i++) {
        if (memcmp(bytes + i, from, from_size) == 0) {
            match = bytes + i;
            break;
        }
    }
    if (match == NULL) return 0u;
    memcpy(match, to, to_size);
    return size;
}

static size_t insert_text(uint8_t *bytes, size_t size, size_t capacity,
                          const char *after, const char *addition)
{
    size_t after_size = strlen(after), add_size = strlen(addition);
    uint8_t *match = NULL;
    if (size > capacity || add_size > capacity - size) return 0u;
    for (size_t i = 0u; i + after_size <= size; i++) {
        if (memcmp(bytes + i, after, after_size) == 0) {
            match = bytes + i + after_size;
            break;
        }
    }
    if (match == NULL) return 0u;
    size_t tail = size - (size_t)(match - bytes);
    memmove(match + add_size, match, tail);
    memcpy(match, addition, add_size);
    return size + add_size;
}

static const char *const ccd_seed_names[CCD_SEED_COUNT] = {
    "valid_sub_transpose", "duplicate_field", "unsupported_session",
    "duplicate_mode", "plba_index_conflict", "missing_image",
    "short_image", "short_sub", "companion_scope", "valid_repeat",
};

uint32_t seeds_count(void) { return CCD_SEED_COUNT; }

const char *seeds_name(uint32_t index)
{
    if (index >= CCD_SEED_COUNT) return "invalid";
    if (index == 0u) {
        printf("fuzz.regress.ccd seed_count=%lu file_cap=%lu fixtures=disc_boot_legacy.ccd,disc_boot_legacy.img,disc_boot_legacy.sub\n",
               (unsigned long)seeds_count(),
               (unsigned long)(CCD_INPUT_MAX + CCD_TAG_BYTES));
    }
    printf("fuzz.regress.ccd seed[%lu]=%s\n", (unsigned long)index,
           ccd_seed_names[index]);
    return ccd_seed_names[index];
}

size_t seeds_build(uint32_t index, uint8_t *bytes)
{
    static uint8_t *fixture;
    static size_t fixture_size;
    uint8_t mode = CCD_NORMAL;
    size_t size;
    if (bytes == NULL || index >= CCD_SEED_COUNT) return 0u;
    if (fixture == NULL) fixture = read_fixture(SCHWAY_FUZZ_CCD_FIXTURE,
                                                 CCD_INPUT_MAX, &fixture_size);
    if (fixture == NULL || fixture_size + CCD_TAG_BYTES > SEEDS_SIZE_MAX) return 0u;
    memcpy(bytes, CCD_TAG, 4u);
    uint8_t *payload = bytes + CCD_TAG_BYTES;
    memcpy(payload, fixture, fixture_size);
    size = fixture_size;
    switch (index) {
    case 0u: mode = CCD_NORMAL; break;
    case 9u: mode = 5u; break;
    case 1u:
        size = insert_text(payload, size, SEEDS_SIZE_MAX - CCD_TAG_BYTES,
                           "Version=3\n", "Version=3\n");
        if (size == 0u) return 0u;
        break;
    case 2u:
        size = replace_text(payload, size, "[Session 1]", "[Session 2]");
        if (size == 0u) return 0u;
        break;
    case 3u:
        size = insert_text(payload, size, SEEDS_SIZE_MAX - CCD_TAG_BYTES,
                           "MODE=2\n", "MODE=1\n");
        if (size == 0u) return 0u;
        break;
    case 4u:
        size = replace_text(payload, size, "PLBA=0\n[Entry 4]",
                            "PLBA=1\n[Entry 4]");
        if (size == 0u) return 0u;
        break;
    case 5u: mode = CCD_MISSING_IMAGE; break;
    case 6u: mode = CCD_SHORT_IMAGE; break;
    case 7u: mode = CCD_SHORT_SUB; break;
    case 8u: mode = CCD_ESCAPE_SUB; break;
    default: return 0u;
    }
    bytes[4] = mode;
    return size + CCD_TAG_BYTES;
}

static int replay_seed(const uint8_t *data, size_t size)
{
    static uint8_t *candidate;
    if (candidate == NULL) {
        candidate = (uint8_t *)malloc(SEEDS_SIZE_MAX);
        if (candidate == NULL) broken("could not allocate seed comparator");
    }
    for (uint32_t i = 0u; i < seeds_count(); i++) {
        size_t expected_size = seeds_build(i, candidate);
        if (expected_size == size && memcmp(data, candidate, size) == 0) return (int)i;
    }
    return -1;
}
#endif

static int copy_file(const char *source, const char *destination, size_t cap,
                     size_t limit)
{
    size_t size;
    uint8_t *bytes = read_fixture(source, cap, &size);
    int ok = bytes != NULL && size <= limit && write_file(destination, bytes, size);
    free(bytes);
    return ok;
}

static int make_path(char *path, size_t cap, const char *name)
{
    int length = snprintf(path, cap, "%s/%s", SCHWAY_FUZZ_WORK_DIR, name);
    return length >= 0 && (size_t)length < cap;
}

static int sample_disc(schway_disc_image *image, uint8_t first[2352],
                       uint8_t last[2352], uint8_t first_sub[96],
                       uint8_t last_sub[96], uint32_t *leadout)
{
    const schway_disc *disc = schway_disc_image_disc(image);
    schway_toc toc;
    if (disc == NULL || disc->read_sector == NULL || disc->toc == NULL) return 0;
    memset(&toc, 0, sizeof toc);
    toc.size = (uint32_t)sizeof toc;
    if (disc->toc(disc->user, &toc) == 0u || toc.track_count != 2u ||
        toc.tracks[1].leadout < toc.tracks[0].index1) return 0;
    *leadout = toc.tracks[1].leadout;
    return disc->read_sector(disc->user, toc.tracks[0].index1, first, first_sub) != 0u &&
           disc->read_sector(disc->user, *leadout - 1u, last, last_sub) != 0u;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static schway_disc_image *baseline;
    static char descriptor[1024], image_path[1024], sub_path[1024];
    static char escape_descriptor[1024], escape_image[1024], escape_sub[1024], outside_sub[1024];
    static int initialized;
    const uint8_t *input = data;
    size_t input_size = size;
    unsigned int mode = CCD_NORMAL;
    uint8_t before_first[2352], before_last[2352], after_first[2352], after_last[2352];
    uint8_t before_first_sub[96], before_last_sub[96], after_first_sub[96], after_last_sub[96];
    uint32_t before_leadout = 0u, after_leadout = 0u;
    schway_disc_image *candidate = NULL;
    schway__disc_ccd *direct_reader = NULL;
    schway_disc_image_error error;
    schway_disc_image_status dispatched, direct;
    char *candidate_path = descriptor;
    if (size > CCD_INPUT_MAX + CCD_TAG_BYTES || (size != 0u && data == NULL)) return 0;
    if (initialized == 0) {
        if (!make_path(descriptor, sizeof descriptor, "candidate/input.ccd") ||
            !make_path(image_path, sizeof image_path, "candidate/input.img") ||
            !make_path(sub_path, sizeof sub_path, "candidate/input.sub") ||
            !make_path(escape_descriptor, sizeof escape_descriptor, "candidate/escape.ccd") ||
            !make_path(escape_image, sizeof escape_image, "candidate/escape.img") ||
            !make_path(escape_sub, sizeof escape_sub, "candidate/escape.sub") ||
            !make_path(outside_sub, sizeof outside_sub, "outside.sub") ||
            schway_disc_image_open(SCHWAY_FUZZ_CCD_FIXTURE, &baseline, &error) !=
                SCHWAY_DISC_IMAGE_OK || baseline == NULL ||
            !sample_disc(baseline, before_first, before_last, before_first_sub,
                         before_last_sub, &before_leadout)) {
            broken("owned CCD baseline or build-owned paths were unavailable");
        }
        initialized = 1;
    }
    if (size >= CCD_TAG_BYTES && memcmp(data, CCD_TAG, 4u) == 0) {
        mode = data[4];
        input = data + CCD_TAG_BYTES;
        input_size = size - CCD_TAG_BYTES;
        if (mode > CCD_ESCAPE_SUB) mode = CCD_NORMAL;
    }
    if (input_size > CCD_INPUT_MAX) return 0;
    if (!sample_disc(baseline, before_first, before_last, before_first_sub,
                    before_last_sub, &before_leadout)) {
        broken("retained CCD reader failed before candidate open");
    }
    candidate_path = mode == CCD_ESCAPE_SUB ? escape_descriptor : descriptor;
    if (!copy_file(SCHWAY_FUZZ_CCD_IMAGE,
                   mode == CCD_ESCAPE_SUB ? escape_image : image_path,
                   CCD_MEDIA_MAX, SIZE_MAX) ||
        (mode != CCD_MISSING_IMAGE && mode != CCD_ESCAPE_SUB &&
         !copy_file(SCHWAY_FUZZ_CCD_SUB, sub_path, CCD_MEDIA_MAX, SIZE_MAX))) {
        broken("could not populate bounded same-root companions");
    }
    if (mode == CCD_MISSING_IMAGE) (void)remove(image_path);
    if (mode == CCD_SHORT_IMAGE) {
        uint8_t short_image[1] = {0u};
        if (!write_file(image_path, short_image, sizeof short_image)) broken("could not write short IMG");
    }
    if (mode == CCD_SHORT_SUB) {
        uint8_t short_sub[1] = {0u};
        if (!write_file(sub_path, short_sub, sizeof short_sub)) broken("could not write short SUB");
    }
#if SCHWAY_CCD_CAN_SYMLINK
    if (mode == CCD_ESCAPE_SUB) {
        (void)remove(escape_sub);
        (void)remove(outside_sub);
        if (!copy_file(SCHWAY_FUZZ_CCD_SUB, outside_sub, CCD_MEDIA_MAX, SIZE_MAX) ||
            symlink("../outside.sub", escape_sub) != 0) {
            broken("could not create the bounded external SUB sentinel");
        }
    }
#else
    if (mode == CCD_ESCAPE_SUB) {
        mode = CCD_SHORT_SUB;
        uint8_t short_sub[1] = {0u};
        if (!write_file(escape_sub, short_sub, sizeof short_sub)) broken("could not write bounded SUB control");
    }
#endif
    if (!write_file(candidate_path, input, input_size)) broken("could not write bounded CCD input");
    dispatched = schway_disc_image_open(candidate_path, &candidate, &error);
    direct = schway__disc_ccd_open(candidate_path, &direct_reader, &error);
    if ((dispatched == SCHWAY_DISC_IMAGE_OK) != (direct == SCHWAY_DISC_IMAGE_OK) ||
        (dispatched == SCHWAY_DISC_IMAGE_OK) != (candidate != NULL) ||
        (direct == SCHWAY_DISC_IMAGE_OK) != (direct_reader != NULL)) {
        schway_disc_image_close(candidate);
        schway__disc_ccd_close(direct_reader);
        broken("dispatcher/direct open or failed output cleanup diverged");
    }
#ifdef SCHWAY_FUZZ_REPLAY
    int seed = replay_seed(data, size);
    static const schway_disc_image_status expected[CCD_SEED_COUNT] = {
        SCHWAY_DISC_IMAGE_OK, SCHWAY_DISC_IMAGE_BAD_IMAGE,
        SCHWAY_DISC_IMAGE_UNSUPPORTED, SCHWAY_DISC_IMAGE_BAD_IMAGE,
        SCHWAY_DISC_IMAGE_BAD_IMAGE, SCHWAY_DISC_IMAGE_IO,
        SCHWAY_DISC_IMAGE_BAD_IMAGE, SCHWAY_DISC_IMAGE_BAD_IMAGE,
        SCHWAY_DISC_IMAGE_BAD_IMAGE, SCHWAY_DISC_IMAGE_OK,
    };
    if (seed >= 0) {
        schway_disc_image_status wanted = expected[seed];
#if !SCHWAY_CCD_CAN_SYMLINK
        if (seed == 8) wanted = SCHWAY_DISC_IMAGE_BAD_IMAGE;
#endif
        if (dispatched != wanted) {
            schway_disc_image_close(candidate);
            schway__disc_ccd_close(direct_reader);
            broken("named CCD seed changed its expected parser result");
        }
    }
#endif
    if (dispatched == SCHWAY_DISC_IMAGE_OK) {
        uint8_t first[2352], last[2352], first_sub[96], last_sub[96];
        uint8_t sector[2352], subcode[96];
        uint32_t leadout;
        const schway_disc *disc = schway_disc_image_disc(candidate);
        if (!sample_disc(candidate, first, last, first_sub, last_sub, &leadout) ||
            leadout != before_leadout || memcmp(first, before_first, sizeof first) != 0 ||
            memcmp(last, before_last, sizeof last) != 0 ||
            memcmp(first_sub, before_first_sub, sizeof first_sub) != 0 ||
            memcmp(last_sub, before_last_sub, sizeof last_sub) != 0 ||
            memcmp(first_sub, last_sub, sizeof first_sub) == 0) {
            schway_disc_image_close(candidate);
            schway__disc_ccd_close(direct_reader);
            broken("accepted CCD changed source sectors or SUB transpose output");
        }
        memset(sector, 0xa5, sizeof sector);
        memset(subcode, 0x5a, sizeof subcode);
        if (disc == NULL || disc->read_sector(disc->user, leadout, sector, subcode) != 0u ||
            sector[0] != 0xa5u || subcode[0] != 0x5au) {
            schway_disc_image_close(candidate);
            schway__disc_ccd_close(direct_reader);
            broken("CCD out-of-range read changed output bytes");
        }
    }
    if (!sample_disc(baseline, after_first, after_last, after_first_sub,
                    after_last_sub, &after_leadout) || after_leadout != before_leadout ||
        memcmp(before_first, after_first, 2352u) != 0 ||
        memcmp(before_last, after_last, 2352u) != 0 ||
        memcmp(before_first_sub, after_first_sub, 96u) != 0 ||
        memcmp(before_last_sub, after_last_sub, 96u) != 0) {
        schway_disc_image_close(candidate);
        schway__disc_ccd_close(direct_reader);
        broken("candidate open changed the previously retained CCD reader");
    }
    schway_disc_image_close(candidate);
    schway__disc_ccd_close(direct_reader);
    return 0;
}

#define SCHWAY_REPLAY_NAME "fuzz.regress.ccd"
#include "replay_main.h"
