---
phase: 10-pbp-disc-images
plan: "13"
subsystem: media
tags: [PBP, PSMF, CI, WASI, verification]
requires:
  - phase: 10-pbp-disc-images
    provides: bounded PBP provider, runner and libretro integrations
provides:
  - Clean Phase 10 source review with the Nightly fixture dependency resolved
  - Passing final macOS arm64 local workflow, including release package checks
  - Phase 10 main CI, release pipeline and v0.10.0 install-line verification
affects: [11-ecm-clonecd]
actuals:
  tokens: 4413
  tasks: 2
  commits: 1
commits: 1
plan_head_before: cc46618447594d350c6b4c86d076b71b9650c40f
plan_head_after: e9ce168
tech-stack:
  added: []
  patterns:
    - Cheap inventory, wiring and budget checks before the full workflow
key-files:
  created:
    - .planning/phases/10-pbp-disc-images/10-VERIFICATION-EVIDENCE.md
    - .planning/phases/10-pbp-disc-images/10-CI-EFFICIENCY.md
  modified:
    - tests/compat/tests.cmake
    - .planning/phases/10-pbp-disc-images/10-REVIEW.md
key-decisions:
  - Keep the 3,901-line cap and defer the generic SPDX scanner.
  - Set the Phase 10 WASI size bound to the measured 875,971-byte linked module.
  - Run exact-head hosted CI and Nightly once at the ship PR to avoid a duplicate full workload.
  - Keep the full current-main and release CI gates because the artifact-reuse review found different archive bytes.
requirements-completed:
  - PBP-01
coverage:
  - id: D1
    description: Malformed PBP probe reports invalid metadata without leaking private input, and the local full workflow passes.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci --no-tests=error -R '^compat[.]probe[.]malformed$' --output-on-failure"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci (655 tests and 7 release checks)"
        status: pass
    human_judgment: false
  - id: D2
    description: Exact-head hosted CI, Nightly, hash/fuzz evidence and linked WASI size.
    requirement: PBP-01
    verification:
      - kind: other
        ref: "CI 44954051682 on PR head 22c5ca7; Nightly 44947489816 and CI 44947489771 on code head df83881; main CI 44961621903 and release pipeline 44961621930 on release SHA 636ac89; v0.10.0 has 19 verified assets and install-line-latest passed."
        status: pass
    human_judgment: false
duration: 38m
completed: 2026-11-22
status: complete
---

# Phase 10 Plan 13: Review and final verification summary

**The Phase 10 review is clean, malformed-PBP refusal preserves input redaction, and the complete local macOS workflow passes.**

## Performance

- **Duration:** About 38 minutes from the prior saved state checkpoint; active time was not measured.
- **Started:** 2026-11-21T15:15:19Z (prior saved state checkpoint)
- **Completed:** 2026-11-21T15:53:05Z
- **Tasks:** 2 plan tasks; the exact-head hosted run is scheduled with phase ship.
- **Files modified:** 7, including the phase state and roadmap.

## Accomplishments

- Resolved the reviewed Nightly PBP fixture dependency warning in commit `76bdc68`; recorded its clean disposition in `cc46618`.
- Fixed the compatibility probe to expect malformed PBP metadata rejection while retaining the no-stdout and path-redaction checks. Focused review and the focused selector passed.
- Ran the full local CMake CI workflow on the corrected content: all 655 regular tests and all 7 release checks passed in 78 seconds. The build step was incremental.
- Kept the repository gate at 3,900/3,901 lines. The WASI size bound remains unchanged because this Mac has no WASI sysroot or linked module.

## Task Commits

1. **Task 1: Review the phase before final local verification** — `76bdc68` (workflow fix), `cc46618` (review disposition).
2. **Task 2: Correct the malformed-PBP redaction probe** — `e9ce168` (test-only fix).

The plan ledger recorded one commit after `cc46618`; the two earlier Phase 10 review commits were already present at this executor's start.

