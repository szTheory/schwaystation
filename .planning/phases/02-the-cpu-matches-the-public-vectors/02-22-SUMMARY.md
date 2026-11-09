---
phase: 02-the-cpu-matches-the-public-vectors
plan: 22
subsystem: testing
tags: [fuzz, libfuzzer, ps-exe, loader, asan, ubsan, nightly, replay]

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: the PS-EXE validator, probe and loader (plan 02-08), the CPU run loop with stop reasons and STOP_ON_FAULT (02-17), the stub kernel and the exit port (02-15, 02-16)
provides:
  - tests/fuzz/exe.c, the loader fuzz target and its invariants, and tests/fuzz/seeds.c, 32 in-memory seeds
  - schwaystation-fuzz-exe-replay, a dependency-free replay main (seeds, then .hex crashers; --write DIR writes the seeds as files) and the gating test fuzz.regress.exe on every leg
  - the fuzz preset (clang-21, fuzzer-no-link, ASan, UBSan, SCHWAYSTATION_FUZZ) and schwaystation-fuzz-exe
  - the fuzz job in nightly.yml, joined to the rolling nightly issue (written, not yet dispatched)
affects: [02-23, 02-24, 02-25, 02-26, 02-27]

actuals:
  tokens: 8179
  tasks: 2
  commits: 4

plan_head_before: 23098b22cbec82e4b74b8e463587f1a8f6b89ca3
plan_head_after: af8be7919954660305cc38b218ac0a021239f43f
commits: 4

tech-stack:
  added: []
  patterns:
    - "a fuzz target is one file with LLVMFuzzerTestOneInput, built twice: with a replay main (no engine, every leg, gating) and with libFuzzer's (fuzz preset, nightly, report-only)"
    - "seeds are table rows built in memory and a minimised crasher is committed as hex text, so no seed byte is committed"
    - "an accepted load is checked by patching the text segment and launch block of the pre-load RAM copy from the instance and requiring the rest equal"

key-files:
  created:
    - tests/fuzz/exe.c
    - tests/fuzz/seeds.c
    - tests/fuzz/seeds.h
    - tests/fuzz/replay_main.h
  modified:
    - tests/fuzz/tests.cmake
    - tests/fuzz/inventory.txt
    - CMakePresets.json
    - .github/workflows/nightly.yml
    - docs/testing.md
    - src/cpu.h

key-decisions:
  - "the replay always runs the in-memory seeds before any file argument, so a replay can never pass having run nothing and needs no no-input self test"
  - "an empty input returns at once: the public entry points refuse an empty file as an argument error, which is not a loader invariant"
  - "the seeds' hammer code is generated from a table of KSEG1 windows by a six-function encoder, devices first (expansion regions last, since the first read there faults) and gaps in their own seed, because STOP_ON_FAULT ends the run at the first fault"
  - "libFuzzer runs with -timeout=20 and keeps its crash files as an artifact, so a hang is a crasher and a crash can be minimised"

patterns-established:
  - "tests/fuzz/regress/<name>.hex plus a manifest row is the way a crasher is kept; docs/testing.md says so"

requirements-completed: []

