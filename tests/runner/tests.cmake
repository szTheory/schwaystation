# schwaystation-run: frame hashes, PNG output, bench report, usage and exit statuses, build revision.
#
# Run as a script (cmake -DCASE=<case> ... -P tests.cmake), this file is one
# runner check; include()d by tests/tests.cmake, it registers the tests.
if(CMAKE_SCRIPT_MODE_FILE)
  string(REPLACE "|" ";" EMULATOR "${EMULATOR}")
  # Runs the runner with ARGN and fails unless it exits with code and its
  # merged output matches regex.
  function(expect_exit code regex)
    execute_process(COMMAND ${EMULATOR} ${RUN} ${ARGN} RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL code OR NOT out MATCHES "${regex}")
      message(FATAL_ERROR "schwaystation-run ${ARGN}: expected exit ${code} and output matching\n"
        "  ${regex}\ngot exit ${rc} and output\n${out}")
    endif()
  endfunction()

  # Runs the runner with ARGN through expect_fail.cmake, which passes only
  # for exit 2 and a usage message.
  function(refuse)
    set(CMD ${EMULATOR} ${RUN} ${ARGN})
    set(CODE 2)
    set(REGEX "usage")
    include(${CMAKE_CURRENT_LIST_DIR}/../cmake/expect_fail.cmake)
  endfunction()

  # Runs one case of the bench unit test and fails unless it exits 0.
  function(unit case)
    execute_process(COMMAND ${EMULATOR} ${UNIT} ${case} RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "schwaystation-test-bench ${case}: exit ${rc}\n${out}")
    endif()
  endfunction()

  if(CASE STREQUAL "cheats_cli")
    foreach(var PYTHON3 OUT)
      if(NOT DEFINED ${var} OR "${${var}}" STREQUAL "")
        message(FATAL_ERROR "runner cheat CLI case requires ${var}")
      endif()
    endforeach()
    file(MAKE_DIRECTORY "${OUT}")
    set(cheat_exe "${OUT}/runner-cheat-boundary.exe")
    set(program [=[
import struct, sys

def lui(rt, immediate): return (0x0f << 26) | (rt << 16) | immediate
def ori(rt, rs, immediate): return (0x0d << 26) | (rs << 21) | (rt << 16) | immediate
def lbu(rt, offset, rs): return (0x24 << 26) | (rs << 21) | (rt << 16) | offset
def lhu(rt, offset, rs): return (0x25 << 26) | (rs << 21) | (rt << 16) | offset
def addiu(rt, rs, immediate): return (0x09 << 26) | (rs << 21) | (rt << 16) | (immediate & 0xffff)
def bne(rs, rt, offset): return (0x05 << 26) | (rs << 21) | (rt << 16) | (offset & 0xffff)
def sb(rt, offset, rs): return (0x28 << 26) | (rs << 21) | (rt << 16) | offset
def jump(address): return (0x02 << 26) | ((address >> 2) & 0x03ffffff)

instructions = [
    lui(11, 0xbf80),
    ori(11, 11, 0x2080),            # debug port in t3
    lui(8, 0x8000),                 # RAM base in t0
    lbu(9, 0x100, 8),               # expected byte cheat in t1
    addiu(10, 0, 0x5a),
    bne(9, 10, 9), 0,               # branch to fail
    lhu(9, 0x102, 8),               # expected halfword cheat in t1
    addiu(10, 0, 0x1234),
    bne(9, 10, 5), 0,               # branch to fail
    addiu(9, 0, ord('Y')),
    sb(9, 0, 11),
    jump(0x80010134), 0,            # stay after success output
    addiu(9, 0, ord('N')),
    sb(9, 0, 11),
    jump(0x80010144), 0,            # stay after failure output
]
header = bytearray(0x800)
header[:8] = b"PS-X EXE"
struct.pack_into("<I", header, 0x10, 0x80010100)
struct.pack_into("<I", header, 0x14, 0)
struct.pack_into("<I", header, 0x18, 0x80010000)
struct.pack_into("<I", header, 0x1c, 0x800)
struct.pack_into("<I", header, 0x30, 0x801ff000)
struct.pack_into("<I", header, 0x34, 0x1000)
header[0x4c:0x4c + 55] = b"Sony Computer Entertainment Inc. for North America area"
image = bytearray(0x800)
struct.pack_into("<%dI" % len(instructions), image, 0x100, *instructions)
with open(sys.argv[1], "wb") as output:
    output.write(header)
    output.write(image)
]=])
    execute_process(COMMAND "${PYTHON3}" -c "${program}" "${cheat_exe}"
      RESULT_VARIABLE fixture_rc OUTPUT_VARIABLE fixture_out ERROR_VARIABLE fixture_err)
    if(NOT fixture_rc STREQUAL "0")
      message(FATAL_ERROR "could not generate the self-authored runner cheat executable: ${fixture_rc}\n"
        "${fixture_out}${fixture_err}")
    endif()
    execute_process(COMMAND ${EMULATOR} "${RUN}" --exe "${cheat_exe}" --frames 1 --tty
      RESULT_VARIABLE baseline_rc OUTPUT_VARIABLE baseline_out ERROR_VARIABLE baseline_err)
    set(baseline_output "${baseline_out}${baseline_err}")
    if(NOT baseline_rc STREQUAL "0" OR NOT baseline_output MATCHES "tty N")
      message(FATAL_ERROR "self-authored runner cheat executable did not report its expected no-cheat value:\\n"
        "exit ${baseline_rc}\\n${baseline_output}")
    endif()
    execute_process(COMMAND ${EMULATOR} "${RUN}" --exe "${cheat_exe}" --frames 1 --tty
        --cheat "30000100 00A5" --cheat "80000102 1234" --cheat "30000100 005A"
      RESULT_VARIABLE valid_rc OUTPUT_VARIABLE valid_out ERROR_VARIABLE valid_err)
    set(valid_output "${valid_out}${valid_err}")
    if(NOT valid_rc STREQUAL "0" OR NOT valid_output MATCHES "tty Y" OR
       valid_output MATCHES "tty N")
      message(FATAL_ERROR "runner did not apply repeatable byte/halfword cheats in option order before the first frame:\n"
        "exit ${valid_rc}\n${valid_output}")
    endif()

    set(rejected_png "${OUT}/runner-cheat-invalid.png")
    file(REMOVE "${rejected_png}")
    execute_process(COMMAND ${EMULATOR} "${RUN}" --exe "${cheat_exe}" --frames 1 --tty
        --png "1:${rejected_png}" --cheat "D0000100 00A5"
      RESULT_VARIABLE invalid_rc OUTPUT_VARIABLE invalid_out ERROR_VARIABLE invalid_err)
    string(TOLOWER "${invalid_out}${invalid_err}" invalid_output_lower)
    if(NOT invalid_rc STREQUAL "2" OR
       NOT invalid_output_lower MATCHES "(cheat|unsupported|unknown option)" OR
       "${invalid_out}${invalid_err}" MATCHES "(tty [YN]|frame [0-9])" OR EXISTS "${rejected_png}")
      message(FATAL_ERROR "runner did not reject invalid cheat text before execution or output mutation:\n"
        "exit ${invalid_rc}\n${invalid_out}${invalid_err}")
    endif()

    set(excess_args "")
    foreach(index RANGE 1 33)
      list(APPEND excess_args --cheat "30000100 00A5")
    endforeach()
    set(excess_png "${OUT}/runner-cheat-too-many.png")
    file(REMOVE "${excess_png}")
    execute_process(COMMAND ${EMULATOR} "${RUN}" --exe "${cheat_exe}" --frames 1 --tty
        --png "1:${excess_png}" ${excess_args}
      RESULT_VARIABLE excess_rc OUTPUT_VARIABLE excess_out ERROR_VARIABLE excess_err)
    string(TOLOWER "${excess_out}${excess_err}" excess_output_lower)
    if(NOT excess_rc STREQUAL "2" OR NOT excess_output_lower MATCHES "too many --cheat entries" OR
       "${excess_out}${excess_err}" MATCHES "(tty [YN]|frame [0-9])" OR EXISTS "${excess_png}")
      message(FATAL_ERROR "runner did not reject an over-cap cheat list before execution or output mutation:\n"
        "exit ${excess_rc}\n${excess_out}${excess_err}")
    endif()
  elseif(CASE STREQUAL "cheats_hash")
    if(NOT DEFINED OUT OR "${OUT}" STREQUAL "")
      message(FATAL_ERROR "runner cheat hash case requires OUT")
    endif()
    file(MAKE_DIRECTORY "${OUT}")
    string(REPEAT "[0-9a-f]" 64 digits)
    function(cheat_state result_name)
      execute_process(COMMAND ${EMULATOR} ${RUN} --frames 1 --hash-state ${ARGN}
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "^state sha256 ${digits}\n$")
        message(FATAL_ERROR "runner cheat hash ${ARGN}: expected exit 0 and one state hash, got ${rc}\n${out}${err}")
      endif()
      string(REGEX MATCH "state sha256 ([0-9a-f]+)" state_line "${out}")
      set(${result_name} "${CMAKE_MATCH_1}" PARENT_SCOPE)
    endfunction()

    cheat_state(no_cheat)
    cheat_state(no_cheat_repeat)
    cheat_state(normalized_upper --cheat "30000100 00A5")
    cheat_state(normalized_lower --cheat "30000100+00a5")
    cheat_state(single_entry --cheat "30000100 00A5")
    cheat_state(duplicate_entry --cheat "30000100 00A5" --cheat "30000100 00A5")
    cheat_state(order_ab --cheat "30000100 00A5" --cheat "30000101 00B6")
    cheat_state(order_ba --cheat "30000101 00B6" --cheat "30000100 00A5")
    if(no_cheat STREQUAL normalized_upper OR
       NOT no_cheat STREQUAL no_cheat_repeat OR
       NOT normalized_upper STREQUAL normalized_lower OR
       NOT single_entry STREQUAL normalized_upper OR
       single_entry STREQUAL duplicate_entry OR
       order_ab STREQUAL order_ba)
      message(FATAL_ERROR "runner state hashes did not preserve no-cheat and normalized configuration identity:\n"
        "no-cheat=${no_cheat} repeat=${no_cheat_repeat}\n"
        "upper=${normalized_upper} lower=${normalized_lower} single=${single_entry} duplicate=${duplicate_entry}\n"
        "A+B=${order_ab} B+A=${order_ba}")
    endif()

    set(rejected_png "${OUT}/runner-cheat-invalid-hash.png")
    file(REMOVE "${rejected_png}")
    execute_process(COMMAND ${EMULATOR} ${RUN} --frames 1 --hash-state --png "1:${rejected_png}"
        --cheat "D0000100 00A5"
      RESULT_VARIABLE invalid_rc OUTPUT_VARIABLE invalid_out ERROR_VARIABLE invalid_err)
    string(TOLOWER "${invalid_out}${invalid_err}" invalid_output_lower)
    if(NOT invalid_rc STREQUAL "2" OR invalid_out MATCHES "state sha256" OR
       NOT invalid_output_lower MATCHES "(cheat|unsupported|unknown option)" OR EXISTS "${rejected_png}")
      message(FATAL_ERROR "invalid cheat input produced state output or a file: exit ${invalid_rc}\n${invalid_out}${invalid_err}")
    endif()

    execute_process(COMMAND ${EMULATOR} ${RUN} --bios "${OUT}/unused.bin"
        --disc "${OUT}/unused.cue" --hash-state
      RESULT_VARIABLE disc_rc OUTPUT_VARIABLE disc_out ERROR_VARIABLE disc_err)
    if(NOT disc_rc STREQUAL "2" OR
       NOT "${disc_out}${disc_err}" MATCHES "--hash-state is unavailable with --disc")
      message(FATAL_ERROR "runner accepted --hash-state with --disc: exit ${disc_rc}\n${disc_out}${disc_err}")
    endif()
  elseif(CASE STREQUAL "patch_cli")
    foreach(var IN ITEMS RUN RUN_HOOKED OUT PYTHON3)
      if(NOT DEFINED ${var} OR "${${var}}" STREQUAL "")
        message(FATAL_ERROR "runner patch CLI case requires ${var}")
      endif()
    endforeach()
    file(MAKE_DIRECTORY "${OUT}")
    set(source "${OUT}/patch-cli-source.bin")
    set(cue "${OUT}/patch-cli-source.cue")
    set(patch "${OUT}/patch-cli.ppf")
    set(bios "${OUT}/patch-cli-inert-bios.bin")
    set(program [=[
import hashlib, struct, sys
source_path, cue_path, patch_path, bios_path = sys.argv[1:]
raw = bytearray()
for lba in range(32):
    sector = bytearray(2352)
    sector[0] = 0
    sector[1:11] = b"\xff" * 10
    sector[11] = 0
    minute, second = divmod((lba + 150) // 75, 60)
    frame = (lba + 150) % 75
    bcd = lambda value: ((value // 10) << 4) | (value % 10)
    sector[12:16] = bytes((bcd(minute), bcd(second), bcd(frame), 2))
    sector[16:24] = b"\x00\x00\x08\x00\x00\x00\x08\x00"
    sector[24] = 0x11
    raw.extend(sector)
with open(source_path, "wb") as stream:
    stream.write(raw)
with open(cue_path, "w", encoding="ascii") as stream:
    stream.write('FILE "patch-cli-source.bin" BINARY\n  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n')
patch = bytearray(b"PPF10\x00" + bytes(50))
patch.extend(struct.pack("<IB", 24, 1))
patch.append(0xA5)
with open(patch_path, "wb") as stream:
    stream.write(patch)
with open(bios_path, "wb") as stream:
    stream.write(bytes(524288))
print(hashlib.sha256(raw).hexdigest())
]=])
    execute_process(COMMAND "${PYTHON3}" -c "${program}" "${source}" "${cue}" "${patch}" "${bios}"
      RESULT_VARIABLE fixture_rc OUTPUT_VARIABLE source_sha ERROR_VARIABLE fixture_err
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(LENGTH "${source_sha}" source_sha_length)
    if(NOT fixture_rc STREQUAL "0" OR NOT source_sha_length EQUAL 64 OR
       NOT source_sha MATCHES "^[0-9a-f]+$")
      message(FATAL_ERROR "could not generate the raw patch CLI fixture: ${fixture_rc}\n${fixture_err}")
    endif()
    file(SHA256 "${source}" source_sha_before)
    execute_process(COMMAND "${RUN}" --bios "${bios}" --disc "${cue}"
        --patch "${patch}" --patch-source-sha256 "${source_sha}" --frames 1
      RESULT_VARIABLE valid_rc OUTPUT_VARIABLE valid_out ERROR_VARIABLE valid_err)
    set(valid_output "${valid_out}${valid_err}")
    if(NOT valid_rc STREQUAL "0" OR
       NOT valid_output MATCHES "patch format=PPF1 source-sha256=${source_sha} checks=source-sha256 records=1")
      message(FATAL_ERROR "runner did not report the source-bound PPF1 checks:\n"
        "exit ${valid_rc}\n${valid_output}")
    endif()
    execute_process(COMMAND "${RUN_HOOKED}" --bios "${bios}" --disc "${cue}"
        --patch "${patch}" --patch-source-sha256 "${source_sha}"
        --frames 1 --test-patch-sector-zero
      RESULT_VARIABLE provider_rc OUTPUT_VARIABLE provider_out ERROR_VARIABLE provider_err)
    set(provider_output "${provider_out}${provider_err}")
    if(NOT provider_rc STREQUAL "0" OR
       NOT provider_output MATCHES "patch test sector0 byte24=A5")
      message(FATAL_ERROR "runner patch overlay did not reach the sector provider callback:\n"
        "exit ${provider_rc}\n${provider_output}")
    endif()
    file(SHA256 "${source}" source_sha_after)
    if(NOT source_sha_before STREQUAL source_sha_after)
      message(FATAL_ERROR "runner patching modified its source image: ${source_sha_before} != ${source_sha_after}")
    endif()
    file(REMOVE "${source}" "${cue}" "${patch}" "${bios}")
  elseif(CASE STREQUAL "boot_full")
    foreach(var RUN BIOS FIXTURES)
      if(NOT DEFINED ${var} OR "${${var}}" STREQUAL "")
        message(FATAL_ERROR "runner full-boot case requires ${var}")
      endif()
    endforeach()
    set(disc "${FIXTURES}/disc_boot.cue")
    execute_process(COMMAND ${EMULATOR} "${RUN}" --bios "${BIOS}" --disc "${disc}"
        --frames 1200 --stop-on mailbox --hash
      RESULT_VARIABLE default_rc OUTPUT_VARIABLE default_out ERROR_VARIABLE default_err)
    execute_process(COMMAND ${EMULATOR} "${RUN}" --bios "${BIOS}" --disc "${disc}"
        --boot full --frames 1200 --stop-on mailbox --hash
      RESULT_VARIABLE full_rc OUTPUT_VARIABLE full_out ERROR_VARIABLE full_err)
    set(default_output "${default_out}${default_err}")
    set(full_output "${full_out}${full_err}")
    if(NOT default_rc STREQUAL "0" OR NOT full_rc STREQUAL "0" OR
       NOT default_output MATCHES "mailbox SWMB status=00" OR
       NOT full_output MATCHES "mailbox SWMB status=00" OR
       NOT default_output MATCHES "mailbox 0001=424f4f54" OR
       NOT full_output MATCHES "mailbox 0001=424f4f54" OR
       NOT default_output MATCHES "frame [0-9]+ sha256 [0-9a-f]+" OR
       NOT full_output MATCHES "frame [0-9]+ sha256 [0-9a-f]+" OR
       NOT default_output STREQUAL full_output)
      message(FATAL_ERROR "full BIOS boot differs from the default runner path:\n"
        "default (${default_rc}): ${default_output}\nfull (${full_rc}): ${full_output}")
    endif()
  elseif(CASE STREQUAL "boot_fast")
    foreach(var RUN BIOS FIXTURES)
      if(NOT DEFINED ${var} OR "${${var}}" STREQUAL "")
        message(FATAL_ERROR "runner fast-boot case requires ${var}")
      endif()
    endforeach()
    set(disc "${FIXTURES}/disc_boot.cue")
    execute_process(COMMAND ${EMULATOR} "${RUN}" --bios "${BIOS}" --disc "${disc}"
        --frames 1200 --stop-on mailbox --hash
      RESULT_VARIABLE full_rc OUTPUT_VARIABLE full_out ERROR_VARIABLE full_err)
    execute_process(COMMAND ${EMULATOR} "${RUN}" --bios "${BIOS}" --disc "${disc}"
        --boot fast --frames 1200 --stop-on mailbox --hash
      RESULT_VARIABLE fast_rc OUTPUT_VARIABLE fast_out ERROR_VARIABLE fast_err)
    set(full_output "${full_out}${full_err}")
    set(fast_output "${fast_out}${fast_err}")
    string(REGEX MATCHALL "mailbox [0-9a-f]+=[0-9a-f]+" full_records "${full_output}")
    string(REGEX MATCHALL "mailbox [0-9a-f]+=[0-9a-f]+" fast_records "${fast_output}")
    string(REGEX MATCH "frame [0-9]+ sha256 [0-9a-f]+" full_frame_line "${full_output}")
    string(REGEX MATCH "frame [0-9]+ sha256 [0-9a-f]+" fast_frame_line "${fast_output}")
    if(NOT full_rc STREQUAL "0" OR NOT fast_rc STREQUAL "0" OR
       NOT full_output MATCHES "mailbox SWMB status=00" OR
       NOT fast_output MATCHES "mailbox SWMB status=00" OR
       NOT full_output MATCHES "mailbox 0001=424f4f54" OR
       NOT fast_output MATCHES "mailbox 0001=424f4f54" OR
       NOT full_records STREQUAL fast_records OR
       full_frame_line STREQUAL "" OR fast_frame_line STREQUAL "" OR
       NOT full_frame_line STREQUAL fast_frame_line)
      message(FATAL_ERROR "fast boot did not preserve the eligible disc guest result:\n"
        "full (${full_rc}): ${full_output}\nfast (${fast_rc}): ${fast_output}")
    endif()
  elseif(CASE STREQUAL "boot_invalid")
    foreach(var RUN BIOS FIXTURES OUT HELLO_EXE)
      if(NOT DEFINED ${var} OR "${${var}}" STREQUAL "")
        message(FATAL_ERROR "runner fast-boot controls require ${var}")
      endif()
    endforeach()
    execute_process(COMMAND ${EMULATOR} "${RUN}" --boot fast --disc "${FIXTURES}/disc_boot.cue"
        --frames 1
      RESULT_VARIABLE no_bios_rc OUTPUT_VARIABLE no_bios_out ERROR_VARIABLE no_bios_err)
    execute_process(COMMAND ${EMULATOR} "${RUN}" --boot fast --bios "${BIOS}" --frames 1
      RESULT_VARIABLE no_disc_rc OUTPUT_VARIABLE no_disc_out ERROR_VARIABLE no_disc_err)
    file(MAKE_DIRECTORY "${OUT}")
    string(REPEAT "S" 32768 ineligible_iso)
    set(ineligible_path "${OUT}/runner-boot-ineligible.iso")
    file(WRITE "${ineligible_path}" "${ineligible_iso}")
    execute_process(COMMAND ${EMULATOR} "${RUN}" --boot fast --bios "${BIOS}"
        --disc "${ineligible_path}" --frames 1
      RESULT_VARIABLE ineligible_rc OUTPUT_VARIABLE ineligible_out ERROR_VARIABLE ineligible_err)
    if(NOT no_bios_rc STREQUAL "2" OR NOT no_bios_err MATCHES "--disc needs --bios" OR
       no_bios_out MATCHES "frame [0-9]+ sha256" OR
       NOT no_disc_rc STREQUAL "2" OR NOT no_disc_err MATCHES "--boot needs --bios and --disc" OR
       no_disc_out MATCHES "frame [0-9]+ sha256" OR
       NOT ineligible_rc STREQUAL "2" OR
       NOT ineligible_err MATCHES "--boot fast needs an eligible disc boot executable" OR
       ineligible_out MATCHES "frame [0-9]+ sha256")
      message(FATAL_ERROR "fast boot accepted a missing BIOS, missing disc, or ineligible disc:\n"
        "no BIOS (${no_bios_rc}): ${no_bios_out}${no_bios_err}\n"
        "no disc (${no_disc_rc}): ${no_disc_out}${no_disc_err}\n"
        "ineligible (${ineligible_rc}): ${ineligible_out}${ineligible_err}")
    endif()
    execute_process(COMMAND ${EMULATOR} "${RUN}" --exe "${HELLO_EXE}" --tty --frames 10
      RESULT_VARIABLE exe_rc OUTPUT_VARIABLE exe_out ERROR_VARIABLE exe_err)
    set(exe_output "${exe_out}${exe_err}")
    if(NOT exe_rc STREQUAL "0" OR NOT exe_output MATCHES "tty hello port")
      message(FATAL_ERROR "standalone BIOS-free --exe stub path regressed: ${exe_rc}: ${exe_output}")
    endif()
  elseif(CASE STREQUAL "disc_load")
    string(REPEAT "[0-9a-f]" 64 digits)
    if(NOT DEFINED HOOKED)
      message(FATAL_ERROR "runner disc identity test runner was not provided")
    endif()
    set(NORMAL_RUN ${RUN})
    set(RUN ${HOOKED})
    function(run_disc_identity image save_dir result_name)
      file(REMOVE_RECURSE "${save_dir}")
      file(MAKE_DIRECTORY "${save_dir}")
      set(input_args)
      if(ARGC GREATER 3)
        set(input_args --inputs "${ARGV3}")
      endif()
      foreach(slot IN ITEMS slot1 slot2)
        execute_process(COMMAND ${EMULATOR} ${RUN} --bios "${BIOS}" --disc "${image}"
            --frames 1 ${input_args} --save-dir "${save_dir}" --memcard-names serial
            --test-card-write "${slot}"
          RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
        if(NOT rc STREQUAL "0")
          message(FATAL_ERROR "disc identity failed for ${image} slot ${slot}: exit ${rc}\n${out}${err}")
        endif()
      endforeach()
      file(GLOB cards LIST_DIRECTORIES FALSE "${save_dir}/*.srm" "${save_dir}/*.mcr")
      list(LENGTH cards card_count)
      if(NOT card_count EQUAL 2)
        message(FATAL_ERROR "disc identity should use one shared basename for both slots: ${cards}")
      endif()
      set(slot1_name "")
      set(slot2_name "")
      foreach(card IN LISTS cards)
        get_filename_component(card "${card}" NAME)
        if(card MATCHES "[.]srm$")
          set(slot1_name "${card}")
        elseif(card MATCHES "[.]1[.]mcr$")
          set(slot2_name "${card}")
        endif()
      endforeach()
      string(REGEX REPLACE "[.]srm$" "" slot1_stem "${slot1_name}")
      string(REGEX REPLACE "[.]1[.]mcr$" "" slot2_stem "${slot2_name}")
      if(slot1_name STREQUAL "" OR NOT slot1_stem STREQUAL slot2_stem)
        message(FATAL_ERROR "the two card slots did not share an identity: ${cards}")
      endif()
      set(${result_name} "${slot1_name}" PARENT_SCOPE)
    endfunction()
    run_disc_identity("${FIXTURES}/disc_boot.cue" "${OUT}/runner-disc-identities" base_card)
    if(NOT base_card MATCHES "^sgi2-${digits}[.]srm$")
      message(FATAL_ERROR "missing/invalid first-disc serial should use the v2 full-content digest: ${base_card}")
    endif()
    if(CHD_ONLY)
      if(CHD_ENABLED)
        execute_process(COMMAND ${EMULATOR} ${RUN} --probe "${FIXTURES}/disc_boot.cue"
          RESULT_VARIABLE cue_probe_rc OUTPUT_VARIABLE cue_probe_out ERROR_VARIABLE cue_probe_err)
        execute_process(COMMAND ${EMULATOR} ${RUN} --probe "${FIXTURES}/disc_boot.chd"
          RESULT_VARIABLE chd_probe_rc OUTPUT_VARIABLE chd_probe_out ERROR_VARIABLE chd_probe_err)
        if(NOT cue_probe_rc STREQUAL "0" OR NOT chd_probe_rc STREQUAL "0" OR
           NOT cue_probe_out STREQUAL chd_probe_out)
          message(FATAL_ERROR "CUE and CHD probe identity differs:\n"
            "CUE ${cue_probe_rc}\n${cue_probe_out}${cue_probe_err}\n"
            "CHD ${chd_probe_rc}\n${chd_probe_out}${chd_probe_err}")
        endif()
        foreach(format IN ITEMS cue chd)
          execute_process(COMMAND ${EMULATOR} ${RUN} --bios "${BIOS}"
              --disc "${FIXTURES}/disc_boot.${format}" --frames 1200 --stop-on mailbox --hash
            RESULT_VARIABLE ${format}_rc OUTPUT_VARIABLE ${format}_out ERROR_VARIABLE ${format}_err)
          if(NOT ${format}_rc STREQUAL "0" OR NOT ${format}_out MATCHES "mailbox SWMB status=00" OR
             NOT ${format}_out MATCHES "frame [0-9]+ sha256 ${digits}")
            message(FATAL_ERROR "${format} disc run failed: exit ${${format}_rc}\n"
              "${${format}_out}${${format}_err}")
          endif()
        endforeach()
        if(NOT cue_out STREQUAL chd_out)
          message(FATAL_ERROR "CUE and CHD guest output or state hash differs:\nCUE ${cue_out}\nCHD ${chd_out}")
        endif()
        run_disc_identity("${FIXTURES}/disc_boot.chd" "${OUT}/runner-disc-identities-chd" chd_card)
        if(NOT chd_card STREQUAL base_card)
          message(FATAL_ERROR "equivalent CUE and CHD content produced different memory-card identities: ${base_card} vs ${chd_card}")
        endif()
      else()
        expect_exit(77 "unsupported: CHD [(]SCHWAYSTATION_CHD=OFF[)]"
          --probe "${FIXTURES}/disc_boot.chd")
        expect_exit(77 "unsupported: CHD [(]SCHWAYSTATION_CHD=OFF[)]"
          --bios "${BIOS}" --disc "${FIXTURES}/disc_boot.chd" --frames 1)
      endif()
      return()
    endif()

    set(moved_dir "${OUT}/runner-disc-identity-moved")
    file(REMOVE_RECURSE "${moved_dir}")
    file(MAKE_DIRECTORY "${moved_dir}")
    file(COPY_FILE "${FIXTURES}/disc_boot.bin" "${moved_dir}/renamed-data.bin")
    file(COPY_FILE "${FIXTURES}/disc_boot_track02.bin" "${moved_dir}/renamed-audio.bin")
    file(WRITE "${moved_dir}/renamed.cue"
      "FILE \"renamed-data.bin\" BINARY\nTRACK 01 MODE2/2352\nINDEX 01 00:00:00\n"
      "FILE \"renamed-audio.bin\" BINARY\nTRACK 02 AUDIO\nINDEX 00 00:00:00\nINDEX 01 00:02:00\n")
    run_disc_identity("${moved_dir}/renamed.cue" "${OUT}/runner-disc-identity-moved-cards" moved_card)
    if(NOT moved_card STREQUAL base_card)
      message(FATAL_ERROR "moving and renaming equivalent disc files changed identity: ${base_card} vs ${moved_card}")
    endif()

    set(playlist_dir "${OUT}/runner-disc-identity-playlist")
    file(REMOVE_RECURSE "${playlist_dir}")
    file(MAKE_DIRECTORY "${playlist_dir}")
    foreach(file IN ITEMS disc_boot.cue disc_boot.bin disc_boot_track02.bin
        disc_boot_playlist.cue disc_boot_playlist.bin disc_boot_playlist_track02.bin)
      file(COPY_FILE "${FIXTURES}/${file}" "${playlist_dir}/${file}")
    endforeach()
    file(WRITE "${playlist_dir}/identity.m3u" "disc_boot.cue\ndisc_boot_playlist.cue\n")
    run_disc_identity("${playlist_dir}/identity.m3u" "${OUT}/runner-disc-identity-playlist-cards" playlist_card)
    if(NOT playlist_card MATCHES "^sgi2-${digits}[.]srm$")
      message(FATAL_ERROR "later valid serial replaced the missing first serial instead of hashing the playlist: ${playlist_card}")
    endif()
    file(WRITE "${playlist_dir}/spaced.m3u" "  disc_boot.cue  \n\tdisc_boot_playlist.cue  \n")
    run_disc_identity("${playlist_dir}/spaced.m3u" "${OUT}/runner-disc-identity-spaced-cards" spaced_card)
    if(NOT spaced_card STREQUAL playlist_card)
      message(FATAL_ERROR "M3U whitespace changed the content identity: ${playlist_card} vs ${spaced_card}")
    endif()
    file(WRITE "${playlist_dir}/identity-swap.inputs" "1 lid=open\n1 disc=1\n1 lid=closed\n")
    run_disc_identity("${playlist_dir}/identity.m3u" "${OUT}/runner-disc-identity-swap-cards"
      swapped_card "${playlist_dir}/identity-swap.inputs")
    if(NOT swapped_card STREQUAL playlist_card)
      message(FATAL_ERROR "swapping discs changed the frozen card identity: ${playlist_card} vs ${swapped_card}")
    endif()
    file(WRITE "${playlist_dir}/one-member.m3u" "disc_boot.cue\n")
    run_disc_identity("${playlist_dir}/one-member.m3u" "${OUT}/runner-disc-identity-one-member-cards" one_member_card)
    if(one_member_card STREQUAL base_card OR one_member_card STREQUAL playlist_card)
      message(FATAL_ERROR "standalone, one-member playlist and two-member playlist domains collided")
    endif()
    file(WRITE "${playlist_dir}/serial-first.m3u" "disc_boot_playlist.cue\ndisc_boot.cue\n")
    run_disc_identity("${playlist_dir}/serial-first.m3u" "${OUT}/runner-disc-identity-serial-cards" serial_card)
    if(NOT serial_card STREQUAL "SLES-12346.srm")
      message(FATAL_ERROR "the valid first-member serial did not win over later members: ${serial_card}")
    endif()
    file(COPY_FILE "${playlist_dir}/disc_boot_playlist_track02.bin"
      "${playlist_dir}/changed-audio.bin")
    if(NOT DEFINED PYTHON3 OR PYTHON3 STREQUAL "")
      message(FATAL_ERROR "runner disc identity test needs the configured Python 3 executable")
    endif()
    execute_process(COMMAND "${PYTHON3}" -c
        "from pathlib import Path; import sys; p=Path(sys.argv[1]); b=bytearray(p.read_bytes()); b[-1] ^= 1; p.write_bytes(b)"
        "${playlist_dir}/changed-audio.bin"
      RESULT_VARIABLE patch_audio_rc OUTPUT_VARIABLE patch_audio_out ERROR_VARIABLE patch_audio_err)
    if(NOT patch_audio_rc STREQUAL "0")
      message(FATAL_ERROR "could not alter the late audio sector for the identity test: ${patch_audio_out}${patch_audio_err}")
    endif()
    file(COPY_FILE "${playlist_dir}/changed-audio.bin" "${playlist_dir}/disc_boot_playlist_track02.bin")
    run_disc_identity("${playlist_dir}/identity.m3u" "${OUT}/runner-disc-identity-changed-cards" changed_card)
    if(changed_card STREQUAL playlist_card)
      message(FATAL_ERROR "changing the late audio sector did not change playlist identity")
    endif()

    set(failed_identity_dir "${OUT}/runner-disc-identities-failed")
    file(REMOVE_RECURSE "${failed_identity_dir}")
    file(MAKE_DIRECTORY "${failed_identity_dir}")
    execute_process(COMMAND ${EMULATOR} ${RUN} --bios "${BIOS}" --disc "${FIXTURES}/disc_boot.cue"
        --frames 1 --save-dir "${failed_identity_dir}" --memcard-names serial
        --test-disc-identity-read-failure
      RESULT_VARIABLE failed_identity_rc OUTPUT_VARIABLE failed_identity_out
      ERROR_VARIABLE failed_identity_err)
    file(GLOB failed_identity_cards LIST_DIRECTORIES FALSE
      "${failed_identity_dir}/*.srm" "${failed_identity_dir}/*.mcr")
    if(NOT failed_identity_rc STREQUAL "1" OR
       NOT failed_identity_err MATCHES "cannot read disc for card identity" OR
       failed_identity_cards)
      message(FATAL_ERROR "disc identity sector failure published card paths: ${failed_identity_rc}\n"
        "${failed_identity_cards}\n${failed_identity_out}${failed_identity_err}")
    endif()
    set(RUN ${NORMAL_RUN})
    execute_process(COMMAND ${EMULATOR} ${RUN} --bios "${BIOS}" --disc "${FIXTURES}/disc_boot.cue"
        --frames 1 --hash-state
      RESULT_VARIABLE state_rc OUTPUT_VARIABLE state_out ERROR_VARIABLE state_err)
    string(FIND "${state_err}" "--hash-state is unavailable with --disc" state_message)
    if(NOT state_rc STREQUAL "2" OR state_message EQUAL -1 OR
       state_out MATCHES "frame [0-9]+ sha256")
      message(FATAL_ERROR "disc --hash-state must be rejected before running, got exit ${state_rc}\n"
        "${state_out}${state_err}")
    endif()
    function(run_disc path result_name)
      execute_process(COMMAND ${EMULATOR} ${RUN} --bios "${BIOS}" --disc "${path}"
          --frames 1200 --stop-on mailbox --hash
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "mailbox SWMB status=00" OR
         NOT out MATCHES "frame [0-9]+ sha256 ${digits}")
        message(FATAL_ERROR "BIOS disc run ${path}: expected a sealed guest mailbox and hashes, "
          "got exit ${rc}\n${out}${err}")
      endif()
      set(${result_name} "${out}" PARENT_SCOPE)
    endfunction()
    run_disc("${FIXTURES}/disc_boot.cue" cue_output)
    run_disc("${FIXTURES}/disc_boot_runner.m3u" playlist_output)
    string(REPEAT "S" 32768 cooked_iso)
    file(WRITE "${OUT}/runner-disc-valid.iso" "${cooked_iso}")
    execute_process(COMMAND ${EMULATOR} ${RUN} --bios "${BIOS}" --disc "${OUT}/runner-disc-valid.iso"
        --frames 1 --hash
      RESULT_VARIABLE iso_rc OUTPUT_VARIABLE iso_out ERROR_VARIABLE iso_err)
    if(NOT iso_rc STREQUAL "0" OR NOT iso_out MATCHES "frame 1 sha256 ${digits}")
      message(FATAL_ERROR "BIOS cooked ISO attach failed: exit ${iso_rc}\n${iso_out}${iso_err}")
    endif()
    file(WRITE "${OUT}/runner-disc-missing-bios.bin" "missing")
    execute_process(COMMAND ${EMULATOR} ${RUN} --bios "${OUT}/runner-disc-no-bios.bin"
        --disc "${FIXTURES}/disc_boot.cue" --frames 1
      RESULT_VARIABLE missing_rc OUTPUT_VARIABLE missing_out ERROR_VARIABLE missing_err)
    execute_process(COMMAND ${EMULATOR} ${RUN} --bios "${OUT}/runner-disc-missing-bios.bin"
        --disc "${FIXTURES}/disc_boot.cue" --frames 1
      RESULT_VARIABLE short_rc OUTPUT_VARIABLE short_out ERROR_VARIABLE short_err)
    if(NOT missing_rc STREQUAL "1" OR NOT missing_err MATCHES "cannot open" OR
       NOT short_rc STREQUAL "1" OR NOT short_err MATCHES "BIOS must be exactly 524288 bytes" OR
       missing_out MATCHES "frame [0-9]+ sha256" OR short_out MATCHES "frame [0-9]+ sha256")
      message(FATAL_ERROR "missing/short BIOS was not refused before the first frame:\n"
        "missing: ${missing_rc}\n${missing_out}${missing_err}\nshort: ${short_rc}\n${short_out}${short_err}")
    endif()
  elseif(CASE STREQUAL "disc_pbp")
    if(NOT DEFINED PYTHON3 OR PYTHON3 STREQUAL "")
      message(FATAL_ERROR "PBP runner controls need the configured Python 3 executable")
    endif()
    if(NOT DEFINED HOOKED OR NOT DEFINED BIOS OR NOT EXISTS "${BIOS}")
      message(FATAL_ERROR "PBP runner controls need the test runner and owned BIOS")
    endif()
    set(root "${OUT}/runner-disc-pbp")
    file(MAKE_DIRECTORY "${root}")
    set(python_code [=[
import pathlib
import struct
import sys

source = bytearray(pathlib.Path(sys.argv[1]).read_bytes())
root = pathlib.Path(sys.argv[2])
psar = 0x10000
toc = 0x10800
index = 0x14000
data = 0x110000

def save(name, image):
    (root / name).write_bytes(image)

image = bytearray(source)
image[psar:psar + 12] = b"PGD" + b"\0" * 9
save("pgd.pbp", image)

image = bytearray(source)
image[psar:psar + 12] = b"UNKNOWN!!!!!"
save("unknown.pbp", image)

image = bytearray(source)
struct.pack_into("<H", image, index + 4, 0)
save("corrupt-table.pbp", image)

image = bytearray(source)
first_size = struct.unpack_from("<H", image, index + 4)[0]
if first_size < 2:
    raise SystemExit("first PBP block is too short to truncate")
del image[data + first_size - 1]
struct.pack_into("<H", image, index + 4, first_size - 1)
for block in range(1, 4):
    entry = index + block * 32
    offset = struct.unpack_from("<I", image, entry)[0]
    struct.pack_into("<I", image, entry, offset - 1)
size = len(image)
struct.pack_into("<I", image, psar + 12, size - psar)
struct.pack_into("<I", image, psar + 16, size - 0x6420)
struct.pack_into("<I", image, psar + 0x1220, size - 0xD2CF)
save("truncated-block.pbp", image)
]=])
    execute_process(COMMAND "${PYTHON3}" -c "${python_code}"
      "${FIXTURES}/schwaystation-disc_boot.pbp" "${root}"
      RESULT_VARIABLE mutate_rc OUTPUT_VARIABLE mutate_out ERROR_VARIABLE mutate_err)
    if(NOT mutate_rc STREQUAL "0")
      message(FATAL_ERROR "could not create bounded PBP runner controls: ${mutate_out}${mutate_err}")
    endif()

    execute_process(COMMAND ${EMULATOR} ${RUN} --probe "${FIXTURES}/schwaystation-disc_boot.pbp"
      RESULT_VARIABLE pbp_probe_rc OUTPUT_VARIABLE pbp_probe_out ERROR_VARIABLE pbp_probe_err)
    execute_process(COMMAND ${EMULATOR} ${RUN} --probe "${FIXTURES}/disc_boot_pbp.cue"
      RESULT_VARIABLE cue_probe_rc OUTPUT_VARIABLE cue_probe_out ERROR_VARIABLE cue_probe_err)
    if(NOT pbp_probe_rc STREQUAL "0" OR NOT cue_probe_rc STREQUAL "0" OR
       NOT pbp_probe_out STREQUAL cue_probe_out OR NOT pbp_probe_out MATCHES "format=disc")
      message(FATAL_ERROR "PBP and single-track CUE probe identity differs:\n"
        "PBP ${pbp_probe_rc}\n${pbp_probe_out}${pbp_probe_err}\n"
        "CUE ${cue_probe_rc}\n${cue_probe_out}${cue_probe_err}")
    endif()

    foreach(format IN ITEMS pbp cue)
      if(format STREQUAL "pbp")
        set(image "${FIXTURES}/schwaystation-disc_boot.pbp")
      else()
        set(image "${FIXTURES}/disc_boot_pbp.cue")
      endif()
      execute_process(COMMAND ${EMULATOR} ${RUN} --bios "${BIOS}" --disc "${image}"
          --frames 1200 --hash
        RESULT_VARIABLE ${format}_run_rc OUTPUT_VARIABLE ${format}_run_out ERROR_VARIABLE ${format}_run_err)
      if(NOT ${format}_run_rc STREQUAL "0" OR
         NOT ${format}_run_out MATCHES "frame 1200 sha256 [0-9a-f]+")
        message(FATAL_ERROR "${format} bounded PBP guest run failed: exit ${${format}_run_rc}\n"
          "${${format}_run_out}${${format}_run_err}")
      endif()
    endforeach()
    if(NOT pbp_run_out STREQUAL cue_run_out)
      message(FATAL_ERROR "PBP and single-track CUE guest output/state hash differs:\n"
        "PBP ${pbp_run_out}\nCUE ${cue_run_out}")
    endif()

    function(pbp_identity image save_dir result_name)
      file(REMOVE_RECURSE "${save_dir}")
      file(MAKE_DIRECTORY "${save_dir}")
      foreach(slot IN ITEMS slot1 slot2)
        execute_process(COMMAND ${EMULATOR} "${HOOKED}" --bios "${BIOS}" --disc "${image}"
            --frames 1 --save-dir "${save_dir}" --memcard-names serial --test-card-write "${slot}"
          RESULT_VARIABLE identity_rc OUTPUT_VARIABLE identity_out ERROR_VARIABLE identity_err)
        if(NOT identity_rc STREQUAL "0")
          message(FATAL_ERROR "PBP content identity failed for ${image} ${slot}: exit ${identity_rc}\n"
            "${identity_out}${identity_err}")
        endif()
      endforeach()
      file(GLOB cards LIST_DIRECTORIES FALSE "${save_dir}/*.srm" "${save_dir}/*.mcr")
      list(LENGTH cards card_count)
      if(NOT card_count EQUAL 2)
        message(FATAL_ERROR "PBP content identity expected two isolated card files: ${cards}")
      endif()
      set(stem "")
      foreach(card IN LISTS cards)
        get_filename_component(card "${card}" NAME)
        if(card MATCHES "[.]srm$")
          string(REGEX REPLACE "[.]srm$" "" stem "${card}")
        elseif(card MATCHES "[.]1[.]mcr$")
          string(REGEX REPLACE "[.]1[.]mcr$" "" other_stem "${card}")
          if(NOT stem STREQUAL "" AND NOT stem STREQUAL other_stem)
            message(FATAL_ERROR "PBP slots produced different identity stems: ${cards}")
          endif()
          set(stem "${other_stem}")
        endif()
      endforeach()
      set(${result_name} "${stem}" PARENT_SCOPE)
    endfunction()
    pbp_identity("${FIXTURES}/schwaystation-disc_boot.pbp" "${root}/cards-pbp" pbp_card)
    pbp_identity("${FIXTURES}/disc_boot_pbp.cue" "${root}/cards-cue" cue_card)
    if(pbp_card STREQUAL "" OR NOT pbp_card STREQUAL cue_card)
      message(FATAL_ERROR "PBP and single-track CUE produced different save identities: ${pbp_card} vs ${cue_card}")
    endif()

    function(expect_pbp_failure image code pattern)
      execute_process(COMMAND ${EMULATOR} ${RUN} --probe "${image}"
        RESULT_VARIABLE probe_rc OUTPUT_VARIABLE probe_out ERROR_VARIABLE probe_err)
      if(NOT probe_rc STREQUAL "${code}" OR NOT probe_err MATCHES "${pattern}" OR
         probe_out MATCHES "frame [0-9]+ sha256")
        message(FATAL_ERROR "PBP probe ${image} expected exit ${code} and ${pattern}, got ${probe_rc}\n"
          "${probe_out}${probe_err}")
      endif()
      execute_process(COMMAND ${EMULATOR} "${HOOKED}" --bios "${BIOS}" --disc "${image}" --frames 1
        RESULT_VARIABLE run_rc OUTPUT_VARIABLE run_out ERROR_VARIABLE run_err)
      if(NOT run_rc STREQUAL "${code}" OR NOT run_err MATCHES "${pattern}" OR
         run_out MATCHES "frame [0-9]+ sha256")
        message(FATAL_ERROR "PBP runner ${image} expected pre-frame exit ${code} and ${pattern}, got ${run_rc}\n"
          "${run_out}${run_err}")
      endif()
    endfunction()
    expect_pbp_failure("${root}/pgd.pbp" 77 "PBP layout is unsupported")
    expect_pbp_failure("${root}/unknown.pbp" 77 "PBP layout is unsupported")
    expect_pbp_failure("${root}/corrupt-table.pbp" 1 "PBP metadata is invalid")
    expect_pbp_failure("${root}/truncated-block.pbp" 1 "PBP data block is truncated or invalid")
  elseif(CASE STREQUAL "disc_probe")
    execute_process(COMMAND ${EMULATOR} ${RUN} --probe "${FIXTURES}/disc_boot.cue"
      RESULT_VARIABLE cue_rc OUTPUT_VARIABLE cue_out ERROR_VARIABLE cue_err)
    execute_process(COMMAND ${EMULATOR} ${RUN} --probe "${FIXTURES}/disc_boot_runner.m3u"
      RESULT_VARIABLE playlist_rc OUTPUT_VARIABLE playlist_out ERROR_VARIABLE playlist_err)
    if(NOT cue_rc STREQUAL "0" OR NOT cue_out MATCHES "format=disc" OR
       NOT cue_out MATCHES "serial=redacted" OR
       NOT cue_out MATCHES "rcheevos_md5=redacted" OR
       NOT cue_out MATCHES "track1_sha1=redacted" OR
       NOT playlist_rc STREQUAL "0" OR NOT playlist_out MATCHES "format=disc" OR
       NOT playlist_out MATCHES "playlist_count=2")
      message(FATAL_ERROR "disc probe failed:\nCUE ${cue_rc}\n${cue_out}${cue_err}\n"
        "playlist ${playlist_rc}\n${playlist_out}${playlist_err}")
    endif()
    if(NOT DEFINED PYTHON3 OR PYTHON3 STREQUAL "")
      message(FATAL_ERROR "sector-4 region probe needs the configured Python 3 executable")
    endif()
    file(MAKE_DIRECTORY "${OUT}")
    set(patch_sector4 [==[
from pathlib import Path
import sys
image = bytearray(Path(sys.argv[1]).read_bytes())
start = 4 * 2352 + 24
if start + 2048 > len(image):
    raise SystemExit("disc fixture is too short for sector 4")
sector = bytearray(2048)
marker = b"          Licensed  by          Sony Computer Entertainment Euro pe   "
sector[:len(marker)] = marker
image[start:start + 2048] = sector
Path(sys.argv[2]).write_bytes(image)
]==])
    execute_process(COMMAND "${PYTHON3}" -c "${patch_sector4}"
        "${FIXTURES}/disc_boot.bin" "${OUT}/disc_boot_region.bin"
      RESULT_VARIABLE patch_rc OUTPUT_VARIABLE patch_out ERROR_VARIABLE patch_err)
    if(NOT patch_rc STREQUAL "0")
      message(FATAL_ERROR "could not create the sector-4 probe image: ${patch_out}${patch_err}")
    endif()
    file(COPY_FILE "${FIXTURES}/disc_boot_track02.bin" "${OUT}/disc_boot_region_track02.bin")
    file(WRITE "${OUT}/disc_boot_region.cue"
      "FILE \"disc_boot_region.bin\" BINARY\n"
      "  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n"
      "FILE \"disc_boot_region_track02.bin\" BINARY\n"
      "  TRACK 02 AUDIO\n    INDEX 00 00:00:00\n    INDEX 01 00:02:00\n")
    execute_process(COMMAND ${EMULATOR} ${RUN} --probe "${OUT}/disc_boot_region.cue"
      RESULT_VARIABLE region_rc OUTPUT_VARIABLE region_out ERROR_VARIABLE region_err)
    if(NOT region_rc STREQUAL "0" OR
       NOT region_out MATCHES "disc_licence_area=europe" OR
       NOT region_out MATCHES "region_guess=pal" OR
       NOT region_out MATCHES "region_conflicts=0x00000004")
      message(FATAL_ERROR "sector-4 licence area was not extracted before the EXE marker:\n"
        "exit ${region_rc}\n${region_out}${region_err}")
    endif()
  elseif(CASE STREQUAL "disc_legacy")
    if(NOT DEFINED HOOKED OR NOT EXISTS "${BIOS}")
      message(FATAL_ERROR "legacy runner checks need the hooked runner and owned BIOS")
    endif()
    file(SIZE "${FIXTURES}/disc_boot_pbp_source.bin" ecm_source_bytes)
    file(SIZE "${FIXTURES}/disc_boot.bin" ccd_track1_bytes)
    file(SIZE "${FIXTURES}/disc_boot_track02.bin" ccd_track2_bytes)
    math(EXPR ecm_remainder "${ecm_source_bytes} % 2352")
    math(EXPR ecm_sectors "${ecm_source_bytes} / 2352")
    math(EXPR ccd_source_bytes "${ccd_track1_bytes} + ${ccd_track2_bytes}")
    math(EXPR ccd_remainder "${ccd_source_bytes} % 2352")
    math(EXPR ccd_sectors "${ccd_source_bytes} / 2352")
    if(NOT ecm_remainder EQUAL 0 OR NOT ecm_sectors EQUAL 64 OR
       NOT ccd_remainder EQUAL 0 OR NOT ccd_sectors EQUAL 222)
      message(FATAL_ERROR "legacy source extent changed before the runner guest check: ECM=${ecm_source_bytes}, CCD=${ccd_source_bytes}")
    endif()
    function(assert_legacy_counts manifest expected)
      if(NOT EXISTS "${manifest}")
        message(FATAL_ERROR "legacy source-count manifest is missing: ${manifest}")
      endif()
      file(STRINGS "${manifest}" rows REGEX "^(source_sectors|guest_reads|guest_outputs)=")
      list(LENGTH rows row_count)
      if(NOT row_count EQUAL 3)
        message(FATAL_ERROR "legacy manifest must have source, requested and produced sector counts: ${manifest}")
      endif()
      foreach(row IN LISTS rows)
        string(REPLACE "=" ";" fields "${row}")
        list(GET fields 0 field)
        list(GET fields 1 value)
        if(NOT value STREQUAL expected)
          message(FATAL_ERROR "${manifest}: ${field}=${value}, expected ${expected} before guest execution")
        endif()
      endforeach()
    endfunction()
    assert_legacy_counts("${FIXTURES}/disc_boot_legacy_ecm.manifest" "${ecm_sectors}")
    assert_legacy_counts("${FIXTURES}/disc_boot_legacy_ccd.manifest" "${ccd_sectors}")

    set(root "${OUT}/runner-disc-legacy")
    file(REMOVE_RECURSE "${root}")
    file(MAKE_DIRECTORY "${root}")
    foreach(name IN ITEMS disc_boot_legacy_literal.ecm disc_boot_legacy_modeled.ecm
        disc_boot_legacy_literal.cue disc_boot_legacy_modeled.cue)
      file(COPY_FILE "${FIXTURES}/${name}" "${root}/${name}")
    endforeach()
    file(COPY_FILE "${FIXTURES}/disc_boot_legacy_literal.ecm" "${root}/uppercase.ECM")
    file(WRITE "${root}/uppercase.cue"
      "FILE \"uppercase.ECM\" BINARY\n  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n")
    file(COPY_FILE "${FIXTURES}/disc_boot_legacy.ccd" "${root}/mixed.CCD")
    file(COPY_FILE "${FIXTURES}/disc_boot_legacy.img" "${root}/mixed.img")
    file(COPY_FILE "${FIXTURES}/disc_boot_legacy.sub" "${root}/mixed.sub")
    file(COPY_FILE "${FIXTURES}/disc_boot_legacy.ccd" "${root}/mixed-nosub.ccd")
    file(COPY_FILE "${FIXTURES}/disc_boot_legacy.img" "${root}/mixed-nosub.img")
    file(WRITE "${root}/legacy.m3u"
      "disc_boot_legacy_literal.cue\ndisc_boot_legacy_modeled.cue\n")

    function(run_legacy_probe image result_name)
      execute_process(COMMAND ${EMULATOR} ${RUN} --probe "${image}"
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "format=disc")
        message(FATAL_ERROR "legacy probe failed for ${image}: exit ${rc}\n${out}${err}")
      endif()
      set(${result_name} "${out}" PARENT_SCOPE)
    endfunction()
    function(expect_legacy_probe image expected label)
      run_legacy_probe("${image}" actual)
      if(NOT actual STREQUAL expected)
        message(FATAL_ERROR "${label} probe identity differs from the owned raw source CUE:\n"
          "expected:\n${expected}\nactual:\n${actual}")
      endif()
    endfunction()
    run_legacy_probe("${FIXTURES}/disc_boot_legacy_source.cue" ecm_source_probe)
    expect_legacy_probe("${FIXTURES}/disc_boot_legacy_literal.cue" "${ecm_source_probe}" "literal ECM CUE")
    expect_legacy_probe("${FIXTURES}/disc_boot_legacy_modeled.cue" "${ecm_source_probe}" "modeled ECM CUE")
    expect_legacy_probe("${FIXTURES}/disc_boot_legacy_literal.ecm" "${ecm_source_probe}" "direct ECM")
    expect_legacy_probe("${root}/uppercase.cue" "${ecm_source_probe}" "uppercase ECM CUE child")
    expect_legacy_probe("${root}/uppercase.ECM" "${ecm_source_probe}" "uppercase ECM extension")

    execute_process(COMMAND ${EMULATOR} ${RUN} --probe "${root}/legacy.m3u"
      RESULT_VARIABLE m3u_rc OUTPUT_VARIABLE m3u_out ERROR_VARIABLE m3u_err)
    if(NOT m3u_rc STREQUAL "0" OR NOT m3u_out MATCHES "playlist_count=2" OR
       NOT m3u_out MATCHES "serial=redacted")
      message(FATAL_ERROR "legacy ECM CUE playlist probe did not retain M3U policy: ${m3u_rc}\n${m3u_out}${m3u_err}")
    endif()

    run_legacy_probe("${FIXTURES}/disc_boot.cue" ccd_source_probe)
    expect_legacy_probe("${FIXTURES}/disc_boot_legacy.ccd" "${ccd_source_probe}" "direct CCD")
    expect_legacy_probe("${root}/mixed.CCD" "${ccd_source_probe}" "uppercase CCD extension")
    expect_legacy_probe("${root}/mixed-nosub.ccd" "${ccd_source_probe}" "CCD without optional SUB")

    string(REPEAT "[0-9a-f]" 64 digest)
    function(run_legacy_guest image result_name)
      execute_process(COMMAND ${EMULATOR} ${RUN} --bios "${BIOS}" --disc "${image}"
          --frames 1200 --hash
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "frame 1200 sha256 ${digest}")
        message(FATAL_ERROR "bounded legacy guest failed for ${image}: exit ${rc}\n${out}${err}")
      endif()
      set(${result_name} "${out}" PARENT_SCOPE)
    endfunction()
    run_legacy_guest("${FIXTURES}/disc_boot_legacy_source.cue" ecm_source_guest)
    run_legacy_guest("${FIXTURES}/disc_boot_legacy_literal.cue" ecm_literal_guest)
    run_legacy_guest("${FIXTURES}/disc_boot_legacy_modeled.cue" ecm_modeled_guest)
    run_legacy_guest("${FIXTURES}/disc_boot_legacy_literal.ecm" ecm_direct_guest)
    run_legacy_guest("${root}/legacy.m3u" ecm_m3u_guest)
    if(NOT ecm_source_guest STREQUAL ecm_literal_guest OR
       NOT ecm_source_guest STREQUAL ecm_modeled_guest OR
       NOT ecm_source_guest STREQUAL ecm_direct_guest OR
       NOT ecm_literal_guest STREQUAL ecm_m3u_guest)
      message(FATAL_ERROR "ECM/CUE/M3U bounded guest output differs from the owned source CUE")
    endif()
    run_legacy_guest("${FIXTURES}/disc_boot.cue" ccd_source_guest)
    run_legacy_guest("${FIXTURES}/disc_boot_legacy.ccd" ccd_guest)
    run_legacy_guest("${root}/mixed.CCD" ccd_upper_guest)
    run_legacy_guest("${root}/mixed-nosub.ccd" ccd_nosub_guest)
    if(NOT ccd_source_guest STREQUAL ccd_guest OR NOT ccd_guest STREQUAL ccd_upper_guest OR
       NOT ccd_guest STREQUAL ccd_nosub_guest)
      message(FATAL_ERROR "CCD bounded guest output differs from its owned source CUE")
    endif()

    function(legacy_save_identity image save_dir result_name)
      file(REMOVE_RECURSE "${save_dir}")
      file(MAKE_DIRECTORY "${save_dir}")
      foreach(slot IN ITEMS slot1 slot2)
        execute_process(COMMAND ${EMULATOR} "${HOOKED}" --bios "${BIOS}" --disc "${image}"
            --frames 1 --save-dir "${save_dir}" --memcard-names serial --test-card-write "${slot}"
          RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
        if(NOT rc STREQUAL "0")
          message(FATAL_ERROR "legacy save identity failed for ${image} ${slot}: exit ${rc}\n${out}${err}")
        endif()
      endforeach()
      file(GLOB cards LIST_DIRECTORIES FALSE "${save_dir}/*.srm" "${save_dir}/*.1.mcr")
      list(LENGTH cards card_count)
      if(NOT card_count EQUAL 2)
        message(FATAL_ERROR "legacy save identity expected both memory-card files: ${cards}")
      endif()
      set(stem "")
      foreach(card IN LISTS cards)
        get_filename_component(card_name "${card}" NAME)
        if(card_name MATCHES "[.]srm$")
          string(REGEX REPLACE "[.]srm$" "" card_stem "${card_name}")
        elseif(card_name MATCHES "[.]1[.]mcr$")
          string(REGEX REPLACE "[.]1[.]mcr$" "" card_stem "${card_name}")
        else()
          message(FATAL_ERROR "unexpected legacy memory-card path: ${card}")
        endif()
        if(stem STREQUAL "")
          set(stem "${card_stem}")
        elseif(NOT stem STREQUAL card_stem)
          message(FATAL_ERROR "legacy memory-card slots have different identities: ${cards}")
        endif()
      endforeach()
      set(${result_name} "${stem}" PARENT_SCOPE)
    endfunction()
    legacy_save_identity("${FIXTURES}/disc_boot_legacy_source.cue" "${root}/save-ecm-source" ecm_source_save)
    legacy_save_identity("${FIXTURES}/disc_boot_legacy_literal.cue" "${root}/save-ecm-reader" ecm_reader_save)
    legacy_save_identity("${FIXTURES}/disc_boot_legacy_image.cue" "${root}/save-ccd-source" ccd_source_save)
    legacy_save_identity("${root}/mixed-nosub.ccd" "${root}/save-ccd-reader" ccd_reader_save)
    if(NOT ecm_source_save STREQUAL ecm_reader_save OR NOT ccd_source_save STREQUAL ccd_reader_save)
      message(FATAL_ERROR "legacy source/read save identities differ: ECM ${ecm_source_save} vs ${ecm_reader_save}, CCD ${ccd_source_save} vs ${ccd_reader_save}")
    endif()
  elseif(CASE STREQUAL "disc_unsupported")
    file(MAKE_DIRECTORY "${OUT}/runner-disc-unsupported")
    set(root "${OUT}/runner-disc-unsupported")
    if(NOT DEFINED PYTHON3 OR PYTHON3 STREQUAL "")
      message(FATAL_ERROR "legacy malformed-media checks need the configured Python 3 executable")
    endif()
    set(ecm_mutation [==[
from pathlib import Path
import sys
data = bytearray(Path(sys.argv[1]).read_bytes())
if sys.argv[3] == "truncate":
    data = data[:-1]
elif sys.argv[3] == "edc":
    data[-1] ^= 1
else:
    raise SystemExit("unknown ECM mutation")
Path(sys.argv[2]).write_bytes(data)
]==])
    foreach(variant IN ITEMS truncated invalid-edc)
      if(variant STREQUAL "truncated")
        set(mutation truncate)
      else()
        set(mutation edc)
      endif()
      execute_process(COMMAND "${PYTHON3}" -c "${ecm_mutation}"
          "${FIXTURES}/disc_boot_legacy_literal.ecm" "${root}/${variant}.ecm" "${mutation}"
        RESULT_VARIABLE mutation_rc OUTPUT_VARIABLE mutation_out ERROR_VARIABLE mutation_err)
      if(NOT mutation_rc STREQUAL "0")
        message(FATAL_ERROR "could not create ${variant} ECM test input: ${mutation_out}${mutation_err}")
      endif()
    endforeach()
    file(WRITE "${root}/nested-edc.cue"
      "FILE \"invalid-edc.ecm\" BINARY\nTRACK 01 MODE2/2352\nINDEX 01 00:00:00\n")
    file(COPY_FILE "${FIXTURES}/disc_boot_legacy_modeled.ecm" "${root}/modeled.ecm")
    file(WRITE "${root}/nested-2336.cue"
      "FILE \"modeled.ecm\" BINARY\nTRACK 01 MODE2/2336\nINDEX 01 00:00:00\n")
    file(COPY_FILE "${FIXTURES}/disc_boot_legacy.ccd" "${root}/missing-companion.ccd")
    file(READ "${FIXTURES}/disc_boot_legacy.ccd" ccd_descriptor)
    file(COPY_FILE "${FIXTURES}/disc_boot_legacy.img" "${root}/unsupported-version.img")
    file(COPY_FILE "${FIXTURES}/disc_boot_legacy.img" "${root}/unsupported-session.img")
    set(ccd_version "${ccd_descriptor}")
    string(REPLACE "Version=3" "Version=4" ccd_version "${ccd_version}")
    if("${ccd_version}" STREQUAL "${ccd_descriptor}")
      message(FATAL_ERROR "could not mutate CCD version test input")
    endif()
    file(WRITE "${root}/unsupported-version.ccd" "${ccd_version}")
    set(ccd_session "${ccd_descriptor}")
    string(REPLACE "Sessions=1" "Sessions=2" ccd_session "${ccd_session}")
    if("${ccd_session}" STREQUAL "${ccd_descriptor}")
      message(FATAL_ERROR "could not mutate CCD session test input")
    endif()
    file(WRITE "${root}/unsupported-session.ccd" "${ccd_session}")
    file(COPY_FILE "${FIXTURES}/disc_boot_legacy.ccd" "${root}/nested-ccd.ccd")
    file(COPY_FILE "${FIXTURES}/disc_boot_legacy.img" "${root}/nested-ccd.img")
    file(WRITE "${root}/nested-ccd.cue"
      "FILE \"nested-ccd.ccd\" BINARY\nTRACK 01 MODE2/2352\nINDEX 01 00:00:00\n")
    file(COPY_FILE "${FIXTURES}/disc_boot_legacy_literal.ecm" "${OUT}/legacy-outside.ecm")
    file(WRITE "${root}/nested-escape.cue"
      "FILE \"../legacy-outside.ecm\" BINARY\nTRACK 01 MODE2/2352\nINDEX 01 00:00:00\n")
    file(WRITE "${root}/nested-chd.m3u" "unsupported.chd\n")
    file(WRITE "${root}/nested-pbp.m3u" "unsupported.pbp\n")
    function(expect_disc_run_failure image expected_rc pattern label)
      execute_process(COMMAND ${EMULATOR} ${RUN} --bios "${BIOS}" --disc "${image}" --frames 1 --hash
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT "${rc}" STREQUAL "${expected_rc}" OR NOT err MATCHES "${pattern}" OR
         out MATCHES "frame [0-9]+ sha256")
        message(FATAL_ERROR "${label} should fail before guest output with exit ${expected_rc} / ${pattern}; got ${rc}\n${out}${err}")
      endif()
    endfunction()
    function(expect_disc_probe_failure image expected_rc pattern label)
      execute_process(COMMAND ${EMULATOR} ${RUN} --probe "${image}"
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT "${rc}" STREQUAL "${expected_rc}" OR NOT err MATCHES "${pattern}")
        message(FATAL_ERROR "${label} probe should fail with exit ${expected_rc} / ${pattern}; got ${rc}\n${out}${err}")
      endif()
    endfunction()

    expect_disc_run_failure("${root}/truncated.ecm" 1 "ECM stream is malformed" "truncated ECM")
    expect_disc_probe_failure("${root}/truncated.ecm" 1 "ECM stream is malformed" "truncated ECM")
    expect_disc_run_failure("${root}/invalid-edc.ecm" 1 "ECM stream is malformed" "invalid ECM EDC")
    expect_disc_probe_failure("${root}/invalid-edc.ecm" 1 "ECM stream is malformed" "invalid ECM EDC")
    expect_disc_run_failure("${root}/missing-companion.ccd" 1 "CCD IMG companion is missing" "missing CCD IMG")
    expect_disc_probe_failure("${root}/missing-companion.ccd" 1 "CCD IMG companion is missing" "missing CCD IMG")
    foreach(variant IN ITEMS version session)
      expect_disc_run_failure("${root}/unsupported-${variant}.ccd" 77
        "CCD metadata is outside the supported subset" "unsupported CCD ${variant}")
      expect_disc_probe_failure("${root}/unsupported-${variant}.ccd" 77
        "CCD metadata is outside the supported subset" "unsupported CCD ${variant}")
    endforeach()
    expect_disc_run_failure("${root}/nested-edc.cue" 1 "ECM stream is malformed" "nested invalid ECM EDC")
    expect_disc_run_failure("${root}/nested-escape.cue" 1 "unsafe FILE child path" "canonical CUE child escape")
    expect_disc_run_failure("${root}/nested-2336.cue" 1 "unsupported cue track mode" "unproven ECM 2336-byte mapping")
    expect_disc_run_failure("${root}/nested-ccd.cue" 77 "unsupported: CCD cannot be a CUE track source" "CCD CUE-child policy")
    expect_disc_run_failure("${root}/nested-pbp.m3u" 77
      "unsupported: PBP playlist entries are not supported" "PBP playlist refusal")
    if(NOT CHD_ENABLED)
      expect_disc_run_failure("${root}/unsupported.chd" 77 "unsupported: CHD" "CHD-off direct refusal")
      expect_disc_probe_failure("${root}/unsupported.chd" 77 "unsupported: CHD" "CHD-off direct refusal")
      expect_disc_run_failure("${root}/nested-chd.m3u" 77 "unsupported: CHD" "CHD-off playlist refusal")
    endif()
    execute_process(COMMAND ${EMULATOR} ${RUN} --bios "${BIOS}" --disc "${root}/missing.iso" --frames 1
      RESULT_VARIABLE io_rc OUTPUT_VARIABLE io_out ERROR_VARIABLE io_err)
    if(NOT io_rc STREQUAL "1" OR io_err MATCHES "unsupported:")
      message(FATAL_ERROR "missing ISO must stay an infrastructure error, got ${io_rc}\n${io_out}${io_err}")
    endif()
  elseif(CASE STREQUAL "inputs_disc_controls")
    string(REPEAT "[0-9a-f]" 64 digits)
    function(run_control_movie movie result_name)
      execute_process(COMMAND ${EMULATOR} ${RUN} --bios "${BIOS}" --disc "${FIXTURES}/disc_boot_runner.m3u"
          --frames 360 --hash --inputs "${movie}"
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "frame 360 sha256 ${digits}" OR
         NOT out MATCHES "lag frames=360")
        message(FATAL_ERROR "disc control movie ${movie} failed: exit ${rc}\n${out}${err}")
      endif()
      set(${result_name} "${out}" PARENT_SCOPE)
    endfunction()
    run_control_movie("${INPUTS}" pressed)
    run_control_movie("${INPUTS}" repeated)
    if(NOT pressed STREQUAL repeated)
      message(FATAL_ERROR "frame-300 pad edge and ordered disc controls changed the repeated run output")
    endif()
    file(WRITE "${OUT}/runner-disc-closed-swap.inputs" "1 disc=1\n")
    execute_process(COMMAND ${EMULATOR} ${RUN} --bios "${BIOS}" --disc "${FIXTURES}/disc_boot_runner.m3u"
        --frames 1 --inputs "${OUT}/runner-disc-closed-swap.inputs"
      RESULT_VARIABLE closed_rc OUTPUT_VARIABLE closed_out ERROR_VARIABLE closed_err)
    file(WRITE "${OUT}/runner-disc-range.inputs" "1 lid=open\n1 disc=2\n")
    execute_process(COMMAND ${EMULATOR} ${RUN} --bios "${BIOS}" --disc "${FIXTURES}/disc_boot_runner.m3u"
        --frames 1 --inputs "${OUT}/runner-disc-range.inputs"
      RESULT_VARIABLE range_rc OUTPUT_VARIABLE range_out ERROR_VARIABLE range_err)
    if(NOT closed_rc STREQUAL "2" OR NOT closed_out STREQUAL "" OR
       NOT closed_err MATCHES "disc swap requires lid=open" OR
       NOT range_rc STREQUAL "2" OR NOT range_out STREQUAL "" OR
       NOT range_err MATCHES "outside the installed playlist")
      message(FATAL_ERROR "invalid disc controls were not rejected before execution:\n"
        "closed: ${closed_rc}\n${closed_out}${closed_err}\nrange: ${range_rc}\n${range_out}${range_err}")
    endif()
  elseif(CASE STREQUAL "bench_format")
    # The report's text from fixed samples, then the runner's own report:
    # every line present, in order, as a whole number. Times are never
    # compared, as they vary from run to run.
    unit(format)
    set(num "[0-9]+")
    set(report "frames 30\np50_us ${num}\np90_us ${num}\np99_us ${num}\np99[.]9_us ${num}\n"
      "p99[.]99_us ${num}\np99[.]999_us ${num}\nmax_us ${num}\nmax_rss_kib ${num}\n")
    string(CONCAT report ${report})
    expect_exit(0 "^${report}$" --bench 30)
    # One frame: every percentile is the one sample, so all seven times are
    # equal.
    execute_process(COMMAND ${EMULATOR} ${RUN} --bench 1 RESULT_VARIABLE rc OUTPUT_VARIABLE out)
    string(REGEX MATCHALL "_us [0-9]+" times "${out}")
    list(REMOVE_DUPLICATES times)
    list(LENGTH times n)
    if(NOT rc STREQUAL "0" OR NOT out MATCHES "^frames 1\n" OR NOT n EQUAL 1)
      message(FATAL_ERROR "--bench 1: expected exit 0 and seven equal times, got exit ${rc}\n${out}")
    endif()
  elseif(CASE STREQUAL "bench_bytes")
    # The sample buffer is sized exactly up to the cap; a count that is zero,
    # negative, missing or above the command line's limit is refused before
    # anything runs, never a wrapped allocation.
    unit(bytes)
    refuse(--bench 0)
    refuse(--bench -1)
    refuse(--bench)
    refuse(--bench 1000001)
    refuse(--bench 1073741824)
    refuse(--bench 4294967295)
  elseif(CASE STREQUAL "stop")
    # The flag, then a runner built to raise SIGTERM inside frame 5: the run
    # ends after that frame, still prints what was asked, and exits 0.
    unit(stop)
    set(RUN ${HOOKED})
    expect_exit(0 "^frame 5 sha256 ${EXPECTED}\n$" --frames 60 --hash)
    expect_exit(0 "^frames 5\n.*max_rss_kib [0-9]+\n$" --bench 60)
  elseif(CASE STREQUAL "cards_multitap")
    set(RUN ${HOOKED})
    string(REPEAT "[0-9a-f]" 64 digits)
    execute_process(COMMAND ${EMULATOR} ${RUN} --help RESULT_VARIABLE help_rc
      OUTPUT_VARIABLE help_out ERROR_VARIABLE help_err)
    if(NOT help_rc STREQUAL "0" OR NOT help_out MATCHES "--multitap-port1 enabled" OR
       NOT help_out MATCHES "--multitap-port2 enabled")
      message(FATAL_ERROR "runner help does not document both multitap options: ${help_out}${help_err}")
    endif()
    set(card_dir "${OUT}/runner-cards-multitap")
    file(REMOVE_RECURSE "${card_dir}")
    file(MAKE_DIRECTORY "${card_dir}")
    set(card_selectors port1a port1b port1c port1d port2a port2b port2c port2d)
    foreach(selector IN LISTS card_selectors)
      expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${card_dir}"
        --test-card-write "${selector}")
    endforeach()
    set(endpoint_card_files
      "${card_dir}/schwaystation.srm"
      "${card_dir}/schwaystation.p1b.mcr"
      "${card_dir}/schwaystation.p1c.mcr"
      "${card_dir}/schwaystation.p1d.mcr"
      "${card_dir}/schwaystation.1.mcr"
      "${card_dir}/schwaystation.p2b.mcr"
      "${card_dir}/schwaystation.p2c.mcr"
      "${card_dir}/schwaystation.p2d.mcr")
    set(expected_card_files
      "${card_dir}/schwaystation.srm"
      "${card_dir}/schwaystation.p1b.mcr"
      "${card_dir}/schwaystation.p1c.mcr"
      "${card_dir}/schwaystation.p1d.mcr"
      "${card_dir}/schwaystation.1.mcr"
      "${card_dir}/schwaystation.p2b.mcr"
      "${card_dir}/schwaystation.p2c.mcr"
      "${card_dir}/schwaystation.p2d.mcr")
    list(SORT expected_card_files)
    file(GLOB actual_card_files LIST_DIRECTORIES FALSE "${card_dir}/*.srm" "${card_dir}/*.mcr")
    list(SORT actual_card_files)
    if(NOT actual_card_files STREQUAL expected_card_files)
      message(FATAL_ERROR "runner did not persist exactly eight stable card socket files: ${actual_card_files}")
    endif()
    set(sector_values)
    foreach(card_file IN LISTS expected_card_files)
      if(NOT EXISTS "${card_file}")
        message(FATAL_ERROR "runner did not persist all eight card sockets; missing ${card_file}")
      endif()
      file(SIZE "${card_file}" card_size)
      file(READ "${card_file}" sector_value HEX OFFSET 896 LIMIT 8)
      if(NOT card_size EQUAL 131072 OR sector_value STREQUAL "0000000000000000")
        message(FATAL_ERROR "runner card has an invalid image size or empty written sector: ${card_file}")
      endif()
      list(APPEND sector_values "${sector_value}")
    endforeach()
    list(REMOVE_DUPLICATES sector_values)
    list(LENGTH sector_values unique_sector_count)
    if(NOT unique_sector_count EQUAL 8)
      message(FATAL_ERROR "card sockets did not retain eight unique sector payloads: ${sector_values}")
    endif()

    function(hash_card_set result_name)
      set(values)
      foreach(path IN LISTS expected_card_files)
        file(SHA256 "${path}" value)
        list(APPEND values "${value}")
      endforeach()
      set(${result_name} "${values}" PARENT_SCOPE)
    endfunction()
    hash_card_set(initial_hashes)
    set(reloaded_hashes "${initial_hashes}")
    foreach(index RANGE 0 7)
      list(GET card_selectors ${index} selector)
      list(GET endpoint_card_files ${index} endpoint_file)
      string(REPLACE "port" "reload-port" reload_mode "${selector}")
      expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${card_dir}"
        --test-card-write "${reload_mode}")
      hash_card_set(next_hashes)
      list(FIND expected_card_files "${endpoint_file}" selected_index)
      foreach(card_index RANGE 0 7)
        list(GET reloaded_hashes ${card_index} before)
        list(GET next_hashes ${card_index} after)
        if(card_index EQUAL selected_index)
          if(before STREQUAL after)
            message(FATAL_ERROR "fresh-process reload did not update ${selector}")
          endif()
        elseif(NOT before STREQUAL after)
          message(FATAL_ERROR "fresh-process update of ${selector} changed another card")
        endif()
      endforeach()
      set(reloaded_hashes "${next_hashes}")
    endforeach()

    expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${card_dir}")
    hash_card_set(clean_hashes)
    if(NOT clean_hashes STREQUAL reloaded_hashes)
      message(FATAL_ERROR "a clean exit rewrote an unchanged card image")
    endif()
    foreach(mode partial-port1b bad-port1b)
      expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${card_dir}"
        --test-card-write "${mode}")
      hash_card_set(aborted_hashes)
      if(NOT aborted_hashes STREQUAL clean_hashes)
        message(FATAL_ERROR "${mode} changed card bytes without a successful completed write")
      endif()
    endforeach()
    expect_exit(1 "cannot save card file" --frames 1 --save-dir "${card_dir}"
      --test-card-write port1b --test-save-failure rename)
    hash_card_set(failed_save_hashes)
    file(GLOB temporary_files "${card_dir}/*.schwaystation.tmp.*")
    if(NOT failed_save_hashes STREQUAL clean_hashes OR temporary_files)
      message(FATAL_ERROR "failed atomic replacement changed a stored card or left a temporary")
    endif()

    function(multitap_state result_name)
      execute_process(COMMAND ${EMULATOR} ${RUN} --frames 2 --hash-state ${ARGN}
        RESULT_VARIABLE state_rc OUTPUT_VARIABLE state_out ERROR_VARIABLE state_err)
      if(NOT state_rc STREQUAL "0" OR NOT state_out MATCHES "state sha256 ${digits}")
        message(FATAL_ERROR "multitap state run failed (${state_rc}): ${state_out}${state_err}")
      endif()
      string(REGEX MATCH "state sha256 ([0-9a-f]+)" state_match "${state_out}")
      set(${result_name} "${CMAKE_MATCH_1}" PARENT_SCOPE)
    endfunction()
    multitap_state(default_state)
    multitap_state(disabled_state --multitap-port1 disabled --multitap-port2 disabled)
    multitap_state(port1_state --multitap-port1 enabled)
    multitap_state(port2_state --multitap-port2 enabled)
    multitap_state(both_state --multitap-port1 enabled --multitap-port2 enabled)
    if(NOT default_state STREQUAL disabled_state OR default_state STREQUAL port1_state OR
       default_state STREQUAL port2_state OR port1_state STREQUAL port2_state OR
       port1_state STREQUAL both_state OR port2_state STREQUAL both_state)
      message(FATAL_ERROR "multitap options are not independent or default-disabled")
    endif()
    set(movie "${OUT}/runner-multitap.inputs")
    file(WRITE "${movie}" "2 p1=dualshock:X:128,128,128,128 p2=.\n")
    multitap_state(movie_disabled --inputs "${movie}")
    multitap_state(movie_port1 --inputs "${movie}" --multitap-port1 enabled)
    if(movie_disabled STREQUAL movie_port1)
      message(FATAL_ERROR "an input movie update discarded the selected multitap option")
    endif()
    refuse(--frames 1 --multitap-port1 invalid)
    refuse(--frames 1 --multitap-port1 enabled --multitap-port1 disabled)
    refuse(--frames 1 --multitap-port1)
  elseif(CASE STREQUAL "cards_persistence")
    set(NORMAL_RUN ${RUN})
    set(RUN ${HOOKED})
    string(REPEAT "[0-9a-f]" 64 digits)
    set(card_dir "${OUT}/runner-cards-persistence")
    file(MAKE_DIRECTORY "${card_dir}")
    file(WRITE "${card_dir}/schwaystation.srm" "old-slot-one")
    file(WRITE "${card_dir}/schwaystation.1.mcr" "old-slot-two")
    file(SHA256 "${card_dir}/schwaystation.1.mcr" old_slot_two)
    expect_exit(0 "frame 3 sha256 ${digits}" --frames 3 --hash --save-dir "${card_dir}"
      --test-card-write slot1)
    file(SIZE "${card_dir}/schwaystation.srm" slot_one_size)
    file(READ "${card_dir}/schwaystation.srm" slot_one_sector HEX OFFSET 896 LIMIT 8)
    file(SHA256 "${card_dir}/schwaystation.1.mcr" new_slot_two)
    if(NOT slot_one_size EQUAL 131072 OR NOT slot_one_sector STREQUAL "314e6b88a5c2dffc" OR
       NOT old_slot_two STREQUAL new_slot_two)
      message(FATAL_ERROR "normal exit did not save only slot 1's completed card generation")
    endif()

    # The same two-slot guest has independent normalized serial identities.
    if(NOT DEFINED FIXTURES)
      message(FATAL_ERROR "runner card fixture path was not provided")
    endif()
    set(identity_dir "${OUT}/runner-cards-identities")
    file(REMOVE_RECURSE "${identity_dir}")
    file(MAKE_DIRECTORY "${identity_dir}")
    set(identity_a_exe "${OUT}/sLuS_123.45.exe")
    set(identity_b_exe "${OUT}/SLUS_123.46.exe")
    file(COPY_FILE "${FIXTURES}/memcard_roundtrip.exe" "${identity_a_exe}")
    file(COPY_FILE "${FIXTURES}/memcard_roundtrip.exe" "${identity_b_exe}")
    function(run_identity_guest image label)
      execute_process(COMMAND ${EMULATOR} ${NORMAL_RUN} --exe "${image}" --frames 120 --stop-on mailbox
          --save-dir "${identity_dir}" --memcard-names serial
        RESULT_VARIABLE identity_rc OUTPUT_VARIABLE identity_out ERROR_VARIABLE identity_err)
      if(NOT identity_rc STREQUAL "0" OR NOT identity_out MATCHES "mailbox SWMB status=00")
        message(FATAL_ERROR "${label} automatic identity guest failed (${identity_rc}): ${identity_out}${identity_err}")
      endif()
    endfunction()
    function(check_identity_card path value label)
      if(NOT EXISTS "${path}")
        message(FATAL_ERROR "${label} card was not persisted: ${path}")
      endif()
      file(SIZE "${path}" card_size)
      file(READ "${path}" card_value HEX OFFSET 128 LIMIT 1)
      string(TOLOWER "${card_value}" card_value)
      file(READ "${path}" frame_checksum HEX OFFSET 255 LIMIT 1)
      string(TOLOWER "${frame_checksum}" frame_checksum)
      if(NOT card_size EQUAL 131072 OR NOT card_value STREQUAL value OR
         NOT frame_checksum STREQUAL value)
        message(FATAL_ERROR "${label} card has size/value/checksum ${card_size}/${card_value}/${frame_checksum}")
      endif()
    endfunction()
    run_identity_guest("${identity_a_exe}" "identity A first process")
    run_identity_guest("${identity_b_exe}" "identity B first process")
    set(identity_a_slot1 "${identity_dir}/SLUS-12345.srm")
    set(identity_a_slot2 "${identity_dir}/SLUS-12345.1.mcr")
    set(identity_b_slot1 "${identity_dir}/SLUS-12346.srm")
    set(identity_b_slot2 "${identity_dir}/SLUS-12346.1.mcr")
    check_identity_card("${identity_a_slot1}" "01" "first-process identity A slot 1")
    check_identity_card("${identity_a_slot2}" "02" "first-process identity A slot 2")
    check_identity_card("${identity_b_slot1}" "01" "first-process identity B slot 1")
    check_identity_card("${identity_b_slot2}" "02" "first-process identity B slot 2")
    file(GLOB identity_files LIST_DIRECTORIES FALSE "${identity_dir}/*.srm" "${identity_dir}/*.mcr")
    set(expected_identity_files "${identity_a_slot1}" "${identity_a_slot2}" "${identity_b_slot1}" "${identity_b_slot2}")
    list(SORT identity_files)
    list(SORT expected_identity_files)
    if(NOT identity_files STREQUAL expected_identity_files)
      message(FATAL_ERROR "two identities did not produce exactly four card files: ${identity_files}")
    endif()
    file(SHA256 "${identity_b_slot1}" identity_b_slot1_before)
    file(SHA256 "${identity_b_slot2}" identity_b_slot2_before)
    run_identity_guest("${identity_a_exe}" "identity A second process")
    check_identity_card("${identity_a_slot1}" "02" "second-process identity A slot 1")
    check_identity_card("${identity_a_slot2}" "03" "second-process identity A slot 2")
    file(SHA256 "${identity_b_slot1}" identity_b_slot1_after)
    file(SHA256 "${identity_b_slot2}" identity_b_slot2_after)
    if(NOT identity_b_slot1_before STREQUAL identity_b_slot1_after OR
       NOT identity_b_slot2_before STREQUAL identity_b_slot2_after)
      message(FATAL_ERROR "running identity A changed identity B's cards")
    endif()

    # A rejected second-slot import cannot change the first complete file.
    file(COPY_FILE "${identity_a_slot2}" "${identity_dir}/identity-a-slot2.backup")
    file(WRITE "${identity_a_slot2}" "")
    file(SHA256 "${identity_a_slot1}" identity_a_slot1_before_refusal)
    execute_process(COMMAND ${EMULATOR} ${RUN} --exe "${identity_a_exe}" --frames 120 --stop-on mailbox
        --save-dir "${identity_dir}" --memcard-names serial
      RESULT_VARIABLE refused_rc OUTPUT_VARIABLE refused_out ERROR_VARIABLE refused_err)
    file(SHA256 "${identity_a_slot1}" identity_a_slot1_after_refusal)
    if(NOT refused_rc STREQUAL "1" OR NOT refused_err MATCHES "cannot import card file" OR
       NOT identity_a_slot1_before_refusal STREQUAL identity_a_slot1_after_refusal)
      message(FATAL_ERROR "refused automatic identity import changed an existing complete file: ${refused_out}${refused_err}")
    endif()
    file(COPY_FILE "${identity_dir}/identity-a-slot2.backup" "${identity_a_slot2}")
    file(REMOVE "${identity_dir}/identity-a-slot2.backup")

    # A forced rename failure on the selected identity preserves both stored slots.
    file(SHA256 "${identity_a_slot1}" identity_a_slot1_before_failure)
    file(SHA256 "${identity_a_slot2}" identity_a_slot2_before_failure)
    execute_process(COMMAND ${EMULATOR} ${RUN} --exe "${identity_a_exe}" --frames 1 --save-dir "${identity_dir}"
        --memcard-names serial --test-card-write slot1 --test-save-failure rename
      RESULT_VARIABLE failed_rc OUTPUT_VARIABLE failed_out ERROR_VARIABLE failed_err)
    file(SHA256 "${identity_a_slot1}" identity_a_slot1_after_failure)
    file(SHA256 "${identity_a_slot2}" identity_a_slot2_after_failure)
    if(NOT failed_rc STREQUAL "1" OR NOT failed_err MATCHES "cannot save card file" OR
       NOT identity_a_slot1_before_failure STREQUAL identity_a_slot1_after_failure OR
       NOT identity_a_slot2_before_failure STREQUAL identity_a_slot2_after_failure)
      message(FATAL_ERROR "failed automatic identity publication changed an existing card: ${failed_out}${failed_err}")
    endif()

    file(SHA256 "${identity_a_slot1}" identity_a_slot1_before_b)
    run_identity_guest("${identity_b_exe}" "identity B second process")
    check_identity_card("${identity_b_slot1}" "02" "second-process identity B slot 1")
    check_identity_card("${identity_b_slot2}" "03" "second-process identity B slot 2")
    file(SHA256 "${identity_a_slot1}" identity_a_slot1_after_b)
    if(NOT identity_a_slot1_before_b STREQUAL identity_a_slot1_after_b)
      message(FATAL_ERROR "running identity B changed identity A's cards")
    endif()
  elseif(CASE STREQUAL "cards_sigterm")
    set(RUN ${HOOKED})
    string(REPEAT "[0-9a-f]" 64 digits)
    set(card_dir "${OUT}/runner-cards-sigterm")
    file(MAKE_DIRECTORY "${card_dir}")
    file(WRITE "${card_dir}/schwaystation.srm" "unchanged-slot-one")
    file(SHA256 "${card_dir}/schwaystation.srm" old_slot_one)
    expect_exit(0 "frame 5 sha256 ${EXPECTED}" --frames 60 --hash --save-dir "${card_dir}"
      --test-card-write slot2)
    file(SIZE "${card_dir}/schwaystation.1.mcr" slot_two_size)
    file(READ "${card_dir}/schwaystation.1.mcr" slot_two_sector HEX OFFSET 896 LIMIT 8)
    file(SHA256 "${card_dir}/schwaystation.srm" new_slot_one)
    if(NOT slot_two_size EQUAL 131072 OR NOT slot_two_sector STREQUAL "627f9cb9d6f3102d" OR
       NOT old_slot_one STREQUAL new_slot_one)
      message(FATAL_ERROR "SIGTERM did not flush only slot 2 after frame 5")
    endif()
  elseif(CASE STREQUAL "cards_controls")
    set(RUN ${HOOKED})
    string(REPEAT "[0-9a-f]" 64 digits)

    # Missing cards begin as formatted blanks and stay absent until a game writes them.
    set(missing_dir "${OUT}/runner-cards-missing")
    file(MAKE_DIRECTORY "${missing_dir}")
    expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${missing_dir}")
    if(EXISTS "${missing_dir}/schwaystation.srm" OR EXISTS "${missing_dir}/schwaystation.1.mcr")
      message(FATAL_ERROR "an unchanged blank card was written")
    endif()

    # An imported short raw image is preserved byte-for-byte when its generation is unchanged.
    set(unchanged_dir "${OUT}/runner-cards-unchanged")
    file(MAKE_DIRECTORY "${unchanged_dir}")
    file(WRITE "${unchanged_dir}/schwaystation.srm" "unchanged-short-card")
    file(SHA256 "${unchanged_dir}/schwaystation.srm" old_unchanged)
    expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${unchanged_dir}")
    file(SHA256 "${unchanged_dir}/schwaystation.srm" new_unchanged)
    if(NOT old_unchanged STREQUAL new_unchanged OR EXISTS "${unchanged_dir}/schwaystation.1.mcr")
      message(FATAL_ERROR "an unchanged short card was replaced or an absent slot was created")
    endif()

    # Empty imports fail before the first frame and leave the source untouched.
    set(invalid_dir "${OUT}/runner-cards-invalid-import")
    file(MAKE_DIRECTORY "${invalid_dir}")
    file(WRITE "${invalid_dir}/schwaystation.srm" "")
    file(SHA256 "${invalid_dir}/schwaystation.srm" old_invalid)
    execute_process(COMMAND ${EMULATOR} ${RUN} --frames 1 --hash --save-dir "${invalid_dir}"
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    file(SHA256 "${invalid_dir}/schwaystation.srm" new_invalid)
    if(NOT rc STREQUAL "1" OR NOT err MATCHES "cannot import card file" OR
       out MATCHES "frame 1 sha256" OR NOT old_invalid STREQUAL new_invalid)
      message(FATAL_ERROR "an invalid card did not fail before execution and preserve its source\n${out}${err}")
    endif()

    # Half-transactions and rejected checksums cannot dirty a durable generation.
    foreach(mode partial1 bad1)
      set(preserve_dir "${OUT}/runner-cards-${mode}")
      file(MAKE_DIRECTORY "${preserve_dir}")
      file(WRITE "${preserve_dir}/schwaystation.srm" "durable-${mode}")
      file(SHA256 "${preserve_dir}/schwaystation.srm" old_preserved)
      expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${preserve_dir}"
        --test-card-write "${mode}")
      file(SHA256 "${preserve_dir}/schwaystation.srm" new_preserved)
      if(NOT old_preserved STREQUAL new_preserved)
        message(FATAL_ERROR "${mode} changed a card without a completed successful write")
      endif()
    endforeach()

    # A successful slot 1 write followed by a rejected write flushes only the completed data.
    set(rejected_after_success_dir "${OUT}/runner-cards-rejected-after-success")
    file(MAKE_DIRECTORY "${rejected_after_success_dir}")
    expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${rejected_after_success_dir}"
      --test-card-write slot1_then_bad)
    file(SIZE "${rejected_after_success_dir}/schwaystation.srm" slot_one_size)
    file(READ "${rejected_after_success_dir}/schwaystation.srm" slot_one_sector HEX OFFSET 896 LIMIT 8)
    if(NOT slot_one_size EQUAL 131072 OR NOT slot_one_sector STREQUAL "314e6b88a5c2dffc" OR
       EXISTS "${rejected_after_success_dir}/schwaystation.1.mcr")
      message(FATAL_ERROR "a rejected write replaced completed data or dirtied the other slot")
    endif()

    # A slot 2 write must preserve slot 1 and create only slot 2.
    set(slot_two_dir "${OUT}/runner-cards-slot-two")
    file(MAKE_DIRECTORY "${slot_two_dir}")
    file(WRITE "${slot_two_dir}/schwaystation.srm" "keep-slot-one")
    file(SHA256 "${slot_two_dir}/schwaystation.srm" old_slot_one)
    expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${slot_two_dir}"
      --test-card-write slot2)
    file(SHA256 "${slot_two_dir}/schwaystation.srm" new_slot_one)
    file(SIZE "${slot_two_dir}/schwaystation.1.mcr" slot_two_size)
    file(READ "${slot_two_dir}/schwaystation.1.mcr" slot_two_sector HEX OFFSET 896 LIMIT 8)
    if(NOT old_slot_one STREQUAL new_slot_one OR NOT slot_two_size EQUAL 131072 OR
       NOT slot_two_sector STREQUAL "627f9cb9d6f3102d")
      message(FATAL_ERROR "slot 2 persistence changed the wrong slot or saved incorrect data")
    endif()

    # Explicit serial naming creates only the serial card paths.
    set(serial_dir "${OUT}/runner-cards-serial")
    file(MAKE_DIRECTORY "${serial_dir}")
    expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${serial_dir}"
      --memcard-names serial --serial SLUS_123.45 --test-card-write slot1)
    if(NOT EXISTS "${serial_dir}/SLUS_123.45.srm" OR
       EXISTS "${serial_dir}/schwaystation.srm")
      message(FATAL_ERROR "serial card naming did not use the validated serial basename")
    endif()

    # A validated PS-EXE with a complete serial-shaped source stem selects its
    # normalized identity automatically when serial naming is opted in.
    set(exe_serial_dir "${OUT}/runner-cards-exe-serial")
    file(MAKE_DIRECTORY "${exe_serial_dir}")
    set(serial_exe "${OUT}/sLuS_123.45.EXE")
    file(COPY_FILE "${FIXTURES}/memcard_roundtrip.exe" "${serial_exe}")
    expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${exe_serial_dir}"
      --memcard-names serial --exe "${serial_exe}" --test-card-write slot1)
    if(NOT EXISTS "${exe_serial_dir}/SLUS-12345.srm" OR
       EXISTS "${exe_serial_dir}/sLuS_123.45.srm" OR
       EXISTS "${exe_serial_dir}/schwaystation.srm")
      message(FATAL_ERROR "PS-EXE serial naming did not normalize the complete source stem")
    endif()

    # The documented three-letter LSP prefix keeps the same serial grammar.
    set(lsp_serial_dir "${OUT}/runner-cards-lsp-serial")
    file(REMOVE_RECURSE "${lsp_serial_dir}")
    file(MAKE_DIRECTORY "${lsp_serial_dir}")
    set(lsp_exe "${OUT}/LSP_123.45.exe")
    file(COPY_FILE "${FIXTURES}/memcard_roundtrip.exe" "${lsp_exe}")
    expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${lsp_serial_dir}"
      --memcard-names serial --exe "${lsp_exe}" --test-card-write slot1)
    if(NOT EXISTS "${lsp_serial_dir}/LSP-12345.srm")
      message(FATAL_ERROR "three-letter LSP serial prefix was not normalized")
    endif()

    # An explicit override remains literal even when its prefix is reserved for
    # automatic digest identities.
    set(literal_identity_dir "${OUT}/runner-cards-literal-identity")
    file(MAKE_DIRECTORY "${literal_identity_dir}")
    expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${literal_identity_dir}"
      --memcard-names serial --serial sGi1-literal --exe "${serial_exe}" --test-card-write slot1)
    file(GLOB literal_identity_files LIST_DIRECTORIES FALSE "${literal_identity_dir}/*.srm")
    set(expected_literal_identity_files "${literal_identity_dir}/sGi1-literal.srm")
    if(NOT literal_identity_files STREQUAL expected_literal_identity_files)
      message(FATAL_ERROR "literal serial override was normalized or treated as an automatic digest")
    endif()

    # A serial-shaped-but-invalid source stem falls back to a framed digest of
    # all executable bytes; moving the same bytes cannot change that identity.
    set(digest_dir "${OUT}/runner-cards-exe-digest")
    file(REMOVE_RECURSE "${digest_dir}")
    file(MAKE_DIRECTORY "${digest_dir}" "${OUT}/renamed")
    set(homebrew_exe "${OUT}/homebrew.exe")
    set(invalid_serial_exe "${OUT}/renamed/slus_123.4.exe")
    file(COPY_FILE "${FIXTURES}/memcard_roundtrip.exe" "${homebrew_exe}")
    file(COPY_FILE "${FIXTURES}/memcard_roundtrip.exe" "${invalid_serial_exe}")
    expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${digest_dir}"
      --memcard-names serial --exe "${homebrew_exe}" --test-card-write slot1)
    set(exe_digest "sgi1-115696bfa2b40f832c731a520fe237039582ad768916dfaaa207737f285378cd")
    if(NOT EXISTS "${digest_dir}/${exe_digest}.srm")
      message(FATAL_ERROR "missing complete framed executable digest card path")
    endif()
    expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${digest_dir}"
      --memcard-names serial --exe "${invalid_serial_exe}" --test-card-write slot1)
    file(GLOB digest_card_files LIST_DIRECTORIES FALSE "${digest_dir}/*.srm" "${digest_dir}/*.mcr")
    set(expected_digest_card_files "${digest_dir}/${exe_digest}.srm")
    if(NOT digest_card_files STREQUAL expected_digest_card_files OR
       EXISTS "${digest_dir}/homebrew.srm" OR EXISTS "${digest_dir}/slus_123.4.srm")
      message(FATAL_ERROR "executable paths changed the automatic digest identity: ${digest_card_files}")
    endif()

    # Accepted trailing bytes participate in the same full-file digest frame.
    set(tail_exe "${OUT}/homebrew-tail.exe")
    file(COPY_FILE "${homebrew_exe}" "${tail_exe}")
    file(APPEND "${tail_exe}" "tail-change")
    expect_exit(0 "frame 1 sha256 ${digits}" --frames 1 --hash --save-dir "${digest_dir}"
      --memcard-names serial --exe "${tail_exe}" --test-card-write slot2)
    set(tail_digest "sgi1-1cbe12ac81973779678ada3e9839e44d792f84d8359c48c7b83ffd7ce898d314")
    if(NOT EXISTS "${digest_dir}/${tail_digest}.1.mcr" OR
       EXISTS "${digest_dir}/${tail_digest}.srm")
      message(FATAL_ERROR "accepted executable tail bytes did not change the full content identity")
    endif()

    # A forced atomic replacement failure returns status 1, preserves the old card, and cleans its temp.
    set(failed_save_dir "${OUT}/runner-cards-failed-save")
    file(MAKE_DIRECTORY "${failed_save_dir}")
    file(WRITE "${failed_save_dir}/schwaystation.srm" "durable-before-failure")
    file(SHA256 "${failed_save_dir}/schwaystation.srm" old_save)
    expect_exit(1 "cannot save card file" --frames 1 --save-dir "${failed_save_dir}"
      --test-card-write slot1 --test-save-failure rename)
    file(SHA256 "${failed_save_dir}/schwaystation.srm" new_save)
    file(GLOB temporary_files "${failed_save_dir}/*.schwaystation.tmp.*")
    if(NOT old_save STREQUAL new_save OR temporary_files)
      message(FATAL_ERROR "a failed atomic replacement damaged the old card or left a temporary file")
    endif()

    # A missing save directory is not created and cannot produce a successful dirty-card exit.
    set(no_directory "${OUT}/runner-cards-no-directory")
    file(REMOVE_RECURSE "${no_directory}")
    expect_exit(1 "cannot save card file" --frames 1 --save-dir "${no_directory}"
      --test-card-write slot1)
    if(EXISTS "${no_directory}")
      message(FATAL_ERROR "the runner created a save directory that the caller did not provide")
    endif()

    # Invalid naming options are refused as usage errors before any run.
    refuse(--serial SLUS_123.45)
    refuse(--save-dir "${serial_dir}" --memcard-names serial --serial ../escape)
    refuse(--save-dir "${serial_dir}" --memcard-names serial)
  elseif(CASE STREQUAL "audio_failures")
    set(RUN ${HOOKED})
    execute_process(COMMAND ${EMULATOR} ${RUN} --frames 1 --hash-audio --test-audio-failure
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT rc STREQUAL "1" OR NOT err MATCHES "schway_get_audio failed")
      message(FATAL_ERROR "audio drain failure: expected exit 1 and its error, got ${rc}\n${out}${err}")
    endif()
    refuse(--frames 1 --hash-audio --hash-audio)
    if(DEFINED FIXTURES)
      refuse(--probe "${FIXTURES}/spu_tone.exe" --hash-audio)
    endif()

    string(REPEAT "[0-9a-f]" 64 digits)
    execute_process(COMMAND ${EMULATOR} ${RUN} --frames 1 --hash --hash-audio --hash-state --hash-vram
      RESULT_VARIABLE combined_rc OUTPUT_VARIABLE combined_out ERROR_VARIABLE combined_err)
    string(REGEX MATCH "frame 1 sha256 ${digits}" frame_line "${combined_out}")
    string(REGEX MATCH "audio sha256 ${digits}" audio_line "${combined_out}")
    string(REGEX MATCH "state sha256 ${digits}" state_line "${combined_out}")
    string(REGEX MATCH "vram sha256 ${digits}" vram_line "${combined_out}")
    string(FIND "${combined_out}" "${frame_line}" frame_position)
    string(FIND "${combined_out}" "${audio_line}" audio_position)
    string(FIND "${combined_out}" "${state_line}" state_position)
    string(FIND "${combined_out}" "${vram_line}" vram_position)
    if(NOT combined_rc STREQUAL "0" OR frame_line STREQUAL "" OR audio_line STREQUAL "" OR
       state_line STREQUAL "" OR vram_line STREQUAL "" OR audio_position LESS_EQUAL frame_position OR
       state_position LESS_EQUAL audio_position OR vram_position LESS_EQUAL state_position)
      message(FATAL_ERROR "combined hashes: expected four ordered digests and exit 0, got ${combined_rc}\n${combined_out}${combined_err}")
    endif()
  elseif(CASE STREQUAL "audio_hash")
    unit(audio_hash)
    set(silent_one_frame_hash 2b8443142803ae42226c9b3c4a1092587cb5edfa61308925ff5f55ff02bedc57)
    expect_exit(0 "^audio sha256 ${silent_one_frame_hash}\n$" --frames 1 --hash-audio)

    if(DEFINED FIXTURES)
      execute_process(COMMAND ${EMULATOR} ${RUN} --exe "${FIXTURES}/spu_tone.exe"
        --bench 620 --stop-on mailbox --hash-audio RESULT_VARIABLE partial_rc OUTPUT_VARIABLE partial_out
        ERROR_VARIABLE partial_err)
      string(REGEX MATCH "audio sha256 ([0-9a-f]+)" partial_line "${partial_out}")
      set(partial_hash "${CMAKE_MATCH_1}")
      string(REGEX MATCH "frames ([0-9]+)" partial_frames_line "${partial_out}")
      set(partial_frames "${CMAKE_MATCH_1}")
      if(NOT partial_rc STREQUAL "0" OR partial_line STREQUAL "" OR partial_frames STREQUAL "")
        message(FATAL_ERROR "partial tone stream: expected exit 0 and an audio hash, got ${partial_rc}\n${partial_out}${partial_err}")
      endif()

      execute_process(COMMAND ${EMULATOR} ${RUN} --exe "${FIXTURES}/spu_tone.exe"
        --frames "${partial_frames}" --hash-audio RESULT_VARIABLE tone_rc OUTPUT_VARIABLE tone_out
        ERROR_VARIABLE tone_err)
      string(REGEX MATCH "audio sha256 ([0-9a-f]+)" tone_line "${tone_out}")
      set(tone_hash "${CMAKE_MATCH_1}")
      set(tone_golden c07146eb20b62452e0dc86bac5bb26490cc130097e4a814ce24a28f3a34349f9)
      if(NOT tone_rc STREQUAL "0" OR tone_line STREQUAL "" OR NOT tone_hash STREQUAL tone_golden)
        message(FATAL_ERROR "tone stream: expected exit 0 and the pinned PCM hash, got ${tone_rc}\n${tone_out}${tone_err}")
      endif()
      if(NOT partial_hash STREQUAL tone_hash)
        message(FATAL_ERROR "mailbox stop changed the PCM prefix hash (${partial_hash} != ${tone_hash})")
      endif()

      execute_process(COMMAND ${EMULATOR} ${RUN} --frames "${partial_frames}" --hash-audio
        RESULT_VARIABLE silent_rc OUTPUT_VARIABLE silent_out ERROR_VARIABLE silent_err)
      string(REGEX MATCH "audio sha256 ([0-9a-f]+)" silent_line "${silent_out}")
      set(silent_hash "${CMAKE_MATCH_1}")
      if(NOT silent_rc STREQUAL "0" OR silent_line STREQUAL "")
        message(FATAL_ERROR "silent stream: expected exit 0 and an audio hash, got ${silent_rc}\n${silent_out}${silent_err}")
      endif()
      if(silent_hash STREQUAL tone_hash)
        message(FATAL_ERROR "tone and silent runs produced the same PCM hash (${tone_hash})")
      endif()
    endif()
  elseif(CASE STREQUAL "hash_state")
    # One well-formed line, the same twice, and a different one a frame later. CMake regexes have no
    # {64}, so the sixty-four digits are repeated.
    string(REPEAT "[0-9a-f]" 64 digits)
    foreach(frames 5 5 6)
      execute_process(COMMAND ${EMULATOR} ${RUN} --frames ${frames} --hash-state RESULT_VARIABLE rc
        OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "^state sha256 ${digits}\n$")
        message(FATAL_ERROR "--frames ${frames} --hash-state: expected exit 0 and one 64-digit line, "
          "got exit ${rc}\n${out}${err}")
      endif()
      list(APPEND states "${out}")
    endforeach()
    list(GET states 0 first)
    list(GET states 1 second)
    list(GET states 2 third)
    if(NOT first STREQUAL second OR first STREQUAL third)
      message(FATAL_ERROR "the state hash is not stable for the same run or not different for another:\n"
        "${first}${second}${third}")
    endif()
  elseif(CASE STREQUAL "inputs_script")
    file(WRITE "${OUT}/runner-empty.inputs" "")
    file(WRITE "${OUT}/runner-button.inputs" "2 p1=dualshock:X:128,128,128,128 p2=.\n")
    string(REPEAT "[0-9a-f]" 64 digits)
    function(run_movie frames movie result_name)
      set(movie_arg)
      if(NOT movie STREQUAL "none")
        set(movie_arg --inputs "${movie}")
      endif()
      execute_process(COMMAND ${EMULATOR} ${RUN} --frames ${frames} --hash --hash-state
        ${movie_arg} ${ARGN}
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "frame ${frames} sha256 ${digits}" OR
         NOT out MATCHES "state sha256 ${digits}")
        message(FATAL_ERROR "--frames ${frames} input '${movie}': expected exit 0 and frame/state hashes, "
          "got exit ${rc}\\n${out}${err}")
      endif()
      string(REGEX MATCH "frame ${frames} sha256 ([0-9a-f]+)" frame_line "${out}")
      set(frame_hash "${CMAKE_MATCH_1}")
      string(REGEX MATCH "state sha256 ([0-9a-f]+)" state_line "${out}")
      set(state_hash "${CMAKE_MATCH_1}")
      set(${result_name}_frame "${frame_hash}" PARENT_SCOPE)
      set(${result_name}_state "${state_hash}" PARENT_SCOPE)
      set(${result_name}_output "${out}" PARENT_SCOPE)
    endfunction()
    run_movie(1 none no_movie_before)
    run_movie(1 "${OUT}/runner-button.inputs" event_before)
    run_movie(2 none no_movie_after)
    run_movie(2 "${OUT}/runner-empty.inputs" empty_movie)
    run_movie(2 "${OUT}/runner-button.inputs" event_after)
    run_movie(2 "${OUT}/runner-button.inputs" event_repeat)
    run_movie(2 "${OUT}/runner-empty.inputs" multitap_empty --multitap-port1 enabled)
    run_movie(2 "${OUT}/runner-button.inputs" multitap_button --multitap-port1 enabled)
    if(NOT no_movie_before_frame STREQUAL event_before_frame OR
       NOT no_movie_before_state STREQUAL event_before_state)
      message(FATAL_ERROR "an input scheduled for frame 2 changed the frame-1 state")
    endif()
    if(NOT no_movie_after_frame STREQUAL empty_movie_frame OR
       NOT no_movie_after_state STREQUAL empty_movie_state)
      message(FATAL_ERROR "an empty input movie changed the no-input run")
    endif()
    if(NOT no_movie_after_frame STREQUAL event_after_frame)
      message(FATAL_ERROR "input changed the content-free frame hash")
    endif()
    if(no_movie_after_state STREQUAL event_after_state)
      message(FATAL_ERROR "the event-frame input did not change the saved latched state")
    endif()
    if(multitap_empty_state STREQUAL multitap_button_state)
      message(FATAL_ERROR "legacy p1 movie input did not change socket A state with port 1 multitap")
    endif()
    if(NOT event_after_output STREQUAL event_repeat_output)
      message(FATAL_ERROR "replaying the same input movie changed its hashes")
    endif()
  elseif(CASE STREQUAL "inputs_controls")
    string(REPEAT "[0-9a-f]" 64 digits)
    function(accept_input_movie name contents frames result_name)
      set(path "${OUT}/runner-input-${name}.txt")
      file(CONFIGURE OUTPUT "${path}" NEWLINE_STYLE UNIX CONTENT "${contents}")
      execute_process(COMMAND ${EMULATOR} ${RUN} --frames ${frames} --hash --hash-state --inputs "${path}"
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "frame ${frames} sha256 ${digits}" OR
         NOT out MATCHES "state sha256 ${digits}")
        message(FATAL_ERROR "valid input '${name}': expected exit 0 and frame/state hashes, "
          "got exit ${rc}\n${out}${err}")
      endif()
      set(${result_name} "${out}" PARENT_SCOPE)
    endfunction()
    function(refuse_input_movie name contents)
      set(path "${OUT}/runner-input-${name}.txt")
      file(CONFIGURE OUTPUT "${path}" NEWLINE_STYLE UNIX CONTENT "${contents}")
      execute_process(COMMAND ${EMULATOR} ${RUN} --frames 1 --hash --hash-state --inputs "${path}"
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
      if(NOT rc STREQUAL "2" OR NOT out STREQUAL "" OR NOT err MATCHES "invalid input movie" OR
         NOT err MATCHES "usage")
        message(FATAL_ERROR "invalid input '${name}': expected bounded usage refusal before output, "
          "got exit ${rc}\n${out}${err}")
      endif()
    endfunction()
    string(CONCAT endpoints "# controller endpoints\r\n"
      "1000000 p1=dualshock:.:0,255,0,255 p2=.\r\n"
      "1 p1=dualshock:UDLRSsTOXQ1234<>M:0,255,0,255 p2=digital:UDLRSsTOXQ1234:128,128,128,128 # frame one\r\n")
    accept_input_movie(endpoints "${endpoints}" 1 endpoints_output)
    string(CONCAT duplicate "2 p1=dualshock:X:128,128,128,128 p2=.\n"
      "2 p1=dualshock:X:128,128,128,128 p2=.\n")
    accept_input_movie(duplicate "${duplicate}" 2 duplicate_output)
    if(NOT duplicate_output MATCHES "frame 2 sha256 ${digits}")
      message(FATAL_ERROR "an identical same-frame record was not accepted")
    endif()
    refuse_input_movie(frame_zero "0 p1=. p2=.\n")
    refuse_input_movie(frame_over "1000001 p1=. p2=.\n")
    refuse_input_movie(frame_overflow "4294967295 p1=. p2=.\n")
    refuse_input_movie(port_zero "1 p0=. p2=.\n")
    refuse_input_movie(port_three "1 p1=. p3=.\n")
    refuse_input_movie(port_order "1 p2=. p1=.\n")
    refuse_input_movie(device "1 p1=analog:.:128,128,128,128 p2=.\n")
    refuse_input_movie(button_symbol "1 p1=digital:Z:128,128,128,128 p2=.\n")
    refuse_input_movie(button_repeat "1 p1=digital:XX:128,128,128,128 p2=.\n")
    string(ASCII 233 non_ascii)
    refuse_input_movie(non_ascii "1 p1=digital:${non_ascii}:128,128,128,128 p2=.\n")
    refuse_input_movie(axis_low "1 p1=dualshock:.:-1,128,128,128 p2=.\n")
    refuse_input_movie(axis_high "1 p1=dualshock:.:256,128,128,128 p2=.\n")
    refuse_input_movie(axis_encoding "1 p1=dualshock:.:1x,128,128,128 p2=.\n")
    refuse_input_movie(missing_axis "1 p1=dualshock:.:128,128,128 p2=.\n")
    string(CONCAT conflicting_frame "2 p1=dualshock:X:128,128,128,128 p2=.\n"
      "2 p1=dualshock:.:128,128,128,128 p2=.\n")
    refuse_input_movie(conflicting_frame "${conflicting_frame}")
    string(CONCAT late_invalid "1 p1=dualshock:X:128,128,128,128 p2=.\n"
      "2 p1=dualshock:.:999,128,128,128 p2=.\n")
    refuse_input_movie(late_invalid "${late_invalid}")
    refuse(--frames 1 --inputs)
    refuse(--frames 1 --inputs "${OUT}/runner-input-endpoints.txt" --inputs "${OUT}/runner-input-endpoints.txt")
    expect_exit(1 "cannot read input movie" --frames 1 --inputs "${OUT}/runner-input-missing.txt")
  elseif(CASE STREQUAL "no_args")
    refuse()
  elseif(CASE STREQUAL "unknown")
    # An option this release does not have, and --help or --version with
    # anything else.
    refuse(--frames 1 --disc x.cue)
    refuse(--frames 1 --help)
    refuse(--version --frames 1)
  elseif(CASE STREQUAL "exe")
    # --exe takes one file, never with --probe, and --tty needs it.
    refuse(--exe)
    refuse(--frames 1 --exe)
    refuse(--exe --hash)
    refuse(--exe a.exe --exe b.exe)
    refuse(--exe a.exe --probe b.exe)
    refuse(--probe b.exe --exe a.exe)
    refuse(--tty)
    refuse(--frames 1 --tty)
    refuse(--bench 1 --tty)
    # --stop-on takes mailbox, once, and needs --exe.
    refuse(--stop-on mailbox)
    refuse(--frames 1 --stop-on mailbox)
    refuse(--exe a.exe --stop-on)
    refuse(--exe a.exe --stop-on exit)
    refuse(--exe a.exe --stop-on mailbox --stop-on mailbox)
    # --trace takes first-fault, once, and needs --exe; --no-stop-on-fault needs --exe.
    refuse(--trace first-fault)
    refuse(--frames 1 --trace first-fault)
    refuse(--exe a.exe --trace)
    refuse(--exe a.exe --trace cpu)
    refuse(--exe a.exe --trace first-fault --trace first-fault)
    refuse(--no-stop-on-fault)
    refuse(--frames 1 --no-stop-on-fault)
  elseif(CASE STREQUAL "exe_missing")
    # A file that is not there, or is not a PS-EXE, exits 1 with a generic
    # reason; private file paths are not echoed and no frame runs.
    expect_exit(1 "schwaystation-run: input: cannot (open|read)" --exe "${OUT}/missing.exe")
    expect_exit(1 "schwaystation-run: input: PS-EXE invalid" --exe "${CMAKE_CURRENT_LIST_FILE}"
      --tty)
  elseif(CASE STREQUAL "positional")
    # This release loads no disc, so a file name is a usage error.
    refuse(--frames 1 game.cue)
    refuse(game.cue)
  elseif(CASE STREQUAL "frame_range")
    refuse(--frames 0)
    refuse(--frames 1000001)
    refuse(--frames 4294967296)
    refuse(--frames -1)
    refuse(--frames 1x)
    refuse(--frames)
    refuse(--hash --frames)
    refuse(--frames 1 --frames 1)
  elseif(CASE STREQUAL "png_syntax")
    # Each malformed or out-of-range --png is refused, whichever order the
    # options come in, and writes nothing.
    file(REMOVE "${OUT}")
    refuse(--frames 5 --png)
    refuse(--frames 5 --png "${OUT}")
    refuse(--frames 5 --png "5:")
    refuse(--frames 5 --png ":${OUT}")
    refuse(--frames 5 --png "0:${OUT}")
    refuse(--frames 5 --png "x:${OUT}")
    refuse(--frames 5 --png "6:${OUT}")
    refuse(--png "6:${OUT}" --frames 5)
    refuse(--frames 5 --png "5:${OUT}" --png "5:${OUT}")
    if(EXISTS "${OUT}")
      message(FATAL_ERROR "a refused --png wrote ${OUT}")
    endif()
  elseif(CASE STREQUAL "region")
    refuse(--frames 1 --region)
    refuse(--frames 1 --region secam)
    refuse(--frames 1 --region NTSC-U)
    refuse(--frames 1 --region ntsc-u --region pal)
  elseif(CASE STREQUAL "machine")
    refuse(--frames 1 --machine)
    refuse(--frames 1 --machine unknown)
    refuse(--frames 1 --machine pu7 --machine pu18)
    foreach(board IN ITEMS pu7 pu8-early pu8-late pu18 pu20 pu22 pu23 pm41 pm41-2)
      execute_process(COMMAND ${EMULATOR} ${RUN} --frames 1 --region ntsc-u --machine ${board} --hash-state
        RESULT_VARIABLE board_rc OUTPUT_VARIABLE board_out ERROR_VARIABLE board_err)
      if(NOT board_rc STREQUAL "0" OR NOT board_out MATCHES "state sha256 [0-9a-f]+")
        message(FATAL_ERROR "documented machine token ${board} or its GPU pairing was rejected:\n"
          "exit ${board_rc}\n${board_out}${board_err}")
      endif()
    endforeach()
  elseif(CASE STREQUAL "region_auto")
    if(NOT DEFINED BIOS OR NOT EXISTS "${BIOS}" OR
       NOT DEFINED PYTHON3 OR PYTHON3 STREQUAL "")
      message(FATAL_ERROR "auto region test needs the checked BIOS and configured Python 3")
    endif()
    execute_process(COMMAND ${EMULATOR} ${RUN} --frames 1 --hash-state --region auto
      RESULT_VARIABLE auto_fallback_rc OUTPUT_VARIABLE auto_fallback_out ERROR_VARIABLE auto_fallback_err)
    execute_process(COMMAND ${EMULATOR} ${RUN} --frames 1 --hash-state --region ntsc-u
      RESULT_VARIABLE ntsc_u_rc OUTPUT_VARIABLE ntsc_u_out ERROR_VARIABLE ntsc_u_err)
    if(NOT auto_fallback_rc STREQUAL "0" OR NOT ntsc_u_rc STREQUAL "0" OR
       NOT auto_fallback_out STREQUAL ntsc_u_out)
      message(FATAL_ERROR "no-hint auto region must retain NTSC-U:\n"
        "auto ${auto_fallback_rc}\n${auto_fallback_out}${auto_fallback_err}\n"
        "NTSC-U ${ntsc_u_rc}\n${ntsc_u_out}${ntsc_u_err}")
    endif()
    file(MAKE_DIRECTORY "${OUT}")
    set(patch_sector4 [==[
from pathlib import Path
import sys
image = bytearray(Path(sys.argv[1]).read_bytes())
start = 4 * 2352 + 24
if start + 2048 > len(image):
    raise SystemExit("disc fixture is too short for sector 4")
sector = bytearray(2048)
marker = b"          Licensed  by          Sony Computer Entertainment Euro pe   "
sector[:len(marker)] = marker
image[start:start + 2048] = sector
Path(sys.argv[2]).write_bytes(image)
]==])
    execute_process(COMMAND "${PYTHON3}" -c "${patch_sector4}"
        "${FIXTURES}/disc_boot.bin" "${OUT}/disc_boot_region.bin"
      RESULT_VARIABLE patch_rc OUTPUT_VARIABLE patch_out ERROR_VARIABLE patch_err)
    if(NOT patch_rc STREQUAL "0")
      message(FATAL_ERROR "could not create the auto-region disc fixture: ${patch_out}${patch_err}")
    endif()
    file(COPY_FILE "${FIXTURES}/disc_boot_track02.bin" "${OUT}/disc_boot_region_track02.bin")
    file(WRITE "${OUT}/disc_boot_region.cue"
      "FILE \"disc_boot_region.bin\" BINARY\n"
      "  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n"
      "FILE \"disc_boot_region_track02.bin\" BINARY\n"
      "  TRACK 02 AUDIO\n    INDEX 00 00:00:00\n    INDEX 01 00:02:00\n")
    execute_process(COMMAND ${EMULATOR} ${RUN} --bios "${BIOS}" --disc "${OUT}/disc_boot_region.cue"
        --frames 1 --hash --region auto
      RESULT_VARIABLE auto_rc OUTPUT_VARIABLE auto_out ERROR_VARIABLE auto_err)
    execute_process(COMMAND ${EMULATOR} ${RUN} --bios "${BIOS}" --disc "${OUT}/disc_boot_region.cue"
        --frames 1 --hash --region ntsc-j
      RESULT_VARIABLE override_rc OUTPUT_VARIABLE override_out ERROR_VARIABLE override_err)
    if(NOT auto_rc STREQUAL "0" OR NOT override_rc STREQUAL "0" OR
       NOT auto_err MATCHES "region hint disagreement:.*disc=pal.*exe=ntsc-u; selected=pal" OR
       NOT override_err MATCHES "region hint disagreement:.*disc=pal.*exe=ntsc-u; selected=ntsc-j" OR
       auto_out STREQUAL override_out)
      message(FATAL_ERROR "auto or explicit region selection lost precedence:\n"
        "auto ${auto_rc}\n${auto_out}${auto_err}\n"
        "override ${override_rc}\n${override_out}${override_err}")
    endif()
  elseif(CASE STREQUAL "hash_machine")
    execute_process(COMMAND ${EMULATOR} ${RUN} --frames 1 --region ntsc-u --machine pu7 --hash-state
      RESULT_VARIABLE same_a_rc OUTPUT_VARIABLE same_a_out ERROR_VARIABLE same_a_err)
    execute_process(COMMAND ${EMULATOR} ${RUN} --frames 1 --region ntsc-u --machine pu7 --hash-state
      RESULT_VARIABLE same_b_rc OUTPUT_VARIABLE same_b_out ERROR_VARIABLE same_b_err)
    execute_process(COMMAND ${EMULATOR} ${RUN} --frames 1 --region ntsc-u --machine pu18 --hash-state
      RESULT_VARIABLE other_rc OUTPUT_VARIABLE other_out ERROR_VARIABLE other_err)
    if(NOT same_a_rc STREQUAL "0" OR NOT same_b_rc STREQUAL "0" OR
       NOT other_rc STREQUAL "0" OR NOT same_a_out STREQUAL same_b_out OR
       same_a_out STREQUAL other_out OR NOT same_a_out MATCHES "state sha256 [0-9a-f]+")
      message(FATAL_ERROR "machine profiles did not hash deterministically and distinctly:\n"
        "pu7 A ${same_a_rc}\n${same_a_out}${same_a_err}\n"
        "pu7 B ${same_b_rc}\n${same_b_out}${same_b_err}\n"
        "pu18 ${other_rc}\n${other_out}${other_err}")
    endif()
  elseif(CASE STREQUAL "png_file")
    # The decode check runs first, so the pinned digest is only ever
    # compared for a file whose IHDR and every pixel are already known good.
    file(REMOVE "${OUT}")
    expect_exit(0 "^$" --frames 5 --png "5:${OUT}")
    execute_process(COMMAND ${EMULATOR} ${DECODE} png_file "${OUT}" RESULT_VARIABLE rc
      OUTPUT_VARIABLE out ERROR_VARIABLE out)
    if(NOT rc STREQUAL "0")
      message(FATAL_ERROR "the PNG does not decode to the card (exit ${rc})\n${out}")
    endif()
    file(SHA256 "${OUT}" sha256)
    if(NOT sha256 STREQUAL EXPECTED)
      message(FATAL_ERROR "PNG SHA-256 mismatch: expected ${EXPECTED} got ${sha256}")
    endif()
  else()
    message(FATAL_ERROR "unknown CASE '${CASE}'")
  endif()
  return()
endif()

# Every command that starts the runner goes through the build's
# CMAKE_CROSSCOMPILING_EMULATOR, so an emulated lane runs its own guest binary.
set(runner_cmd ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-run>)

# Fault prevented: the runner's printed card hash, the line hosts and CI
# compare, drifts from the pinned card or differs on another platform.
schway_add_test(NAME runner.hash
  COMMAND ${runner_cmd} --frames 60 --hash
  PASS_REGEX "frame 60 sha256 ${SCHWAYSTATION_TESTCARD_SHA256}"
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a card that does not move, so the frame hash cannot tell
# frame 5 from frame 60.
schway_add_test(NAME runner.hash.frame5
  COMMAND ${runner_cmd} --frames 5 --hash
  PASS_REGEX "frame 5 sha256 ${SCHWAYSTATION_TESTCARD_SHA256_FRAME5}"
  LABELS portable runner TIMEOUT 30)

# Fault prevented: --region not reaching the frame the host sees.
schway_add_test(NAME runner.hash.ntsc_j
  COMMAND ${runner_cmd} --frames 60 --hash --region ntsc-j
  PASS_REGEX "frame 60 sha256 ${SCHWAYSTATION_TESTCARD_SHA256_NTSC_J}"
  LABELS portable runner TIMEOUT 30)
schway_add_test(NAME runner.hash.pal
  COMMAND ${runner_cmd} --frames 60 --hash --region pal
  PASS_REGEX "frame 60 sha256 ${SCHWAYSTATION_TESTCARD_SHA256_PAL}"
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a lane whose hashes are never compared with the others.
# Writes build/<preset>/hashes.txt, the file CI collects from every lane and
# compares byte for byte.
# run 43671490171: macOS x64 hit 120 s; run 43665013436 passed in 105.81 s. Raise to 240 s.
string(REPLACE ";" "$<SEMICOLON>" runner_emulator "${CMAKE_CROSSCOMPILING_EMULATOR}")
set(runner_hashes ${PROJECT_SOURCE_DIR}/tests/cmake/hashes.cmake)
set(runner_fixtures_arg "")
if(SCHWAYSTATION_FIXTURES)
  set(runner_fixtures_arg -DFIXTURES=${PROJECT_BINARY_DIR}/fixtures)
  cmake_language(DEFER CALL set_property TEST runner.hashes_file APPEND PROPERTY FIXTURES_REQUIRED disc_fmv_predicates)
endif()
schway_add_test(NAME runner.hashes_file
  COMMAND ${CMAKE_COMMAND} -DMODE=write -DRUN=$<TARGET_FILE:schwaystation-run>
    "-DEMULATOR=${runner_emulator}" ${runner_fixtures_arg} -DOUT=${PROJECT_BINARY_DIR}
    -P ${runner_hashes}
  LABELS portable runner TIMEOUT 240)

# Fault prevented: a comparison that passes lanes which differ, or a lane
# that silently drops out. All eight files must match, a missing eighth must
# fail, and a different eighth must fail even when the first seven match.
set(runner_fixture ${PROJECT_BINARY_DIR}/tests/hashes)
set(runner_files "")
foreach(lane linux-x64 linux-arm64 macos-x64 macos-arm64 windows-x64 windows-arm64 be32 wasm32)
  file(WRITE ${runner_fixture}/${lane}.txt "frame 60 sha256 00\n")
  list(APPEND runner_files ${runner_fixture}/${lane}.txt)
endforeach()
string(REPLACE ";" "$<SEMICOLON>" runner_compare_files "${runner_files}")
schway_add_test(NAME runner.hashes_compare.eight
  COMMAND ${CMAKE_COMMAND} "-DFILES=${runner_compare_files}" -DCOUNT=8 -DMODE=compare -P ${runner_hashes}
  LABELS runner TIMEOUT 10)
list(POP_BACK runner_files)
file(WRITE ${runner_fixture}/b.txt "frame 60 sha256 01\n")
# CMake wraps long messages, so the regexes allow a break between words.
foreach(case IN ITEMS "count;;expected 8 files,[^/]*got 7" "byte;b.txt;b.txt[^/]*differs")
  list(GET case 0 name)
  list(GET case 1 eighth)
  list(GET case 2 regex)
  set(files ${runner_files})
  if(eighth)
    list(APPEND files ${runner_fixture}/${eighth})
  endif()
  string(REPLACE ";" "\\;" files "${files}")
  set(cmd ${CMAKE_COMMAND} "-DFILES=${files}" -DCOUNT=8 -DMODE=compare -P ${runner_hashes})
  string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
  schway_add_test(NAME runner.hashes_compare.${name}.self_test
    COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=${regex}"
      -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
    LABELS runner TIMEOUT 10)
endforeach()

# The PNG encoder and its checksums, checked by an independent parse, and the
# shared converter's known answers. The same program decodes the runner's PNG
# for runner.png_file.
add_executable(schwaystation-test-runner
  ${CMAKE_CURRENT_LIST_DIR}/test_png.c
  ${PROJECT_SOURCE_DIR}/runner/png.c
  ${PROJECT_SOURCE_DIR}/runner/crc32.c
  ${PROJECT_SOURCE_DIR}/runner/frame_hash.c
  ${PROJECT_SOURCE_DIR}/runner/sha256.c)
target_include_directories(schwaystation-test-runner PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-runner PRIVATE schwaystation::schwaystation
  schwaystation-host)
set_target_properties(schwaystation-test-runner PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-runner)

add_executable(schwaystation-test-cheats ${CMAKE_CURRENT_LIST_DIR}/cheats.c)
target_link_libraries(schwaystation-test-cheats PRIVATE
  schwaystation::schwaystation schwaystation-host)
set_target_properties(schwaystation-test-cheats PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-cheats)

# The region precedence resolver is deliberately host-only and has no runner,
# disc-image or core dependency, so the table test links only this module.
add_executable(schwaystation-test-region-probe
  ${CMAKE_CURRENT_LIST_DIR}/test_region_probe.c
  ${PROJECT_SOURCE_DIR}/runner/region.c)
target_include_directories(schwaystation-test-region-probe PRIVATE
  ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/generated ${PROJECT_SOURCE_DIR}/runner)
set_target_properties(schwaystation-test-region-probe PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-region-probe)

# Fault prevented: an encoder whose chunk CRCs, Adler-32 or stored blocks are
# wrong, so a viewer or the RetroArch compare rejects or misreads the file.
schway_add_test(NAME runner.png
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-runner> png
  LABELS portable runner TIMEOUT 10)

# Fault prevented: a wrong 15-bit to XRGB8888 rule. The runner's PNG and the
# libretro core share the converter, so comparing them cannot catch it.
schway_add_test(NAME runner.xrgb8888
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-runner> xrgb8888
  LABELS portable runner TIMEOUT 10)

foreach(case rgb24 video_bounds native_hash)
  schway_add_test(NAME runner.${case}
    COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-runner> ${case}
    LABELS portable runner TIMEOUT 10)
endforeach()

string(REPLACE ";" "|" runner_emulator_bars "${CMAKE_CROSSCOMPILING_EMULATOR}")
set(runner_check ${CMAKE_COMMAND} -DRUN=$<TARGET_FILE:schwaystation-run>
  "-DEMULATOR=${runner_emulator_bars}")

set(runner_disc_playlist ${PROJECT_BINARY_DIR}/fixtures/disc_boot_runner.m3u)
# Fault prevented: board identity is omitted from or destabilizes the saved state hash.
schway_add_test(NAME runner.hash.machine
  COMMAND ${runner_check} -DCASE=hash_machine -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a platform-native newline changes a generated fixture hash
# between CI lanes even though each lane parses the same playlist entries.
file(CONFIGURE OUTPUT "${runner_disc_playlist}" CONTENT "disc_boot.cue\ndisc_boot_playlist.cue\n"
  NEWLINE_STYLE UNIX)
set(runner_disc_input_script ${PROJECT_BINARY_DIR}/tests/runner-disc-controls.inputs)
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/tests")
file(WRITE "${runner_disc_input_script}"
  "300 p1=digital:X:128,128,128,128 p2=.\n300 lid=open\n300 disc=1\n300 lid=closed\n")
if(TARGET schwaystation-disc-fixtures)
  add_dependencies(schwaystation-test-runner-disc schwaystation-disc-fixtures)
endif()

# The API probe proves a closed-lid rejection leaves the copied host TOC intact,
# and that an open-lid swap installs the second provider descriptor.
schway_add_test(NAME runner.disc.api
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-runner-disc>
    ${runner_disc_playlist}
  LABELS portable runner TIMEOUT 15)

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/tests/runner-disc-chd)
file(WRITE ${PROJECT_BINARY_DIR}/tests/runner-disc-chd/invalid.chd "not a CHD image\n")
# Fault prevented: CHD callbacks or TOC differ from equivalent CUE sectors,
# or a closed-lid / rejected provider operation mutates the selected disc.
schway_add_test(NAME runner.disc.api.chd
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-runner-disc>
    chd ${PROJECT_BINARY_DIR}/fixtures/disc_boot.cue
    ${PROJECT_BINARY_DIR}/fixtures/disc_boot.chd
    ${PROJECT_BINARY_DIR}/tests/runner-disc-chd/invalid.chd
    $<BOOL:${SCHWAYSTATION_CHD}>
  LABELS portable runner TIMEOUT 30)
set_tests_properties(runner.disc.api.chd PROPERTIES
  FIXTURES_REQUIRED "disc_media_preflight;compat_features_preflight")

# Fault prevented: frame-indexed script application that skips the selected boundary or fails to
# apply the controller edge, eject, playlist selection, or final lid close.
schway_add_test(NAME runner.inputs.disc_apply
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-runner-disc>
    ${runner_disc_playlist} ${runner_disc_input_script}
  LABELS portable runner TIMEOUT 15)

schway_add_test(NAME runner.disc.load
  COMMAND ${runner_check} -DCASE=disc_load -DBIOS=${PROJECT_BINARY_DIR}/openbios/openbios-fast.bin
    -DFIXTURES=${PROJECT_BINARY_DIR}/fixtures -DOUT=${PROJECT_BINARY_DIR}/tests
    -DHOOKED=$<TARGET_FILE:schwaystation-run-test> -DPYTHON3=${Python3_EXECUTABLE}
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 180)

# Fault prevented: CHD probing or guest sectors disagree with equivalent CUE
# input, card identity aliases, or CHD-off builds accept an image without their
# explicit unsupported result.
schway_add_test(NAME runner.disc.chd
  COMMAND ${runner_check} -DCASE=disc_load -DCHD_ONLY=ON
    -DCHD_ENABLED=$<BOOL:${SCHWAYSTATION_CHD}>
    -DBIOS=${PROJECT_BINARY_DIR}/openbios/openbios-fast.bin
    -DFIXTURES=${PROJECT_BINARY_DIR}/fixtures -DOUT=${PROJECT_BINARY_DIR}/tests/runner-disc-chd
    -DHOOKED=$<TARGET_FILE:schwaystation-run-test> -DPYTHON3=${Python3_EXECUTABLE}
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 180)
set_tests_properties(runner.disc.chd PROPERTIES
  FIXTURES_REQUIRED "disc_media_preflight;compat_features_preflight")

# Fault prevented: runner PBP probing, guest sectors and save identity differ from the owned
# single-track CUE source, or malformed/unsupported input reaches guest execution.
schway_add_test(NAME runner.disc.pbp
  COMMAND ${runner_check} -DCASE=disc_pbp -DBIOS=${PROJECT_BINARY_DIR}/openbios/openbios-fast.bin
    -DFIXTURES=${PROJECT_BINARY_DIR}/fixtures -DOUT=${PROJECT_BINARY_DIR}/tests
    -DHOOKED=$<TARGET_FILE:schwaystation-run-test> -DPYTHON3=${Python3_EXECUTABLE}
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 180)
set_tests_properties(runner.disc.pbp PROPERTIES FIXTURES_REQUIRED "disc_pbp_preflight")

schway_add_test(NAME runner.disc.probe
  COMMAND ${runner_check} -DCASE=disc_probe -DFIXTURES=${PROJECT_BINARY_DIR}/fixtures
    -DOUT=${PROJECT_BINARY_DIR}/tests/runner-disc-probe -DPYTHON3=${Python3_EXECUTABLE}
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: the BIOS boot mode silently bypassing normal startup, skipping the
# shell for arbitrary or BIOS-free content, or changing the standalone EXE stub path.
foreach(mode IN ITEMS full fast invalid)
  schway_add_test(NAME runner.boot.${mode}
    COMMAND ${runner_check} -DCASE=boot_${mode}
      -DBIOS=${PROJECT_BINARY_DIR}/openbios/openbios-fast.bin
      -DFIXTURES=${PROJECT_BINARY_DIR}/fixtures -DOUT=${PROJECT_BINARY_DIR}/tests
      -DHELLO_EXE=${PROJECT_BINARY_DIR}/fixtures/hello.exe
      -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS portable runner TIMEOUT 180)
  set_tests_properties(runner.boot.${mode} PROPERTIES
    FIXTURES_REQUIRED "disc_legacy_preflight;disc_legacy_parity")
endforeach()

set(runner_disc_legacy_timeout 120)
if(SCHWAYSTATION_INVENTORY STREQUAL "asan")
  set(runner_disc_legacy_timeout 240)
elseif(SCHWAYSTATION_OS STREQUAL "macos" AND SCHWAYSTATION_ARCH STREQUAL "x64")
  set(runner_disc_legacy_timeout 240)
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "powerpc")
  # run 45086798403: QEMU-PPC timed out at the 960 s cap under guest-test contention; scale 8 makes this 32 min.
  set(runner_disc_legacy_timeout 240)
endif()
schway_add_test(NAME runner.disc.legacy
  COMMAND ${runner_check} -DCASE=disc_legacy -DBIOS=${PROJECT_BINARY_DIR}/openbios/openbios-fast.bin
    -DFIXTURES=${PROJECT_BINARY_DIR}/fixtures -DOUT=${PROJECT_BINARY_DIR}/tests
    -DHOOKED=$<TARGET_FILE:schwaystation-run-test>
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT ${runner_disc_legacy_timeout})
set_tests_properties(runner.disc.legacy PROPERTIES
  FIXTURES_REQUIRED "disc_legacy_preflight;disc_legacy_parity")

schway_add_test(NAME runner.disc.unsupported
  COMMAND ${runner_check} -DCASE=disc_unsupported
    -DCHD_ENABLED=$<BOOL:${SCHWAYSTATION_CHD}> -DBIOS=${PROJECT_BINARY_DIR}/openbios/openbios-fast.bin
    -DFIXTURES=${PROJECT_BINARY_DIR}/fixtures -DOUT=${PROJECT_BINARY_DIR}/tests
    -DPYTHON3=${Python3_EXECUTABLE}
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a state hash that is unstable, malformed or blind to the frames run, so devices
# a test-card frame cannot show are not compared across lanes.
schway_add_test(NAME runner.hash_state
  COMMAND ${runner_check} -DCASE=hash_state -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Released memory reads stay exercised while cheat parsing and ordering are
# added by later phase plans.
schway_add_test(NAME runner.cheats.memory_read
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-cheats> memory-read
  LABELS portable runner TIMEOUT 30)
foreach(case parser bounds order)
  schway_add_test(NAME runner.cheats.${case}
    COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-cheats> ${case}
    LABELS portable runner TIMEOUT 30)
endforeach()

# Fault prevented: runner options or state hashes silently claim unsupported
# patch and cheat behavior.
schway_add_test(NAME runner.cheats.cli
  COMMAND ${runner_check} -DCASE=cheats_cli -DPYTHON3=${Python3_EXECUTABLE}
    -DOUT=${PROJECT_BINARY_DIR}/tests/runner-cheats -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)
schway_add_test(NAME runner.cheats.hash
  COMMAND ${runner_check} -DCASE=cheats_hash -DOUT=${PROJECT_BINARY_DIR}/tests/runner-cheats
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)
schway_add_test(NAME runner.patch.cli
  COMMAND ${runner_check} -DCASE=patch_cli -DRUN=$<TARGET_FILE:schwaystation-run>
    -DRUN_HOOKED=$<TARGET_FILE:schwaystation-run-test>
    -DOUT=${PROJECT_BINARY_DIR}/tests/runner-patch-cli -DPYTHON3=${Python3_EXECUTABLE}
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a movie applied after the frame latch, rejected after execution starts, or
# changes the content-free frame instead of the serialized latched input state.
schway_add_test(NAME runner.inputs.script
  COMMAND ${runner_check} -DCASE=inputs_script -DOUT=${PROJECT_BINARY_DIR}/tests
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: malformed frame, port, button, axis, ordering, or encoding values reaching the core.
schway_add_test(NAME runner.inputs.controls
  COMMAND ${runner_check} -DCASE=inputs_controls -DOUT=${PROJECT_BINARY_DIR}/tests
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

schway_add_test(NAME runner.inputs.disc_controls
  COMMAND ${runner_check} -DCASE=inputs_disc_controls -DBIOS=${PROJECT_BINARY_DIR}/openbios/openbios-fast.bin
    -DFIXTURES=${PROJECT_BINARY_DIR}/fixtures -DINPUTS=${runner_disc_input_script}
    -DOUT=${PROJECT_BINARY_DIR}/tests
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 300)

# Fault prevented: the reference PNG the RetroArch screenshot is compared
# with changes bytes (converter, layout or encoder) while the frame hash
# still passes, or no longer shows the card.
schway_add_test(NAME runner.png_file
  COMMAND ${runner_check} -DCASE=png_file -DOUT=${PROJECT_BINARY_DIR}/tests/card.png
    -DDECODE=$<TARGET_FILE:schwaystation-test-runner> -DEXPECTED=${SCHWAYSTATION_TESTCARD_PNG_SHA256}
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a command line that runs something it should refuse, so a
# CI step hashes a frame nobody asked for: no arguments, an option this
# release lacks, a file name, --exe or --tty misused, a frame count out of
# range or missing, a malformed or out-of-range --png, or an unknown region.
foreach(case no_args unknown positional exe frame_range png_syntax region machine)
  schway_add_test(NAME runner.usage.${case}
    COMMAND ${runner_check} -DCASE=${case} -DOUT=${PROJECT_BINARY_DIR}/tests/refused.png
      -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS portable runner TIMEOUT 30)
endforeach()

# Fault prevented: serial, disc licence and executable hints are resolved in
# priority order while every disagreement remains visible to diagnostics.
schway_add_test(NAME runner.region.probe
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-region-probe>
  LABELS portable runner TIMEOUT 10)

# Fault prevented: default auto region or explicit override ignores resolved identity.
schway_add_test(NAME runner.region.auto
  COMMAND ${runner_check} -DCASE=region_auto -DBIOS=${PROJECT_BINARY_DIR}/openbios/openbios-fast.bin
    -DFIXTURES=${PROJECT_BINARY_DIR}/fixtures -DOUT=${PROJECT_BINARY_DIR}/tests/runner-region-auto
    -DPYTHON3=${Python3_EXECUTABLE} -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# The bench statistics and the stop flag, tested directly, and a runner built
# with a hook that raises SIGTERM from inside frame 5.
add_executable(schwaystation-test-bench
  ${CMAKE_CURRENT_LIST_DIR}/test_runner.c
  ${PROJECT_SOURCE_DIR}/runner/bench.c
  ${PROJECT_SOURCE_DIR}/runner/sha256.c
  ${PROJECT_SOURCE_DIR}/runner/stop.c)
set_target_properties(schwaystation-test-bench PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-bench)

# Fault prevented: an audio hash that omits PCM samples, depends on drain
# partitioning, or treats a silent run as nonzero.
set(runner_audio_check ${runner_check} -DUNIT=$<TARGET_FILE:schwaystation-test-bench>)
if(SCHWAYSTATION_FIXTURES)
  list(APPEND runner_audio_check -DFIXTURES=${PROJECT_BINARY_DIR}/fixtures)
endif()
# run 43657441300: macOS x64 hit the 60 s bound (1 sample); provisional 120 s.
schway_add_test(NAME runner.audio.hash
  COMMAND ${runner_audio_check} -DCASE=audio_hash -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 120)

add_executable(schwaystation-test-run-stop ${SCHWAYSTATION_RUNNER_SOURCES})
target_include_directories(schwaystation-test-run-stop PRIVATE ${SCHWAYSTATION_RUNNER_INCLUDE}
  ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-run-stop PRIVATE schwaystation::schwaystation
  schwaystation-host schwaystation_disc_image)
target_compile_definitions(schwaystation-test-run-stop PRIVATE _POSIX_C_SOURCE=200809L
  SCHWAY_RUN_RAISE_AT=5 SCHWAY_RUN_TEST_HOOKS=1 SCHWAY_HOST_CARD_TESTING=1)
set_target_properties(schwaystation-test-run-stop PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-run-stop)

# Fault prevented: the audio drain's failure is reported as success or a hash
# mismatch, and invalid CLI combinations are mistaken for run failures.
schway_add_test(NAME runner.audio.failures
  COMMAND ${runner_check} -DHOOKED=$<TARGET_FILE:schwaystation-test-run-stop>
    -DCASE=audio_failures -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

set(bench_check ${runner_check} -DUNIT=$<TARGET_FILE:schwaystation-test-bench>)

# Fault prevented: a percentile that interpolates, picks the wrong rank or
# reads unsorted samples, so the report names a time no frame took.
schway_add_test(NAME runner.bench.percentile
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-bench> percentile
  LABELS bench portable runner TIMEOUT 10)

# Fault prevented: hostile guest terminal control bytes reaching the user's terminal.
schway_add_test(NAME runner.tty_sanitize
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-bench> tty_sanitize
  LABELS portable runner TIMEOUT 10)

# Fault prevented: a bench report missing a line, out of order or not in
# whole units, so a script reading it misreads the times.
schway_add_test(NAME runner.bench.format
  COMMAND ${bench_check} -DCASE=bench_format -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS bench portable runner TIMEOUT 30)

# Fault prevented: a sample count whose buffer size wraps to a small
# allocation, so the frame loop writes past it.
schway_add_test(NAME runner.bench.bytes
  COMMAND ${bench_check} -DCASE=bench_bytes -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS bench portable runner TIMEOUT 30)

# Fault prevented: SIGINT or SIGTERM killing the run mid-frame or losing the
# output already asked for, so an interrupted CI job reports nothing.
schway_add_test(NAME runner.stop
  COMMAND ${bench_check} -DHOOKED=$<TARGET_FILE:schwaystation-test-run-stop>
    -DEXPECTED=${SCHWAYSTATION_TESTCARD_SHA256_FRAME5} -DCASE=stop -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: completed card generations are not flushed on normal exit or after a clean
# SIGTERM, or an unchanged slot is needlessly replaced.
set(runner_cards_check ${runner_check} -DHOOKED=$<TARGET_FILE:schwaystation-test-run-stop>
  -DEXPECTED=${SCHWAYSTATION_TESTCARD_SHA256_FRAME5} -DOUT=${PROJECT_BINARY_DIR}/tests
  -DFIXTURES=${PROJECT_BINARY_DIR}/fixtures)
foreach(case controls persistence sigterm)
  schway_add_test(NAME runner.cards.${case}
    COMMAND ${runner_cards_check} -DCASE=cards_${case} -P ${CMAKE_CURRENT_LIST_FILE}
    LABELS portable runner TIMEOUT 60)
endforeach()

# Fault prevented: runner card paths or protocol state alias across adapter sockets.
schway_add_test(NAME runner.cards.multitap
  COMMAND ${runner_cards_check} -DOUT=${PROJECT_BINARY_DIR}/tests/runner-cards-multitap
    -DCASE=cards_multitap -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a runner that exits 0 after failing to write a file, or
# calls a failure a usage error.
set(runner_failure_cmd ${runner_cmd} --frames 1 --png "1:${PROJECT_BINARY_DIR}/tests/missing/x.png")
string(REPLACE ";" "$<SEMICOLON>" runner_failure_cmd "${runner_failure_cmd}")
schway_add_test(NAME runner.exit.failure
  COMMAND ${CMAKE_COMMAND} "-DCMD=${runner_failure_cmd}" -DCODE=1 "-DREGEX=cannot write"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS portable runner TIMEOUT 30)

# Fault prevented: a missing or invalid --exe file that is run anyway, or that
# is reported as a usage error, so a CI step passes on nothing.
schway_add_test(NAME runner.exe.missing
  COMMAND ${runner_check} -DCASE=exe_missing -DOUT=${PROJECT_BINARY_DIR}/tests
    -P ${CMAKE_CURRENT_LIST_FILE}
  LABELS portable runner TIMEOUT 30)

# The trace record formatter, tested directly: it links the formatter alone, with no instance.
add_executable(schwaystation-test-trace-format
  ${CMAKE_CURRENT_LIST_DIR}/test_trace.c
  ${PROJECT_SOURCE_DIR}/runner/trace.c)
target_link_libraries(schwaystation-test-trace-format PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-trace-format PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-trace-format)

# Fault prevented: a trace line whose fields, order, hex width or event and device names drift from
# what docs/runner.md says, so a script that reads the tail misparses it. One golden line per event.
schway_add_test(NAME runner.trace.golden
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-trace-format>
  LABELS portable runner TIMEOUT 10)

# Fault prevented: a documented exit status that differs from what the runner returns. --help's
# table and docs/runner.md's must match line for line, in either direction; the self test checks a
# copy of the page without the hang row, which must fail naming that row.
set(runner_doc ${PROJECT_SOURCE_DIR}/docs/runner.md)
set(runner_doc_copy ${PROJECT_BINARY_DIR}/tests/runner_without_hang.md)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${runner_doc})
file(READ ${runner_doc} runner_doc_text)
string(REGEX REPLACE "\n  8  hang[^\n]*" "" runner_doc_text "${runner_doc_text}")
file(WRITE ${runner_doc_copy} "${runner_doc_text}")
schway_add_test(NAME runner.exit_table
  COMMAND ${runner_check} -DDOC=${runner_doc}
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/exit_table.cmake
  LABELS portable runner TIMEOUT 30)
# The emulator's own semicolons would be split a second time inside -DCMD, so it travels with a bar
# between its words and exit_table.cmake turns the bars back.
string(REPLACE ";" "|" runner_emulator_bars "${CMAKE_CROSSCOMPILING_EMULATOR}")
set(runner_table_cmd ${CMAKE_COMMAND} -DRUN=$<TARGET_FILE:schwaystation-run>
  "-DEMULATOR=${runner_emulator_bars}" -DDOC=${runner_doc_copy}
  -P ${PROJECT_SOURCE_DIR}/tests/cmake/exit_table.cmake)
string(REPLACE ";" "$<SEMICOLON>" runner_table_cmd "${runner_table_cmd}")
schway_add_test(NAME runner.exit_table.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${runner_table_cmd}" -DCODE=1
    "-DREGEX=lacks the exit status line[^/]*8[^/]*hang"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS portable runner TIMEOUT 30)

# Fault prevented: --version printing something a script cannot parse.
schway_add_test(NAME runner.version
  COMMAND ${runner_cmd} --version
  PASS_REGEX "schwaystation-run [0-9]+[.][0-9]+[.][0-9]+ ([0-9a-f]+|unknown)"
  LABELS portable runner TIMEOUT 10)

# Fault prevented: testing a stale configure after a commit, so the results
# describe a tree other than HEAD. The self-test forces a mismatch.
set(runner_revision ${runner_check} -DSOURCE=${PROJECT_SOURCE_DIR}
  -P ${PROJECT_SOURCE_DIR}/tests/cmake/revision.cmake)
schway_add_test(NAME build.revision_matches_head
  COMMAND ${runner_revision}
  LABELS runner TIMEOUT 30)
set(runner_revision_cmd ${CMAKE_COMMAND} -DRUN=$<TARGET_FILE:schwaystation-run>
  -DREVISION=0000000000000000000000000000000000000000 -DSOURCE=${PROJECT_SOURCE_DIR}
  -P ${PROJECT_SOURCE_DIR}/tests/cmake/revision.cmake)
string(REPLACE ";" "$<SEMICOLON>" runner_revision_cmd "${runner_revision_cmd}")
schway_add_test(NAME build.revision_matches_head.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${runner_revision_cmd}" -DCODE=1
    "-DREGEX=configured at[^/]*0000000000000000000000000000000000000000[^/]*HEAD[^/]*is[^/]*[0-9a-f]"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS runner TIMEOUT 30)
