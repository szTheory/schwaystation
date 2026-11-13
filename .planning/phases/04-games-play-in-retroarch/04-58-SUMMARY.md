---
phase: 04-games-play-in-retroarch
plan: "58"
subsystem: cpu
tags: [irq, mips, state, libretro, gpu]
requires:
  - phase: 04-games-play-in-retroarch plan 57
    provides: Registered guest IRQ0 hook and B0(17h) interrupted-context return
provides:
  - Repeated IRQ0 context, state replay and exception-precedence regressions
  - Bounded source-derived progress evidence for the build-158 overlap caller
affects: [04-47 image checkpoint, 04-48 GPU reference logs, phase 04 verification]
actuals:
  tokens: 12995
  tasks: 2
  commits: 8
plan_head_before: c721ea4dfa4169a8106107806eb7e135f4dedcab
plan_head_after: 94224f1abca6f3a8daa30e8b168fc94a1d4c362e
tech-stack:
  added: []
  patterns:
    - Real MIPS execution for guest IRQ and return tests
    - Bounded trace-chunk observation with explicit trace-loss detection
key-files:
  created: []
  modified:
    - tests/fixtures/boot_tests.c
    - tests/fixtures/src/stub/stub.asm
    - src/stub_kernel.c
    - tests/card/card_tests.c
    - src/card.c
    - libretro/libretro.c
    - tests/libretro/host.c
    - src/cpu_exec.inc
    - tests/fixtures/tests.cmake
    - tests/built/tests.cmake
    - tests/compat/allow.txt
    - tests/compat/ledger.csv
    - docs/hardware/cpu.md
    - docs/testing.md
    - .planning/phases/04-games-play-in-retroarch/04-47-CHECKPOINT.md
key-decisions:
  - Keep the overlap image comparison at eight advances; use a separate fixed 24-advance caller-progress witness derived from measured uploads and the guest's copy/VSync work.
  - Update the interim v4 state and OpenBIOS frame pins only after isolated object controls attributed each changed hash to its reviewed source change.
patterns-established:
  - Distinguish caller control-flow progress from GPU pixel agreement.
  - Preserve the caller's IRQ acknowledgement and prove restoration at the first resumed guest instruction.
requirements-completed: []
coverage:
  - id: D1
    description: CI proves repeated registered IRQ0 delivery, architectural context restoration, live-hook state replay and SWMB/unsupported-path precedence.
    requirement: GAME-01
    verification:
      - kind: unit
        ref: "fixtures.boot.stub_irq_context, fixtures.boot.stub_irq_state, fixtures.boot.stub_irq_controls and generic IRQ/SYSCALL controls"
        status: pass
    human_judgment: false
  - id: D2
    description: The retained overlap guest completes its source-counted uploads and copies, reaches its VSync loop, and continues servicing IRQ callbacks within a fixed allowance.
    requirement: GAME-01
    verification:
      - kind: integration
        ref: "cc -std=c17 -Iinclude -Isrc -Ibuild/ci/generated build/ci/irq-overlap-probe.c build/ci/libschwaystation.a -o build/ci/irq-overlap-probe && build/ci/irq-overlap-probe build/nightly/gpu-reference/gpu/vram-to-vram-overlap/vram-to-vram-overlap.exe 24"
        status: pass
    human_judgment: false
metrics:
  duration: 86 min
  completed: 2026-11-13
status: complete
---

# Phase 04 Plan 58: Registered IRQ0 Validation Summary

**Registered IRQ0 callbacks now preserve and replay guest CPU state, and the actual overlap caller completes its upload/copy work and reaches VSync under a source-derived bound.**

## Performance

- **Duration:** 86 min, measured from the recorded recovery start at 2026-11-13T04:37:24Z through the final local checks.
- **Runtime/model:** Codex, OpenGSD 1.20.0; gpt-6-luna, xhigh.
- **Tasks:** 2.
- **Files modified:** 22 across the plan's commits before this summary.

## Accomplishments

