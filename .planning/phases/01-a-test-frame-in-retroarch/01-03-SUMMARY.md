---
phase: 01-a-test-frame-in-retroarch
plan: 03
subsystem: core, testing, docs
tags: [c17, ctest, counting-allocator, test-card, save-state, visitor, embedding-guide]

requires:
  - phase: 01-01
    provides: "core sources, state walk with RECORD mode, schway_add_test, per-directory inventories"
provides:
  - "tests/alloc.h counting allocator (call counts, last sizes, byte totals, fail_from)"
  - "schwaystation-test-core: 18 core.* cases (allocation, card, tuple, pitch, info, audio, profile, reset, API)"
  - "schwaystation-test-state: 5 core.state.* cases (round trip, stable bytes, rejects, visit, record sweep)"
  - "SCHWAY__STATE_HEADER_SIZE in src/state.h, shared by state.c and the tests"
  - "docs/embedding.md: embedding guide with a test-card section"
affects: [01-04, 01-05, 01-06, 01-07, 01-08, 01-14]

actuals:
  tokens: 18505
  tasks: 3
  commits: 3
plan_head_before: 9eb03f8ae6e06787938066b0da42e4a8ec31110d
plan_head_after: 887641ffcaad6196d6332292bd5904a00583a4a7

tech-stack:
  added: []
  patterns:
    - "Case-dispatch test programs: main(argc, argv) picks a case by name from a sorted table, CHECK_DONE gives the exit status"
    - "Expected values written from the documents; schway__testcard_pixel used only as a whole-frame oracle"
    - "In-test FNV-1a 64 hash over display words, low byte first; never the runner's SHA code"
    - "Refusal checks compare save bytes before and after the refused call"

key-files:
  created:
    - tests/alloc.h
    - tests/core/test_core.c
    - tests/core/test_state.c
    - docs/embedding.md
  modified:
    - tests/core/tests.cmake
    - tests/core/inventory.txt
    - src/state.h
    - src/state.c

key-decisions:
  - "The state header size is SCHWAY__STATE_HEADER_SIZE in src/state.h (moved from a file-local define in state.c), so the tests take the header length from the source instead of restating it"
  - "No other src/ change: every contract case passed against plan 01-01's implementation, and mutants of src/ were caught (see Verification)"
  - "The audio ring holds 4 x 887 = 3,548 pairs in both regions; five undrained frames overflow it in NTSC (about 3,687 pairs) and PAL (about 4,434 pairs), and the test asserts written 3,548 and dropped = due - 3,548"

requirements-completed: [FRAME-02, FRAME-09]

