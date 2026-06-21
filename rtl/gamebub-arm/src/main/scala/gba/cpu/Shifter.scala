package gba.cpu

import chisel3._
import chisel3.util._

object ShiftKind extends ChiselEnum {
  val LogicalShiftLeft = Value
  val LogicalShiftRight = Value
  val ArithmeticShiftRight = Value
  val RotateRight = Value
  val RotateRightWithExtend = Value
}

class Shifter extends Module {
  val io = IO(new Bundle {
    val in = Input(UInt(32.W))
    val shiftKind = Input(ShiftKind())
    val shiftAmount = Input(UInt(6.W))

    val out = Output(UInt(32.W))

    val carryIn = Input(Bool())
    val carryOut = Output(Bool())

    /// Used for data processing with register shift. If set, will latch the bottom 8-bits
    /// of the operand, allowing it to be used as the shiftAmount in another cycle.
    val latchShift = Input(Bool())
    /// Used for data processing with register shift. If set, uses the latched shift amount.
    val useLatchedShift = Input(Bool())

    /// Save/restore state port (local word 0: latchedShift)
    val state = new StatePort
  })

  // Shift latching for shift-by-register
  val latchedShift = Reg(UInt(8.W))
  when (io.latchShift) {
    latchedShift := io.in
  }

  // Save/restore: word 0 is latchedShift. The write path is independent of the
  // latch above (the CPU is frozen during restore), and is placed last so it wins.
  io.state.readData := latchedShift
  when (io.state.writeEnable && io.state.address === 0.U) {
    latchedShift := io.state.writeData(7, 0)
  }
  val operand = io.in
  val amt = Mux(io.useLatchedShift, latchedShift, io.shiftAmount)   // 0..255 (8b) reg / 0..63 imm
  val k = amt(4, 0)                                                  // mod-32 amount

  val lsl = io.shiftKind === ShiftKind.LogicalShiftLeft
  val asr = io.shiftKind === ShiftKind.ArithmeticShiftRight
  val ror = io.shiftKind === ShiftKind.RotateRight
  val rrx = io.shiftKind === ShiftKind.RotateRightWithExtend
  val big = amt >= 32.U                                              // out-of-range (LSL/LSR/ASR)

  // One funnel right-shifter serves LSL/LSR/ASR. The selects (kind, amount) are
  // early; only `operand` is late, so it traverses a single barrel network instead
  // of four shifters + boundary muxes + a kind mux in series.
  //   LSL: right-shift the *bit-reversed* operand, reverse the result (fill 0).
  //   LSR: right-shift operand, fill 0.
  //   ASR: right-shift operand, fill the funnel's high half with the sign bit.
  val src    = Mux(lsl, Reverse(operand), operand)
  val hiFill = Fill(32, asr & operand(31))
  val funnel = (Cat(hiFill, src) >> k)(31, 0)
  val shifted = Mux(lsl, Reverse(funnel), funnel)

  // ROR is a direct rotate (valid for any amount, mod 32); RRX rotates right
  // through carry by one; out-of-range arithmetic shift saturates to the sign.
  val rored  = operand.rotateRight(k)
  val rrxOut = Cat(io.carryIn, operand(31, 1))
  val asrBig = Fill(32, operand(31))

  io.out := Mux(rrx, rrxOut,
            Mux(ror, rored,
            Mux(big, Mux(asr, asrBig, 0.U),   // LSL/LSR overflow -> 0, ASR -> sign
                     shifted)))

  // Carry-out = a single late operand bit chosen by (kind, amount); off the
  // critical `out` leg. Mirrors the per-kind boundary rules of the original.
  io.carryOut := Mux(rrx, operand(0),
    Mux(amt === 0.U, io.carryIn,
    Mux(ror, Mux(k === 0.U, operand(31), operand(k - 1.U)),
    Mux(lsl, Mux(amt < 32.U, operand((32.U - amt)(4, 0)),
                 Mux(amt === 32.U, operand(0), false.B)),
    // LSR / ASR: in-range -> operand[amt-1]; LSR overflow -> (==32 ? bit31 : 0),
    // ASR overflow -> bit31.
    Mux(amt < 32.U, operand((amt - 1.U)(4, 0)),
        Mux(asr, operand(31), Mux(amt === 32.U, operand(31), false.B)))))))
}
