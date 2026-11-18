# Phase 7: Workflow settings, runners, and pin audit

Audit date: 2026-11-18. This is a read-only review of repository settings, active workflow action pins, hosted runner labels, and pinned build/test inputs. It does not change repository settings or dependency pins.

## Repository settings and update alerts

| Check | Command | Status and result |
| --- | --- | --- |
| Compare repository settings with checked-in policy | `sh scripts/apply-repo-settings.sh --check` | Exit 0; `no drift on szTheory/schwaystation`. No credentials or private identity were printed. |
| Vulnerability alerts | `gh api repos/szTheory/schwaystation/vulnerability-alerts --silent` | Exit 0; alerts are enabled. |
| Open Dependabot alerts | `gh api 'repos/szTheory/schwaystation/dependabot/alerts?state=open&per_page=100' --jq length` | Exit 0; 0 open alerts. |
| Dependabot version-update configuration | `test -e .github/dependabot.yml` | Absent. This is an update-PR configuration gap, distinct from enabled security alerts and the empty open-alert list. GitHub documents Actions updates through a `github-actions` entry in this file: [Dependabot configuration for Actions](https://docs.github.com/en/code-security/how-tos/secure-your-supply-chain/secure-your-dependencies/auto-update-actions). |

The checked-in settings match the repository. The read-only comparison does not test write authority and no settings write was attempted.

## SHA-pinned GitHub Actions

All active `uses:` references in `.github/workflows/ci.yml`, `nightly.yml`, and `release.yml` resolve to the stated upstream version. The pin is the full release commit SHA shown in the workflows.

| Action | Active pin | Current upstream release evidence | Audit result |
| --- | --- | --- | --- |
| `actions/checkout` | `v7.0.1`, `3d3c42e5aac5ba805825da76410c181273ba90b1` | [v7.0.1](https://github.com/actions/checkout/releases/tag/v7.0.1) | Current. |
| `actions/upload-artifact` | `v7.0.2`, `cf430e030ddbb5b0abf93d22962f4752f3646cd9` | [v7.0.2](https://github.com/actions/upload-artifact/releases/tag/v7.0.2) | Current. |
| `actions/download-artifact` | `v8.0.2`, `9000827ccba6bdab643e8b6fd33ac0654aef8333` | [v8.0.2](https://github.com/actions/download-artifact/releases/tag/v8.0.2) | Current. |
| `actions/attest` | `v4.2.2`, `1e69f48acb82d1966a394da916b4c1698aa569d6` | [v4.2.2](https://github.com/actions/attest/releases/tag/v4.2.2) | Current. |
| `actions/create-github-app-token` | `v3.3.0`, `64ab729a6294000f5aa4a4bc0b73d1f32bc45510` | [v3.4.0](https://github.com/actions/create-github-app-token/releases/tag/v3.4.0) | Upstream has v3.4.0; its notes update the action's upstream dependencies. No action-specific problem or measured workflow failure justifies changing this pin in the audit task. |
| `googleapis/release-please-action` | `v5.0.0`, `45996ed1f6d02564a971a2fa1b5860e934307cf7` | [v5.0.0](https://github.com/googleapis/release-please-action/releases/tag/v5.0.0) | Current. |

The older `dependabot/fetch-metadata` mention in `.planning/preparation/ENGINEERING.md` is not used by the active workflows and is not an active action pin.

## Hosted runner labels and image updates

Run [44307395327](https://github.com/szTheory/schwaystation/actions/runs/44307395327) is a successful full-CI run with the same workload fingerprint as run 44253375502, recorded in [07-CI-EFFICIENCY.md](07-CI-EFFICIENCY.md). Its job API returned 22 entries: 18 succeeded and the four optional `title`, `perf`, `rehearsal`, and `cleanup` jobs were skipped. The matrix exercised `ubuntu-26.04`, `ubuntu-26.04-arm`, `macos-15`, `macos-15-intel`, `windows-2025`, and `windows-11-arm`; the hash-equality and required aggregate jobs also succeeded.

The current [runner-images label map](https://github.com/actions/runner-images/blob/main/README.md) maps the active labels to Ubuntu 26 x64/arm64, macOS 15 arm64/Intel, Windows Server 2025, and Windows 11 arm64 with Visual Studio 2026. The upstream [runner-images releases](https://github.com/actions/runner-images/releases) listed these recent image builds when checked:

| Image family | Current upstream release | Published |
| --- | --- | --- |
| Ubuntu 26 x64 | [ubuntu26/20261116.202](https://github.com/actions/runner-images/releases/tag/ubuntu26/20261116.202) | 2026-11-17 |
| Ubuntu 26 arm64 | [ubuntu26-arm64/20261116.190](https://github.com/actions/runner-images/releases/tag/ubuntu26-arm64/20261116.190) | 2026-11-17 |
| macOS 15 arm64 | [macos-15-arm64/20261118.0550](https://github.com/actions/runner-images/releases/tag/macos-15-arm64/20261118.0550) | 2026-11-18 |
| Windows Server 2025 | [win25/20261116.335](https://github.com/actions/runner-images/releases/tag/win25/20261116.335) | 2026-11-17 |
| Windows 11 arm64 with VS 2026 | [win11-vs2026-arm64/20261115.219](https://github.com/actions/runner-images/releases/tag/win11-vs2026-arm64/20261115.219) | 2026-11-17 |

The run API provides the labels, not an immutable hosted-image digest. The release list did not provide a current macOS 15 Intel image tag, so no Intel image version is inferred from the successful job. These are rolling hosted labels rather than repository-pinned images. The recent matching run passed on every required platform, and the inspected release metadata identifies no concrete runner change needed here; keep the labels and continue reviewing failures against the same runner evidence.

## Active package and reference pins

| Pin group | Public immutable ref and actual use | Evidence and decision |
| --- | --- | --- |
| armips | Commit `62adab4ef30da765f5cf22a451eb08a59c54dc8b`, with archive SHA-256 in `pins.txt`; `cmake/Armips.cmake` builds the fixture assembler used by `cmake/DiscFixtures.cmake`. | [Pinned upstream commit](https://github.com/Kingcom/armips/commit/62adab4ef30da765f5cf22a451eb08a59c54dc8b) resolves to the pinned full SHA (2026-08-01). Keep the immutable commit and digest. |
| mkpsxiso and build closure | `mkpsxiso` v2.30 (commit `54fb1644ed8741223583e2dcda358b75a205e214`); tinyxml2 `3dcad8e3c38e7091ae3771bf63020027f91715ce`, miniaudio `9634bedb5b5a2ca38c1ee7108a9358a4e233f14d`, filesystem `0e72911ba0f37f3eb883b181b855a5f4eb046ca1`, ThreadPool `251db61ff3e3c7b16436c9936c53e6f68ff07720`; `psxavenc` commit `bdc24e897ed09d2fd3d64cdbadc625b0673c76db`. `cmake/Mkpsxiso.cmake`, `cmake/Psxavenc.cmake`, and CI build these host tools to produce run-local disc media. | [mkpsxiso v2.30](https://github.com/Lameguy64/mkpsxiso/releases/tag/v2.30) is still the latest upstream release checked (2026-07-06); its archive and each separate source archive are hash-pinned. Keep the exact source refs and digests. |
| WASI compiler and libraries | No `wasi-sdk` pin is consumed. `.github/workflows/ci.yml` and `nightly.yml` install Ubuntu `wasi-libc` and `libclang-rt-21-dev-wasm32`; `cmake/toolchains/wasm32-wasi.cmake` selects `clang-21` and `wasm-ld-21` from the hosted image. | These distribution/image packages follow the repository's documented apt policy in `pins.txt`; the WASI lane passed in the matching CI run. Do not treat the stale `wasi-sdk` note in preparation material as an active dependency. |
| Wasmtime | `48.0.7` for x86_64 Linux, aarch64 Linux, and aarch64 macOS; each release asset has an independent SHA-256 in `pins.txt`. `scripts/wasm-test-emulator.sh` runs the pinned CLI for the wasm32 test lane; nightly CI also runs it for a benchmark. | [48.0.7](https://github.com/bytecodealliance/wasmtime/releases/tag/v48.0.7) is an LTS release. The current patched LTS release is [48.0.9](https://github.com/bytecodealliance/wasmtime/releases/tag/v48.0.9), published 2026-11-16. The [AsyncWriteStream advisory](https://github.com/bytecodealliance/wasmtime/security/advisories/GHSA-5m8g-xhq4-4h93) lists `wasmtime-wasi` versions `<48.0.9` as affected, but explicitly states that the Wasmtime CLI does not use the vulnerable stream. The separate [Wasmtime GC advisory](https://github.com/bytecodealliance/wasmtime/security/advisories/GHSA-43j2-gcp4-m22g) patches 48.0.9 and says the issue does not apply with the Wasm GC proposal disabled or without a host function that allocates and returns GC objects. The current C-built WASI module has no such host integration. There is no demonstrated exploit path in the present CLI-only lane; however, `pins.txt`'s existing policy is to take an LTS patch when it fixes a security advisory. Record 48.0.9 as the next pin-bump candidate for the normal `ci:` pin-update step; the bump must use the three new v48.0.9 asset SHA-256 values from its upstream release API, not the existing v48.0.7 digests. This audit does not modify pins. |
| OpenBIOS and cross compiler | CI fetches `pcsx-redux/nugget` commit `9db6d88d23c4da7a39b1a4b9729f1e5fff80dec9` and builds only `openbios`; `gcc-mipsel-none-elf-linux` is the hash-checked PSn00bSDK v0.24 GCC 12.3.0 build tool. `cmake/OpenBIOS.cmake` verifies size and expected digest before consumers receive the run-local artifact. | [Pinned nugget commit](https://github.com/pcsx-redux/nugget/commit/9db6d88d23c4da7a39b1a4b9729f1e5fff80dec9) resolves to the full SHA (2026-10-19); [PSn00bSDK v0.24](https://github.com/Lameguy64/PSn00bSDK/releases/tag/v0.24) provides the pinned compiler archive. Keep both immutable source inputs and hashes. |
| ps1-tests references | Capture archive `build-158` has a SHA-256 pin; text logs and selected source files use commit `f727802fead11f1daa7549285548392ef87749cb` with per-file SHA-256 values. GPU, CD, SPU, and MDEC reference modules consume the selected files. | [build-158](https://github.com/JaCzekanski/ps1-tests/releases/tag/build-158) remains the latest public release checked (2021-04-22); [source commit](https://github.com/JaCzekanski/ps1-tests/commit/f727802fead11f1daa7549285548392ef87749cb) resolves to the recorded full SHA (2023-03-05). The age is visible, while the immutable refs, archive/member checks, and file hashes make the inputs reproducible. No newer upstream release was found to replace them. |
| SingleStepTests r3000 | `pins.txt` records per-vector blob SHA-1 and size against commit `51e227447d0b93882a8f13c87c6cee72819efdd3`; nightly fetches blobs through GitHub API and checks `git hash-object`. | [Pinned vector commit](https://github.com/SingleStepTests/r3000/commit/51e227447d0b93882a8f13c87c6cee72819efdd3) resolves to that full SHA (2026-09-04). Keep the commit and per-blob identities. |
| RetroArch host | Version `1.22.2` universal Metal DMG has an explicit SHA-256 pin in `pins.txt`; the `retroarch` CI job consumes it on `macos-15`. | [RetroArch v1.22.2](https://github.com/libretro/RetroArch/releases/tag/v1.22.2) is still the latest upstream release checked (2025-11-20). Keep the digest-pinned host artifact. |
| libchdr | No active pin or consumer exists. The Phase 9 CHD requirement proposes a future libchdr BSD-3 closure. | This is a roadmap proposal, not an active dependency. Do not add a pin before the Phase 9 implementation selects and verifies its closure. |

## Commands and evidence

| Command | Exit | Evidence |
| --- | ---: | --- |
| `sh scripts/apply-repo-settings.sh --check` | 0 | No settings drift; stdout was recorded above. |
| `gh api repos/szTheory/schwaystation/vulnerability-alerts --silent` | 0 | Security alerts endpoint enabled. |
| `gh api 'repos/szTheory/schwaystation/dependabot/alerts?state=open&per_page=100' --jq length` | 0 | 0 open alerts. |
| `gh api repos/szTheory/schwaystation/actions/runs/44307395327/jobs --paginate --jq '.jobs[] \| [.name, (.labels \| join(",")), .conclusion] \| @tsv'` | 0 | Runner labels and job results for the linked full-CI run. |
| `gh api repos/actions/runner-images/releases?per_page=100` | 0 | Current official image tags listed above. |
| GitHub REST `releases/latest`, release-tag and commit lookups for the pinned upstream repositories | 0 | Version/ref URLs are linked in the tables; full commit IDs and archive/blob hashes remain in `pins.txt` and CI. |
| `git diff --check HEAD` | 0 | This audit document has no whitespace errors. |

No full test suite or hosted workflow was run for this audit. The only plan-specific hosted evidence reused here is the successful full-CI run linked above.
