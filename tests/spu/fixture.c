/* Full-stream comparison between the guest SPU path and a CPU-free replay. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/internal.h"
#include "schwaystation.h"

#define FIXTURE_HOST_FRAMES 601u /* guest VBlank 600 completes on host frame 601 */
#define MAX_PAIRS (FIXTURE_HOST_FRAMES * 887u)
#define RING_PAIRS (4u * 887u)
#define KEYON_QUANTUM 256u
#define KEYON_LIMIT_CYCLES UINT64_C(10000000)
#define SPU_REGISTER_BASE UINT32_C(0x1F801C00)
#define SPU_KON UINT32_C(0x1F801D88)

struct spu_event {
    uint32_t address;
    uint16_t value;
};

struct capture {
    uint64_t frame_end_ticks[FIXTURE_HOST_FRAMES];
    uint64_t kon_tick;
    int16_t first_sample[2];
    int32_t first_envelope[2];
    uint32_t frame_count;
    uint32_t pairs;
    uint32_t error_stage;
};

struct region_case {
    uint32_t region;
    const char *name;
};

static uint32_t failures;
static uint32_t checks;
static int16_t guest_pcm[MAX_PAIRS * 2u];
static int16_t cpu_free_pcm[MAX_PAIRS * 2u];
static int16_t drain_pcm[RING_PAIRS * 2u];

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

static void expect(uint32_t condition, const char *message)
{
    checks++;
    (void)printf("%s %u - %s\n", condition != 0u ? "ok" : "not ok", checks, message);
    failures += condition == 0u ? 1u : 0u;
}

static schway *make_instance(uint32_t region)
{
    schway_config config;
    schway_profile profile;
    schway *inst = NULL;
    memset(&config, 0, sizeof config);
    config.size = (uint32_t)sizeof config;
    config.allocator.alloc = plain_alloc;
    config.allocator.free = plain_free;
    if (schway_create(&config, &inst) != SCHWAY_STATUS_OK || inst == NULL) {
        return NULL;
    }
    memset(&profile, 0, sizeof profile);
    profile.size = (uint32_t)sizeof profile;
    profile.region = region;
    if (schway_set_profile(inst, &profile) != SCHWAY_STATUS_OK) {
        schway_destroy(inst);
        return NULL;
    }
    return inst;
}

static uint32_t append_audio(schway *inst, int16_t *pcm, uint32_t *pairs)
{
    schway_audio audio;
    memset(drain_pcm, 0x5A, sizeof drain_pcm);
    memset(&audio, 0, sizeof audio);
    audio.size = (uint32_t)sizeof audio;
    audio.samples = drain_pcm;
    audio.capacity = RING_PAIRS;
    if (schway_get_audio(inst, &audio) != SCHWAY_STATUS_OK || audio.dropped != 0u ||
        *pairs > MAX_PAIRS || audio.written > MAX_PAIRS - *pairs) {
        return 1u;
    }
    memcpy(pcm + (size_t)*pairs * 2u, drain_pcm,
           (size_t)audio.written * 2u * sizeof(int16_t));
    *pairs += audio.written;
    return 0u;
}

static uint32_t parse_brr_table(const char *path, uint8_t blocks[2][16])
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return 1u;
    }
    char line[256];
    uint32_t count = 0u;
    while (fgets(line, sizeof line, file) != NULL) {
        char *text = line;
        while (*text == ' ' || *text == '\t') {
            text++;
        }
        if (strncmp(text, "spu_brr_block ", 14u) != 0) {
            continue;
        }
        int flags = 0;
        int seed = 0;
        if (count >= 2u || sscanf(text, "spu_brr_block %i, %i", &flags, &seed) != 2 ||
            flags < 0 || flags > UINT8_MAX || seed < 0 || seed > UINT8_MAX) {
            (void)fclose(file);
            return 1u;
        }
        blocks[count][0] = 0x04u;
        blocks[count][1] = (uint8_t)flags;
        for (uint32_t byte = 0u; byte < 14u; byte++) {
            uint8_t high = (uint8_t)((uint32_t)seed + 2u * byte) & 0x0Fu;
            uint8_t low = (uint8_t)((uint32_t)seed + 2u * byte + 1u) & 0x0Fu;
            blocks[count][byte + 2u] = (uint8_t)((high << 4) | low);
        }
        count++;
    }
    (void)fclose(file);
    return count == 2u ? 0u : 1u;
}

