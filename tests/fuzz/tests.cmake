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

# CHD replay calls the production parser in a fixed build-owned directory.
# The enabled lane uses the generated fixture as a known-good input and keeps
# its required fixture setup explicit; disabled builds assert unsupported.
set(chd_work_dir "${PROJECT_BINARY_DIR}/tests/fuzz-work/chd")
file(MAKE_DIRECTORY "${chd_work_dir}")
set(chd_guest_work_dir "${chd_work_dir}")
set(chd_fixture "${PROJECT_BINARY_DIR}/fixtures/disc_boot.chd")
set(chd_guest_fixture "${chd_fixture}")
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  set(chd_guest_work_dir "/tests/fuzz-work/chd")
  set(chd_guest_fixture "/fixtures/disc_boot.chd")
endif()

add_executable(schwaystation-fuzz-chd-replay ${fuzz_dir}/chd.c)
target_compile_definitions(schwaystation-fuzz-chd-replay PRIVATE
  SCHWAY_FUZZ_REPLAY SCHWAY_FUZZ_CHD_SEEDS
  "SCHWAYSTATION_CHD=$<BOOL:${SCHWAYSTATION_CHD}>"
  "SCHWAY_FUZZ_WORK_DIR=\"${chd_guest_work_dir}\"")
if(SCHWAYSTATION_CHD)
  target_compile_definitions(schwaystation-fuzz-chd-replay PRIVATE
    "SCHWAY_FUZZ_CHD_FIXTURE=\"${chd_guest_fixture}\"")
endif()
target_include_directories(schwaystation-fuzz-chd-replay PRIVATE ${PROJECT_SOURCE_DIR}/host)
target_link_libraries(schwaystation-fuzz-chd-replay PRIVATE schwaystation_disc_image)
set_target_properties(schwaystation-fuzz-chd-replay PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-fuzz-chd-replay)
if(SCHWAYSTATION_CHD)
  schway_add_test(NAME fuzz.regress.chd
    COMMAND schwaystation-fuzz-chd-replay "${chd_fixture}"
    LABELS fuzz portable TIMEOUT 60)
  set_tests_properties(fuzz.regress.chd PROPERTIES FIXTURES_REQUIRED disc_media_preflight)
else()
  schway_add_test(NAME fuzz.regress.chd
    COMMAND schwaystation-fuzz-chd-replay
    LABELS fuzz portable TIMEOUT 60)
endif()

set(chd_seed_corpus "${PROJECT_BINARY_DIR}/tests/fuzz-work/chd-corpus")
file(MAKE_DIRECTORY "${chd_seed_corpus}")
schway_add_test(NAME fuzz.regress.chd.seed_corpus
  COMMAND schwaystation-fuzz-chd-replay --write "${chd_seed_corpus}"
  LABELS fuzz portable TIMEOUT 60)
if(SCHWAYSTATION_CHD)
  set_tests_properties(fuzz.regress.chd.seed_corpus PROPERTIES
    FIXTURES_REQUIRED disc_media_preflight)
endif()

# The new hostile-media targets call the actual host parsers or core devices.
# CUE and M3U receive build-owned working directories containing only their
# small, fixed media child, so fuzzed paths cannot name arbitrary host files.
foreach(subsystem IN ITEMS cue cnf m3u cdrom mdec sbi)
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
  elseif(subsystem STREQUAL "cnf" OR subsystem STREQUAL "sbi")
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

# PBP mutation scripts target two build-owned accepted images, so the replay
# and sanitizer exercise production block decoding without storing disc bytes.
set(pbp_work_dir "${PROJECT_BINARY_DIR}/tests/fuzz-work/pbp")
file(MAKE_DIRECTORY "${pbp_work_dir}")
set(pbp_guest_work_dir "${pbp_work_dir}")
set(pbp_fixture "${PROJECT_BINARY_DIR}/fixtures/schwaystation-disc_boot.pbp")
set(pbp_guest_fixture "${pbp_fixture}")
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  set(pbp_guest_work_dir "/tests/fuzz-work/pbp")
  set(pbp_guest_fixture "/fixtures/schwaystation-disc_boot.pbp")
