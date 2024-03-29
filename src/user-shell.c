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
#define strncpy(dest, src, len) syscall(86, (uint32_t) dest, (uint32_t) src, (uint32_t) len)

#define get_cursor_loc(rw,cl) syscall(50, (uint32_t) &rw, (uint32_t) &cl, 0)
#define set_cursor_loc(rw,cl) syscall(51, rw, cl, 0)
#define print_to_screen(str,loc,color) syscall(52, (uint32_t) str, (uint32_t) &loc, color)
#define clear_screen() syscall(53, 0, 0, 0)
#define read_file(request,retcode) syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0)
#define read_directory(request,retcode) syscall(1, (uint32_t) &request, (uint32_t) &retcode, 0)
#define write_file(request,retcode) syscall(2, (uint32_t) &request, (uint32_t) &retcode, 0)
#define delete(request,retcode) syscall(3, (uint32_t) &request, (uint32_t) &retcode, 0)
#define dirtable_linear_search(parent_cluster_n, req, retcode) syscall(5, (uint32_t) parent_cluster_n, (uint32_t) &req, (uint32_t) &retcode)
#define memcpy(dest,src,len) syscall(10, (uint32_t) dest, (uint32_t) src, len)
#define memset(dest,val,len) syscall(11, (uint32_t) dest, (uint32_t) val, len)

#define get_cur_working_dir(cur_working_dir, dir_table) syscall(60, (uint32_t)cur_working_dir, (uint32_t) dir_table, 0)

#define get_cur_tick(cur_tick) syscall(69, (uint32_t)&cur_tick, 0, 0)
#define print_block(loc, n, color) syscall(70, (uint32_t) &loc, (uint32_t) n, color)

