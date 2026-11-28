# User guidance and public-claim audit

Plan 17-01 records the source evidence and initial disposition for each inspected claim. Plan 17-08 closes the
reproduced documentation drift after Plan 06 selected the existing `gpu_motion` frame-120 compatibility predicate.
The selected row adds one named fixture observation only; it does not change any hardware behavior or broaden
title, profile, or device claims.

| Area and claim | Consumer and source | Status | Evidence and disposition |
| --- | --- | --- | --- |
| Core targets, built-in test card, PS-X EXE path and supported disc subsets | `README.md`; `docs/runner.md`; `libretro/CMakeLists.txt` | Supported, with scope limits | Current runner/provider options and libretro extension projection include CUE, ISO, M3U, ECM, CloneCD and the supported unencrypted PSISO PBP subset; CHD is conditional on the build. Keep the test-card and PS-X EXE claims separate from retail-game playability. |
| All modeled console revisions and regions | `README.md`; `docs/embedding.md`; profile tests and required-capability matrix | Supported as machine-profile availability; title support unmeasured | The project exposes machine profiles and region timing. The compatibility matrix currently has 27 required profile rows, all unknown; do not imply that each profile or retail title has passed a complete session. Existing README caveat is appropriate. |
| Compatibility results are fixture-limited; libretro stays experimental while required checks are unknown | `README.md`; `docs/compatibility.md`; generated info template | Current; selected fixture row added in Plan 07 | The Redump inventory has 13,407 source entries, all unknown. The ledger now has five measured fixture rows, including the Plan 07 `gpu_motion` frame-120 hash. Required capability results remain unknown (including all 27 profile rows). Latest release `.info` says `is_experimental = "true"`. Preserve these limits. |
| macOS Apple Silicon install command downloads a same-release checksum list and installs the core and info file | `README.md`; current `v0.16.0` release; release `install-line-latest` workflow | Verified | The verbatim installer succeeded against latest `v0.16.0` and installed both expected paths. Its checksum control refused a copy with one flipped byte and installed nothing. The published release run is [45923155197](https://github.com/szTheory/schwaystation/actions/runs/45923155197). |
| Current release asset names and package availability | `docs/maintaining.md`; GitHub release assets | Current | The latest release has 18 zip assets plus `SHA256SUMS`; the documented cross-platform naming and the README macOS arm64 asset exist. No asset rename is needed. |
| User supplies BIOS and disc images; the project does not distribute them | `README.md`; `ASSET_POLICY.md`; publish workflow | Current | The public policy and release scan keep personal firmware and commercial media out of project assets. Preserve the user-owned-content boundary. |
| Runner frame hashes are portable | `README.md`; `docs/testing.md`; hash-equality CI job | Supported by existing cross-build gate | The CI workflow compares hash outputs across six native build legs, BE32 and WASI. The README example is a deterministic test-card result, not a game-playability result. |
| Embedding library is C17, instance-owned and host-driven | `docs/embedding.md`; `CMakeLists.txt`; public header | Supported as API contract | The source-tree target exports the public header/package, and the documented allocator, BIOS and disc bytes are host supplied. The API contract states no environment, filesystem, wall-clock, thread or floating-point dependency. No claim change is indicated by this tune-up. |
| Runner formats, bounded input behavior and exit guidance | `docs/runner.md`; runner/provider implementation and tests | Current; compatibility remains scoped | The user guide describes the shared provider paths and bounded file/input checks. It does not establish that every supported image or title completes a playable session; the compatibility report remains the source for measured predicates. |
| Compatibility inventory and generated result counts | `docs/compatibility.md`; `tests/compat/ledger.csv`; `tests/compat/required-capabilities.csv` | Current snapshot, with explicit unknowns | The checked-in report accurately separates the 13,407-entry DAT inventory, fixture results and required capabilities. It makes no broad compatibility claim. Regeneration freshness remains owned by the existing policy test. |
| Repository-check and RetroArch test-code ceilings | `docs/testing.md`; `tests/hygiene/tests.cmake`; `tests/cmake/line_budget.cmake` | Current | The active repository-check limit is 4,694 and the measured count is 4,694; the RetroArch limit and measured count are both 300. Plan 17 does not raise either ceiling. |
| Published quality values | `docs/testing.md` performance section; `quality.json` and current hosted producers | Corrected | `docs/testing.md` now points to canonical `quality.json` bounds and the Plan 03 producer report for exact observed values, configurations, dates, and the WASI evidence limit. It no longer repeats stale metrics. |
| Archived Phase 7 evidence links | `docs/testing.md`; archived v2.0 phase artifacts | Corrected | All four relative links now target `.planning/milestones/v2.0-phases/07-faster-and-more-trustworthy-ci/`; each target exists. |
| Maintainer instructions for release, CI and pinned downloads | `docs/maintaining.md`; `.github/workflows/release.yml`, `ci.yml`, `nightly.yml`, `triage.yml` | Current; unchanged | Plan 02 found no repeated comparable full-CI workload and retained timeouts; Plan 04 found no settings, release, Dependabot, or pin-policy drift. `docs/testing.md` now describes the broader pinned tool/build/test input set accurately. No maintenance-policy rewrite is indicated. |
| Hardware-model explanations and their links | 11 `docs/hardware/*.md` pages; preparation notes and cited tests | No reproduced drift; audited pages unchanged | The five pages in Plan 08 remain byte-for-byte unchanged. The selected frame-120 fixture is evidence for one NTSC-U executable hash, not a hardware-conformance measurement, so it does not warrant a GPU hardware claim. Existing sources and boundaries remain applicable. |

## Local-link and evidence checks

- A one-off Markdown local-target scan covered `README.md` and all 24 `docs/**/*.md` pages. It found the four
  Phase 7 references above; Plan 17-08 corrected them to the archive path. The corrected targets exist, and no
  other changed local link is broken.
- `gh release view` identified `v0.16.0`, 18 zip assets, `SHA256SUMS`, and its publication date. The successful
  release workflow URL is recorded in `17-SKELETON.md`.
- The README installer and flipped-byte control are recorded with their commands and exit statuses in
  `17-SKELETON.md`; no logs or release bytes are committed.
- The README installer line is unchanged, so Plan 17-01's successful verbatim install and flipped-byte control
  remain the current end-to-end evidence. No release download was repeated.
- `docs/maintaining.md` and all five audited hardware pages are byte-for-byte unchanged. The maintenance audit
  supports retaining its no-Dependabot-auto-merge policy, one BE32 portable job, and full scheduled/manual
  Nightly workload.

## Plan 17-08 final dispositions

| Original audit item | Final disposition | Path and evidence |
| --- | --- | --- |
| Core targets, built-in test card, PS-X EXE path and supported disc subsets | Retained with existing scope limits. | `README.md`, `docs/runner.md`; source and consumer audit above. |
| Modeled revisions and regions | Retained as machine-profile availability only; no title support claim added. | `README.md`, `docs/embedding.md`; all 27 required profile rows remain unknown in `docs/compatibility.md`. |
| Fixture-limited compatibility and experimental libretro status | Updated the audit to five measured fixture rows, including the selected frame-120 predicate; all 13,407 Redump entries and required capability rows remain unknown. | [`17-IMPROVEMENT-EVIDENCE.md`](17-IMPROVEMENT-EVIDENCE.md), [`ledger.csv`](../../../tests/compat/ledger.csv), [`docs/compatibility.md`](../../../docs/compatibility.md). |
| macOS Apple Silicon installer and checksum behavior | Retained unchanged; Plan 17-01's verbatim install and flipped-byte rejection remain the evidence. | [`17-SKELETON.md`](17-SKELETON.md); [release workflow run 45923155197](https://github.com/szTheory/schwaystation/actions/runs/45923155197). |
| Release assets and package names | Retained; no asset rename or availability issue was found. | [`17-UPGRADE-AUDIT.md`](17-UPGRADE-AUDIT.md), “Active downloaded pins and reference inputs.” |
| User-owned BIOS and disc boundary | Retained; project asset policy and publish checks remain unchanged. | `README.md`, `ASSET_POLICY.md`, release audit in [`17-DOC-AUDIT.md`](17-DOC-AUDIT.md). |
| Portable runner frame hashes | Retained as test-card/fixture evidence, not retail-game playability. | `README.md`, `docs/testing.md`, six native lanes plus BE32/WASI hash comparison. |
| C17 embedding API and host-supplied inputs | Retained; no API or host-boundary drift was found. | `docs/embedding.md`, `CMakeLists.txt`, public header. |
| Runner formats, bounded inputs and exit guidance | Retained; supported formats do not establish that every image or title completes a session. | `docs/runner.md`; compatibility evidence remains separately scoped. |
| Compatibility inventory and generated result counts | The checked-in report currently shows five fixture rows and 27 unknown profile capabilities. Final generator/freshness execution is reserved for Plan 17-09. | `docs/compatibility.md`; current input identities recorded below. |
| Repository-check and RetroArch code ceilings | Retained at 4,694 counted lines and 300 lines. | `tests/hygiene/tests.cmake`, `tests/cmake/line_budget.cmake`; Plan 17-08 budget command passed. |
| Published quality values and performance assumptions | Corrected by linking the canonical metric registry and producer report; low-power rates are labeled assumptions and Wasmtime is described as hash-determinism coverage, not browser speed. | [`docs/testing.md`](../../../docs/testing.md), [`quality.json`](../../../quality.json), [`17-BASELINES.md`](17-BASELINES.md). |
| Archived Phase 7 evidence links | Corrected all four references to the v2.0 archive. | [`docs/testing.md`](../../../docs/testing.md); link-target existence check passed. |
| Release, CI, pin, and update maintenance policy | Retained after the live audit; no Dependabot auto-merge, one BE32 portable job, and full scheduled/manual Nightly are preserved. | [`docs/maintaining.md`](../../../docs/maintaining.md), [`17-UPGRADE-AUDIT.md`](17-UPGRADE-AUDIT.md), [`17-CI-EFFICIENCY.md`](17-CI-EFFICIENCY.md). |
| Hardware explanations and source confidence | GPU, SPU, CD-ROM, MDEC, and SIO pages are unchanged. The added frame-120 hash does not establish silicon conformance or performance. | `docs/hardware/{gpu,spu,cdrom,mdec,sio}.md`; Plan 01 source audit, Plan 04 pin/image audit, and [`17-IMPROVEMENT-EVIDENCE.md`](17-IMPROVEMENT-EVIDENCE.md). |

### Compatibility freshness handoff to Plan 17-09

The current checked-in report already displays the Plan 07 frame-120 row, and the unknown title/profile/provider
combinations and experimental metadata remain unchanged. Plan 17-08 did not run `scripts/compat-doc.sh` or edit
`docs/compatibility.md`; Plan 17-09 owns the final regeneration and freshness check. At closeout, the input/output
Git blob identities were:

| File | Git blob |
| --- | --- |
| `tests/compat/ledger.csv` | `a8ce81781888fe2c7dedb80ee2f61e85a2ff5ccf` |
| `tests/compat/features.csv` | `fc35b20972c1239c9d88622aa9c7bc2362773ce2` |
| `tests/compat/required-capabilities.csv` | `5efa0ebf0b8a96653be7b2d5aa2b77d37354fab8` |
| `scripts/compat-doc.sh` | `5c046e1d938669d353fc93682e4b86b9b6e9a891` |
| `docs/compatibility.md` | `493da6834801a42e64948e79c43c3967a5966f75` |
