#include "lib-header/portio.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/gdt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/kernel_loader.h"
#include "lib-header/idt.h"
#include "lib-header/interrupt.h"
#include "lib-header/keyboard.h"
#include "lib-header/fat32.h"
#include "lib-header/paging.h"
#include "lib-header/kernel_loader.h"

void amongus() {
    printString("  _", 5, 20, 0xF);
    printString(" | |", 6, 20, 0xF);
    printString(" | |__   ___ _   _", 7, 20, 0xF);
    printString(" | '_ \\ / _ \\ | | |", 8, 20, 0xF);
    printString(" | | | |  __/ |_| |", 9, 20, 0xF);
    printString(" |_| |_|\\___|\\__, |", 10, 20, 0xF);
    printString("              __/ |", 11, 20, 0xF);
    printString("             |___/", 12, 20, 0xF);

    printBlock(3, 8, 7, 0xF);
    printBlock(4, 7, 2, 0xF); printBlock(4, 9, 7, 0x4); printBlock(4, 15, 1, 0xF);
    printBlock(5, 7, 2, 0xF); printBlock(5, 9, 3, 0x4); printBlock(5, 12, 5, 0xF);
    printBlock(6, 6, 2, 0xF); printBlock(6, 8, 3, 0x4); printBlock(6, 11, 1, 0xF); printBlock(6, 12, 4, 0x3); printBlock(6, 16, 1, 0xF);
    printBlock(7, 5, 3, 0xF); printBlock(7, 8, 3, 0x4); printBlock(7, 11, 1, 0xF); printBlock(7, 12, 4, 0x3); printBlock(7, 16, 1, 0xF);
    printBlock(8, 5, 1, 0xF); printBlock(8, 6, 1, 0x4); printBlock(8, 7, 1, 0xF); printBlock(8, 8, 4, 0x4); printBlock(8, 12, 4, 0xF);
    printBlock(9, 5, 1, 0xF); printBlock(9, 6, 1, 0x4); printBlock(9, 7, 1, 0xF); printBlock(9, 8, 7, 0x4); printBlock(9, 15, 1, 0xF);
    printBlock(10, 5, 1, 0xF); printBlock(10, 6, 1, 0x4); printBlock(10, 7, 1, 0xF); printBlock(10, 8, 7, 0x4); printBlock(10, 15, 1, 0xF);
    printBlock(11, 5, 3, 0xF); printBlock(11, 8, 7, 0x4); printBlock(11, 15, 1, 0xF);
    printBlock(12, 7, 1, 0xF); printBlock(12, 8, 3, 0x4); printBlock(12, 11, 1, 0xF); printBlock(12, 12, 3, 0x4); printBlock(12, 15, 1, 0xF);
    printBlock(13, 7, 2, 0xF); printBlock(13, 9, 2, 0x4); printBlock(13, 11, 1, 0xF); printBlock(13, 12, 3, 0x4); printBlock(13, 15, 1, 0xF);
    printBlock(14, 8, 3, 0xF); printBlock(14, 12, 3, 0xF);
}

// void kernel_setup(void) {
//     enter_protected_mode(&_gdt_gdtr);
//     pic_remap();
//     initialize_idt();
//     activate_keyboard_interrupt();
//     framebuffer_clear();
//     framebuffer_write(1, 1, '>', 0xF, 0x0);
//     framebuffer_set_cursor(1, 3);
//     initialize_filesystem_fat32();
//     keyboard_state_activate();

//     // struct ClusterBuffer cbuf[5];
//     // for (uint32_t i = 0; i < 5; i++)
//     //     for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
//     //         cbuf[i].buf[j] = i + 'a';

//     // struct FAT32DriverRequest request = {
//     //     .buf                   = cbuf,
//     //     .name                  = "ikanaide",
//     //     .ext                   = "uwu",
//     //     .parent_cluster_number = ROOT_CLUSTER_NUMBER,
//     //     .buffer_size           = 0,
//     // } ;

//     // write(request); // Create folder "ikanaide"
//     // memcpy(request.name, "kano1\0\0\0", 8);
//     // write(request); // Create folder "kano1"

//     // // // ini buat yang file di dalam folder
//     // // memcpy(request.name, "ikanaido", 8);
//     // // memcpy(request.ext, "txt", 3);
//     // // request.parent_cluster_number = 3;
//     // // request.buffer_size = 1;

//     // // write(request);

//     // memcpy(request.name, "ikanaide", 8);
//     // memcpy(request.ext, "\0\0\0", 3);

//     // delete(request); // Delete first folder, thus creating hole in FS

