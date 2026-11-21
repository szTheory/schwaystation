#include "disc_pbp.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../test_open.h"
#include "pbp_inflate.h"

#define SCHWAY__DISC_PBP_TEST_HEADER_BYTES 40u
#define SCHWAY__DISC_PBP_TEST_MARKER_BYTES 12u
#define SCHWAY__DISC_PBP_TEST_IMAGE_BYTES \
    (SCHWAY__DISC_PBP_TEST_HEADER_BYTES + SCHWAY__DISC_PBP_TEST_MARKER_BYTES)

static int make_path(char path[1024], const char *directory, const char *name)
{
    int size = snprintf(path, 1024u, "%s/%s", directory, name);
    return size >= 0 && (size_t)size < 1024u;
}

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

static void put_le32(uint8_t bytes[4], uint32_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}

static void make_envelope(uint8_t bytes[SCHWAY__DISC_PBP_TEST_IMAGE_BYTES],
                          const uint8_t marker[SCHWAY__DISC_PBP_TEST_MARKER_BYTES])
{
    memset(bytes, 0, SCHWAY__DISC_PBP_TEST_IMAGE_BYTES);
    memcpy(bytes, "\0PBP", 4u);
    for (uint32_t i = 0u; i < 8u; i++) {
        put_le32(bytes + 8u + (size_t)i * 4u, SCHWAY__DISC_PBP_TEST_HEADER_BYTES);
    }
    memcpy(bytes + SCHWAY__DISC_PBP_TEST_HEADER_BYTES, marker,
           SCHWAY__DISC_PBP_TEST_MARKER_BYTES);
}

static int expect_rejected(const char *directory, const char *name,
                           const uint8_t *bytes, size_t size,
                           schway_disc_image_status expected)
{
    char path[1024];
    schway_disc_image_error error;
    schway__disc_pbp *reader = (schway__disc_pbp *)(uintptr_t)1u;
    schway_disc_image_status status;

    if (!make_path(path, directory, name)) {
        fprintf(stderr, "ASSERT disc.image.pbp_headers: path is too long for %s\n", name);
        return 0;
    }
    if (!write_bytes(path, bytes, size)) {
        fprintf(stderr, "ASSERT disc.image.pbp_headers: could not write %s\n", name);
        return 0;
    }
    memset(&error, 0, sizeof(error));
    status = schway__disc_pbp_open(path, &reader, &error);
    if (reader != NULL || status != expected || error.status != (uint32_t)expected ||
        error.size != (uint32_t)sizeof(error) || error.message[0] == '\0') {
        fprintf(stderr, "ASSERT disc.image.pbp_headers: %s returned %u, expected %u, output=%p\n",
                name, (unsigned int)status, (unsigned int)expected, (void *)reader);
        if (reader == NULL) schway__disc_pbp_close(reader);
        (void)remove(path);
        return 0;
    }
    schway__disc_pbp_close(reader);
    if (remove(path) != 0) {
        fprintf(stderr, "ASSERT disc.image.pbp_headers: could not remove %s\n", name);
        return 0;
    }
    return 1;
}

