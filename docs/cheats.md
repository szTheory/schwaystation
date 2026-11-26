# GameShark cheats

The shared host parser accepts only unconditional constant writes in this form:

```text
TTAAAAAA DDDD
```

`TT` is `30` for an 8-bit write or `80` for a 16-bit write. `AAAAAA` is a six-digit physical offset in the
first 2 MiB of main RAM (`000000` through `1FFFFF`); values outside that range are rejected instead of being
masked or wrapped. `DDDD` is the 16-bit value. A `30` write requires its high byte to be zero. An `80` write
requires an even offset.

Hexadecimal digits may use either case. ASCII whitespace (space, tab, vertical tab, form feed, CR, or LF) and
`+` separate the code and value and separate successive writes. For example, the following two spellings
normalize to the same ordered writes:

```text
30000010 00A5
80000012 1234
```

```text
30000010+00a5+80000012+1234
```

Comments, conditional or stateful types, unsupported types, incomplete pairs, extra digits, empty text, and
invalid final writes reject the entire replacement. The old entry remains unchanged. Parsing never writes RAM;
the host applies an accepted table through the checked `schway_write_ram` operation at its frame boundary.

The shared table has 32 indexed entries, 64 writes per entry, 1,024 input bytes per entry, and 256 normalized
writes total across all entries. Its deterministic order is ascending entry index, then text order. Overlapping
writes therefore use last-write-wins behavior. Disabled entries are retained but skipped during application;
disabling an entry does not undo writes already made to RAM. Reset clears every entry and its enabled state.
