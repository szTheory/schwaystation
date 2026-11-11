/* GPU beam tests run through the public time budget and real register bus. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"
#include "state.h"

#define GP1 UINT32_C(0x1F801814)
#define TIMER0_COUNT UINT32_C(0x1F801100)
#define TIMER0_MODE UINT32_C(0x1F801104)
#define TIMER0_TARGET UINT32_C(0x1F801108)
#define TIMER1_MODE UINT32_C(0x1F801114)
#define I_STAT UINT32_C(0x1F801070)
#define EXE_LOAD_ADDR UINT32_C(0x80010000)
#define MAX_FIELDS 256u

static int failures;

#define CHECK(expr)                                                                               \
    do {                                                                                          \
        if (!(expr)) {                                                                            \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr);                          \
            failures++;                                                                           \
        }                                                                                         \
    } while (0)

static void *plain_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void plain_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static schway *make_instance(uint32_t region, uint32_t flags)
{
    schway_config config;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = plain_alloc;
    config.allocator.free = plain_free;
    schway *inst = NULL;
    CHECK(schway_create(&config, &inst) == SCHWAY_STATUS_OK);
    if (inst == NULL) {
        return NULL;
    }
    schway_profile profile = {(uint32_t)sizeof profile, region, flags};
    CHECK(schway_set_profile(inst, &profile) == SCHWAY_STATUS_OK);
    return inst;
}

static void write_port(schway *inst, uint32_t addr, uint32_t value)
{
    uint32_t cycles = 0u;
    CHECK(schway__bus_write(inst, addr, 4u, value, &cycles) == 0);
}

static uint32_t read_port(schway *inst, uint32_t addr)
{
    uint32_t cycles = 0u;
    uint32_t value = 0u;
    CHECK(schway__bus_read(inst, addr, 4u, &value, &cycles) == 0);
    return value;
}

static void run_cycles(schway *inst, uint64_t cycles)
{
    schway_budget budget;
    memset(&budget, 0, sizeof budget);
    budget.size = (uint32_t)sizeof budget;
    budget.max_cycles = cycles;
    schway_run_result result;
    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    CHECK(schway_run(inst, &budget, &result) == SCHWAY_STATUS_OK);
    CHECK(result.stop == SCHWAY_STOP_BUDGET);
}

static void run_one_frame(schway *inst)
{
    schway_run_result result;
    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    CHECK(schway_run_frame(inst, &result) == SCHWAY_STATUS_OK);
    CHECK(result.stop == SCHWAY_STOP_FRAME);
}

static void put_le32(uint8_t *p, uint32_t value)
{
    for (uint32_t i = 0u; i < 4u; i++) {
        p[i] = (uint8_t)(value >> (i * 8u));
    }
}

static void put_le64(uint8_t *p, uint64_t value)
{
    for (uint32_t i = 0u; i < 8u; i++) {
        p[i] = (uint8_t)(value >> (i * 8u));
    }
}

static void load_guest(schway *inst, const uint32_t *program, size_t words)
{
    uint8_t exe[0x1000];
    memset(exe, 0, sizeof exe);
    memcpy(exe, "PS-X EXE", 8u);
    put_le32(exe + 0x10u, EXE_LOAD_ADDR);
    put_le32(exe + 0x18u, EXE_LOAD_ADDR);
    put_le32(exe + 0x1Cu, 0x800u);
    for (size_t i = 0u; i < words; i++) {
        put_le32(exe + 0x800u + i * 4u, program[i]);
    }
    CHECK(schway_load_exe(inst, exe, sizeof exe) == SCHWAY_STATUS_OK);
}

static void load_spin(schway *inst)
{
    static const uint32_t program[] = {0x08004000u, 0u}; /* j 80010000h; nop */
    load_guest(inst, program, sizeof program / sizeof program[0]);
}

static uint64_t ceil_div(uint64_t n, uint64_t d);

static uint64_t frame_tick(uint32_t region, uint32_t lines)
{
    uint64_t clock_num = region == SCHWAY_REGION_PAL ? UINT64_C(709379) : UINT64_C(715909);
    uint64_t line_clocks = region == SCHWAY_REGION_PAL ? UINT64_C(3406) : UINT64_C(3413);
    return ceil_div((uint64_t)lines * line_clocks * UINT64_C(451584), clock_num);
}

static void reset_beam(schway *inst, uint32_t mode)
{
    inst->gpu.display_mode = mode;
    inst->gpu.beam_tick = inst->ticks;
    inst->gpu.beam_ratio_remainder = 0u;
    inst->gpu.beam_line_clock = 0u;
    inst->gpu.beam_line = 0u;
    inst->gpu.beam_dot_phase = 0u;
    inst->timers.blank = 0u;
    schway__sched_sync(inst);
}

