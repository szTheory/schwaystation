#ifndef SCHWAYSTATION_RUNNER_INPUTS_H
#define SCHWAYSTATION_RUNNER_INPUTS_H

#include <stddef.h>
#include <stdint.h>

#include "schwaystation.h"

#define SCHWAY__INPUTS_MAX_FRAME 1000000u

typedef struct schway__input_event {
    uint32_t frame;
    uint32_t line;
    schway_input input;
} schway__input_event;

typedef struct schway__input_movie {
    schway__input_event *events;
    size_t count;
} schway__input_movie;

typedef enum schway__inputs_status {
    SCHWAY__INPUTS_OK,
    SCHWAY__INPUTS_IO,
    SCHWAY__INPUTS_INVALID,
    SCHWAY__INPUTS_MEMORY
} schway__inputs_status;

typedef struct schway__inputs_error {
    uint32_t line;
    const char *reason;
} schway__inputs_error;

schway__inputs_status schway__inputs_load(const char *path, schway__input_movie *movie,
                                          schway__inputs_error *error);
schway_status schway__inputs_apply_frame(schway *inst, const schway__input_movie *movie,
                                         size_t *cursor, schway_input *snapshot, uint32_t frame);
void schway__inputs_release(schway__input_movie *movie);

#endif /* SCHWAYSTATION_RUNNER_INPUTS_H */
