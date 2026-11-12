# Phase 4 executable coverage

All items below are covered by the numbered plans in this directory. Task acceptance and its adjacent automated/fails_when pair are the executable contract. Every registered test gates; code review and accepted fixes precede final verification in plan 32. No owner UAT or further budget approval is required within the explicitly approved 2700-line ceiling. The owner revised it after reviewing the complete 2633-line proposal, including all counted controls; 1513 remains the historical phase-3 ratchet. Older plan/research references to 1850 or 2300 are superseded by REQUIREMENTS and AUTONOMOUS.

## Goal and requirements

| Source | Outcome | Executable plans |
| --- | --- | --- |
| GOAL / roadmap criterion 1 | Native GPU, display modes, moving portable frames and real hardware references | 03-12, 27, 30-31 |
| GOAL / criterion 2 | Button presses change play; DualShock and host rumble agree | 14-15, 18, 28 |
| GOAL / criterion 3 | Card saves survive fresh processes with missing/wrong-card controls | 16-17, 19, 28 |
| GOAL / criterion 4 | OpenBIOS shell, protected ledger and temporary local-library proof | 20-26 |
| GOAL / criterion 5 | BIOS-free real RetroArch content and exact runner screenshots | 27-29 |
| REQ GAME-01 | Decoder/transfers/cache, complete pixel and primitive pipeline, motion pins, references and fuzz | 03-08, 11-12, 30-31 |
| REQ GAME-02 | Beam/dot scheduling, native fields/hash metadata, DMA readiness, timing fixture, geometry | 09-12, 27 |
| REQ GAME-03 | Scheduled bytes/ACK, digital/DualShock, watchdog, movies/lag, both host layouts and rumble | 14-15, 18, 28, 31 |
| REQ GAME-04 | Stable card spans, successful-write generations, atomic persistence, imports, signal/fresh-process controls | 16-17, 19, 28, 31 |
| REQ GAME-05 | Copied BIOS API/digests, isolated recognition table, archive scan, pinned Linux build and portable shell | 20-23 |
| REQ GAME-06 | Ten columns, exact ratchet/allowances, both protected base artifacts, fresh docs, meaningful private-library seam | 24-26, 28 |
| REQ GAME-07 | BIOS-free content, bounded real-host harness, exact images and guarded functional claims | 27-29 |

## Research constraints

| Research item | Executable coverage |
| --- | --- |
| Verified build-158 digest and ten captures versus three log-only cases | 30 tasks 1-2: real NIGHTLY runner/judge targets, exact clipping selection, all named image/log registrations; synthetic behavior controls also run in ci |
| Public GCC/nugget pins, Linux-only compilation and measured output digest | 22 tasks 1-2, 23: actual Linux run evidence, digest/corruption control, run-local transfer and shell execution |
| Real DAT MD5/SHA1 recognition versus universal SHA256 reporting | 20-21: copied bytes, name-independent identity, isolated synthetic keys and archive-member rejection |
| Existing instance/state/scheduler/DMA/converter/host seams | 02-11, 14-17, 20, 27-28: real integration and bounded reset/state tests |
| Private-library metadata must measure something while disc execution is unavailable | 25-26: SWAY-00001, exact MD5, independent track SHA-1 and matching test-only ledger/DAT; metadata.identity passes, boot/play stay unknown |
| First introduction has no prior ledger or judge | 24 task 1: absent-both base means empty/all-unknown prior results with actual candidate fixture checks and independent review; one missing base artifact fails; later runs execute both copied base artifacts |
| Inventories, short slices, first-caller todos, budgets and automation-first evidence | 01, 02, 09-13, 16-17, 20-22, 27-29, 32; source/tooling review findings remain seeds |

## Locked decisions