static void arm_midline_dot_irq(schway *inst, uint32_t target)
{
    write_port(inst, GP1, 0x08000001u); /* 320-dot mode, 8 GPU clocks per dot */
    write_port(inst, TIMER0_TARGET, target);
    write_port(inst, TIMER0_MODE, 0x00000110u); /* dot source and one-shot target IRQ */
}

static uint64_t ceil_div(uint64_t n, uint64_t d)
{
    return n / d + (n % d != 0u ? 1u : 0u);
}

static uint64_t midline_irq_tick(uint32_t region, uint32_t target)
{
    uint64_t numerator = region == SCHWAY_REGION_PAL ? UINT64_C(709379) : UINT64_C(715909);
    uint64_t denominator = UINT64_C(451584);
    uint64_t dots = (uint64_t)target + 2u; /* the timer's documented write hold */
    return ceil_div(dots * 8u * denominator, numerator);
}

static void test_midline_dot_irq(void)
{
    static const uint32_t regions[] = {
        SCHWAY_REGION_NTSC_J, SCHWAY_REGION_NTSC_U, SCHWAY_REGION_PAL};
    static const uint32_t gpu_flags[] = {0u, SCHWAY_PROFILE_GPU_V0};
    const uint32_t target = 37u;

    for (size_t r = 0u; r < sizeof regions / sizeof regions[0]; r++) {
        for (size_t f = 0u; f < sizeof gpu_flags / sizeof gpu_flags[0]; f++) {
            schway *split = make_instance(regions[r], gpu_flags[f]);
            schway *whole = make_instance(regions[r], gpu_flags[f]);
            CHECK(split != NULL && whole != NULL);
            if (split == NULL || whole == NULL) {
                schway_destroy(split);
                schway_destroy(whole);
                continue;
            }
            arm_midline_dot_irq(split, target);
            arm_midline_dot_irq(whole, target);
            uint64_t exact = midline_irq_tick(regions[r], target);
            CHECK(exact < 2500u); /* the target lands before the 3413/3406-clock line end */

            run_cycles(split, exact - 1u);
            CHECK((read_port(split, I_STAT) & (UINT32_C(1) << 4)) == 0u);
            run_cycles(split, 1u);
            CHECK((read_port(split, I_STAT) & (UINT32_C(1) << 4)) != 0u);
            run_cycles(split, 1u);

            run_cycles(whole, exact + 1u);
            size_t state_size = schway_state_size(split);
            CHECK(state_size == schway_state_size(whole));
            uint8_t *split_state = (uint8_t *)malloc(state_size);
            uint8_t *whole_state = (uint8_t *)malloc(state_size);
            CHECK(split_state != NULL && whole_state != NULL);
            if (split_state != NULL && whole_state != NULL) {
                CHECK(schway_save_state(split, split_state, state_size) == SCHWAY_STATUS_OK);
                CHECK(schway_save_state(whole, whole_state, state_size) == SCHWAY_STATUS_OK);
                CHECK(memcmp(split_state, whole_state, state_size) == 0);
            }
            free(split_state);
            free(whole_state);

            uint32_t clear_cycles = 0u;
            CHECK(schway__bus_write(split, I_STAT, 4u, UINT32_MAX & ~(UINT32_C(1) << 4),
                                    &clear_cycles) == 0);
            run_cycles(split, 9u);
            CHECK((read_port(split, I_STAT) & (UINT32_C(1) << 4)) == 0u);
            CHECK(split->ticks == exact + 10u);
            schway_destroy(whole);
            schway_destroy(split);
        }
    }
}

static void test_field_edges(void)
{
    static const uint32_t regions[] = {
        SCHWAY_REGION_NTSC_J, SCHWAY_REGION_NTSC_U, SCHWAY_REGION_PAL};
    for (size_t r = 0u; r < sizeof regions / sizeof regions[0]; r++) {
        uint32_t region = regions[r];
        schway *equal = make_instance(region, 0u);
        CHECK(equal != NULL);
        if (equal == NULL) {
            continue;
        }
        equal->exe_loaded = 1u;
        reset_beam(equal, 1u);
        equal->gpu.vertical_range = (2u << 10) | 2u;
        schway__gpu_sync(equal, frame_tick(region, 3u));
        CHECK((equal->irq.stat & 1u) != 0u);
        CHECK((equal->timers.blank & 1u) == 0u);
        CHECK(equal->gpu.beam_line == 3u);
        schway_destroy(equal);

        schway *inverted = make_instance(region, 0u);
        CHECK(inverted != NULL);
        if (inverted == NULL) {
            continue;
        }
        inverted->exe_loaded = 1u;
        reset_beam(inverted, 1u);
        inverted->gpu.vertical_range = (2u << 10) | 4u;
        schway__gpu_sync(inverted, frame_tick(region, 5u));
        CHECK((inverted->irq.stat & 1u) != 0u);
        CHECK((inverted->timers.blank & 1u) == 0u);
        CHECK(inverted->gpu.beam_line == 5u);
        schway_destroy(inverted);

        schway *out_of_range = make_instance(region, 0u);
        CHECK(out_of_range != NULL);
        if (out_of_range == NULL) {
            continue;
        }
        out_of_range->exe_loaded = 1u;
        reset_beam(out_of_range, 1u);
        out_of_range->gpu.vertical_range = (1023u << 10);
        uint32_t lines = region == SCHWAY_REGION_PAL ? 314u : 263u;
        schway__gpu_sync(out_of_range, frame_tick(region, lines));
        CHECK(out_of_range->frame == 1u);
        CHECK(out_of_range->gpu.beam_line == 0u);
        CHECK((out_of_range->irq.stat & 1u) == 0u);
        CHECK((out_of_range->timers.blank & 1u) == 0u);
        schway_destroy(out_of_range);
    }
}

