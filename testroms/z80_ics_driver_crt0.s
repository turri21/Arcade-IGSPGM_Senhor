        .module z80_ics_driver_crt0
        .globl _main
        .globl _z80_ics_isr
        .globl _z80_ics_nmi

        .area _HEADER (ABS)
        .org 0x0000
        jp _start

        .org 0x0038
        jp _z80_ics_isr

        .org 0x0066
        jp _z80_ics_nmi

        .area _CODE
_start:
        di
        ld sp,#0x6fff
        call _main
_halt:
        jr _halt
