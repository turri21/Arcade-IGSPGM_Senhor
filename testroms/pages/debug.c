#include "../system.h"
#include "../memory_map.h"
#include "../page.h"

#include "../util.h"
#include "../tilemap.h"
#include "../igs023.h"
#include "../color.h"
#include "../gui.h"

#include "../debug_link.h"

static uint16_t frame_count = 0;

static void init()
{
    igs023_init();
    text_reset();
    set_default_palette();

    frame_count = 0;
}

static void update()
{
    igs023_wait_vblank();
    char status[256];
    text_cursor(1, 2);
    textf("VBL: %05X  IRQ: %05X FRAME: %05X\n", igs023_get_vblank_count(), igs023_get_irq4_count(), frame_count);
    debug_link_status(status, 256);
    textf("%s\n", status);
    textf("%s\n", debug_link_check_active() ? "ACTIVE" : "INACTIVE");


    u8 buffer[32];
    if (debug_link_read(buffer, 32) > 0)
    {
        textf("BYTE: %02X\n", buffer[0]);
    }

    gui_begin(2, 8);
    if (gui_button("SEND"))
    {
        debug_link_write("Hello World!", 12);
    }
    gui_end();
    frame_count++;
}

PAGE_REGISTER(debug, init, update, NULL);

