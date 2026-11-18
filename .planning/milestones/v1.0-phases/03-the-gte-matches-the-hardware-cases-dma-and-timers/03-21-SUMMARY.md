---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 21
subsystem: docs
tags:
  - citations
  - hardware-pages
  - comments
  - wr-06

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: the gte, dma and timers pages and the hardware pages this plan points at (plans 03-13 to 03-19)
provides:
  - every comment in src/ cites the docs/hardware page and heading (or psx-spx) that states the fact, with no citation of PSX-SYSTEM sections 2 to 6 or PSX-GPU-AND-GTE section 5
  - the bus, COP0 and CPU pages cite psx-spx and no planning file
  - the CPU page says the CPU takes interrupts from the vertical blank, the DMA and the timers
  - the completed todo for the phase-2 review finding WR-06
affects:
  - plan 03-23, which shrinks the preparation sections to pointers and no longer has a citation pointing back
  - plan 03-25, the todo ledger, which names the completed todo 030

actuals:
  tokens: 17200
  tasks: 2
  commits: 2

plan_head_before: 8554d7cf4dfe5d1dfce1d6f1f9dc9780696e068d
plan_head_after: 8197c0e7e7ec8ac9b26e66458c4512861f0c2c66
commits: 2

tech-stack:
  added: []
  patterns:
    - "a hardware comment cites docs/hardware/<page>.md and a heading that exists on that page; a hardware page cites psx-spx or a pin, never a preparation file"

key-files:
  created:
    - .planning/todos/completed/030-cpu-page-interrupt-sentence.md
  modified:
    - src/bus.c
    - src/bus.h
    - src/cop0.c
    - src/cpu.c
    - src/cpu.h
    - src/cpu_exec.inc
    - src/irq.c
    - src/gte.c
    - src/dma.c
    - src/dma.h
    - docs/hardware/bus.md
    - docs/hardware/cop0.md
    - docs/hardware/cpu.md

key-decisions:
  - "A source comment names the public page and one of that page's own headings (checked by script against the page's headings), so the reader lands on the section that states the fact; the old subsection names (Load delay, SR bits, I-cache) were mapped to the headings that now hold them"
  - "A hardware page cites the psx-spx page for a fact that comes from a published document and says the core's own model where a cost is the core's, instead of citing the preparation file's cost model"

patterns-established:
  - "re-pointing a citation is checked by the same grep that finds it, and the code with comments stripped is compared against HEAD to show the change is comment-only"

requirements-completed: []

coverage:
  - id: D1
    description: "No comment in src/ cites PSX-SYSTEM sections 2 to 6 or PSX-GPU-AND-GTE section 5 in any form (numbered, wrapped, unnumbered); each now names a docs/hardware page and heading or a psx-spx page"
    requirement: GTE-01
    verification:
      - kind: other
        ref: "test -z \"$(grep -rnoE '(PSX-SYSTEM|PSX-GPU-AND-GTE)( sections? [0-9]+)?' src | grep -vE '(PSX-SYSTEM sections? (1|7|8|9|10)|PSX-GPU-AND-GTE sections? [1-46-9])$')\", exit 0 (73 mentions at HEAD, 0 after)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The change is comment-only: the build, every test and every hash are unchanged and hygiene.no_float passes"
    requirement: GTE-01
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci, exit 0"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset hygiene, exit 0"
        status: pass
      - kind: other
        ref: "ctest --test-dir build/ci -R '^(core\\.trace_hash_equal|runner\\.hashes_file|abi\\.symbols)$' --no-tests=error, exit 0"
        status: pass
    human_judgment: false
  - id: D3
    description: "docs/hardware pages cite no preparation file, and cpu.md no longer says the CPU does not take interrupts"
    requirement: GTE-02
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^docs\\.' --no-tests=error, exit 0, and ! grep -nE 'PSX-SYSTEM|PSX-GPU-AND-GTE|PSXDEV-EXTRACTS' docs/hardware/*.md, exit 0"
        status: pass
      - kind: other
        ref: "grep -q 'does not take interrupts' docs/hardware/cpu.md exits 1 (baseline 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "The completed todo for review finding WR-06 exists"
    requirement: GTE-03
    verification:
      - kind: other
        ref: "test -f .planning/todos/completed/030-cpu-page-interrupt-sentence.md, exit 0"
        status: pass
    human_judgment: false

duration: 5min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 21: Re-point the preparation citations and fix the CPU interrupt sentence Summary

**73 source-comment citations of the preparation sections re-pointed to the docs/hardware pages and headings, the bus, COP0 and CPU pages moved onto psx-spx, and the CPU page's "does not take interrupts" sentence corrected (WR-06).**

## Performance

- **Duration:** 5 min
- **Started:** 2026-11-10T08:04:51Z
- **Completed:** 2026-11-10T08:09:28Z
- **Tasks:** 2
- **Files modified:** 14 (13 modified, 1 created)

## Accomplishments

