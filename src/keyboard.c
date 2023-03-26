#include "lib-header/keyboard.h"
#include "lib-header/portio.h"
#include "lib-header/framebuffer.h"
#include "lib-header/stdmem.h"
#include "lib-header/interrupt.h"

const char keyboard_scancode_1_to_ascii_map[256] = {
      0, 0x1B, '1', '2', '3', '4', '5', '6',  '7', '8', '9',  '0',  '-', '=', '\b', '\t',
    'q',  'w', 'e', 'r', 't', 'y', 'u', 'i',  'o', 'p', '[',  ']', '\n',   0,  'a',  's',
    'd',  'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0, '\\',  'z', 'x',  'c',  'v',
    'b',  'n', 'm', ',', '.', '/',   0, '*',    0, ' ',   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0, '-',    0,    0,   0,  '+',    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,

      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
};

static struct KeyboardDriverState keyboard_state = {
    .keyboard_input_on = FALSE,
    .buffer_index      = 0,
    .keyboard_buffer   = {0}
};

static int row = 0;

void keyboard_state_activate(void){
    activate_keyboard_interrupt();
    keyboard_state.keyboard_input_on = TRUE;  
}

void keyboard_state_deactivate(void){
    keyboard_state.keyboard_input_on = FALSE;
    keyboard_state.buffer_index      = 0;
    memset(keyboard_state.keyboard_buffer, 0, KEYBOARD_BUFFER_SIZE);
}

void get_keyboard_buffer(char *buf){
    memcpy(buf, keyboard_state.keyboard_buffer, KEYBOARD_BUFFER_SIZE);
}

bool is_keyboard_blocking(void){
    return keyboard_state.keyboard_input_on;
}

void keyboard_isr(void){
    if (!keyboard_state.keyboard_input_on)
        keyboard_state.buffer_index = 0;
    else {
        uint8_t  scancode    = in(KEYBOARD_DATA_PORT);
        char     mapped_char = keyboard_scancode_1_to_ascii_map[scancode];
        
        /**
         * TODO:
         * - handle extended scancode
         * - change keyboard buffer clearance -> should not be cleared
         * - handle multi row input with framebuffer_get_cursor()
        */

        if (scancode & 0x80) { //ignore release interrupt
            // .. ignore
        } else if (mapped_char == '\b' && keyboard_state.buffer_index > 0) {
            keyboard_state.buffer_index--;
            framebuffer_set_cursor(row, keyboard_state.buffer_index);
            framebuffer_write(row, keyboard_state.buffer_index, ' ', 0xF, 0x0);
        } else if (mapped_char == '\b' && keyboard_state.buffer_index == 0) {
            // Do nothing
        } else if (mapped_char == '\n') {
            keyboard_state_deactivate();
            row++;
        } else {
            framebuffer_set_cursor(row, keyboard_state.buffer_index + 1);
            framebuffer_write(row, keyboard_state.buffer_index, mapped_char, 0xF, 0x0);
            keyboard_state.keyboard_buffer[keyboard_state.buffer_index++] = mapped_char;
        }
    }
    pic_ack(IRQ_KEYBOARD);

}