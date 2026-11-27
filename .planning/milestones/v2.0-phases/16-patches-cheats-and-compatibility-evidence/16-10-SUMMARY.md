---
phase: 16-patches-cheats-and-compatibility-evidence
plan: "10"
subsystem: runner
tags: [C, CMake, disc-patching, PPF, BPS]
requires:
  - phase: "16-06"
    provides: bounded runner option validation and output-refusal patterns
  - phase: "16-09"
    provides: complete BPS candidate transformation and raw CUE patch source
provides:
  - One transactional patch input with PPF source SHA policy and BPS CRC reporting
  - Generated runner CLI coverage for supported formats and unsupported media layouts
affects: [runner, disc-patching, phase-16-verification]
actuals:
  tokens: 8244
  tasks: 2
  commits: 3
commits: 3
plan_head_before: 3d19212fec1794284941793b9102a8b6c3db6c6e
plan_head_after: 62c17e711391f1b8a46359871d2cd48ad361edf0
tech-stack:
  added: []
  patterns:
    - Validate and publish patch candidates in the host before creating the emulator instance
    - Generate bounded media under temporary build output for real runner CLI checks
key-files:
  created: []
  modified:
    - runner/main.c
    - tests/runner/tests.cmake
    - docs/runner.md
    - docs/patches.md
    - docs/discs.md
key-decisions:
  - One patch is accepted per invocation; PPF requires whole-source SHA-256 and BPS always checks its three CRCs.
  - Patch source mapping is limited to one raw MODE1/2352 or MODE2/2352 CUE track beginning at file offset zero.
  - Validation reports list format and successful checks without revealing input paths.
patterns-established:
  - Successful CLI patch reads are observed through the existing production sector callback.
  - Rejected CLI patches must leave the source digest unchanged and produce neither a frame hash nor a PNG.
requirements-completed:
  - PATCH-01
coverage:
  - id: D1
    description: The runner validates and applies one supported PPF or BPS patch before emulation and reports the checks performed.
    requirement: PATCH-01
    verification:
      - kind: integration
        ref: runner.patch.cli
        status: pass
    human_judgment: false
  - id: D2
    description: Patch candidate mapping, format validation, bounds and atomic rejection remain covered by generated provider tests.
    requirement: PATCH-01
    verification:
      - kind: unit
        ref: disc.patch.mapping, disc.patch.ppf1, disc.patch.ppf2, disc.patch.ppf3, disc.patch.bps, disc.patch.atomic, disc.patch.bounds
        status: pass
    human_judgment: false
  - id: D3
    description: CLI usage, source identity policy, disc mapping limits and refusal statuses match the runner implementation.
    requirement: PATCH-01
    verification:
      - kind: other
        ref: Reviewed docs/runner.md, docs/patches.md and docs/discs.md against runner/main.c and passing CLI coverage.
        status: pass
    human_judgment: false
duration: 44m
completed: 2026-11-26
status: complete
---

# Phase 16 Plan 16-10: Expose the Transactional Patch Path in the Runner Summary

**The runner now applies one validated PPF or BPS patch before emulation and reports its source checks without exposing input paths.**

## Performance

- **Duration:** 44 minutes
- **Started:** 2026-11-26T03:28:36Z
- **Completed:** 2026-11-26T04:12:57Z
- **Runtime/model:** Codex GSD executor, gpt-6-luna
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments

- Added --patch FILE and optional --patch-source-sha256 HEX. PPF requires the complete raw source SHA-256; BPS always validates its source, target and patch CRCs and optionally binds the full source SHA as well.
- Validated and installed the candidate after opening the disc and before creating an emulator instance or running frames. Errors use typed patch status and the runner's existing usage, failure and unsupported exit codes.
- Added generated-media CLI coverage for PPF1, PPF2, PPF3 and BPS through the real runner. A test-only observer reads patched bytes through the same production sector callback. Refusal checks cover wrong and missing identity, malformed late PPF records, bad BPS CRC, unsupported ISO/ECM/playlist/multitrack layouts, duplicate and incompatible options, PNG/frame suppression, and unchanged source bytes.
- Documented CLI syntax, source checks, exit behavior and the one-track raw CUE mapping. The existing --disc --hash-state rejection remains covered.

## Execution Sequence

