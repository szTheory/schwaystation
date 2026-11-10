---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 13
subsystem: testing
tags: [gte, reference-model, differential-test, property-test, psx-spx, splitmix64]

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: the engine's four public declarations (src/gte.h), the shared hardware-case replay (tests/gte/vectors.h) and the seeded generator (tests/prop.h)
provides:
  - an independent straight-C GTE model (tests/gte/ref.h, tests/gte/ref.c) that passes all 1,150 hardware cases on its own
  - gte.ref.vectors, gte.ref.property and gte.ref.property.self_test
affects: [phase 3 verification, any later change to src/gte.c, GTE-01]

commits: 3
plan_head_before: 16b1c3ae8405f8189ff29200be472998bf1fc483
plan_head_after: 2c638701a4acc6a38fac130d625e37bb16c3f2ca

actuals:
  tokens: 11300
  tasks: 3
  commits: 3

tech-stack:
  added: []
  patterns:
    - "an independent model written from the public hardware description, anchored to hardware data first, then compared with the shipped engine over seeded edge-biased inputs"
    - "a negative control that flips one engine-side bit and must fail through expect_fail.cmake"

key-files:
  created:
    - tests/gte/ref.h
    - tests/gte/ref.c
    - tests/gte/ref_tests.c
  modified:
    - tests/gte/gte_tests.c
    - tests/gte/tests.cmake
    - tests/gte/inventory.txt

key-decisions:
  - "The model keeps SZ3 and the IR3 flag on the whole 44-bit sum shifted right by 12, while IR1 to IR3 saturate the 32-bit MAC: the hardware cases pin the first (a sum whose low 32 bits are positive gives SZ3 = 0), psx-spx's IR = MAC pins the second"
  - "The property test also compares the cycle count of every command against the psx-spx opcode table, beyond the 64 registers the plan asked for"
  - "One case in four draws small 16-bit halves instead of edge words, so the unsaturated paths of long sums run; three in four follow the plan's edge-biased draw"
  - "All of bits 6 to 24 of the command word are drawn at random (sf, lm, the matrix, vector and translation fields, the fake number and the bits psx-spx calls zero and ignored), so the test also proves the engine ignores what the hardware ignores"

patterns-established:
  - "Independence is kept by construction: ref.c includes neither src/ headers nor shares a helper or table with the engine, and an acceptance grep keeps it so"

requirements-completed: [GTE-01]

coverage:
  - id: D1
    description: "The independent model reproduces the 50 register-only and all 1,150 hardware cases, anchoring it to hardware data before it is compared with the engine"
    requirement: GTE-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^gte\\.ref\\.vectors$' --no-tests=error"
        status: pass
    human_judgment: false
  - id: D2
    description: "A differential test of 22,000 seeded edge-biased cases (1,000 per command) compares the engine and the model on all 64 registers, FLAG included, and on the cycles, and fails unless every case ran"
    requirement: GTE-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^gte\\.ref\\.property$' --no-tests=error"
        status: pass
    human_judgment: false
  - id: D3
    description: "The negative control flips one engine-side output bit and the property test must exit 1 naming the difference"
    requirement: GTE-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^gte\\.ref\\.property\\.self_test$' --no-tests=error"
        status: pass
    human_judgment: false
  - id: D4
    description: "The model carries the quirks the hardware table pins thinly: far-colour MVMVA, the fourth matrix, saturation at the end of a sum, IR3 under lm and IR0 on RTPT's last vertex"
    requirement: GTE-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^gte\\.ref\\.(vectors|property)$' --no-tests=error"
        status: pass
    human_judgment: false
  - id: D5
    description: "Rest of GTE-01 on the shipped engine: the 1,150 hardware cases, the reciprocal-table formula test, the fixture on the real CPU path and the cycle and stall rules, all run by the ci workflow"
    requirement: GTE-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -L '^gte$' --no-tests=error; ctest --test-dir build/ci -R 'gte_vectors|cpu.timing.gte' --no-tests=error"
        status: pass
    human_judgment: false
  - id: D6
    description: "The notice for the third-party vector data is enforced by the manifest-driven policy gate"
    requirement: GTE-01
    verification:
      - kind: integration
        ref: "cmake --workflow --preset hygiene (policy.notices and policy.notices.self_test)"
        status: pass
    human_judgment: false

duration: 12min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 13: The independent GTE reference and its property test Summary

**A plain-integer GTE model written from psx-spx alone passes all 1,150 hardware cases and agrees with the engine on 22,000 seeded edge-biased cases, all 64 registers and the cycles included, with a flipped-bit negative control.**

## Performance

- **Duration:** 12 min
- **Started:** 2026-11-10T06:58:00Z
- **Completed:** 2026-11-10T07:11:00Z
- **Tasks:** 3
- **Files modified:** 6 (3 created, 3 modified)

## Accomplishments

- `tests/gte/ref.c` models the register file (every format and read rule: SXYP move-on-write, IRGB and ORGB, LZCR, the H sign bug, FLAG bit 31 derived on read), the divide from its formula (no table), and all 22 commands, one function each with its own arithmetic. It includes no header from `src/` and shares nothing with `src/gte.c`.
- The model replays all 1,150 hardware cases through the shared harness (`gte.ref.vectors`, `--expect 1150`), so the later differential test compares two models that both match hardware data.
- `gte.ref.property` runs 1,000 cases of each of the 22 commands from the printed seed 0x6774652D70726F70 (splitmix64 from `tests/prop.h`), writes the same 64 words to both sides in register order, runs the same command word, compares all 64 registers and the cycle count, and exits 1 unless 22,000 cases ran. 19,323 of the 22,000 set at least one flag, so the saturation paths run constantly.
- `gte.ref.property.self_test` flips one bit of IR0 on the engine's side in case 12,345 and must exit 1 with the difference named.
- The model reaches the engine only through `schway__gte_write`, `schway__gte_exec` and `schway__gte_read`. `src/gte.c` was never opened, so the seed-fix commit the plan reserves was not needed.

