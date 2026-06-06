package gba.cpu

import chisel3._

/// A simple indexed 32-bit port for reading and writing CPU state, used to
/// implement save/restore (snapshots).
///
/// `address` selects a 32-bit word of state. `readData` reflects the addressed
/// word combinationally. Asserting `writeEnable` writes `writeData` into the
/// addressed word immediately (no commit step); multi-word registers are written
/// one word at a time via read-modify-write.
///
/// The address is a global 6-bit index at the top level. Submodules receive the
/// same bundle but interpret `address` in their own local 0-based space (the top
/// subtracts each submodule's base before routing).
class StatePort extends Bundle {
  val address = Input(UInt(6.W))
  val writeData = Input(UInt(32.W))
  val writeEnable = Input(Bool())
  val readData = Output(UInt(32.W))
}
