---
phase: 16-patches-cheats-and-compatibility-evidence
plan: "12"
subsystem: host
tags: [c, cmake, DAT, compatibility, evidence]
requires:
  - phase: 16-10
    provides: Local library and compatibility inventory conventions.
provides:
  - Bounded host-side DAT occurrence parsing and aggregate inventory output in the runner.
  - Private per-occurrence fingerprints and exact evidence joins that leave unmatched entries unknown.
affects: [compatibility, library-inventory, evidence]
actuals:
  tokens: 13936
  tasks: 2
  commits: 9
  plan_head_before: be22dcb01078afff77ca673a6736f0bbbe085973
  plan_head_after: 595ae898f363122df3798229447d9f3886b30dc3
tech-stack:
  added: []
  patterns:
    - Bounded host parsing with source-order occurrence identities and private evidence reports.
    - Hash serial bytes for duplicate accounting without retaining or publishing serial strings.
key-files:
  created:
    - host/dat_inventory.c
    - host/dat_inventory.h
    - docs/compatibility-inventory.md
  modified:
    - CMakeLists.txt
    - runner/main.c
    - tests/compat/tests.cmake
    - tests/compat/library_tests.c
key-decisions:
  - Keep DAT parsing and file access in the host/runner path; publish only the pinned source identity and aggregate counts by default.
  - Join supplied results by source digest, ordinal, entry/content fingerprints, predicate, profile, status, and evidence.
patterns-established:
  - Missing and repeated serials affect accounting only; occurrence order remains the stable private join key.
requirements-completed: [COMPAT-01]
coverage:
  - id: D1
    description: The pinned Redump DAT is accounted for by source occurrence, with untested entries remaining unknown.
    requirement: COMPAT-01
    verification:
      - kind: integration
        ref: "build/ci/schwaystation-run --library-inventory $DAT_TMP_FILE after the D-06 SHA-256 check"
        status: pass
    human_judgment: false
  - id: D2
    description: Synthetic tests cover missing/repeated/encoded serials, occurrence reports, exact evidence joins, malformed inputs, and bounds.
    requirement: COMPAT-01
    verification:
      - kind: unit
        ref: "ctest --preset ci --no-tests=error -R '^(compat[.]inventory[.]accounting|compat[.]inventory[.]controls|compat[.]library[.]synthetic_result|compat[.]library[.]controls)$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D3
    description: The reproduction guide states the data boundary and that inventory counts do not establish playability.
    requirement: COMPAT-01
    verification:
      - kind: other
        ref: docs/compatibility-inventory.md
        status: pass
    human_judgment: false
metrics:
  duration: 46m 36s
  completed: 2026-11-26
status: complete
---

# Phase 16 Plan 12: Bounded DAT occurrence inventory Summary

**The runner now counts every pinned DAT occurrence while keeping titles and serials out of public output and leaving untested entries unknown.**

## Performance

- **Duration:** 46m 36s
- **Started:** 2026-11-26T04:27:08Z
- **Completed:** 2026-11-26T05:13:44Z
- **Executor model/runtime:** gpt-6-luna / Codex OpenGSD executor
- **Tasks:** 2
- **Plan files modified:** 7

## Accomplishments

- Added a bounded clrmamepro parser in the host adapter and a non-emulating `--library-inventory` runner mode. Public output contains source pin/digest and counts only; optional private output retains one fingerprint row per source occurrence.
- Added exact evidence joins with source, ordinal, entry/content identity, predicate, profile, status, and evidence digest checks. Entries without evidence remain unknown, and identity matching is not described as playability.
- Verified the pinned D-06 DAT at SHA-256 `52978215854d307bfb5353052c9ce3c34995310a970d586f0100bb67f7de01a8`: 13,407 entries, 0 pass, 0 fail, 13,407 unknown, 177 missing serials, and 988 entries belonging to duplicate-serial groups. The raw DAT was removed after the check.

## Verification Evidence

Checks ran from the repository root. The final order was: build and pass the four exact acceptance selectors; fetch the pinned DAT to a task-specific temporary file; verify its digest before parsing; run the inventory command; validate the emitted aggregate counts; remove the raw DAT and confirm no task temporary remains. No owner BIOS or disc paths were used.

