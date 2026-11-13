# Fault prevented: the .info claim checker accepts a malformed or ungrounded
# capability, or stops rejecting one of its required metadata failures.
cmake_minimum_required(VERSION 3.25)

foreach(required INFO_FILE INVENTORY CHECKER EXPECT_FAIL WORK)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "info_claims_self_test: -D${required} is required")
  endif()
endforeach()
file(MAKE_DIRECTORY "${WORK}")
file(READ "${INFO_FILE}" base_info)
file(READ "${INVENTORY}" base_inventory)

function(reject name info inventory pattern)
  set(args)
  if(NOT "${info}" STREQUAL "")
    list(APPEND args "-DINFO_FILE=${info}")
  endif()
  if(NOT "${inventory}" STREQUAL "")
    list(APPEND args "-DINVENTORY=${inventory}")
  endif()
  set(command "${CMAKE_COMMAND};${args};-P;${CHECKER}")
  execute_process(COMMAND "${CMAKE_COMMAND}" "-DCMD=${command}" -DCODE=1
    "-DREGEX=${pattern}" -P "${EXPECT_FAIL}"
    RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE err)
  if(NOT result STREQUAL "0")
    message(FATAL_ERROR "${name}: expected refusal matching ${pattern}; got ${result}: ${out}${err}")
  endif()
endfunction()

function(replace_info name before after pattern)
  string(FIND "${base_info}" "${before}" position)
  if(position LESS 0)
    message(FATAL_ERROR "${name}: source .info does not contain expected field")
  endif()
  string(REPLACE "${before}" "${after}" changed "${base_info}")
  set(path "${WORK}/${name}.info")
  file(WRITE "${path}" "${changed}")
  reject("${name}" "${path}" "${INVENTORY}" "${pattern}")
endfunction()

function(append_info name suffix pattern)
  set(path "${WORK}/${name}.info")
  file(WRITE "${path}" "${base_info}${suffix}")
  reject("${name}" "${path}" "${INVENTORY}" "${pattern}")
endfunction()

reject("missing arguments" "" "" "-DINFO_FILE is required")
reject("missing inventory argument" "${INFO_FILE}" "" "-DINVENTORY is required")
reject("missing metadata file" "${WORK}/absent.info" "${INVENTORY}" "INFO_FILE does not exist")
reject("missing inventory file" "${INFO_FILE}" "${WORK}/absent.inventory" "INVENTORY does not exist")
replace_info("missing required key" "core_options = \"true\"\n" "" "missing .info key core_options")
append_info("duplicate required key" "supports_no_game = \"true\"\n"
  "duplicate .info key supports_no_game")
replace_info("malformed input claim" "input_descriptors = \"true\""
  "input_descriptors = \"yes\"" "input_descriptors must be a boolean string")
replace_info("no-game refusal" "supports_no_game = \"true\""
  "supports_no_game = \"false\"" "supports_no_game must stay true")
replace_info("core-options refusal" "core_options = \"true\""
  "core_options = \"false\"" "core_options must stay true")
replace_info("experimental refusal" "is_experimental = \"true\""
  "is_experimental = \"false\"" "keep experimental true until v2.0")

string(REPLACE "libretro.input.descriptors\tlibretro,portable\n" ""
  without_input "${base_inventory}")
if(without_input STREQUAL base_inventory)
  message(FATAL_ERROR "self-test inventory lacks libretro.input.descriptors row")
endif()
set(without_input_path "${WORK}/without-input.inventory")
file(WRITE "${without_input_path}" "${without_input}")
reject("ungrounded input claim" "${INFO_FILE}" "${without_input_path}"
  "input_descriptors has no inventory test")

string(REPLACE "libretro.memory.boundaries\tlibretro,portable\n" ""
  without_memory "${base_inventory}")
if(without_memory STREQUAL base_inventory)
  message(FATAL_ERROR "self-test inventory lacks libretro.memory.boundaries row")
endif()
set(without_memory_path "${WORK}/without-memory.inventory")
file(WRITE "${without_memory_path}" "${without_memory}")
reject("ungrounded memory claim" "${INFO_FILE}" "${without_memory_path}"
  "memory_descriptors has no inventory test")

replace_info("disk control refusal" "disk_control = \"false\""
  "disk_control = \"true\"" "unsupported disk_control claim must remain false")
replace_info("extension refusal" "supported_extensions = \"exe|psexe\""
  "supported_extensions = \"exe|cue\"" "only PS-EXE extensions are supported")
foreach(unsupported IN ITEMS savestate cheats load_subsystem)
  replace_info("unsupported ${unsupported}" "${unsupported} = \"false\""
    "${unsupported} = \"true\"" "unsupported ${unsupported} claim must remain false")
endforeach()
append_info("ungrounded true claim" "camera = \"true\"\n"
  "no test mapping for true .info claim camera")

string(REPLACE "libretro.content.no_bios\tlibretro,portable\n" ""
  without_no_bios "${base_inventory}")
if(without_no_bios STREQUAL base_inventory)
  message(FATAL_ERROR "self-test inventory lacks libretro.content.no_bios row")
endif()
set(without_no_bios_path "${WORK}/without-no-bios.inventory")
file(WRITE "${without_no_bios_path}" "${without_no_bios}")
reject("shared EXE/fullpath mapping" "${INFO_FILE}" "${without_no_bios_path}"
  "supported_extensions has no inventory test")

message(STATUS "info_claims_self_test: all 19 metadata refusal controls passed")
