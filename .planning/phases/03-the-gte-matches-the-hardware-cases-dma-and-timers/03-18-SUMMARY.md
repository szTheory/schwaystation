---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 18
subsystem: testing
tags:
  - fuzz
  - dma
  - timers
  - gte
  - armips
  - build

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: the DMA engine, the counters and the GTE the hostile programs drive (plans 07 to 15), the fixtures the digest comparison covers (plan 17 added the last)
provides:
  - CODE_DMA, CODE_TIMERS and CODE_GTE seed kinds in tests/fuzz/seeds.c and ten rows that fuzz.regress.exe replays on every lane
  - the pinned armips built with CMAKE_UNITY_BUILD=ON, every assembled fixture byte-identical
affects:
  - the nightly libFuzzer campaign, which now starts from the ten new seeds (--write emits them)
  - the CI build step, whose armips share shrinks

actuals:
  tokens: 1400
  tasks: 2
  commits: 2

plan_head_before: 5ab12e016a67dc0b30d2c4f4e9ed4ba99e4f3ff0
plan_head_after: ef610eafdbddb94f0ccb0ac5a7acbd21eb37ce1f
commits: 2

tech-stack:
  added: []
  patterns:
    - "a seed row's variant is its place among the rows of the same code kind, so a kind with several rows adds no field to the table"
    - "a hostile program ends in a spin on its own jump so a restarted setup never re-arms a busy channel"

key-files:
  created: []
  modified:
    - tests/fuzz/seeds.c
    - cmake/Armips.cmake

key-decisions:
  - "The three kinds take their variant from the row's index among rows of the same kind, in seeds_build, instead of a new struct field: every existing positional row would otherwise need a trailing initializer under -Werror=missing-field-initializers"
  - "The DMA, timer and GTE programs end in a jump to themselves, not back to the start, so the setup runs once and the channel, counter or coprocessor state is left to run for the rest of the two frames"
  - "The unity flag is its own list(APPEND) with a comment rather than part of the set() line, so the one-line reason sits next to it"

requirements-completed: []

coverage:
  - id: D1
    description: "tests/fuzz/seeds.c emits hostile DMA programs (a self-referencing list, lists into the scratchpad, the BIOS and unmapped space, OTC with BCR 0 from MADR FFFFFCh, OTC with every ignored bit set, BCR FFFFFFFFh on all seven channels), a timer storm and the GTE all-ones and every-command programs, and fuzz.regress.exe replays each within its two frames with every loader invariant intact"
    requirement: "GTE-01"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fuzz\\.regress\\.exe$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: "The pinned armips builds with CMAKE_UNITY_BUILD=ON and every one of the 24 fixtures keeps its SHA-256 after the fixtures are deleted and reassembled by the unity-built assembler"
    requirement: "GTE-03"
    verification:
      - kind: other
        ref: "shasum -a 256 build/ci/fixtures/* before and after, compared with cmp"
        status: pass
      - kind: integration
        ref: "ctest --test-dir build/ci -L '^fixtures$' --no-tests=error"
        status: pass
    human_judgment: false

duration: 22min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 18: Hostile device seeds and the armips unity build Summary

**Ten DMA, timer and GTE fuzz seeds replayed on every lane, and the pinned armips built as a unity build with all 24 fixture digests unchanged.**

## Performance

