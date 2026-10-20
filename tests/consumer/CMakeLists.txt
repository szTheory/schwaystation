# A separate project that finds the installed package, as an outside program
# would. The install tests build it against <build>/stage and run consumer
# (C). A -DCMAKE_MSVC_RUNTIME_LIBRARY on the command line picks the C runtime
# on MSVC; other compilers ignore it.
cmake_minimum_required(VERSION 3.25)
project(consumer C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(schwaystation CONFIG REQUIRED)

add_executable(consumer main.c)
target_link_libraries(consumer PRIVATE schwaystation::schwaystation)

# Where pkg-config exists (not MSVC, whose linker does not read -l), the same
# program also builds from schwaystation.pc alone, after find_package, so the
# two ways of finding the library do not disturb each other.
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND AND NOT MSVC)
  set(ENV{PKG_CONFIG_PATH} "${schwaystation_DIR}/../../pkgconfig")
  pkg_check_modules(schwaystation_pc REQUIRED IMPORTED_TARGET schwaystation)
  add_executable(consumer_pc main.c)
  target_link_libraries(consumer_pc PRIVATE PkgConfig::schwaystation_pc)
endif()
