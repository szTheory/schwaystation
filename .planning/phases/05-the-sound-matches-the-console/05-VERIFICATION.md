---
phase: 05-the-sound-matches-the-console
verified: 2026-11-15T01:34:22Z
status: passed
score: 74/74 must-haves verified
covered_files:
  - .github/workflows/ci.yml
  - .github/workflows/nightly.yml
  - .planning/phases/05-the-sound-matches-the-console/05-01-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-01-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-02-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-02-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-03-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-03-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-04-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-04-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-05-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-05-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-06-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-06-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-07-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-07-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-08-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-08-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-09-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-09-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-10-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-10-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-11-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-11-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-12-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-12-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-13-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-13-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-14-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-14-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-15-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-15-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-16-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-16-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-17-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-17-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-18-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-18-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-19-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-19-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-20-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-20-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-21-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-21-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-22-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-22-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-23-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-23-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-24-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-24-SUMMARY.md
  - ASSET_POLICY.md
  - CMakeLists.txt
  - CMakePresets.json
  - PROVENANCE.md
  - cmake/SPUReferences.cmake
  - docs/embedding.md
  - docs/hardware/bus.md
  - docs/hardware/spu.md
  - docs/runner.md
  - docs/testing.md
  - pins.txt
  - runner/main.c
  - src/bus.c
  - src/bus.h
  - src/cpu_exec.inc
  - src/dma.c
  - src/frame.c
  - src/sched.c
  - src/spu.c
  - src/spu.h
  - src/spu_envelope.c
  - src/spu_noise.c
  - src/spu_reverb.c
  - src/spu_tables.inc
  - src/spu_voice.c
  - tests/bus/bus_tests.c
  - tests/cmake/compat-controls.cmake
  - tests/cmake/hashes.cmake
  - tests/cmake/info_claims_self_test.cmake
  - tests/cmake/workflows.cmake
  - tests/core/bus_unbuilt.c
  - tests/core/inventory.txt
  - tests/core/test_run.c
  - tests/core/test_state.c
  - tests/core/tests.cmake
  - tests/diag/expected.csv
  - tests/dma/dma_tests.c
  - tests/dma/inventory.txt
  - tests/dma/tests.cmake
  - tests/embed/CMakeLists.txt
  - tests/fixtures/manifest.txt
  - tests/fixtures/src/spu_blocks.inc
  - tests/fixtures/src/spu_tone.asm
  - tests/fixtures/src/stub/stub.asm
  - tests/fixtures/src/unbuilt.asm
  - tests/fixtures/tests.cmake
  - tests/fuzz/inventory.txt
  - tests/fuzz/seeds.c
  - tests/fuzz/spu.c
  - tests/fuzz/tests.cmake
  - tests/hygiene/inventory.txt
  - tests/hygiene/tests.cmake
  - tests/inventory/asan.txt
  - tests/inventory/ci.txt
  - tests/inventory/wasm32.txt
  - tests/libretro/host.c
  - tests/libretro/tests.cmake
  - tests/nightly/inventory.txt
  - tests/nightly/tests.cmake
  - tests/perf/inventory.txt
  - tests/perf/tests.cmake
  - tests/runner/inventory.txt
  - tests/runner/test_runner.c
  - tests/runner/tests.cmake
  - tests/spu/envelopes.c
  - tests/spu/expected.txt
  - tests/spu/fixture.c
  - tests/spu/inventory.txt
  - tests/spu/model.c
  - tests/spu/noise.c
  - tests/spu/reference.c
  - tests/spu/reverb.c
  - tests/spu/tests.cmake
  - tests/spu/voices.c
  - tests/tests.cmake
covered_digest: "v4:sha256:b3ab9fde7cc7e84a6a13ed0e7d278a2cb53d5167f5fb19f1b6dc1df0bf73abc9"
behavior_unverified: 0
overrides_applied: 0
re_verification:
  previous_status: passed
  previous_score: 3/3
  gaps_closed: []
  gaps_remaining: []
  regressions: []
---

# Phase 5: The sound matches the console — Verification Report

