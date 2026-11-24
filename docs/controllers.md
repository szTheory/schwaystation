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

## Special controllers

The core accepts these exact controller IDs through `retro_set_controller_port_device`. The three analog
subclasses use the vendored libretro formula `RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG, id)`, which yields
`0x205` for flight mode, `0x305` for neGcon, and `0x405` for Jogcon. The mouse uses the base
`RETRO_DEVICE_MOUSE` ID. Other subclasses are ignored.

| Device | Frontend controls and serial mapping |
| --- | --- |
| Dual Analog flight stick | Keeps the DualShock button mapping. Right-stick X/Y map to RX/RY and left-stick X/Y map to LX/LY with the existing signed-to-byte conversion, centered at 128. |
| Mouse | Mouse X/Y are relative signed values and left/right buttons map to the PS1 mouse buttons. The adapter preserves each signed callback value; the core saturates movement to the device's −128 to +127 report range. A missing callback supplies zero movement and released buttons. |
| neGcon | RetroPad Start, D-pad, R shoulder, B face button, and A face button map to the neGcon button word. Left-stick X maps linearly across 0–255 to twist. The positive side of right-stick Y, inverted, drives I pressure; the positive side of right-stick X drives II pressure. L2 drives L as 0 or 255. Pressure inputs have no extra curve or deadzone. |
| Jogcon | Uses the neGcon button mapping. Left-stick X supplies the raw signed dial word: its 16-bit pattern is retained with bit 0 cleared, and a negative or positive value sets the corresponding direction flag. Zero clears both flags. No force-feedback behavior is inferred. |

These mappings are host controls chosen for remapping convenience. The emulated core receives integer values and
does not apply additional curves, deadzones, or physical-dial response models.
