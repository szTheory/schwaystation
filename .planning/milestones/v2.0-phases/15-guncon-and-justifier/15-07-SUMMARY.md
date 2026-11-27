---
phase: 15-guncon-and-justifier
plan: "07"
subsystem: libretro-input
tags: [libretro, lightgun, guncon, justifier, frame-latch]
requires:
  - phase: 15-04
    provides: [GunCon and Justifier device handling]
  - phase: 15-05
    provides: [bounded beam sensing and live VRAM sampling]
provides:
  - Official GunCon and Justifier libretro selection on direct physical ports
  - Frame-latched LIGHTGUN coordinates, controls, and fail-closed capability handling
  - Documented controller mappings and frontend capability limits
affects: [15-08-PLAN.md, GUN-02, SIO-05]
actuals:
  tokens: 11922
  tasks: 2
  commits: 4
commits: 4
plan_head_before: ea41417b2400beb41fadb0c1336285159265d6e8
plan_head_after: c3bb282784f68528173d1415542ba9770cdbfffa
tech-stack:
  added: []
  patterns: [frame-latched host input, device-type capability gate, test-only internal inspection]
key-files:
  created: []
  modified: [libretro/libretro.c, tests/libretro/host.c, docs/controllers.md, docs/libretro.md]
key-decisions:
  - "Use the official LIGHTGUN base ID and Justifier subclass 2; do not offer guns behind multitaps."
  - "Treat libretro capabilities as device-type support only; zero input_state values remain neutral and cannot identify absent controls."
patterns-established:
  - "Query device-type capability once per retro_run before queueing the frame snapshot."
  - "Test unsupported sensor behavior through the real host, serial, and GPU paths without production test exports."
requirements-completed: [GUN-02, SIO-05]
coverage:
  - id: D1
    description: "GunCon and Justifier use official libretro IDs and map supported LIGHTGUN controls through a single frame-latched snapshot."
    requirement: SIO-05
    verification:
      - kind: integration
        ref: "libretro.input.lightgun; libretro.input.multitap; libretro.input.descriptors"
        status: pass
    human_judgment: false
  - id: D2
    description: "Missing callbacks and capability responses clear aim and trigger, cannot leave a sensor hit, and recover when support returns."
    requirement: GUN-02
    verification:
      - kind: integration
        ref: "libretro.input.gun_unavailable"
        status: pass
    human_judgment: false
  - id: D3
    description: "Controller documentation explains all LIGHTGUN mappings and the limits of type-level capability reporting."
    requirement: SIO-05
    verification:
      - kind: other
        ref: "docs/controllers.md#special-controllers; docs/libretro.md#controller-input"
        status: pass
    human_judgment: false
duration: "about 24 minutes from first task commit to last task commit; active start not separately recorded"
completed: 2026-11-25
status: complete
---

# Phase 15 Plan 07: Official LIGHTGUN Controls Summary

**GunCon and Justifier now use official libretro LIGHTGUN IDs, one frame-latched input snapshot, and explicit fail-closed behavior when host support is missing.**

## Performance

- **Duration:** About 24 minutes across the recorded task-commit window; the actual start and active time were not separately captured.
- **Commit window:** 2026-11-25T13:17:30Z (`15-07` first task commit) to 2026-11-25T13:41:17Z (last task commit).
- **Tasks:** 2.
- **Files modified:** 4.
- **Runtime/model:** Codex/OpenGSD 1.20.0 executor, `gpt-6-luna`; reasoning effort was not exposed by the runtime.

## Accomplishments

- Exposed GunCon as `RETRO_DEVICE_LIGHTGUN` and Justifier as `RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_LIGHTGUN, 2)` on direct physical ports only. The existing adapter polls base LIGHTGUN coordinates, trigger, offscreen, reload, and class-specific buttons before submitting one snapshot.
- Added a host regression that produces a real bright-center sensor result, removes the input callback, and verifies stale aim, trigger, and sensor state clear. Failed capability queries and missing capability bits also stay unsupported without log spam; restoring support recovers input.
- Documented the complete mappings and that libretro reports device types, not per-control availability. Neutral zero input cannot distinguish a missing control from a released control.

