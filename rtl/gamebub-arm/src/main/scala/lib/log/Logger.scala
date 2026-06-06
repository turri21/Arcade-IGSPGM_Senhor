package lib.log

/**
 * No-op logging stub.
 *
 * The upstream gamebub project uses a Chisel `Logger` that emits simulation
 * `printf`s. For the standalone ARM7TDMI core we replace it with a no-op that
 * keeps the same public API, so existing `logger.debug(cf"...")` call sites in
 * the core compile unchanged while producing no `$fwrite`/printf blocks in the
 * generated Verilog.
 */
object Logger {
  def apply(module: String): Logger = new Logger
  def apply(module: String, enable: chisel3.Bool): Logger = new Logger
}

class Logger {
  def crit(log: chisel3.Printable): Unit = ()
  def error(log: chisel3.Printable): Unit = ()
  def warn(log: chisel3.Printable): Unit = ()
  def info(log: chisel3.Printable): Unit = ()
  def debug(log: chisel3.Printable): Unit = ()
}
