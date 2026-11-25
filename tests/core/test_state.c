/* Save states and the field visitor through the public header:
   schwaystation-test-state <case>. The layout is ARCHITECTURE section 5: a
   header (magic, state format, total size, body CRC-32), then every field
   little-endian in visit order. The test also sees src/, for the header
   size, the field record and the CRC. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "internal.h"
#include "state.h"
#include "../check.h"

#define MAX_FIELDS 1024u
#define H_FORMAT 8u
#define H_TOTAL 12u
#define H_BODY_CRC 16u
#define JOY_DATA 0x1F801040u
#define JOY_MODE 0x1F801048u
#define JOY_CTRL 0x1F80104Au
#define JOY_BAUD 0x1F80104Eu
#define I_STAT 0x1F801070u

/* Five buffers for saved states, sized from the core's own answer, so they
   follow the state as it grows. */
static uint8_t *good;
static uint8_t *bad;
static uint8_t *before;
static uint8_t *after;
static uint8_t *stream; /* the visitor's own serialisation */
static size_t state_cap;
static struct schway__state_field fields[MAX_FIELDS];
static uint32_t offset_of(const schway *inst, const char *name);
static int field_record_matches(const schway *inst, const char *name, uint32_t width,
                                uint32_t count);

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

static schway *make(uint32_t region)
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
    schway_profile p;
    memset(&p, 0, sizeof p);
    p.size = (uint32_t)sizeof p;
    p.region = region;
    CHECK_EQ_U64(schway_set_profile(inst, &p), SCHWAY_STATUS_OK);
    return inst;
}

static schway *make_board(uint32_t region, uint32_t board, uint32_t gpu_v0)
{
    schway *inst = make(region);
    schway_profile p;
    memset(&p, 0, sizeof p);
    p.size = (uint32_t)sizeof p;
    p.region = region;
    p.flags = gpu_v0 != 0u ? SCHWAY_PROFILE_GPU_V0 : 0u;
    p.board = board;
    CHECK_EQ_U64(schway_set_profile(inst, &p), SCHWAY_STATUS_OK);
    return inst;
}

/* Sizes the five buffers from a fresh instance. */
static void bufs_init(void)
{
    schway *probe = make(SCHWAY_REGION_NTSC_U);
    state_cap = schway_state_size(probe);
    schway_destroy(probe);
    CHECK(state_cap > SCHWAY__STATE_HEADER_SIZE);
    good = (uint8_t *)malloc(state_cap);
    bad = (uint8_t *)malloc(state_cap);
    before = (uint8_t *)malloc(state_cap);
    after = (uint8_t *)malloc(state_cap);
    stream = (uint8_t *)malloc(state_cap);
    if (state_cap <= SCHWAY__STATE_HEADER_SIZE || good == NULL || bad == NULL ||
        before == NULL || after == NULL || stream == NULL) {
        exit(1);
    }
}

static void bufs_free(void)
{
    free(good);
    free(bad);
    free(before);
    free(after);
    free(stream);
}

static void run_to(schway *inst, uint64_t frame)
{
    while (inst->frame < frame) {
        CHECK_EQ_U64(schway_run_frame(inst, NULL), SCHWAY_STATUS_OK);
    }
}

static void bus_write(schway *inst, uint32_t addr, uint32_t width, uint32_t value)
{
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, addr, width, value, &cycles), 0u);
}

static uint32_t bus_read(schway *inst, uint32_t addr, uint32_t width)
{
    uint32_t cycles = 0u;
    uint32_t value = 0u;
    CHECK_EQ_U64(schway__bus_read(inst, addr, width, &value, &cycles), 0u);
    return value;
}

static void advance_ticks(schway *inst, uint64_t tick)
{
    inst->ticks = tick;
    schway__sched_sync(inst);
    schway__sched_run(inst);
    schway__spu_catch_up(inst, inst->ticks);
}

static uint16_t spu_attr(const schway *inst)
{
    const uint32_t offset = 0x1F801DAAu - 0x1F801C00u;
    for (uint32_t i = inst->spu.pending_count; i > 0u; i--) {
        if (inst->spu.pending[i - 1u].address == offset) {
            return inst->spu.pending[i - 1u].value;
        }
    }
    return inst->spu.reg[offset >> 1];
}

static size_t save(const schway *inst, uint8_t *buf)
{
    size_t n = schway_state_size(inst);
    CHECK(n > SCHWAY__STATE_HEADER_SIZE && n <= state_cap);
    if (n <= SCHWAY__STATE_HEADER_SIZE || n > state_cap) {
        exit(1);
    }
    CHECK_EQ_U64(schway_save_state(inst, buf, n), SCHWAY_STATUS_OK);
    return n;
}

/* FNV-1a 64 over the display words, low byte first. */
static uint64_t frame_hash(const schway *inst)
{
    schway_video v;
    memset(&v, 0, sizeof v);
    v.size = (uint32_t)sizeof v;
    CHECK_EQ_U64(schway_get_video(inst, &v), SCHWAY_STATUS_OK);
    uint64_t hash = UINT64_C(14695981039346656037);
    for (uint32_t y = 0; y < v.height; y++) {
        const uint8_t *row = (const uint8_t *)v.pixels + (size_t)y * v.pitch_bytes;
        for (uint32_t i = 0; i < 2u * v.width; i++) {
            hash = (hash ^ row[i]) * UINT64_C(1099511628211);
        }
    }
    return hash;
}

static uint64_t state_bytes_hash(const uint8_t *bytes, size_t size)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t i = 0u; i < size; i++) {
        hash = (hash ^ bytes[i]) * UINT64_C(1099511628211);
    }
    return hash;
}

static void put_le(uint8_t *p, uint64_t v, uint32_t width)
{
    for (uint32_t i = 0; i < width; i++) {
        p[i] = (uint8_t)(v >> (8u * i));
    }
}

/* Recomputes the body CRC-32 after an edit, so a load gets past the CRC
   check to the step a case names. */
static void reseal(uint8_t *b, size_t size)
{
    uint32_t crc = schway__crc32_update(0u, b + SCHWAY__STATE_HEADER_SIZE,
                                        size - SCHWAY__STATE_HEADER_SIZE);
    put_le(b + H_BODY_CRC, crc, 4u);
}

static void reseal_q_record(uint8_t q[12])
{
    uint16_t crc = 0u;
    for (uint32_t i = 0u; i < 10u; i++) {
        crc ^= (uint16_t)q[i] << 8;
        for (uint32_t bit = 0u; bit < 8u; bit++) {
            uint32_t shifted = (uint32_t)crc << 1;
            crc = (crc & 0x8000u) != 0u ? (uint16_t)(shifted ^ 0x1021u)
                                        : (uint16_t)shifted;
        }
    }
    crc = (uint16_t)~crc;
    q[10] = (uint8_t)(crc >> 8);
    q[11] = (uint8_t)crc;
}

/* Loads b into inst expecting want, and checks the instance saves the same
   bytes as before the call. */
static void expect_refused(const char *what, schway *inst, const uint8_t *b, size_t size,
                           schway_status want)
{
    uint32_t audio_head = inst->audio_head;
    uint32_t audio_count = inst->audio_count;
    uint32_t audio_dropped = inst->audio_dropped;
    int16_t audio_before[SCHWAY__AUDIO_RING_PAIRS][2];
    memcpy(audio_before, inst->audio, sizeof audio_before);
    size_t n = save(inst, before);
    schway_status got = schway_load_state(inst, b, size);
    if (got != want) {
        fprintf(stderr, "%s: load status %u, want %u\n", what, (unsigned)got, (unsigned)want);
        check_failures++;
    }
    if (save(inst, after) != n || memcmp(before, after, n) != 0) {
        fprintf(stderr, "%s: a refused load changed the instance\n", what);
        check_failures++;
    }
    if (inst->audio_head != audio_head || inst->audio_count != audio_count ||
        inst->audio_dropped != audio_dropped ||
        memcmp(audio_before, inst->audio, sizeof audio_before) != 0) {
        fprintf(stderr, "%s: a refused load changed the host audio ring\n", what);
        check_failures++;
    }
}

static uint8_t state_exchange_byte(schway *inst, uint8_t sent, uint8_t ack)
{
    uint64_t start = inst->ticks;
    bus_write(inst, JOY_DATA, 1u, sent);
    advance_ticks(inst, start + 1088u);
    uint8_t received = (uint8_t)bus_read(inst, JOY_DATA, 1u);
    if (ack != 0u) {
        advance_ticks(inst, start + 1426u);
        bus_write(inst, I_STAT, 4u, 0u);
        bus_write(inst, JOY_CTRL, 2u, 0x1013u);
        advance_ticks(inst, start + 1494u);
        bus_write(inst, JOY_CTRL, 2u, 0x1003u);
    }
    return received;
}

