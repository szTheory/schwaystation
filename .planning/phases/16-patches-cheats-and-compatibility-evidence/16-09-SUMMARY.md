---
phase: 16-patches-cheats-and-compatibility-evidence
plan: "09"
subsystem: disc-patching
tags: [bps1, crc32, raw-cue, bounded-staging, temporary-file]

requires:
  - phase: 16-08
    provides: Identity-bound patch candidates and exact raw CUE track mapping
provides:
  - Complete BPS1 target transformation with all four actions and native CRC validation
  - Bounded in-memory or private temporary-file staging before candidate publication
  - BPS malformed-input, I/O failure, source-preservation, and overlay-preservation coverage
affects: [16-10, runner-patching]

actuals:
  tokens: 14352
  tasks: 2
  commits: 3
  plan_head_before: 445d699d27b8ad8f176a977aaa82081ce2949db2
  plan_head_after: d9baaf03760cc8dc2244bce60f0851ce5cf168eb
commits: 3
plan_head_before: 445d699d27b8ad8f176a977aaa82081ce2949db2
plan_head_after: d9baaf03760cc8dc2244bce60f0851ce5cf168eb

tech-stack:
  added: []
  patterns:
    - Stage and validate a complete byte target before exposing it to the image reader
    - Use memory for small targets and a checked private temporary file for larger targets

key-files:
  created:
    - host/disc_bps.c
  modified:
    - host/disc_patch.c
    - cmake/DiscTargets.cmake
    - tests/disc/patch.c
    - docs/patches.md

key-decisions:
  - BPS source, target, and patch CRC-32 checks are mandatory format checks, not cryptographic identity.
  - A caller-supplied source SHA-256 is optional for BPS and supplements its required native CRC checks.
  - BPS targets must retain the source byte count; targets up to 1 MiB use memory and larger targets use a private temporary file.

requirements-completed:
  - PATCH-01
coverage:
  - id: D1
    description: All four BPS1 actions produce a complete deterministic target before the image can publish it.
    requirement: PATCH-01
    verification:
      - kind: unit
        ref: "ctest --preset ci --no-tests=error -R '^(disc[.]patch[.]bps)$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: Malformed BPS sizes, cursors, varints, CRCs, and staging I/O preserve source bytes and the active overlay.
    requirement: PATCH-01
    verification:
      - kind: unit
        ref: "ctest --preset ci --no-tests=error -R '^(disc[.]patch[.]bps|disc[.]patch[.]atomic|disc[.]patch[.]bounds)$' --output-on-failure"
        status: pass
    human_judgment: false

duration: 63m 51s
completed: 2026-11-26
status: complete
---

# Phase 16 Plan 09: Stage and validate complete BPS transformations Summary

**BPS1 patches now stage and validate a complete raw-track target, with checked memory or temporary-file storage and transactional rejection.**

## Performance

- **Duration:** 63m 51s
- **Started:** 2026-11-26T02:15:00Z
- **Completed:** 2026-11-26T03:18:51Z
- **Tasks:** 2
- **Files modified:** 5
- **Runtime/model:** Codex/GSD executor, `gpt-6-luna`

## Accomplishments

- Implemented BPS1 SourceRead, TargetRead, SourceCopy, and overlapping TargetCopy with checked varints, relative cursors, source/target size bounds, and exact source, target, and patch CRC-32 ranges.
- Staged targets no larger than 1 MiB in memory and larger targets in a private temporary file. Candidate publication occurs only after the entire target is materialized and verified; failed I/O closes staged storage and leaves the source and active overlay unchanged.
- Added generated BPS fixtures for all actions, both relative directions, overlapping target copies, sector-crossing writes, and a write in the final source sector. Rejection tests cover CRC corruption, truncated footer, overflowing varints, over-limit metadata, source/target size mismatch, cursor bounds, unwritten TargetCopy, late invalid action, and optional SHA-256 binding.
- Documented BPS CRC limitations, limits, optional source SHA-256, and the staging policy. Kept `tests/disc/patch.c` at 1,199 lines while preserving its existing PPF cases.

## Execution Sequence

1. Confirmed a clean isolated worktree on `worktree-agent-p16-09-20261126` at base `445d699d`; read the project instructions, Phase 16 context/research/source audit, preparation index and decisions, and plan. The published BPS specification resolved the action, varint, cursor, and CRC ranges without a new format decision.
2. Added the BPS selector test first. It failed on the intended candidate-load assertion (not compile, discovery, or fixture setup), and the TDD evidence checker accepted the report as `RED_EVIDENCE_OK`. Committed the test as `7644f97`.
3. Implemented the BPS transformer and connected it to the patch candidate path and CMake target. The first production build exposed missing cross-file prototypes under the project warning policy; added the declarations, rebuilt successfully, and committed production as `b80f959`. The tracer verification then passed `disc.patch.bps` 1/1.
4. Added malformed-input, source-preservation, large-target, and injected staging-I/O regressions plus BPS documentation. The failure-mode tests were already handled by Task 1's production code, so Task 2 required no redundant production change. Consolidated repeated test helpers to keep the file below the project 1,200-line limit. Committed as `d9baaf0`.
5. Ran the final exact selector set, docs budget, native consumer builds, and repository line-budget producer. All completed successfully; the worktree is clean before summary bookkeeping.

## Checks

