---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 19
subsystem: runner
tags:
  - runner
  - state-hash
  - hashes
  - cross-lane

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: the gte_vectors, dma_chain and timers_lines fixtures (plans 07, 16 and 17) and the state walk that covers the new devices
provides:
  - schwaystation-run --hash-state, which prints state sha256 over the saved state after its 20-byte header, with no core API
  - runner.hash_state, a portable test that the line is well formed, stable for one run and different a frame later
  - frame and state lines of hello.exe, gte_vectors.exe, dma_chain.exe and timers_lines.exe in hashes.txt, so the eight-lane comparison covers the GTE, DMA and timers
affects:
  - the hash-equality job, which now compares eight more lines per lane through the same file
  - the GPU phase, whose pictures make the frame lines differ between fixtures

actuals:
  tokens: 1456
  tasks: 2
  commits: 2

plan_head_before: a0a7f05a2b7adf2db2338bb1f7a6d82c42f15821
plan_head_after: 391a38abcf96f54ce9e9fbb21e275465cc7c93d8
commits: 2

tech-stack:
  added: []
  patterns:
    - "a lane-comparison line is taken from the end of a run's output with a capture group, so a diagnostic line ahead of it (unbuilt dev=gpu) does not enter the comparison"
    - "CMake regexes have no repetition count, so a fixed-width hex digest is matched by string(REPEAT) of the digit class"

key-files:
  created: []
  modified:
    - runner/main.c
    - tests/runner/tests.cmake
    - tests/runner/inventory.txt
    - tests/cmake/hashes.cmake
    - docs/runner.md
    - docs/testing.md

key-decisions:
  - "The runner keeps its own constant for the 20-byte state header, since runner code does not include src/state.h; docs/embedding.md is the cited source of the layout"
  - "hashes.cmake takes the last two lines of each run with a capture group instead of requiring exactly two lines, because dma_chain.exe prints an unbuilt dev=gpu line first; the state already holds that counter, so it is compared through the state hash"

patterns-established:
  - "hashes.txt exe lines are the frame line then the state line of each fixture, prefixed with exe and the file name"

requirements-completed: []

coverage:
  - id: D1
    description: "schwaystation-run --hash-state prints state sha256 and 64 lowercase hex digits over the state after its header, after the runner's other lines; a save failure exits 1; it is in --help and docs/runner.md"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^runner\\.(hash_state|hash|usage\\..*|exit_table)$' --no-tests=error"
        status: pass
      - kind: other
        ref: "build/ci/schwaystation-run --frames 5 --hash-state | grep -qE '^state sha256 [0-9a-f]{64}$'"
        status: pass
    human_judgment: false
  - id: D2
    description: "hashes.txt carries the frame and state hash of hello.exe, gte_vectors.exe, dma_chain.exe and timers_lines.exe at frame 30 on every lane, with the repository-check total unchanged at 1513 lines"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci, then test with grep -c of the exe state lines in build/ci/hashes.txt at least 4"
        status: pass
      - kind: other
        ref: "cmake --workflow --preset hygiene (policy.gate_budget at limit 1513)"
        status: pass
    human_judgment: false

duration: 19min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 19: Runner state hash and the four fixtures in hashes.txt Summary

**`--hash-state` hashes the saved machine state so the GTE, DMA and timers, invisible in a test-card frame, are compared across the eight lanes through the four fixtures' lines in `hashes.txt`, with the gate total still at its 1513-line limit.**

## Performance

- **Duration:** 19 min
- **Started:** 2026-11-10T07:42:00Z
- **Completed:** 2026-11-10T08:01:00Z
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments

- The runner prints `state sha256 <hex>` over the state bytes after the 20-byte header, after every other line it prints, with `schway_state_size` and `schway_save_state` and no core change. A failed size or save exits 1.
- `runner.hash_state` runs `--frames 5 --hash-state` twice and `--frames 6` once: each prints exactly one 64-digit line, the two runs of 5 frames agree, and frame 6 differs.
- `hashes.txt` now holds eight `exe` lines, the frame and the state of `hello.exe`, `gte_vectors.exe`, `dma_chain.exe` and `timers_lines.exe`. The four frame lines are equal (the card) and the four state lines differ, which is the point of the flag.
- The `hashes.cmake` block was folded into a loop of the same length as the block it replaced, so the gate total is 1513 before and after against a limit of 1513.

