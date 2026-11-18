---
phase: 04-games-play-in-retroarch
reviewed: 2026-11-13
depth: standard
files_reviewed: 55
findings:
  critical: 0
  warning: 0
  info: 0
  total: 0
  necessity: 0
resolved_findings:
  critical: 2
  warning: 1
status: passed
---

# Phase 4 source review

The staged production review and subsequent affected-file reviews have no unresolved production findings at source head `50c927b3a1d3746b3d3e7c50de539b90b4dae8f7`. This is a code-review disposition, not a claim of final phase verification or hosted CI success.

## Scope and preservation

The stable review covered 36 production files at `8c3bdcb`; the later review covered 19 other production files at `c721ea4`. Their original findings and frozen-source evidence are retained in `04-REVIEW-STABLE.md` and `04-REVIEW-REMAINING.md`. A fresh manifest comparison found 34 stable and 13 remaining files byte-identical to their reviewed snapshots. Only the eight files below needed later delta disposition. The companion fingerprint records the actual current bytes for all 55 files. No full unchanged-source re-review was performed.

| Changed files | Review disposition |
|---|---|
| `src/card.c` | Pending write-error reply is serialized/restored separately from receiver-owned fresh-card metadata, image and generation. Registered caller regression and invalid-state controls were integrated in `c14ed86`; the later visitor-label rename changes no serialized values. |
| `libretro/libretro.c` | Region changes apply on explicit content reload. Runtime option refresh preserves the active instance, loaded program and SAVE_RAM lifetime; input options still update live. Repair and real caller regression landed in `7f27339`. |
| `src/cpu_exec.inc` | Read/write trace timestamps move after the actual KSEG1 write-queue drain; charges and access order remain unchanged. Repair `5cc1dc2` retains isolated/log paths; queue-depth controls independently check access gaps and total ticks. |
| `src/stub_kernel.c`, `tests/fixtures/src/stub/stub.asm` | Inactive B0(17h) branches to the existing bounded stop. Active IRQ context restoration, guest acknowledgement and registration remain unchanged. Generated image matches the assembly; directed return/context regressions and actual source-derived overlap-caller progress are recorded in `04-58-SUMMARY.md`. |
| `src/gpu_span.c` | Fixed-point line color and spatial tie choices are bounded and reviewed against directed octants/descending half-channel controls and unchanged hardware captures. The documentation identifies the model's confidence; it does not claim uniquely measured physical precision. |
| `src/gpu.c` | Horizontal VRAM copies read each bounded 16-word batch before writes, retaining forward row order, wrapping, mask policy and costs. Tests include overlapping widths 3/16/17 and the exact hardware reference. Subsequent status changes preserve the real DMA busy guard and qualify CPU-fed STAT28 by DMA direction and parser phase. |
| `src/gpu_triangle.c` | TPage bit 11 uses the already serialized GP1(09h) gate, including retained-latch and clear behavior; unsupported GPU-v0 handling remains explicit. No field, schema or texture-sampling behavior was added. |

## Resolved findings

- The two stable critical findings (live region destroying content/card spans; omitted pending card error) and the remaining warning (pre-drain access timestamp) are fixed in the commits above with registered regressions. The original reports retain historical `issues_found` status for their frozen bases; those statuses do not describe current source.
- The BIOS-identity finding was retracted after inspecting the correct frozen bus-state dependency: identity was already serialized and checked. No redundant identity field was added.
- The proposed runner frame-count-overflow issue was retracted: per-digit parsing rejects overflow before multiplication; the real invalid-input caller exits with usage status.

## Oracle and status review limits

Exact source-bound alternates for the line guest's unset fourth color and the transparency guest's different clear color are reviewed exceptions, not tolerances or skipped regions. Their EXE digest, case, exact location/count and refusal controls bind them; all remaining pixels compare to the unchanged captured words. Plan 47 records actual ten-capture execution. Plan 48 adds independently executed register predicates against three pinned hardware logs. The DMA log driver honors the core's actual stall before sampling, matching the CPU seam. All captured pixels and hardware log expectations remain unchanged.

The status fix in `50c927b` was reviewed at its actual merged source boundary. GP1(09h)/TBY2 is confined to the observed latch/status behavior; source terminology does not justify an invented texture-bypass shader or second VRAM bank. FIFO-space DREQ reporting changes without weakening actual DMA transfer readiness. Integer-only, instance-owned state and bounded execution contracts remain intact.

Final local and hosted verification, final test/tool/doc integration and requirement-level acceptance are recorded separately after the coherent source head is tested. A later production change invalidates only its affected-file disposition and must be reviewed before final phase verification.
