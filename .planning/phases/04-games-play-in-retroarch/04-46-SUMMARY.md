---
phase: 04-games-play-in-retroarch
plan: "46"
subsystem: gpu-testing
tags: [ps1, gpu, png, rgb555, ctest]
requires:
  - phase: "04-30"
    provides: Synthetic RGB555 reference checks and the prior image-input contract
provides:
  - A pinned clipping executable compared against its captured whole-VRAM image
  - A bounded Python standard-library adapter for the selected pinned PNG corpus
  - Fetch and PNG refusal controls, plus minimal startup BIOS syscall support in the test stub
affects:
  - "Phase 4 GPU hardware-reference coverage"
  - "04-47 remaining nine GPU captures"
  - "04-48 GPU logs and required lane"
actuals:
  tokens: 15557
  tasks: 2
  commits: 4
commits: 4
plan_head_before: 1d2c27da62e1d187f8e6b3efcc47a4504e4de5bd
plan_head_after: 86a695f6e2ee2d1bd9884f9a0239eb1dab7698df
tech-stack:
  added: [Python standard library for test-input decoding]
  patterns:
    - Decode only the bounded PNG formats present in the selected pinned capture set.
    - Compare every native RGB555 VRAM word against independent temporary expected pixels.
key-files:
  created:
    - cmake/GPUReferences.cmake
    - tests/gpu/reference_png.py
  modified:
    - tests/gpu/reference.c
    - tests/gpu/tests.cmake
    - tests/gpu/inventory.txt
    - tests/nightly/tests.cmake
    - tests/nightly/inventory.txt
    - tests/fixtures/src/stub/stub.asm
    - tests/fixtures/boot_tests.c
    - tests/fixtures/tests.cmake
    - tests/fixtures/inventory.txt
    - src/stub_kernel.c
    - tests/fixtures/manifest.txt
    - pins.txt
    - CMakePresets.json
    - docs/testing.md
    - docs/hardware/cpu.md
    - PROVENANCE.md
    - THIRD-PARTY-NOTICES.md
    - .github/workflows/phase4-windows-path-probe.yml
key-decisions:
  - "Use Python's standard library only to decode the ten selected PNGs into temporary canonical RGB555LE data."
  - "Judge the real clipping run by bounded progress and a full 524288-word VRAM comparison, clearing only bit 15."
  - "Support only the documented SYS(01h)/SYS(02h) startup calls in the test BIOS stub, preserving valid SWMB handler priority and the existing reset-installed entry table."
requirements-completed: []
coverage:
  - id: D1
    description: "The pinned clipping executable advances emulation and matches its captured 1024x512 RGB555 VRAM at zero tolerance."
    requirement: GAME-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/nightly -R '^gpu\\.reference\\.clipping$' --no-tests=error --verbose; 524288 words compared, one progress advance, no fault"
        status: pass
    human_judgment: false
  - id: D2
    description: "The adapter decodes the selected PNG corpus and refuses malformed or mutated input."
    requirement: GAME-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^gpu\\.reference\\.png_controls$' --output-on-failure"
        status: pass
      - kind: other
        ref: "python3 /private/tmp/phase4-independent-png-check.py; independently compared all ten outputs with installed ImageMagick raw RGB and RGB555 quantization"
        status: pass
    human_judgment: false
  - id: D3
    description: "Pinned-reference fetch controls accept a valid digest and reject a wrong digest or mutated source."
    requirement: GAME-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^gpu\\.reference\\.fetch_controls$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D4
    description: "Minimal startup syscalls let the actual clipping guest boot while preserving SWMB exception behavior and the original 48-byte reset-installed A/B/C table."
    requirement: GAME-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^(fixtures\\.boot\\.stub_syscalls|fixtures\\.stub\\.window|fixtures\\.stub_drift|cpu\\.exception\\.syscall_(epc|bd)|fixtures\\.trace\\.exception_loop)$' --parallel 4 --output-on-failure"
        status: pass
      - kind: other
        ref: "Compared current and pre-change generated stub bytes at entry_slots offset 0x238; all 48 bytes are identical."
        status: pass
    human_judgment: false
  - id: D5
    description: "Existing GPU frame/state hashes and audio output remain unchanged."
    requirement: GAME-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^(core\\.audio\\.pairs|fixtures\\.gpu_motion|fixtures\\.gpu_motion\\.hash\\.(60|120|180)|fixtures\\.gpu_motion\\.state\\.(150|180))$' --parallel 4 --output-on-failure"
        status: pass
    human_judgment: false
  - id: D6
    description: "The full local CI, package/release checks, and repository hygiene pass with a four-job local CTest overlay."
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci-j4; 515 tests, package step, and five release checks"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset hygiene-j4; 23 checks; repository-check budget 2687/2700 lines"
        status: pass
    human_judgment: false