**Plan metadata:** separate documentation/state commit follows this summary.

## Files Created/Modified

- `tests/compat/tests.cmake` — expects supported PBP parsing to reject the malformed fixture as invalid metadata, without disclosing its path.
- `10-REVIEW.md` — records the follow-up test review and verification.
- `10-VERIFICATION-EVIDENCE.md` — local evidence and the exact-hosted run boundary.
- `10-CI-EFFICIENCY.md` — measured local critical path, historical hosted baseline, and efficiency disposition.

## Decisions Made

- Keep the 3,901-line ceiling and defer the generic SPDX scanner, as the owner selected.
- Set the WASI bound from the measured 875,971-byte linked PBP-enabled producer on hosted run 44924014258; the later repaired-head WASI producer accepted the same value.
- Keep exact-head CI and Nightly attempts consolidated through the ship PR; cancel an obsolete CI tail once a deterministic failure is fixed locally.

## Deviations from Plan

### Auto-fixed issues

**1. [Rule 1 - Bug] Updated a stale malformed-PBP test expectation.**
- **Found during:** Task 2.
- **Issue:** The compatibility probe still expected all `.pbp` files to be unsupported after this phase added PBP parsing.
- **Fix:** Require the existing invalid-metadata diagnostic and retain stdout/path redaction assertions.
- **Files modified:** `tests/compat/tests.cmake`.
- **Verification:** Focused selector passed 2/2; the corrected full workflow passed 655/655 plus 7/7 release checks.
- **Committed in:** `e9ce168`.

**2. [Plan sequencing] Defer exact-head hosted measurements to the single ship PR run.**
- **Issue:** Starting a standalone hosted CI and Nightly run now would duplicate the required ship-PR workloads. Phase rules put ship after formal verification.
- **Disposition:** Record all local evidence now; Phase 10's exact-head CI/Nightly run, hosted lane inventory, Actions timing and linked WASI size remain explicit pending fields in the evidence and coverage metadata. Parent orchestration will inspect these runs at ship and update the evidence before release completion.

**Total deviations:** 2 (one test expectation fix; one hosted-run sequencing adjustment).
**Impact on plan:** No acceptance cases were removed. PBP-01 remains pending until the hosted exact-head requirements are established.

## Issues Encountered

The first broad local workflow run failed only because the malformed PBP compatibility test had a pre-feature expectation. After the focused correction and review, the one final full local workflow passed. The host cannot build the WASI target; `quality.json` was not changed. Exact-head hosted evidence remains pending the ship PR.

## User Setup Required

None. No owner-only verification is required.

## Self-Check: PASSED

- Summary, review addendum, verification evidence and CI-efficiency evidence exist.
- Task commit `e9ce168` is present on the phase branch.
- Exact-head hosted CI/Nightly and the linked WASI size remain explicitly pending for ship.

## Next Phase Readiness

Ready for formal Phase 10 verification. At ship, run and inspect one exact-head CI and Nightly attempt, extract the actual linked WASI module size from the hosted job log, and update the evidence before confirming release. No user action is needed.

---
*Phase: 10-pbp-disc-images*
*Completed: 2026-11-21*

## Ship follow-up: cross-platform CI repairs (2026-11-21)

The first PR CI attempt (`50efc31`) and its Nightly run exposed five causes before release: a stale hygiene
classifier-output comparison, the old WASI size bound, a pointer-to-integer assertion in the BE32 host test,
MSVC's C4996 rejection of direct `fopen`, and an omitted `date_days.cmake` dependency in the protected-base
sparse checkout. Those failures were fixed together. The WASI job's actual linked module measured 875,971
bytes; `quality.json` now records that measurement and its run/job URL. The run was cancelled after its failing
checks were diagnosed; it is not counted as passing hosted evidence.

