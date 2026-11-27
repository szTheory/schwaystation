---
phase: 16-patches-cheats-and-compatibility-evidence
plan: "06"
subsystem: runner
tags: [C, CMake, GameShark, state-hash]
requires:
  - phase: "16-03"
    provides: shared bounded GameShark parser, ordered host cheat table, canonical encoding
provides:
  - bounded repeatable runner --cheat options applied at frame boundaries
  - configuration-aware runner state hashes when cheats are configured
affects: [runner, cheats, state-hashes, phase-16-verification]
actuals:
  tokens: 6830
  tasks: 2
  commits: 3
commits: 3
plan_head_before: 445d699d27b8ad8f176a977aaa82081ce2949db2
plan_head_after: 6ec6171043695407b0eb4c44112f9194bb59a6f7
tech-stack:
  added: []
  patterns:
    - use the shared parser and bounded canonical host-cheat table from the runner adapter
    - hash normalized cheat continuation only when at least one entry is configured
key-files:
  created: []
  modified:
    - runner/main.c
    - tests/runner/tests.cmake
    - docs/runner.md
    - docs/cheats.md
key-decisions:
  - "D-03: accept up to 32 ordered entries using the shared parser; reject invalid invocation before execution or output mutation."
  - "D-05: append a versioned, domain-separated canonical cheat table only for configured cheats; preserve the no-cheat hash input."
patterns-established:
  - "Runner option text is validated before instance execution; accepted writes apply once before each requested frame."
  - "The optional state-hash suffix uses the canonical encoded host table, never C structure bytes or original spelling."
requirements-completed:
  - CHEAT-01
coverage:
  - id: D1
    description: The runner accepts bounded repeated cheat codes, rejects invalid lists before side effects, and applies writes at frame start.
    requirement: CHEAT-01
    verification:
      - kind: integration
        ref: runner.cheats.cli
        status: pass
      - kind: unit
        ref: runner.cheats.parser and runner.cheats.order
        status: pass
    human_judgment: false
  - id: D2
    description: Configured normalized cheat continuation changes runner state hashes while no-cheat state pins remain unchanged.
    requirement: CHEAT-01
    verification:
      - kind: integration
        ref: runner.cheats.hash and runner.hashes_file
        status: pass
      - kind: integration
        ref: fixtures.gpu_motion.state.150 and fixtures.gpu_motion.state.180
        status: pass
    human_judgment: false
duration: 40min
completed: 2026-11-26
status: complete
---

# Phase 16 Plan 16-06: Expose Runner Cheats and Configuration-Aware State Hashes Summary

**The runner now applies bounded repeated GameShark codes at frame start and extends state hashes with normalized cheat configuration while preserving no-cheat hashes.**

## Performance

- **Duration:** 40 minutes (actual elapsed time; the focused local media prerequisite added work beyond the estimate)
- **Started:** 2026-11-26T02:21:35Z
- **Completed:** 2026-11-26T03:02:00Z
- **Model:** gpt-6-luna
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments

- Added repeatable `--cheat CODE` parsing backed by the shared 32-entry table. The runner rejects malformed, unsupported, and over-cap lists before executing a frame or writing requested output.
- Applied the enabled normalized table once before each requested frame, including each PS-EXE `schway_run` frame.
- Extended configured `--hash-state` input with the canonical 16,656-byte cheat table after the `schwaystation-run/cheat-state` domain and version byte. With no cheats, the legacy state-body hash remains unchanged.
- Documented the CLI behavior, canonical hash contribution, and separation from disc-patch identity.

## Task Commits

1. **Task 1 RED:** `c2b0e3b` (`test(16-06): cover runner cheat CLI frame writes`)
2. **Task 1 GREEN:** `add0252` (`feat(16-06): apply runner cheats at frame boundaries`)
3. **Task 2 tests and documentation:** `6ec6171` (`test(16-06): cover runner cheat state hashes`)

The plan ledger measured 3 commits from `445d699d27b8ad8f176a977aaa82081ce2949db2` through `6ec6171043695407b0eb4c44112f9194bb59a6f7`. The plan SUMMARY is committed separately after this file is written.

## Files Modified

- `runner/main.c` — parses bounded cheat options, applies the shared table at each frame boundary, and optionally adds normalized cheat state to the state hash.
- `tests/runner/tests.cmake` — exercises the real runner with a self-authored PS-EXE, invalid and over-cap inputs, hash normalization, entry order, duplicate entries, and rejected `--disc --hash-state`.
- `docs/runner.md` — documents the repeatable CLI option and frame-start behavior.
- `docs/cheats.md` — documents the runner hash suffix and no-cheat compatibility.

## Decisions Made