static void test_short_terminal_round_trip(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    inst->pad.multitap[0] = 1u;
    for (uint32_t socket = 0u; socket < 4u; socket++) {
        uint32_t endpoint = socket == 0u ? 0u : 1u + socket;
        schway__pad_set_connected(inst, endpoint, 1u);
    }
    bus_write(inst, JOY_MODE, 2u, 0x000Cu);
    bus_write(inst, JOY_BAUD, 2u, 0x0088u);
    bus_write(inst, JOY_CTRL, 2u, 0x1003u);

    /* A requested Slot A poll arms the long method-1 reply. */
    (void)state_exchange_byte(inst, 0x01u, 1u);
    (void)state_exchange_byte(inst, 0x42u, 1u);
    (void)state_exchange_byte(inst, 0x01u, 1u);
    (void)state_exchange_byte(inst, 0x00u, 1u);
    (void)state_exchange_byte(inst, 0x00u, 0u);
    (void)state_exchange_byte(inst, 0x01u, 1u);
    (void)state_exchange_byte(inst, 0x42u, 1u);
    (void)state_exchange_byte(inst, 0x01u, 1u);
    for (uint32_t i = 0u; i < 32u; i++) {
        (void)state_exchange_byte(inst, 0x00u, i != 31u ? 1u : 0u);
    }
    CHECK_EQ_U64(inst->sio.tap_history_req[0], 1u);
    CHECK_EQ_U64(inst->sio.tap_history_reply[0], 1u);

    /* After the short reply's request byte, the terminal ID byte is next. */
    (void)state_exchange_byte(inst, 0x01u, 1u);
    (void)state_exchange_byte(inst, 0x42u, 1u);
    (void)state_exchange_byte(inst, 0x01u, 1u);
    CHECK_EQ_U64(inst->sio.tap_tx_mode[0], 3u);
    CHECK_EQ_U64(inst->sio.tap_tx_index[0], 3u);
    CHECK_EQ_U64(inst->sio.tap_tx_endpoint[0], 0u);
    CHECK_EQ_U64(inst->sio.tap_slot_mask[0], 1u);
    CHECK_EQ_U64(inst->sio.tap_tx_request[0], 1u);

    size_t n = save(inst, good);
    schway *copy = make(SCHWAY_REGION_NTSC_U);
    CHECK_EQ_U64(schway_load_state(copy, good, n), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(copy->sio.tap_tx_mode[0], 3u);
    CHECK_EQ_U64(copy->sio.tap_tx_index[0], 3u);
    CHECK_EQ_U64(save(copy, after), n);
    CHECK(memcmp(good, after, n) == 0);
    CHECK_EQ_HEX(state_exchange_byte(copy, 0x00u, 0u), 0x41u);
    CHECK_EQ_U64(copy->sio.tap_tx_mode[0], 0u);
    CHECK_EQ_U64(copy->sio.tap_history_reply[0], 2u);
    schway_destroy(copy);
    schway_destroy(inst);
}

static void seed_endpoint_inputs(schway *inst)
{
    schway_input input;
    memset(&input, 0, sizeof input);
    input.size = (uint32_t)sizeof input;
    input.multitap[0] = 1u;
    input.multitap[1] = 1u;
    for (uint32_t port = 0u; port < 2u; port++) {
        for (uint32_t socket = 0u; socket < 4u; socket++) {
            uint32_t endpoint = socket == 0u ? port : 2u + port * 3u + socket - 1u;
            schway_controller_input *controller = &input.controllers[port][socket];
            controller->device = endpoint % 7u;
            controller->buttons = (uint16_t)(0x1020u + endpoint);
            for (uint32_t axis = 0u; axis < 4u; axis++) {
                controller->axes[axis] = (uint8_t)(30u + endpoint * 4u + axis);
            }
            controller->analog_button = (uint8_t)(endpoint & 1u);
            controller->mouse_dx = (int16_t)((int32_t)endpoint - 4);
            controller->mouse_dy = (int16_t)(4 - (int32_t)endpoint);
            controller->negcon_twist = (uint8_t)(50u + endpoint);
            controller->negcon_i = (uint8_t)(60u + endpoint);
            controller->negcon_ii = (uint8_t)(70u + endpoint);
            controller->negcon_l = (uint8_t)(80u + endpoint);
            controller->jogcon_dial = (uint16_t)(1000u + endpoint);
            controller->jogcon_flags = (uint16_t)(endpoint & 3u);
        }
    }
    CHECK_EQ_U64(schway_set_input(inst, &input), SCHWAY_STATUS_OK);
    schway__pad_frame_start(inst);
}

static void check_endpoint_inputs(const schway *inst)
{
    for (uint32_t port = 0u; port < 2u; port++) {
        for (uint32_t socket = 0u; socket < 4u; socket++) {
            uint32_t endpoint = socket == 0u ? port : 2u + port * 3u + socket - 1u;
            uint32_t device = endpoint % 7u;
            CHECK_EQ_U64(inst->pad.device[endpoint], device);
            CHECK_EQ_U64(inst->pad.present[endpoint], device != 0u ? 1u : 0u);
            uint16_t input_buttons = (uint16_t)(0x1020u + endpoint);
            uint16_t expected_buttons = device == SCHWAY_INPUT_DEVICE_MOUSE
                                            ? (uint16_t)(0xF0FFu |
                                                         (0x0C00u &
                                                          (uint16_t)~input_buttons))
                                            : (uint16_t)~input_buttons;
            CHECK_EQ_HEX(inst->pad.buttons[endpoint], expected_buttons);
            for (uint32_t axis = 0u; axis < 4u; axis++) {
                CHECK_EQ_U64(inst->pad.axes[endpoint * 4u + axis],
                             30u + endpoint * 4u + axis);
            }
            CHECK_EQ_U64(inst->pad.analog_button[endpoint], endpoint & 1u);
            CHECK_EQ_U64(inst->pad.mouse_dx[endpoint],
                         device == SCHWAY_INPUT_DEVICE_MOUSE ? 32764u + endpoint : 32768u);
            CHECK_EQ_U64(inst->pad.mouse_dy[endpoint],
                         device == SCHWAY_INPUT_DEVICE_MOUSE ? 32772u - endpoint : 32768u);
            CHECK_EQ_U64(inst->pad.mouse_startup_ack[endpoint],
                         device == SCHWAY_INPUT_DEVICE_MOUSE ? 1u : 0u);
            CHECK_EQ_U64(inst->pad.negcon_twist[endpoint], 50u + endpoint);
            CHECK_EQ_U64(inst->pad.negcon_i[endpoint], 60u + endpoint);
            CHECK_EQ_U64(inst->pad.negcon_ii[endpoint], 70u + endpoint);
            CHECK_EQ_U64(inst->pad.negcon_l[endpoint], 80u + endpoint);
            CHECK_EQ_U64(inst->pad.jogcon_dial[endpoint], 1000u + endpoint);
            CHECK_EQ_U64(inst->pad.jogcon_flags[endpoint], endpoint & 3u);
        }
    }
    CHECK_EQ_U64(inst->pad.multitap[0], 1u);
    CHECK_EQ_U64(inst->pad.multitap[1], 1u);
}

static void seed_serialized_gun(schway *inst)
{
    inst->pad.multitap[0] = 0u;
    inst->pad.device[0] = SCHWAY_INPUT_DEVICE_JUSTIFIER;
    inst->pad.present[0] = 1u;
    inst->pad.gun_input[0].x = 31;
    inst->pad.gun_input[0].y = 45;
    inst->pad.gun_input[0].flags = SCHWAY_GUN_INPUT_VALID | SCHWAY_GUN_INPUT_TRIGGER;
    inst->pad.gun_enabled[0] = 1u;
    inst->pad.gun_result_x[0] = 123u;
    inst->pad.gun_result_y[0] = 45u;
    inst->pad.gun_result_valid[0] = 1u;
    inst->gpu.gun_sensor[0].armed = 1u;
    inst->gpu.gun_sensor[0].aim_x = 31;
    inst->gpu.gun_sensor[0].aim_y = 45;
    inst->gpu.gun_sensor[0].target_line = 34u;
    inst->gpu.gun_sensor[0].target_clock = 56u;
    inst->gpu.gun_sensor[0].deadline = inst->gpu.beam_tick + 100u;
    inst->gpu.gun_sensor[0].phase = 1u;

    inst->pad.device[1] = SCHWAY_INPUT_DEVICE_GUNCON;
    inst->pad.present[1] = 1u;
    inst->pad.gun_input[1].x = -7;
    inst->pad.gun_input[1].y = -9;
    inst->pad.gun_input[1].flags = SCHWAY_GUN_INPUT_OFFSCREEN;
    inst->pad.gun_result_x[1] = 1u;
    inst->pad.gun_result_y[1] = 5u;
    inst->pad.gun_result_valid[1] = 1u;
    inst->gpu.gun_sensor[1].consumed = 1u;
    inst->gpu.gun_sensor[1].aim_x = 4;
    inst->gpu.gun_sensor[1].aim_y = 5;
    inst->gpu.gun_sensor[1].target_line = 22u;
    inst->gpu.gun_sensor[1].target_clock = 88u;
    inst->gpu.gun_sensor[1].phase = 2u;
}

static void check_serialized_gun(const schway *inst)
{
    CHECK_EQ_U64(inst->pad.device[0], SCHWAY_INPUT_DEVICE_JUSTIFIER);
    CHECK_EQ_U64(inst->pad.gun_input[0].x, 31u);
    CHECK_EQ_U64(inst->pad.gun_input[0].y, 45u);
    CHECK_EQ_U64(inst->pad.gun_input[0].flags,
                 SCHWAY_GUN_INPUT_VALID | SCHWAY_GUN_INPUT_TRIGGER);
    CHECK_EQ_U64(inst->pad.gun_enabled[0], 1u);
    CHECK_EQ_U64(inst->pad.gun_result_x[0], 123u);
    CHECK_EQ_U64(inst->pad.gun_result_y[0], 45u);
    CHECK_EQ_U64(inst->pad.gun_result_valid[0], 1u);
    CHECK_EQ_U64(inst->gpu.gun_sensor[0].armed, 1u);
    CHECK_EQ_U64(inst->gpu.gun_sensor[0].consumed, 0u);
    CHECK_EQ_U64(inst->gpu.gun_sensor[0].aim_x, 31u);
    CHECK_EQ_U64(inst->gpu.gun_sensor[0].aim_y, 45u);
    CHECK_EQ_U64(inst->gpu.gun_sensor[0].target_line, 34u);
    CHECK_EQ_U64(inst->gpu.gun_sensor[0].target_clock, 56u);
    CHECK_EQ_U64(inst->gpu.gun_sensor[0].deadline, inst->gpu.beam_tick + 100u);
    CHECK_EQ_U64(inst->gpu.gun_sensor[0].phase, 1u);
    CHECK_EQ_U64(inst->pad.device[1], SCHWAY_INPUT_DEVICE_GUNCON);
    CHECK(inst->pad.gun_input[1].x == -7);
    CHECK(inst->pad.gun_input[1].y == -9);
    CHECK_EQ_U64(inst->pad.gun_input[1].flags, SCHWAY_GUN_INPUT_OFFSCREEN);
    CHECK_EQ_U64(inst->pad.gun_result_x[1], 1u);
    CHECK_EQ_U64(inst->pad.gun_result_y[1], 5u);
    CHECK_EQ_U64(inst->pad.gun_result_valid[1], 1u);
    CHECK_EQ_U64(inst->gpu.gun_sensor[1].armed, 0u);
    CHECK_EQ_U64(inst->gpu.gun_sensor[1].consumed, 1u);
    CHECK_EQ_U64(inst->gpu.gun_sensor[1].aim_x, 4u);
    CHECK_EQ_U64(inst->gpu.gun_sensor[1].aim_y, 5u);
    CHECK_EQ_U64(inst->gpu.gun_sensor[1].target_line, 22u);
    CHECK_EQ_U64(inst->gpu.gun_sensor[1].target_clock, 88u);
    CHECK_EQ_U64(inst->gpu.gun_sensor[1].deadline, UINT64_MAX);
    CHECK_EQ_U64(inst->gpu.gun_sensor[1].phase, 2u);
}

/* Run to 30 and save endpoint input, run to 60; load the save and run to 60
   again: the same frame, endpoint state and serialized bytes. */
static void test_round_trip(void)
{
    static const uint32_t regions[] = {SCHWAY_REGION_NTSC_U, SCHWAY_REGION_PAL};
    for (size_t k = 0; k < 2u; k++) {
        schway *inst = make(regions[k]);
        run_to(inst, 30);
        seed_endpoint_inputs(inst);
        size_t n = save(inst, good);
        run_to(inst, 60);
        uint64_t h1 = frame_hash(inst);
        size_t m = save(inst, before);
        CHECK_EQ_U64(m, n);
        CHECK_EQ_U64(schway_load_state(inst, good, n), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(inst->frame, 30);
        check_endpoint_inputs(inst);
        run_to(inst, 60);
        uint64_t h2 = frame_hash(inst);
        CHECK_EQ_HEX(h1, h2);
        save(inst, after);
        CHECK(memcmp(before, after, n) == 0);
        /* And into a second instance of the same region. */
        schway *other = make(regions[k]);
        CHECK_EQ_U64(schway_load_state(other, good, n), SCHWAY_STATUS_OK);
        check_endpoint_inputs(other);
        run_to(other, 60);
        CHECK_EQ_HEX(frame_hash(other), h1);
        schway_destroy(other);
        schway_destroy(inst);
    }

    schway *gun = make(SCHWAY_REGION_NTSC_U);
    schway *gun_restore = make(SCHWAY_REGION_NTSC_U);
    seed_serialized_gun(gun);
    size_t gun_size = save(gun, good);
    schway_input pending_gun;
    memset(&pending_gun, 0, sizeof pending_gun);
    pending_gun.size = (uint32_t)sizeof pending_gun;
    pending_gun.device[0] = SCHWAY_INPUT_DEVICE_JUSTIFIER;
    pending_gun.guns[0].x = 201;
    pending_gun.guns[0].y = 77;
    pending_gun.guns[0].flags = SCHWAY_GUN_INPUT_VALID;
    CHECK_EQ_U64(schway_set_input(gun, &pending_gun), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(save(gun, before), gun_size);
    CHECK(memcmp(good, before, gun_size) == 0);
    CHECK_EQ_U64(schway_load_state(gun_restore, good, gun_size), SCHWAY_STATUS_OK);
    check_serialized_gun(gun_restore);
    CHECK_EQ_U64(save(gun_restore, after), gun_size);
    CHECK(memcmp(good, after, gun_size) == 0);
    schway_destroy(gun_restore);
    schway_destroy(gun);

    schway *reset = make(SCHWAY_REGION_NTSC_U);
    for (uint32_t endpoint = 0u; endpoint < SCHWAY__PAD_ENDPOINT_COUNT; endpoint++) {
        uint8_t received = 0u;
        reset->pad.present[endpoint] = 1u;
        reset->pad.device[endpoint] = SCHWAY_INPUT_DEVICE_DIGITAL;
        CHECK_EQ_U64(schway__pad_byte(reset, endpoint, 0x01u, reset->ticks, &received), 1u);
        CHECK_EQ_U64(schway__pad_byte(reset, endpoint, 0x42u, reset->ticks, &received), 1u);
        CHECK_EQ_U64(schway__pad_byte(reset, endpoint, 0u, reset->ticks, &received), 1u);
        CHECK_EQ_U64(schway__pad_byte(reset, endpoint, 0u, reset->ticks, &received), 1u);
        CHECK_EQ_U64(reset->pad.phase[endpoint], 3u);
    }
    CHECK_EQ_U64(schway_reset(reset, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    for (uint32_t endpoint = 0u; endpoint < SCHWAY__PAD_ENDPOINT_COUNT; endpoint++) {
        CHECK_EQ_U64(reset->pad.phase[endpoint], 0u);
        CHECK_EQ_U64(reset->pad.command[endpoint], 0u);
        CHECK_EQ_U64(reset->pad.data_index[endpoint], 0u);
    }
    schway_destroy(reset);
}

/* Save, load, save: the same bytes. */
static void test_stable_bytes(void)
{
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    run_to(inst, 45);
    size_t n = save(inst, good);
    CHECK_EQ_U64(schway_load_state(inst, good, n), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(save(inst, after), n);
    CHECK(memcmp(good, after, n) == 0);
    printf("state_bytes=%zu\n", n);
    schway_destroy(inst);

    schway *early = make_board(SCHWAY_REGION_NTSC_U, SCHWAY_BOARD_PU7, 1u);
    run_to(early, 45u);
    CHECK_EQ_U64(save(early, before), n);
    uint64_t early_hash = state_bytes_hash(before, n);

    schway *repeat = make_board(SCHWAY_REGION_NTSC_U, SCHWAY_BOARD_PU7, 1u);
    run_to(repeat, 45u);
    CHECK_EQ_U64(save(repeat, bad), n);
    CHECK_EQ_HEX(state_bytes_hash(bad, n), early_hash);
    CHECK(state_bytes_hash(good, n) != early_hash);
    schway_destroy(repeat);
    schway_destroy(early);
}

/* A cross-board load restores fixed board identity but keeps serialized
   mutable RAM_SIZE instead of resetting it to the board's boot default. */
static void test_profile(void)
{
    schway *src = make_board(SCHWAY_REGION_NTSC_U, SCHWAY_BOARD_PU7, 1u);
    bus_write(src, 0x1F801060u, 4u, 0x1234u);
    run_to(src, 4u);
    size_t n = save(src, good);

    schway *dst = make_board(SCHWAY_REGION_NTSC_U, SCHWAY_BOARD_PU18, 0u);
    bus_write(dst, 0x1F801060u, 4u, 0x5678u);
    CHECK_EQ_U64(dst->board, SCHWAY_BOARD_PU18);
    CHECK_EQ_U64(schway_load_state(dst, good, n), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(dst->board, SCHWAY_BOARD_PU7);
    CHECK_EQ_U64(dst->profile_flags, SCHWAY_PROFILE_GPU_V0);
    CHECK_EQ_U64(dst->spu_revision, 2922u);
    CHECK_EQ_U64(dst->cd_firmware, 0u);
    CHECK_EQ_U64(dst->ram_ctrl_boot, 0x0B80u);
    CHECK_EQ_U64(dst->has_parallel_port, 1u);
    CHECK_EQ_U64(dst->has_serial_port, 0u);
    CHECK_EQ_U64(dst->bus.ram_size, 0x1234u);
    CHECK_EQ_U64(save(dst, after), n);
    CHECK(memcmp(good, after, n) == 0);
    schway_destroy(dst);
    schway_destroy(src);

    /* Old-sized callers may select GPU v0 while inheriting the PU-18 board.
       That documented compatibility state must survive serialization. */
    schway *legacy = make(SCHWAY_REGION_NTSC_U);
    schway_profile old;
    memset(&old, 0, sizeof old);
    old.size = (uint32_t)offsetof(schway_profile, board);
    old.region = SCHWAY_REGION_NTSC_U;
    old.flags = SCHWAY_PROFILE_GPU_V0;
    old.board = SCHWAY_BOARD_PM41_2; /* outside the caller's declared size */
    CHECK_EQ_U64(schway_set_profile(legacy, &old), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(legacy->board, SCHWAY_BOARD_PU18);
    CHECK_EQ_U64(legacy->profile_flags, SCHWAY_PROFILE_GPU_V0);
    n = save(legacy, good);

    schway *legacy_target = make(SCHWAY_REGION_NTSC_U);
    CHECK_EQ_U64(schway_load_state(legacy_target, good, n), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(legacy_target->board, SCHWAY_BOARD_PU18);
    CHECK_EQ_U64(legacy_target->profile_flags, SCHWAY_PROFILE_GPU_V0);
    CHECK_EQ_U64(save(legacy_target, after), n);
    CHECK(memcmp(good, after, n) == 0);
    schway_destroy(legacy_target);
    schway_destroy(legacy);
}

/* The opt-in boot bit is part of the existing serialized profile flags, and
   unknown bits remain an atomic state rejection. */
static void test_fast_boot_profile(void)
{
    schway *src = make(SCHWAY_REGION_NTSC_U);
    schway_profile profile;
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = SCHWAY_REGION_NTSC_U;
    profile.flags = SCHWAY_PROFILE_FAST_BOOT;
    CHECK_EQ_U64(schway_set_profile(src, &profile), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(src->profile_flags, SCHWAY_PROFILE_FAST_BOOT);
    size_t n = save(src, good);

    schway *dst = make(SCHWAY_REGION_NTSC_U);
    CHECK_EQ_U64(schway_load_state(dst, good, n), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(dst->profile_flags, SCHWAY_PROFILE_FAST_BOOT);
    CHECK_EQ_U64(save(dst, after), n);
    CHECK(memcmp(good, after, n) == 0);

    size_t nf = schway__state_record(src, fields, MAX_FIELDS);
    CHECK(nf <= MAX_FIELDS);
    uint32_t profile_flags_at = 0u;
    for (size_t i = 0u; i < nf && i < MAX_FIELDS; i++) {
        if (strcmp(fields[i].name, "instance.profile_flags") == 0) {
            profile_flags_at = fields[i].offset;
        }
    }
    CHECK(profile_flags_at >= SCHWAY__STATE_HEADER_SIZE);
    if (profile_flags_at >= SCHWAY__STATE_HEADER_SIZE) {
        memcpy(bad, good, n);
        put_le(bad + profile_flags_at, SCHWAY_PROFILE_FAST_BOOT | 0x20u, 4u);
        reseal(bad, n);
        expect_refused("unknown fast-boot profile flag", dst, bad, n,
                       SCHWAY_STATUS_ERR_STATE_FIELD);
    }

    schway_destroy(dst);
    schway_destroy(src);
}

/* Each corruption is refused with its own status before any byte applies. */
static void test_rejects(void)
{
    schway *src = make(SCHWAY_REGION_NTSC_U);
    run_to(src, 60);
    size_t n = save(src, good);
    schway *target = make(SCHWAY_REGION_NTSC_U);
    run_to(target, 5);

    memcpy(bad, good, n);
    bad[3] ^= 0x01u;
    expect_refused("magic", target, bad, n, SCHWAY_STATUS_ERR_STATE_MAGIC);
    expect_refused("short magic", target, good, 4u, SCHWAY_STATUS_ERR_STATE_MAGIC);

    memcpy(bad, good, n);
    bad[H_FORMAT] ^= 0x02u;
    expect_refused("format", target, bad, n, SCHWAY_STATUS_ERR_STATE_VERSION);

    memcpy(bad, good, n);
    put_le(bad + H_FORMAT, 19u, 4u);
    expect_refused("previous format", target, bad, n, SCHWAY_STATUS_ERR_STATE_VERSION);

    memcpy(bad, good, n);
    put_le(bad + H_FORMAT, 17u, 4u);
    expect_refused("format 17", target, bad, n, SCHWAY_STATUS_ERR_STATE_VERSION);

    memcpy(bad, good, n);
    put_le(bad + H_FORMAT, 5u, 4u);
    expect_refused("released revision-5 state", target, bad, n,
                   SCHWAY_STATUS_ERR_STATE_VERSION);

    memcpy(bad, good, n);
    bad[H_TOTAL] ^= 0x01u;
    expect_refused("size word", target, bad, n, SCHWAY_STATUS_ERR_STATE_FIELD);
    expect_refused("one byte short", target, good, n - 1u, SCHWAY_STATUS_ERR_STATE_FIELD);

    memcpy(bad, good, n);
    bad[n - 1u] ^= 0x10u;
    expect_refused("body byte", target, bad, n, SCHWAY_STATUS_ERR_STATE_CRC);
    memcpy(bad, good, n);
    bad[H_BODY_CRC] ^= 0x01u;
    expect_refused("crc word", target, bad, n, SCHWAY_STATUS_ERR_STATE_CRC);

    /* Another valid region, resealed: the CRC passes and the field check
       refuses it, since a state loads only into its own region. */
    size_t nf = schway__state_record(src, fields, MAX_FIELDS);
    CHECK(nf <= MAX_FIELDS);
    uint32_t region_at = 0;
    uint32_t board_at = 0;
    uint32_t profile_flags_at = 0;
    uint32_t pad_phase_at = 0u;
    uint32_t pad_device_at = 0u;
    uint32_t pad_present_at = 0u;
    uint32_t pad_multitap_at = 0u;
    uint32_t pad_mouse_dx_at = 0u;
    uint32_t sio_tap_mode_at = 0u;
    uint32_t sio_tap_index_at = 0u;
    uint32_t sio_tap_endpoint_at = 0u;
    uint32_t sio_tap_slot_mask_at = 0u;
    uint32_t card_tx_active_at = 0u;
    uint32_t card_tx_endpoint_at = 0u;
    uint32_t card_socket_b_phase_at = 0u;
    for (size_t i = 0; i < nf && i < MAX_FIELDS; i++) {
        if (strcmp(fields[i].name, "instance.region") == 0) {
            region_at = fields[i].offset;
        } else if (strcmp(fields[i].name, "instance.board") == 0) {
            board_at = fields[i].offset;
        } else if (strcmp(fields[i].name, "instance.profile_flags") == 0) {
            profile_flags_at = fields[i].offset;
        } else if (strcmp(fields[i].name, "pad.phase") == 0) {
            pad_phase_at = fields[i].offset;
        } else if (strcmp(fields[i].name, "pad.device") == 0) {
            pad_device_at = fields[i].offset;
        } else if (strcmp(fields[i].name, "pad.present") == 0) {
            pad_present_at = fields[i].offset;
        } else if (strcmp(fields[i].name, "pad.multitap") == 0) {
            pad_multitap_at = fields[i].offset;
        } else if (strcmp(fields[i].name, "pad.mouse_dx") == 0) {
            pad_mouse_dx_at = fields[i].offset;
        } else if (strcmp(fields[i].name, "sio.tap_tx_mode") == 0) {
            sio_tap_mode_at = fields[i].offset;
        } else if (strcmp(fields[i].name, "sio.tap_tx_index") == 0) {
            sio_tap_index_at = fields[i].offset;
        } else if (strcmp(fields[i].name, "sio.tap_tx_endpoint") == 0) {
            sio_tap_endpoint_at = fields[i].offset;
        } else if (strcmp(fields[i].name, "sio.tap_slot_mask") == 0) {
            sio_tap_slot_mask_at = fields[i].offset;
        } else if (strcmp(fields[i].name, "sio.card_tx_active") == 0) {
            card_tx_active_at = fields[i].offset;
        } else if (strcmp(fields[i].name, "sio.card_tx_endpoint") == 0) {
            card_tx_endpoint_at = fields[i].offset;
        } else if (strcmp(fields[i].name, "card.port1.socketB.phase") == 0) {
            card_socket_b_phase_at = fields[i].offset;
        }
    }

    CHECK(pad_phase_at >= SCHWAY__STATE_HEADER_SIZE &&
          sio_tap_mode_at >= SCHWAY__STATE_HEADER_SIZE &&
          sio_tap_index_at >= SCHWAY__STATE_HEADER_SIZE &&
          sio_tap_slot_mask_at >= SCHWAY__STATE_HEADER_SIZE &&
          pad_device_at >= SCHWAY__STATE_HEADER_SIZE &&
          pad_present_at >= SCHWAY__STATE_HEADER_SIZE &&
          pad_multitap_at >= SCHWAY__STATE_HEADER_SIZE &&
          pad_mouse_dx_at >= SCHWAY__STATE_HEADER_SIZE);
    if (pad_phase_at >= SCHWAY__STATE_HEADER_SIZE) {
        memcpy(bad, good, n);
        put_le(bad + pad_phase_at + 7u, 3u, 1u);
        reseal(bad, n);
        expect_refused("payload phase on disconnected endpoint", target, bad, n,
                       SCHWAY_STATUS_ERR_STATE_FIELD);
    }
    if (pad_device_at >= SCHWAY__STATE_HEADER_SIZE &&
        pad_present_at >= SCHWAY__STATE_HEADER_SIZE) {
        memcpy(bad, good, n);
        put_le(bad + pad_device_at + 2u, SCHWAY_INPUT_DEVICE_DIGITAL, 1u);
        put_le(bad + pad_present_at + 2u, 1u, 1u);
        reseal(bad, n);
        expect_refused("tap socket connected with tap disabled", target, bad, n,
                       SCHWAY_STATUS_ERR_STATE_FIELD);
    }
    if (sio_tap_mode_at >= SCHWAY__STATE_HEADER_SIZE &&
        sio_tap_index_at >= SCHWAY__STATE_HEADER_SIZE &&
        sio_tap_slot_mask_at >= SCHWAY__STATE_HEADER_SIZE) {
        memcpy(bad, good, n);
        put_le(bad + sio_tap_mode_at, 2u, 1u);
        put_le(bad + sio_tap_index_at, 1u, 1u);
        put_le(bad + sio_tap_slot_mask_at, 1u, 1u);
        reseal(bad, n);
        expect_refused("tap transaction on disabled port", target, bad, n,
                       SCHWAY_STATUS_ERR_STATE_FIELD);
    }
    if (sio_tap_mode_at >= SCHWAY__STATE_HEADER_SIZE &&
        sio_tap_index_at >= SCHWAY__STATE_HEADER_SIZE &&
        sio_tap_endpoint_at >= SCHWAY__STATE_HEADER_SIZE &&
        sio_tap_slot_mask_at >= SCHWAY__STATE_HEADER_SIZE &&
        pad_multitap_at >= SCHWAY__STATE_HEADER_SIZE) {
        static const struct {
            const char *name;
            uint8_t mode;
            uint8_t index;
            uint8_t endpoint;
            uint8_t slot_mask;
        } invalid_tap_indices[] = {
            {"long reply past terminal", 2u, 35u, 0u, 1u},
            {"slot A reply past terminal", 1u, 9u, 0u, 1u},
            {"short reply past terminal", 3u, 4u, 0u, 1u},
            {"method 2 reply past terminal", 4u, 9u, 2u, 0u},
        };
        for (size_t i = 0u; i < sizeof invalid_tap_indices / sizeof invalid_tap_indices[0];
             i++) {
            memcpy(bad, good, n);
            put_le(bad + sio_tap_mode_at, invalid_tap_indices[i].mode, 1u);
            put_le(bad + sio_tap_index_at, invalid_tap_indices[i].index, 1u);
            put_le(bad + sio_tap_endpoint_at, invalid_tap_indices[i].endpoint, 1u);
            put_le(bad + sio_tap_slot_mask_at, invalid_tap_indices[i].slot_mask, 1u);
            put_le(bad + pad_multitap_at, 1u, 1u);
            reseal(bad, n);
            expect_refused(invalid_tap_indices[i].name, target, bad, n,
                           SCHWAY_STATUS_ERR_STATE_FIELD);
        }
    }
    if (sio_tap_mode_at >= SCHWAY__STATE_HEADER_SIZE &&
        sio_tap_index_at >= SCHWAY__STATE_HEADER_SIZE &&
        sio_tap_endpoint_at >= SCHWAY__STATE_HEADER_SIZE &&
        sio_tap_slot_mask_at >= SCHWAY__STATE_HEADER_SIZE) {
        memcpy(bad, good, n);
        put_le(bad + sio_tap_mode_at, 2u, 1u);
        put_le(bad + sio_tap_index_at, 1u, 1u);
        put_le(bad + sio_tap_endpoint_at, 5u, 1u);
        put_le(bad + sio_tap_slot_mask_at, 1u, 1u);
        put_le(bad + pad_multitap_at, 1u, 1u);
        reseal(bad, n);
        expect_refused("tap transaction selects other physical port", target, bad, n,
                       SCHWAY_STATUS_ERR_STATE_FIELD);
    }
    if (pad_mouse_dx_at >= SCHWAY__STATE_HEADER_SIZE) {
        memcpy(bad, good, n);
        put_le(bad + pad_mouse_dx_at, 0u, 2u);
        reseal(bad, n);
        expect_refused("mouse movement outside latched range", target, bad, n,
                       SCHWAY_STATUS_ERR_STATE_FIELD);
    }
    if (card_tx_active_at >= SCHWAY__STATE_HEADER_SIZE &&
        card_tx_endpoint_at >= SCHWAY__STATE_HEADER_SIZE &&
        card_socket_b_phase_at >= SCHWAY__STATE_HEADER_SIZE) {
        memcpy(bad, good, n);
        put_le(bad + card_tx_active_at, 1u, 1u);
        put_le(bad + card_tx_endpoint_at, 1u, 1u);
        put_le(bad + card_socket_b_phase_at, 1u, 1u);
        reseal(bad, n);
        expect_refused("card transaction on disabled tap socket", target, bad, n,
                       SCHWAY_STATUS_ERR_STATE_FIELD);
    }
    const struct {
        const char *what;
        const char *field;
        uint64_t value;
        uint32_t width;
    } invalid_gun[] = {
        {"gun device input flags", "pad.gun0.flags", 0x10u, 4u},
        {"gun device input reserved field", "pad.gun0.reserved", 1u, 4u},
        {"negative scheduled aim", "gpu.gun0.aim_x", UINT32_MAX, 4u},
        {"inactive gun deadline", "gpu.gun0.deadline", 1u, 8u},
        {"gun sensor phase", "gpu.gun0.phase", 3u, 4u},
    };
    for (size_t i = 0u; i < sizeof invalid_gun / sizeof invalid_gun[0]; i++) {
        memcpy(bad, good, n);
        put_le(bad + offset_of(src, invalid_gun[i].field), invalid_gun[i].value,
               invalid_gun[i].width);
        reseal(bad, n);
        expect_refused(invalid_gun[i].what, target, bad, n,
                       SCHWAY_STATUS_ERR_STATE_FIELD);
    }
    CHECK(region_at >= SCHWAY__STATE_HEADER_SIZE);
    if (region_at >= SCHWAY__STATE_HEADER_SIZE) {
        static const uint32_t others[] = {SCHWAY_REGION_NTSC_J, SCHWAY_REGION_PAL};
        for (size_t k = 0; k < 2u; k++) {
            memcpy(bad, good, n);
            put_le(bad + region_at, others[k], 4u);
            reseal(bad, n);
            expect_refused("region", target, bad, n, SCHWAY_STATUS_ERR_STATE_FIELD);
        }
    }

    CHECK(board_at >= SCHWAY__STATE_HEADER_SIZE);
    CHECK(profile_flags_at >= SCHWAY__STATE_HEADER_SIZE);
    if (board_at >= SCHWAY__STATE_HEADER_SIZE &&
        profile_flags_at >= SCHWAY__STATE_HEADER_SIZE) {
        memcpy(bad, good, n);
        put_le(bad + board_at, 0u, 4u);
        reseal(bad, n);
        expect_refused("board zero", target, bad, n, SCHWAY_STATUS_ERR_STATE_FIELD);

        memcpy(bad, good, n);
        put_le(bad + board_at, SCHWAY_BOARD_PM41_2 + 1u, 4u);
        reseal(bad, n);
        expect_refused("board above maximum", target, bad, n,
                       SCHWAY_STATUS_ERR_STATE_FIELD);

    }

    /* The architectural zero register and COP0's stored/derived bits cannot
       be chosen by a state file, even when its checksum is valid. */
    uint32_t cpu_r_at = 0u;
    uint32_t cop0_at = 0u;
    for (size_t i = 0u; i < nf && i < MAX_FIELDS; i++) {
        if (strcmp(fields[i].name, "cpu.r") == 0) {
            cpu_r_at = fields[i].offset;
        } else if (strcmp(fields[i].name, "cop0.r") == 0) {
            cop0_at = fields[i].offset;
        }
    }
    CHECK(cpu_r_at >= SCHWAY__STATE_HEADER_SIZE);
    CHECK(cop0_at >= SCHWAY__STATE_HEADER_SIZE);
    if (cpu_r_at >= SCHWAY__STATE_HEADER_SIZE && cop0_at >= SCHWAY__STATE_HEADER_SIZE) {
        memcpy(bad, good, n);
        put_le(bad + cpu_r_at, 1u, 4u);
        reseal(bad, n);
        expect_refused("cpu.r0", target, bad, n, SCHWAY_STATUS_ERR_STATE_FIELD);

        memcpy(bad, good, n);
        put_le(bad + cop0_at + 12u * 4u, 0x00400040u, 4u);
        reseal(bad, n);
        expect_refused("cop0.SR reserved bit", target, bad, n, SCHWAY_STATUS_ERR_STATE_FIELD);

        memcpy(bad, good, n);
        put_le(bad + cop0_at + 13u * 4u, 0x00000400u, 4u);
        reseal(bad, n);
        expect_refused("cop0.CAUSE interrupt mismatch", target, bad, n,
                       SCHWAY_STATUS_ERR_STATE_FIELD);
    }

    size_t m = schway_state_size(target);
    CHECK_EQ_U64(schway_save_state(target, after, m - 1u), SCHWAY_STATUS_ERR_BUFFER_TOO_SMALL);
    CHECK_EQ_U64(schway_save_state(target, after, 0u), SCHWAY_STATUS_ERR_BUFFER_TOO_SMALL);

    /* The control: the untouched state, resealed, loads. */
    memcpy(bad, good, n);
    reseal(bad, n);
    CHECK(memcmp(bad, good, n) == 0);
    CHECK_EQ_U64(schway_load_state(target, bad, n), SCHWAY_STATUS_OK);
    test_short_terminal_round_trip();
    schway_destroy(target);
    schway_destroy(src);
}

/* The visitor's own little-endian serialisation of what it is handed. */
struct visit_ctx {
    size_t n;            /* bytes written to stream */
    size_t calls;
    int bad_width, bad_name, overflow;
    int saw_frame, saw_vram;
    const char *names[MAX_FIELDS];
};

static void on_field(void *user, const char *name, const void *data, uint32_t count,
                     uint32_t width)
{
    struct visit_ctx *c = (struct visit_ctx *)user;
    if (c->calls < MAX_FIELDS) {
        c->names[c->calls] = name;
    }
    c->calls++;
    if (name == NULL || name[0] == '\0') {
        c->bad_name = 1;
        return;
    }
    c->saw_frame |= strcmp(name, "instance.frame") == 0;
    c->saw_vram |= strcmp(name, "gpu.vram") == 0;
    if (width != 1u && width != 2u && width != 4u && width != 8u) {
        c->bad_width = 1;
        return;
    }
    if ((size_t)count * width > state_cap - c->n) {
        c->overflow = 1;
        return;
    }
    const uint8_t *p = (const uint8_t *)data;
    for (uint32_t i = 0; i < count; i++) {
        uint64_t v = 0;
        if (width == 1u) {
            v = p[i];
        } else if (width == 2u) {
            uint16_t x;
            memcpy(&x, p + 2u * (size_t)i, 2u);
            v = x;
        } else if (width == 4u) {
            uint32_t x;
            memcpy(&x, p + 4u * (size_t)i, 4u);
            v = x;
        } else {
            memcpy(&v, p + 8u * (size_t)i, 8u);
        }
        put_le(stream + c->n, v, width);
        c->n += width;
    }
}

/* The visitor hands over every field of the body: written little-endian in
   visit order, its fields are exactly the bytes a save writes after the
   header, with the card drawn at frame 60. */
static void test_visit(void)
{
    static const uint32_t regions[] = {SCHWAY_REGION_NTSC_U, SCHWAY_REGION_PAL};
    for (size_t k = 0; k < 2u; k++) {
        schway *inst = make(regions[k]);
        run_to(inst, 60);
        size_t n = save(inst, good);
        static struct visit_ctx c, again;
        memset(&c, 0, sizeof c);
        CHECK_EQ_U64(schway_state_visit(inst, on_field, &c), SCHWAY_STATUS_OK);
        CHECK(!c.bad_name && !c.bad_width && !c.overflow);
        CHECK(c.saw_frame && c.saw_vram);
        CHECK(c.calls >= 2u && c.calls <= MAX_FIELDS);
        static const struct {
            const char *name;
            uint32_t width, count;
        } gun_fields[] = {
            {"pad.gun0.x", 4u, 1u},
            {"pad.gun0.y", 4u, 1u},
            {"pad.gun0.flags", 4u, 1u},
            {"pad.gun0.reserved", 4u, 1u},
            {"pad.gun1.x", 4u, 1u},
            {"pad.gun1.y", 4u, 1u},
            {"pad.gun1.flags", 4u, 1u},
            {"pad.gun1.reserved", 4u, 1u},
            {"pad.gun_result_x", 2u, 2u},
            {"pad.gun_result_y", 2u, 2u},
            {"pad.gun_valid", 1u, 2u},
            {"pad.gun_enabled", 1u, 2u},
            {"gpu.gun0.armed", 4u, 1u},
            {"gpu.gun0.consumed", 4u, 1u},
            {"gpu.gun0.aim_x", 4u, 1u},
            {"gpu.gun0.aim_y", 4u, 1u},
            {"gpu.gun0.target_line", 4u, 1u},
            {"gpu.gun0.target_clock", 4u, 1u},
            {"gpu.gun0.deadline", 8u, 1u},
            {"gpu.gun0.phase", 4u, 1u},
            {"gpu.gun1.armed", 4u, 1u},
            {"gpu.gun1.consumed", 4u, 1u},
            {"gpu.gun1.aim_x", 4u, 1u},
            {"gpu.gun1.aim_y", 4u, 1u},
            {"gpu.gun1.target_line", 4u, 1u},
            {"gpu.gun1.target_clock", 4u, 1u},
            {"gpu.gun1.deadline", 8u, 1u},
            {"gpu.gun1.phase", 4u, 1u},
        };
        for (size_t i = 0u; i < sizeof gun_fields / sizeof gun_fields[0]; i++) {
            CHECK(field_record_matches(inst, gun_fields[i].name, gun_fields[i].width,
                                       gun_fields[i].count));
        }
        CHECK_EQ_U64(c.n, n - SCHWAY__STATE_HEADER_SIZE);
        CHECK(c.n == n - SCHWAY__STATE_HEADER_SIZE &&
              memcmp(stream, good + SCHWAY__STATE_HEADER_SIZE, c.n) == 0);
        /* The names are the same strings on every call. */
        memset(&again, 0, sizeof again);
        CHECK_EQ_U64(schway_state_visit(inst, on_field, &again), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(again.calls, c.calls);
        for (size_t i = 0; i < c.calls && i < MAX_FIELDS; i++) {
            CHECK(again.names[i] == c.names[i]);
        }
        schway_destroy(inst);
    }
    CHECK_EQ_U64(schway_state_visit(NULL, on_field, NULL), SCHWAY_STATUS_ERR_ARGUMENT);
}

/* Every bounded body field written just outside its range, resealed, is
   refused by the field check, not the CRC, and changes nothing; a header
   field outside its value is refused before the body is read. */
static void test_record_sweep(void)
{
    schway *src = make(SCHWAY_REGION_NTSC_U);
    run_to(src, 60);
    size_t n = save(src, good);
    CHECK(n <= 4u * 1024u * 1024u);
    schway *target = make(SCHWAY_REGION_NTSC_U);
    run_to(target, 5);
    size_t nf = schway__state_record(src, fields, MAX_FIELDS);
    CHECK(nf >= 5u && nf <= MAX_FIELDS);
    size_t swept_body = 0, swept_header = 0;
    for (size_t i = 0; i < nf && i < MAX_FIELDS; i++) {
        const struct schway__state_field *fd = &fields[i];
        CHECK(fd->name != NULL);
        CHECK((size_t)fd->offset + (size_t)fd->width * fd->count <= n);
        uint64_t type_max =
            fd->width == 8u ? UINT64_MAX : (UINT64_C(1) << (8u * fd->width)) - 1u;
        const uint64_t outside[2] = {fd->max + 1u, fd->min - 1u};
        const int sweep[2] = {fd->max < type_max, fd->min > 0u};
        int header = fd->offset < SCHWAY__STATE_HEADER_SIZE;
        for (size_t j = 0; j < 2u; j++) {
            if (!sweep[j]) {
                continue;
            }
            memcpy(bad, good, n);
            put_le(bad + fd->offset, outside[j], fd->width);
            reseal(bad, n);
            if (header) {
                /* Magic, format and size each have their own status; the
                   CRC word itself takes any value. */
                size_t n0 = save(target, before);
                schway_status st = schway_load_state(target, bad, n);
                CHECK(st != SCHWAY_STATUS_OK && st != SCHWAY_STATUS_ERR_STATE_CRC);
                CHECK(save(target, after) == n0 && memcmp(before, after, n0) == 0);
                swept_header++;
            } else {
                expect_refused(fd->name, target, bad, n, SCHWAY_STATUS_ERR_STATE_FIELD);
                swept_body++;
            }
        }
    }
    printf("state fields: %lu recorded, %lu body and %lu header values refused\n",
           (unsigned long)nf, (unsigned long)swept_body, (unsigned long)swept_header);
    /* instance.frame, instance.region (both sides), instance.ticks and
       instance.tick_acc; gpu.vram takes every halfword. */
    CHECK(swept_body >= 5u);
    CHECK(swept_header >= 8u);
    /* A stamp ahead of a clock is refused at its recorded maximum, so each
       such field must record one below the type's: the probe above then
       reached the bound and refused the stamp one past it. */
    static const char *const stamps[] = {"gte.ready", "dma.busy_until", "dma.stall_until"};
    for (size_t k = 0; k < sizeof stamps / sizeof stamps[0]; k++) {
        int found = 0;
        for (size_t i = 0; i < nf && i < MAX_FIELDS; i++) {
            if (strcmp(fields[i].name, stamps[k]) == 0) {
                found = 1;
                CHECK(fields[i].max < UINT64_MAX);
            }
        }
        CHECK(found);
    }
    /* The accumulator's remainder is below the frame denominator, 715,909
       cycles for NTSC (ARCHITECTURE section 1), so a remainder equal to it
       is refused whatever bound the record reports. */
    for (size_t i = 0; i < nf && i < MAX_FIELDS; i++) {
        if (strcmp(fields[i].name, "instance.tick_acc") == 0) {
            memcpy(bad, good, n);
            put_le(bad + fields[i].offset, 715909u, 8u);
            reseal(bad, n);
            expect_refused("tick_acc at the denominator", target, bad, n,
                           SCHWAY_STATUS_ERR_STATE_FIELD);
            memcpy(bad, good, n);
            put_le(bad + fields[i].offset, 715908u, 8u);
            reseal(bad, n);
            CHECK_EQ_U64(schway_load_state(target, bad, n), SCHWAY_STATUS_OK);
            CHECK_EQ_U64(target->tick_acc, 715908u);
        }
    }
    memcpy(bad, good, n);
    reseal(bad, n);
    CHECK_EQ_U64(schway_load_state(target, bad, n), SCHWAY_STATUS_OK);
    schway_destroy(target);
    schway_destroy(src);
}

/* The offset of the first field called name in a saved state, or 0. */
static uint32_t offset_of(const schway *inst, const char *name)
{
    size_t nf = schway__state_record(inst, fields, MAX_FIELDS);
    CHECK(nf <= MAX_FIELDS);
    for (size_t i = 0; i < nf && i < MAX_FIELDS; i++) {
        if (strcmp(fields[i].name, name) == 0) {
            return fields[i].offset;
        }
    }
    CHECK(0);
    return 0u;
}

static int field_record_matches(const schway *inst, const char *name, uint32_t width,
                                uint32_t count)
{
    size_t nf = schway__state_record(inst, fields, MAX_FIELDS);
    if (nf > MAX_FIELDS) {
        return 0;
    }
    for (size_t i = 0; i < nf && i < MAX_FIELDS; i++) {
        if (strcmp(fields[i].name, name) == 0) {
            return fields[i].width == width && fields[i].count == count;
        }
    }
    return 0;
}

static uint32_t disc_read_a(void *user, uint32_t lba, uint8_t sector[2352], uint8_t subcode[96])
{
    (void)user;
    (void)lba;
    (void)sector;
    (void)subcode;
    return 0u;
}

static uint32_t disc_read_b(void *user, uint32_t lba, uint8_t sector[2352], uint8_t subcode[96])
{
    (void)user;
    (void)lba;
    (void)sector;
    (void)subcode;
    return 0u;
}

static void seed_cdrom(schway *inst)
{
    inst->profile_flags = SCHWAY_PROFILE_CD_LICENSED | SCHWAY_PROFILE_CD_UNLOCK;
    inst->cdrom.parameter[15] = 0xA1u;
    inst->cdrom.result[8] = 0xB2u;
    inst->cdrom.interrupt[7] = 4u;
    inst->cdrom.bank = 2u;
    inst->cdrom.parameter_head = 15u;
    inst->cdrom.parameter_count = 3u;
    inst->cdrom.result_head = 8u;
    inst->cdrom.result_count = 1u;
    inst->cdrom.interrupt_head = 7u;
    inst->cdrom.interrupt_count = 1u;
    inst->cdrom.interrupt_mask = 0x15u;
    inst->cdrom.paused = 1u;
    inst->cdrom.muted = 1u;
    inst->cdrom.drive_speed = 2u;
    inst->cdrom.drive_speed_changed = 1u;
    inst->cdrom.unlock_enabled = 1u;
    inst->cdrom.unlock_step = 6u;
    inst->cdrom.command = 0x06u;
    inst->cdrom.command_pending = 1u;
    inst->cdrom.status = 0x20u;
    inst->cdrom.mode = 0x80u;
    inst->cdrom.file = 3u;
    inst->cdrom.channel = 4u;
    inst->cdrom.last_command = 0x19u;
    inst->cdrom.motor_on = 1u;
    inst->cdrom.read_active = 1u;
    inst->cdrom.bfrd = 1u;
    inst->cdrom.sector_int1_pending = 1u;
    inst->cdrom.read_s = 1u;
    inst->cdrom.sector_retry = 1u;
    inst->cdrom.current_lba = 123u;
    inst->cdrom.target_lba = 456u;
    const uint8_t atv_pending[4] = {0x10u, 0x20u, 0x30u, 0x40u};
    const uint8_t atv_active[4] = {0x50u, 0x60u, 0x70u, 0x80u};
    memcpy(inst->cdrom.atv_pending, atv_pending, sizeof atv_pending);
    memcpy(inst->cdrom.atv_active, atv_active, sizeof atv_active);
    const uint8_t q_record[12] = {
        0x41u, 0x01u, 0x01u, 0x00u, 0x00u, 0x11u,
        0x00u, 0x00u, 0x02u, 0x11u, 0x84u, 0x29u
    };
    memcpy(inst->cdrom.q_record, q_record, sizeof q_record);
    inst->cdrom.q_record_valid = 1u;
    inst->cdrom.sector_head = 2u;
    inst->cdrom.sector_count = 1u;
    inst->cdrom.sector_offset = 12u;
    inst->cdrom.sector[2][0] = 0xC3u;
    inst->cdrom.sector[2][SCHWAY__CDROM_SECTOR_BYTES - 1u] = 0xD4u;
    inst->cdrom.sector_lba[2] = 123u;
    inst->cdrom.sector_size[2] = 2048u;
    inst->cdrom.xa_history[1][SCHWAY__CDROM_XA_TAPS - 1u] = -321;
    inst->cdrom.xa_adpcm_history[0][0] = 456;
    inst->cdrom.xa_adpcm_history[1][1] = -789;
    inst->cdrom.xa_phase = 5u;
    inst->cdrom.audio[1][0] = 0x1234u;
    inst->cdrom.audio[1][1] = 0xFEDCu;
    inst->cdrom.audio_head = 1u;
    inst->cdrom.audio_count = 3u;
    inst->cdrom.xa_audio[1][0] = 0x2468u;
    inst->cdrom.xa_audio[1][1] = 0xFEDCu;
    inst->cdrom.xa_audio_head = 1u;
    inst->cdrom.xa_audio_count = 3u;
    schway__spu_cd_audio_active(inst);
    inst->cdrom.command_event = 1u;
    inst->cdrom.sector_event = 1u;
    inst->cdrom.command_deadline = 1000u;
    inst->cdrom.sector_deadline = 2000u;
}

static void seed_mdec(schway *inst)
{
    inst->mdec.input[0] = 0x11223344u;
    inst->mdec.input[SCHWAY__MDEC_INPUT_WORDS - 1u] = 0x55667788u;
    inst->mdec.output[0] = 0x99AABBCCu;
    inst->mdec.output[SCHWAY__MDEC_OUTPUT_WORDS - 1u] = 0xDDEEFF00u;
    inst->mdec.quant[SCHWAY__MDEC_TABLE_BYTES - 1u] = 0x5Au;
    inst->mdec.scale[SCHWAY__MDEC_BLOCK_SAMPLES - 1u] = -123;
    inst->mdec.block[0] = 456;
    inst->mdec.work[SCHWAY__MDEC_BLOCK_SAMPLES - 1u] = -789;
    inst->mdec.input_head = 1u;
    inst->mdec.input_count = 2u;
    inst->mdec.input_half = 1u;
    inst->mdec.output_head = SCHWAY__MDEC_OUTPUT_WORDS - 1u;
    inst->mdec.output_count = 3u;
    inst->mdec.command = 0x38000000u;
    inst->mdec.command_words = 17u;
    inst->mdec.words_remaining = 9u;
    inst->mdec.block_index = 4u;
    inst->mdec.depth = 2u;
    inst->mdec.signed_output = 1u;
    inst->mdec.bit15 = 1u;
    inst->mdec.control = (1u << 30) | (1u << 29);
    inst->mdec.command_active = 1u;
    inst->mdec.block_pending = 1u;
    inst->mdec.block_deadline = 900u;
}

static void seed_cdrom_mdec(schway *inst)
{
    seed_cdrom(inst);
    seed_mdec(inst);
}

static uint32_t cdrom_mdec_fields_match(schway *inst)
{
    static const struct {
        const char *name;
        uint32_t width, count;
    } expected_fields[] = {
        {"cdrom.parameter", 1u, SCHWAY__CDROM_FIFO_CAP},
        {"cdrom.result", 1u, SCHWAY__CDROM_FIFO_CAP},
        {"cdrom.interrupt", 1u, SCHWAY__CDROM_IRQ_CAP},
        {"cdrom.sector", 1u, SCHWAY__CDROM_SECTOR_CAP * SCHWAY__CDROM_SECTOR_BYTES},
        {"cdrom.q_record", 1u, 12u},
        {"cdrom.q_record_valid", 4u, 1u},
        {"cdrom.atv_pending", 1u, 4u},
        {"cdrom.atv_active", 1u, 4u},
        {"disc.identity_valid", 4u, 1u},
        {"disc.identity_sha256", 1u, 32u},
        {"cdrom.sector_lba", 4u, SCHWAY__CDROM_SECTOR_CAP},
        {"cdrom.sector_size", 4u, SCHWAY__CDROM_SECTOR_CAP},
        {"cdrom.bfrd", 4u, 1u},
        {"cdrom.sector_int1_pending", 4u, 1u},
        {"cdrom.read_s", 4u, 1u},
        {"cdrom.sector_retry", 4u, 1u},
        {"cdrom.xa_history", 2u, 2u * SCHWAY__CDROM_XA_TAPS},
        {"cdrom.xa_adpcm_history", 2u, 4u},
        {"cdrom.audio", 2u, 2u * SCHWAY__CDROM_AUDIO_CAP},
        {"cdrom.audio_head", 4u, 1u},
        {"cdrom.audio_count", 4u, 1u},
        {"cdrom.xa_audio", 2u, 2u * SCHWAY__CDROM_XA_AUDIO_CAP},
        {"cdrom.xa_audio_head", 4u, 1u},
        {"cdrom.xa_audio_count", 4u, 1u},
        {"cdrom.command_deadline", 8u, 1u},
        {"cdrom.sector_deadline", 8u, 1u},
        {"cdrom.seek_deadline", 8u, 1u},
        {"mdec.input", 4u, SCHWAY__MDEC_INPUT_WORDS},
        {"mdec.output", 4u, SCHWAY__MDEC_OUTPUT_WORDS},
        {"mdec.quant", 1u, SCHWAY__MDEC_TABLE_BYTES},
        {"mdec.scale", 2u, SCHWAY__MDEC_BLOCK_SAMPLES},
        {"mdec.block", 2u, SCHWAY__MDEC_BLOCK_SAMPLES},
        {"mdec.work", 4u, SCHWAY__MDEC_BLOCK_SAMPLES},
        {"mdec.input_head", 4u, 1u},
        {"mdec.input_count", 4u, 1u},
        {"mdec.input_half", 4u, 1u},
        {"mdec.output_head", 4u, 1u},
        {"mdec.output_count", 4u, 1u},
        {"mdec.block_deadline", 8u, 1u},
    };
    size_t missing_fields = 0u;
    for (size_t i = 0; i < sizeof expected_fields / sizeof expected_fields[0]; i++) {
        if (!field_record_matches(inst, expected_fields[i].name, expected_fields[i].width,
                                  expected_fields[i].count)) {
            missing_fields++;
        }
    }
    return (uint32_t)missing_fields;
}

static void cdrom_mdec_round_trip(schway *src, schway *dst, size_t n)
{
    CHECK_EQ_U64(save(dst, bad), n);
    CHECK(memcmp(good, bad, n) == 0);
    memset(&dst->cdrom, 0, sizeof dst->cdrom);
    memset(&dst->mdec, 0, sizeof dst->mdec);
    CHECK_EQ_U64(schway_load_state(dst, good, n), SCHWAY_STATUS_OK);
    CHECK(memcmp(&dst->cdrom, &src->cdrom, sizeof src->cdrom) == 0);
    CHECK(memcmp(&dst->mdec, &src->mdec, sizeof src->mdec) == 0);
    CHECK(dst->disc_host.callbacks.user == (void *)dst);
    CHECK(dst->disc_host.callbacks.read_sector == disc_read_b);
    CHECK_EQ_U64(dst->disc_host.attached, 1u);
    CHECK_EQ_U64(save(dst, after), n);
    CHECK(memcmp(good, after, n) == 0);
}

static void cdrom_mdec_reject_invalid(schway *src, schway *dst, size_t n)
{
    const struct {
        const char *what;
        const char *field;
        uint32_t delta;
        uint64_t value;
        uint32_t width;
    } invalid[] = {
        {"CD parameter FIFO head", "cdrom.parameter_head", 0u,
         SCHWAY__CDROM_FIFO_CAP, 4u},
        {"CD result FIFO count", "cdrom.result_count", 0u,
         SCHWAY__CDROM_FIFO_CAP + 1u, 4u},
        {"CD interrupt FIFO head", "cdrom.interrupt_head", 0u,
         SCHWAY__CDROM_IRQ_CAP, 4u},
        {"CD sector FIFO count", "cdrom.sector_count", 0u,
         SCHWAY__CDROM_SECTOR_CAP + 1u, 4u},
        {"CD XA phase", "cdrom.xa_phase", 0u, 6u, 4u},
        {"CD audio queue count", "cdrom.audio_count", 0u,
         SCHWAY__CDROM_AUDIO_LIMIT + 1u, 4u},
        {"CD XA queue count", "cdrom.xa_audio_count", 0u,
         SCHWAY__CDROM_XA_AUDIO_CAP + 1u, 4u},
        {"CD speed", "cdrom.drive_speed", 0u, 3u, 4u},
        {"CD unlock state", "cdrom.unlock_enabled", 0u, 2u, 4u},
        {"CD unlock step", "cdrom.unlock_step", 0u, 7u, 4u},
        {"CD Q record CRC", "cdrom.q_record", 10u, 0u, 1u},
        {"CD event deadline", "cdrom.command_deadline", 0u,
         SCHWAY__STATE_TICKS_MAX, 8u},
        {"CD inactive seek deadline", "cdrom.seek_deadline", 0u, 1u, 8u},
        {"MDEC input FIFO head", "mdec.input_head", 0u,
         SCHWAY__MDEC_INPUT_WORDS, 4u},
        {"MDEC input halfword selector", "mdec.input_half", 0u, 2u, 4u},
        {"MDEC DMA control", "mdec.control", 0u, 0x10000000u, 4u},
        {"MDEC output FIFO count", "mdec.output_count", 0u,
         SCHWAY__MDEC_OUTPUT_WORDS + 1u, 4u},
        {"MDEC block index", "mdec.block_index", 0u, 6u, 4u},
        {"MDEC block deadline", "mdec.block_deadline", 0u, 1001u, 8u},
    };
    for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; i++) {
        memcpy(bad, good, n);
        put_le(bad + offset_of(src, invalid[i].field) + invalid[i].delta,
               invalid[i].value, invalid[i].width);
        reseal(bad, n);
        expect_refused(invalid[i].what, dst, bad, n, SCHWAY_STATUS_ERR_STATE_FIELD);
    }
    size_t q_at = offset_of(src, "cdrom.q_record");
    memcpy(bad, good, n);
    bad[q_at + 7u] = 0xFAu;
    reseal_q_record(bad + q_at);
    reseal(bad, n);
    expect_refused("CD Q record BCD", dst, bad, n, SCHWAY_STATUS_ERR_STATE_FIELD);
    memcpy(bad, good, n);
    bad[q_at + 1u] = 0x02u;
    reseal_q_record(bad + q_at);
    reseal(bad, n);
    expect_refused("CD Q track outside TOC", dst, bad, n, SCHWAY_STATUS_ERR_STATE_FIELD);
}

static void cdrom_mdec_run_events(schway *src, schway *dst, size_t n)
{
    /* The events are crossed by one long budget or twelve shorter budgets;
       both paths must save the same machine state. */
    schway_budget budget;
    memset(&budget, 0, sizeof budget);
    budget.size = (uint32_t)sizeof budget;
    budget.max_cycles = 6000u;
    schway_run_result result;
    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    CHECK_EQ_U64(schway_run(src, &budget, &result), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(result.stop, SCHWAY_STOP_BUDGET);
    CHECK_EQ_U64(result.ticks, 6000u);
    budget.max_cycles = 500u;
    for (size_t i = 0; i < 12u; i++) {
        CHECK_EQ_U64(schway_run(dst, &budget, &result), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(result.stop, SCHWAY_STOP_BUDGET);
    }
    CHECK_EQ_U64(save(src, good), n);
    CHECK_EQ_U64(save(dst, after), n);
    CHECK(memcmp(good, after, n) == 0);
    CHECK(dst->disc_host.callbacks.read_sector == disc_read_b);
    CHECK(dst->disc_host.callbacks.user == (void *)dst);
}

static void cdrom_mdec_reset_state(schway *dst)
{
    /* The device structs are larger than the default WASI process stack. */
    static struct schway__cdrom zero_cd;
    static struct schway__mdec zero_mdec;
    memset(&zero_cd, 0, sizeof zero_cd);
    zero_cd.drive_speed = 1u;
    zero_cd.atv_active[0] = 0x80u;
    zero_cd.atv_active[2] = 0x80u;
    zero_cd.atv_pending[0] = 0x80u;
    zero_cd.atv_pending[2] = 0x80u;
    memset(&zero_mdec, 0, sizeof zero_mdec);
    zero_mdec.block_index = 4u;
    CHECK_EQ_U64(schway_reset(dst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    CHECK(memcmp(&dst->cdrom, &zero_cd, sizeof zero_cd) == 0);
    CHECK(memcmp(&dst->mdec, &zero_mdec, sizeof zero_mdec) == 0);
}

static void test_cdrom_mdec_state(void)
{
    schway *src = make(SCHWAY_REGION_NTSC_U);
    schway *dst = make(SCHWAY_REGION_NTSC_U);
    seed_cdrom_mdec(src);
    seed_cdrom_mdec(dst);
    src->disc_host.toc.track_count = 1u;
    dst->disc_host.toc.track_count = 1u;
    src->disc_host.callbacks.user = src;
    src->disc_host.callbacks.read_sector = disc_read_a;
    src->disc_host.attached = 1u;
    src->disc_host.identity_valid = 1u;
    memset(src->disc_host.identity_sha256, 0x4Du, sizeof src->disc_host.identity_sha256);
    dst->disc_host.callbacks.user = dst;
    dst->disc_host.callbacks.read_sector = disc_read_b;
    dst->disc_host.attached = 1u;
    dst->disc_host.identity_valid = 1u;
    memcpy(dst->disc_host.identity_sha256, src->disc_host.identity_sha256,
           sizeof dst->disc_host.identity_sha256);
    uint32_t missing_fields = cdrom_mdec_fields_match(src);
    CHECK_EQ_U64(missing_fields, 0u);
    if (missing_fields == 0u) {
        size_t n = save(src, good);
        cdrom_mdec_round_trip(src, dst, n);
        cdrom_mdec_reject_invalid(src, dst, n);
        cdrom_mdec_run_events(src, dst, n);
        cdrom_mdec_reset_state(dst);
    }
    schway_destroy(dst);
    schway_destroy(src);
}

static void test_spu_state_continuation(void)
{
    static const struct {
        const char *name;
        uint32_t width, count;
    } expected_fields[] = {
        {"spu.ram", 1u, SCHWAY__SPU_RAM_SIZE},
        {"spu.reg", 2u, SCHWAY__SPU_REG_HALFWORDS},
        {"spu.voice.reg", 2u, SCHWAY__SPU_VOICE_COUNT * 8u},
        {"spu.voice.history", 2u, SCHWAY__SPU_VOICE_COUNT * 3u},
        {"spu.voice.sample_counter", 4u, SCHWAY__SPU_VOICE_COUNT},
        {"spu.voice.envelope", 4u, SCHWAY__SPU_VOICE_COUNT},
        {"spu.voice.envelope_counter", 4u, SCHWAY__SPU_VOICE_COUNT},
        {"spu.voice.envelope_phase", 4u, SCHWAY__SPU_VOICE_COUNT},
        {"spu.voice.sample", 2u, SCHWAY__SPU_VOICE_COUNT},
        {"spu.voice.output", 2u, SCHWAY__SPU_VOICE_COUNT},
        {"spu.voice.sweep_level", 4u, SCHWAY__SPU_VOICE_COUNT * 2u},
        {"spu.voice.sweep_counter", 4u, SCHWAY__SPU_VOICE_COUNT * 2u},
        {"spu.transfer_fifo", 2u, SCHWAY__SPU_FIFO_HALFWORDS},
        {"spu.transfer_head", 4u, 1u},
        {"spu.transfer_count", 4u, 1u},
        {"spu.transfer_address", 4u, 1u},
        {"spu.pending.address", 2u, SCHWAY__SPU_PENDING_WRITES},
        {"spu.pending.value", 2u, SCHWAY__SPU_PENDING_WRITES},
        {"spu.pending_count", 4u, 1u},
        {"spu.noise_phase", 4u, 1u},
        {"spu.noise_lfsr", 2u, 1u},
        {"spu.capture_cursor", 4u, 1u},
        {"spu.reverb_fir", 2u, 2u * SCHWAY__SPU_REVERB_FIR_TAPS},
        {"spu.reverb_work", 4u, 2u * SCHWAY__SPU_REVERB_WORK_WORDS},
        {"spu.reverb_fir_cursor", 4u, 1u},
        {"spu.reverb_work_cursor", 4u, 1u},
        {"spu.reverb_address", 4u, 1u},
        {"spu.main_sweep_level", 4u, 2u},
        {"spu.main_sweep_counter", 4u, 2u},
        {"spu.irqa_addr", 4u, 1u},
        {"spu.irq_enabled", 4u, 1u},
        {"spu.next_tick", 8u, 1u},
    };
    schway *src = make(SCHWAY_REGION_NTSC_U);
    schway *dst = make(SCHWAY_REGION_NTSC_U);
    src->spu.ram[0] = 0x12u;
    src->spu.ram[SCHWAY__SPU_RAM_SIZE - 1u] = 0x34u;
    src->spu.reg[255] = 0x5678u;
    src->spu.voice[0].history[2] = -123;
    src->spu.voice[23].envelope = -456;
    src->spu.voice[23].sample_counter = 789u;
    src->spu.transfer_fifo[31] = 0x9ABCu;
    src->spu.transfer_head = 7u;
    src->spu.transfer_count = 3u;
    src->spu.transfer_address = 0x1234u;
    src->spu.pending[0].address = 0x20u;
    src->spu.pending[0].value = 0xABCDu;
    src->spu.pending[1].address = 0x27Eu;
    src->spu.pending[1].value = 0xBCDEu;
    src->spu.pending_count = 2u;
    src->spu.noise_phase = 17u;
    src->spu.noise_lfsr = 0x1234u;
    src->spu.capture_cursor = 23u;
    src->spu.reverb_fir[1][38] = -78;
    src->spu.reverb_work[1][31] = -8765;
    src->spu.reverb_fir_cursor = 17u;
    src->spu.reverb_work_cursor = 21u;
    src->spu.reverb_address = 0x2000u;
    src->spu.main_sweep_level[0] = -900;
    src->spu.main_sweep_counter[1] = 37u;
    src->spu.irqa_addr = 0x4000u;
    src->spu.irq_enabled = 1u;
    src->spu.next_tick = SCHWAY__CYCLES_PER_PAIR;
    size_t n = save(src, good);
    size_t missing_fields = 0u;
    for (size_t i = 0; i < sizeof expected_fields / sizeof expected_fields[0]; i++) {
        if (!field_record_matches(src, expected_fields[i].name, expected_fields[i].width,
                                  expected_fields[i].count)) {
            missing_fields++;
        }
    }
    CHECK_EQ_U64(missing_fields, 0u);

    /* A queued host sample is deliberately excluded from the state and must
       be discarded when the saved machine resumes. */
    schway__spu_event(dst, dst->spu.next_tick);
    CHECK_EQ_U64(dst->audio_count, 1u);
    CHECK_EQ_U64(schway_load_state(dst, good, n), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(dst->audio_count, 0u);
    CHECK_EQ_U64(dst->audio_head, 0u);
    CHECK_EQ_U64(dst->spu.next_tick, src->spu.next_tick);
    CHECK_EQ_U64(save(dst, after), n);
    CHECK(memcmp(good, after, n) == 0);

    struct visit_ctx c;
    memset(&c, 0, sizeof c);
    CHECK_EQ_U64(schway_state_visit(src, on_field, &c), SCHWAY_STATUS_OK);
    CHECK(!c.bad_name && !c.bad_width && !c.overflow);
    CHECK_EQ_U64(c.n, n - SCHWAY__STATE_HEADER_SIZE);
    CHECK(c.n == n - SCHWAY__STATE_HEADER_SIZE &&
          memcmp(stream, good + SCHWAY__STATE_HEADER_SIZE, c.n) == 0);

    schway__spu_event(src, src->spu.next_tick);
    schway__spu_event(dst, dst->spu.next_tick);
    CHECK_EQ_U64(src->audio_count, 1u);
    CHECK_EQ_U64(dst->audio_count, 1u);
    int16_t src_pair[2] = {0, 0}, dst_pair[2] = {0, 0};
    schway_audio src_audio, dst_audio;
    memset(&src_audio, 0, sizeof src_audio);
    memset(&dst_audio, 0, sizeof dst_audio);
    src_audio.size = (uint32_t)sizeof src_audio;
    src_audio.samples = src_pair;
    src_audio.capacity = 1u;
    dst_audio.size = (uint32_t)sizeof dst_audio;
    dst_audio.samples = dst_pair;
    dst_audio.capacity = 1u;
    CHECK_EQ_U64(schway_get_audio(src, &src_audio), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(schway_get_audio(dst, &dst_audio), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(src_audio.written, 1u);
    CHECK_EQ_U64(dst_audio.written, 1u);
    CHECK(memcmp(src_pair, dst_pair, sizeof src_pair) == 0);
    CHECK_EQ_U64(src->spu.next_tick, 2u * SCHWAY__CYCLES_PER_PAIR);
    CHECK_EQ_U64(dst->spu.next_tick, src->spu.next_tick);
    schway_destroy(dst);
    schway_destroy(src);
}

static void test_spu_invalid_states(void)
{
    struct invalid_field {
        const char *name;
        uint32_t offset;
        uint32_t width;
        uint64_t value;
    };
    schway *src = make(SCHWAY_REGION_NTSC_U);
    schway *dst = make(SCHWAY_REGION_NTSC_U);
    size_t n = save(src, good);
    schway__spu_event(dst, dst->spu.next_tick);
    CHECK_EQ_U64(dst->audio_count, 1u);
    const struct invalid_field invalid[] = {
        {"SPU transfer FIFO head", offset_of(src, "spu.transfer_head"), 4u,
         SCHWAY__SPU_FIFO_HALFWORDS},
        {"SPU transfer FIFO count", offset_of(src, "spu.transfer_count"), 4u,
         SCHWAY__SPU_FIFO_HALFWORDS + 1u},
        {"SPU transfer pointer range", offset_of(src, "spu.transfer_address"), 4u,
         SCHWAY__SPU_RAM_SIZE},
        {"SPU transfer pointer alignment", offset_of(src, "spu.transfer_address"), 4u, 1u},
        {"SPU pending write count", offset_of(src, "spu.pending_count"), 4u,
         SCHWAY__SPU_PENDING_WRITES + 1u},
        {"SPU pending address outside register image",
         offset_of(src, "spu.pending.address"), 2u, UINT16_MAX},
        {"SPU pending address alignment", offset_of(src, "spu.pending.address"), 2u, 1u},
        {"SPU pending FIFO address", offset_of(src, "spu.pending.address"), 2u, 0x1A8u},
        {"SPU pending current volume address", offset_of(src, "spu.pending.address"), 2u,
         0x200u},
        {"SPU pending STATX address", offset_of(src, "spu.pending.address"), 2u, 0x1AEu},
        {"SPU capture cursor", offset_of(src, "spu.capture_cursor"), 4u, 0x200u},
        {"SPU voice envelope phase", offset_of(src, "spu.voice.envelope_phase") +
                                          (SCHWAY__SPU_VOICE_COUNT - 1u) * 4u,
         4u, 5u},
        {"SPU FIR cursor", offset_of(src, "spu.reverb_fir_cursor"), 4u,
         SCHWAY__SPU_REVERB_FIR_TAPS},
        {"SPU work cursor", offset_of(src, "spu.reverb_work_cursor"), 4u,
         SCHWAY__SPU_REVERB_WORK_WORDS},
        {"SPU noise LFSR", offset_of(src, "spu.noise_lfsr"), 2u, 0u},
        {"SPU reverb address alignment", offset_of(src, "spu.reverb_address"), 4u, 1u},
        {"SPU IRQA alignment", offset_of(src, "spu.irqa_addr"), 4u, 1u},
        {"SPU sample phase", offset_of(src, "spu.next_tick"), 8u, 512u},
    };
    for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; i++) {
        memcpy(bad, good, n);
        if (strncmp(invalid[i].name, "SPU pending ", 12u) == 0) {
            put_le(bad + offset_of(src, "spu.pending_count"), 1u, 4u);
        }
        put_le(bad + invalid[i].offset, invalid[i].value, invalid[i].width);
        reseal(bad, n);
        expect_refused(invalid[i].name, dst, bad, n, SCHWAY_STATUS_ERR_STATE_FIELD);
    }
    memcpy(bad, good, n);
    put_le(bad + offset_of(src, "spu.pending_count"), 18u, 4u);
    put_le(bad + offset_of(src, "spu.pending.address") + 17u * 2u, UINT16_MAX, 2u);
    reseal(bad, n);
    expect_refused("SPU invalid address in later active slot", dst, bad, n,
                   SCHWAY_STATUS_ERR_STATE_FIELD);
    schway_destroy(dst);
    schway_destroy(src);
}

/* The geometry engine, DMA and counter fields come back from a save and a
   load byte for byte; a timer's base tick is no part of the state; and a
   load refuses the DMA bits the hardware does not store and a current
   channel that is not started, and changes nothing. */
static void test_device_fields(void)
{
    schway *src = make(SCHWAY_REGION_NTSC_U);
    run_to(src, 5);
    CHECK_EQ_HEX(src->dma.dpcr, 0x07654321u);
    CHECK_EQ_U64(src->dma.cur, SCHWAY__DMA_NONE);
    src->gte.ready = src->cpu.cycle + 20u;
    src->gte.d[3] = 0x80000001u;
    src->gte.c[31] = 0x7FFFF000u;
    src->dma.madr[2] = 0x123456u;
    src->dma.bcr[2] = 0x00040010u;
    src->dma.chcr[2] = 0x01000201u;
    src->dma.cur = 2u;
    src->dma.phase[2] = 1u;
    src->dma.busy_until = src->ticks + 1000u;
    src->dma.dicr = 0x00A38000u;
    src->dma.crc[6] = 0xDEADBEEFu;
    /* The counters are stored as seen at the instance's tick, so only a view
       that does not move shows that the base tick is no part of the state:
       sources 1 of counters 0 and 1 have no clock yet, and counter 2 waits
       for its synchronisation. */
    src->timers.t[0].mode = 0x0100u;
    src->timers.t[1].value = 0x1234u;
    src->timers.t[1].mode = 0x0100u;
    src->timers.t[2].mode = 0x0001u;
    src->timers.t[2].paused = 1u;
    src->timers.t[2].hold = 2u;
    uint32_t cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(src, 0xBF801DAAu, 2u, 0x0020u, &cycles), 0u);
    CHECK_EQ_HEX(spu_attr(src), 0x0020u);
    size_t n = save(src, good);
    schway *dst = make(SCHWAY_REGION_NTSC_U);
    CHECK_EQ_HEX(spu_attr(dst), 0u);
    cycles = 0u;
    CHECK_EQ_U64(schway__bus_write(dst, 0xBF801DAAu, 2u, 0x0010u, &cycles), 0u);
    CHECK_EQ_U64(schway_load_state(dst, good, n), SCHWAY_STATUS_OK);
    CHECK(save(dst, after) == n && memcmp(good, after, n) == 0);
    CHECK_EQ_U64(dst->gte.ready, src->gte.ready);
    CHECK_EQ_HEX(dst->gte.d[3], 0x80000001u);
    CHECK_EQ_HEX(dst->dma.chcr[2], 0x01000201u);
    CHECK_EQ_U64(dst->dma.cur, 2u);
    CHECK_EQ_HEX(dst->dma.crc[6], 0xDEADBEEFu);
    CHECK_EQ_HEX(spu_attr(dst), 0x0020u);
    CHECK_EQ_U64(dst->timers.t[1].value, 0x1234u);
    CHECK_EQ_U64(dst->timers.base, dst->ticks);
    src->timers.base = 12345u;
    CHECK(save(src, after) == n && memcmp(good, after, n) == 0);

    uint32_t chcr2 = offset_of(src, "dma.chcr") + 8u;
    uint32_t chcr3 = chcr2 + 4u;
    uint32_t dicr_at = offset_of(src, "dma.dicr");
    uint32_t cur_at = offset_of(src, "dma.cur");
    uint32_t phase2_at = offset_of(src, "dma.phase") + 2u * 4u;
    uint32_t phase3_at = phase2_at + 4u;
    uint32_t busy_at = offset_of(src, "dma.busy_until");
    uint32_t stall_at = offset_of(src, "dma.stall_until");
    const struct {
        const char *what;
        uint32_t at;
        uint32_t v;
    } refused[] = {
        {"chcr bit 2", chcr2, 0x01000205u},
        {"chcr bit 31", chcr2, 0x81000201u},
        {"dicr bit 6", dicr_at, 0x00A38040u},
        {"dicr bit 31", dicr_at, 0x80A38000u},
        {"current channel not started", cur_at, 3u},
    };
    for (size_t i = 0; i < sizeof refused / sizeof refused[0]; i++) {
        memcpy(bad, good, n);
        put_le(bad + refused[i].at, refused[i].v, 4u);
        reseal(bad, n);
        expect_refused(refused[i].what, dst, bad, n, SCHWAY_STATUS_ERR_STATE_FIELD);
    }
    memcpy(bad, good, n);
    put_le(bad + phase2_at, 6u, 4u);
    reseal(bad, n);
    expect_refused("DMA phase outside the engine states", dst, bad, n,
                   SCHWAY_STATUS_ERR_STATE_FIELD);

    /* A future CPU stall needs either a running unit or its completed unit
       deadline; an unlinked stamp with no current DMA deadline is invalid. */
    memcpy(bad, good, n);
    put_le(bad + phase2_at, 0u, 4u);
    put_le(bad + cur_at, SCHWAY__DMA_NONE, 4u);
    put_le(bad + busy_at, 0u, 8u);
    put_le(bad + stall_at, src->ticks + 1u, 8u);
    reseal(bad, n);
    expect_refused("DMA stall without a current channel", dst, bad, n,
                   SCHWAY_STATUS_ERR_STATE_FIELD);

    /* The same current channel, started, loads. */
    memcpy(bad, good, n);
    put_le(bad + chcr3, 0x01000000u, 4u);
    put_le(bad + phase2_at, 0u, 4u);
    put_le(bad + phase3_at, 1u, 4u);
    put_le(bad + cur_at, 3u, 4u);
    reseal(bad, n);
    CHECK_EQ_U64(schway_load_state(dst, bad, n), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(dst->dma.cur, 3u);
    CHECK_EQ_U64(schway_reset(src, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    CHECK_EQ_HEX(spu_attr(src), 0u);
    schway_destroy(dst);
    schway_destroy(src);

    /* A loaded ATTR latch, rather than a destination's prior value or a
       duplicated DMA request level, supplies the next DMA4 block. */
    schway *spu_src = make(SCHWAY_REGION_NTSC_U);
    schway *spu_dst = make(SCHWAY_REGION_NTSC_U);
    for (uint32_t i = 0u; i < 32u; i++) {
        bus_write(spu_src, 0x80001000u + i * 4u, 4u, 0xA5000000u + i);
    }
    bus_write(spu_src, 0xBF801DAAu, 2u, 0x0020u);
    bus_write(spu_src, 0xBF8010F0u, 4u, 0x0FEDCBA9u);
    bus_write(spu_src, 0xBF8010F4u, 4u, 0x00900000u);
    bus_write(spu_src, 0xBF8010C0u, 4u, 0x1000u);
    bus_write(spu_src, 0xBF8010C4u, 4u, (2u << 16) | 16u);
    bus_write(spu_src, 0xBF8010C8u, 4u, 0x01000201u);
    CHECK_EQ_U64(spu_src->dma.cur, 4u);
    CHECK_EQ_U64(spu_src->dma.words[4], 16u);
    CHECK_EQ_U64((spu_src->dma.dreq >> 4) & 1u, 0u);
    size_t spu_n = save(spu_src, good);
    bus_write(spu_dst, 0xBF801DAAu, 2u, 0x0010u);
    CHECK_EQ_HEX(spu_attr(spu_dst), 0x0010u);
    CHECK_EQ_U64(schway_load_state(spu_dst, good, spu_n), SCHWAY_STATUS_OK);
    CHECK_EQ_HEX(spu_attr(spu_dst), 0x0020u);
    CHECK_EQ_U64(spu_dst->dma.cur, 4u);
    CHECK_EQ_U64(spu_dst->dma.words[4], 16u);
    CHECK_EQ_U64(save(spu_dst, after), spu_n);
    CHECK(memcmp(good, after, spu_n) == 0);
    uint64_t first_end = spu_dst->dma.busy_until;
    schway__dma_event(spu_dst, first_end);
    CHECK_EQ_U64(spu_dst->dma.cur, 4u);
    CHECK(spu_dst->dma.busy_until > first_end);
    CHECK_EQ_U64(spu_dst->dma.words[4], 16u);
    uint64_t second_start = spu_dst->dma.busy_until;
    schway__dma_event(spu_dst, second_start);
    CHECK_EQ_U64(spu_dst->dma.words[4], 32u);
    CHECK((spu_dst->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY) != 0u);
    uint64_t second_end = spu_dst->dma.busy_until;
    schway__dma_event(spu_dst, second_end);
    CHECK_EQ_U64(spu_dst->dma.cur, SCHWAY__DMA_NONE);
    CHECK_EQ_U64(spu_dst->dma.chcr[4] & SCHWAY__DMA_CHCR_BUSY, 0u);
    CHECK_EQ_HEX(spu_dst->dma.madr[4], 0x1080u);
    CHECK_EQ_U64(spu_src->dma.words[4], 16u);
    CHECK_EQ_HEX(spu_attr(spu_src), 0x0020u);
    schway_destroy(spu_dst);
    schway_destroy(spu_src);

    test_spu_state_continuation();
    test_spu_invalid_states();
    test_cdrom_mdec_state();
}

/* Accepted counter boundaries must either advance to another loadable state
   or refuse the frame without changing even the unsaved audio and output. */
static void test_run_bounds(void)
{
    static const uint32_t regions[] = {
        SCHWAY_REGION_NTSC_J, SCHWAY_REGION_NTSC_U, SCHWAY_REGION_PAL
    };
    for (size_t k = 0; k < sizeof regions / sizeof regions[0]; k++) {
        schway *inst = make(regions[k]);
        schway *snapshot = malloc(sizeof *snapshot);
        CHECK(snapshot != NULL);
        if (snapshot == NULL) {
            exit(1);
        }
        size_t n = save(inst, good);
        size_t nf = schway__state_record(inst, fields, MAX_FIELDS);
        CHECK(nf <= MAX_FIELDS);
        uint32_t frame_at = 0u, ticks_at = 0u, acc_at = 0u, end_at = 0u, spu_next_at = 0u;
        uint64_t frame_max = 0u, ticks_max = 0u;
        for (size_t i = 0; i < nf && i < MAX_FIELDS; i++) {
            if (strcmp(fields[i].name, "instance.frame") == 0) {
                frame_at = fields[i].offset;
                frame_max = fields[i].max;
            } else if (strcmp(fields[i].name, "instance.ticks") == 0) {
                ticks_at = fields[i].offset;
                ticks_max = fields[i].max;
            } else if (strcmp(fields[i].name, "instance.tick_acc") == 0) {
                acc_at = fields[i].offset;
            } else if (strcmp(fields[i].name, "instance.frame_end") == 0) {
                end_at = fields[i].offset;
            } else if (strcmp(fields[i].name, "spu.next_tick") == 0) {
                spu_next_at = fields[i].offset;
            }
        }
        CHECK(frame_at != 0u && ticks_at != 0u && acc_at != 0u && end_at != 0u &&
              spu_next_at != 0u);
        if (frame_at == 0u || ticks_at == 0u || acc_at == 0u || end_at == 0u ||
            spu_next_at == 0u) {
            exit(1);
        }
        uint64_t den = schway__instance_frame_den(regions[k]);
        for (uint32_t carry = 0; carry < 2u; carry++) {
            uint64_t acc = carry ? den - 1u : 0u;
            uint64_t step = (acc + schway__instance_frame_num(regions[k])) / den;
            const uint64_t frames[] = {frame_max, frame_max - 1u, 0u, 0u, 0u, 0u, 0u};
            const uint64_t ticks[] = {
                0u, 0u, ticks_max, ticks_max - 1u,
                ticks_max - step, ticks_max - step + 1u, ticks_max - step - 1u
            };
            const int advances[] = {0, 1, 0, 0, 1, 0, 1};
            for (size_t j = 0; j < sizeof frames / sizeof frames[0]; j++) {
                memcpy(bad, good, n);
                put_le(bad + frame_at, frames[j], 8u);
                put_le(bad + ticks_at, ticks[j], 8u);
                put_le(bad + acc_at, acc, 8u);
                /* The frame that starts here ends one step on. */
                put_le(bad + end_at, ticks[j] + step, 8u);
                uint64_t next_spu_tick =
                    ticks[j] >= ticks_max - SCHWAY__CYCLES_PER_PAIR
                        ? UINT64_MAX
                        : (ticks[j] - ticks[j] % SCHWAY__CYCLES_PER_PAIR) +
                              SCHWAY__CYCLES_PER_PAIR;
                put_le(bad + spu_next_at, next_spu_tick, 8u);
                reseal(bad, n);
                CHECK_EQ_U64(schway_load_state(inst, bad, n), SCHWAY_STATUS_OK);
                memcpy(snapshot, inst, sizeof *snapshot);
                schway_run_result out, untouched;
                memset(&out, 0xA5, sizeof out);
                out.size = (uint32_t)sizeof out;
                memcpy(&untouched, &out, sizeof out);
                schway_status st = schway_run_frame(inst, &out);
                if (advances[j]) {
                    CHECK_EQ_U64(st, SCHWAY_STATUS_OK);
                    CHECK_EQ_U64(out.stop, SCHWAY_STOP_FRAME);
                    CHECK_EQ_U64(out.frame, frames[j] + 1u);
                    CHECK_EQ_U64(out.ticks, ticks[j] + step);
                    CHECK_EQ_U64(save(inst, before), n);
                    CHECK_EQ_U64(schway_load_state(inst, before, n), SCHWAY_STATUS_OK);
                    CHECK_EQ_U64(save(inst, after), n);
                    CHECK(memcmp(before, after, n) == 0);
                    /* Each successful boundary advance exhausts one counter. */
                    memcpy(snapshot, inst, sizeof *snapshot);
                    memcpy(&out, &untouched, sizeof out);
                    st = schway_run_frame(inst, &out);
                }
                CHECK_EQ_U64(st, SCHWAY_STATUS_ERR_STATE_FIELD);
                CHECK(memcmp(snapshot, inst, sizeof *snapshot) == 0);
                CHECK(memcmp(&untouched, &out, sizeof out) == 0);
                CHECK_EQ_U64(schway_run_frame(inst, NULL), SCHWAY_STATUS_ERR_STATE_FIELD);
                CHECK(memcmp(snapshot, inst, sizeof *snapshot) == 0);
            }
        }
        free(snapshot);
        schway_destroy(inst);
    }
}

static void check_public_versions(void)
{
    schway_versions v;
    memset(&v, 0, sizeof v);
    v.size = (uint32_t)sizeof v;
    CHECK_EQ_U64(schway_get_versions(&v), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(v.api, SCHWAY_API_VERSION);
    CHECK_EQ_U64(v.abi, SCHWAY__ABI_VERSION);
    CHECK_EQ_U64(v.state_format, 21u);
    CHECK_EQ_U64(v.behaviour_revision, 21u);
}

static void check_cpu_power_on(schway *inst)
{
    CHECK_EQ_HEX(inst->cpu.pc, 0xBFC00000u);
    CHECK_EQ_HEX(inst->cpu.cop0.r[12], 0x00400000u);
    CHECK_EQ_U64(inst->cpu.cop0.r[15], 2u);
    CHECK_EQ_HEX(inst->cpu.cop0.r[9], 0xFFFFFFFFu);
    CHECK_EQ_HEX(inst->cpu.cop0.r[11], 0xFFFFFFFFu);
    CHECK_EQ_U64(inst->cpu.cop0.r[13], 0u);
    for (uint32_t i = 0; i < 32u; i++) {
        CHECK_EQ_U64(inst->cpu.r[i], 0u);
    }
    CHECK_EQ_U64(inst->cpu.hi | inst->cpu.lo, 0u);
}

static void seed_cpu_bus_fields(schway *inst)
{
    inst->cpu.r[5] = 0x12345678u;
    inst->cpu.hi = 0xCAFEF00Du;
    inst->cpu.pc = 0x80010000u;
    inst->cpu.cop0.r[14] = 0x80020004u;
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    CHECK_EQ_U64(inst->bus.ram[0x1234], 0u);
    CHECK_EQ_U64(inst->irq.stat | inst->irq.mask, 0u);

    /* Guest RAM is little-endian; the first byte past the 8 MiB window the power-on RAM_SIZE
       selects is a bus error. */
    uint32_t cycles = 0u, word = 0u;
    CHECK_EQ_U64(schway__bus_write(inst, 0x1234u, 4u, 0xA1B2C3D4u, &cycles), 0u);
    CHECK_EQ_HEX(inst->bus.ram[0x1234], 0xD4u);
    CHECK_EQ_HEX(inst->bus.ram[0x1237], 0xA1u);
    CHECK_EQ_U64(schway__bus_read(inst, 0x1235u, 2u, &word, &cycles), 0u);
    CHECK_EQ_HEX(word, 0xB2C3u);
    CHECK_EQ_U64(schway__bus_fetch(inst, 0x1234u, &word, &cycles), 0u);
    CHECK_EQ_HEX(word, 0xA1B2C3D4u);
    CHECK_EQ_U64(schway__bus_write(inst, 0x1FFFFFu, 1u, 0x5Au, &cycles), 0u);
    CHECK(schway__bus_read(inst, 0x7FFFFEu, 4u, &word, &cycles) != 0);
    CHECK(schway__bus_read(inst, 0x800000u, 1u, &word, &cycles) != 0);
    CHECK_EQ_U64(word, 0u);
    /* A bus error keeps the first fault; clear it so the latch below is the first. */
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_READ);
    memset(&inst->fault, 0, sizeof inst->fault);
    inst->bus.scratch[0x3FF] = 0x77u;
    inst->bus.memctl[8] = 0x0B88u;
    inst->bus.unbuilt_n[SCHWAY__DEV_GPU] = 3u;
}

static void seed_cpu_fault_irq(schway *inst)
{
    /* The first fault wins; a later one changes nothing. */
    schway__fault_latch(inst, SCHWAY__FAULT_UNMAPPED_READ, SCHWAY__DEV_GPU, 4u, 0x80010000u,
                        0x8C220000u, 0x1F801814u);
    schway__fault_latch(inst, SCHWAY__FAULT_COP_UNUSABLE, SCHWAY__DEV_NONE, 0u, 1u, 2u, 3u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_READ);
    CHECK_EQ_HEX(inst->fault.addr, 0x1F801814u);

    /* An enabled interrupt raises the line; a disabled one does not. */
    schway__irq_raise(inst, 2u);
    CHECK_EQ_U64(schway__irq_line(inst), 0u);
    inst->irq.mask = 0x4u;
    inst->cpu.cop0.r[13] |= 1u << 10;
    CHECK_EQ_U64(schway__irq_line(inst), 1u);
    schway__irq_raise(inst, 11u);
    CHECK_EQ_HEX(inst->irq.stat, 0x4u);
}

static void seed_cpu_tty(schway *inst)
{
    for (uint32_t i = 0; i < 4097u; i++) {
        schway__tty_push(inst, (uint8_t)i);
    }
    CHECK_EQ_U64(inst->tty.count, 4096u);
    CHECK_EQ_U64(inst->tty.dropped, 1u);
    CHECK_EQ_HEX(inst->tty.buf[255], 0xFFu);
    inst->exit_pending = 1u;
}

static void check_cpu_loaded(schway *inst)
{
    CHECK_EQ_HEX(inst->cpu.r[5], 0x12345678u);
    CHECK_EQ_HEX(inst->cpu.hi, 0xCAFEF00Du);
    CHECK_EQ_HEX(inst->cpu.pc, 0x80010000u);
    CHECK_EQ_HEX(inst->cpu.cop0.r[14], 0x80020004u);
    CHECK_EQ_HEX(inst->bus.ram[0x1234], 0xD4u);
    CHECK_EQ_HEX(inst->bus.ram[0x1FFFFF], 0x5Au);
    CHECK_EQ_HEX(inst->bus.scratch[0x3FF], 0x77u);
    CHECK_EQ_HEX(inst->bus.memctl[8], 0x0B88u);
    CHECK_EQ_U64(inst->bus.unbuilt_n[SCHWAY__DEV_GPU], 3u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_UNMAPPED_READ);
    CHECK_EQ_U64(inst->fault.device, SCHWAY__DEV_GPU);
    CHECK_EQ_U64(inst->fault.width, 4u);
    CHECK_EQ_HEX(inst->fault.pc, 0x80010000u);
    CHECK_EQ_HEX(inst->fault.opcode, 0x8C220000u);
    CHECK_EQ_HEX(inst->fault.addr, 0x1F801814u);
    CHECK_EQ_HEX(inst->irq.stat, 0x4u);
    CHECK_EQ_HEX(inst->irq.mask, 0x4u);
}

/* CPU, bus and fault state round-trip; reset restores power-on values. */
static void test_cpu_fields(void)
{
    check_public_versions();
    schway *inst = make(SCHWAY_REGION_NTSC_U);
    check_cpu_power_on(inst);
    seed_cpu_bus_fields(inst);
    seed_cpu_fault_irq(inst);
    seed_cpu_tty(inst);
    size_t n = save(inst, good);
    CHECK_EQ_U64(schway_reset(inst, SCHWAY_RESET_POWER), SCHWAY_STATUS_OK);
    CHECK_EQ_U64(inst->cpu.r[5], 0u);
    CHECK_EQ_HEX(inst->cpu.pc, 0xBFC00000u);
    CHECK_EQ_U64(inst->bus.ram[0x1234], 0u);
    CHECK_EQ_U64(inst->fault.kind, SCHWAY__FAULT_NONE);
    CHECK_EQ_U64(inst->tty.count | inst->tty.dropped | inst->exit_pending, 0u);
    CHECK_EQ_U64(schway_load_state(inst, good, n), SCHWAY_STATUS_OK);
    check_cpu_loaded(inst);
    CHECK_EQ_U64(save(inst, after), n);
    CHECK(memcmp(good, after, n) == 0);
    schway_destroy(inst);
}

static const struct {
    const char *name;
    void (*run)(void);
} cases[] = {
    {"state.cpu_fields", test_cpu_fields},
    {"state.device_fields", test_device_fields},
    {"state.profile", test_profile},
    {"profile.fast_boot", test_fast_boot_profile},
    {"state.record_sweep", test_record_sweep},
    {"state.rejects", test_rejects},
    {"state.round_trip", test_round_trip},
    {"state.run_bounds", test_run_bounds},
    {"state.stable_bytes", test_stable_bytes},
    {"state.visit", test_visit},
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: schwaystation-test-state <case>\n");
        return 2;
    }
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        if (strcmp(argv[1], cases[i].name) == 0) {
            bufs_init();
            cases[i].run();
            bufs_free();
            CHECK_DONE();
        }
    }
    fprintf(stderr, "schwaystation-test-state: unknown case %s\n", argv[1]);
    return 2;
}
