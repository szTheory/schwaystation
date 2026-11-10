---
phase: 04-games-play-in-retroarch
researched: 2026-11-10
confidence: medium
runtime: Codex, OpenGSD 1.20.0
model: gpt-6-luna at xhigh
---

# Phase 4 implementation research

The independent researcher supplied the findings below. Its long-running turn was interrupted after the useful
findings had been delivered; it returned its existing notes without further tools. The orchestrator transcribed
this handoff and rechecked both archive digests and ZIP members. Discussion decisions remain in 04-CONTEXT.md.
This is implementation guidance, not another phase gate. No source was changed by research.

## Verified inputs

| Input | Official asset and pin | SHA-256 | Confidence |
| --- | --- | --- | --- |
| GPU test programs/captures | https://github.com/JaCzekanski/ps1-tests/releases/download/build-158/tests.zip; source f727802fead1 | 5de0e82854c1a511697a7ade381c2d778ec4b2fff4a7d7ac79e8098c35eb859b | High, downloaded and hashed |
| Linux cross GCC | https://github.com/Lameguy64/PSn00bSDK/releases/download/v0.24/gcc-mipsel-none-elf-12.3.0-linux.zip | 228f031a25cf2687d8845fd1421f625bafc211fa27da428e458e80d030a726f8 | High, downloaded and hashed; not executed here |
| OpenBIOS source | https://github.com/pcsx-redux/nugget/tree/9db6d88d23c4da7a39b1a4b9729f1e5fff80dec9/openbios | Pin is public and MIT; output digest must be established with the pinned Linux toolchain | High for source/license, build output not measured locally |
| BIOS digest source | https://github.com/libretro/libretro-database/blob/12c21cf79f1e/dat/System.dat | Existing source pin; rows use MD5/SHA-1/CRC, not SHA-256 | High |

Downloads remain only at `/private/tmp/schwaystation-ps1-tests-build-158-tests.zip` and
`/private/tmp/schwaystation-gcc-mipsel-none-elf-12.3.0-linux.zip`. Executors may reuse these files after digest
verification, without executing Linux binaries on macOS. No archive bytes enter git. Keep notices and manifest
policy for anything transcoded or vendored. OpenBIOS transfer is within one CI run only, never a release artifact.

Evidence: `shasum -a 256` on both files exited 0 and produced the table digests. `unzip -Z1` exited 0; member
inspection found the image/log split below. The host has CMake, Ninja, RetroArch 1.22.2 and gh, but neither
mipsel-none-elf-gcc nor mipsel-linux-gnu-gcc. Use the pinned Linux CI lane for the OpenBIOS build/output digest;
no owner toolchain setup is needed.

## GPU oracle evidence correction

GAME-01's original blanket vram.png wording names captures that do not exist in the pinned release.
The ZIP contains `gpu/<case>/vram.png` for clipping, lines, rectangles, transparency, quad, triangle,
uv-interpolation, texture-overflow, vram-to-vram-overlap and clut-cache. It has only `psx.log` and an executable
for gp0-e1 and mask-bit. It has no gpustat directory; the pinned source supplies `gpu/gpustat/psx.log`.

Preserve all named behaviors: exact quantized RGB555 image checks for the ten image cases, plus real register/log
assertions against the pinned hardware logs for gp0-e1, mask-bit and gpustat. Do not fabricate captures, silently
exclude cases or turn failures into report-only tests. Record the evidence-backed wording correction in the
requirements and the plan that admits these inputs. Reference mismatches locate arithmetic defects; known-answer
cases still pin exact rejection boundaries, saturation/truncation and raw-texture dither semantics.

## Existing implementation seams

- Reuse instance-owned fixed-width state/visitor validation, scheduler deadlines, CPU bus hooks and DMA units.
  The current API is v3. Add only phase-used v4 names and test all of them in tests/api/v4.c; retain v1-v3 callers.
