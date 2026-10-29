# Every directory's tests. Each file is include()d, never add_subdirectory()d,
# so the top-level directory's TESTS property holds every name and the
# inventory check in cmake/SchwaystationTests.cmake sees them all. The hygiene
# lane registers only its own tests, and the nofp lane only the abi and nofp
# tests.
if(SCHWAYSTATION_HYGIENE)
  include(${CMAKE_CURRENT_LIST_DIR}/hygiene/tests.cmake)
  return()
endif()
if(SCHWAYSTATION_NOFP)
  include(${CMAKE_CURRENT_LIST_DIR}/abi/tests.cmake)
  include(${CMAKE_CURRENT_LIST_DIR}/nofp/tests.cmake)
  return()
endif()
include(${CMAKE_CURRENT_LIST_DIR}/testcard_pins.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/abi/tests.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/core/tests.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/install/tests.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/perf/tests.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/policy/tests.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/release/tests.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/runner/tests.cmake)
if(SCHWAYSTATION_BUILD_LIBRETRO)
  include(${CMAKE_CURRENT_LIST_DIR}/libretro/tests.cmake)
endif()
