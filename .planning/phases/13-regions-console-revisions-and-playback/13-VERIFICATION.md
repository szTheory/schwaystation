---
phase: 13-regions-console-revisions-and-playback
verified: 2026-11-23T22:48:22Z
status: passed
score: "32/32 must-haves verified"
covered_files:
  - .planning/phases/13-regions-console-revisions-and-playback/13-01-PLAN.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-01-SUMMARY.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-02-PLAN.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-02-SUMMARY.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-03-PLAN.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-03-SUMMARY.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-04-PLAN.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-04-SUMMARY.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-05-PLAN.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-05-SUMMARY.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-06-PLAN.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-06-SUMMARY.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-07-PLAN.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-07-SUMMARY.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-08-PLAN.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-08-SUMMARY.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-09-PLAN.md
  - .planning/phases/13-regions-console-revisions-and-playback/13-09-SUMMARY.md
  - cmake/DiscFixtures.cmake
  - cmake/MDECReferences.cmake
  - docs/compatibility.md
  - docs/embedding.md
  - docs/libretro.md
  - docs/runner.md
  - docs/testing.md
  - host/disc_probe.c
  - host/disc_probe.h
  - include/schwaystation.h
  - libretro/CMakeLists.txt
  - libretro/libretro.c
  - runner/CMakeLists.txt
  - runner/main.c
  - runner/probe.c
  - runner/region.c
  - runner/region.h
  - scripts/compat-doc.sh
  - src/bus.c
  - src/cdrom.c
  - src/cpu.c
  - src/instance.c
  - src/internal.h
  - src/state.c
  - tests/abi/inventory.txt
  - tests/abi/tests.cmake
  - tests/api/v4.c
  - tests/api/v8.c
  - tests/cdrom/inventory.txt
  - tests/cdrom/model.c
  - tests/cdrom/tests.cmake
  - tests/cmake/compat-ratchet.cmake
  - tests/compat/features.csv
  - tests/compat/inventory.txt
  - tests/compat/tests.cmake
  - tests/core/inventory.txt
  - tests/core/test_core.c
  - tests/core/test_state.c
  - tests/core/tests.cmake
  - tests/fixtures/disc_boot_playlist.xml
  - tests/fixtures/manifest.txt
  - tests/fixtures/tests.cmake
  - tests/gpu/beam.c
  - tests/gpu/spans.c
  - tests/libretro/host.c
  - tests/libretro/inventory.txt
  - tests/libretro/tests.cmake
  - tests/mdec/color_reference.c
  - tests/nightly/inventory.txt
  - tests/runner/inventory.txt
  - tests/runner/test_region_probe.c
  - tests/runner/tests.cmake
covered_digest: "v4:sha256:9a7dabbe39ae1d6e193669dd54526075fb1f5644a07ca3ffc9eff696ff91758f"
behavior_unverified: 0
overrides_applied: 0
decision_coverage:
  honored: 7
  total: 7
  not_honored: []
human_verification: []
---

# Phase 13: Regions, Console Revisions and Playback — Verification

**Phase Goal:** Players can select documented hardware profiles, opt in to the eligible BIOS-shell shortcut, and play supported FMV cases within verified bounds.
**Verified:** 2026-11-23T22:48:22Z
**Status:** passed

## Goal Achievement

All four roadmap success criteria and all 28 plan-level truths were checked against source, tests, build wiring and the current local test outputs. Required artifacts from all nine plans passed the GSD artifact check (21/21). The key-link helper could not follow several C symbol paths because it searches for literal target names; those links were traced through the actual calls and exercised by the focused selectors listed below.

### Observable Truths

