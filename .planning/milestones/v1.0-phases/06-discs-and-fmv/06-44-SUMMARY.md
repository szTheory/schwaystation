---
phase: 06-discs-and-fmv
plan: "44"
subsystem: verification
tags: [phase-review, local-acceptance, mdec, cdrom, ci]
requires: ["06-43"]
provides:
  - "Reviewed-source fingerprint and resolved review dispositions for the revised v1.0 MDEC claim."
  - "Complete local Phase 6 CI, hygiene, replay, MDEC and CD-ROM reference acceptance evidence."
affects: ["06-45 hosted acceptance", "06-23/24 local acceptance" ]
tech-stack:
  added: []
  patterns:
    - "Record a content fingerprint for the exact reviewed and locally verified source snapshot."
    - "Keep hardware-only fidelity limits separate from deterministic implementation regression and source coverage."
key-files:
  created:
    - .planning/phases/06-discs-and-fmv/06-44-SUMMARY.md
  modified:
    - .planning/phases/06-discs-and-fmv/06-REVIEW.md
    - .planning/phases/06-discs-and-fmv/06-VERIFICATION.md
    - .planning/STATE.md
    - .planning/ROADMAP.md
key-decisions:
  - "The revised Phase 6 v1.0 contract has passed local acceptance; hosted exact-head and release checks remain required."
  - "Exact 8-bit and full frame/movie hardware output remain unproved and are not claimed."
requirements-completed: []
coverage:
  - id: PHASE6-SOURCE-REVIEW
    description: "The source and contract review found no open implementation findings after fixes; WR-09 and WR-10 were re-reviewed and resolved. The current reviewed non-planning worktree has a stable fingerprint."
    requirement: DISC-03
    verification:
      kind: other
      ref: "06-REVIEW.md Plan 06-44 addendum; fingerprint e3b37688944e0d076f56914547cd2718743e4df6e6e2d3d7d533e6f0a4424752 across 157 changed non-planning files. Source review preceded final local acceptance; only planning/state/evidence bookkeeping changed afterward."
      status: pass
    human_judgment: false
  - id: PHASE6-COMPLETE-LOCAL-CI
    description: "The full CI workflow and release lane passed on the reviewed source snapshot, including complete disc boot, owned media, frozen API, libretro, runner, example, MDEC and fuzz coverage."
    requirement: DISC-01
    verification:
      kind: integration
      ref: "cmake --workflow --preset ci (exit 0; 611/611 tests; ci-release 5/5); ctest --test-dir build/ci -L disc --no-tests=error --output-on-failure (exit 0; 25/25); ctest --test-dir build/ci -L mdec --no-tests=error --output-on-failure (exit 0; 12/12); ctest --test-dir build/ci -R '^fuzz[.]regress[.](cue|cnf|m3u|cdrom|mdec)$' --no-tests=error --output-on-failure (exit 0; 5/5)."
      status: pass
    human_judgment: false
  - id: PHASE6-HYGIENE
    description: "The complete repository hygiene workflow passes, including source, pin, inventory, commit and gate-budget checks."
    requirement: DISC-04
    verification:
      kind: integration
      ref: "cmake --workflow --preset hygiene (exit 0; 24/24). `libretro.vendored` also passed 1/1 after restoring the pinned file's original bytes."
      status: pass
    human_judgment: false
  - id: DISC03-LOCAL-MDEC-AND-CDROM-REFERENCES
    description: "Every surviving nightly MDEC and CD-ROM reference passes, and all main-CI/nightly/media-only inventories expose the expected names and labels."
    requirement: DISC-03
    verification:
      kind: integration
      ref: "ctest --test-dir build/mdec-reference -L mdec --no-tests=error --output-on-failure (exit 0; 12/12); its JUnit inventory check passed. `cmake --build build/cdrom-reference --target schwaystation-test-cdrom-reference` (exit 0); `ctest --test-dir build/cdrom-reference -R '^cdrom[.]reference[.](timing|getloc|controls)$' --no-tests=error --output-on-failure` (exit 0; 3/3); its JUnit check passed. Parsed `ctest --show-only=json-v1` and compared names and labels against the inventories: main CI, nightly, MDEC-only and CD-ROM-only each matched all 12 applicable MDEC selectors; CD-ROM-only did not prepare the MDEC corpus."
      status: pass
    human_judgment: false
  - id: PHASE6-HOSTED-ACCEPTANCE
    description: "Exact-head hosted CI/nightly and background RetroArch acceptance, ship, merge and release checks remain assigned to the next plan."
    requirement: DISC-04
    verification:
      kind: e2e
      ref: "No hosted run URL exists yet. Plan 06-45 owns the exact-head workflow runs and RetroArch screenshot-to-runner comparison, then the authorized ship/auto-merge and release confirmation."
      status: pending
    human_judgment: false
---

# Phase 6 Plan 06-44: Review and complete local acceptance

**The reviewed Phase 6 source passes the complete local CI and hygiene workflows, all surviving disc/MDEC references, and the media replay checks. Hosted acceptance remains next.**

## Performance and runtime

- **Duration:** Active time not tracked; execution and evidence review resumed on 2026-11-17.
- **Runtime/model:** Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID not exposed).
- **Commit:** `e47a14a` (`feat(phase-06): integrate disc media and MDEC support`), created with the repository GSD helper after the local gates passed.
- **Hosted run:** None yet; Plan 06-45 owns hosted acceptance.

## Local acceptance

- The reviewer rechecked the revised claim, selector wiring, source guards, original expected hardware bytes, unchanged arithmetic and cycle cap, direct model controls, and the preserved owned STR DMA-to-display path. No open implementation finding remains in the reviewed diff. Exact hardware output limits remain explicit.
- The reviewed source fingerprint is recorded in `06-REVIEW.md` and `06-VERIFICATION.md`. No source, test, build or user-documentation change followed local acceptance.
- Full local CI passed 611/611, the release lane 5/5, hygiene 24/24, the disc label 25/25, main-CI MDEC 12/12, fuzz replays 5/5, nightly MDEC references 12/12, and CD-ROM references 3/3.
- All four CTest graph comparisons matched their inventories. The CD-ROM-only graph listed the same MDEC selectors without downloading or preparing MDEC assets.
- The local portions of the previously pending 06-23/24 acceptance are satisfied. Hosted RetroArch, exact-head nightly, ship and release acceptance are still pending Plan 06-45.

## Next

Plan 06-45 validates the committed exact head on hosted CI/nightly and in a background RetroArch session, then completes the already-authorized ship, merge and release workflow.
