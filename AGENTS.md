# schwaystation

A Sony PlayStation (PS1) emulator core in C. This repository ships three things: a C library, a headless
command-line runner (`schwaystation-run`), and a libretro core (`schwaystation_libretro`). RetroArch or another
host provides the window. One core covers every console revision (SCPH-1000 through SCPH-102 and the PS one) and
every region (NTSC-J, NTSC-U/C, PAL); revisions and regions are machine profiles, not second emulators.

Project state lives in `.planning/` (PROJECT.md, REQUIREMENTS.md, ROADMAP.md, STATE.md). Reference material
with sources is indexed in `.planning/preparation/README.md`; open the file it points to before designing or
building a subsystem. Settled choices are in `.planning/preparation/DECISIONS.md`; do not reopen one without new
evidence. Lessons from the sibling emulators are in `.planning/LESSONS.md`; re-read it at every discuss step.

## Rules

1. **Reuse permissively, never GPL.** MIT, BSD, ISC, zlib and CC0 code may seed a module, with its notice kept as
   `PROVENANCE.md` describes: ares (ISC), the MAME device files whose own header reads BSD-3-Clause (never
   `src/mame/sony/psx.cpp`, which is GPL), and pcsx-redux/nugget (MIT, including OpenBIOS). DuckStation,
   Mednafen and Beetle PSX, PCSX-ReARMed, PCSX-Reloaded and the PCSX-Redux core, Avocado, Rustation and MiSTer
   PSX are never opened and no copy of them is kept in the workspace; reference emulators run only as released
   binaries.
2. **Three deliverables.** Library, runner, libretro core. No window, audio-device or input-device code in the
   deliverables; `examples/` may hold one example host that is built but never tested or packaged.
3. **Clean tree.** No BIOS, disc, executable, memory-card or state bytes except fixtures listed in
   `tests/fixtures/manifest.txt`. No personal paths, e-mail addresses or names. Commits use the GitHub noreply
   identity. `scripts/hygiene.sh` checks every commit and push (enable the hooks with
   `git config core.hooksPath .githooks`).
4. **Deterministic, bounded core.** Integer-only: no floating point anywhere. All state lives in the instance.
   The same inputs give the same frame, audio and state hashes on every platform. `schway_run_frame` and
   `schway_run` return within a bounded amount of emulated time with a stop reason, even for a hostile disc or
   executable. The core never reads a wall clock, a file or the environment; the host supplies disc sectors and
   the BIOS.
5. **Tested and documented together, within budgets.** Every behaviour change lands with a test that
   `cmake --workflow --preset ci` runs. The README stays user-facing and short; testing, CI and maintainer docs
   live in `docs/`; the public header stays terse and puts behaviour prose in `docs/`.
6. **Verification is automation-first; the owner verifies nothing by default.** This is settled; do not
   re-propose it. Deterministic checks go in tests; integration, end-to-end and smoke checks go in CI when they
   catch recurring regressions; a check with no recurring value is run once by the agent and recorded as a
   command and an exit status. The agent makes the judgement calls itself: it reads docs against the code, views
   images and screenshots, and runs RetroArch. What only a merge can show is confirmed at ship (rule 9), not in
   UAT. Ask the owner only for identity-bound consent (the App key, admin settings, a merge that rule 10's
   auto-merge cannot make) or a run on their own hardware, and say why.
7. **Proportional gates.** Repository-check code is CMake `-P` or POSIX sh. Its budget is an absolute line count
   per phase, named in REQUIREMENTS and ratcheted down at ship, never a fraction of `src/` (gates are front-loaded
   while `src/` is small). Real-host end-to-end test code (RetroArch) stays under 300 lines in total, checked by
   a test. Every gate names the fault it prevents; a gate that no requirement names is a seed, not phase-1 work.
   A third code-review fix round or a second gap round needs new evidence (XFER-085); leftovers become one todo
   each, bound to the plan in the next phase that adds their first caller. Code review runs before verification, never after it.
   Review findings about test or tooling code become seeds, not requirements. Re-plan a gap only with new
   evidence.
8. **No invented ceremony.** Never introduce attempt quotas, "formal slots", admission or publication
   ceremonies, lifetime counters, qualification campaigns, self-metering clocks or privileged OS queries. A
   session is bounded by time and tokens only. A failing harness assertion is a test bug to triage, not a release
   gate. Requirements never name a fact that only a published release can show, and never need a person who is
   not the owner.
9. **One GSD step per session, then stop.** Report what finished and the exact next command. Never run
   `/gsd-next`, `/gsd-autonomous`, `/gsd-manager`, `/gsd-progress --next` or `--do`, any GSD command whose
   arguments contain `--auto` or `--chain`, or `/gsd-profile-user`, and never set the hygiene bypasses
   `SCHWAYSTATION_ALLOW_CHAIN` or `SCHWAYSTATION_ALLOW_MAIN_COMMIT`. Inside a step, proceed without routine
   confirmations. Record the runtime and model used in STATE.md at every phase step.
10. **Branch, then pull request.** Never commit on `main` once a remote exists. Before the first GSD step that
    commits, branch off `origin/main`: `phase/NN-slug` for a phase, `milestone/vX.Y-slug` for milestone steps,
    `gsd/roadmap` for the kickoff. After `/gsd-ship`, give the pull request a Conventional Commit title, run
    `gh pr merge --auto --squash` in a shell (never inside GSD command text), and confirm a CI run exists on the
    head commit; after the merge, confirm the release pull request is green. Every phase ends in a release; the
    release pull request is never held. The phase-1 `ci:` tail (arming the perf gate, promoting the wasm32 lane)
    is a numbered step of its own between the phase-1 release and `/gsd-discuss-phase 2`.
11. **Evidence is a command, an exit status and a run URL**, never a committed log. Evidence is written by the
    command that produced it, from the repository root, with no `cd` chains.
12. **Pins are public.** A seed or fixture is pinned to a commit that `gh api repos/<o>/<r>/commits/<sha>`
    answers 200 for, never to a sibling's working tree or an unpushed branch. Sibling code is fetched with
    `gh api`, never read from a local checkout.
13. **Lessons.** Inbound lessons in `.planning/LESSONS.md` are hypotheses until reproduced here. When the owner
    asks to "export lessons", produce a pasteable block from that file's template.
14. **Plan shape.** Plan 1 of a phase lands the CMake skeleton and the per-directory test inventories; later
    plans run in parallel waves wherever they share no inventory or header, and sequentially where they do. A
    plan past 30 minutes is split. No test is "non-gating".

## GSD summaries

Every plan SUMMARY carries `requirements-completed:` and a block-style `coverage:` list. Each entry has `id`,
`description`, `requirement`, `verification` (kind one of unit, integration, e2e, automated_ui,
manual_procedural, other; `ref`; `status`) and `human_judgment` (true only for what rule 6 leaves to the
owner). No flow style `{}` and no inline `#` comments in that block. Plan acceptance checks assert on exit
status and tolerant regexes, never on a tool's exact output text.

## Style

Small modules, one per chip, plain control flow, fixed-width integer types. Public names start with `schway_`;
internal cross-file names start with `schway__<module>_`. A hardware comment says what the hardware does and
cites its source (a document, or a `.planning/preparation` section). Registers keep their documented names. No
planning IDs or other emulators' identifiers in code, test names or user docs. To decide how to do something,
follow the ladder at ponytail.dev: remove the need, reuse what is here, use the standard library, use the
platform, and only then write the minimum new code. Another copy and paste beats another dependency: when
outside code is needed, a permissive seed with its notice is preferred to a new package, so the dependency tree
stays small and flat.
