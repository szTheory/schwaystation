# Phase 15 source coverage and execution map

Planning runtime/model: Codex, GSD planner persona; primary session model ID unavailable.
Current Phase 15 research already verifies the repository/library seams, so no new dependency or external API discovery is needed. No install task, user setup, physical capture or workflow edit is planned. Security enforcement is disabled in project configuration.
Estimate calibration returned factor 1, sample_count 0, confidence low. Each plan targets 20–28 minutes; measured overruns require a narrower continuation, not dropped behavior.

CONTEXT has named decisions rather than D-NN identifiers. The local C labels below index its existing bullets without changing them. Executor actions cite the corresponding named decisions and DEC.75. The orchestrator clarified the libretro ambiguity: preserve center aim and type/callback gating; never claim per-control capability detection.

| Source | Item | Coverage | Status |
|---|---|---|---|
| GOAL | Deterministic trigger, aim, reload using beam and host input | 02–09, integrated 10 | COVERED |
| REQ | GUN-01: protocols, beam timestamp, threshold, bright/dark/edge/offscreen and IRQ | 03–06, 09–10 | COVERED |
| REQ | GUN-02: LIGHTGUN/POINTER controls; missing host unsupported/no false hit | 02, 05, 07–08, 10 | COVERED |
| REQ | SIO-05: official IDs, every class documented, frame boundary, no device/window code | 02–03, 07–10 | COVERED |
| CONTEXT C1 | Public extension after full 228-byte prefix; API 10; nested stride 24; size validation | 01–02 | COVERED |
| CONTEXT C2 | Explicit availability, no missing/out-of-range hit; same latched device/aim/control snapshot | 02–03, 05, 07–08 | COVERED |
| CONTEXT C3 | Existing byte/ACK timing; 5A63/5A31; Timer0/1 before IRQ10 | 04, 06 | COVERED |
| CONTEXT C4 | Integer GPU clock; bounded aimed-pixel event, live VRAM sample; geometry/region mapping | 03, 05–06 | COVERED |
| CONTEXT C5 | One documented RGB5 model threshold, boundary cases; offscreen distinct; ambiguous codes labeled | 04–05, 10 | COVERED |
| CONTEXT C6 | Pending host data excluded; fixed-width real in-flight fields and necessary revisions only | 03, 09 | COVERED |
| CONTEXT C7 | Official LIGHTGUN and Justifier subclass 2; base polling; all mapping docs | 07, 10 | COVERED |
| CONTEXT C8 | Complete LIGHTGUN controls; POINTER subset without separate reload; one snapshot per frame | 07–08 | COVERED |
| CONTEXT C9 | Capability query from retro_run; missing callback/type unsupported and no hit; neutral zero ambiguity | 07–08; interpretation below | COVERED |
| CONTEXT C10 | Direct-port gun support; retain old tap controllers; no dual-Justifier claim | 02–04, 06–08, 10 | COVERED |
| CONTEXT C11 | API/SIO/beam/in-process host vectors and only exercised compatibility predicates | 01–10 | COVERED |
| CONTEXT C12 | ABI/stride/inventory/selector/fixture/budget cheap preflights; existing tests first | Every execution contract, 01–02, 10 | COVERED |
| CONTEXT C13 | Producer-led quality checks, conditional act, inspect every future CI run, named Docker cleanup | Every execution contract, 09–10 | COVERED |
| RESEARCH | Existing instance-owned C17 input, FSM, GPU/timer/scheduler seams; no packages | 02–09 | COVERED |
| RESEARCH | Architectural responsibility: API normalizes, pad protocol, GPU timing, host callbacks, tests/docs evidence | 02–10 with bounded ownership below | COVERED |
| RESEARCH | RGB555 mask bit and reconstructed packed RGB24 displayed-pixel representation | 05 | COVERED |
| RESEARCH | Timer/GPU same-tick ordering and bounded zero-progress rejection | 05–06, 09 | COVERED |
| RESEARCH | All horizontal modes, region/board timing, display geometry and signed bounds | 02, 05 | COVERED |
| RESEARCH | Preserve prior released caller tests when API/device identifiers append | 01–02 | COVERED |
| RESEARCH | State continuation, no-host globals/I/O, deterministic split budgets | 03, 05–06, 09 | COVERED |
| RESEARCH | Physical-threshold uncertainty and no retail-compatibility inference (SW-ORACLE-001) | 04–05, 10 and every contract | COVERED |
| RESEARCH | Proportional existing gates, short BE32 selectors, no generic extra checker | 01, 10 and every contract | COVERED |
| RESEARCH | Public source provenance and no forbidden emulator source | All plans inherit AGENTS; no outside code/import task | COVERED |

