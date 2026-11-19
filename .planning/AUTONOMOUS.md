# Autonomous runs for milestone v2.0 — The whole library

The owner authorized autonomous work through milestone v2.0, starting at Phase 7, with each phase shipped before
the next and the milestone audit, completion and cleanup at the end (AGENTS.md rule 9). This file holds the
standing guidance for the steps that need the most judgement. Every discuss and verify step reads it, autonomous
or not, and follows it without asking.

## Discuss steps: fan out, synthesize, follow

For every decision point, fan out subagents (for context-window efficiency) and keep fanning out to diminishing
returns, then synthesize one coherent set of one-shot recommendations that fit the project goals, and follow
it. Do not ask the owner a routine question.

The owner explicitly authorizes auto-following the best-supported recommendations in autonomous discuss,
including accepting the recommended answers in smart-discuss batches without a separate confirmation. Pause
only when an unresolved choice materially changes scope, authority, risk, cost or a phase/milestone boundary,
or when owner-only information is required. This is not general risk acceptance; explain the concrete choice
before asking.

- Consider each decision point through every relevant stakeholder and technical-role lens: security, product,
  software architecture, devops and CI, plus the domain's own specialties (CPU and bus design, DSP and audio,
  3D and 2D graphics, distributed systems, C and the build toolchain). Add design lenses (conventions,
  principle of least surprise, patterns and anti-patterns) only for a phase that has a user interface.
- For each lens record pros, cons, tradeoffs, examples, patterns, anti-patterns, footguns and lessons learned.
- Prefer reputable primary sources (hardware documents, the preparation index in
  `.planning/preparation/README.md`, upstream test suites). Look at other emulators and ecosystems for ideas
  only, within rule 1: never open the emulators that rule names.
- Run an adversarial pass over the synthesized set before following it.
- Re-read `.planning/LESSONS.md`, and do not reopen a choice in `.planning/preparation/DECISIONS.md` without new
  evidence.
- Dependency taste: another copy and paste beats another dependency. A needed dependency is fine, but the tree
  stays small and flat (secure, and no stray abstractions). A permissive seed with its notice is preferred.

## Verify steps: automate, shift left, hand off nothing by default

The goal is zero human verification. This is settled (rule 6); do not re-propose it or loop back to it.

Launch automated apps and tests in the background under the standing preference in `AGENTS.md`.
Headless CLI and hosted checks already avoid desktop activation. Apply this to local RetroArch
checks as well; diagnose background failures before considering any foreground check.

- Automate integration, end-to-end, smoke and seam checks, shifted left. A check goes into CI only when it
  catches a recurring regression (rule 6); a check with no recurring value is run once by the agent and recorded
  as a command and an exit status. Throwaway verification is fine.
- Take automation as far as it gives useful feedback and no further; do not build what nothing will read.
- Hand the owner only what is identity-bound or needs their own hardware, and say why.
- Record what was automated in the phase's verification report so no later step revisits it.

## CI and release steps: look for cheap speed-ups

Whenever a CI or release run is in progress or finished, look for low-hanging wins in wall-clock time, iteration
time and Actions cost: caching, job parallelism, skipping a job a change cannot affect, trimmed matrix legs,
timeouts that follow measured runs. Never trade away check quality. Apply a safe win in the same step; record
anything larger as one todo (rule 7).

## Current authorization and boundaries

On 2026-11-15, the owner renewed autonomous authorization to finish milestone v1.0, starting with Phase 6.
The discussion, verification and CI guidance above is standing policy: use focused role fan-out, primary
sources, an adversarial pass and a synthesized best-supported recommendation; automate useful verification,
run one-off checks once, and do not introduce routine owner UAT or duplicate checks.

Ship each phase before advancing to the next. OpenGSD's `$gsd-autonomous` runs discuss → plan → execute →
verify and, when all phases are complete, audit → complete → cleanup; it does not ship phases between
verification and lifecycle. Therefore run each remaining phase with `$gsd-autonomous --from N --to N` so it
stops after that phase's verification and skips milestone lifecycle, then run `$gsd-ship N` and confirm the PR
is merged, main CI is green and the automatic release is published. After the final phase is shipped and
released, run milestone audit, completion and cleanup. Do not run `$gsd-new-milestone` until the current
milestone is complete, its release is published, main CI is green, and the working tree is clean.

