# Phase 7: Faster and more trustworthy CI - Research

**Researched:** 2026-11-18
**Domain:** GitHub Actions workflows, CI classification, and repository quality checks
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

### CI evidence and efficiency
- Compare at most 40 recent runs only when workflow, matrix leg, and workload fingerprint match. Show sample size, evidence links, critical path, queue/wall times where available, retry/flaky signals, runner-core use, and cache evidence. State when data cannot support a comparison; never infer a speedup from unlike runs.
- Derive each timeout from twice the slowest matching successful or failed execution duration, retaining the established setup floor and naming the source run ID beside the value. Fix each evidenced flake or identify the stronger retained signal that makes it redundant; do not mask or drop required coverage.
- Treat BE32's 65-72 minute runs as the observed current critical path. Inspect whether the same workload is needlessly repeated across PR, main, and release events before proposing changes. Retain the required post-merge main CI and release rehearsal, all eight relevant lanes, and the existing planning-only fast path. No broad matrix trimming, blind cache, or concurrency increase.
- Run the report and focused local preflights first; use one changed-head hosted run for coherent workflow changes. A scoped `act --dryrun` validates wiring only and is not evidence that jobs/tests pass.

### Weekly triage artifact
- Produce a short-retention artifact from scheduled and manually dispatched runs. It contains a fixed schema and only useful open-issue metadata plus the latest workflow/run status; omit bodies, logs, authors, branches, commit SHAs, and any message/posting action.
- Filter pull requests returned by the issue API. Bound and sanitize untrusted titles/labels, removing control/ANSI text, e-mail addresses, and private filesystem paths before JSON encoding. Use the smallest job-level read permissions needed and no write permissions; use the repository's existing SHA-pinned artifact action and a seven-day retention period.
- Prefer a small built-in shell/JQ implementation over adding a runtime dependency or general-purpose report service. Add focused positive and hostile-input controls.

