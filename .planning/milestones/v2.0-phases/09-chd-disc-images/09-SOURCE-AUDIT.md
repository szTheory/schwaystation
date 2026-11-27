# Phase 9 source coverage audit

All four phase requirements and all nine discussion decisions have executable coverage. The approved scope is unchanged; PBP, ECM/CCD, SBI and broader CHD reconstruction remain in their declared later/deferred scope.

| Source | Item | Coverage | Status |
|--------|------|----------|--------|
| GOAL | Supported CHD behaves like its uncompressed disc | 02–07 reader/fixture/parity; 11 all lanes; 12 shipped hosts; 13 packages; 14 owned guest | Covered |
| REQ | MEDIA-01 existing media identity/TOC/sector/subcode/errors and atomic replacement | 01, 05, 06, 07, 12, 14 | Covered |
| REQ | MEDIA-02 checked limits, confined paths, fuzz and pre-guest inventory | 01, 04, 05, 07, 11, 12, 14 | Covered |
| REQ | CHD-01 pinned optional reader, supported subset, eight-lane parity, notices and claims | 02–07, 09–14 | Covered |
| REQ | COMPAT-02 feature evidence, protected prior passes, dated allowances and cheap key/count preflight | 08, 11, 14 | Covered |
| CONTEXT | D-01 existing raw-sector/subcode/two-index projection | 01, 05, 06, 12, 14 | Covered |
| CONTEXT | D-02 stored/virtual pregaps, no logical padding, explicit unsupported semantics | 05, 06 | Covered |
| CONTEXT | D-03 host-only static reader, default OFF, failure safety | 02, 05–07, 09, 12–14 | Covered |
| CONTEXT | D-04 owned run-local fixture, complete permissive generator closure, cap and identical bytes | 03, 04, 11 | Covered |
| CONTEXT | D-05 cheap geometry/boundary preflight followed by complete parity | 01, 04–07, 11, 12, 14 | Covered |
| CONTEXT | D-06 malformed/fuzz, atomic replacement and protected ledger | 05–08, 12, 14 | Covered |
| CONTEXT | D-07 enabled release artifacts after parity, accurate ON/OFF claims and notices | 09, 11, 13 | Covered |
| CONTEXT | D-08 measured exact-SHA CI reuse and live/post-run efficiency review | 10, 11; final verification and ship | Covered |
| CONTEXT | D-09 cheap checks first and proportional recurring gates | All plans; 01, 08, 09, 11, 13 and 14 carry the explicit budget/preflight contracts | Covered |
| RESEARCH | Exact static C reader closure, bundled codec options and notices | 02, 09 | Covered |
| RESEARCH | Unresolved fixture-generator complete license/build closure | 03 prerequisite; 04 cannot compile/use an uncleared route | Covered |
| RESEARCH | Metadata/padding/pregap/version/mode/subcode semantics | 05, 06 | Covered |
| RESEARCH | Hostile header/extents/path safety and replay/campaign coverage | 02, 05, 07 | Covered |
| RESEARCH | Eight-lane hash evidence and one checked ephemeral fixture producer | 04, 06, 11 | Covered |
| RESEARCH | ON/OFF metadata and archive closure, eighteen retained names | 09 configured metadata/notice gate; 13 Tasks 1+2 explicitly configure, build, package and validate ON then OFF archives; 11 retains required hosted release checks | Covered |
| RESEARCH | Runner/libretro/guest integration and evidence ledger | 08 ledger contract; 12 hosts; 14 owned guest and evidence | Covered |
| RESEARCH | Validation Architecture and no duplicate full-suite bookkeeping | Each task's automated command; 14 final verification sequence | Covered |
| TODO | 005 triggered SPDX and release notices checks | 09 gate; 13 both configurations and closeout | Covered |
| TODO | 050 exact-SHA CI cost/artifact reuse decision | 10, 11 | Covered |

## Spec-less edge resolution