## Task Commits

1. **Wire complete LIGHTGUN input through the test host** — `51d75cf` (RED), `a1b6955` (implementation).
2. **Fail closed on missing host class and publish mapping** — `6ada3d1` (regression coverage), `c3bb282` (documentation).

**Plan metadata:** the executor's GSD SDK closeout attempt could not create `.git/index.lock` (`Operation not permitted`). The phase orchestrator completed the same GSD metadata closeout from a context permitted to update the branch.

## Files Modified

- `libretro/libretro.c` — official controller selection, capability check, coordinate mapping, and test-only sensor/frame setup seams.
- `tests/libretro/host.c` — GunCon/Justifier mapping, descriptor, unsupported-host, recovery, and no-stale-hit integration checks.
- `docs/controllers.md` — complete gun mappings and unsupported-host semantics.
- `docs/libretro.md` — discoverable controller capability summary and link.

## Verification Evidence

- Initial preflight and the Task 1 RED evidence are recorded in the `input-lightgun-red` evidence files under `.gsd/tdd-evidence/15-07/`; the classifier accepted the intended mapping assertion. Task 1's focused GREEN build and selector set passed 3/3.
- Final preflight: `git diff --check` passed; `LC_ALL=C sort -c tests/libretro/inventory.txt` passed; `cmake --preset ci` configured successfully; the four required selectors were present exactly once.
- Final local verification: built `schwaystation-test-libretro-host`, `schwaystation-libretro-card-test`, and `schwaystation_libretro`; `libretro.input.gun_unavailable`, `libretro.input.lightgun`, `libretro.input.descriptors`, and `libretro.input.multitap` passed 4/4 in 0.02 seconds.
- Task 2's runtime capability and transition-logging behavior was already in the Task 1 implementation. The new unsupported-host integration test therefore passed once its test-only 320×240 bright scan window was prepared; no valid RED failure was produced and no runtime change was made merely to force one. The test uses test-build-only symbols; the descriptor selector loads the production module and passed.
- No workflow changed, so hosted CI and `act` were not run. No quality metric producer or Docker resource applied to these changes. The configured staged-file hygiene hook ran on the commits; an extra full-tree hygiene scan was stopped after exceeding 30 seconds.

## Decisions and Deviations

- Kept gun support to direct physical ports. POINTER fallback remains with Plan 15-08; no tap-gun behavior was introduced.
- Capability checks cover the LIGHTGUN device type only. Individual zero controls are neutral by API contract and do not imply unsupported controls.
- Task 2's RED phase was omitted after inspecting the prior Task 1 implementation and confirming that it already failed closed for null callbacks and capability loss. The regression test adds coverage around that existing implementation instead of removing correct code to manufacture a failure.

**Total deviations:** 1 TDD sequencing adjustment (runtime behavior already existed; expanded regression coverage).
**Impact on plan:** Both planned tasks are covered; implementation remains within the authorized direct-port scope.

## Issues Encountered

- The first test-only bright-frame helper ran before a scan window was configured, so it could not create a sensor hit. The test seam now programs the GPU through its bus registers before filling the active scan window, and the focused integration test confirms the resulting hit.

## User Setup Required

None - no external service or owner hardware setup required.

## Next Phase Readiness

Plan 15-07 is complete. Plan 15-08 owns POINTER fallback behavior; phase review, verification, and release remain outside this plan.

## Self-Check: PASSED

- The summary exists, and all four task commits are ancestors of HEAD.
- The measured plan range from `ea41417` contains four commits and ends at `c3bb282`.
- Coverage classification passed all 3 deliverables; the final focused test set passed 4/4 and no new stub patterns were found.

---
*Phase: 15-guncon-and-justifier*
*Completed: 2026-11-25*
