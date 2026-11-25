/* API v10 freezes the released v9 input layout for existing callers. */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Keep the current API declarations available under private names, while
   this translation unit owns the complete released v9 caller layout. */
#define schway_controller_input schway_current_controller_input
#define schway_input schway_current_input
#define schway_set_input schway_set_input_current
#include "schwaystation.h"
#undef schway_set_input
#undef schway_input
#undef schway_controller_input

#include "../check.h"

typedef struct schway_controller_input {
    uint32_t device;
    uint16_t buttons;
    uint8_t axes[4];
    uint8_t analog_button;
    uint8_t reserved0;
    int16_t mouse_dx, mouse_dy;
    uint8_t negcon_twist, negcon_i, negcon_ii, negcon_l;
    uint16_t jogcon_dial;
    uint16_t jogcon_flags;
} schway_controller_input;

typedef struct schway_input {
    uint32_t size;
    uint32_t device[2];
    uint16_t buttons[2];
    uint8_t axes[2][4];
    uint8_t analog_button[2];
    uint16_t reserved0;
    uint32_t multitap[2];
    schway_controller_input controllers[2][4];
} schway_input;

/* RED contract probe: model the planned append without depending on names
   that the public API has not added yet. */
typedef struct expected_gun_input {
    int32_t x;
    int32_t y;
    uint32_t flags;
    uint32_t reserved0;
} expected_gun_input;

typedef struct expected_v10_input {
    schway_input v9;
    expected_gun_input guns[2];
} expected_v10_input;

schway_status schway_set_input(schway *inst, const void *input);

_Static_assert(offsetof(schway_input, size) == 0u, "API v9 input.size");
_Static_assert(offsetof(schway_input, device) == 4u, "API v9 input.device");
_Static_assert(offsetof(schway_input, buttons) == 12u, "API v9 input.buttons");
_Static_assert(offsetof(schway_input, axes) == 16u, "API v9 input.axes");
_Static_assert(offsetof(schway_input, analog_button) == 24u,
               "API v9 input.analog_button");
_Static_assert(offsetof(schway_input, reserved0) == 26u, "API v9 input.reserved0");
_Static_assert(offsetof(schway_input, multitap) == 28u, "API v9 input.multitap");
_Static_assert(offsetof(schway_input, controllers) == 36u,
               "API v9 input.controllers");
_Static_assert(sizeof(schway_controller_input) == 24u, "API v9 endpoint input size");
_Static_assert(offsetof(schway_controller_input, device) == 0u,
               "API v9 endpoint.device");
_Static_assert(offsetof(schway_controller_input, buttons) == 4u,
               "API v9 endpoint.buttons");
_Static_assert(offsetof(schway_controller_input, axes) == 6u,
               "API v9 endpoint.axes");
_Static_assert(offsetof(schway_controller_input, analog_button) == 10u,
               "API v9 endpoint.analog_button");
_Static_assert(offsetof(schway_controller_input, reserved0) == 11u,
               "API v9 endpoint.reserved0");
_Static_assert(offsetof(schway_controller_input, mouse_dx) == 12u,
               "API v9 endpoint.mouse_dx");
_Static_assert(offsetof(schway_controller_input, mouse_dy) == 14u,
               "API v9 endpoint.mouse_dy");
_Static_assert(offsetof(schway_controller_input, negcon_twist) == 16u,
               "API v9 endpoint.negcon_twist");
_Static_assert(offsetof(schway_controller_input, negcon_i) == 17u,
               "API v9 endpoint.negcon_i");
_Static_assert(offsetof(schway_controller_input, negcon_ii) == 18u,
               "API v9 endpoint.negcon_ii");
_Static_assert(offsetof(schway_controller_input, negcon_l) == 19u,
               "API v9 endpoint.negcon_l");
_Static_assert(offsetof(schway_controller_input, jogcon_dial) == 20u,
               "API v9 endpoint.jogcon_dial");
_Static_assert(offsetof(schway_controller_input, jogcon_flags) == 22u,
               "API v9 endpoint.jogcon_flags");
_Static_assert(sizeof(((schway_input *)0)->controllers[0]) == 96u,
               "API v9 four-endpoint row stride");
_Static_assert(sizeof(((schway_input *)0)->controllers) == 192u,
               "API v9 eight-endpoint stride");
_Static_assert(sizeof(schway_input) == 228u, "API v9 input size");
_Static_assert(offsetof(expected_v10_input, guns) == 228u, "API v10 guns offset");
_Static_assert(sizeof(expected_gun_input) == 16u, "API v10 gun input size");
_Static_assert(sizeof(expected_v10_input) == 260u, "API v10 input size");

