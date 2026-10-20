# Release archives: one ZIP per install component, named
# schwaystation-COMPONENT-OS-ARCH.zip with no version, so a download link to the
# latest release never changes, and with no top-level directory. The package
# step of the ci workflow writes them into the build directory.

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(SCHWAYSTATION_OS linux)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(SCHWAYSTATION_OS macos)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(SCHWAYSTATION_OS windows)
else()
  # Not a release target: the archives are named after the system as CMake
  # reports it, so the build still configures.
  string(TOLOWER "${CMAKE_SYSTEM_NAME}" SCHWAYSTATION_OS)
  message(WARNING "packaging: '${CMAKE_SYSTEM_NAME}' is not a release target; "
    "archives are named '${SCHWAYSTATION_OS}'")
endif()

# The architecture the binaries are built for, not the build machine's.
if(MSVC)
  set(packaging_arch "${CMAKE_C_COMPILER_ARCHITECTURE_ID}")
else()
  set(packaging_arch "${CMAKE_SYSTEM_PROCESSOR}")
  list(LENGTH CMAKE_OSX_ARCHITECTURES packaging_osx_count)
  if(APPLE AND packaging_osx_count EQUAL 1)
    set(packaging_arch "${CMAKE_OSX_ARCHITECTURES}")
  endif()
endif()
if(packaging_arch MATCHES "^(x86_64|AMD64|x64)$")
  set(SCHWAYSTATION_ARCH x64)
elseif(packaging_arch MATCHES "^(arm64|aarch64|ARM64)$")
  set(SCHWAYSTATION_ARCH arm64)
else()
  string(TOLOWER "${packaging_arch}" SCHWAYSTATION_ARCH)
  message(WARNING "packaging: '${packaging_arch}' is not a release architecture; "
    "archives are named '${SCHWAYSTATION_ARCH}'")
endif()

# Every archive carries the licence and the third-party notices at its root:
# the MIT notices of the code and of libretro.h travel with the binaries.
foreach(packaging_component IN ITEMS library runner libretro)
  install(FILES LICENSE THIRD-PARTY-NOTICES.md
    DESTINATION . COMPONENT ${packaging_component})
endforeach()

set(CPACK_GENERATOR ZIP)
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_COMPONENTS_ALL library runner libretro)
set(CPACK_PACKAGE_DIRECTORY ${PROJECT_BINARY_DIR})
# The component name in these variables is upper case and the value has no
# extension (cpack-generators(7), CPACK_ARCHIVE_<component>_FILE_NAME).
foreach(packaging_component IN ITEMS library runner libretro)
  string(TOUPPER ${packaging_component} packaging_upper)
  set(CPACK_ARCHIVE_${packaging_upper}_FILE_NAME
    "schwaystation-${packaging_component}-${SCHWAYSTATION_OS}-${SCHWAYSTATION_ARCH}")
endforeach()
include(CPack)
