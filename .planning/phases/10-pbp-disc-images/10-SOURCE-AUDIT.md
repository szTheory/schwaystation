# Phase 10 source coverage audit

All phase source items have an implementing plan. The earlier floor/SPDX fold was corrected by the
orchestrator before canonical planning; TODOs 002/003/005 remain pending under their recorded bindings.

| Source | Item | Plans | Coverage |
|---|---|---|---|
| GOAL | Players open documented unencrypted PBP and receive clear results for other variants | 04–10, 12–13 | Covered |
| REQ | PBP-01 bounded, deterministic PSISO provider; unsupported PGD/unknown layouts; independent permissive inflater | 01–10, 12–13 | Covered |
| CONTEXT | D-01 fixture-proven single-data-track subset, no unknown/audio/multi-disc claim | 04–06, 08–10, 12 | Covered |
| CONTEXT | D-02 malformed versus unsupported; failure-atomic image/output | 01, 05–09 | Covered |
| CONTEXT | D-03 checked bounds and bounded full-block validation before public open/guest use | 03, 05–07 | Covered |
| CONTEXT | D-04 separate host provider, unchanged core/public ABI, CHD OFF and no system zlib | 01–03, 06, 08–10 | Covered |
| CONTEXT | D-05 three exact MIT modular files, own shim/export header, private symbols, provenance/notices | 02–03, 09–10 | Covered |
| CONTEXT | D-06 run-local owned fixture, source/output identity/counts, all-sector and boundary parity before guests | 04, 06, 12 | Covered |
| CONTEXT | D-07 only TODO 006/051 folded, original file coverage and pending generic gates preserved | 02, 11 | Covered |
| CONTEXT | D-08 fixed 3,901-line ceiling, preserving consolidation before added checks | 01–02, 09, 11, 13 | Covered |
| CONTEXT | D-09 conservative PR Nightly routing, full schedule/manual/fuzz and hosted evidence | 11, 13 | Covered |
| CONTEXT | D-10 cheap preflights, measured CI efficiency, cases/platforms/quality retained | 01, 04, 06–07, 10–13 | Covered |
| CONTEXT | D-11 independent full Main and Release gates | 11, 13 and existing ship/release workflow | Covered |
| RESEARCH | Exact public miniz pin/digests and source-preserving private symbol aliases | 02–03, 10 | Covered |
| RESEARCH | Raw stream completion, exact input/output sizes and one-block output memory | 03, 05–07 | Covered |
| RESEARCH | Reverse-engineered TOC/PSISO claims require independent source-sector parity | 04–06, 12 | Covered |
| RESEARCH | Fixture generator and final-block behavior must be tested, no committed image bytes | 04–07 | Covered |
| RESEARCH | Gate cap fit and old path-filter assertions retain their original fault coverage | 01, 09, 11 | Covered; exact 3,899-line recipe in 10-GATE-BUDGET.md |
| RESEARCH | Applicable native/ASan/BE32/WASI consumers and eight-lane fixture/hash evidence | 07, 13 | Covered |
| RESEARCH | Actual module size/CI timing from the changed producer and no unsupported savings claim | 13 | Covered |

## Dependency and file ownership

| Wave | Plans | Shared-state reason |
|---:|---|---|
| 1 | 10-01 | Shared control-driver reduction and CMake/inventory skeleton first |
| 2 | 10-02, 10-11 | Source admission and Nightly routing own disjoint files |
| 3 | 10-03 | Inflater wrapper consumes admitted source target |
| 4 | 10-04 | Owned fixture production and preflight |
| 5 | 10-05 | Strict PSISO metadata/table validation |
| 6 | 10-06 | Public callback routing and whole-source parity |
| 7 | 10-07, 10-08, 10-09 | Fuzz, runner, libretro own separate source/test inventories |
| 8 | 10-10 | ON/OFF package/link evidence consumes both hosts |
| 9 | 10-12 | Bounded guests and protected feature ledger |
| 10 | 10-13 | Source review precedes final local/hosted verification |

No same-wave plans share a modified file. The configured execution mode remains sequential; waves document
safe independence without changing that setting. Every task owns at most five named files and has a runnable
failure-sensitive verify command. Final hosted waiting is observation of one changed-head run, not another
implementation slice or permission gate; record elapsed and active time separately.

## Scope exclusions

PGD decryption, multi-disc switching, compressed audio/unknown TOCs, generic source-wide SPDX scanning,
generic pins/floor gates, Main/Release artifact reuse, and later-phase media/peripheral/patch/cheat work are
not planned. The accepted subset and named controls are preserved in full.