endif()
add_executable(schwaystation-fuzz-pbp-replay ${fuzz_dir}/pbp.c)
target_compile_definitions(schwaystation-fuzz-pbp-replay PRIVATE
  SCHWAY_FUZZ_REPLAY SCHWAY_FUZZ_PBP_SEEDS
  "SCHWAY_FUZZ_WORK_DIR=\"${pbp_guest_work_dir}\""
  "SCHWAY_FUZZ_PBP_FIXTURE=\"${pbp_guest_fixture}\"")
target_include_directories(schwaystation-fuzz-pbp-replay PRIVATE ${PROJECT_SOURCE_DIR}/host)
target_link_libraries(schwaystation-fuzz-pbp-replay PRIVATE schwaystation_disc_image)
set_target_properties(schwaystation-fuzz-pbp-replay PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-fuzz-pbp-replay)
# Fault prevented: hostile PSISO metadata or streams publish a bad reader.
schway_add_test(NAME fuzz.regress.pbp
  COMMAND schwaystation-fuzz-pbp-replay
  LABELS fuzz portable TIMEOUT 60)
set_tests_properties(fuzz.regress.pbp PROPERTIES FIXTURES_REQUIRED disc_pbp_preflight)

# ECM and CloneCD replay the generated, owned legacy fixtures through the
# production parser and dispatcher on every portable lane.
foreach(subsystem IN ITEMS ecm ccd)
  string(TOUPPER ${subsystem} seed_mode)
  set(replay_target schwaystation-fuzz-${subsystem}-replay)
  set(work_dir "${PROJECT_BINARY_DIR}/tests/fuzz-work/${subsystem}")
  file(MAKE_DIRECTORY "${work_dir}" "${work_dir}/candidate")
  set(guest_work_dir "${work_dir}")
  if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
    set(guest_work_dir "/tests/fuzz-work/${subsystem}")
  endif()
  if(subsystem STREQUAL "ecm")
    set(ecm_guest_work_dir "${guest_work_dir}")
  else()
    set(ccd_guest_work_dir "${guest_work_dir}")
  endif()
  add_executable(${replay_target} ${fuzz_dir}/${subsystem}.c)
  target_compile_definitions(${replay_target} PRIVATE
    SCHWAY_FUZZ_REPLAY SCHWAY_FUZZ_${seed_mode}_SEEDS
    "SCHWAY_FUZZ_WORK_DIR=\"${guest_work_dir}\"")
  target_include_directories(${replay_target} PRIVATE ${PROJECT_SOURCE_DIR}/host)
  target_link_libraries(${replay_target} PRIVATE schwaystation_disc_image)
  add_dependencies(${replay_target} schwaystation-legacy-disc-fixtures)
  if(subsystem STREQUAL "ecm")
    set(literal_fixture "${PROJECT_BINARY_DIR}/fixtures/disc_boot_legacy_literal.ecm")
    set(modeled_fixture "${PROJECT_BINARY_DIR}/fixtures/disc_boot_legacy_modeled.ecm")
    set(guest_literal_fixture "${literal_fixture}")
    set(guest_modeled_fixture "${modeled_fixture}")
    if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
      set(guest_literal_fixture "/fixtures/disc_boot_legacy_literal.ecm")
      set(guest_modeled_fixture "/fixtures/disc_boot_legacy_modeled.ecm")
    endif()
    set(ecm_guest_literal_fixture "${guest_literal_fixture}")
    set(ecm_guest_modeled_fixture "${guest_modeled_fixture}")
    target_compile_definitions(${replay_target} PRIVATE
      "SCHWAY_FUZZ_ECM_LITERAL=\"${guest_literal_fixture}\"")
    target_compile_definitions(${replay_target} PRIVATE
      "SCHWAY_FUZZ_ECM_MODELED=\"${guest_modeled_fixture}\"")
  else()
    set(ccd_fixture "${PROJECT_BINARY_DIR}/fixtures/disc_boot_legacy.ccd")
    set(ccd_image "${PROJECT_BINARY_DIR}/fixtures/disc_boot_legacy.img")
    set(ccd_sub "${PROJECT_BINARY_DIR}/fixtures/disc_boot_legacy.sub")
    set(guest_ccd_fixture "${ccd_fixture}")
    set(guest_ccd_image "${ccd_image}")
    set(guest_ccd_sub "${ccd_sub}")
    if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
      set(guest_ccd_fixture "/fixtures/disc_boot_legacy.ccd")
      set(guest_ccd_image "/fixtures/disc_boot_legacy.img")
      set(guest_ccd_sub "/fixtures/disc_boot_legacy.sub")
    endif()
    set(ccd_guest_fixture "${guest_ccd_fixture}")
    set(ccd_guest_image "${guest_ccd_image}")
    set(ccd_guest_sub "${guest_ccd_sub}")
    target_compile_definitions(${replay_target} PRIVATE
      "SCHWAY_FUZZ_CCD_FIXTURE=\"${guest_ccd_fixture}\"")
    target_compile_definitions(${replay_target} PRIVATE
      "SCHWAY_FUZZ_CCD_IMAGE=\"${guest_ccd_image}\"")
    target_compile_definitions(${replay_target} PRIVATE
      "SCHWAY_FUZZ_CCD_SUB=\"${guest_ccd_sub}\"")
  endif()
  set_target_properties(${replay_target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
  schway_warnings(${replay_target})
  schway_add_test(NAME fuzz.regress.${subsystem}
    COMMAND ${replay_target}
    LABELS fuzz portable TIMEOUT 60)
  set_tests_properties(fuzz.regress.${subsystem} PROPERTIES
    FIXTURES_REQUIRED "disc_legacy_preflight;disc_legacy_parity")
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

  foreach(subsystem IN ITEMS cue cnf m3u cdrom mdec sbi)
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
    elseif(subsystem STREQUAL "cnf" OR subsystem STREQUAL "sbi")
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

  add_executable(schwaystation-fuzz-chd ${fuzz_dir}/chd.c)
  target_compile_definitions(schwaystation-fuzz-chd PRIVATE
    "SCHWAYSTATION_CHD=1"
    "SCHWAY_FUZZ_WORK_DIR=\"${chd_guest_work_dir}\""
    "SCHWAY_FUZZ_CHD_FIXTURE=\"${chd_guest_fixture}\"")
  target_include_directories(schwaystation-fuzz-chd PRIVATE ${PROJECT_SOURCE_DIR}/host)
  target_link_libraries(schwaystation-fuzz-chd PRIVATE schwaystation_disc_image)
  target_compile_options(schwaystation-fuzz-chd PRIVATE -fsanitize=fuzzer)
  target_link_options(schwaystation-fuzz-chd PRIVATE -fsanitize=fuzzer)
  set_target_properties(schwaystation-fuzz-chd PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
  schway_warnings(schwaystation-fuzz-chd)

  add_executable(schwaystation-fuzz-pbp ${fuzz_dir}/pbp.c)
  target_compile_definitions(schwaystation-fuzz-pbp PRIVATE
    "SCHWAY_FUZZ_WORK_DIR=\"${pbp_guest_work_dir}\""
    "SCHWAY_FUZZ_PBP_FIXTURE=\"${pbp_guest_fixture}\"")
  target_include_directories(schwaystation-fuzz-pbp PRIVATE ${PROJECT_SOURCE_DIR}/host)
  target_link_libraries(schwaystation-fuzz-pbp PRIVATE schwaystation_disc_image)
  target_compile_options(schwaystation-fuzz-pbp PRIVATE -fsanitize=fuzzer)
  target_link_options(schwaystation-fuzz-pbp PRIVATE -fsanitize=fuzzer)
  set_target_properties(schwaystation-fuzz-pbp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
  schway_warnings(schwaystation-fuzz-pbp)

  foreach(subsystem IN ITEMS ecm ccd)
    set(target schwaystation-fuzz-${subsystem})
    add_executable(${target} ${fuzz_dir}/${subsystem}.c)
    target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/host)
    target_link_libraries(${target} PRIVATE schwaystation_disc_image)
    add_dependencies(${target} schwaystation-legacy-disc-fixtures)
    if(subsystem STREQUAL "ecm")
      target_compile_definitions(${target} PRIVATE
        "SCHWAY_FUZZ_WORK_DIR=\"${ecm_guest_work_dir}\"")
      target_compile_definitions(${target} PRIVATE
        "SCHWAY_FUZZ_ECM_LITERAL=\"${ecm_guest_literal_fixture}\"")
      target_compile_definitions(${target} PRIVATE
        "SCHWAY_FUZZ_ECM_MODELED=\"${ecm_guest_modeled_fixture}\"")
    else()
      target_compile_definitions(${target} PRIVATE
        "SCHWAY_FUZZ_WORK_DIR=\"${ccd_guest_work_dir}\"")
      target_compile_definitions(${target} PRIVATE
        "SCHWAY_FUZZ_CCD_FIXTURE=\"${ccd_guest_fixture}\"")
      target_compile_definitions(${target} PRIVATE
        "SCHWAY_FUZZ_CCD_IMAGE=\"${ccd_guest_image}\"")
      target_compile_definitions(${target} PRIVATE
        "SCHWAY_FUZZ_CCD_SUB=\"${ccd_guest_sub}\"")
    endif()
    target_compile_options(${target} PRIVATE -fsanitize=fuzzer)
    target_link_options(${target} PRIVATE -fsanitize=fuzzer)
    set_target_properties(${target} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
    schway_warnings(${target})
  endforeach()

endif()

foreach(subsystem IN ITEMS ppf cheat)
  set(replay_target schwaystation-fuzz-${subsystem}-replay)
  add_executable(${replay_target} ${fuzz_dir}/${subsystem}.c)
  target_compile_definitions(${replay_target} PRIVATE SCHWAY_FUZZ_REPLAY)
  target_include_directories(${replay_target} PRIVATE
    ${PROJECT_SOURCE_DIR}/host ${PROJECT_SOURCE_DIR}/runner)
  if(subsystem STREQUAL "ppf")
    target_link_libraries(${replay_target} PRIVATE schwaystation_disc_image)
  else()
    target_link_libraries(${replay_target} PRIVATE schwaystation-host schwaystation::schwaystation)
  endif()
  set_target_properties(${replay_target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
  schway_warnings(${replay_target})

  schway_add_test(NAME fuzz.regress.${subsystem}
    COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} "$<TARGET_FILE:${replay_target}>"
    LABELS fuzz portable TIMEOUT 60)

  if(SCHWAYSTATION_FUZZ)
    set(fuzz_target schwaystation-fuzz-${subsystem})
    add_executable(${fuzz_target} ${fuzz_dir}/${subsystem}.c)
    target_include_directories(${fuzz_target} PRIVATE
      ${PROJECT_SOURCE_DIR}/host ${PROJECT_SOURCE_DIR}/runner)
    if(subsystem STREQUAL "ppf")
      target_link_libraries(${fuzz_target} PRIVATE schwaystation_disc_image)
    else()
      target_link_libraries(${fuzz_target} PRIVATE schwaystation-host schwaystation::schwaystation)
    endif()
    target_compile_options(${fuzz_target} PRIVATE -fsanitize=fuzzer)
    target_link_options(${fuzz_target} PRIVATE -fsanitize=fuzzer)
    set_target_properties(${fuzz_target} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
    schway_warnings(${fuzz_target})
  endif()
endforeach()
