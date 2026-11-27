# Phase 16 source and coverage audit

The phase goal is: players can use supported disc patches and cheat strings, and can see which library claims the shipped evidence supports. This audit maps every source constraint to executable work; it does not claim implementation or verification.

| Source | Item | Plan(s) | Status |
|---|---|---|---|
| GOAL | Supported player patch and cheat paths with evidence-supported library claims | 02–16, 18 | COVERED |
| REQ | PATCH-01: PPF1/2/3 and BPS, identity/bounds, immutable source, deterministic reads, atomic rejection | 01, 07–11, 16, 18 | COVERED |
| REQ | CHEAT-01: runner/libretro bounded text, explicit core write, no invalid-input effects, relevant state/hash inclusion | 01–06, 11, 14, 16, 18 | COVERED |
| REQ | COMPAT-01: public pinned licensed DAT, every one of 13,407 entries, pass/fail/unknown accounting and provenance | 01, 12–13, 16, 18 | COVERED |
| REQ | COMPAT-03: complete required categories, fresh exact-build results, no false non-experimental claim | 01, 14–16, 18 | COVERED |
| CONTEXT | D-01: exact raw 2352 source mapping, SHA binding/native checks, geometry, ordered overlaps and sampled identity boundary | 07–10, 16 | COVERED |
| CONTEXT | D-02: sparse ordered PPF and complete staged BPS candidate, no mutation before validation | 07–10 | COVERED |
| CONTEXT | D-03: 30/80 only, frame-start ordered RAM writes, transactional replacement, bounded whole table and lifecycle | 02–06, 11, 14 | COVERED |
| CONTEXT | D-04: narrow checked RAM write and additive public API version | 01–02, 16 | COVERED |
| CONTEXT | D-05: canonical future cheat state in runner hash/libretro envelope, validate before apply, no disc-state expansion | 03–06 | COVERED |
| CONTEXT | D-06: exact DAT pin/digest, occurrence accounting, sparse evidence and no bulk source redistribution | 12–13, 16 | COVERED |
| CONTEXT | D-07: complete fresh capability eligibility and protected real features.csv baseline | 14–16 | COVERED |
| CONTEXT | D-08: both long BE32 tests, exact selector/JUnit/hash contract and measured efficiency only | 11, 15, 17–18 | COVERED |
| RESEARCH | PPF3 64-bit offsets, optional 1,024-byte block, no PPF2-style size field; PPF1/2 independent golden layout fixtures | 07–08 | COVERED |
| RESEARCH | BPS four actions, varint/relative-cursor overflow, overlapping TargetCopy and all three CRC ranges | 09 | COVERED |
| RESEARCH | Native block/CRC checks are non-cryptographic; full external PPF SHA-256 and accurate performed-check report | 07–10, 16 | COVERED |
| RESEARCH | Reject ambiguous multi-track/compressed/pregap/synthetic mappings; prove exact raw file offset to callback LBA | 07, 10 | COVERED |
| RESEARCH | Validate complete transformed target before publication, cleanup staging I/O on all outcomes | 07–11 | COVERED |
| RESEARCH | Canonical GameShark RAM window, width/alignment, bounded text/global writes and no address wrapping | 02–06, 11 | COVERED |
| RESEARCH | Fixed endian host envelope, stable size, full candidate validation and continuation equality | 03, 05–06 | COVERED |
| RESEARCH | DAT string/nesting/duplicate/missing serial correctness and count every source occurrence before joins | 12–13 | COVERED |
| RESEARCH | Experimental metadata and cheats guard currently hard-coded; revise only with actual corresponding evidence | 14–15 | COVERED |
| RESEARCH | Main CI protected sparse checkout omits features.csv and workflow assertion must change together | 15 | COVERED |
| RESEARCH | Existing profile/protocol rows do not prove boot, input-driven play, picture or sound | 14–16 | COVERED |
| RESEARCH | Cheap count/key/coverage/selector/budget preflights, existing test-output reuse and bounded fuzz | 01, 11, 14–18 | COVERED |
| RESEARCH | Permissive primary specs, no new package, no prohibited sources or committed binary/media payloads | Every plan execution contract | COVERED |
| RESEARCH | Source-derived quality bounds, affected native consumers and exact hosted measurements | 18 and orchestrator reviewed-head verification | COVERED |

## Concrete implementation choices

