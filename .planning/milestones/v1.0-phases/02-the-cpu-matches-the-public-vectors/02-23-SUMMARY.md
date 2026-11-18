---
phase: 02-the-cpu-matches-the-public-vectors
plan: 23
subsystem: runner
tags:
  - runner
  - mailbox
  - hang
  - exit-table
  - fixtures
  - hashes

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: schway_peek, the stub kernel and hello.exe's sealed mailbox (02-16, 02-17), --exe, --tty and exit status 6 (02-20)
provides:
  - schwaystation-run --stop-on mailbox, reading the 256-byte mailbox at RAM 10000h through schway_peek once per frame
  - the mailbox lines, the CRC-32 check, exit 6 for a fault status or a CRC mismatch and the hang line with exit 8
  - tests/cmake/exit_table.cmake with runner.exit_table and its self test (todo 007 closed by this plan)
  - hang.exe, mailbox_fault.exe and mailbox_crc.exe fixtures and their tests
  - hashes.txt line exe hello.exe frame 30 sha256 for the eight-lane comparison
affects:
  - 02-24
  - 02-25
  - 02-26
  - 02-27

actuals:
  tokens: 8200
  tasks: 3
  commits: 3

plan_head_before: cae6bb9f647f938e98db3098cb81a42259e3913f
plan_head_after: 1f8e2321b3bb327c5957e86af2653eef402fd3cd
commits: 3

tech-stack:
  added: []
  patterns:
    - "the runner reads the fixture mailbox with schway_peek, so the core has no mailbox code and no stop reason for it"
    - "expect_runner in the fixtures test file runs the runner through expect_fail.cmake for one exit status and one tolerant regex"

key-files:
  created:
    - runner/mailbox.c
    - runner/mailbox.h
    - tests/cmake/exit_table.cmake
    - tests/fixtures/src/hang.asm
    - tests/fixtures/src/mailbox_fail.asm
  modified:
    - runner/main.c
    - runner/exit_status.h
    - runner/CMakeLists.txt
    - tests/fixtures/tests.cmake
    - tests/fixtures/inventory.txt
    - tests/runner/tests.cmake
    - tests/runner/inventory.txt
    - tests/cmake/hashes.cmake
    - docs/runner.md

key-decisions:
  - "--stop-on mailbox is the only mode that reports a hang; a plain --exe run that reaches its cap still exits 0, so hello.exe's tty and hash runs keep passing"
  - "a done status checks the CRC-32 over bytes 10h to FFh; a fault status skips the CRC because the stub kernel writes FFh over the status word without rewriting it, and prints the same lines with the count the word left"
  - "the mailbox is read after the frame's tty lines and before the exit-port outcome is acted on, so a HOOK stop in the same frame is read and an exit-port code still reaches status 6"
  - "the mailbox layout follows the committed stub and mailbox.inc (CRC 04h, status 08h, count 0Ah, records 10h), as plan 02-17 decided, not the older layout in the conformance document"

requirements-completed:
  - CPU-04

coverage:
  - id: D1
    description: "--stop-on mailbox reads the mailbox through schway_peek after every frame; a sealed mailbox with status 00h and a matching CRC-32 prints the status line and one line per record and exits 0"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.hello\\.mailbox$' --no-tests=error (exit 0)"
        status: pass
      - kind: other
        ref: "build/ci/schwaystation-run --exe build/ci/fixtures/hello.exe --tty --stop-on mailbox --frames 30 | grep -qE '^mailbox SWMB status=00' (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The success-criterion command schwaystation-run --exe hello.exe --tty --stop-on mailbox, with the 600-frame default cap, prints tty lines and the SWMB line and exits 0"
    requirement: "CPU-04"
    verification:
      - kind: e2e
        ref: "ctest --test-dir build/ci -R '^fixtures\\.hello\\.criterion4$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D3
    description: "A program that never reports done or fault is a hang: hang frames=<N> pc=0x<pc> and exit 8, the pc read through the state visitor's cpu.pc"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.hang$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "A mailbox with status FFh prints its lines and exits 6, and a done status with a wrong CRC prints mailbox crc mismatch and exits 6"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.(mailbox_fault|mailbox_crc)$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D5
    description: "The exit table lists 0, 1, 2, 6 and 8, and runner.exit_table fails when --help's table and docs/runner.md differ line for line in either direction; the self test fails a page copy without the 8 row"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.exit_table(\\.self_test)?$' --no-tests=error (exit 0)"
        status: pass
      - kind: other
        ref: "build/ci/schwaystation-run --help | grep -qE '^ +8 +hang' (exit 0, baseline at HEAD before the plan: exit 1)"
        status: pass
    human_judgment: false
  - id: D6
    description: "--stop-on is refused with exit 2 when it lacks a value, names anything but mailbox, is given twice or has no --exe"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner\\.usage\\.exe$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D7
    description: "hashes.txt carries exe hello.exe frame 30 sha256 followed by 64 hex digits, from --exe hello.exe --frames 30 --hash, so hash-equality compares the CPU-run frame across lanes"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0, 226 of 226 tests) then grep -qE '^exe hello\\.exe frame 30 sha256 [0-9a-f]+$' build/ci/hashes.txt (exit 0)"
        status: pass
    human_judgment: false
  - id: D8
    description: "docs/runner.md documents --stop-on mailbox, the mailbox layout and lines, the CRC rule, the hang line, statuses 6 and 8 and the frame cap, inside the page budget"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^(runner\\.exit_table|docs\\.budgets)$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false

