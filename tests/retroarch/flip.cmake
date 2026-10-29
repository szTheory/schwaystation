# The screenshot compare's negative control, run as
#   cmake -DCOMPARE=<compare> -DEXPECTED=<bmp> -DBMP=<bmp> -DFLIPPED=<bmp> -P flip.cmake
# Fault prevented: a compare that cannot fail. BMP is copied to FLIPPED and
# every bit of one byte of pixel (7,3) is inverted, at an offset read from
# the BMP header; compare EXPECTED FLIPPED must then exit 1 naming exactly
# that pixel.
cmake_minimum_required(VERSION 3.25)
foreach(var COMPARE EXPECTED BMP FLIPPED)
  if("${${var}}" STREQUAL "")
    message(FATAL_ERROR "flip: -D${var} is required")
  endif()
endforeach()
file(COPY_FILE "${BMP}" "${FLIPPED}")

# A little-endian unsigned field of FLIPPED.
function(flip_field out offset bytes)
  file(READ "${FLIPPED}" hex OFFSET ${offset} LIMIT ${bytes} HEX)
  string(REGEX MATCHALL ".." hex "${hex}")
  list(REVERSE hex)
  list(JOIN hex "" hex)
  math(EXPR value "0x${hex}")
  set(${out} ${value} PARENT_SCOPE)
endfunction()
flip_field(data 10 4)
flip_field(width 18 4)
flip_field(height 22 4)
flip_field(bpp 28 2)
set(x 7)
set(y 3)
set(row ${y})
if(height LESS 2147483648)
  math(EXPR row "${height} - 1 - ${y}")
endif()
math(EXPR off "${data} + ${row} * ((${width} * ${bpp} + 31) / 32 * 4) + ${x} * ${bpp} / 8")
file(READ "${FLIPPED}" before OFFSET ${off} LIMIT 1 HEX)
math(EXPR value "0x${before} ^ 255")
math(EXPR d1 "${value} / 64")
math(EXPR d2 "(${value} / 8) % 8")
math(EXPR d3 "${value} % 8")
execute_process(COMMAND sh -c "printf '\\${d1}${d2}${d3}' | dd of=\"$1\" bs=1 seek=$2 conv=notrunc 2>/dev/null"
  sh "${FLIPPED}" ${off} RESULT_VARIABLE rc)
file(READ "${FLIPPED}" after OFFSET ${off} LIMIT 1 HEX)
if(NOT rc EQUAL 0 OR after STREQUAL before)
  message(FATAL_ERROR "flip: could not invert byte ${off} of ${FLIPPED}")
endif()

execute_process(COMMAND "${COMPARE}" "${EXPECTED}" "${FLIPPED}" RESULT_VARIABLE rc OUTPUT_VARIABLE out)
string(STRIP "${out}" out)
message(STATUS "flip: compare exited ${rc}: ${out}")
if(NOT rc STREQUAL "1" OR NOT out MATCHES "^first difference at ${x},${y}: ")
  message(FATAL_ERROR "flip: want exit 1 naming pixel ${x},${y}, got exit ${rc}")
endif()
