/* Bounded host-side sector provider; file access never enters the core. */
#include "disc_image.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCHWAY__DISC_IMAGE_MAX_BYTES UINT64_C(1073741824)
#define SCHWAY__DISC_IMAGE_ISO_SECTOR_BYTES 2048u
#define SCHWAY__DISC_IMAGE_RAW_SECTOR_BYTES 2352u
#define SCHWAY__DISC_IMAGE_MIN_SECTORS 16u

struct schway_disc_image {
    FILE *file;
    uint64_t file_bytes;
    uint32_t sector_count;
    schway_disc descriptor;
    schway_toc toc;
};

static unsigned char schway__disc_image_ascii_lower(unsigned char value)
{
    if (value >= (unsigned char)'A' && value <= (unsigned char)'Z') {
        return (unsigned char)(value - (unsigned char)'A' + (unsigned char)'a');
    }
    return value;
}

static int schway__disc_image_extension_is(const char *path, const char *extension)
{
    const char *dot = strrchr(path, '.');
    const char *slash = strrchr(path, '/');
    const char *backslash = strrchr(path, '\\');
    const char *last = slash;
    if (backslash != NULL && (last == NULL || backslash > last)) last = backslash;
    if (dot == NULL || (last != NULL && dot < last) || strlen(dot) != strlen(extension)) {
        return 0;
    }
    for (size_t i = 0u; extension[i] != '\0'; i++) {
        if (schway__disc_image_ascii_lower((unsigned char)dot[i]) !=
            schway__disc_image_ascii_lower((unsigned char)extension[i])) return 0;
    }
    return 1;
}

static void schway__disc_image_error_set(schway_disc_image_error *error,
                                          schway_disc_image_status status,
                                          uint32_t line, const char *message)
{
    if (error == NULL) return;
    memset(error, 0, sizeof *error);
    error->size = (uint32_t)sizeof *error;
    error->status = (uint32_t)status;
    error->line = line;
    (void)snprintf(error->message, sizeof error->message, "%s", message);
}

static uint8_t schway__disc_image_edc_step(uint8_t value, uint32_t *edc)
{
    *edc ^= value;
    for (uint32_t bit = 0u; bit < 8u; bit++) {
        *edc = (*edc >> 1) ^ ((*edc & 1u) != 0u ? UINT32_C(0xd8018001) : 0u);
    }
    return value;
}

static uint8_t schway__disc_image_ecc_forward(uint8_t value)
{
    uint16_t doubled = (uint16_t)((uint16_t)value << 1);
    if ((value & 0x80u) != 0u) doubled = (uint16_t)(doubled ^ 0x11du);
    return (uint8_t)doubled;
}

static void schway__disc_image_ecc(const uint8_t *source, uint32_t major_count,
                                    uint32_t minor_count, uint32_t major_mult,
                                    uint32_t minor_inc, uint8_t *destination)
{
    uint32_t size = major_count * minor_count;
    uint8_t backward[256];
    for (uint32_t i = 0u; i < 256u; i++) {
        uint8_t value = (uint8_t)i;
        backward[value ^ schway__disc_image_ecc_forward(value)] = value;
    }
    for (uint32_t major = 0u; major < major_count; major++) {
        uint32_t index = (major >> 1) * major_mult + (major & 1u);
        uint8_t a = 0u;
        uint8_t b = 0u;
        for (uint32_t minor = 0u; minor < minor_count; minor++) {
            uint8_t value = source[index];
            index += minor_inc;
            if (index >= size) index -= size;
            a ^= value;
            b ^= value;
            a = schway__disc_image_ecc_forward(a);
        }
        a = backward[schway__disc_image_ecc_forward(a) ^ b];
        destination[major] = a;
        destination[major + major_count] = (uint8_t)(a ^ b);
    }
}

static void schway__disc_image_mode1_header(uint8_t sector[SCHWAY__DISC_IMAGE_RAW_SECTOR_BYTES],
                                            uint32_t lba)
{
    uint32_t absolute = lba + 150u;
    uint32_t minute = (absolute / (60u * 75u)) % 100u;
    uint32_t second = (absolute / 75u) % 60u;
    uint32_t frame = absolute % 75u;
#define SCHWAY__BCD(value) ((uint8_t)((((value) / 10u) << 4) | ((value) % 10u)))
    sector[0] = 0u;
    memset(sector + 1u, 0xff, 10u);
    sector[11] = 0u;
    sector[12] = SCHWAY__BCD(minute);
    sector[13] = SCHWAY__BCD(second);
    sector[14] = SCHWAY__BCD(frame);
    sector[15] = 1u;
#undef SCHWAY__BCD
}

