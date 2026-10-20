# Runs the built libretro core in RetroArch, unattended and with no content,
# and keeps the screenshot RetroArch takes after N frames.
#
#   cmake -DRETROARCH=<RetroArch.app/Contents/MacOS/RetroArch> -DCORE=<core>
#         -DINFO=<.info> -DTEMPLATE=<test.cfg.in> -DRA_DIR=<run directory>
#         -DFRAMES=<N> -DSHOT=<png> -P run_retroarch.cmake
#
# Fault prevented: a core that a real frontend cannot start, or whose frame
# there differs from the one the library produces. RetroArch gets a home,
# configuration and every directory inside RA_DIR, so nothing outside it is
# read or written. The launch is bounded at 50 seconds; any failure prints
# RetroArch's verbose log. Needs a GUI session: RetroArch opens a window.
cmake_minimum_required(VERSION 3.25)
foreach(var RETROARCH CORE INFO TEMPLATE RA_DIR FRAMES SHOT)
  if("${${var}}" STREQUAL "")
    message(FATAL_ERROR "run_retroarch: -D${var} is required")
  endif()
endforeach()

# A fresh run directory holding every directory test.cfg names, so
# RetroArch never falls back to a default location for a missing one.
file(REMOVE_RECURSE "${RA_DIR}")
foreach(sub home assets cache cores info logs playlists saves shots states system)
  file(MAKE_DIRECTORY "${RA_DIR}/${sub}")
endforeach()
configure_file("${TEMPLATE}" "${RA_DIR}/test.cfg" @ONLY)
file(COPY "${INFO}" DESTINATION "${RA_DIR}/info")
file(REAL_PATH "${CORE}" core)
file(REAL_PATH "${RETROARCH}" retroarch)
get_filename_component(shot_dir "${SHOT}" DIRECTORY)
file(MAKE_DIRECTORY "${shot_dir}")
file(REMOVE "${SHOT}")

# The macOS app creates first-run directories under its home folder even
# when every configured path is redirected, so the child gets its own home
# (HOME for POSIX calls, CFFIXED_USER_HOME for Foundation's).
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env "HOME=${RA_DIR}/home" "CFFIXED_USER_HOME=${RA_DIR}/home"
    "${retroarch}" --verbose --config "${RA_DIR}/test.cfg" -L "${core}"
    "--max-frames=${FRAMES}" --max-frames-ss "--max-frames-ss-path=${SHOT}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE log
  ERROR_VARIABLE log
  TIMEOUT 50)
file(WRITE "${RA_DIR}/retroarch.log" "${log}")

if(NOT result STREQUAL "0")
  message(FATAL_ERROR "RetroArch exited with '${result}'. Its log:\n${log}")
endif()
if(NOT EXISTS "${SHOT}")
  message(FATAL_ERROR "RetroArch wrote no screenshot at ${SHOT}. Its log:\n${log}")
endif()
file(SIZE "${SHOT}" size)
message(STATUS "RetroArch ran ${FRAMES} frames and wrote ${SHOT} (${size} bytes)")
