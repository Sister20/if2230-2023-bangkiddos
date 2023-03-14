#include "lib-header/idt.h"

/**
 * The predefined Interrupt Descriptor Table (IDT) in the system, `interrupt_descriptor_table`.
 * The IDTGate and IDT follows the Intel IA-32 Manual, Volume 3A, Section 6.11.
 * IDT is initialized empty. 
*/

struct InterruptDescriptorTable interrupt_descriptor_table = {}; // init empty

/**
 * The predefined Interrupt Descriptor Table Register (IDTR) in the system, `_idt_idtr`.
 * The IDTR follows the Intel IA-32 Manual, Volume 3A, Section 6.11.
*/

struct IDTR _idt_idtr = {
    .size = sizeof(interrupt_descriptor_table) - 1,
    .address = &interrupt_descriptor_table
};

/**
 * Set IDT with proper values and load with lidt
 * The values are loaded from isr_stub_table in intsetup.s
 */
void initialize_idt(void){
    /* Set all IDT entry with handler from isr_stub_table*/
  for (int i = 0; i < ISR_STUB_TABLE_LIMIT; i++)
  {
    set_interrupt_gate(i, isr_stub_table[i], GDT_KERNEL_CODE_SEGMENT_SELECTOR, 0);
  }

    __asm__ volatile("lidt %0" : : "m"(_idt_idtr));
    __asm__ volatile("sti");
}

/**
 * Set IDTGate with proper interrupt handler values.
 * Will directly edit global IDT variable and set values properly
 */
void set_interrupt_gate(uint8_t int_vector, void *handler_address, uint16_t gdt_seg_selector, uint8_t privilege){
    /* Get the IDTGate pointer from interrupt_descriptor_table */
    struct IDTGate *idt_int_gate = &interrupt_descriptor_table.table[int_vector];

    /* Set all the values accordingly */
    idt_int_gate->offset_low = (uint32_t)handler_address & 0xFFFF;
    idt_int_gate->offset_high = ((uint32_t)handler_address >> 16) & 0xFFFF;
    idt_int_gate->segment = gdt_seg_selector;
    idt_int_gate->_reserved = 0;
    idt_int_gate->_r_bit_1 = INTERRUPT_GATE_R_BIT_1;
    idt_int_gate->_r_bit_2 = INTERRUPT_GATE_R_BIT_2;
    idt_int_gate->gate_32 = 1; 
    idt_int_gate->_r_bit_3 = INTERRUPT_GATE_R_BIT_3;
    idt_int_gate->dpl = privilege;
    idt_int_gate->present = 1;
}

