# Phase 12: SBI and LibCrypt - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-11-23
**Phase:** 12-SBI and LibCrypt
**Areas discussed:** SBI binding and selection, bad-Q command behavior, CI iteration efficiency

---

## SBI binding and disc selection

| Option | Description | Selected |
|--------|-------------|----------|
| Keep one SBI callback outside the selected provider | Centralizes loading but can reuse disc A's numeric LBA entries for disc B. | |
| Bind each parsed sidecar to its concrete image object and TOC/LBA map | Uses the existing host provider and descriptor callback; M3U child selection naturally changes the association. The SBI format itself still cannot authenticate the pairing. | ✓ |

**User's choice:** Follow the best-supported recommendation under the standing autonomous authorization.
**Notes:** The adversarial review confirmed no PBP multi-disc path exists. Test overlapping LBAs across M3U children and preserve the explicit SBI identity limitation.

---

## Bad-Q GetlocP behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Test only the internal sector helper | Fast but does not verify guest-visible data, interrupt and GetlocP behavior together. | |
| Exercise a short CD-ROM command sequence with a disc swap | Tests the previous-good position, bad-Q sector data delivery and stale Q state across a selected-disc change. | ✓ |

**User's choice:** Follow the best-supported recommendation under the standing autonomous authorization.
**Notes:** Adversarial review found that sector reads save valid-looking Q before consulting bad-Q, seek completion does not consult bad-Q, and disc swap retains the old Q record. Cover each behavior without changing data delivery.

---

## CI iteration efficiency

| Option | Description | Selected |
|--------|-------------|----------|
| Keep capturing runner PCM inside both callback comparisons | Preserves current behavior but runs the same guest capture twice per region. | |
| Capture once per region and keep both host comparisons | Removes three redundant guest invocations while preserving all regions, six comparisons, and capture file/hash checks. | ✓ |
| Change Main/Release or Nightly workflow routing in this SBI phase | Could save hosted time but needs separate fail-closed routing evidence and the project gate cap is already constrained. | |

**User's choice:** Follow the measured, low-risk recommendation under the standing autonomous authorization.
**Notes:** The BE32 `libretro.host` job measured 18m52s. Main/Release matrices stay required; todo 050 records differing archive digests, and pending todo 051 is a separate Nightly-routing task.

## the agent's Discretion

- Select bounded SBI parser helpers, record/vector coverage and generated fixture format from the project-pinned source facts.
- Make malformed existing sidecars a clear, failure-atomic open error; treat absent sidecars as no overlay.
- Preserve all code/test assertions and run focused checks before expensive suites.

## Deferred Ideas

- No Nightly/Main/Release classifier or workflow changes in this phase; revisit only in a dedicated, budget-compliant, fail-closed routing task.
- No multi-disc PBP or cryptographic SBI-to-disc authentication; neither is provided by the current supported format contract.
