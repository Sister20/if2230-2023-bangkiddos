#include "lib-header/stdtype.h"
#include "lib-header/idt.h"

/**
 * The predefined Interrupt Descriptor Table (IDT) in the system, `interrupt_descriptor_table`.
 * The IDTGate and IDT follows the Intel IA-32 Manual, Volume 3A, Section 6.11.
 * IDT is initialized empty. 
*/

struct InterruptDescriptorTable interrupt_descriptor_table; // init empty

/**
 * The predefined Interrupt Descriptor Table Register (IDTR) in the system, `_idt_idtr`.
 * The IDTR follows the Intel IA-32 Manual, Volume 3A, Section 6.11.
*/

struct IDTR _idt_idtr = {
    .size = sizeof(interrupt_descriptor_table) - 1,
    .address = &interrupt_descriptor_table
};