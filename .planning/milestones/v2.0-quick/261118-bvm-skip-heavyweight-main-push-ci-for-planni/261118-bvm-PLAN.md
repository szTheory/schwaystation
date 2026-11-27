---
quick_id: 261118-bvm
phase: quick-261118-bvm
plan: "01"
description: Skip heavyweight main-push CI for trustworthy planning-only diffs
mode: quick
type: execute
wave: 1
depends_on: []
autonomous: true
requirements:
  - FRAME-05
  - FRAME-06
files_modified:
  - .github/workflows/ci.yml
  - tests/hygiene/tests.cmake
  - docs/testing.md
estimate:
  tokens: 14000
  raw_tokens: 14000
  tasks: 2
  confidence: low
must_haves:
  truths:
    - A direct main push with a complete planning-only diff reports heavy=false while its required hygiene and roll-up jobs still run.
    - Source, workflow, release-input and uncertain push diffs retain heavy CI.
    - PR classification and release workflow calls retain their existing conservative behavior.
    - Local checks expose classification mistakes before the long hosted matrix starts.
  artifacts:
    - path: .github/workflows/ci.yml
      provides: Direct-push diff classification before expensive jobs
    - path: tests/hygiene/tests.cmake
      provides: Deterministic push and failure controls in the existing ci.classify test
    - path: docs/testing.md
      provides: Accurate classification and local feedback guidance
  key_links:
    - from: GitHub push before/after commit IDs
      to: scripts/classify-changes.sh
      via: A complete git diff with both sides of renames represented
    - from: changes.outputs.heavy
      to: Existing heavy-job conditions and CI required
      via: Existing job dependencies and allowed skips
---

<objective>
Make planning-only main pushes finish through the cheap required checks instead of repeating the full platform and QEMU matrix. Preserve every check for source changes, release builds and any comparison that cannot be trusted. This is a separately authorized maintenance task after the completed milestone, not a new product milestone.
</objective>

<execution_context>
This quick plan follows the standard GSD execution workflow and summary format.
</execution_context>

<context>
@AGENTS.md
@.planning/AUTONOMOUS.md
@.planning/STATE.md
@.planning/RETROSPECTIVE.md
@.planning/milestones/v1.0-REQUIREMENTS.md
@.github/workflows/ci.yml
@.github/workflows/release.yml
@scripts/classify-changes.sh
@tests/hygiene/tests.cmake
@tests/hygiene/inventory.txt
@docs/testing.md

Discovery level: 0. Reuse the existing POSIX classifier, Git and CMake test conventions. No new package or action is needed. Existing archive requirements FRAME-05 and FRAME-06 govern required checks and release proof; do not reopen the completed roadmap.

The counted gate files currently total exactly 2,700 lines, their approved ceiling. Keep the new push orchestration in the existing workflow classify step and extend the existing ci.classify test in tests/hygiene/tests.cmake. Do not raise the ceiling, move existing logic, compress it to evade counting, or add another counted shell/check file. Keep the existing test name and inventory.

Observed waste: PR 24 changed only planning artifacts, so its PR matrix correctly skipped; its merge still triggered full main CI because the classify step currently forces every non-PR event to heavy=true. The extra run succeeded in about 38 minutes. Existing act and Docker are available; a changes-job dry run succeeded using a writable temporary action cache. That proves workflow structure only. Native CMake/CTest remains the cheap feedback path; hosted CI proves the actual platform, artifacts and RetroArch integration.
</context>

<tasks>

<task type="tracer" tdd="true">
  <name>Classify direct main pushes and exercise the actual workflow shell locally</name>
  <files>.github/workflows/ci.yml, tests/hygiene/tests.cmake</files>
  <behavior>
    - A direct main push between two real commits whose changed paths are exclusively under .planning/ produces heavy=false and release_inputs=false.
    - A source change, a source-to-planning or planning-to-source rename, or a path merely resembling .planning/ produces heavy=true.
    - A release-input change produces heavy=true and release_inputs=true.
    - A failed pull-request file listing keeps release_inputs=true as well as heavy=true.
    - Missing, malformed or zero commit IDs; an unavailable before commit; fetch/diff failure; checkout/after mismatch; an empty list; or a list beyond the classifier limit produces heavy=true and release_inputs=true.
    - A release call with its nonempty inputs.ref and a workflow dispatch stay heavy even when their commit happens to contain only planning changes.
    - The existing PR API path and its listing/count controls still execute as before.
  </behavior>
  <action>
First extend the existing ci.classify control to execute the workflow's actual classify run body with controlled environment values and throwaway Git repositories. Extract that one marked/identified run block during CMake configure rather than maintaining a second implementation of the event router. Retain the current path-list cases and tolerant outcome assertions. Use fixture repositories under the binary directory, Git's per-command noreply identity, local remotes and failure stubs where needed; require no network, token or owner action for these tests. Start with a failing planning-only push fixture.

Then update the classify step. Pass event, workflow/call-ref, before and after identifiers through explicit environment fields. Route a direct CI push to main into the new Git path only when no release-call ref is supplied; keep release workflow calls and manual runs heavy. Validate both endpoint IDs as nonzero full commit hashes before passing them to Git. Confirm the checked-out commit matches the after ID. Resolve the before commit locally or fetch that exact ID from origin with no tags and bounded shallow history; any failure returns both conservative outputs successfully so downstream coverage runs.

Generate the complete endpoint diff with rename detection disabled so a rename contributes its old and new paths. Force Git path quoting so control characters or unusual names cannot disguise a path outside .planning/. Do not use trigger-level paths filters, the GitHub compare API's truncated file list, a merge-base/HEAD-parent substitute, or a push payload's partial commits list. On a successful nonempty bounded list, pass its accurate count and paths to the existing scripts/classify-changes.sh. Preserve that script's PR input contract and the existing PR API branch. Keep all current required-job, release-rehearsal, concurrency and timeout conditions intact.

