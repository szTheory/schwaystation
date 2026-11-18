---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 05
subsystem: gte
tags:
  - gte
  - seed
  - registers
  - vectors
  - notices

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: the GTE module skeleton and state walk (plan 01), the transcoded 1,150-case container and policy.notices (plan 02)
provides:
  - src/gte.c seeded from ares interpreter-gte.cpp with the full register file read and write rules and the seed's SPDX header
  - tests/gte/vectors.h, a container reader and fixed-order replay through caller-supplied write, exec and read functions
  - schwaystation-test-gte with the vectors case and the regs_sweep case, registered as gte.vectors.part1 and gte.regs.sweep
  - the THIRD-PARTY-NOTICES section "ares GTE seed" and the PROVENANCE section 4 GTE seed row
  - docs/hardware/gte.md, the first GTE page
affects:
  - 03-26
  - 03-07
  - 03-13
  - phase-3 plans that add GTE numerics, commands, the reference model and the COP2 connection

actuals:
  tokens: 7700
  tasks: 2
  commits: 2

plan_head_before: ad9845b5ff3b2861c6a5e6629f2dcf706f18ce9a
plan_head_after: 4fc5bffeb784152a69e886f9c04e0b879c041683
commits: 2

tech-stack:
  added: []
  patterns:
    - "a seeded file starts with an SPDX line naming both licences, the upstream path, the full 40-hex commit, the holders and what changed, and a THIRD-PARTY-NOTICES section whose Applies-to line names the file"
    - "the vector replay lives in a header that holds no engine code: three caller-supplied functions reach the implementation, so a second implementation can be replayed through the same file"
    - "a sweep test allocates the struct on its own heap block so the sanitizer redzones prove no access leaves it"

key-files:
  created:
    - tests/gte/vectors.h
    - tests/gte/gte_tests.c
    - docs/hardware/gte.md
  modified:
    - src/gte.c
    - src/gte.h
    - tests/gte/tests.cmake
    - tests/gte/inventory.txt
    - THIRD-PARTY-NOTICES.md
    - PROVENANCE.md

key-decisions:
  - "16-bit registers (VZ0-2, OTZ, IR0-3, SZ0-3, RT33, L33, LB3, H, DQA, ZSF3, ZSF4) are stored as 16-bit patterns and extended on read, sign for all but OTZ and SZ; SXYP, IRGB and ORGB hold no value of their own; LZCR is stored and derived when LZCS is written; FLAG keeps bits 12 to 30 and bit 31 is recomputed on write, on read and at the end of every command"
  - "the replay clears registers 0 to 63 by writing 0 through the same write function, not by resetting the engine, so the clear is part of the replayed order"
  - "regs_sweep also checks that no command reports more than 44 cycles, a bound the CPU wait relies on"
  - "the page names the ares seed only in its notice paragraph; the SPDX line and the notices section are the notice the rules require for a permissive seed"

patterns-established:
  - "Pattern: test cases that take a file argument take --expect N and exit 1 on another count, the zero-case guard of the vectors harness"

requirements-completed: []

