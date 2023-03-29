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

void printString(char *string, uint8_t row, uint8_t col) {
    uint8_t i = 0;
    char c = string[i];

    // Bit    7 6 5 4 3 2 1 0
    // Data   R R R G G G B B

    while (c != '\0')
    {
        framebuffer_write(row, col + i, c, 0xF, 0x0);
        i++;
        c = string[i];
    }
}

void printBlock(uint8_t row, uint8_t col, uint8_t n, uint8_t color)
{
    uint8_t i = col;
    uint8_t count = 0;

    while (count < n) {
        framebuffer_write(row, i + count, ' ', 0xF, color);
        count ++;
    }
}

void amongus() {
    printString("  _", 5, 20);
    printString(" | |", 6, 20);
    printString(" | |__   ___ _   _", 7, 20);
    printString(" | '_ \\ / _ \\ | | |", 8, 20);
    printString(" | | | |  __/ |_| |", 9, 20);
    printString(" |_| |_|\\___|\\__, |", 10, 20);
    printString("              __/ |", 11, 20);
    printString("             |___/", 12, 20);

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

void kernel_setup(void) {
    /* Initialization */
    enter_protected_mode(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    initialize_filesystem_fat32();
    keyboard_state_activate();

    struct ClusterBuffer cbuf[5];
    for (uint32_t i = 0; i < 5; i++)
        for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
            cbuf[i].buf[j] = i + 'a';

    struct FAT32DriverRequest request = {
        .buf                   = cbuf,
        .name                  = "ikanaide",
        .ext                   = "uwu",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 0,
    } ;

    write(request); // Create folder "ikanaide"
    memcpy(request.name, "kano1\0\0\0", 8);
    write(request);  // Create folder "kano1"
    memcpy(request.name, "ikanaide", 8);
    delete(request); // Delete first folder, thus creating hole in FS

    memcpy(request.name, "daijoubu", 8);
    request.buffer_size = 5*CLUSTER_SIZE;
    write(request);  // Create fragmented file "daijoubu"

    struct ClusterBuffer readcbuf;
    read_clusters(&readcbuf, ROOT_CLUSTER_NUMBER+1, 1); 
    // If read properly, readcbuf should filled with 'a'

    request.buffer_size = CLUSTER_SIZE;
    read(request);   // Failed read due not enough buffer size
    request.buffer_size = 5*CLUSTER_SIZE;
    read(request);   // Success read on file "daijoubu"
}