### Quality, docs, and dependencies
- Keep one checked-in `quality.json` with the required measured ceilings and floors for instructions per frame, Wasm bytes, serialized state bytes, and comparable CI critical-path seconds. The dated implementation adjustment adds the reason for the repository-check cap change to `reasons[]`. Reuse CMake `string(JSON)` and negative controls.
- Check lesson uniqueness by both identity and normalized lesson text, with a negative control. Apply the 3,300-line repository-check ceiling from the dated [budget decision](07-CONTEXT.md#repository-check-cap-adjustment-on-2026-11-18), ratchet it to the measured total at ship and maintain the 300-line RetroArch ceiling. Each new gate must prevent a named recurring fault.
- Score all seven dimensions with direct evidence links, then implement and verify exactly one evidence-backed improvement in the lowest-scoring dimension. Do not select the improvement before scoring.
- Check repository settings with the existing `scripts/apply-repo-settings.sh --check`, current Dependabot major updates, runner images, and named tool pins. Add a minimal grouped Dependabot configuration for GitHub Actions if evidence confirms the absent config is the weakest upgrade-path item; do not enable auto-merge.
- Refresh only documentation shown stale by code/settings changes, including README install guidance, device/size budgets, roadmap, and prompt-coverage records. Keep detailed evidence in phase artifacts and user-facing README concise.

### Execution and verification
- Before long builds, test suites, guest runs, or hosted workflows, run the applicable cheap inventory/count/selector/budget check. Reuse current CMake checks and existing presets instead of adding an overlapping gate.
- Repository-check code remains CMake `-P` or POSIX shell, within the existing phase budget. Keep code review before final verification, then ship and release this phase before Phase 8.

### the agent's Discretion

No separate discretion section was recorded in CONTEXT.md.

### Deferred Ideas (OUT OF SCOPE)

- Do not add owner hardware captures, full emulator accuracy campaigns, new CI service dependencies, general dashboards, or a separate report-generation platform in this tune-up.
- Keep the usual requirement for broad hosted CI on behavior/workflow changes. One-off documentation and planning edits use attributable existing evidence and do not justify another full suite.
- If BE32 remains the critical path after the redundancy analysis and no measured safe shortening is available, record that outcome and its constraint instead of removing the lane or claiming an unmeasured optimization.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| TUNE-01 | A scheduled weekly triage job creates a read-only artifact from open project issues and the latest run status; it does not post comments or messages, and it sanitizes untrusted issue/log text and removes private paths and identifiers before publication. | GitHub Issues and Actions APIs have separate read scopes; filter PR-shaped issue results; use a fixed metadata schema and short artifact retention. |
| TUNE-02 | A CI-efficiency report compares up to the latest 40 runs with matching workflow, matrix and workload fingerprints; it identifies critical-path jobs, duplicated work, flaky checks, core usage and caching evidence. Any optimization retains required checks and states its measured before/after result; if runs are not comparable, the report says so and does not infer a speedup. | Workflow-runs API exposes run metadata and supports paging/filtering; existing run-history evidence supports a specific release-metadata PR reduction without changing main or release checks. |
| TUNE-03 | The report records the slowest job and its run ID, reviews every flaky test by naming the stronger signal that replaces it or fixing the cause, and derives job timeouts as twice the slowest matching run with the evidence run ID beside each value. | Preserve attributable run links/IDs and classify only comparable workloads; existing timeout comments can be updated in place. |
| TUNE-04 | A linked 1–5 review covers performance, CI/CD, ease of use, architecture, documentation/README, upgrade path and compatibility breadth; the phase makes one evidence-backed improvement to the weakest dimension and verifies its stated result. | Context requires scoring first; existing run evidence identifies a metadata-only release-PR path as a potential CI/CD improvement. |
| TUNE-05 | `quality.json` records the existing Ir/frame, wasm size, serialized-state bytes and CI critical-path measurements, with CI enforcing checked-in ceilings/floors ratcheted to measured values; `policy.lessons_unique` rejects duplicate lesson identities or duplicate lesson text and has a negative control. | Existing CMake `string(JSON)` checks and negative-control style are suitable; no additional library is needed. |
| TUNE-06 | The phase checks repository settings, Dependabot majors, runner images and the existing tool pins; refreshes affected maintainer/hardware docs, README install guidance, size budgets, roadmap and prompt-coverage records; and records which items were unchanged with evidence. No pin, cache or runner change is made without a measured reason. | Use the existing settings checker and current workflow/pin inventory; evidence decides whether to add grouped Dependabot configuration. |
</phase_requirements>

## Summary

**Primary recommendation:** keep the workflow implementation in the existing POSIX shell, CMake `-P`, GitHub API, and `jq` patterns. Grant the weekly job read-only access to issues and Actions data, upload only the sanitized JSON report with the already pinned artifact action, and never pass untrusted title or label text through shell interpolation. The Actions permission model makes job-level scopes useful here: once scopes are explicitly set, unspecified scopes are none. [CITED: GitHub Actions workflow syntax](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax)

The Issues API returns both issues and pull requests, with a `pull_request` object identifying the latter. Filter those records before aggregation. Workflow-run listing requires Actions read permission for authenticated access, and exposes run metadata separately from logs; there is no need to fetch logs for the requested digest. [CITED: GitHub Issues API](https://docs.github.com/en/rest/issues/issues), [CITED: GitHub workflow-runs API](https://docs.github.com/en/rest/actions/workflow-runs)

The report artifact is visible to repository readers, so treat all its fields as public. Keep the fixed schema limited to issue numbers, bounded sanitized titles, allowlisted labels, dates, and workflow status with a run locator. Exclude bodies, logs, authors, branch names, commit SHAs, usernames, and local paths. A seven-day per-artifact expiry is supported by the upload action. [CITED: artifact access](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/download-workflow-artifacts), [CITED: upload-artifact retention](https://github.com/actions/upload-artifact#retention-period)

For CI time reduction, keep all post-merge main CI and release validation. The evidence supports only a narrow PR selector: a release PR changed generated metadata files only yet spent 60m21s in BE32. Add an opt-in selector to the existing classifier for pull requests and fail closed for missing, incomplete, renamed-outside-allowlist, oversized, or unrecognized path lists. Do not activate that selector for main pushes or release-input changes. This recommendation is an inference from the measured run and existing route tests, not a claim that release metadata changes can never affect a product build.

## Project Constraints (from AGENTS.md)

- Preserve the three deliverables (C library, headless runner, libretro core), deterministic integer-only core, and host-supplied files; this phase has no reason to alter emulator behavior.
- Reuse permissively licensed material only. Keep the repository free of BIOS, disc, executable, card, state, personal-path, email, and identity bytes outside the approved fixture manifest.
- Every behavior change needs a test in the CI workflow; all tests gate. Use automation first and no owner hardware verification for this internal CI phase.
- Repository-check code is only CMake `-P` or POSIX shell, bounded by the 3,300-line milestone ceiling and ratcheted to the measured total at ship. The total RetroArch test code remains no more than 300 lines. Name the fault each gate prevents.
- Review code before final verification. Keep code-review tooling findings as seeds, and do not introduce a third review-fix round without new evidence.
- Avoid new ceremony, privileged OS checks, or self-metered time limits. Keep changes within the authorized v2.0 scope and stop only at real owner/identity/phase boundaries.
- Commit phase work on its `phase/NN-slug` branch, never on `main`; preserve the existing uncommitted config edit. Record runtime/model in STATE at each GSD step.
- Evidence consists of a command, exit status, and run URL, written by the command from repository root. Each plan SUMMARY includes `requirements-completed` and a structured block-style `coverage` entry per requirement.
- Plan 1 establishes CMake wiring and per-directory test inventories; independent later plans may be parallelized, a plan over 30 minutes must be split, and no test is non-gating.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Weekly issue/run report | GitHub-hosted workflow | GitHub REST APIs, shell/JQ | The workflow owns schedule, token scope, artifact lifetime, and job result. The API supplies only project metadata. |
| CI path classification | POSIX shell classifier | CMake hygiene test | The classifier selects CI lanes; its marked workflow entry point and synthetic route tests validate wiring and fail-closed behavior. |
| Metric and lesson ratchets | CMake `-P` test | JSON/Markdown inputs | Existing CMake policy checks keep the gate dependency-light and integrate with the repository's CI preset. |
| Seven-dimension review and pin/settings audit | Phase report and maintainer docs | Existing settings and pin checks | This is a bounded evidence review, not a runtime dashboard or a new service. |

## Standard Stack

| Existing tool/pattern | Purpose | Recommendation |
|-----------------------|---------|----------------|
| GitHub Actions `GITHUB_TOKEN` scopes | Read only Issues and workflow-run data | Set job-level permissions to only `issues: read` and `actions: read`; add `contents: read` only for a checkout. Do not grant write scopes. |
| GitHub REST Issues and workflow-runs endpoints | Enumerate open issue metadata and recent run status | Request only the fields needed for the fixed schema. Filter `pull_request` records. Do not fetch logs or issue bodies. |
| POSIX shell and `jq` | Existing repo workflow data extraction and JSON construction | Keep untrusted strings as JSON values; use `jq` to encode output and scrub text. Never splice expression text into an inline shell program. |
| Pinned `actions/upload-artifact` already used in the repo | Publish the one generated JSON file | Reuse the existing SHA pin and fail if the output file is missing. Use `retention-days: 7`. |
| CMake `-P` and current CTest hygiene integration | Register and enforce selector, schema, and uniqueness controls | Extend the existing CMake/hygiene test seam and keep negative controls gating. |

**Installation:** None. Avoid new package/runtime dependencies. Existing workflow examples use `jq` and `gh`, and existing workflows upload artifacts without widening the declared read-only workflow token scopes. [VERIFIED: `.github/workflows/nightly.yml:44-45,78-82,213-216`]

## Architecture Patterns

### Triage artifact data flow

```text
schedule / workflow_dispatch
  -> one job with issues:read + actions:read (+ contents:read only if checkout)
  -> list open issues and workflow runs via API
  -> filter PRs; select allowlisted metadata; bound and sanitize titles
  -> emit fixed JSON using jq
  -> upload only that file with 7-day retention
```

Job-level permissions are preferable to relying on a repository default: GitHub documents that scopes omitted after specifying permissions are set to none. The issue-list API requires Issues read; the workflow-runs endpoint requires Actions read for private-repository token use. [CITED: GitHub workflow syntax](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax), [CITED: GitHub Issues API](https://docs.github.com/en/rest/issues/issues), [CITED: GitHub workflow-runs API](https://docs.github.com/en/rest/actions/workflow-runs)

The artifact should not include any payload that is only available in issue bodies or logs. Titles are untrusted data and must be treated as data, never evaluated by the runner. GitHub explicitly lists issue titles and labels among potentially untrusted contexts. [CITED: GitHub script-injection guidance](https://docs.github.com/en/actions/concepts/security/script-injections)

### Fail-closed release metadata selector

Reuse the existing changed-path classifier and production workflow test seam. Restrict fast classification to pull requests authored by app/schwaystation-release from the repository's release-please--branches--main branch targeting main, with matching fresh REST and event head SHA, and exactly the three generated metadata files as distinct ordinary modifications. A mismatched author, fork, rename, extra path, missing, stale, malformed or incomplete listing follows heavy CI. Keep release-input routing independent and preserve the main push route. [VERIFIED: PR #23 and existing classifier/test seam]

Existing regression tests extract and run the marked classifier shell block and exercise synthetic event/path routes. Extend those controls for the new PR-only branch instead of creating a second workflow simulator. The test must prove generated-only PR paths skip the heavy matrix, a mixed/unrecognized path does not, and a main push with the same paths stays heavy. [VERIFIED: `tests/hygiene/tests.cmake:183-228,305-340`]

## Don't Hand-Roll

| Problem | Avoid | Use instead | Why |
|---------|-------|-------------|-----|
| JSON quoting | `printf`-based hand-built JSON or shell string concatenation | `jq` object construction | Correctly preserves JSON structure for punctuation, control characters, and quotes. |
| Actions authorization | Broad workflow-level or write token scopes | Job-level least-privilege scopes | GitHub treats omitted scopes as none once specific scopes are declared. |
| Issue/PR classification | Assuming the Issues API returns only issues | Filter records carrying the API `pull_request` property | GitHub documents that issue endpoints can return both kinds. |
| CI route testing | A second copy of workflow classification logic | The current marked workflow shell seam and CMake hygiene tests | Tests exercise the production selector and its event wiring together. |
| Artifact retention | Keeping an unbounded report or custom storage service | Existing upload action with short per-artifact retention | Workflow artifact retention is configurable per artifact; repository readers can download it. |

## Common Pitfalls

1. **Untrusted text reaches shell syntax.** Do not interpolate event expressions or API titles/labels in `run:` source or shell variable declarations. Fetch data as JSON and pipe it to a structured `jq` transform. JSON encoding protects the JSON file but does not itself remove emails or local filesystem paths; apply the explicit scrub/bounds before output. [CITED: GitHub script-injection guidance](https://docs.github.com/en/actions/concepts/security/script-injections)
2. **Privacy cleanup is incomplete.** Omit issue bodies, logs, authors, branches, and SHAs entirely. Test titles containing ANSI/control characters, email addresses, absolute home paths, unusually long text, and JSON punctuation. Treat the final artifact as public to every repository reader. [CITED: GitHub artifact access](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/download-workflow-artifacts)
3. **Issue listing silently includes PRs.** Filter entries with `pull_request` before counting or serializing them. [CITED: GitHub Issues API](https://docs.github.com/en/rest/issues/issues)
4. **A selector skips main or release checks by accident.** The new fast path is opt-in only for pull requests. A route-test must prove the same metadata-only paths on a main push still go through heavy CI; release-input changes continue to set the rehearsal path.
5. **A smaller sample is reported as a speedup.** The efficiency report must expose the matching-run count and workload fingerprint; if the comparison is not like-for-like, label the result inconclusive.

## Code Examples

The existing repo uses `jq` for workflow summaries and issue/run metadata and declares read-only workflow permissions. For the weekly report, pass the API's JSON stream to a `jq` filter that selects the fixed fields, applies text substitutions and bounds, and emits a new JSON object. Keep the auth token in an environment variable, never in a generated script or report. The current Nightly workflow provides the in-repository `gh`/`jq` and artifact-upload patterns. [VERIFIED: `.github/workflows/nightly.yml:44-45,78-82,213-231`]

For APIs: the Issues endpoint returns open items by default and can return pull requests too; the workflow-runs endpoint lists runs and supports `per_page` up to 100. Request up to the required maximum sample, then group by workflow, matrix leg, and workload fingerprint before calculating comparisons. [CITED: GitHub Issues API](https://docs.github.com/en/rest/issues/issues), [CITED: GitHub workflow-runs API](https://docs.github.com/en/rest/actions/workflow-runs)

## Environment Availability

| Dependency | Required by | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| GitHub Actions and read-only REST APIs | Weekly digest and run-history evidence | Existing project workflows already use these APIs | N/A | None required for hosted artifact creation |
| `gh` and `jq` | Existing workflow query/report pattern | Used by the repo's current Nightly and CI workflows | Runner-managed | If unavailable on a new runner image, fail the short job clearly; do not add an install or dependency before confirming need |
| `actions/upload-artifact` | Publish short-lived digest | Already pinned and used by existing workflows | Repository pin | None; use the existing SHA-pinned action |
| `act` | Optional local workflow wiring preflight | Prior project evidence records scoped `act --dryrun` success for the `changes` job | Existing local install | CTest/hygiene selector checks remain authoritative locally |

**Missing dependencies with no fallback:** None identified.

## Security Domain

OWASP ASVS is written for web applications and web services, so its controls are only an analogy for this GitHub workflow. Relevant themes are authorization scoping, structured-input validation, and data protection/retention; the workflow-specific controls below are governed primarily by GitHub's own Actions documentation. [CITED: OWASP ASVS overview](https://asvs.owasp.org/)

| ASVS area | Applies | Workflow mapping |
|-----------|---------|------------------|
| V2 Validation and Business Logic | Yes, by analogy | Bound and sanitize issue titles/labels; enforce a fixed output schema. [CITED: OWASP ASVS V2](https://github.com/OWASP/ASVS/blob/master/5.0/en/0x11-V2-Validation-and-Business-Logic.md) |
| V8 Authorization | Yes, by analogy | Grant only issue and Actions read scopes; omitted token scopes are none. [CITED: OWASP ASVS V8](https://github.com/OWASP/ASVS/blob/master/5.0/en/0x17-V8-Authorization.md), [CITED: GitHub workflow syntax](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax) |
| V14 Data Protection | Yes, by analogy | Exclude private text and identifiers, expose only public metadata, and expire the artifact quickly. [CITED: OWASP ASVS V14](https://github.com/OWASP/ASVS/blob/master/5.0/en/0x23-V14-Data-Protection.md), [CITED: artifact access](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/download-workflow-artifacts) |
| Authentication/session/cryptography controls | No | No user login, app session, or cryptographic application protocol is added by this workflow. |

### Threat patterns

| Pattern | Threat | Control |
|---------|--------|---------|
| Issue title or label passed into a shell command | Script injection | Keep data out of inline script source; pass through JSON and `jq`, then encode output. |
| Broad `GITHUB_TOKEN` permissions | Unauthorized writes if a step/action is compromised | Explicit job-level read permissions only; no issue comments or messages. |
| Bodies, logs, usernames, branches, paths, or SHAs copied into artifact | Disclosure to repository readers | Fixed allowlist schema, omit high-risk fields, explicit path/email scrub, seven-day expiry. |
| Unexpected/partial changed-file API result | Required CI gets skipped | Fail closed to the existing heavy route and test event/path combinations. |

## Sources

### Primary sources

- [GitHub Actions workflow syntax](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax) — schedule and workflow dispatch, job-level token permissions, and omitted scopes.
- [GitHub REST Issues API](https://docs.github.com/en/rest/issues/issues) — list repository issues, the read permission required, and `pull_request` field filtering.
- [GitHub REST workflow-runs API](https://docs.github.com/en/rest/actions/workflow-runs) — list runs, read permission, and pagination parameters.
- [GitHub script-injection guidance](https://docs.github.com/en/actions/concepts/security/script-injections) — event fields such as titles and labels are untrusted inputs.
- [Downloading workflow artifacts](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/download-workflow-artifacts) and [`actions/upload-artifact`](https://github.com/actions/upload-artifact#retention-period) — repository-reader access and per-artifact retention.
- [OWASP ASVS 5.0](https://asvs.owasp.org/) — applicable security themes; used by analogy because the deliverable is a workflow, not a web application.

### In-repository patterns

- `.github/workflows/nightly.yml:44-45,78-82,213-231` — read-only top-level permission, SHA-pinned artifact upload, and existing `gh`/`jq` workflow patterns.
- `scripts/classify-changes.sh:1-38` and `.github/workflows/ci.yml:53-99,975-1002` — fail-closed path classification and required aggregator.
- `tests/hygiene/tests.cmake:176-228,305-340` — production workflow shell extraction and event/path route fixtures.
- `.planning/REQUIREMENTS.md:11-32` and `07-CONTEXT.md` — phase limits and locked scope.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Sanitized issue titles improve triage enough to justify their residual redaction risk. | Summary | If not, omit titles and retain issue numbers, allowlisted labels, and dates. |
| A2 | A PR-only generated-metadata skip is safe because post-merge CI and published release validation remain. | Architecture Patterns | If actual release generation consumes changed metadata in a product build, keep heavy CI on that path. |

## Resolved decisions

- **Required checks.** Product PR and main CI have similar long jobs. Keep required post-merge CI, actual Release workflow validation and release-input rehearsals. Release PR #23 identifies a narrower optimization. The App's generated-only PR can skip duplicate heavy work when its current author, branch, base, head SHA and exact three-file payload are corroborated by the REST API. Preserve ordinary PR, fork, uncertain-data and main push heavy behavior.
- **Artifact authorization.** Nightly already uploads through its SHA-pinned artifact action while using only read scopes. Reuse that upload mechanism for triage with the least required read scopes and grant no Actions write permission. Verify actual sanitized artifact upload and download at ship, when the workflow is on the default branch.

## Metadata

**Confidence breakdown:**
- GitHub Actions/API behavior: HIGH — official GitHub documentation checked.
- In-repository route and artifact patterns: HIGH — workflow and CMake hygiene sources opened.
- Generated-release-metadata optimization: HIGH for the measured waste, MEDIUM for the safety inference; enforce an exact fail-closed allowlist and retain independent post-merge/release validation.

**Research date:** 2026-11-18
**Valid until:** 2026-12-18
