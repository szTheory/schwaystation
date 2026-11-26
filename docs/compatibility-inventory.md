# Local library inventory

`schwaystation-run --library-inventory DAT` parses a Sony PlayStation DAT and prints public aggregate counts. It counts every game occurrence, including entries with no serial and every occurrence of a repeated serial. `duplicate_serial_entries` counts all entries belonging to repeated-serial groups. These counts describe the DAT only; they do not establish that a title, disc, or console is playable.

Use the pinned library source from `.planning/preparation/DECISIONS.md` (D-06) to reproduce the project inventory. The runner reports the source as pinned only when the input file's SHA-256 matches the pinned DAT digest; other files are labeled `unverified`.

```sh
cmake --build --preset ci --target schwaystation-run schwaystation-test-library
ctest --preset ci --no-tests=error -R '^(compat[.]inventory[.]accounting|compat[.]inventory[.]controls)$' --output-on-failure
build/ci/schwaystation-run --library-inventory 'path/to/Sony - PlayStation.dat'
```

The project does not redistribute the DAT or a generated title table. Keep the source file local; public output contains only the source digest and aggregate counts.

## Private occurrence report

To make per-occurrence evidence joins, pass `--library-inventory-output PATH`. This writes CSV columns `ordinal,entry_sha256,content_sha256,status,predicate,profile,evidence`. Ordinals preserve DAT order. `entry_sha256` identifies the raw game record; `content_sha256` fingerprints its ordered direct ROM/DISK definitions. Neither column contains a title or serial. The report is written atomically, and paths containing a `..` component are rejected. Pass `--library-evidence PATH` to join exact evidence into this private report.

The parser bounds the DAT at 32 MiB, evidence at 4 MiB, tokens and evidence lines at 8 KiB, nesting at 64 levels, and game occurrences at 32,768. Malformed or over-budget inputs fail without replacing an existing report.

## Evidence input

`--library-evidence PATH` accepts this exact CSV header:

```text
source_sha256,ordinal,entry_sha256,content_sha256,predicate,profile,status,evidence
```

Each row joins to a single source digest and ordinal and must match the entry and content fingerprints. Ordinals cannot repeat, and a file must use one predicate and profile. Status is `pass` or `fail`; an occurrence without a matching row remains `unknown`. A pass/fail result includes the evidence label and a digest of the evidence file, and the command exits nonzero when any supplied result fails. Duplicate, malformed, ambiguous, or mismatched evidence is rejected. Evidence is a recorded external result; the inventory tool does not open or validate disc media.

Evidence should be produced by a named reproducible check and kept outside the repository when it would contain restricted or personal data. The source, occurrence, content fingerprint, predicate, profile, status, and evidence label together make the join explicit without publishing title or serial strings.
