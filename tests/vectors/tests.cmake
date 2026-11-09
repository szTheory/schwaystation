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

# Fault prevented: an instruction whose result, bus cycles or delay latches
# differ from the public vectors. The count is the zero-case guard: a run that
# replayed fewer cases than expected fails.
schway_add_test(NAME vectors.r3000.sample
  COMMAND schwaystation-test-vectors ${vectors_dir}/sst-sample.bin --only ADDU --expect 100
    --allow ${vectors_dir}/allow.txt
  LABELS portable vectors TIMEOUT 60)
