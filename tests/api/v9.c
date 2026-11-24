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

schway_status schway_set_input(schway *inst, const void *input);

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
_Static_assert(SCHWAY_API_VERSION == 9, "API v9 version");
_Static_assert(SCHWAY_INPUT_DEVICE_NONE == 0u, "API v9 none identifier");
_Static_assert(SCHWAY_INPUT_DEVICE_DIGITAL == 1u, "API v9 digital identifier");
_Static_assert(SCHWAY_INPUT_DEVICE_DUALSHOCK == 2u, "API v9 DualShock identifier");
_Static_assert(SCHWAY_INPUT_DEVICE_MOUSE == 3u, "API v9 mouse identifier");
_Static_assert(SCHWAY_INPUT_DEVICE_NEGCON == 4u, "API v9 NegCon identifier");
_Static_assert(SCHWAY_INPUT_DEVICE_JOGCON == 5u, "API v9 JogCon identifier");
_Static_assert(SCHWAY_INPUT_DEVICE_DUAL_ANALOG_FLIGHT == 6u,
               "API v9 dual analog flight identifier");

typedef struct observed_input {
    uint16_t buttons[8];
    uint8_t axes[32];
    uint8_t devices[8];
    uint32_t saw_buttons;
    uint32_t saw_axes;
    uint32_t saw_devices;
} observed_input;

typedef struct observed_card_state {
    char names[160][96];
    uint32_t count;
    uint32_t duplicate;
    uint32_t phase_seen[8];
} observed_card_state;

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

static void observe_card_state(void *user, const char *name, const void *data,
                               uint32_t count, uint32_t width)
{
    observed_card_state *observed = (observed_card_state *)user;
    static const char *const phase_names[8] = {
        "card.slot1.phase", "card.port1.socketB.phase", "card.port1.socketC.phase",
        "card.port1.socketD.phase", "card.slot2.phase", "card.port2.socketB.phase",
        "card.port2.socketC.phase", "card.port2.socketD.phase"
    };
    (void)data;
    if (strncmp(name, "card.", 5u) != 0) {
        return;
    }
    if (observed->count < 160u) {
        for (uint32_t i = 0u; i < observed->count; i++) {
            if (strcmp(observed->names[i], name) == 0) {
                observed->duplicate = 1u;
            }
        }
        size_t length = strlen(name);
        if (length < sizeof observed->names[0]) {
            memcpy(observed->names[observed->count], name, length + 1u);
            observed->count++;
        } else {
            observed->duplicate = 1u;
        }
    } else {
        observed->duplicate = 1u;
    }
    if (count == 1u && width == 4u) {
        for (uint32_t i = 0u; i < 8u; i++) {
            if (strcmp(name, phase_names[i]) == 0) {
                observed->phase_seen[i] = 1u;
            }
        }
    }
}