//     // memcpy(request.name, "daijoubu", 8);
//     // request.parent_cluster_number = ROOT_CLUSTER_NUMBER;
//     // request.buffer_size = 0;
//     // memcpy(request.ext, "uwu", 3);
//     // request.buffer_size = 5 * CLUSTER_SIZE;
//     // write(request); // Create fragmented file "daijoubu"

//     // struct ClusterBuffer readcbuf;
//     // read_clusters(&readcbuf, ROOT_CLUSTER_NUMBER+1, 1); 
//     // // If read properly, readcbuf should filled with 'a'

//     // request.buffer_size = CLUSTER_SIZE;
//     // read(request);   // Failed read due not enough buffer size
//     // request.buffer_size = 5*CLUSTER_SIZE;
//     // read(request);   // Success read on file "daijoubu"

//     while (TRUE);
// }

// void kernel_setup(void) {
//     enter_protected_mode(&_gdt_gdtr);
//     pic_remap();
//     initialize_idt();
//     activate_keyboard_interrupt();
//     framebuffer_clear();
//     framebuffer_set_cursor(0, 0);
//     initialize_filesystem_fat32();
//     gdt_install_tss();
//     set_tss_register();

//     // Allocate first 4 MiB virtual memory
//     allocate_single_user_page_frame((uint8_t*) 0);

//     // Testing
//     struct FAT32DriverRequest req = {
//         .buf = (uint8_t *)0,
//         .name = "f1\0\0\0\0\0\0",
//         .ext = "\0\0\0",
//         .parent_cluster_number = ROOT_CLUSTER_NUMBER,
//         .buffer_size = 0,
//     };
//     write(req);

//     memcpy(req.name, "f2", 2);
//     write(req);

//     memcpy(req.name, "f3", 2);
//     write(req);

//     // move parent to f1
//     req.parent_cluster_number = 0x8;

//     memcpy(req.name, "f4", 2);
//     write(req);

//     memcpy(req.name, "f5", 2);
//     write(req);

//     // move parent to f5
//     // req.parent_cluster_number = 0xc;

//     // memcpy(req.name, "f6", 2);
//     // write(req);

//     // Write shell into memory (assuming shell is less than 1 MiB)
//     struct FAT32DriverRequest request = {
//         .buf                   = (uint8_t*) 0,
//         .name                  = "shell",
//         .ext                   = "\0\0\0",
//         .parent_cluster_number = ROOT_CLUSTER_NUMBER,
//         .buffer_size           = 0x100000,
//     };
//     read(request);

//     // Set TSS $esp pointer and jump into shell 
//     set_tss_kernel_current_stack();
//     // execute shell
//     framebuffer_set_cursor(1, 0);
//     kernel_execute_user_program((uint8_t*) 0x0);

//     while (TRUE);
// }

void kernel_setup(void) {
    enter_protected_mode(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    initialize_filesystem_fat32();
    gdt_install_tss();
    set_tss_register();

    // Allocate first 4 MiB virtual memory
    allocate_single_user_page_frame((uint8_t*) 0);

    // Testing
    struct FAT32DriverRequest req = {
        .buf = (uint8_t *)0,
        .name = "f1\0\0\0\0\0\0",
        .ext = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size = 0,
    };
    write(req);

    memcpy(req.name, "f2", 2);
    write(req);

    memcpy(req.name, "f3", 2);
    write(req);

    // move parent to f1
    memcpy(req.name, "f1", 2);
    int8_t idx = dirtable_linear_search(ROOT_CLUSTER_NUMBER, req);
    struct FAT32DirectoryTable dirtable; get_curr_working_dir(ROOT_CLUSTER_NUMBER, &dirtable);
    struct FAT32DirectoryEntry f1 = dirtable.table[idx];
    uint32_t f1_cluster_nm = f1.cluster_high << 16 | f1.cluster_low;

    req.parent_cluster_number = f1_cluster_nm;

    memcpy(req.name, "f4", 2);
    write(req);

    memcpy(req.name, "f5", 2);
    write(req);

    // move parent to f5
    // req.parent_cluster_number = 0xc;

    // memcpy(req.name, "f6", 2);
    // write(req);

    // Write shell into memory (assuming shell is less than 1 MiB)
    struct FAT32DriverRequest request = {
        .buf                   = (uint8_t*) 0,
        .name                  = "shell",
        .ext                   = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 0x100000,
    };
    read(request);

    // Set TSS $esp pointer and jump into shell 
    set_tss_kernel_current_stack();
    // execute shell
    framebuffer_set_cursor(1, 0);
    kernel_execute_user_program((uint8_t*) 0x0);

    while (TRUE);
}
