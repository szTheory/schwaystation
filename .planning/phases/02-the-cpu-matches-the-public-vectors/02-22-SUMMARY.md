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
  - the fuzz job in nightly.yml, joined to the rolling nightly issue and dispatched once on the branch (run 42720996208, success)
affects: [02-23, 02-24, 02-25, 02-26, 02-27]

actuals:
  tokens: 8300
  tasks: 3
  commits: 32

plan_head_before: 23098b22cbec82e4b74b8e463587f1a8f6b89ca3
plan_head_after: 5c99bb4
commits: 32

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
    - tests/vectors/vectors.c
    - tests/runner/tests.cmake
    - tests/cmake/exit_table.cmake

key-decisions:
  - "the replay always runs the in-memory seeds before any file argument, so a replay can never pass having run nothing and needs no no-input self test"
  - "an empty input returns at once: the public entry points refuse an empty file as an argument error, which is not a loader invariant"
  - "the seeds' hammer code is generated from a table of KSEG1 windows by a six-function encoder, devices first (expansion regions last, since the first read there faults) and gaps in their own seed, because STOP_ON_FAULT ends the run at the first fault"
  - "libFuzzer runs with -timeout=20 and keeps its crash files as an artifact, so a hang is a crasher and a crash can be minimised"

patterns-established:
  - "tests/fuzz/regress/<name>.hex plus a manifest row is the way a crasher is kept; docs/testing.md says so"

requirements-completed: [CPU-05]

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
        ref: "cmake --workflow --preset ci (exit 0, fuzz.regress.exe 0.16 s)"
        status: pass
      - kind: integration
        ref: "hosted CI on head 5c99bb4, all jobs success including be32 (fuzz.regress.exe passed under qemu-ppc in 3.65 s) and wasm32: https://github.com/szTheory/schwaystation/actions/runs/42722584020"
        status: pass
    human_judgment: false
  - id: D4
    description: "The fuzz preset builds schwaystation-fuzz-exe and schwaystation-fuzz-exe-replay with clang-21 and fuzzer-no-link, address and undefined, the engine linked only into the fuzz target, under the asan inventory without fixtures, registering no extra test"
    requirement: "CPU-05"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset fuzz (exit 0 with Homebrew llvm@21 as clang-21, both targets linked); the same preset built and ran on ubuntu-26.04 in the nightly fuzz job, with AddressSanitizer: https://github.com/szTheory/schwaystation/actions/runs/42720996208"
        status: pass
    human_judgment: false
  - id: D5
    description: "nightly.yml has a fuzz job on ubuntu-26.04 that builds the fuzz preset, writes the seeds with the replay's --write mode and runs the target for 60 s with -max_len=2099200, joined to the report job's needs; dispatched once on the branch: the run concluded success, the fuzz job ran 3248 inputs in 61 s with AddressSanitizer and UBSan at 53 executions per second, 462 coverage points, no crash"
    requirement: "CPU-05"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset hygiene (exit 0, 21 of 21: timeout_comments and action_pins cover the new job); grep -qE '^  fuzz:' .github/workflows/nightly.yml (exit 0)"
        status: pass
      - kind: e2e
        ref: "gh workflow run nightly.yml --ref phase/02-the-cpu-matches-the-public-vectors (exit 0); gh run watch 42720996208 --exit-status (exit 0), conclusion success, fuzz job 129514627660: https://github.com/szTheory/schwaystation/actions/runs/42720996208"
        status: pass
    human_judgment: false
  - id: D6
    description: "hygiene.no_float passes again: a comment in src/cpu.h that said double fault made the text scan fail, which blocked this plan's hygiene verification"
    requirement: "CPU-05"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset hygiene (exit 0, 21 of 21)"
        status: pass
    human_judgment: false
  - id: D7
    description: "The be32 leg builds and passes in full: the vectors harness compiles where size_t is 32 bits, and the exit table self test runs the emulator with its library path"
    requirement: "CPU-05"
    verification:
      - kind: integration
        ref: "be32 job of CI run 42722584020 on head 5c99bb4 (success); the same job on the earlier head failed twice, first to build tests/vectors/vectors.c and then in runner.exit_table.self_test"
        status: pass
    human_judgment: false

duration: 55min plus hosted waits
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 22: The loader fuzz target Summary

**A libFuzzer target for the PS-EXE loader that also runs the loaded program for two frames, 32 seeds built in memory, a gating replay on every leg and a nightly 60-second job that has run once under ASan and UBSan with no crash**

## Performance

- **Duration:** about 55 min of authoring (the original session), plus the hosted waits and two be32 fixes in this one
- **Started:** 2026-11-09T22:30:00Z (approximate; the start time was not recorded)
- **Completed:** 2026-11-09
- **Tasks:** 3
- **Files modified:** 13 (4 new)

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
  `ci`, `asan`, `be32` and `wasm32` inventories; it takes 0.16 s natively and 3.65 s under qemu-ppc.
- The `fuzz` configure, build and workflow presets, and the `fuzz` job in `nightly.yml`, which is part of the report
  job's `needs`.
