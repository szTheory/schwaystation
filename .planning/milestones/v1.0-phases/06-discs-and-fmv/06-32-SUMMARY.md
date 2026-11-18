---
phase: 06-discs-and-fmv
plan: "32"
subsystem: mdec
tags: [mdec, packing, clipping, padding]
dependency_graph:
  requires: [06-31]
  provides: [mdec-nonuniform-packing-vectors, mdec-clipping-vectors, mdec-padding-vector]
  affects: [06-12]
tech_stack:
  added: []
  patterns: [fixed-output-model-vectors]
key_files:
  created: []
  modified: [tests/mdec/model.c, tests/mdec/tests.cmake, tests/mdec/inventory.txt, docs/hardware/mdec.md]
decisions:
  - Use sparse DC/AC and scale-table inputs with fixed raw FIFO words to expose pixel-order changes.
  - Check both signed 8-bit clipping endpoints and compare padded and unpadded FIFO outputs.
metrics:
  duration: "at least 7m34s"
  completed_date: 2026-11-16
status: complete
actuals:
  tokens: 1531.75
  tasks: 1
  commits: 1
  plan_head_before: 516f8ba7d8e9377af6c38ab2c9d79b27f76472be
  plan_head_after: 5034d3213068c6d73837ff6a89a588c8755181fc
requirements-completed: []
coverage:
  - id: DISC-03
    description: Nonuniform 4/8-bit packing, signed clipping endpoints, and FE00 pre-DC padding controls
    requirement: DISC-03
    verification:
      kind: unit
      ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-mdec && ctest --test-dir build/ci -R '^mdec\\.(words|vectors)$' --no-tests=error --output-on-failure (exit 0, 2/2)"
      status: partial
    human_judgment: false
---

# Phase 6 Plan 32: Cover MDEC packing, clipping and padding vectors

Registered nonuniform MDEC outputs that distinguish 4-bit and 8-bit pixel order, both signed clipping endpoints, and FE00 padding before DC.

Runtime/model: Codex/OpenGSD 1.20.0; executor gpt-6-luna xhigh. Wall-time lower bound: at least 7m34s. Focused review was clear and the final gate passed 2/2. No hosted run applies.

## Completed work

- Added a sparse DC/AC vector with fixed raw FIFO words for both 4-bit and 8-bit output.
- Asserted all 16 words at positive and negative signed clipping endpoints.
- Compared the exact 4-bit FIFO sequence for a block with FE00 padding before DC against its unpadded equivalent.
- Registered `mdec.vectors`, kept the inventory sorted, and documented the new controls.

## Verification

Command:

```sh
cmake --preset ci && cmake --build --preset ci --target schwaystation-test-mdec && ctest --test-dir build/ci -R '^mdec\.(words|vectors)$' --no-tests=error --output-on-failure
```

Exit status: 0. Both selected tests passed after review.

The first run failed because the 4-bit test command inherited the 8-bit depth bit and the saturation expectation covered only the first row. Those were test-vector construction errors; the corrected exact vectors pass against the existing implementation. No production behavior change was needed, and the initial failure is not claimed as valid RED evidence.

## Remaining acceptance

The vectors deferred by Plan 06-31 are now covered. DISC-03 remains partial until Plan 06-12's pinned block/status guests and downstream Phase 6 integration pass.

## Self-Check: PASSED

- Task commit `5034d32` is reachable from `plan_head_after`.
- The exact post-review gate passed both registered MDEC tests.
- No test is skipped or self-disabled.
