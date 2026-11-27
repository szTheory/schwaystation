---
phase: 11-ecm-and-clonecd-images
plan: "12"
subsystem: ci-media-fixtures
tags: [ecm, clonecd, ci, nightly, preflight]

requires:
  - phase: 11-11
    provides: Bounded guest evidence and protected provider ledger.
provides:
  - Nightly seed and fixture producer closure before bounded timed campaigns.
  - Routing controls and local evidence for the ECM/CCD campaign preflight.
affects: [ECM-01, CCD-01, phase-11-review]

actuals:
  tasks: 1
  commits: 1
commits: 1
plan_head_after: 4fbb8b6348bc9c58053b048f946aebdd98f492f7

tech-stack:
  added: []
  patterns:
    - Keep fixture/source extent and selector preflights ahead of timed campaigns.
    - Use the existing route selector and act dry-run to inspect workflow routing without starting campaigns.

key-files:
  created: []
  modified:
    - .github/workflows/ci.yml
    - .github/workflows/nightly.yml
    - tests/cmake/workflows.cmake
    - tests/disc/legacy.c
    - docs/maintaining.md
    - docs/media-testing.md
    - docs/testing.md
    - .planning/phases/11-ecm-and-clonecd-images/deferred-items.md

key-decisions:
  - Preserve the 3,901 counted-line cap and defer the generic SPDX scanner.
  - Keep Main, Nightly and Release hosted evidence separate for final phase review.
  - Move the unstarted CHD-OFF package and installed SDL2/CHD-ON validation into Plans 11-14 and 11-15.

patterns-established:
  - Validate campaign source extents, produced counts and fixture selectors before timed work.
  - Record local routing evidence without treating it as hosted workflow evidence.

requirements-completed: []
coverage:
  - id: NIGHTLY-FIXTURE-PREFLIGHT
    description: Nightly routes build ECM/CCD fixture producers and check producer/seed closure before timed campaign execution.
    requirement: ECM-01, CCD-01
    verification:
      - kind: integration
        ref: "cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake; act schedule --dryrun -W .github/workflows/nightly.yml"
        status: pass
    human_judgment: false

duration: 33min 25sec
completed: 2026-11-22
status: complete
---

# Phase 11 Plan 12: Nightly fixture preflight and routing

**Nightly producer and seed checks now run before timed ECM/CCD campaigns; the package and optional-host validation remains assigned to separate follow-up plans.**

## Performance

- **Duration:** 33 minutes 25 seconds, measured from 2026-11-22T19:00:32Z to 19:33:57Z.
- **Runtime/model:** Codex with OpenGSD 1.20.0; configured executor `gpt-6-luna`; primary runtime model ID unavailable.
- **Tasks:** 1 completed in this scoped plan. The original unstarted Tasks 2–3 were moved to Plans 11-14 and 11-15.
- **Bottleneck:** The planned validation scope exceeded the project's 30-minute plan limit; the Nightly routing slice itself stayed bounded. No emulation campaign ran.

## Verification Evidence

| Command/check | Exit | Result |
| --- | ---: | --- |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | 0 | 3,899 of 3,901 counted lines |
| Positive `disc_registration` check and temporary negative mutations removing a fixture producer or moving a campaign before preflight | 0 / 1 / 1 | Positive route passed; both invalid orderings were rejected |
| `actionlint` for CI and Nightly workflows | 0 | Both changed workflows passed; combined all-workflow lint reported only the existing Release `concurrency.queue: max` warning |
| `act schedule --dryrun -W .github/workflows/nightly.yml` with the configured runner mappings | 0 | Route included producers and source/replay preflight ahead of the bounded campaign step; no campaign executed |
| Fresh ASan/UBSan legacy fixture build and focused `legacy_preflight` / `legacy_parity` selector | 0 | Both checks passed after NUL-terminating the manifest buffer |

The change is committed as `ef50063`; the partial-execution checkpoint is `4fbb8b6`. There was no hosted CI, full suite, timed campaign or Docker run. Main, Nightly and Release hosted evidence remains distinct and is deferred to final phase review.

## Scope and Requirement Status

This summary closes Task 1 only. The original CHD-OFF package/evidence checks and installed SDL2 example/CHD-ON checks were not started and now belong to Plans 11-14 and 11-15. Cross-platform evidence and hosted Main/Nightly/Release checks remain with final phase review. The feature ledger and docs will be updated from observed results. Neither ECM-01 nor CCD-01 is complete; both remain phase-level requirements pending provider, package, host and final verification evidence.

## Next Phase Readiness

Plan 11-14 is next for CHD-OFF package acceptance. Plan 11-15 follows for installed SDL2 example and CHD-ON consumers. After those focused plans, the phase still needs source review and final verification/ship evidence. No owner action is needed.

---
*Phase: 11-ecm-and-clonecd-images*
*Completed: 2026-11-22*
