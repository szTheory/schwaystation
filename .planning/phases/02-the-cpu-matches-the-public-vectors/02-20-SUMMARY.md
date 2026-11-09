---
phase: 02-the-cpu-matches-the-public-vectors
plan: 20
subsystem: runner
tags: [runner, exe, tty, exit-port, usage, docs, fixtures]

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: schway_load_exe and the probe's file checks (02-08, 02-13), the stub kernel and the TTY ring with schway_get_tty (02-16, 02-18), the exit port and the HOOK stop (02-17)
provides:
  - schwaystation-run --exe FILE, with a 600-frame default when neither --frames nor --bench is given
  - schwaystation-run --tty, which sets the debug-port flag and prints each guest line as tty <bytes>
  - exit status 6 (fixture) for an exit-port byte other than 00h, with the line exit code=0x<hh>
  - tests/fixtures/src/exit_port.asm and the tests fixtures.hello.tty, fixtures.exit_port, runner.usage.exe and runner.exe.missing
  - the runner page's Running a PS-EXE, TTY and Exit port sections
affects: [02-21, 02-23, 02-24, 02-25, 02-27]

actuals:
  tokens: 3500
  tasks: 3
  commits: 3

plan_head_before: 12afabab3f623e172feb604008f3f449a09f05d8
plan_head_after: 3adfe4bb756b6abb79f3ac13b1cc9ccd0bb2a506
commits: 3

tech-stack:
  added: []
  patterns:
    - "the probe's file reader and refusal line are shared with --exe, so both commands refuse a file in one form"
    - "a script case that checks two images registers as one test and calls a local expect_exit helper"

key-files:
  created:
    - tests/fixtures/src/exit_port.asm
  modified:
    - runner/main.c
    - runner/probe.c
    - runner/probe.h
    - runner/exit_status.h
    - tests/fixtures/tests.cmake
    - tests/fixtures/inventory.txt
    - tests/runner/tests.cmake
    - tests/runner/inventory.txt
    - docs/runner.md

key-decisions:
  - "the exit port acts only with the debug-port flag, so a fixture that reports through it is run with --tty; without --tty the write is ignored and the run goes the full frame count"
  - "a line over 4096 bytes is printed in pieces of 4096, so a guest that never ends a line cannot grow the runner's memory"
  - "a missing file reads as cannot open the file, the probe's wording; runner.exe.missing matches cannot (open|read)"
  - "reaching the frame cap with the program still running exits 0; the hang report belongs to the later plan that lands it"

patterns-established:
  - "an --exe refusal is schwaystation-run: <file>: <reason> and exit 1, the core's status text for an invalid image"

requirements-completed: [CPU-04]

coverage:
  - id: D1
    description: "--exe FILE reads the file, loads it with schway_load_exe and runs the --frames count, or 600 frames when neither --frames nor --bench is given; a missing or invalid file exits 1 with a line naming the file and the reason"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.exe\\.missing$' --no-tests=error (exit 0)"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0, 212 of 212 tests)"
        status: pass
    human_judgment: false
  - id: D2
    description: "--exe given twice or with --probe, --exe without a file, and --tty without --exe are usage errors with exit 2 and nothing run"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.usage\\.exe$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D3
    description: "--tty sets the debug-port flag, drains the ring after every frame and prints each completed line as tty <bytes>, so hello.exe prints tty hello port and tty hello putchar and exits 0"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.hello\\.tty$' --no-tests=error (exit 0)"
        status: pass
      - kind: integration
        ref: "build/ci/schwaystation-run --exe build/ci/fixtures/hello.exe --tty --frames 10 | grep -qE '^tty .*hello putchar' (exit 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "A HOOK stop from the exit port ends the run: byte 00h exits 0 with no report, byte 2Ah prints exit code=0x2a and exits 6; status 6 is in the --help table"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.exit_port$' --no-tests=error (exit 0)"
        status: pass
      - kind: integration
        ref: "build/ci/schwaystation-run --help | grep -qE '^ +6 +fixture' (exit 0)"
        status: pass
    human_judgment: false
  - id: D5
    description: "The debug-port flag is clear unless --tty is given: a run of exit_port_fail.exe without --tty ignores the port write and exits 0"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "build/ci/schwaystation-run --exe build/ci/fixtures/exit_port_fail.exe (exit 0, no report line)"
        status: pass
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.tty\\.flag_off$' --no-tests=error (exit 0), the core-level retail case"
        status: pass
    human_judgment: false
  - id: D6
    description: "docs/runner.md documents --exe, --tty, the 600-frame default, the tty line format, the exit port and status 6, within the page budget"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^docs\\.' --no-tests=error (exit 0)"
        status: pass
      - kind: other
        ref: "grep -q -- '--tty' docs/runner.md && grep -q -- '--exe' docs/runner.md && grep -q '600' docs/runner.md (exit 0)"
        status: pass
    human_judgment: false

duration: 7min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 20: The runner loads a PS-EXE, prints its text and ends on the exit port Summary

**`schwaystation-run --exe FILE --tty` loads a PS-EXE, runs it 600 frames by default, prints the guest's text as `tty` lines from both the debug port and putchar, and ends on the exit port with status 0, or status 6 and `exit code=0x<hh>`.**

## Performance

- **Duration:** 7 min
- **Completed:** 2026-11-09T22:45:00Z
- **Tasks:** 3
- **Files modified:** 10 (one created)

## Accomplishments

