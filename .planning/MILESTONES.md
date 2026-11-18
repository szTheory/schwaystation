# Project Milestones: schwaystation

## v1.0 PlayStation games play with sound in RetroArch (Shipped: 2026-11-18)

**Delivered:** A deterministic, permissively licensed PlayStation library, runner and libretro core with CPU, GTE,
GPU, SPU, CD-ROM, XA/CD-DA, MDEC and disk-control paths, plus an owned disc that boots through OpenBIOS in
RetroArch.

**Phases completed:** 1–6 (195 plans, 381 tasks)

**Key accomplishments:**
- Shipped the three deliverables and an automated release pipeline across six platforms, plus big-endian and wasm32 lanes.
- Matched the public R3000A vectors and 1,150 hardware GTE cases; integrated DMA and timer behavior.
- Added GPU rendering, display modes, digital and DualShock input, memory cards, OpenBIOS and a compatibility ledger.
- Matched pinned SPU memory-transfer logs and verified deterministic, cross-platform PCM parity between runner and libretro.
- Added bounded cue/bin, ISO and M3U media paths, XA and CD-DA playback, MDEC decoding and disk control; booted the owned `disc_boot` fixture in RetroArch.
- Audited all 30 requirements, six phase verifications and seven cross-phase flows; no critical requirement, integration or user-flow gap remains.

**Phase releases:** [PR #4](https://github.com/szTheory/schwaystation/pull/4), [PR #8](https://github.com/szTheory/schwaystation/pull/8), [PR #12](https://github.com/szTheory/schwaystation/pull/12), [PR #14](https://github.com/szTheory/schwaystation/pull/14), [PR #18](https://github.com/szTheory/schwaystation/pull/18), [PR #22](https://github.com/szTheory/schwaystation/pull/22).

**Package release:** [Release PR #23](https://github.com/szTheory/schwaystation/pull/23) published v0.6.0. The GSD v1.0 milestone is not a semver v1.0 package release; no v1.0 tag was configured.

**Hosted evidence:** [All-platform release-PR CI](https://github.com/szTheory/schwaystation/actions/runs/44253419074), [full reference-vector job](https://github.com/szTheory/schwaystation/actions/runs/44233700921/jobs/134564625732), and [v0.6.0 release workflow](https://github.com/szTheory/schwaystation/actions/runs/44262491280). Exact-source hosted results were reused for the planning-only closeout; no local long suite was rerun.

**Closeout type:** `override_closeout`.

**Known verification overrides:** 42 newly acknowledged, 0 carried forward (see [STATE.md Deferred Items](STATE.md#deferred-items)). These are 29 pending todos, 11 dormant future seeds and two documented deferred items; acknowledgment records deferral and does not resolve them.

**Technical debt and limits:** Exact 8-bit MDEC hardware output and complete external frame/movie conformance are not claimed. The pinned 4-bit guest, a separate deterministic 8-bit regression, direct controls, complete compressed-source preflights and the owned STR path are verified. The audit records other non-blocking accuracy and tooling limits.

**What's next:** No next milestone is active. Future directions remain in `.planning/PRODUCT-ROADMAP.md` and the seed ledger; this closeout does not start v2.0.

---