- **Duration:** 22 min
- **Started:** 2026-11-10T07:35:00Z
- **Completed:** 2026-11-10T07:57:00Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- `tests/fuzz/seeds.c` gains `CODE_DMA`, `CODE_TIMERS` and `CODE_GTE` with their emitters and ten rows: `dma_self_ref_list`, `dma_list_scratch`, `dma_list_bios`, `dma_list_unmapped`, `dma_otc_bcr_zero_wrap`, `dma_otc_ignored_bits`, `dma_bcr_all_ones`, `timer_storm`, `gte_all_ones`, `gte_every_command`. The replay went from 32 to 42 inputs and passes; no new target and no new CI job.
- Each program sets what it arms (all seven DMA enables, the three counters' target 0 with MODE 78h and `I_MASK` 70h and SR with IEc and IM2, or SR.CU2 with FFFFFFFFh by MTC2 and CTC2 to all 64 registers and 256 command words) and then spins, so the core runs the hostile state for the two frames. They run clean through `schwaystation-run --exe` as well.
- `cmake/Armips.cmake` adds `-DCMAKE_UNITY_BUILD=ON` to the assembler's arguments. All 24 fixtures were deleted and reassembled with the unity-built binary; the digests are identical.

## Task Commits

1. **Task 1: Tracer - DMA seed through the replay, then the rest of the DMA, timer and GTE rows** - `9c6483e` (test)
2. **Task 2: The armips unity build with unchanged fixture digests** - `ef610ea` (build)

**Plan metadata:** the docs commit that follows this file.

## Files Created/Modified

- `tests/fuzz/seeds.c` - three code kinds, `emit_sw`, `emit_spin`, `emit_dma`, `emit_timers`, `emit_gte`, ten rows, and a 512-word program buffer (the every-command seed is 325 words)
- `cmake/Armips.cmake` - the unity flag with a one-line reason

## Evidence

Run from the repository root, no `cd` chains. The local host is macOS arm64 with the Apple clang compiler.

| Command | Exit status |
|---|---|
| `cmake --workflow --preset ci` | 0 (334 tests passed, 2 release tests passed) |
| `ctest --test-dir build/ci -R '^fuzz\.regress\.exe$' --no-tests=error --output-on-failure` | 0 |
| `ctest --test-dir build/ci -L '^fixtures$' --no-tests=error` | 0 |
| `grep -q 'CMAKE_UNITY_BUILD=ON' cmake/Armips.cmake` | 0 |
| `shasum -a 256 build/ci/fixtures/* > fixtures-before.txt` (before any change, 24 fixtures including `timers_lines.exe`) | 0 |
| `rm -f build/ci/fixtures/*; cmake --build build/ci; shasum -a 256 build/ci/fixtures/* > fixtures-after.txt` | 0 |
| `cmp fixtures-before.txt fixtures-after.txt` | 0 |

The armips build, after `rm -rf build/ci/armips-ep build/ci/armips-build` and `cmake --build build/ci --target schwaystation-armips` on an 18-core host:

| Build | Translation units | CPU (user) | Wall |
|---|---|---|---|
| Before (no unity) | 61 steps | 34.2 s | 4.5 s |
| After (`CMAKE_UNITY_BUILD=ON`) | 10 steps | 10.5 s | 3.2 s |

The CPU figure is the comparable one: CI runners have fewer cores, and the plan's 87 s of 125 s on the macOS x64 step is a serial-ish figure. The eight-lane digest comparison in CI is the cross-platform confirmation; if MSVC cannot build the pin as a unity build, the fallback is `actions/cache` keyed on the armips pin, decided at ship.

## Decisions Made

See `key-decisions` above.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] A struct field for the variant did not compile**
- **Found during:** Task 1
- **Issue:** adding a trailing `variant` field to `struct seed` made every positional row of the existing table a `-Wmissing-field-initializers` error under `-Werror`.
- **Fix:** removed the field; `seeds_build` takes the variant as the row's place among rows of the same code kind.
- **Files modified:** tests/fuzz/seeds.c
- **Verification:** `fuzz.regress.exe` passes with 42 inputs.
- **Committed in:** 9c6483e

**2. [Rule 1 - Bug] `emit_spin` read and incremented its index in one expression**
- **Found during:** Task 1
- **Issue:** `w[n++] = op_j(start + 4u * n)` is `-Wunsequenced` under `-Werror`.
- **Fix:** index with `n` and `n + 1u` and return `n + 2u`.
- **Files modified:** tests/fuzz/seeds.c
- **Verification:** builds clean on this lane.
- **Committed in:** 9c6483e

---

**Total deviations:** 2 auto-fixed (1 blocking, 1 bug), both found by the compiler before the first commit.
**Impact on plan:** none; the addition is 115 lines against the plan's "about 60", because the emitters are shared helpers and the GTE every-command seed needs its loop.

## Issues Encountered

- The first "before" timing was taken after the plan's snapshot: the digest snapshot was taken at the start from the existing tree (24 fixtures, including `timers_lines.exe` from plan 17, as the dispatch note said), and the baseline armips time came from a clean rebuild of the same tree before the flag was added.
- The asan and be32 lanes need clang-21, which this host lacks; hosted CI runs them. Nothing here was faked: the seeds are plain integer code with no host dependence, but those two lanes are unconfirmed until CI.
- `requirements-completed` is empty as the plan's output spec says; the plan's `requirements` (GTE-01 to GTE-03) are not completed by a seed row and a build flag.

## Triage

No findings; no seeds dropped. Every new seed kept every invariant `exe.c` checks and returned within its two frames, so no core bug surfaced.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for the next plan in wave 7 or the phase's closing steps. At ship, the eight-lane fixture digest comparison confirms the unity build on every platform; MSVC is the one lane that might need the `actions/cache` fallback.

## Self-Check: PASSED

- FOUND: tests/fuzz/seeds.c, cmake/Armips.cmake
- FOUND commits: 9c6483e, ef610ea
