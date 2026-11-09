# Register-level bus behaviour.
# The bus and the interrupt registers through the module's own interface. One
# executable runs the case named on its command line. It also sees src/, so it
# can drive the bus, load a BIOS image and read the fault record.
add_executable(schwaystation-test-bus ${CMAKE_CURRENT_LIST_DIR}/bus_tests.c)
target_include_directories(schwaystation-test-bus PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(schwaystation-test-bus PRIVATE schwaystation::schwaystation)
set_target_properties(schwaystation-test-bus PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
schway_warnings(schwaystation-test-bus)

# Fault prevented: a RAM mirror that reads another address, a window past the
# banks RAM_SIZE selects that answers, or a RAM access that charges the wrong
# cycles.
schway_add_test(NAME bus.ram_mirrors
  COMMAND schwaystation-test-bus ram_mirrors
  LABELS bus portable TIMEOUT 10)