- Every comment in `src/` that cited PSX-SYSTEM sections 2 to 6 or PSX-GPU-AND-GTE section 5 now cites the `docs/hardware` page and a heading that exists on it. The citation gate listed 73 mentions in `src/` at HEAD (the plan said 66; see Deviations) and lists none now. By file: cpu_exec.inc 24, cpu.h 15, cop0.c 11, bus.c 9, bus.h 4, irq.c 4, dma.h 3, cpu.c 1 (the I-cache comment), dma.c 1, gte.c 1. Wrapped forms ("(PSX-SYSTEM" then "section 2, ...", and "section" then "2") were joined by hand.
- Comment-only: with block comments stripped, each of the 10 changed source files is identical to HEAD. `cmake --workflow --preset ci` and `--preset hygiene` exit 0, and `core.trace_hash_equal`, `runner.hashes_file` and `abi.symbols` pass.
- `docs/hardware/bus.md`, `cop0.md` and `cpu.md` cite psx-spx pages (CPU Specifications, Memory Map, Memory Control, Interrupts) instead of the preparation file: 38 numbered citations replaced and 8 prose references (the three "Facts come from" paragraphs, two ARCHITECTURE cost citations in bus.md, one in cop0.md, one wrapped mention in cpu.md, and a stray claim label) reworded. `dma.md`, `gte.md` and `timers.md` had none.
- `docs/hardware/cpu.md` now says the CPU takes interrupts from the vertical blank, the DMA and the timers, each raising its own `I_STAT` bit, as the COP0 page's Interrupts section describes.
- `.planning/todos/completed/030-cpu-page-interrupt-sentence.md` records WR-06 as resolved.

## Task Commits

1. **Task 1: source comments re-pointed (tracer)** - `723c4d5` (docs). `src/irq.c` went first and the ci workflow ran green before the rest.
2. **Task 2: hardware pages, interrupt sentence and todo** - `8197c0e` (docs)

**Plan metadata:** the commit that follows this summary (docs: complete plan).

## Files Created/Modified

- `src/bus.c`, `src/bus.h`, `src/cop0.c`, `src/cpu.c`, `src/cpu.h`, `src/cpu_exec.inc`, `src/irq.c`, `src/dma.c`, `src/dma.h`, `src/gte.c` - comment citations only
- `docs/hardware/bus.md`, `docs/hardware/cop0.md`, `docs/hardware/cpu.md` - citations moved to psx-spx, the CPU interrupt sentence
- `.planning/todos/completed/030-cpu-page-interrupt-sentence.md` - WR-06, resolved

## Decisions Made

- Source comments name the page and one of its real headings. The old subsection names did not all match the page headings ("Load delay" is "Delay latches" on `cpu.md`, "I-cache" is "Instruction cache" on `bus.md`), so they were mapped, and a script checked that every `docs/hardware/<page>.md, <heading>` in `src/` names a heading that exists (it found three that did not and they were fixed).
- In the hardware pages the cost model citations of the preparation file's ARCHITECTURE section were replaced by saying the figure is the core's own cost model, since a public page cannot cite a planning file.
- The psx-spx subsection names kept in the hardware pages are the ones the replaced citations used; they were not re-read against psx-spx in this plan (the plan asked for a re-pointing, not a re-verification of each fact).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The gate found 73 mentions in src/, not 66, and one file outside the plan's list**
- **Found during:** Task 1
- **Issue:** Plans 03-08 to 03-19 added comments that cite PSX-SYSTEM section 4 in `src/dma.h` (3 mentions), `src/dma.c` (1) and a PSX-GPU-AND-GTE reference in `src/gte.c` (1) after the plan's count was taken. `src/dma.h` is not in the plan's `files_modified`, but the task's acceptance criterion requires the gate to find none in `src/`.
- **Fix:** Re-pointed them with the rest, including `src/dma.h`.
- **Files modified:** `src/dma.h`, `src/dma.c`, `src/gte.c`
- **Verification:** the citation gate exits 0 with no output.
- **Committed in:** `723c4d5`

**2. [Rule 3 - Blocking] Dangling references left by the re-pointing**
- **Found during:** Tasks 1 and 2
- **Issue:** Citations of the form "section 2, SR bits; section 6" and "section 3, section 9" lost their file name when the first part moved, and a bare "C4" claim label in `bus.md` meant nothing without its source.
- **Fix:** Rewrote them: "SR and Interrupts", "docs/hardware/bus.md; PSX-SYSTEM section 9" (section 9 stays citable and keeps its number on the same line as the file name), "psx-spx Interrupts"; the label was dropped.
- **Verification:** grep for dangling "section N" in `src/` and `docs/hardware/` finds only ARCHITECTURE and the allowed PSX-SYSTEM section 9.
- **Committed in:** `723c4d5`, `8197c0e`

---

**Total deviations:** 2 auto-fixed (both Rule 3)
**Impact on plan:** Both were needed for the plan's own gate to pass. No behaviour changed and no scope was added.

## Issues Encountered

None. The first replacement pass mapped some multi-line citations to the wrong page (it read only the first line of the subsection name); the heading check script found and fixed them before the commit.

## Not run locally

`asan` and `be32` need clang-21, which this host lacks; hosted CI runs them. The change is comment and prose only (comment-stripped sources are identical to HEAD), so neither can see it.

## Known Stubs

None.

## Threat Flags

None. No code, endpoint or schema changed.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 03-23 can shrink PSX-SYSTEM sections 2 to 6 and PSX-GPU-AND-GTE section 5 to pointers: nothing in `src/` or `docs/hardware/` cites them. Tests in `tests/` still cite them (62 mentions at plan time) and belong to plan 03-22.

## Self-Check: PASSED

- `src/` citation gate: exit 0, empty.
- `! grep -nE 'PSX-SYSTEM|PSX-GPU-AND-GTE|PSXDEV-EXTRACTS' docs/hardware/*.md`: exit 0.
- Commits `723c4d5` and `8197c0e` are ancestors of HEAD; `.planning/todos/completed/030-cpu-page-interrupt-sentence.md` exists.

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*