| Decision | Implemented by plans and acceptance |
| --- | --- |
| D-01 | 03, 05-08: integer edge/top-left triangles, stored fixed-point attributes, DDA lines and spans with immediate VRAM writes |
| D-02 | 04-08, 30-31: all pixel/transfer/cache/primitive families and directed boundary/overflow/arithmetic discrimination |
| D-03 | 05, 08, 30: explicit dither eligibility, no rectangle/raw-texture eligibility inferred from unrelated captures |
| D-04 | 30: digest-pinned fetch-only inputs, exact ten-image and three-log comparisons; all registered cases gate |
| D-05 | 03-04, 08, 11, 31: shared bounded CPU/DMA decoder, transfer/reset/copy behavior, busy/readiness and hostile streams |
| D-06 | 09, 12: exact ratio/line constants and per-profile doc/derived timing records |
| D-07 | 09: lazy deadlines, mid-line dot IRQ, equal-tick order, MMIO catch-up and split-run carry |
| D-08 | 09, 31: field wrap independent of Y2/IRQ, reset-resistant public bound, no-content pins |
| D-09 | 10, 27, 32: native-height field-sequential interlace, native bytes/metadata hash and reconciled prose |
| D-10 | 10, 27: bounded RGB24 conversion and independent colors, correct geometry/rate notifications |
| D-11 | 12-13: distinct motion frames, exact line/dot/hblank records, unchanged 30-record mailbox and real cross-lane hashes |
| D-12 | 14-15, 16, 31: distinct SIO byte/ACK ticks, digital/DualShock commands, watchdog, input latch and per-frame rumble |
| D-13 | 18, 28: none/one/three/held movies, lag excluding peeks, both controller layouts and runner/host agreement |
| D-14 | 16, 19, 31: two stable spans, staged writes, successful-47h generation and card/state separation |
| D-15 | 17, 19, 28: host-only atomic I/O, canonical names, imports/lint, serial names and fresh-process/SIGTERM controls |
| D-16 | 22-23: pinned Linux OpenBIOS, measured digest, in-run transfer only and portable shell frame |
| D-17 | 20-21: universal exact-size SHA256, actual DAT recognition, unknown-image loading and isolated-key/archive controls |
| D-18 | 24: ten meaningful columns, exact transitions/dated allowances, truthful first introduction and protected later base judge plus ledger |
| D-19 | 24-26, 28: fresh generated docs, owner-only local preset, CI temporary-fixture proof, sole results.csv and experimental metadata |
| D-20 | 27-28, 32: BIOS-free stub execution, no-game card, guarded truthful .info and no early disc claims |
| D-21 | 29: one hosted macOS content session, two exact runner comparisons and at-most-300-line harness |
| D-22 | 03, 15-16, 20, 32: append-only API v4, real consumers, frozen old callers and explicit state/behavior revisions |
| D-23 | 01-03, 09-13, 16-17, 20-21, 27-28: first-plan seams, declared ownership and concrete first-caller todos |
| D-24 | 12, 24-26, 30, 32: authoritative budget, all behavior tests gate, useful automated evidence and review before final verification |
| D-25 | 13, 22, 29: first-ci-edit nofp remedy, measured CI costs and frozen PR body guidance |

## Raw edge candidates

Rows preserve the input order of `/tmp/schway-phase4-edge-coverage.json`. Generic probes are mapped to the actual domain contract; they introduce no unrelated policy or new gate.

