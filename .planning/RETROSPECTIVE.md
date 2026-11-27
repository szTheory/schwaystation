# Project Retrospective

*A living document updated after each milestone. Lessons feed forward into future planning.*

## Milestone: v1.0 — PlayStation games play with sound in RetroArch

**Shipped:** 2026-11-18
**Phases:** 6 | **Plans:** 195 | **Tasks:** 381

### What Was Built

- A deterministic C library, headless runner and libretro core with CPU, GPU, audio, CD-ROM, media and disk-control paths.
- Public-vector CPU and hardware GTE coverage, cross-platform output hashes, a sound path, and an owned disc that boots in RetroArch.
- Automated release CI on six platform legs plus big-endian and wasm32 validation.

### What Worked

- Each implementation phase shipped through its own pull request and automatic package release.
- Pinned public vectors, self-authored guests and hosted RetroArch runs gave useful evidence without relying on a retail BIOS or owner hardware session.
- Reusing exact-tree hosted CI evidence for the stale-verification refresh avoided rerunning the long local suite.
- The milestone audit checked real wiring across the CPU, bus, scheduler, devices, runner, libretro and media paths; it found no broken requirement flow.

### What Was Inefficient

- Phase 6 initially depended on an owner hardware capture that was not available and on complete external frame/movie references that public records did not provide. The scope was revised late. Future milestones should confirm required evidence sources and owner-hardware availability before making them critical-path gates.
- Long guest runs were attempted before compressed-source unit counts were compared with requested and produced output. A cheap source-completeness preflight now runs before those media tests; keep source coverage, decoder progress and hardware fidelity as separate claims.
- A cross-phase review found stale report wording and malformed YAML in archived summaries. Parse frontmatter, check report fingerprints and reconcile requirement IDs before the expensive milestone audit or suite.

### Patterns Established

- Another small local implementation beats adding a dependency when the standard library and existing code are sufficient.
- Run cheap structural and input-completeness checks before long emulator or CI jobs; use broad CI only for a changed source tree or recurring integration risk.
- When evidence cannot settle exact hardware behavior, preserve the original pin and discrepancy, state the accepted deterministic scope, and do not overclaim accuracy.
- Treat owner hardware as an optional evidence source unless the owner has scheduled a session; continue on public and self-authored evidence where it can answer the question.

### Key Lessons

1. Confirm public evidence, fixture completeness and owner-hardware availability during planning, before an acceptance gate depends on them.
2. Compare encoded source units with the requested and produced output before starting expensive emulation; those values answer different questions.
3. Add fast consistency checks for frontmatter syntax, requirement traceability and verification fingerprints before long test or audit passes.
4. Reuse exact-source hosted results for documentation-only verification refreshes; do not rerun broad suites without a changed source tree or a concrete recurring regression risk.
5. Keep hardware-conformance claims narrower than deterministic model coverage when no complete hardware record exists.

### Cost Observations

- Model mix and session count: not measured consistently; exact active time was not available.
- Timeline: first phase release PR #4 merged 2026-10-29; final phase PR #22 merged 2026-11-18.
- Notable: planning and integration evidence became the critical path late in Phase 6; source-completeness preflights and the revised public-evidence plan removed the dependency on an unscheduled hardware capture.

---

## Milestone: v2.0 — The whole library

**Shipped:** 2026-11-26
**Phases:** 10 | **Plans:** 125 | **Tasks:** 221

### What Was Built

- Extended the deterministic C library, headless runner and libretro core with CHD, PBP, ECM, CloneCD/IMG/SUB and SBI/LibCrypt support.
- Added per-game card lifecycle, multitap and special controller protocols, PAL/revision profiles, opt-in fast boot, host-side PPF/BPS overlays, and bounded GameShark support.
- Added a pinned 13,407-entry DAT inventory and a 51-row required-capability matrix that keeps untested combinations unknown and libretro experimental.
- Published v0.16.0 after all ten phase PRs, main CI and the full release workflow completed.

### What Worked

- Each phase shipped through its own PR and release; the source and release PRs both merged with their required CI gates green.
- Cheap selector, fixture, inventory and budget checks caught structural mismatches before broad suites.
- Reusing exact-release CI evidence refreshed stale phase reports without repeating the full cross-platform suite.
- A read-only integration pass traced all 34 requirements through their final consumers and found no broken flow.

### What Was Inefficient

- BE32 remained the critical path: the final main source run took 69m16s wall time, with BE32 at 68m23s; the full Release run took 76m38s, with BE32 at 68m30s. TODO-052 retains the measured sharding opportunity until complete selector/JUnit/artifact/hash coverage and a real hosted speedup are proven.
- The milestone audit found malformed YAML in one plan summary and phase-level requirements absent from some interim summary metadata. A cheap GSD `summary-extract` pass caught these before closing; we corrected the YAML and added explicit phase-closeout metadata, then refreshed affected fingerprints without rerunning tests.
- The Nightly metadata shortcut could not be added within the existing repository-check line cap without dropping its controls. TODO-051 keeps the safe full-workload fallback.

### Patterns Established

- Parse every plan summary and refresh verification fingerprints before aggregating a milestone; reconcile missing IDs against final phase verification instead of guessing from prose.
- Use local/Act routing checks before expensive CI when workflow wiring changes; for planning-only changes, reuse exact-source hosted evidence and avoid an equivalent broad rerun.
- Measure the job that owns the critical path and preserve all required checks until an exact split proves complete coverage and lower elapsed time.

### Key Lessons

1. Run the actual planning-tool parser over summary metadata; a regular expression can miss malformed YAML or misunderstand phase-level closure.
2. A pending shard is a hypothesis until its disjoint test union, JUnit reports, fixtures and downstream artifacts are proven on the hosted runner.
3. A changed workflow path can save iteration time only when its failure route remains conservative and every scheduled/manual gate stays complete.
4. Reuse green exact-head CI for documentation and fingerprint refreshes; do not repeat a long suite for bookkeeping.

### Cost Observations

- Main source CI took 69m16s wall time; full Release CI took 76m38s. BE32 was the longest critical-path job in both. The timing evidence does not provide a reliable dollar-cost estimate.
- Model mix and session count were not measured consistently. No local test, fuzz or Docker process was started for milestone auditing or cleanup.
- Timeline: Phase 7's v0.7.0 release began the milestone delivery; Phase 16's v0.16.0 release and the audit completed the v2.0 scope.

## Cross-Milestone Trends

Both milestones used per-phase pull requests and automated releases. v2.0's CI lanes are materially different from the early v1.0 runs, so the available timings do not support a comparable trend in Actions cost or critical-path duration. Keep the BE32 evidence and TODO-052 for the next workflow design that can measure a comparable before/after result.