This 2026-11-15 authorization supersedes the 2026-11-10 note below AGENTS.md rule 9 that stopped autonomous
work before ship. It does not authorize skipping a phase's verify or ship, bypassing repository rules, or
crossing the milestone boundary. Ask only for identity-bound consent, owner-hardware checks, or a real blocker.

On 2026-11-17, the owner renewed authorization to continue without routine consultation through Phase 6's
release and the v1.0 milestone audit, completion and cleanup. No owner capture is available or planned. This
extends the stop-at-verification boundary in AGENTS.md rule 9 for this run only; keep every named acceptance
gate, do not alter DISC-03 or its cycle cap to force a pass, and do not start v2.0 under this authorization.

On 2026-11-18, after v1.0 was released and main CI passed, the owner approved milestone v2.0, “The whole
library,” based on SEED-001. SEED-007 is required as its first phase. This supersedes the v1.0-only authorization
above and its “do not start v2.0” boundary. The owner's standing request is to continue autonomously through v2.0
research, requirements, roadmap, each phase's discuss → plan → execute → review → verify → ship cycle, then the
milestone audit, completion and cleanup. Keep to the approved SEED-001 scope; the browser page, states and movies,
and measured-accuracy campaign stay out of v2.0. Follow the branch, release, security, privacy, determinism and
verification rules. Pause only for a material scope change, identity-bound consent, owner-hardware action or a
concrete external blocker. Do not plan or ask for a new hardware capture; none is available soon.

For CI and CD efficiency, first measure critical-path time and duplicate work. Prefer removing redundant
runs and serial bottlenecks over adding caches, concurrency or dependencies. Keep all required coverage.
Batch ready fuzz/API/oracle work on one changed head, reuse fingerprinted review for unchanged files, and stop
an obsolete known-failing CI tail before dispatching its repaired head. Capture shell results with `rc` or
`task_rc` rather than zsh's read-only `status`; if a wrapper fails after all checks finish green, record that
accurately instead of repeating the full suite for bookkeeping.

Before a long test, build, emulation or hosted run, do the cheapest relevant consistency check first: verify
inputs and decoded output counts, command selectors and workflow wiring, inventories, or budget totals. Use
that preflight to catch a structural mismatch before spending time on the full run; maintain it as a gate only
when it prevents a recurring fault.

For delegated research, write concise findings and verified pins to the assigned artifact early. When the
remaining uncertainties are routine implementation probes, return them to the executor instead of delaying
the whole phase. If a handoff stalls, recover existing evidence and artifacts before repeating any research.

When editing plan commands that contain regex anchors such as `$'`, use literal patches or replacement
callbacks: JavaScript replacement strings interpret `$'` as surrounding text and can corrupt task blocks.
Before handing plans to a checker, use the existing failure-direction probe and one-shot balanced-tag/shell
syntax checks. Keep these as workflow practice, not another repository gate.

Execution should consume the existing context, research and predecessor interfaces instead of repeating broad
research. Reopen a settled point only for a concrete missing fact or contradictory evidence. Keep required
summary metadata and verification evidence, and avoid repeating the same outcome across prose sections.

After adding rows to an exact test inventory, preserve its separators and sort the rows bytewise before
configuring (for example, `LC_ALL=C sort -o FILE FILE`). Several phase-4 tracers first failed configuration
only because their new rows were unsorted. This is an editing practice, not a new gate or test.

Phase 4 API/state/behavior revision 4 is unreleased and is completed across its implementation slices. Preserve released/frozen API v1–v3 callers. Add concrete v4 fields and update affected interim v4 content-state goldens with evidence; these additions do not require another revision bump within this phase. Once revision 4 is released, the normal compatibility/version policy applies.

The owner explicitly approved raising the Phase 4 repository-check ceiling to 2,700 lines after
reviewing the complete 2,633-line library and host-claim proposal, including all counted control helpers.
This supersedes older Phase 4 plan text naming 1,850 or 2,300. Preserve every named check, honest counting and
the existing 300-line RetroArch limit; do not relocate or minify gate logic. Ratchet the actual counted
total at ship. Plan 04-26 may resume from its verified patch without another budget question within this cap.
The owner also renewed permission to apply measured low-hanging efficiency improvements while preserving
coverage. Count the entire shipping gate and its negative/self-test helpers in resource proposals; the
earlier prototype sample omitted a 164-line registered-control helper.

