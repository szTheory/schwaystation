---
phase: 02-the-cpu-matches-the-public-vectors
plan: 11
subsystem: api
tags: [api-v2, bounded-run, peek, first-fault, frame-end, api-names, embedding-guide]

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: API version 2, the PS-EXE probe and loader, profile flags (plan 02-08)
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: the first-fault record and its internal enums, the bus (plans 02-03, 02-07)
provides:
  - schway_run with a cycle budget, stops 2, 4 and 5, SCHWAY_RUN_STOP_ON_FAULT, and exit_code and fault_present on the run result (size 32, first size 24)
  - instance.frame_end in the state, so a run can stop mid-frame and a mid-frame state resumes
  - schway_peek for RAM and the scratchpad, side-effect free
  - schway_fault and schway_get_fault with ten kinds and eighteen devices pinned to the internal enums
  - docs.api_names and its self test, and an embedding guide that names every public name
affects: [02-12, 02-13, 02-14, 02-17, 02-19, 02-25, 02-26, 02-27]

actuals:
  tokens: 14200
  tasks: 3
  commits: 3

plan_head_before: e620e077e7e5cbf6c828dd6876be4ac2ef681ff6
plan_head_after: c99ef9965cd9b4cbdcc2458272177444563e62dc
commits: 3

tech-stack:
  added: []
  patterns:
    - "one static advance(inst, limit) serves schway_run and schway_run_frame: cycles = min(limit, frame_end - ticks), refusal before any write, the frame completes only on reaching frame_end"
    - "a cross-field state rule reads the decoded values the walk returns (ticks, frame_end) and calls schway__state_check, so VALIDATE refuses a hostile frame end before APPLY"
    - "a public enum is asserted equal to its internal enum with _Static_assert, one line per name"
    - "a repository check is run on a header copy with one extra name, through expect_fail.cmake, to prove it fails"

key-files:
  created:
    - tests/core/test_run.c
    - tests/cmake/api_names.cmake
  modified:
    - include/schwaystation.h
    - src/internal.h
    - src/frame.c
    - src/instance.c
    - src/state.c
    - tests/api/v2.c
    - tests/core/test_state.c
    - tests/core/tests.cmake
    - tests/core/inventory.txt
    - tests/policy/tests.cmake
    - tests/policy/inventory.txt
    - docs/embedding.md

key-decisions:
  - "A call to schway_run never crosses a frame boundary, and a budget that ends exactly on the boundary reports FRAME; the split property is therefore tested by looping calls until the ticks have moved N, which is what a host does too"
  - "instance.frame_end is a state field with the range 0 to 2^62 plus the longest frame (680,824 cycles for PAL), and VALIDATE also requires ticks <= frame_end and frame_end - ticks <= the region's longest frame, so a run from a hostile state is bounded to one frame of work"
  - "tick_acc keeps its meaning (the accumulator's remainder at the start of the frame); completing a frame sets tick_acc to (tick_acc + num) % den and adds (new tick_acc + num) / den to frame_end"
  - "An advance whose end tick would pass 2^62, or that completes a frame at the frame limit, returns ERR_STATE_FIELD with nothing changed; a budget that stops short of a frame_end beyond the limit still runs"
  - "max_cycles 0 changes nothing, the profile lock included; any positive advance locks the profile, because a run can now stop inside the first frame"
  - "With SCHWAY_RUN_STOP_ON_FAULT set and a fault already latched, a run returns STOP_FAULT before any cycle; stop 3 and 6 stay undefined and exit_code is 0 until a program can run"
  - "schway_set_profile recomputes frame_end while ticks is 0, because power-on fixed it for the NTSC-U default"
  - "docs.api_names compares only schway_ and SCHWAY_ names; SCHWAYSTATION_ build names do not match, so the seed's OTHER file list was dropped"

patterns-established:
  - "Property tests loop schway_run until the result's ticks moved the requested cycles"
  - "Gate total after this plan: 1,457 lines against the 1,520 limit (1,392 before the port)"

requirements-completed: [CPU-04, CPU-05]

