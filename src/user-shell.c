#include "lib-header/stdtype.h"
#include "lib-header/fat32.h"
#include "lib-header/string.h"
#include "lib-header/user-shell.h"

static struct ShellState state = {
    .working_directory = ROOT_CLUSTER_NUMBER,
    .command_buffer    = {0},
    .buffer_index      = 0,
    .buffer_length     = 0,
    .command_start_location = {0, 0}
};

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

int main(void) {
    // struct ClusterBuffer cl           = {0};
    // struct FAT32DriverRequest request = {
    //     .buf                   = &cl,
    //     .name                  = "ikanaide",
    //     .ext                   = "\0\0\0",
    //     .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    //     .buffer_size           = CLUSTER_SIZE,
    // };
    // int32_t retcode;
    // syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0);
    // if (retcode == 0)
    //     syscall(5, (uint32_t) "owo\n", 4, 0xF);

    print_shell_directory();
    while (TRUE) {
        listen_to_keyboard();
    }

    return 0;

}

void print_shell_directory() {
    char * directory = "";
    /* Get path to current directory */

    char * shell_prompt = SHELL_DIRECTORY;
    struct location cursor_loc = {0, 0};

    // concat
    syscall(83, (uint32_t) shell_prompt, (uint32_t) directory, 0); // strcat(shell_prompt, directory);
    syscall(83, (uint32_t) shell_prompt, (uint32_t) SHELL_PROMPT, 0); // strcat(shell_prompt, SHELL_PROMPT);

    uint8_t rw, cl;
    syscall(50, (uint32_t) &rw, (uint32_t) &cl, 0); // get cursor loc
    rw = rw + ((uint8_t) cl / 80);
    cl = cl % 80;

    cursor_loc.row = rw; cursor_loc.col = cl;

    syscall(52, (uint32_t) shell_prompt, (uint32_t) &cursor_loc, SHELL_COMMAND_COLOR); // print shell prompt

    uint8_t prompt_len;
    syscall(80, (uint32_t) shell_prompt, (uint32_t) &prompt_len, 0); // strlen(shell_prompt);

    syscall(50, (uint32_t) &rw, (uint32_t) &cl, 0); // get cursor loc
    rw = rw + (prompt_len / 80);
    cl = cl + (prompt_len % 80);
    syscall(51, rw, cl, 0); // set cursor loc

    state.command_start_location.row = rw;
    state.command_start_location.col = cl;

}

void listen_to_keyboard() {
    struct KeyboardDriverState keyboard;
    syscall(4, (uint32_t) &keyboard, 0, 0); // get keyboard state
    uint8_t rw, cl;
    syscall(50, (uint32_t) &rw, (uint32_t) &cl, 0); // get cursor loc

    if (state.buffer_length != keyboard.buffer_length) { //changes made
        if (keyboard.last_char == '\n') {
            // process_command(state.command_buffer);
        } else if (keyboard.last_char == '\b' && state.buffer_index > 0) {
            state.buffer_index--;
            state.buffer_length--;
            state.command_buffer[state.buffer_index] = 0;

            if (cl == 0 && rw > state.command_start_location.row) {
                rw--;
                cl = 79;
            } else {
                cl--;
            }
            struct location cursor_loc = {rw, cl};
            syscall(51, rw, cl, 0); // set cursor loc
            // empty the framebuffer
            syscall(52, (uint32_t) " ", (uint32_t) &cursor_loc, SHELL_COMMAND_COLOR);
        } else if (keyboard.last_char == '\b' && state.buffer_index == 0) {
            // do nothing
        } else {
            state.command_buffer[state.buffer_index++] = keyboard.last_char;
            state.buffer_length = keyboard.buffer_length;


            struct location cursor_loc;
            cursor_loc.row = rw; cursor_loc.col = cl;
            
            syscall(52, (uint32_t) &keyboard.last_char, (uint32_t) &cursor_loc, SHELL_COMMAND_COLOR); // print last char

            syscall(51, rw, cl + 1, 0); // set cursor loc
        }
    } else if (keyboard.last_scancode != 0) {

    }
}

void ask_for_command() {
    listen_to_keyboard();
    // parse_command();
    // execute_command();
}

// void process_command(char* cmd) {
//     syscall(80)
// }