static int check_card_socket_api(schway *inst)
{
    uint32_t failures = 0u;
    uint32_t endpoint = 0u;
    uint32_t loads_ok = 0u;
    uint32_t gets_ok = 0u;
    uint32_t generations_ok = 0u;
    schway_span spans[8];
    uint8_t inputs[8][2];
    memset(spans, 0, sizeof spans);
    memset(inputs, 0, sizeof inputs);
    for (uint32_t port = SCHWAY_CARD_PORT_1; port <= SCHWAY_CARD_PORT_2; port++) {
        for (uint32_t socket = SCHWAY_CARD_SOCKET_A; socket <= SCHWAY_CARD_SOCKET_D; socket++) {
            inputs[endpoint][0] = (uint8_t)(0x31u + endpoint);
            inputs[endpoint][1] = (uint8_t)(0xA0u + endpoint);
            if (schway_load_memcard_socket(inst, port, socket, inputs[endpoint],
                                           sizeof inputs[endpoint]) == SCHWAY_STATUS_OK) {
                loads_ok++;
            } else {
                failures++;
            }
            if (schway_get_memcard(inst, port, socket, &spans[endpoint]) == SCHWAY_STATUS_OK) {
                gets_ok++;
                if (spans[endpoint].size != SCHWAY_MEMCARD_SIZE ||
                    ((const uint8_t *)spans[endpoint].data)[0] != inputs[endpoint][0] ||
                    ((const uint8_t *)spans[endpoint].data)[1] != inputs[endpoint][1] ||
                    ((const uint8_t *)spans[endpoint].data)[2] != 0xFFu) {
                    failures++;
                }
            } else {
                failures++;
            }
            uint64_t generation = UINT64_MAX;
            if (schway_memcard_socket_generation(inst, port, socket, &generation) !=
                    SCHWAY_STATUS_OK || generation != 0u) {
                failures++;
            } else {
                generations_ok++;
            }
            endpoint++;
        }
    }
    for (uint32_t i = 0u; i < 8u; i++) {
        for (uint32_t j = i + 1u; j < 8u; j++) {
            if (spans[i].data != NULL && spans[i].data == spans[j].data &&
                inputs[i][0] != inputs[j][0]) {
                failures++;
            }
        }
    }
    schway_span legacy;
    if (schway_get_memory(inst, SCHWAY_MEMORY_CARD1, &legacy) != SCHWAY_STATUS_OK ||
        legacy.data != spans[0].data || legacy.size != spans[0].size) {
        failures++;
    }
    if (schway_get_memory(inst, SCHWAY_MEMORY_CARD2, &legacy) != SCHWAY_STATUS_OK ||
        legacy.data != spans[4].data || legacy.size != spans[4].size) {
        failures++;
    }
    schway_span untouched = {inputs[0], 19u};
    if (schway_get_memcard(inst, 0u, SCHWAY_CARD_SOCKET_A, &untouched) !=
            SCHWAY_STATUS_ERR_ARGUMENT || untouched.data != inputs[0] || untouched.size != 19u) {
        failures++;
    }
    if (schway_get_memcard(inst, SCHWAY_CARD_PORT_1, 0u, &untouched) !=
            SCHWAY_STATUS_ERR_ARGUMENT || untouched.data != inputs[0] || untouched.size != 19u) {
        failures++;
    }
    observed_card_state state;
    memset(&state, 0, sizeof state);
    if (schway_state_visit(inst, observe_card_state, &state) != SCHWAY_STATUS_OK ||
        state.count != 136u || state.duplicate != 0u) {
        failures++;
    }
    for (uint32_t i = 0u; i < 8u; i++) {
        failures += state.phase_seen[i] == 1u ? 0u : 1u;
    }
    uint32_t unique_spans = 0u;
    uint32_t phase_paths = 0u;
    for (uint32_t i = 0u; i < 8u; i++) {
        phase_paths += state.phase_seen[i];
        if (spans[i].data == NULL) {
            continue;
        }
        uint32_t first = 1u;
        for (uint32_t j = 0u; j < i; j++) {
            if (spans[j].data == spans[i].data) {
                first = 0u;
            }
        }
        unique_spans += first;
    }
    if (schway_run_frame(inst, NULL) != SCHWAY_STATUS_OK) {
        failures++;
    } else {
        uint8_t replacement[2] = {0xE1u, 0xE2u};
        schway_status status = schway_load_memcard_socket(inst, SCHWAY_CARD_PORT_1,
                                                          SCHWAY_CARD_SOCKET_B,
                                                          replacement, sizeof replacement);
        if (status != SCHWAY_STATUS_ERR_ARGUMENT || spans[1].data == NULL ||
            ((const uint8_t *)spans[1].data)[0] != inputs[1][0]) {
            failures++;
        }
    }
    printf("1..1\n%s 1 - API v9 exposes eight independent card sockets\n",
           failures == 0u ? "ok" : "not ok");
    if (failures != 0u) {
        printf("# actual load/get/generation=%u/8/%u/8/%u/8 spans=%u/8 state-fields=%u/136 phase-paths=%u/8\n",
               loads_ok, gets_ok, generations_ok, unique_spans, state.count, phase_paths);
    }
    return (int)failures;
}