- Followed D-03: command-line entries are enabled, bounded, ordered, and parsed before execution. Overlapping writes keep last-write-wins behavior through the shared table.
- Followed D-05: hash the saved machine-state body as before, then append the domain, version byte, and canonical table only when a cheat entry is configured.

## Verification Evidence

The self-authored executable was first run against the unchanged runner and reported `tty N`; with ordered byte and halfword cheats it reported `tty Y`. Invalid code and a 33-entry list both returned usage status 2 without guest output or PNG creation. The first fixture draft had incorrect MIPS control flow and did not reach the intended observable; it was fixed before the RED run and was treated as fixture setup, not RED evidence.

Task 1's RED command was `ctest --preset ci --no-tests=error -R '^runner[.]cheats[.]cli$' --output-junit .gsd/tdd-evidence/16-06/task1-red.xml`; it exited 8 because the pre-feature runner rejected `--cheat` as an unknown option. The target assertion executed, and `gsd_run check tdd-red-evidence .gsd/tdd-evidence/16-06/task1-red.json --raw` returned `RED_EVIDENCE_OK`. The baseline and valid-code observations were checked before classifying that failure.

Task 1's successful plan gate:

```sh
set -o pipefail && git diff --check HEAD && cmake --preset ci && ctest --preset ci --show-only=json-v1 -R '^(runner[.]cheats[.]cli|runner[.]cheats[.]parser|runner[.]cheats[.]order)$' | python3 -c 'import json,sys; names=[t["name"] for t in json.load(sys.stdin)["tests"]]; required=set("runner.cheats.cli runner.cheats.parser runner.cheats.order".split()); missing=required-set(names); print("missing selectors:",sorted(missing)); sys.exit(bool(missing) or len(names)!=len(set(names)))' && cmake --build --preset ci --target schwaystation-run schwaystation-run-test schwaystation-test-cheats && ctest --preset ci --no-tests=error -R '^(runner[.]cheats[.]cli|runner[.]cheats[.]parser|runner[.]cheats[.]order)$' --output-on-failure
```

**Result:** exit 0; all 3 selected tests passed; CTest reported 0.07 seconds. An earlier invocation exited 8 because the no-execution assertion matched help text containing `frame N`; the assertion was narrowed to actual TTY output or numbered frame output before the passing run.

The `runner.cheats.hash` script was first exercised directly with `cmake -DRUN="$PWD/build/ci/schwaystation-run" -DCASE=cheats_hash -DOUT="$PWD/build/ci/tests/runner-cheats" -P tests/runner/tests.cmake`. The first invocation exited 1 because the disc-policy probe omitted `--bios`, so the parser rejected `--disc` earlier. The test was corrected to provide an unused BIOS path, allowing the intended `--hash-state`-with-disc rejection to be observed; the direct script then exited 0.

The successful Task 2 plan gate was:

```sh
set -o pipefail && git diff --check HEAD && cmake --preset ci && ctest --preset ci --show-only=json-v1 -R '^(runner[.]cheats[.]cli|runner[.]cheats[.]hash|runner[.]hashes_file)$' | python3 -c 'import json,sys; names=[t["name"] for t in json.load(sys.stdin)["tests"]]; required=set("runner.cheats.cli runner.cheats.hash runner.hashes_file".split()); missing=required-set(names); print("missing selectors:",sorted(missing)); sys.exit(bool(missing) or len(names)!=len(set(names)))' && cmake --build --preset ci --target schwaystation-run schwaystation-run-test schwaystation-test-disc-media schwaystation-test-disc-fixture && ctest --preset ci --no-tests=error -R '^(runner[.]cheats[.]cli|runner[.]cheats[.]hash|runner[.]hashes_file)$' --output-on-failure
```

**Result:** exit 0; CTest ran the three selected tests and their fixture setup, 6/6 passed in 36.08 seconds. The required local preflight was `cmake --build --preset ci --target schwaystation-test-disc-media schwaystation-test-disc-fixture` (exit 0). The first selector attempt exited 8 before the runner assertions because `schwaystation-test-disc-media` was not built. After building it, CTest reached `disc.fixture.fmv` but lacked the fast OpenBIOS input, so the pinned CI fixture producer was run locally.

The OpenBIOS prerequisite used pinned Nugget commit `9db6d88d23c4da7a39b1a4b9729f1e5fff80dec9` and the `gcc-mipsel-none-elf-linux` archive pinned in `pins.txt`. The archive fetch, clone/checkout, and a task-named `--rm` amd64 container build produced both normal and fast images; `cmake -DOPENBIOS_INPUT=build/ci/openbios/openbios.bin -DEXPECTED_FILE=tests/built/expected.txt -P cmake/OpenBIOS.cmake` and the corresponding `openbios-fast.bin` / `tests/built/fast-expected.txt` check exited 0. Both outputs were 524,288 bytes and matched their expected SHA-256 digests. The container exited and removed itself; the existing `gcc:13` image was not created or removed. After `runner.hashes_file` passed, the task-owned Nugget checkout, toolchain archive/extraction, generated BIOS files, and generated cheat-test executable were removed. Other local build outputs remain ignored under `build/ci/`.

