---
title: "Prompt coverage"
summary: "Every ask in the owner's kickoff prompt (2026-10-19) mapped to the file and section that answers it, with a status; and where the owner's constraints and preparation's recommendations differ."
read_when: "At kickoff review and at every milestone close; when the owner asks whether something was forgotten."
updated: 2026-10-19
---

# Prompt coverage

Statuses: **Documented** (a home and a settled recommendation), **Partial** (home exists; something remains, named),
**Later** (a seed holds it), **Inapplicable** (with the reason). Homes are preparation file stems with section
numbers (SYS = PSX-SYSTEM, GPU = PSX-GPU-AND-GTE, SCM = PSX-SPU-CDROM-AND-MDEC, PER = PSX-PERIPHERALS-AND-SAVES,
ECO = PSX-ECOSYSTEM-AND-USERS, PX = PSXDEV-EXTRACTS); AGENTS rule N; DECISIONS DEC.NN; SEED-NNN; "v1.0 phN".
Rows are never renumbered or deleted; new asks take the next number.

## 1. Coverage

| # | Owner ask | Home | Status | Remaining / when |
|---|---|---|---|---|
| 1 | A PlayStation emulator called schwaystation; build the right system | AGENTS preamble; BRIEF; DECISIONS DEC.01, DEC.04; SYS 8 | Documented | every hardware file is PS1-only; other consoles are labelled precedent |
| 2 | Lessons from gabbaboy, glueyneo, nesturbator, cogmaster, muhtari, genewhip: their .planning, code, commits, pitfalls, fastest progress | `.planning/LESSONS.md` XFER-001 to 105; ENGINEERING 15; GSD-HANDOFF 1; DECISIONS DEC.97 to 107 | Documented | re-read at every discuss (AGENTS preamble) |
| 3 | Do only the OpenGSD new-project step; stop before any phase (the owner switches models) | GSD-HANDOFF 2; AGENTS rule 9; DEC.97, DEC.98 | Documented | the kickoff session ends when the roadmap PR is merged |
| 4 | Playstead lessons; homebrew in CI | LESSONS XFER-025, 050, 103; CONFORMANCE 1, 2, 7; HOSTS 4 | Documented | Playstead's `.psx` extension rule and `biosRequired` are Playstead-side owner tasks (GSD-HANDOFF 7) |
| 5 | DNA of the owner's mature OSS projects (lattice_stripe, exifcleaner): CI/CD, release, triage | ENGINEERING 4 to 7, 13; LESSONS XFER-026 to 033 | Documented | |
| 6 | A reusable core with clean video/audio/input interfaces for other frontends | ARCHITECTURE 3, 4; HOSTS 7, 8; DEC.75, DEC.76 | Documented | `examples/sdl2/` proves a second consumer once discs play |
| 7 | Incredibly fast | ARCHITECTURE 8, 12; ENGINEERING 8, 14; ECO 4; DEC.19, DEC.20, DEC.85 | Documented | Ir/frame gate armed in the phase-1 `ci:` tail; cached interpreter and JIT v6.0 (SEED-005) |
| 8 | Correct; address what people dislike about other emulators | ECO 2 (C.NN, G.NN); CONFORMANCE 1 to 3; SYS 10, GPU 7, SCM 7 (confidence tables) | Documented | |
| 9 | Best-practice C, CMake, CI, with an adversarial fan-out per decision point through every stakeholder lens | ENGINEERING 2 to 4; BRIEF (fan-out method); DECISIONS; DEC.106 | Documented | discuss runs the fan-out again per phase |
| 10 | Code that is a joy to read, educational, KISS, YAGNI, ponytail.dev | AGENTS Style; ENGINEERING 10; DEC.90 | Documented | |
| 11 | Baselines in every measurable dimension, ratcheted to diminishing returns | ENGINEERING 8 (`quality.json`); CONFORMANCE 3.2 (ledger ratchet); DEC.86 | Partial | Ir/frame, header and README budgets in v1.0; `quality.json`, the digest and both audits from the v2.0 tune-up (SEED-007) |
| 12 | Hardware documentation with provenance, primary sources first, a RAG-friendly second brain in .planning | BRIEF (tiers, pins, markers); README (index); SYS, GPU, SCM, PER, PX; PROVENANCE 7 | Documented | facts graduate to `docs/hardware/<chip>.md` as subsystems land (ENGINEERING 11) |
| 13 | Development velocity as fast as possible | GSD-HANDOFF 1 to 3; LESSONS XFER-001, 002, 068, 083, 094; DEC.06, DEC.07, DEC.99, DEC.107 | Documented | phase 1 ports cogmaster at a public pin |
| 14 | Telemetry without overhead, optional | ARCHITECTURE 6 (counters, first fault, hooks); DEC.24 (traced loop only when hooks are installed); ECO 2 G.05 (no network telemetry) | Documented | hooks ABI v5.0 (SEED-004) |
| 15 | Great day-0/1/2 DX, easy adoption, docs never stale, self-documenting code | ENGINEERING 11, 12; HOSTS 1, 4, 7; DEC.89 | Documented | |
| 16 | Property-based testing, fuzzing, safe with hostile input | ENGINEERING 3; PER 7; CONFORMANCE 6; AGENTS rule 4; DEC.46, DEC.81 | Documented | fuzz targets report-only until green (AGENTS rule 8) |
| 17 | Fast CI and builds | ENGINEERING 4; DEC.79; SEED-007 | Documented | |
| 18 | Ship early and often; automated releases | AGENTS rule 10; ENGINEERING 5; DEC.06, DEC.87 | Documented | every phase a release; first one v1.0 ph1 |
| 19 | Always PRs, green main, triage at every step, auto-merge and auto-release with zero human verification where possible | AGENTS rules 6, 10; ENGINEERING 5 to 7; `scripts/apply-repo-settings.sh`; DEC.88 | Documented | |
| 20 | 12-factor secrets (`.env.local` locally, Actions secrets in CI) | ENGINEERING 13; `.env.example`; GSD-HANDOFF 7; DEC.95 | Documented | |
| 21 | Rolling near/mid/long roadmap revised at every milestone | `.planning/PRODUCT-ROADMAP.md`; seeds; GSD-HANDOFF 4; DEC.05 | Documented | |
| 22 | Performance stress testing to p99.999 on critical flows, folded into CI | ENGINEERING 8 (nightly `bench` tails over 300,000 frames; per-PR Ir gate); DEC.85 | Documented | |
| 23 | Recurring CI/CD efficiency audits baked in | ENGINEERING 12; SEED-007 | Partial | Ir/frame, header and README budgets in v1.0; `quality.json`, the digest and both audits from the v2.0 tune-up (SEED-007) |
| 24 | Recurring weakest-quality-dimension reviews baked in | ENGINEERING 12; SEED-007 | Partial | Ir/frame, header and README budgets in v1.0; `quality.json`, the digest and both audits from the v2.0 tune-up (SEED-007) |
| 25 | Verification shifted left and automated; only the truly necessary handed to the owner; recorded so it never loops back | AGENTS rule 6; DEC.101; GSD-HANDOFF 3 | Documented | settled, never re-proposed |
| 26 | Stop after each GSD phase so the owner can check and switch models | AGENTS rule 9; GSD-HANDOFF 3, 5, 6; `scripts/hygiene.sh` stop policy; DEC.98 | Documented | |
| 27 | Releases the owner can download and try on a Mac through some frontend (Playstead not ready) | HOSTS 3 (RetroArch Metal); README install line; DEC.70 | Documented | test frame v1.0 ph1; games ph3; sound ph4 |
| 28 | The owner's physical discs as test input | ASSET_POLICY 4; CONFORMANCE 3.3 (local `library` preset); DEC.83 | Documented | local only, from v1.0 ph3; never in CI |
| 29 | Speedrunning | DEC.08, DEC.44 (no speed changes); ECO 3.2; HOSTS 5; DEC.66 (lag frames); SEED-002 | Partial | lag count in the runner v1.0; movies v3.0 |
| 30 | Machine/AI-assisted speedruns and TAS | ECO 3.3; HOSTS 5; SEED-002, SEED-004 | Later | v3.0 movies, v5.0 tools |
| 31 | AI playthroughs | ECO 3.4; HOSTS 5 (Gymnasium order); SEED-004 | Later | v5.0 |
| 32 | Portable to low-power devices (DS, PSP, handhelds) | ECO 4; ENGINEERING 9, 14; GPU 9; SEED-005 | Partial | be32 and wasm32 lanes v1.0; device budgets are ratchet inputs; console lanes v6.0 |
| 33 | Consistent frame rate, no stutter, low latency | ARCHITECTURE 1 (bounded run); DEC.11 (input at vblank); HOSTS 1, 3; ENGINEERING 8; ECO 2 C.04, C.09, G.02 | Documented | |
| 34 | Great interface design for integration ergonomics | ARCHITECTURE 3; HOSTS 7; DEC.75 | Documented | consumer-minimal API growth per phase |
| 35 | All regions | SYS 8; DEC.04, DEC.17; ECO 2 C.12 | Documented | three region profiles v1.0; PAL and LibCrypt titles over the ledger v2.0 (SEED-001) |
| 36 | Different monitor/display types | GPU 4 (display mapping), GPU 8 (v1 frame format); HOSTS 8 (host helpers outside the hash); DEC.30, DEC.31, DEC.34 | Documented | |
| 37 | Maximise and ratchet compatibility: disc formats, controllers, protections, revisions | CONFORMANCE 3.2; SCM 2.7, 3; PER 2, 8; SYS 8; DEC.43, DEC.61, DEC.83; SEED-001 | Documented | cue/bin, iso, m3u and two pads v1.0; CHD, PBP, ECM, LibCrypt, multitap, guns v2.0 |
| 38 | Easy saves | PER 3.5, 3.6; HOSTS 1, 4, 8; DEC.62, DEC.67 | Documented | Beetle card layout v1.0; states v3.0 |
| 39 | ROM-hacker friendly without compromising the design | PER 6; HOSTS 5; DEC.65; SEED-004 | Later | PPF/BPS overlay and GameShark in the host layers v2.0; hooks v5.0 |
| 40 | Public repo, no PII | AGENTS rule 3; `scripts/hygiene.sh`; ASSET_POLICY 1, 6 | Documented | |
| 41 | MIT or Apache | DEC.03 (MIT); PROVENANCE 1 | Documented | |
| 42 | What people like and dislike about emulators in general | ECO 1.3, 2 | Documented | |
| 43 | Strangler-fig reuse of permissive code | PROVENANCE 4; AGENTS rule 1; DEC.52, DEC.53 | Documented | |
| 44 | Best-in-class in every quality lens, not a generic emulator | ECO 6; DECISIONS; ENGINEERING 12; PRODUCT-ROADMAP | Documented | |
| 45 | Power efficiency and battery life on handhelds | ENGINEERING 14; ECO 4; GPU 9; SEED-005 | Partial | measured in the nightly bench from v1.0; device runs v6.0 |
| 46 | Efficient use in the browser through WebAssembly | ENGINEERING 14; HOSTS 6; DEC.73, DEC.96; SEED-008 | Partial | wasm32 hash lane v1.0 ph1; the page after v1.0 |
| 47 | A GSD second brain the owner can reference with a gsd new command | this directory; README; GSD-HANDOFF 2 (the `/gsd-new-project` resume text) | Documented | |
| 48 | Keep all documentation current as the project evolves | README "Keeping it current"; ENGINEERING 11, 12 (docs freshness loop); DEC.89 | Documented | PROMPT-COVERAGE and PRODUCT-ROADMAP refreshed at each milestone close |

