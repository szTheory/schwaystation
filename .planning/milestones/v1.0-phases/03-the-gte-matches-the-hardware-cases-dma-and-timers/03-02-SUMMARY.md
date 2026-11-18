---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 02
subsystem: testing
tags:
  - gte
  - vectors
  - provenance
  - notices
  - hygiene
  - python

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: the empty tests/gte directory and the phase-3 skeleton (plan 01)
provides:
  - tools/gte2bin.py, the stdlib-only transcoder that refuses any input but the pinned upstream blob
  - tests/vectors/gte/ps1tests-1150.bin, the 1,150 hardware-captured GTE cases as a count and 516-byte records
  - tests/fixtures/src/gte_cases.inc, 26 cases copied verbatim for the gte_vectors fixture
  - the manifest, pins.txt and THIRD-PARTY-NOTICES.md rows for the table, with the upstream MIT text
  - policy.notices and its negative control, the manifest-driven notices gate (30 lines)
  - policy.gate_budget ratcheted to the measured 1513 lines
affects:
  - 03-04
  - 03-05
  - phase-3 plans that replay the table (the harness, the reference model, the gte_vectors fixture)

actuals:
  tokens: 9000
  tasks: 3
  commits: 3

plan_head_before: 4a70a40ae9f263c64b0e55a63995a9a790d5494c
plan_head_after: 4861f31ce18801157bd8c3db8cbefa9897d7bc85
commits: 3

tech-stack:
  added: []
  patterns:
    - "a committed third-party table is one manifest row per file, one notices section whose Applies-to line names both, and a blob row in pins.txt; policy.notices checks the first two"
    - "the maintainer transcoder verifies the git-blob SHA-1 of its input before parsing, so a wrong or edited upstream file fails loudly"

key-files:
  created:
    - tools/gte2bin.py
    - tests/vectors/gte/ps1tests-1150.bin
    - tests/fixtures/src/gte_cases.inc
    - tests/cmake/notices.cmake
  modified:
    - .gitignore
    - tests/fixtures/manifest.txt
    - pins.txt
    - THIRD-PARTY-NOTICES.md
    - tests/hygiene/tests.cmake
    - tests/hygiene/inventory.txt
    - docs/testing.md
    - docs/maintaining.md
    - .planning/todos/pending/005-notices-gates.md

key-decisions:
  - "The include holds 26 cases: one per command (22, the first whose output FLAG has bit 31, else a non-zero FLAG, else the first) plus register-only cases 1 to 4; a record is 66 words (case number, opcode, 64 inputs) so a guard test can map a record back to the table"
  - "The gate budget limit is 1513, exactly the measured total, so any later gate line must replace or fold an older one"

patterns-established:
  - "Pattern: a gate script is usage line, Fault prevented line, then logic, and a self test in the same lane that writes a broken fixture into the build tree at configure time and expects exit 1 with every name"

requirements-completed: []

coverage:
  - id: D1
    description: "tools/gte2bin.py refuses a wrong input, asserts 1,150 cases and the 50 plus 22 x 50 histogram, and a second run reproduces both outputs byte for byte"
    requirement: "GTE-01"
    verification:
      - kind: other
        ref: "python3 -I tools/gte2bin.py $TMPDIR/ps1tests/tests.c $TMPDIR/gte-a.bin --inc $TMPDIR/gte-a.inc, then cmp against both committed files; exit 0"
        status: pass
      - kind: other
        ref: "python3 -I tools/gte2bin.py tools/gte2bin.py $TMPDIR/x.bin; exit 1"
        status: pass
    human_judgment: false
  - id: D2
    description: "The vector file is 593,404 bytes (count plus 1,150 x 516) and the 26-case include carries upstream case numbers and the full upstream commit"
    requirement: "GTE-01"
    verification:
      - kind: other
        ref: "wc -c of tests/vectors/gte/ps1tests-1150.bin equals 593404; grep -c 'test [0-9]' of the include is 26"
        status: pass
    human_judgment: false
  - id: D3
    description: "The manifest, pin and notice rows name both files with the pinned blob, the full commit, the SHA-256 and the upstream MIT text"
    requirement: "GTE-01"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset hygiene (hygiene.tree, policy.notices); sh scripts/hygiene.sh --tree exit 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "policy.notices fails for a noticed manifest row on no Applies-to line and for an Applies-to path that does not exist"
    requirement: "GTE-01"
    verification:
      - kind: integration
        ref: "policy.notices and policy.notices.self_test in tests/hygiene/tests.cmake"
        status: pass
    human_judgment: false
  - id: D5
    description: "policy.gate_budget limit and docs/testing.md fall from 1520 to the measured 1513, and the transcode procedure is in docs/maintaining.md"
    requirement: "GTE-01"
    verification:
      - kind: integration
        ref: "policy.gate_budget in the hygiene preset; test of cat scripts/*.sh tests/cmake/*.cmake .githooks/* | wc -l at most 1520"
        status: pass
    human_judgment: false

