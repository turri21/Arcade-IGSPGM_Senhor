#include "../system.h"
#include "../memory_map.h"
#include "../page.h"

#include "../util.h"
#include "../tilemap.h"
#include "../igs023.h"
#include "../gui.h"
#include "../color.h"

static void wobble(s16 offset)
{
    for (s16 y = 0; y < 64; y++)
    {
        //VRAM->bg_scroll[y + 96 + 8] = 64 + (sin_approx((y << 2) + offset) >> 2);
        VRAM->bg_scroll[y + 96 + 8] = y + 32;
    }
}

static u8 sy = 0;

static void init()
{
    igs023_init();
    text_reset();
    set_default_palette();
   
    memset(VRAM->bg, 0, sizeof(VRAM->bg));
    memset(VRAM->bg_scroll, 0, sizeof(VRAM->bg_scroll));
 
    VRAM->bg[4 + (3 * 64)].code = 0x7;
    VRAM->bg[4 + (3 * 64)].attrib = 0x0;

    IGS023_BG_CTRL_SET(0x210);
}

static void update()
{
    igs023_wait_vblank();

    if ((igs023_get_vblank_count() & 0x3) == 0)
    {
        IGS023_BG_CTRL_SET(( IGS023_BG_CTRL_GET() & 0xfc1f ) | (( sy & 0x1f ) << 5));

        sy = (sy + 1) & 0x1f;
        //sy = 0x1f;
    }
}

PAGE_REGISTER(bg_scale_test, init, update, NULL);

