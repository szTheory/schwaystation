#include "disc_chd.h"

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

static int make_path(char path[1024], const char *directory)
{
    int size = snprintf(path, 1024u, "%s/chd-header-test.bin", directory);
    return size >= 0 && (size_t)size < 1024u;
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

int main(int argc, char **argv)
{
    int result = 0;
    char path[1024];
#if SCHWAYSTATION_CHD
    uint8_t header[124];
    static const uint8_t truncated[] = "MComprHD";
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
    memcpy(header, "MComprHD", 8u);
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
