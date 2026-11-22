# Phase 11 plan coverage and execution order

Runtime/model: Codex planner, inherited model identifier unavailable; OpenGSD 1.20.0. Research used a dedicated GSD researcher. Estimate calibration returned factor 1, sample_count 0, confidence low. No library dependency is added; discovery used primary ECM documentation and the project-pinned CloneCD references. Source implementation is not part of planning.

## Multi-source coverage audit

| Source | Item | Coverage | Status |
|---|---|---|---|
| GOAL | Open ECM/CloneCD with documented track, sector and subcode behavior | 03–11 | COVERED |
| REQ | ECM-01 bounded random access, exact bytes, hostile-input rejection, counts before guests | 03,05,07,08,09,10,11,12 | COVERED |
| REQ | CCD-01 modes/indexes/pregaps/SUB, first/last reads and companion confinement | 04,06,07,08,09,10,11,12,13 | COVERED |
| RESEARCH | Host-only provider ownership; unchanged public ABI/core | 01,03,04,08,09 | COVERED |
| RESEARCH | ECM type1=2352; type2/3=2336; literal header spans; independent integrity vectors | 03,05,07 | COVERED |
| RESEARCH | Decoded-byte checkpoints, checked arithmetic, fixed read work/memory, terminator/EDC/EOF | 03,05,10 | COVERED |
| RESEARCH | Version3 single-session CCD subset and Entry/INDEX reconciliation | 04,06,07 | COVERED |
| RESEARCH | Same-stem canonical companion containment before content access | 13,06,10 | COVERED |
| RESEARCH | Independent asymmetric SUB vector plus distinct valid Q/GetlocP proof | 06,09 | COVERED |
| RESEARCH | Deterministic producers, output closure, source/request/output preflight and identity | 07–12 | COVERED |
| RESEARCH | Protected ledger, bounded guest proof, endian/WASI/sanitizer replay | 07–12 | COVERED |
| RESEARCH | Existing trusted-files boundary and separate Main/Release gates remain intact | 02,12 | COVERED |
| CONTEXT | D-01 complete scan, ECM termination/checksum and bounded random reads | 03,05,10 | COVERED |
| CONTEXT | D-02 exact stream/model mapping and evidence-backed rejection | 03,05,07 | COVERED |
| CONTEXT | D-03 no inferred TOC; exact raw alignment; proven CUE source path | 03,05,08 | COVERED |
| CONTEXT | D-04 fixed resource/work bounds and active-image preservation | 03,05,09,10 | COVERED |
| CONTEXT | D-05 fixture-proven CCD fields and confined same-stem companions | 04,13,06,10 | COVERED |
| CONTEXT | D-06 explicit Entry/INDEX/pregap/mode reconciliation | 04,06,07,11 | COVERED |
| CONTEXT | D-07 absent/empty/exact SUB; independent transpose and Q proof | 04,06,09 | COVERED |
| CONTEXT | D-08 superseded by D-12: metadata shortcut deferred after measured cap audit; current full route retained | 02 | DEFERRED WITH EVIDENCE |
| CONTEXT | D-09 3901-line cap; generic SPDX deferred; exact source notices | 01,02,07,09,12 | COVERED |
| CONTEXT | D-10 Main/Release separate, cheap checks before long work, act and CI timing | 02,07,10,12 | COVERED |
| CONTEXT | D-11 owned fixtures, parity/counts, shared consumers, fuzz/endian/WASI and sorted inventory | 01,03–12 | COVERED |

Excluded deliberately: generic SPDX scanner, broad CI redesign/matrix removal, undocumented CCD variants and unproven ECM sector layouts. Rejection is the planned behavior for unproven inputs, as directed by context.

## Dependency and ownership map

| Wave | Plans | Needs | Creates or expands |
|---|---|---|---|
| 1 | 01 | Existing host provider API and counted controls | Bounded private reader APIs, CMake targets and exact provider inventory |
| 2 | 02,03 | 01 APIs/inventory and refactored controls | Independent CI route; literal ECM callback |
| 3 | 04 | 01 APIs and 03 shared dispatcher edits | CCD single-track callback and confined companions |
| 4 | 05,13 | ECM03; CCD04 | Independent ECM stream/CUE expansion and CCD companion confinement |
| 5 | 06 | CCD04,13 | CloneCD index/SUB expansion after confined opening is complete |
| 6 | 07 | Both complete provider modules | Owned generated fixtures, geometry/count preflight and complete parity |
| 7 | 08,09,10 | 07 fixture and parity setup | Independent runner/probe, libretro, fuzz owners |
| 8 | 11 | Host/replay evidence | Existing guest predicates and protected feature rows |
| 9 | 12 | 02 routes plus11 evidence | Full CI producer/corpus closure, portability/package/example acceptance |

Plans03/04 serialize because both edit the shared dispatcher and existing image test. Plan13 continues CCD module/test ownership after Plan04; Plan05 is independent in the same wave. Plan06 waits for Plan13 before expanding the same CCD files. Plans08/09/10 have no shared header, inventory or source ownership. Config currently disables parallel dispatch; wave annotations expose safe independence without changing that policy. No checkpoints or user-only setup are needed.

Every plan contains at most three tasks; each task names at most five modified files. Scope is projected below 30 minutes per plan, with explicit split-on-overrun instructions. Project rule14 requires the initial CMake/inventory skeleton, so Plan01 proves a rejecting private API slice before Plans03/04 introduce successful reader paths through the already-proven host architecture. This avoids advertising successful media before full validation.

## Failure analysis and reachability

The highest-risk faults are byte/sector confusion, reciprocal fixture mistakes, canonical companion escapes and incomplete build/seed closure. Their checks precede guest/campaign work: bounded complete ECM validation, independent reconstruction/SUB vectors, pre-open path negatives, exact source/request/output preflights, then shared-host and guest proof. All artifacts have callers: dispatcher, existing CUE source path, fixture producer target, exact CTest registration, host load/probe/disk control, feature ledger or existing CI jobs.

Source review/fixes precede final phase verification. Final local/hosted checks belong to the orchestrator lifecycle after implementation; these plans do not claim that source or hosted CI has already passed. Main and Release remain separate required checks. No owner action is needed for planning.
