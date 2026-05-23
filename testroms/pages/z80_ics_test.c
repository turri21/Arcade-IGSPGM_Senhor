#include <stddef.h>

#include "../system.h"
#include "../page.h"
#include "../tilemap.h"
#include "../igs023.h"
#include "../color.h"
#include "../input.h"
#include "../gui.h"
#include "../z80_ics_host.h"

static u16 selected_voice;
static u16 selected_reg;
static u16 selected_width;
static u16 value;
static u16 result;
static u16 driver_magic;
static u16 op_count;
static z80_ics_voice_t voice_shadow;
static z80_ics_irq_counts_t irq_counts;

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

static u8 width_value(void)
{
    if (selected_width > Z80_ICS_WIDTH_LOWER8)
        selected_width = Z80_ICS_WIDTH_16;
    return (u8)selected_width;
}

static void clamp_fields(void)
{
    selected_voice &= 0x1f;
    selected_reg &= 0xff;
    if (selected_width > 2)
        selected_width = 0;
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

    selected_voice = 0;
    selected_reg = 0x4c;
    selected_width = Z80_ICS_WIDTH_LOWER8;
    value = 0;
    result = 0;
    driver_magic = 0;
    op_count = 0;
    z80_ics_make_sample_voice(0, &voice_shadow);

    z80_ics_init();
    if (z80_ics_ping(&driver_magic))
        op_count++;
    z80_ics_get_irq_counts(&irq_counts);
}

static void draw_voice_summary(void)
{
    textf("V AHI %04X ALO %04X FC %04X\n", voice_shadow.osc_acc_hi, voice_shadow.osc_acc_lo, voice_shadow.osc_fc);
    textf("V S %04X/%02X E %04X/%02X\n", voice_shadow.osc_start_hi, voice_shadow.osc_start_lo, voice_shadow.osc_end_hi, voice_shadow.osc_end_lo);
    textf("V CNF %02X PAN %02X VC %02X\n", voice_shadow.osc_conf, voice_shadow.pan, voice_shadow.vol_ctrl);
}

static void update(void)
{
    igs023_wait_vblank();
    input_update();
    clamp_fields();

    text_color(1);
    text_cursor(2, 2);
    text("Z80 ICS DEBUG\n");
    textf("DRV %04X RDY %04X SEQ %02X\n", driver_magic, z80_ics_ready(), z80_ics_last_seq());
    textf("STAT %s ERR %04X OPS %04X\n", status_name(z80_ics_last_status()), z80_ics_last_error(), op_count);
    //textf("REG V%02X R%02X W%u VAL %04X\n", selected_voice, selected_reg, selected_width, value);
    textf("RESULT %04X\n", result);
    //textf("IRQ T0 %08X T1 %08X\n", irq_counts.timer0, irq_counts.timer1);
    textf("IRQ O  %08X V  %08X\n", irq_counts.osc, irq_counts.vol);
    //textf("IRQ SP %08X\n", irq_counts.spurious);
    //draw_voice_summary();

    gui_begin(2, 8);
    gui_u16("VOICE", &selected_voice);
    gui_u16("REG", &selected_reg);
    gui_u16("WIDTH", &selected_width);
    gui_u16("VALUE", &value);

    if (gui_button("READREG"))
    {
        if (z80_ics_read_reg((u8)selected_voice, (u8)selected_reg, width_value(), &result))
            op_count++;
    }
    if (gui_button("WRITEREG"))
    {
        if (z80_ics_write_reg((u8)selected_voice, (u8)selected_reg, width_value(), value))
            op_count++;
    }
    if (gui_button("RDVOICE"))
    {
        if (z80_ics_read_voice((u8)selected_voice, &voice_shadow))
            op_count++;
    }
    if (gui_button("WRVOICE"))
    {
        if (z80_ics_write_voice((u8)selected_voice, &voice_shadow))
            op_count++;
    }
    if (gui_button("SAMPLE0"))
    {
        z80_ics_make_sample_voice(0, &voice_shadow);
    }
    if (gui_button("SAMPLE1"))
    {
        z80_ics_make_sample_voice(1, &voice_shadow);
    }
    if (gui_button("PLAY"))
    {
        voice_shadow.osc_ctl = 0x00;
        if (z80_ics_write_voice((u8)selected_voice, &voice_shadow))
            op_count++;
    }
    if (gui_button("STOP"))
    {
        if (z80_ics_write_reg((u8)selected_voice, 0x10, Z80_ICS_WIDTH_UPPER8, 0x0f))
            op_count++;
    }
    if (gui_button("IRQGET"))
    {
        if (z80_ics_get_irq_counts(&irq_counts))
            op_count++;
    }
    if (gui_button("IRQCLR"))
    {
        if (z80_ics_reset_irq_counts(&irq_counts))
            op_count++;
    }
    gui_end();
}

PAGE_REGISTER(z80_ics_test, init, update, NULL);
