# Phase 8 planning coverage

| Source | Item | Plans | Coverage |
|---|---|---|---|
| Goal | Per-game cards, card interchange and recovery on reopening | 01–10 | Covered |
| Requirements | SAVE-01 normalized and content-derived identity | 02, 03, 08 | Covered |
| Requirements | SAVE-02 validated import and explicit export | 02 | Covered |
| Requirements | SAVE-03 two slots, two games, restart and atomic persistence | 01–03, 08 | Covered |
| Requirements | SAVE-04 in-process and hosted libretro save continuation and claim | 01, 02, 05, 07, 09, 10 | Covered |
| Requirements | SAVE-05 guest PocketStation storage | 01, 04, 06, 09 | Covered |
| Context | D-01 opt-in runner serial with literal override | 01–03, 08 | Covered |
| Context | D-02 first-member serial or full ordered content digest, identity frozen at open | 02, 03, 08 | Covered |
| Context | D-03 independent libretro basename persistence for EXE, disc and M3U | 05 | Covered |
| Context | D-04 raw and supported full-card format validation | 02 | Covered |
| Context | D-05 transactional card import, export and publication | 02, 03, 05 | Covered |
| Context | D-06 runner host helpers, isolation and fresh-process reload | 01, 03, 08 | Covered |
| Context | D-07 SAVE_RAM reinjection, independent card 2 and claim after real hosted proof | 01, 05, 07, 10 | Covered |
| Context | D-08 cheap preflight before builds and hosted tests | 01, 06, 07, 09, 10 | Covered |
| Context | D-09 supported PocketStation storage and metadata, retained ordinary card protocol | 01, 04, 06 | Covered |
| Context | D-10 complete writes only, pinned transcript and scope reconciliation | 04, 06, 09 | Covered |
| Research | Existing SHA-256 and full canonical TOC/sector provider. Measure startup cost. | 03, 08 | Covered |
| Research | GME directory fields, VGS and PSV full-card structure, and single-save refusal | 02 | Covered |
| Research | Source-pinned flash range, fixed metadata, bounded FUNC 01 and guest transcripts | 04, 06 | Covered |
| Research | Real RetroArch process lifecycle and fixture artifact wiring at existing caps | 01, 05, 07, 09, 10 | Covered |
| Research | Existing host/core split, data ownership, deterministic validation and approved provenance | 02–08 | Covered |
| Assumption delta | Promote the save-game identity into a host resolver and freeze both slot paths over M3U swaps | 03, 08 | Covered |

The implementation resolves the exact wrapper fields and PocketStation mappings against the pinned PSX-SPX source before producing code or vectors. The hosted run checks the actual RetroArch lifecycle. Code and asset scope follows the Phase 8 context and requirements.

| Wave | Plans | Inputs needed |
|---|---|---|
| 1 | 01 | Existing runner fixture, host claim checker and fixture producer |
| 2 | 02, 04 | 01 registered fixture and claim coverage |
| 3 | 03, 05, 06 | 02 host path/format helpers; 04 PocketStation contract and state |
| 4 | 07, 08 | 05 working frontend card handling; 03 runner resolver |
| 5 | 09 | 06 completed serial state, 07 hosted wiring, 08 runner disc resolver |
| 6 | 10 | 09 reviewed source and focused checks |

Baseline preflight measured 3,901 repository gate lines and 300 RetroArch harness lines, both at their existing caps. Existing fixture inventory order passed. No package installation is planned.
