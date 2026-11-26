---
phase: 16-patches-cheats-and-compatibility-evidence
plan: "04"
subsystem: libretro
tags: [C, libretro, GameShark, cheats]
requires:
  - phase: 16-patches-cheats-and-compatibility-evidence
    provides: [bounded shared cheat parser and checked RAM-write API from plan 16-03]
provides:
  - libretro cheat callbacks with frame-boundary RAM application
  - callback rejection, ordering, limits, and lifecycle regression coverage
affects: [libretro serialization, cheat capability evidence]
actuals:
  tokens: 3487
  tasks: 2
  commits: 3
tech-stack:
  added: []
  patterns:
    - libretro owns a bounded host cheat table and applies it before core frame execution
key-files:
  created: []
  modified:
    - libretro/libretro.c
    - tests/libretro/host.c
    - docs/libretro.md
key-decisions:
  - Keep cheat parsing and configuration in the libretro adapter, using the shared parser and checked RAM write.
  - Clear cheats on unload, deinit, and new content load; preserve them across ordinary reset.
patterns-established:
  - Apply enabled entries once after input preparation and before schway_run_frame.
requirements-completed:
  - CHEAT-01
coverage:
  - id: callback-application
    description: Libretro callbacks apply supported byte and halfword writes at the frame boundary.
    requirement: CHEAT-01
    verification:
      - kind: integration
        ref: libretro.cheats.apply focused CTest selector
        status: pass
    human_judgment: false
  - id: callback-controls
    description: Rejection, ordering, disabled entries, aggregate limits, reset, and content lifecycle preserve the documented contract.
    requirement: CHEAT-01
    verification:
      - kind: integration
        ref: libretro.cheats.controls focused CTest selector
        status: pass
    human_judgment: false
duration: 31m
completed: 2026-11-26
status: complete
plan_head_before: 445d699d27b8ad8f176a977aaa82081ce2949db2
plan_head_after: 6b43445287c32b951cbe7882907108b756639424
---

# Phase 16 Plan 04: Libretro GameShark Callbacks Summary

**Libretro GameShark callbacks now apply validated `30` byte and `80` halfword writes before each frame, with transactional rejection and explicit lifecycle behavior.**

## Performance