| # | Requirement / category | Existing acceptance or justified assumption |
| --- | --- | --- |
| 1 | GAME-01 adjacency | 06 tasks 1-2: top-left ownership has no shared-edge crack/double blend; inclusive clip and exact span boundaries |
| 2 | GAME-01 empty | 03 task 1 partial/reset packets; 04 task 1 zero-encoded/one-pixel transfers; 06 zero-area and 08 zero-size spans |
| 3 | GAME-01 encoding | 04 little-endian halfwords and 30 canonical little-endian RGB555/PNG quantization; inputs are binary words, so Unicode normalization does not apply |
| 4 | GAME-01 ordering | 04 task 2 ordered overlapping copies/cache effects and 11 cross-block/CPU continuation; no sorting contract is introduced |
| 5 | GAME-01 idempotency | 04 task 2 duplicate writes/mask/cache controls; draws are stateful and blend/copy need not be idempotent |
| 6 | GAME-01 concurrency | 06 independent instances, 31 interleaved instances and partial-state reload; instances are single-threaded, so shared-instance parallel calls are not promised |
| 7 | GAME-02 adjacency | 09 target tick-1/tick/tick+1 and equal-tick events; 10 inverted and boundary display ranges |
| 8 | GAME-02 empty | 09 no-content pins, 10 empty/inverted/display-off output; repeated video reset still returns BUDGET |
| 9 | GAME-02 encoding | 10 native 15/24-bit bytes/pitch/wrap and metadata; 27 independent host colors and native interlace |
| 10 | GAME-02 ordering | 09 explicit timer/beam/DMA/SIO event ordering and split-run carry, with MMIO catch-up |
| 11 | GAME-03 boundary | 14 byte/ACK/IRQ deadline assertions, 15 exact watchdog tick-1/tick/tick+1 and truncated commands |
| 12 | GAME-03 precision | 14 baud-derived 1088-cycle completion versus distinct device ACK, 15 integer 33868800-tick watchdog and axis endpoints in 28 |
| 13 | GAME-03 concurrency | 14 pending-byte/ACK save/load, reset/deselect and independent ports/instances; 15 deterministic latch/split-run carry |
| 14 | GAME-04 adjacency | 16 both slots, first/last sectors, bounded addresses and stable separate spans; 28 memory alias boundaries |
| 15 | GAME-04 empty | 17 missing/empty/oversized/short import controls and FF tails; 19 missing/wrong-card process controls |
| 16 | GAME-04 ordering | 16 old bytes until final 47h, one completed generation; 19 per-slot dirty tracking and signal-safe completion/flush |
| 17 | GAME-04 idempotency | 16 repeated successes increment once per completed write; 17 read-only lint and 19 unchanged generations avoid extra flushes |
| 18 | GAME-04 concurrency | 16 interruption/state validation, 17 failed-write preservation, 19 SIGTERM; instances are single-threaded and host persistence owns I/O |
| 19 | GAME-05 adjacency | 20 exact 524288-byte size and ROM-window reset fetch; 21 every nested archive member is scanned |
| 20 | GAME-05 empty | 20 invalid-size/late-load atomicity and 27 BIOS-free EXE/no-game paths; 21 zero-member/wrong-archive infrastructure failure |
| 21 | GAME-05 encoding | 20-21 known SHA256/MD5/SHA1 facts and name-independent digest recognition; names never establish identity |
| 22 | GAME-05 ordering | 20 validate-before-copy and immutable ownership; 22 verify source/toolchain/output before any consumer; 23 digest failure differs from boot failure |
| 23 | GAME-05 concurrency | 20 changing/freeing host source cannot alter copied ROM; 22 checked run-local handoff; no shared-instance threading is promised |
| 24 | GAME-06 adjacency | 24 distinct serial+MD5+predicate keys, duplicate rejection and exact transition boundaries; 25 bounded CUE/ISO extents |
| 25 | GAME-06 empty | 24 genuine empty prior baseline versus malformed one-missing baseline; 26 missing/bad input fails while a valid empty directory has zero records |
| 26 | GAME-06 ordering | 24 stable generated byte order; 26 duplicate representations and repeatable result ordering with an explicit evaluation date |
| 27 | GAME-07 boundary | 27 partial-run late-load refusal and geometry changes; 28 exact axis/memory alias endpoints; 29 checks the harness ceiling |
| 28 | GAME-07 adjacency | 28 descriptor alias/select/disconnect boundaries and false-claim control; geometry notifications follow actual dimensions in 27 |
| 29 | GAME-07 empty | 27 empty system/, NULL/no-content behavior and invalid/truncated EXE; 28 absent host callback controls |
| 30 | GAME-07 ordering | 27 load/run/unload/reset lifecycle and notification counts; 28 once-per-frame latch and deinit reset |
| 31 | GAME-07 precision | 10 independent RGB conversion, 28 exact signed-axis and motor scaling, 29 pixel-exact screenshots at both frame limits |

Deferred SPU/audio, CD-ROM controller/m3u/MDEC, later library formats and unresolved physical timing alternatives remain outside this phase exactly as CONTEXT specifies. Metadata probing does not claim disc boot/play. No source item is silently dropped.

## Execution-evidence slice expansion

Original references above denote the complete ordered chains below. First IDs preserve their callers; each chain retains every original acceptance item. See04-SIZING.md for evidence and retained/consolidated scope.

| Original plan | Complete executable chain |
| --- | --- |
|04-16|04-16, 04-33, 04-34|
|04-17|04-17, 04-35|
|04-18|04-18, 04-36|
|04-19|04-19, 04-37|
|04-20|04-20, 04-38|
|04-21|04-21|
|04-22|04-22, 04-39|
|04-23|04-23, 04-50, 04-51, 04-52, 04-53, 04-40|
|04-24|04-24, 04-41|
|04-25|04-25, 04-49, 04-42|
|04-26|04-26|
|04-27|04-27, 04-43|
|04-28|04-28, 04-44, 04-45|
|04-29|04-54, 04-55, 04-29|
|04-30|04-30, 04-46, 04-47, 04-48|
|04-31|04-31|
|04-32|04-32|

## Source-owned boot blocker

