# Phase 04 Plan Check: GPUSTAT Field Status

## ISSUES FOUND

**Plan reviewed:** 04-53 only. This is a focused static plan review; no builds or tests were run.

The source decision is supported by the pinned [PSX-SPX status-register documentation](https://psx-spx.consoledev.net/dev/ps1/gpu/status-register/) and commit [519c8dbcc025467a6c80b02e61aafe3adcfa497b](https://github.com/psx-spx/psx-spx.github.io/commit/519c8dbcc025467a6c80b02e61aafe3adcfa497b). The plan preserves v0/v2 differences, uses one instance-owned saved phase, validates it as 0..1 without tying it to mode or frame parity, and limits claims to the existing fixed beam clock. It explicitly leaves recognizable-shell acceptance to 04-40; 04-40 depends on 04-53, so that check is sequenced after this fix.

Plan structure is valid: 2 tasks, 8 distinct files, wave 28 after 04-52. The 23–29 minute forecast is within the project limit; its token estimate is under budget but low-confidence (no calibrated phase actuals). No architectural responsibility map, PATTERNS.md, or applicable research open questions were found. Nyquist validation is disabled in project config.

### Warning

**[verification_derivation] Every focused verification command must fail when a newly required case is missing or fails.**

- Plan: 04-53, tasks 1 and 2.
- Evidence: Task 1’s filter includes `fixtures.gpu_motion.state.150/180`, so those existing cases can run and pass when `gpu.beam.status_fields` is absent. Task 2’s filter includes existing `gpu.beam.reject_malformed_state` and `gpu.beam.field_edges`, so they can pass when `gpu.beam.status_controls` is absent. `--no-tests=error` only rejects an empty selection. The action says to demonstrate a genuine RED, but does not give an exact pre-implementation command and expected failing result.
- Example fix (non-binding): run the new case alone before the production change and record its expected failure, then make the green checks require each newly named case to be registered and pass.

### Structured Issues

```yaml
issues:
  - plan: "04-53"
    dimension: "verification_derivation"
    severity: "warning"
    required_property: "Every focused verification command must fail when a newly required case is missing or fails"
    description: "Task 1's CTest filter can pass on the two existing fixture-state cases if gpu.beam.status_fields is missing. Task 2's filter can pass on the existing reject_malformed_state and field_edges cases if gpu.beam.status_controls is missing. --no-tests=error only detects an empty selection. The action also requests a genuine RED without specifying a runnable pre-implementation command and expected failing result."
    fix_hint: "Run the new case alone before the production change and record its expected failure; make the green checks require each newly named case to be registered and pass."
```

**Recommendation:** 0 blockers, 1 warning. The plan’s state-model and evidence decisions are sound; tighten only the new-case RED/GREEN verification so it cannot pass without those cases.

## Warning resolution

The coordinator tightened the plan's focused checks after this independent review: each new status case runs alone with `--no-tests=error`; existing fixture and beam controls run separately. Task 1 now specifies the pre-production command and requires a failure from the new parity assertions, excluding a missing registration or build failure. No source design, scope, hardware oracle or timing claim changed. The warning is resolved; implementation may proceed.