static void test_status_fields(void)
{
    const uint32_t region = SCHWAY_REGION_NTSC_U;
    const uint32_t lines = 263u;
    static const uint32_t gpu_flags[] = {0u, SCHWAY_PROFILE_GPU_V0};

    for (size_t f = 0u; f < sizeof gpu_flags / sizeof gpu_flags[0]; f++) {
        uint32_t flags = gpu_flags[f];
        schway *inst = make_instance(region, flags);
        CHECK(inst != NULL);
        if (inst == NULL) {
            continue;
        }
        load_spin(inst);

        uint32_t status = read_port(inst, GP1);
        CHECK(((status >> 13u) & 1u) == (flags == 0u ? 1u : 0u));
        CHECK(((status >> 31u) & 1u) == 0u);

        write_port(inst, GP1, 0x07041C00u); /* Y1=0, Y2=263. */
        write_port(inst, GP1, 0x08000024u); /* 480-line interlace, NTSC. */
        status = read_port(inst, GP1);
        CHECK(((status >> 13u) & 1u) == (flags == 0u ? 1u : 0u));
        CHECK(((status >> 31u) & 1u) == 0u);

        uint32_t peek = 0u;
        CHECK(schway__gpu_peek(inst, GP1, 4u, &peek) == 0);
        CHECK(peek == status);
        CHECK(read_port(inst, GP1) == status);

        run_cycles(inst, frame_tick(region, lines - 1u));
        status = read_port(inst, GP1);
        CHECK(inst->gpu.beam_line == lines - 1u);
        CHECK(((status >> 13u) & 1u) == 0u);
        CHECK(((status >> 31u) & 1u) == 1u);

        size_t state_size = schway_state_size(inst);
        uint8_t *state = (uint8_t *)malloc(state_size);
        CHECK(state != NULL);
        if (state != NULL) {
            CHECK(schway_save_state(inst, state, state_size) == SCHWAY_STATUS_OK);
            schway *restored = make_instance(region, flags);
            CHECK(restored != NULL);
            if (restored != NULL) {
                load_spin(restored);
                write_port(restored, GP1, 0x07041C00u);
                write_port(restored, GP1, 0x08000024u);
                CHECK(((read_port(restored, GP1) >> 13u) & 1u) ==
                      (flags == 0u ? 1u : 0u));
                CHECK(schway_load_state(restored, state, state_size) == SCHWAY_STATUS_OK);
                uint32_t restored_status = read_port(restored, GP1);
                CHECK(((restored_status >> 13u) & 1u) == 0u);
                CHECK(((restored_status >> 31u) & 1u) == 1u);
                schway_destroy(restored);
            }
            free(state);
        }

        write_port(inst, GP1, 0x08000004u); /* Disable IRS while retaining phase. */
        status = read_port(inst, GP1);
        CHECK(((status >> 13u) & 1u) == 0u);
        run_one_frame(inst);
        run_one_frame(inst);
        CHECK(((read_port(inst, GP1) >> 13u) & 1u) == 0u);
        write_port(inst, GP1, 0x01000000u); /* Reset the command buffer only. */
        CHECK(((read_port(inst, GP1) >> 13u) & 1u) == 0u);

        write_port(inst, GP1, 0x00000000u); /* Full GPU reset restores the profile default. */
        status = read_port(inst, GP1);
        CHECK(((status >> 13u) & 1u) == (flags == 0u ? 1u : 0u));
        CHECK(((status >> 31u) & 1u) == 0u);
        schway_destroy(inst);
    }
}

static void write_status_range(schway *inst, uint32_t y1, uint32_t y2)
{
    write_port(inst, GP1, 0x07000000u | ((y2 & 0x03FFu) << 10u) | (y1 & 0x03FFu));
}

static void write_status_mode(schway *inst, uint32_t region, uint32_t mode)
{
    if (region == SCHWAY_REGION_PAL) {
        mode |= 0x08u;
    }
    write_port(inst, GP1, 0x08000000u | mode);
}

