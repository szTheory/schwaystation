---
phase: 06-discs-and-fmv
plan: "03"
subsystem: device-integration
tags: [c, cdrom, mdec, bus, scheduler, state]
requires:
  - phase: 06-01
    provides: CD-ROM and MDEC production object targets in the core archive.
  - phase: 06-02
    provides: Host-owned disc callbacks and copied TOC metadata.
provides:
  - Bounded per-instance CD-ROM and MDEC state, reset contracts, and chip-owned state visitors.
  - CD-ROM and MDEC MMIO owner routes and deterministic scheduler deadline integration.
affects: [06-04, 06-05, 06-11, 06-26]
actuals:
  tokens: 7094
  tasks: 2
  commits: 2
commits: 2
plan_head_before: 42a19f93675e6e7e7aa52f7662f4c9a39d33cc44
plan_head_after: f290366ee89eb59c7d405a16ef888a0933266c4b
tech-stack:
  added: []
  patterns:
    - Device FIFOs and deadlines live in the instance; callbacks remain host-owned.
    - The scheduler takes the minimum device deadline and dispatches ties in documented order.
key-files:
  created:
    - src/cdrom.h
    - src/mdec.h
  modified:
    - src/internal.h
    - src/instance.c
    - src/cdrom.c
    - src/mdec.c
    - src/bus.c
    - src/sched.c
decisions:
  - "Keep fixed-size sector, audio, and MDEC buffers in each instance; size them for the accepted algorithms."
  - "Keep host callbacks and TOC metadata outside the chip state visitors."
metrics:
  duration: 18min
  completed: 2026-11-15
status: complete
requirements-completed: []
coverage:
  - id: D1
    description: CD-ROM and MDEC state, reset, and MMIO contracts compile into the core library.
    requirement: DISC-01
    verification:
      kind: integration
      ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-run schwaystation-test-sched && ctest --test-dir build/ci -R '^core\\.no_busy_loop$|^core\\.sched' --no-tests=error --output-on-failure; exit 0; local run, no hosted URL"
      status: pass
    human_judgment: false
  - id: D2
    description: The existing bounded-run and scheduler regression lane passes with CD-ROM and MDEC deadlines included.
    requirement: DISC-03
    verification:
      kind: integration
      ref: "The exact focused command above ran after each task; 8/8 selected tests passed both times, exit 0; no hosted URL."
      status: pass
    human_judgment: false
---

# Phase 6 Plan 03: Device and Scheduler Integration Summary

**Instance-owned CD-ROM and MDEC state now has bounded capacities, reset and visitor contracts, and bus/scheduler integration.**

## Performance

- **Duration:** 18 minutes
- **Started:** 2026-11-15T16:25:39Z
- **Completed:** 2026-11-15T16:43:41Z
- **Tasks:** 2
- **Files modified:** 8
- **Runtime/model:** Codex, OpenGSD 1.20.0; executor gpt-6-luna xhigh

## Accomplishments

- Added fixed-capacity CD-ROM command/result/interrupt FIFOs, eight sector slots, XA history and a bounded stereo queue, plus MDEC input/output FIFOs, tables and block state.
- Reset both devices on POWER, added chip-owned field visitors, and kept host disc callbacks outside serialized device state.
- Routed CD-ROM and MDEC MMIO through their owners and folded their deadlines into the scheduler's fixed event order.
- The named focused command passed after each task: configure and build succeeded, and all 8 selected no-busy-loop/scheduler tests passed. These tests establish compilation and scheduler regression stability; CD command, sector and MDEC decode behavior remains assigned to later caller plans.

## Task Commits

1. **Task 1: Define and reset bounded CD and MDEC state** — b79c004 (feat)
2. **Task 2: Route MMIO and dispatch device deadlines** — f290366 (feat)

## Verification Evidence

All checks ran from the repository root in the background. Local runs have no hosted run URL.

| Command | Result |
|---|---|
| cmake --preset ci && cmake --build --preset ci --target schwaystation-test-run schwaystation-test-sched && ctest --test-dir build/ci -R '^core\.no_busy_loop$|^core\.sched' --no-tests=error --output-on-failure | Task 1: exit 0; 8/8 selected tests passed. Task 2: exit 0; 8/8 selected tests passed. |
| git diff --check | Exit 0 before task commits. |

## Decisions Made

- The CD-ROM keeps 16-byte parameter/result FIFOs, eight 2,352-byte sector slots, 29-tap XA history, and 2,048 stereo sample slots; MDEC keeps a 32-word input FIFO and a 192-word output FIFO. These capacities are fixed in the device headers.
- The CD and MDEC state visitors cover device-owned state only. The host callback/TOC state remains outside them.
- The scheduler dispatches ties as timers, GPU, DMA, CD-ROM, SPU, SIO, and MDEC, matching the accepted architecture order.

## Deviations from Plan

### Task sequencing and TDD scope

The plan marks both tasks tdd=true, but its file lists and named verification command contain no new behavior test. The first CD command and MDEC decode test callers are explicitly assigned to Plans 06-04 and 06-11, and state acceptance is assigned to Plan 06-26. The parent execution scope also limits changes to the declared device/bus/scheduler files and this summary. I therefore ran the two named focused checks after each task without adding test files or RED commits; this does not claim those later device behaviors.

The MDEC reset and state visitor implementation landed in the first task commit alongside its header because POWER reset in src/instance.c must link to the device reset function. This follows Task 1's action text, although src/mdec.c appears under Task 2's file list.

## Known Stubs

- **src/cdrom.c:180, 189** — CD reads currently return zero and writes are ignored. The command/status model is assigned to Plan 06-04; sector delivery is assigned to Plan 06-05, with XA/CD-DA later in the phase.
- **src/mdec.c:124, 133** — MDEC reads currently return zero and writes are ignored. Command/status and block decode behavior is assigned to Plan 06-11 and its later MDEC callers.
- **src/cdrom.c:38** — The CD-ROM field visitor is not yet called by src/state.c; Plan 06-26 owns state integration and serialization acceptance.
- **src/mdec.c:54** — The MDEC field visitor is not yet called by src/state.c; Plan 06-26 owns state integration and serialization acceptance.

## Next Phase Readiness

Plan 06-04 can add the first CD command through the wired MMIO, IRQ and deadline path. Plan 06-11 can add MDEC commands and DMA endpoints using the bounded state. Plan 06-26 wires the prepared state visitors into save-state validation.

---
*Phase: 06-discs-and-fmv*
*Completed: 2026-11-15*

## Self-Check: PASSED

All eight implementation files and this summary exist; task commits b79c004 and f290366 are ancestors of HEAD; the persisted plan ledger measures two task commits from 42a19f9 to f290366.