coverage:
  - id: D1
    description: "schway_run stops at max_cycles with BUDGET or at the frame boundary with FRAME, max_cycles 0 changes nothing, a stopped run resumes to the same state as run_frame, size tags and flags are checked before any work, and the counter limits refuse with nothing changed"
    requirement: CPU-04
    verification:
      - kind: unit
        ref: "tests/core/test_run.c#budget_resume (ctest core.run.budget_resume)"
        status: pass
      - kind: integration
        ref: "tests/api/v2.c (ctest abi.api_v2)"
        status: pass
    human_judgment: false
  - id: D2
    description: "With no program loaded, running N cycles then M leaves the same state bytes, frame, ticks and audio pairs as N + M cycles, including splits on a frame boundary, on 767, 768 and 769 cycles and with ring drops, in all three regions"
    requirement: CPU-04
    verification:
      - kind: unit
        ref: "tests/core/test_run.c#split_equivalence (ctest core.run.split_equivalence)"
        status: pass
    human_judgment: false
  - id: D3
    description: "schway_peek reads RAM and the scratchpad little-endian at widths 1, 2 and 4, refuses every other space, width, alignment and range with ERR_ARGUMENT and the value untouched, and peeking every address leaves the whole instance byte for byte as it was"
    requirement: CPU-05
    verification:
      - kind: unit
        ref: "tests/core/test_run.c#peek_values, #peek_rejects, #peek_no_side_effects (ctest core.run.peek_values, core.run.peek_rejects, core.run.peek_no_side_effects)"
        status: pass
    human_judgment: false
  - id: D4
    description: "schway_get_fault returns the latched first-fault record with every field, a zero record with its size tag when none is latched, the run result's fault_present follows it, STOP_ON_FAULT stops before any cycle, and the record survives a state round trip and clears on a power reset"
    requirement: CPU-05
    verification:
      - kind: unit
        ref: "tests/core/test_run.c#fault_none, #fault_latched (ctest core.run.fault_none, core.run.fault_latched)"
        status: pass
    human_judgment: false
  - id: D5
    description: "The frame's end is in the state: a mid-frame state resumes, and a state whose frame end is before its ticks or more than one frame past them, or beyond the counter limit, is refused with nothing changed"
    requirement: CPU-04
    verification:
      - kind: unit
        ref: "tests/core/test_run.c#budget_resume (ctest core.run.budget_resume)"
        status: pass
      - kind: unit
        ref: "tests/core/test_state.c#run_bounds (ctest core.state.run_bounds)"
        status: pass
    human_judgment: false
  - id: D6
    description: "Every new public name is in tests/api/v2.c, the fault kinds and devices equal the internal enums, and the struct offsets and first sizes are pinned"
    requirement: CPU-05
    verification:
      - kind: integration
        ref: "ctest -R '^(abi.api_v2|abi.api_version|abi.symbols)$'"
        status: pass
    human_judgment: false
  - id: D7
    description: "docs.api_names fails when a header name is missing from docs/embedding.md or the guide names one the header lacks, its self test fails a header copy with one extra function, and the guide describes bounded runs, peek, the fault record, PS-EXE loading and the profile flags"
    requirement: CPU-04
    verification:
      - kind: integration
        ref: "ctest -R '^docs.api_names(.self_test)?$'"
        status: pass
    human_judgment: false

duration: 10min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 11: Bounded run, peek, first-fault record and the public-names check Summary

**schway_run stops at a cycle budget or the frame boundary and resumes exactly, with the frame's end in the state, plus a side-effect-free peek, the public first-fault record and docs.api_names holding the guide to the header.**

## Performance