duration: 6min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 02: The GTE hardware cases as an own vector file Summary

**1,150 hardware-captured GTE cases transcoded from the pinned MIT table into a 593,404-byte binary with a 26-case fixture include, an MIT notice and pin, a 30-line manifest-driven `policy.notices` gate, and the gate budget ratcheted to 1513**

## Performance

- **Duration:** 6 min
- **Started:** 2026-11-10T04:52:24Z
- **Completed:** 2026-11-10T04:58:13Z (code and docs; the close-out commit follows)
- **Tasks:** 3
- **Files modified:** 13 (4 created)

## Accomplishments

- `tools/gte2bin.py` verifies the git-blob SHA-1 of its input, parses the 1,150 cases, asserts the histogram (50 register-only, 22 commands at 50 each) and writes the `.bin` and the `.inc` with no timestamp or path. A second run is byte for byte equal.
- `tests/vectors/gte/ps1tests-1150.bin` has SHA-256 `e8a0e61a57a6582ac53d39b941eeaa83b87498444a3ada915d9de73d313dd9c2` (a u32 count, then the records).
- Selected case numbers in `tests/fixtures/src/gte_cases.inc` (26 records, 23,258 bytes): 51, 138, 151, 201, 251, 301, 351, 401, 451, 501, 551, 601, 651, 701, 751, 801, 851, 904, 951, 1001, 1051, 1101 (commands 01h to 3Fh in ascending order) and register-only cases 1, 2, 3, 4. The include's SHA-256 is `22d83f53a818096106543758c20c8cab3218255af740faff37ff403b6e581220`.
- `policy.notices` (`tests/cmake/notices.cmake`, 30 lines) and `policy.notices.self_test` are in the hygiene lane and inventory; an experiment with the notices file cut before the new section made the gate name all three binaries and tables.
- `policy.gate_budget` limit is 1513; the measured total of `scripts/*.sh tests/cmake/*.cmake .githooks/*` is 1513 (1483 before the gate, 30 lines added).
- `docs/maintaining.md` gains "Regenerating the GTE vector file"; todo 005 is trimmed to the SPDX scan and the `.info` check, rebound to the first plan that needs them.

## Task Commits

1. **Task 1: Tracer, the transcoder writes the vector file and a fresh run reproduces it** - `51da1cb` (feat)
2. **Task 2: Manifest, pin and notice rows, and policy.notices with its negative control** - `8a59db2` (test)
3. **Task 3: The budget ratchet and the maintainer procedure** - `4861f31` (docs)

**Plan metadata:** the close-out commit (`docs(03-02): complete ...`) follows this file.

## Files Created/Modified

- `tools/gte2bin.py` - the transcoder (stdlib only, run as `python3 -I`)
- `tests/vectors/gte/ps1tests-1150.bin` - the 1,150 hardware cases
- `tests/fixtures/src/gte_cases.inc` - the fixture's 26 input cases
- `tests/cmake/notices.cmake` - the notices gate, seeded from cogmaster f09ac97096ac
- `tests/hygiene/tests.cmake`, `tests/hygiene/inventory.txt` - the two new tests and the new budget limit
- `tests/fixtures/manifest.txt`, `pins.txt`, `THIRD-PARTY-NOTICES.md` - the rows and the "ps1-tests GTE cases" section
- `.gitignore` - admits the one new binary path, as the r3000 sample's line does
- `docs/testing.md`, `docs/maintaining.md` - the budget text and the regeneration procedure
- `.planning/todos/pending/005-notices-gates.md` - rebound to the first non-MIT or SPDX-bearing file

## Decisions Made