coverage:
  - id: D1
    description: "create makes exactly one allocation and destroy one free of the same size; nothing in between allocates; fail_from 1 gives ERR_NO_MEMORY with out unchanged and nothing leaked"
    requirement: FRAME-09
    verification:
      - kind: unit
        ref: "tests/core/tests.cmake#core.no_alloc_after_load"
        status: pass
    human_judgment: false
  - id: D2
    description: "Every card rule in first-match order at frames 1, 5 and 60, the counter strip (frame 5, 60 and 0x80000000), frames differ, every display word redrawn each frame"
    requirement: FRAME-02
    verification:
      - kind: unit
        ref: "tests/core/tests.cmake#core.testcard.pixels"
        status: pass
      - kind: unit
        ref: "tests/core/tests.cmake#core.testcard.counter"
        status: pass
      - kind: unit
        ref: "tests/core/tests.cmake#core.testcard.frames_differ"
        status: pass
      - kind: unit
        ref: "tests/core/tests.cmake#core.testcard.full_redraw"
        status: pass
    human_judgment: false
  - id: D3
    description: "Display tuple from the power-on range words (320 x 240, pitch 2048, divider 8) and equal hashes through 640- and 2048-byte pitches with padding untouched"
    requirement: FRAME-02
    verification:
      - kind: unit
        ref: "tests/core/tests.cmake#core.video.tuple"
        status: pass
      - kind: unit
        ref: "tests/core/tests.cmake#core.video.pitch"
        status: pass
    human_judgment: false
  - id: D4
    description: "Exact tick accumulator and audio pairs over 4,000 frames per region (566,203 or 566,204 and 680,823 or 680,824 cycles; 737 or 738 and 886 or 887 pairs; total = floor(ticks / 768)); ring overflow dropped and counted; zero-capacity drain"
    requirement: FRAME-09
    verification:
      - kind: unit
        ref: "tests/core/tests.cmake#core.audio.pairs"
        status: pass
      - kind: unit
        ref: "tests/core/tests.cmake#core.audio.ring"
        status: pass
      - kind: unit
        ref: "tests/core/tests.cmake#core.info"
        status: pass
    human_judgment: false
  - id: D5
    description: "Profile rejection and lock, power reset, size-tag rules, status text, refused calls change nothing, allocator hooks required, destroy(NULL) and run_frame(inst, NULL)"
    requirement: FRAME-09
    verification:
      - kind: unit
        ref: "tests/core/tests.cmake#core.profile.rejected"
        status: pass
      - kind: unit
        ref: "tests/core/tests.cmake#core.profile.locked"
        status: pass
      - kind: unit
        ref: "tests/core/tests.cmake#core.reset.power"
        status: pass
      - kind: unit
        ref: "tests/core/tests.cmake#core.api.size_tags"
        status: pass
      - kind: unit
        ref: "tests/core/tests.cmake#core.api.statuses"
        status: pass
      - kind: unit
        ref: "tests/core/tests.cmake#core.api.error_changes_nothing"
        status: pass
      - kind: unit
        ref: "tests/core/tests.cmake#core.create.allocator"
        status: pass
    human_judgment: false
  - id: D6
    description: "Two instances stepped interleaved give the same frame hashes and state bytes as each stepped alone"
    requirement: FRAME-02
    verification:
      - kind: unit
        ref: "tests/core/tests.cmake#core.instances.independent"
        status: pass
    human_judgment: false
  - id: D7
    description: "State round trip, save-load-save stable bytes, each corruption refused with its status before applying, visitor serialisation equals the saved body, record sweep refuses every bounded field"
    requirement: FRAME-09
    verification:
      - kind: unit
        ref: "tests/core/tests.cmake#core.state.round_trip"
        status: pass
      - kind: unit
        ref: "tests/core/tests.cmake#core.state.stable_bytes"
        status: pass
      - kind: unit
        ref: "tests/core/tests.cmake#core.state.rejects"
        status: pass
      - kind: unit
        ref: "tests/core/tests.cmake#core.state.visit"
        status: pass
      - kind: unit
        ref: "tests/core/tests.cmake#core.state.record_sweep"
        status: pass
    human_judgment: false
  - id: D8
    description: "docs/embedding.md explains the API lifecycle and has a test-card section on what each band catches and how to read the counter"
    requirement: FRAME-09
    verification:
      - kind: other
        ref: "grep -c -i counter docs/embedding.md prints 2; wc -c docs/embedding.md under 61440; planning-ID grep count 0; agent read the guide against src/instance.c, src/frame.c and src/state.c"
        status: pass
    human_judgment: false
  - id: D9
    description: "No planning ID in src, tests/core or tests/alloc.h; no seed name anywhere outside the excluded files"
    verification:
      - kind: other
        ref: "git grep --untracked planning-ID scan over src tests/core tests/alloc.h; test $? -eq 1 exit 0; cruft grep a=1 b=1 exit 0"
        status: pass
    human_judgment: false

duration: 9min
completed: 2026-10-20
status: complete
---

# Phase 1 Plan 03: Core contract tests and the embedding guide Summary

**23 core unit cases in two programs (counting-allocator proof, the test card rule by rule, display tuple and pitch, the exact cycle and audio-pair accumulator over 4,000 frames per region, API size-tag and refusal contracts, two-instance independence, state round trip, refusals and visitor byte equality), plus docs/embedding.md with a test-card section.**

## Performance

- **Duration:** 9 min
- **Started:** 2026-10-20T03:10:05Z
- **Completed:** 2026-10-20T03:19:25Z
- **Tasks:** 3
- **Files modified:** 8 (4 created, 4 modified)

## Accomplishments

- `tests/alloc.h` and `core.no_alloc_after_load`: create makes 1 allocation, 120 frames and every get/set/reset/save/load/visit call make none, destroy makes 1 free of the same size, and `fail_from = 1` gives `ERR_NO_MEMORY` with `*out` unchanged and byte totals equal.
- `schwaystation-test-core`: 17 more `core.*` cases covering every card band in first-match order, the counter (including frame 0x80000000 set through `src/internal.h`), the full redraw against a sentinel, the D-04 tuple and a range-word change to 256 x 224, 640/2048 pitch copies, info per region, the accumulator and pairs, the ring, profile, reset, size tags (inputs at 0, first - 1, 4097, 0xFFFFFFFF, non-zero extra; outputs at 0, first - 1, ours, ours + 64, 8192), statuses, refused calls and two instances.
- `schwaystation-test-state`: round trip and stable bytes in NTSC-U and PAL, rejection of magic, short buffer, format, size word, one byte short, body byte, CRC word and both other regions (resealed), the visitor's own little-endian stream compared with the save body by `memcmp`, stable name pointers, and a record sweep that refuses 5 body and 20 header values plus a remainder equal to the NTSC denominator.
- `docs/embedding.md` (10 KB): getting it, size tags, the allocator, running, video, audio, save states, statuses, versions and "The test card".

## Task Commits

1. **Task 1: Tracer - counting allocator through every API call** - `2750076` (test)
2. **Task 2: Card, scan-out, audio, info, profile, reset and API contract cases** - `80c42df` (test)
3. **Task 3: State round trip, rejection and visitor, and docs/embedding.md** - `887641f` (test)

