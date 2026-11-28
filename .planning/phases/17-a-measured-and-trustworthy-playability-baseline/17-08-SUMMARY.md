---
phase: 17-a-measured-and-trustworthy-playability-baseline
plan: "08"
subsystem: documentation-quality
tags: [maintainer-docs, performance-evidence, compatibility-audit]
requires:
  - phase: 17-07
    provides:
      - Selected gpu_motion frame-120 compatibility predicate and measured evidence
provides:
  - Current measured-evidence guidance with repaired archived links
  - Final dispositions for every original documentation audit item
  - Plan 17-09 compatibility freshness handoff with input identities
affects: [17-09]
actuals:
  tokens: 5033
  tasks: 3
  commits: 2
  plan_head_before: 0535e879f717415c0fbce12ebf4a71c15905cc57
  plan_head_after: e02dacb8a896db3993284d7040ded4567862b80a
tech-stack:
  added: []
  patterns:
    - Canonical quality registry links with producer provenance in the audit report
key-files:
  created:
    - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-08-SUMMARY.md
  modified:
    - docs/testing.md
    - .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-DOC-AUDIT.md
key-decisions:
  - Keep README installation, maintainer policy, and all five audited hardware pages unchanged because their audits found no drift.
  - Keep generated docs/compatibility.md read-only here; Plan 17-09 owns its final regeneration and freshness check.
requirements-completed: []
coverage:
  - id: P17-08-A
    description: Testing guidance points to current producer evidence, labels assumptions correctly, and links to archived reports.
    requirement: TUNE-10
    verification:
      - kind: other
        ref: "cmake -DROOT=. -P tests/cmake/budgets.cmake && git diff --check origin/main — exit 0; changed-link path check — exit 0"
        status: pass
    human_judgment: false
  - id: P17-08-B
    description: Every original documentation drift row has a final disposition and Plan 17-09 owns compatibility freshness.
    requirement: TUNE-11
    verification:
      - kind: other
        ref: "git diff --check origin/main and test -s .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-DOC-AUDIT.md — exit 0; input blob identities recorded"
        status: pass
    human_judgment: false
  - id: P17-08-C
    description: README install guidance, maintainer policy, and five audited hardware pages remain unchanged after review.
    requirement: TUNE-11
    verification:
      - kind: other
        ref: "git diff --exit-code -- README.md docs/maintaining.md docs/hardware/gpu.md docs/hardware/spu.md docs/hardware/cdrom.md docs/hardware/mdec.md docs/hardware/sio.md docs/compatibility.md — exit 0"
        status: pass
    human_judgment: false
metrics:
  duration: 16min
  completed: 2026-11-28
  status: complete
  commits: 2
  plan_head_before: 0535e879f717415c0fbce12ebf4a71c15905cc57
  plan_head_after: e02dacb8a896db3993284d7040ded4567862b80a
---

# Phase 17 Plan 08: Documentation Freshness Summary

**Current producer references and four archived links are corrected; compatibility and hardware claims retain their measured scope.**

## Performance

- **Runtime/model:** Codex with gpt-6-luna; OpenGSD 1.23.0.
- **Duration:** About 16 minutes active, within the 25-minute estimate.
- **Started:** Approximately 2026-11-28T04:26:00Z.
- **Completed:** 2026-11-28T04:42:10Z.
- **Tasks:** 3/3.
- **Files modified:** 3, including this summary.

## Accomplishments

- Replaced duplicated stale metric figures with the canonical `quality.json` registry and linked the Phase 17 producer report for exact outputs, configurations, dates, and the unavailable exact hosted WASI value.
- Corrected all four broken Phase 7 links to the v2.0 archive. Added the current limits: low-power rates are assumptions pending named-device evidence, and pinned Wasmtime coverage checks deterministic hashes rather than browser speed.
- Corrected the pinned-input consumer description. The audit dispositions every original row, confirms README and maintainer guidance need no edit, and leaves GPU, SPU, CD-ROM, MDEC, and SIO pages unchanged.
- Recorded compatibility input/output blob identities and left final generated documentation freshness to Plan 17-09. The frame-120 row remains a single fixture hash; required capabilities stay unknown and libretro stays experimental.

## Task Commits

1. **Task 1: Refresh measured installation and maintainer guidance** — `0523567` (`docs`).
2. **Task 2: Reconcile affected hardware guidance** — no files changed after review; no commit required.
3. **Task 3: Close the drift inventory and hand off generated compatibility freshness** — `e02dacb` (`docs`).

The plan ledger measured two task commits from `0535e879f717415c0fbce12ebf4a71c15905cc57` through `e02dacb8a896db3993284d7040ded4567862b80a`. A separate metadata commit records this summary.

## Files Created/Modified

- [docs/testing.md](../../../docs/testing.md) — current producer/source links, metric caveats, low-power assumptions, and pinned-input consumers.
- [17-DOC-AUDIT.md](17-DOC-AUDIT.md) — final dispositions for all original audit rows and Plan 17-09 freshness handoff.
- [17-08-SUMMARY.md](17-08-SUMMARY.md) — execution and verification record.

## Verification

- `cmake -DROOT=. -P tests/cmake/budgets.cmake && git diff --check origin/main && test -s .planning/phases/17-a-measured-and-trustworthy-playability-baseline/17-DOC-AUDIT.md` — exit 0. The final `docs/testing.md` size is 61,435 bytes against its 61,440-byte limit.
- Changed relative targets in `docs/testing.md` and `17-DOC-AUDIT.md`, plus the `maintaining.md#pinned-downloads` heading, were checked with a one-off Python `Path.is_file` check — exit 0.
- `git diff --exit-code -- README.md docs/maintaining.md docs/hardware/gpu.md docs/hardware/spu.md docs/hardware/cdrom.md docs/hardware/mdec.md docs/hardware/sio.md docs/compatibility.md` — exit 0; unaffected pages remain unchanged.
- The existing installer proof was reused from Plan 17-01: [release workflow run 45923155197](https://github.com/szTheory/schwaystation/actions/runs/45923155197). The install line did not change, so no new download was made.
- No build or product test suite ran; this plan is documentation-only.

## Decisions Made

- Keep the existing README installer and `docs/maintaining.md` as audited. The current installer evidence and no-Dependabot-auto-merge, BE32, and Nightly dispositions still match the source audits.
- Keep all five hardware pages byte-for-byte unchanged. The added `gpu_motion` frame-120 fixture does not measure hardware conformance or device performance.
- Keep `docs/compatibility.md` untouched in this plan; Plan 17-09 owns final generation and freshness.

## Deviations from Plan

None. The edited guidance fits its existing byte budget; no byte limit or verification gate changed.

## Issues Encountered

The first draft of `docs/testing.md` exceeded its existing byte limit. It was tightened before commit to 61,435 bytes, and the exact budget check then passed.

## User Setup Required

None.

## Next Phase Readiness

Plan 17-09 can run the final compatibility generator/freshness check against the recorded input identities, then finish the phase-level roadmap, prompt, and lesson reconciliation. No user action is needed for this plan.

---
*Phase: 17-a-measured-and-trustworthy-playability-baseline*
*Completed: 2026-11-28*

## Self-Check: PASSED

- Summary file exists and is nonempty.
- Task commits `0523567` and `e02dacb` are ancestors of the current branch head.
- The plan ledger measures two task commits from `0535e879f717415c0fbce12ebf4a71c15905cc57` through `e02dacb8a896db3993284d7040ded4567862b80a`.