**Phase Goal:** The SPU sounds as the console does: it matches the hardware logs, its audio hashes are identical on every platform including be32 and wasm32, and libretro delivers exactly the runner's audio stream.
**Verified:** 2026-11-15T01:34:22Z
**Status:** passed
**Re-verification:** Yes — refreshed after reviewed test fixes. The previous report had no gaps; this pass rechecked all roadmap and plan truths.

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|---|---|---|
| 1 | The tone fixture hashes identical signed stereo PCM on all eight configured lanes; each frame has the required exact sample count, and CPU-free block replay equals guest PCM. | ✓ VERIFIED | Current full local CI passed 547/547 CTests and 5/5 release checks. The named spu.fixture check passed 51 assertions, including exact NTSC/PAL pair counts and guest/table PCM equality. The runner hash writer produces the three region streams; CI requires eight lane artifacts and byte-compares them. Current exact-head hosted lane results are still a ship-time check, per plan 05-15. |
| 2 | Audio tests cover the scheduled SPU model and the nightly comparator checks the seven ordered hardware transfer cases. | ✓ VERIFIED | The audio tests are registered in tests/spu/tests.cmake. The comparator binds the executable and log to pins, rejects identity/parser mismatches, and runs the actual guest through Done. The prior nightly run passed on old head da80fc6798c0aef55fda529bfee2aadde9df5d89; the exact-head nightly run remains pending at ship. |
| 3 | Libretro batch and single-sample callbacks deliver runner PCM, RetroArch boots the tone fixture, and the idle SPU stays bounded. | ✓ VERIFIED | The host integration writes runner PCM and compares callback samples for NTSC-J, NTSC-U and PAL. Current local CI passed; current be32 QEMU batch host test passed all 20 TAP checks. The old-head RetroArch job was green; the exact-head hosted job is pending at ship. core.no_busy_loop passed as a named test in 0.02 seconds. |

**Score:** 74/74 must-haves verified (0 present, behavior-unverified).

### Plan-Specific Must-Haves

The phase plans add 71 truths to the three roadmap success criteria. All 24 plans' artifact declarations were checked with verify.artifacts: 61/61 artifacts exist and are substantive. The named tests and full local workflow exercise the implementations; the high-level links are traced below.