static uint32_t status_at_tick(schway *inst, uint64_t tick)
{
    uint32_t status = 0u;
    schway__gpu_sync(inst, tick);
    CHECK(schway__gpu_peek(inst, GP1, 4u, &status) == 0);
    return status;
}

static void check_status_peeks_are_pure(schway *inst)
{
    size_t state_size = schway_state_size(inst);
    uint8_t *before = (uint8_t *)malloc(state_size);
    uint8_t *after = (uint8_t *)malloc(state_size);
    CHECK(before != NULL && after != NULL);
    if (before == NULL || after == NULL) {
        free(before);
        free(after);
        return;
    }
    CHECK(schway_save_state(inst, before, state_size) == SCHWAY_STATUS_OK);
    uint32_t expected = 0u;
    CHECK(schway__gpu_peek(inst, GP1, 4u, &expected) == 0);
    for (uint32_t i = 0u; i < 16u; i++) {
        uint32_t peek = 0u;
        CHECK(schway__gpu_peek(inst, GP1, 4u, &peek) == 0);
        CHECK(peek == expected);
    }
    CHECK(schway_save_state(inst, after, state_size) == SCHWAY_STATUS_OK);
    CHECK(memcmp(before, after, state_size) == 0);
    free(before);
    free(after);
}

static void test_status_beam_edges(uint32_t region, uint32_t flags)
{
    uint32_t lines = region == SCHWAY_REGION_PAL ? 314u : 263u;
    uint32_t mode = 0x24u;
    schway *inst = make_instance(region, flags);
    CHECK(inst != NULL);
    if (inst == NULL) {
        return;
    }
    write_status_range(inst, 0u, lines);
    write_status_mode(inst, region, mode);
    uint32_t status = read_port(inst, GP1);
    CHECK(inst->gpu.field_status == 1u);
    CHECK(((status >> 13u) & 1u) == (flags == 0u ? 1u : 0u));
    CHECK(((status >> 31u) & 1u) == 0u);
    for (uint32_t i = 0u; i < 8u; i++) {
        CHECK(read_port(inst, GP1) == status);
    }
    check_status_peeks_are_pure(inst);

    uint64_t last_line = frame_tick(region, lines - 1u);
    status = status_at_tick(inst, last_line - 1u);
    CHECK(inst->gpu.beam_line == lines - 2u);
    CHECK(inst->gpu.field_status == 1u);
    CHECK(((status >> 13u) & 1u) == (flags == 0u ? 1u : 0u));
    CHECK(((status >> 31u) & 1u) == 0u);
    status = status_at_tick(inst, last_line);
    CHECK(inst->gpu.beam_line == lines - 1u);
    CHECK(inst->gpu.field_status == 0u);
    CHECK(((status >> 13u) & 1u) == 0u);
    CHECK(((status >> 31u) & 1u) == 1u);
    status = status_at_tick(inst, last_line + 1u);
    CHECK(inst->gpu.field_status == 0u);
    CHECK(((status >> 31u) & 1u) == 1u);

    uint64_t wrap = frame_tick(region, lines);
    status = status_at_tick(inst, wrap - 1u);
    CHECK(inst->gpu.beam_line == lines - 1u);
    CHECK(inst->gpu.field_status == 0u);
    CHECK(((status >> 31u) & 1u) == 1u);
    status = status_at_tick(inst, wrap);
    CHECK(inst->gpu.beam_line == 0u);
    CHECK(inst->gpu.field_status == 0u);
    CHECK(((status >> 13u) & 1u) == 0u);
    CHECK(((status >> 31u) & 1u) == 1u);
    status = status_at_tick(inst, wrap + 1u);
    CHECK(((status >> 31u) & 1u) == 1u);
    schway_destroy(inst);

    schway *vertical_240 = make_instance(region, flags);
    CHECK(vertical_240 != NULL);
    if (vertical_240 == NULL) {
        return;
    }
    write_status_range(vertical_240, 0u, lines);
    write_status_mode(vertical_240, region, 0x20u); /* IRS on with VDS off. */
    status = status_at_tick(vertical_240, frame_tick(region, lines - 1u));
    CHECK(vertical_240->gpu.field_status == 0u);
    CHECK(((status >> 13u) & 1u) == 0u);
    CHECK(((status >> 31u) & 1u) == ((lines - 1u) & 1u));
    schway_destroy(vertical_240);
}

