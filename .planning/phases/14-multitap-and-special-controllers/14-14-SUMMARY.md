---
phase: 14-multitap-and-special-controllers
plan: "14"
subsystem: libretro-memory-card-persistence
tags: [libretro, memory-card, multitap, serialization]
requires:
  - phase: 14-13
    provides: Seven adapter-owned card sidecars with independent dirty generations
provides:
  - Regression coverage for eight-card persistence across state and lifecycle transitions
  - Documented frontend and adapter save ownership
affects: [libretro, memory-card-persistence, phase-14-verification]
actuals:
  tasks: 1
  commits: 1
plan_head_before: 12233d40ab83f87d4121e98d59f0038cad49995d
plan_head_after: 39cd8189966d38336627b79e98fef82d44cf56f1
tech-stack:
  added: []
  patterns:
    - Verify card bytes and dirty generations separately from serialized protocol state
key-files:
  created: []
  modified:
    - libretro/libretro.c
    - tests/libretro/host.c
    - docs/saves.md
key-decisions:
  - Keep physical port1A frontend-owned and outside adapter sidecar load and flush loops.
  - Preserve each sidecar path from the initial content identity across disc swaps and failed loads.
requirements-completed: []
coverage:
  - id: D1
    description: Eight card images and generations remain independent from protocol-state restore and content lifecycle transitions; port1A remains frontend-owned.
    requirement: SIO-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^libretro[.]cards[.](multitap|persistence|controls)$' --output-on-failure (exit 0, 3/3)"
        status: pass
    human_judgment: false
duration: 32m22s
completed: 2026-11-24
status: complete
---

# Phase 14 Plan 14: Eight-card lifecycle isolation Summary

**Libretro regression checks now prove that state restore, reset, failed loads and disc swaps do not roll back card contents or change their save identity.**

## Performance

- **Runtime/model:** Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable.
- **Duration:** 32m22s, exceeding the requested 30-minute plan cap by 2m22s.
- **Started:** 2026-11-24T11:20:03Z (executor-reported).
- **Completed:** 2026-11-24; the executor stopped after task verification and commit, then the orchestrator completed GSD bookkeeping.
- **Tasks:** 1.
- **Files modified:** 3.

## Accomplishments

- Extended the private libretro host harness to check all eight card byte images and generations across serialize/unserialize and reset while separately checking an in-flight protocol continuation.
- Covered failed content loading, two-disc playlist swaps, failed replacement, deinit cleanup and stable sidecar identity.
- Documented frontend versus adapter save ownership and linked the controller mapping guidance.

## Task Commits

1. **Task 1:** `39cd818` — prove libretro card lifecycle isolation.

## Files Modified

- `libretro/libretro.c` — test-only lifecycle inspection hooks; no production runtime behavior changed.
- `tests/libretro/host.c` — focused state, reset, content-swap and failure-isolation assertions.
- `docs/saves.md` — ownership and state-boundary guidance.

## Checks

- `git diff --check` — exit 0.
- `cmake --preset ci` — exit 0.
- `cmake --build --preset ci --target schwaystation-test-libretro-host schwaystation_libretro` — exit 0.
- `ctest --preset ci --show-only=json-v1 -R '^libretro[.]cards[.](multitap|persistence|controls)$'` — exit 0; all three selectors registered.
- `ctest --preset ci --no-tests=error -R '^libretro[.]cards[.](multitap|persistence|controls)$' --output-on-failure` — exit 0; 3/3 passed in 18.34s.
- The direct `cards-multitap` host run — exit 0. It was unexpectedly green because the production invariants were already present; no artificial RED commit or runtime change was introduced.

No broad suite, hosted CI, Act, or Docker run was needed for this test and documentation slice. The final review and integrated verification remain assigned to Plan 14-15.

## Runtime Findings

The passing lifecycle behavior already existed in production. This plan adds test-only observation hooks and durable regression coverage; it does not claim a new runtime fix. The executor exceeded its 30-minute bound while building the lifecycle assertions, stopped before any further implementation, and left only GSD closeout to the orchestrator.

## Self-Check: PASSED

- The implementation, test and documentation paths exist.
- Commit `39cd818` is present on the active phase branch.
- All three owned CTest selectors passed, and no production runtime behavior changed.
- The shared SIO-01 requirement remains gated until the remaining declaring plan completes.

---
*Phase: 14-multitap-and-special-controllers*
*Completed: 2026-11-24*
