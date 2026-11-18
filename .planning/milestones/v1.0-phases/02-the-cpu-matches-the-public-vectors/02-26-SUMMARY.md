---
phase: 02-the-cpu-matches-the-public-vectors
plan: 26
subsystem: docs
tags:
  - docs
  - gate-budget
  - maintaining
  - scans
  - readme

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: the last gates of the phase (docs.api_names from 02-11, runner.exit_table from 02-23), the fuzz and nightly jobs (02-22), the exe label and the diag data directory (02-17, 02-25)
provides:
  - the measured final gate total, 1495 lines against the 1520 cap, with no deferral
  - docs/testing.md sections for the fixtures toolchain, the vectors harness options and the nightly jobs outside the required check, and the Budgets text naming the cap and the ship ratchet
  - docs/maintaining.md procedures Bumping armips, Re-pinning the CPU vectors and Regenerating the stub kernel, each step a command
  - a README section naming --exe, --probe and --trace first-fault
  - a planning-identifier scan and an emulator-name scan over code and docs that both exit 1, and a clean source tree after a clean ci workflow
affects:
  - 02-27
  - ship

actuals:
  tokens: 4100
  tasks: 3
  commits: 3

plan_head_before: 125184011328a089bd4dbf6ec0f38abcfe084ffe
plan_head_after: 1f7c544c959f7479a2e0defa289342fe515fbf66
commits: 3

tech-stack:
  added: []
  patterns:
    - "maintainer procedures are numbered lists in which every step is a command, and the vectors one was run end to end once and reproduced the committed sample byte for byte"
    - "the hardware pages point to docs/maintaining.md for the stub regeneration commands rather than repeating them"

key-files:
  created: []
  modified:
    - docs/testing.md
    - docs/maintaining.md
    - README.md
    - docs/hardware/bus.md
    - docs/hardware/cpu.md
    - src/cpu_exec.inc

key-decisions:
  - "The gate-line total after the last gate of the phase is 1495, at most 1520, so the deferral branch did not run: no gate file changed and todos 007 and 008 stay as they are; ship ratchets the cap to the total it then measures"
  - "The budget text says the cap is never raised and a gate that does not fit replaces or folds an older one, and it names the literal 1520"
  - "The stub regeneration commands live once, in docs/maintaining.md; the CPU page links to them"

requirements-completed:
  - CPU-01
  - CPU-02
  - CPU-03
  - CPU-04
  - CPU-05

coverage:
  - id: D1
    description: "The final gate-line total is measured after the last gate landed and is at most 1520, so no check is deferred and each of docs.api_names and runner.exit_table is registered"
    requirement: "CPU-01"
    verification:
      - kind: other
        ref: "t=$(cat scripts/*.sh tests/cmake/*.cmake .githooks/* | wc -l) && test $t -le 1520, t was 1495 (exit 0)"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset hygiene then check_junit.cmake against tests/inventory/hygiene.txt (exit 0, 21 tests ran once each)"
        status: pass
      - kind: integration
        ref: "grep -q '^docs.api_names' tests/policy/inventory.txt && grep -q '^runner.exit_table' tests/runner/inventory.txt (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "docs/testing.md describes the lanes, the nightly jobs outside CI required, the fixtures toolchain, the vectors harness options, the exe label and tests/diag; docs/maintaining.md has the three procedures; README names --exe, --probe and --trace first-fault and stays under 16 KiB"
    requirement: "CPU-04"
    verification:
      - kind: other
        ref: "grep -q 'vectors-full' docs/testing.md && grep -q 'Bumping armips' docs/maintaining.md && grep -q -- '--exe' README.md (exit 0, baseline at HEAD before the plan: exit 1)"
        status: pass
      - kind: other
        ref: "test $(wc -c < README.md) -lt 16384 (exit 0, 4.2 KiB)"
        status: pass
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^docs\\.' --no-tests=error (exit 0, 3 tests)"
        status: pass
    human_judgment: false
  - id: D3
    description: "The vectors re-pinning procedure reproduces the committed sample: the 55 pinned blobs fetched with the procedure's loop each hash to their pin, and tools/sst_slice.py over them gives a file whose SHA-256 equals tests/vectors/r3000/sst-sample.bin"
    requirement: "CPU-01"
    verification:
      - kind: other
        ref: "gh api contents listing at the pin diffed with the r3000 rows of pins.txt (exit 0, no difference), the loop's git hash-object checks (exit 0), then shasum -a 256 of the sliced sample and the committed one, both d6ef9a9c..."
        status: pass
    human_judgment: false
  - id: D4
    description: "After rm -rf build/ci and a full cmake --workflow --preset ci, git status lists nothing under src, include, runner, tests or tools, so no test writes outside the build tree"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "rm -rf build/ci && cmake --workflow --preset ci && test -z \"$(git status --porcelain --untracked-files=all -- src include runner tests tools)\" (exit 0, 240 of 240 tests ran once each)"
        status: pass
    human_judgment: false
  - id: D5
    description: "No planning identifier appears in code, tests, CMake, scripts, workflows, config or user docs, and no other emulator's name appears in src, include, runner, tests or docs"
    requirement: "CPU-05"
    verification:
      - kind: other
        ref: "git grep -nIE -e 'D-[0-9]{2}' -e 'CPU-0[0-9]' -e 'FRAME-0[0-9]' -e 'PX\\.[0-9]' -e 'DEC\\.[0-9]' -e 'XFER-[0-9]' -e '\\[CF\\.[0-9]' over src include runner host libretro tests tools cmake scripts .githooks .github CMakeLists.txt CMakePresets.json pins.txt docs README.md (exit 1; three hits before, fixed in 1f7c544)"
        status: pass
      - kind: other
        ref: "git grep -nIiw -e ares -e nugget -e duckstation -e mednafen -e pcsx -e mame -e beetle -e avocado -e rustation -- src include runner tests docs (exit 1; one hit before, fixed in 1f7c544)"
        status: pass
    human_judgment: false
  - id: D6
    description: "The three hardware pages were read once by hand against the code by the agent, and the one duplicated procedure on the CPU page was replaced by a link"
    requirement: "CPU-05"
    verification:
      - kind: manual_procedural
        ref: "the Hand reading section of this file; ctest --test-dir build/ci -R '^docs\\.' --no-tests=error and cmake --workflow --preset hygiene after the edits (exit 0)"
        status: pass
    human_judgment: false
