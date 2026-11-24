# Controllers and multitap

The libretro core accepts the standard DualShock (ANALOG), PlayStation Controller (JOYPAD), and None devices.
Its two multitap options are disabled by default and take effect when content is loaded or reloaded:

| Core option | Values | Default | Effect |
| --- | --- | --- | --- |
| `schwaystation_multitap_port1` | `disabled`, `enabled` | `disabled` | Add sockets B–D to physical controller port 1. |
| `schwaystation_multitap_port2` | `disabled`, `enabled` | `disabled` | Add sockets B–D to physical controller port 2. |

With a tap enabled on a physical port, frontend players are assigned in compact order. The core keeps each
player attached to that physical endpoint; disabling one tap does not shift the other port's socket A.

| Port 1 tap | Port 2 tap | Frontend player mapping | Active players |
| --- | --- | --- | ---: |
| Disabled | Disabled | 1 → port 1A; 2 → port 2A | 2 |
| Enabled | Disabled | 1–4 → port 1A–D; 5 → port 2A | 5 |
| Disabled | Enabled | 1 → port 1A; 2–5 → port 2A–D | 5 |
| Enabled | Enabled | 1–4 → port 1A–D; 5–8 → port 2A–D | 8 |

Libretro reports descriptors for the active players in that order. `retro_run` polls frontend input once, then
delivers each player's snapshot to its assigned controller endpoint before advancing the emulated frame.