1. Confirmed the clean assigned worktree at base 3d19212fec1794284941793b9102a8b6c3db6c6e, loaded project and Phase 16 context, and captured the plan commit ledger.
2. Added the first runner.patch.cli test before the runner change. It exited 8 because the runner rejected --patch as an unknown option; the TDD evidence checker returned RED_EVIDENCE_OK. Committed the RED test as b14fbb6.
3. Added option parsing, bounded patch-file reads, candidate validation and replacement, validation reporting, and provider-read observation. A strict build caught an overlength synopsis string; splitting it into two printed pieces fixed the compiler error. The focused selector passed. Committed the tracer slice as 471d9f9 and reran its feedback gate successfully on that commit.
4. Expanded the generated CLI test and documentation. Early attempts exposed harness setup mistakes: a hook-only argument reached the production runner, CUE newlines were escaped as literal backslash-n, a Python helper was missing, and the no-frame detector matched --frames in usage text. These were corrected. The expanded behavior then passed against the existing implementation, so no redundant production edit or manufactured RED was added. Committed tests and docs as 62c17e7.
5. Re-ran the eight plan selectors plus the existing state-hash rejection, repository gate-budget check and documentation budget check. All passed. Temporary generated media and the inert BIOS were removed by Python temporary-directory cleanup.

## Checks

- Task 1 RED: ctest --preset ci --no-tests=error -R '^(runner[.]patch[.]cli)$' --output-on-failure --output-junit .gsd/tdd-evidence/16-10/runner-patch-red.xml — exit 8; the target assertion ran and failed because --patch was unknown before implementation. The JUnit report was written under the worktree's ignored evidence directory. gsd_run check tdd-red-evidence .gsd/tdd-evidence/16-10/runner-patch-red.json --raw — exit 0, RED_EVIDENCE_OK.
- Task 1 tracer: set -o pipefail && git diff --check HEAD && /usr/bin/time -p cmake --preset ci && ctest --preset ci --show-only=json-v1 -R '^(runner[.]patch[.]cli)$' | python3 -c 'import json,sys; names=[t["name"] for t in json.load(sys.stdin)["tests"]]; required={"runner.patch.cli"}; missing=required-set(names); print("missing selectors:",sorted(missing)); sys.exit(bool(missing) or len(names)!=len(set(names)))' && /usr/bin/time -p cmake --build --preset ci --target schwaystation-run schwaystation-run-test && /usr/bin/time -p ctest --preset ci --no-tests=error -R '^(runner[.]patch[.]cli)$' --output-on-failure — exit 0 on the committed tracer head; the selector was present, both targets built, and runner.patch.cli passed 1/1. Command duration: 1.13 seconds.
- Task 2 focused verification: set -o pipefail && git diff --check HEAD && /usr/bin/time -p cmake --preset ci && ctest --preset ci --show-only=json-v1 -R '^(runner[.]patch[.]cli|disc[.]patch[.]mapping|disc[.]patch[.]ppf1|disc[.]patch[.]ppf2|disc[.]patch[.]ppf3|disc[.]patch[.]bps|disc[.]patch[.]atomic|disc[.]patch[.]bounds)$' | python3 -c 'import json,sys; names=[t["name"] for t in json.load(sys.stdin)["tests"]]; required=set("runner.patch.cli disc.patch.mapping disc.patch.ppf1 disc.patch.ppf2 disc.patch.ppf3 disc.patch.bps disc.patch.atomic disc.patch.bounds".split()); missing=required-set(names); print("missing selectors:",sorted(missing)); sys.exit(bool(missing) or len(names)!=len(set(names)))' && /usr/bin/time -p cmake --build --preset ci --target schwaystation-run schwaystation-run-test schwaystation-test-disc-patch && /usr/bin/time -p ctest --preset ci --no-tests=error -R '^(runner[.]patch[.]cli|disc[.]patch[.]mapping|disc[.]patch[.]ppf1|disc[.]patch[.]ppf2|disc[.]patch[.]ppf3|disc[.]patch[.]bps|disc[.]patch[.]atomic|disc[.]patch[.]bounds)$' --output-on-failure — exit 0; all eight selectors were present and passed, CTest total 0.32 seconds, full command 1.78 seconds.
- Existing hash-state policy: ctest --preset ci --no-tests=error -R '^runner[.]hash_state$' --output-on-failure — exit 0, 1/1 passed, including --disc --hash-state refusal.
- Repository gate budget: set -o pipefail && /usr/bin/time -p cmake --preset hygiene && ctest --preset hygiene --show-only=json-v1 -R '^(policy[.]gate_budget)$' | python3 -c 'import json,sys; names=[t["name"] for t in json.load(sys.stdin)["tests"]]; missing={"policy.gate_budget"}-set(names); print("missing selectors:",sorted(missing)); sys.exit(bool(missing) or len(names)!=len(set(names)))' && /usr/bin/time -p ctest --preset hygiene --no-tests=error -R '^(policy[.]gate_budget)$' --output-on-failure — exit 0; budget stayed within the declared 3,901-line ceiling, tool wall time 1.44 seconds.
- Documentation budget: ctest --preset ci --no-tests=error -R '^docs[.]budgets$' --output-on-failure — exit 0, 1/1 passed in 0.03 seconds.
- git diff --check HEAD passed before each implementation commit and during final focused verification. Generated fixtures were confined to build output and absent after the CLI test. No hosted run or URL applies to this plan.