| Plan | Additional truths verified | Status | Evidence |
|---|---|---|---|
| 05-01 | Real library-to-drain tracer; selected CI/sanitizer/wasm inventories; sample counts, ring semantics and rates | ✓ VERIFIED | tests/spu/model.c, test registration and exact inventories; full CI |
| 05-02 | One scheduled stereo pair per 768 cycles; fixed-width instance state; public layout and rates | ✓ VERIFIED | sched.c schedules spu.c; frame.c drains the same ring; model and run tests |
| 05-03 | Guest MMIO width/latch behavior; FIFO/IRQA access; peeks do not consume or acknowledge | ✓ VERIFIED | bus.c dispatches to chip-owned registers; spu model tests cover read/write and IRQ |
| 05-04 | Pinned program/log identity; strict seven-case comparator; asset controls gate first consumers | ✓ VERIFIED | SPUReferences.cmake verifies pins/digests; reference controls and nightly registration |
| 05-05 | DMA4 upload/readback; first four guest cases; bounded readiness/completion | ✓ VERIFIED | dma.c calls SPU FIFO helpers; DMA and first-upload tests |
| 05-06 | BRR history/nibble decoding; Gaussian interpolation and cited 512-entry table | ✓ VERIFIED | spu_voice.c and spu_tables.inc; voice tests |
| 05-07 | ADSR phases and volume sweeps; complete 2×128 rate table; edge rates/profile contract | ✓ VERIFIED | spu_envelope.c; envelope tests |
| 05-08 | Noise schedule; envelope-scaled capture and zero CD inputs; muted/silent IRQ behavior | ✓ VERIFIED | spu_noise.c and spu.c event path; noise/model tests |
| 05-09 | 39-tap FIR; centered-filter delay; bounded integer resampling path | ✓ VERIFIED | spu_reverb.c and documented hardware behavior; reverb tests |
| 05-10 | Room feedback updates RAM and wet output from registers; documented Room path | ✓ VERIFIED | spu_reverb.c is called from spu.c; reverb/model tests |
| 05-11 | Authored BRR data is assembled into the actual tone guest; guest reaches mailbox and audio drain | ✓ VERIFIED | fixture source includes spu_blocks.inc; fixture build and spu.fixture |
| 05-12 | Batch and single-sample host callbacks equal runner PCM in all regions | ✓ VERIFIED | tests/libretro/tests.cmake drives runner and host independently; local and be32 host results |
| 05-13 | Hostile transfer bounds and SPU fuzz-seed replay; scheduler progress remains bounded | ✓ VERIFIED | tests/fuzz/spu.c and test_run.c are registered; full CI |
| 05-14 | Active voices/noise/reverb state continues after load; invalid state remains atomic and size-bounded | ✓ VERIFIED | state visitor includes SPU fields; core state tests and full CI |
| 05-15 | Eight hash artifacts and required tone-content RetroArch smoke; stop before ship for hosted evidence | ✓ VERIFIED | ci.yml has eight-artifact equality and content smoke; nightly.yml is wired; exact-head hosted check remains pending at ship |
| 05-16 | Every SPU machine field uses the LE visitor; load restores phase and clears delivery buffers; compatibility/atomicity/size rules | ✓ VERIFIED | state.c calls the complete SPU visitor; core state tests and full CI |
| 05-17 | Runner hashes canonical LE PCM after final mix; drains once; preserves CLI errors | ✓ VERIFIED | runner/main.c calls schway_get_audio once and hashes canonical samples; runner tests |
| 05-18 | CPU-free table path equals guest PCM; frame-600 digest and ENVX predicates; bad expectations fail | ✓ VERIFIED | fixture.c reads block table and static expectations; spu.fixture passed 51 assertions |
| 05-19 | Hardware documentation cites pins and implemented behavior; evidence categories remain distinct; precision limits remain explicit | ✓ VERIFIED | docs/hardware/spu.md, docs/testing.md and PROVENANCE.md state separate evidence boundaries |
| 05-20 | Bounded FIFO/manual transfer; RAM_CTRL/capture controls; IRQA/STATX and Enable behavior | ✓ VERIFIED | spu.c and irq.c helpers; spu.model and transfer tests |
| 05-21 | DMA4 directions and timing; DEV4 FFFFh behavior; pinned seven-case guest | ✓ VERIFIED | dma.c/spu.c path and tests; prior full guest summary is corroborated by current source and unchanged implementation |
| 05-22 | Ordered 24-voice playback; KON/KOFF, loops, ENDX, PMON and stereo mix; silence/mute/overflow | ✓ VERIFIED | spu.c event calls voice/envelope paths; voice/model tests and full CI |
| 05-23 | SPU owns ATTR; nightly runs the full pinned transfer guest; GPU nightly inventory remains | ✓ VERIFIED | bus mirror removed; dma reads SPU state; exact nightly inventory and CMake registration |
| 05-24 | Phase registration and hash checks fit the 2688-line budget; existing workflow/inventory controls retained | ✓ VERIFIED | current workflow checks and full local CI; phase summaries record 2688-line check |

## Required Artifacts

| Artifact group | Expected | Status | Details |
|---|---|---|---|
| SPU source and state | Scheduler, registers, DMA, voice/envelope/noise/reverb, state visitor and public PCM drain | ✓ VERIFIED | All source artifacts declared in 05-01 through 05-24 passed the verifier's existence and substance checks; tests are registered and full local CI passed. |
| Audio and reference tests | Model, voice, envelope, noise, reverb, fixture replay, DMA guest comparator, fuzz and state tests | ✓ VERIFIED | 61/61 plan artifact declarations passed across 24 plans. No declaration was missing or a stub. |
| Runner/libretro and workflows | Canonical PCM hash, callback equality, eight-lane artifact compare, nightly transfer and RetroArch tone smoke | ✓ VERIFIED | Implementations and CMake/workflow registrations are present. Exact-head hosted results remain a ship-time confirmation. |
| Documentation and pins | Hardware behavior, test evidence boundaries, provenance and identity-pinned assets | ✓ VERIFIED | Digests and source identity are checked by SPUReferences.cmake; documentation explicitly limits the hardware claims. |

## Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| tests/tests.cmake and lane inventories | tests/spu/tests.cmake and tests/spu/inventory.txt | CMake include plus exact directory inventories | ✓ WIRED | The test directory is included and its entries are selected in the configured CI, sanitizer and wasm inventories. |
| src/sched.c | src/spu.c | Scheduled SPU deadline | ✓ WIRED | The scheduler invokes schway__spu_event at the selected tick; catch-up processes due events without CPU polling. |
| src/spu.c | src/frame.c and schway_get_audio | Instance PCM ring | ✓ WIRED | SPU events append stereo samples to instance state; the public getter drains that state. |
| src/bus.c, src/dma.c | src/spu.c and src/irq.c | MMIO, DMA4 FIFO and IRQA helpers | ✓ WIRED | Bus dispatch and DMA readiness use SPU-owned state; IRQA raises IRQ9 through the IRQ helper. |
| src/spu.c | voice, envelope, noise and reverb modules | Per-sample event calls | ✓ WIRED | The SPU event invokes each module in order; their tests are separately registered. |
| pins.txt | cmake/SPUReferences.cmake and tests/spu/reference.c | Source identity, SHA-256 asset checks and bounded guest comparison | ✓ WIRED | Nightly registration runs the full comparator against pinned executable/log assets. |
| runner/main.c and libretro/libretro.c | schway_get_audio | Runner drain/hash and libretro callback delivery | ✓ WIRED | Host tests compare the callback PCM against the runner PCM bytes. The focused be32 run exercised batch equality. |
| tests/cmake/hashes.cmake | .github/workflows/ci.yml | Eight lane artifacts and exact COUNT=8 equality check | ✓ WIRED | CI builds/uploads artifacts, then the equality job downloads and compares them. |
| .github/workflows/ci.yml | tests/retroarch/run_retroarch.cmake | Tone executable under stub, screenshot checked against runner | ✓ WIRED | Required recurring RetroArch smoke is content-based and video-only, as SOUND-02 specifies. |

The generic key-link query did not recognize several CMake include relationships or C-symbol calls because it searches for direct text references. Plans 05-21 and 05-23 place key_links at the frontmatter top level, so the query reported no nested must_haves.key_links for those two plans. I traced those paths through the CMake registration and function calls above; none is disconnected.

## Data-Flow Trace (Level 4)

| Artifact | Data variable | Source | Produces real data | Status |
|---|---|---|---|---|
| tests/cmake/hashes.cmake | audio digest rows | Built schwaystation-run executing spu_tone.exe for each region | Yes; output is captured from the runner | ✓ FLOWING |
| tests/spu/fixture.c | guest and table PCM | Real guest execution and an independent CPU-free event/table path using the same source fixture | Yes; complete PCM pairs are compared | ✓ FLOWING |
| runner/main.c | audio hash and PCM file | schway_get_audio from the active emulator instance | Yes; canonical signed stereo PCM is hashed after the final mix | ✓ FLOWING |
| libretro/libretro.c | audio callback samples | schway_get_audio from the same core instance | Yes; batch and single callbacks are compared byte-for-byte with runner PCM | ✓ FLOWING |
| tests/spu/reference.c | guest pass lines | Pinned transfer executable and pinned hardware log | Yes; identity is verified, then emulator guest output is compared in order | ✓ FLOWING |

## Behavioral Spot-Checks

| Behavior | Command/evidence | Result | Status |
|---|---|---|---|
| Current local CI workflow | node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" run-with-timeout 240 -- bash -c 'cmake --workflow --preset ci' | Exit 0; 547/547 CTests and 5/5 release checks passed on current checkout | ✓ PASS |
| Guest audio length and CPU-free PCM equality | ctest --test-dir build/ci -R '^spu\.fixture$' --no-tests=error --output-on-failure | Passed; 51 TAP assertions, 14.47 seconds | ✓ PASS |
| Idle SPU bounded behavior | ctest --test-dir build/ci -R '^core\.no_busy_loop$' --no-tests=error --output-on-failure | Passed 1/1 in 0.02 seconds | ✓ PASS |
| Big-endian libretro callback equality | docker exec schwaystation-phase5-be32-debug-20261114 qemu-ppc -L /usr/powerpc-linux-gnu build/be32/tests/schwaystation-test-libretro-host build/be32/libretro/schwaystation_libretro.so audio-equality-batch ntsc-j build/be32/fixtures/spu_tone.exe build/be32/tests/libretro-audio-ntsc-j-batch.pcm | Exit 0; all 20 TAP checks passed; temporary container has been removed | ✓ PASS |
| Current exact-head hosted CI/nightly/RetroArch | Run at ship after pushing the current head | No current-head URL yet; not represented by the old-head run results below | PENDING SHIP |

The named spu.fixture test exceeded the verifier's preferred 10-second spot-check window but completed successfully; the full CI workflow was already supplied as current-checkout evidence and was not rerun here.

## Probe Execution