An independent review found the correction diff clean. Cheap workflow, line-budget, quality, inventory and
route checks passed; the full local workflow passed 655/655 tests and 7/7 release checks. The focused
protected-ratchet tests passed with a complete base checkout. The repaired head still requires one exact-head
hosted CI and Nightly run before release. The failed-attempt timing and local evidence are recorded in
`10-CI-EFFICIENCY.md` and `10-VERIFICATION-EVIDENCE.md`.

## Second ship-fix follow-up (2026-11-21)

PR CI 44927726830 exposed a missing PBP provider link in the SDL2 example; Nightly 44927726826 exposed that the
full fuzz build preset omitted the PBP fixture and seed validation occurred after earlier timed campaigns. The
example now builds the provider and inflater; Nightly builds the PBP fixture and validates all seeds before
starting any campaign. Focused local verification passed: the SDL2 example linked, PBP replay wrote 14 seeds,
the workflow-closure check and actionlint passed. Independent review found the seed-order assertion itself was
too weak: a literal `||` was parsed as regex alternation, and the loader check was ordered before `seed_output`.
The assertion now treats `||` literally and places the loader requirement after loop closure; the real workflow
passes and a temporary inside-the-loop campaign regression is rejected. Both hosted attempts remain
failed/cancelled evidence, not ship approval; rerun the complete required CI and Nightly on the next head.

## Third ship-fix follow-up (2026-11-21)

The next exact-head Nightly run failed before fuzz campaigns because Ninja had no rule for media outputs used by
the PBP fixture. `DiscFixtures.cmake` now declares `cdda.wav`, `movie.str`, and `xa.xa` as byproducts of the
`media.sha256` generator. A fresh local configure with no external media directory and direct Ninja producer
queries passed. Independent review found and we fixed a false-positive path in the static guard; it now checks
byproducts only in the `media.sha256` command block, and a temporary misplaced-output fixture is rejected. The
guard stays within the 3,901-line cap. Exact-head CI and Nightly still need to pass before ship.

## Final exact-head hosted verification (2026-11-21)

PR CI [44947489771](https://github.com/szTheory/schwaystation/actions/runs/44947489771) and Nightly
[44947489816](https://github.com/szTheory/schwaystation/actions/runs/44947489816) passed on
`df838815a5a4afd99f95d50e8caf66813aaa7cb4`. The WASI module passed the recorded 875,971-byte bound; the
successful comparison does not emit its newly measured exact byte count. CI took 63m30s wall/8,250 runner-
seconds with BE32 at 62m18s; Nightly took 9m43s/908 runner-seconds. Comparison with the previous complete run
did not support a coverage-preserving workflow reduction. The details and remaining release boundary are in
`10-VERIFICATION-EVIDENCE.md` and `10-CI-EFFICIENCY.md`.

## Phase 10 release and main verification (2026-11-22)

The phase PR #34 merged as `9bf39aa`; release PR #35 then merged `v0.10.0` at
`636ac89d4add3192520084a73fa61295a8422bbc`. Current-main CI
[44961621903](https://github.com/szTheory/schwaystation/actions/runs/44961621903) and the full Release
workflow [44961621930](https://github.com/szTheory/schwaystation/actions/runs/44961621930) both passed on that
same SHA. Main CI took 85m51s wall and 9,207 summed job-seconds; BE32/QEMU was the critical path at 5,095s.
The release artifact matrix used about 6,150 summed runner-seconds; its BE32 lane took 2,520s. The release was
published as Latest with all 19 expected assets. Signature/download verification and the `install-line-latest`
check passed. These are elapsed job-time measurements; GitHub exposed no dollar billing data.

The metadata-only release PR correctly took CI's fast route, but Nightly classified its three generated files
as relevant and spent 419 runner-seconds before cancellation. A narrow future change should route only the
complete exact set `.release-please-manifest.json`, `CHANGELOG.md`, `version.txt` away from PR Nightly. Preserve
the full route for mixed, incomplete, unknown or untrusted file lists. Existing CI and release guidance is
already discoverable in `AGENTS.md`, `.planning/AUTONOMOUS.md`, and `docs/maintaining.md`.