- `git diff --check HEAD && LC_ALL=C sort -cu tests/compat/inventory.txt && ctest --preset ci --show-only=json-v1 -R '^(compat[.]inventory[.]accounting|compat[.]inventory[.]controls|compat[.]library[.]synthetic_result|compat[.]library[.]controls)$'` with an exact unique-selector Python assertion, followed by `cmake --preset ci` — **exit 0**.
- `cmake --build --preset ci --target schwaystation-run schwaystation-run-test schwaystation-test-library` — **exit 0**.
- `ctest --preset ci --no-tests=error -R '^(compat[.]inventory[.]accounting|compat[.]inventory[.]controls|compat[.]library[.]synthetic_result|compat[.]library[.]controls)$' --output-on-failure` — **exit 0**, 5/5 tests passed, including `compat.features.preflight`.
- Pinned source fetch from `https://raw.githubusercontent.com/libretro/libretro-database/12c21cf79f1e20eff6ac68b80875816c717d0a7a/metadat/redump/Sony%20-%20PlayStation.dat` and `shasum -a 256 "$DAT_TMP_FILE"` — **exit 0**, digest matched D-06. `build/ci/schwaystation-run --library-inventory "$DAT_TMP_FILE"` — **runner exit 0** with the counts above. The first surrounding shell wrapper returned **exit 1** because its awk assertion misread correct output; a map-based check against the captured output returned **exit 0**. The cleanup trap removed the DAT, and the task-specific temp paths were confirmed absent.
- Final stub scan and `git diff --check be22dcb01078afff77ca673a6736f0bbbe085973..HEAD` — **exit 0**; no TODO/FIXME/placeholder stubs were found in the seven plan files.

## TDD Gate Compliance

- Task 1 RED: `compat.inventory.accounting` failed as expected with **exit 8** before the runner mode existed; committed as `a364ce7`. GREEN: parser/runner commit `d158042`; the exact accounting selector passed.
- Task 2 RED/GREEN: ambiguous duplicate-serial-field test failed with **exit 8** (`f8c8fac`); `96b5771` fixed detection when the first serial value was empty. Evidence/bounds tests were added in `1e6d823`; the source pin assertion exposed a mislabeled synthetic source, fixed in `548a08a`.
- The pinned DAT check then exposed a valid UTF-8 serial rejected by the parser. The encoded-serial regression failed with **exit 8** (`7d8d5e5`); `595ae89` hashes decoded serial bytes for duplicate accounting. The exact acceptance selector set passed afterward.

## Task Commits

1. **Task 1: Parse source occurrences into private inventory accounting** — `a364ce7`, `d158042`, `548a08a`.
2. **Task 2: Prove missing/duplicate and malformed DAT handling** — `f8c8fac`, `96b5771`, `1e6d823`, `4e9b135`, `7d8d5e5`, `595ae89`.

The nine task commits are measured from `plan_head_before` through `plan_head_after`; the summary commit is separate.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Reject duplicate serial fields even when the first value is empty**
- **Found during:** Task 2
- **Issue:** An empty first serial field could otherwise make a later serial look like the only field.
- **Fix:** Track whether the serial field appeared independently of whether it has a value.
- **Files modified:** `host/dat_inventory.c`
- **Verification:** `compat.inventory.controls` passed.
- **Committed in:** `96b5771`.

**2. [Rule 1 - Bug] Report a source pin only for the matching DAT digest**
- **Found during:** Task 2
- **Issue:** Synthetic DAT input could be labeled with the pinned source commit without matching its digest.
- **Fix:** Compare the input SHA-256 with the D-06 digest before printing the pin.
- **Files modified:** `runner/main.c`
- **Verification:** Synthetic accounting reports `source_pin=unverified`; the pinned source reports the D-06 commit.
- **Committed in:** `548a08a`.

**3. [Rule 1 - Bug] Count non-ASCII serial values in the pinned DAT**
- **Found during:** Pinned-source acceptance check
- **Issue:** A valid UTF-8 serial value caused the parser to reject the full source.
- **Fix:** Hash decoded serial bytes for duplicate grouping, without storing or printing the identifier.
- **Files modified:** `host/dat_inventory.c`, `tests/compat/tests.cmake`.
- **Verification:** New regression and all four exact acceptance selectors passed; pinned DAT inventory returned the complete 13,407-entry count.
- **Committed in:** `7d8d5e5`, `595ae89`.

**Total deviations:** 3 auto-fixed Rule 1 correctness issues. All fixes remained in the planned files.

## Known Stubs

None.

## Next Phase Readiness

The pinned source occurrence accounting path is ready for the orchestrator's code review and reviewed-head phase verification. The real source correctly remains entirely unknown without matching per-occurrence evidence.

## Self-Check: PASSED

- All seven plan files and this summary exist at the required paths.
- All nine task commits listed in the measured plan range are ancestors of the current plan head.
- The worktree had no untracked files other than this summary before its metadata commit; task DAT temp paths were absent.

---
*Phase: 16-patches-cheats-and-compatibility-evidence*
*Completed: 2026-11-26*