| Source | New evidence and retained coverage | Executable plan |
| --- | --- | --- |
| GOAL criterion 4 / GAME-05 / RESEARCH actual pinned firmware boot / CONTEXT D-16, D-22–D-24 | The matching pinned OpenBIOS POST byte write faults before splash startup. Model only that exact write as an ignored status sink with fault/unbuilt/width/neighbor controls and source-confidence documentation. All inspected-shell/native-frame/same-run portable acceptance remains in 40; no hardware measurement or shell success is inferred from the fix. | 50 before 40, after 23/39 |

## Source-owned ATTR first caller

| Source | New evidence and retained coverage | Executable plan |
| --- | --- | --- |
| GOAL criterion 4 / GAME-05 / RESEARCH real pinned firmware boot / CONTEXT D-16, D-22–D-24 | The real boot trace polls writable ATTR at 1F801DAAh after writing transfer mode 0020h. Exact halfword readback needs one instance/reset/state-owned latch, with unchanged unsupported-width/neighbor/STATX unbuilt behavior, costs, independent ownership sweep and attributed interim-v4 content-state pins. This adds no audio, SPU timing, hardware reset-value or shell-success claim. Real inspected shell and same-run portable tuple remain in 40. | 51 before 40, after 50/23/39 |

## Source-owned DMA4 first caller

| Source | New evidence and retained coverage | Executable plan |
| --- | --- | --- |
| GOAL criterion 4 / GAME-05 / RESEARCH real pinned firmware upload / CONTEXT D-16, D-22–D-24 | Actual enabled/active DMA4 waits with zero DREQ and words. Derive persistent automatic write readiness from saved ATTR mode, retain manual block credits and existing atomic sink/costs, and test continuation/deassert/deadline/state boundaries. No duplicate producer state, SPU FIFO/audio/RAM or hardware-latency claim; actual recognizable-shell and same-run portable acceptance remain in 40. | 52 before 40, after 51 |

## Source-owned GPUSTAT field status

| Source | New evidence and retained coverage | Executable plan |
| --- | --- | --- |
| GOAL criterion 4 / GAME-05 / RESEARCH real pinned shell VSync first caller / CONTEXT D-16, D-22–D-24 | Missing dynamic bits 31/13 block the real caller. Primary PSX-SPX 519c8db supersedes stale bit 13 prose and requires one saved phase for interlace-off retention; scheduled status, v0/v2, mode/blank/state/purity controls use the existing explicitly limited beam clock model. Actual captured readiness log 48 remains independent; only 40 accepts a recognizable shell/portable tuple. | 53 before 40, after 52; see 53-EVIDENCE |

## Source-owned hosted portability prerequisites

This focused amendment retains the complete four-source audit above; every other GOAL, GAME-01–GAME-07, RESEARCH and D-01–D-25 mapping remains covered by its existing plan. No deferred item is introduced.

| Source | Retained contract and concrete new evidence | Plan | Status |
| --- | --- | --- | --- |
| GOAL criteria 2/4/5 | Existing controller/metadata code must compile and register correctly before a normal hosted run can prove play and exact screenshots. Actual GCC/MSVC/configuration failures are preserved in 29 CHECKPOINT. | 54 → 55 → 29 | COVERED |
| REQ GAME-03/GAME-07 | Four variants, two family-specific references, 300 frames each, all pixel/audio/state/mailbox/poll checks, analog packets and both-port motors remain intact. Active pixels retain poison canaries; only unread trailing fill work is removed. Actual macOS timeout is evidence, not a proved cause. | 54, 55; actual host proof remains 29 | COVERED |
| REQ GAME-06 | Selected inventory controls compat inclusion; native/BE32 retain all cases and complete emulator arguments reach direct and shell-owned consumers. ASan/WASI exact inventories remain unchanged, with no fabricated fixtures. | 54 | COVERED |
| RESEARCH existing converter/host/inventory seams | Reuse packed callback/reference and existing CMake/portable helper patterns; no new dependency, gate, source subsystem or package. Hosted compiler/BE32/WASI acceptance is from the next combined changed-head run, never inferred locally. | 54, 55, 29 | COVERED |
| CONTEXT D-13, D-19, D-21, D-23–D-25 | Preserve distinct 59/60 images with Cross at 60 and exact runner PNGs/empty BIOS; zero-input replay is diagnostic only. Respect local safety stop, 2700/300 budgets and existing job/case timeouts; measure work and actual timing without assuming speedup. | 54, 55, 29 | COVERED |
