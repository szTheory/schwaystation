/* API v9 first freezes the released API v8 input prefix for older callers. */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Compile this translation unit's caller view with the released v8 input
   declaration, while retaining the current declarations for every other API. */
#define schway_input schway_current_input
#define schway_set_input schway_set_input_current
#include "schwaystation.h"
#undef schway_set_input
#undef schway_input

#include "../check.h"

typedef struct schway_input {
    uint32_t size;
    uint32_t device[2];
    uint16_t buttons[2];
    uint8_t axes[2][4];
    uint8_t analog_button[2];
    uint16_t reserved0;
} schway_input;

schway_status schway_set_input(schway *inst, const schway_input *input);

_Static_assert(sizeof(schway_input) == 28u, "released v8 input size");
_Static_assert(offsetof(schway_input, size) == 0u, "released v8 input.size");
_Static_assert(offsetof(schway_input, device) == 4u, "released v8 input.device");
_Static_assert(offsetof(schway_input, buttons) == 12u, "released v8 input.buttons");
_Static_assert(offsetof(schway_input, axes) == 16u, "released v8 input.axes");
_Static_assert(offsetof(schway_input, analog_button) == 24u,
               "released v8 input.analog_button");
_Static_assert(offsetof(schway_input, reserved0) == 26u, "released v8 input.reserved0");
_Static_assert(sizeof(schway_controller_input) == 24u, "API v9 endpoint input size");
_Static_assert(offsetof(schway_current_input, multitap) == 28u, "API v9 input.multitap");
_Static_assert(offsetof(schway_current_input, controllers) == 36u,
               "API v9 input.controllers");
_Static_assert(sizeof(schway_current_input) == 228u, "API v9 input size");

typedef struct observed_input {
    uint16_t buttons[2];
    uint8_t axes[8];
    uint8_t devices[2];
    uint32_t saw_buttons;
    uint32_t saw_axes;
    uint32_t saw_devices;
} observed_input;

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

static void observe_input(void *user, const char *name, const void *data,
                          uint32_t count, uint32_t width)
{
    observed_input *observed = (observed_input *)user;
    if (strcmp(name, "pad.buttons") == 0 && count == 2u && width == 2u) {
        memcpy(observed->buttons, data, sizeof observed->buttons);
        observed->saw_buttons = 1u;
    } else if (strcmp(name, "pad.axes") == 0 && count == 8u && width == 1u) {
        memcpy(observed->axes, data, sizeof observed->axes);
        observed->saw_axes = 1u;
    } else if (strcmp(name, "pad.device") == 0 && count == 2u && width == 1u) {
        memcpy(observed->devices, data, sizeof observed->devices);
        observed->saw_devices = 1u;
    }
}

int main(void)
{
    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = test_alloc;
    config.allocator.free = test_free;

    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&config, &inst), SCHWAY_STATUS_OK);
    if (inst == NULL) CHECK_DONE();

    schway_input input;
    memset(&input, 0, sizeof input);
    input.size = 28u;
    input.device[0] = SCHWAY_INPUT_DEVICE_DIGITAL;
    input.device[1] = SCHWAY_INPUT_DEVICE_DUALSHOCK;
    input.buttons[0] = UINT16_C(0xA55A);
    input.buttons[1] = UINT16_C(0xC33C);
    input.axes[0][0] = 17u;
    input.axes[0][1] = 39u;
    input.axes[0][2] = 81u;
    input.axes[0][3] = 123u;
    input.axes[1][0] = 145u;
    input.axes[1][1] = 167u;
    input.axes[1][2] = 209u;
    input.axes[1][3] = 231u;

    /* This pointer has exactly the 28 bytes allocated by a released v8 caller. */
    schway_status old_status = schway_set_input(inst, &input);
    if (old_status != SCHWAY_STATUS_OK) {
        CHECK_EQ_U64(old_status, SCHWAY_STATUS_OK);
        schway_destroy(inst);
        CHECK_DONE();
    }
    CHECK_EQ_U64(schway_run_frame(inst, NULL), SCHWAY_STATUS_OK);

    observed_input observed;
    memset(&observed, 0, sizeof observed);
    CHECK_EQ_U64(schway_state_visit(inst, observe_input, &observed), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(observed.saw_buttons, 1u);
    CHECK_EQ_U64(observed.saw_axes, 1u);
    CHECK_EQ_U64(observed.saw_devices, 1u);
    CHECK_EQ_HEX(observed.buttons[0], (uint16_t)~input.buttons[0]);
    CHECK_EQ_HEX(observed.buttons[1], (uint16_t)~input.buttons[1]);
    CHECK(memcmp(observed.axes, input.axes, sizeof observed.axes) == 0);
    CHECK_EQ_U64(observed.devices[0], SCHWAY_INPUT_DEVICE_DIGITAL);
    CHECK_EQ_U64(observed.devices[1], SCHWAY_INPUT_DEVICE_DUALSHOCK);

    schway_destroy(inst);
    CHECK_DONE();
}