| # | Truth | Status | Evidence |
|---|---|---|---|
| 1 | PAL timing and audio cadence use the documented integer ratios; region remains separate from board and title identity. | VERIFIED | `src/internal.h`, `src/instance.c`, `runner/region.c`; `core.audio.pairs`, `runner.hash.pal`, `runner.region.auto`, `libretro.host.pal`, and `libretro.host.disc_audio` passed. |
| 2 | Every documented board/GPU tuple is selectable; invalid tuples are rejected atomically; behavior-affecting board values are validated, serialized and hashed. | VERIFIED | Nine-entry defaults table and independent profile fields in `src/instance.c`; state handling in `src/state.c`; `core.profile.machine`, `core.profile.rejected`, and `core.state.profile` passed. Compatibility table has 27 exercised board/region rows; its three preflight/control/ratchet selectors passed. |
| 3 | Full BIOS boot remains default; fast boot skips only the eligible initialized shell handoff and still requires BIOS and eligible disc content. | VERIFIED | `src/cpu.c`, `runner/main.c`, `libretro/libretro.c`; `core.boot.fast`, `core.profile.fast_boot`, `runner.boot.full`, `runner.boot.fast`, `libretro.content.boot`, and `libretro.content.profile` passed. Negative controls cover missing BIOS/disc, ineligible content and nearby PCs. |
| 4 | Supported MDEC references and the bounded movie diagnostic preserve their limits; input coverage is checked before guest execution. | VERIFIED | `cmake/MDECReferences.cmake` wires input and color-asset fixtures before the diagnostic; `tests/mdec/color_reference.c` accepts only the recorded movie15 terminal state. Nightly source preflight, color assets, 4-bit reference, 8-bit regression and edge diagnostic passed 5/5. The edge result reports complete input (1,440 blocks / 240 macroblocks), expected incomplete guest, no display comparison, no fault, and 1,902,230 ticks below the 4,915,200 cap. |

### Plan-Level Must-Haves

| Plan | Truths | Status | Evidence |
|---|---:|---|---|
| 13-01 | 4 | VERIFIED | Public append-only board field and nine-entry table; default, legal pairing, legacy-sized profile, invalid pairing and atomic rejection assertions in `core.profile.machine` and `core.profile.rejected`. |
| 13-02 | 3 | VERIFIED | Fixture-gated Nightly diagnostic; exact expected-incomplete signature; unchanged 4-bit/8-bit controls and cap. All five relevant Nightly tests passed. |
| 13-03 | 3 | VERIFIED | Board defaults flow to modeled GPU/RAM, CD firmware, SPU and port identity. `core.profile.machine` passed; `cdrom.profile.firmware` is part of the green full CI workflow. |
| 13-04 | 2 | VERIFIED | Board identity is included in validation, state bytes and hashes; rejection is validated before mutation. `core.state.profile` passed; the full CI workflow includes `core.state.rejects`. |
| 13-05 | 3 | VERIFIED | Runner resolver precedence and explicit overrides are exercised by `runner.region.probe` and `runner.region.auto`; PAL hash and audio cadence checks passed. |
| 13-06 | 3 | VERIFIED | Libretro applies independent board/region choices at content load; PAL video/audio and profile selectors passed. |
| 13-07 | 3 | VERIFIED | CPU hook checks the exact shell entry, return address, BIOS, disc and opt-in flag; default and adjacent-PC controls passed in `core.boot.fast`. |
| 13-08 | 4 | VERIFIED | Runner and libretro expose full/fast selection; boot eligibility and PS-EXE default paths are checked by the runner/libretro boot selectors in the focused run and full CI. |
| 13-09 | 3 | VERIFIED | 27 profile rows are guarded by the compatibility preflight and ratchet; title ledger is unchanged; roadmap records plan closeout pending aggregate verification. Compatibility selectors passed 3/3. |

### Required Artifacts