The deterministic probe surfaced thirteen applicable edges and no unclassified rows. Each is resolved with an explicit testable must-have; none is dismissed or left as a hidden assumption.

| Requirement | Category | Explicit acceptance and plan |
|-------------|----------|------------------------------|
| MEDIA-01 | adjacency | Touching track/index extents retain correct exclusive boundaries; 01/05/06 |
| MEDIA-01 | empty | Empty/missing input fails and valid single-track input has explicit geometry; 01/05 |
| MEDIA-01 | ordering | TOC track order is deterministic; duplicates/out-of-order metadata fail; 05 |
| MEDIA-02 | boundary | Exact limits and one-over values are checked before allocation/decode; 05/07 |
| MEDIA-02 | precision | Addition/multiplication/narrowing cannot wrap or lose extent precision; 05/07 |
| CHD-01 | adjacency | Pregap/storage-padding/track/hunk transitions map to correct logical LBAs; 05/06 |
| CHD-01 | empty | Empty/truncated headers and degenerate track input fail explicitly; 02/05 |
| CHD-01 | ordering | Track numbers and TOC ordering are validated and stable; 05/06 |
| COMPAT-02 | boundary | Zero/one/many rows and allowance-date limits have controls; 08 |
| COMPAT-02 | adjacency | Missing/duplicate keys fail and prior rows cannot disappear without allowance; 08 |
| COMPAT-02 | empty | Empty ledger/no required evidence fails preflight; 08 |
| COMPAT-02 | ordering | Feature rows are deterministic and ordered; full-output hashes are stable; 06/08 |
| COMPAT-02 | precision | Row/count/allowance/budget totals use bounded checked arithmetic; 08 |

## Prohibition probe

The two-stage recall/precision pass was applied to each of MEDIA-01, MEDIA-02, CHD-01 and COMPAT-02. Candidate outcomes included corrupting media, path escape, hidden format reconstruction, commercial-disc bytes in fixtures, GPL source in the dependency closure, misleading capability claims, private content in reports, lost prior passing evidence, unbounded allocation and dropping CI coverage. Correctness/resource/path candidates are already owned by the explicit edge and parser tasks; source/asset/claim/coverage constraints are explicit existing AGENTS/CONTEXT rules. No additional bespoke values/safety prohibition survives as a new item, so no descriptor or human-only check is fabricated. Canon security concerns remain in the concrete MEDIA-02 bounds/confinement/fuzz tasks.

## Execution and verification boundaries

- Plans 01–14 have two or three tasks and at most ten files each. Shared files/headers/inventories force dependencies; independent ledger and read-only CI measurement start after the skeleton. Plan 13 follows parity, malformed/replay work and Plan 09 to avoid concurrent ON/OFF cache mutations; Plan 11 waits for both archive configurations, Plan 12 proves both hosts, and final Plan 14 records owned-guest evidence before review.
- The fixture generator is not approved by planning. Its exact full closure audit is Plan 03's prerequisite; missing clearance stops fixture use and leaves independent plans ready.
- Repository-check helpers remain within the complete 3,901-line inventory and RetroArch within 300 lines. Existing duplication must be removed without dropping cases before adding counted checks; no relocation/minification or automatic quality-bound increase is allowed.
- Every automated verify command is from the repository root. `git diff --check` in these commands intentionally checks current uncommitted edits and supplements the named behavior assertions; it is not evidence that a committed feature works.
- Source code review precedes final verification. Final required exact-source hosted CI covers all eight applicable parity lanes, all previous tests and archive rehearsal; ship then confirms merged main CI and release. Each live/completed run is inspected for critical path, duplicate work and runner-minute cost. No equivalent green full suite is repeated for bookkeeping.
- Runtime/model: Codex typed gsd-planner, gpt-6.1-sol xhigh. Estimate calibration returned factor 1, sample_count 0, confidence low. Plans retain those calibrated values. If an implementation slice grows beyond 30 minutes, split its remaining concrete work; no acceptance case is dropped.
