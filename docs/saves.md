# Memory card filenames and persistence

The host card path helper builds one filename for a one-based physical port and socket coordinate. It appends one
of these suffixes to the already selected content basename, serial or validated media identity:

| Physical port | Socket A | Socket B | Socket C | Socket D |
|---|---|---|---|---|
| 1 | `.srm` | `.p1b.mcr` | `.p1c.mcr` | `.p1d.mcr` |
| 2 | `.1.mcr` | `.p2b.mcr` | `.p2c.mcr` | `.p2d.mcr` |

For example, basename `game` produces `game.srm` for port 1/socket A, `game.1.mcr` for port 2/socket A, and
`game.p2c.mcr` for port 2/socket C. The physical port numbers and socket letters are stable whether a multitap is
enabled or disabled. The legacy A paths stay unchanged, and the host does not add a sharing option. Controller
and multitap socket selection is described in [controllers.md](controllers.md).

Runner naming may use the content basename or its existing serial identity policy. Libretro continues to use the
content basename; the frontend owns port 1/socket A through `RETRO_MEMORY_SAVE_RAM`, and the adapter owns the
port 2/socket A sidecar. The B–D suffixes define stable names for additional persisted sockets; their loading and
flushing remains the responsibility of the corresponding adapter.

Card images and generation counters are persistent data, not ordinary save-state fields. Loading a save state
restores in-flight card protocol progress but does not roll back the receiving instance's card bytes or generation.
A completed guest Write changes the addressed card sector and generation. A host save replaces a complete image
through an exclusive sibling temporary and atomic rename; a failed flush or replacement preserves the previous
file. The core itself never opens card files.