- Add `SCHWAY_API_VERSION 11` for `schway_write_ram` per corrected D-04; the separate library ABI remains compatible because the operation is additive and no existing layout/signature changes. Plan 02 owns the new caller/registration and adjusts only the older callers' current-version comparisons, retaining their frozen layouts and behavior assertions.
- Shared host cheats have 32 entry indices, at most 64 lines per entry, at most 1,024 text bytes per entry, and at most 256 normalized writes across the table. Widths are 1/2 and offsets are canonical first-2-MiB main RAM only.
- PPF requires a supplied whole-source SHA-256 for every version, plus all applicable native checks. The supported source is a single raw CUE data track starting at file byte zero, with no stored or virtual pregap. PPF bounds are 16 MiB of patch data and 262,144 records.
- BPS uses the same exact raw stream, stages targets of at most 1 MiB in memory for portable/WASI cases, and private storage for larger targets. Source/target stay within the existing 1 GiB logical-image cap; patch and metadata caps are 64 MiB and 1 MiB.
- DAT input is processed by a host parser reached through a non-emulating runner command, so there is no fourth packaged program. Aggregate stdout is key=value; private per-occurrence output is optional and never committed.
- The capability evaluator imports completed existing CTest JUnit after the suite, paired with the configure-time source/configuration fingerprint. In-suite evaluator tests use isolated synthetic inputs. Missing integrated play evidence stays unknown; the actual metadata stays experimental until every required predicate passes.
- The protected feature ledger remains historical evidence protected against the actual base. It is not relabeled as fresh runtime evidence. A current capability view comes from completed measured results.

## Dependency and ownership audit

| Wave | Plans | Shared-file rule |
|---|---|---|
| 1 | 16-01 | Sequential owner |
| 2 | 16-02, 16-07 | No source, header or inventory overlaps within wave |
| 3 | 16-03, 16-08 | No source, header or inventory overlaps within wave |
| 4 | 16-04, 16-06, 16-09 | No source, header or inventory overlaps within wave |
| 5 | 16-05, 16-10, 16-11 | No source, header or inventory overlaps within wave |
| 6 | 16-12 | Sequential owner |
| 7 | 16-13 | Sequential owner |
| 8 | 16-14 | Sequential owner |
| 9 | 16-15 | Sequential owner |
| 10 | 16-16, 16-17 | No source, header or inventory overlaps within wave |
| 11 | 16-18 | Sequential owner |

Plan 01 establishes the CMake test skeleton and checks all six per-directory inventories. It uses the existing ABI registration/inventory and api_v10 test as its released-caller baseline; Plan 02 creates api_v11 and registers it immediately before adding the public write operation. Plan 01's 12 paths are a project-mandated first-plan scope exception under AGENTS.md rule 14: five expanded registration/inventory pairs plus two buildable entry sources are owned together, and the sixth ABI inventory is checked unchanged. Each of its three tasks modifies at most five files. The patch test source is created before Plan 01's first configure so its target never refers to a missing source. Plan 02 modifies eight files in two API-only tasks; Plan 03 owns all five shared-cheat/parser files in three tasks. Both later plans are below ten modified files, and no task exceeds five. Later waves reuse those registrations; source ownership prevents parallel edits to shared runner, libretro, header, provider or compatibility files. Normal STATE/SUMMARY bookkeeping is serialized by the orchestrator. All plans target 20–28 minutes with 2–3 tasks. The inherited coarse setting does not override the owner's 30-minute split rule.

The 18 schemas and structures were validated with the GSD CLI with no warnings. Decision coverage passed for all eight decisions, and the explicit same-wave file intersection check found no shared files. Each task modifies at most five files. No application, build or behavior test ran as part of planning.

## Plan-check revision

The first independent check found two blockers: Plan 01 listed 15 modified files, and the research artifact retained three unresolved questions. The revised plan moves api_v11 creation/registration into Plan 02 and uses the existing api_v10 baseline in Plan 01. A follow-up scope warning is addressed by moving Plan 02's shared host-cheat wiring/test task into the existing parser owner, Plan 03; Plan 02 now has eight paths, Plan 03 has five, and their dependency/wave order stays unchanged. Plan 01's broader registration set is retained under the explicit project rule described above. The research now records explicit resolved dispositions: every PPF version requires external whole-source SHA-256 plus applicable native checks (Plans 07–10), only the specified raw single-track/no-pregap CUE mapping is supported (Plans 07 and 10), and BPS uses bounded memory/private-file staging before publication (Plan 09). Those choices agree with the current context and plans; none remains open for execution to decide.

## Deliberate evidence boundaries

COMPAT-03 permits experimental=true when the matrix is incomplete, unknown or failing. Plans enumerate required boot-to-play rows and judge only their actual predicates; no task fabricates a passing guest result or claims every DAT title plays. Existing v1.0 MDEC accuracy, physical gun calibration and general product save-state claims remain outside this phase's evidence.

TODO052 is evaluated using both long tests. The timing/coverage analysis is in scope; a workflow split stays pending without a comparable measured before/after result and full exact coverage/artifact proof. Source review runs before final phase verification. The orchestrator owns the reviewed-head full local workflow and hosted gates; the autonomous run stops at the phase verify boundary before ship.

## Plan verification command conventions

Commands inherit the working root-relative CMake/CTest pattern from Phase 15. New exact selectors are registered by Plan 01 before later use. Hygiene-only selectors run with the hygiene preset, and behavior selectors run with ci. `git diff --check HEAD` checks the in-progress tree intentionally. Every CTest selection uses exact names, checks enumeration first and specifies no-tests=error. The new selectors' pass condition remains each underlying assertion's exit status.
