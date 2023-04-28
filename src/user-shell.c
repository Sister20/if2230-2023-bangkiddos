#include "lib-header/stdtype.h"
#include "lib-header/fat32.h"
#include "lib-header/string.h"
#include "lib-header/user-shell.h"
#include "lib-header/framebuffer.h"

#define strlen(str,lenvar) syscall(80, (uint32_t) str, (uint32_t) &lenvar, 0)
#define strcat(str1,str2) syscall(83, (uint32_t) str1, (uint32_t) str2, 0)
#define strcpy(dest,src) syscall(82, (uint32_t) dest, (uint32_t) src, 0)
#define strset(dest,ch,len) syscall(84, (uint32_t) dest, (uint32_t) ch, len)
#define strsplit(str,delim,result) syscall(85, (uint32_t) str, (uint32_t) delim, (uint32_t) result)
#define get_cursor_loc(rw,cl) syscall(50, (uint32_t) &rw, (uint32_t) &cl, 0)
#define set_cursor_loc(rw,cl) syscall(51, rw, cl, 0)
#define print_to_screen(str,loc,color) syscall(52, (uint32_t) str, (uint32_t) &loc, color)
#define clear_screen() syscall(53, 0, 0, 0)
#define read_file(request,retcode) syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0)
#define read_directory(request,retcode) syscall(1, (uint32_t) &request, (uint32_t) &retcode, 0)
#define write_file(request,retcode) syscall(2, (uint32_t) &request, (uint32_t) &retcode, 0)
#define delete_file(request,retcode) syscall(3, (uint32_t) &request, (uint32_t) &retcode, 0)
#define memcpy(dest,src,len) syscall(10, (uint32_t) dest, (uint32_t) src, len)
#define memset(dest,val,len) syscall(11, (uint32_t) dest, (uint32_t) val, len)

#define get_cur_working_dir(cur_working_dir, dir_table) syscall(60, (uint32_t)cur_working_dir, (uint32_t) dir_table, 0)

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
    struct ClusterBuffer cl           = {0};
    struct FAT32DriverRequest request = {
        .buf                   = &cl,
        .name                  = "amogus",
        .ext                   = "txt",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = CLUSTER_SIZE,
    };
    int32_t retcode;
    syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0);
    if (retcode == 0) {
        syscall(5, (uint32_t) "owo\n", 4, 0xF);
    }

    print_shell_directory();
    while (TRUE) {
        listen_to_keyboard();
    }

    return 0;

}

uint8_t strcmp(char* str1, char* str2){
    int i = 0;
    while(str1[i] != 0 && str2[i] != 0){
        if(str1[i] > str2[i]){
            return 1;
        }else if(str1[i] < str2[i]){
            return -1;
        }
        i++;
    }
    if(str1[i] == 0 && str2[i] == 0){
        return 0;
    }else if(str1[i] == 0){
        return -1;
    }else{
        return 1;
    }
}

void print_shell_directory() {
    char directory[256] = "";
    /* Get path to current directory */

    char shell_prompt[256] = SHELL_DIRECTORY;
    struct location cursor_loc = {0, 0};

    // concat
    strcat(shell_prompt, directory);
    strcat(shell_prompt, SHELL_PROMPT);

    uint8_t rw, cl;
    get_cursor_loc(rw, cl);
    rw = rw + ((uint8_t) cl / 80);
    cl = cl % 80;

    cursor_loc.row = rw; cursor_loc.col = cl;

    print_to_screen(shell_prompt, cursor_loc, SHELL_PROMPT_COLOR);

    uint8_t prompt_len;
    strlen(shell_prompt, prompt_len);

    get_cursor_loc(rw, cl);
    rw = rw + (prompt_len / 80);
    cl = cl + (prompt_len % 80);
    set_cursor_loc(rw, cl);

    state.command_start_location.row = rw;
    state.command_start_location.col = cl;
}

