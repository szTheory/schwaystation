---
phase: 08-per-game-card-saves
plan: "08-09"
reviewed: 2026-11-19
depth: standard
scope_commits: [f9988e1, d8bcc79, 2b0d935]
files_reviewed: 16
findings:
  critical: 0
  high: 0
  medium: 0
  low: 0
status: clean
---

# Phase 08 follow-up code review

Reviewed the new Phase 8 changes after the existing phase report: the two-process RetroArch card save path, its required guest-artifact handoff, and runner disc save-identity resolution. Read the changed source and corresponding tests in context.

## Findings

No significant implementation, security, performance, or test-wiring issues found.

The identity resolver resolves the first declared member once before either card path is applied. It returns an error before card loading when provider reads fail, and its fallback digest frames standalone media separately from playlists, hashes ordered TOC fields and all provider sectors, and excludes filesystem paths and playlist text. The runner tests cover first-member serial precedence, moved media, playlist formatting and membership, late audio changes, swaps, both card slots, and no card files on an injected read failure.

The RetroArch check reuses the existing macOS host job and fixture artifact, runs two separate processes to prove card continuation, and checks both card images after each process. The new test does not add a workflow matrix leg. Its measured CI duration remains pending the exact-head hosted run.

## Review limits

This review covers the changes in commits `f9988e1`, `d8bcc79`, and `2b0d935` plus their related tests. The earlier Phase 8 review remains the record for the 08-05/08-06 code. The phase's full focused verification has not run yet.

## Self-Check: PASSED

All 16 paths changed by the reviewed commits exist, and the reviewed commit IDs are present in the current history.
