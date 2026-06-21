derive_pll_clocks
derive_clock_uncertainty

# core specific constraints

# ARM7 memory-ready is a 2-cycle (multicycle) feedback path. The ARM only advances
# when arm_advance = arm_en & mem_ready, and the cache mem_ready cannot assert in the
# same cycle a new ALU-generated address appears: it gates on arm_addr_stable =
# (arm_addr == arm_addr_q), and arm_addr_q is the previous cycle's address (igs027a.sv).
# So the io_mem_ADDR -> cache tag lookup -> mem_ready -> arm_advance feedback always has
# at least two system-clock periods to resolve, regardless of the catch-up counter.
#
# mem_ready and arm_advance are marked (* synthesis keep *) in igs027a.sv so each
# survives as a single net; every register clock-enable / write-strobe in this feedback
# fans out from them. Target the NETS (get_nets), not pins: mem_ready is a net and the
# previous get_pins match found 0 objects, so it relaxed nothing.
set_multicycle_path -setup -end 2 -through [get_nets {*igs027a|mem_ready *igs027a|arm_advance}]
set_multicycle_path -hold  -end 1 -through [get_nets {*igs027a|mem_ready *igs027a|arm_advance}]

# The rest of the feedback fans out far beyond mem_ready: the ARM's combinational
# address (io_mem_ADDR) drives the shared prot_cache/share-cache tag+miss FSMs, the
# share-RAM savestate readback (sh_state, ssbus_share), the alternate protection chip
# igs022 (cache shared, idle when the ARM runs), and even the 68k ce (deferred on
# igs027a_share_ready). Every one of these is gated by the same addr_stable settle (or
# is only live while the ARM is frozen / its chip idle), so the whole ARM -> rest-of-
# core cone is >= 2 cycles. A -through on the address is unreliable (synthesis
# duplicates the bus); scope it structurally instead: from the ARM, to everything in
# the core except the ARM itself (the register-file writeback stays single-cycle and is
# not in this set).
set _arm_regs [get_keepers {*ARM7TDMI:arm|*}]
set _core_nonarm [remove_from_collection [get_keepers {*pgm_inst|*}] $_arm_regs]
set_multicycle_path -setup -end 2 -from $_arm_regs -to $_core_nonarm
set_multicycle_path -hold  -end 1 -from $_arm_regs -to $_core_nonarm