duration: 6min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 23: The runner stops on the fixture mailbox and tells a hang from a failure Summary

**`schwaystation-run --exe hello.exe --tty --stop-on mailbox` prints hello's text and its mailbox records and exits 0; a program that never finishes exits 8 with `hang frames=N pc=0x...`, a fault status or a bad CRC exits 6, and the exit table cannot drift from `docs/runner.md`.**

## Performance

- **Duration:** 6 min
- **Started:** 2026-11-09T23:00:11Z
- **Completed:** 2026-11-09T23:06:00Z
- **Tasks:** 3
- **Files modified:** 14 (five created)

## Accomplishments

- `runner/mailbox.c` reads the magic and the status word with `schway_peek`, and only at status 00h or FFh reads the full 256 bytes, so a running program costs two peeks per frame. A done status is checked against the CRC-32 over bytes 10h to FFh with the runner's own CRC routine.
- `main.c` reads the mailbox after each frame's tty output and before acting on a HOOK stop. Done ends the run, fault and a CRC mismatch are status 6, and reaching the frame cap with the mailbox still pending prints `hang frames=<N> pc=0x<pc>` and exits 8. The program counter comes from the state visitor's `cpu.pc` field.
- The criterion command, `--exe hello.exe --tty --stop-on mailbox` with no `--frames`, is its own test, `fixtures.hello.criterion4`.
- `hang.exe`, `mailbox_fault.exe` and `mailbox_crc.exe` are built from two new sources; the last two come from one source with `-equ MODE`.
- `tests/cmake/exit_table.cmake` is the cogmaster seed, renamed, with a self test that checks a page copy lacking the 8 row and requires the failure to name it. The gate total after this plan is **1494 lines** (`cat scripts/*.sh tests/cmake/*.cmake .githooks/* | wc -l`), against the 1520 limit; plan 02-26 measures the final total and alone decides any deferral.
- `hashes.txt` ends with `exe hello.exe frame 30 sha256 <hex>`.
- This plan closes todo 007 (the exit-table gate); plan 02-27 alone moves the todo file.

## Task Commits

1. **Task 1: Tracer - hello's mailbox read through schway_peek ends the run with exit 0** - `d9bcb25` (feat)
2. **Task 2: Hang, fixture failure, CRC mismatch, the exit table and its check** - `2644c1d` (feat)
3. **Task 3: The hello frame hash in hashes.txt and the runner page** - `1f8e232` (docs)

**Plan metadata:** recorded in the docs commit that carries this file.

## Files Created/Modified