## 2. Owner constraints vs preparation's choices

| Owner required | Preparation chose | Why |
|---|---|---|
| MIT or Apache | MIT | same as the siblings; compatible with every frontend; the slot a permissive PS1 core fills is the product thesis (ECO 6) |
| Correct; remove what users dislike (BIOS friction is complaint C.01) | No HLE BIOS: a retail BIOS is user-supplied and recognised by digest; OpenBIOS is a CI stand-in only, never shipped (DEC.49, DEC.50) | HLE is the top compatibility complaint and firmware bytes never enter the repository |
| Maximise compatibility (disc formats) | No CHD in v1.0: cue/bin, iso and m3u only; `.chd` refused with a planned-version message; libchdr behind an option in v2.0 (DEC.43) | three more licences in every archive and a decompression path the fixture cap cannot cover; the ledger comes first |
| Play games with sound; FMV counts | MDEC in v1.0 phase 6 with the CD-ROM, after the SPU in phase 5, integer IDCT (DEC.06, DEC.33) | FMV is in most titles' boot path; the decoded-frame hash is part of the determinism claim |
| Stop after each phase | One GSD step per session, enforced by AGENTS rule 9 and a hygiene check (DEC.98) | stronger than asked, because chaining flags leak from global defaults (XFER-034) |
| A frontend on the Mac to try releases (Playstead not ready) | No GUI in this repository; RetroArch (Metal build) is the frontend, Playstead later | sibling rule that core repositories own no GUI; one install line |
| Speedrunning, TAS, AI, handhelds, browser | Horizons by reach and dependency: players first (v1.0, v2.0), states and movies (v3.0), measured accuracy (v4.0), tools (v5.0), everywhere (v6.0); the browser seed can jump the queue after v1.0 | each later horizon depends on the earlier one; every phase still ships |
| Primary sources, sanity-checked upstream | A preparation pack plus a fact-check and fresh-eyes review before kickoff; GSD's researcher still runs before every phase; psxdev.net facts are (unverified) because the site is offline | both; preparation is not a substitute for per-phase research |
| The owner's physical discs as test input | Only in the local `library` preset; only serial, hash, title, result and date leave the machine (ASSET_POLICY 4) | a public repository and CI never see a commercial sector |

## 3. Keeping this file true
Refresh at kickoff review and at each milestone close. A new owner ask takes the next number. A row moves from
Later to Documented only when its seed is promoted and the work is planned. Requirement traceability in
`REQUIREMENTS.md` does not replace this check: it covers requirements, this covers asks.