The no-cheat frozen state pins were checked separately with `ctest --preset ci --no-tests=error -R '^(fixtures[.]gpu_motion[.]state[.]150|fixtures[.]gpu_motion[.]state[.]180)$' --output-on-failure`; exit 0, both passed, CTest total 1.41 seconds. The sorted runner inventory and the repository line-budget check also passed; the budget remained exactly 3,901 of 3,901 lines. `git diff --check HEAD` passed before each task commit.

## TDD Gate Compliance

- **Task 1:** valid RED (`c2b0e3b`) preceded the CLI GREEN (`add0252`). The generated EXE's intended baseline and valid-cheat output were verified; the recorded failure was specifically the unsupported `--cheat` option.
- **Task 2:** the hash implementation was included in the Task 1 implementation commit before its dedicated hash assertions were added. The parent explicitly directed preserving that verified commit rather than removing behavior to manufacture a RED. The Task 2 hash tests and docs were committed afterward in `6ec6171` and passed, but Task 2 did not have a valid pre-implementation RED commit. This sequencing deviation is reported rather than represented as TDD-compliant.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Test assertion] Avoided matching usage help as execution output.**
- **Found during:** Task 1 verification
- **Issue:** The over-cap test treated the synopsis text `frame N` as evidence that a frame ran.
- **Fix:** Match only actual `tty Y/N` or numbered frame output.
- **Files modified:** `tests/runner/tests.cmake`
- **Verification:** Task 1 focused gate passed, 3/3.
- **Committed in:** `add0252`

**2. [Rule 3 - Test prerequisite] Built the local producers required by the existing lane hash selector.**
- **Found during:** Task 2 verification
- **Issue:** `runner.hashes_file` requires the disc-media and disc-FMV setup executables and the CI-built fast OpenBIOS image, which were absent from the targeted runner build.
- **Fix:** Built the existing test targets and generated the pinned, digest-checked OpenBIOS inputs locally; removed the task-owned generated inputs after the selector passed.
- **Files modified:** no tracked source files; generated files stayed under ignored `build/ci/` paths and were removed.
- **Verification:** Task 2 focused gate passed, 6/6.
- **Committed in:** not committed; local verification prerequisite only.

**3. [TDD ordering] Added Task 2 assertions after hash behavior was already committed.**
- **Found during:** Task 2
- **Issue:** The hash hunk had been included in the Task 1 implementation commit, leaving no honest pre-implementation RED for Task 2.
- **Fix:** Following the parent's direction, preserved the verified commit and reported the ordering deviation; added and ran the hash tests without rewriting history.
- **Files modified:** `tests/runner/tests.cmake`, `docs/cheats.md`
- **Verification:** `runner.cheats.hash`, `runner.hashes_file`, and both frozen GPU state pins passed.
- **Committed in:** `6ec6171`

**Total deviations:** 3 (2 automated test/setup fixes, 1 TDD sequencing deviation)
**Impact on plan:** All requested behavior is implemented and tested. Task 2's RED/GREEN order is the only process gap; no implementation or verification blocker remains.

## Known Stubs

- `tests/runner/tests.cmake:194` — the existing `runner.patch.cli` script case remains an unimplemented placeholder. It is outside Plan 16-06 and is assigned to Plan 16-10; it does not block the cheat CLI or state-hash goals here.

## Issues Encountered

- The first OpenBIOS make attempt ran before the pinned archive had been extracted and exited 2. The archive was extracted, the pinned Linux toolchain was run inside an amd64 container, and both expected images then built and passed digest validation.
- `runner.hashes_file` needed a real fast OpenBIOS image in addition to runner targets. Reproducing the repository's pinned local CI producer supplied that fixture. No generated OpenBIOS or cheat-test executable remains; other generated CI fixtures remain ignored under `build/ci/`.

## User Setup Required

None. No user action is needed.

## Next Phase Readiness

Plan 16-06 is complete. Runner cheats and configuration-aware state hashes are ready for the remaining Phase 16 plans; Plan 16-10 owns the unrelated runner disc-patch CLI placeholder.

## Self-Check: PASSED

- SUMMARY exists at the required phase path.
- Commits `c2b0e3b`, `add0252`, and `6ec6171` are ancestors of this plan's current HEAD.
- The pre-summary plan ledger measures 3 commits from the recorded base.

---
*Phase: 16-patches-cheats-and-compatibility-evidence*
*Completed: 2026-11-26*