static uint32_t parse_event_table(const char *path, struct spu_event *events,
                                  uint32_t capacity, uint32_t *event_count,
                                  uint16_t *key_on)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return 1u;
    }
    char line[256];
    uint32_t count = 0u;
    uint32_t kon_count = 0u;
    uint32_t pending_t0 = 0u;
    uint32_t has_t0 = 0u;
    while (fgets(line, sizeof line, file) != NULL) {
        int immediate = 0;
        if (sscanf(line, " li t0, %i", &immediate) == 1) {
            pending_t0 = (uint32_t)immediate;
            has_t0 = 1u;
        }

        char mnemonic[16] = {0};
        char source[16] = {0};
        char base[16] = {0};
        unsigned offset = 0u;
        if (sscanf(line, " %15s %15[^,], %x(%15[^)])", mnemonic, source, &offset, base) != 4 ||
            strcmp(mnemonic, "sh") != 0 || strcmp(base, "s1") != 0 || offset >= 0x200u) {
            continue;
        }

        uint32_t value = 0u;
        if (strcmp(source, "zero") == 0) {
            value = 0u;
        } else if (strcmp(source, "t0") == 0 && has_t0 != 0u) {
            value = pending_t0;
        } else {
            continue;
        }
        uint32_t address = SPU_REGISTER_BASE + offset;
        if (address == SPU_KON) {
            *key_on = (uint16_t)value;
            kon_count++;
            continue;
        }
        if (count >= capacity || value > UINT16_MAX) {
            (void)fclose(file);
            return 1u;
        }
        events[count].address = address;
        events[count].value = (uint16_t)value;
        count++;
    }
    (void)fclose(file);
    *event_count = count;
    return count != 0u && kon_count == 1u && *key_on == 3u ? 0u : 1u;
}

static int32_t reference_sar32(int32_t value, uint32_t shift)
{
    if (shift == 0u) {
        return value;
    }
    int64_t divisor = INT64_C(1) << shift;
    int64_t wide = value;
    if (wide >= 0) {
        return (int32_t)(wide / divisor);
    }
    return (int32_t)(-(((-wide) + divisor - 1) / divisor));
}

static int16_t reference_first_brr_sample(const uint8_t block[16])
{
    uint32_t shift = block[0] & 0x0Fu;
    uint32_t filter = (block[0] >> 4) & 7u;
    int32_t nibble = (int32_t)(block[2] & 0x0Fu);
    if (nibble >= 8) {
        nibble -= 16;
    }
    if (shift >= 13u) {
        shift = 9u;
    }
    /* Both authored blocks use filter zero, so reset predictor history is silent. */
    if (filter != 0u) {
        return 0;
    }
    return (int16_t)reference_sar32(nibble * 4096, shift);
}

static int32_t reference_first_attack(uint16_t adsr1)
{
    uint32_t shift = (adsr1 >> 10) & 0x1Fu;
    uint32_t step_code = (adsr1 >> 8) & 3u;
    int32_t step = (int32_t)(7u - step_code);
    if (shift < 11u) {
        step *= (int32_t)(UINT32_C(1) << (11u - shift));
    }
    return step > 0x7FFF ? 0x7FFF : step;
}

static uint32_t read_exe(const char *path, uint8_t **bytes, size_t *size)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL || fseek(file, 0, SEEK_END) != 0) {
        if (file != NULL) {
            (void)fclose(file);
        }
        return 1u;
    }
    long length = ftell(file);
    if (length <= 0 || fseek(file, 0, SEEK_SET) != 0) {
        (void)fclose(file);
        return 1u;
    }
    *bytes = (uint8_t *)malloc((size_t)length);
    if (*bytes == NULL || fread(*bytes, 1u, (size_t)length, file) != (size_t)length) {
        free(*bytes);
        *bytes = NULL;
        (void)fclose(file);
        return 1u;
    }
    *size = (size_t)length;
    (void)fclose(file);
    return 0u;
}

