# Phase 04 Plan Check: DMA4 ATTR Readiness

## VERIFICATION PASSED

**Plan reviewed:** 04-52, including its dependency and wave relationship with 04-40.

Plan 04-52 is a complete, bounded behavior slice: it derives DMA4 request readiness from the saved SPU ATTR mode while preserving manual DREQ credits, current bus arbitration and timing, and the existing DMA4 sink. It covers multiblock continuation, mode changes and deassertion, stop/disable/wrong-direction controls, interleaved instances, read-ahead deadlines, and save/load validation without adding serialized state or changing the state version. Its source, test, inventory, and hardware documentation changes are planned together. The forecast is 22–28 minutes across nine files, within the project’s 30-minute plan limit; this is a substantive engine behavior boundary, not a mechanical test/doc split.

The plan specifies a real bus-driven failing regression before implementation, then targeted count, address, CRC, cost, completion/IRQ, and negative-control checks registered in the existing CMake/CTest inventory and ordinary CI. It retains a single full CI and hygiene closeout. It does not claim SPU FIFO timing, hardware confidence, full shell success, new API/state behavior, or an owner UAT step; frozen state revisions 1–3, the revision-4 format, and existing phase gates remain intact.

Plan 04-52 is the only ready plan in wave 27. Plan 04-40 now depends on it and remains in wave 28, where it must resume actual shell inspection; the shell remains unaccepted. The DAG and ROADMAP propagation preserve the prior completed waves and all existing future acceptance criteria. Native plan-structure checks pass for both plans.

**Scope and sizing judgment:** no concrete coverage, linkage, ordering, or oversizing/over-splitting issue found. This was a static plan review; no tests or builds were run.
