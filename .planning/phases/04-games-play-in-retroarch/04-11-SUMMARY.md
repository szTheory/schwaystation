---
phase: 04-games-play-in-retroarch
plan: "11"
subsystem: dma
tags: [dma, gpu, state-validation, state-v4]
requires:
  - phase: 04-games-play-in-retroarch (plans 01-10)
    provides: DMA scheduling, GPU packet decoding, GPUREAD, and state serialization interfaces
provides:
  - DMA2 uses the shared GPU GP0 packet and GPUREAD paths
  - GPU readiness, DREQ, and modeled busy deadlines gate DMA work
  - DMA state validation preserves format-4 fields and checks cross-field links
  - Linked-list yield counts are tracked independently per DMA channel
affects: [dma, gpu, state, phase-04]
actuals:
  tokens: 20097
  tasks: 2
  commits: 4
plan_head_before: 218947623d116db839cad72f3185c0016257f41a
plan_head_after: 1bfc795dc9c024f29332135879de55ef1d030448
tech-stack:
  added: []
  patterns:
    - DMA2 shares GPU packet/readback instance state with CPU accesses
    - Per-channel linked-list progress reuses remain without changing the v4 nodes_done field
key-files:
  created: []
  modified:
    - src/dma.c
    - src/dma.h
    - src/gpu.c
    - src/gpu.h
    - tests/core/test_state.c
    - tests/dma/dma_tests.c
    - tests/gpu/gpu_tests.c
    - docs/hardware/dma.md
    - docs/hardware/gpu.md
key-decisions:
  - Keep state format 4 and the scalar nodes_done field shape; store per-channel linked-list counts in remain.
  - Treat busy sync-mode-3 rewrites as a derived fail-closed policy, not a hardware timing claim.
  - Accept valid DMA phases 0 through 5 and reject out-of-range phases plus invalid cross-field combinations.
requirements-completed: []
coverage:
  - id: D1
    description: DMA2 linked-list uploads and readbacks use the shared GPU packet and VRAM paths.
    requirement: GAME-02
    verification:
      - kind: integration
        ref: "dma.gpu.linked_list_readback; dma.read.chain_saves; ctest --test-dir build/ci -L '^gpu$|^dma$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: GPU readiness, DREQ, busy deadlines, and atomic DMA timing are exercised.
    requirement: GAME-02
    verification:
      - kind: integration
        ref: "gpu.dma.ready_budget; dma.stall.engines_decay; dma.chopping.band; GPU/DMA label suite (89 tests, exit 0)"
        status: pass
    human_judgment: false
  - id: D3
    description: Malformed DMA state, busy sync-mode-3 rewriting, and per-channel list yields have executable contracts.
    requirement: GAME-02
    verification:
      - kind: unit
        ref: "core.state.device_fields; dma.fault.sync3; dma.list.independent_yields; focused DMA/state set (13 tests, exit 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: GPU-rendered pixels produced by DMA2 match independent expected RGB555 values.
    requirement: GAME-01
    verification:
      - kind: integration
        ref: "dma.list.walk; dma.chopping.band; GPU/DMA label suite (89 tests, exit 0)"
        status: pass
    human_judgment: false
metrics:
  duration: 100min
  completed: 2026-11-10
status: complete
---

# Phase 04 Plan 11: GPU DMA2 and DMA State Summary

**DMA2 now shares the GPU packet and readback pipeline, with readiness deadlines, validated DMA state, and per-channel list yields.**

## Performance

- **Duration:** about 100 minutes
- **Started:** 2026-11-10 20:05 EST (approximate; first task commit was at 20:20 EST)
- **Completed:** 2026-11-10 21:40 EST
- **Tasks:** 2
- **Files modified:** 17

## Accomplishments

- Connected DMA2 writes to GP0 and reads to GPUREAD, sharing packet and transfer state with CPU accesses; added deterministic GPU busy costs and readiness/DREQ behavior while preserving atomic DMA unit costs.
- Added DMA phase, current-channel, busy deadline, CPU-hold, and transfer-count validation; modeled reserved sync-mode-3 rewrites as fail-closed and isolated linked-list yield accounting per channel.
- Updated DMA/GPU hardware notes and closed todos 040, 041, 043, and 045. The v4 state layout, frozen v1-v3 callers, GPUv0 flag value, and test-card pins remain unchanged.

## Task Commits

1. **Task 1 RED: GPU DMA2 behavior tests** — `d397438` (`test`)
2. **Task 1 GREEN: shared GPU packet/readback endpoints** — `90d8130` (`feat`)
3. **Task 2 RED: DMA state, rewrite, and yield tests** — `d3a7a0a` (`test`)
4. **Task 2 GREEN: DMA state validation and per-channel yields** — `1bfc795` (`feat`)

The metadata commit contains this summary and the normal planning state updates; its hash is recorded in Git history after this file is committed.

