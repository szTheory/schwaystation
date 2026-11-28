# Phase 17: A measured and trustworthy playability baseline - Discussion Log

> **Audit trail only.** Decisions are captured in 17-CONTEXT.md. This log preserves the alternatives considered.

**Date:** 2026-11-27
**Phase:** 17-a-measured-and-trustworthy-playability-baseline
**Areas discussed:** CI evidence and reliability; quality and performance claims; repository upgrades and documentation

The owner asked for all relevant decision points to be considered, with broad role-based analysis and one synthesized recommendation. The project's autonomous discussion rules direct the agent to follow the best-supported recommendation and ask only for unresolved choices that materially change scope, authority, risk, cost, or a phase boundary. Three focused advisor reviews and an adversarial reconciliation found no such owner-only choice before planning.

---

## CI evidence and reliability

| Option | Description | Selected |
|--------|-------------|----------|
| Pool the newest 40 runs regardless of route or workload | Produces a larger sample but mixes incomparable jobs and can imply false speedups. | |
| Use only matching workflow, matrix, and workload fingerprints; keep unmatched outcomes visibly separate | Preserves evidence quality and supports a defensible critical path and timeout calculation. | ✓ |
| Split long BE32 tests immediately | May reduce the tail but repeats setup and risks losing tests or downstream artifacts without a complete selector/JUnit contract. | |

**Recommendation applied under the owner's autonomous discuss authorization:** report only comparable timing, derive timeouts from twice the slowest matching job, and keep the BE32 split conditional on complete local preflights and a measured hosted improvement.
**Notes:** TODO-052 already directs Phase 17 to refresh the candidate with both libretro.host and libretro.input.movies. TODO-051 stays deferred pending a measured gate refactor. TODO-022 and TODO-049 are reviewed as report/audit items; no coverage reduction is assumed.

---

## Quality and performance claims

| Option | Description | Selected |
|--------|-------------|----------|
| Choose the most obvious improvement before scoring | Faster to plan but risks selecting work that is not the weakest current dimension. | |
| Score all seven dimensions from linked evidence, then implement one improvement to the weakest | Meets TUNE-09 and prevents an unmeasured preference from setting scope. | ✓ |
| Broaden low-power or compatibility claims using estimates and inventory counts | Adds apparent coverage but overstates what the core and hosted evidence establish. | |

**Recommendation applied under the owner's autonomous discuss authorization:** remeasure existing producers and keep low-power device and compatibility claims limited to measured evidence. Choose exactly one change after scoring, using an impact-based tie-break when needed.
**Notes:** Do not infer browser speed from wasm32 hash equality or broad title support from fixture/inventory results.

---

## Repository upgrades and documentation

| Option | Description | Selected |
|--------|-------------|----------|
| Change pins, caches, or update automation as part of every tune-up | May reduce drift but adds churn and risk without a current reason. | |
| Audit current policy and consumers; change only for specific evidence | Retains stable pins and scopes maintenance changes to a demonstrated need. | ✓ |
| Enable unattended Dependabot auto-merge | Reduces review effort but conflicts with the later Phase 7 decision to keep updates reviewable and auto-merge off. | |

**Recommendation applied under the owner's autonomous discuss authorization:** run the existing settings check, inspect current upstream evidence, and refresh only stale guidance. Consider TODO-010 only if the new scoring selects upgrade path as weakest; do not fold TODO-011/012 into the phase.
**Notes:** Phase 7's later specific decision and delivery evidence govern over the older conflicting DEC.94 wording. No Dependabot auto-merge workflow is active.

---

## the agent's Discretion

- Select matching CI cohorts, score evidence, and the single improvement after current measurements are available.
- Keep the smallest implementation that meets a requirement; prefer existing tooling or a small focused helper over another dependency.
- Use cheap preflights before long work and make no permanent checker unless it prevents a named recurring fault.

## Role and adversarial synthesis

- **CI/DevOps:** fingerprint matching and explicit critical-path calculation make runs comparable; a shard could shorten BE32 but repeats setup and creates routing/artifact risks. Keep the shard conditional on exact selector and JUnit-union controls plus a hosted measurement.
- **Test and security engineering:** preserve every required case, assertion, platform, and conservative unknown-path fallback. A timeout increase can hide a stall, and skipping a test based on an incomplete classifier can hide a real regression.
- **Performance engineering:** Cachegrind Ir/frame is comparable only with the defined producer and toolchain; wall time is runner-dependent. Do not infer device speed, browser speed, or billing from another metric.
- **Product and compatibility:** users need an accurate install path and explicit evidence limits. A fixture pass, inventory count, BIOS shell, or one title does not establish general retail playability.
- **Supply-chain and maintenance:** current SHA pins and reviewable Dependabot PRs reduce drift without adding a runtime dependency. Unattended auto-merge conflicts with the later Phase 7 direction and would hide security/release review.
- **Adversarial reconciliation:** the CI candidate is vulnerable to a false speedup from mixed run routes or duplicated setup; quality bounds are vulnerable to stale or mismatched producers; the upgrade audit has an older DEC.94 statement that conflicts with the newer Phase 7 outcome. The recommendations reject all three failure modes and preserve the complete evidence chain.

## Deferred Ideas

- Nightly PR routing remains deferred until its required classifier controls fit within the existing gate budget.
- No speculative cache, concurrency increase, broad matrix reduction, unrelated pin update, browser-speed claim, or broad compatibility claim.
- Phase 18 and Phase 19 playability work remain outside Phase 17.