static void test_status_range_edges(uint32_t region, uint32_t flags)
{
    uint32_t lines = region == SCHWAY_REGION_PAL ? 314u : 263u;
    schway *inst = make_instance(region, flags);
    CHECK(inst != NULL);
    if (inst == NULL) {
        return;
    }
    write_status_range(inst, 3u, 6u);
    write_status_mode(inst, region, 0x04u); /* Progressive 480-line signal mode. */
    uint64_t y1 = frame_tick(region, 3u);
    uint64_t y2 = frame_tick(region, 6u);
    run_cycles(inst, y1 - 1u);
    uint32_t status = read_port(inst, GP1);
    CHECK(inst->gpu.beam_line == 2u);
    CHECK(((status >> 31u) & 1u) == 0u);
    run_cycles(inst, 1u);
    status = read_port(inst, GP1);
    CHECK(inst->gpu.beam_line == 3u);
    CHECK(((status >> 31u) & 1u) == 1u);
    run_cycles(inst, 1u);
    status = read_port(inst, GP1);
    CHECK(inst->gpu.beam_line == 3u);
    CHECK(((status >> 31u) & 1u) == 1u);

    write_port(inst, GP1, 0x03000001u); /* Disable display without changing status parity. */
    status = read_port(inst, GP1);
    CHECK(((status >> 23u) & 1u) == 1u);
    CHECK(((status >> 31u) & 1u) == 1u);

    run_cycles(inst, 1u);
    status = read_port(inst, GP1);
    CHECK(inst->gpu.beam_line == 3u);
    CHECK(((status >> 31u) & 1u) == 1u);

    run_cycles(inst, y2 - inst->ticks - 1u);
    status = read_port(inst, GP1);
    CHECK(inst->gpu.beam_line == 5u);
    CHECK(((status >> 31u) & 1u) == 1u);
    run_cycles(inst, 1u);
    status = read_port(inst, GP1);
    CHECK(inst->gpu.beam_line == 6u);
    CHECK(((status >> 31u) & 1u) == 0u);
    run_cycles(inst, 1u);
    status = read_port(inst, GP1);
    CHECK(inst->gpu.beam_line == 6u);
    CHECK(((status >> 31u) & 1u) == 0u);
    CHECK((inst->irq.stat & 1u) != 0u);
    write_port(inst, I_STAT, ~UINT32_C(1)); /* I_STAT clears written zeroes. */
    status = read_port(inst, GP1);
    CHECK((inst->irq.stat & 1u) == 0u);
    CHECK(((status >> 31u) & 1u) == 0u);
    run_cycles(inst, 1u);
    status = read_port(inst, GP1);
    CHECK(inst->gpu.beam_line == 6u);
    CHECK(((status >> 31u) & 1u) == 0u);
    schway_destroy(inst);

    const struct {
        uint32_t y1;
        uint32_t y2;
        uint32_t observe_lines;
    } invalid_ranges[] = {
        {0u, 0u, 1u},
        {4u, 2u, 5u},
        {lines, 1023u, 2u},
    };
    for (size_t i = 0u; i < sizeof invalid_ranges / sizeof invalid_ranges[0]; i++) {
        schway *invalid = make_instance(region, flags);
        CHECK(invalid != NULL);
        if (invalid == NULL) {
            continue;
        }
        write_status_range(invalid, invalid_ranges[i].y1, invalid_ranges[i].y2);
        write_status_mode(invalid, region, 0x04u);
        run_cycles(invalid, frame_tick(region, invalid_ranges[i].observe_lines));
        status = read_port(invalid, GP1);
        CHECK(((status >> 31u) & 1u) == 0u);
        if (invalid_ranges[i].y1 == lines) {
            CHECK((invalid->irq.stat & 1u) == 0u);
        }
        schway_destroy(invalid);
    }
}

static void test_status_irq_ack(uint32_t region, uint32_t flags)
{
    uint32_t lines = region == SCHWAY_REGION_PAL ? 314u : 263u;
    schway *inst = make_instance(region, flags);
    CHECK(inst != NULL);
    if (inst == NULL) {
        return;
    }
    write_status_range(inst, 0u, lines);
    write_status_mode(inst, region, 0x04u);
    uint32_t before = read_port(inst, GP1);
    write_port(inst, 0x1F801810u, 0x1F000000u); /* GP0(1Fh) requests GPU IRQ. */
    uint32_t pending = read_port(inst, GP1);
    CHECK(((pending >> 24u) & 1u) == 1u);
    CHECK(((pending >> 31u) & 1u) == ((before >> 31u) & 1u));
    write_port(inst, GP1, 0x02000000u);
    uint32_t acknowledged = read_port(inst, GP1);
    CHECK(((acknowledged >> 24u) & 1u) == 0u);
    CHECK(((acknowledged >> 31u) & 1u) == ((before >> 31u) & 1u));
    schway_destroy(inst);
}