---

# Phase 2 Plan 26: Final gate total, guides, README and the scans Summary

**The gate code ends the phase at 1495 of 1520 lines with nothing deferred, the testing guide and the maintainer guide cover the fixtures toolchain, the vectors harness and three re-pinning and regeneration procedures, the README names `--exe`, `--probe` and `--trace first-fault`, and both leak scans exit 1.**

## Performance

- **Duration:** about 9 min
- **Started:** 2026-11-09T23:55Z
- **Completed:** 2026-11-10T00:03Z
- **Tasks:** 3
- **Files modified:** 6

## Gate measurement (Task 1)

- Command: `cat scripts/*.sh tests/cmake/*.cmake .githooks/* | wc -l`. Result: **1495** (scripts 586 lines, `tests/cmake` 902, hooks 7), at most the 1520 cap.
- **Branch taken: none.** The deferral branch did not run. No gate file changed, `docs.api_names` and `runner.exit_table` stay registered with their self tests, and todos 007 and 008 carry no deferral line.
- Change from phase 1's 1392: **+103 lines.** `tests/cmake/api_names.cmake` accounts for 55 and `tests/cmake/exit_table.cmake` for 29; the other 19 are small additions to existing gate scripts.
- **For ship:** the final total to ratchet the cap to is **1495**. The cap is not raised. `docs/testing.md` Budgets now says the cap is 1520, was 1495 at the phase's last gate, is never raised, and ship sets it to the measured total.

## Accomplishments

- `docs/testing.md`: a section on the fixtures toolchain (the armips pin, the host-native build, no committed fixture except the generated stub array, `fixtures.stub_drift`, which harness runs what); a section on the vectors harness with its options table, exit statuses and the two named rules; a sentence making the nightly workflow's `vectors-full` and `fuzz` jobs (and the benches) explicitly not part of `CI required`.
- `docs/maintaining.md`: "Bumping armips", "Re-pinning the CPU vectors" and "Regenerating the stub kernel" in numbered steps that are each a command, and a paragraph on the nightly workflow's two non-bench jobs. The vectors procedure was run once end to end: the listing command's output equals the 55 `r3000-` rows of `pins.txt`, the 55 blobs fetched with its loop each hash to their pin, and `tools/sst_slice.py` over them reproduces `tests/vectors/r3000/sst-sample.bin` byte for byte (SHA-256 `d6ef9a9c...`). The armips procedure's commit query returned the current pin and its `ext` listing names the two directories PROVENANCE records.
- `README.md`: a "Run a program" section with `--exe`, `--tty`, `--probe` and `--trace first-fault`; 4.2 KiB of the 16 KiB budget.
- The scans found four hits, all fixed in place: two planning identifiers in comments of `src/cpu_exec.inc` and one emulator's name on the bus page (now "a reference emulator"). The CPU page's stub-regeneration block, which named a target and duplicated the new procedure, now links to it.
- A clean `build/ci` followed by the whole ci workflow leaves `git status --porcelain --untracked-files=all -- src include runner tests tools` empty, so CTest's parallel fixtures, exe and vectors tests write only into the build tree.

## Task Commits

1. **Task 1: Tracer - measure the final gate total, apply the budget rule, prove the budget gate passes** - `992ec6f` (docs)
2. **Task 2: The guides, the README and the source-tree write check** - `b90626f` (docs)
3. **Task 3: The planning-ID and emulator-name scans and the notes for ship** - `1f7c544` (docs)

**Plan metadata:** recorded in the docs commit that carries this file.

## Files Created/Modified

- `docs/testing.md` - Budgets text; fixtures toolchain; vectors harness; nightly jobs outside the required check
- `docs/maintaining.md` - the three procedures and the nightly paragraph
- `README.md` - the run-a-program section
- `docs/hardware/bus.md` - no emulator name; one paragraph rewrapped
- `docs/hardware/cpu.md` - the stub regeneration block replaced by a link to the maintainer page
- `src/cpu_exec.inc` - two comments lose a planning identifier each (comment-only change)