| Artifact | Status | Details |
|---|---|---|
| `include/schwaystation.h`, `src/instance.c` | VERIFIED | Public machine constants/field reach atomic validation and applied defaults. |
| `src/state.c`, `tests/core/test_state.c` | VERIFIED | Board and fast-boot fields have versioned serialization, validation and regression coverage. |
| `runner/region.c`, `runner/main.c` | VERIFIED | Content hints and explicit host options resolve into independent core profile values. |
| `libretro/libretro.c`, `tests/libretro/host.c` | VERIFIED | Region, board and fast-boot options are applied during load and checked in-process. |
| `cmake/MDECReferences.cmake`, `tests/mdec/color_reference.c` | VERIFIED | Existing preflight and assets gate the bounded edge case; other MDEC references remain checked. |
| `tests/compat/features.csv`, `scripts/compat-doc.sh` | VERIFIED | Exercised profile rows are validated and documented without claiming game compatibility. |

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| Public profile API | Board defaults and modeled devices | `schway_set_profile` validates and applies the table used by GPU, bus and CD-ROM paths. | WIRED | `core.profile.machine`, `core.profile.rejected`, and `cdrom.profile.firmware` pass. |
| Instance profile | Save-state codec | Board and flags are included in state validation, bytes and hashes. | WIRED | `core.state.profile` and `core.state.rejects` pass. |
| Host media identity | Runner/libretro region resolver | Serial, license area and executable markers resolve before the core receives a concrete region. | WIRED | `runner.region.auto`, `runner.region.probe`, and `libretro.content.profile` pass. |
| Runner/libretro fast option | CPU shell handoff | Host eligibility sets `SCHWAY_PROFILE_FAST_BOOT`; CPU honors it only at the initialized shell address. | WIRED | Core, runner and libretro boot selectors pass, including negative controls. |
| Nightly test registration | MDEC preflight and bounded runner | Fixture dependencies force input preflight and color assets before `mdec.reference.edge`. | WIRED | Selector closure was exactly the five expected tests; all passed. |
| Compatibility evidence | Ratchet and generated documentation | Feature ledger rows are checked and rendered from current source data. | WIRED | Compatibility checks passed 3/3 and `scripts/compat-doc.sh ... --check` exited 0. |

### Data-Flow Trace

| Input | Flow | Status |
|---|---|---|
| Runner region/board options and disc/EXE identity | Host resolver → concrete `schway_profile.region` and `.board` → core defaults/timing. | FLOWING |
| Libretro option and selected playlist member | Selected member probe → host resolver → core profile at content load. | FLOWING |
| Fast boot request | Host eligibility check → profile flag → exact CPU shell-entry guard. | FLOWING |
| Pinned MDEC source and reference assets | Input preflight → color-asset fixture → bounded guest diagnostic and explicit result classification. | FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| Region cadence, board/profile/state, runner/libretro profile and boot behavior, and regular MDEC guest controls | `ctest --preset ci --no-tests=error -R '^(core[.]profile[.](machine|rejected|fast_boot)|core[.]state[.]profile|core[.]boot[.]fast|core[.]audio[.]pairs|runner[.](hash[.]pal|region[.]auto|boot[.](full|fast))|libretro[.](host[.](pal|disc_audio)|content[.](boot|profile)|options[.]profile)|mdec[.](guest_4bit|guest_8bit_regression|input_coverage))$' --output-on-failure` | Exit 0; 21/21 including fixture prerequisites, 2.15 seconds total. | PASS |
| MDEC preflight, 4-bit reference, 8-bit baseline and bounded edge case | `ctest --preset nightly --no-tests=error -R '^mdec[.]reference[.](input_preflight|4bit|8bit_regression|edge)$' --output-on-failure` | Exit 0; 5/5 including color assets, 0.07 seconds total. | PASS |
| Compatibility preflight, controls and ratchet | `ctest --preset ci --no-tests=error -R '^compat[.]features[.](preflight|ratchet|controls)$' --output-on-failure` | Exit 0; 3/3, 0.51 seconds total. | PASS |

### Probe Execution

No phase-declared or conventional `probe-*.sh` scripts were found. Not applicable; the registered CTest checks are the phase's probes.

### Requirements Coverage

