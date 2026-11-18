/* Bounded replay of hostile SYSTEM.CNF bytes through the production parser. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disc_image.h"

#define INPUT_MAX 4096u

static void broken(const char *what)
{
    fprintf(stderr, "fuzz.regress.cnf: %s\n", what);
    abort();
}

static int matches(const uint8_t *bytes, size_t size, const char *text)
{
    size_t expected = strlen(text);
    return size == expected && memcmp(bytes, text, expected) == 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size > INPUT_MAX) return 0;
    static const char valid[] =
        "BOOT = cdrom:\\SLUS_123.45;1\r\nTCB = 64\r\nEVENT = 256\r\nSTACK = 801FFF00h\r\n";
    static const char escape[] = "BOOT = CDROM:\\..\\SLUS_123.45;1\n";
    schway_disc_system_cnf parsed;
    memset(&parsed, 0xA5, sizeof parsed);
    schway_disc_image_status status = schway_disc_image_parse_system_cnf(data, size, &parsed);
    if (status == SCHWAY_DISC_IMAGE_OK) {
        if (parsed.size != (uint32_t)sizeof parsed || parsed.boot_name[0] == '\0') {
            broken("successful CNF parse returned an invalid result");
        }
    } else if (status != SCHWAY_DISC_IMAGE_BAD_SYSCNF) {
        broken("CNF parser returned an unexpected status");
    }
    if ((matches(data, size, valid) && status != SCHWAY_DISC_IMAGE_OK) ||
        (matches(data, size, escape) && status != SCHWAY_DISC_IMAGE_BAD_SYSCNF)) {
        broken("known valid or escaping CNF seed changed its parser result");
    }
    return 0;
}

#define SCHWAY_REPLAY_NAME "fuzz.regress.cnf"
#include "replay_main.h"
