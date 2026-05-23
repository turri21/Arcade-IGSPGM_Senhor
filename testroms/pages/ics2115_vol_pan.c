#include <stddef.h>

#include "../system.h"
#include "../page.h"
#include "../tilemap.h"
#include "../igs023.h"
#include "../color.h"
#include "../input.h"
#include "../gui.h"
#include "../z80_ics_host.h"

static u16 value;
static u16 result;
static u16 driver_magic;
static u16 op_count;
static z80_ics_voice_t voice;

static const char *status_name(u16 status)
{
    switch (status)
    {
    case Z80_ICS_STATUS_READY: return "READY";
    case Z80_ICS_STATUS_BUSY:  return "BUSY ";
    case Z80_ICS_STATUS_DONE:  return "DONE ";
    case Z80_ICS_STATUS_ERROR: return "ERROR";
    default: return "-----";
    }
}

static void init(void)
{
    igs023_init();
    text_reset();
    set_default_palette();
    input_init();

    IGS023_BG_CTRL_SET(0xffff);
    IGS023_FG_X_SET(8);
    IGS023_FG_Y_SET(8);

    driver_magic = 0;
    op_count = 0;

    z80_ics_init();
    if (z80_ics_ping(&driver_magic))
        op_count++;

    voice.vol_acc = 0xffff;
    voice.pan = 0x7f;
}

static void update(void)
{
    igs023_wait_vblank();
    input_update();

    text_color(1);
    text_cursor(2, 2);
    text("Z80 ICS DEBUG\n");
    textf("DRV %04X RDY %04X SEQ %02X\n", driver_magic, z80_ics_ready(), z80_ics_last_seq());
    textf("STAT %s ERR %04X OPS %04X\n", status_name(z80_ics_last_status()), z80_ics_last_error(), op_count);

    gui_begin(2, 8);
    u16 v = voice.vol_acc;
    if(gui_u16("VOL", &v))
    {
        voice.vol_acc = v;
        if(z80_ics_write_reg(0, 0x9, Z80_ICS_WIDTH_16, voice.vol_acc)) op_count++;
    
    }
    
    if(gui_u8("PAN", &voice.pan, 0, 0xff))
    {
        if(z80_ics_write_reg(0, 0x9, Z80_ICS_WIDTH_UPPER8, voice.pan)) op_count++;
    }

    if (gui_button("PLAY"))
    {
        set_osc_acc(&voice, 0x75750);
        voice.osc_start_hi = 0;
        voice.osc_end_hi = 0xffff;
        voice.osc_fc = 0;
        voice.vol_incr = 0;
        voice.vol_ctrl = 0x3;
        voice.osc_ctl = 0;
        voice.osc_conf = 0x08;

        if (z80_ics_write_voice(0, &voice))
            op_count++;
    }
    gui_end();
}

PAGE_REGISTER(ics2115_vol_pan, init, update, NULL);
