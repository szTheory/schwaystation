---
phase: 14-multitap-and-special-controllers
plan: "05"
subsystem: sio-special-controllers
tags: [c, sio, mouse, negcon, multitap, input]
requires:
  - phase: 14-multitap-and-special-controllers
    provides: Independent controller endpoints and tap polling methods from Plan 14-04
provides:
  - Frame-latched, saturated mouse movement and active-low buttons on direct and tap endpoints
  - Raw neGcon twist and pressure controls with its documented button layout
  - Mouse startup ACK-low release through the addressed endpoint and exact tap record lengths
affects: [phase-14-controller-plans, libretro-input, sio-state]
actuals:
  tokens: 7989
  tasks: 2
  commits: 5
tech-stack:
  added: []
  patterns:
    - Fixed-width, per-instance serial input snapshots with bounded frame latching
    - Controller-specific tap record lengths padded with FF only to the fixed socket record size
key-files:
  created: []
  modified:
    - src/pad.h
    - src/pad.c
    - src/sio.c
    - tests/sio/pad_tests.c
    - docs/hardware/sio.md
key-decisions:
  - "Latch and saturate relative mouse movement once per frame; repeated polls reuse that snapshot, while a frame without fresh input reports zero movement and preserves button levels."
  - "Release a mouse's modeled startup ACK-low hold only through an addressed 01h access at that endpoint, including physical port 2 and tap sockets."
  - "Expose neGcon twist, I, II and L bytes raw, without a response curve or deadzone."
  - "Keep phase requirements SIO-02 and SIO-03 pending because their libretro mapping and Jogcon portions are owned by later plans."
metrics:
  duration: 29m16s
  completed: 2026-11-24
  commits: 5
status: complete
plan_head_before: 37836e2da486bbbe33e16c545530e1bbddd45379
plan_head_after: 50a93462076b4aef4bf16700bfd41ed47a7e9b89
requirements-completed: []
coverage:
  - id: D1
    description: Mouse ID, saturated frame-latched movement, active-low buttons, reset/abort behavior, startup ACK release and state restoration on direct/tap endpoints
    requirement: SIO-02
    verification:
      - kind: integration
        ref: "sio.special.mouse; exact focused CTest command below exited 0 with the selector passing"
        status: pass
    human_judgment: false
  - id: D2
    description: neGcon ID, documented active-low button bits and independently raw twist/I/II/L bytes on both ports and tap socket D
    requirement: SIO-03
    verification:
      - kind: integration
        ref: "sio.special.negcon; exact focused CTest command below exited 0 with the selector passing"
        status: pass
    human_judgment: false
  - id: D3
    description: Method 1 tap records preserve mouse padding and exact neGcon record length
    requirement: SIO-02
    verification:
      - kind: integration
        ref: "sio.tap.poll_methods; exact focused CTest command below exited 0 with the selector passing"
        status: pass
    human_judgment: false
---

# Phase 14 Plan 05: Frame-Latched Mouse and neGcon Serial Vectors Summary

**Direct and tap serial endpoints now return bounded mouse and neGcon payloads with the documented IDs, button bits, raw controls and startup/reset behavior.**

## Performance

- **Duration:** 29m16s wall time (05:25:25–05:54:41 UTC); active time was not separately measured.
- **Runtime/model:** Codex GSD executor, configured `gpt-6-luna` xhigh; primary runtime model ID unavailable.
- **Tasks:** 2; **plan commits:** 5; **implementation files modified:** 5.

## Accomplishments

- Added mouse ID `5A12`, signed movement saturation to `[-128, 127]`, one-frame movement consumption, retained button levels, bounded reset/abort behavior, and addressed startup ACK release for either port and tap endpoints.
- Added neGcon ID `5A23` with raw twist, I, II and L values and the documented active-low buttons. Direct ports and tap socket D retain their controller-specific response sizes.
- Documented the host-input contract and the pinned PER.01 mouse and neGcon payload references in `docs/hardware/sio.md`.

## Verification Evidence

All commands ran locally from the repository root. The SIO inventory sort and selector preflight ran before configure/build. Exact focused verification exited 0: all three required selectors were present and 3/3 tests passed in 0.03 seconds. The initial mouse TDD failure was preserved and classified with `RED_EVIDENCE_OK`; no broad phase workflow, hosted CI, Act job or Docker resource was used because no workflow wiring changed.

```sh
set -o pipefail && git diff --check && LC_ALL=C sort -cu tests/sio/inventory.txt && ctest --test-dir build/ci --show-only=json-v1 -R '^(sio[.]special[.](mouse|negcon)|sio[.]tap[.]poll_methods)$' | python3 -c 'import json,sys; actual={t["name"] for t in json.load(sys.stdin)["tests"]}; required=set("sio.special.mouse sio.special.negcon sio.tap.poll_methods".split()); print("missing selectors:", sorted(required-actual)); sys.exit(bool(required-actual))' && cmake --preset ci && cmake --build --preset ci --target schwaystation-test-sio-pad schwaystation-test-sio --parallel 4 && ctest --preset ci --no-tests=error -R '^(sio[.]special[.](mouse|negcon)|sio[.]tap[.]poll_methods)$' --output-on-failure
```

Result: exit 0; selector preflight printed `missing selectors: []`; configure and both targeted builds succeeded; `sio.special.mouse`, `sio.special.negcon` and `sio.tap.poll_methods` passed (3/3, 0.03 seconds total).

## Decisions Made

- The mouse’s pending host delta is consumed at a frame boundary and stored in serial state; no host input is added to the save-state contract.
- Mouse startup ACK-low is modeled per endpoint and progresses only when that same addressed endpoint receives `01h`; the transport adds no wall clock or polling loop.
- neGcon analog controls stay raw. Any response shaping remains a host responsibility.

## Deviations from Plan

### Auto-fixed Issues

**1. Corrected the mouse reserved-bit mask against the pinned primary controller reference**
- **Found during:** Task 1 source check
- **Issue:** The initial mask did not preserve the documented fixed value of bits 8–9 while keeping unused outer bits high.
- **Fix:** Set bits 8–9 to zero, bits 0–7 and 12–15 to one, and varied only the active-low right/left bits; the mouse vector checks the wire mask.
- **Files modified:** `src/pad.c`, `tests/sio/pad_tests.c`
- **Commit:** `5d77069`

### TDD Sequence Note

Task 1 used a real mouse RED before its GREEN implementation; `.gsd/tdd-evidence/14-05/mouse-red.json` classifies it as `RED_EVIDENCE_OK`. The shared controller response switch also received the neGcon branch in Task 1's GREEN commit. Task 2 then added dedicated neGcon assertions against that branch and verified them green. No separate Task 2 RED is claimed; reverting already-green shared code to manufacture one would misrepresent the sequence and discard verified work.

## Commits

- `96b9aa7` test(14-05): add failing mouse protocol vectors
- `5bc7308` feat(14-05): implement frame-latched mouse protocol
- `5d77069` fix(14-05): preserve documented mouse button bits
- `e44a17a` test(14-05): cover neGcon wire vectors
- `50a9346` docs(14-05): document mouse and neGcon protocols

No stubs or unresolved verification gaps were found in the plan files. Phase-level SIO-02 and SIO-03 remain pending their later libretro-input and Jogcon slices. No additional threat surface was introduced.

## Self-Check: PASSED

- Summary file exists at `.planning/phases/14-multitap-and-special-controllers/14-05-SUMMARY.md`.
- The five measured plan commits are ancestors of `50a9346`.
- `plan_head_before`, `plan_head_after` and commit count match the persisted ledger at summary creation.