## Files Created/Modified

- `tests/alloc.h` - counting allocator seeded from muhtari d698364b180d (MIT, same author); the commit body carries the Seeded from line
- `tests/core/test_core.c` - 18 core cases, case dispatch on argv[1]
- `tests/core/test_state.c` - 5 state cases
- `tests/core/tests.cmake` - both executables and 23 registrations (labels core,portable; perf,portable for no_alloc_after_load)
- `tests/core/inventory.txt` - 23 sorted lines
- `src/state.h` - `SCHWAY__STATE_HEADER_SIZE`
- `src/state.c` - uses it instead of a file-local define
- `docs/embedding.md` - the embedding guide

## Verification

- `cmake --workflow --preset ci` exit 0 (24 tests, 100% passed)
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake` exit 0 (24 tests ran once each)
- `ctest --test-dir build/ci -L portable` exit 0; `ctest --test-dir build/ci -L core --output-on-failure` exit 0
- Cruft grep (plan task 3 form, `git grep --untracked -nIi -e cogmaster -e muhtari -e 'master system' -e 7800 -e palette ...` and `-nIiw -e sms`): a=1 b=1, exit 0
- Planning-ID scan `git grep --untracked -nE -e 'D-[0-9]{2}' -e 'FRAME-[0-9]' -e 'XFER-[0-9]' -e 'DEC\.[0-9]' -- src tests/core tests/alloc.h; test $? -eq 1` exit 0
- `grep -E -c 'float|double'` over tests/core/*.c src/*.c src/*.h: 0 in every file
- Mutation check run once by the agent (each mutant built, run, then restored): counter LSB-left caught by core.testcard.counter; mask band one row short by core.testcard.pixels; accumulator remainder dropped by core.audio.pairs; uncounted drops by core.audio.ring; X2 changed by core.video.tuple; extra-byte check disabled by core.api.size_tags; a stale card by four testcard/video cases; VALIDATE skipped by core.state.rejects and record_sweep; visitor empty or one value short by core.state.visit; region bound widened by core.state.rejects; tick_acc bound off by one by core.state.record_sweep (after the fix below).

## Decisions Made

- The state header length lives in `src/state.h` as `SCHWAY__STATE_HEADER_SIZE`, which the plan's "the header length comes from src/state.h" needed; it is a relocated macro, not a new function or public name.
- Expected values come from the card rules, ARCHITECTURE sections 1 and 4 and the psx-spx display defaults; `schway__testcard_pixel` is the oracle only for whole-frame equality.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Test overfilled the audio ring in api.error_changes_nothing**
- **Found during:** Task 2
- **Issue:** the case ran 7 undrained frames (about 5,160 pairs) and expected them all queued, but the ring holds 3,548 pairs.
- **Fix:** run 3 frames, fewer pairs than the ring holds.
- **Files modified:** tests/core/test_core.c
- **Commit:** 80c42df

**2. [Rule 2 - Missing check] Record sweep trusted the recorded bound**
- **Found during:** Task 3 mutation check
- **Issue:** widening the `instance.tick_acc` bound to `den` went undetected, because the sweep writes `max + 1` from the record itself.
- **Fix:** an independent check that a remainder of 715,909 (the NTSC denominator, ARCHITECTURE section 1) is refused and 715,908 loads.
- **Files modified:** tests/core/test_state.c
- **Commit:** 887641f

**3. [Process] Mutation helper reverted an uncommitted edit**
- **Found during:** Task 3
- **Issue:** the helper restored mutants with `git checkout -- src/state.c`, which also discarded the uncommitted header-size move.
- **Fix:** reapplied the edit, backed it up to the scratchpad, and restored later mutants from that backup. Nothing was committed in the bad state.

**Total deviations:** 2 auto-fixed (1 test bug, 1 missing check) and 1 process slip. **Impact:** none on scope; no `src/` behaviour change was needed.

## Issues Encountered

- The first mutant for the extra-byte check did not compile (unused parameter under `-Werror`); it was replaced by one that compiles, which core.api.size_tags caught.

## Known Stubs

None. Audio is silent by design in this phase (docs/embedding.md says so); the pair counts are real.

## Next Phase Readiness

- Ready for 01-04. `ctest -L portable` now has 23 core cases for the be32 and wasm32 lanes in plan 01-14's hosted run. On this host core.audio.pairs takes 0.9 s, inside its 10 s timeout times the lane scale.
- `ci-msvc` was not built here (no MSVC on this host); the test sources avoid VLAs and floating point and cast every narrowing, but MSVC `/W4` is first checked on the hosted lane.

## Self-Check: PASSED

- FOUND: tests/alloc.h, tests/core/test_core.c, tests/core/test_state.c, docs/embedding.md
- FOUND: 2750076, 80c42df, 887641f (ancestors of HEAD)
