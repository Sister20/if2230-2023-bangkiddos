#include "lib-header/framebuffer.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/portio.h"

void framebuffer_set_cursor(uint8_t r, uint8_t c)
{
    /* Implementation */
    uint16_t pos = r * 80 + c;

    out(CURSOR_PORT_CMD, 15);
    out(CURSOR_PORT_DATA, (uint8_t)(pos & 0xFF));
    
    out(CURSOR_PORT_CMD, 14);
    out(CURSOR_PORT_DATA, (uint8_t) ((pos >> 8) & 0xFF));
}

void framebuffer_get_cursor(uint8_t *r, uint8_t *c)
{
    /* Implementation */
    uint16_t pos;
    
    out(CURSOR_PORT_CMD, 15);
    pos = in(CURSOR_PORT_DATA) & 0xFF;

    out(CURSOR_PORT_CMD, 14);
    pos |= (in(CURSOR_PORT_DATA) & 0xFF) << 8;

    *r = pos / 80;
    *c = pos % 80;
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg)
{
    /* Implementation */
    uint16_t color;
    volatile uint16_t * location;
    volatile uint16_t * fb;

    fb = (volatile uint16_t*) 0xC00B8000;

    /*
        Bit:     | 15 14 13 12 11 10 9 8 | 7 6 5 4 | 3 2 1 0 |
        Content: | ASCII                 | FG      | BG      |
    */

    // bg | fg
    color = (bg << 4) | (fg & 0x0F);
    location = fb + (row * 80 + col);

    // add ASCII
    *location = (color << 8) | c;
}

void framebuffer_clear(void)
{
    /* Implementation */

    // memset(MEMORY_FRAMEBUFFER, 0, 25 * 80 * 2);

    /* agar sudah ter-set semua buffernya */
    for (int i = 0; i < 25; i ++) {
        for (int j = 0; j < 80; j ++) {
            framebuffer_write(i, j, ' ', 0xF, 0x0);
        }
    }
}

void printString(char *string, uint8_t row, uint8_t col, uint8_t color) {
    uint8_t i = 0;
    char c = string[i];

    // Bit    7 6 5 4 3 2 1 0
    // Data   R R R G G G B B

    while (c != '\0')
    {
        framebuffer_write(row, col + i, c, color, 0x0);
        i++;
        c = string[i];
    }
}

void printBlock(uint8_t row, uint8_t col, uint8_t n, uint8_t color) {
    uint8_t i = col;
    uint8_t count = 0;

    while (count < n) {
        framebuffer_write(row, i + count, ' ', 0xF, color);
        count ++;
    }
}