## Decisions Made

See `key-decisions`. In short: no deferral because the total is within the cap, the cap text names the literal limit and the ratchet, and the regeneration commands exist once.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The budget text first wrote the limit as 1,520**
- **Found during:** Task 1
- **Issue:** the acceptance check is `grep -q '1520' docs/testing.md`, which a comma-formatted number does not match.
- **Fix:** the page writes 1520 and 1495 without a thousands separator.
- **Files modified:** docs/testing.md
- **Verification:** the criterion exits 0
- **Committed in:** 992ec6f

**2. [Rule 1 - Bug] Scan hits in files the plan did not list**
- **Found during:** Task 3
- **Issue:** the scans found `D-29` and `DEC.24` in two comments of `src/cpu_exec.inc` and an emulator's name in `docs/hardware/bus.md`; the plan lists only `docs/testing.md` for in-place fixes, but its action says to fix any hit in the file that holds it.
- **Fix:** the identifiers are dropped (the comments still cite their document sections) and the name became "a reference emulator".
- **Files modified:** src/cpu_exec.inc, docs/hardware/bus.md
- **Verification:** both scans exit 1; `cmake --workflow --preset ci` exit 0 (240 tests) and `--preset hygiene` exit 0 (21 tests) after the change
- **Committed in:** 1f7c544

**3. [Rule 1 - Bug] A duplicated procedure on the CPU page**
- **Found during:** Task 3, the hand reading
- **Issue:** the page duplicated the stub regeneration commands that `docs/maintaining.md` now holds, so two copies could drift.
- **Fix:** the CPU page links to the maintainer procedure; the commands are the same ones.
- **Files modified:** docs/hardware/cpu.md
- **Verification:** `ctest -R '^docs\.'` and the hygiene lane exit 0
- **Committed in:** 1f7c544

---

**Total deviations:** 3 auto-fixed (2 bugs, 1 blocking)
**Impact on plan:** no scope growth; each is a text fix required by a stated check or by the one-copy rule.

## Hand reading

I read `docs/hardware/cpu.md`, `docs/hardware/cop0.md` and `docs/hardware/bus.md` once, in full, against the code paths they name. Beyond the fixes above (the two identifiers, the emulator's name and the duplicated regeneration commands, whose build target and command were in fact correct) I found nothing that contradicts the code; the stub and mailbox tables match the committed stub and the runner's mailbox reader (status at 08h, count at 0Ah, handler word at 0Ch, records from 10h).

## Notes for ship

1. **CPU-04's handler wording.** The requirement says "an exception handler recording into the mailbox". The stub's default handler records Cause, EPC, BadVaddr and SR in the exception record in kernel RAM at `80000210h` and, when the mailbox magic is present, sets the mailbox status word to FFh; it does not write records into the mailbox itself. The requirement is met by that handler as designed, and ship can align the requirement text with it.
2. **The 801FF00h typo.** The plan names CPU-04's `801FF00h` as a typo for `801FFF00h`. `.planning/REQUIREMENTS.md` already reads `801FFF00h` for the stack default (grep of `801FF00h` over `.planning/ROADMAP.md`, `REQUIREMENTS.md` and `PROJECT.md` finds nothing), so no change is needed there; the runner and the stub use `801FFF00h` (`fixtures.hello.boot`).
3. **Roadmap criteria 4 and 5 run as written, without `--frames`.** `schwaystation-run --exe hello.exe --tty --stop-on mailbox` and the first-fault trace command stop on their own under the runner's 600-frame default cap for `--exe`, pinned by `fixtures.hello.criterion4` and `fixtures.trace.criterion5`.
4. **The final gate total to ratchet to is 1495**, measured after the last gate of the phase.

## Issues Encountered

None.

## Known Stubs

None. The plan touched documentation and two comments.

## Threat Flags

None. No network endpoint, auth path, file access pattern or schema changed; the vectors procedure was run against the public upstream repository with read-only API calls and its files stayed in the session scratch directory.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 02-27 runs the hosted CI and nightly jobs on the branch, measures timeouts, closes the probe pull request and moves the closed todos (007, 008, 019, 020, 023) from `pending` to `completed`.
- No push, hosted run or pull-request step was part of this plan.

## Self-Check: PASSED

- Files: docs/testing.md, docs/maintaining.md, README.md, docs/hardware/bus.md, docs/hardware/cpu.md, src/cpu_exec.inc present and committed.
- Commits 992ec6f, b90626f and 1f7c544 are ancestors of HEAD; `git rev-list --count 125184011328a089bd4dbf6ec0f38abcfe084ffe..HEAD` was 3 before this file.
- Acceptance commands: both Task 1 criteria exit 0, both Task 2 criteria exit 0, both Task 3 scans exit 1.
- Verification: `cmake --workflow --preset ci` exit 0 and its JUnit check (240 tests ran once each); `cmake --workflow --preset hygiene` exit 0 and its JUnit check (21 tests ran once each).

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*