coverage:
  - id: D1
    description: "LLVMFuzzerTestOneInput probes and loads each input into a power-reset instance and requires probe and load to agree, the status to be OK, ERR_EXE_INVALID or ERR_EXE_RANGE, a refused load to change no RAM byte, an accepted one only the text segment and the launch block, and two frames of the loaded program under STOP_ON_FAULT to return OK with a stop reason"
    requirement: "CPU-05"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^fuzz[.]regress[.]exe$' --no-tests=error (exit 0); a mutant of src/exe.c that flipped a RAM byte outside the text segment aborted the replay with the invariant named (exit 134)"
        status: pass
    human_judgment: false
  - id: D2
    description: "tests/fuzz/seeds.c builds 32 seeds in memory: a valid EXE, text at the RAM end, wrapping and KSEG2 spans, pc0 zero, text size zero, a size one below and one above each bound, and EXEs whose code hammers device windows and unmapped gaps; no seed or crasher byte is a tracked binary file"
    requirement: "CPU-05"
    verification:
      - kind: unit
        ref: "build/ci/tests/schwaystation-fuzz-exe-replay (exit 0, 32 inputs) and t=$(git ls-files -- 'tests/fuzz/*.bin' 'tests/fuzz/*.exe') && test -z $t (exit 0)"
        status: pass
    human_judgment: false
  - id: D3
    description: "fuzz.regress.exe replays every seed and every tests/fuzz/regress/*.hex through a dependency-free replay main on every leg and is an ordinary gating CTest labelled fuzz and portable"
    requirement: "CPU-05"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0, 208 of 208 tests, fuzz.regress.exe 0.16 s)"
        status: pass
      - kind: integration
        ref: "the be32 and wasm32 legs of the CI run on the pushed head (not yet run: the push was refused)"
        status: unknown
    human_judgment: false
  - id: D4
    description: "The fuzz preset builds schwaystation-fuzz-exe and schwaystation-fuzz-exe-replay with clang-21 and fuzzer-no-link, address and undefined, the engine linked only into the fuzz target, under the asan inventory without fixtures, registering no extra test"
    requirement: "CPU-05"
    verification:
      - kind: integration
        ref: "PATH with Homebrew llvm@21 as clang-21 first: cmake --workflow --preset fuzz (exit 0, both targets linked); schwaystation-fuzz-exe on the seeds for 10 s in a copy of the build without AddressSanitizer: 1135 runs, 103 exec/s, no crash (exit 0)"
        status: pass
      - kind: integration
        ref: "the same binaries run with AddressSanitizer: the ASan runtime hung in dyld initialisers on this host, so no ASan run exists; the nightly dispatch is the first"
        status: unknown
    human_judgment: false
  - id: D5
    description: "nightly.yml has a fuzz job on ubuntu-26.04 that builds the fuzz preset, writes the seeds with the replay's --write mode and runs the target for 60 s with -max_len=2099200, joined to the report job's needs; dispatched once on the branch"
    requirement: "CPU-05"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset hygiene (exit 0, 21 of 21: timeout_comments and action_pins cover the new job); grep -qE '^  fuzz:' .github/workflows/nightly.yml (exit 0)"
        status: pass
      - kind: e2e
        ref: "gh workflow run nightly.yml --ref phase/02-the-cpu-matches-the-public-vectors, its conclusion and executions per second (not run: git push was refused by the permission system, so the branch on origin lacks the job)"
        status: unknown
    human_judgment: false
  - id: D6
    description: "hygiene.no_float passes again: a comment in src/cpu.h that said double fault made the text scan fail, which blocked this plan's hygiene verification"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset hygiene (exit 0, 21 of 21)"
        status: pass
    human_judgment: false

duration: 55min
completed: 2026-11-09
status: halted
---

# Phase 2 Plan 22: The loader fuzz target Summary

**A libFuzzer target for the PS-EXE loader that also runs the loaded program for two frames, 32 seeds built in memory, a gating replay on every leg and a nightly 60-second job, with the push and the one nightly dispatch still to do**

Status is `halted`, not `complete`: Task 3's push, dispatch and green run could not be made. `git push` was refused by
the permission system, so the branch on origin does not have the `fuzz` job and the nightly run the plan asks for
does not exist. Everything that does not need the remote is done and verified.

## Performance

- **Duration:** about 55 min, including a hung ASan run left to time out for two minutes
- **Started:** 2026-11-09T22:30:00Z (approximate; the start time was not recorded)
- **Completed:** 2026-11-09
- **Tasks:** 2 complete, 1 partial
- **Files modified:** 10 (4 new)

## Accomplishments

- `tests/fuzz/exe.c` checks, per input: probe and load agree; the status is OK, `ERR_EXE_INVALID` or
  `ERR_EXE_RANGE`; a refused load changes no RAM byte and an accepted one only the text segment and the launch
  block; two frames of the loaded program run under `SCHWAY_RUN_STOP_ON_FAULT`, each `schway_run` returning OK with
  one of the four stop reasons. A broken invariant prints which and calls `abort()`.
- `tests/fuzz/seeds.c` builds 32 seeds in memory. Run through the loader, they cover every status the invariants
  accept and every stop the hammer and spin code can reach (a spin runs two whole frames to `FRAME`; the hammer seeds
  end at the first fault, at an expansion window read, a gap or a bad `pc0`).
- `schwaystation-fuzz-exe-replay` is a port of the sibling replay main with `--write DIR` added and `.hex` crashers
  decoded as text. It always runs the seeds first. `fuzz.regress.exe` is a gating CTest (`fuzz`, `portable`) in the
  `ci`, `asan`, `be32` and `wasm32` inventories; it takes 0.16 s natively.
- The `fuzz` configure, build and workflow presets, and the `fuzz` job in `nightly.yml`, which is part of the report
  job's `needs`.
- `docs/testing.md` names the lane and the target, says the replay gates and the campaign reports, and says how a
  crasher is kept.

## Task Commits

1. **Task 1: Tracer, the replay runs the in-memory seeds through the target on the ci workflow** - `088f296` (test)
2. **Task 2: The fuzz preset and the libFuzzer target** - `e2524fa` (test), with a blocking fix `c219811` (fix)
3. **Task 3: The nightly fuzz job, dispatched once on the branch** - `af8be79` (test): the job and the docs only; the push and the dispatch are not done

**Plan metadata:** the commit of this summary.

## Files Created/Modified

