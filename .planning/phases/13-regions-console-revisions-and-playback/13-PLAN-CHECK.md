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

All nine plans pass `verify.plan-structure`; each has two tasks with complete files, actions, verification and done criteria. Plan-level file lists cover the task file lists. Dependencies are acyclic and waves are consistent. The shared CI build tree is serialized through Waves 1–9, and Plan 05 declares Plan 04's board-aware state/hash prerequisite.

Plan 02 uses the existing Nightly preset with `SCHWAYSTATION_MEDIA_REFERENCE_ONLY=mdec`. Task 1 configures that lane and asserts the exact four pre-existing baseline/preflight/budget selector names; Task 2 sorts the Nightly inventory, configures the same lane, asserts exactly `{mdec.reference.input_preflight, mdec.reference.edge}` before building, then runs the focused CTest selection. The Nightly configure/build/test presets and target exist. The MDEC-only branch already calls `schway_register_mdec_reference_tests`, so no edit to `tests/nightly/tests.cmake` is needed. Plan 02 task files exactly match its frontmatter; it depends on Plan 01 and remains in Wave 2.

Both revised Plan 02 command bodies pass a fresh `bash -n`; the other 16 are unchanged from the prior validated pass. All commands have `[VALIDATED: bash -n exit 0]` records. Previously supplied path/failing-direction probe results showed no findings for the then-current 18 commands; those probes were not rerun in this bounded audit.

Nyquist checks 8a–8e are not applicable because this phase's RESEARCH.md has no `Validation Architecture` section. Per-plan calibrated estimates range from 9,000 to 15,000 tokens, below the 100,000-token budget. All have low confidence because this project has no completed phase actuals for calibration.

CI/CD sequencing keeps cheap inventory, selector, fixture and budget checks ahead of expensive work. The plans retain the full local CI workflow and required hosted Main/Release gates, inspect hosted critical path and runner cost, and use `act` only when it adds evidence or checks changed routing. Docker cleanup is scoped to task-owned, identifiable resources.

The prior review findings are resolved: the MDEC selector set is asserted before its guest runs; the illegal profile tuple matrix is exhaustive; validate-only records are present; Plan 05 follows Plan 04; and Plan 03 no longer lists the unused `src/spu.c` path.

## Plan summary

| Plan | Tasks | Wave | Dependencies | Status |
|---|---:|---:|---|---|
| 13-01 | 2 | 1 | — | Valid |
| 13-02 | 2 | 2 | 13-01 | Valid |
| 13-03 | 2 | 3 | 13-02 | Valid |
| 13-04 | 2 | 4 | 13-03 | Valid |
| 13-05 | 2 | 5 | 13-01, 13-03, 13-04 | Valid |
| 13-06 | 2 | 6 | 13-03, 13-05 | Valid |
| 13-07 | 2 | 7 | 13-04, 13-05, 13-06 | Valid |
| 13-08 | 2 | 8 | 13-05, 13-06, 13-07 | Valid |
| 13-09 | 2 | 9 | 13-01 through 13-08 | Valid |

Plans verified. Run `$gsd-execute-phase 13` when ready to execute.
