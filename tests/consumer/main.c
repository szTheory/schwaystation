/* A program that knows only the installed headers: it supplies its own
   allocator, creates an instance, runs one frame, reads the frame's geometry
   and destroys the instance. Exits 0 and prints one fixed line on success,
   1 otherwise. */
#include <schwaystation.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The core has no default allocator; the caller passes one. */
static void *consumer_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void consumer_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

int main(void)
{
    schway_versions v;
    schway_config cfg;
    schway_video video;
    schway *inst = NULL;
    schway_status st;
    int ok;

    /* Zero every struct, then set its size, as the header requires. */
    memset(&v, 0, sizeof v);
    v.size = (uint32_t)sizeof v;
    if (schway_get_versions(&v) != SCHWAY_STATUS_OK) {
        printf("schway_get_versions failed\n");
        return 1;
    }

    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = consumer_alloc;
    cfg.allocator.free = consumer_free;
    st = schway_create(&cfg, &inst);
    if (st != SCHWAY_STATUS_OK) {
        printf("schway_create: %s\n", schway_status_string(st));
        return 1;
    }

    memset(&video, 0, sizeof video);
    video.size = (uint32_t)sizeof video;
    st = schway_run_frame(inst, NULL);
    if (st == SCHWAY_STATUS_OK) {
        st = schway_get_video(inst, &video);
    }
    /* The test card fills the whole 320 x 240 display area. */
    ok = st == SCHWAY_STATUS_OK && video.pixels != NULL && video.width == 320u &&
         video.height == 240u;
    schway_destroy(inst);
    if (!ok) {
        printf("frame failed: %s, %ux%u\n", schway_status_string(st), (unsigned)video.width,
               (unsigned)video.height);
        return 1;
    }
    printf("consumer: api %u, one frame %ux%u\n", (unsigned)v.api, (unsigned)video.width,
           (unsigned)video.height);
    return 0;
}
