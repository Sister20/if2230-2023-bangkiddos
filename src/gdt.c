#include "lib-header/stdtype.h"
#include "lib-header/gdt.h"

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to GDT definition in Intel Manual & OSDev.
 * Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */

struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {
            /* Implementation */
            .segment_low       = 0,
            .segment_high      = 0,

            .base_low          = 0,
            .base_mid          = 0,
            .base_high         = 0,

            .type_bit          = 0,

            .non_system        = 0,
            .dpl               = 0,
            .present_bit       = 0,
            .long_mode         = 0,
            .big               = 0,
            .gran              = 0
        },
        {
            /* Implementation */
            .segment_low = 0xFFFF,
            .segment_high = 0xF,

            .base_low          = 0,
            .base_mid          = 0,
            .base_high         = 0,

            .type_bit          = 0xA,

            .non_system        = 1,
            .dpl               = 0,
            .present_bit       = 1,
            .long_mode         = 0,
            .big               = 1,
            .gran              = 1
        },
        {
            /* Implementation */
            .segment_low       = 0xFFFF,
            .segment_high      = 0xF,

            .base_low          = 0,
            .base_mid          = 0,
            .base_high         = 0,

            .type_bit          = 0x2,

            .non_system        = 1,
            .dpl               = 0,
            .present_bit       = 1,
            .long_mode         = 0,
            .big               = 1,
            .gran              = 1
        },
        {/* TODO: User   Code Descriptor */
            /* Implementation */
            .segment_low       = 0xFFFF,
            .segment_high      = 0xF,

            .base_low          = 0,
            .base_mid          = 0,
            .base_high         = 0,

            .type_bit          = 0xA,

            .non_system        = 1,
            .dpl               = 0x3,
            .present_bit       = 1,
            .long_mode         = 0,
            .big               = 1,
            .gran              = 1
        },
        {/* TODO: User   Data Descriptor */
            /* Implementation */
            .segment_low       = 0xFFFF,
            .segment_high      = 0xF,

            .base_low          = 0,
            .base_mid          = 0,
            .base_high         = 0,

            .type_bit          = 0x2,

            .non_system        = 1,
            .dpl               = 0x3,
            .present_bit       = 1,
            .long_mode         = 0,
            .big               = 1,
            .gran              = 1
        },
        {
            .segment_high      = (sizeof(struct TSSEntry) & (0xF << 16)) >> 16,
            .segment_low       = sizeof(struct TSSEntry),

            .base_high         = 0,
            .base_mid          = 0,
            .base_low          = 0,

            .type_bit          = 0x9,
            
            .non_system        = 0,    // S bit
            .dpl               = 0,    // DPL
            .present_bit       = 1,    // P bit
            .big               = 1,    // D/B bit
            .long_mode         = 0,    // L bit
            .gran              = 0,    // G bit
        }}};

/**
 * _gdt_gdtr, predefined system GDTR.
 * GDT pointed by this variable is already set to point global_descriptor_table above.
 * From: https://wiki.osdev.org/Global_Descriptor_Table, GDTR.size is GDT size minus 1.
 */
struct GDTR _gdt_gdtr = {
    /* Implementation */

    .size = sizeof(global_descriptor_table) - 1,
    .address = &global_descriptor_table
};

void gdt_install_tss(void) {
    uint32_t base = (uint32_t) &_interrupt_tss_entry;
    global_descriptor_table.table[5].base_high = (base & (0xFF << 24)) >> 24;
    global_descriptor_table.table[5].base_mid  = (base & (0xFF << 16)) >> 16;
    global_descriptor_table.table[5].base_low  = base & 0xFFFF;
}
