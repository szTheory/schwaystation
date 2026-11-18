---
phase: 06-discs-and-fmv
plan: "31"
subsystem: mdec
tags: [mdec, save-state, dma, fifo]
dependency_graph:
  requires: [06-11]
  provides: [mdec-halfword-cursor, mdec-fifo-backpressure, mdec-raw-dma1-controls]
  affects: [06-12, 06-13]
tech_stack:
  added: []
  patterns: [serialized-halfword-cursor, deadline-free-fifo-wait]
key_files:
  created: [docs/hardware/mdec.md]
  modified: [src/mdec.c, src/internal.h, src/mdec.h, tests/core/test_state.c, tests/mdec/model.c, tests/mdec/tests.cmake, tests/mdec/inventory.txt, docs/embedding.md]
decisions:
  - Keep an EOB high-halfword queued across blocks and save/load.
  - Represent a completed block waiting for FIFO room with no event deadline; DMA1 reads resume it.
  - Validate MDEC state control against the actual DMA0/DMA1 enable mask.
metrics:
  duration: "at least 31 minutes"
  completed_date: 2026-11-16
status: complete
actuals:
  tokens: 6852
  tasks: 2
  commits: 4
  plan_head_before: f1ff8780ea6d6a85b39949a12a10d623bec1e9b3
  plan_head_after: f0de9b36475ecafefd247881eb782090d3dc6780
requirements-completed: []
coverage:
  - id: DISC-03
    description: MDEC stream continuation, FIFO backpressure, state round-trip, raw mono output, DMA1 layout, and baseline timing controls
    requirement: DISC-03
    verification:
      kind: unit
      ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-mdec schwaystation-test-state && ctest --test-dir build/ci -R '^mdec\\.(block|status|bounds|words|fifo|stream)$|^core\\.state\\.' --no-tests=error --output-on-failure (exit 0, 14/14)"
      status: partial
    human_judgment: false
---

# Phase 6 Plan 31: Preserve MDEC stream and output under FIFO pressure

The MDEC now retains an EOB high halfword across save states, holds completed output blocks when its FIFO is full, and proves raw mono words separately from DMA1 RAM layout.

Runtime/model: Codex/OpenGSD 1.20.0; executor gpt-6-luna xhigh. Wall-time lower bound: at least 31 minutes. The focused post-review command passed all 14 selected tests. No hosted run applies.

## Completed work

- Serialized the MDEC input halfword cursor and advanced state format and behavior revision to 12.
- Retained completed blocks under output FIFO backpressure without scheduling a same-tick retry; DMA1 reads resume publication when a whole block fits.
- Validated the serialized MDEC control word against the supported DMA0/DMA1 bits and rejected an unsupported control bit in state-corruption coverage.
- Added raw little-endian output hashes for signed and unsigned 4-bit and 8-bit mono blocks, plus exact DMA1 RAM words and final MADR checks.
- Documented the implemented MDEC path, FIFO behavior, and the unstamped timing-log limitation.

## Verification

Command:

```sh
cmake --preset ci && cmake --build --preset ci --target schwaystation-test-mdec schwaystation-test-state && ctest --test-dir build/ci -R '^mdec\.(block|status|bounds|words|fifo|stream)$|^core\.state\.' --no-tests=error --output-on-failure
```

Exit status: 0. All 14 selected tests passed: 8 state tests and 6 MDEC tests.

The first DMA1 restart control exposed a test-clock issue: the harness completed a DMA unit at its future `busy_until` but left `inst->ticks` behind, so the next unit was correctly ineligible. The helper now advances the instance through that boundary before starting another DMA operation.

## Deviations and remaining acceptance

Plan 06-31 exceeded its 30-minute execution bound. Its completed cursor, backpressure, state, raw-output, DMA1, and timing-documentation scope is closed here. Focused review found that the flat raw vectors did not prove pixel ordering, and no registered vector explicitly asserted output clipping or `FE00h` padding before DC. Dependent Plan 06-32 adds those controls. DISC-03 remains partial until the pinned guest checks and downstream integration pass.

## Self-Check: PASSED

- Task commits are present in the plan ledger and reachable from `plan_head_after`.
- Commits: `d4d8e01`, `727d339`, `7338d25`, `f0de9b3`.
- The focused gate passed with 14 selected tests.
- The exact remaining vector gaps are assigned to Plan 06-32; no test is skipped or self-disabled.
