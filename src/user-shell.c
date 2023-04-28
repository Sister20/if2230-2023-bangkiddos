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
    char * amogus_song = "You're a sneaky little impostor!\n"
"Aren't you?\n"
"But you're among us!\n"
"I can feel it!\n"
"I can feel it in my bones!\n"
"So why don't you show yourself?\n"
"And leave us all alone?";

    struct ClusterBuffer cl           = {0};
    
    uint8_t text_len;
    strlen(amogus_song, text_len);
    memcpy(cl.buf, amogus_song, text_len);

    struct FAT32DriverRequest request = {
        .buf                   = &cl,
        .name                  = "amogus",
        .ext                   = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = CLUSTER_SIZE,
    };
    int32_t retcode;
    write_file(request, retcode);
    if (retcode == 0) {
        // struct location loc = {12, 0};
        // print_to_screen("amogus.txt created successfully", loc, SHELL_COMMAND_COLOR);    
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

    if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "cls") == 0) {
        clear_screen();
    } else if (strcmp(cmd, "ls") == 0) {
        struct location loc = {rw, 0};

        struct FAT32DirectoryTable dir_table;
        get_cur_working_dir(state.working_directory, (uint32_t) &dir_table);

        print_cur_working_dir(loc, dir_table);
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

        char arg[MAX_COMMAND_LENGTH];
        strcpy(arg, buffer[1]);

        cat(arg);
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

void print_cur_working_dir(struct location loc, struct FAT32DirectoryTable dir_table) {
    // Iterate through the directory entries and print the names of subdirectories
    for (int i = 1; i < 64; i++) { // not including parent
        if (dir_table.table[i].user_attribute == UATTR_NOT_EMPTY) {
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
            loc.row ++;
            print_to_screen(dir_name, loc, SHELL_COMMAND_COLOR);
        }
    }

    uint8_t rw, cl;
    rw = loc.row + 1;
    cl = 0;
    set_cursor_loc(rw, cl);
}

void cat(char filename[256]) {
    uint8_t rw, cl;
    get_cursor_loc(rw, cl);
    struct location cursor_loc = {rw, cl};

    struct ClusterBuffer res = {0};
    struct FAT32DriverRequest req = {
        .buf                   = &res,
        .name                  = "\0\0\0\0",
        .ext                   = "\0\0\0",
        .parent_cluster_number = state.working_directory,
        .buffer_size           = CLUSTER_SIZE,
    };

    char split_filename[MAX_COMMAND_SPLIT][MAX_COMMAND_LENGTH] = {0};
    strsplit(filename, '.', split_filename);

    strcpy(req.name, split_filename[0]);
    strcpy(req.ext, split_filename[1]);

    uint8_t stat;

    if (req.name[0] == '\0') {
        stat = 3;
    } else {
        read_file(req, stat);   
    }


    char msg[256] = {0};

    switch (stat)
    {
    case 0:
        /* success */
        print_to_screen(res.buf, cursor_loc, SHELL_COMMAND_COLOR);

        uint8_t row_count = 0;
        uint8_t i = 0;
        char c = res.buf[0];
        while (c != '\0') {
            if (c == '\n') {
                row_count++;
            }
            i++;
            c = res.buf[i];
        }
        rw += row_count;
        set_cursor_loc(rw, 0);

        break;
    case 1:
        /* not a file */
        strcat(msg, "\'");
        strcat(msg, filename);
        strcat(msg, "\' is not a file");
        print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
        break;
    case 2:
        /* Not enough buffer */
        print_to_screen("Not enough buffer. File too big!", cursor_loc, SHELL_COMMAND_COLOR);
        break;
    case 3:
        /* File not found */
        strcat(msg, "File \'");
        strcat(msg, filename);
        strcat(msg, "\' not found");

        print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
        break;
    default:
        break;
    }

    if (rw + 1 >= 25) {
        clear_screen();
    } else {
        rw++;
        cl = 0;
        set_cursor_loc(rw, cl);
    }
}
