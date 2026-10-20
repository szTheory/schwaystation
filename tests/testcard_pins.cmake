# The test card's pinned frame hash: SHA-256 of NTSC-U frame 60 as
# schwaystation-run --frames 60 --hash prints it (the 13 metadata words, then
# the 320 x 240 rows, all little-endian; ARCHITECTURE section 7). Computed
# from the card's rules and the power-on display words by an independent
# one-off script, not by this code; every test reads it from here. A changed
# card changes it only together with the behaviour revision.
set(SCHWAYSTATION_TESTCARD_SHA256 be3538480b252f5bcfe7cb5b876a71c72d3795621b4278e405af61c425c06d71)
if(NOT SCHWAYSTATION_TESTCARD_SHA256 MATCHES "^[0-9a-f]+$")
  message(FATAL_ERROR "testcard_pins: SCHWAYSTATION_TESTCARD_SHA256 must be 64 lowercase hex digits")
endif()
string(LENGTH "${SCHWAYSTATION_TESTCARD_SHA256}" pin_length)
if(NOT pin_length EQUAL 64)
  message(FATAL_ERROR "testcard_pins: SCHWAYSTATION_TESTCARD_SHA256 must be 64 lowercase hex digits")
endif()