static void test_status_split_run(uint32_t region, uint32_t flags)
{
    uint32_t lines = region == SCHWAY_REGION_PAL ? 314u : 263u;
    schway *split = make_instance(region, flags);
    schway *whole = make_instance(region, flags);
    CHECK(split != NULL && whole != NULL);
    if (split == NULL || whole == NULL) {
        schway_destroy(split);
        schway_destroy(whole);
        return;
    }
    write_status_range(split, 0u, lines);
    write_status_range(whole, 0u, lines);
    write_status_mode(split, region, 0x24u);
    write_status_mode(whole, region, 0x24u);
    uint64_t edge = frame_tick(region, lines - 1u);
    run_cycles(split, edge - 1u);
    run_cycles(split, 1u);
    run_cycles(whole, edge);
    CHECK(split->gpu.beam_line == whole->gpu.beam_line);
    CHECK(split->gpu.field_status == whole->gpu.field_status);
    CHECK(read_port(split, GP1) == read_port(whole, GP1));
    size_t state_size = schway_state_size(split);
    uint8_t *split_state = (uint8_t *)malloc(state_size);
    uint8_t *whole_state = (uint8_t *)malloc(state_size);
    CHECK(split_state != NULL && whole_state != NULL);
    if (split_state != NULL && whole_state != NULL) {
        CHECK(schway_save_state(split, split_state, state_size) == SCHWAY_STATUS_OK);
        CHECK(schway_save_state(whole, whole_state, state_size) == SCHWAY_STATUS_OK);
        CHECK(memcmp(split_state, whole_state, state_size) == 0);
    }
    free(split_state);
    free(whole_state);
    schway_destroy(whole);
    schway_destroy(split);
}

static void test_status_state_roundtrip(uint32_t region, uint32_t flags, uint32_t source_phase)
{
    uint32_t lines = region == SCHWAY_REGION_PAL ? 314u : 263u;
    uint64_t last_line = frame_tick(region, lines - 1u);
    schway *source = make_instance(region, flags);
    CHECK(source != NULL);
    if (source == NULL) {
        return;
    }
    load_spin(source);
    write_status_range(source, 0u, lines);
    write_status_mode(source, region, 0x24u);
    if (source_phase == 0u) {
        run_cycles(source, last_line);
    }
    CHECK(source->gpu.field_status == source_phase);
    size_t state_size = schway_state_size(source);
    uint8_t *state = (uint8_t *)malloc(state_size);
    CHECK(state != NULL);
    if (state == NULL) {
        schway_destroy(source);
        return;
    }
    CHECK(schway_save_state(source, state, state_size) == SCHWAY_STATUS_OK);

    schway *target = make_instance(region, flags);
    CHECK(target != NULL);
    if (target == NULL) {
        free(state);
        schway_destroy(source);
        return;
    }
    load_spin(target);
    if (source_phase == 1u) {
        write_status_range(target, 0u, lines);
        write_status_mode(target, region, 0x24u);
        run_cycles(target, last_line);
        write_status_mode(target, region, 0x04u);
    }
    CHECK(target->gpu.field_status != source_phase);
    CHECK(target->gpu.display_mode != source->gpu.display_mode);
    CHECK(schway_load_state(target, state, state_size) == SCHWAY_STATUS_OK);
    CHECK(target->gpu.field_status == source_phase);
    uint32_t status = read_port(target, GP1);
    CHECK(((status >> 13u) & 1u) == (flags == 0u ? source_phase : 0u));
    CHECK(((status >> 31u) & 1u) == (source_phase == 0u ? 1u : 0u));

    if (source_phase == 0u) {
        write_status_mode(target, region, 0x04u);
        CHECK(target->gpu.field_status == 0u);
        run_one_frame(target);
        run_one_frame(target);
        CHECK(target->gpu.field_status == 0u);
        status = read_port(target, GP1);
        CHECK(((status >> 13u) & 1u) == 0u);

        uint8_t *progressive_state = (uint8_t *)malloc(state_size);
        CHECK(progressive_state != NULL);
        if (progressive_state != NULL) {
            CHECK(schway_save_state(target, progressive_state, state_size) == SCHWAY_STATUS_OK);
            schway *restored = make_instance(region, flags);
            CHECK(restored != NULL);
            if (restored != NULL) {
                load_spin(restored);
                write_status_range(restored, 0u, lines);
                write_status_mode(restored, region, 0x24u);
                CHECK(restored->gpu.field_status == 1u);
                CHECK(schway_load_state(restored, progressive_state, state_size) ==
                      SCHWAY_STATUS_OK);
                CHECK(restored->gpu.display_mode == (region == SCHWAY_REGION_PAL ? 0x0Cu : 0x04u));
                CHECK(restored->gpu.field_status == 0u);
                run_one_frame(restored);
                run_one_frame(restored);
                CHECK(restored->gpu.field_status == 0u);
                CHECK(((read_port(restored, GP1) >> 13u) & 1u) == 0u);
                schway_destroy(restored);
            }
            free(progressive_state);
        }
    }
    free(state);
    schway_destroy(target);
    schway_destroy(source);
}

