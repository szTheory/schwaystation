# Phase 7 quality review

Scores are recorded before selecting a change. Scale: 1 is unusable; 2 is a demonstrated substantial gap; 3 is working with material measurable friction; 4 is verified with a bounded deficiency; 5 is verified across consequential use cases in scope.

## Scores

| Dimension | Score | Evidence and limitation | Rationale |
| --- | ---: | --- | --- |
| Performance | 4 | The 2026-11-18 baseline records three zero-spread Cachegrind samples for each of two workloads (1,619,573 and 137,752,334 Ir/frame), a 3,755,020-byte serialized state, and two exact-matching CI paths of 2,249 s and 3,293 s. [Baseline](07-BASELINES.md), [run 44218206100](https://github.com/szTheory/schwaystation/actions/runs/44218206100), [run 44253375502](https://github.com/szTheory/schwaystation/actions/runs/44253375502), [run 44307395327](https://github.com/szTheory/schwaystation/actions/runs/44307395327). Wasm module size remains unmeasured because the local WASI toolchain is unavailable. | The measured producers are repeatable and tied to exact inputs; one planned metric is still missing, so the baseline is not complete. |
| CI/CD | 3 | The 2026-11-18 exact-head sample includes successful required CI on six runner labels in [run 44307395327](https://github.com/szTheory/schwaystation/actions/runs/44307395327). The repeated full-CI group took 37:29 and 54:53, both dominated by BE32; [Plan 02's report](07-CI-EFFICIENCY.md) also found no configured Actions cache and no confirmed same-workload flaky test. | CI is working across the required matrix, but the repeated 37–55 minute critical path creates measurable maintainer delay. Plan 06 owns the exact release-PR routing change. |
| Ease of use | 4 | The released [runner guide](../../../docs/runner.md) documents BIOS, PS-EXE, disc, region, input, and error behavior; [embedding guide](../../../docs/embedding.md) covers CMake and pkg-config installation. The README's opening status is misleading; that defect is scored under documentation below. | The main task-specific paths are explained with commands and constraints, with one prominent discovery issue. |
| Architecture | 4 | The [embedding contract](../../../docs/embedding.md) documents per-instance state, host-supplied BIOS bytes, allocation ownership, bounded runs, versioned structs, and no file, clock, thread, or floating-point dependency. | The library boundary is explicit and deterministic; the documentation does not include a third-party embedder integration result. |
| Documentation/README | 2 | On 2026-11-18, [README.md](../../../README.md) says the release cannot run a BIOS or disc and lists core components as future work, while the released [runner guide](../../../docs/runner.md) documents BIOS boot and CUE/ISO/M3U paths. The [compatibility ledger](../../../docs/compatibility.md) is appropriately narrower: four passing fixture rows, not commercial-game coverage. | The README's prominent release-status statement is false and contradicts the supported runner behavior, a substantial user-facing gap. |
| Upgrade path | 2 | The 2026-11-18 [upgrade audit](07-UPGRADE-AUDIT.md) found vulnerability alerts enabled and zero open Dependabot alerts, but no `.github/dependabot.yml`, so automated version-update PRs are not configured. Most active Actions pins matched current releases; the audit identifies one non-urgent action update and a Wasmtime patch candidate with no demonstrated CLI exploit path. | Immutable pins and security alerts are present, but there is no recurring version-update route to propose reviewed pin changes. |
| Compatibility breadth | 3 | The released [compatibility ledger](../../../docs/compatibility.md), dated 2026-11-11 to 2026-11-13, records four passing NTSC-U fixture cases: one BIOS shell and three PS-EXE fixtures. It makes no commercial-title claim, and broader combinations remain unknown. | The evidence is deliberately narrow and honestly reported. It demonstrates repeatable fixture compatibility, with substantial breadth work left for later product phases. |

## Selection and tie-break

Documentation/README and upgrade path tie for the lowest score at 2. The README defect has the more direct present user impact: it tells a reader that BIOS and disc execution are unavailable even though the runner guide documents those paths. Its correction is measurable against the exact old claim and does not broaden the released compatibility claim. The missing Dependabot route is a future maintenance gap, but the audit finds alerts enabled and the active pins largely current. Therefore select **documentation/README** and leave Dependabot unconfigured in this plan.

## Selected outcome

- **Before:** `README.md` says “it does not run a BIOS or a disc yet” and describes the CPU and device work as future releases.
- **After:** Replace only that inaccurate opening status text with the currently documented runner capabilities and an explicit limit that compatibility evidence remains fixture-scoped.
- **Observable producer:** `! grep -Fq 'does not run a BIOS or a disc yet' README.md` must exit 0, followed by `git diff --check`.
- **Scope:** One README status correction. Do not add a Dependabot file or alter CI routing in this plan.

