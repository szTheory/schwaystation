# Existing CMake and install skeleton

Captured during Plan 17-01 against the Phase 17 branch. The registration structure already satisfies the phase
shape; this plan records it and corrects one stale budget comment without adding a test scaffold or gate.

## Registration path

`CMakeLists.txt` includes `cmake/SchwaystationTests.cmake` and then `tests/tests.cmake`. The latter includes the
selected `tests/<directory>/tests.cmake` files into the top-level directory. `SchwaystationTests.cmake` defers a
check until those includes finish, then compares each registered test name and sorted label set with the union
selected by `tests/inventory/<SCHWAYSTATION_INVENTORY>.txt` and the per-directory `inventory.txt` files.

The hygiene preset selects only `tests/hygiene/tests.cmake`; the regular CI inventory also includes
`tests/policy/tests.cmake`. Their current exact inventory identities are:

### `tests/hygiene/inventory.txt` → `tests/hygiene/tests.cmake`

24 registered rows; the sorted inventory is the exact test-name/label set:

```text
ci.classify	hygiene
ci.title	hygiene
ci.title.self_test	hygiene
hygiene.no_bios_bytes.self_test	hygiene
hygiene.no_float	hygiene
hygiene.no_float.self_test	hygiene
hygiene.pre_push_scope	hygiene
hygiene.timeout_comments	hygiene
hygiene.timeout_comments.self_test	hygiene
hygiene.tree	hygiene
hygiene.tree.self_test	hygiene
policy.action_pins	hygiene,policy
policy.action_pins.self_test	hygiene,policy
policy.e2e_budget	hygiene,policy
policy.e2e_budget.self_test	hygiene,policy
policy.gate_budget	hygiene,policy
policy.gate_budget.self_test	hygiene,policy
policy.media_registration	hygiene,policy
policy.notices	hygiene,policy
policy.notices.self_test	hygiene,policy
policy.publish_ref	hygiene,policy
policy.publish_set	hygiene,policy
policy.publish_set.self_test	hygiene,policy
policy.publish_signer	hygiene,policy
```

### `tests/policy/inventory.txt` → `tests/policy/tests.cmake`

12 registered rows; the sorted inventory is the exact test-name/label set:

```text
ci.classify_policy	policy
ci.triage	policy
docs.api_names	policy,portable
docs.api_names.self_test	policy,portable
docs.budgets	policy
policy.fixture_budget	policy,portable
policy.lessons_unique	policy
policy.lessons_unique.self_test	policy
policy.no_skip	policy
policy.no_skip.self_test	policy
policy.pass_regex_status.self_test	policy,portable
policy.quality_metrics	policy
```

The one-off inventory preflight sorted both files in `LC_ALL=C` and compared their registered test names with
the corresponding `schway_add_test` names; it exited 0. Label equality is enforced by the existing deferred CMake
inventory check above. Both inventories were left byte-for-byte unchanged.

## Existing gate budgets

The repository-check ceiling remains 4,694 lines for `scripts/*.sh`, `tests/cmake/*.cmake` and `.githooks/*`;
the existing `tests/cmake/line_budget.cmake` command measured 4,694 and exited 0. The RetroArch end-to-end
ceiling remains 300 lines; the same producer measured 300 and exited 0. The hygiene test comment now names the
Phase 17 ceiling. The CMake limits and test registrations did not change.

## Released README installer evidence

The latest public release during this check was `v0.16.0`, published 2026-11-27. It contains 18 zip assets and
`SHA256SUMS`. The successful release workflow is
[run 45923155197](https://github.com/szTheory/schwaystation/actions/runs/45923155197) on commit
`a2f37b46eab24f53e30f99c6b3faff6eedc8f2f8`.

The README installer was executed with a task-owned temporary directory and isolated `HOME`:

```sh
TMPDIR=/private/tmp/schwaystation-plan17-01 cmake -DREADME=README.md -DBASE_URL=https://github.com/szTheory/schwaystation/releases/latest/download/ -DHOME_DIR=/private/tmp/schwaystation-plan17-01/home-installed -P tests/cmake/install_line.cmake
```

Exit status: 0. The installed files were `Library/Application Support/RetroArch/cores/schwaystation_libretro.dylib`
and `Library/Application Support/RetroArch/info/schwaystation_libretro.info`. The info file reports version `0.16.0`
and `is_experimental = "true"`.

The release archive and checksum list were downloaded to the same temporary directory, and the checksum list
passed `shasum -a 256 -c --ignore-missing`. The existing flipped-byte control then exited 0 after the README
installer rejected the altered archive with exit 1 and installed no core:

```sh
TMPDIR=/private/tmp/schwaystation-plan17-01 cmake -DREADME=README.md -DBASE_URL=file:///private/tmp/schwaystation-plan17-01/download/ -DFLIP=1 -DHOME_DIR=/private/tmp/schwaystation-plan17-01/home-tampered -P tests/cmake/install_line.cmake
```

The release identity and run URL identify the tested public artifact; the temporary archive, installed core and
logs are disposable and are not project fixtures.