static void test_status_controls(void)
{
    static const uint32_t regions[] = {
        SCHWAY_REGION_NTSC_J, SCHWAY_REGION_NTSC_U, SCHWAY_REGION_PAL};
    static const uint32_t gpu_flags[] = {0u, SCHWAY_PROFILE_GPU_V0};
    for (size_t r = 0u; r < sizeof regions / sizeof regions[0]; r++) {
        for (size_t f = 0u; f < sizeof gpu_flags / sizeof gpu_flags[0]; f++) {
            uint32_t region = regions[r];
            uint32_t flags = gpu_flags[f];
            schway *retained = make_instance(region, flags);
            CHECK(retained != NULL);
            if (retained != NULL) {
                uint32_t lines = region == SCHWAY_REGION_PAL ? 314u : 263u;
                write_status_range(retained, 0u, lines);
                write_status_mode(retained, region, 0x24u);
                CHECK(retained->gpu.field_status == 1u);
                write_status_mode(retained, region, 0x04u);
                CHECK(retained->gpu.field_status == 1u);
                write_status_mode(retained, region, 0x14u);
                write_status_mode(retained, region, 0x44u);
                CHECK(retained->gpu.field_status == 1u);
                CHECK(((read_port(retained, GP1) >> 13u) & 1u) ==
                      (flags == 0u ? 1u : 0u));
                write_port(retained, GP1, 0x01000000u);
                CHECK(retained->gpu.field_status == 1u);
                write_status_mode(retained, region, 0x24u);
                CHECK(retained->gpu.field_status == 1u);
                schway_destroy(retained);
            }
            test_status_beam_edges(region, flags);
            test_status_range_edges(region, flags);
            test_status_irq_ack(region, flags);
            test_status_split_run(region, flags);
            test_status_state_roundtrip(region, flags, 0u);
            test_status_state_roundtrip(region, flags, 1u);
        }
    }
}

static void test_public_field_wrap(void)
{
    schway *inst = make_instance(SCHWAY_REGION_NTSC_U, 0u);
    CHECK(inst != NULL);
    if (inst == NULL) {
        return;
    }
    write_port(inst, GP1, 0x070FFC00u); /* Y2=1023 suppresses VBlank. */
    load_spin(inst);
    schway_run_result result;
    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    CHECK(schway_run_frame(inst, &result) == SCHWAY_STATUS_OK);
    CHECK(result.stop == SCHWAY_STOP_FRAME);
    CHECK(result.frame == 1u);
    CHECK(inst->frame == 1u);
    CHECK(result.ticks >= frame_tick(SCHWAY_REGION_NTSC_U, 263u));
    CHECK(result.ticks < frame_tick(SCHWAY_REGION_NTSC_U, 263u) + SCHWAY__INSN_CYCLES_MAX);
    CHECK((inst->irq.stat & 1u) == 0u);
    schway_destroy(inst);
}

static void test_reset_call_bound(void)
{
    static const uint32_t reset_loop[] = {
        0x3C081F80u, /* lui t0, 1F80h */
        0x35081814u, /* ori t0, t0, 1814h (GPU GP1) */
        0xAD000000u, /* sw zero, 0(t0): GP1(00h) */
        0x1000FFFEu, /* beq zero, zero, reset write */
        0u,          /* delay slot */
    };
    schway *inst = make_instance(SCHWAY_REGION_NTSC_U, 0u);
    CHECK(inst != NULL);
    if (inst == NULL) {
        return;
    }
    load_guest(inst, reset_loop, sizeof reset_loop / sizeof reset_loop[0]);
    schway_run_result result;
    memset(&result, 0, sizeof result);
    result.size = (uint32_t)sizeof result;
    CHECK(schway_run_frame(inst, &result) == SCHWAY_STATUS_OK);
    uint64_t max_field = schway__instance_frame_max(inst->region);
    CHECK(result.stop == SCHWAY_STOP_BUDGET);
    CHECK(result.frame == 0u);
    CHECK(result.ticks >= max_field);
    CHECK(result.ticks < max_field + SCHWAY__INSN_CYCLES_MAX);
    CHECK(inst->frame == 0u);

    size_t state_size = schway_state_size(inst);
    uint8_t *state = (uint8_t *)malloc(state_size);
    CHECK(state != NULL);
    if (state != NULL) {
        CHECK(schway_save_state(inst, state, state_size) == SCHWAY_STATUS_OK);
        schway *restored = make_instance(SCHWAY_REGION_NTSC_U, 0u);
        CHECK(restored != NULL);
        if (restored != NULL) {
            CHECK(schway_load_state(restored, state, state_size) == SCHWAY_STATUS_OK);
            schway_destroy(restored);
        }
    }
    free(state);
    schway_destroy(inst);
}

