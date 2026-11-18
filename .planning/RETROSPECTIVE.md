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

## Cross-Milestone Trends

This is the first milestone retrospective. There is not yet enough cross-milestone data to claim a trend.
