---
phase: 04-games-play-in-retroarch
reviewed: 2026-11-13T04:56:19Z
depth: standard
files_reviewed: 19
files_reviewed_list:
  - runner/main.c
  - src/bus.c
  - src/bus.h
  - src/cop0.c
  - src/cpu.c
  - src/cpu_exec.inc
  - src/dma.c
  - src/dma.h
  - src/frame.c
  - src/gpu.c
  - src/gpu.h
  - src/gpu_pixel.c
  - src/gpu_span.c
  - src/gpu_triangle.c
  - src/sched.c
  - src/stub_kernel.c
  - src/timers.c
  - src/timers.h
  - tests/fixtures/src/stub/stub.asm
findings:
  critical: 0
  warning: 1
  info: 0
  total: 1
  necessity: 0
status: issues_found
---

# Phase 4: Code Review Report

**Reviewed:** 2026-11-13T04:56:19Z

**Depth:** standard

**Files Reviewed:** 19
**Status:** issues_found

## Summary

Reviewed the explicit 19-file scope from the frozen c721ea4 snapshot, including the runner, CPU/device execution paths, and stub assembly. One warning remains: traced KSEG1 accesses are timestamped before their write-queue drain. No critical issues were found. The proposed card and region repair deltas were separately checked against their approved contracts: both pass review; neither is represented here as applied source.

## Warnings

### WR-01: KSEG1 trace timestamps omit write-queue drain cycles

**Severity:** WARNING

**File:** `src/cpu_exec.inc:485-496,511-523`
**Evidence:** Frozen source blob `55b7d9d3c6ab0d1fb0486b55f067bb0f93c52818`; timing contract in `docs/hardware/cpu.md:406-410,443-446`.
**Issue:** `TRACE_BEGIN` records the access timestamp before the KSEG1 path calls `cpu_wq_drain`, which advances the instruction's local cycle count before the bus read or write. For a traced uncached access with queued stores, the reported cycle is early by the drain length. The actual MMIO access uses the later tick, so traces can misorder or misplace accesses used to diagnose timing faults.
**Fix:** Stamp the bus access after `cpu_wq_drain` on the KSEG1 paths (for both reads and writes); retain the current timestamp for paths that do not drain the queue.

---

_Reviewed: 2026-11-13T04:56:19Z_

_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_

## Targeted Repair Delta Disposition

Reviewed `/private/tmp/phase4-trace-fix/trace-clock.patch` against the frozen c721ea4 base and its manifest. **Disposition: PASS.** In `src/cpu_exec.inc`, the trace timestamp is now captured after any KSEG1 write-queue drain and immediately before the bus operation; isolated accesses are stamped in their own no-drain branch. This implements the documented “cycles spent before access” clock without changing cycle charges or access order. The test extension covers reads and writes at queue depths 0–4 against the cold-cache empty-queue baseline and checks total ticks as a separate control. The supplied RED/GREEN record reports the old code fails the eight access-gap checks while total ticks pass, and the candidate passes those checks and all seven existing trace modes. I did not rerun the harness. The original c721 source finding above remains a base-snapshot finding until the candidate is applied and the registered checks pass in the shared tree.

## Targeted D04 Source-Variant Oracle Disposition

**Recommendation: accept a narrowly keyed alternate oracle for this fixture.** The pinned release and image-introduction sources both leave the fourth `LINE_G4` color unset: [`5f73d61` source](https://github.com/JaCzekanski/ps1-tests/blob/5f73d61b27a8d51269b54dde1c456d2d2f7dc865/gpu/lines/main.c) and [`d1a63eec` source](https://github.com/JaCzekanski/ps1-tests/blob/d1a63eec6a0cbcefa6bb22c2500907e6281f6eee/gpu/lines/main.c). The independent model in `source-variant-check.py` selects 64 geometry-derived pixels on the two specified diagonals and calculates a zero-fourth-color cyan ramp using integer interpolation, the documented dither/quantization, and blend equations; the model compares all other 524,224 words to the unchanged capture. It does not use observed framebuffer values to construct expected values.

Keep this exception tied to the exact named fixture/source and zero-fourth-color packet variant. Require the 64 explicit points to be exercised exactly, preserve exact capture comparison everywhere else, and add negative controls for nearby or initialized-color cases. Do not turn it into a tolerance, region skip, capture edit, or claim about unique hardware precision. In particular, make the row-0 arithmetic auditable: at (210,140), the preceding initialized red segment with dither −3 quantizes to RGB555 `0x001f`; ABR0 over white `0x7fff` produces destination `0x3dff`; the final zero-color segment quantizes to black and ABR0 over `0x3dff` produces `0x1cef`. Invalidate the variant if its pinned source, fixture, or packet input changes.
