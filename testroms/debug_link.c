#include <stdint.h>
#include <stdbool.h>

#include "printf/printf.h"
#include "debug_link.h"

typedef struct u8_rom
{
    uint8_t pad0;
    uint8_t v;
    uint8_t pad1;
    uint8_t pad2;
} u8_rom;

typedef volatile struct CommsRegisters
{
    uint8_t  magic[4];

    // only the least significant byte is relevant for these, using 32-bits to avoid potential atomic issues
    u8_rom   active;
    u8_rom   pending;
    u8_rom   in_seq;
    u8_rom   out_seq;
    uint32_t debug1;
    uint32_t debug2;

    uint8_t  reserved0[1024 - (7 * 4)];

    u8_rom   in_byte;
    uint8_t  reserved2[512 - (1 * 4)];

    uint16_t out_area[256];
} CommsRegisters;

_Static_assert(sizeof(CommsRegisters) == 2048, "CommsRegisters size mismatch");

CommsRegisters *comms_regs = (CommsRegisters *)(0x1f800 << 1);

static bool magic_valid = false;
static bool comms_active = false;

uint8_t comms_in_seq;
uint8_t comms_out_seq;
volatile uint16_t dummy_read;

static bool comms_check_magic()
{ 
    if(!magic_valid)
    {
        if( comms_regs->magic[0] == 'I' && comms_regs->magic[1] == 'P' && comms_regs->magic[2] == 'O' && comms_regs->magic[3] == 'C' )
        {
            magic_valid = true;
        }
    }

    return magic_valid;
}

bool debug_link_check_active()
{
    if( comms_regs->active.v == 1 ) return comms_check_magic();
    return false;
}


bool debug_link_update()
{
    bool active = debug_link_check_active();

    if (!comms_active && active)
    {
        comms_active = true;
        comms_in_seq = 0;
        comms_out_seq = 0;
    }
    else if (comms_active && !active)
    {
        comms_active = false;
    }

    return active;
}

void debug_link_status(char *str, int len)
{
    snprintf(str, len, "ACT: %01X IN: %02X/%02X OUT: %02X/%02X %08X %08X", comms_regs->active.v, comms_regs->in_seq.v, comms_in_seq, comms_regs->out_seq.v, comms_out_seq, comms_regs->debug1, comms_regs->debug2);
}

int debug_link_read(void *buffer, int maxlen)
{
    if (!debug_link_update()) return 0;

    int len = 0;

    uint8_t *buffer8 = (uint8_t *)buffer;

    while(comms_regs->active.v && ( (comms_in_seq != comms_regs->in_seq.v) || comms_regs->pending.v ))
    { 
        if(comms_in_seq != comms_regs->in_seq.v)
        {
            buffer8[len] = comms_regs->in_byte.v;
            comms_in_seq++;
            len++;

            if (len == maxlen) return len;
        }
    }
    return len;
}

int debug_link_write(const void *data, int len)
{
    int sent = 0;

    if (!debug_link_update()) return 0;

    const uint8_t *data8 = (const uint8_t *)data;

    while (sent < len)
    {
        const uint8_t b = data8[sent];
        dummy_read = comms_regs->out_area[b];
        comms_out_seq++;
        while (comms_regs->out_seq.v != comms_out_seq)
        {
            if( !comms_regs->active.v )
            {
                break;
            }
        };
        sent++;
    }

    return sent;
}
