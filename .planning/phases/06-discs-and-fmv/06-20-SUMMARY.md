---
phase: 06-discs-and-fmv
plan: "20"
subsystem: hostile-media-fuzz
tags: [disc, mdec, fuzz, replay]
requirements-completed: []
coverage:
  - id: DISC-FUZZ-PARSERS
    description: "CUE, CNF and M3U seeds replay through the production disc image and SYSTEM.CNF parsers with bounded input and filesystem access."
    requirement: DISC-02
    verification:
      kind: integration
      ref: "cmake --preset ci (exit 0); build all ten replay targets (exit 0); ctest --test-dir build/ci -R '^fuzz\\.regress\\.(cue|cnf|m3u|cdrom|mdec|exe|gpu|memcard|sio|spu)$|^policy\\.fixture_budget$' --no-tests=error --output-on-failure (exit 0, 11/11 passed)."
      status: pass
    human_judgment: false
  - id: DISC-FUZZ-DEVICES
    description: "CD-ROM command and MDEC bitstream seeds exercise production device paths under fixed input and emulated-cycle limits."
    requirement: DISC-01
    verification:
      kind: unit
      ref: "The same 11-test replay selection passed (exit 0). A temporary inclusive CNF input-bound mutation made fuzz.regress.cnf fail as expected (CTest exit 8); the correct bound was restored, rebuilt, and the final selection passed."
      status: pass
    human_judgment: false
  - id: DISC-FUZZ-RECIPES
    description: "Source-generated regression recipes are bytewise inventoried and stay within the existing fuzz and fixture budgets."
    requirement: DISC-01
    verification:
      kind: other
      ref: "LC_ALL=C sort -c tests/fuzz/inventory.txt (exit 0); tests/fuzz and tests/fixtures measured 132 KiB and 312 KiB; direct 2,700-line gate command exited 0 at 2,699/2,700; sh scripts/hygiene.sh --tree (exit 0); git diff --check (exit 0)."
      status: pass
    human_judgment: false
  - id: DISC-FUZZ-CAMPAIGNS
    description: "LibFuzzer campaign targets are defined; local campaign linking and nightly hosted execution remain for Plan 06-23."
    requirement: DISC-01
    verification:
      kind: integration
      ref: "Campaign targets are declared under SCHWAYSTATION_FUZZ. This Mac lacks the Clang libFuzzer runtime, so no local campaign binary or run is claimed; no hosted run URL applies yet. Plan 06-23 owns the preset and nightly wiring."
      status: partial
    human_judgment: false
metrics:
  duration: "About 32 minutes wall time from the 04:35 UTC checkpoint to closeout; active time was not tracked reliably."
  tasks: 3
  runtime: "Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable)."
completed: 2026-11-17
status: complete
---

# Phase 6 Plan 20: Bounded hostile-media replay

Added five deterministic replay harnesses for CUE, CNF, M3U, CD-ROM commands and MDEC bitstreams. The parser
harnesses use the production host parsers and build-owned temporary media; the device harnesses use the production
MMIO paths. Per-campaign seed capacities and emulated-work limits keep each replay bounded. Five new CTest rows
are sorted in the fuzz inventory.

The recipes are generated from `tests/fuzz/seeds.c`, so no binary fixture was added and
`tests/fixtures/manifest.txt` correctly remains unchanged. The five CMake libFuzzer campaign targets are defined,
but their local link/run could not be verified because this Mac has no Clang libFuzzer runtime. Plan 06-23 owns
adding these targets to the explicit fuzz preset and running the campaigns in the hosted nightly lane.

## Verification

- `cmake --preset ci` — exit 0.
- `cmake --build build/ci --target schwaystation-fuzz-exe-replay schwaystation-fuzz-gpu-replay schwaystation-fuzz-memcard-replay schwaystation-fuzz-sio-replay schwaystation-fuzz-spu-replay schwaystation-fuzz-cue-replay schwaystation-fuzz-cnf-replay schwaystation-fuzz-m3u-replay schwaystation-fuzz-cdrom-replay schwaystation-fuzz-mdec-replay` — exit 0.
- `ctest --test-dir build/ci -R '^fuzz\.regress\.(cue|cnf|m3u|cdrom|mdec|exe|gpu|memcard|sio|spu)$|^policy\.fixture_budget$' --no-tests=error --output-on-failure` — exit 0, 11/11 passed.
- A temporary deliberately incorrect inclusive input bound was rejected by `fuzz.regress.cnf` (expected CTest exit 8); the source was restored and rebuilt before the final passing run.
- `LC_ALL=C sort -c tests/fuzz/inventory.txt`, `git diff --check`, and `sh scripts/hygiene.sh --tree` — exit 0. The direct equivalent of the 2,700-line gate counted 2,699 lines and exited 0. The fuzz tree measured 132 KiB and the fixture tree 312 KiB.
- No hosted run URL applies yet. No physical-console capture was performed or claimed. Public PS1 test programs and their published hardware logs provide reusable reference evidence for the nightly lane; they do not establish hardware equivalence for unmeasured behavior.

## Remaining phase acceptance

This plan adds local replay coverage but does not complete DISC-01, DISC-02 or DISC-03 phase acceptance. Hosted fuzz campaigns, all-leg evidence and the background RetroArch disc comparison remain with Plan 06-23. The owner has a PlayStation and capture card but is unavailable for a capture now; use public hardware records and automated checks, and only request a narrowly bounded owner capture later if a hardware-only result still blocks acceptance.

## Next

Continue with Plan 06-23: integrate per-leg disc equality, nightly reference/fuzz runs and background RetroArch disc smoke.
