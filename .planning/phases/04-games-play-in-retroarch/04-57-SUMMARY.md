---
phase: 04-games-play-in-retroarch
plan: "57"
subsystem: cpu
tags: [mips, irq0, stub, fixtures]
requires:
  - phase: 04-games-play-in-retroarch
    provides:
      - Existing PS-EXE fixture loader and generated stub-kernel path
      - IRQ controller and COP0 interrupt entry
provides:
  - Guest-registered IRQ0 callback through B0(19h) and architectural B0(17h) return
  - Focused guest tracer with independent expected register state
affects:
  - 04-58
  - 04-47
  - cpu
  - stub
actuals:
  tokens: 9646
  tasks: 2
  commits: 2
commits: 2
plan_head_before: 2724d5f6364dea4da2bffac0669c1fde355d6e74
plan_head_after: 5858e5cfc887d4cd9a246dde2abfc46f6be704c0
tech-stack:
  added: []
  patterns:
    - Guest-owned IRQ acknowledgement with a ROM-managed saved CPU context
    - Generated stub bytes checked against assembled source and pinned fixture provenance
key-files:
  created: []
  modified:
    - tests/fixtures/src/stub/stub.asm
    - src/stub_kernel.c
    - tests/fixtures/boot_tests.c
    - tests/fixtures/manifest.txt
    - PROVENANCE.md
    - docs/hardware/cpu.md
key-decisions:
  - Keep dispatch and return bodies out of line so existing call, vector and A/B/C slot labels and bytes stay fixed.
  - Save the hook pointer and interrupt context in the stub's private guest RAM; add no core fields or state revision.
  - Exercise the first real IRQ0 caller in the existing fixture case; leave the expanded matrix to Plan 58.
duration: 47min
completed: 2026-11-12
status: complete
requirements-completed: []
coverage:
  - id: D1
    description: A guest registers an IRQ0 hook, observes and acknowledges its pending bit, increments a guest counter, and returns with independently expected CPU state.
    requirement: GAME-01
    verification:
      - kind: unit
        ref: fixtures.boot.stub_syscalls; baseline RED exit 8 and post-commit focused CTest selection 7/7 passed
        status: pass
      - kind: other
        ref: Generated array matched the assembled image; original 48-byte slot compare at file offset 0x23C passed
        status: pass
    human_judgment: false
  - id: D2
    description: The limited B0(19h)/B0(17h) contract, private RAM range, SWMB priority, unsupported cases, and measured image size are documented.
    requirement: GAME-01
    verification:
      - kind: other
        ref: Scoped CPU-doc diff check, three focused CTests, and manifest/provenance digest checks passed; broad diff check reported unrelated existing Markdown hard breaks
        status: pass
    human_judgment: false
---

# Phase 04 Plan 57: Registered IRQ0 Hook and Return Summary

**The own stub now delivers one real IRQ0 to a guest-registered callback and restores its interrupted CPU state through B0(17h).**

## Performance

- **Duration:** 47m 8s
- **Started:** 2026-11-13T03:39:47Z
- **Completed:** 2026-11-13T04:26:55Z
- **Tasks:** 2
- **Files modified:** 6
- **Runtime/model:** Codex, OpenGSD 1.20.0; gpt-6-luna, xhigh

## Accomplishments

- Implemented B0(19h) registration and one IRQ0 callback path. The guest observes `I_STAT & I_MASK`, clears only IRQ0, increments its own counter, and calls B0(17h); the stub restores r1-r31 except scratch `k0`, HI, LO, SR and EPC.
- Replaced the tracer's circular context-memory comparison with a known guest wait-loop PC and literal GPR values set before IRQ injection. The guest callback fields are checked against the literal JmpBuf supplied by the test.
- Preserved the assembled A/B/C slot image and target labels, generated the C array from the assembled image, and recorded the distinct source and image digests.
- Documented the limited ABI, private 80000300h-800003BFh range, SWMB precedence and unsupported nested/other BIOS paths.

## Task Commits

1. **Task 1: Register, deliver, acknowledge and return one real IRQ0 callback** — `53b0d19` (`feat`)
2. **Task 2: Publish the exact own-stub layout and regenerated-image provenance** — `5858e5c` (`docs`)

## Files Created/Modified

- `tests/fixtures/src/stub/stub.asm` — Implements the narrow B0(19h) hook and B0(17h) architectural return.
- `src/stub_kernel.c` — Regenerated source-identical stub image.
- `tests/fixtures/boot_tests.c` — Adds the guest registration, callback acknowledgement, counter and return tracer.
- `tests/fixtures/manifest.txt` — Records the generated C source digest.
- `PROVENANCE.md` — Records the assembled image size and digest separately from the source digest.
- `docs/hardware/cpu.md` — Documents the supported call pair, callback fields, RAM use and limitations.

## TDD Execution

- **RED:** Against the baseline generated stub, `fixtures.boot.stub_syscalls` executed and failed the intended hook, acknowledgement and return assertions (CTest exit 8). `gsd_run check tdd-red-evidence build/ci/04-57-red.json --raw` accepted the persisted report as `RED_EVIDENCE_OK` (exit 0).
- **GREEN:** With the generated implementation restored, the focused plan command passed all seven selected CTests, both before and after the implementation commit.
- **REFACTOR:** The oracle was strengthened to use known guest state rather than read the stub's saved RAM as its own expected value. No extra source refactor was needed.
- **Commit sequencing deviation:** Implementation and GREEN already existed in the working tree when the baseline RED replay was made. To preserve honest history, test and implementation landed together in `53b0d19`; no retroactive red-only commit or history rewrite was made.