static void check_legacy_input(schway *inst, const schway_current_input *input)
{
    observed_input observed;
    memset(&observed, 0, sizeof observed);
    CHECK_EQ_U64(schway_state_visit(inst, observe_input, &observed), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(observed.saw_buttons, 1u);
    CHECK_EQ_U64(observed.saw_axes, 1u);
    CHECK_EQ_U64(observed.saw_devices, 1u);
    CHECK_EQ_HEX(observed.buttons[0], (uint16_t)~input->buttons[0]);
    CHECK_EQ_HEX(observed.buttons[1], (uint16_t)~input->buttons[1]);
    CHECK(memcmp(observed.axes, input->axes, sizeof input->axes) == 0);
    CHECK_EQ_U64(observed.devices[0], input->device[0]);
    CHECK_EQ_U64(observed.devices[1], input->device[1]);
}

static void expect_rejected_without_replacing(schway *inst,
                                               const schway_current_input *baseline,
                                               const void *invalid,
                                               schway_status expected)
{
    CHECK_EQ_U64(schway_set_input(inst, baseline), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_set_input(inst, invalid), expected);
    CHECK_EQ_U64(schway_run_frame(inst, NULL), SCHWAY_STATUS_OK);
    check_legacy_input(inst, baseline);
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

    check_failures += check_card_socket_api(inst);

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

    check_legacy_input(inst, (const schway_current_input *)(const void *)&input);

    /* Every byte-sized extension boundary is a valid caller-sized prefix.
       malloc gives each iteration exactly the advertised object extent. */
    for (uint32_t size = 28u; size <= (uint32_t)sizeof(schway_current_input); size++) {
        uint8_t *bytes = (uint8_t *)calloc((size_t)size, 1u);
        CHECK(bytes != NULL);
        if (bytes == NULL) {
            schway_destroy(inst);
            CHECK_DONE();
        }
        memcpy(bytes, &size, sizeof size);
        CHECK_EQ_U64(schway_set_input(inst, bytes), SCHWAY_STATUS_OK);
        free(bytes);
    }

    schway_current_input extended;
    memset(&extended, 0, sizeof extended);
    extended.size = (uint32_t)sizeof extended;
    extended.multitap[0] = 1u;
    for (uint32_t port = 0u; port < 2u; port++) {
        for (uint32_t socket = 0u; socket < 4u; socket++) {
            schway_controller_input *controller = &extended.controllers[port][socket];
            controller->device = (port * 4u + socket) % 7u;
            controller->buttons = (uint16_t)(UINT16_C(0x1000) + port * 4u + socket);
            controller->axes[0] = (uint8_t)(10u + port * 4u + socket);
            controller->axes[1] = (uint8_t)(20u + port * 4u + socket);
            controller->axes[2] = (uint8_t)(30u + port * 4u + socket);
            controller->axes[3] = (uint8_t)(40u + port * 4u + socket);
            controller->analog_button = (uint8_t)((port + socket) & 1u);
            controller->mouse_dx = (int16_t)(-4 + (int32_t)(port * 4u + socket));
            controller->mouse_dy = (int16_t)(4 + (int32_t)(port * 4u + socket));
            controller->negcon_twist = (uint8_t)(50u + socket);
            controller->negcon_i = (uint8_t)(60u + socket);
            controller->negcon_ii = (uint8_t)(70u + socket);
            controller->negcon_l = (uint8_t)(80u + socket);
            controller->jogcon_dial = (uint16_t)(1000u + port * 4u + socket);
            controller->jogcon_flags = (uint16_t)((port + socket) & 3u);
        }
    }
    CHECK_EQ_U64(schway_set_input(inst, &extended), SCHWAY_STATUS_OK);

    schway_current_input baseline;
    memset(&baseline, 0, sizeof baseline);
    baseline.size = (uint32_t)sizeof baseline;
    baseline.device[0] = SCHWAY_INPUT_DEVICE_DIGITAL;
    baseline.device[1] = SCHWAY_INPUT_DEVICE_DUALSHOCK;
    baseline.buttons[0] = UINT16_C(0xA55A);
    baseline.buttons[1] = UINT16_C(0xC33C);
    baseline.axes[0][0] = 17u;
    baseline.axes[0][1] = 39u;
    baseline.axes[0][2] = 81u;
    baseline.axes[0][3] = 123u;
    baseline.axes[1][0] = 145u;
    baseline.axes[1][1] = 167u;
    baseline.axes[1][2] = 209u;
    baseline.axes[1][3] = 231u;

    schway_current_input bad = baseline;
    bad.size = 27u;
    expect_rejected_without_replacing(inst, &baseline, &bad, SCHWAY_STATUS_ERR_STRUCT_SIZE);
    bad = baseline;
    bad.device[0] = 7u;
    expect_rejected_without_replacing(inst, &baseline, &bad, SCHWAY_STATUS_ERR_ARGUMENT);
    bad = baseline;
    bad.analog_button[1] = 2u;
    expect_rejected_without_replacing(inst, &baseline, &bad, SCHWAY_STATUS_ERR_ARGUMENT);
    bad = baseline;
    bad.multitap[0] = 2u;
    expect_rejected_without_replacing(inst, &baseline, &bad, SCHWAY_STATUS_ERR_ARGUMENT);
    bad = baseline;
    bad.controllers[1][3].device = 7u;
    expect_rejected_without_replacing(inst, &baseline, &bad, SCHWAY_STATUS_ERR_ARGUMENT);
    bad = baseline;
    bad.controllers[0][2].analog_button = 2u;
    expect_rejected_without_replacing(inst, &baseline, &bad, SCHWAY_STATUS_ERR_ARGUMENT);
    bad = baseline;
    bad.controllers[1][0].reserved0 = 1u;
    expect_rejected_without_replacing(inst, &baseline, &bad, SCHWAY_STATUS_ERR_ARGUMENT);
    bad = baseline;
    bad.controllers[1][2].jogcon_flags = 4u;
    expect_rejected_without_replacing(inst, &baseline, &bad, SCHWAY_STATUS_ERR_ARGUMENT);
    bad = baseline;
    bad.reserved0 = 1u;
    expect_rejected_without_replacing(inst, &baseline, &bad, SCHWAY_STATUS_ERR_ARGUMENT);

    uint8_t oversized[sizeof baseline + 8u];
    memset(oversized, 0, sizeof oversized);
    memcpy(oversized, &baseline, sizeof baseline);
    uint32_t oversized_size = (uint32_t)sizeof oversized;
    memcpy(oversized, &oversized_size, sizeof oversized_size);
    CHECK_EQ_U64(schway_set_input(inst, oversized), SCHWAY_STATUS_OK);
    oversized[sizeof baseline + 7u] = 1u;
    expect_rejected_without_replacing(inst, &baseline, oversized,
                                      SCHWAY_STATUS_ERR_STRUCT_SIZE);
    schway_current_input too_large = baseline;
    too_large.size = 4097u;
    expect_rejected_without_replacing(inst, &baseline, &too_large,
                                      SCHWAY_STATUS_ERR_STRUCT_SIZE);

    schway_destroy(inst);
    CHECK_DONE();
}
