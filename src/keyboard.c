#include "lib-header/keyboard.h"
#include "lib-header/portio.h"
#include "lib-header/framebuffer.h"
#include "lib-header/stdmem.h"
#include "lib-header/interrupt.h"

#define LEFT_SHIFT_KEY 0x2A
#define RIGHT_SHIFT_KEY 0x36

// Define the keyboard scancode to ASCII map
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
    .keyboard_buffer   = {0},
    .uppercase_on      = FALSE
};

void keyboard_state_activate(void){
    activate_keyboard_interrupt();
    keyboard_state.keyboard_input_on = TRUE;  
}

void keyboard_state_deactivate(void){
    keyboard_state.keyboard_input_on = FALSE;
    keyboard_state.buffer_index      = 0;
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
        char     mapped_char  = keyboard_scancode_1_to_ascii_map[scancode];

        uint8_t c_row, c_col; // cursor row and column
        framebuffer_get_cursor(&c_row, &c_col);

        if (scancode & 0x80) { // release interrupt
            // check if the released key is the left or right Shift key
            if (scancode == (LEFT_SHIFT_KEY | 0x80)) {
                keyboard_state.uppercase_on = !keyboard_state.uppercase_on;
            } else if (scancode == (RIGHT_SHIFT_KEY | 0x80)) {
                keyboard_state.uppercase_on = !keyboard_state.uppercase_on;
            }
        } else if (scancode == LEFT_SHIFT_KEY || scancode == RIGHT_SHIFT_KEY) {
            // check if pressed key is the left or right Shift key
            keyboard_state.uppercase_on = !keyboard_state.uppercase_on;
        } else if (mapped_char == '\b' && keyboard_state.buffer_index > 0) {
            // backspace
            keyboard_state.buffer_index--;
            framebuffer_set_cursor(c_row, c_col - 1);
            framebuffer_write(c_row, c_col - 1, '\0', 0xF, 0x0);
        } else if (mapped_char == '\b' && keyboard_state.buffer_index == 0) {
            // do nothing if backspace is pressed but there is no character to delete.
        } else if (mapped_char == '\n') {
            keyboard_state.keyboard_buffer[keyboard_state.buffer_index++] = '\0';
            framebuffer_set_cursor(c_row + 1, 0);
            
            /**
             * buffer is still available at this point
            */
            
            // Reading stops when enter is pressed.
            keyboard_state_deactivate();
        } else {
            // handle uppercase letters for Shift and Capslock

            if (scancode == 0x3A) {
                keyboard_state.uppercase_on = !keyboard_state.uppercase_on;
            } else {
                if (keyboard_state.uppercase_on) {
                    if (mapped_char >= 'a' && mapped_char <= 'z') {
                        mapped_char -= 'a' - 'A'; // convert to uppercase
                    } else if (mapped_char >= 'A' && mapped_char <= 'Z') {
                        mapped_char += 'a' - 'A'; // convert to lowercase
                    }
                }

                keyboard_state.keyboard_buffer[keyboard_state.buffer_index++] = mapped_char;
                framebuffer_write(c_row, c_col, mapped_char, 0xF, 0x0);
                framebuffer_set_cursor(c_row, c_col + 1);
            }
        }
    }
    pic_ack(IRQ_KEYBOARD);
}