## Task Commits

1. Task 1 RED: b14fbb6 (test(16-10): add failing patch CLI test)
2. Task 1 GREEN: 471d9f9 (feat(16-10): expose runner patch loading)
3. Task 2 tests and documentation: 62c17e7 (test(16-10): cover runner patch formats and refusals)

The plan ledger measured three task commits from 3d19212fec1794284941793b9102a8b6c3db6c6e through 62c17e711391f1b8a46359871d2cd48ad361edf0. The summary metadata is committed separately.

## Files Modified

- runner/main.c — parses one patch input, loads and validates it before instance creation, reports checks, and preserves generic path-free errors.
- tests/runner/tests.cmake — generates raw CUE, PPF/BPS, ECM, ISO, playlist, multitrack and inert BIOS data in a cleaned temporary build directory, then exercises CLI success and refusal paths.
- docs/runner.md — documents patch options, ordering, reports and examples.
- docs/patches.md — documents runner usage, SHA/CRC policy, exit statuses and patch limits.
- docs/discs.md — documents the patchable raw track mapping and unsupported source layouts.

## Decisions Made

- Followed D-01/D-02: expose one patch overlay per run and keep parsing and patching in the host provider.
- Required a complete source SHA-256 for PPF. Kept it optional for BPS while requiring BPS source, target and patch CRC checks.
- Kept accepted patch sources to one raw data track with INDEX 01 at the start of its CUE file; other provider formats remain unpatchable.

## TDD Gate Compliance

- Task 1 had a valid RED on runner.patch.cli before implementation, followed by the passing GREEN and post-commit tracer feedback run.
- Task 2's expanded tests were added after Task 1's runner behavior was already implemented. Initial failures were harness setup errors; after fixing them, the new format and refusal cases passed against existing provider behavior. No false RED or redundant production change was introduced.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Error classification] Reported an unseekable patch file as I/O failure**
- Found during: Task 1 review
- Issue: A failed ftell call was initially grouped with an empty patch as malformed data.
- Fix: Return the typed patch I/O status for file-size query failure and keep only a zero-length file classified as empty.
- Files modified: runner/main.c
- Verification: The strict runner build and runner.patch.cli passed.
- Committed in: 471d9f9

**Total deviations:** 1 auto-fixed error classification. **Impact:** No scope change; the runner preserves the distinction between malformed input and file I/O failure.

## Issues Encountered

- The first expanded CLI run was not behavioral RED evidence: it passed the hook-only option to the production runner and used literal backslash-n bytes in the generated CUE. Later setup runs also caught a missing Python helper and an output detector that matched --frames in the usage text. All were fixed before the passing focused run.
- The first summary-only commit was rejected by the hygiene hook because the recorded report path had an absolute user-directory prefix. Replaced it with the worktree-relative ignored evidence path; the normal GSD summary commit then passed.
- STATE.md and ROADMAP.md were not changed in the isolated plan worktree, as instructed by the orchestrator.

## Known Stubs

None.

## User Setup Required

None.

## Next Phase Readiness

PATCH-01 runner integration and focused local checks are complete. The parent orchestrator owns phase-level review and verification on the reviewed source. No blocker remains.

---
Phase: 16-patches-cheats-and-compatibility-evidence
Completed: 2026-11-26

## Self-Check: PASSED

- Summary file exists at the expected phase path.
- Task commits b14fbb6, 471d9f9 and 62c17e7 are ancestors of the worktree HEAD.
- Worktree root and branch match the assigned plan worktree.
