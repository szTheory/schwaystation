# The hashes every lane prints, and their comparison across lanes, run as
#   cmake -DMODE=write -DRUN=<schwaystation-run> [-DEMULATOR=<list>] [-DFIXTURES=<dir>] -DOUT=<dir> -P hashes.cmake
#   cmake -DMODE=compare -DFILES=<list> -DCOUNT=<n> -P hashes.cmake
if(MODE STREQUAL "write")
  # Fault prevented: a lane's output silently not compared. Writes
  # <OUT>/hashes.txt, the file CI collects from every lane, in a fixed order:
  # four frame hashes, then the frame 5 PNG's digest, then one line per
  # assembled fixture file in FIXTURES (when given), sorted by name, so the
  # eight lanes' fixture bytes are compared too, then device fixture hashes and
  # the moving GPU fixture's frames and continuation state.
  set(lines "")
  foreach(case "ntsc-u;5" "ntsc-u;60" "ntsc-j;60" "pal;60")
    list(GET case 0 region)
    list(GET case 1 frames)
    set(args --frames ${frames} --hash --region ${region})
    execute_process(COMMAND ${EMULATOR} "${RUN}" ${args}
      OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE rc)
    if(NOT rc STREQUAL "0" OR NOT out MATCHES "^frame ${frames} sha256 [0-9a-f]+\n$")
      message(FATAL_ERROR "schwaystation-run ${args}: expected exit 0 and one hash line, "
        "got exit ${rc}\n${out}${err}")
    endif()
    string(APPEND lines "${region} ${out}")
  endforeach()
  # The reference PNG is deterministic too, so its digest is compared as well.
  execute_process(COMMAND ${EMULATOR} "${RUN}" --frames 5 --png "5:${OUT}/hashes-card.png"
    OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE rc)
  if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "schwaystation-run --frames 5 --png: exit ${rc}\n${out}${err}")
  endif()
  file(SHA256 "${OUT}/hashes-card.png" png)
  string(APPEND lines "ntsc-u png 5 sha256 ${png}\n")
  if(NOT "${FIXTURES}" STREQUAL "")
    file(GLOB names RELATIVE "${FIXTURES}" "${FIXTURES}/*")
    list(SORT names)
    foreach(name IN LISTS names)
      file(SHA256 "${FIXTURES}/${name}" digest)
      string(APPEND lines "fixture ${name} sha256 ${digest}\n")
    endforeach()
    # The frame and state the programs ran, which differ between lanes if an instruction, device or the stub kernel does.
    foreach(exe hello gte_vectors dma_chain timers_lines)
      execute_process(COMMAND ${EMULATOR} "${RUN}" --exe "${FIXTURES}/${exe}.exe" --frames 30 --hash --hash-state
        OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE rc)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "(frame 30 sha256 [0-9a-f]+)\n(state sha256 [0-9a-f]+)\n$")
        message(FATAL_ERROR "schwaystation-run ${exe}.exe: expected exit 0 and the two hash lines, got exit ${rc}\n${out}${err}")
      endif()
      string(APPEND lines "exe ${exe}.exe ${CMAKE_MATCH_1}\nexe ${exe}.exe ${CMAKE_MATCH_2}\n")
    endforeach()
    foreach(frames IN ITEMS 60 120)
      execute_process(COMMAND ${EMULATOR} "${RUN}" --exe "${FIXTURES}/gpu_motion.exe"
        --frames ${frames} --hash OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE rc)
      if(NOT rc STREQUAL "0" OR NOT out MATCHES "^(frame ${frames} sha256 [0-9a-f]+)\n$")
        message(FATAL_ERROR "schwaystation-run gpu_motion.exe --frames ${frames}: expected exit 0 and one hash line, got exit ${rc}\n${out}${err}")
      endif()
      string(APPEND lines "exe gpu_motion.exe ${CMAKE_MATCH_1}\n")
    endforeach()
    execute_process(COMMAND ${EMULATOR} "${RUN}" --exe "${FIXTURES}/gpu_motion.exe"
      --frames 150 --hash-state OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE rc)
    if(NOT rc STREQUAL "0" OR NOT out MATCHES "^state sha256 ([0-9a-f]+)\n$")
      message(FATAL_ERROR "schwaystation-run gpu_motion.exe --frames 150 --hash-state: expected exit 0 and one state line, got exit ${rc}\n${out}${err}")
    endif()
    string(APPEND lines "exe gpu_motion.exe state 150 sha256 ${CMAKE_MATCH_1}\n")
    execute_process(COMMAND ${EMULATOR} "${RUN}" --exe "${FIXTURES}/gpu_motion.exe"
      --frames 180 --hash --hash-state OUTPUT_VARIABLE out ERROR_VARIABLE err RESULT_VARIABLE rc)
    if(NOT rc STREQUAL "0" OR NOT out MATCHES "^frame 180 sha256 ([0-9a-f]+)\nstate sha256 ([0-9a-f]+)\n$")
      message(FATAL_ERROR "schwaystation-run gpu_motion.exe --frames 180 --hash --hash-state: expected exit 0 and frame/state lines, got exit ${rc}\n${out}${err}")
    endif()
    string(APPEND lines "exe gpu_motion.exe frame 180 sha256 ${CMAKE_MATCH_1}\nexe gpu_motion.exe state 180 sha256 ${CMAKE_MATCH_2}\n")
  endif()
  # Unix newlines on every host: file(WRITE) writes CRLF on Windows.
  file(CONFIGURE OUTPUT "${OUT}/hashes.txt" NEWLINE_STYLE UNIX CONTENT "${lines}")
elseif(MODE STREQUAL "compare")
  # Fault prevented: lanes that disagree, or a lane that stops uploading and
  # silently drops out. Passes only for exactly COUNT existing files that are
  # byte-identical.
  list(LENGTH FILES n)
  if(NOT n EQUAL COUNT)
    message(FATAL_ERROR "hashes: expected ${COUNT} files, got ${n}: ${FILES}")
  endif()
  list(GET FILES 0 first)
  foreach(file IN LISTS FILES)
    if(NOT EXISTS "${file}")
      message(FATAL_ERROR "hashes: ${file} does not exist")
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files "${first}" "${file}"
      RESULT_VARIABLE rc OUTPUT_QUIET ERROR_QUIET)
    if(NOT rc EQUAL 0)
      file(READ "${first}" want)
      file(READ "${file}" got)
      message(FATAL_ERROR "hashes: ${file} differs from ${first}\n${first}:\n${want}${file}:\n${got}")
    endif()
  endforeach()
  message(STATUS "hashes: ${n} identical files")
else()
  message(FATAL_ERROR "hashes: -DMODE=write or -DMODE=compare is required")
endif()
