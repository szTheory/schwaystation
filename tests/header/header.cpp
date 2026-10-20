// The public header alone, compiled as C++17 with warnings fatal. Compiling
// this file is the test; nothing links it.
#include <type_traits>

#include "schwaystation.h"

// Every boundary struct starts with a uint32_t size tag at offset 0.
#define SIZE_FIRST(T)                                                                              \
    static_assert(offsetof(T, size) == 0 && std::is_same<decltype(T::size), uint32_t>::value,    \
                  #T " starts with uint32_t size")
SIZE_FIRST(schway_config);
SIZE_FIRST(schway_versions);
SIZE_FIRST(schway_profile);
SIZE_FIRST(schway_info);
SIZE_FIRST(schway_run_result);
SIZE_FIRST(schway_video);
SIZE_FIRST(schway_audio);
static_assert(sizeof(schway_allocator) >= 3 * sizeof(void *), "allocator holds three pointers");
static_assert(sizeof(schway_status) == 4, "status is 32 bits");
static_assert(SCHWAY_API_VERSION == 1, "API version");

// Each function keeps its declared signature, with C linkage.
static schway_status (*const get_versions_fn)(schway_versions *) = schway_get_versions;
static schway_status (*const create_fn)(const schway_config *, schway **) = schway_create;
static void (*const destroy_fn)(schway *) = schway_destroy;
static schway_status (*const set_profile_fn)(schway *, const schway_profile *) = schway_set_profile;
static schway_status (*const reset_fn)(schway *, uint32_t) = schway_reset;
static schway_status (*const run_frame_fn)(schway *, schway_run_result *) = schway_run_frame;
static schway_status (*const get_info_fn)(const schway *, schway_info *) = schway_get_info;
static schway_status (*const get_video_fn)(const schway *, schway_video *) = schway_get_video;
static schway_status (*const get_audio_fn)(schway *, schway_audio *) = schway_get_audio;
static size_t (*const state_size_fn)(const schway *) = schway_state_size;
static schway_status (*const save_state_fn)(const schway *, void *, size_t) = schway_save_state;
static schway_status (*const load_state_fn)(schway *, const void *, size_t) = schway_load_state;
static schway_status (*const state_visit_fn)(const schway *, schway_state_visitor,
                                             void *) = schway_state_visit;
static const char *(*const status_string_fn)(schway_status) = schway_status_string;
static_assert(std::is_same<schway_state_visitor,
                           void (*)(void *, const char *, const void *, uint32_t, uint32_t)>::value,
              "visitor signature");

int header_check(schway_status st);

int header_check(schway_status st)
{
    // A C++ caller zeroes a struct with empty braces, then sets its size.
    schway_video io = {};
    io.size = static_cast<uint32_t>(sizeof io);
    (void)get_versions_fn;
    (void)create_fn;
    (void)destroy_fn;
    (void)set_profile_fn;
    (void)reset_fn;
    (void)run_frame_fn;
    (void)get_info_fn;
    (void)get_video_fn;
    (void)get_audio_fn;
    (void)state_size_fn;
    (void)save_state_fn;
    (void)load_state_fn;
    (void)state_visit_fn;
    (void)status_string_fn;
    // A status compares against the constants without a sign or enum warning.
    return (st == SCHWAY_STATUS_OK || st == SCHWAY_STATUS_ERR_BUFFER_TOO_SMALL) &&
           io.reserved0 == 0u;
}
