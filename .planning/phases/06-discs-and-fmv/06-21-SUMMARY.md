---
phase: 06-discs-and-fmv
plan: "21"
subsystem: cd-media-state
tags: [disc, save-state, cachegrind, hardware-reference]
actuals:
  tasks: 2
requirements-completed: []
coverage:
  - id: DISC-01-ACTIVE-MEDIA-STATE
    description: "Save states bind to the host-supplied disc identity, reject missing or different media before applying state, and never call host media callbacks during save/load."
    requirement: DISC-01
    verification:
      kind: integration
      ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-api-v6 schwaystation-test-state schwaystation-test-run schwaystation-test-cdrom schwaystation-test-core --parallel 2 && ctest --test-dir build/ci -R '^abi\\.api_v6$|^core\\.state\\.|^core\\.no_busy_loop$|^cdrom\\.sectors$|^core\\.no_alloc_after_load$|^core\\.no_host_clock$|^perf\\.gate\\.cd_idle_' --no-tests=error --output-on-failure (exit 0, 16/16)."
      status: partial
    human_judgment: false
  - id: DISC-01-IDLE-DRIVE-BOUND
    description: "Idle and lid-open drive scheduling returns within a fixed bound; hosted Cachegrind measures the attached-drive cost against the same public no-disc frame path."
    requirement: DISC-01
    verification:
      kind: integration
      ref: "Focused Plan 21 CTest command (exit 0, 16/16); ctest --test-dir build/ci -L perf --no-tests=error --output-on-failure (exit 0, 19/19); fake Cachegrind handoff smoke only (exit 0). The actual Ir measurement is pending hosted CI; Valgrind is not installed locally and no hosted run URL exists yet."
      status: partial
    human_judgment: false
  - id: DISC-03-MEDIA-STATE-FIELDS
    description: "Device-owned CD/XA/MDEC state fields and identity participate in state visits, stable serialization, round trips and malformed-state rejection."
    requirement: DISC-03
    verification:
      kind: unit
      ref: "Focused Plan 21 CTest command (exit 0); core.state.* selected 8/8, including field visitor, stable bytes, round trip, rejection and bounded-run cases. Full MDEC guest acceptance remains open in downstream plans."
      status: partial
    human_judgment: false
  - id: DISC-04-DISC-IDENTITY-API
    description: "API v6 accepts a host-supplied SHA-256 media identity while retaining the old descriptor prefix for emulation; save-state disk-control acceptance remains with downstream consumers."
    requirement: DISC-04
    verification:
      kind: integration
      ref: "abi.api_v6 and the focused Plan 21 CTest command (exit 0); public API test covers same/different/missing identity, legacy descriptor size, malformed identity fields, atomic rejection and zero callback calls."
      status: partial
    human_judgment: false
metrics:
  duration: "Active duration was not tracked; execution resumed after a 22-hour pause."
  runtime: "Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable)."
completed: 2026-11-16
status: complete
---

# Phase 6 Plan 21: Media state identity and bounded idle work

The plan resolves the state/media boundary: hosts supply a fixed SHA-256 identity when attaching a disc, the core snapshots it without reading sectors, and save states compare that identity before changing the receiver. Legacy size-tagged descriptors remain usable for emulation, while state save/load refuses an attached disc whose identity is unavailable. The idle-CD performance control now uses the same public frame path without a disc, so Cachegrind measures the incremental drive cost instead of unrelated scheduler work.

## Work completed

- Added API v6 disc identity fields and a distinct state/disc mismatch status. Save/load state contains the identity but no host callbacks, pointers or file handles.
- Added consumer API coverage for identical, different, missing and unknown media identities; it checks atomic mismatch, legacy descriptors, malformed fields and that save/load never invokes media callbacks.
- Extended no-busy-loop checks to idle and lid-open media, and added a hosted fixed instruction allowance for an idle attached CD drive.
- Added a workflow consistency check that requires both SPU and CD idle Cachegrind cases in hosted perf CI. The repository-check total is 2,699 lines against the 2,700 ceiling.
- Reconciled state-format and host-interface documentation. Corrected the architecture page to describe the actual state header and serialized media boundary.
- Recorded public SCPH-5501 measurements: the seek capture shows state-dependent, nonmonotonic times, and the MDEC capture reports DMA0-start to DMA1-completion ranges of 947–1,007 cycles for one block, 3,534–3,609 for six, and 12,780–12,825 for 24 blocks. These records do not settle a portable seek mapping, an isolated per-block MDEC baseline, or XA interpolation rounding. See [CD-ROM seek timing](https://psx.wiki/tests/cd-rom-seek-timing/), [MDEC status and decode timing](https://psx.wiki/tests/mdec-status-decode-timing/), and [MDEC output](https://psx.wiki/tests/mdecout/).

## Verification

- `cmake --preset ci` plus targeted build and selected state/API/idle/CD CTests — exit 0, 16/16.
- `ctest --test-dir build/ci -L perf --no-tests=error --output-on-failure` — exit 0, 19/19.
- `cmake -DROOT=. -DCASE=spu_registration -P tests/cmake/workflows.cmake` — exit 0; checks hosted workflow wiring for both idle Cachegrind cases.
- `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=2700 -P tests/cmake/line_budget.cmake` — exit 0, 2,699/2,700 lines.
- Inventory ordering and `git diff --check` — exit 0.
- The fake Cachegrind launcher smoke-tested the CMake handoff and allowance comparator only. It did not measure instructions. Valgrind is not installed locally; the real measurement is configured for the hosted perf job, so no hosted run URL is available yet.
- The initial attempt to select `policy.spu_registration` through the `ci` CTest tree found no tests because that policy test belongs to the hygiene lane. Running its script directly succeeded; the workflow was not changed to hide the selector mismatch.

## Remaining acceptance

DISC-01, DISC-03 and DISC-04 remain partial until the owned-disc boot/playback, complete MDEC guest, runner and libretro disk-control callers, and hosted checks pass. Public hardware records reduce uncertainty, but do not establish bit-exact behavior for every PS1 revision. The owner has a real console and capture card but has no time for a capture now; no owner run is scheduled. No hosted run URL applies yet.

## Next

Continue with Plan 06-15, which boots the owned disc through OpenBIOS and exercises ReadN. Its fixture host must compute and pass the disc identity from host-owned data before attaching the API v6 descriptor.