coverage:
  - id: D1
    description: "The GTE register file follows the architectural read and write rules (SXYP, IRGB and ORGB, LZCR, sign extension, FLAG bit 31) and matches the 50 register-only hardware cases replayed in the upstream order"
    requirement: "GTE-01"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^gte\\.vectors\\.part1$' (50 cases, 0 failures, exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The container reader reads little-endian byte by byte, rejects a length that is not a count plus whole records, and carries no seed text"
    requirement: "GTE-01"
    verification:
      - kind: other
        ref: "schwaystation-test-gte vectors on an empty file exits 1; --expect 51 on the 50 register-only cases exits 1; grep -q 'ares\\|interpreter' tests/gte/vectors.h returns 1"
        status: pass
    human_judgment: false
  - id: D3
    description: "The seed is attributed: SPDX header on src/gte.c naming MIT AND ISC with the full commit, the notices section whose Applies-to line names src/gte.c, and the PROVENANCE seed row with the zero-diff replacement condition"
    requirement: "GTE-01"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset hygiene (policy.notices, hygiene.no_float, hygiene.tree; 23 of 23 tests, exit 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "Every register index and every command number at edge values with sf and lm both ways reads deterministically, never reports more than 44 cycles, and stays inside the struct"
    requirement: "GTE-01"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^gte\\.regs\\.sweep$' (exit 0); the asan lane runs it under ASan and UBSan in hosted CI"
        status: pass
    human_judgment: false
  - id: D5
    description: "docs/hardware/gte.md documents the register file, command encoding, FLAG layout, timing, the hardware cases and the seed notice, under the byte budget and with no planning identifier"
    requirement: "GTE-01"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^docs\\.budgets$' (exit 0); test $(wc -c < docs/hardware/gte.md) -lt 61440 (10,868 bytes)"
        status: pass
    human_judgment: false

duration: 7min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 05: Seed the GTE register file Summary

**The ares GTE register file ported to integer C (SPDX header, ISC notice section, provenance seed row) passes all 50 register-only hardware cases through a caller-driven replay harness, with a sanitizer-ready register and command sweep and the first GTE page**

## Performance

- **Duration:** 7 min
- **Started:** 2026-11-10T05:18:51Z
- **Completed:** 2026-11-10T05:26:06Z (code and docs; the close-out commit follows)
- **Tasks:** 2
- **Files modified:** 9 outside `.planning` (3 created)

## Accomplishments

- `schway__gte_read` and `schway__gte_write` follow the architectural form: SXYP reads SXY2 and a write pushes the screen FIFO, IRGB sets IR1 to IR3 and either of IRGB and ORGB reads them back as 5-bit fields, LZCR is derived when LZCS is written, the 16-bit registers keep 16-bit patterns and extend as the hardware shows (H, DQA, ZSF3 and ZSF4 sign-extend), and FLAG bit 31 is recomputed on a write, on a read and at the end of every command. Nothing else in `src/gte.c` changed, so the cycle table and the state walk from plan 01 stand.
- The full ares commit is `9408cb43d4948fc3ea6e152a307a34348df3fe04` (resolved with `gh api repos/ares-emulator/ares/commits/9408cb43d494 --jq .sha`). The SPDX line at the top of `src/gte.c`, with the THIRD-PARTY-NOTICES section "ares GTE seed", is the notice AGENTS rule 1 requires for a permissive seed, not an other-emulator identifier in code. The ISC text in the notices section is the root `LICENSE` text of that commit, with the holders as it names them ("ares team, Near et al", 2004 to 2025).
- No single-precision factor and no comment word naming a non-integer type entered the seed file: only the register paths were ported, and the projection's `* 1.0f` belongs to the numerics plan. `hygiene.no_float` passes on both commits.
- `tests/gte/vectors.h` reads the container byte by byte (a length that is not a count plus whole 516-byte records is refused), replays in the exact order (clear 0 to 63, write 0 to 63, command unless register-only, read 0 to 63) and holds no seed text. `gte.vectors.part1` runs 50 cases with `--ops reg` and exits 1 on any other count.
- `gte.regs.sweep` writes every register at the seven edge values and runs every command number with sf and lm both ways on registers filled with each value.
- PROVENANCE section 4's GTE row is now a seed row: replace when an own implementation passes a zero-diff differential over the 1,150 cases and the property test with hashes equal on all eight lanes.

## Task Commits

1. **Task 1: Tracer - the seeded register file passes the 50 register-only hardware cases through the replay harness** - `03ca717` (feat)
2. **Task 2: The register sweep under the sanitizers and the GTE page** - `4fc5bff` (test)

**Plan metadata:** the docs commit that carries this summary, STATE.md and ROADMAP.md.

The tracer gate ran in the default end-of-phase mode with an automated-only verify: `cmake --workflow --preset ci`, `ctest -L '^gte$'` and `cmake --workflow --preset hygiene` were re-run, passed, and the expansion task went ahead.

## Files Created/Modified

- `src/gte.c` - the seed: register file read and write rules, FLAG, LZCR and IRGB helpers, SPDX header
- `src/gte.h` - the struct comment states the storage form
- `tests/gte/vectors.h` - container reader and replay
- `tests/gte/gte_tests.c` - the `vectors` and `regs_sweep` cases
- `tests/gte/tests.cmake`, `tests/gte/inventory.txt` - the executable and the two tests
- `THIRD-PARTY-NOTICES.md`, `PROVENANCE.md` - the notice section and the seed row
- `docs/hardware/gte.md` - the GTE page (10,868 bytes)

## Decisions Made

See `key-decisions` above. Two points for the later GTE plans: the 16-bit registers hold masked patterns on write and are masked again on read, so a hostile loaded state with extra upper bits reads as the architectural value, but the numerics must take `d[]` and `c[]` entries through the same 16-bit view (the page says H is unsigned in the arithmetic); and `d[15]`, `d[28]`, `d[29]` and, apart from the derived count, `d[31]` hold nothing and are not read by the commands.

## Deviations from Plan

None - plan executed exactly as written.

One build fix inside Task 1 is recorded for completeness: the first compile of `parse_ops` tripped `-Wcast-qual` (a `const char *` cast to `char *` for `strtoul`); it was rewritten to advance the `const char *` cursor and take the end pointer from `strtoul` separately. This was before any commit.

## Issues Encountered

- Not run locally: the `asan` and `be32` lanes need clang-21, which this host lacks; hosted CI runs them. `gte.regs.sweep` is the case written for the asan lane's ASan and UBSan, and the new code uses fixed-width unsigned integers only, no signed shift and no floating point. The ci and hygiene workflows both exit 0 here; the ci JUnit check reports 251 tests ran once each.
- The broken-windows ledger `.planning/WINDOWS.md` does not exist, so the stubs below were not appended to it.

## Known Stubs

| File | Stub | Resolved by |
|------|------|-------------|
| src/gte.c | `schway__gte_exec` still returns the documented cycle count and computes nothing; only FLAG bit 31 is recomputed on exit, so the 1,100 command cases of the table fail until the numerics land | the numerics and first-seven-commands plan (03-26) and the remaining-commands plan (03-07) |

`docs/hardware/gte.md` says so in its "State of the engine" section.

## Threat Flags

None. No endpoint, auth path, file access or schema at a trust boundary is added; `schwaystation-test-gte` reads one file named on its command line and is test code.

## Evidence (commands and exit statuses)

- `cmake --workflow --preset ci`: exit 0 (251 tests, 100% passed)
- `ctest --test-dir build/ci -L '^gte$' --no-tests=error --output-on-failure`: exit 0 (`gte.vectors.part1`, then both gte tests in the ci run)
- `ctest --test-dir build/ci -R '^gte\.regs\.sweep$|^docs\.budgets$' --no-tests=error --output-on-failure`: exit 0 (2 of 2)
- `cmake --workflow --preset hygiene`: exit 0 (23 of 23)
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake`: exit 0 (251 tests ran once each)
- `build/ci/tests/schwaystation-test-gte vectors tests/vectors/gte/ps1tests-1150.bin --expect 51 --ops reg`: exit 1 (count guard); the same on an empty file: exit 1
- Acceptance: `head -n 5 src/gte.c | grep -q 'SPDX-License-Identifier: MIT AND ISC'` exit 0; `grep -q 'src/gte.c' THIRD-PARTY-NOTICES.md && grep -q '9408cb43d494' PROVENANCE.md` exit 0; `grep -q 'ares\|interpreter' tests/gte/vectors.h` exit 1 as required; the planning-identifier grep over the page, `src/gte.c` and `tests/gte/gte_tests.c` exit 1 as required; the page is 10,868 bytes against 61,440

## Next Phase Readiness

The register file, the harness and the notice are in place. Plan 03-26 adds the numerics, the divide and the first seven commands and widens `gte.vectors.part1`; the reference-model plan can read `tests/gte/vectors.h` (it holds no seed text). The CPU's COP2 forms still take the unbuilt path until the plan that connects them.

## Self-Check: PASSED

- Created files exist: `tests/gte/vectors.h`, `tests/gte/gte_tests.c`, `docs/hardware/gte.md`.
- Commits `03ca717` and `4fc5bff` are ancestors of HEAD; the plan measured 2 commits from the recorded base `ad9845b`.
- Every task's acceptance commands exited as the plan says; `cmake --workflow --preset ci` and `cmake --workflow --preset hygiene` pass.

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*