- Added real-CPU regressions for repeated IRQ0 callbacks, independent register/HI/LO/SR/EPC restoration, live-hook save/load replay, SWMB precedence, reset-cleared registration and bounded unsupported paths. The inactive B0(17h) regression exposed a guest branch that incorrectly returned without an active saved context; the stub now enters its existing bounded spin for that case.
- Preserved the guest's ownership of `I_STAT` acknowledgement and verified that unrelated pending sources remain set. The hook enters through the registered JmpBuf, acknowledges IRQ0, returns to the interrupted EPC and continues guest work.
- Observed the retained, identity-verified build-158 overlap executable with the public trace and peek APIs. At 24 bounded advances it completed all 27,657 one-pixel uploads, all 147 VRAM-copy packets (21 each at sizes 2, 8 and 15; 84 at size 16), then made 274,241 reads of the source-derived VSync counter and received nine further callbacks after entering that loop. All 23 observed IRQs returned to their interrupted PCs; the trace anchor remained present throughout.
- Kept the independent image-comparison limit at eight advances. That earlier progress observation completed only 10,633 of 27,657 uploads, so the caller-progress allowance was corrected to 24 from observed per-advance work plus the known copy/VSync tail. This is progress evidence only; the 34,390 image differences remain unclassified in 04-47.
- Applied the reviewed card pending-error state, explicit region reload and KSEG1 trace-timestamp fixes. Updated affected state and OpenBIOS pins only after controls isolated the causes; frame, audio and frozen API v1-v3 pins remain unchanged.

## Task Commits

1. **Task 1: Expand registered IRQ context and state coverage** — `733d33c` (tests), `6677e2c` (inactive IRQ return fix).
2. **Reviewed card state repair** — `c14ed86`.
3. **Explicit region reload repair** — `7f27339`.
4. **KSEG1 trace timestamp repair** — `5cc1dc2`.
5. **Reviewed region/state/OpenBIOS pin updates** — `bb767a2`.
6. **Card field-label hygiene correction** — `db3e426`.
7. **Task 2: Record caller progress and checkpoint evidence** — `94224f1`.

## Evidence

- Task 1 focused IRQ selection — exit 0, 12/12 cases, including the three new fixtures, generic IRQ/SYSCALL controls and stub drift. The expansion found the real inactive B0(17h) branch defect; the test commit preceded the fix commit.
- The overlap progress command shown in coverage compiled with `-Ibuild/ci/generated` and exited 0. It reported 13,078,486 emulated ticks, 24 advances, 23 callbacks, all 27,657 uploads, all 147 copies, VSync reads and follow-on callbacks. The eight-advance observation is retained as the evidence for changing only the progress allowance.
- The independent raster control used this command from the repository root and exited 0 with frame hash `df2704d7cf3f01ff70ab4534a443729bba77bf73e3488c836513f9f8b86a9372`:

  ```sh
  build/ci/schwaystation-run --region ntsc-u --bios build/ci/openbios/openbios.bin --frames 240 --hash --png 240:/private/tmp/phase4-openbios-attribution/current.png
  ```

  Replacing only `gpu_triangle.c.o` with the pre-`cf9f287` object restored the previous `4d1f0ff260e2cf065ae0b7add4ba60353b2b01241dda8325309b5398ba00acec` pin — exit 0 on the same checked BIOS. Visual review found the same 640×239 cube scene, pose and colors; 474 changed pixels were all within one pixel of a color boundary. This attributes the pin update to the capture-validated raster correction.
- Replacing only `card.c.o` with the pre-card-fix object in a copy of the current archive restored the prior gpu_motion state pins at frames 150 and 180 exactly. The current values are `3a7ba868f865632a56a75fcfc9e70258313af5b4b17051e766fc2f6faa47cc83` and `653cfd5f3fc834fc64eb9c8633a589047081c329263327242d5158a02eb2e489`; the added serialized values are the two zero pending-error latches. Frame pins did not change.
- After updating expectations, the focused state/OpenBIOS/compatibility/geometry selection passed 9/9. Card checks passed 10/10; after renaming the visitor labels to avoid the no-float scanner's `1.e` false positive, the affected IRQ/state/geometry selection passed 7/7 and `hygiene.no_float` passed 1/1.
- `cmake --workflow --preset ci-j4` — exit 0; 520/520 ordinary tests and 5/5 release checks passed. This run included the reviewed card, region and trace fixes and the updated expected pins. The later card visitor-label-only change was followed by a full build and the focused checks above.
- `cmake --workflow --preset hygiene-j4 && git diff --check 013ecf93eabf0fde9530226290e4a68af8e04d15` — final exit 0; hygiene passed 23/23 and the diff check passed.

## Decisions Made

- The first eight advances were not enough for the guest's complete finite upload loop. Keep the separate image bound untouched and use 24 advances for caller-progress acceptance.
- The public trace anchor must remain recoverable across each bounded chunk; missing or ambiguous anchors fail the one-shot observation rather than silently dropping trace records.
- The actual caller witness establishes IRQ callback, acknowledgement, return and VSync progress only. Exact pixels remain under the separate 04-47 checkpoint.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Stop inactive B0(17h) calls instead of resuming without saved context.**