- `runner/mailbox.c`, `runner/mailbox.h` - the mailbox read, CRC check and print
- `runner/main.c` - `--stop-on mailbox`, the per-frame read, the hang report and the `cpu.pc` reader
- `runner/exit_status.h` - status 8 `hang` and the widened status 6 meaning
- `runner/CMakeLists.txt` - `mailbox.c` in the runner's source list, which the stop-hook runner shares
- `tests/fixtures/src/hang.asm`, `tests/fixtures/src/mailbox_fail.asm` - the two new fixture sources
- `tests/fixtures/tests.cmake`, `tests/fixtures/inventory.txt` - the `mailbox` and `criterion4` script cases, `expect_runner` and five tests
- `tests/cmake/exit_table.cmake` - the exit-table check, seeded from cogmaster f09ac97096ac (same author, MIT)
- `tests/runner/tests.cmake`, `tests/runner/inventory.txt` - `runner.exit_table`, its self test and the `--stop-on` refusals in `runner.usage.exe`
- `tests/cmake/hashes.cmake` - the hello frame line
- `docs/runner.md` - the synopsis, option row, Mailbox section and status rows

## Decisions Made

- A plain `--exe` run that reaches its cap still exits 0. Only `--stop-on mailbox` asks for a verdict, so `fixtures.hello.tty` and the hash runs, whose program loops forever by design, are unchanged.
- A fault status skips the CRC check. The stub kernel overwrites the status word with FFh and clears the version, count and id bytes without rewriting the CRC, so for that path the CRC is never valid; the runner prints the count the word left, which is 0 for a stub fault.
- Reading the mailbox comes before the HOOK break in the frame loop, so a program that seals its mailbox and writes the exit port in one frame has both reported.
- The layout is the committed stub's (status 08h, count 0Ah, records 10h), per plan 02-17, not the older layout in the conformance document.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] CMake regular expressions have no `{n}` count**
- **Found during:** Task 2
- **Issue:** the plan's hang regex `pc=0x[0-9a-f]{8}` matches nothing in CMake, so `fixtures.hang` failed against output that was correct.
- **Fix:** the regex is built with `string(REPEAT "[0-9a-f]" 8 ...)`, which keeps the eight-digit requirement.
- **Files modified:** tests/fixtures/tests.cmake
- **Verification:** `ctest -R '^fixtures\.hang$'` exit 0
- **Committed in:** 2644c1d

**2. [Rule 3 - Blocking] The page's exit-table rows moved from Task 3 to Task 2**
- **Found during:** Task 2
- **Issue:** Task 2's verify runs `runner.exit_table`, which compares `--help` with `docs/runner.md`, and `docs/runner.md` is listed only under Task 3; the check would fail at the Task 2 commit.
- **Fix:** Task 2 updates the two table rows (6 widened, 8 added) in `docs/runner.md`; Task 3 adds the rest of the page.
- **Files modified:** docs/runner.md
- **Verification:** `runner.exit_table` and its self test exit 0 at the Task 2 commit
- **Committed in:** 2644c1d

**3. [Rule 2 - Missing critical] `--stop-on` refusals are tested**
- **Found during:** Task 2
- **Issue:** the plan registers no test for the new option's usage errors, and every behaviour change lands with a test.
- **Fix:** five `refuse` lines in the `exe` case of `runner.usage.exe` (no `--exe`, no value, wrong value, given twice).
- **Files modified:** tests/runner/tests.cmake
- **Verification:** `ctest -R '^runner\.usage\.exe$'` exit 0
- **Committed in:** 2644c1d

---

**Total deviations:** 3 auto-fixed (1 bug, 1 missing critical, 1 blocking)
**Impact on plan:** no scope growth; each follows from the CMake dialect, the test ordering or the project's test rule.

## Issues Encountered

None.

## Known Stubs

None.

## Threat Flags

None. The runner reads only the core's RAM through the public peek; the new fixtures carry no BIOS, disc or state bytes.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 02-26 measures the gate total (1494 lines after this plan, limit 1520) and decides any deferral.
- Plan 02-27 moves todo 007 out of `pending`.
- No push, hosted run or pull-request step was part of this plan; the push policy for this phase is unchanged.

## Self-Check: PASSED

- Files: runner/mailbox.c, runner/mailbox.h, tests/cmake/exit_table.cmake, tests/fixtures/src/hang.asm, tests/fixtures/src/mailbox_fail.asm present.
- Commits d9bcb25, 2644c1d and 1f8e232 are ancestors of HEAD; `git rev-list --count cae6bb9f647f938e98db3098cb81a42259e3913f..HEAD` was 3 before this file.
- `cmake --workflow --preset ci` exit 0 (226 of 226 tests) and `cmake --workflow --preset hygiene` exit 0 (21 of 21); the plan's three acceptance commands exit 0.

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*