## Task Commits

1. **Task 1: --hash-state prints a stable state hash that differs when the state does** - `fb7365d` (feat)
2. **Task 2: state hashes of the four executables in hashes.txt, inside the gate budget** - `391a38a` (test)

**Plan metadata:** the docs commit that carries this summary.

## Files Created/Modified

- `runner/main.c` - the `--hash-state` option, `print_state_hash`, usage text and header comment
- `tests/runner/tests.cmake` - the `hash_state` case and the `runner.hash_state` registration
- `tests/runner/inventory.txt` - `runner.hash_state` with labels portable and runner
- `tests/cmake/hashes.cmake` - the loop over the four fixtures with both hash lines
- `docs/runner.md` - the option in the synopsis and options table, and why the state hash exists
- `docs/testing.md` - the `hashes.txt` sentence says `exe` lines

## Gate total

`cat scripts/*.sh tests/cmake/*.cmake .githooks/* | wc -l`: 1513 before, 1513 after; `policy.gate_budget` limit 1513, never raised.

## Decisions Made

See key-decisions above. The state hash is deliberately a runner feature built on the public save call, as the phase's decision asked, so no API version change was needed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The plan's two-line output check does not hold for dma_chain.exe**
- **Found during:** Task 2
- **Issue:** `dma_chain.exe` prints `unbuilt dev=gpu n=9 ...` before the hash lines (the program touches the GPU, which is not built), so a check for exactly two lines fails.
- **Fix:** the block matches the frame and state lines at the end of the output with a capture group and appends those two, keeping eight `exe` lines as the plan names. The unbuilt count is part of the saved state, so it is still compared through the state hash.
- **Files modified:** `tests/cmake/hashes.cmake`
- **Verification:** `cmake --workflow --preset ci` exits 0 and `build/ci/hashes.txt` has eight `exe` lines
- **Committed in:** `391a38a`

**2. [Rule 1 - Bug] CMake regex has no `{64}` repetition**
- **Found during:** Task 1
- **Issue:** the first draft of the `hash_state` case used `[0-9a-f]{64}`, which CMake's regex engine does not support, so a correct line failed the check.
- **Fix:** the digit class is repeated 64 times with `string(REPEAT)`.
- **Files modified:** `tests/runner/tests.cmake`
- **Verification:** `runner.hash_state` passes
- **Committed in:** `fb7365d`

**3. [Rule 2 - Missing] docs/testing.md said "the `exe` line" of hashes.txt**
- **Found during:** Task 2
- **Issue:** the file now has several `exe` lines per lane.
- **Fix:** one word changed, no line added.
- **Files modified:** `docs/testing.md`
- **Committed in:** `391a38a`

---

**Total deviations:** 3 auto-fixed (2 Rule 1, 1 Rule 2)
**Impact on plan:** none on scope or the gate budget.

## Issues Encountered

- The plan's `requirements-completed:` is empty on purpose: the plan states it, so GTE-01, GTE-02 and GTE-03 are not marked here. The requirements line that names `--hash-state` is a separate item for the phase's requirements step.
- The asan and be32 lanes need clang-21, which this host lacks, and the eight-lane equality of `hashes.txt` runs only on hosted CI. Neither was run here. Locally `cmake --workflow --preset ci` and `cmake --workflow --preset hygiene` exit 0.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plans that follow in the phase can rely on the state lines in `hashes.txt`. The hosted `hash-equality` run is the first time the four state lines are compared across lanes; a difference there is a determinism bug in a device, not a test bug.

## Self-Check: PASSED

- `runner/main.c`, `tests/cmake/hashes.cmake`, `tests/runner/tests.cmake`, `docs/runner.md`: found
- commits `fb7365d` and `391a38a`: on this branch
- `cmake --workflow --preset ci` exit 0; `cmake --workflow --preset hygiene` exit 0; the plan's grep and `wc -l` acceptance checks exit 0

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*