- `tests/fuzz/exe.c` - the target, its invariants and the replay entry
- `tests/fuzz/seeds.c`, `tests/fuzz/seeds.h` - the in-memory seeds and the small MIPS encoder behind the hammer code
- `tests/fuzz/replay_main.h` - the replay main, seeded from the sibling's, with `--write` and `.hex` input
- `tests/fuzz/tests.cmake`, `tests/fuzz/inventory.txt` - the replay target, `fuzz.regress.exe`, and the `SCHWAYSTATION_FUZZ` option that adds the libFuzzer target
- `CMakePresets.json` - the `fuzz` configure, build and workflow presets
- `.github/workflows/nightly.yml` - the `fuzz` job
- `docs/testing.md` - the lane row, the tool note and "The loader fuzz target"
- `src/cpu.h` - one comment reworded (see Deviations)

## Decisions Made

See `key-decisions`. The two worth knowing: the replay needs no "ran nothing" guard because the seeds always run, and
the hammer seeds are split into device windows and gaps so each reaches its own addresses before the first fault ends
the run.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] hygiene.no_float failed on a comment in src/cpu.h**
- **Found during:** Task 2 (`cmake --workflow --preset hygiene`)
- **Issue:** `src/cpu.h:231` said "a double fault"; the integer-only text scan rejects the word `double` in any
  comment. The failure predates this plan (the comment came with the run loop) and fails the hygiene verification of
  Tasks 2 and 3 and the `hygiene` job.
- **Fix:** reworded to "a fault in a vector"; no code changed.
- **Files modified:** `src/cpu.h`
- **Verification:** `cmake --workflow --preset hygiene` exit 0, 21 of 21
- **Committed in:** `c219811`

**2. [Rule 2 - Missing critical] A docs section and a crash artifact**
- **Found during:** Task 3
- **Issue:** AGENTS rule 5 asks for behaviour and its docs together, and a crash found by the nightly job would
  otherwise be lost with the runner.
- **Fix:** a "The loader fuzz target" section and a lane row in `docs/testing.md`; the job passes
  `-artifact_prefix` and uploads `build/fuzz/crash` on failure; `-timeout=20` makes a hang a crasher.
- **Files modified:** `docs/testing.md`, `.github/workflows/nightly.yml`
- **Committed in:** `af8be79`

**3. [Plan reading] The replay has no `--seeds` flag**
- The sibling's replay needed `--seeds` to run its seeds. Here every run starts with them, which is what the third
  truth asks ("replays every seed and every committed crasher"), and the `--write DIR` mode is the only flag.

---

**Total deviations:** 2 auto-fixed (1 blocking, 1 missing critical) and 1 plan reading
**Impact on plan:** none on scope; the fix is one comment.

## Issues Encountered

- **Push refused.** `git push origin phase/02-the-cpu-matches-the-public-vectors` was denied by the permission
  system, so Task 3's dispatch (`gh workflow run nightly.yml --ref phase/02-the-cpu-matches-the-public-vectors`), its
  watch, the recorded run URL, conclusion and executions per second, and the CI run for the push, did not happen. The
  branch is four commits ahead of origin (`088f296`, `c219811`, `e2524fa`, `af8be79`) with PR #7 untouched. Nothing
  was worked around.
- **ASan did not start locally.** The fuzz preset builds here with Homebrew llvm@21 standing in for clang-21, but
  every binary with AddressSanitizer hung in dyld's initialisers (0.7 CPU, no output, killed after 2.5 minutes).
  The same sources built without ASan (fuzzer-no-link plus UBSan) ran 1135 libFuzzer runs in 11 s with no crash, at
  about 103 exec/s, so a 60-second job runs about 6,000 inputs without ASan and fewer with it. The nightly dispatch
  is the first ASan run.
- No crasher was found, so `tests/fuzz/regress/` and the manifest row do not exist yet.

## User Setup Required

None - no external service configuration required. To finish Task 3, someone who may push needs to run, from the
repository root:

```
git push origin phase/02-the-cpu-matches-the-public-vectors
gh workflow run nightly.yml --ref phase/02-the-cpu-matches-the-public-vectors
```

then watch the run, and record its URL, conclusion and the executions per second from the job log. A crash is
minimised with `-minimize_crash=1` and kept as `tests/fuzz/regress/<name>.hex` with its fix and a manifest row.

## Next Phase Readiness

- `fuzz.regress.exe` is in the inventories and passes; later plans' fuzz targets can copy `replay_main.h` and the
  `tests/fuzz/tests.cmake` shape.
- Open: the push, the one dispatch and its result (D5), the be32 and wasm32 runs of the replay (D3) and the first ASan
  run (D4). CPU-05 stays unmarked until they are in.

## Self-Check: PASSED

- Created files exist: `tests/fuzz/exe.c`, `tests/fuzz/seeds.c`, `tests/fuzz/seeds.h`, `tests/fuzz/replay_main.h`.
- Commits `088f296`, `c219811`, `e2524fa` and `af8be79` are ancestors of HEAD.
- Re-run after the last commit: `cmake --workflow --preset ci` exit 0 (208 of 208) and `cmake --workflow --preset hygiene` exit 0 (21 of 21).

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09 (halted at the push)*
