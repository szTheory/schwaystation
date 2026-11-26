---
phase: 16-patches-cheats-and-compatibility-evidence
plan: "08"
subsystem: disc-patching
tags: [ppf2, ppf3, raw-cue, sha256, sparse-overlay]

requires:
  - phase: 16-07
    provides: Identity-bound PPF1 candidates and exact raw CUE track mapping
provides:
  - PPF2 source-size and validation-block checks with 32-bit records
  - PPF3 BIN parsing with optional block checks, 64-bit records, checked undo data, and bounded FileID trailers
  - Atomic malformed-input regression coverage for PPF2 and PPF3
affects: [16-09, 16-10]

actuals:
  tokens: 12226
  tasks: 2
  commits: 3
  plan_head_before: 542580e5ca0770820647aab45b75722b484713f0
  plan_head_after: 8a9e834d632bbf9a5873bbb22521bdee65eb93ed
commits: 3
plan_head_before: 542580e5ca0770820647aab45b75722b484713f0
plan_head_after: 8a9e834d632bbf9a5873bbb22521bdee65eb93ed

tech-stack:
  added: []
  patterns:
    - Validate the complete native PPF header and record stream before allocating a published candidate
    - Report only the whole-source SHA-256 and native checks that actually passed

key-files:
  created: []
  modified:
    - host/disc_patch.c
    - tests/disc/patch.c
    - docs/patches.md

key-decisions:
  - PPF2 requires an exact 32-bit source-size match and its 1,024-byte block at source offset 0x9320.
  - PPF3 supports BIN image type only; its 1,024-byte block is optional and it has no source-size field.
  - PPF3 undo bytes are length-checked but only forward patch bytes enter the overlay; FileID trailers require exact markers and lengths and cap descriptions at 3,072 bytes.
  - PPF2 FileID trailers remain outside this implemented subset.

requirements-completed:
  - PATCH-01
coverage:
  - id: D1
    description: PPF2 patches validate source identity, exact source size, native block bytes, and 32-bit record bounds before overlay creation.
    requirement: PATCH-01
    verification:
      - kind: unit
        ref: "ctest --preset ci --no-tests=error -R '^(disc[.]patch[.]ppf2)$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: PPF3 patches validate BIN headers, optional native block bytes, 64-bit record bounds, undo payloads, and bounded FileID trailers without partial application.
    requirement: PATCH-01
    verification:
      - kind: unit
        ref: "ctest --preset ci --no-tests=error -R '^(disc[.]patch[.]ppf1|disc[.]patch[.]ppf2|disc[.]patch[.]ppf3|disc[.]patch[.]atomic|disc[.]patch[.]bounds)$' --output-on-failure"
        status: pass
    human_judgment: false

duration: 20m 23s
completed: 2026-11-26
status: complete
---

# Phase 16 Plan 08: Complete PPF2 and PPF3 native checks and records Summary

**PPF2 and PPF3 now validate their version-specific native headers and record layouts before the shared raw-track overlay becomes visible.**

## Performance

- **Duration:** 20m 23s
- **Started:** 2026-11-26T01:36:34Z
- **Completed:** 2026-11-26T01:56:57Z
- **Tasks:** 2
- **Files modified:** 3
- **Runtime/model:** Codex/OpenGSD, `gpt-6-luna`

## Accomplishments

- Added PPF2's exact image-size and 1,024-byte source-block checks, plus 32-bit little-endian record offsets.
- Added PPF3 BIN headers, optional 1,024-byte block checks, 64-bit little-endian record offsets, undo-payload bounds, and exact FileID trailer validation with a 3,072-byte description cap.
- Expanded generated-media tests to cover native-check mismatches, optional checks, overflow and out-of-source offsets, truncated undo and trailers, invalid flags and image types, zero-length and over-limit records, late-record rejection, source immutability, and preservation of the prior overlay.
- Documented which checks are cryptographic identity evidence and which are native format checks; the full-source SHA-256 remains required for every PPF version.

## Execution Sequence

1. Read phase context, plan, preparation index and patch-format sources; confirmed a clean worktree at `542580e` on `worktree-agent-p16-08-20261126`.
2. Added PPF2/PPF3 acceptance tests first. The targeted CTest run exited 8 because both valid-format assertions reached the existing unsupported-format response; `gsd_run check tdd-red-evidence` returned `RED_EVIDENCE_OK` for each selector.
3. Implemented the version-specific parser and documentation. The Task 1 tracer verification passed its selector preflight, patch-target build and both PPF2/PPF3 tests before proceeding.
4. Added malformed-input and atomicity regressions. The Task 2 TDD run passed before any Task 2 production change; inspection confirmed Task 1 already rejects each case. The classifier reported `INVALID_RED (unexpected_green)`, so no redundant parser edit was made.
5. Re-ran the final five-selector preflight, focused build and CTest set; all passed. The tree was clean after the three task commits.

## Checks

