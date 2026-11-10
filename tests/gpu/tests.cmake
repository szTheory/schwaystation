# The existing core test executable exercises the no-content test-card and its
# native video tuple through the public API. These smoke cases keep that path
# selected while the GPU's own test executable is added by its implementation.
foreach(case IN ITEMS testcard.pixels video.tuple)
  schway_add_test(NAME gpu.${case}
    COMMAND schwaystation-test-core ${case}
    LABELS gpu portable TIMEOUT 10)
endforeach()
