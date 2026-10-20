# The test card's pinned frame hashes, as schwaystation-run --frames N --hash
# prints them (the 13 metadata words, then the 320 x 240 rows, all
# little-endian; ARCHITECTURE section 7). Computed from the card's rules and
# the power-on display words by an independent one-off script, not by this
# code; every test reads them from here. A changed card changes them only
# together with the behaviour revision.
#
# NTSC-U frame 60, the default region.
set(SCHWAYSTATION_TESTCARD_SHA256 be3538480b252f5bcfe7cb5b876a71c72d3795621b4278e405af61c425c06d71)
# NTSC-U frame 5: the counter strip moves, so it differs from frame 60.
set(SCHWAYSTATION_TESTCARD_SHA256_FRAME5 c3409c280e768d81ce09c5de0eb2d7e09e8d589bf740672b560b174ff583946c)
# Frame 60 with --region ntsc-j and --region pal: the region word differs.
set(SCHWAYSTATION_TESTCARD_SHA256_NTSC_J 547d307f7d4bba16894f82c604a7329620bbbea9407e5d8e5008750f10680d91)
set(SCHWAYSTATION_TESTCARD_SHA256_PAL 8b8f3d0ac086930610a927ec7510738b0d8773c09deef0de0a2f107b2e8f589a)

foreach(pin SCHWAYSTATION_TESTCARD_SHA256 SCHWAYSTATION_TESTCARD_SHA256_FRAME5
    SCHWAYSTATION_TESTCARD_SHA256_NTSC_J SCHWAYSTATION_TESTCARD_SHA256_PAL)
  string(LENGTH "${${pin}}" pin_length)
  if(NOT ${pin} MATCHES "^[0-9a-f]+$" OR NOT pin_length EQUAL 64)
    message(FATAL_ERROR "testcard_pins: ${pin} must be 64 lowercase hex digits")
  endif()
endforeach()
# Fault prevented: a region that does not reach the frame, pinned as if it did.
set(pins ${SCHWAYSTATION_TESTCARD_SHA256} ${SCHWAYSTATION_TESTCARD_SHA256_NTSC_J}
  ${SCHWAYSTATION_TESTCARD_SHA256_PAL})
list(REMOVE_DUPLICATES pins)
list(LENGTH pins pin_count)
if(NOT pin_count EQUAL 3)
  message(FATAL_ERROR "testcard_pins: the three regions' frame 60 pins must differ pairwise")
endif()
