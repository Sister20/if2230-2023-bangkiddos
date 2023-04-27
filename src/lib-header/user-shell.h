#include "stdtype.h"

#define SHELL_DIRECTORY "user@bangkiddOS:~"
#define SHELL_PROMPT    "$ "

#define KEYBOARD_BUFFER_SIZE 256
#define SHELL_BUFFER_SIZE 256

#define SHELL_PROMPT_COLOR  178
#define SHELL_COMMAND_COLOR 0xF

struct location {
    uint8_t row;
    uint8_t col;
};

struct ShellState {
    uint32_t            working_directory;
    char                command_buffer[SHELL_BUFFER_SIZE];
    uint8_t             buffer_index;
    uint8_t             buffer_length;
    struct location     command_start_location;
};

struct KeyboardDriverState {
    bool    read_extended_mode;
    bool    keyboard_input_on;
    uint8_t buffer_index;
    char    keyboard_buffer[KEYBOARD_BUFFER_SIZE];
    uint8_t buffer_length;
    char    last_char;
    uint8_t last_scancode;
    
    // tambahan
    bool    uppercase_on;
} __attribute((packed));

void ask_for_command();
void print_shell_directory();
void listen_to_keyboard();
void process_command(char * command);