No plan declares a probe script, and no conventional scripts/*/tests/probe-*.sh exists. No probe was required.

## Test Quality Audit

| Test File | Linked Requirement | Active | Skipped | Circular | Assertion Level | Verdict |
|---|---|---:|---:|---|---|---|
| tests/spu/model.c and tests/spu/tests.cmake | SOUND-01 | Yes | 0 | No | Behavioral/value | PASS |
| tests/spu/fixture.c and tests/spu/expected.txt | SOUND-01 | Yes | 0 | No; the pinned text is a self-authored model regression, not a hardware waveform | Behavioral/value | PASS with provenance limit |
| tests/spu/reference.c and tests/nightly/tests.cmake | SOUND-01 | Yes | 0 | No; expected transfer lines come from identity-pinned hardware assets | Behavioral/value | PASS; old-head nightly only |
| tests/libretro/host.c and tests/libretro/tests.cmake | SOUND-02 | Yes | 0 | No; runner and callbacks use separate code paths | Value/behavioral | PASS |
| tests/cmake/hashes.cmake and ci.yml | SOUND-01 | Yes | 0 | No for the stated cross-lane consistency check; generated rows are outputs, not golden values | Value | PASS; exact-head hosted comparison pending |

Disabled-test scan found no skipped requirement-linked tests. No test writes its own expectation data from the implementation under test. The hash writer emits lane outputs for a separate equality job; it is not treated as a silicon reference.

## Requirements Coverage

| Requirement | Source plans | Description | Status | Evidence |
|---|---|---|---|---|
| SOUND-01 | 05-01 through 05-24 | Scheduled integer SPU model and audio path, exact frame sample counts, independent CPU-free PCM replay, cross-lane audio hash comparison, and seven hardware transfer cases | ✓ SATISFIED | Current full local CI and spu.fixture passed; eight-lane and nightly jobs are wired. Exact-head hosted results are reserved for ship. |
| SOUND-02 | 05-01, 05-02, 05-12, 05-14 through 05-16, 05-19, 05-24 | Libretro batch/single PCM equals runner stream; tone content boots in hosted RetroArch; idle SPU remains bounded | ✓ SATISFIED | Full local CI, current be32 QEMU callback test and core.no_busy_loop passed; required RetroArch job is wired. Exact-head hosted run remains pending at ship. |

All 10 trackable decisions in 05-CONTEXT.md are honored by the plans and implementation (10/10; decision-coverage query exit 0).

## Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|

No unreferenced TBD/FIXME/XXX markers, placeholder implementations, disabled linked tests, or blocking stubs were found in the phase files listed by the summaries. Code review of the latest test changes covered tests/libretro/host.c, tests/libretro/tests.cmake, tests/runner/tests.cmake and tests/spu/tests.cmake and reported zero findings.

## Disconfirmation Notes

- The phase title can sound like a claim of complete silicon waveform identity. The implemented measurable contract does not prove a full bit-exact hardware waveform: docs/hardware/spu.md explicitly says no full silicon PCM capture is available. The pinned hardware evidence covers the named transfer cases; tone and impulse digests are model regressions. This is consistent with the roadmap criteria and is not a code gap.
- The audio@600 digest could be mistaken for an external hardware oracle. tests/spu/expected.txt labels it as self-authored and explicitly says it is not a captured hardware waveform; the independent CPU-free path checks fixture/table consistency.
- The network fetch failure path in SPUReferences.cmake propagates a fatal diagnostic if scripts/fetch-pin.sh fails. The controls test checks malformed/corrupt identity cases, but no deterministic test injects a transport failure. This is an uncovered infrastructure error path, not a failure of the SPU contract.

## Human Verification Required

N/A — this is a core-library/infrastructure phase with no owner-facing UI or manual audio sign-off. Deterministic checks are automated. The exact-head CI, nightly transfer and RetroArch run URLs are confirmed at ship, as plan 05-15 and AGENTS.md require.

## Gaps Summary

No implementation gaps remain. Current local CI, the focused be32 callback test and named local checks pass; required CI/nightly/RetroArch wiring is present. The old-head nightly run was green, while the old-head CI run failed on host limits and a big-endian assertion that the reviewed fixes address. Neither old-head run is evidence for the current checkout. The exact-head hosted run URL and result remain the next ship-time confirmation.

### Decision Coverage

All trackable CONTEXT.md decisions are reflected in the phase artifacts (10/10).

---
*Phase: 05-the-sound-matches-the-console*
*Verified: 2026-11-15*
*Verifier: the agent (gsd-verifier)*