## Decisions Made

- The baseline symbol map places `entry_slots` at ROM file offset `0x23C`; the plan text names `0x238`. I preserved the measured baseline labels and compared all 48 original slot bytes at `0x23C`, without moving targets or changing state pins.
- The saved context is in the existing serialized guest RAM, with the k0 slot reserved. No public interface, core field or state revision changed.
- Plan 57 covers one first-caller IRQ0 path. Plan 58 owns expanded matrix coverage and the broad phase closeout.

## Deviations from Plan

**1. Baseline slot offset differs from the plan text.**

- **Found during:** Task 1
- **Issue:** The plan specifies the 48-byte table at file offset `0x238`, while baseline armips symbols place `entry_slots` at `BFC0023C` (file offset `0x23C`).
- **Fix:** Preserved the actual baseline labels and compared the exact 48 bytes at offset `0x23C`; kept `call_a`, `call_b`, `vector_jump` and `entry_slots` at their original addresses.
- **Verification:** Baseline and new symbol listings matched, and `cmp` of the 48-byte slices exited 0. Existing GPU state pins passed without golden changes.
- **Committed in:** `53b0d19`.

**2. TDD commit order was kept faithful to the worktree state.**

- **Found during:** Task 1
- **Issue:** The production implementation and GREEN were already present before the baseline RED replay.
- **Fix:** Recorded the genuine baseline failure and subsequent GREEN, then committed source, test, generated bytes, manifest and provenance atomically rather than manufacturing a separate earlier test commit.
- **Verification:** The final baseline replay failed the named test (exit 8); the GSD RED classifier passed; the restored implementation passed all seven focused CTests.
- **Committed in:** `53b0d19`.

**Total deviations:** 2. No implementation scope was added.

## Evidence

- Baseline RED: build `schwaystation-test-boot`, then run `ctest --test-dir build/ci -R '^fixtures\.boot\.stub_syscalls$' --no-tests=error --output-on-failure --output-junit /private/tmp/04-57-final-red-v2.xml --parallel 1` against the baseline generated array — exit 8 on the intended assertions.
- RED classifier: `gsd_run check tdd-red-evidence build/ci/04-57-red.json --raw` — exit 0, `RED_EVIDENCE_OK`.
- Full focused plan verification: `cmake --preset ci && cmake --build --preset ci --target schwaystation-fixtures schwaystation-test-boot schwaystation-run && ctest --test-dir build/ci -R '^(fixtures\.boot\.stub_syscalls|fixtures\.stub\.window|fixtures\.stub_drift|fixtures\.boot\.(default_handler|registered_handler)|fixtures\.gpu_motion\.state\.(150|180))$' --no-tests=error --output-on-failure --parallel 4` — exit 0, 7/7 passed after `53b0d19`.
- Task 2 native check: before the docs commit, git diff --check HEAD -- docs/hardware/cpu.md — exit 0; `ctest --test-dir build/ci -R '^(fixtures\.stub\.window|fixtures\.stub_drift|fixtures\.boot\.stub_syscalls)$' --no-tests=error --output-on-failure --parallel 4` — exit 0, 3/3 passed.
- The prescribed broader `git diff --check 013ecf93eabf0fde9530226290e4a68af8e04d15` returned exit 2 for existing trailing-space Markdown hard breaks in `.planning/phases/04-games-play-in-retroarch/04-REVIEW-STABLE.md` lines 57-59 and 93-94. No line in the changed CPU documentation was reported; the unrelated review artifact was left untouched.
- Generated source/image integrity: `cmp src/stub_kernel.c /private/tmp/phase4-stub-regenerated.c` — exit 0. Source SHA-256 `a4f113b8ffe100bb7dd2059a3f134c221d2159433ab264142f7e98fc523cb4fb`; assembled image size 1,552 bytes, SHA-256 `a78a9268ecc5060a5f0590d32f348aba3bfb80e0ee7bd5114151906ddc71b0f2`; manifest and provenance strings matched.
- One-shot slot check: assemble the baseline stub and the current fixture with pinned armips, extract 48 bytes at file offset 572 (`0x23C`) from each using `dd`, then `cmp` — exit 0. Baseline and current labels remain `call_a=BFC001FC`, `call_b=BFC00214`, `vector_jump=BFC0022C`, `entry_slots=BFC0023C`.
- No hosted run or RetroArch session was required for this native-only slice; these checks ran locally and have no hosted run URL.

## Issues Encountered

The first register oracle could have validated a matching save/restore offset error against the stub's own RAM. It was changed to compare CPU state with constants established by the guest before the interrupt and the known wait-loop address. The final baseline RED and GREEN runs use this independent oracle.

## User Setup Required

None. No owner action is needed for this plan.

## Next Phase Readiness

Plan 58 must add its expanded validation and broad closeout before the incomplete Plan 47 can resume. This summary does not claim full IRQ ABI coverage, overlap-ROM completion, hardware capture agreement or GAME-01 completion.

## Self-Check: PASSED

- Summary and implementation files exist.
- Task commits `53b0d19` and `5858e5c` are ancestors of HEAD.
- The persisted plan ledger measures two task commits from `2724d5f6364dea4da2bffac0669c1fde355d6e74` through `5858e5cfc887d4cd9a246dde2abfc46f6be704c0`.