static int run_headers(const char *directory)
{
    static const uint8_t pgd_marker[SCHWAY__DISC_PBP_TEST_MARKER_BYTES] = {
        'P', 'G', 'D', 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u
    };
    static const uint8_t unknown_marker[SCHWAY__DISC_PBP_TEST_MARKER_BYTES] = {
        'U', 'N', 'K', 'N', 'O', 'W', 'N', '0', '0', '0', '0', '0'
    };
    static const uint8_t psiso_marker[SCHWAY__DISC_PBP_TEST_MARKER_BYTES] = {
        'P', 'S', 'I', 'S', 'O', 'I', 'M', 'G', '0', '0', '0', '0'
    };
    uint8_t short_header[SCHWAY__DISC_PBP_TEST_HEADER_BYTES - 1u] = { 0u };
    uint8_t wrong_magic[SCHWAY__DISC_PBP_TEST_IMAGE_BYTES] = { 0u };
    uint8_t envelope[SCHWAY__DISC_PBP_TEST_IMAGE_BYTES];

    memcpy(short_header, "\0PBP", 4u);
    if (!expect_rejected(directory, "pbp-empty.bin", NULL, 0u,
                         SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_rejected(directory, "pbp-short.bin", short_header, sizeof(short_header),
                         SCHWAY_DISC_IMAGE_BAD_IMAGE) ||
        !expect_rejected(directory, "pbp-wrong-magic.bin", wrong_magic, sizeof(wrong_magic),
                         SCHWAY_DISC_IMAGE_BAD_IMAGE)) return 1;

    make_envelope(envelope, pgd_marker);
    if (!expect_rejected(directory, "pbp-encrypted-marker.bin", envelope, sizeof(envelope),
                         SCHWAY_DISC_IMAGE_UNSUPPORTED)) return 1;
    make_envelope(envelope, unknown_marker);
    if (!expect_rejected(directory, "pbp-unknown-marker.bin", envelope, sizeof(envelope),
                         SCHWAY_DISC_IMAGE_UNSUPPORTED)) return 1;

    make_envelope(envelope, psiso_marker);
    put_le32(envelope + 8u, UINT32_C(0x40000001));
    if (!expect_rejected(directory, "pbp-oversized-offset.bin", envelope, sizeof(envelope),
                         SCHWAY_DISC_IMAGE_BAD_IMAGE)) return 1;
    make_envelope(envelope, psiso_marker);
    put_le32(envelope + 8u + 7u * 4u, UINT32_C(0xfffffff8));
    if (!expect_rejected(directory, "pbp-overflow-offset.bin", envelope, sizeof(envelope),
                         SCHWAY_DISC_IMAGE_BAD_IMAGE)) return 1;
    return 0;
}

static int expect_inflate(const char *name, const uint8_t *input, size_t input_size,
                          const uint8_t *expected, size_t expected_size,
                          size_t output_size, int expected_success)
{
    uint8_t output[128];
    size_t i;
    int success;

    memset(output, 0xa5, sizeof(output));
    success = schway__pbp_inflate_raw(input, input_size, output, output_size);
    if (success != expected_success) {
        fprintf(stderr, "ASSERT disc.image.pbp_inflate: %s returned %d, expected %d\n",
                name, success, expected_success);
        return 0;
    }
    if (success != 0) {
        if (expected_size != output_size || memcmp(output, expected, expected_size) != 0) {
            fprintf(stderr, "ASSERT disc.image.pbp_inflate: %s output differs\n", name);
            return 0;
        }
        for (i = expected_size; i < sizeof(output); i++) {
            if (output[i] != 0xa5u) {
                fprintf(stderr, "ASSERT disc.image.pbp_inflate: %s wrote past output\n", name);
                return 0;
            }
        }
    } else {
        for (i = 0u; i < sizeof(output); i++) {
            if (output[i] != 0xa5u) {
                fprintf(stderr, "ASSERT disc.image.pbp_inflate: %s changed output on failure\n", name);
                return 0;
            }
        }
    }
    return 1;
}

static int run_inflate(void)
{
    static const uint8_t stored_stream[] = {
        0x01u, 0x05u, 0x00u, 0xfau, 0xffu, 'h', 'e', 'l', 'l', 'o'
    };
    static const uint8_t stored_output[] = { 'h', 'e', 'l', 'l', 'o' };
    static const uint8_t huffman_stream[] = {
        0x2bu, 0x4eu, 0xceu, 0x28u, 0x4fu, 0xacu, 0x2cu, 0x2eu, 0x49u, 0x2cu,
        0xc9u, 0xccu, 0xcfu, 0x53u, 0x48u, 0xcau, 0x2fu, 0xcdu, 0x4bu, 0x49u,
        0x4du, 0x51u, 0x28u, 0x4au, 0x2cu, 0x57u, 0x70u, 0x71u, 0x75u, 0xf3u,
        0x71u, 0x0cu, 0x71u, 0x55u, 0x28u, 0xa6u, 0x82u, 0x0au, 0x00u
    };
    static const uint8_t huffman_output[] =
        "schwaystation bounded raw DEFLATE "
        "schwaystation bounded raw DEFLATE "
        "schwaystation bounded raw DEFLATE ";
    uint8_t trailing[sizeof(stored_stream) + 1u];
    size_t output_size = sizeof(huffman_output) - 1u;

    memcpy(trailing, stored_stream, sizeof(stored_stream));
    trailing[sizeof(stored_stream)] = 0u;
    if (!expect_inflate("stored block", stored_stream, sizeof(stored_stream),
                        stored_output, sizeof(stored_output), sizeof(stored_output), 1) ||
        !expect_inflate("fixed Huffman block", huffman_stream, sizeof(huffman_stream),
                        huffman_output, output_size, output_size, 1) ||
        !expect_inflate("trailing input", trailing, sizeof(trailing),
                        stored_output, sizeof(stored_output), sizeof(stored_output), 0) ||
        !expect_inflate("truncated input", huffman_stream, sizeof(huffman_stream) - 1u,
                        huffman_output, output_size, output_size, 0) ||
        !expect_inflate("short expected output", huffman_stream, sizeof(huffman_stream),
                        huffman_output, output_size, output_size - 1u, 0) ||
        !expect_inflate("long expected output", huffman_stream, sizeof(huffman_stream),
                        huffman_output, output_size, output_size + 1u, 0) ||
        !expect_inflate("output overflow", huffman_stream, sizeof(huffman_stream),
                        huffman_output, output_size, 10u, 0) ||
        !expect_inflate("empty input", stored_stream, 0u,
                        stored_output, sizeof(stored_output), sizeof(stored_output), 0) ||
        !expect_inflate("empty output", stored_stream, sizeof(stored_stream),
                        stored_output, sizeof(stored_output), 0u, 0)) return 1;
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--inflate") == 0) return run_inflate();
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-disc-pbp <build-test-directory>|--inflate\n");
        return 2;
    }
    return run_headers(argv[1]);
}
