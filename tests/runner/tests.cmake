# schwaystation-run: frame hashes, PNG output, bench report, usage and exit statuses, build revision.

# Fault prevented: the runner's printed card hash, the line hosts and CI
# compare, drifts from the pinned card or differs on another platform. The
# command goes through CMAKE_CROSSCOMPILING_EMULATOR, so an emulated lane runs
# its own guest binary.
schway_add_test(NAME runner.hash
  COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:schwaystation-run> --frames 60 --hash
  PASS_REGEX "frame 60 sha256 ${SCHWAYSTATION_TESTCARD_SHA256}"
  LABELS portable runner TIMEOUT 30)