- **Duration:** 31 minutes (3 minutes past the plan's 20–28 minute target)
- **Started:** 2026-11-26T02:14:51Z
- **Completed:** 2026-11-26T02:45:18Z
- **Runtime/model:** Codex/OpenGSD; configured executor model `gpt-6-luna` (model ID not exposed)
- **Tasks:** 2
- **Files modified:** 3
- **Task commits:** 3 measured from the plan ledger

## Accomplishments

- Implemented bounded callback parsing, entry reset, warning diagnostics on rejected input, and one ordered application pass before `schway_run_frame`.
- Verified invalid index, malformed final line, and aggregate-limit rejection preserve all RAM bytes and serialized core state immediately; the last valid entry remains active.
- Covered duplicate-address ordering, disable/re-enable, reset versus power reset, and unload, reload, and deinit isolation. Documented the callback contract and linked the shared cheat syntax.
- Kept the `.info` `cheats` claim false pending the serialization tests and claim guard.

## Task Commits

1. **Task 1 RED: Add libretro frame-application test** — `bb91929` (`test`)
2. **Task 1 GREEN: Apply libretro cheats before frames** — `c5de136` (`feat`)
3. **Task 2: Cover rejection and lifecycle behavior** — `6b43445` (`test`, includes docs)

The measured plan range is three commits, from `445d699d27b8ad8f176a977aaa82081ce2949db2` through `6b43445287c32b951cbe7882907108b756639424`. The summary commit is separate.

## TDD Execution Evidence

Task 1's RED run executed `libretro.cheats.apply` and failed only on the three expected RAM-write assertions (CTest exit 8, 1 test, 0.03 s). `gsd-tools.cjs check tdd-red-evidence .gsd/tdd-evidence/16-04/task1-red.json --raw` returned `RED_EVIDENCE_OK` (exit 0). The semantic record confirms there were no setup, build, or unrelated assertion failures. The GREEN implementation then passed the focused selector.

Task 2 adds proof for behavior implemented in Task 1, so its focused controls test was run against that implementation and passed; no additional production change was needed. The initial test iterations exposed only test setup issues, described below.

## Verification Evidence

All checks ran locally from the worktree root. The exact Task 1 `<verify>` command passed once for GREEN and again after its commit as the tracer feedback gate. Each run returned exit 0; CTest passed `libretro.cheats.apply` 1/1 (0.02 s and 0.03 s). The command was:

```sh
set -o pipefail && git diff --check HEAD && cmake --preset ci && ctest --preset ci --show-only=json-v1 -R '^(libretro[.]cheats[.]apply)$' | python3 -c 'import json,sys; names=[t["name"] for t in json.load(sys.stdin)["tests"]]; required=set("libretro.cheats.apply".split()); missing=required-set(names); print("missing selectors:",sorted(missing)); sys.exit(bool(missing) or len(names)!=len(set(names)))' && cmake --build --preset ci --target schwaystation-test-libretro-host schwaystation-libretro-card-test && ctest --preset ci --no-tests=error -R '^(libretro[.]cheats[.]apply)$' --output-on-failure
```

The exact Task 2 selector command returned exit 0; the selector preflight found both unique tests, and CTest passed 2/2 in 0.10 s. The host/module build reported no work on this final run. `docs.budgets` passed 1/1 in 0.03 s. Inventory sorting, `git diff --check`, and the initial `cmake --preset ci` configuration passed.

```sh
set -o pipefail && git diff --check HEAD && LC_ALL=C sort -c tests/libretro/inventory.txt && cmake --preset ci && ctest --preset ci --show-only=json-v1 -R '^(libretro[.]cheats[.]apply|libretro[.]cheats[.]controls)$' | python3 -c 'import json,sys; names=[t["name"] for t in json.load(sys.stdin)["tests"]]; required=set("libretro.cheats.apply libretro.cheats.controls".split()); missing=required-set(names); print("missing selectors:",sorted(missing)); sys.exit(bool(missing) or len(names)!=len(set(names)))' && cmake --build --preset ci --target schwaystation-test-libretro-host schwaystation-libretro-card-test && ctest --preset ci --no-tests=error -R '^(libretro[.]cheats[.]apply|libretro[.]cheats[.]controls)$' --output-on-failure
```

```sh
ctest --preset ci --no-tests=error -R '^docs[.]budgets$' --output-on-failure
```

No hosted run was part of this plan; hosted evidence and the broad workflow remain with phase verification.

## Files Modified

- `libretro/libretro.c` — owns and resets the shared cheat table, rejects invalid replacements without publishing them, logs a bounded warning when logging is available, and applies writes before frame execution.
- `tests/libretro/host.c` — exercises real `retro_run`, rejection atomicity, ordering, limits, disable/reset, and lifecycle behavior through the loaded module.
- `docs/libretro.md` — describes supported libretro cheat behavior and links to `docs/cheats.md`.

## Decisions Made

- Used a bounded string scan before invoking the shared parser; callback rejection leaves the prior table untouched.
- Applied cheats after options and input setup, immediately before `schway_run_frame`.
- Used middle-RAM test addresses with a no-cheat sentinel check so the test-card guest does not mask the callback result.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Test harness blocker] Allowed the cheat test modes to use the card-test module**
- **Found during:** Task 1 RED
- **Issue:** The host intentionally rejected test-only module exports before reaching the new cheat assertions.
- **Fix:** Classified both cheat test modes alongside the existing modes that may load the in-process card-test module; the guard itself remains intact.
- **Files modified:** `tests/libretro/host.c`
- **Verification:** The corrected target reached and failed only on the planned RAM assertions, then passed after implementation.
- **Committed in:** `bb91929`

**Total deviations:** 1 auto-fixed test harness blocker. No production scope changed.

## Issues Encountered

- Early controls assertions used addresses in guest-used RAM and expected the lower-index write after `retro_reset` even though the higher-index overlapping entry remained enabled. The test now proves its middle-RAM location is untouched without cheats and expects the active higher-index write after reset.
- An early proposed `80` code at aligned offset `0x80032` was valid. The invalid-final-line case now uses misaligned `0x80033`; the parser and adapter behavior did not need changes.

## User Setup Required

None.

## Next Phase Readiness

The libretro adapter is ready for plan 16-05 to wrap core state with normalized cheat configuration. The `.info` cheat claim remains false until that coverage and the claim guard land. `STATE.md` and `ROADMAP.md` were intentionally left unchanged in the isolated plan worktree for the orchestrator to update after integration.

---
*Phase: 16-patches-cheats-and-compatibility-evidence*
*Completed: 2026-11-26*

## Self-Check: PASSED

- The SUMMARY and all three owned files exist.
- Task commits `bb91929`, `c5de136`, and `6b43445` are ancestors of the current plan head.
- `git diff --check HEAD` passed before the summary commit.
