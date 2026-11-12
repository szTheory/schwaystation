# Phase 4: Games play in RetroArch - Context

**Gathered:** 2026-11-10
**Status:** Ready for planning

<domain>
## Phase Boundary

Implement GAME-01 through GAME-07 and all five phase-4 roadmap criteria: the native GPU rasterizer and
beam/display machine, scheduled SIO0 digital/DualShock pads and rumble, two persistent memory cards,
OpenBIOS shell boot, a protected compatibility ledger/private library preset, and BIOS-free PS-EXE play
in real RetroArch. Three deliverables remain the library, runner and libretro core. SPU audio, CD-ROM,
disc parsing/m3u and MDEC stay in phases 5/6. No frontend/window is added.

The owner explicitly renewed autonomous authorization and advisor fan-out. Three advisors independently
covered rendering, beam/host integration and peripherals/boot/compatibility, followed by a combined
adversarial pass. Recommendations are followed without routine owner questions. AGENTS rules and
.planning/AUTONOMOUS.md govern all steps. Stop after phase-4 verification, before ship.
</domain>

<decisions>
## Implementation Decisions

### Core, GPU and rendering
- **D-01:** Preserve DEC.28/29/30/35: spec-led C17 integer edge-function/top-left triangles, 32-bit fixed-point
  attribute steppers, integer-DDA lines, span rectangles, immediate VRAM writes and a line-granular beam.
  Reuse existing helpers first. A small ares ISC or MAME per-file BSD seed is allowed only with retained notice
  and an independent check; do not port a whole renderer or add a dependency. Wider integer setup arithmetic
  is allowed to prevent overflow while stored steppers stay 32-bit.
- **D-02:** Implement the complete GAME-01 pixel pipeline, transfers, cache, clipping, span rejection and
  primitive families, with directed boundary/overflow tests. Start at documented arithmetic and tune rounding
  and stepper precision against pinned references. Distinguish a model choice from a hardware measurement.
  Named images do not automatically discriminate every arithmetic alternative: add small known-answer cases
  for blend truncation, saturation and exact size boundaries instead of claiming coverage from an unrelated image.
- **D-03:** Dither follows the documented PSX-SPX primitive rules: Gouraud/modulated polygons and lines,
  never rectangles; raw textured semi-transparent polygons do not gain dither merely because ares does so.
  Pinned quad is untextured and transparency disables dither, so neither settles that raw-texture question.
  New hardware evidence can change the model; do not manufacture hardware labels or require owner capture.
- **D-04:** Fetch-only build-158 executables and captures are digest-pinned, noticed and never distributed.
  Quantize reference images to RGB555 and require exact whole-VRAM comparison for the named GAME-01 cases,
  with only documented case-specific allow.txt variants. New archive inspection found three named cases
  without captures: gp0-e1 and mask-bit have only logs; gpustat exists only in the pinned source. Preserve
  those behaviors with actual register/log assertions and correct GAME-01 accordingly (04-RESEARCH.md).
  Every registered test gates. A released binary oracle
  may help locate a divergence; it cannot replace required hardware-reference checks or become a new release gate.
- **D-05:** CPU GP0 writes and DMA2 share one bounded packet decoder and transfer state. Preserve the existing
  atomic block/node DMA timing and stalls; implement GPU request/readiness bits 26/28 and command busy budget.
  Test packets crossing blocks/nodes, readback, overlapping copies, reset-mid-packet and hostile endless streams.

### Beam, timers and native video
- **D-06:** Keep DEC.09/.10's exact GPU/CPU ratio accumulator and 3413/3406 clocks per line with 263/314 lines.
  No new hardware capture resolves fractional alternatives. Label resulting timing records doc/derived.
- **D-07:** Advance the beam lazily but expose the next relevant blank/display and dot-target timer event to
  the scheduler; do not defer dot IRQs to a line end. Catch GPU/timers up through MMIO access ticks. Specify and
  test deterministic equal-tick ordering and phase carry, plus split-run equivalence and a mid-line dot IRQ.
  The scheduler must avoid per-dot polling when a next deadline can be derived.
- **D-08:** A bounded field/frame wrap ends run_frame independently of programmable display Y2 or a vblank IRQ.
  Oversized Y2 may suppress an IRQ but must never suppress the stop boundary. Replace the synthetic frame IRQ
  for loaded content with actual beam edges; retain the no-content test card and its existing hashes.
  New PSX-SPX documentation describes Y1/Y2 edge comparisons and GP1(00h) restarting video timing. Test
  those edges and reset explicitly; repeated video resets still cannot evade an independent emulated-time
  bound on a public run call, which returns a documented stop reason instead of hanging.