## Files Created/Modified

- `src/dma.c`, `src/dma.h` — DMA2 GPU integration, state validation, busy rewrite behavior, and per-channel list progress.
- `src/gpu.c`, `src/gpu.h` — shared DMA2 accessors, readiness, and integer busy model.
- `tests/dma/dma_tests.c`, `tests/gpu/gpu_tests.c`, `tests/core/test_state.c` — DMA/GPU behavior and state contracts.
- `docs/hardware/dma.md`, `docs/hardware/gpu.md` — implemented timing and state behavior.
- `.planning/todos/completed/040-dma-state-validation-tightening.md`, `041-dma-sync3-busy-rewrite.md`, `043-timers-vestigial-code.md`, `045-dma-nodes-done-per-channel.md` — completed follow-up records.

## Decisions Made

- Preserved state format 4 and its scalar `nodes_done` field shape. Each DMA channel's `remain` slot owns its list-node count.
- Kept sync-mode-3 busy rewrite behavior explicitly marked as a derived safety policy because console behavior is unmeasured.
- The plan text said “phase > 1” should be rejected, but phases 2 through 5 are valid running/waiting states. Validation therefore rejects out-of-range phase values and invalid relationships, while retaining valid phases 0 through 5.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Preserve the CPU hold deadline when a DMA unit finishes ahead of the CPU tick**
- **Found during:** Task 2 full CI regression run.
- **Issue:** Clearing `stall_until` in `dma_finish` lost the CPU hold when a DMA-register read completed its device unit ahead of the CPU's current tick. This broke OTC read completion, engine decay, and a DMA chain fixture.
- **Fix:** Retained the completed unit's hold stamp and validated that saved state against the completed unit deadline when no channel is current.
- **Files modified:** `src/dma.c`, `tests/core/test_state.c`, `docs/hardware/dma.md`.
- **Verification:** Focused DMA/state set passed 13/13; GPU/DMA label suite passed 89/89.
- **Committed in:** `1bfc795`.

**2. Clarify the DMA phase acceptance condition**
- **Found during:** Task 2 state-contract implementation.
- **Issue:** The plan's shorthand “phase > 1” conflicts with the existing state machine, where phases 2 through 5 represent valid block, gap, next-node, and stop states.
- **Fix:** Kept those valid phases and tested out-of-range phases plus inconsistent current-channel, busy, and stall links.
- **Verification:** `core.state.device_fields` passed in the focused 13-test set.
- **Committed in:** `1bfc795`.

**Total deviations:** 2 (one correctness fix and one plan-text clarification). No new dependencies or gates were added.

## Evidence

- `ctest --test-dir build/ci -L '^gpu$|^dma$' --no-tests=error --output-on-failure` — exit 0; 89/89 tests passed.
- Focused DMA/state regression set in `build/ci` — exit 0; 13/13 passed, including state sweep, linked-list yield, sync3 rewrite, chopping, readback, and DMA-chain fixture cases.
- `cmake -S . -B build/hygiene -DSCHWAYSTATION_TIMEOUT_SCALE=4` — exit 0. The repository's documented slow-lane setting was needed because both full-tree scans exceeded their default 60-second timeout on this host.
- `cmake --workflow --preset hygiene` — exit 0; 23/23 checks passed. The two tree scans each took about 136 seconds.
- `sh scripts/hygiene.sh --tree` — exit 0.
- `cmake --workflow --preset ci` — the detached retry reached all five workflow stages; 407/407 CI tests and 2/2 release archive tests passed, and all three packages were generated. **Process exit status unavailable:** the launcher did not capture it. The separate status file with value 8 belongs to the earlier, pre-fix run and is not evidence for this retry.
- Task 2 RED evidence records for `core.state.device_fields`, `dma.fault.sync3`, and `dma.list.independent_yields` each returned `RED_EVIDENCE_OK`; the named target failures were the planned assertions. Task 1 RED and GREEN commits are ordered `d397438` then `90d8130`.

## Issues Encountered

- The default hygiene timeout was shorter than this host's full-tree scan runtime. The supported timeout scale allowed the unchanged checks to complete; no hygiene violations were reported.
- The full CI retry's process exit could not be recovered from its detached launcher. Its logged stages and all test results are recorded above; the captured GPU/DMA and hygiene commands exited successfully.

## User Setup Required

None.

## Next Phase Readiness

DMA2/GPU state and transfer behavior are available to subsequent Phase 04 work. GAME-01 and GAME-02 remain open because their broader rendering, scan-out, adapter, and fixture criteria are outside this plan.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-10*

## Self-Check: PASSED

- Summary file exists.
- Task commits `d397438`, `90d8130`, `d3a7a0a`, and `1bfc795` are ancestors of HEAD.
- No stub patterns were found in the modified source, test, or hardware documentation files.
