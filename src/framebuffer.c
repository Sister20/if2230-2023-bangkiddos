#include "lib-header/framebuffer.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/portio.h"

void framebuffer_set_cursor(uint8_t r, uint8_t c)
{
    // TODO : Implement
    uint16_t pos = r * 80 + c;

    out(CURSOR_PORT_CMD, 15);
    out(CURSOR_PORT_DATA, (uint8_t)(pos & 0xFF));
    
    out(CURSOR_PORT_CMD, 14);
    out(CURSOR_PORT_DATA, (uint8_t) ((pos >> 8) & 0xFF));
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg)
{
    // TODO : Implement
    uint16_t color;
    volatile uint16_t * location;
    volatile uint16_t * fb;

    fb = (volatile uint16_t*) 0xb8000;
    color = (bg << 4) | (fg & 0x0F);
    location = fb + (row * 80 + col);
    *location = (color << 8) | c;
}

void framebuffer_clear(void)
{
    memset(MEMORY_FRAMEBUFFER, 0, 25 * 80 * 2);
}