- Task 1 RED: `ctest --preset ci --no-tests=error --output-on-failure --output-junit "$(git rev-parse --show-toplevel)/.gsd/tdd-evidence/16-08/task1-red.xml" -R '^(disc[.]patch[.]ppf2|disc[.]patch[.]ppf3)$'` — exit 8 as intended; CTest-reported time 0.02s. Both target assertions failed at the valid PPF candidate checks, and both records classified as `RED_EVIDENCE_OK`.
- Task 1 tracer verify: `set -o pipefail && git diff --check HEAD && cmake --preset ci && ctest --preset ci --show-only=json-v1 -R '^(disc[.]patch[.]ppf2|disc[.]patch[.]ppf3)$' | python3 -c 'import json,sys; names=[t["name"] for t in json.load(sys.stdin)["tests"]]; required=set("disc.patch.ppf2 disc.patch.ppf3".split()); missing=required-set(names); print("missing selectors:",sorted(missing)); sys.exit(bool(missing) or len(names)!=len(set(names)))' && cmake --build --preset ci --target schwaystation-test-disc-patch && ctest --preset ci --no-tests=error -R '^(disc[.]patch[.]ppf2|disc[.]patch[.]ppf3)$' --output-on-failure` — exit 0; both tests passed, CTest-reported time 0.01s.
- Final Task 2 verify: `set -o pipefail && git diff --check HEAD && LC_ALL=C sort -cu tests/disc/inventory.txt && cmake --preset ci && ctest --preset ci --show-only=json-v1 -R '^(disc[.]patch[.]ppf1|disc[.]patch[.]ppf2|disc[.]patch[.]ppf3|disc[.]patch[.]atomic|disc[.]patch[.]bounds)$' | python3 -c 'import json,sys; names=[t["name"] for t in json.load(sys.stdin)["tests"]]; required=set("disc.patch.ppf1 disc.patch.ppf2 disc.patch.ppf3 disc.patch.atomic disc.patch.bounds".split()); missing=required-set(names); print("missing selectors:",sorted(missing)); sys.exit(bool(missing) or len(names)!=len(set(names)))' && cmake --build --preset ci --target schwaystation-test-disc-patch && ctest --preset ci --no-tests=error -R '^(disc[.]patch[.]ppf1|disc[.]patch[.]ppf2|disc[.]patch[.]ppf3|disc[.]patch[.]atomic|disc[.]patch[.]bounds)$' --output-on-failure` — exit 0; inventory and unique-selector checks passed, all five tests passed, CTest-reported time 0.02s.
- Task 2 TDD evidence: the same five selectors with `--output-junit` exited 0 and passed 5/5 in 0.02s. The classifier returned `INVALID_RED (unexpected_green)` for `disc.patch.bounds`; this correctly blocked a redundant GREEN implementation step.
- `git diff --check HEAD` and `LC_ALL=C sort -cu tests/disc/inventory.txt` — exit 0. The worktree was clean before writing this summary.
- No hosted CI or run URL applies to these local checks; no child process remained running.

## Task Commits

1. **Task 1 RED: PPF2/PPF3 native layout tests** — `cce1cf2` (`test`)
2. **Task 1 GREEN: PPF2/PPF3 parser and documentation** — `f0671e5` (`feat`)
3. **Task 2: malformed PPF rejection and atomicity tests** — `8a9e834` (`test`)

## Files Created/Modified

- `host/disc_patch.c` — PPF2/PPF3 native validation, version-width record parsing, undo consumption and FileID validation before candidate publication.
- `tests/disc/patch.c` — self-authored PPF2/PPF3 layouts, native-check cases and malformed-input atomicity coverage.
- `docs/patches.md` — supported PPF subset, identity checks, validation boundaries and limitations.

## TDD Gate Compliance

- Task 1 had valid RED evidence for both PPF2 and PPF3, followed by the passing tracer verification and implementation commit.
- Task 2's malformed-input tests passed before any Task 2 production change because Task 1's parser already validated these paths. The machine classifier marked the evidence `unexpected_green`; the tests were kept and committed without adding redundant production code.

## Decisions Made

- Reject PPF3 GI and unknown image types; accept BIN only. Require exact block-check and undo flag values and a zero reserved byte.
- Validate PPF3 FileID start/end markers and trailing length against the exact remaining bytes. The selected subset does not accept PPF2 FileID trailers.
- Keep PPF block and size checks separate from the required full-source SHA-256 identity check.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Test bug] Corrected the successful-overlay replacement expectation**
- **Found during:** Task 1 GREEN verification.
- **Issue:** The second PPF3 fixture expected bytes from the first active overlay to survive a successful replacement.
- **Fix:** Reset the expected image to the original source before applying the replacement candidate's final-byte record.
- **Files modified:** `tests/disc/patch.c`
- **Verification:** The Task 1 PPF2/PPF3 selector set passed.
- **Committed in:** `f0671e5`.

**2. Task 2 was already satisfied by the Task 1 parser**
- **Found during:** Task 2 TDD RED run.
- **Issue:** All requested malformed PPF2/PPF3 cases passed before Task 2 production edits.
- **Resolution:** Confirmed the parser's rejection and atomicity paths by source inspection, retained the regression tests, and made no redundant implementation change.
- **Files modified:** `tests/disc/patch.c`
- **Verification:** All five patch selectors passed; the evidence classifier reported `unexpected_green`.
- **Committed in:** `8a9e834`.

## Issues Encountered

- The first Task 1 CTest report path was relative to `build/ci`; the result was rerun with the required absolute in-worktree JUnit path, then both RED records validated. No task behavior or source change was needed for this evidence-path correction.

## User Setup Required

None.

## Next Phase Readiness

Plan 16-09 can add BPS parsing to the shared patch candidate path. Plan 16-10 can exercise the resulting PPF/BPS behavior through the runner CLI. `.planning/STATE.md` and `.planning/ROADMAP.md` were left unchanged in this plan worktree as requested.

---
*Phase: 16-patches-cheats-and-compatibility-evidence*
*Completed: 2026-11-26*

## Self-Check: PASSED

- The summary file exists at the required phase path.
- Task commits `cce1cf2`, `f0671e5`, and `8a9e834` are ancestors of the worktree HEAD.
