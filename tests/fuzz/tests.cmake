# Every lane registers replay binaries; the fuzz preset (SCHWAYSTATION_FUZZ,
# clang-21) also builds the libFuzzer campaigns, which register no tests.
option(SCHWAYSTATION_FUZZ "Build libFuzzer campaigns" OFF)
set(fuzz_dir ${CMAKE_CURRENT_LIST_DIR})

# schwaystation-fuzz-exe-replay runs exe.c's invariants over the in-memory
# seeds and every committed regression input, with no fuzzing engine, so it
# builds on every lane; --write DIR writes the seeds as files for the engine.
add_executable(schwaystation-fuzz-exe-replay ${fuzz_dir}/exe.c ${fuzz_dir}/seeds.c)
target_compile_definitions(schwaystation-fuzz-exe-replay PRIVATE SCHWAY_FUZZ_REPLAY)
target_include_directories(schwaystation-fuzz-exe-replay PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-fuzz-exe-replay PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-fuzz-exe-replay PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-fuzz-exe-replay)

# Committed regressions: each a minimised input that once broke an invariant,
# as hexadecimal text. The directory appears with the first one; the replay
# takes files, not directories, so they are listed here.
file(GLOB fuzz_regress CONFIGURE_DEPENDS ${fuzz_dir}/regress/*.hex)
list(SORT fuzz_regress)

# Fault prevented: a loader that accepts a header the probe refuses, changes
# RAM outside the text segment and the launch block, or leaves a hostile
# executable running past two frames. The replay gates on every leg, including
# be32 and wasm32; only the nightly libFuzzer campaign reports without gating.
schway_add_test(NAME fuzz.regress.exe
  COMMAND schwaystation-fuzz-exe-replay ${fuzz_regress}
  LABELS fuzz portable TIMEOUT 60)

# Each subsystem replay uses the existing replay and seed interfaces, with a
# compile-time corpus selected for that subsystem's real operations.
foreach(subsystem IN ITEMS gpu sio memcard spu)
  add_executable(schwaystation-fuzz-${subsystem}-replay
    ${fuzz_dir}/${subsystem}.c ${fuzz_dir}/seeds.c)
  string(TOUPPER ${subsystem} seed_mode)
  target_compile_definitions(schwaystation-fuzz-${subsystem}-replay PRIVATE
    SCHWAY_FUZZ_REPLAY SCHWAY_FUZZ_${seed_mode}_SEEDS)
  target_include_directories(schwaystation-fuzz-${subsystem}-replay PRIVATE
    ${PROJECT_SOURCE_DIR}/src)
  target_link_libraries(schwaystation-fuzz-${subsystem}-replay PRIVATE schwaystation::schwaystation)
  set_target_properties(schwaystation-fuzz-${subsystem}-replay PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
  schway_warnings(schwaystation-fuzz-${subsystem}-replay)
  # Fault prevented: a hostile subsystem stream escapes its state or time bounds.
  schway_add_test(NAME fuzz.regress.${subsystem}
    COMMAND schwaystation-fuzz-${subsystem}-replay
    LABELS fuzz portable TIMEOUT 60)
endforeach()

# The new hostile-media targets call the actual host parsers or core devices.
# CUE and M3U receive build-owned working directories containing only their
# small, fixed media child, so fuzzed paths cannot name arbitrary host files.
foreach(subsystem IN ITEMS cue cnf m3u cdrom mdec)
  string(TOUPPER ${subsystem} seed_mode)
  set(replay_target schwaystation-fuzz-${subsystem}-replay)
  add_executable(${replay_target} ${fuzz_dir}/${subsystem}.c ${fuzz_dir}/seeds.c)
  target_compile_definitions(${replay_target} PRIVATE
    SCHWAY_FUZZ_REPLAY SCHWAY_FUZZ_${seed_mode}_SEEDS)
  set_target_properties(${replay_target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
  if(subsystem STREQUAL "cue" OR subsystem STREQUAL "m3u")
    set(work_dir "${PROJECT_BINARY_DIR}/tests/fuzz-work/${subsystem}")
    file(MAKE_DIRECTORY "${work_dir}")
    set(guest_work_dir "${work_dir}")
    if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
      set(guest_work_dir "/tests/fuzz-work/${subsystem}")
    endif()
    target_compile_definitions(${replay_target} PRIVATE
      "SCHWAY_FUZZ_WORK_DIR=\"${guest_work_dir}\"")
    target_link_libraries(${replay_target} PRIVATE schwaystation_disc_image)
  elseif(subsystem STREQUAL "cnf")
    target_include_directories(${replay_target} PRIVATE ${PROJECT_SOURCE_DIR}/host)
    target_link_libraries(${replay_target} PRIVATE schwaystation_disc_image)
  else()
    target_include_directories(${replay_target} PRIVATE ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(${replay_target} PRIVATE schwaystation::schwaystation)
  endif()
  schway_warnings(${replay_target})
  # Fault prevented: hostile media commands or paths escape their bounded parser/device state.
  schway_add_test(NAME fuzz.regress.${subsystem}
    COMMAND ${replay_target}
    LABELS fuzz portable TIMEOUT 60)
endforeach()

if(SCHWAYSTATION_FUZZ)
  # The libFuzzer binary keeps libFuzzer's main. The fuzz preset builds every
  # object with ASan, UBSan and coverage instrumentation, so the core's own
  # branches guide the fuzzer; only this target links the engine.
  add_executable(schwaystation-fuzz-exe ${fuzz_dir}/exe.c ${fuzz_dir}/seeds.c)
  target_include_directories(schwaystation-fuzz-exe PRIVATE ${PROJECT_SOURCE_DIR}/src)
  target_link_libraries(schwaystation-fuzz-exe PRIVATE schwaystation::schwaystation)
  target_compile_options(schwaystation-fuzz-exe PRIVATE -fsanitize=fuzzer)
  target_link_options(schwaystation-fuzz-exe PRIVATE -fsanitize=fuzzer)
  set_target_properties(schwaystation-fuzz-exe PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
  schway_warnings(schwaystation-fuzz-exe)

  add_executable(schwaystation-fuzz-spu ${fuzz_dir}/spu.c)
  target_include_directories(schwaystation-fuzz-spu PRIVATE ${PROJECT_SOURCE_DIR}/src)
  target_link_libraries(schwaystation-fuzz-spu PRIVATE schwaystation::schwaystation)
  target_compile_options(schwaystation-fuzz-spu PRIVATE -fsanitize=fuzzer)
  target_link_options(schwaystation-fuzz-spu PRIVATE -fsanitize=fuzzer)
  set_target_properties(schwaystation-fuzz-spu PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
  schway_warnings(schwaystation-fuzz-spu)

  foreach(subsystem IN ITEMS cue cnf m3u cdrom mdec)
    set(target schwaystation-fuzz-${subsystem})
    add_executable(${target} ${fuzz_dir}/${subsystem}.c)
    if(subsystem STREQUAL "cue" OR subsystem STREQUAL "m3u")
      set(work_dir "${PROJECT_BINARY_DIR}/tests/fuzz-work/${subsystem}")
      file(MAKE_DIRECTORY "${work_dir}")
      set(guest_work_dir "${work_dir}")
      if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
        set(guest_work_dir "/tests/fuzz-work/${subsystem}")
      endif()
      target_compile_definitions(${target} PRIVATE
        "SCHWAY_FUZZ_WORK_DIR=\"${guest_work_dir}\"")
      target_link_libraries(${target} PRIVATE schwaystation_disc_image)
    elseif(subsystem STREQUAL "cnf")
      target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/host)
      target_link_libraries(${target} PRIVATE schwaystation_disc_image)
    else()
      target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/src)
      target_link_libraries(${target} PRIVATE schwaystation::schwaystation)
    endif()
    target_compile_options(${target} PRIVATE -fsanitize=fuzzer)
    target_link_options(${target} PRIVATE -fsanitize=fuzzer)
    set_target_properties(${target} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
    schway_warnings(${target})
  endforeach()
endif()
