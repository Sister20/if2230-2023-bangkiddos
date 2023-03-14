#include "lib-header/portio.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/gdt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/kernel_loader.h"
#include "lib-header/idt.h"
#include "lib-header/interrupt.h"

void kernel_setup(void) {
    /* Initialization */
    enter_protected_mode(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    framebuffer_clear();

    /* Write to framebuffer */
    framebuffer_write(3, 9, ' ', 0, 0xF);
    framebuffer_write(3, 14, ' ', 0, 0xF);
    framebuffer_write(5, 9, ' ', 0, 0xF);
    framebuffer_write(6, 10, ' ', 0, 0xF);
    framebuffer_write(6, 11, ' ', 0, 0xF);
    framebuffer_write(6, 12, ' ', 0, 0xF);
    framebuffer_write(6, 13, ' ', 0, 0xF);
    framebuffer_write(5, 14, ' ', 0, 0xF);
    framebuffer_set_cursor(3, 9);

    /* Test interrupt */
    __asm__("int $0x4");
    while (TRUE);
}