- **D-09:** GAME-02 already specifies native-height, field-sequential interlace: preserve Y2-Y1 core height,
  native 15/24-bit bytes, metadata hashing, window moves and display-off zeros. Correct conflicting DEC.34/prep
  wording rather than changing this requirement. Host conversion/presentation is outside the native hash.
- **D-10:** Extend the shared converter for packed RGB24 with wrap/bounds/pitch checks and known-answer colors.
  libretro calls SET_GEOMETRY in retro_run on dimension changes; reserve SET_SYSTEM_AV_INFO for rate changes.
  Test runner/host equality independently of the shared converter so one bug cannot bless both outputs.
- **D-11:** gpu_motion frame 60/120/180 must differ and pin across lanes; extend timers_lines with the exact
  per-profile line/dot/hblank records that GAME-02 asks for. A frame cap is a count, not an expanded mailbox size:
  the mailbox still holds at most 30 records. Keep fixture judging and cross-lane state/frame hashes real.

### Pads, SIO0 and cards
- **D-12:** Keep scheduled SIO0 byte completion and device ACK events with explicit reference ticks, including
  1088 cycles at baud 0088h and the IRQ7 lower bound. Preserve the prepared pad/card ACK distinctions; do not
  collapse every delay into one constant. Add digital and full phase-required DualShock command/watchdog/rumble
  behavior, deterministic input latching and per-frame rumble reports.
- **D-13:** pad_counter is the integrated behavior discriminator: none versus press@60 changes both frame and
  presses, three edges count three, held input counts once. Runner input scripts report lag excluding peeks.
  In-process libretro host tests map both pad layouts and compare input-driven video/rumble with the runner.
- **D-14:** Keep two stable raw 131072-byte card spans and successful-47h generation semantics. Stage sector writes
  until successful protocol completion, matching DEC.67's commit-at-completion rule; test wrong checksums and
  interrupted writes without missed dirty data. Do not claim unmeasured failed-write behavior as hardware fact.
  Cards remain separate from ordinary save states as already settled; validate before applying state changes.
- **D-15:** Host persistence owns all I/O. GAME-04's .srm and .1.mcr names win over stale .1/.2 naming prose;
  use temp, fsync and rename, clean SIGTERM frame completion/flush, raw/FF-padded import and lint without rewrite.
  Preserve applicable settled legacy wrapper import and serial-name policy with real consumers and tests,
  without growing this into a card-management UI. Fresh processes must show 1 then 2, plus missing/wrong controls.

### Firmware, compatibility and hosts
- **D-16:** Build OpenBIOS from public nugget pin 9db6d88d23c4 with the pinned Linux mipsel GCC release binary.
  Its missing pins.txt row is agent-owned first-fetch SHA-256 work, not an approval gate. Verify MIT at the pin,
  pin build output in tests/built/expected.txt, transfer only within the run, never package/upload firmware.
  A corrupted digest is an infrastructure error; the shell frame must pin across all applicable lanes.
- **D-17:** Report SHA-256 for every exactly-524288-byte BIOS image while recognizing retail images using
  the MD5/SHA-1 digests actually present in the pinned System.dat. Unknown and explicit OpenBIOS remain loadable
  under the documented API contract; a name never establishes identity. Reconcile stale SHA-256-table prose.
  Vendored digest rows retain notices; firmware archive scanning and test-key isolation have negative controls.
- **D-18:** Use ten meaningful ledger columns by keeping serial and rcheevos MD5 separate, preserving the
  existing combined identity and predicate as the key. Correct the nine-column sample/header in preparation;
  do not weaken GAME-06 or add padding. Maintain the exact allowed ratchet transitions and dated allowances.
  The PR baseline judge and ledger come from the base commit, so head changes cannot weaken their own judge.
- **D-19:** Generate docs/compatibility.md and check freshness, with real fixture rows and source confidence.
  The local-only library preset reads only owner-configured directories, runs no CI test against owner content,
  and writes only build/library/results.csv. Prove the path with a temporary fixture directory and absent/bad
  inputs. Never inspect the owner's .env.local or persist private content or paths in repository evidence.
- **D-20:** libretro PS-EXE fullpath loading uses the existing real-MIPS stub when system/ has no BIOS. Correct
  stale host prose that unconditionally requires BIOS for EXEs. Keep supports_no_game test-card operation.
  .info advertises only functional surfaces: disk_control remains false and disc functionality/extensions are
  not claimed before phase 6. Configure-time guards and in-process behavior tests verify each current claim.