Review the resulting diff for unsafe skips before final verification. Cover the actual router as well as path classification so a correct helper cannot mask a workflow wiring error. Keep both tasks within 30 minutes each; report a concrete obstacle rather than starting a broad local matrix.
  </action>
  <verify>
    <automated>cmake --preset hygiene &amp;&amp; ctest --test-dir build/hygiene -R '^ci[.]classify$|^policy[.]gate_budget$' --no-tests=error --output-on-failure</automated>
  </verify>
  <done>The new positive case and all failure/rename/release controls pass locally; PR behavior is retained; the gate-line ceiling stays unchanged; every uncertain comparison selects the full matrix.</done>
</task>

<task type="auto">
  <name>Document the fast feedback path and validate the one changed workflow</name>
  <files>docs/testing.md, .github/workflows/ci.yml</files>
  <action>
Update the changes-job row and nearby CI guidance to describe trusted planning-only PR and main-push skips, conservative fallbacks, and full release-call coverage. Name the cheap local sequence: focused ci.classify and gate-budget checks, workflow lint, then the existing hygiene workflow. Describe act as an optional job-scoped dry run for workflow wiring; use the installed binary, an explicit Linux platform mapping and architecture, and a writable temporary action cache. State that a dry run does not execute tests or replace hosted matrix/artifact/RetroArch evidence. Do not install an act dependency, add global configuration, forward credentials, or run publication jobs locally.

Run the focused checks before the existing hygiene workflow. Reuse the working actionlint command and its two existing runner-label/concurrency exclusions; do not add broad new lint exclusions. Run one optional changes-job act dry run only if it adds useful workflow parsing evidence. Have the orchestrator open one Conventional Commit ci: PR, arm squash auto-merge, and follow the already-required hosted checks. This workflow change legitimately runs the heavy PR/rehearsal checks once; do not repeatedly launch full suites or cancel needed evidence to save time. After merge, confirm main CI and the release workflow report success. The change itself is outside .planning/, so its own main push remains heavy; later planning-only pushes benefit from the new path. Record commands, statuses and run URLs in the quick SUMMARY without committed logs.
  </action>
  <verify>
    <automated>actionlint -shellcheck= -pyflakes= -ignore 'label "ubuntu-26\.04(-arm)?" is unknown' -ignore 'unexpected key "queue" for "concurrency" section' .github/workflows/ci.yml .github/workflows/release.yml .github/workflows/nightly.yml</automated>
  </verify>
  <done>CI documentation matches the implemented router; focused tests, workflow lint and hygiene pass before hosted CI; one reviewed PR is ready for the orchestrator's authorized merge and main/release confirmation.</done>
</task>

</tasks>

<threat_model>
| Boundary | Fault | Severity | Mitigation |
|----------|-------|----------|------------|
| Push metadata to Git | Invalid or missing endpoints select an incomplete diff | high | Validate exact IDs, verify checkout, fetch the exact before commit, and select full CI on any failure. |
| Git paths to classifier | Renames or special filenames hide a code path under planning | high | Disable rename detection, retain both endpoints, force quoting, and test rename/path controls. |
| Event router to job conditions | A release call or manual run bypasses its full proof | high | Keep those routes conservative and exercise the actual workflow shell with synthetic call metadata. |
| Local simulation to completion claim | An act result is mistaken for hosted integration proof | medium | Record dry-run scope explicitly and retain required hosted checks before merge. |
</threat_model>

<verification>
Cheap checks first: git diff --check; configure hygiene; run ci.classify and policy.gate_budget; run the grounded actionlint command; review before final verification; then cmake --workflow --preset hygiene. Confirm tests remain in the existing registered inventory. A full local emulator matrix is unnecessary for this tooling-only change; the required hosted PR/main matrix provides its existing coverage once. Preserve no-self-skip and all aggregate-required checks.
</verification>

<source_audit>
| Source | Item | Coverage |
|--------|------|----------|
| GOAL | User-authorized immediate CI efficiency repair after milestone closeout | Both tasks; no new product milestone. |
| REQ | Archived FRAME-05: required repository checks remain enforceable | Task 1 conservative router and gating hygiene controls. |
| REQ | Archived FRAME-06: release input changes retain rehearsal and publication proof | Both tasks preserve and test call/release behavior. |
| RESEARCH | Existing act assessment: local Docker simulation differs from hosted matrix; main event routing causes the avoidable duplicate work | Task 2 keeps act optional and fixes the measured event path. |
| CONTEXT | Parent's locked scope: planning-only direct pushes, complete diff, fail closed, preserve PR/release behavior, cheap tests first | Task 1 behavior matrix and both task actions. |
</source_audit>

<success_criteria>
- Only complete trusted .planning/-only direct main pushes gain a heavy-job skip.
- Required hygiene and roll-up still report for every push; code, release, manual and uncertain inputs retain full coverage.
- The existing ci.classify test catches router, rename, release-call and failure mistakes without network or new dependencies.
- The current 2,700-line gate ceiling remains intact.
- The orchestrator receives a reviewable small change, honest local evidence, and one hosted PR/main verification path.
</success_criteria>

<output>
Create .planning/quick/261118-bvm-skip-heavyweight-main-push-ci-for-planni/261118-bvm-SUMMARY.md with requirements-completed, block-style coverage entries, runtime/model, commands with exit statuses, and hosted run URLs. Update the quick-task STATE row through the GSD schema helper. Do not modify ROADMAP.md or begin a product milestone.
</output>
