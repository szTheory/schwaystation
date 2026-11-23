# Phase 13 Plan Check

**Phase:** Regions, console revisions and playback  
**Plans verified:** 9  
**Status:** VERIFICATION PASSED

## Coverage summary

| Requirement | Plans | Status |
|---|---|---|
| PROFILE-01 | 05, 06 | Covered: host-only region resolution, explicit override and fallback, PAL/NTSC timing, audio cadence and portable hash regressions. |
| PROFILE-02 | 01, 03, 04, 05, 06, 09 | Covered: all nine legal and nine illegal board/GPU pairs, derived defaults, legacy profile behavior, state validation/serialization/hash, host selection and tested-tuple ledger evidence. |
| BOOT-01 | 07, 08, 09 | Covered: full BIOS boot default, narrow opt-in hook, BIOS and eligible-disc checks in both hosts, and separate standalone EXE/PS-EXE paths. |
| MDEC-01 | 02, 09 | Covered: pinned-source preflight before the bounded edge guest, separate source/decoder/guest/display results, preserved 4-bit and 8-bit baselines and cycle cap. |

All four Phase 13 requirement IDs appear in plan `requirements` metadata. The plans honor the locked context decisions and do not introduce deferred work. Board behavior comes from the shared defaults table; only modeled RAM/GPU/CD behavior changes, while SPU and port defaults remain per-instance identity. Region inference remains host-side. Fast boot is opt-in, requires BIOS-backed eligible content at the host boundary, and leaves the BIOS-free executable path intact.

The MDEC edge case is selected from pinned inputs during execution, as allowed by the context's discretion and research's fixture-confidence note. Its CTest fixture reuses the existing source-unit preflight and maintains the evidence boundaries. No source-audit gap remains in the plan.

## Plan integrity and verification

All nine plans pass `verify.plan-structure`; each has one or two tasks with complete files, actions, verification and done criteria. Plan-level file lists cover the task file lists. Dependencies are acyclic and waves are consistent. The shared CI build tree is serialized through Waves 1–9, and Plan 05 declares Plan 04's board-aware state/hash prerequisite.

Plan 02 now reuses `tests/mdec/color_reference.c::run_color_guest(movie15)` through the existing MDEC-only Nightly lane. Its single task modifies exactly `cmake/MDECReferences.cmake`, `tests/mdec/color_reference.c` and `tests/nightly/inventory.txt`; the existing Nightly file already invokes the shared registration helper. The preset, MDEC-only cache variable, target, inventory and both required fixtures align. Plan 02 depends on Plan 01 and remains in Wave 2. Its expected-incomplete result must report four distinct fields: `source=complete`, `decoder=output-fifo-empty`, `guest=expected-incomplete`, and `display=not-compared`. The plan accepts this only for the exact pinned source coverage and terminal DMA/MDEC signature, after eight consecutive empty-FIFO wait samples following successful run/TTY operations with no fault, and below the unchanged cap. This is a diagnostic result, not playback success; any completed guest still must pass both VRAM and display comparisons.

The bounded `movie15` run has already been observed as a RED outcome at 1,902,230 ticks: source coverage was complete (1,440 blocks / 240 macroblocks / zero trailing), there was no fault, the 1,696-word DMA0 command was exhausted with the MDEC FIFO empty, DMA0 idle and DMA1 waiting at remain=9/phase=2, guest `Done` was absent, and display comparison was not reached. This is the measured signature Plan 02 will classify as an expected-incomplete diagnostic. No rerun was performed for this review.

The selector assertion expects `mdec.reference.input_preflight`, `mdec.reference.color_assets` and `mdec.reference.edge`. A read-only CTest `--show-only` query of the existing `coverage_movie15` consumer confirmed CTest adds both required fixture setup tests to the selected set. The new assertion therefore accounts for fixture expansion. The task command also checks inventory sort and the 3,901-line cap before configuring/building, then runs only the bounded edge selection. No configure, build or guest test was run in this final bounded review.

The current Plan 02 command body passes a fresh `bash -n`, has a matching validate-only record, and its one task's file list exactly matches frontmatter. The other 16 commands are unchanged from the previous validated pass, making 17 current task commands total. Previously supplied path/failing-direction probe results showed no findings for their then-current version; those probes were not rerun in this bounded audit.

Nyquist checks 8a–8e are not applicable because this phase's RESEARCH.md has no `Validation Architecture` section. Per-plan calibrated estimates range from 9,000 to 15,000 tokens, below the 100,000-token budget. All have low confidence because this project has no completed phase actuals for calibration.

CI/CD sequencing keeps cheap inventory, selector, fixture and budget checks ahead of expensive work. The plans retain the full local CI workflow and required hosted Main/Release gates, inspect hosted critical path and runner cost, and use `act` only when it adds evidence or checks changed routing. Docker cleanup is scoped to task-owned, identifiable resources.

The prior review findings are resolved: the MDEC selector set is asserted before its guest runs; the illegal profile tuple matrix is exhaustive; validate-only records are present; Plan 05 follows Plan 04; and Plan 03 no longer lists the unused `src/spu.c` path.

## Plan summary

| Plan | Tasks | Wave | Dependencies | Status |
|---|---:|---:|---|---|
| 13-01 | 2 | 1 | — | Valid |
| 13-02 | 1 | 2 | 13-01 | Valid |
| 13-03 | 2 | 3 | 13-02 | Valid |
| 13-04 | 2 | 4 | 13-03 | Valid |
| 13-05 | 2 | 5 | 13-01, 13-03, 13-04 | Valid |
| 13-06 | 2 | 6 | 13-03, 13-05 | Valid |
| 13-07 | 2 | 7 | 13-04, 13-05, 13-06 | Valid |
| 13-08 | 2 | 8 | 13-05, 13-06, 13-07 | Valid |
| 13-09 | 2 | 9 | 13-01 through 13-08 | Valid |

Plans verified. Run `$gsd-execute-phase 13` when ready to execute.