runtime: "Codex/OpenGSD 1.20.0, gpt-6-luna xhigh"
duration: 96min
started: "2026-11-12T18:34:07-05:00 (earliest visible plan boundary: prior plan head)"
completed: "2026-11-12T20:09:52-05:00"
status: complete
---

# Phase 4 Plan 46: Captured GPU clipping reference Summary

**The pinned clipping guest now boots through the small test BIOS stub and matches its captured whole-VRAM image exactly.**

## Performance

- **Duration:** 96 minutes from the earliest visible boundary, the preceding plan's head at 18:34 ET; the exact executor dispatch time was not persisted.
- **Completed:** 2026-11-12
- **Tasks:** 2
- **Files changed:** 20
- **Runtime/model:** Codex/OpenGSD 1.20.0, gpt-6-luna xhigh

## Accomplishments

- Added a host-Python standard-library decoder for the bounded PNG formats in the ten selected pinned captures. Decoded pixels stay under `build/nightly`; no capture or derived pixels are committed.
- Added the nightly clipping capture case. It runs the pinned executable, requires actual progress without a fault, and compares all 524288 VRAM words against the canonical little-endian RGB555 image, clearing only bit 15.
- Added ordinary-CI fetch and PNG refusal controls and documented the setup and non-distribution policy.
- Added narrowly scoped SYS(01h)/SYS(02h) support to the test BIOS stub so the real guest starts. Valid SWMB handling and unsupported-service stop behavior remain intact.
- Kept both existing GPU state pins and all existing frame/audio pins unchanged. The 48-byte reset-installed entry table at ROM offset `0x238` is byte-identical to the pre-change image.

## Captured-oracle evidence

The actual local command was `ctest --test-dir build/nightly -R '^gpu\.reference\.clipping$' --no-tests=error --verbose` (exit 0), after building `schwaystation-run` and `schwaystation-test-gpu-reference`. It compared 524288 words with `advances=1`, `stop=budget`, `ticks=566204`, and zero tolerance. The guest loops on VSync; the judge accepts this bounded no-fault budget stop only after VRAM has advanced.

The verified archive SHA-256 is `5de0e82854c1a511697a7ade381c2d778ec4b2fff4a7d7ac79e8098c35eb859b`. The executable SHA-256 is `45817b0abfe7d1741c5c68fd47309eed9fff814e6a4615c3946a0eb11fbd3b2a`, the captured PNG SHA-256 is `1ad669775234c916fc29089e8f240fa16396159865362e1ae9fa2009d194d85f`, and the temporary decoded golden SHA-256 is `b3c356b29c2feae42dae440df74774f943e3326f4e82626bd5ea6b506ca0c0ae`.

The independent one-off check `python3 /private/tmp/phase4-independent-png-check.py` exited 0 and compared all ten canonical outputs against installed ImageMagick raw RGB values with separate RGB555 quantization. It also checked transparent RGBA samples without compositing. This was an independent local check, not a recurring gate. No hosted run is claimed for this source.

## Verification

