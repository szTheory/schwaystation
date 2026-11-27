---
phase: 15-guncon-and-justifier
plan: "08"
subsystem: libretro-input
tags: [libretro, lightgun, pointer, multitap]
requires:
  - phase: 15-07
    provides: "LIGHTGUN host polling, deterministic aim mapping, and frame-latched input"
provides:
  - "Capability-gated POINTER fallback for GunCon and Justifier"
  - "Integration coverage for reload/offscreen snapshots, direct-port mixing, tap rejection, and frame changes"
affects: [phase-15-verification]
actuals:
  tokens: 7735
  tasks: 2
  commits: 4
plan_head_before: e8fd4af50ab271b00702d3526af7451c63a6d67c
plan_head_after: a6f6b093edc33ace004fea976f676eea4a9ff6a2
tech-stack:
  added: []
  patterns:
    - "Capability selects one supported host device type per frame; LIGHTGUN has precedence over POINTER."
    - "Host input and controller selection are captured for a frame and applied at the next frame boundary."
key-files:
  created: []
  modified:
    - libretro/libretro.c
    - tests/libretro/host.c
    - docs/controllers.md
    - docs/libretro.md
key-decisions:
  - "POINTER supports only X/Y, PRESSED, and IS_OFFSCREEN; forced reload remains LIGHTGUN-only."
  - "GunCon and Justifier are restricted to direct physical ports; multitap sockets reject gun selections."
  - "The Justifier controller mapping does not claim two-player Justifier support."
requirements-completed: [GUN-02, SIO-05]
coverage:
  - id: D1
    description: "A POINTER-capable frontend drives GunCon and Justifier aim, trigger, and offscreen state, with LIGHTGUN precedence."
    requirement: GUN-02
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^(libretro[.]input[.]pointer|libretro[.]input[.]lightgun|libretro[.]input[.]gun_unavailable)$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: "Frame changes, direct-port mixing, and multitap selections cannot reuse stale gun input or route guns to tap sockets."
    requirement: SIO-05
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^(libretro[.]input[.]pointer|libretro[.]input[.]gun_unavailable|libretro[.]input[.]multitap|libretro[.]input[.]special|sio[.]gun[.]latch)$' --output-on-failure"
        status: pass
    human_judgment: false
duration: 29m 26s
completed: 2026-11-25
status: complete
---

# Phase 15 Plan 08: Pointer fallback and gun input boundaries Summary

**Libretro now falls back to POINTER when LIGHTGUN is unavailable, while direct-port and frame-boundary tests guard against stale or unsupported gun input.**

## Performance

- **Runtime/model:** Codex/OpenGSD 1.23.0; gsd-executor gpt-6-luna xhigh; primary model ID unavailable.
- **Duration:** 29m 26s, measured from the Plan 15-07 closeout commit to the final task commit.
- **Started:** 2026-11-25T13:49:09Z
- **Completed:** 2026-11-25T14:18:35Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments

- Added the official POINTER base-device subset for both GunCon and Justifier. LIGHTGUN takes precedence; POINTER X/Y, PRESSED, and IS_OFFSCREEN are supported, while POINTER has no forced-reload action.
- Expanded the libretro integration case to cover LIGHTGUN reload/offscreen transitions, stable serial reads, capability and selection changes at frame boundaries, mixed pad/gun use on both direct ports, and gun rejection on multitap sockets.
- Documented direct-port limits and the scope of the Justifier device mapping.

## Task Commits

1. **Task 1 RED:** `3390f94` — failing POINTER fallback integration probe.
2. **Task 1 GREEN:** `f8e2a4e` — capability-gated POINTER fallback.
3. **Task 2:** `059cc0f` — gun snapshot and topology regression coverage.
4. **Task 2 documentation:** `a6f6b09` — clarify gun topology limits.

The measured plan commit range is `e8fd4af50ab271b00702d3526af7451c63a6d67c` through `a6f6b093edc33ace004fea976f676eea4a9ff6a2`: **4 task commits**. Summary/state closeout is committed separately.

## Files Modified

- `libretro/libretro.c` — selects LIGHTGUN or POINTER once per frame and maps the supported host controls.
- `tests/libretro/host.c` — exercises fallback, precedence, frame latching, mixed direct-port selection, and tap restrictions.
- `docs/controllers.md` — documents POINTER fallback controls and coordinate mapping.
- `docs/libretro.md` — documents capability requirements and direct-port/tap limitations.

## Verification Evidence

- `cmake --preset ci` — exit 0; selector routing showed all required Task 2 tests exactly once.
- `ctest --preset ci --no-tests=error -R '^(abi[.]api_v9|docs[.]budgets|policy[.]fixture_budget)$' --output-on-failure` — exit 0, 3/3 passed.
- `cmake --build --preset ci --target schwaystation-test-libretro-host schwaystation_libretro schwaystation-test-sio-pad` — exit 0.
- Task 1 focused selectors (`libretro.input.pointer`, `libretro.input.lightgun`, `libretro.input.gun_unavailable`) — exit 0, 3/3 passed.
- Task 2 focused selectors (`libretro.input.pointer`, `libretro.input.gun_unavailable`, `libretro.input.multitap`, `libretro.input.special`, `sio.gun.latch`) — exit 0, 5/5 passed.
- `git diff --check` — exit 0. The staged hygiene hook ran on both task commits.
- A broad `scripts/hygiene.sh --tree` check was canceled after about 25 seconds (exit 130) because the configured staged hook already covered each commit; no pass is claimed for the canceled scan.
- Task 1 RED used the target pointer selector and produced the expected missing-fallback assertion failure (CTest exit 8); `.gsd/tdd-evidence/15-08/pointer-red.json` passed `gsd-tools check tdd-red-evidence` with `RED_EVIDENCE_OK`.
- Task 2 did not produce a valid RED: its required latching and tap-restriction behavior was already implemented by earlier plans. The first focused run exposed an incomplete test mock after pointer and LIGHTGUN button state were separated; the mock setup was corrected, then all five selectors passed. No product failure was hidden or waived.
- The full `cmake --workflow --preset ci` was not run here; phase verification reserves that suite. No workflow files changed, so Act added no extra coverage. No Docker resource, fuzz campaign, or detached test process was started.

## Decisions Made

- Use LIGHTGUN when both device types are advertised and use POINTER only as the fallback.
- Keep POINTER support limited to controls declared by libretro; do not infer forced reload.
- Preserve direct physical-port gun routing and reject gun selections on multitap sockets.

## Deviations from Plan

Task 2 required no production change in `libretro/libretro.c`: source inspection and the new integration assertions confirmed the existing frame latch and topology guard already satisfy its behavior. The task added regression checks and documentation instead of duplicating working logic. The Task 2 test-first attempt had no valid RED because those behaviors predated this plan.

## Issues Encountered

After separating POINTER button state from LIGHTGUN button state in the test host, the precedence case needed to set the LIGHTGUN trigger explicitly. Its initial assertion failure was a test fixture error, corrected before the final focused run; no product behavior change was required.

## User Setup Required

None.

## Next Phase Readiness

Plan 15-08 is complete and Plan 15-09 is next. The final phase verification still owns the full local workflow and hosted gates; this plan does not establish retail-game or calibrated physical-gun compatibility.

## Self-Check: PASSED

The summary exists, all four task commits are ancestors of HEAD, coverage classification passed for both deliverables, no stub markers were found in the modified source/docs, and no commit deleted tracked files.

---
*Phase: 15-guncon-and-justifier*
*Completed: 2026-11-25*
