/* XA-ADPCM sector decode tests through the internal CD decoder seam. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "cdxa.h"
#include "internal.h"
#include "../check.h"

static void *test_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void test_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static schway *make(void)
{
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = test_alloc;
    cfg.allocator.free = test_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) {
        exit(1);
    }
    return inst;
}

static void make_sector(uint8_t sector[SCHWAY__CDXA_RAW_SECTOR_BYTES], uint8_t coding)
{
    memset(sector, 0, SCHWAY__CDXA_RAW_SECTOR_BYTES);
    memset(&sector[1], 0xFF, 10u);
    sector[11] = 0u;
    sector[15] = 2u;
    sector[16] = 1u;
    sector[17] = 2u;
    sector[18] = 0x64u;
    sector[19] = coding;
    (void)memcpy(&sector[20], &sector[16], 4u);
    for (uint32_t portion = 0u; portion < 18u; portion++) {
        uint8_t *chunk = &sector[24u + portion * 128u];
        for (uint32_t header = 0u; header < 8u; header++) {
            chunk[4u + header] = 0u;
        }
        (void)memcpy(chunk, &chunk[4], 4u);
        (void)memcpy(&chunk[12], &chunk[8], 4u);
        memset(&chunk[16], 0x11, 112u);
    }
}

static void decode_form2_audio(void)
{
    schway *inst = make();
    uint8_t sector[SCHWAY__CDXA_RAW_SECTOR_BYTES];
    int16_t output[SCHWAY__CDXA_MAX_OUTPUT_FRAMES * 2u];
    uint32_t output_frames = 0u;
    make_sector(sector, 1u);
    uint32_t decoded = schway__cdxa_decode(inst, sector, sizeof sector, output,
                                             SCHWAY__CDXA_MAX_OUTPUT_FRAMES, &output_frames);
    CHECK_EQ_U64(decoded, 1u);
    CHECK_EQ_U64(output_frames, 2352u);
    CHECK_EQ_U64(inst->cdrom.xa_history[0][0], 4096u);
    CHECK_EQ_U64(inst->cdrom.xa_history[1][0], 4096u);
    schway_destroy(inst);
}

int main(void)
{
    decode_form2_audio();
    CHECK_DONE();
}