- `ctest --test-dir build/ci -R '^gpu\.reference\.(fetch_controls|png_controls)$' --verbose` — exit 0.
- Directed SYS and generic exception tests — all eight selected cases passed.
- `fixtures.gpu_motion`, frame hashes at 60/120/180, state hashes at 150/180, and `core.audio.pairs` — all selected cases passed; expected hashes were unchanged.
- `cmake --workflow --preset ci-j4` — exit 0; all 515 CI tests passed, packages were created, and all five release checks passed.
- `cmake --workflow --preset hygiene-j4` — exit 0; all 23 hygiene checks passed.
- The first full CI attempt exposed a 20-second `core.state.record_sweep` timeout under automatic test parallelism and state-hash changes from the relocated BIOS entry slots. The local ignored CMake user overlay caps CTest at four jobs; the sweep then passed in 14.47 seconds. Tracked and hosted preset defaults remain unchanged.

## Task Commits

| Work | Commit |
| --- | --- |
| Task 1: captured clipping path and minimal guest startup support | `8ac0954` |
| Task 2: fetch and PNG controls | `b4c24ff` |
| Preserve the reset-installed BIOS table and existing state pins | `b7a4010` |
| Correct notice pin reference and provisional diagnostic timeout comment | `86a695f` |

## Key Files

- `cmake/GPUReferences.cmake` and `tests/gpu/reference_png.py` — pinned capture setup and bounded standard-library decoder.
- `tests/gpu/reference.c` — full-VRAM capture comparator and failure controls.
- `tests/fixtures/src/stub/stub.asm` and `src/stub_kernel.c` — minimal startup syscalls with stable installed table addresses.
- `tests/nightly/tests.cmake`, `tests/gpu/tests.cmake`, and their inventories — gating registration.
- `docs/testing.md`, `PROVENANCE.md`, and `THIRD-PARTY-NOTICES.md` — operation and source policy.

## Decisions Made

- Keep the canonical expected data independent from emulator output and verify every VRAM word.
- Treat a progressing, fault-free `SCHWAY_STOP_BUDGET` as a valid bounded capture result for this VSync-looping guest; still reject faults, unbuilt stops, unsupported stops, and no progress.
- Keep SYS support limited to the two documented startup services; preserve unknown service and other exception behavior.
- Use an ignored local four-job CMake workflow overlay because the tracked CI preset selects automatic concurrency and the state sweep has a 20-second bound.

## Deviations from Plan

### Auto-fixed issues

1. **Rule 2 — missing startup behavior:** The pinned guest calls SYS(01h)/SYS(02h), which the SWMB-only stub stopped. Added only these audited services and a directed CPU regression. Valid SWMB records retain priority; unknown services and other exceptions still stop.
2. **Rule 1 — capture harness stop handling:** The guest loops on VSync, so one bounded run can correctly end with `SCHWAY_STOP_BUDGET`. The judge now accepts that stop only with no fault and actual VRAM progress, followed by the exact full-image comparison.
3. **Rule 1 — serialized state drift:** The initial syscall block moved the reset-installed A/B call targets, changing two `gpu_motion` state pins. Moved the handler block after `entry_slots`; its original 48 bytes and both committed state hashes now match. No pin or golden was updated.
4. **Rule 3 — hygiene blockers:** The notice checker interpreted the repository name as a local path, and the temporary Windows diagnostic timeout lacked its required provisional rationale. The notice now names the existing `ps1-tests-gpu-captures` row in `pins.txt`; the workflow change adds only a comment and does not alter probe behavior.

## Issues Encountered

- The first hand-encoded syscall regression used the wrong opcode and initially exercised an unsupported selector. Corrected the test encoding, preserved the history, and then passed the directed test and actual clipping capture.
- The first full hygiene run failed only the notice and timeout-comment checks above. Both targeted checks passed after correction, and the full 23-check workflow then passed.
- No BIOS, disc, capture, or decoded image was added to the repository. No hosted or RetroArch run is claimed.

## User Setup Required

None.

## Next Phase Readiness

This plan completes only the clipping capture slice. GAME-01 remains pending until 04-47 verifies the remaining nine image references and 04-48 verifies the three GPU log cases and required lane. The phase coordinator continues with plan 52; no owner action is needed here.

## Known Stubs

None introduced by this plan.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-12*

## Self-Check: PASSED

- `cmake/GPUReferences.cmake`, `tests/gpu/reference_png.py`, and this summary exist.
- Commits `8ac0954`, `b4c24ff`, `b7a4010`, and `86a695f` are ancestors of the current plan head.
