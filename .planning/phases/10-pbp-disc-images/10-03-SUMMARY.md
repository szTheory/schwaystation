---
phase: 10-pbp-disc-images
plan: "03"
subsystem: host-media
tags: [c, pbp, deflate, cmake, ctest]
requires:
  - phase: 10-pbp-disc-images
    provides:
      - pinned modular MIT inflater and private symbol aliases
      - PBP header test seam
provides:
  - bounded raw-block wrapper with exact input/output consumption
  - atomic failure behavior for malformed and size-mismatched streams
  - registered inflate tests for CHD ON and OFF configurations
affects: [host-media, disc-testing]
actuals:
  tasks: 2
  commits: 1
plan_head_before: b5c9c95
plan_head_after: c108c18
commits: 1
tech-stack:
  added: []
  patterns:
    - bounded scratch decode followed by success-only copy to preserve failure atomicity
key-files:
  created:
    - host/pbp_inflate.c
    - host/pbp_inflate.h
  modified:
    - cmake/Miniz.cmake
    - tests/disc/pbp.c
    - tests/disc/tests.cmake
    - tests/disc/inventory.txt
    - docs/media-testing.md
key-decisions:
  - Accept a raw block only after terminal status, exact input consumption and exact output length.
  - Keep a one-byte overflow sentinel in bounded scratch and leave caller output untouched on every failure.
requirements-completed: []
coverage:
  - id: I1
    description: Known stored and fixed-Huffman raw streams decode to exact bytes, and malformed, truncated, trailing and size-mismatched streams fail atomically.
    requirement: PBP-01
    verification:
      - kind: unit
        ref: "build/ci/tests/schwaystation-test-disc-pbp --inflate; exit 0"
        status: pass
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^disc[.]image[.]pbp_(headers|inflate)$' --output-on-failure with CHD ON; exit 0, 2/2"
        status: pass
    human_judgment: false
  - id: I2
    description: The wrapper uses only the private standalone decoder and neither CHD configuration introduces a system zlib link.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: "CHD OFF configure/build and inflate selector; exit 0; archive and wrapper nm checks show only prefixed standalone symbols"
        status: pass
      - kind: integration
        ref: "CHD ON build of schwaystation-test-disc-pbp and schwaystation-run, link command and otool checks; exit 0, no system zlib"
        status: pass
    human_judgment: false
  - id: I3
    description: The exact inventory is sorted and the fixed repository-check ceiling remains satisfied.
    requirement: PBP-01
    verification:
      - kind: other
        ref: "LC_ALL=C sort -cu tests/disc/inventory.txt; git diff --check; line_budget.cmake; exit 0, 3900/3901"
        status: pass
    human_judgment: false
duration: "About five minutes for resumed validation and closeout; the inherited active interval was not recorded."
completed: 2026-11-21
status: complete
---

# Phase 10 Plan 03: Bounded Raw Inflater Summary

**PBP now has a private raw inflater wrapper that accepts only complete, exact-length blocks and never exposes partial output.**

## Performance

- **Runtime/model:** Codex session; primary model ID is not exposed. Two read-only Plan 10-03 audits used `gpt-6-luna` xhigh.
- **Elapsed interval:** About five minutes for resumed validation and closeout; the prior executor interval was not preserved, so total active time is unknown.
- **Task commit:** `c108c18`.
- **Observed bottleneck:** Reconfiguring CHD ON rebuilt 55 small actions in the existing CI tree; it completed in seconds. The targeted checks remained cheap, so no wider suite was started.

## Accomplishments

- Added `schway__pbp_inflate_raw`, which initializes a fresh decoder for every block, uses raw mode and bounded non-wrapping scratch, and copies bytes only after terminal success plus exact compressed-input and output-length checks.
- Added fixed stored-block and fixed-Huffman controls, including trailing, truncated, short/long expected size, overflow, empty-input and empty-output failures. Every failure asserts the caller's sentinel output remains unchanged.
- Registered `disc.image.pbp_inflate` in the sorted inventory and documented the private host contract.
- Proved the wrapper references the prefixed standalone `tinfl` symbol; the CHD-ON runner and PBP test link commands contain no system `-lz`, and `otool -L` shows no system zlib dependency.
- The fixed repository gate remains 3,900/3,901 lines.

## Verification

- CHD-OFF configure, targeted build, direct `--inflate` test, focused CTest selectors, and private-symbol/link checks all exited 0 before the inherited checkpoint.
- `LC_ALL=C sort -cu tests/disc/inventory.txt && cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake && git diff --check`: exit 0; gate 3,900/3,901.
- `ctest --preset ci --no-tests=error -N -R '^disc[.]image[.]pbp_(headers|inflate)$'`: exit 0; both selectors present before the CHD-ON build.
- `cmake --preset ci -DSCHWAYSTATION_CHD=ON -DSCHWAYSTATION_BUILD_CHD_TOOL=ON`: exit 0.
- `cmake --build --preset ci --target schwaystation-test-disc-pbp schwaystation-run`: exit 0; 55 build actions.
- `ctest --preset ci --no-tests=error -R '^disc[.]image[.]pbp_(headers|inflate)$' --output-on-failure`: exit 0; 2/2 in 0.02s.
- CHD-ON archive/object `nm`, final test/runner link commands and `otool -L`: exit 0; private decoder symbol is prefixed and no system zlib dependency appears.
- No workflow changed, so `act` and hosted CI are not applicable to this plan. Phase review and broad hosted validation remain in Plan 10-13.

## Next Plan Readiness

Plan 10-04 produces deterministic build-local PBP bytes and validates fixture identity and geometry before later parser or guest work. Phase-wide PBP-01 remains pending.
