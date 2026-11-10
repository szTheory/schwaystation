---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 23
subsystem: docs
tags:
  - preparation
  - corrections
  - pointers
  - conformance

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: the docs/hardware pages (plans 03-13 to 03-19) and the re-pointed source and test citations (plans 03-21 and 03-22), so nothing outside the preparation files names a shrunk section
provides:
  - the corrections phase 3 found are recorded in ARCHITECTURE, CONFORMANCE, PSXDEV-EXTRACTS and DECISIONS where a later step would look
  - PSX-SYSTEM sections 2 to 6 and PSX-GPU-AND-GTE section 5 are pointers to the docs/hardware pages with the corrections each section needed
  - every Sources row, the C3 to C6 choices and choice 5.A stay, so every cited ID still resolves
affects:
  - phase 4 planning, which reads the corrected preparation files and the pointers instead of the duplicated hardware text

actuals:
  tokens: 9200
  tasks: 2
  commits: 2

plan_head_before: 5da128346d0bacebef1ca19c99b5b9e5436a76ac
plan_head_after: ddb4a2ba921e1bafd3324f169be025478bf19e50
commits: 2

tech-stack:
  added: []
  patterns:
    - "a preparation section that a public page now replaces keeps its heading, its Sources rows and its numbered choice paragraphs, and says the corrections found while building it"

key-files:
  created: []
  modified:
    - .planning/preparation/ARCHITECTURE.md
    - .planning/preparation/CONFORMANCE.md
    - .planning/preparation/DECISIONS.md
    - .planning/preparation/PSXDEV-EXTRACTS.md
    - .planning/preparation/PSX-SYSTEM.md
    - .planning/preparation/PSX-GPU-AND-GTE.md
    - .planning/preparation/README.md

key-decisions:
  - "ARCHITECTURE keeps the first-draft deadline and overrun text and says in front of it that the deadline is derived and never stored and that the bound is 1,024 cycles plus the DMA unit bound, so a reader of the older sentence is told which one holds"
  - "The C3 to C6 and 5.A choice paragraphs stay in the shrunk sections because ARCHITECTURE cites them by label; C5 and C6 each gain a sentence saying what phase 3 built"
  - "The DICR correction reads: the row named bits 0-6 and ares and the built core store bits 0-5, with bit 6 unverified and unmodelled, as docs/hardware/dma.md states"

requirements-completed: []

coverage:
  - id: D1
    description: "ARCHITECTURE says the next deadline is derived and never stored, the overrun bound is 1,024 cycles plus the DMA unit bound, and gte.c, dma.c and timers.c are phase 3"
    requirement: GTE-01
    verification:
      - kind: other
        ref: "grep -q 'derived and never stored' .planning/preparation/ARCHITECTURE.md, exit 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "CONFORMANCE has at most 30 gte_vectors records copied from table cases with no invented corner, mailbox offsets that follow tests/fixtures/src/mailbox.inc, a dma_chain row in both fixture tables, and a judge with the two sources doc and hw"
    requirement: GTE-02
    verification:
      - kind: other
        ref: "grep -q 'dma_chain' .planning/preparation/CONFORMANCE.md, exit 0; grep -q 'loads 64 records' .planning/preparation/CONFORMANCE.md, exit 1"
        status: pass
    human_judgment: false
  - id: D3
    description: "PX.21 notes the 112,0xx..112,5xx figure is a 16-bit wrap artefact of a 71,4xx system-clock / 8 count and is not pinned, and DEC.51 says a committed third-party byte needs a manifest row"
    requirement: GTE-03
    verification:
      - kind: other
        ref: "grep -qE '^\\| PX\\.21 .*112,0xx.*16-bit wrap artefact' .planning/preparation/PSXDEV-EXTRACTS.md and grep -qE '^\\| DEC\\.51 .*manifest row' .planning/preparation/DECISIONS.md, exit 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "PSX-SYSTEM sections 2 to 6 and PSX-GPU-AND-GTE section 5 are pointers to the docs/hardware pages with the phase 3 corrections, the Sources rows stay, and the README index and question rows name the pages"
    requirement: GTE-01
    verification:
      - kind: other
        ref: "test -z \"$(grep -rnoE '(PSX-SYSTEM|PSX-GPU-AND-GTE)( sections? [0-9]+)?' src tests docs | grep -vE '(PSX-SYSTEM sections? (1|7|8|9|10)|PSX-GPU-AND-GTE sections? [1-46-9])$')\" and grep -q docs/hardware .planning/preparation/PSX-SYSTEM.md and grep -q docs/hardware/gte.md .planning/preparation/PSX-GPU-AND-GTE.md, exit 0"
        status: pass
      - kind: other
        ref: "a shell loop over every [SYS.nn] and [GPU.nn] cited in .planning/preparation finds a Sources row for each, no UNRESOLVED line"
        status: pass
    human_judgment: false
  - id: D5
    description: "No file in the repository holds a personal path after the edits"
    requirement: GTE-03
    verification:
      - kind: integration
        ref: "cmake --workflow --preset hygiene, exit 0 (23 of 23 tests)"
        status: pass
    human_judgment: false

duration: 6 min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 23: Preparation corrections and pointers Summary

**Phase 3's findings are written into ARCHITECTURE, CONFORMANCE, PX.21 and DEC.51, and the duplicated hardware sections of PSX-SYSTEM and PSX-GPU-AND-GTE are pointers to the public pages with every citation ID still resolving.**

## Performance

- **Duration:** 6 min
- **Completed:** 2026-11-10T08:19:00Z
- **Tasks:** 2
- **Files modified:** 7 (60 insertions, 181 deletions)

