#include "lib-header/keyboard.h"
#include "lib-header/portio.h"
#include "lib-header/framebuffer.h"
#include "lib-header/stdmem.h"
#include "lib-header/interrupt.h"
#include "lib-header/string.h"

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
    .read_extended_mode = FALSE,
    .keyboard_input_on  = FALSE,
    .buffer_index       = 0,
    .keyboard_buffer    = {0},
    .buffer_length      = 0,
    .last_char          = 0,
    .last_scancode      = 0,
    .uppercase_on       = FALSE
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

struct KeyboardDriverState get_keyboard_state(void){
    return keyboard_state;
}

void reset_scancode(void){
    keyboard_state.last_scancode = 0;
}

void reset_last_char(void){
    keyboard_state.last_char = 0;
}

bool is_keyboard_blocking(void){
    return keyboard_state.keyboard_input_on;
}

void clear_buffer() {
    for (int i = 0; i < 256; i ++) {
        keyboard_state.keyboard_buffer[i] = 0;
    }

    keyboard_state.buffer_index = 0;
    keyboard_state.buffer_length = 0;
    keyboard_state.last_char = 0;
    keyboard_state.last_scancode = 0;
}

// @deprecated, handled in user program
// void processCommand(uint8_t row) {
    // char command[lengthBuffer() + 1];

    // for (int i = 0; i < lengthBuffer(); i ++) {
    //     command[i] = keyboard_state.keyboard_buffer[i];
    // }

    // command[lengthBuffer()] = '\0';

    // if (strcmp(command, "clear") == 0) {
    //     framebuffer_clear();

    //     framebuffer_write(1, 1, '>', 0xF, 0x0);
    //     framebuffer_set_cursor(1, 3);
    // } else if (strcmp(command, "") == 0) {
    //     if (row + 1 >= 25) {
    //         framebuffer_clear();

    //         framebuffer_write(1, 1, '>', 0xF, 0x0);
    //         framebuffer_set_cursor(1, 3);
    //     } else {
    //         framebuffer_write(row + 1, 1, '>', 0xF, 0x0);
    //         framebuffer_set_cursor(row + 1, 3);
    //     }
    // } else {
    //     // printString("\'", row + 2, 3);
    //     // printString(command, row + 2 , 4);
    //     // printString("\'", row + 2, lengthBuffer() + 4);
    //     // printString(" is not recognized as an internal command", row + 2, lengthBuffer() + 5);

    //     if (row + 4 >= 25) {
    //         framebuffer_clear();

    //         framebuffer_write(1, 1, '>', 0xF, 0x0);
    //         framebuffer_set_cursor(1, 3);
    //     } else {
    //         framebuffer_write(row + 4, 1, '>', 0xF, 0x0);
    //         framebuffer_set_cursor(row + 4, 3);
    //     }
    // }
// }

void keyboard_isr(void){
    if (!keyboard_state.keyboard_input_on)
        keyboard_state.buffer_index = 0;
    else {
        uint8_t  scancode    = in(KEYBOARD_DATA_PORT);

        // @deprecated, handled by user program
        // uint8_t c_row, c_col; // cursor row and column
        // framebuffer_get_cursor(&c_row, &c_col);

        if (scancode == EXTENDED_SCANCODE_BYTE) {
            keyboard_state.read_extended_mode = TRUE;
        } else {
            char mapped_char = keyboard_scancode_1_to_ascii_map[scancode];

            if (keyboard_state.read_extended_mode) {
                keyboard_state.read_extended_mode = FALSE;

                if (scancode == EXT_SCANCODE_LEFT) {
                    if (keyboard_state.buffer_index > 0) {
                        keyboard_state.buffer_index--;
                        keyboard_state.last_scancode = scancode;

                        // @deprecated, handled by user program
                        // if (c_row != 0 && c_col == 0) {
                        //     framebuffer_set_cursor(c_row - 1, 79);
                        // } else {
                        //     framebuffer_set_cursor(c_row, c_col - 1);
                        // }
                    }
                } else if (scancode == EXT_SCANCODE_RIGHT) {
                    if (keyboard_state.buffer_index < strlen(keyboard_state.keyboard_buffer)) {
                        keyboard_state.buffer_index++;
                        keyboard_state.last_scancode = scancode;
                        // framebuffer_set_cursor(c_row, c_col + 1);
                    }
                }
            } else {
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
                    keyboard_state.buffer_length--;
                    keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = 0;
                    keyboard_state.last_char = '\b';


                    // @deprecated, handled by user program
                    // if (c_row != 0 && c_col == 0) {
                    //     framebuffer_set_cursor(c_row - 1, 79);
                    //     framebuffer_write(c_row - 1, 79, '\0', 0xF, 0x0);
                    // } else {
                    //     framebuffer_set_cursor(c_row, c_col - 1);
                    //     framebuffer_write(c_row, c_col - 1, '\0', 0xF, 0x0);
                    // }

                } else if (mapped_char == '\b' && keyboard_state.buffer_index == 0) {
                    // do nothing if backspace is pressed but there is no character to delete.
                } else if (mapped_char == '\n') {
                    keyboard_state.keyboard_buffer[keyboard_state.buffer_index++] = '\n';
                    keyboard_state.buffer_length++;
                    keyboard_state.last_char = '\n';
                    // @deprecated, handled by user program
                    // processCommand(c_row);
                    // clearBuffer();

                    /**
                     * buffer is still available at this point
                    */
                    
                    // Reading stops when enter is pressed.
                    // keyboard_state_deactivate();
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
                        keyboard_state.buffer_length++;
                        keyboard_state.last_char = mapped_char;

                        // @deprecated, handled by user program
                        // framebuffer_write(c_row, c_col, mapped_char, 0xF, 0x0);
                        // framebuffer_set_cursor(c_row, c_col + 1);
                    }
                }
            }
        }
    }
    pic_ack(IRQ_KEYBOARD);
}