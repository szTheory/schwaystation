---
phase: 16-patches-cheats-and-compatibility-evidence
plan: "17"
subsystem: ci
tags: [BE32, CTest, CI]
requirements-completed: [COMPAT-03]
coverage:
  - id: D1
    description: Both long tests and the existing BE32 job have recorded timings and an exact proposed selector partition.
    requirement: COMPAT-03
    verification:
      - kind: other
        ref: .planning/todos/pending/052-be32-movie-test-shard.md
        status: pass
    human_judgment: false
metrics:
  completed: 2026-11-26
  duration: 8m
commits: 1
plan_head_before: a757d80f7c8a8ba8ab56d884f98f58f5df1cd1c1
plan_head_after: 00f44dadd1417d16f28e2ad33e989e5e799bb074
status: complete
---

# Phase 16 Plan 17 Summary

The proposed partition covers the entire current portable inventory, with one shard for each long libretro test and the broad shard retaining the hash artifact producer. Both comparable hosted runs placed the audio host check and the serial movie check at the end of BE32. The production job remains intact pending a comparable hosted before-and-after run and exact union of its JUnit reports.

The timing and test-set evidence lives in `.planning/todos/pending/052-be32-movie-test-shard.md`. The maintainer note in `docs/maintaining.md` records when the production split can proceed.

## Checks run

- `gh run view 45685800743 --json status,conclusion,url,jobs` and the matching command for 45697774644 exited 0. The two successful BE32 jobs took 72m24s and 72m40s. [PR run](https://github.com/szTheory/schwaystation/actions/runs/45685800743), [Main CI run](https://github.com/szTheory/schwaystation/actions/runs/45697774644).
- `gh api --allow-escape-sequences repos/szTheory/schwaystation/actions/jobs/<job>/logs` with local filtering exited 0 for both jobs. It confirmed the two per-test durations and complete test counts. Only the measured values were retained.
- The four `ctest --preset ci --show-only=json-v1 -L '^portable$'` enumerations, fixture and hash dependency assertions, and exact set comparison exited 0. The native registration matches the declared portable inventory: 692 broad tests, one audio test and one movie test, with zero overlap. Temporary enumeration outputs were removed.
- `git diff --check` and the todo binding and evidence assertions exited 0.

Runtime: Codex coordinator with OpenGSD 1.23.0. The primary model ID was unavailable.

## Self-Check: PASSED

The existing hosted evidence and local partition support the proposed follow-up.