## Accomplishments

- ARCHITECTURE section 1 says the deadline is derived from the devices and never stored, the overrun bound is `SCHWAY__INSN_CYCLES_MAX` (1,024) plus the DMA unit bound (2,691,328), the first draft's 512 and 256-cycle chunk are named as superseded (and that `core.overrun_bound` was never built), and the section 2 table gives `gte.c`, `dma.c` and `timers.c` as phase 3. Section 5 drops `next_deadline` from the state table with a note.
- CONFORMANCE: `gte_vectors` has at most 30 records copied from table cases with no invented corner (a corner has no `hw` source), it runs the real CPU path, and a guard test reaches each expected CRC by two paths; a `dma_chain` row is in both fixture tables with what it proves and why the oracle cannot be fooled; the mailbox table follows `tests/fixtures/src/mailbox.inc` (CRC at 04h, status at 08h, version 09h, count 0Ah, id 0Bh); the judge's sources are `doc` and `hw`; section 8 lists `dma_chain`.
- PX.21 carries the 16-bit wrap artefact note (the same log shows 71,407..71,412 sysclk/8 counts, about 0.9% above the NTSC frame, from a PAL-crystal console forced to NTSC); DEC.51 says a committed third-party byte needs a manifest row and a notice.
- PSX-SYSTEM sections 2 to 6 and PSX-GPU-AND-GTE section 5 are pointers to `docs/hardware/{cpu,cop0,bus,dma,timers,gte}.md`. The recorded corrections: DICR bits 0-6 in the old row against bits 0-5 in ares and the core; a list node above 8 MiB cannot raise a DMA error because the bit-23 end test fires first; the 1000h-node guard yields instead of aborting; ares holds a written timer value 1 cycle against psx-spx's 2; the GTE wait covers MFC2, CFC2, SWC2 and a new command and not MTC2, CTC2 or LWC2, and SWC2 is not immediate; the interrupt entry runs the instruction at the program counter, not the word at EPC.
- PSX-SYSTEM went from 351 to 272 lines and PSX-GPU-AND-GTE from 337 to 290. The README index and question rows name the pages, and the `updated:` dates of the seven files moved to 2026-11-10.

## Task Commits

1. **Task 1: the recorded corrections (tracer)** - `0283a80` (docs)
2. **Task 2: shrink PSX-SYSTEM 2 to 6 and PSX-GPU-AND-GTE 5 to pointers** - `ddb4a2b` (docs)

The tracer's verify command was re-run end to end after Task 1 and passed before Task 2 began.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical functionality] The C3 to C6 and 5.A choice paragraphs were kept in the shrunk sections**
- **Found during:** Task 2
- **Issue:** ARCHITECTURE cites `[ARCH.01 C5]`, `[ARCH.01 C6]` and `[ARCH.02 5.A]` by label, and the plan's rule is to keep every anchor the planning files cite. Replacing the whole section body would have left those labels without a target.
- **Fix:** each label's paragraph stays; C5 and C6 gain a sentence saying phase 3 built a different scheduler and a lazy closed form, and 5.A gains one saying what was built.
- **Files modified:** `.planning/preparation/PSX-SYSTEM.md`, `.planning/preparation/PSX-GPU-AND-GTE.md`
- **Commit:** `ddb4a2b`

**2. [Rule 1 - Bug] ARCHITECTURE's older deadline and overrun sentences were kept and prefixed instead of deleted**
- **Found during:** Task 1
- **Issue:** later sections and sources still read the first-draft sentences; deleting them would have changed text other lines depend on, while leaving them bare would repeat a claim phase 3 proved wrong.
- **Fix:** the corrected statement comes first and names the old one as the first draft.
- **Files modified:** `.planning/preparation/ARCHITECTURE.md`
- **Commit:** `0283a80`

**Total deviations:** 2 auto-fixed (1 Rule 2, 1 Rule 1). **Impact:** none on scope; both keep citations resolvable.

## Issues Encountered

None. The plan's wording "DICR bits 0-6 where ares has 0-5" is ambiguous about which side is wrong; the record states the facts as the built core and `docs/hardware/dma.md` hold them (stored bits 0-5, bit 6 unverified) rather than choosing a side.

## Verification

- `grep -q 'dma_chain' .planning/preparation/CONFORMANCE.md && grep -qE '^\| PX\.21 .*112,0xx.*16-bit wrap artefact' .planning/preparation/PSXDEV-EXTRACTS.md && grep -qE '^\| DEC\.51 .*manifest row' .planning/preparation/DECISIONS.md`, exit 0.
- `grep -q 'loads 64 records' .planning/preparation/CONFORMANCE.md; test $? -eq 1`, exit 0.
- The citation gate over `src tests docs` prints nothing and the two pointer greps pass, exit 0.
- PSX-SYSTEM has fewer lines than at HEAD before the shrink (272 against 351) and still has `## Sources`, exit 0.
- Every `[SYS.nn]` and `[GPU.nn]` cited in `.planning/preparation` has a Sources row.
- `cmake --workflow --preset hygiene`, exit 0, 23 of 23 tests passed.
- Not run: `cmake --workflow --preset ci`, asan and be32 (planning files only; the asan and be32 lanes need clang-21, which this host lacks, and hosted CI runs them).

## Known Stubs

None.

## Threat Flags

None.

## Next Phase Readiness

Ready for the next plan of the phase. Phase 4's first plan reads the corrected files and the pointers.

## Self-Check: PASSED

- All seven modified files exist and carry the edits above.
- Commits `0283a80` and `ddb4a2b` are ancestors of HEAD.
