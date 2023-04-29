#include "stdtype.h"

#define SHELL_DIRECTORY "user@bangkiddOS:~"
#define SHELL_PROMPT    "$ "

#define KEYBOARD_BUFFER_SIZE 256
#define SHELL_BUFFER_SIZE 256

#define SHELL_PROMPT_COLOR  178
#define SHELL_COMMAND_COLOR 0xF

#define EXT_SCANCODE_LEFT      0x4B
#define EXT_SCANCODE_RIGHT     0x4D

#define MAX_COMMAND_SPLIT 16
#define MAX_COMMAND_LENGTH 256



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
    char                dir_string[256];
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
void process_command();
uint8_t strcmp(char * str1, char * str2);
void reset_command_buffer();
void change_dir(char path[256], struct FAT32DirectoryTable dir_table);
void print_cur_working_dir(struct location loc, struct FAT32DirectoryTable dir_table);
void cat(char arg[256]);
void rm(char arg[256]);
void mv(char src[256], char dest[256]);
void mkdir(char arg[256]);
void cp(char src[256], char dest[256]);