- Task 1 RED: `ctest --preset ci --no-tests=error -R '^(disc[.]patch[.]bps)$' --output-on-failure --output-junit "$(git rev-parse --show-toplevel)/.gsd/tdd-evidence/16-09/red.xml"` — exit 8 as intended; `disc.patch.bps` failed at the valid candidate-load assertion. The CTest target ran in about 0.005s. `gsd_run check tdd-red-evidence .gsd/tdd-evidence/16-09/red.json --raw` — exit 0, `RED_EVIDENCE_OK`.
- Task 1 tracer: `set -o pipefail && git diff --check HEAD && cmake --preset ci && ctest --preset ci --show-only=json-v1 -R '^(disc[.]patch[.]bps)$' | python3 -c 'import json,sys; names=[t["name"] for t in json.load(sys.stdin)["tests"]]; required=set("disc.patch.bps".split()); missing=required-set(names); print("missing selectors:",sorted(missing)); sys.exit(bool(missing) or len(names)!=len(set(names)))' && cmake --build --preset ci --target schwaystation-test-disc-patch && ctest --preset ci --no-tests=error -R '^(disc[.]patch[.]bps)$' --output-on-failure` — exit 0; selector present and test passed 1/1, CTest total 0.02s.
- Final plan verification: `set -o pipefail && git diff --check HEAD && cmake --preset ci && ctest --preset ci --show-only=json-v1 -R '^(disc[.]patch[.]bps|disc[.]patch[.]atomic|disc[.]patch[.]bounds|docs[.]budgets)$' | python3 -c 'import json,sys; names=[t["name"] for t in json.load(sys.stdin)["tests"]]; required=set("disc.patch.bps disc.patch.atomic disc.patch.bounds docs.budgets".split()); missing=required-set(names); print("missing selectors:",sorted(missing)); sys.exit(bool(missing) or len(names)!=len(set(names)))' && cmake --build --preset ci --target schwaystation-test-disc-patch && ctest --preset ci --no-tests=error -R '^(disc[.]patch[.]bps|disc[.]patch[.]atomic|disc[.]patch[.]bounds|docs[.]budgets)$' --output-on-failure` — exit 0 in about 1.2s wall time; all selectors were present and all four tests passed (CTest total 0.03s).
- Repository gate budget: `cmake --preset hygiene && ctest --preset hygiene --show-only=json-v1 -R '^(policy[.]gate_budget)$' | python3 -c 'import json,sys; names=[t["name"] for t in json.load(sys.stdin)["tests"]]; required=set("policy.gate_budget".split()); missing=required-set(names); print("missing selectors:",sorted(missing)); sys.exit(bool(missing) or len(names)!=len(set(names)))' && ctest --preset hygiene --no-tests=error -R '^(policy[.]gate_budget)$' --output-on-failure` — exit 0 in 0.68s; verbose producer reported 3,901 lines against the 3,901-line limit.
- Native consumers and docs: `cmake --build --preset ci --target schwaystation-run schwaystation_libretro` — exit 0 in 0.83s; docs budget was included in the final four-selector test run and passed.
- `wc -l tests/disc/patch.c` — 1,199 lines. `git diff --check HEAD` passed as part of final verification. No hosted run or URL applies to this plan.

## Task Commits

1. **Task 1 RED: BPS transformation coverage** — `7644f97` (`test`)
2. **Task 1: BPS transformation and staging implementation** — `b80f959` (`feat`)
3. **Task 2: BPS rejection/staging regressions and documentation** — `d9baaf0` (`test`)

## Files Created/Modified

- `host/disc_bps.c` — bounded BPS1 parser, CRC validation, full-target staging, and checked reads.
- `host/disc_patch.c` — BPS candidate creation, source binding, report checks, and staged-target cleanup/application.
- `cmake/DiscTargets.cmake` — BPS transformer and shared CRC implementation in the disc image target.
- `tests/disc/patch.c` — BPS action, malformed-input, staging-I/O, source, and overlay coverage; 1,199 lines total.
- `docs/patches.md` — supported BPS behavior, limits, staging, and CRC identity limitations.

## TDD Gate Compliance

- Task 1 had valid RED evidence for `disc.patch.bps`, committed before the implementation. The post-implementation tracer build and selector passed.
- Task 2 added negative and large-staging cases after Task 1's implementation. They passed on the first run because the already-committed BPS code covered those paths; source inspection confirmed the reject-and-cleanup behavior, so no unrelated production edits were made.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Build] Added cross-file BPS interface declarations**
- **Found during:** Task 1 production build.
- **Issue:** The compiler's missing-prototype warning prevented the disc patch test target from building.
- **Fix:** Declared the internal BPS transformer and target helper functions at their implementation and caller boundaries.
- **Files modified:** `host/disc_bps.c`, `host/disc_patch.c`.
- **Verification:** The patch test target built and the Task 1 tracer selector passed.
- **Committed in:** `b80f959`.

**Total deviations:** 1 auto-fixed build issue. **Impact:** No scope change; implementation and all planned cases were completed.

## Issues Encountered

- The first native consumer command used `schwaystation-libretro`, which is not the generated CMake target name and exited 1. The CMake error identified `schwaystation_libretro`; rebuilding with `cmake --build --preset ci --target schwaystation-run schwaystation_libretro` passed.
- `.planning/STATE.md` and `.planning/ROADMAP.md` remain unchanged in this isolated plan worktree as instructed.

## User Setup Required

None.

## Next Phase Readiness

The complete BPS candidate path is available for Plan 16-10 runner integration. No blocker remains.

---
*Phase: 16-patches-cheats-and-compatibility-evidence*
*Completed: 2026-11-26*

## Self-Check: PASSED

- The required summary file exists at the Phase 16 Plan 09 path.
- Task commits `7644f97`, `b80f959`, and `d9baaf0` are ancestors of the task-commit HEAD.
