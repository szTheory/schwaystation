---
phase: 15-guncon-and-justifier
fixed_at: 2026-11-25T16:35:49Z
review_path: .planning/phases/15-guncon-and-justifier/15-REVIEW.md
iteration: 1
findings_in_scope: 2
fixed: 2
skipped: 0
status: all_fixed
---

# Phase 15: Code Review Fix Report

**Fixed at:** 2026-11-25T16:35:49Z
**Source review:** `.planning/phases/15-guncon-and-justifier/15-REVIEW.md`
**Iteration:** 1

**Summary:**
- Findings in scope: 2
- Fixed: 2
- Skipped: 0

## Fixed Issues

### CR-01: Display enable changes the no-executable BIOS timing path

**Files modified:** `src/frame.c`, `src/gpu.c`, `tests/gpu/beam.c`
**Commit:** `d7ab38e`
**Applied fix:** Restored generic beam activation to executable-loaded or nonzero display mode. Frame-start scheduler synchronization now runs only when a direct gun was present before or after input latching. An inactive mode-zero GPU may arm a valid light sensor before returning idle. Added checks that enabled mode-zero output alone leaves the generic beam idle and that an armed gun still samples a bright pixel at display mode zero. The existing `built.openbios.shell` check pins and verifies the exact 240-frame hash `df2704d7cf3f01ff70ab4534a443729bba77bf73e3488c836513f9f8b86a9372`.

### CR-02: Validated multitap state can route a gun poll into two-entry arrays

**Files modified:** `src/pad.c`, `tests/core/test_state.c`
**Commit:** `f30c7fd`
**Applied fix:** State loading rejects GunCon and Justifier devices in every multitap endpoint, independent of the saved multitap flag. `poll_reply` returns `UINT8_MAX` for a gun endpoint at index 2 or higher before accessing direct-gun arrays. Added checksum-correct malformed states for GunCon and Justifier on enabled taps; the existing refusal helper confirms the state and host audio ring remain unchanged.

## Verification

Verification ran in the main checkout (`workflow.use_worktrees` is disabled).

- Red check before CR-02 implementation: `ctest --test-dir build/ci -R '^core[.]state[.]rejects$' --output-on-failure` — exit 8, malformed gun endpoints were accepted.
- Preflight: `git diff --check`, sorted GPU/core inventory checks, and `cmake --preset ci` — exit 0.
- Focused build: `cmake --build --preset ci --target schwaystation-run schwaystation-test-gpu-beam schwaystation-test-state schwaystation-test-sio` — exit 0.
- Focused regressions: `ctest --test-dir build/ci -R '^(built[.]openbios[.]shell|gpu[.]gun[.]sample|core[.]state[.]rejects|sio[.]gun[.](latch|guncon|justifier|state))$' --output-on-failure` — exit 0; all 7 selected tests passed in 1.58 seconds.
- Differential check established that removing unconditional frame-start GPU synchronization restored the exact OpenBIOS hash; the committed gun-scoped path retains it.

All test commands completed synchronously; no fuzz campaign, Docker resource, or background probe was started. Process-table inspection was unavailable in this host, but CTest returned after each selected child test completed.

---

_Fixed: 2026-11-25T16:35:49Z_
_Fixer: the agent (gsd-code-fixer)_
_Iteration: 1_
