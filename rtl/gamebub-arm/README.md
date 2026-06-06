# gamebub-arm

A standalone, synthesizable **ARM7TDMI-S** CPU core, written in
[Chisel](https://github.com/chipsalliance/chisel) and packaged so it can be
re-used in other designs as generated SystemVerilog.

The core was extracted from the [Game Bub](https://www.crowdsupply.com/second-bedroom/game-bub)
FPGA project (`fpga/src/main/scala/gba/cpu`), where it powers the Game Boy
Advance emulation core. This repository contains only the CPU-relevant Chisel
source, a generator that emits Verilog, and the CPU test suite — nothing
GBA- or FPGA-platform-specific.

## License

GPL-3.0-only. This core is a derivative of the Game Bub FPGA source, which is
licensed under GPLv3, and therefore inherits that license. See `LICENSE`.

Original project: Game Bub by Eli Lipsitz — <https://eli.lipsitz.net/posts/introducing-gamebub/>