- The include record layout is 66 words, case number first, so the later guard test maps each fixture record back to a table case without parsing comments.
- The limit equals the measured total (1513): the cap only falls, and any later gate line must fold an older one.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Plan inconsistency] The advisor's SHA-256 is of the records without the count word**
- **Found during:** Task 1 (acceptance criteria)
- **Issue:** The acceptance check `shasum -a 256 ... | grep -E '^ed630d2f[0-9a-f]+58d2 '` cannot hold together with the required 593,404-byte layout. The whole 593,404-byte file hashes to `e8a0e61a...d9c2`. The digest `ed630d2f...58d2` is that of the 593,400 bytes after the 4-byte count: `tail -c +5 tests/vectors/gte/ps1tests-1150.bin | shasum -a 256` prints `ed630d2fadce6c1d656dde643c2884ab454d43cdb364c21bee46fcabcda658d2`. So the advisor's record layout (opcode, 64 inputs, 64 outputs, 516 bytes each) is reproduced exactly; only the count header differs from what the advisor hashed.
- **Fix:** kept the layout the plan and the context specify (count plus records, 593,404 bytes) and recorded both digests. The manifest and notice carry the whole-file digest, `e8a0e61a57a6582ac53d39b941eeaa83b87498444a3ada915d9de73d313dd9c2`. The criterion's intent (independent confirmation of the records) is met by the record digest above.
- **Files modified:** none (recorded here only)
- **Verification:** `tail -c +5` digest equals the advisor's; the size check and the byte-for-byte rerun pass.

**2. [Rule 3 - Blocking] The ignore file excluded the new binary**
- **Found during:** Task 1 (staging)
- **Issue:** `.gitignore` ignores `*.bin` and admits only the r3000 sample by path, so `tests/vectors/gte/ps1tests-1150.bin` could not be staged.
- **Fix:** added `!/tests/vectors/gte/ps1tests-1150.bin` beside the r3000 line. No `git add -f`.
- **Files modified:** `.gitignore`
- **Committed in:** `51da1cb`

**3. [Rule 3 - Blocking] The manifest rows land with Task 1, not Task 2**
- **Found during:** Task 1 (commit)
- **Issue:** the pre-commit hygiene check refuses a staged binary that is not in the manifest, so the two manifest rows had to be in the first commit.
- **Fix:** committed the two `tests/fixtures/manifest.txt` rows with Task 1; Task 2 added the pin, notice and gate rows. The notices section did not exist for one commit; `policy.notices` was not yet in the tree, so no gate saw the gap.
- **Committed in:** `51da1cb`

---

**Total deviations:** 3 (1 plan inconsistency, 2 blocking)
**Impact on plan:** no change to any deliverable or limit; the layout, size and digests the later plans consume are as specified.

## Issues Encountered

- `notices.cmake` was 32 lines on the first write; it was trimmed to 30 by shortening the argument guard to one test and dropping the per-variable loop (within the plan's 31 to 37 line trim band).

## Known Stubs

None. The include and the `.bin` are data, not placeholders.

## Threat Flags

None. No network endpoint, auth path or schema is added; the new files are test data, a maintainer tool CI never runs and a hygiene script.

## Not run locally

`asan` and `be32` need clang-21, which this host lacks; hosted CI runs them. None of this plan's files are built into either lane (data, a Python tool and CMake scripts of the hygiene lane). `cmake --workflow --preset hygiene` and `cmake --workflow --preset ci` both exit 0 here.

## Evidence (commands and exit statuses)

- `python3 -I tools/gte2bin.py "$TMPDIR/ps1tests/tests.c" "$TMPDIR/gte-a.bin" --inc "$TMPDIR/gte-a.inc"` then `cmp` against `tests/vectors/gte/ps1tests-1150.bin` and `tests/fixtures/src/gte_cases.inc`: exit 0
- `git hash-object "$TMPDIR/ps1tests/tests.c"` printed `a03aa72173f122bf916039f438c8cde89e61e526`
- `cmake --workflow --preset hygiene`: exit 0 (23 of 23 tests); `cmake -DJUNIT=build/hygiene/junit.xml -DINVENTORY=tests/inventory/hygiene.txt -P tests/cmake/check_junit.cmake`: exit 0
- `sh scripts/hygiene.sh --tree`: exit 0; `cmake --workflow --preset ci`: exit 0
- `cat scripts/*.sh tests/cmake/*.cmake .githooks/* | wc -l`: 1513

## Next Phase Readiness

The table, its notice and the fixture include are in place for the harness, the reference model and the `gte_vectors` fixture plans. Nothing blocks them.

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*

## Self-Check: PASSED
