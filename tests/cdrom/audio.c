/* CD audio enters the native SPU sample path through a bounded device queue. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"
#include "cdrom.h"
#include "spu.h"
#include "../check.h"

static void *audio_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void audio_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static schway *audio_make(void)
{
    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = audio_alloc;
    config.allocator.free = audio_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&config, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) {
        exit(1);
    }
    return inst;
}

static void cdda_queue_drains_at_native_tick(void)
{
    schway *inst = audio_make();
    inst->cdrom.audio[0][0] = 1200u;
    inst->cdrom.audio[0][1] = (uint16_t)(int16_t)-2400;
    inst->cdrom.audio_count = 1u;

    schway__spu_event(inst, inst->spu.next_tick);

    CHECK_EQ_U64(inst->cdrom.audio_count, 0u);
    schway_destroy(inst);
}

int main(int argc, char **argv)
{
    if (argc != 2 || strcmp(argv[1], "cdda") != 0) {
        return 2;
    }
    cdda_queue_drains_at_native_tick();
    return check_failures == 0 ? 0 : 1;
}
