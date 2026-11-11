/* API version 4 adds memory spans, memory-card loading and the GPU profile flag. */
#include <stdlib.h>
#include <string.h>

#include "schwaystation.h"
#include "../check.h"

#if SCHWAY_API_VERSION >= 4
static uint8_t v4_card1[SCHWAY_MEMCARD_SIZE];
static uint8_t v4_card2[SCHWAY_MEMCARD_SIZE];
static uint32_t visitor_saw_card_persistent_bytes;

static void *v4_alloc(void *user, size_t size)
{
    (void)user;
    return malloc(size);
}

static void v4_free(void *user, void *ptr, size_t size)
{
    (void)user;
    (void)size;
    free(ptr);
}

static void v4_state_visitor(void *user, const char *name, const void *data,
                             uint32_t count, uint32_t width)
{
    (void)user;
    (void)data;
    (void)count;
    (void)width;
    if (strcmp(name, "card.slot1.bytes") == 0 || strcmp(name, "card.slot1.generation") == 0 ||
        strcmp(name, "card.slot2.bytes") == 0 || strcmp(name, "card.slot2.generation") == 0) {
        visitor_saw_card_persistent_bytes = 1u;
    }
}
#endif

int main(void)
{
    CHECK(SCHWAY_API_VERSION >= 4);
#if SCHWAY_API_VERSION >= 4
    CHECK_EQ_U64(SCHWAY_INPUT_DEVICE_NONE, 0u);
    CHECK_EQ_U64(SCHWAY_INPUT_DEVICE_DIGITAL, 1u);
    CHECK_EQ_U64(SCHWAY_INPUT_DEVICE_DUALSHOCK, 2u);
    schway_config cfg;
    memset(&cfg, 0, sizeof cfg);
    cfg.size = (uint32_t)sizeof cfg;
    cfg.allocator.alloc = v4_alloc;
    cfg.allocator.free = v4_free;
    schway *inst = NULL;
    CHECK_EQ_U64(schway_create(&cfg, &inst), SCHWAY_STATUS_OK);
    if (inst != NULL) {
        CHECK_EQ_U64(SCHWAY_PROFILE_GPU_V0, 4u);
        schway_profile profile = {sizeof profile, SCHWAY_REGION_NTSC_U, SCHWAY_PROFILE_GPU_V0};
        CHECK_EQ_U64(schway_set_profile(inst, &profile), SCHWAY_STATUS_OK);
        schway_span first;
        schway_span second;
        CHECK_EQ_U64(schway_get_vram(inst, &first), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(first.size, 1024u * 512u * sizeof(uint16_t));
        CHECK(first.data != NULL);
        CHECK(((const uint16_t *)first.data)[0] == 0u);
        schway_span ram;
        schway_span scratch;
        schway_span vram;
        schway_span bios;
        schway_span card1;
        schway_span card2;
        CHECK_EQ_U64(schway_get_memory(inst, SCHWAY_MEMORY_RAM, &ram), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(ram.size, 2u * 1024u * 1024u);
        CHECK(ram.data != NULL);
        CHECK_EQ_U64(schway_get_memory(inst, SCHWAY_MEMORY_SCRATCHPAD, &scratch), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(scratch.size, 1024u);
        CHECK(scratch.data != NULL);
        CHECK_EQ_U64(schway_get_memory(inst, SCHWAY_MEMORY_VRAM, &vram), SCHWAY_STATUS_OK);
        CHECK(vram.data == first.data);
        CHECK_EQ_U64(vram.size, first.size);
        CHECK_EQ_U64(schway_get_memory(inst, SCHWAY_MEMORY_BIOS, &bios), SCHWAY_STATUS_OK);
        CHECK(bios.data != NULL);
        CHECK(bios.size != 0u);
        CHECK_EQ_U64(schway_get_memory(inst, SCHWAY_MEMORY_CARD1, &card1), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(card1.size, SCHWAY_MEMCARD_SIZE);
        CHECK_EQ_U64(schway_get_memory(inst, SCHWAY_MEMORY_CARD2, &card2), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(card2.size, SCHWAY_MEMCARD_SIZE);
        for (uint32_t i = 0u; i < SCHWAY_MEMCARD_SIZE; i++) {
            v4_card1[i] = (uint8_t)(i * 3u + 1u);
            v4_card2[i] = (uint8_t)(i * 5u + 2u);
        }
        CHECK_EQ_U64(schway_load_memcard(inst, SCHWAY_CARD_SLOT_1, v4_card1, sizeof v4_card1),
                     SCHWAY_STATUS_OK);
        CHECK_EQ_U64(schway_load_memcard(inst, SCHWAY_CARD_SLOT_2, v4_card2, sizeof v4_card2),
                     SCHWAY_STATUS_OK);
        CHECK(((const uint8_t *)card1.data)[0] == v4_card1[0]);
        CHECK(((const uint8_t *)card2.data)[1] == v4_card2[1]);
        uint64_t generation = UINT64_MAX;
        CHECK_EQ_U64(schway_memcard_generation(inst, SCHWAY_CARD_SLOT_1, &generation),
                     SCHWAY_STATUS_OK);
        CHECK_EQ_U64(generation, 0u);
        size_t state_size = schway_state_size(inst);
        CHECK_EQ_U64(schway_state_visit(inst, v4_state_visitor, NULL), SCHWAY_STATUS_OK);
        CHECK(visitor_saw_card_persistent_bytes == 0u);
        schway_span invalid_span = { (const void *)ram.data, 17u };
        CHECK_EQ_U64(schway_get_memory(inst, 99u, &invalid_span), SCHWAY_STATUS_ERR_ARGUMENT);
        CHECK(invalid_span.data == ram.data);
        CHECK_EQ_U64(invalid_span.size, 17u);
        CHECK_EQ_U64(schway_load_memcard(inst, 0u, v4_card1, sizeof v4_card1),
                     SCHWAY_STATUS_ERR_ARGUMENT);
        CHECK_EQ_U64(schway_load_memcard(inst, SCHWAY_CARD_SLOT_1, v4_card1,
                                         sizeof v4_card1 - 1u), SCHWAY_STATUS_ERR_ARGUMENT);
        CHECK(((const uint8_t *)card1.data)[0] == v4_card1[0]);
        CHECK_EQ_U64(schway_state_size(inst), state_size);
        CHECK_EQ_U64(schway_run_frame(inst, NULL), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(schway_get_vram(inst, &second), SCHWAY_STATUS_OK);
        CHECK(second.data == first.data);
        CHECK_EQ_U64(second.size, first.size);
        uint8_t card_before_refused_import = ((const uint8_t *)card1.data)[0];
        v4_card1[0] ^= 0xFFu;
        CHECK_EQ_U64(schway_load_memcard(inst, SCHWAY_CARD_SLOT_1, v4_card1, sizeof v4_card1),
                     SCHWAY_STATUS_ERR_ARGUMENT);
        CHECK_EQ_U64(((const uint8_t *)card1.data)[0], card_before_refused_import);
        v4_card1[0] ^= 0xFFu;

        schway_input input;
        memset(&input, 0, sizeof input);
        input.size = (uint32_t)sizeof input;
        input.device[0] = SCHWAY_INPUT_DEVICE_DUALSHOCK;
        input.device[1] = SCHWAY_INPUT_DEVICE_NONE;
        input.axes[0][0] = 128u;
        input.axes[0][1] = 128u;
        input.axes[0][2] = 128u;
        input.axes[0][3] = 128u;
        CHECK_EQ_U64(schway_set_input(inst, &input), SCHWAY_STATUS_OK);
        CHECK_EQ_U64(schway_set_input(inst, NULL), SCHWAY_STATUS_ERR_ARGUMENT);

        schway_run_result result;
        memset(&result, 0, sizeof result);
        result.size = (uint32_t)sizeof result;
        CHECK_EQ_U64(schway_run_frame(inst, &result), SCHWAY_STATUS_OK);
        (void)result.lag;
        (void)result.rumble[0];
        (void)result.rumble[1];

        struct old_run_result {
            uint32_t size;
            uint32_t stop;
            uint64_t frame;
            uint64_t ticks;
            uint32_t canary;
        } old_result;
        memset(&old_result, 0, sizeof old_result);
        old_result.size = 24u;
        old_result.canary = 0xC35AA53Cu;
        CHECK_EQ_U64(schway_run_frame(inst, (schway_run_result *)(void *)&old_result),
                     SCHWAY_STATUS_OK);
        CHECK_EQ_HEX(old_result.canary, 0xC35AA53Cu);
        schway_destroy(inst);
    }
#endif
    CHECK_DONE();
}