static void schway__disc_image_mode1_trailer(
    uint8_t sector[SCHWAY__DISC_IMAGE_RAW_SECTOR_BYTES])
{
    uint32_t edc = 0u;
    for (uint32_t i = 0u; i < 2064u; i++) {
        (void)schway__disc_image_edc_step(sector[i], &edc);
    }
    sector[2064] = (uint8_t)edc;
    sector[2065] = (uint8_t)(edc >> 8);
    sector[2066] = (uint8_t)(edc >> 16);
    sector[2067] = (uint8_t)(edc >> 24);
    memset(sector + 2068u, 0, 8u);
    schway__disc_image_ecc(sector + 12u, 86u, 24u, 2u, 86u, sector + 2076u);
    schway__disc_image_ecc(sector + 12u, 52u, 43u, 86u, 88u, sector + 2248u);
}

static uint32_t schway__disc_image_read_sector(void *user, uint32_t lba,
                                               uint8_t sector[2352], uint8_t subcode[96])
{
    schway_disc_image *image = (schway_disc_image *)user;
    uint8_t cooked[SCHWAY__DISC_IMAGE_ISO_SECTOR_BYTES];
    uint64_t offset;
    if (image == NULL || sector == NULL || lba >= image->sector_count) return 0u;
    offset = (uint64_t)lba * SCHWAY__DISC_IMAGE_ISO_SECTOR_BYTES;
    if (offset > image->file_bytes ||
        SCHWAY__DISC_IMAGE_ISO_SECTOR_BYTES > image->file_bytes - offset ||
        offset > (uint64_t)LONG_MAX || fseek(image->file, (long)offset, SEEK_SET) != 0 ||
        fread(cooked, 1u, sizeof cooked, image->file) != sizeof cooked) return 0u;

    /* MODE1/2048 is a cooked data track. Present its bytes as a raw Mode 1
       sector, as specified by the host sector contract (PSX-SPU-CDROM-AND-MDEC §3). */
    memset(sector, 0, SCHWAY__DISC_IMAGE_RAW_SECTOR_BYTES);
    schway__disc_image_mode1_header(sector, lba);
    memcpy(sector + 16u, cooked, sizeof cooked);
    schway__disc_image_mode1_trailer(sector);
    if (subcode != NULL) memset(subcode, 0, 96u);
    return 1u;
}

static uint32_t schway__disc_image_read_toc(void *user, schway_toc *out)
{
    const schway_disc_image *image = (const schway_disc_image *)user;
    if (image == NULL || out == NULL || out->size != sizeof *out) return 0u;
    *out = image->toc;
    return 1u;
}