- The nightly dispatch ran green on the branch: run `42720996208`
  (https://github.com/szTheory/schwaystation/actions/runs/42720996208), conclusion `success`, every job green.
  The `fuzz` job (129514627660) ran libFuzzer with AddressSanitizer and UBSan for 60 s: 3248 runs in 61 s, 53
  executions per second at the end, `cov: 462 ft: 1020 corp: 127`, no crash, no hang. That is the first ASan run of the
  target; the local host could not start ASan binaries. So no crasher exists and `tests/fuzz/regress/` and its manifest
  row are not created.
- `docs/testing.md` names the lane and the target, says the replay gates and the campaign reports, and says how a
  crasher is kept.

## Task Commits

1. **Task 1: Tracer, the replay runs the in-memory seeds through the target on the ci workflow** - `088f296` (test)
2. **Task 2: The fuzz preset and the libFuzzer target** - `e2524fa` (test), with a blocking fix `c219811` (fix)
3. **Task 3: The nightly fuzz job, dispatched once on the branch** - `af8be79` (test), then the hosted be32 fixes `6ed9df1` and `5c99bb4` (fix)

**Plan metadata:** the commit of this summary.

`commits: 32` is the measured count of `plan_head_before..HEAD`; the range also holds plans 02-20, 02-21 and 02-23
to 02-25, which ran while this plan waited for its push. Six of the 32 are this plan's own.

## Files Created/Modified

- `tests/fuzz/exe.c`, `tests/fuzz/seeds.c`, `tests/fuzz/seeds.h` - the target, its invariants, the in-memory seeds and the small MIPS encoder behind the hammer code
- `tests/fuzz/replay_main.h` - the replay main, seeded from the sibling's, with `--write` and `.hex` input
- `tests/fuzz/tests.cmake`, `tests/fuzz/inventory.txt` - the replay target, `fuzz.regress.exe`, and the `SCHWAYSTATION_FUZZ` option that adds the libFuzzer target
- `CMakePresets.json` - the `fuzz` configure, build and workflow presets
- `.github/workflows/nightly.yml` - the `fuzz` job
- `docs/testing.md` - the lane row, the tool note and "The loader fuzz target"
- `src/cpu.h` - one comment reworded (see Deviations)
- `tests/vectors/vectors.c`, `tests/runner/tests.cmake`, `tests/cmake/exit_table.cmake` - the two be32 fixes (see Deviations)

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

**3. [Rule 3 - Blocking] The be32 leg did not build tests/vectors/vectors.c**
- **Found during:** the hosted CI run on the pushed head (the be32 leg of the replay had never run)
- **Issue:** `load_upstream` compared `(uint64_t)n > 0xFFFFFFFFu` with `n` a `size_t`; on powerpc `size_t` is 32 bits,
  so `-Werror=type-limits` rejected the always-false comparison and stopped the build. The line is from plan 02-15.
- **Fix:** `if (n != (size_t)(uint32_t)n)`, the same check without a constant out of range.
- **Files modified:** `tests/vectors/vectors.c`
- **Verification:** `cmake --workflow --preset ci` exit 0; the be32 build passes on run 42722584020
- **Committed in:** `6ed9df1`

**4. [Rule 3 - Blocking] runner.exit_table.self_test failed on be32**
- **Found during:** the next hosted CI run (be32: 207 of 208, the one failure)
- **Issue:** the qemu emulator list `qemu-ppc;-L;/usr/powerpc-linux-gnu` was split a second time inside `-DCMD`, so the
  negative control ran `qemu-ppc` without `-L` and saw a loader error instead of the missing table row. The test is
  from plan 02-23 and could only fail on a leg with a multi-word emulator.
- **Fix:** the self test passes the emulator with bars between its words and `exit_table.cmake` turns them back to
  semicolons.
- **Files modified:** `tests/runner/tests.cmake`, `tests/cmake/exit_table.cmake`
- **Verification:** `cmake --workflow --preset ci` and `--preset hygiene` exit 0; the exit table script run with
  `-DEMULATOR='env|X=1'` printed its line count (exit 0); be32 passes on run 42722584020
- **Committed in:** `5c99bb4`

**5. [Plan reading] The replay has no `--seeds` flag**
- The sibling's replay needed `--seeds` to run its seeds. Here every run starts with them, which is what the third
  truth asks ("replays every seed and every committed crasher"), and the `--write DIR` mode is the only flag.

---

**Total deviations:** 4 auto-fixed (3 blocking, 1 missing critical) and 1 plan reading
**Impact on plan:** none on scope; each fix is one line or one comment. Two of them are in earlier plans' files and
only a leg that had never run could show them.

## Issues Encountered

- **ASan did not start locally.** The fuzz preset builds here with Homebrew llvm@21 standing in for clang-21, but
  every binary with AddressSanitizer hung in dyld's initialisers. The nightly run on ubuntu-26.04 is where ASan ran
  first, and it found nothing in 3248 inputs.
- The nightly fuzz job's rate (53 exec/s with ASan) means a 60 s run is a smoke campaign, not coverage of the whole
  input space; the seeds and the replay carry the gating weight.
- Earlier hosted CI runs on this branch (for older heads) failed or were cancelled; the run on head `5c99bb4` is
  fully green: https://github.com/szTheory/schwaystation/actions/runs/42722584020

## User Setup Required

None - no external service configuration required.

## Known Stubs

None.

## Next Phase Readiness

- `fuzz.regress.exe` is in the inventories and passes on every leg; later plans' fuzz targets can copy
  `replay_main.h` and the `tests/fuzz/tests.cmake` shape.
- Plans 02-26 and 02-27, which waited on this plan's push, are unblocked.

## Self-Check: PASSED

- Created files exist: `tests/fuzz/exe.c`, `tests/fuzz/seeds.c`, `tests/fuzz/seeds.h`, `tests/fuzz/replay_main.h`.
- Commits `088f296`, `c219811`, `e2524fa`, `af8be79`, `6ed9df1` and `5c99bb4` are ancestors of HEAD.
- After the last code commit: `cmake --workflow --preset ci` exit 0 and `cmake --workflow --preset hygiene` exit 0; hosted CI run 42722584020 success on `5c99bb4`.

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*
