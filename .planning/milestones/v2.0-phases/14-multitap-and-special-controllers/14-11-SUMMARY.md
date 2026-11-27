---
phase: 14-multitap-and-special-controllers
plan: "11"
subsystem: libretro
tags: [libretro, multitap, controller-input]
requires:
  - phase: 14-10
    provides: Default two-player callback-to-serial test seam
provides:
  - Independent reload-required multitap options for both physical controller ports
  - Compact descriptors and serial routing for all four tap topologies
affects: [phase-14-libretro-input, controller-mapping]
actuals:
  tokens: 8624
  tasks: 1
  commits: 3
plan_head_before: 888518b65caae1a6a3ac53da3ea74dd28d4adcd0
plan_head_after: 474aa6a3ea1f4c35d76a36e78a31ee6e7ea9991c
tech-stack:
  added: []
  patterns:
    - Build controller descriptors from the active compact player topology
    - Route each frontend player through a bounded physical-port/socket mapping
key-files:
  created:
    - docs/controllers.md
  modified:
    - libretro/libretro.c
    - tests/libretro/host.c
key-decisions:
  - Sample multitap options only after a content load succeeds so rejected content leaves the running topology unchanged.
  - Accept only exact NONE, JOYPAD, and ANALOG device IDs; do not mask arbitrary subclasses into accepted devices.
patterns-established:
  - Exercise frontend inputs through real endpoint serial replies and count polls per frame.
requirements-completed: [SIO-01]
coverage:
  - id: D1
    description: "Both taps expose the expected compact player order, with independent endpoint replies and one frontend poll per frame."
    requirement: SIO-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^libretro[.]input[.](multitap|descriptors)$' --output-on-failure (exit 0, 2/2)"
        status: pass
    human_judgment: false
duration: 24min
completed: 2026-11-24
status: complete
---

# Phase 14 Plan 11: Compact libretro multitap players Summary

**Libretro maps active players compactly across either or both taps and keeps every player on its own physical socket.**

## Performance

- **Duration:** 24 minutes
- **Started:** 2026-11-24T09:42:42Z
- **Completed:** 2026-11-24
- **Runtime/model:** Codex GSD executor, configured gpt-6-luna xhigh; primary runtime model ID unavailable
- **Tasks:** 1
- **Files modified:** 3

## Accomplishments

- Added disabled-by-default schwaystation_multitap_port1 and schwaystation_multitap_port2 options. They apply after a successful content load and require a reload.
- Published controller info and input descriptors for the compact order across no taps, either single tap, and both taps.
- Routed each active frontend player to its own controller snapshot and physical serial endpoint. The host test confirms distinct replies for all eight sockets and exactly one input_poll per frame.
- Rejected arbitrary device subclasses and reset all eight device-selection and mode-toggle slots during deinit.
- Documented the option values and active player mapping in docs/controllers.md.

## Task Commits

1. **Task 1 RED: Cover all tap topologies and serial routing** — b72c468 (test)
2. **Task 1 RED: Preserve topology after rejected content reload** — b1e8fd3 (test)
3. **Task 1 GREEN: Expose compact libretro multitap players** — 474aa6a (feat)

## Files Created/Modified

- libretro/libretro.c — tap options, compact topology publication, eight-player input polling and physical endpoint routing.
- tests/libretro/host.c — all-topology callback-to-wire checks, invalid subclass and inactive-player checks, rejected-reload stability.
- docs/controllers.md — frontend player order and tap-option mapping table.

## Decisions Made

- Tap topology changes take effect only after content loads successfully. A rejected replacement leaves the existing frontend descriptors and running topology in place.
- Controller selection accepts only exact NONE, JOYPAD, and ANALOG IDs. Masking subclasses would accept devices the adapter does not implement.

## TDD Evidence

- libretro.input.multitap RED failed on missing tap options, expanded endpoint routing, and subclass rejection. The evidence checker returned RED_EVIDENCE_OK; semantic review confirmed the target executed and failed on planned assertions.
- A second RED failed because an invalid executable load republished the tap topology before rejecting content. Its evidence also returned RED_EVIDENCE_OK.
- GREEN moved topology application after successful content setup. The final focused selectors passed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Keep topology unchanged when content reload fails**
- **Found during:** Task 1 focused verification
- **Issue:** The first implementation applied tap options before content validation, allowing a failed replacement to change descriptors while the old instance remained loaded.
- **Fix:** Apply and publish options only after BIOS/content loading succeeds.
- **Files modified:** libretro/libretro.c, tests/libretro/host.c
- **Verification:** The failed-reload regression is included in libretro.input.multitap; the final selector passed.
- **Committed in:** 474aa6a

## Checks and Iteration Time

- git diff --check — exit 0.
- LC_ALL=C sort -cu tests/libretro/inventory.txt — exit 0; inventory unchanged.
- CTest selector preflight for libretro.input.multitap and libretro.input.descriptors — exit 0; both registered selectors present.
- cmake --preset ci — exit 0; reused the existing build/ci Ninja tree.
- cmake --build --preset ci --target schwaystation-test-libretro-host schwaystation_libretro — exit 0.
- ctest --preset ci --no-tests=error -R '^libretro[.]input[.](multitap|descriptors)$' --output-on-failure — exit 0; 2/2 passed after the final fix.
- Both test-first RED runs failed as expected and passed gsd-tools check tdd-red-evidence; ignored records are under .gsd/tdd-evidence/14-11/.

No broad suite, hosted CI, Act run, or Docker resource was needed for this focused slice. Final phase review and integrated verification remain assigned to Plan 15. No user action is needed.

## Next Phase Readiness

Plan 14-11 completes the libretro tap topology slice. Plan 14-12 can add special controller device IDs and their host mappings against the established eight-player routing.

## Self-Check: PASSED

- Summary and documentation files exist at their expected paths.
- Task commits b72c468, b1e8fd3, and 474aa6a are ancestors of HEAD.
- The changed implementation, tests, and documentation contain no new TODO/FIXME or placeholder stubs.
- No new network endpoint, authorization path, file-access pattern, or schema boundary was introduced.

---
*Phase: 14-multitap-and-special-controllers*
*Completed: 2026-11-24*