void listen_to_keyboard() {
    struct KeyboardDriverState keyboard;
    syscall(4, (uint32_t) &keyboard, 0, 0); // get keyboard state
    uint8_t rw, cl;
    get_cursor_loc(rw, cl);

    if (state.buffer_length != keyboard.buffer_length) { //changes made
        if (keyboard.last_char == '\n') {
            state.command_buffer[state.buffer_index] = 0;
            process_command();

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
            set_cursor_loc(rw, cl);
            // empty the framebuffer
            print_to_screen(" ", cursor_loc, SHELL_COMMAND_COLOR);
        } else if (keyboard.last_char == '\b' && state.buffer_index == 0) {
            // do nothing
        } else {
            state.command_buffer[state.buffer_index++] = keyboard.last_char;
            state.buffer_length = keyboard.buffer_length;


            struct location cursor_loc;
            cursor_loc.row = rw; cursor_loc.col = cl;
            
            print_to_screen(&keyboard.last_char, cursor_loc, SHELL_COMMAND_COLOR);

            set_cursor_loc(rw, cl + 1);
        }
    } else if (keyboard.last_scancode != 0) {
        if (keyboard.last_scancode == EXT_SCANCODE_LEFT) {
            if (state.buffer_index > 0) {
                state.buffer_index--;
                if (cl == 0 && rw > state.command_start_location.row) {
                    rw--;
                    cl = 79;
                } else {
                    cl--;
                }
                set_cursor_loc(rw, cl);
            }
        } else if (keyboard.last_scancode == EXT_SCANCODE_RIGHT) {
            if (state.buffer_index < state.buffer_length) {
                state.buffer_index++;
                set_cursor_loc(rw, cl + 1);
            }
        }
    }
}

void process_command() {
    char buffer[MAX_COMMAND_SPLIT][MAX_COMMAND_LENGTH] = {0};
    strsplit(state.command_buffer, ' ', buffer);

    char cmd[MAX_COMMAND_LENGTH];
    strcpy(cmd, buffer[0]); 

    uint8_t cmd_len;
    strlen(cmd, cmd_len);
    uint8_t rw, cl;
    get_cursor_loc(rw, cl);

    strcpy(cmd, buffer[0]);

    if (strcmp(cmd, "clear") == 0) {
        clear_screen();
    } else if (strcmp(cmd, "ls") == 0) {
        struct location loc = {rw, 0};

        struct FAT32DirectoryTable dir_table;
        get_cur_working_dir(state.working_directory, (uint32_t) &dir_table);

        // Iterate through the directory entries and print the names of subdirectories
        for (int i = 0; i < 64; i++)
        {
            if (dir_table.table[i].user_attribute == UATTR_NOT_EMPTY &&
                dir_table.table[i].attribute == ATTR_SUBDIRECTORY)
            {
                char dir_name[9];
                int j;
                for (j = 0; j < 8; j++)
                {
                    if (dir_table.table[i].name[j] == ' ')
                    {
                        break;
                    }
                    dir_name[j] = dir_table.table[i].name[j];
                }
                dir_name[j] = '\0';
                // Append the subdirectory name to the buffer
                loc.row ++;
                print_to_screen(dir_name, loc, SHELL_COMMAND_COLOR);

                // strcat(dir, dir_name);
                // strcat(dir, " ");
            }
        }


        rw = loc.row + 1;
        cl = 0;
        set_cursor_loc(rw, cl);
    } else if (strcmp(cmd, "") == 0) {
        if (rw + 1 >= 25) {
            clear_screen();
        } else {
            rw++;
            cl = 0;
            set_cursor_loc(rw, cl);
        }
    } else if (strcmp(cmd, "cat") == 0) {
        // struct location cursor_loc = {rw + 1, 0};
        set_cursor_loc(rw + 1, 0);

        // char arg[MAX_COMMAND_LENGTH];
        // strcpy(arg, buffer[1]);

        // cat(arg, cursor_loc);
    } else {
        struct location cursor_loc = {rw + 1, 0};

        print_to_screen("\'", cursor_loc, SHELL_COMMAND_COLOR);
        cursor_loc.col += 1;
        print_to_screen(cmd, cursor_loc, SHELL_COMMAND_COLOR);
        cursor_loc.col += cmd_len;
        print_to_screen("\'", cursor_loc, SHELL_COMMAND_COLOR);
        cursor_loc.col += 2;
        print_to_screen("is not recognized as an internal command", cursor_loc, SHELL_COMMAND_COLOR);

        if (rw + 2 >= 25) {
            clear_screen();
        } else {
            rw += 2;
            cl = 0;
            set_cursor_loc(rw, cl);
        }
    }
    reset_command_buffer();
    print_shell_directory();
}

void reset_command_buffer() {
    state.buffer_index = 0;
    state.buffer_length = 0;
    strset(state.command_buffer, 0, SHELL_BUFFER_SIZE);
}

// void cat(char filename[256]) {
//     uint8_t rw, cl;
//     get_cursor_loc(rw, cl);
//     struct location cursor_loc = {rw, cl};

//     struct ClusterBuffer buf = {0};
//     struct FAT32DriverRequest req = {
//         .buf                   = &cl,
//         .name                  = "temp",
//         .ext                   = "txt",
//         .parent_cluster_number = ROOT_CLUSTER_NUMBER,
//         .buffer_size           = CLUSTER_SIZE,
//     };

//     memcpy(req.name, filename, strlen(filename));

//     uint8_t stat;
//     read_file(req, stat);   
// }