static void test_equal_timer_beam_dma(void)
{
    schway *inst = make_instance(SCHWAY_REGION_NTSC_U, 0u);
    CHECK(inst != NULL);
    if (inst == NULL) {
        return;
    }
    write_port(inst, GP1, 0x08000001u); /* 8 GPU clocks per dot. */
    write_port(inst, GP1, 0x06018000u); /* Hblank at 24 GPU clocks. */
    write_port(inst, TIMER0_TARGET, 1u);
    write_port(inst, TIMER0_MODE, 0x00000110u); /* target IRQ on Timer0 dots. */
    uint64_t event_tick = ceil_div(UINT64_C(24) * UINT64_C(451584), UINT64_C(715909));

    inst->dma.cur = 0u;
    inst->dma.phase[0] = 3u; /* one DMA gap ends at the tied event */
    inst->dma.chcr[0] = SCHWAY__DMA_CHCR_BUSY;
    inst->dma.busy_until = event_tick;
    schway__sched_recalc(inst);
    CHECK(inst->sched.next == event_tick);
    run_cycles(inst, event_tick);
    CHECK(inst->ticks == event_tick);
    CHECK((inst->irq.stat & (1u << 4)) != 0u);
    CHECK((inst->timers.blank & (1u << 1)) != 0u);
    CHECK(inst->dma.cur == SCHWAY__DMA_NONE);
    CHECK(inst->dma.phase[0] == 4u);
    CHECK(inst->sched.next > event_tick);
    schway_destroy(inst);
}

static uint32_t state_offset(const schway *inst, const char *name)
{
    struct schway__state_field fields[MAX_FIELDS];
    size_t count = schway__state_record(inst, fields, MAX_FIELDS);
    CHECK(count <= MAX_FIELDS);
    for (size_t i = 0u; i < count && i < MAX_FIELDS; i++) {
        if (strcmp(fields[i].name, name) == 0) {
            return fields[i].offset;
        }
    }
    CHECK(0);
    return 0u;
}

static void test_reject_malformed_beam_state(void)
{
    schway *source = make_instance(SCHWAY_REGION_NTSC_U, 0u);
    schway *target = make_instance(SCHWAY_REGION_NTSC_U, 0u);
    CHECK(source != NULL && target != NULL);
    if (source == NULL || target == NULL) {
        schway_destroy(source);
        schway_destroy(target);
        return;
    }
    size_t n = schway_state_size(source);
    uint8_t *good = (uint8_t *)malloc(n);
    uint8_t *bad = (uint8_t *)malloc(n);
    schway *snapshot = (schway *)malloc(sizeof *snapshot);
    CHECK(good != NULL && bad != NULL && snapshot != NULL);
    if (good == NULL || bad == NULL || snapshot == NULL) {
        free(good);
        free(bad);
        free(snapshot);
        schway_destroy(source);
        schway_destroy(target);
        return;
    }
    CHECK(schway_save_state(source, good, n) == SCHWAY_STATUS_OK);
    const struct {
        const char *name;
        uint64_t value;
        uint32_t width;
    } invalid[] = {
        {"gpu.beam_tick", 1u, 8u},
        {"gpu.beam_ratio_remainder", UINT64_C(451584), 4u},
        {"gpu.beam_line_clock", 3413u, 4u},
        {"gpu.beam_line", 263u, 4u},
        {"gpu.beam_dot_phase", 10u, 4u},
        {"gpu.field_status", 2u, 4u},
    };
    for (size_t i = 0u; i < sizeof invalid / sizeof invalid[0]; i++) {
        memcpy(bad, good, n);
        uint32_t offset = state_offset(source, invalid[i].name);
        if (invalid[i].width == 8u) {
            put_le64(bad + offset, invalid[i].value);
        } else {
            put_le32(bad + offset, (uint32_t)invalid[i].value);
        }
        schway__state_seal(bad, n);
        memcpy(snapshot, target, sizeof *snapshot);
        CHECK(schway_load_state(target, bad, n) == SCHWAY_STATUS_ERR_STATE_FIELD);
        CHECK(memcmp(snapshot, target, sizeof *snapshot) == 0);
    }
    free(good);
    free(bad);
    free(snapshot);
    schway_destroy(source);
    schway_destroy(target);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        return EXIT_FAILURE;
    }
    if (strcmp(argv[1], "midline_dot_irq") == 0) {
        test_midline_dot_irq();
    } else if (strcmp(argv[1], "field_edges") == 0) {
        test_field_edges();
    } else if (strcmp(argv[1], "status_fields") == 0) {
        test_status_fields();
    } else if (strcmp(argv[1], "status_controls") == 0) {
        test_status_controls();
    } else if (strcmp(argv[1], "public_field_wrap") == 0) {
        test_public_field_wrap();
    } else if (strcmp(argv[1], "reset_call_bound") == 0) {
        test_reset_call_bound();
    } else if (strcmp(argv[1], "equal_timer_beam_dma") == 0) {
        test_equal_timer_beam_dma();
    } else if (strcmp(argv[1], "reject_malformed_beam_state") == 0) {
        test_reject_malformed_beam_state();
    } else {
        return EXIT_FAILURE;
    }
    if (failures != 0) {
        fprintf(stderr, "%d GPU beam assertion(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