## Task Commits

1. **Task 1: tracer, the register file and RTPS through the shared replay** - `4b2e3aa` (test)
2. **Task 2: every command in the model, against all 1,150 cases** - `a8d23e6` (test)
3. **Task 3: the differential property test and its negative control** - `2c63870` (test)

**Plan metadata:** recorded in the docs commit that follows this file.

## Files Created/Modified

- `tests/gte/ref.h` - `struct gte_ref` (plain integer fields) and the three entry points plus the two case entry points
- `tests/gte/ref.c` - the register file, the 44-bit and 32-bit accumulator rules, the divide, the 22 commands and the dispatch
- `tests/gte/ref_tests.c` - `ref_vectors` (replay with `--expect` and `--ops`) and `property [--mutate]`
- `tests/gte/gte_tests.c` - two dispatch lines and the usage text
- `tests/gte/tests.cmake` - sources and the three registrations with their fault comments
- `tests/gte/inventory.txt` - the three names, sorted

## Decisions Made

See `key-decisions` above. All three numerical readings that psx-spx leaves open were settled by hardware cases or by the page's own formulas and none by looking at the engine.

## Settled differences between the model and the engine

None. The first full property run matched the engine on every case. The only correction made while writing the model came from the hardware cases, before any comparison with the engine: RTPS cases 83 and 87 (and 71, 78, 89, which followed from them) showed SZ3 taking the whole 44-bit sum shifted right by 12, not the 32-bit MAC3, and the model was changed to that (the table's case 83 has MAC3 = 1376424Eh, positive, and SZ3 = 0 with flag 18 set).

Because a first-run agreement can mean the comparison is blind, the model was mutated ten ways in a scratch copy (no wrap of the 44-bit accumulator, the depth-cue step on every RTPT vertex, SX from the low 32 bits, OTZ from the low 32 bits, the blend's IR saturating with lm set, the far-colour first-term flag, the fourth matrix's third row, RTPS's IR3 ignoring lm, H read unsigned, GPL without the shift). Nine failed the property test at once, naming the command and register; the tenth did not compile (an unused variable) and was not retried. The scratch edits were reverted and are not in any commit.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The model's own SZ3 and IR3-flag source was wrong**
- **Found during:** Task 1 (the 100-case replay of register-only and RTPS cases)
- **Issue:** Took SZ3 and the IR3 flag from the 32-bit MAC3, the literal reading of psx-spx's "MAC3 SAR ((1-sf)*12)"; five RTPS cases failed.
- **Fix:** Take them from the whole 44-bit sum shifted right by 12, as the cases show.
- **Files modified:** tests/gte/ref.c
- **Verification:** `gte.ref.vectors` 100 of 100, then 1,150 of 1,150
- **Committed in:** 4b2e3aa (the Task 1 commit; the wrong version was never committed)

### Additions beyond the plan text (Rule 2, strictly widening the comparison)

- The property test also compares the returned cycle count against the psx-spx table (a wider check; it passed).
- One case in four draws small 16-bit halves, and bits 6 to 24 of the command word are all random; the plan asked for sf, lm, the MVMVA fields and the fake bits.

### Reading of the plan

- `gte_tests.c` was read for the dispatch, and also for the existing `vectors` case's argument parsing (lines 34 to 183), to mirror its `--expect` and `--ops` syntax. None of the GTE cases, `src/gte.c`, ares, the notice section, `docs/hardware/gte.md`, preparation section 5 or earlier GTE SUMMARYs were opened.
- psx-spx was fetched with `curl` from the page the plan names and converted to text in the session scratch directory; nothing from it is committed.

**Total deviations:** 1 auto-fixed (Rule 1, in an uncommitted draft), 2 widenings.
**Impact on plan:** None on scope; the comparison is wider than specified, never narrower.

## Issues Encountered

None. A `-Wconversion`-strict build passed first time; the `ci` workflow ends with 317 tests passing (315 plus the two release tests), the junit inventory check reports 317 ran once each, and the `hygiene` workflow passes 23 of 23.

## Not run locally

The `asan` and `be32` lanes need clang-21 and an emulator this host lacks; hosted CI runs them. The model is integer-only, avoids signed shifts and divides by powers of two, uses a 64-bit stream and has no host dependence, so the same 22,000 cases are expected on every lane. The `wasm32` lane was not run locally either.

## Known Stubs

None.

## Threat Flags

None. The change adds test code only.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

GTE-01 is complete: the 1,150 cases, the reciprocal-table test, the fixture's rows, the timing tests and now the property test against an independent model all pass. The next plan in the wave order can proceed.

## Self-Check: PASSED

- tests/gte/ref.h, tests/gte/ref.c, tests/gte/ref_tests.c exist.
- Commits 4b2e3aa, a8d23e6 and 2c63870 are ancestors of HEAD; `git rev-list --count 16b1c3a..HEAD` before this file was 3.
- Acceptance greps: `gte_ref_exec` present in ref.c; no `schway__gte`, `unr_table` or `sar64` in ref.c; `ref_vectors ... --expect 1150` in tests.cmake; the 22,000 count asserted in ref_tests.c.

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*
