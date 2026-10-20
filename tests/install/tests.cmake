# The library as an outside project consumes it.
#
# Fault prevented: an install tree, package file or header that only works
# inside this build, so a program built against a release archive fails to
# configure, compile or link.
#
# install.stage installs the library component into <build>/stage. The
# consumer tests build tests/consumer against it with find_package, each in
# its own build directory, and run it. Each consumer gets this build's
# compilers, build type and flags, each as one argument so an empty value
# survives. The install_staged fixture keeps parallel CTest from running a
# consumer before the stage.
schway_add_test(NAME install.stage
  COMMAND ${CMAKE_COMMAND} --install ${PROJECT_BINARY_DIR}
    --component library --prefix ${PROJECT_BINARY_DIR}/stage --config $<CONFIG>
  LABELS install TIMEOUT 60)
set_tests_properties(install.stage PROPERTIES FIXTURES_SETUP install_staged)

get_property(install_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
set(install_config)
if(install_multi_config)
  set(install_config --build-config $<CONFIG>)
endif()
set(install_build_options
  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
  -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
  -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
  "-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}"
  "-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}"
  "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}"
  "-DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}")

# name, build directory, C runtime, program, expected output
set(install_consumers
  "install.consumer|consumer|MultiThreaded$<$<CONFIG:Debug>:Debug>|consumer|consumer: api 1, one frame 320x240")
foreach(install_row IN LISTS install_consumers)
  string(REPLACE "|" ";" install_row "${install_row}")
  list(GET install_row 0 install_name)
  list(GET install_row 1 install_dir)
  list(GET install_row 2 install_runtime)
  list(GET install_row 3 install_program)
  list(GET install_row 4 install_expect)
  schway_add_test(NAME ${install_name}
    COMMAND ${CMAKE_CTEST_COMMAND} --build-and-test
      ${CMAKE_CURRENT_LIST_DIR}/../consumer ${PROJECT_BINARY_DIR}/${install_dir}
      --build-generator ${CMAKE_GENERATOR}
      ${install_config}
      --build-options
        -DCMAKE_PREFIX_PATH=${PROJECT_BINARY_DIR}/stage
        -DCMAKE_MSVC_RUNTIME_LIBRARY=${install_runtime}
        ${install_build_options}
      --test-command ${install_program}
    LABELS install TIMEOUT 120 PASS_REGEX "${install_expect}")
  set_tests_properties(${install_name} PROPERTIES FIXTURES_REQUIRED install_staged)
endforeach()