## Concrete discretionary choices

- Public API: schway_gun_input is int32_t x/y plus uint32_t flags/reserved0, 16 bytes; guns[2] starts at byte 228 and total current input size is 260. C aim is active output pixels, availability is explicit.
- Brightness: sum of emitted RGB5 channels at least 48; ignore RGB555 mask bit. Packed RGB24 is reconstructed using scanout addressing then reduced by integer shifts. This is an emulator model, never a calibrated sensor claim.
- GunCon unavailable/dark response: X=1/Y=10; explicit offscreen/reload: X=1/Y=5. PER.01 interpretations overlap; docs identify this chosen interpretation.
- State format/behavior become 21 once actual gun hardware state is serialized; pending host input stays excluded.
- Libretro: the callback and device-type capability are the enforceable availability boundary. Advertised class plus callback permits center coordinates; zero trigger/pressed is neutral. The API cannot determine whether a particular zero-valued control is absent. Tests prove missing callback/capability is unsupported/no-hit and supported center trigger works. Do not create a center dead zone or assert per-control support detection.
- Existing features.csv is the synthetic feature ledger. It already has peripheral and predicate columns, unlike the retail/media identity ledger. Gun predicates belong there and in its generated compatibility docs; do not invent retail identity rows.

## Needs / creates / wave map

| Plan | Needs | Creates / observable result | Wave |
|---|---|---|---|
| 01 | Released API 9 and current test graph | Frozen old-caller tracer plus ABI/SIO/GPU/host exact registrations | 1 |
| 02 | 01 | Append-only validated API 10 gun snapshot | 2 |
| 03 | 02 | Atomic frame latch; pad result and GPU timing state declarations | 3 |
| 04 | 03 | GunCon byte replies and stable error/offscreen reports | 4 |
| 05 | 03 | Live displayed-pixel beam sample and region/geometry conversion | 4 |
| 06 | 04,05 | Justifier protocol enable and timer-visible IRQ10 | 5 |
| 07 | 04,05 | Official host gun selection, complete LIGHTGUN mapping, type/callback guard | 5 |
| 08 | 07 | POINTER subset and host/topology transition cases | 6 |
| 09 | 06 | Fixed-width sensor/protocol state continuation and aggregate measurement | 6 |
| 10 | 08,09 | Synthetic feature evidence, final docs/quality and review-before-verify handoff | 7 |

No same-wave plan modifies a shared header, inventory or implementation file. Shared pad/GPU implementations and shared libretro host source are sequential. Plan 01 owns all registration/inventory changes; future tasks consume those selectors. Its leading tracer intentionally follows the proven Phase 14 old-caller→frame→device pattern while the full gun stack is built through the short expansion plans.

No checkpoints: every choice above is within existing authorization. The orchestrator owns review, final verification, hosted CI, ship and release after these implementation plans; executor summaries report only commands actually run.

## Planning validation

All ten files passed frontmatter.validate --schema plan and verify.plan-structure using the installed GSD CLI (exit 0). A one-shot Python check ran bash -n over all 21 automated commands, parsed their inline Python with ast.parse, and exercised each selector assertion with missing, complete and duplicate selector inputs (exit 0). It also checked closing output tags, task file limits and same-wave file ownership; no conflicts were found. The only structure warning concerns git diff --check, which intentionally checks uncommitted whitespace before a task commit; behavior proof comes from the required CTest selectors. No source implementation, build or test suite was run during planning, and no commit was made.

## Exclusions, not source gaps

Multitap guns, shared multi-Justifier IRQ routing, two-player Justifier wiring, physical optical calibration/aperture, owner capture, a new emulator-specific gun option/dependency and todo 052 movie sharding remain deferred. No source item is silently removed. CI requirements/platform bounds remain required; a metadata-only planning edit does not launch broad CI.

