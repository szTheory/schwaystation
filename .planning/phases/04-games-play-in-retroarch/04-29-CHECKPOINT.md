---
phase: 04-games-play-in-retroarch
plan: "29"
status: incomplete
acceptance_complete: 0
acceptance_total: 2
resume_after: ["04-56"]
runtime: Codex, OpenGSD 1.20.0
model: executor gpt-6-luna xhigh; focused prerequisite planner gpt-6.1-sol high
---

# RetroArch content checkpoint

Plan 29 has no SUMMARY and remains incomplete. Diagnostic commits 3133c14, 41dd821 and 993d705 are retained; do not reset/amend them to fabricate task order. Neither wiring/actual hosted content acceptance nor completed evidence/documentation acceptance has passed.

The normal full run at 41dd821 failed: https://github.com/szTheory/schwaystation/actions/runs/43271212007. Its RetroArch job was skipped by aggregate build dependency; it produced no BSV or content screenshots. At 993d705 the existing host job condition permits execution when not cancelled despite another build leg failing, while the required CI result still fails on any build failure. No successful run of that change is claimed.

## First run and completed source prerequisites

- GCC Linux x64/arm64 and BE32 fail compilation when signed comparison sums initialize unsigned pressed in tests/libretro/host.c. Plan 54 makes each term explicitly unsigned without changing the 0–3 range.
- Both MSVC builds fail warning-as-error on runner/probe.c read-only fopen sites. Plan 54 applies one private portable opener to all four sites, preserving diagnostics and failures.
- ASan excludes compat in its inventory and disables fixture generation, but tests/tests.cmake includes compat unconditionally and assigns setup properties to absent gpu_motion/pad_counter/memcard tests. WASI likewise excludes compat. Plan 54 makes the selected inventory own inclusion, preserving every native/BE32 compatibility case.
- The newer compatibility helpers call raw test executables and shell-owned runners without forwarding the full emulator; source inspection confirms this seam, while the prior GCC compile failure prevented runtime evidence. Plan 54 adapts the actual consumers, including ordinary-mode mirror and multiargument forwarding controls.
- Hosted macOS x64 input.movies timed out at 120.09 seconds; the other 508 cases passed, total 212.16 seconds. Plan 55 removes only unread trailing poison-buffer writes. The 3,538,944,000-byte full-fill work count is static evidence, not proof of timeout cause or future speedup. Keep every frame/reference/assertion and existing limits.

## Second terminal hosted run

The full run https://github.com/szTheory/schwaystation/actions/runs/43292417084 at 05a4c48 is terminal and unsuccessful. Plans 54/55 are complete source slices, but this run does not establish complete portability or content acceptance:

- Both Windows jobs still fail C4996 with warnings as errors in host/disc_probe.c; runner/probe.c's earlier helper is already repaired. The diagnostics report four sites; current source has five actual binary fopen sites. Plan 56 applies the same private portable opener to every actual site without changing failure/cleanup/privacy semantics.
- ASan generation fails because tests/libretro/tests.cmake depends on absent schwaystation-fixtures while the preset disables production. Its selected inventory contains all 18 libretro cases, including real fixture consumers. Plan 56 explicitly enables the existing producer in asan and fuzz, separates standalone fixture registration by selected inventory, preserves all selected consumers and normal CI's 74 fixture cases, and updates stale no-armips comments. A target guard or skipped needed test is not a repair.
- macOS x64 libretro.input.movies still times out at 120.07 seconds; its other 508 of 509 cases pass. Plan 55's measured byte saving therefore has not proved a timeout remedy.
- BE32 is cancelled at its seven-minute job setting with 474/476 tests reported. libretro.input.movies starts at 19:46:21 and the job is cancelled at 19:47:17; fixtures.pad_counter.movies takes 234.45 seconds and runner.hashes_file 102.38 seconds. This is cancellation evidence, not a proved hang or a complete movie timing sample.
- The existing hosted replay-v2 zero-input record/playback diagnostic succeeds. Its private temporary BSV is 16598 bytes with SHA256 516d8120cb1c931adc24aaf6860247cbef54cfdeab438fcf88c20ed7d84afa11; both 259-byte PNGs are identical. Keep this input/content in private temporary storage, never in the tree. It proves neither deterministic Cross-at-60 injection nor the required distinct screenshots and earns no GAME-07 acceptance.

The 120-second native movie timeout originated in executor 28 and the seven-minute BE32 job setting in executor 22. They are current configuration, not owner-specified consent limits. Preserve both during 56; perform separately attributed timing/scheduling diagnosis before retrying or tuning them. Do not infer cause/speedup from write counts, invent an approval boundary, drop coverage, or silently absorb timing work into this source prerequisite. ASan's older 189-test/28-second sample does not forecast the real producer/current-consumer run.

## Resume contract

Local pinned RetroArch 1.22.2 remains stopped after an unresolved LaunchServices-managed network denial. No local direct execution, escalation, security workaround or foreground activation is authorized. Continue only via the existing hosted background session driver. No new Xvfb, packages or dependencies are adopted.

Replay-v2 recording/playback now has actual hosted zero-input evidence as recorded above. A deterministic frame-indexed Cross producer remains unresolved; do not manufacture an old BSV format or treat replay hotkeys/zero input as controller injection. The advisor's temporary report is optional context; this checkpoint preserves its operative findings.

After 56 and separate timing diagnosis, coordinator commits/pushes the repaired/wired head and dispatches one normal full CI run. Use the same run to validate Windows/Linux sanitizer repairs, actual movie timing and hosted replay/input evidence; avoid per-task matrices at a known-broken head. Preserve current timeout settings inside 56, nofp at least 3 minutes, the honest 2700 gate ceiling and 300 total RetroArch lines. No local RetroArch retry or workaround is authorized.

Plan 29 still requires a real hosted background session, empty system/, deterministic Cross at frame 60, two DISTINCT decoded screenshots at 59 and 60, each pixel-exact against its corresponding runner PNG. A zero-input record/playback equality or identical pair cannot satisfy this. Require command, exit status, actual run URL and matching source head; retain comparator wrong-pixel/dimension controls. If actual replay evidence leaves input production unresolved, keep 29 checkpointed and report the precise missing fact rather than advancing dependent plans.

Completed phase count is 48 of 56; current execution ordinal remains 49. Plans 54/55 are complete. Remaining order is 56 → 29 → 30 → 46 → 47 → 48 → 31 → 32, with separate timing diagnosis before the next hosted retry. Plan 29 remains 0/2 acceptance and has no SUMMARY. No owner action is needed for authorized source repair/diagnosis. Coordinator/checker acts next; stop after phase verification before ship.
