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

typedef struct expected_v10_input {
    schway_input v9;
    schway_gun_input guns[2];
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
_Static_assert(SCHWAY_API_VERSION >= 10, "API v10 remains available");
_Static_assert(SCHWAY_INPUT_DEVICE_GUNCON == 7u, "API v10 GunCon identifier");
_Static_assert(SCHWAY_INPUT_DEVICE_JUSTIFIER == 8u, "API v10 Justifier identifier");
_Static_assert(SCHWAY_GUN_INPUT_VALID == 1u, "API v10 valid flag");
_Static_assert(SCHWAY_GUN_INPUT_TRIGGER == 2u, "API v10 trigger flag");
_Static_assert(SCHWAY_GUN_INPUT_OFFSCREEN == 4u, "API v10 offscreen flag");
_Static_assert(SCHWAY_GUN_INPUT_RELOAD == 8u, "API v10 reload flag");
_Static_assert(offsetof(schway_gun_input, x) == 0u, "API v10 gun.x");
_Static_assert(offsetof(schway_gun_input, y) == 4u, "API v10 gun.y");
_Static_assert(offsetof(schway_gun_input, flags) == 8u, "API v10 gun.flags");
_Static_assert(offsetof(schway_gun_input, reserved0) == 12u, "API v10 gun.reserved0");
_Static_assert(sizeof(schway_gun_input) == 16u, "API v10 gun input size");
_Static_assert(offsetof(expected_v10_input, guns) == 228u, "API v10 guns offset");
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

static void expect_rejected_v10_input(schway *inst,
                                      const schway_current_input *baseline,
                                      const void *invalid,
                                      schway_status expected)
{
    CHECK_EQ_U64(schway_set_input(inst, baseline), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_set_input(inst, invalid), expected);
    CHECK_EQ_U64(schway_run_frame(inst, NULL), SCHWAY_STATUS_OK);
    observed_input observed;
    check_observed_input(inst, &observed);
    CHECK_EQ_U64(observed.devices[0], baseline->device[0]);
    CHECK_EQ_U64(observed.devices[1], baseline->device[1]);
    CHECK_EQ_U64(observed.buttons[0], (uint16_t)~baseline->buttons[0]);
    CHECK_EQ_U64(observed.buttons[1], (uint16_t)~baseline->buttons[1]);
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

    CHECK(SCHWAY_API_VERSION >= 10u);
    expected_v10_input extended;
    memset(&extended, 0, sizeof extended);
    extended.v9.size = (uint32_t)sizeof extended;
    extended.v9.device[0] = SCHWAY_INPUT_DEVICE_GUNCON;
    extended.v9.device[1] = SCHWAY_INPUT_DEVICE_JUSTIFIER;
    extended.guns[0].x = 0;
    extended.guns[0].y = 0;
    extended.guns[0].flags = SCHWAY_GUN_INPUT_VALID | SCHWAY_GUN_INPUT_TRIGGER;
    extended.guns[1].x = 17;
    extended.guns[1].y = 29;
    extended.guns[1].flags = SCHWAY_GUN_INPUT_VALID;
    CHECK_EQ_U64(schway_set_input(inst, &extended), SCHWAY_STATUS_OK);

    schway_current_input baseline;
    memset(&baseline, 0, sizeof baseline);
    baseline.size = (uint32_t)sizeof baseline;
    baseline.device[0] = SCHWAY_INPUT_DEVICE_DIGITAL;
    baseline.device[1] = SCHWAY_INPUT_DEVICE_DUALSHOCK;
    baseline.buttons[0] = UINT16_C(0xA55A);
    baseline.buttons[1] = UINT16_C(0xC33C);
    baseline.guns[0].flags = SCHWAY_GUN_INPUT_VALID | SCHWAY_GUN_INPUT_TRIGGER;
    schway_current_input invalid = baseline;
    invalid.buttons[0] ^= UINT16_C(0xFFFF);
    invalid.guns[0].flags = 0x10u;
    expect_rejected_v10_input(inst, &baseline, &invalid, SCHWAY_STATUS_ERR_ARGUMENT);
    invalid = baseline;
    invalid.buttons[0] ^= UINT16_C(0xFFFF);
    invalid.guns[0].reserved0 = 1u;
    expect_rejected_v10_input(inst, &baseline, &invalid, SCHWAY_STATUS_ERR_ARGUMENT);
    invalid = baseline;
    invalid.buttons[0] ^= UINT16_C(0xFFFF);
    invalid.guns[0].x = -1;
    expect_rejected_v10_input(inst, &baseline, &invalid, SCHWAY_STATUS_ERR_ARGUMENT);
    invalid = baseline;
    invalid.buttons[0] ^= UINT16_C(0xFFFF);
    invalid.device[0] = SCHWAY_INPUT_DEVICE_GUNCON;
    invalid.multitap[0] = 1u;
    expect_rejected_v10_input(inst, &baseline, &invalid, SCHWAY_STATUS_ERR_ARGUMENT);
    invalid = baseline;
    invalid.buttons[0] ^= UINT16_C(0xFFFF);
    invalid.multitap[0] = 1u;
    invalid.controllers[0][2].device = SCHWAY_INPUT_DEVICE_JUSTIFIER;
    expect_rejected_v10_input(inst, &baseline, &invalid, SCHWAY_STATUS_ERR_ARGUMENT);

    /* Every partial size from the released prefix through the current object
       is an exact caller allocation and is normalized without an overread. */
    for (uint32_t size = (uint32_t)sizeof(schway_input);
         size <= (uint32_t)sizeof(schway_current_input); size++) {
        uint8_t *partial = (uint8_t *)calloc((size_t)size, 1u);
        CHECK(partial != NULL);
        if (partial == NULL) {
            schway_destroy(inst);
            CHECK_DONE();
        }
        memcpy(partial, &size, sizeof size);
        CHECK_EQ_U64(schway_set_input(inst, partial), SCHWAY_STATUS_OK);
        free(partial);
    }

    /* A size-tagged input can end partway through a flags word. Keep the
       supplied prefix byte zero so the partial object representation is
       valid on both little- and big-endian hosts; omitted bytes are zero-filled. */
    uint8_t partial_flags[237];
    memset(partial_flags, 0, sizeof partial_flags);
    uint32_t partial_size = (uint32_t)sizeof partial_flags;
    uint32_t guncon = SCHWAY_INPUT_DEVICE_GUNCON;
    memcpy(partial_flags, &partial_size, sizeof partial_size);
    memcpy(partial_flags + offsetof(schway_input, device), &guncon, sizeof guncon);
    CHECK_EQ_U64(schway_set_input(inst, partial_flags), SCHWAY_STATUS_OK);

    /* Supply a complete flags word to check valid GunCon input independent of
       the host byte order. */
    uint8_t complete_flags[240];
    memset(complete_flags, 0, sizeof complete_flags);
    partial_size = (uint32_t)sizeof complete_flags;
    uint32_t valid_flags = SCHWAY_GUN_INPUT_VALID | SCHWAY_GUN_INPUT_TRIGGER;
    memcpy(complete_flags, &partial_size, sizeof partial_size);
    memcpy(complete_flags + offsetof(schway_input, device), &guncon, sizeof guncon);
    memcpy(complete_flags + 236u, &valid_flags, sizeof valid_flags);
    CHECK_EQ_U64(schway_set_input(inst, complete_flags), SCHWAY_STATUS_OK);

    uint8_t partial_bad[241];
    memset(partial_bad, 0, sizeof partial_bad);
    memcpy(partial_bad, &baseline, sizeof(schway_input));
    partial_size = (uint32_t)sizeof partial_bad;
    memcpy(partial_bad, &partial_size, sizeof partial_size);
    uint32_t valid_flag = SCHWAY_GUN_INPUT_VALID;
    memcpy(partial_bad + 236u, &valid_flag, sizeof valid_flag);
    partial_bad[240] = 1u;
    expect_rejected_v10_input(inst, &baseline, partial_bad, SCHWAY_STATUS_ERR_ARGUMENT);

    uint8_t oversized[sizeof(schway_current_input) + 8u];
    memset(oversized, 0, sizeof oversized);
    memcpy(oversized, &baseline, sizeof baseline);
    uint32_t oversized_size = (uint32_t)sizeof oversized;
    memcpy(oversized, &oversized_size, sizeof oversized_size);
    CHECK_EQ_U64(schway_set_input(inst, oversized), SCHWAY_STATUS_OK);
    oversized[sizeof oversized - 1u] = 1u;
    expect_rejected_v10_input(inst, &baseline, oversized, SCHWAY_STATUS_ERR_STRUCT_SIZE);

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
