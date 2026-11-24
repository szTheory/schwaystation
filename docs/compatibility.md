# Compatibility results

Generated from `tests/compat/ledger.csv` and `tests/compat/features.csv`.

## Counts

| Status | Region | Media | Rows |
| --- | --- | --- | ---: |
| pass | ntsc-u | bios | 1 |
| pass | ntsc-u | exe | 3 |

## Results

| Title | Serial | Status | Predicate | Confidence | Evidence | Date |
| --- | --- | --- | --- | --- | --- | --- |
| OpenBIOS shell | fixture/openbios-shell | pass | frame@240=df2704d7cf3f01ff70ab4534a443729bba77bf73e3488c836513f9f8b86a9372 | measured fixture | built.openbios.shell:exit=0;bios_sha256=92c2f6d5e6ff056bf2ba177f37246e5468b6164337935297cf7b22266af20cc8;core=5cc1dc2 | 2026-11-13 |
| gpu_motion | fixture/gpu_motion | pass | frame@60=20c3dcac8ba7b17ac4a6fd611164d1ca2c1aab03419f72c9ef58f04fe6d47042 | measured fixture | fixtures.gpu_motion.hash.60:exit=0;core=59ad2c5d589b | 2026-11-11 |
| memcard_roundtrip | fixture/memcard_roundtrip | pass | frame@120=cd1c037ef800397a9c95eb23ebc5524710f55db2eca8e03c975f841f0e9232db | measured fixture | fixtures.memcard_roundtrip.persistence:exit=0;core=59ad2c5d589b | 2026-11-11 |
| pad_counter | fixture/pad_counter | pass | frame@300=cfcae1a726602580ac1741eb597c951cd3977f8f96a8df6be30c9ea50d9543b4 | measured fixture | fixtures.pad_counter.press60:exit=0;core=59ad2c5d589b | 2026-11-11 |

## Feature evidence

The key is provider, profile, protection, peripheral and predicate. Every field is required; unknown is valid until fresh evidence exists. A pass must match the fresh result status and evidence reference. Prior passes cannot regress or disappear without a matching dated allowance.
Phase 10 onward adds provider/profile/protection/peripheral rows as each capability lands. A profile tuple describes only a tested profile-selection predicate; it does not establish title boot/play or hardware conformance. Feature allowances use `<date> <provider> <profile> <protection>!<peripheral>!<predicate> behavior-rev=4 expires=<date> <reason>` and may cover at most 60 days.

| Provider | Profile | Protection | Peripheral | Predicate | Status | Evidence | Date |
| --- | --- | --- | --- | --- | --- | --- | --- |
| ccd | ntsc-u | none | cdrom | disc_boot.mixed.64data+158audio+toc+indexes+pregap+subcode+guest.boot+read+audio+fmv | pass | disc.image.ccd.exit0;disc.image.legacy_parity.ccd.222x1+sub.21312.exit0;runner.disc.legacy.exit0;libretro.content.legacy.exit0;disc.fixture.ccd.boot+read+audio+fmv.exit0;package.chd_off.archive_contents+notices.exit0;example.sdl2.staged.compile.exit0;build.chd_on.runner+libretro+ecm+ccd.exit0;disc.image.ecm.exit0;disc.image.ccd.exit0;libretro.info.claims.exit0;install.consumer.exit0;hosted_main.unknown;hosted_nightly.unknown;hosted_release.unknown;fuzz.unknown | 2026-11-22 |
| ccd | ntsc-u | none | cdrom | sbi.selected_provider+bad_q_data+previous_good_getlocp | pass | disc.image.sbi.provider_binding.exit0;cdrom.sbi.selected_child_bad_q.exit0 | 2026-11-23 |
| chd | ntsc-u | none | cdrom | disc_boot.toc+sectors+subcode | pass | provider.disc.image.chd_parity.222x2;runner.disc.chd.exit0;libretro.content.chd.exit0;release.archive_contents+notices.chd_on_off;disc.fixture.chd.boot+read+audio+fmv.exit0 | 2026-11-20 |
| core | ntsc-j.pm41-2.v2 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | ntsc-j.pm41.v2 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | ntsc-j.pu18.v2 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | ntsc-j.pu20.v2 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | ntsc-j.pu22.v2 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | ntsc-j.pu23.v2 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | ntsc-j.pu7.v0 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | ntsc-j.pu8-early.v0 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | ntsc-j.pu8-late.v2 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | ntsc-u.pm41-2.v2 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | ntsc-u.pm41.v2 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | ntsc-u.pu18.v2 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | ntsc-u.pu20.v2 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | ntsc-u.pu22.v2 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | ntsc-u.pu23.v2 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | ntsc-u.pu7.v0 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | ntsc-u.pu8-early.v0 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | ntsc-u.pu8-late.v2 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | pal.pm41-2.v2 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | pal.pm41.v2 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | pal.pu18.v2 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | pal.pu20.v2 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | pal.pu22.v2 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | pal.pu23.v2 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | pal.pu7.v0 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | pal.pu8-early.v0 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| core | pal.pu8-late.v2 | none | none | profile.select | pass | core.profile.machine.exit0;libretro.options.profile.exit0 | 2026-11-23 |
| ecm | ntsc-u | none | cdrom | disc_boot.one_data_track.64x1+toc+sectors+guest.boot+read+fmv | pass | disc.image.ecm.literal+modeled.exit0;disc.image.legacy_parity.ecm.64x1.exit0;runner.disc.legacy.exit0;libretro.content.legacy.exit0;disc.fixture.ecm.boot+read+fmv.exit0;package.chd_off.archive_contents+notices.exit0;example.sdl2.staged.compile.exit0;build.chd_on.runner+libretro+ecm+ccd.exit0;disc.image.ecm.exit0;disc.image.ccd.exit0;libretro.info.claims.exit0;install.consumer.exit0;hosted_main.unknown;hosted_nightly.unknown;hosted_release.unknown;fuzz.unknown | 2026-11-22 |
| pbp | ntsc-u | none | cdrom | disc_boot.one_data_track+toc+sectors+subcode+guest.boot+read+fmv_first_frame | pass | provider.disc.image.pbp_parity.64x1.exit0;runner.disc.pbp.exit0;libretro.content.pbp.exit0;release.archive_contents+notices.chd_on_off;disc.fixture.pbp.boot+read+fmv.exit0;hosted_ci.not_run | 2026-11-21 |
