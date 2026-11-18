---
phase: 06-discs-and-fmv
plan: "17"
subsystem: runner-disc-input
tags: [runner, disc, input, probe]
actuals:
  tasks: 2
  files: 12
  duration: "Active time not measured reliably after the 22-hour pause."
requirements-completed: []
coverage:
  - id: DISC-02-RUNNER-LOAD
    description: "The runner attaches CUE, M3U, and cooked ISO media through the shared provider; BIOS boots from the owned CUE and playlist images reach the sealed mailbox."
    requirement: DISC-02
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^runner\\.disc|^runner\\.inputs\\.|^runner\\.exit_table$|^runner\\.usage\\.' --no-tests=error --output-on-failure (exit 0, 16/16); no hosted run URL applies."
      status: pass
    human_judgment: false
  - id: DISC-02-UNSUPPORTED-PROBE
    description: "Direct and nested CHD/PBP/ECM/CCD inputs report the named unsupported format and exit 77, including CUE probing when the unsupported child contains readable raw sectors."
    requirement: DISC-02
    verification:
      kind: integration
      ref: "runner.disc.unsupported in the final focused CTest selector (exit 0); no hosted run URL applies."
      status: pass
    human_judgment: false
  - id: DISC-04-RUNNER-DISC-CONTROLS
    description: "Frame 299 leaves the movie untouched; frame 300 applies the digital Cross edge, opens the lid, changes the provider TOC to playlist entry 1, and closes the lid. The runner executes through frame 360 and repeated output matches."
    requirement: DISC-04
    verification:
      kind: integration
      ref: "runner.inputs.disc_apply and runner.inputs.disc_controls in the final focused CTest selector (exit 0); no hosted run URL applies."
      status: pass
    human_judgment: false
  - id: DISC-02-CLI-CONTRACTS
    description: "Disc runs reject unsupported state hashing before emulation, and the help exit table matches the runner manual."
    requirement: DISC-02
    verification:
      kind: integration
      ref: "runner.disc.load and runner.exit_table in the final focused CTest selector (exit 0); no hosted run URL applies."
      status: pass
    human_judgment: false
metrics:
  duration: "The plan resumed after the owner-reported 22-hour pause; active duration was not tracked reliably."
  runtime: "Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable); standard gsd-code-reviewer review."
completed: 2026-11-17
status: complete
---

# Phase 6 Plan 17: Runner disc loading and controls

The headless runner now boots CUE and M3U media through OpenBIOS, attaches cooked ISO images, probes supported
playlists through the shared provider, and rejects unsupported formats with their named status. Frame-indexed
scripts can raise the lid, select a playlist entry, and close the lid while preserving source order. The owner has
a real PlayStation and capture card but has no time for a hardware capture now; the local owned-image BIOS path
provides the automated fallback for this plan.

## Work completed

- Added `--disc FILE` for CUE/BIN, cooked ISO, and M3U input, requiring `--bios`; the runner keeps provider file
  handles alive until after instance destruction and sets the licensed CD profile needed by the BIOS disc path.
- Added frame-indexed `lid=open`, `disc=N`, and `lid=closed` controls. The entire script is validated before
  instance creation; equal-frame operations retain source order, and swaps with a closed lid or an invalid index
  are refused before a frame runs.
- Added unsupported direct/nested media status coverage. CUE probes preflight the shared provider so unsupported
  child extensions are refused even when their files contain readable raw sectors; supported CUE metadata still
  uses the existing probe path.
- Added a frame-boundary regression that confirms no event applies by frame 299 and that all four events apply at
  frame 300, including the actual pad edge, alternate TOC, and final closed lid. The runner integration test now
  runs through frame 360, past the guest's mailbox completion at frame 274.
- Rejected `--hash-state` with `--disc` before starting because provider-backed discs do not yet supply the stable
  state identity required by serialization. The help synopsis and manual make the limitation clear.
- The standard code review finished clean after correcting a quadratic input conflict scan, incomplete provider
  link wiring, the disc BIOS license profile, and test coverage that previously stopped before frame 300.

## Verification

- `git diff --check && LC_ALL=C sort -c tests/runner/inventory.txt` — exit 0.
- `cmake --preset ci` — exit 0.
- `cmake --build --preset ci --target schwaystation-run schwaystation-run-test schwaystation-test-run-stop schwaystation-test-runner-disc` — exit 0.
- `ctest --test-dir build/ci -R '^runner\\.disc|^runner\\.inputs\\.|^runner\\.exit_table$|^runner\\.usage\\.' --no-tests=error --output-on-failure` — exit 0, 16/16; 2.46 seconds.
- Standard code review — clean; no remaining findings.
- No hosted run URL applies. No physical PlayStation capture is claimed or needed for this plan.

## Remaining phase acceptance

This plan completes the runner portion of the disc and disk-control requirements. Independent disc identity, libretro disk control, per-platform equality, fuzz replay, and the hosted RetroArch smoke remain with later Phase 6 plans.

## Next

Continue with Plan 06-18: libretro extended disk control and disc PCM.
