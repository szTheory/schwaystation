---
phase: 04-games-play-in-retroarch
plan: "29"
status: incomplete
acceptance_complete: 0
acceptance_total: 2
resume_after: ["04-54", "04-55"]
runtime: Codex, OpenGSD 1.20.0
model: executor gpt-6-luna xhigh; focused prerequisite planner gpt-6.1-sol high
---

# RetroArch content checkpoint

Plan 29 has no SUMMARY and remains incomplete. Diagnostic commits 3133c14, 41dd821 and 993d705 are retained; do not reset/amend them to fabricate task order. Neither wiring/actual hosted content acceptance nor completed evidence/documentation acceptance has passed.

The normal full run at 41dd821 failed: https://github.com/szTheory/schwaystation/actions/runs/43271212007. Its RetroArch job was skipped by aggregate build dependency; it produced no BSV or content screenshots. At 993d705 the existing host job condition permits execution when not cancelled despite another build leg failing, while the required CI result still fails on any build failure. No successful run of that change is claimed.

## Concrete prerequisites

- GCC Linux x64/arm64 and BE32 fail compilation when signed comparison sums initialize unsigned pressed in tests/libretro/host.c. Plan 54 makes each term explicitly unsigned without changing the 0–3 range.
- Both MSVC builds fail warning-as-error on runner/probe.c read-only fopen sites. Plan 54 applies one private portable opener to all four sites, preserving diagnostics and failures.
- ASan excludes compat in its inventory and disables fixture generation, but tests/tests.cmake includes compat unconditionally and assigns setup properties to absent gpu_motion/pad_counter/memcard tests. WASI likewise excludes compat. Plan 54 makes the selected inventory own inclusion, preserving every native/BE32 compatibility case.
- The newer compatibility helpers call raw test executables and shell-owned runners without forwarding the full emulator; source inspection confirms this seam, while the prior GCC compile failure prevented runtime evidence. Plan 54 adapts the actual consumers, including ordinary-mode mirror and multiargument forwarding controls.
- Hosted macOS x64 input.movies timed out at 120.09 seconds; the other 508 cases passed, total 212.16 seconds. Plan 55 removes only unread trailing poison-buffer writes. The 3,538,944,000-byte full-fill work count is static evidence, not proof of timeout cause or future speedup. Keep every frame/reference/assertion and existing limits.

## Resume contract

Local pinned RetroArch 1.22.2 remains stopped after an unresolved LaunchServices-managed network denial. No local direct execution, escalation, security workaround or foreground activation is authorized. Continue only via the existing hosted background session driver. No new Xvfb, packages or dependencies are adopted.

The replay flags were confirmed in official documentation and pinned-binary strings; replay-v2 recording/playback has not yet succeeded in an actual run here. The existing zero-input replay probe is diagnostic only. A deterministic frame-indexed Cross producer remains unresolved; do not manufacture an old BSV format or treat replay hotkeys/zero input as controller injection. The advisor's temporary report is optional context; this checkpoint preserves its operative findings.

After 54 and 55, coordinator commits/pushes the repaired/wired head and dispatches one normal full CI run. Use that same run to validate the repairs, actual movie timing and hosted replay recording; avoid a redundant matrix at the known-broken source head. Keep the exact 7-minute BE32 job, nofp at least 3 minutes, 120-second native movie case, honest 2700 gate ceiling and 300 total RetroArch lines.

Plan 29 still requires a real hosted background session, empty system/, deterministic Cross at frame 60, two DISTINCT decoded screenshots at 59 and 60, each pixel-exact against its corresponding runner PNG. A zero-input record/playback equality or identical pair cannot satisfy this. Require command, exit status, actual run URL and matching source head; retain comparator wrong-pixel/dimension controls. If actual replay evidence leaves input production unresolved, keep 29 checkpointed and report the precise missing fact rather than advancing dependent plans.

Completed phase count remains 46; current execution ordinal remains 47. The appended prerequisite order is 54 → 55 → 29 → 30 → 46 → 47 → 48 → 31 → 32. No owner action is needed for the authorized repairs. Coordinator/checker acts next; stop after phase verification before ship.