typedef struct observed_input {
    uint16_t buttons[8];
    uint8_t axes[32];
    uint8_t devices[8];
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
    if (strcmp(name, "pad.buttons") == 0 && count == 8u && width == 2u) {
        memcpy(observed->buttons, data, sizeof observed->buttons);
        observed->saw_buttons = 1u;
    } else if (strcmp(name, "pad.axes") == 0 && count == 32u && width == 1u) {
        memcpy(observed->axes, data, sizeof observed->axes);
        observed->saw_axes = 1u;
    } else if (strcmp(name, "pad.device") == 0 && count == 8u && width == 1u) {
        memcpy(observed->devices, data, sizeof observed->devices);
        observed->saw_devices = 1u;
    }
}

static void check_observed_input(schway *inst, observed_input *observed)
{
    memset(observed, 0, sizeof *observed);
    CHECK_EQ_U64(schway_state_visit(inst, observe_input, observed), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(observed->saw_buttons, 1u);
    CHECK_EQ_U64(observed->saw_axes, 1u);
    CHECK_EQ_U64(observed->saw_devices, 1u);
}

static uint32_t endpoint_index(uint32_t port, uint32_t socket)
{
    return socket == 0u ? port : 2u + port * 3u + socket - 1u;
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

    CHECK_EQ_U64(SCHWAY_API_VERSION, 10u);
    expected_v10_input extended;
    memset(&extended, 0, sizeof extended);
    extended.v9.size = (uint32_t)sizeof extended;
    extended.v9.device[0] = SCHWAY_INPUT_DEVICE_DIGITAL;
    extended.guns[0].x = 0;
    extended.guns[0].y = 0;
    extended.guns[0].flags = 1u; /* explicit validity makes (0, 0) meaningful */
    CHECK_EQ_U64(schway_set_input(inst, &extended), SCHWAY_STATUS_OK);

    /* Allocate exactly the released v9 object extent: no gun extension exists. */
    schway_input *input = (schway_input *)calloc(1u, sizeof *input);
    CHECK(input != NULL);
    if (input == NULL) {
        schway_destroy(inst);
        CHECK_DONE();
    }
    input->size = (uint32_t)sizeof *input;

    for (uint32_t port = 0u; port < 2u; port++) {
        input->device[port] = port == 0u ? SCHWAY_INPUT_DEVICE_DIGITAL
                                        : SCHWAY_INPUT_DEVICE_DUALSHOCK;
        input->buttons[port] = (uint16_t)(UINT16_C(0xA520) + port * 0x111u);
        for (uint32_t axis = 0u; axis < 4u; axis++) {
            input->axes[port][axis] = (uint8_t)(17u + port * 80u + axis * 13u);
        }
    }
    CHECK_EQ_U64(schway_set_input(inst, input), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_run_frame(inst, NULL), SCHWAY_STATUS_OK);

    observed_input observed;
    check_observed_input(inst, &observed);
    for (uint32_t port = 0u; port < 2u; port++) {
        CHECK_EQ_U64(observed.devices[port], input->device[port]);
        CHECK_EQ_U64(observed.buttons[port], (uint16_t)~input->buttons[port]);
        CHECK(memcmp(&observed.axes[port * 4u], input->axes[port], 4u) == 0);
    }

    input->multitap[0] = 1u;
    input->multitap[1] = 1u;
    for (uint32_t port = 0u; port < 2u; port++) {
        for (uint32_t socket = 0u; socket < 4u; socket++) {
            uint32_t endpoint = endpoint_index(port, socket);
            schway_controller_input *controller = &input->controllers[port][socket];
            controller->device = (endpoint % 2u) == 0u ? SCHWAY_INPUT_DEVICE_DIGITAL
                                                       : SCHWAY_INPUT_DEVICE_DUALSHOCK;
            controller->buttons = (uint16_t)(UINT16_C(0x5A00) + endpoint * 0x31u);
            for (uint32_t axis = 0u; axis < 4u; axis++) {
                controller->axes[axis] = (uint8_t)(9u + endpoint * 17u + axis * 7u);
            }
        }
    }
    CHECK_EQ_U64(schway_set_input(inst, input), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_run_frame(inst, NULL), SCHWAY_STATUS_OK);
    check_observed_input(inst, &observed);
    for (uint32_t port = 0u; port < 2u; port++) {
        for (uint32_t socket = 0u; socket < 4u; socket++) {
            uint32_t endpoint = endpoint_index(port, socket);
            const schway_controller_input *controller = &input->controllers[port][socket];
            CHECK_EQ_U64(observed.devices[endpoint], controller->device);
            CHECK_EQ_U64(observed.buttons[endpoint], (uint16_t)~controller->buttons);
            CHECK(memcmp(&observed.axes[endpoint * 4u], controller->axes, 4u) == 0);
        }
    }

    free(input);
    schway_destroy(inst);
    CHECK_DONE();
}
