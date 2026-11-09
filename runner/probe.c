/* schwaystation-run --probe: the identity of a PS-EXE file. The core's
   validator supplies the header fields; the runner adds the file's name and
   size and two digests. No instance is made and no frame runs. */
#include "probe.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "md5.h"
#include "schwaystation.h"
#include "sha256.h"

#define EXE_HEADER 0x800u
#define EXE_MAX (EXE_HEADER + 0x200000u) /* the validator's largest file */

/* Writes text with every byte outside printable ASCII shown as '?', so a
   hostile name cannot add a line or a control sequence to the report. */
static void put_text(FILE *f, const char *text)
{
    for (const char *p = text; *p != '\0'; p++) {
        unsigned char c = (unsigned char)*p;
        fputc(c >= 0x20u && c < 0x7Fu ? (int)c : '?', f);
    }
}

static void refuse(const char *path, const char *why)
{
    fputs("schwaystation-run: ", stderr);
    put_text(stderr, path);
    fprintf(stderr, ": %s\n", why);
}

/* The file's name without its directory, split at either kind of slash. */
static const char *base_name(const char *path)
{
    const char *name = path;
    for (const char *p = path; *p != '\0'; p++) {
        if (*p == '/' || *p == '\\') {
            name = p + 1;
        }
    }
    return name;
}

/* The licence text names the console's market (psx-spx, "PSX EXE Format");
   the guess is that market's video standard. */
static const char *licence_area_name(uint32_t area)
{
    switch (area) {
    case SCHWAY_LICENCE_JAPAN:
        return "japan";
    case SCHWAY_LICENCE_EUROPE:
        return "europe";
    case SCHWAY_LICENCE_NORTH_AMERICA:
        return "north_america";
    default:
        return "none";
    }
}

static const char *region_guess(uint32_t area)
{
    switch (area) {
    case SCHWAY_LICENCE_JAPAN:
        return "ntsc-j";
    case SCHWAY_LICENCE_EUROPE:
        return "pal";
    case SCHWAY_LICENCE_NORTH_AMERICA:
        return "ntsc-u";
    default:
        return "unknown";
    }
}

/* The achievement-hash rule for a PS-EXE: MD5 over the file's first
   t_size + 0800h bytes, the header included (rcheevos src/hash/rc_hash_disc.c,
   rc_hash_psx, line 965). The validator has checked that many bytes exist. A
   bare file has no boot-file name, which a disc's hash prepends. */
static void rcheevos_md5(const uint8_t *file, uint32_t hash_len, char hex[33])
{
    schway__md5 m;
    uint8_t digest[16];
    schway__md5_init(&m);
    schway__md5_update(&m, file, hash_len);
    schway__md5_final(&m, digest);
    schway__md5_hex(digest, hex);
}

int schway__probe_print(FILE *out, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        refuse(path, "cannot open the file");
        return 1;
    }
    uint8_t *file = (uint8_t *)malloc((size_t)EXE_MAX + 1u);
    if (file == NULL) {
        (void)fclose(f);
        refuse(path, "out of memory");
        return 1;
    }
    /* One byte more than the largest valid file, so a longer file is seen as
       too long rather than cut to a valid size. */
    size_t n = fread(file, 1u, (size_t)EXE_MAX + 1u, f);
    int read_ok = ferror(f) == 0;
    (void)fclose(f);
    if (!read_ok) {
        free(file);
        refuse(path, "cannot read the file");
        return 1;
    }
    if (n == 0u) {
        free(file);
        refuse(path, "the file is empty");
        return 1;
    }
    schway_exe_info info;
    memset(&info, 0, sizeof info);
    info.size = (uint32_t)sizeof info;
    schway_status st = schway_probe_exe(file, n, &info);
    if (st != SCHWAY_STATUS_OK) {
        free(file);
        refuse(path, schway_status_string(st));
        return 1;
    }

    schway__sha256 s;
    uint8_t digest[32];
    char sha_hex[65];
    char md5_hex[33];
    schway__sha256_init(&s);
    schway__sha256_update(&s, file, n);
    schway__sha256_final(&s, digest);
    schway__sha256_hex(digest, sha_hex);
    rcheevos_md5(file, info.hash_len, md5_hex);
    free(file);

    fputs("format=exe\nserial=n/a\n", out);
    fprintf(out, "licence_area=%s\nregion_guess=%s\nexe_name=", licence_area_name(info.licence_area),
            region_guess(info.licence_area));
    put_text(out, base_name(path));
    fprintf(out, "\nexe_size=%lu\nsha256=%s\nrcheevos_md5=%s\n", (unsigned long)n, sha_hex, md5_hex);
    fprintf(out, "pc0=0x%08lx\ngp0=0x%08lx\n", (unsigned long)info.pc0, (unsigned long)info.gp0);
    fprintf(out, "t_addr=0x%08lx\nt_size=0x%08lx\n", (unsigned long)info.t_addr,
            (unsigned long)info.t_size);
    fprintf(out, "d_addr=0x%08lx\nd_size=0x%08lx\n", (unsigned long)info.d_addr,
            (unsigned long)info.d_size);
    fprintf(out, "b_addr=0x%08lx\nb_size=0x%08lx\n", (unsigned long)info.b_addr,
            (unsigned long)info.b_size);
    fprintf(out, "s_addr=0x%08lx\ns_size=0x%08lx\n", (unsigned long)info.s_addr,
            (unsigned long)info.s_size);
    fprintf(out, "lint=0x%08lx\n", (unsigned long)info.lint);
    return 0;
}
