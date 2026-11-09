# The SingleStepTests sample, replayed through the CPU's one-instruction entry.
# One executable reads the container, replays each case's bus from its recorded
# cycles and compares the final state field by field. It also sees src/, so it
# can load and read the CPU's registers and latches.
add_executable(schwaystation-test-vectors ${CMAKE_CURRENT_LIST_DIR}/vectors.c)
target_include_directories(schwaystation-test-vectors PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-vectors PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-vectors PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-vectors)

set(vectors_dir ${CMAKE_CURRENT_LIST_DIR}/r3000)
set(vectors_sample ${vectors_dir}/sst-sample.bin)
set(vectors_scratch ${PROJECT_BINARY_DIR}/tests)
# The ALU, shift, logic, set and LUI files, 100 cases each.
set(vectors_alu ADDIU,ADDU,AND,ANDI,LUI,NOR,OR,ORI,SLL,SLLV,SLT,SLTI,SLTIU,SLTU,SRA,SRAV,SRL,SRLV,SUBU,XOR,XORI)
# The trapping adds, SYSCALL and BREAK, and every branch and jump: the files
# that take the exception entry or leave the branch record set.
set(vectors_exc ADD,ADDI,BCondZ,BEQ,BGTZ,BLEZ,BNE,BREAK,J,JAL,JALR,JR,SUB,SYSCALL)

# Fault prevented: an instruction whose result, bus cycles or delay latches
# differ from the public vectors. The count is the zero-case guard: a run that
# replayed fewer cases than expected fails.
schway_add_test(NAME vectors.r3000.sample
  COMMAND schwaystation-test-vectors ${vectors_sample} --only ${vectors_alu},${vectors_exc} --expect 3500
    --allow ${vectors_dir}/allow.txt
  LABELS portable vectors TIMEOUT 60)

# One command line for expect_fail.cmake: the emulated lane's own guest binary,
# then the arguments, with the list separators protected.
function(schway__vectors_cmd out)
  set(cmd ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-test-vectors> ${ARGN})
  string(REPLACE ";" "$<SEMICOLON>" cmd "${cmd}")
  set(${out} "${cmd}" PARENT_SCOPE)
endfunction()

# The two unreadable inputs. The truncated one is the first 1,000 bytes of the
# sample, copied with the one CMake call that keeps NUL bytes. Their names
# avoid the words the controls look for in the output.
file(MAKE_DIRECTORY ${vectors_scratch})
file(WRITE ${vectors_scratch}/vectors_none.bin "")
# A drive-letter path gets the slash a file URL needs. Not string(REGEX
# REPLACE "^(.)": CMake restarts "^" after each match, so "D:/a" became "/D/:/a".
set(vectors_url "${vectors_sample}")
if(NOT vectors_url MATCHES "^/")
  string(PREPEND vectors_url "/")
endif()
file(DOWNLOAD file://${vectors_url} ${vectors_scratch}/vectors_cut.bin RANGE_END 999
  STATUS vectors_status)
list(GET vectors_status 0 vectors_code)
if(NOT vectors_code EQUAL 0)
  message(FATAL_ERROR "vectors: copying the first 1000 bytes of the sample failed: ${vectors_status}")
endif()

# Fault prevented: a harness that passes when it read nothing, or only part of
# the file.
foreach(kind IN ITEMS empty truncated)
  if(kind STREQUAL "empty")
    set(input ${vectors_scratch}/vectors_none.bin)
  else()
    set(input ${vectors_scratch}/vectors_cut.bin)
  endif()
  schway__vectors_cmd(cmd ${input} --expect 1)
  schway_add_test(NAME vectors.r3000.${kind}.self_test
    COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=vectors:.*(empty|short|truncated)"
      -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
    LABELS portable vectors TIMEOUT 60)
endforeach()

# Fault prevented: a harness that compares only bus cycles, or skips the
# COP0 fields or a delay latch, so a wrong register, a wrong field or a wrong
# latch passes. Each control inverts one bit of the first case's expected final
# field and must fail naming that field.
foreach(field IN ITEMS r5 epc tar cause branch_target load_value)
  schway__vectors_cmd(cmd ${vectors_sample} --only ADDU --expect 100 --flip-expected ${field})
  schway_add_test(NAME vectors.r3000.flip.${field}.self_test
    COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=final ${field}"
      -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
    LABELS portable vectors TIMEOUT 60)
endforeach()

# Fault prevented: an allow line that hides a failure it does not name, or
# that stays after its failure is fixed. One dated line covers the one case the
# flip breaks; without the flip the same line matches nothing and is an error.
file(WRITE ${vectors_scratch}/vectors_allow.txt
  "# dated lines only\n2026-11-09 ADDU 000 https://example.invalid/issues/1\n")
set(vectors_allow_args ${vectors_sample} --only ADDU --expect 100
  --allow ${vectors_scratch}/vectors_allow.txt)
schway__vectors_cmd(cmd ${vectors_allow_args} --flip-expected r5)
schway_add_test(NAME vectors.r3000.allow.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=0 "-DREGEX=vectors: 100 cases, 0 failures"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS portable vectors TIMEOUT 60)
schway__vectors_cmd(cmd ${vectors_allow_args})
schway_add_test(NAME vectors.r3000.allow_stale.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=allow line ADDU 000 matched no failing case"
    -P ${PROJECT_SOURCE_DIR}/tests/cmake/expect_fail.cmake
  LABELS portable vectors TIMEOUT 60)