- GPU owns the current 1024x512 VRAM and shared CPU/DMA2 packet state. Rendering and beam state must remain
  deterministic under save/load, reset and split runs. Use bounded transfers, polygon rejection and list yields.
- Timer dot/hblank input APIs exist, but dot-clock IRQs require a derived mid-line deadline, not line-end batches.
  MMIO catches devices up to the access tick; test equal-tick ordering and repeated GP1 reset boundedness.
- host/xrgb8888.c, runner PNG/hash paths, libretro.host and the RetroArch driver are reusable. Add 24-bit packed
  input and dynamic geometry; preserve native field bytes/hashes. The real-host harness is 293/300 lines now.
- SIO/pad/card implementations are new. Schedule byte completion and ACK distinctly. Commit staged card payload
  only on successful protocol completion so raw spans and generation cannot disagree after a rejected write.
- Append BIOS identification separately from loading: SHA-256 reporting is distinct from DAT digest recognition.
  Unknown/OpenBIOS images must not be refused merely for lacking a retail match. Do not package firmware.

## Private library seam

The researcher recommended truthful unavailable/unknown classification for cue/bin before the phase-6 backend.
That is necessary for unsupported execution, but enumeration-only failure must not masquerade as a meaningful
GAME-06 success. The planner must cover every requirement field and its real temporary-fixture proof. Prefer the
smallest host-only identity/probe support needed by this first caller, reusing it in phase 6; this does not imply a
CD-ROM controller, m3u support or a public disc-play claim. Probe predicates can pass when measured; boot/play
predicates remain unknown/unavailable until a real supported execution establishes them. Never label metadata
inspection as a successful disc boot. If the contract is genuinely impossible without changing phase scope,
state the exact conflict rather than silently weakening the requirement.

## Slice graph and first callers

Plan 1 lands the CMake skeleton and per-directory inventories. Freeze shared headers/inventories before independent
waves; shared GPU files, SIO/card transport and root fixture registration are sequential seams. Split work into
under-30-minute contracts: GPU decoder/transfers; pixel pipeline; each primitive/stepper family; beam/timer events;
DMA2 readiness/busy/stalls; video conversion; SIO/digital pads; DualShock; staged cards; runner inputs/persistence;
BIOS API/identity; pinned Linux OpenBIOS build; protected ledger and local preset; libretro surfaces; hosted session;
GPU nightly references; docs and final cross-lane integration. Do not compress the entire GPU into one plan.

Bind existing todos to first callers: DMA/state/CHCR/list counts to GPU DMA2/state; timer cleanup to beam/timers;
EXE/bounds to runner/content; firmware to BIOS/OpenBIOS; .info to host claims; persistence to card hosts; nofp timeout
034 to the first ci.yml edit. The current repository-check budget is 1513 lines and the RetroArch total is 293.
Count added gate code honestly, reuse existing helpers and do not add unnamed gates. A phase-specific budget
needs explicit requirement wording and concrete accounting; no fraction of src or silent limit increase.

## Verification and remaining probes

Use `cmake --workflow --preset ci`, `cmake --workflow --preset hygiene`, subsystem CTest labels with
`--no-tests=error --output-on-failure`, and the existing JUnit inventory command. Every behavior test belongs to
ci and every test gates. Existing phase-3 scripts provide proven commands; avoid inventing another harness.
A multi-argument emulator must survive nested CMake negative-control commands (the phase-3 ship fix pins this).

Measure OpenBIOS output on Linux, exact hardware-log predicates and source pins at fixture admission, interpolation
rounding against the captured images, and actual CI costs when their callers land. Those are implementation
probes, not owner approvals. Nyquist and security-enforcement templates are disabled by project config; retain
ordinary adversarial tests for malformed GPU/SIO/card inputs, firmware rejection, private-data leaks and a head PR
weakening its own compatibility judge. Review precedes final verification, which reads .planning/AUTONOMOUS.md.