schway_disc_image_status schway_disc_image_open(const char *path,
                                                schway_disc_image **out,
                                                schway_disc_image_error *error)
{
    FILE *file;
    long file_end;
    uint64_t file_bytes;
    schway_disc_image *image;
    schway_disc_image_status status = SCHWAY_DISC_IMAGE_BAD_IMAGE;
    if (out == NULL || path == NULL || path[0] == '\0') {
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_BAD_ARGUMENT, 0u,
                                     "invalid disc image path or output");
        return SCHWAY_DISC_IMAGE_BAD_ARGUMENT;
    }
    *out = NULL;
    if (error != NULL) {
        memset(error, 0, sizeof *error);
        error->size = (uint32_t)sizeof *error;
    }
    if (schway__disc_image_extension_is(path, ".chd")) {
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_UNSUPPORTED, 0u,
                                     "unsupported: CHD (planned v2.0)");
        return SCHWAY_DISC_IMAGE_UNSUPPORTED;
    }
    if (schway__disc_image_extension_is(path, ".pbp")) {
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_UNSUPPORTED, 0u,
                                     "unsupported: PBP (planned v2.0)");
        return SCHWAY_DISC_IMAGE_UNSUPPORTED;
    }
    if (schway__disc_image_extension_is(path, ".ecm")) {
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_UNSUPPORTED, 0u,
                                     "unsupported: ECM (planned v2.0)");
        return SCHWAY_DISC_IMAGE_UNSUPPORTED;
    }
    if (schway__disc_image_extension_is(path, ".ccd")) {
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_UNSUPPORTED, 0u,
                                     "unsupported: CCD (planned v2.0)");
        return SCHWAY_DISC_IMAGE_UNSUPPORTED;
    }
    if (!schway__disc_image_extension_is(path, ".iso")) {
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_BAD_IMAGE, 0u,
                                     "expected a cue sheet or ISO image");
        return SCHWAY_DISC_IMAGE_BAD_IMAGE;
    }
    file = fopen(path, "rb");
    if (file == NULL) {
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_IO, 0u,
                                     "cannot open disc image");
        return SCHWAY_DISC_IMAGE_IO;
    }
    if (fseek(file, 0L, SEEK_END) != 0 || (file_end = ftell(file)) < 0L ||
        fseek(file, 0L, SEEK_SET) != 0) {
        (void)fclose(file);
        schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_IO, 0u,
                                     "cannot size disc image");
        return SCHWAY_DISC_IMAGE_IO;
    }
    file_bytes = (uint64_t)(unsigned long)file_end;
    if (file_bytes > SCHWAY__DISC_IMAGE_MAX_BYTES) {
        status = SCHWAY_DISC_IMAGE_LIMIT;
        goto reject_file;
    }
    if (file_bytes == 0u || file_bytes % SCHWAY__DISC_IMAGE_ISO_SECTOR_BYTES != 0u ||
        file_bytes / SCHWAY__DISC_IMAGE_ISO_SECTOR_BYTES < SCHWAY__DISC_IMAGE_MIN_SECTORS) {
        status = SCHWAY_DISC_IMAGE_BAD_IMAGE;
        goto reject_file;
    }
    image = (schway_disc_image *)calloc(1u, sizeof *image);
    if (image == NULL) {
        status = SCHWAY_DISC_IMAGE_OUT_OF_MEMORY;
        goto reject_file;
    }
    image->file = file;
    image->file_bytes = file_bytes;
    image->sector_count = (uint32_t)(file_bytes / SCHWAY__DISC_IMAGE_ISO_SECTOR_BYTES);
    image->descriptor.size = (uint32_t)sizeof image->descriptor;
    image->descriptor.user = image;
    image->descriptor.read_sector = schway__disc_image_read_sector;
    image->descriptor.toc = schway__disc_image_read_toc;
    image->toc.size = (uint32_t)sizeof image->toc;
    image->toc.track_count = 1u;
    image->toc.tracks[0].size = (uint32_t)sizeof image->toc.tracks[0];
    image->toc.tracks[0].type = SCHWAY_DISC_TRACK_MODE1;
    image->toc.tracks[0].leadout = image->sector_count;
    *out = image;
    schway__disc_image_error_set(error, SCHWAY_DISC_IMAGE_OK, 0u, "ok");
    return SCHWAY_DISC_IMAGE_OK;

reject_file:
    (void)fclose(file);
    schway__disc_image_error_set(error, status, 0u,
                                 status == SCHWAY_DISC_IMAGE_LIMIT ?
                                     "disc image exceeds the 1 GiB limit" :
                                 status == SCHWAY_DISC_IMAGE_OUT_OF_MEMORY ?
                                     "out of memory opening disc image" :
                                     "ISO must contain at least 16 complete 2048-byte sectors");
    return status;
}

void schway_disc_image_close(schway_disc_image *image)
{
    if (image == NULL) return;
    if (image->file != NULL) (void)fclose(image->file);
    free(image);
}

const schway_disc *schway_disc_image_disc(const schway_disc_image *image)
{
    return image == NULL ? NULL : &image->descriptor;
}

const char *schway_disc_image_status_string(schway_disc_image_status status)
{
    switch (status) {
    case SCHWAY_DISC_IMAGE_OK: return "ok";
    case SCHWAY_DISC_IMAGE_BAD_ARGUMENT: return "invalid argument";
    case SCHWAY_DISC_IMAGE_IO: return "disc image I/O error";
    case SCHWAY_DISC_IMAGE_BAD_CUE: return "BAD_CUE";
    case SCHWAY_DISC_IMAGE_BAD_IMAGE: return "BAD_IMAGE";
    case SCHWAY_DISC_IMAGE_LIMIT: return "disc image exceeds limits";
    case SCHWAY_DISC_IMAGE_OUT_OF_MEMORY: return "out of memory";
    case SCHWAY_DISC_IMAGE_UNSUPPORTED: return "unsupported disc image";
    }
    return "unknown disc image status";
}
