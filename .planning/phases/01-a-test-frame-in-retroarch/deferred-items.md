# Deferred items, phase 01

- Found during plan 01-10, outside its scope: on macOS with AppleClang, `cmake --workflow --preset dev` (Debug)
  fails `abi.undefined` because `libschwaystation.a` references `memcpy_chk` and `memset_chk`. It fails the same
  way at the pre-plan commit `f270f0e` (that tree exported with `git archive` and built Debug). The `ci` preset
  (Release) passes. Two possible fixes: a `-D_FORTIFY_SOURCE=0` core option, or the two names on the Darwin
  helper row of `tests/cmake/abi.cmake`. This belongs to the plan that next edits `schway_core_options` or
  `tests/cmake/abi.cmake`.