static uint32_t append_guest_chunk(schway *inst, struct capture *capture)
{
    return append_audio(inst, guest_pcm, &capture->pairs);
}

static uint32_t capture_guest(const char *exe_path, uint32_t region,
                              const uint8_t blocks[2][16], struct capture *capture)
{
    memset(capture, 0, sizeof *capture);
    uint8_t *exe = NULL;
    size_t exe_size = 0u;
    if (read_exe(exe_path, &exe, &exe_size) != 0u) {
        capture->error_stage = 1u;
        return 1u;
    }
    schway *inst = make_instance(region);
    if (inst == NULL || schway_load_exe(inst, exe, exe_size) != SCHWAY_STATUS_OK) {
        free(exe);
        schway_destroy(inst);
        capture->error_stage = 2u;
        return 1u;
    }
    free(exe);

    uint32_t found_keyon = 0u;
    uint64_t budget_calls = KEYON_LIMIT_CYCLES / KEYON_QUANTUM;
    while (budget_calls-- != 0u && inst->spu.voice[0].envelope_phase == 0u) {
        schway_budget budget;
        schway_run_result result;
        memset(&budget, 0, sizeof budget);
        budget.size = (uint32_t)sizeof budget;
        budget.max_cycles = KEYON_QUANTUM;
        memset(&result, 0, sizeof result);
        result.size = (uint32_t)sizeof result;
        if (schway_run(inst, &budget, &result) != SCHWAY_STATUS_OK ||
            result.stop == SCHWAY_STOP_FAULT || inst->fault.kind != SCHWAY__FAULT_NONE ||
            append_guest_chunk(inst, capture) != 0u) {
            capture->error_stage = 3u;
            schway_destroy(inst);
            return 1u;
        }
        if (inst->spu.voice[0].envelope_phase != 0u) {
            if (inst->spu.next_tick < SCHWAY__CYCLES_PER_PAIR) {
                capture->error_stage = 4u;
                schway_destroy(inst);
                return 1u;
            }
            capture->kon_tick = inst->spu.next_tick - SCHWAY__CYCLES_PER_PAIR;
            capture->first_sample[0] = inst->spu.voice[0].sample;
            capture->first_sample[1] = inst->spu.voice[1].sample;
            capture->first_envelope[0] = inst->spu.voice[0].envelope;
            capture->first_envelope[1] = inst->spu.voice[1].envelope;
            found_keyon = 1u;
        }
    }
    if (found_keyon == 0u) {
        capture->error_stage = 5u;
        schway_destroy(inst);
        return 1u;
    }

    while (capture->frame_count < FIXTURE_HOST_FRAMES) {
        schway_run_result result;
        memset(&result, 0, sizeof result);
        result.size = (uint32_t)sizeof result;
        if (schway_run_frame(inst, &result) != SCHWAY_STATUS_OK ||
            result.stop != SCHWAY_STOP_FRAME || inst->fault.kind != SCHWAY__FAULT_NONE ||
            append_guest_chunk(inst, capture) != 0u) {
            capture->error_stage = 6u;
            schway_destroy(inst);
            return 1u;
        }
        capture->frame_end_ticks[capture->frame_count++] = result.ticks;
        if (inst->bus.ram[0x10008u] == 0u) {
            break;
        }
    }
    uint32_t valid = capture->frame_count != 0u &&
                     capture->frame_count <= FIXTURE_HOST_FRAMES &&
                     inst->bus.ram[0x10008u] == 0u && inst->bus.ram[0x10009u] == 1u &&
                     inst->bus.ram[0x1000Au] == 3u && capture->pairs != 0u;
    uint32_t table_matches = 1u;
    for (uint32_t copy = 0u; copy < 2u; copy++) {
        uint32_t copy_base = copy == 0u ? 0x1000u : 0x2000u;
        for (uint32_t block = 0u; block < 2u; block++) {
            uint32_t block_base = copy_base + block * 16u;
            for (uint32_t word = 0u; word < 8u; word++) {
                uint32_t actual = schway__spu_ram_peek(inst, block_base + word * 2u);
                uint32_t expected = (uint32_t)blocks[block][word * 2u] |
                                    ((uint32_t)blocks[block][word * 2u + 1u] << 8);
                if (actual != expected) {
                    (void)printf("# table region=%u copy=%u block=%u word=%u actual=%04X expected=%04X\n",
                                 region, copy, block, word, actual, expected);
                }
                table_matches &= actual == expected;
            }
        }
    }
    valid &= table_matches;
    if (valid == 0u) {
        (void)printf("# capture region=%u host-frames=%u mailbox=%u version=%u records=%u pairs=%u table=%u\n",
                     region, capture->frame_count, inst->bus.ram[0x10008u],
                     inst->bus.ram[0x10009u], inst->bus.ram[0x1000Au],
                     capture->pairs, table_matches);
        capture->error_stage = 7u;
    }
    schway_destroy(inst);
    return valid != 0u ? 0u : 1u;
}

