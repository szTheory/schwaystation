---
phase: 06-discs-and-fmv
plan: "39"
subsystem: MDEC
tags: [mdec, ps1-tests, dma1, 4bit]
requires:
  - phase: 06-discs-and-fmv
    provides: "Pinned MDEC guests, exact output logs, and the unsigned 4-bit candidate"
provides:
  - "Source-grounded direct MDECout DMA1 caller contract with hardware and emulator addresses reported separately"
  - "Standalone 4-bit model selector and passing pinned 4-bit guest comparison"
affects: [DISC-03, 06-13]
actuals:
  tasks: 2
  commits: 1
commits: 1
plan_head_before: fd1725c
plan_head_after: f070353
tech-stack:
  added: []
  patterns:
    - "Compare pinned output bytes at the captured DMA1 start only after the pinned caller source proves direct-pointer transfer"
    - "Keep the hardware log's stack address separate from the emulated run's stack address"
key-files:
  created: []
  modified:
    - tests/mdec/reference.c
    - tests/mdec/model.c
    - tests/mdec/tests.cmake
    - tests/mdec/inventory.txt
    - docs/hardware/mdec.md
decisions:
  - "The pinned helper writes its data argument directly to DMA1 MADR; there is no CPU copy to test. The log target and emulated DMA1 target are addresses from separate runs and may differ because the output buffer is stack-local."
  - "The unsigned 4-bit candidate matches both the standalone model vector and pinned guest output. Keep the 8-bit and ordered-status selectors gating and DISC-03 incomplete."
requirements-completed: []
coverage:
  - id: MDEC-DIRECT-DMA1-TARGET
    description: "Pinned source proves the output pointer goes directly to DMA1, and the emulator checks expected bytes at its captured transfer address."
    requirement: DISC-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/mdec-reference -R '^mdec\\.reference\\.4bit$' --no-tests=error --output-on-failure; exit 0. The pinned SCM.12 helper writes data to MDECout MADR; the test reports the hardware log target and emulated start separately."
        status: pass
    human_judgment: false
  - id: MDEC-UNSIGNED-4BIT
    description: "The isolated model vector and pinned guest match all 4-bit output bytes."
    requirement: DISC-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^mdec\\.guest_4bit$' --no-tests=error --output-on-failure; selected test passed."
        status: pass
      - kind: integration
        ref: "ctest --test-dir build/mdec-reference -R '^mdec\\.reference\\.4bit$' --no-tests=error --output-on-failure; exit 0."
        status: pass
    human_judgment: false
  - id: MDEC-8BIT
    description: "The 8-bit model and pinned guest match exact expected bytes."
    requirement: DISC-03
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^mdec\\.guest_input$' --no-tests=error --output-on-failure; exit 8 with 12 word mismatches in the 8-bit vector."
        status: fail
      - kind: integration
        ref: "ctest --test-dir build/mdec-reference -R '^mdec\\.reference\\.8bit$' --no-tests=error --output-on-failure; exit 8, first mismatch byte 6: expected 04, actual 05."
        status: fail
    human_judgment: false
  - id: MDEC-ORDERED-STATUS
    description: "The status guest reaches Done and matches the pinned trace within its unchanged cycle cap."
    requirement: DISC-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/mdec-reference -R '^mdec\\.reference\\.status$' --no-tests=error --output-on-failure; exit 8 at 19,200 x 256 cycles (4,915,200), with 82 status records, 82 input writes, zero output reads, and no Done marker."
        status: fail
    human_judgment: false
  - id: MDEC-REFERENCE-CONTROLS
    description: "Pinned-log mutation and ordering controls remain registered and reject changed input."
    requirement: DISC-03
    verification:
      - kind: integration
        ref: "ctest --test-dir build/mdec-reference -R '^mdec\\.reference\\.controls$' --no-tests=error --output-on-failure; exit 0."
        status: pass
    human_judgment: false
duration: "about 27 minutes wall"
completed: 2026-11-16
status: complete
---

# Phase 6 Plan 39: MDEC caller target and 4-bit verification

Traced the pinned helper through DMA1 setup: it sends its caller's stack-buffer pointer directly to the MDECout MADR, with no CPU staging copy. The hardware log and emulator run use separate stack addresses, so the comparator now reports both and compares pinned output bytes at the emulator's captured DMA1 start. The isolated unsigned 4-bit model and pinned guest checks pass.

## Test Evidence

| Check | Result |
|---|---|
| `cmake --preset ci` | exit 0 |
| `cmake --build --preset ci --target schwaystation-test-mdec` | exit 0 |
| `ctest --test-dir build/ci -R '^mdec\\.guest_4bit$' --no-tests=error --output-on-failure` | passed |
| `ctest --test-dir build/ci -R '^mdec\\.guest_4bit$|^mdec\\.guest_input$' --no-tests=error --output-on-failure` | exit 8; standalone 4-bit passed, combined vector had 12 8-bit word mismatches |
| `cmake -S . -B build/mdec-reference -G Ninja -DSCHWAYSTATION_INVENTORY=nightly -DSCHWAYSTATION_MEDIA_REFERENCE_ONLY=mdec` | exit 0 |
| `cmake --build build/mdec-reference --target schwaystation-test-mdec-reference` | exit 0 |
| `mdec.reference.4bit` | exit 0 |
| `mdec.reference.8bit` | exit 8; byte 6 expected `04`, actual `05` |
| `mdec.reference.status` | exit 8 at unchanged 4,915,200-cycle cap; 82 status, 82 input, 0 output, no Done |
| `mdec.reference.controls` | exit 0 |
| `git diff --check`; sorted MDEC inventory | exit 0 |

## Remaining Phase Work

The 8-bit arithmetic mismatch and ordered-status guest remain gating failures. Their selectors and the status run cap were preserved. The status-oracle completeness warning is recorded as deferred test-tooling seed `SEED-261116-osn`; it does not change this plan's implementation scope. DISC-03 remains incomplete, so Phase 6 is not ready for verification or ship.

## Runtime

Codex/OpenGSD 1.20.0; root model GPT-6 (session model id unavailable), code-review agent gpt-6-luna xhigh. Wall time from plan resume to closeout: about 27 minutes.

## Hardware-backed status counter follow-up (2026-11-17)

The SCPH-5501 status/decode capture confirms that the low 16-bit parameter counter wraps to `FFFFh` after a nonempty decode or table command consumes its final word. `src/mdec.c` now models that terminal value; the existing `mdec.status` selector checks full `8004FFFFh` after both table command forms and `FFFFh` while decode output is pending. The focused status test passes. This does not resolve the exact 8-bit output mismatch or change the preserved status guest cycle bound, so DISC-03 remains incomplete.
