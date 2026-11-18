---
phase: 06-discs-and-fmv
plan: "23"
subsystem: ci-and-host-integration
requirements-completed: []
coverage:
  - id: P23-LOCAL-DISC
    description: The local disc, CD-ROM, XA, and owned fixture selectors pass on the reviewed integration.
    requirement: DISC-02
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -L disc --no-tests=error --output-on-failure (25/25, exit 0)"
      status: pass
    human_judgment: false
  - id: P23-HASH-SEAM
    description: The runner hash predicate producer seam and local registration checks pass.
    requirement: DISC-02
    verification:
      kind: integration
      ref: "cmake --workflow --preset ci; runner.hashes_file (included in the full workflow); cmake -DROOT=. -DCASE=disc_registration -P tests/cmake/workflows.cmake"
      status: pass
    human_judgment: false
  - id: P23-HOSTED-EQUALITY
    description: Cross-platform hosted equality has no run evidence on this reviewed source.
    requirement: DISC-02
    verification:
      kind: integration
      ref: "Hosted CI not run; local cmake --workflow --preset ci exits 8 on mdec.guest_input."
      status: pending
    human_judgment: false
  - id: P23-NIGHTLY
    description: Nightly CD/MDEC references and bounded fuzz campaigns have no hosted run evidence.
    requirement: DISC-03
    verification:
      kind: integration
      ref: "Hosted nightly not run; no hosted run URL exists."
      status: pending
    human_judgment: false
  - id: P23-RETROARCH
    description: The hosted background RetroArch disc-frame comparison remains unverified.
    requirement: DISC-04
    verification:
      kind: e2e
      ref: "Hosted RetroArch job not run; no hosted run URL exists."
      status: pending
    human_judgment: false
status: gaps_found
runtime_model: "Codex/OpenGSD 1.20.0; primary GPT-6 session, exact model ID unavailable; gsd-code-reviewer gpt-6-luna xhigh. Active duration not measured."
source: uncommitted worktree based on df6b11d
---

# Phase 6 Plan 23: CI and Host Integration

The local integration wiring is in place and the disc-specific selectors pass. Plan acceptance remains open because cross-platform hosted equality, nightly reference/fuzz execution, and the background RetroArch comparison have no successful hosted run. The final local CI run also retains the independent MDEC acceptance failure documented in [phase verification](06-VERIFICATION.md).

## Evidence

- `cmake --workflow --preset ci` — exit 8; configure and build passed, then CTest reported 609/610 passing. The sole failure was `mdec.guest_input`; CTest's test phase took 65.77 seconds.
- `ctest --test-dir build/ci -L disc --no-tests=error --output-on-failure` — exit 0; 25/25 passed in 11.48 seconds.
- `ctest --test-dir build/ci -L mdec --no-tests=error --output-on-failure` — exit 8; 10/11 passed in 0.08 seconds. The pinned 8-bit stream differs in 12 output words; the first is actual `0x0005FFFF`, expected `0x0004FFFF`.
- `cmake -DROOT=. -DCASE=disc_registration -P tests/cmake/workflows.cmake` — exit 0.
- `cmake --workflow --preset hygiene` — exit 0; 24/24 passed.

No hosted run URL applies. The RetroArch acceptance job and complete nightly media/fuzz lane were not dispatched while the required local MDEC oracle remains red. No owner hardware capture is planned or requested.

## Plan status

The local integration implementation and its review are complete. The plan remains `gaps_found` until hosted acceptance can run on a locally passing MDEC source. No requirement is marked complete here, and no phase release is claimed.