static uint32_t replay_cpu_free(uint32_t region, const uint8_t blocks[2][16],
                                const struct spu_event *events, uint32_t event_count,
                                uint16_t key_on, const struct capture *capture,
                                uint32_t *pair_count)
{
    schway *inst = make_instance(region);
    if (inst == NULL) {
        return 1u;
    }
    for (uint32_t copy = 0u; copy < 2u; copy++) {
        uint32_t copy_base = copy == 0u ? 0x1000u : 0x2000u;
        for (uint32_t block = 0u; block < 2u; block++) {
            uint32_t block_base = copy_base + block * 16u;
            for (uint32_t word = 0u; word < 8u; word++) {
                uint16_t value = (uint16_t)((uint16_t)blocks[block][word * 2u] |
                                            ((uint16_t)blocks[block][word * 2u + 1u] << 8));
                schway__spu_ram_write(inst, block_base + word * 2u, value);
            }
        }
    }
    for (uint32_t event = 0u; event < event_count; event++) {
        schway__spu_reg_write(inst, events[event].address, events[event].value);
    }

    *pair_count = 0u;
    uint32_t keyon_queued = 0u;
    for (uint32_t frame = 0u; frame < capture->frame_count; frame++) {
        uint64_t frame_end = capture->frame_end_ticks[frame];
        if (frame != 0u && frame_end <= capture->frame_end_ticks[frame - 1u]) {
            schway_destroy(inst);
            return 1u;
        }
        while (schway__spu_next(inst) <= frame_end) {
            uint64_t deadline = schway__spu_next(inst);
            if (deadline == capture->kon_tick && keyon_queued == 0u) {
                schway__spu_reg_write(inst, SPU_KON, key_on);
                keyon_queued = 1u;
            }
            inst->ticks = deadline;
            schway__spu_event(inst, deadline);
        }
        if (append_audio(inst, cpu_free_pcm, pair_count) != 0u) {
            schway_destroy(inst);
            return 1u;
        }
    }
    schway_destroy(inst);
    return keyon_queued != 0u ? 0u : 1u;
}

static uint32_t file_exists(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return 0u;
    }
    (void)fclose(file);
    return 1u;
}