- `--exe` reads at most 0800h + 2 MiB + 1 bytes through the probe's reader, loads the image after the profile is set and runs the frame loop; `--frames` or `--bench` wins over the 600-frame default.
- `--tty` sets `SCHWAY_PROFILE_DEBUG_PORT` and nothing else does. After every frame the runner drains `schway_get_tty` into a line collector and prints `tty <raw bytes>`; a partial line is flushed at the end of the run.
- The loop reads `schway_run_result.stop`; a HOOK stop ends it, byte 00h as status 0 and any other byte as `exit code=0x<hh>` and status 6, which joined the exit table and `--help`.
- `exit_port.asm` assembles twice (`-equ CODE 0` and `-equ CODE 0x2A`); `fixtures.exit_port` runs both. `fixtures.hello.tty` runs hello.exe, the tracer, and was verified end to end with the whole ci workflow before the exit port work began.
- The usage refusals for `--exe` and `--tty` live in the new `runner.usage.exe` case, and `runner.exe.missing` covers a missing file and a file that is not a PS-EXE.
- The runner page gained the synopsis and options rows, three sections and the status 6 row.

## Task Commits

1. **Task 1: Tracer - the runner loads hello.exe and prints its tty lines** - `ba7d27a` (feat)
2. **Task 2: The exit port, exit 6 and the --exe refusals** - `ecd62e1` (feat)
3. **Task 3: The runner page for --exe, --tty and exit 6** - `3adfe4b` (docs)

**Plan metadata:** recorded in the docs commit that carries this file.

## Files Created/Modified

- `runner/main.c` - the `--exe` and `--tty` options, the profile flag, the load, the line collector and the exit-port outcome
- `runner/probe.c`, `runner/probe.h` - `schway__read_exe_file` and `schway__refuse_file`, split out of the probe so `--exe` shares them
- `runner/exit_status.h` - status 6, `fixture`
- `tests/fixtures/src/exit_port.asm` - writes `CODE` to 1F802082h and loops
- `tests/fixtures/tests.cmake`, `tests/fixtures/inventory.txt` - the `tty` and `exit_port` script cases, two images and two tests
- `tests/runner/tests.cmake`, `tests/runner/inventory.txt` - `runner.usage.exe` and `runner.exe.missing`
- `docs/runner.md` - the user-facing description

## Decisions Made

- The exit port needs the debug-port flag in the core, so the exit-port fixtures run with `--tty`; the page says that without `--tty` the port has no effect.
- A line over 4096 bytes is printed in pieces of 4096 to keep the runner's memory bounded for a hostile guest.
- The runner does not report the ring's dropped-byte count; a program that writes more than 4 KiB in one frame keeps the first 4096 bytes and the page says so.
- Reaching the frame cap with the program still running is status 0; plan 02-23 lands the hang report.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The probe's reader was reused rather than copied, so probe.c and probe.h changed**
- **Found during:** Task 1
- **Issue:** the plan lists only runner/main.c, but the file's read, size cap and refusal wording already existed as static code in probe.c; a second copy would drift from it.
- **Fix:** split `schway__read_exe_file` and `schway__refuse_file` out of `schway__probe_print`; its behaviour and its report are unchanged.
- **Files modified:** runner/probe.c, runner/probe.h
- **Verification:** `fixtures.probe.hello` and the full ci workflow passed (exit 0).
- **Committed in:** ba7d27a

**2. [Rule 1 - Bug] The missing-file message is the probe's, not the plan's `cannot read`**
- **Found during:** Task 2
- **Issue:** the plan expects `cannot read` for a missing file, but the shared reader says `cannot open the file` for a failed open and `cannot read the file` for a failed read.
- **Fix:** `runner.exe.missing` matches `cannot (open|read)`, a tolerant regex as the project rules ask; the wording stays one form across `--probe` and `--exe`.
- **Files modified:** tests/runner/tests.cmake
- **Verification:** `ctest -R '^runner\.exe\.missing$'` exit 0
- **Committed in:** ecd62e1

**3. [Rule 3 - Blocking] The exit-port fixtures need `--tty`**
- **Found during:** Task 2
- **Issue:** the core decodes the exit port only with the debug-port flag, and the plan's prohibition keeps that flag clear unless `--tty` is given, so the fixtures are run with `--tty`.
- **Fix:** the `exit_port` script case passes `--tty`; the page documents it.
- **Files modified:** tests/fixtures/tests.cmake, docs/runner.md
- **Committed in:** ecd62e1 and 3adfe4b

---

**Total deviations:** 3 auto-fixed (1 bug, 2 blocking)
**Impact on plan:** no scope growth; each follows from the committed core and shared code.

## Issues Encountered

None.

## Known Stubs

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 02-23 can widen status 6 and add the mailbox outcome statuses; the runner already reads `schway_run_result` after each frame.
- The 600-frame cap is where the hang report lands.
- No push or hosted run was part of this plan.

## Self-Check: PASSED

- Files: runner/main.c, runner/probe.c, runner/probe.h, runner/exit_status.h, tests/fixtures/src/exit_port.asm, docs/runner.md present.
- Commits ba7d27a, ecd62e1 and 3adfe4b are ancestors of HEAD; `git rev-list --count 12afabab3f623e172feb604008f3f449a09f05d8..HEAD` was 3 before this file.
- `cmake --workflow --preset ci` exit 0 (212 of 212 tests); the plan's acceptance commands exit 0.

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*