Preserve passing commits when resuming a checkpointed plan. If TDD or task ordering was missed before the
checkpoint, record the actual sequence in SUMMARY and STATE and continue from the verified HEAD. Do not amend,
reset or split solely to make history appear ideal or to manufacture RED/GREEN timing; legitimate corrective
history work remains available when the actual correction calls for it.

On 2026-11-12 the owner explicitly requested faster implementation and process after prolonged Phase 4 integration. Keep small related repairs under the current plan/owner with focused checks and honest summary updates; split only material scope or work exceeding the existing 30-minute planning rule. Run affected native configure/compiler/target checks before broad CI, combine coherent changed-head fixes into one hosted run when practical, and do not repeat broad suites for metadata. Executor-chosen provisional test/job timeouts may be adjusted from attributed evidence without owner approval; actual owner caps and every required behavior check remain binding. Parallel read-only diagnosis may remove independent investigation from the source executor's critical path.

The owner renewed the efficiency correction on2026-11-13 after the long integration run: scope a planning/checker dependency to the code/interfaces it actually governs, and keep independent already-authorized implementation moving while that review is read-only. Preserve exactly one local writer and phase code-review-before-final-verification; do not serialize unrelated ready work behind a draft review. Use retained agent-owned ci-j4/hygiene-j4 local overlays through Phase4 rather than recreate/delete perplan; tracked/hosted defaults remain unchanged.

Future subsystem plans bring pinned real oracle programs/captures into the earliest runnable behavior slices. Do not defer all real-program integration to a tail after synthetic tests have codified assumptions. Wire fetch/decode early, run the applicable real case when its first caller works, keep every required case gating, and preserve asset rules. Reuse reproducible throwaway static arithmetic analysis to shorten hypotheses; it is not a new maintained harness or repository gate.

The owner has a real PlayStation and capture card, but says a hardware capture will not be available anytime soon. Do not plan Phase 6 work around an owner capture or ask for one again. Search public hardware records, record any unresolved evidence gap, and keep advancing independent work. Revisit owner hardware only if a future scope decision truly requires it.

For compressed media references, count source units from the pinned input and compare them with both the caller's request and the guest's produced output before starting a long emulation run. Source completeness, decode progress, guest completion and hardware fidelity are separate facts. Report each one accurately; complete source coverage is not a decoder or conformance pass.

Before compiling pinned media tools, verify the archive digest, expected build files, submodule closure, exact-pin licences and input coverage. If a tool needs native libraries unavailable on every matrix leg, build it once on an existing compatible producer, publish a one-run artifact, and make every consumer validate its digest before continuing. Keep full disc construction and acceptance on every leg.

On 2026-11-17 the owner explicitly approved the Phase 6 unblock proposal in
`.planning/phases/06-discs-and-fmv/06-UNBLOCK-PROPOSAL.md`. This supersedes the earlier instruction to preserve
the exact 8-bit/full-image MDEC acceptance gates, only to the following scope: v1.0 claims deterministic integer
MDEC decoding with the available verified hardware coverage, not full 8-bit or full-frame/movie hardware
conformance. Preserve the exact 4-bit guest and direct model controls, the complete owned `disc_boot` STR
integration path, and all other named requirements. Do not fit a `±2` tolerance, change expected hardware bytes,
change the 4,915,200-cycle cap, or synthesize missing compressed input from output images. Remove or replace the
unfinishable 8-bit, ordered-status-guest, and full frame/movie conformance selectors with useful passing
gating checks only as specified by the successor plan; retain their pins and discrepancies as evidence for the
later measured-accuracy horizon. The frame and first movie inputs are complete (300 and 240 macroblocks); their
guest output/completion remains unresolved, and the frame display probe fails the existing pixel comparison.
Keep the compressed-unit coverage preflight before media execution. Continue
through Phase 6 review, the remaining local and hosted gates, ship, and the v1.0 audit/completion/cleanup under the
existing authorization. This approval does not authorize starting v2.0 before v1.0 is complete or published and
does not claim hardware accuracy beyond the retained evidence.
