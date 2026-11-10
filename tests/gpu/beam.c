/* GPU beam tests run through the public time budget and real register bus. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"

#define GP1 UINT32_C(0x1F801814)
#define TIMER0_COUNT UINT32_C(0x1F801100)
#define TIMER0_MODE UINT32_C(0x1F801104)
#define TIMER0_TARGET UINT32_C(0x1F801108)
#define I_STAT UINT32_C(0x1F801070)

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

int main(int argc, char **argv)
{
    if (argc != 2 || strcmp(argv[1], "midline_dot_irq") != 0) {
        return EXIT_FAILURE;
    }
    test_midline_dot_irq();
    if (failures != 0) {
        fprintf(stderr, "%d GPU beam assertion(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