- **Duration:** 10 min (the start time was not recorded; measured from the previous plan's last commit)
- **Started:** 2026-11-09T20:40:51Z
- **Completed:** 2026-11-09T20:50:52Z
- **Tasks:** 3
- **Files modified:** 14

## Accomplishments

- `schway_run(inst, &budget, &result)` advances to the frame boundary (FRAME) or `max_cycles` (BUDGET); `schway_run_frame` is the same advance with no limit. The result gains `exit_code` and `fault_present` (size 32, first size 24), stops 2, 4 and 5 exist, 3 and 6 do not.
- The frame's end is a state field. A run can stop mid-frame, a mid-frame state saves and resumes, and a hostile state cannot push a run past one frame of work (AGENTS rule 4). N cycles then M equal N + M for the state bytes, frame, ticks and audio pairs, in all three regions.
- `schway_peek` reads RAM and the scratchpad at widths 1, 2 and 4; a sweep of every address at every width leaves the whole `struct schway` byte for byte unchanged.
- `schway_fault` (56 bytes) and `schway_get_fault` expose the latched record; the ten kinds and eighteen devices are asserted equal to the internal enums.
- `docs.api_names` (ported from the cogmaster seed, 55 lines) and its self test; `docs/embedding.md` now names all 92 public names and gains sections on bounded runs, peeking, the first fault, loading a PS-EXE and the profile flags.
- Gate total after the port: 1,457 lines against the 1,520 limit (measured with the same patterns as `policy.gate_budget`; it was 1,392). This plan closes todo 008; plan 02-27 alone moves the todo file. Plan 02-26 measures the phase's final total and alone decides any deferral.

## Task Commits

1. **Task 1 (tracer): schway_run stops at a cycle budget and resumes to the frame boundary** - `bf85f8c` (feat)
2. **Task 2: split equivalence, schway_peek and schway_get_fault** - `16126c7` (feat)
3. **Task 3: docs.api_names and a guide that names every public name** - `c99ef99` (test; the commit body carries `Seeded from cogmaster f09ac97096ac tests/cmake/api_names.cmake (same author, MIT).`)

**Plan metadata:** the docs commit that carries this summary, STATE.md and ROADMAP.md.

## Files Created/Modified

- `include/schwaystation.h` - stops, `schway_budget`, spaces, fault kinds and devices, `schway_fault`, `schway_run`, `schway_peek`, `schway_get_fault`
- `src/frame.c` - the static `advance`, `schway_run`, the shared result writer
- `src/state.c`, `src/internal.h` - `instance.frame_end` and its cross-field rules, layout pins, `schway__instance_frame_max`
- `src/instance.c` - power-on and profile set the first frame's end; `schway_peek`, `schway_get_fault`
- `tests/core/test_run.c` - seven `core.run.*` cases
- `tests/core/test_state.c` - `run_bounds` sets the frame end it now has to craft
- `tests/api/v2.c` - every new name, constant and field
- `tests/cmake/api_names.cmake`, `tests/policy/tests.cmake`, `tests/policy/inventory.txt` - the names check, registration and negative control
- `docs/embedding.md` - the new sections

## Decisions Made

See `key-decisions` above. The two that later plans lean on: a call never crosses a frame boundary (plan 02-17's CPU loop runs up to `frame_end` inside the same advance), and `frame_end` is validated against the region's longest frame, so the CPU loop's per-call work stays bounded from any loadable state.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] tests/core/test_state.c edited although the plan does not list it**
- **Found during:** Task 1 (first full ctest after adding the state field)
- **Issue:** `core.state.run_bounds` writes crafted frame, ticks and accumulator values into a saved state; with `instance.frame_end` in the state those states fail the new `ticks <= frame_end` rule on load.
- **Fix:** the test now looks up `instance.frame_end` through the record walk and writes `ticks + step` beside the other fields. Its expectations (which crafted counters advance, which are refused) are unchanged and still pass.
- **Files modified:** tests/core/test_state.c
- **Verification:** `ctest -R core.state.run_bounds` passes
- **Committed in:** bf85f8c

**2. [Rule 2 - Missing critical functionality] frame_end is bounded in the state walk, and set_profile refreshes it**
- **Found during:** Task 1 (design of the state field)
- **Issue:** a state field read as the advance target would let a loaded state with `frame_end` near 2^62 make one `schway_run_frame` do unbounded work (a loop over silent pairs), against AGENTS rule 4; a power-on `frame_end` fixed for NTSC-U would also be wrong after `schway_set_profile` picks another region.
- **Fix:** VALIDATE requires `ticks <= frame_end <= ticks + the region's longest frame` and the field's range ends at 2^62 plus that length so the first frame past the limit still saves and loads; `schway_set_profile` recomputes `frame_end` while `ticks` is 0. Added `schway__instance_frame_max`.
- **Files modified:** src/state.c, src/internal.h, src/instance.c
- **Verification:** `core.run.budget_resume` loads states with `frame_end` at ticks - 1, ticks + 680,825 (refused) and ticks, ticks + 680,824 (accepted), and runs to the counter limit
- **Committed in:** bf85f8c

---

**Total deviations:** 2 auto-fixed (1 blocking, 1 missing critical)
**Impact on plan:** both follow from the plan's own `frame_end` state field; no scope beyond it.

## Issues Encountered

- The `asan` and `nofp` workflows cannot configure on this host (no `clang-21`, and `-mgeneral-regs-only` needs GCC); they run in CI. The `ci` and `hygiene` workflows pass locally (138 and 21 tests).
- One read-only `git stash list` ran by mistake while checking the tree; it changed nothing and no other stash command was run.

## Known Stubs

- `schway_run_result.exit_code` is always 0 and `SCHWAY_STOP_HOOK` is returned by nothing: no program can run yet. The guide says so ("reserved: nothing returns it yet"); plan 02-17 makes the exit port set both. Not recorded in `.planning/WINDOWS.md`, which does not exist.

## Threat Flags

None - no new network, auth or file surface; the new state field is range-checked in VALIDATE before it is applied.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 02-17 can run the CPU up to `frame_end` inside `advance` and report HOOK and `exit_code`; `SCHWAY_RUN_STOP_ON_FAULT` is already checked at the top of `schway_run` and needs a per-instruction check there.
- Any later public name needs a line in `docs/embedding.md` or `docs.api_names` fails.

## Self-Check: PASSED

- FOUND: tests/core/test_run.c, tests/cmake/api_names.cmake
- FOUND commits: bf85f8c, 16126c7, c99ef99 (`git merge-base --is-ancestor` each)
- `cmake --workflow --preset ci` exit 0 (138 tests); `cmake --workflow --preset hygiene` exit 0 (21 tests)

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*
