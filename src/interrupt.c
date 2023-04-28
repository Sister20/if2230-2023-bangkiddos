#include "lib-header/interrupt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/string.h"
#include "lib-header/keyboard.h"
#include "lib-header/fat32.h"

/** 
 * Note for others :
 * in and out are defined in portio.h
 * 
 * Start of PIC Remapping Section 
 */
void io_wait(void) {
    out(0x80, 0);
}

void pic_ack(uint8_t irq) {
    if (irq >= 8)
        out(PIC2_COMMAND, PIC_ACK);
    out(PIC1_COMMAND, PIC_ACK);
}

void pic_remap(void) {
    uint8_t a1, a2;

    // Save masks
    a1 = in(PIC1_DATA); 
    a2 = in(PIC2_DATA);

    // Starts the initialization sequence in cascade mode
    out(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); 
    io_wait();
    out(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out(PIC1_DATA, PIC1_OFFSET); // ICW2: Master PIC vector offset
    io_wait();
    out(PIC2_DATA, PIC2_OFFSET); // ICW2: Slave PIC vector offset
    io_wait();
    out(PIC1_DATA, 0b0100);      // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    io_wait();
    out(PIC2_DATA, 0b0010);      // ICW3: tell Slave PIC its cascade identity (0000 0010)
    io_wait();

    out(PIC1_DATA, ICW4_8086);
    io_wait();
    out(PIC2_DATA, ICW4_8086);
    io_wait();

    // Restore masks
    out(PIC1_DATA, a1);
    out(PIC2_DATA, a2);
}
/* End of PIC Remapping Section */


struct location {
    uint8_t row;
    uint8_t col;
};

/* System Calls */
void syscall(struct CPURegister cpu, __attribute__((unused)) struct InterruptStack info) {

    /** File System syscall */
    if (cpu.eax == 0) {
        struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu.ebx;
        *((int8_t*) cpu.ecx) = read(request);

    /** Keyboard listener */
    } else if (cpu.eax == 4) {
        /**
         * listen to keyboard, return keyboard driver state
         * ebx = ptr to keyboard driver state
        */

        keyboard_state_activate();
        __asm__("sti"); // Due IRQ is disabled when main_interrupt_handler() called
        struct KeyboardDriverState keyboard = get_keyboard_state();
        memcpy((void *) cpu.ebx, (void *) &keyboard, sizeof(struct KeyboardDriverState));
        reset_scancode();
        if (keyboard.last_char == '\b') {
            reset_last_char();
        } else if (keyboard.last_char == '\n') {
            clear_buffer();
        }
    } 

    /** Graphical User Interface syscall */
    else if (cpu.eax == 50) {
        /** 
         * get framebuffer curser location
         * ebx = ptr row
         * ecx = ptr col
        */
        framebuffer_get_cursor((uint8_t *) cpu.ebx,  (uint8_t *) cpu.ecx);
    } else if (cpu.eax == 51) {
        /**
         * set framebuffer cursor location
         * ebx = row
         * ecx = col
        */
        framebuffer_set_cursor(cpu.ebx, cpu.ecx);
    } else if (cpu.eax == 52) {
        /**
         * print string to framebuffer
         * ebx = string (ptr to char)
         * ecx = framebuffer location (has attr row and col)
         * edx = color
        */
        struct location loc = *(struct location *) cpu.ecx;
        printString((char *) cpu.ebx, loc.row, loc.col, cpu.edx);
    } else if (cpu.eax == 53) {
        /**
         * clear framebuffer screen
        */
        framebuffer_clear();
        framebuffer_set_cursor(1, 0);
    }

    /** Commands */
    else if (cpu.eax == 60) {
        /**
         * print current working directory
         * ecx = location (struct location)
         * ebx = buffer (ptr to char)
         * edx = color
         */
        struct FAT32DirectoryTable *dir_table = (struct FAT32DirectoryTable *)cpu.ecx;
        get_curr_working_dir(cpu.ebx, dir_table);
    }

    /** String operation */ 
    else if (cpu.eax == 80) {
        /**
         * strlen
         * ebx = string (ptr to char)
         * ecx = length (ptr to int)
        */
        *((int *) cpu.ecx) = strlen((char *) cpu.ebx);
    } else if (cpu.eax == 81) {
        /**
         * strcmp
         * ebx = string1 (ptr to char)
         * ecx = string2 (ptr to char)
         * edx = result (ptr to int)
        */
        *((int *) cpu.edx) = strcmp((char *) cpu.ebx, (char *) cpu.ecx);
    } else if (cpu.eax == 82) {
        /**
         * strcpy
         * ebx = dest (ptr to char)
         * ecx = src (ptr to char)
        */
        strcpy((char *) cpu.ebx, (char *) cpu.ecx);
    } else if (cpu.eax == 83) {
        /**
         * strcat
         * ebx = dest (ptr to char)
         * ecx = src (ptr to char)
        */
        strcat((char *) cpu.ebx, (char *) cpu.ecx);
    } else if (cpu.eax == 84) {
        /**
         * strset
         * ebx = dest (ptr to char)
         * ecx = char
         * edx = length
        */
        strset((char *) cpu.ebx, (char) cpu.ecx, cpu.edx);
    }

}

void main_interrupt_handler(
    __attribute__((unused)) struct CPURegister cpu,
    uint32_t int_number,
    __attribute__((unused)) struct InterruptStack info
) {
    switch (int_number) {
        case PIC1_OFFSET + IRQ_KEYBOARD:
            keyboard_isr();
            break;
        case 0x30:
            syscall(cpu, info);
            break;
        default:
            break;
    }
}

/* Activate PIC mask for keyboard only */
void activate_keyboard_interrupt(void){
    out(PIC1_DATA, PIC_DISABLE_ALL_MASK ^ (1 << IRQ_KEYBOARD)); //mask irq for keyboard only
    out(PIC2_DATA, PIC_DISABLE_ALL_MASK); //mask irq for slave pic
}

struct TSSEntry _interrupt_tss_entry = {
    // TODO: initialize the fields of the TSS entry as appropriate
    .prev_tss = 0,
    .esp0 = 0,
    .ss0 = GDT_KERNEL_DATA_SEGMENT_SELECTOR,
    .unused_register = {0}
};

void set_tss_kernel_current_stack(void) {
    uint32_t stack_ptr;
    // Reading base stack frame instead esp
    __asm__ volatile ("mov %%ebp, %0": "=r"(stack_ptr) : /* <Empty> */);
    // Add 8 because 4 for ret address and other 4 is for stack_ptr variable
    _interrupt_tss_entry.esp0 = stack_ptr + 8; 
}