| Requirement | Source Plan | Status | Evidence |
|---|---|---|---|
| PROFILE-01 | 13-05, 13-06 | SATISFIED | PAL frame/audio integer ratios and region separation are tested; host resolution and portable hash checks pass. The full local workflow ran on macOS arm64; hosted platform parity remains a required ship gate. |
| PROFILE-02 | 13-01, 13-03, 13-04, 13-05, 13-06, 13-09 | SATISFIED | Nine tuples, invalid pairings, legacy caller behavior, state/hash handling, and 27 exercised compatibility rows are covered. |
| BOOT-01 | 13-07, 13-08 | SATISFIED | Full default and fast paths are separately covered; host paths require BIOS and eligible disc executable; no HLE path was added. |
| MDEC-01 | 13-02 | SATISFIED | Source-unit preflight precedes the bounded edge case; 4-bit accepted output and 8-bit deterministic baseline pass; the cycle cap and no-conformance boundary are preserved. |

All four Phase 13 requirements appear in plan frontmatter and map to this phase in `REQUIREMENTS.md`; no orphaned requirement was found.

### Decision Coverage

The GSD decision-coverage check found 7/7 trackable decisions honored by plans, summaries and implementation artifacts. No decision was left unaccounted for.

### Test Quality Audit

| Test area | Linked requirements | Disabled tests | Circular expected values | Assertion strength | Verdict |
|---|---|---:|---:|---|---|
| Core profile, cadence and state | PROFILE-01, PROFILE-02, BOOT-01 | 0 | 0 | Exact ratio, tuple, state-byte and rejection assertions | PASS |
| Runner and libretro profile/boot | PROFILE-01, PROFILE-02, BOOT-01 | 0 | 0 | Multi-step option, content and eligibility behavior | PASS |
| MDEC references and edge diagnostic | MDEC-01 | 0 | 0 | Pinned inputs/assets, exact diagnostic signature and bounded cycle assertions | PASS |
| Compatibility feature ledger | PROFILE-02 | 0 | 0 | Exact key, sorted inventory, row and ratchet checks | PASS |

Expected values are fixed by cited timing documentation or pinned fixture/reference data; the test setup does not regenerate expected output by asking the core under test. No disabled requirement-linked tests or debt markers were found.

### Anti-Patterns Found

None. The changed source/test scan found no unresolved `TBD`, `FIXME` or `XXX` markers. No disabled test pattern was found in the requirement-linked C/CMake tests.

### Human Verification Required

None. The phase has no UI or owner-hardware acceptance condition; runner, libretro and core behavior is exercised by automated tests. Retail-game compatibility and hardware conformance are not claimed by this phase.

## CI and Iteration Efficiency

The final local `cmake --workflow --preset ci` run passed at source head `ac4bb12`: exit 0, 691/691 CI tests, all three macOS arm64 package archives, and 8/8 release checks in 81 seconds. No source or workflow change followed that run; the later review-disposition commit is documentation only.

The slowest sampled CI test was `policy.quality_metrics` at 44.35 seconds; its serialized subprocess scenarios account for most of the test-lane critical path. A read-only audit found no safe small split: parallelizing it needs isolation for generated files and a harness refactor. `libretro.host.disc_audio` and movie-input tests are also slower cases, but preserve distinct coverage. No safe efficiency fix was supported by the measured evidence, so assertions and jobs remain intact. The GitHub workflow did not change, and Act would add no local coverage beyond the local workflow. Hosted runners, permissions, artifacts and release automation still require the ship run. No Docker resources were created.

## Verification Scope

The local full workflow and focused tests establish the current macOS arm64 behavior. Cross-platform hash agreement, hosted workflow execution, release-pipeline results and Actions cost must be confirmed at ship before the phase is released. This automated verification has no owner action.

---

_Verified: 2026-11-23T22:48:22Z_  
_Verifier: the agent (gsd-verifier)_
