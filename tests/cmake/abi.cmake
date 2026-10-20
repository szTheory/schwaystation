# cmake -DMODE=<symbols|globals|undefined|clock> -DLIB=<archive> -DNM=<nm>
#       -DSYSTEM=<system>/<processor> [-DHEADER=<schwaystation.h>] -P abi.cmake
# Fault prevented, by mode:
#   symbols    a defined external name that is neither a schway_ function the
#              header declares nor an internal schway__ name, or a declared
#              function that nothing defines.
#   globals    mutable state outside the instance: a writable data symbol
#              (nm types B b D d C).
#   undefined  the core calling libc, libm or a floating-point helper: every
#              unresolved name is a C memory function or a helper below.
#   clock      the core reading a host clock, which breaks determinism.
cmake_minimum_required(VERSION 3.25)
foreach(var MODE LIB NM SYSTEM)
  if("${${var}}" STREQUAL "")
    message(FATAL_ERROR "abi: -D${var} is required")
  endif()
endforeach()
if(NOT EXISTS "${LIB}" OR IS_DIRECTORY "${LIB}")
  message(FATAL_ERROR "abi: library ${LIB} does not exist")
endif()

# Compiler helpers per system, "system regex:names". The core builds with
# -fno-stack-protector -U_FORTIFY_SOURCE, so no __stack_chk_* or __*_chk name.
set(helpers
  # Apple clang lowers a zeroing memset to bzero and a repeating 16-byte
  # store loop to memset_pattern16 (nm -u on the core, -O3).
  "^Darwin/:bzero __bzero memset_pattern16"
  # x86-64 ELF position-independent code names the linker-defined GOT symbol
  # (System V x86-64 psABI, section 3.5).
  "^Linux/(x86_64|amd64|AMD64)$:_GLOBAL_OFFSET_TABLE_"
  # 32-bit PowerPC has no 64-bit divide or shift; GCC calls the libgcc
  # integer routines (GCC internals manual, "Integer library routines").
  "^Linux/(ppc|powerpc)$:__udivdi3 __umoddi3 __ashldi3 __lshrdi3"
  # MSVC addresses switch tables from the linker-defined image base (PE
  # format, "__ImageBase"); x64's auto-vectorizer reads the CPU level.
  "^Windows/:__ImageBase"
  "^Windows/(AMD64|x86_64)$:__isa_available")
set(allowed memcpy memmove memset memcmp)
foreach(row IN LISTS helpers)
  string(REGEX MATCH "^([^:]*):(.*)$" unused "${row}")
  set(names "${CMAKE_MATCH_2}")
  if(SYSTEM MATCHES "${CMAKE_MATCH_1}")
    separate_arguments(names UNIX_COMMAND "${names}")
    list(APPEND allowed ${names})
  endif()
endforeach()

# Names a compiler gives its own constants, "system regex:name regex", left
# out of the globals and symbols checks. MSVC puts each string literal in a
# read-only COMDAT named ??_C@_ and each 128-bit vector constant in one named
# __xmm@, and on x64 emits __isa_available_default beside the vectorizer's
# CPU check (BoringSSL util/audit_symbols.go allows the same three).
set(generated
  "^Windows/:^(\\?\\?_C@_|__xmm@)"
  "^Windows/(AMD64|x86_64)$:^__isa_available_default$")
set(generated_regex "^$")
foreach(row IN LISTS generated)
  string(REGEX MATCH "^([^:]*):(.*)$" unused "${row}")
  if(SYSTEM MATCHES "${CMAKE_MATCH_1}")
    string(APPEND generated_regex "|${CMAKE_MATCH_2}")
  endif()
endforeach()

# nm -P prints "name type [value size]" per symbol and a header per member.
execute_process(COMMAND "${NM}" -P "${LIB}" RESULT_VARIABLE rc
  OUTPUT_VARIABLE text ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "abi: ${NM} -P ${LIB} failed (${rc}): ${err}")
endif()
string(REPLACE "\n" ";" lines "${text}")
foreach(line IN LISTS lines)
  if(NOT line MATCHES "^([^ ]+) ([A-Za-z])( |$)")
    continue()
  endif()
  set(name "${CMAKE_MATCH_1}")
  set(type "${CMAKE_MATCH_2}")
  if(SYSTEM MATCHES "^Darwin/")
    string(REGEX REPLACE "^_" "" name "${name}") # Mach-O prefixes C names with _
  endif()
  if(type STREQUAL "U")
    list(APPEND undefined "${name}")
    continue()
  endif()
  list(APPEND defined "${name}")
  if(name MATCHES "${generated_regex}")
    continue()
  endif()
  if(type MATCHES "^[BbDdC]$")
    list(APPEND writable "${type} ${name}")
  endif()
  if(type MATCHES "^[A-Z]$")
    list(APPEND external "${name}")
  endif()
endforeach()

set(bad "")
if(MODE STREQUAL "undefined")
  list(REMOVE_ITEM undefined ${defined} ${allowed})
  set(bad ${undefined})
  set(what "unexpected undefined symbol")
elseif(MODE STREQUAL "globals")
  set(bad ${writable})
  set(what "writable data symbol")
elseif(MODE STREQUAL "clock")
  # Optional import prefix and leading underscore; MSVC's time is _time64.
  set(bad ${undefined})
  list(FILTER bad INCLUDE REGEX "^(__imp_)?_?(time(32|64)?|clock|clock_gettime|gettimeofday|timespec_get|mach_absolute_time|mach_continuous_time|QueryPerformanceCounter|GetTickCount64|GetSystemTimeAsFileTime)$")
  set(what "host clock read")
elseif(MODE STREQUAL "symbols")
  file(READ "${HEADER}" header)
  string(REGEX MATCHALL "schway_[a-z0-9_]*\\(" declared "${header}")
  string(REPLACE "(" "" declared "${declared}")
  set(public ${external})
  list(FILTER public INCLUDE REGEX "^schway_[^_]")
  list(FILTER external EXCLUDE REGEX "^schway_")
  set(missing ${declared})
  list(REMOVE_ITEM missing ${public})
  list(REMOVE_ITEM public ${declared})
  list(TRANSFORM missing PREPEND "declared, not defined: ")
  list(TRANSFORM public PREPEND "defined, not declared: ")
  list(TRANSFORM external PREPEND "outside the schway_ namespace: ")
  set(bad ${missing} ${public} ${external})
  set(what "public name mismatch")
else()
  message(FATAL_ERROR "abi: unknown -DMODE=${MODE}")
endif()
list(REMOVE_DUPLICATES bad)
if(bad)
  list(JOIN bad "\n  " bad)
  message(FATAL_ERROR "abi ${MODE}: ${LIB}: ${what}:\n  ${bad}")
endif()
message(STATUS "abi ${MODE}: ${LIB} ok")
