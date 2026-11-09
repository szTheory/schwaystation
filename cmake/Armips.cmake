# The pinned armips, downloaded with its SHA-256 checked and built for the host.
#
#   include(cmake/Armips.cmake)                  sets SCHWAYSTATION_ARMIPS, adds target schwaystation-armips
#   cmake -DURL=<url> -DSHA256=<hex> -DOUT=<file> -P cmake/Armips.cmake    the digest check alone
#
# Fault prevented: a tampered or wrongly pinned assembler accepted. A wrong
# digest fails configure; no POSIX shell is needed, so every host builds it
# the same way.
if(CMAKE_SCRIPT_MODE_FILE)
  foreach(var URL SHA256 OUT)
    if(NOT DEFINED ${var} OR "${${var}}" STREQUAL "")
      message(FATAL_ERROR "armips: -D${var} is required")
    endif()
  endforeach()
  file(DOWNLOAD "${URL}" "${OUT}" EXPECTED_HASH SHA256=${SHA256} STATUS status)
  list(GET status 0 code)
  if(NOT code EQUAL 0)
    file(REMOVE "${OUT}")
    message(FATAL_ERROR "armips: ${URL}: download or SHA-256 check failed: ${status}")
  endif()
  message(STATUS "armips: ${URL}: SHA-256 matches")
  return()
endif()

include(ExternalProject)

# The pins.txt row: name version url sha256.
file(STRINGS "${PROJECT_SOURCE_DIR}/pins.txt" schway__armips_rows REGEX "^armips ")
list(LENGTH schway__armips_rows schway__armips_n)
if(NOT schway__armips_n EQUAL 1)
  message(FATAL_ERROR "armips: pins.txt needs exactly one 'armips' row, found ${schway__armips_n}")
endif()
string(REPLACE " " ";" schway__armips_row "${schway__armips_rows}")
list(GET schway__armips_row 2 schway__armips_url)
list(GET schway__armips_row 3 schway__armips_sha256)
set(schway__armips_commit "")
if(schway__armips_url MATCHES "/archive/([0-9a-f]+)[.]tar[.]gz$")
  set(schway__armips_commit "${CMAKE_MATCH_1}")
endif()
string(LENGTH "${schway__armips_commit}" schway__armips_len)
if(NOT schway__armips_len EQUAL 40)
  message(FATAL_ERROR "armips: the pins.txt URL must name the full 40-character commit: ${schway__armips_url}")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/pins.txt")

# Downloaded once per build directory; a present archive is reused only when
# its digest still matches the pin.
set(schway__armips_archive "${PROJECT_BINARY_DIR}/pins/armips.tar.gz")
set(schway__armips_have FALSE)
if(EXISTS "${schway__armips_archive}")
  file(SHA256 "${schway__armips_archive}" schway__armips_got)
  if(schway__armips_got STREQUAL schway__armips_sha256)
    set(schway__armips_have TRUE)
  endif()
endif()
if(NOT schway__armips_have)
  file(DOWNLOAD "${schway__armips_url}" "${schway__armips_archive}"
    EXPECTED_HASH SHA256=${schway__armips_sha256} STATUS schway__armips_status)
  list(GET schway__armips_status 0 schway__armips_code)
  if(NOT schway__armips_code EQUAL 0)
    file(REMOVE "${schway__armips_archive}")
    message(FATAL_ERROR "armips: ${schway__armips_url}: download or SHA-256 check failed: ${schway__armips_status}")
  endif()
endif()
set(schway__armips_src "${PROJECT_BINARY_DIR}/pins/armips/armips-${schway__armips_commit}")
if(NOT EXISTS "${schway__armips_src}/CMakeLists.txt")
  file(ARCHIVE_EXTRACT INPUT "${schway__armips_archive}" DESTINATION "${PROJECT_BINARY_DIR}/pins/armips")
endif()

# A host-native build: the parent's toolchain file, deployment target, C
# compiler setting and warnings-as-errors are not passed on. std::filesystem
# needs macOS 10.15, above the 10.13 the library itself targets.
set(schway__armips_args -DCMAKE_BUILD_TYPE=Release -DARMIPS_USE_STD_FILESYSTEM=ON
  -DARMIPS_LIBRARY_ONLY=OFF)
if(CMAKE_HOST_APPLE)
  list(APPEND schway__armips_args -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15)
endif()
set(schway__armips_bin "${PROJECT_BINARY_DIR}/armips-build")
set(SCHWAYSTATION_ARMIPS "${schway__armips_bin}/armips${CMAKE_HOST_EXECUTABLE_SUFFIX}")
ExternalProject_Add(schwaystation-armips
  SOURCE_DIR "${schway__armips_src}"
  BINARY_DIR "${schway__armips_bin}"
  PREFIX "${PROJECT_BINARY_DIR}/armips-ep"
  CMAKE_ARGS ${schway__armips_args}
  BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target armips-bin
  INSTALL_COMMAND ""
  BUILD_BYPRODUCTS "${SCHWAYSTATION_ARMIPS}")