static struct ShellState state = {
    .working_directory = ROOT_CLUSTER_NUMBER,
    .command_buffer    = {0},
    .buffer_index      = 0,
    .buffer_length     = 0,
    .command_start_location = {0, 0},
    .dir_string        = {0}
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
        .ext                   = "txt",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = text_len,
    };
    int8_t retcode;
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
    char shell_prompt[256] = SHELL_DIRECTORY;
    struct location cursor_loc = {0, 0};

    // concat
    strcat(shell_prompt, state.dir_string);
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
    } else if (strcmp(cmd, "cd") == 0) {
        set_cursor_loc(rw + 1, 0);

        char arg[MAX_COMMAND_LENGTH] = {0};
        strcpy(arg, buffer[1]);

        struct FAT32DirectoryTable dir_table;
        get_cur_working_dir(state.working_directory, (uint32_t)&dir_table);

        change_dir(arg, dir_table);
    } else if (strcmp(cmd, "ls") == 0) {
        struct location loc = {rw + 1, 0};

        struct FAT32DirectoryTable dir_table;
        get_cur_working_dir(state.working_directory, (uint32_t) &dir_table);

        print_cur_working_dir(loc, dir_table);
    } else if (strcmp(cmd, "amongus") == 0) {
        struct location loc = {rw, cl};
        amongus(loc);

        rw = loc.row + 15;
        cl = 0;
        set_cursor_loc(rw, cl);
    } else if (strcmp(cmd, "pikachu") == 0) {
        struct location loc = {rw, cl};
        pikachu(loc);

        rw = loc.row + 15;
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
        set_cursor_loc(rw + 1, 0);

        char arg[MAX_COMMAND_LENGTH] = {0};
        strcpy(arg, buffer[1]);

        cat(arg);
    } else if (strcmp(cmd, "whereis") == 0) {
        set_cursor_loc(rw + 1, 0);

        struct location loc = {rw + 1, 0};

        char arg[MAX_COMMAND_LENGTH] = {0};
        strcpy(arg, buffer[1]);

        uint8_t paths_found = 0;
        char paths[16][256] = {0};

        whereis(ROOT_CLUSTER_NUMBER, arg, paths, &paths_found);
        if (paths_found == 0) {
            print_to_screen("No such file or directory", loc, SHELL_COMMAND_COLOR);
            set_cursor_loc(rw + 3, 0);
        } else {
            for (uint8_t i = 0; i < paths_found; i++) {
                char directory[MAX_COMMAND_LENGTH] = "~/";
                strcat(directory, paths[i]);
                print_to_screen(directory, loc, SHELL_COMMAND_COLOR);
                loc.row++;
            }
            set_cursor_loc(loc.row + 1, 0);
        }

    } else if (strcmp(cmd, "mv") == 0) {
        set_cursor_loc(rw + 1, 0);

        char file[MAX_COMMAND_LENGTH];
        char dest[MAX_COMMAND_LENGTH];

        strcpy(file, buffer[1]);
        strcpy(dest, buffer[2]);

        mv(file, dest);
    } else if (strcmp(cmd, "mkdir") == 0) {
        set_cursor_loc(rw + 1, 0);

        char arg[MAX_COMMAND_LENGTH] = {0};

        // handle input where the folder name has spaces
        int i = 1;
        uint8_t text_len;
        strcpy(arg, buffer[i]);
        strlen(buffer[i], text_len);
        if (text_len >= 8) {
            uint8_t rw, cl;
            get_cursor_loc(rw, cl);
            struct location cursor_loc = {rw, cl};
            print_to_screen("Foldername is too long", cursor_loc, SHELL_COMMAND_COLOR);
            cursor_loc.row++;
            set_cursor_loc(cursor_loc.row + 1, 0);
            set_cursor_loc(cursor_loc.row + 1, 0);

        } else {
            i++;

            while (TRUE) {
                if (strcmp(buffer[i], "") == 0) {
                    break;
                }

                i++;
            }

            if (i == 2) { // the input is valid
                mkdir(arg);
            } else {
                uint8_t rw, cl;
                get_cursor_loc(rw, cl);
                struct location cursor_loc = {rw, cl};
                print_to_screen("Foldername can't contain spaces", cursor_loc, SHELL_COMMAND_COLOR);
                cursor_loc.row++;
                set_cursor_loc(cursor_loc.row + 1, 0);
                set_cursor_loc(cursor_loc.row + 1, 0);
            }
        }
    } else if (strcmp(cmd, "cp") == 0) {
        set_cursor_loc(rw + 1, 0);

        char file[MAX_COMMAND_LENGTH];
        char dest[MAX_COMMAND_LENGTH];

        strcpy(file, buffer[1]);
        strcpy(dest, buffer[2]);

        cp(file, dest);
    } else if (strcmp(cmd, "rm") == 0) {
        set_cursor_loc(rw + 1, 0);

        char arg[MAX_COMMAND_LENGTH] = {0};
        strcpy(arg, buffer[1]);

        rm(arg);
    } else {
        struct location cursor_loc = {rw + 2, 0};

        print_to_screen("\'", cursor_loc, SHELL_COMMAND_COLOR);
        cursor_loc.col += 1;
        print_to_screen(cmd, cursor_loc, SHELL_COMMAND_COLOR);
        cursor_loc.col += cmd_len;
        print_to_screen("\'", cursor_loc, SHELL_COMMAND_COLOR);
        cursor_loc.col += 2;
        print_to_screen("is not recognized as an internal command", cursor_loc, SHELL_COMMAND_COLOR);


        if (rw + 4 >= 25) {
            clear_screen();
        } else {
            rw += 4;
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


/**
 * @brief     Changes the current directory
 * ---------------------------------------------------------------------------------
*/
void change_dir(char path[256], struct FAT32DirectoryTable dir_table)
{
    uint32_t parent_cluster_number = dir_table.table[0].cluster_high << 16 | dir_table.table[0].cluster_low;
    uint8_t rw, cl;
    get_cursor_loc(rw, cl);
    struct location loc = {rw, cl};

    if (strcmp(path, "..") == 0)
    {
        if (state.working_directory > 2)
        {
            char buffer[MAX_COMMAND_SPLIT][MAX_COMMAND_LENGTH] = {0};
            strsplit(state.dir_string, '/', buffer);

            int8_t count = 0;
            for (int i = 0; i < 16; i++)
            {
                if (buffer[i][0] != 0)
                {
                    count++;
                }
            }

            char res[256] = {0};
            for (int i = 1; i < count; i++)
            {
                if (count != 1)
                {
                    strcat(res, "/");
                }
                strcat(res, buffer[i]);
            }
            uint32_t size = 256;
            strncpy(state.dir_string, res, size);
            state.working_directory = parent_cluster_number;

            // Update access date
            struct ClusterBuffer ress = {0};
            struct FAT32DriverRequest req = {
                .buf = &ress,
                .ext = "\0\0\0",
                .parent_cluster_number = parent_cluster_number,
                .buffer_size = 0,
            };

            strcpy(req.name, dir_table.table[0].name);

            int8_t retcode;
            read_directory(req, retcode);
            if (retcode == 0)
            {
                // success
                ;
            }
        }
    }
    else
    {
        for (int i = 1; i < 64; i++)
        { // not including parent
            if (dir_table.table[i].user_attribute == UATTR_NOT_EMPTY &&
                dir_table.table[i].attribute == ATTR_SUBDIRECTORY)
            { // if subdir

                if (strcmp(dir_table.table[i].name, path) == 0)
                { // if matches
                    strcat(state.dir_string, "/");
                    strcat(state.dir_string, path);

                    uint32_t cluster_number = dir_table.table[i].cluster_high << 16 | dir_table.table[i].cluster_low;

                    state.working_directory = cluster_number;

                    // Update access date
                    struct ClusterBuffer res = {0};
                    struct FAT32DriverRequest req = {
                        .buf = &res,
                        .ext = "\0\0\0",
                        .parent_cluster_number = parent_cluster_number,
                        .buffer_size = 0,
                    };

                    strcpy(req.name, dir_table.table[i].name);

                    int8_t retcode;
                    read_directory(req, retcode);
                    if (retcode == 0)
                    {
                        // success
                        ;
                    }
                    return;
                }
            }
        }
        loc.row++;
        char exception[64] = {0};
        strcat(exception, "cd: ");
        strcat(exception, path);
        strcat(exception, ": No such file or directory");
        print_to_screen(exception, loc, SHELL_COMMAND_COLOR);
        loc.row++;
    }

    rw = loc.row + 1;
    cl = 0;
    set_cursor_loc(rw, cl);
}
/**
 * End of cd
 * ---------------------------------------------------------------------------------
*/

void print_cur_working_dir(struct location loc, struct FAT32DirectoryTable dir_table) {
    int count = 0;
    loc.row++;
    print_to_screen("no   name     type      access_date     modified_date_time     size", loc, SHELL_COMMAND_COLOR);
    loc.row++;
    print_to_screen("========================================================================", loc, 0x3);

    // Iterate through the directory entries and print the names of subdirectories
    for (int i = 1; i < 64; i++)
    { // not including parent
        if (dir_table.table[i].user_attribute == UATTR_NOT_EMPTY)
        {
            count++;
            char dir_name[9] = {0};
            strcat(dir_name, dir_table.table[i].name);

            dir_name[12] = '\0';
            loc.row ++;

            char num[3] = {0};
            int_to_str(count, num);
            strcat(num, ".");
            print_to_screen(num, loc, SHELL_COMMAND_COLOR);

            loc.col += 5;

            print_to_screen(dir_name, loc, SHELL_COMMAND_COLOR);

            loc.col += 9;

            if (dir_table.table[i].attribute == ATTR_SUBDIRECTORY)
            {
                print_to_screen("folder", loc, SHELL_COMMAND_COLOR);
            }
            else
            {
                if (strcmp(dir_table.table[i].ext, "") == 0)
                {
                    print_to_screen("file", loc, SHELL_COMMAND_COLOR);
                }
                else
                {
                    print_to_screen(dir_table.table[i].ext, loc, SHELL_COMMAND_COLOR);
                    loc.col += 4;
                    print_to_screen("file", loc, SHELL_COMMAND_COLOR);
                    loc.col -= 4;
                }
            }

            // Extract day, month, and year from access_date
            char access_date[10] = {0};
            get_date(dir_table.table[i].access_date, access_date);

            loc.col += 10;
            print_to_screen(access_date, loc, SHELL_COMMAND_COLOR);

            // Extract hours and minutes from modified_time
            char modified_time[5] = {0};
            get_time(dir_table.table[i].modified_time, modified_time);

            loc.col += 16;
            print_to_screen(modified_time, loc, SHELL_COMMAND_COLOR);

            // Extract day, month, and year from modified_date

            char modified_date[10] = {0};
            get_date(dir_table.table[i].modified_date, modified_date);

            loc.col += 7;
            print_to_screen(modified_date, loc, SHELL_COMMAND_COLOR);

            loc.col += 16;
            char size[16] = {0};
            int_to_str(dir_table.table[i].filesize, size);
            print_to_screen(size, loc, SHELL_COMMAND_COLOR);

            loc.col += 6;
            print_to_screen("bit", loc, SHELL_COMMAND_COLOR);

            loc.col = 0;
        }
    }

    uint8_t rw, cl;
    rw = loc.row + 2;
    cl = 0;
    set_cursor_loc(rw, cl);
}
/**
 * End of ls section
 * ---------------------------------------------------------------------------------
*/



/**
 * @brief     Display the content of a file
 * ---------------------------------------------------------------------------------
*/
void cat(char arg[256]) {
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
    strsplit(arg, '.', split_filename);

    memcpy(req.name, split_filename[0], 8);
    memcpy(req.ext, split_filename[1], 3);

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
        strcat(msg, arg);
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
        strcat(msg, arg);
        strcat(msg, "\' not found");

        print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
        break;
    default:
        break;
    }

    set_cursor_loc(rw + 2, 0);
}
/**
 * End of cat section
 * ---------------------------------------------------------------------------------
*/



/**
 * @brief     Removes a file from the current working directory
 * ---------------------------------------------------------------------------------
*/
void rm(char arg[256]) {
    uint8_t rw, cl;
    get_cursor_loc(rw, cl);
    struct location cursor_loc = {rw, cl};

    struct ClusterBuffer res = {0};
    struct FAT32DriverRequest req = {
        .buf                   = &res,
        .name                  = "\0\0\0\0",
        .ext                   = "\0\0\0",
        .parent_cluster_number = state.working_directory,
        .buffer_size           = 512,
    };

    char split_filename[MAX_COMMAND_SPLIT][MAX_COMMAND_LENGTH] = {0};
    strsplit(arg, '.', split_filename);

    memcpy(req.name, split_filename[0], 8);
    memcpy(req.ext, split_filename[1], 3);

    if (req.ext[0] == '\0') {
        req.buffer_size = 0;
    }

    uint8_t stat;

    if (req.name[0] == '\0') {
        stat = 1;
    } else {
        delete(req, stat);
    }

    char msg[256] = {0};

    switch (stat)
    {
    case 0:
        /* success */
        strcat(msg, "File/folder \'");
        strcat(msg, arg);
        strcat(msg, "\' deleted");
    
        print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
        break;
    case 1:
        /* File not found */
        strcat(msg, "File \'");
        strcat(msg, arg);
        strcat(msg, "\' not found");

        print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
        break;
    case 2:
        /* Folder is not empty */
        strcat(msg, "Folder \'");
        strcat(msg, arg);
        strcat(msg, "\' is not empty. Can't delete directly.");

        print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
        break;
    default:
        break;
    }

    set_cursor_loc(rw + 2, 0);
}
/**
 * End of rm section
 * ---------------------------------------------------------------------------------
*/



/**
 * @brief Move file/folder from src to dest
 * ---------------------------------------------------------------------------------
*/
void mv(char src[256], char dest[256]) {
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

    /* Looking up the src file/folder ------------------- */
    char src_split[MAX_COMMAND_SPLIT][MAX_COMMAND_LENGTH] = {0};
    strsplit(src, '/', src_split);

    uint8_t i = 0;
    int8_t stat;
    int8_t entry_idx = 0;

    uint32_t prev_dir = state.working_directory;
    uint32_t dir = state.working_directory;
    struct FAT32DirectoryEntry entry;

    while (src_split[i][0] != '\0' && i < MAX_COMMAND_SPLIT) {
        char split_filename[MAX_COMMAND_SPLIT][MAX_COMMAND_LENGTH] = {0};
        strsplit(src_split[i], '.', split_filename);

        memcpy(req.name, split_filename[0], 8);
        memcpy(req.ext, split_filename[1], 3);

        dirtable_linear_search(dir, req, entry_idx);

        if (entry_idx == -1) { // file/folder not found
            char msg[256] = {0};
            strcat(msg, "File/folder \'");
            strcat(msg, src);
            strcat(msg, "\' not found");

            print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
            cursor_loc.row++;
            set_cursor_loc(cursor_loc.row + 1, 0);
            return;
        } else {
            struct FAT32DirectoryTable table_dir;
            get_cur_working_dir(dir, &table_dir);
            entry = table_dir.table[entry_idx];

            if (entry.attribute == ATTR_SUBDIRECTORY) { // the entry is a folder
                req.parent_cluster_number = entry.cluster_high << 16 | entry.cluster_low;
                prev_dir = dir;
                dir = req.parent_cluster_number;
            } else { // file to be moved found
                break;
            }
        }

        i++;
    }
    /* Looking up the src file/folder ------------------- */


    /* Looking up the target file/folder ------------------- */
    struct ClusterBuffer target_res = {0};
    struct FAT32DriverRequest target_req = {
        .buf                   = &target_res,
        .name                  = "\0\0\0\0",
        .ext                   = "\0\0\0",
        .parent_cluster_number = state.working_directory,
        .buffer_size           = CLUSTER_SIZE,
    };

    uint32_t target_dir = state.working_directory;
    struct FAT32DirectoryEntry target_entry;

    char dest_split[MAX_COMMAND_SPLIT][MAX_COMMAND_LENGTH] = {0};
    strsplit(dest, '/', dest_split);
    

    i = 0;
    while (dest_split[i][0] != '\0' && i < MAX_COMMAND_SPLIT) {
        char split_filename[MAX_COMMAND_SPLIT][MAX_COMMAND_LENGTH] = {0};
        strsplit(dest_split[i], '.', split_filename);

        memcpy(target_req.name, split_filename[0], 8);
        memcpy(target_req.ext, split_filename[1], 3);

        dirtable_linear_search(target_dir, target_req, entry_idx);

        if (entry_idx == -1) { // file/folder not found
            break; // this is the requested new place
        } else {
            struct FAT32DirectoryTable table_dir;
            get_cur_working_dir(target_dir, &table_dir);
            target_entry = table_dir.table[entry_idx];

            if (target_entry.attribute == ATTR_SUBDIRECTORY) { // the entry is a folder
                target_req.parent_cluster_number = target_entry.cluster_high << 16 | target_entry.cluster_low;
                target_dir = target_req.parent_cluster_number;
            } else { // can't overwrite file
                char msg[256] = {0};
                strcat(msg, "File/folder \'");
                strcat(msg, dest);
                strcat(msg, "\' already exists");

                print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
                cursor_loc.row++;
                set_cursor_loc(cursor_loc.row + 1, 0);
                return;
            }
        }

        i++;
    }
    /* Looking up the target file/folder ------------------- */


    /* Move/rename file/folder according to src and target ------------ */
    if (entry.attribute == ATTR_SUBDIRECTORY) { // the move request is to a folder
        req.parent_cluster_number = prev_dir;


        delete(req, stat);
        if (stat == 2) {
            char msg[256] = {0};
            strcat(msg, "Folder \'");
            strcat(msg, src);
            strcat(msg, "\' is not empty. Can't move or rename directly.");

            print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
            cursor_loc.row++;
            set_cursor_loc(cursor_loc.row + 1, 0);
            return;
        }

        target_req.buffer_size = 0; //directory buffer size is 0

        if (prev_dir == target_dir) {
            // rename only
            write_file(target_req, stat);
            if (stat == 0) {
                char msg[256] = {0};
                strcat(msg, "Folder \'");
                strcat(msg, src);
                strcat(msg, "\' renamed to \'");
                strcat(msg, dest);
                strcat(msg, "\'");

                print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
                cursor_loc.row++;
                set_cursor_loc(cursor_loc.row + 1, 0);
            } else if (stat == 2) {
                char msg[256] = {0};
                strcat(msg, "Folder \'");
                strcat(msg, dest);
                strcat(msg, "\' already exists");

                print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
                cursor_loc.row++;
                set_cursor_loc(cursor_loc.row + 1, 0);
            }
        } else {
            memcpy(target_req.name, req.name, 8);
            memcpy(target_req.ext, req.ext, 3);
            write_file(target_req, stat);

            if (stat == 0) {
                char msg[256] = {0};
                strcat(msg, "Folder \'");
                strcat(msg, src);
                strcat(msg, "\' moved to \'");
                strcat(msg, dest);
                strcat(msg, "\'");

                print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
                cursor_loc.row++;
                set_cursor_loc(cursor_loc.row + 1, 0);
            } else if (stat == 2) {
                char msg[256] = {0};
                strcat(msg, "Folder \'");
                strcat(msg, dest);
                strcat(msg, "\' already exists");

                print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
                cursor_loc.row++;
                set_cursor_loc(cursor_loc.row + 1, 0);
            }
        }

        
    } else {
        // find the proper buffer_size

        read_file(req, stat);

        if (stat == 2) {
            char msg[256] = {0};
            strcat(msg, "File \'");
            strcat(msg, src);
            strcat(msg, "\' is too big.");

            print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
            cursor_loc.row++;
            set_cursor_loc(cursor_loc.row + 1, 0);
            return;
        }

        uint32_t properSize;
        strlen(req.buf, properSize);
        target_req.buffer_size = properSize;
        memcpy(target_req.buf, req.buf, 512);

        if (target_req.ext[0] != '\0') {
            // request for rename
            write_file(target_req, stat);
            if (stat == 0) {
                char msg[256] = {0};
                strcat(msg, "File \'");
                strcat(msg, src);
                strcat(msg, "\' renamed to \'");
                strcat(msg, dest);
                strcat(msg, "\'");

                print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
                cursor_loc.row++;
                set_cursor_loc(cursor_loc.row + 1, 0);
                delete(req, stat);
            }

        } else {
            // request for move
            memcpy(target_req.name, req.name, 8);
            memcpy(target_req.ext, req.ext, 3);
            
            write_file(target_req, stat);
            if (stat == 0) {
                char msg[256] = {0};
                strcat(msg, "File \'");
                strcat(msg, src);
                strcat(msg, "\' moved to \'");
                strcat(msg, dest);
                strcat(msg, "\'");

                print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
                cursor_loc.row++;
                set_cursor_loc(cursor_loc.row + 1, 0);
                delete(req, stat);
            } else {
                char msg[256] = {0};
                strcat(msg, "File \'");
                strcat(msg, src);
                strcat(msg, "\' failed to be moved to \'");
                strcat(msg, dest);
                strcat(msg, "\'");

                print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
                cursor_loc.row++;
                set_cursor_loc(cursor_loc.row + 1, 0);
            }
        }
    }
    /* Move/rename file/folder according to src and target ------------ */

    set_cursor_loc(cursor_loc.row + 1, 0);
}
/**
 * End of mv section
 * ---------------------------------------------------------------------------------
*/



/**
 * @brief Looks up for location of a file/folder
 * ---------------------------------------------------------------------------------
*/
void whereis(uint32_t cluster_number, char arg[256], char paths[16][256], uint8_t * paths_count) {
    struct FAT32DirectoryTable dirtable; 
    get_cur_working_dir(cluster_number, &dirtable);

    char arg_split[MAX_COMMAND_SPLIT][MAX_COMMAND_LENGTH] = {0};
    strsplit(arg, '.', arg_split);

    char name[8] = {0}; memcpy(name, arg_split[0], 8);
    char ext[3] = {0}; memcpy(ext, arg_split[1], 3);

    for (int i = 1; i < 64; i++) { // not including parent
        if (dirtable.table[i].user_attribute == UATTR_NOT_EMPTY) {
            char file[13] = {0};

            strcat(file, name);
            if (ext[0] != '\0')  {
                strcat(file, ".\0");

                uint8_t file_len = 0;
                strlen(file, file_len);

                memcpy(file + file_len, ext, 3);
            }
            char entry_name[8]; memcpy(entry_name, dirtable.table[i].name, 8);
            char entry_ext[3]; memcpy(entry_ext, dirtable.table[i].ext, 3);
            
            memcpy(name, arg_split[0], 8);
            memcpy(ext, arg_split[1], 3);

            if ((strcmp(name, entry_name) == 0) && (strcmp(ext, entry_ext)) == 0) {
                // found the file/folder
                uint8_t filename_len; strlen(file, filename_len);
                memcpy(paths[*paths_count], file, filename_len);
                (*paths_count)++;
            }

            if (dirtable.table[i].attribute == ATTR_SUBDIRECTORY) {
                char directory_name[256] = {};
                strcat(directory_name, entry_name);
                strcat(directory_name, "/");

                uint32_t cluster_number = dirtable.table[i].cluster_high << 16 | dirtable.table[i].cluster_low;

                uint8_t prev_paths_count = *paths_count;
                whereis(cluster_number, arg, paths, paths_count);

                if (prev_paths_count == *paths_count) {
                    continue;
                } else {
                    for (int i = prev_paths_count; i < *paths_count; i++) {
                        strcat(directory_name, paths[i]);
                        memcpy(paths[i], directory_name, 256);
                    }
                }
            }
        } 
    }
}
/**
 * End of whereis section
 * ---------------------------------------------------------------------------------
*/



/**
 * @brief Make an empty folder in the active directory
 * ---------------------------------------------------------------------------------
*/
void mkdir(char arg[256]) {
    uint8_t rw, cl;
    get_cursor_loc(rw, cl);
    struct location cursor_loc = {rw, cl};

    struct ClusterBuffer res = {0};
    struct FAT32DriverRequest req = {
        .buf                   = &res,
        .name                  = "\0\0\0\0",
        .ext                   = "\0\0\0",
        .parent_cluster_number = state.working_directory,
        .buffer_size           = 0,
    };

    uint32_t properSize;
    strlen(arg, properSize);
    strncpy(req.name, arg, properSize);

    // check if the name is not an empty string
    if (strcmp(arg, "") == 0) {
        print_to_screen("Folder name can't be empty", cursor_loc, SHELL_COMMAND_COLOR);
        cursor_loc.row++;
        set_cursor_loc(cursor_loc.row + 1, 0);
        return;
    }

    // check whether the folder already exists
    uint8_t stat;
    write_file(req, stat);

    char msg[256] = {0};
    switch (stat)
    {
    case 0:
        /* folder doesn't exist, thus make folder */
        char temp[8];
        strncpy(temp, arg, 8);
        strcat(msg, "Folder \'");
        strcat(msg, temp);
        strcat(msg, "\' has been made");

        print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
        cursor_loc.row++;
        set_cursor_loc(cursor_loc.row + 1, 0);
        break;
    case 1:
        /* folder already exists */
        print_to_screen("A folder with the same name already exists", cursor_loc, SHELL_COMMAND_COLOR);
        cursor_loc.row++;
        set_cursor_loc(cursor_loc.row + 1, 0);
        break;
    default:
        /* invalid when writing */
        print_to_screen("Invalid", cursor_loc, SHELL_COMMAND_COLOR);
        cursor_loc.row++;
        set_cursor_loc(cursor_loc.row + 1, 0);
        break;
    }

    set_cursor_loc(cursor_loc.row + 1, 0);
}
/**
 * End of mkdir section
 * ---------------------------------------------------------------------------------
*/



/**
 * @brief Copy a file from the active directory to another destination directory
 * ---------------------------------------------------------------------------------
*/
void cp(char src[256], char dest[256]) {
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

    // look for the src file/folder
    char src_split[MAX_COMMAND_SPLIT][MAX_COMMAND_LENGTH] = {0};
    strsplit(src, '/', src_split);

    uint8_t i = 0;
    int8_t stat;
    int8_t entry_idx = 0;

    uint32_t prev_dir = state.working_directory;
    uint32_t dir = state.working_directory;
    struct FAT32DirectoryEntry entry;

    while (src_split[i][0] != '\0' && i < MAX_COMMAND_SPLIT) {
        char split_filename[MAX_COMMAND_SPLIT][MAX_COMMAND_LENGTH] = {0};
        strsplit(src_split[i], '.', split_filename);

        memcpy(req.name, split_filename[0], 8);
        memcpy(req.ext, split_filename[1], 3);

        dirtable_linear_search(dir, req, entry_idx);

        if (entry_idx == -1) { // file/folder not found
            char msg[256] = {0};
            strcat(msg, "File/folder \'");
            strcat(msg, src);
            strcat(msg, "\' not found");

            print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
            cursor_loc.row++;
            set_cursor_loc(cursor_loc.row + 1, 0);
            return;
        } else {
            struct FAT32DirectoryTable table_dir;
            get_cur_working_dir(dir, &table_dir);
            entry = table_dir.table[entry_idx];

            if (entry.attribute == ATTR_SUBDIRECTORY) { // the entry is a folder
                req.parent_cluster_number = entry.cluster_high << 16 | entry.cluster_low;
                prev_dir = dir;
                dir = req.parent_cluster_number;
            } else { // file to be copied found
                break;
            }
        }

        i++;
    }

    // look for the target file/folder
    struct ClusterBuffer target_res = {0};
    struct FAT32DriverRequest target_req = {
        .buf                   = &target_res,
        .name                  = "\0\0\0\0",
        .ext                   = "\0\0\0",
        .parent_cluster_number = state.working_directory,
        .buffer_size           = CLUSTER_SIZE,
    };

    uint32_t target_dir = state.working_directory;
    struct FAT32DirectoryEntry target_entry;

    char dest_split[MAX_COMMAND_SPLIT][MAX_COMMAND_LENGTH] = {0};
    strsplit(dest, '/', dest_split);
    
    i = 0;
    while (dest_split[i][0] != '\0' && i < MAX_COMMAND_SPLIT) {
        char split_filename[MAX_COMMAND_SPLIT][MAX_COMMAND_LENGTH] = {0};
        strsplit(dest_split[i], '.', split_filename);

        memcpy(target_req.name, split_filename[0], 8);
        memcpy(target_req.ext, split_filename[1], 3);

        dirtable_linear_search(target_dir, target_req, entry_idx);

        if (entry_idx == -1) { // file/folder not found
            break;
        } else {
            struct FAT32DirectoryTable table_dir;
            get_cur_working_dir(target_dir, &table_dir);
            target_entry = table_dir.table[entry_idx];

            cursor_loc.row++;
            set_cursor_loc(cursor_loc.row + 1, 0);

            if (target_entry.attribute == ATTR_SUBDIRECTORY) { // the entry is a folder
                target_req.parent_cluster_number = target_entry.cluster_high << 16 | target_entry.cluster_low;
                target_dir = target_req.parent_cluster_number;
            } else { // can't overwrite file
                char msg[256] = {0};
                strcat(msg, "File/folder \'");
                strcat(msg, dest);
                strcat(msg, "\' already exists");

                print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
                cursor_loc.row++;
                set_cursor_loc(cursor_loc.row + 1, 0);
                return;
            }
        }

        i++;
    }

    // copy the source to target
    if (entry.attribute == ATTR_SUBDIRECTORY) { // copy to a folder
        req.parent_cluster_number = prev_dir;
        char msg[256] = {0};
        strcat(msg, "\'");
        strcat(msg, src);
        strcat(msg, "\' is a folder. Can't copy directly.");

        print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
        cursor_loc.row++;
        set_cursor_loc(cursor_loc.row + 1, 0);
        return; 
    } else {
        // find the proper buffer_size

        read_file(req, stat);

        if (stat == 2) {
            char msg[256] = {0};
            strcat(msg, "File \'");
            strcat(msg, src);
            strcat(msg, "\' is too big.");

            print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
            cursor_loc.row++;
            set_cursor_loc(cursor_loc.row + 1, 0);
            return;
        }

        uint32_t properSize;
        strlen(req.buf, properSize);
        target_req.buffer_size = properSize;
        memcpy(target_req.buf, req.buf, 512);

        if (target_req.ext[0] != '\0') { // copy to another file
            write_file(target_req, stat);
            if (stat == 0) {
                char msg[256] = {0};
                strcat(msg, "File \'");
                strcat(msg, src);
                strcat(msg, "\' copied to file \'");
                strcat(msg, dest);
                strcat(msg, "\'");

                print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
                cursor_loc.row++;
                set_cursor_loc(cursor_loc.row + 1, 0);
            }

        } else { // copy to another directory
            memcpy(target_req.name, req.name, 8);
            memcpy(target_req.ext, req.ext, 3);
            
            write_file(target_req, stat);
            if (stat == 0) {
                char msg[256] = {0};
                strcat(msg, "File \'");
                strcat(msg, src);
                strcat(msg, "\' copied to folder \'");
                strcat(msg, dest);
                strcat(msg, "\'");

                print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
                cursor_loc.row++;
                set_cursor_loc(cursor_loc.row + 1, 0);
            } else {
                char msg[256] = {0};
                strcat(msg, "File \'");
                strcat(msg, src);
                strcat(msg, "\' failed to be copied to folder \'");
                strcat(msg, dest);
                strcat(msg, "\'");

                print_to_screen(msg, cursor_loc, SHELL_COMMAND_COLOR);
                cursor_loc.row++;
                set_cursor_loc(cursor_loc.row + 1, 0);
            }
        }
    }

    set_cursor_loc(cursor_loc.row + 1, 0);
}
/**
 * End of cp section
 * ---------------------------------------------------------------------------------
*/


/* Utils */
void int_to_str(int num, char str[]) {
    int i = 0, sign = 0;
    if (num < 0) {
        sign = 1;
        num = -num;
    }
    do {
        str[i++] = num % 10 + '0';
        num /= 10;
    } while (num > 0);
    if (sign) {
        str[i++] = '-';
    }
    str[i] = '\0';
    for (int j = 0; j < i / 2; j++) {
        char temp = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = temp;
    }
}

void get_date(uint16_t date, char *date_str) {
    // Extract day, month, and year from date
    uint8_t day = date & 0x1F;
    uint8_t month = (date >> 5) & 0x0F;
    uint16_t year = ((date >> 9) & 0x7F) + 2000;

    // Convert day, month, and year to strings
    char day_str[3], month_str[3], year_str[5];
    day_str[0] = '0' + (day / 10); // First digit of day
    day_str[1] = '0' + (day % 10); // Second digit of day
    day_str[2] = '\0';             // Null terminator for string

    month_str[0] = '0' + (month / 10); // First digit of month
    month_str[1] = '0' + (month % 10); // Second digit of month
    month_str[2] = '\0';               // Null terminator for string

    year_str[0] = '0' + (year / 1000);       // Thousands digit of year
    year_str[1] = '0' + ((year / 100) % 10); // Hundreds digit of year
    year_str[2] = '0' + ((year / 10) % 10);  // Tens digit of year
    year_str[3] = '0' + (year % 10);         // Ones digit of year
    year_str[4] = '\0';                      // Null terminator for string

    strcat(date_str, day_str);
    strcat(date_str, "/");
    strcat(date_str, month_str);
    strcat(date_str, "/");
    strcat(date_str, year_str);
}

void get_time(uint16_t time, char*time_str) {
    uint8_t minutes = time & 0x00FF;
    uint8_t hours = (time & 0xFF00) >> 8;

    // Convert hours and minutes to strings
    char hour_str[3], min_str[3];
    hour_str[0] = '0' + (hours / 10);  // First digit of hours
    hour_str[1] = '0' + (hours % 10);  // Second digit of hours
    hour_str[2] = '\0';                // Null terminator for string
    min_str[0] = '0' + (minutes / 10); // First digit of minutes
    min_str[1] = '0' + (minutes % 10); // Second digit of minutes
    min_str[2] = '\0';                 // Null terminator for string

    strcat(time_str, hour_str);
    strcat(time_str, ":");
    strcat(time_str, min_str);
}

void amongus(struct location loc) {
    loc.row = 3;
    loc.col = 8;

    print_block(loc, 7, 0xF); loc.row ++;
    loc.col = 7; print_block(loc, 2, 0xF); loc.col = 9; print_block(loc, 7, 0x4); loc.col = 15; print_block(loc, 1, 0xF); loc.row ++;
    loc.col = 7; print_block(loc, 2, 0xF); loc.col = 9; print_block(loc, 3, 0x4); loc.col = 12; print_block(loc, 5, 0xF); loc.row ++;
    loc.col = 6; print_block(loc, 2, 0xF); loc.col = 8; print_block(loc, 3, 0x4); loc.col = 11; print_block(loc, 1, 0xF); loc.col = 12; print_block(loc, 4, 0x3); loc.col = 16; print_block(loc, 1, 0xF); loc.row ++;
    loc.col = 5; print_block(loc, 3, 0xF); loc.col = 8; print_block(loc, 3, 0x4); loc.col = 11; print_block(loc, 1, 0xF); loc.col = 12; print_block(loc, 4, 0x3); loc.col = 16; print_block(loc, 1, 0xF); loc.row ++;
    loc.col = 5; print_block(loc, 1, 0xF); loc.col = 6; print_block(loc, 1, 0x4); loc.col = 7; print_block(loc, 1, 0xF); loc.col = 8; print_block(loc, 4, 0x4); loc.col = 12; print_block(loc, 4, 0xF); loc.row ++;
    loc.col = 5; print_block(loc, 1, 0xF); loc.col = 6; print_block(loc, 1, 0x4); loc.col = 7; print_block(loc, 1, 0xF); loc.col = 8; print_block(loc, 7, 0x4); loc.col = 15; print_block(loc, 1, 0xF); loc.row ++;
    loc.col = 5; print_block(loc, 1, 0xF); loc.col = 6; print_block(loc, 1, 0x4); loc.col = 7; print_block(loc, 1, 0xF); loc.col = 8; print_block(loc, 7, 0x4); loc.col = 15; print_block(loc, 1, 0xF); loc.row ++;
    loc.col = 5; print_block(loc, 3, 0xF); loc.col = 8; print_block(loc, 7, 0x4); loc.col = 15; print_block(loc, 1, 0xF); loc.row ++;
    loc.col = 7; print_block(loc, 1, 0xF); loc.col = 8; print_block(loc, 3, 0x4); loc.col = 11; print_block(loc, 1, 0xF); loc.col = 12; print_block(loc, 3, 0x4); loc.col = 15; print_block(loc, 1, 0xF); loc.row ++;
    loc.col = 7; print_block(loc, 2, 0xF); loc.col = 9; print_block(loc, 2, 0x4); loc.col = 11; print_block(loc, 1, 0xF); loc.col = 12; print_block(loc, 3, 0x4); loc.col = 15; print_block(loc, 1, 0xF); loc.row ++;
    loc.col = 8; print_block(loc, 3, 0xF); loc.col = 12; print_block(loc, 3, 0xF); loc.row ++;

    loc.row -= 10;
    loc.col = 20;

    delay(1000000);

    print_to_screen("  ___ ", loc, 0xF); loc.row ++;
    print_to_screen(" / __|", loc, 0xF); loc.row ++;
    print_to_screen("| (__ ", loc, 0xF); loc.row ++;
    print_to_screen(" \\___|", loc, 0xF); loc.row ++;

    loc.row -= 4;
    loc.col += 7;
    delay(1000000);

    print_to_screen("  __ _ ", loc, 0xF); loc.row ++;
    print_to_screen(" / _` |", loc, 0xF); loc.row ++;
    print_to_screen("| (_| |", loc, 0xF); loc.row ++;
    print_to_screen(" \\__,_|", loc, 0xF); loc.row ++;

    loc.row -= 4;
    loc.col += 8;
    delay(1000000);

    print_to_screen(" _ __  ", loc, 0xF); loc.row ++;
    print_to_screen("| '_ \\ ", loc, 0xF); loc.row ++;
    print_to_screen("| |_) |", loc, 0xF); loc.row ++;
    print_to_screen("| .__/ ", loc, 0xF); loc.row ++;
    print_to_screen("|_|    ", loc, 0xF); loc.row ++;

    loc.row -= 5;
    loc.col += 8;
    delay(1000000);

    print_to_screen("  ___ ", loc, 0xF); loc.row ++;
    print_to_screen(" / _ \\", loc, 0xF); loc.row ++;
    print_to_screen("|  __/", loc, 0xF); loc.row ++;
    print_to_screen(" \\___|", loc, 0xF); loc.row ++;

    loc.row -= 5;
    loc.col += 7;
    delay(1000000);

    print_to_screen(" _    ", loc, 0xF); loc.row ++;
    print_to_screen("| | __", loc, 0xF); loc.row ++;
    print_to_screen("| |/ /", loc, 0xF); loc.row ++;
    print_to_screen("|   < ", loc, 0xF); loc.row ++;
    print_to_screen("|_|\\_\\", loc, 0xF); loc.row ++;
    delay(1000000);
}

void pikachu(struct location loc) {
    loc.row = 3;
    loc.col = 4;

    print_to_screen("       ,___          .-;'", loc, 0xE); loc.row ++;
    delay(1000000);

    print_to_screen("       `\"-.`\\_...._/`.`", loc, 0xE); loc.row ++;
    delay(1000000);

    print_to_screen("    ,      \\        /", loc, 0xE); loc.row ++;
    delay(1000000);

    print_to_screen(" .-' ',    / ()   ()\\", loc, 0xE); loc.row ++;
    delay(1000000);

    print_to_screen("`'._   \\  /()    .  (|", loc, 0xE); loc.row ++;
    delay(1000000);

    print_to_screen("   / <   |;,     __.;", loc, 0xE); loc.row ++;
    delay(1000000);

    print_to_screen("   '-.'-.|  , \\    , \\", loc, 0xE); loc.row ++;
    delay(1000000);

    print_to_screen("      `>.|;, \\_)    \\_)", loc, 0xE); loc.row ++;
    delay(1000000);

    print_to_screen("       `-;     ,    /", loc, 0xE); loc.row ++;
    delay(1000000);

    print_to_screen("          \\    /   <", loc, 0xE); loc.row ++;
    delay(1000000);

    print_to_screen("           '. <`'-,_)", loc, 0xE); loc.row ++;
    delay(1000000);

    print_to_screen("            '._)", loc, 0xE); loc.row ++;
    delay(1000000);
}

void delay(uint32_t constant) {
    uint32_t currentTick = 0;
    uint32_t cachedTick = 0;
    get_cur_tick(currentTick);
    cachedTick = currentTick + constant;

    while (currentTick < cachedTick)
    {
        get_cur_tick(currentTick);
    }
}