- **D-21:** One hosted macOS RetroArch content session compares pad_counter at two frame limits, with empty
  system/, against exact runner PNGs. Reuse/refactor the existing 293-line harness to remain under 300 lines
  total; keep controller/saves/rumble assertions in the in-process host. No routine owner verification.

### Delivery, testing and proportionality
- **D-22:** API v4 adds only names consumed here, with frozen v1-v3 consumer tests and a v4 caller covering all
  additions. Apply the already-approved append-only compatibility policy, state-format and behavior changes
  through fields/tests/docs without renewed approval for routine slices. Do not silently break the published ABI.
- **D-23:** Plan 1 lands the CMake skeleton and per-directory test inventories; later slices stay under 30
  minutes and run concurrently only where neither headers nor inventories are shared. Bind every pending
  first-caller todo to its concrete plan, including DMA/state/CHCR/list counts, timer cleanup, EXE/bounds,
  firmware, .info, persistence and nofp timeout (034). Avoid source/tooling review-to-requirement growth.
- **D-24:** All behavior changes have CI tests. Add CI integration/e2e/smoke only for recurring feedback;
  perform useful one-shot checks directly and record command, exit status and run URL. Review before final
  verification. Apply the owner-approved phase-specific 2300-line ceiling now named in REQUIREMENTS, preserving phase 3's
  historical 1,513 ratchet. The 787-line additional allowance covers the named required checks and their measured scope;
  ratchet down to actual total at ship. Count code honestly and reuse helpers. No unrequested gates.
- **D-25:** Carry the newly observed nofp timeout evidence into its existing first-CI-caller todo: a one-minute
  cap cancelled two runs while identical commits completed in seconds, and a targeted idle-runner retry passed.
  Apply the existing at-least-three-minute remedy with measured run citations when ci.yml is first edited.
  Freeze the PR body before opening where possible and avoid description-only edits that trigger a full matrix.
  Measure duplicate work and critical path before adding caches, dependencies or concurrency.

### Agent's Discretion
Choose module-local representations, helper boundaries, exact test names and phase-local documentation layout
within these contracts. Resolve implementation uncertainty with bounded tests and primary sources, not owner
acknowledgments. Reopen a settled decision only with new evidence and record the reason.
</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- Existing object-library core, state visitor/validation, deadline scheduler, CPU bus hooks and DMA engine.
- One shared host/xrgb8888.c converter, runner PNG/hash tooling, in-process libretro host and hosted RetroArch driver.
- armips unity fixtures, mailbox judge/negative controls, per-directory inventories, split-run tests and fuzz replays.
- Existing firmware/pin helpers, publish/attestation pipeline and permissive provenance/manifest conventions.

### Established Patterns
- Instance-owned fixed-width integer state, no core I/O/clock/environment, bounded emulated execution.
- One cross-file schway__module namespace, hardware comments cite public docs/hardware pages.
- Exact per-lane inventories, specific negative controls and tolerant output regexes with exit-status assertions.

### Integration Points
- src/bus.c GPU/SIO dispatch; src/sched.c and frame.c beam/timer/stop behavior; src/dma.c GPU channel ends.
- src/internal.h/state.c and public API consumers; runner inputs/cards/BIOS; libretro.c content/input/video/saves.
- tests/{gpu,sio,card,fixtures,compat,libretro}, nightly GPU references and hosted CI cross-lane hashing.
</code_context>

<specifics>
## Specific Ideas

Read .planning/AUTONOMOUS.md at discuss/verify/CI/release steps. Preserve the small, flat dependency tree.
Source tiers and licence boundaries come from preparation/README.md, BRIEF.md, PROVENANCE.md and ASSET_POLICY.md.
Primary references: PSX-SPX GPU/peripheral specifications; ps1-tests at f727802fead1; ares at 9408cb43d494
(ISC, permitted files only); MAME at 96df6a708bdd (only device file headers declaring BSD-3-Clause);
nugget OpenBIOS at 9db6d88d23c4; libretro System.dat at 12c21cf79f1e; RetroArch libretro.h v1.22.2.

No prohibited emulator source is opened. Hardware, documented and derived observations remain distinct.
</specifics>

<deferred>
## Deferred Ideas

SPU/audio is phase 5; CD-ROM/m3u/disc control/MDEC is phase 6. Fractional physical line-length models and
uncaptured raster ambiguities remain evidence questions, with no fabricated hardware proof or owner capture gate.
Enhancements, whole-library formats and new frontends stay in their existing later milestones/seeds.
</deferred>
