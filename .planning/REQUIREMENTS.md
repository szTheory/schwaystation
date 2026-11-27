# Requirements: schwaystation v3.0 — Prove a game plays

**Defined:** 2026-11-27

**Core Value:** PlayStation games behave as they do on the console, identically on every platform, from a small MIT-licensed C library that any host can embed.

## Milestone contract

v3.0 proves playable behavior through the existing C library, headless runner and libretro core. It uses two
evidence paths: an integrated, self-authored homebrew disc in automated RetroArch CI, and one owner-selected
retail title in a local RetroArch session using the owner's BIOS and disc. A playable session boots, accepts input
that changes gameplay, shows a changing picture and produces audible output.

No commercial BIOS, disc, save, screenshot, recording, RAM, trace or TTY content enters the repository, CI
artifacts or releases. The local run may record only the title identity, predicate result and date already allowed
by `ASSET_POLICY.md`; file names and paths stay local. OpenBIOS is the CI stand-in; the retail result uses the
owner's BIOS through the normal RetroArch content path. One retail title pass does not establish compatibility
for other titles or clear the libretro experimental marker.

Every behavior change has a gating check in `cmake --workflow --preset ci`. Determinism remains integer-only and
instance-owned. Real-host RetroArch test code stays at or below 300 lines total, and each retained check names the
recurring fault it prevents. No CI test requires the owner's BIOS or disc. Phase 17 starts with mandatory
SEED-007. The repository-check inventory (`scripts/*.sh`, `tests/cmake/*.cmake`, `.githooks/*`) is 4,694 lines
at milestone start; Phase 17's absolute ceiling is 4,694 and ratchets down to the measured total at ship. Any
later phase that needs a repository-check change declares its absolute per-phase ceiling here before Plan 1.

## v1 Requirements

### Milestone tune-up (SEED-007)

- [ ] **TUNE-07**: The tune-up compares up to the latest 40 CI runs with matching workflow, matrix and workload fingerprints, reports the critical path, duplicate work, flaky checks and core use, and makes no speedup claim when runs are not comparable.
- [ ] **TUNE-08**: The tune-up reports the slowest matching job and run, reviews flaky checks, and derives any changed timeout from twice the slowest comparable run; it fixes the cause or names the stronger signal replacing a flaky check.
- [ ] **TUNE-09**: A linked 1–5 review covers performance, CI/CD, ease of use, architecture, documentation, upgrade path and compatibility breadth; it makes and verifies one evidence-backed improvement to the weakest dimension.
- [ ] **TUNE-10**: The tune-up remeasures the existing `quality.json` producers and ratchets their bounds to measured results, rechecks low-power device assumptions, and makes no guessed budget or quality-bound change.
- [ ] **TUNE-11**: The tune-up checks repository settings, Dependabot, runner images and existing tool pins; refreshes affected maintainer, hardware and install documentation; updates the rolling roadmap and prompt coverage; and exports lessons without changing unrelated pins or caches absent evidence.

### Automated playability

- [ ] **PLAY-01**: A self-authored, generated bootable disc reaches an interactive homebrew game through the normal OpenBIOS handoff in the hosted RetroArch job; within one session, scripted controller input changes gameplay, the picture changes over time, and audible output is present.
- [ ] **PLAY-02**: Replaying the same homebrew input sequence produces deterministic output, and named RetroArch video/audio results agree with the runner's reference for the same guest and inputs.

### Retail playability and compatibility evidence

- [ ] **RETAIL-01**: One owner-selected retail title reaches interactive gameplay in a local RetroArch session with the owner's BIOS and disc; input changes gameplay, the picture changes over time, and audible output is present. The result is recorded only with the title identity and predicates permitted by `ASSET_POLICY.md`; all commercial bytes, captures and private paths remain local.
- [ ] **COMPAT-04**: Compatibility reporting records only the retail title and exact profile, media and playability predicates actually measured; every untested title or combination remains `unknown`, and libretro remains experimental until every required capability row passes.

## Future Requirements

- Fixed-size save states, rewind, run-ahead, frame-exact movies and Playstead continuation (SEED-002).
- A measured browser host and browser performance budgets (SEED-008).
- Broad title/provider/profile compatibility evidence and hardware-calibrated accuracy beyond one retail title (SEED-003).
- Debugger, GDB, scripting and AI interfaces (SEED-004); additional ports and JIT (SEED-005); renderer enhancements outside the hashed path (SEED-006).

## Out of Scope

| Feature | Reason |
|---------|--------|
| Claiming broad commercial-library compatibility or clearing `is_experimental` from one game pass | The required-capability matrix remains the public release criterion. |
| Committing or uploading commercial BIOS, disc, save, screenshot, recording, RAM, trace or TTY data | The asset policy keeps copyrighted content and local evidence private. |
| Physical-console hardware capture | No capture is available or planned; the accepted retail proof is a local RetroArch run. |
| Save states, movies, browser UI, debugger tools, JIT/ports or visual enhancements | These remain separate roadmap milestones and do not establish this playability proof. |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| TUNE-07 | Phase 17 | Pending |
| TUNE-08 | Phase 17 | Pending |
| TUNE-09 | Phase 17 | Pending |
| TUNE-10 | Phase 17 | Pending |
| TUNE-11 | Phase 17 | Pending |
| PLAY-01 | Phase 18 | Pending |
| PLAY-02 | Phase 18 | Pending |
| RETAIL-01 | Phase 19 | Pending |
| COMPAT-04 | Phase 19 | Pending |

**Coverage:**
- v1 requirements: 9 total
- Mapped to phases: 9
- Unmapped: 0

---
*Requirements defined: 2026-11-27*
*Last updated: 2026-11-27 after v3.0 roadmap traceability*