int main(int argc, char **argv)
{
    static const struct region_case regions[] = {
        {SCHWAY_REGION_NTSC_J, "NTSC-J"},
        {SCHWAY_REGION_NTSC_U, "NTSC-U"},
        {SCHWAY_REGION_PAL, "PAL"},
    };
    uint8_t blocks[2][16] = {{0u}};
    struct spu_event events[128];
    uint32_t event_count = 0u;
    uint16_t key_on = 0u;

    (void)printf("TAP version 13\n");
    if (argc != 5) {
        (void)fprintf(stderr,
                      "usage: spu-fixture spu_tone.exe spu_blocks.inc spu_tone.asm schwaystation-run\n");
        return 2;
    }
    expect(file_exists(argv[1]), "the built tone fixture is available");
    expect(parse_brr_table(argv[2], blocks) == 0u,
           "the two authored BRR blocks are read from their source table");
    expect(parse_event_table(argv[3], events, 128u, &event_count, &key_on) == 0u,
           "the authored SPU register and key-on event table is read from the guest source");
    expect(file_exists(argv[4]), "the built runner is available");
    expect(blocks[0][0] == 0x04u && blocks[0][1] == 0x07u &&
               blocks[1][0] == 0x04u && blocks[1][1] == 0x07u,
           "the source reader preserves both 16-byte BRR headers");

    for (uint32_t i = 0u; i < (uint32_t)(sizeof regions / sizeof regions[0]); i++) {
        struct capture capture;
        uint32_t captured = capture_guest(argv[1], regions[i].region, blocks, &capture) == 0u;
        char label[128];
        (void)snprintf(label, sizeof label, "%s guest reaches mailbox frame 600",
                       regions[i].name);
        expect(captured, label);
        if (captured == 0u) {
            (void)printf("# %s capture stage=%u host-frames=%u pairs=%u kon=%llu\n",
                         regions[i].name, capture.error_stage, capture.frame_count,
                         capture.pairs, (unsigned long long)capture.kon_tick);
            continue;
        }

        int16_t expected_sample0 = reference_first_brr_sample(blocks[0]);
        int16_t expected_sample1 = reference_first_brr_sample(blocks[1]);
        if (capture.first_sample[0] != expected_sample0 ||
            capture.first_sample[1] != expected_sample1) {
            (void)printf("# %s first BRR sample actual=(%d,%d) expected=(%d,%d)\n",
                         regions[i].name, capture.first_sample[0], capture.first_sample[1],
                         expected_sample0, expected_sample1);
        }
        (void)snprintf(label, sizeof label, "%s first BRR samples match independent scalar decoding",
                       regions[i].name);
        expect(capture.first_sample[0] == expected_sample0 &&
                   capture.first_sample[1] == expected_sample1,
               label);

        int32_t expected_attack = reference_first_attack(0x000Fu);
        (void)snprintf(label, sizeof label, "%s first ENVX steps match independent scalar ADSR",
                       regions[i].name);
        expect(capture.first_envelope[0] == expected_attack &&
                   capture.first_envelope[1] == expected_attack,
               label);

        uint32_t replay_pairs = 0u;
        uint32_t replayed = replay_cpu_free(regions[i].region, blocks, events, event_count,
                                            key_on, &capture, &replay_pairs) == 0u;
        (void)snprintf(label, sizeof label, "%s CPU-free event loop completes from reset",
                       regions[i].name);
        expect(replayed, label);
        uint32_t equal = replayed != 0u && capture.pairs == replay_pairs &&
                         memcmp(guest_pcm, cpu_free_pcm,
                                (size_t)capture.pairs * 2u * sizeof(int16_t)) == 0;
        if (equal == 0u && replayed != 0u) {
            uint32_t limit = capture.pairs < replay_pairs ? capture.pairs : replay_pairs;
            uint32_t first = 0u;
            while (first < limit && guest_pcm[first * 2u] == cpu_free_pcm[first * 2u] &&
                   guest_pcm[first * 2u + 1u] == cpu_free_pcm[first * 2u + 1u]) {
                first++;
            }
            (void)printf("# %s pairs guest=%u cpu-free=%u first-difference=%u kon-tick=%llu\n",
                         regions[i].name, capture.pairs, replay_pairs, first,
                         (unsigned long long)capture.kon_tick);
        }
        (void)snprintf(label, sizeof label, "%s complete CPU-free PCM equals every guest pair",
                       regions[i].name);
        expect(equal, label);
    }

    (void)printf("1..%u\n", checks);
    return failures == 0u ? 0 : 1;
}