- **Found during:** Task 1.
- **Issue:** A direct inactive B0(17h) call branched through the active-return path.
- **Fix:** Send only the inactive path to the existing bounded spin; retain active return, SWMB priority and generic unsupported paths.
- **Verification:** Focused regression failed before the fix; 12/12 focused cases passed after it. The reviewed IRQ delta had no blocker.
- **Committed in:** `6677e2c`.

**2. [Rule 1 - Compatibility] Preserve pending card error state across saves.**

- **Found during:** Shared closeout.
- **Issue:** Transient card error replies were lost on state import.
- **Fix:** Serialize and validate the per-slot pending error bit while preserving receiver-owned fresh-card metadata.
- **Verification:** Registered card/state/API checks passed; a copied-library control with only baseline `card.c.o` restored the old state hashes exactly.
- **Committed in:** `c14ed86`.

**3. [Rule 1 - Bug] Apply region selection on explicit content reload.**

- **Found during:** Shared closeout.
- **Issue:** Changing the region during loaded content recreated the core and invalidated active memory spans.
- **Fix:** Keep the loaded region stable; apply the option on the next content load and update geometry/lifecycle expectations.
- **Verification:** Targeted lifecycle, content, input and geometry cases passed; final CI passed.
- **Committed in:** `7f27339`, with the follow-up test expectation update in `bb767a2`.

**4. [Rule 3 - Blocking] Move trace timestamps after KSEG1 write-queue drains.**

- **Found during:** Shared closeout.
- **Issue:** Trace timestamps were captured before a real bus access drained queued writes, misreporting access time.
- **Fix:** Capture timestamps after the drain without changing cycle charges or bus order.
- **Verification:** All seven trace cases passed; final CI passed.
- **Committed in:** `5cc1dc2`.

**5. [Rule 1 - Test expectation] Refresh pins for isolated, reviewed state and raster changes.**

- **Found during:** Shared closeout.
- **Issue:** The reviewed card state field changed the serialized state bytes, and the capture-validated integer-origin raster correction changed only one-pixel edge coverage in the OpenBIOS image.
- **Fix:** Update the two affected state pins and the OpenBIOS frame-240 pin with a dated compatibility allowance. Preserve frame hashes, BIOS bytes, source frame, IRQ bounds and the other capture expectations.
- **Verification:** Single-object controls restored each old hash; final CI and compatibility ratchet passed.
- **Committed in:** `bb767a2`.

**6. [Rule 3 - Blocking] Avoid a no-float scanner false positive in internal state labels.**

- **Found during:** Hygiene closeout.
- **Issue:** The names `card.slot1.error_pending` and `card.slot2.error_pending` looked like decimal exponent prefixes to the repository's text scanner.
- **Fix:** Rename only the internal visitor labels to `card.slot1.pending_error` and `card.slot2.pending_error`; serialized bytes are unchanged.
- **Verification:** Card 10/10, affected regression 7/7, `hygiene.no_float` 1/1, and final hygiene 23/23 passed.
- **Committed in:** `db3e426`.

**Total deviations:** 6 small, directly attributable fixes. No new dependency, gate, interface or state revision was added.

## Issues Encountered

The first combined CI run exposed seven failures: two gpu_motion state pins, the OpenBIOS shell hash and two geometry cases, plus two tests skipped because the shell fixture failed. Isolated controls attributed these to the reviewed card-state addition, the already capture-validated raster correction and stale live-region expectations. The fixes above updated the state/compatibility evidence and explicit-reload tests; the subsequent full CI passed. The first hygiene closeout then exposed the `1.e` label false positive, corrected with an internal label rename and verified by the final hygiene run.

## User Setup Required

None. No owner action is needed for this plan.

## Next Phase Readiness

Plan 58 is complete as the IRQ prerequisite for 04-47. The eight-advance image comparison remains unchanged and the 34,390 overlap differences remain unclassified; 04-47 still owns that work. GAME-01 remains partial, so `requirements-completed` is empty. No Phase 4 verification, ship or Phase 5 work is claimed here.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-13*

## Self-Check: PASSED

- The summary file exists and `git diff --check` passed from the measured plan base.
- All eight task/integration commits listed in the plan ledger are ancestors of the recorded pre-metadata HEAD.
- No new TODO/FIXME/placeholder stubs were found in the plan's committed diff.
