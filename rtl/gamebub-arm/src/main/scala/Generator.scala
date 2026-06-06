import _root_.circt.stage.{ChiselStage, FirtoolOption, CIRCTTargetAnnotation, CIRCTTarget}
import chisel3.stage.ChiselGeneratorAnnotation
import gba.cpu.ARM7TDMI

/**
 * Emits readable, per-module SystemVerilog for the standalone ARM7TDMI core.
 *
 * Usage:
 *   sbt "runMain Generator --target-dir generated"
 * produces one .sv file per module in generated/ plus a filelist.f manifest.
 */
object Generator extends App {
  (new ChiselStage).execute(
    args :+ "--split-verilog",                         // one .sv per module into --target-dir (+ filelist.f)
    Seq(
      ChiselGeneratorAnnotation(() => new ARM7TDMI),
      CIRCTTargetAnnotation(CIRCTTarget.SystemVerilog),
      FirtoolOption("--strip-debug-info"),             // drop // src/...scala:NN:NN comments
      FirtoolOption("--disable-all-randomization"),    // drop RANDOMIZE/INIT_RANDOM preamble
      FirtoolOption("--preserve-values=named"),        // keep source names; fewer _GEN temporaries
      FirtoolOption("--lowering-options=disallowExpressionInliningInPorts,emittedLineLength=120"),
    ),
  )
}
