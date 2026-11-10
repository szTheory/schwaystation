# Autonomous runs for milestone v1.0

The owner runs the rest of milestone v1.0 with `/gsd-autonomous` (AGENTS.md rule 9). This file holds the
standing guidance for the two steps that need the most judgement. Every discuss and verify step reads it,
autonomous or not, and follows it without asking.

## Discuss steps: fan out, synthesize, follow

For every decision point, fan out subagents (for context-window efficiency) and keep fanning out to diminishing
returns, then synthesize one coherent set of one-shot recommendations that fit the project goals, and follow
it. Do not ask the owner a routine question.

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
