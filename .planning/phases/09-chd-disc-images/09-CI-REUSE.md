# Phase 9 CI and Release reuse measurements

## Exact-SHA runs

Both workflows ran on the same push, branch, source commit, and first attempt. Every job record carried `head_sha=830bf4cfe74aca92930e34fdf9667c5c1b24c370`.

| Workflow | Run | Source SHA | Run window (UTC) | Result |
| --- | --- | --- | --- | --- |
| Main CI | [44645037841](https://github.com/szTheory/schwaystation/actions/runs/44645037841) | `830bf4cfe74aca92930e34fdf9667c5c1b24c370` | 00:20:12–01:42:34 (82m22s) | success |
| Release | [44645037923](https://github.com/szTheory/schwaystation/actions/runs/44645037923) | `830bf4cfe74aca92930e34fdf9667c5c1b24c370` | 00:20:12–01:24:18 (64m06s) | success |

The Release run was `chore(main): release 0.8.0`; `release-please` selected the same SHA. The published [v0.8.0 release](https://github.com/szTheory/schwaystation/releases/tag/v0.8.0) targets that commit.

## Selected coverage and required checks

The six package-build lanes are Linux x64/arm64, macOS arm64/x64, and Windows x64/arm64. They use the `ci` preset, or `ci-msvc` for Windows, and check the `tests/inventory/ci.txt` suite plus the release-labeled suite before uploading `archives-<os>-<arch>`. The BE32 lane checks `tests/inventory/ci.txt` with the portable label; WASM checks `tests/inventory/wasm32.txt` with the portable label; the GPU lane selects the GPU cases from `tests/inventory/nightly.txt`. The nested Release call keeps these same lanes and selectors, while its allowed skipped jobs are `title`, `perf`, `rehearsal`, and `cleanup`.

The active `main` ruleset requires the `CI required` check. The exact-SHA Main run passed `CI required`; the Release workflow's nested call also passed `release-ci / CI required`. The Main workflow's roll-up depends on every listed lane, and Release `publish` depends on both `release-please` and the full `release-ci` call. The legacy branch-protection status endpoint returned 404 because `main` is governed by a repository ruleset; reading that ruleset confirmed the required check.

## Per-job elapsed runner seconds

Skipped jobs have no runner time; GitHub returned negative timestamp intervals for some skipped records, so they are shown as `skipped` and contribute zero to totals.

| CI job | Main run | Release run (`release-ci /`) |
| --- | ---: | ---: |
| `changes` | 5 | 4 |
| `hygiene` | 70 | 60 |
| `title` | skipped | skipped |
| `gpu-reference` | 25 | 25 |
| `openbios` | 220 | 30 |
| `build (linux-x64)` | 210 | 310 |
| `build (linux-arm64)` | 245 | 245 |
| `build (macos-arm64)` | 258 | 287 |
| `build (macos-x64)` | 539 | 520 |
| `build (windows-x64)` | 409 | 333 |
| `build (windows-arm64)` | 378 | 366 |
| `asan` | 873 | 1,197 |
| `nofp (ubuntu-26.04)` | 21 | 11 |
| `nofp (ubuntu-26.04-arm)` | 11 | 12 |
| `be32` | 4,694 | 3,402 |
| `wasm32` | 269 | 150 |
| `hash-equality` | 6 | 6 |
| `retroarch` | 110 | 92 |
| `perf` | skipped | skipped |
| `rehearsal` | skipped | skipped |
| `cleanup` | skipped | skipped |
| `CI required` | 5 | 2 |

The Release-only jobs took 12 seconds (`release-please`), 361 seconds (`publish`), and 9 seconds (`install-line-latest`).

| Measurement | Main CI | Release CI | Release-only | Combined |
| --- | ---: | ---: | ---: | ---: |
| Jobs recorded | 22 | 22 | 3 | 25 |
| Runner seconds | 8,348 | 7,052 | 382 | 15,782 |
| Runner minutes | 139.13 | 117.53 | 6.37 | 263.03 |

The Main path is `changes → openbios → be32 → hash-equality → CI required`; BE32 is the slow leg. The Release path is `release-please → release-ci changes/openbios → be32 → hash-equality → CI required → publish → install-line-latest`. Job timestamps show the Main workflow completed in 82m22s and the Release workflow in 64m06s while both ran concurrently. This pair does not show a wall-clock saving from concurrent runs.

## Actions artifact identity

The Actions artifact API reports these six package bundles per run. They were unexpired when queried; all recorded `commit.txt` files matched the source SHA above. The complete run-local downloads contained six bundles and 18 product ZIPs in each workflow.

| Artifact | Main size | Main artifact SHA-256 | Release size | Release artifact SHA-256 |
| --- | ---: | --- | ---: | --- |
| `archives-linux-arm64` | 748,148 | `fae18d61555d2619deae8089968d6c31452b5df8ee95563e55b9562196102bb1` | 748,169 | `8e4b13a957325c05203af375441ba0054e8c1281ba519b6ffc52ee52e85b6da4` |
| `archives-linux-x64` | 731,023 | `a16f5d79968371e4410184c89eb3028fe952436ec78db6786c0c4b0714d687b1` | 731,039 | `e0a10ea7920f2f04ab66498deb09496237568953b604549b5039e069b1af4f49` |
| `archives-macos-arm64` | 520,522 | `09a7bfe551ae1d3451f8fb1b120e8f4292fc140dfd5e545a2bf4b8dd671511eb` | 520,483 | `440ce1335d8427e305a64f577b24399355661c9c6057049e20e77c26fba59ecb` |
| `archives-macos-x64` | 522,377 | `22497a07e655439949a87ab0a52df937356482962a7c651cfb1c092978341477` | 522,348 | `3ffa50e6f46ce828d83bd982d379aff155e58bf18091c99dab288e4af7808af7` |
| `archives-windows-arm64` | 740,347 | `693b1a12cdb3a8b2c345bef21a3a0d770d5a29322c00df94b9f7ecc3252d1a0f` | 740,354 | `1a8b2a31ee08ddf1b7c4e3b8cb715a4b1124c4b203aa63f3c442215c718693de` |
| `archives-windows-x64` | 795,883 | `cdea1287a8da55afe63e83e8825847bc534011de2c8351e99be55195a4821d47` | 795,949 | `89ad51b05a95a319733cc2d9e6c72848c482d458e08831dc664d64367a940298` |

Every corresponding product ZIP also differed between Main and Release. The Release SHA-256 column matches the published v0.8.0 GitHub asset digest for every name. `SHA256SUMS` is the nineteenth release asset and is not one of the eighteen package ZIPs.

| Product ZIP | Main SHA-256 | Release / published SHA-256 |
| --- | --- | --- |
| `schwaystation-library-linux-arm64.zip` | `f8e22ef169ac63e21dec6e5f882fd3af13be65e4a67d0d67b1a310d729ec765d` | `a12b2402ebd72523fddb3bbbc163a71455f125a0b8ea37c3f7e50fccefc6bcbc` |
| `schwaystation-library-linux-x64.zip` | `e203fb6d7e24193db0478ecf2d8cee6c2faa673821a24231014c607d1a04e689` | `cb035c90300740dd03760d2f5f95ddd53a1cb049d783a76fecbcccb12482d1f8` |
| `schwaystation-library-macos-arm64.zip` | `aef5e90f46fe1fe94d72596fb8e30d5b75875fce36952a13d250662dbe77049b` | `dc60d7d09f30bd6e1cfdddab22d446641b7c5e45214c79aa100a709e91db3c7c` |
| `schwaystation-library-macos-x64.zip` | `10fd39404c0a96417ad42f07afce6950e9696615e3b1758f51937f697204c177` | `fb72ea3f5fd5f50a0fb0ed7d08859de7141e914fd18e799e80ebf66d2aafc581` |
| `schwaystation-library-windows-arm64.zip` | `13bc5a0350ed41d085b1c773340baf4357af34164785c4782b3d5e6d3977d4aa` | `aba0808061b5a43c9745fee0011e9fa0b36ac7c0a9fe8013b776b2e1f6af876b` |
| `schwaystation-library-windows-x64.zip` | `5b868769ca0f9e0c23f07538e5cd04c65387cd1088616a07bae0a46ad5696288` | `d274dcbbaf19949deaa0d53553988725e4e58af43ad1af5db5928a81b97f7ee6` |
| `schwaystation-libretro-linux-arm64.zip` | `04087ea3295aaca93e7f07a61adab8144dc127df6b79d72cb71e131056ecb398` | `1868f5d0039793e61b7f8e2a1ebeac24348668bdeb8eea9a202de5edf141e5d9` |
| `schwaystation-libretro-linux-x64.zip` | `9cc41b99232211222bb072babdf58255edffdd484dc282f7d71bda78e0872e98` | `29fe19f022091e3d7f987c8c3d17dbd09528f896ebd7fe1e1d6ec82c74721973` |
| `schwaystation-libretro-macos-arm64.zip` | `f8c0e42b5f4e8ab5f32843dc87ab6d1d801ac0c47b6c6e793da8001641039766` | `f77040e0b1ba676c974e4420911aff65008bc5e1d3993f390a7744863f3881c4` |
| `schwaystation-libretro-macos-x64.zip` | `de137c82f0082ec829821c6d171a726e4d3d0c1479608f07d2f5a13bcfd17ad8` | `5f2737ab31a558c7363048358bd3f901ef0866ad7d5fd4f6c2ffe1c8d0d216f9` |
| `schwaystation-libretro-windows-arm64.zip` | `48902710c78971a70dbcb8907e00d940912880eb1d286deef27533bba5f922cb` | `0407a574fc80fb0c3d75eb03a8599d2a081192061ad02f07d735ef66f2624ecc` |
| `schwaystation-libretro-windows-x64.zip` | `e0ad3cd163e01225c2e530d9908a1a81824c7e09cc2d590e7a79df9976809d8f` | `e8b2f8ba29100ee71c95766b5f7221303e997b36524d873d79f431c45818d355` |
| `schwaystation-runner-linux-arm64.zip` | `4e87384482bb781a49884eafc080d688889af9662d63d7f5c9e8b0e7a5514980` | `d77b3d34bfdd4c58d91c25fa98fd525abc256b8927adc7e86fc6f53603ce6d7f` |
| `schwaystation-runner-linux-x64.zip` | `667c911d7ac128fa85fc94523a881679b30cd2002453ac1a9eb1dd802e0cb4df` | `8985be1e97155325a79b46ceff46a0bdc0075fdfedb18c3078b60038e5f8d10b` |
| `schwaystation-runner-macos-arm64.zip` | `d77ca97aacb4da8f539691fde1f5a966837a51c549231562ddc099e4bb7285a6` | `2eca1b76a045124fe434c0c3ab8fa56c01eccdf544e53e6f3ef5ea7dfa2c046a` |
| `schwaystation-runner-macos-x64.zip` | `ed780649caed2dcdb090696b99fca5eb903816cfdf9c39c52a70aa0298e5f1e4` | `39fc4ae011d92c81bfcd495ad50167ff2ace7191a14aabe3c560555908f7ea9a` |
| `schwaystation-runner-windows-arm64.zip` | `166cce6a1160dc88aad059aa2dcf981fcf9a8dee46228c3d1481330ab734c74c` | `636dfcafe2ba182cfbafa445f49e1b8f8b1bce57990087700702eeabe2b8c4f6` |
| `schwaystation-runner-windows-x64.zip` | `c59b6d0b73027449d3c4c61989d6f0bfb35f3df530a5a048d01027c80996aeec` | `e513499ba48123a351efef0c146118c81b9a5fd64797d1fbcf5fed9e9c548129` |

## Disposition

Keep the independent Main and Release CI gates and their six-platform package matrices. Reusing Main's completed required result before Release publication would serialize the release behind Main's longer critical path: the measured Main `CI required` finished at 01:42:33, after the current Release run had completed at 01:24:18. With the measured 361-second publish job and 9-second install-line job after that wait, the projected Release completion is about 88m31s from its start, roughly 24m25s slower than this observed Release run. The 117.53 repeated Release-CI runner-minutes are real cost, but the concurrent pair establishes no wall-clock saving.

Same source SHA and per-lane `commit.txt` are not enough to establish archive identity: all eighteen Main product ZIPs have different SHA-256 digests from the corresponding Release ZIPs. The Release ZIPs, in turn, match the published release assets byte-for-byte. A cross-run producer/consumer would need to bind the release-please SHA to the successful Main push run and its exact attempt, require every expected job and matrix lane, identify all six artifacts by name and digest, validate the eighteen archive names and per-lane commit files, and keep `publish.sh` checksum and attestation validation. Its negative controls would need to cover wrong SHA, wrong attempt, missing or expired artifacts, duplicate or missing lanes, and the race where Release starts before Main finishes. No such path is present or tested, and the added cross-run wait would lengthen the measured Release critical path. Retain the current separate checks; do not add cross-run artifact reuse, a matrix skip, or an untested fallback.

Plan 11 should preserve the Main required `CI required` check, the Release `release-ci / CI required` gate, all six archive producers, and the exact-release-SHA packaging/attestation checks. It can close todo 050 after verifying those invariants remain intact on its changed workflow source.

## Commands and evidence

Read-only GitHub API queries verified both run records, all job pages, both artifact lists, the active `main` ruleset, check runs for the source commit, and the published v0.8.0 assets. All API requests returned exit 0 except the legacy branch-protection endpoint, which returned the expected 404 because the active rule is a repository ruleset.

```sh
gh api repos/szTheory/schwaystation/actions/runs/44645037841
gh api repos/szTheory/schwaystation/actions/runs/44645037923
gh api --paginate --slurp 'repos/szTheory/schwaystation/actions/runs/44645037841/jobs?per_page=100'
gh api --paginate --slurp 'repos/szTheory/schwaystation/actions/runs/44645037923/jobs?per_page=100'
gh api --paginate --slurp 'repos/szTheory/schwaystation/actions/runs/44645037841/artifacts?per_page=100'
gh api --paginate --slurp 'repos/szTheory/schwaystation/actions/runs/44645037923/artifacts?per_page=100'
gh api repos/szTheory/schwaystation/rulesets
gh api repos/szTheory/schwaystation/rulesets/25491748
gh api repos/szTheory/schwaystation/commits/830bf4cfe74aca92930e34fdf9667c5c1b24c370/check-runs
gh api 'repos/szTheory/schwaystation/releases?per_page=100'
gh run download 44645037841 --repo szTheory/schwaystation --pattern 'archives-*' --dir "$TMP/main"
gh run download 44645037923 --repo szTheory/schwaystation --pattern 'archives-*' --dir "$TMP/release"
```

The one-off `jq` aggregation and archive comparator exited 0: Main job seconds summed to 8,348; Release's 22 nested CI jobs to 7,052; its three release-only jobs to 382; all 18 Main/Release product ZIP digest pairs were distinct; and all 18 Release ZIP digests matched their published asset digest. No hosted run, additional build, or permanent reporting helper was added.
